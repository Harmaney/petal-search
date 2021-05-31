#include <SQLiteCpp/SQLiteCpp.h>

#include <cctype>
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
  } * root;

  Trie() : root(new Node) {}

  void Insert(std::string word, std::pair<size_t, double> art) {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i)
      pos = pos->son[word[i]] ? pos->son[word[i]]
                              : (pos->son[word[i]] = new Node);
    pos->arts.push_back(art);
  }

  auto const& Query(std::string word) {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i) pos = pos->son[word[i]];
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
    if (schema[rowID[k]].type == "TEXT")
      return v;
    else if (schema[rowID[k]].type == "INTEGER")
      return std::to_string((int)v);
    else if (schema[rowID[k]].type == "REAL")
      return std::to_string((double)v);
    else
      throw;
  }

  Json Query(std::string table, Json sel) {
    std::string cond = "";
    for (auto& [k, v] : sel.items()) {
      cond += (cond == "" ? "where " : " and ") + k + "=" + ToString(k, v);
    }
    SQLite::Statement query(db, "select * from " + table + " " + cond + ';');
    Json res;
    while (query.executeStep()) {
      Json entry;
      for (size_t i = 0; i < schema.size(); ++i)
        if (schema[i].type == "TEXT")
          entry[schema[i].key] = query.getColumn(i);
        else if (schema[i].type == "INTEGER")
          entry[schema[i].key] = (int)query.getColumn(i);
        else if (schema[i].type == "REAL")
          entry[schema[i].key] = (int)query.getColumn(i);
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
    }
    db.exec("insert into " + table + " (" + keys + ") values(" + values + ");");
  }
};

struct Engine {
  struct Article {
    std::string content;
    double w;
  };

  Sqlite sql;
  Trie tr;
  Jieba jb;
  std::vector<Article> arts;

  Engine() : sql("db.db", {{"CONTENT", "TEXT"}, {"WEIGHT", "REAL"}}) {}

  double GetNorm(KeywordList const& kws) {
    double norm = 0;
    for (auto const& kw : kws) norm += kw.weight * kw.weight;
    return sqrt(norm);
  }

  void AddEntry(std::string content) {
    auto kws = jb.Keywords(content);
    arts.push_back({content, sqrt(GetNorm(kws))});
    for (auto kw : kws) tr.Insert(kw.word, {arts.size() - 1, kw.weight});
    // write back
    sql.Insert("DATABASE", "");
  }

  Json Search(std::string sentence) {
    auto kws = jb.Keywords(sentence);
    std::vector<double> norms(arts.size());
    for (size_t i = 0; i < kws.size(); ++i)
      for (auto const& [id, w] : tr.Query(kws[i].word))
        norms[id] += w * kws[i].weight;
    std::vector<int> rank(arts.size());
    for (size_t i = 0; i < arts.size(); ++i) {
      norms[i] /= arts[i].w;
      rank[i] = i;
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
} db;
