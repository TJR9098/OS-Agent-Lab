CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
AR := $(CROSS)ar
RISCV_ARCH ?= rv64imac

CFLAGS := -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -Iinclude
ASFLAGS := -march=$(RISCV_ARCH) -mabi=lp64
LDFLAGS := -T linker.ld -nostdlib -no-pie -Wl,--build-id=none
LDLIBS := -lgcc

LIB_SRCS := lib/syscall.c lib/string.c lib/ctype.c lib/printf.c lib/stdlib.c lib/malloc.c \
	lib/errno.c lib/termios.c lib/isatty.c lib/stdio_file.c lib/stat.c lib/poll.c \
	lib/signal.c lib/libgen.c lib/time.c lib/resource.c lib/mman.c lib/assert.c \
	lib/pwd.c lib/grp.c lib/unistd_extra.c lib/strerror.c lib/perror.c lib/mntent.c \
	lib/statfs.c lib/inet.c lib/dirent.c lib/fnmatch.c lib/getopt.c lib/socket.c \
	lib/fcntl.c lib/netdb.c lib/utsname.c lib/times.c lib/glob.c
LIB_ASM := lib/setjmp.S
LIB_OBJS := $(LIB_SRCS:.c=.o) $(LIB_ASM:.S=.o)
LIB := lib/libc.a
LIBM_OBJS := lib/libm_stub.o
LIBM := lib/libm.a

CRT_OBJS := crt/start.o

PROGS := sh ls mkdir touch mkhello cat memtest tcc
BIN_DIR := bin
ROOTFS_BIN := rootfs/bin
ROOTFS_INCLUDE := rootfs/include
ROOTFS_LIB := rootfs/lib
ROOTFS_ROOT := rootfs/root
ROOTFS_TMP := rootfs/tmp

BUSYBOX_DIR := ../third_party/busybox
BUSYBOX_BIN := $(BUSYBOX_DIR)/busybox
PICO_DIR := ../third_party/picoc
PICO_BIN := $(PICO_DIR)/picoc
PICO_CFLAGS := -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -DUNIX_HOST -DNO_FP -DVER=\\\"2.1\\\" -I../../user/include
PICO_LDFLAGS := -T ../../user/linker.ld -nostdlib -no-pie ../../user/crt/start.o
PICO_LIBS := ../../user/lib/libc.a -lgcc

all: $(BIN_DIR) $(LIB) $(LIBM) $(PROGS:%=$(BIN_DIR)/%) $(BUSYBOX_BIN) $(PICO_BIN)

$(BIN_DIR):
	mkdir -p $@

$(ROOTFS_BIN):
	mkdir -p $@

$(LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(LIBM): $(LIBM_OBJS)
	$(AR) rcs $@ $^

$(BIN_DIR)/%: src/%.o $(LIB) $(CRT_OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CRT_OBJS) $< $(LIB) $(LDLIBS)

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

lib/%.o: lib/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

crt/%.o: crt/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

$(BUSYBOX_BIN): $(LIB) $(LIBM) $(CRT_OBJS)
	$(MAKE) -C $(BUSYBOX_DIR) ARCH=riscv CROSS_COMPILE=$(CROSS) CC=$(CC) AR=$(AR) \
		CFLAGS="$(CFLAGS) -I../../user/include -I../../user/include/sys -I../../user/include/linux" \
		LDFLAGS="-nostdlib -no-pie -Wl,--build-id=none -L../../user/lib -Wl,-T,../../user/linker.ld -Wl,../../user/crt/start.o" busybox

$(PICO_BIN): $(LIB) $(CRT_OBJS)
	$(MAKE) -C $(PICO_DIR) CC=$(CC) CFLAGS="$(PICO_CFLAGS)" LDFLAGS="$(PICO_LDFLAGS)" \
		LIBS="$(PICO_LIBS)"

install: all $(ROOTFS_BIN)
	cp $(BIN_DIR)/* $(ROOTFS_BIN)/
	cp $(BUSYBOX_BIN) $(ROOTFS_BIN)/vi
	cp $(BUSYBOX_BIN) $(ROOTFS_BIN)/vim
	cp $(PICO_BIN) $(ROOTFS_BIN)/picoc
	mkdir -p $(ROOTFS_INCLUDE) $(ROOTFS_LIB) $(ROOTFS_ROOT) $(ROOTFS_TMP)
	cp -r include/* $(ROOTFS_INCLUDE)/
	cp $(LIB) $(ROOTFS_LIB)/
	cp $(LIBM) $(ROOTFS_LIB)/
	cp crt/start.o $(ROOTFS_LIB)/crt0.o

clean:
	-rm -f $(LIB_OBJS) $(LIBM_OBJS) $(CRT_OBJS) src/*.o $(BIN_DIR)/* $(LIB) $(LIBM)
	-$(MAKE) -C $(BUSYBOX_DIR) clean
	-$(MAKE) -C $(PICO_DIR) clean

.PHONY: all clean install
