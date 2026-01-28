#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  int ret = reboot(0, 0, 0, 0);
  if (ret < 0) {
    dprintf(2, "poweroff: reboot failed (%d)\n", errno);
    return 1;
  }
  return 0;
}

