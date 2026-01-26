#ifndef __LEXER_H__
#define __LEXER_H__
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <string>

namespace front_end {
namespace lexer {
enum class Token {
  // builtin types.
  VOID,
  CHAR,
  SHORT,
  INT,
  LONG,
  FLOAT,
  SIGNED,
  UNSIGNED,
  DOUBLE,
  // Idenifier.
  IDENTIFIER,
  // Keywords.
  IF,
  ELSE,
  FOR,
  DO,
  WHILE,
  RETURN,
  UNION,
  STRUCT,
  // Operator.
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  ADD_ADD,
  SUB_SUB,
  MEMBER,
  AND,
  AND_AND,
  OR,
  OR_OR,
  LT,
  LE,
  GT,
  GE,
  EQ,
  NOT,
  NEG,
  ASSIGN,
  NOT_EQ,
  // Comment.
  COMMENT,
  // Brakets
  OPEN_PAREN,
  CLOSE_PAREN,
  OPEN_BRAKET,
  CLOSE_BRAKET,
  OPEN_BRACE,
  CLOSE_BRACE,
  // Constants.
  STRINGCONST,
  NUMBERCONST,
  CHARCONST,
  // Others.
  END,
  NEW_LINE,
  SHARP,
  COMMA,
  SEMICOLON,
  NumToken
};

typedef union TokVal {
  long LVal;
  size_t ULVal;
  double DVal;
} TokVal;

class Lexer {
  const std::string m_filePath;
  std::fstream m_codeStream;
  bool m_fileEnd;
  char m_currentCh;
  size_t m_currentLine;
  static std::map<char, Token> s_charToToken;
  std::vector<std::string> m_strValueDB;
  // For quickly look for m_strValueDB.
  std::set<std::string> m_strValueDBSet;
  std::queue<TokVal> m_tokenValDB;
  Token next();
  void readChar();

public:
  Lexer(const std::string &fileName);
  inline void getValue(int &value) {
    TokVal val = m_tokenValDB.front();
    m_tokenValDB.pop();
    value = (int)val.LVal;
  }
  inline void getValue(unsigned int &value) {
    TokVal val = m_tokenValDB.front();
    m_tokenValDB.pop();
    value = (unsigned)val.ULVal;
  }
  inline void getValue(float &value) {
    TokVal val = m_tokenValDB.front();
    m_tokenValDB.pop();
    value = (float)val.DVal;
  }
  inline void getValue(double &value) {
    TokVal val = m_tokenValDB.front();
    m_tokenValDB.pop();
    value = val.DVal;
  }
  inline void getValue(std::string &value) {
    TokVal val = m_tokenValDB.front();
    m_tokenValDB.pop();
    size_t index = val.ULVal;
    value = m_strValueDB[index];
  }

  static std::string strTok(Token);

  std::vector<Token> lex();
};
} // namespace lexer
} // namespace front_end
#endif
