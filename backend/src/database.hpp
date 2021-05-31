#include <cctype>
#include <filesystem>
#include <list>
#include <random>

#include "../third_party/json.hpp"
#include "../third_party/sqlite_orm.h"
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

  ~Trie() {
    std::function<void(Node*)> Free = [&](Node* x) {
      for (int i = 0; i < SPLIT_STEP; ++i)
        if (x->son[i]) Free(x->son[i]);
      Free(x);
    };
    Free(root);
  }

  void Insert(std::string word, std::pair<size_t, double> art) {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i) {
      assert((uint8_t)word[i] == word[i]);
      uint8_t ch = word[i];
      pos = pos->son[ch] ? pos->son[ch] : (pos->son[ch] = new Node);
    }
    pos->arts.push_back(art);
  }

  std::list<std::pair<size_t, double>> const* Query(std::string word) const {
    Node* pos = root;
    for (int i = 0; i < word.length(); ++i) {
      uint8_t ch = word[i];
      if (pos->son[ch])
        pos = pos->son[ch];
      else
        return nullptr;
    }
    return &pos->arts;
  }
};

using Json = nlohmann::json;

struct ArtRec {
  std::string content;
  double weight;
  std::string keywords;
};

auto database = sqlite_orm::make_storage(
    "db.db", sqlite_orm::make_table(
                 "ARTS", sqlite_orm::make_column("CONTENT", &ArtRec::content),
                 sqlite_orm::make_column("WEIGHT", &ArtRec::weight),
                 sqlite_orm::make_column("KEYWORDS", &ArtRec::keywords)));

struct Engine {
  struct Article {
    std::string content;
    double w;
    bool deleted;
  };

  Trie tr;
  Jieba jb;
  std::vector<Article> arts;
  int deletedCount;

  Engine() { Load(); }

  void Load() {
    auto artRecs = database.get_all<ArtRec>();

    for (auto i : artRecs) {
      std::cerr << "Loading...\n";
      arts.push_back({i.content, i.weight});
      Json jkws = Json::parse((std::string)i.keywords);
      for (auto kw : jkws)
        tr.Insert(kw["word"], {arts.size() - 1, kw["weight"]});
    }
  }

  double GetNorm(KeywordList const& kws) {
    double norm = 0;
    for (auto const& kw : kws) norm += kw.weight * kw.weight;
    return sqrt(norm);
  }

  Json KeywordsToJson(KeywordList const& kws) {
    Json jkws = Json::array();
    for (auto kw : kws)  // 日
      jkws.push_back({{"word", kw.word}, {"weight", kw.weight}});
    return jkws;
  }

  void AddEntry(std::string content) {
    auto kws = jb.Keywords(content);
    auto w = sqrt(GetNorm(kws));
    arts.push_back({content, w});
    Json jkws = KeywordsToJson(kws);

    database.insert((ArtRec){content, w, jkws.dump()});
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
    std::cerr << kws << '\n';
    std::vector<double> norms(arts.size(), 0);
    for (size_t i = 0; i < kws.size(); ++i) {
      auto p = tr.Query(kws[i].word);
      if (p) {
        for (auto const& [id, w] : *p) norms[id] += w * kws[i].weight;
      }
    }
    std::vector<int> rank;
    for (size_t i = 0; i < arts.size(); ++i) {
      norms[i] /= arts[i].w;
      if (!arts[i].deleted && norms[i] > 0) rank.push_back(i);
    }
    std::sort(rank.begin(), rank.end(),
              [&](int a, int b) -> bool { return norms[a] > norms[b]; });
    Json j;
    for (auto i : rank) {
      j.push_back({{"content", arts[i].content}, {"norm", norms[i]}});
      if (j.size() == 20) break;
    }
    return {{"keywords", KeywordsToJson(kws)}, {"results", j}};
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
