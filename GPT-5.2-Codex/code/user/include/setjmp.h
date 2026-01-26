#pragma once

typedef long jmp_buf[14];
typedef jmp_buf sigjmp_buf;

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

static inline int sigsetjmp(sigjmp_buf env, int savemask) {
  (void)savemask;
  return setjmp(env);
}

static inline void siglongjmp(sigjmp_buf env, int val) {
  longjmp(env, val);
}
