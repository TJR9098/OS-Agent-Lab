#include "lexer.h"

#include <fstream>
#include <iostream>
#include <sstream>

#define IS_DIGIT (m_currentCh >= '0' && m_currentCh <= '9')
#define IS_ATOZ                                                                \
  ((m_currentCh >= 'a' && m_currentCh <= 'z') ||                               \
   (m_currentCh >= 'A' && m_currentCh <= 'Z'))
#define END_TOK                                                                \
  (m_currentCh == ' ' || m_currentCh == '\n' || m_currentCh == ';' ||          \
   m_currentCh == EOF)
#define ID_LEXER                                                               \
  do {                                                                         \
    while (IS_DIGIT || IS_ATOZ || m_currentCh == '_') {                        \
      lexeme += m_currentCh;                                                   \
      readChar();                                                              \
    }                                                                          \
    if (m_strValueDBSet.find(lexeme) == m_strValueDBSet.end()) {               \
      m_strValueDBSet.insert(lexeme);                                          \
      m_strValueDB.push_back(lexeme);                                          \
      TokVal value;                                                            \
      value.ULVal = m_strValueDB.size() - 1;                                   \
      m_tokenValDB.push(value);                                                \
    }                                                                          \
    return Token::IDENTIFIER;                                                  \
  } while (0);
#define LEXER_ERROR(x)                                                         \
  do {                                                                         \
    std::cout << "lexer error in line:" << m_currentLine                       \
              << " lexeme: " << lexeme << x << std::endl;                      \
    exit(-1);                                                                  \
  } while (0);

template <class T> static T convertTo(const std::string &s) {
  std::stringstream ss(s);
  T value;
  ss >> value;
  return value;
}

