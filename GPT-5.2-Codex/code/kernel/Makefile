CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
RISCV_ARCH_ORIGIN := $(origin RISCV_ARCH)
RISCV_ARCH_BASE ?= rv64gc
ifeq ($(RISCV_ARCH_ORIGIN), undefined)
RISCV_ZICSR_SUPPORTED := $(shell printf '' | $(CC) -x c - -c -o /dev/null -march=$(RISCV_ARCH_BASE)_zicsr -mabi=lp64 >/dev/null 2>&1 && echo 1 || echo 0)
ifeq ($(RISCV_ZICSR_SUPPORTED),1)
RISCV_ARCH := $(RISCV_ARCH_BASE)_zicsr
else
RISCV_ARCH := $(RISCV_ARCH_BASE)
endif
endif
RISCV_ARCH ?= $(RISCV_ARCH_BASE)

EXTRA_CFLAGS ?=
CFLAGS := -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -msmall-data-limit=0 -Iinclude $(EXTRA_CFLAGS)
ASFLAGS := -Iinclude -march=$(RISCV_ARCH) -mabi=lp64
LDFLAGS := -T linker.ld -nostdlib -no-pie -Wl,--build-id=none
LDLIBS := -lgcc

C_SRCS := boot.c main.c uart.c printf.c string.c log.c trap.c timer.c sbi.c fdt.c blk.c virtio_blk.c fat32.c vfs.c ramfs.c mm.c vm.c file.c proc.c elf.c syscall.c selftest.c
S_SRCS := entry.S stack.S trapvec.S swtch.S userret.S
OBJS := $(C_SRCS:.c=.o) $(S_SRCS:.S=.o)

all: kernel.elf kernel.bin

kernel.elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

clean:
	-rm -f $(OBJS) kernel.elf kernel.bin

.PHONY: all clean
