#pragma once

#include <stdint.h>

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;
typedef uint32_t speed_t;

#define ICANON 0x0002
#define ECHO 0x0008
#define ECHOE 0x0010
#define ECHOK 0x0020
#define ECHONL 0x0040
#define ISIG 0x0001
#define IXON 0x0400
#define IXOFF 0x1000
#define IXANY 0x0800
#define BRKINT 0x0002
#define INLCR 0x0020
#define ICRNL 0x0100
#define IUCLC 0x0200
#define IMAXBEL 0x2000
#define ONLCR 0x0004

#define TCSANOW 0
#define TCIFLUSH 0
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6

#define B0 0
#define B50 50
#define B75 75
#define B110 110
#define B134 134
#define B150 150
#define B200 200
#define B300 300
#define B600 600
#define B1200 1200
#define B1800 1800
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400
#define B57600 57600
#define B115200 115200
#define B230400 230400

struct termios {
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_cc[32];
  tcflag_t c_ispeed;
  tcflag_t c_ospeed;
};

int tcgetattr(int fd, struct termios *term);
int tcsetattr(int fd, int opt, const struct termios *term);
int tcflush(int fd, int queue_selector);
