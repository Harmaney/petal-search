#include "database.hpp"

int main(int argc, char **argv) {
  Jieba jieba;
  std::cout
      << jieba.Keywords(
             "脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑"
             "瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫"
             "脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑"
             "瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫脑瘫"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障"
             "智障智障智障智障智障智障智障智障智障智障智障智障智障智障智障")
      << '\n';
}
