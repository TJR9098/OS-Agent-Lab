#include <sys/utsname.h>
#include <string.h>

int uname(struct utsname *buf) {
  if (!buf) {
    return -1;
  }
  strcpy(buf->sysname, "agent_os");
  strcpy(buf->nodename, "qemu");
  strcpy(buf->release, "0.1");
  strcpy(buf->version, "0");
  strcpy(buf->machine, "riscv64");
  return 0;
}
