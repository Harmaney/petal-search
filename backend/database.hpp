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
    Node *son[SPLIT_STEP];
    std::list<std::pair<size_t, double>> arts;
  } * root;

  Trie() : root(new Node) {}

  void Insert(std::string word, std::pair<size_t, double> art) {
    Node *pos = root;
    for (int i = 0; i < word.length(); ++i)
      pos = pos->son[word[i]] ? pos->son[word[i]]
                              : (pos->son[word[i]] = new Node);
    pos->arts.push_back(art);
  }

  auto const &Query(std::string word) {
    Node *pos = root;
    for (int i = 0; i < word.length(); ++i) pos = pos->son[word[i]];
    return pos->arts;
  }
};

using Json = nlohmann::json;

struct Database {
  sqlite3 *db;
  Trie tr;
  Jieba jb;
  std::vector<std::string> arts;

  Database() {
    // load from db
  }
  void AddEntry(std::string content) {
    arts.push_back(content);
    auto kws = jb.Keywords(content);
    for (auto kw : kws) tr.Insert(kw.word, {arts.size() - 1, kw.weight});
    // write back
  }

  Json Search(std::string sentence) {
    auto kws = jb.Keywords(sentence);
    std::vector<std::vector<double>> vs(arts.size(),
                                        std::vector<double>(kws.size(), 0));
    for (size_t i = 0; i < kws.size(); ++i)
      for (auto const &[idx, w] : tr.Query(kws[i].word)) vs[idx][i] = w;
  }
} db;
