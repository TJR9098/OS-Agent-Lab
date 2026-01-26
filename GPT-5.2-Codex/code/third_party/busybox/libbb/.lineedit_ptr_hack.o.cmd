cmd_libbb/lineedit_ptr_hack.o := riscv64-unknown-elf-gcc -Wp,-MD,libbb/.lineedit_ptr_hack.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"' -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude -I../../user/include -I../../user/include/sys -I../../user/include/linux    -DKBUILD_BASENAME='"lineedit_ptr_hack"'  -DKBUILD_MODNAME='"lineedit_ptr_hack"' -c -o libbb/lineedit_ptr_hack.o libbb/lineedit_ptr_hack.c

deps_libbb/lineedit_ptr_hack.o := \
  libbb/lineedit_ptr_hack.c \

libbb/lineedit_ptr_hack.o: $(deps_libbb/lineedit_ptr_hack.o)

$(deps_libbb/lineedit_ptr_hack.o):
