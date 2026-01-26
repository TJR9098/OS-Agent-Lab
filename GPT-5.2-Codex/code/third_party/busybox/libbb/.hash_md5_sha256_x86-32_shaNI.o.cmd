cmd_libbb/hash_md5_sha256_x86-32_shaNI.o := riscv64-unknown-elf-gcc -Wp,-MD,libbb/.hash_md5_sha256_x86-32_shaNI.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG  -DBB_VER='"1.36.1"' -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude -I../../user/include -I../../user/include/sys -I../../user/include/linux       -c -o libbb/hash_md5_sha256_x86-32_shaNI.o libbb/hash_md5_sha256_x86-32_shaNI.S

deps_libbb/hash_md5_sha256_x86-32_shaNI.o := \
  libbb/hash_md5_sha256_x86-32_shaNI.S \
    $(wildcard include/config/sha256/hwaccel.h) \

libbb/hash_md5_sha256_x86-32_shaNI.o: $(deps_libbb/hash_md5_sha256_x86-32_shaNI.o)

$(deps_libbb/hash_md5_sha256_x86-32_shaNI.o):
