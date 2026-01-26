# 工具链设置
CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

# 编译选项
CFLAGS := -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude
LDFLAGS := -T linker.ld -nostdlib -no-pie -Wl,--build-id=none
ASFLAGS := -Iinclude -march=rv64imac -mabi=lp64

# QEMU路径
QEMU := /opt/qemu/bin/qemu-system-riscv64

# OpenSBI路径
OPENSBI_BIOS := /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin

# 源文件列表
C_SRCS := main.c sbi.c printf.c panic.c trap.c timer.c mem.c
S_SRCS := entry.S stack.S trapvec.S

# 目标文件列表
OBJS := $(C_SRCS:.c=.o) $(S_SRCS:.S=.o)

# 目标
all: kernel.elf kernel.bin

kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

run: kernel.elf
	$(QEMU) -machine virt -nographic -bios $(OPENSBI_BIOS) -kernel ./kernel.elf -smp 1

clean:
	rm -f *.o kernel.elf kernel.bin

.PHONY: all clean run
