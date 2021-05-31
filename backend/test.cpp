#include <iostream>

#include "json.hpp"

void F(int x) { std::cerr << "1\n"; }

void F(std::string x) { std::cerr << "2\n"; }

int main() {
  nlohmann::json j;
  j["a"] = 1;
  j["b"] = "2";
  F(j["a"]);
  F(j["b"]);
}