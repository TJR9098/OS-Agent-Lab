#include <signal.h>
#include <errno.h>

static sighandler_t g_handlers[64];
static sigset_t g_sigmask;

sighandler_t signal(int sig, sighandler_t handler) {
  if (sig < 0 || sig >= (int)(sizeof(g_handlers) / sizeof(g_handlers[0]))) {
    return SIG_ERR;
  }
  sighandler_t old = g_handlers[sig];
  g_handlers[sig] = handler;
  return old;
}

int raise(int sig) {
  if (sig < 0 || sig >= (int)(sizeof(g_handlers) / sizeof(g_handlers[0]))) {
    return -1;
  }
  sighandler_t h = g_handlers[sig];
  if (h && h != SIG_IGN && h != SIG_DFL) {
    h(sig);
  }
  return 0;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
  if (sig < 0 || sig >= (int)(sizeof(g_handlers) / sizeof(g_handlers[0]))) {
    return -1;
  }
  if (oldact) {
    oldact->sa_handler = g_handlers[sig];
    oldact->sa_mask = 0;
    oldact->sa_flags = 0;
    oldact->sa_restorer = 0;
  }
  if (act) {
    g_handlers[sig] = act->sa_handler;
  }
  return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
  if (oldset) {
    *oldset = g_sigmask;
  }
  if (!set) {
    return 0;
  }
  switch (how) {
    case SIG_BLOCK:
      g_sigmask |= *set;
      break;
    case SIG_UNBLOCK:
      g_sigmask &= ~(*set);
      break;
    case SIG_SETMASK:
      g_sigmask = *set;
      break;
    default:
      return -1;
  }
  return 0;
}

int sigemptyset(sigset_t *set) {
  if (!set) {
    return -1;
  }
  *set = 0;
  return 0;
}

int sigfillset(sigset_t *set) {
  if (!set) {
    return -1;
  }
  *set = ~0UL;
  return 0;
}

int sigaddset(sigset_t *set, int sig) {
  if (!set || sig < 0 || sig >= (int)(sizeof(unsigned long) * 8)) {
    return -1;
  }
  *set |= (1UL << sig);
  return 0;
}

int sigismember(const sigset_t *set, int sig) {
  if (!set || sig < 0 || sig >= (int)(sizeof(unsigned long) * 8)) {
    return -1;
  }
  return ((*set & (1UL << sig)) != 0);
}

int sigsuspend(const sigset_t *mask) {
  (void)mask;
  errno = EINTR;
  return -1;
}
