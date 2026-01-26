#pragma once

#include <sys/types.h>
#include <sys/socket.h>

extern int h_errno;

const char *hstrerror(int err);

struct hostent {
  char *h_name;
  char **h_aliases;
  int h_addrtype;
  int h_length;
  char **h_addr_list;
};

#define h_addr h_addr_list[0]

struct hostent *gethostbyname(const char *name);

struct servent {
  char *s_name;
  char **s_aliases;
  int s_port;
  char *s_proto;
};

struct addrinfo {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  unsigned long ai_addrlen;
  struct sockaddr *ai_addr;
  char *ai_canonname;
  struct addrinfo *ai_next;
};

#define AI_CANONNAME 0x02
#define AI_NUMERICHOST 0x04

#define NI_NUMERICSERV 0x01
#define NI_NAMEREQD 0x02
#define NI_NUMERICHOST 0x04
#define NI_NUMERICSCOPE 0x08

struct servent *getservbyname(const char *name, const char *proto);
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
int getnameinfo(const struct sockaddr *sa, unsigned int salen,
                char *host, unsigned long hostlen,
                char *serv, unsigned long servlen, int flags);
