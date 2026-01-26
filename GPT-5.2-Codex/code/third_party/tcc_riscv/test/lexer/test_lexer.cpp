#include "lexer.h"

#include <iostream>

using namespace front_end::lexer;
int main() {
  Lexer test("test.c");
  auto toks = test.lex();
  for (auto t : toks) {
    std::cout << Lexer::strTok(t) << " ";
  }
  std::cout << std::endl;
  return 0;
}
