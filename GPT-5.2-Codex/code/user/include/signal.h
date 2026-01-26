#pragma once

typedef void (*sighandler_t)(int);
typedef int sig_atomic_t;
typedef unsigned long sigset_t;

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGWINCH 28
#define NSIG 64

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SA_RESTART 0x10000000
#define SA_SIGINFO 0x00000004

struct sigaction {
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, void *, void *);
  } sa_u;
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)(void);
};
#define sa_handler sa_u.sa_handler
#define sa_sigaction sa_u.sa_sigaction

sighandler_t signal(int sig, sighandler_t handler);
int raise(int sig);
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int sig);
int sigismember(const sigset_t *set, int sig);
int sigsuspend(const sigset_t *mask);
