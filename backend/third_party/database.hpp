#include <cctype>
#include <list>
#include <random>

#include "json.hpp"
#include "segmentation.hpp"
#include "sqlite3.h"

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
  sqlite3* pDB;
  std::map<std::string, std::string> rowType;

  Sqlite(std::string fileName,
         std::initializer_list<std::pair<std::string, std::string>> schema) {
    assert(sqlite3_open(fileName.c_str(), &pDB) == SQLITE_OK);
    for (auto& [t, k] : schema) rowType[k] = t;
  }

  Json Query(std::string table, Json sel) {
    static Json res;
    static auto callback = [](void*, int n, char** v, char** key) -> int {
      Json item;
      for (int i = 0; i < n; ++i) item[key[i]] = v[i];
      res.push_back(item);
      return 0;
    };
    std::string cond = "";
    for (auto& [k, v] : sel.items()) {
      cond += (cond == "" ? "where " : " and ") + k + "=" + v;
    }
    res = Json::array();
    char* err;
    if (sqlite3_exec(pDB, ("select * from " + table + " " + cond + ';').c_str(),
                     callback, nullptr, &err) != SQLITE_OK)
      std::cerr << err << '\n';
    return res;
  }

  template <class T>
  std::string ToString(std::string type, T const& v) {
    if (rowType) {
    }
  }

  void Modify(std::string table, Json sel, Json m) {
    std::string cond = "", assign = "";
    for (auto& [k, v] : sel.items()) {
      cond += (cond == "" ? "where " : " and ") + k + "=" + v;
    }
    for (auto& [k, v] : m.items()) {
      assign += (assign == "" ? "" : ",") + k + "=" + ToString(rowType[k], v);
    }
    char* err;
    if (sqlite3_exec(
            pDB,
            ("update " + table + " set " + assign + ' ' + cond + ';').c_str(),
            nullptr, nullptr, &err) != SQLITE_OK)
      std::cerr << err << '\n';
  }

  void Insert(std::string table, Json data) {
    std::string keys = "", values = "";
    for (auto& [k, v] : data.items()) {
      keys += (keys == "" ? "" : ",") + k;
      values += (values == "" ? "" : ",") + ToString(rowType[k], v);
    }
    std::cerr << values << '\n';
    char* err;
    if (sqlite3_exec(
            pDB,
            ("insert into " + table + " (" + keys + ") values(" + values + ");")
                .c_str(),
            nullptr, nullptr, &err) != SQLITE_OK)
      std::cerr << err << '\n';
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

  Engine() : sql("db.db") {}

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
  }
} db;
