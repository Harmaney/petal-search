#include <vector>

#include "cppjieba/Jieba.hpp"

const char *const DICT_PATH = "cppjieba/dict/jieba.dict.utf8";
const char *const HMM_PATH = "cppjieba/dict/hmm_model.utf8";
const char *const USER_DICT_PATH = "cppjieba/dict/user.dict.utf8";
const char *const IDF_PATH = "cppjieba/dict/idf.utf8";
const char *const STOP_WORD_PATH = "cppjieba/dict/stop_words.utf8";

struct Jieba {
  cppjieba::Jieba jieba;
  Jieba()
      : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {}
  std::vector<cppjieba::KeywordExtractor::Word> Keywords(std::string s) {
    std::vector<cppjieba::KeywordExtractor::Word> keywordres;
    jieba.extractor.Extract(s, keywordres, -1);
    return keywordres;
  }
};