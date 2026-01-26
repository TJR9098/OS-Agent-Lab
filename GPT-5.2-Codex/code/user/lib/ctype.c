#include <ctype.h>

int isspace(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int isdigit(int c) {
  return c >= '0' && c <= '9';
}

int isalpha(int c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}

int isxdigit(int c) {
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int iscntrl(int c) {
  return (c >= 0 && c < 0x20) || c == 0x7f;
}

int islower(int c) {
  return c >= 'a' && c <= 'z';
}

int isupper(int c) {
  return c >= 'A' && c <= 'Z';
}

int isprint(int c) {
  return c >= 0x20 && c <= 0x7e;
}

int isgraph(int c) {
  return isprint(c) && !isspace(c);
}

int ispunct(int c) {
  return isprint(c) && !isalnum(c) && !isspace(c);
}

int isascii(int c) {
  return c >= 0 && c <= 0x7f;
}

int toascii(int c) {
  return c & 0x7f;
}

int tolower(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 'a';
  }
  return c;
}

int toupper(int c) {
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 'A';
  }
  return c;
}