namespace front_end {
namespace lexer {

std::map<char, Token> Lexer::s_charToToken = {
    {'(', Token::OPEN_PAREN},  {')', Token::CLOSE_PAREN},
    {'[', Token::OPEN_BRAKET}, {']', Token::CLOSE_BRAKET},
    {'{', Token::OPEN_BRACE},  {'}', Token::CLOSE_BRACE},
    {'\n', Token::NEW_LINE},   {'#', Token::SHARP},
    {'*', Token::MUL},         {',', Token::COMMA},
    {';', Token::SEMICOLON},   {'~', Token::NEG},
    {'%', Token::MOD}};
Lexer::Lexer(const std::string &fileName)
    : m_filePath(fileName), m_currentLine(0), m_fileEnd(false) {
  m_codeStream.open(fileName.c_str(), std::fstream::in);
  readChar();
}
void Lexer::readChar() {
  int ch = m_codeStream.get();
  if (ch == EOF)
    m_fileEnd = true;
  // Still asign eof to m_currentCh,
  // for skip those while loops in next function.
  m_currentCh = (char)ch;
}
Token Lexer::next() {
  if (m_currentCh == ' ') {
    readChar();
    return next();
  }
  if (m_currentCh == '\n') {
    m_currentLine++;
    readChar();
    return next();
  }
  if (s_charToToken.find(m_currentCh) != s_charToToken.end()) {
    char ch = m_currentCh;
    readChar();
    return s_charToToken[ch];
  }
  std::string lexeme;

  // IF && INT && ID.
  if (m_currentCh == 'i') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'f') {
      lexeme += m_currentCh;
      readChar();
      if (END_TOK) {
        return Token::IF;
      }
      ID_LEXER
    }
    if (m_currentCh == 'n') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 't') {
        lexeme += m_currentCh;
        readChar();
        if (END_TOK) {
          return Token::INT;
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // CHAR && ID.
  if (m_currentCh == 'c') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'h') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'a') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'r') {
          lexeme += m_currentCh;
          readChar();
          if (END_TOK) {
            return Token::CHAR;
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // RETURN && ID.
  if (m_currentCh == 'r') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'e') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 't') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'u') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'r') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 'n') {
              lexeme += m_currentCh;
              readChar();
              if (END_TOK) {
                return Token::RETURN;
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // VOID && ID.
  if (m_currentCh == 'v') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'o') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'i') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'd') {
          lexeme += m_currentCh;
          readChar();
          if (END_TOK) {
            return Token::VOID;
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // UNSIGNED && UNION && ID.
  if (m_currentCh == 'u') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'n') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 's') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'i') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'g') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 'n') {
              lexeme += m_currentCh;
              readChar();
              if (m_currentCh == 'e') {
                lexeme += m_currentCh;
                readChar();
                if (m_currentCh == 'd') {
                  lexeme += m_currentCh;
                  readChar();
                  if (END_TOK) {
                    return Token::UNSIGNED;
                  }
                  ID_LEXER
                }
                ID_LEXER
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      if (m_currentCh == 'i') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'o') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'n') {
            lexeme += m_currentCh;
            readChar();
            if (END_TOK) {
              return Token::UNION;
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // SIGNED && STRUCT && SHORT && ID
  if (m_currentCh == 's') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'i') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'g') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'n') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'e') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 'd') {
              lexeme += m_currentCh;
              readChar();
              if (END_TOK) {
                return Token::SIGNED;
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    if (m_currentCh == 't') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'r') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'u') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'c') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 't') {
              lexeme += m_currentCh;
              readChar();
              if (END_TOK) {
                return Token::STRUCT;
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      if (m_currentCh == 'h') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'o') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'r') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 't') {
              lexeme += m_currentCh;
              readChar();
              if (END_TOK) {
                return Token::SHORT;
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // DO && DOUBLE && ID.
  if (m_currentCh == 'd') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'o') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'u') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'b') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'l') {
            lexeme += m_currentCh;
            readChar();
            if (m_currentCh == 'e') {
              lexeme += m_currentCh;
              readChar();
              if (END_TOK) {
                return Token::DOUBLE;
              }
              ID_LEXER
            }
            ID_LEXER
          }
          ID_LEXER
        }
        ID_LEXER
      }
      if (END_TOK) {
        return Token::FOR;
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // FOR && FLOAT && ID.
  if (m_currentCh == 'f') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'o') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'r') {
        lexeme += m_currentCh;
        readChar();
        if (END_TOK) {
          return Token::FOR;
        }
        ID_LEXER
      }
      ID_LEXER
    }
    if (m_currentCh == 'l') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'o') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'a') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 't') {
            lexeme += m_currentCh;
            readChar();
            if (END_TOK) {
              return Token::FLOAT;
            }
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // WHILE && ID.
  if (m_currentCh == 'w') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'h') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'i') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'l') {
          lexeme += m_currentCh;
          readChar();
          if (m_currentCh == 'e') {
            lexeme += m_currentCh;
            readChar();
            if (END_TOK) {
              return Token::WHILE;
            }
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // LONG && ID.
  if (m_currentCh == 'l') {
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh == 'o') {
      lexeme += m_currentCh;
      readChar();
      if (m_currentCh == 'n') {
        lexeme += m_currentCh;
        readChar();
        if (m_currentCh == 'g') {
          lexeme += m_currentCh;
          readChar();
          if (END_TOK) {
            return Token::LONG;
          }
          ID_LEXER
        }
        ID_LEXER
      }
      ID_LEXER
    }
    ID_LEXER
  }

  // IDENTIFIER.
  if (m_currentCh == '_' || IS_ATOZ) {
    lexeme += m_currentCh;
    readChar();
    ID_LEXER
  }

  // CHAR
  if (m_currentCh == '\'') {
    readChar();
    if (m_fileEnd) {
      LEXER_ERROR("incomplete char")
    }
    lexeme += m_currentCh;
    readChar();
    if (m_currentCh != '\'') {
      LEXER_ERROR("error when lex char")
    }
    readChar();
    return Token::CHARCONST;
  }

  // STRING
  if (m_currentCh == '"') {
    readChar();
    while (m_currentCh != '"' && m_currentCh != EOF) {
      lexeme += m_currentCh;
      readChar();
    }
    if (m_currentCh == EOF) {
      LEXER_ERROR("incomplete string")
    }
    if (m_strValueDBSet.find(lexeme) == m_strValueDBSet.end()) {
      m_strValueDBSet.insert(lexeme);
      m_strValueDB.push_back(lexeme);
      TokVal value;
      value.ULVal = m_strValueDB.size() - 1;
      m_tokenValDB.push(value);
    }
    readChar();
    return Token::STRINGCONST;
  }

  // NOT && NOT_EQ.
  if (m_currentCh == '!') {
    readChar();
    if (m_currentCh == '=') {
      readChar();
      return Token::NOT_EQ;
    }
    return Token::NOT;
  }
  // EQ && ASSIGN.
  if (m_currentCh == '=') {
    readChar();
    if (m_currentCh == '=') {
      readChar();
      return Token::EQ;
    }
    return Token::ASSIGN;
  }

  // AND && AND_AND.
  if (m_currentCh == '&') {
    readChar();
    if (m_currentCh == '&') {
      readChar();
      return Token::AND_AND;
    }
    return Token::AND;
  }

  // OR && OR_OR.
  if (m_currentCh == '|') {
    readChar();
    if (m_currentCh == '|') {
      readChar();
      return Token::OR_OR;
    }
    return Token::OR;
  }

  // LT && LE.
  if (m_currentCh == '<') {
    readChar();
    if (m_currentCh == '=') {
      readChar();
      return Token::LE;
    }
    return Token::LT;
  }

  // GT && GE.
  if (m_currentCh == '>') {
    readChar();
    if (m_currentCh == '=') {
      readChar();
      return Token::GE;
    }
    return Token::GT;
  }

  // ADD && ADD_ADD.
  if (m_currentCh == '+') {
    readChar();
    if (m_currentCh == '+') {
      readChar();
      return Token::ADD_ADD;
    }
    return Token::ADD;
  }

  // SUB && SUB_SUB && MEMBER.
  if (m_currentCh == '-') {
    readChar();
    if (m_currentCh == '-') {
      readChar();
      return Token::SUB_SUB;
    }
    if (m_currentCh == '>') {
      readChar();
      return Token::MEMBER;
    }
    return Token::SUB;
  }

  // DIV && COMMENT.
  if (m_currentCh == '/') {
    readChar();
    if (m_currentCh == '/') {
      readChar();
      while (m_currentCh != '\n') {
        lexeme += m_currentCh;
      }
      m_strValueDBSet.insert(lexeme);
      m_strValueDB.push_back(lexeme);
      TokVal value;
      value.ULVal = m_strValueDB.size() - 1;
      m_tokenValDB.push(value);
      return Token::COMMENT;
    }
    return Token::DIV;
  }
  // NUMBER.
  if (m_currentCh > '0' && m_currentCh <= '9') {
    lexeme += m_currentCh;
    readChar();
    if (!IS_DIGIT) {
      size_t val = convertTo<size_t>(lexeme);
      TokVal value;
      value.ULVal = val;
      m_tokenValDB.push(value);
      return Token::NUMBERCONST;
    }
    if (m_currentCh == '.') {
      lexeme += ".";
      readChar();
      while (IS_DIGIT) {
        lexeme += m_currentCh;
        readChar();
      }
      double val = convertTo<double>(lexeme);
      TokVal value;
      value.DVal = val;
      m_tokenValDB.push(value);
      return Token::NUMBERCONST;
    }
  }

  // Do not support Octal number.
  if (m_currentCh == '0') {
    lexeme += "0";
    readChar();
    if (m_currentCh == '.') {
      lexeme += ".";
      readChar();
      while (IS_DIGIT) {
        lexeme += m_currentCh;
        readChar();
      }
      double val = convertTo<double>(lexeme);
      TokVal value;
      value.DVal = val;
      m_tokenValDB.push(value);
      return Token::NUMBERCONST;
    }
    if (m_currentCh == 'x' || m_currentCh == 'X') {
      lexeme += "x";
      readChar();
      while (IS_DIGIT || (m_currentCh >= 'A' && m_currentCh <= 'F')) {
        lexeme += m_currentCh;
        readChar();
      }
      size_t val = convertTo<size_t>(lexeme);
      TokVal value;
      value.ULVal = val;
      m_tokenValDB.push(value);
      return Token::NUMBERCONST;
    }
    size_t val = convertTo<size_t>(lexeme);
    TokVal value;
    value.ULVal = val;
    m_tokenValDB.push(value);
    return Token::NUMBERCONST;
  }
  if (m_currentCh == EOF)
    return Token::END;
  LEXER_ERROR("");
}
std::vector<Token> Lexer::lex() {
  std::vector<Token> toks;
  while (!m_fileEnd) {
    toks.push_back(next());
#ifdef _DEBUG
    // std::cout << strTok[toks.back()] << std::endl;
#endif
  }
  return toks;
}

