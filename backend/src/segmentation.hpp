#include <vector>

#include "../third_party/cppjieba/Jieba.hpp"

const char *const DICT_PATH = "third_party/cppjieba/dict/jieba.dict.utf8";
const char *const HMM_PATH = "third_party/cppjieba/dict/hmm_model.utf8";
const char *const USER_DICT_PATH = "third_party/cppjieba/dict/user.dict.utf8";
const char *const IDF_PATH = "third_party/cppjieba/dict/idf.utf8";
const char *const STOP_WORD_PATH = "third_party/cppjieba/dict/stop_words.utf8";

using KeywordList = std::vector<cppjieba::KeywordExtractor::Word>;
struct Jieba {
  cppjieba::Jieba jieba;
  Jieba()
      : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {}
  KeywordList Keywords(std::string s) {
    KeywordList keywordres;
    jieba.extractor.Extract(s, keywordres, -1);
    return keywordres;
  }
};