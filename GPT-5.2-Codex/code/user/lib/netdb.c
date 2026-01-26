#include <netdb.h>
#include <stddef.h>

int h_errno = 0;

const char *hstrerror(int err) {
  (void)err;
  return "host error";
}

struct hostent *gethostbyname(const char *name) {
  (void)name;
  h_errno = 1;
  return NULL;
}

struct servent *getservbyname(const char *name, const char *proto) {
  (void)name;
  (void)proto;
  return NULL;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
  (void)node;
  (void)service;
  (void)hints;
  if (res) {
    *res = NULL;
  }
  return -1;
}

void freeaddrinfo(struct addrinfo *res) {
  (void)res;
}

int getnameinfo(const struct sockaddr *sa, unsigned int salen,
                char *host, unsigned long hostlen,
                char *serv, unsigned long servlen, int flags) {
  (void)sa;
  (void)salen;
  (void)host;
  (void)hostlen;
  (void)serv;
  (void)servlen;
  (void)flags;
  return -1;
}
