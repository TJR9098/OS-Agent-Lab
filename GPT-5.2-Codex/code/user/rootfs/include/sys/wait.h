#pragma once

#define WIFEXITED(status) (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#define WIFSIGNALED(status) (((status) & 0x7f) != 0 && (((status) & 0x7f) != 0x7f))
#define WTERMSIG(status) ((status) & 0x7f)
#define WCOREDUMP(status) 0
#define WNOHANG 1
