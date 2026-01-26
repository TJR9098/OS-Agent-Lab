cmd_libbb/trim.o := riscv64-unknown-elf-gcc -Wp,-MD,libbb/.trim.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"' -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude -I../../user/include -I../../user/include/sys -I../../user/include/linux    -DKBUILD_BASENAME='"trim"'  -DKBUILD_MODNAME='"trim"' -c -o libbb/trim.o libbb/trim.c

deps_libbb/trim.o := \
  libbb/trim.c \
  include/libbb.h \
    $(wildcard include/config/feature/shadowpasswds.h) \
    $(wildcard include/config/use/bb/shadow.h) \
    $(wildcard include/config/selinux.h) \
    $(wildcard include/config/feature/utmp.h) \
    $(wildcard include/config/locale/support.h) \
    $(wildcard include/config/use/bb/pwd/grp.h) \
    $(wildcard include/config/lfs.h) \
    $(wildcard include/config/feature/buffers/go/on/stack.h) \
    $(wildcard include/config/feature/buffers/go/in/bss.h) \
    $(wildcard include/config/extra/cflags.h) \
    $(wildcard include/config/variable/arch/pagesize.h) \
    $(wildcard include/config/feature/verbose.h) \
    $(wildcard include/config/feature/etc/services.h) \
    $(wildcard include/config/feature/ipv6.h) \
    $(wildcard include/config/feature/seamless/xz.h) \
    $(wildcard include/config/feature/seamless/lzma.h) \
    $(wildcard include/config/feature/seamless/bz2.h) \
    $(wildcard include/config/feature/seamless/gz.h) \
    $(wildcard include/config/feature/seamless/z.h) \
    $(wildcard include/config/float/duration.h) \
    $(wildcard include/config/feature/check/names.h) \
    $(wildcard include/config/feature/prefer/applets.h) \
    $(wildcard include/config/long/opts.h) \
    $(wildcard include/config/feature/pidfile.h) \
    $(wildcard include/config/feature/syslog.h) \
    $(wildcard include/config/feature/syslog/info.h) \
    $(wildcard include/config/warn/simple/msg.h) \
    $(wildcard include/config/feature/individual.h) \
    $(wildcard include/config/shell/ash.h) \
    $(wildcard include/config/shell/hush.h) \
    $(wildcard include/config/echo.h) \
    $(wildcard include/config/sleep.h) \
    $(wildcard include/config/printf.h) \
    $(wildcard include/config/test.h) \
    $(wildcard include/config/test1.h) \
    $(wildcard include/config/test2.h) \
    $(wildcard include/config/kill.h) \
    $(wildcard include/config/killall.h) \
    $(wildcard include/config/killall5.h) \
    $(wildcard include/config/chown.h) \
    $(wildcard include/config/ls.h) \
    $(wildcard include/config/xxx.h) \
    $(wildcard include/config/route.h) \
    $(wildcard include/config/feature/hwib.h) \
    $(wildcard include/config/desktop.h) \
    $(wildcard include/config/feature/crond/d.h) \
    $(wildcard include/config/feature/setpriv/capabilities.h) \
    $(wildcard include/config/run/init.h) \
    $(wildcard include/config/feature/securetty.h) \
    $(wildcard include/config/pam.h) \
    $(wildcard include/config/use/bb/crypt.h) \
    $(wildcard include/config/feature/adduser/to/group.h) \
    $(wildcard include/config/feature/del/user/from/group.h) \
    $(wildcard include/config/ioctl/hex2str/error.h) \
    $(wildcard include/config/feature/editing.h) \
    $(wildcard include/config/feature/editing/history.h) \
    $(wildcard include/config/feature/tab/completion.h) \
    $(wildcard include/config/feature/username/completion.h) \
    $(wildcard include/config/feature/editing/fancy/prompt.h) \
    $(wildcard include/config/feature/editing/savehistory.h) \
    $(wildcard include/config/feature/editing/vi.h) \
    $(wildcard include/config/feature/editing/save/on/exit.h) \
    $(wildcard include/config/pmap.h) \
    $(wildcard include/config/feature/show/threads.h) \
    $(wildcard include/config/feature/ps/additional/columns.h) \
    $(wildcard include/config/feature/topmem.h) \
    $(wildcard include/config/feature/top/smp/process.h) \
    $(wildcard include/config/pgrep.h) \
    $(wildcard include/config/pkill.h) \
    $(wildcard include/config/pidof.h) \
    $(wildcard include/config/sestatus.h) \
    $(wildcard include/config/unicode/support.h) \
    $(wildcard include/config/feature/mtab/support.h) \
    $(wildcard include/config/feature/clean/up.h) \
    $(wildcard include/config/feature/devfs.h) \
  include/platform.h \
    $(wildcard include/config/werror.h) \
    $(wildcard include/config/big/endian.h) \
    $(wildcard include/config/little/endian.h) \
    $(wildcard include/config/nommu.h) \
  ../../user/include/limits.h \
  ../../user/include/byteswap.h \
  ../../user/include/endian.h \
  ../../user/include/stdint.h \
  /usr/lib/gcc/riscv64-unknown-elf/9.3.0/include/stdbool.h \
  ../../user/include/unistd.h \
  ../../user/include/sys/types.h \
  ../../user/include/stddef.h \
  ../../user/include/sys/stat.h \
  ../../user/include/ctype.h \
  ../../user/include/dirent.h \
  ../../user/include/errno.h \
  ../../user/include/fcntl.h \
  ../../user/include/inttypes.h \
  ../../user/include/netdb.h \
  ../../user/include/sys/socket.h \
  ../../user/include/setjmp.h \
  ../../user/include/signal.h \
  ../../user/include/paths.h \
  ../../user/include/stdio.h \
  /usr/lib/gcc/riscv64-unknown-elf/9.3.0/include/stdarg.h \
  ../../user/include/stdlib.h \
  ../../user/include/string.h \
  ../../user/include/libgen.h \
  ../../user/include/poll.h \
  ../../user/include/sys/ioctl.h \
  ../../user/include/sys/mman.h \
  ../../user/include/sys/resource.h \
  ../../user/include/sys/time.h \
  ../../user/include/sys/sysmacros.h \
  ../../user/include/sys/wait.h \
  ../../user/include/termios.h \
  ../../user/include/time.h \
  ../../user/include/sys/param.h \
  ../../user/include/pwd.h \
  ../../user/include/grp.h \
  ../../user/include/mntent.h \
  ../../user/include/sys/statfs.h \
  ../../user/include/arpa/inet.h \
  ../../user/include/netinet/in.h \
  include/xatonum.h \

libbb/trim.o: $(deps_libbb/trim.o)

$(deps_libbb/trim.o):
