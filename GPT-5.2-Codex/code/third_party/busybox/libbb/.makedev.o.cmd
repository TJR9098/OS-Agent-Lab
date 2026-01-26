cmd_libbb/makedev.o := riscv64-unknown-elf-gcc -Wp,-MD,libbb/.makedev.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"' -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude -I../../user/include -I../../user/include/sys -I../../user/include/linux    -DKBUILD_BASENAME='"makedev"'  -DKBUILD_MODNAME='"makedev"' -c -o libbb/makedev.o libbb/makedev.c

deps_libbb/makedev.o := \
  libbb/makedev.c \
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
  ../../user/include/features.h \
  ../../user/include/sys/sysmacros.h \

libbb/makedev.o: $(deps_libbb/makedev.o)

$(deps_libbb/makedev.o):
