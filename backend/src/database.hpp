#include <SQLiteCpp/SQLiteCpp.h>

#include <cctype>
#include <filesystem>
#include <list>
#include <random>

#include "../third_party/json.hpp"
#include "segmentation.hpp"

using ArticleID = uint32_t;

const int SPLIT_STEP = 256;

struct Trie {
  struct Node {
    Node* son[SPLIT_STEP];
    std::list<std::pair<size_t, double>> arts;
    Node() : son{} {}
  } * root;

  Trie() : root(new Node) {}

  void Insert(std::string word, std::pair<size_t, double> art) {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i) {
      assert((uint8_t)word[i] == word[i]);
      uint8_t ch = word[i];
      pos = pos->son[ch] ? pos->son[ch] : (pos->son[ch] = new Node);
    }
    pos->arts.push_back(art);
  }

  auto const& Query(std::string word) {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i) pos = pos->son[(uint8_t)word[i]];
    return pos->arts;
  }
};

using Json = nlohmann::json;

struct Sqlite {
  struct TypeDef {
    std::string key, type;
  };
  using Schema = std::vector<TypeDef>;
  Schema schema;
  std::map<std::string, int> rowID;
  SQLite::Database db;

  Sqlite(std::string fileName, Schema const& schema)
      : schema(schema), db("db.db") {
    for (size_t i = 0; i < schema.size(); ++i) rowID[schema[i].key] = i;
  }

  template <class T>
  std::string ToString(std::string k, T const& v) {
    if (v.is_string())
      return v;
    else if (v.is_number_integer())
      return std::to_string((int)v);
    else if (v.is_number_float())
      return std::to_string((double)v);
    else
      throw;
  }

  Json QueryAll(std::string table) {
    std::string cond = "";
    SQLite::Statement query(db, "select * from " + table + " " + cond + ';');
    Json res;
    while (query.executeStep()) {
      Json entry;
      for (size_t i = 0; i < schema.size(); ++i)
        if (query.getColumn(i).isText())
          entry[schema[i].key] = query.getColumn(i);
        else if (query.getColumn(i).isInteger())
          entry[schema[i].key] = (int)query.getColumn(i);
        else if (query.getColumn(i).isFloat())
          entry[schema[i].key] = (double)query.getColumn(i);
        else
          throw;
      res.push_back(entry);
    }
    return res;
  }

  void Modify(std::string table, Json sel, Json m) {
    std::string cond = "", assign = "";
    for (auto& [k, v] : sel.items())
      cond += (cond == "" ? "where " : " and ") + k + "=" +
              ToString(schema[rowID[k]].type, v);
    for (auto& [k, v] : m.items())
      assign += (assign == "" ? "" : ",") + k + "=" +
                ToString(schema[rowID[k]].type, v);
    db.exec("update " + table + " set " + assign + ' ' + cond + ';');
  }

  void Insert(std::string table, Json data) {
    std::string keys = "", values = "";

    for (auto& [k, v] : data.items()) {
      keys += (keys == "" ? "" : ",") + k;
      values += (values == "" ? "" : ",") + ToString(schema[rowID[k]].type, v);
      std::cerr << k << ' ' << v << '\n';
    }

    db.exec("insert into " + table + " (" + keys + ") values(" + values + ");");
  }
};

struct Engine {
  struct Article {
    std::string content;
    double w;
    bool deleted;
  };

  Sqlite sql;
  Trie tr;
  Jieba jb;
  std::vector<Article> arts;
  int deletedCount;

  Engine()
      : sql("db.db",
            {{"CONTENT", "TEXT"}, {"WEIGHT", "REAL"}, {"KEYWORDS", "TEXT"}}) {
    Load();
  }

  void Load() {
    auto j = sql.Query("ARTS", {});
    for (auto i : j) {
      arts.push_back({i["CONTENT"], i["WEIGHT]"]});
      Json jkws = Json::parse((std::string)i["KEYWORDS"]);
      for (auto kw : jkws)
        tr.Insert(kw["WORD"], {arts.size() - 1, kw["WEIGHT"]});
    }
  }

  double GetNorm(KeywordList const& kws) {
    double norm = 0;
    for (auto const& kw : kws) norm += kw.weight * kw.weight;
    return sqrt(norm);
  }

  void AddEntry(std::string content) {
    auto kws = jb.Keywords(content);
    auto w = sqrt(GetNorm(kws));
    arts.push_back({content, w});
    Json jkws = Json::array();
    for (auto kw : kws) {
      tr.Insert(kw.word, {arts.size() - 1, kw.weight});
      jkws.push_back({{"WORD", kw.word}, {"WEIGHT", kw.weight}});
    }

    sql.Insert(
        "ARTS",
        {{"CONTENT", content}, {"WEIGHT", w}, {"KEYWORDS", jkws.dump()}});
  }

  void BatchAddEntry(std::string folder) {
    for (auto const& it :
         std::filesystem::directory_iterator(std::filesystem::path(folder))) {
      std::cerr << "Adding..." << it.path() << '\n';
      std::ifstream ifs(it.path());
      AddEntry(std::string(std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>()));
    }
  }

  Json Search(std::string sentence) {
    auto kws = jb.Keywords(sentence);
    std::vector<double> norms(arts.size());
    for (size_t i = 0; i < kws.size(); ++i)
      for (auto const& [id, w] : tr.Query(kws[i].word))
        norms[id] += w * kws[i].weight;
    std::vector<int> rank;
    for (size_t i = 0; i < arts.size(); ++i) {
      norms[i] /= arts[i].w;
      if (!arts[i].deleted) rank.push_back(i);
    }
    std::sort(rank.begin(), rank.end(),
              [&](int a, int b) -> bool { return norms[a] > norms[b]; });
    Json j;
    for (auto i : rank) {
      j.push_back(arts[i].content);
      std::cerr << arts[i].content << ' ' << arts[i].w << '\n';
    }
    return j;
  }

  void Delete(size_t id) {
    arts[id].deleted = true;
    if (++deletedCount == 1000) {
      arts = {};
      tr.~Trie();
      Load();
    }
  }
} db;