std::string Lexer::strTok(Token tok) {
  static std::map<Token, std::string> tokToStr{
      {Token::VOID, "VOID"},
      {Token::CHAR, "CHAR"},
      {Token::SHORT, "SHORT"},
      {Token::FLOAT, "FLOAT"},
      {Token::NUMBERCONST, "NUMBERCONST"},
      {Token::INT, "INT"},
      {Token::LONG, "LONG"},
      {Token::UNSIGNED, "UNSIGNED"},
      {Token::SIGNED, "SIGNED"},
      {Token::FOR, "FOR"},
      {Token::IF, "IF"},
      {Token::ELSE, "ELSE"},
      {Token::WHILE, "WHILE"},
      {Token::DO, "DO"},
      {Token::DOUBLE, "DOUBLE"},
      {Token::UNION, "UNION"},
      {Token::STRUCT, "STRUCT"},
      {Token::STRINGCONST, "STRINGCONST"},
      {Token::RETURN, "RETURN"},
      {Token::IDENTIFIER, "IDENTIFIER"},
      {Token::LT, "<"},
      {Token::LE, "<="},
      {Token::GT, ">"},
      {Token::GE, ">="},
      {Token::EQ, "=="},
      {Token::ASSIGN, "="},
      {Token::NOT, "!"},
      {Token::NEG, "~"},
      {Token::MUL, "*"},
      {Token::DIV, "/"},
      {Token::COMMENT, ""},
      {Token::SUB, "-"},
      {Token::ADD, "+"},
      {Token::AND, "&"},
      {Token::OR, "|"},
      {Token::MOD, "%"},
      {Token::SUB_SUB, "--"},
      {Token::AND_AND, "&&"},
      {Token::OR_OR, "||"},
      {Token::ADD_ADD, "++"},
      {Token::MEMBER, "->"},
      {Token::COMMA, ","},
      {Token::SEMICOLON, ";"},
      {Token::SHARP, "#"},
      {Token::OPEN_PAREN, "("},
      {Token::CLOSE_PAREN, ")"},
      {Token::OPEN_BRAKET, "["},
      {Token::CLOSE_BRAKET, "]"},
      {Token::OPEN_BRACE, "{"},
      {Token::CLOSE_BRACE, "}"},
      {Token::CHARCONST, "CHARCONST"},
      {Token::END, "EOF"},
  };

  return tokToStr[tok];
}
} // namespace lexer
} // namespace front_end
