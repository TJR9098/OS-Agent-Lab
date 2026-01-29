# 工具链设置
CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
RISCV_ARCH_BASE := rv64imac
RISCV_ZICSR_SUPPORTED := $(shell printf '' | $(CC) -x c - -c -o /dev/null -march=$(RISCV_ARCH_BASE)_zicsr -mabi=lp64 >/dev/null 2>&1 && echo 1 || echo 0)
ifeq ($(RISCV_ZICSR_SUPPORTED),1)
RISCV_ARCH := $(RISCV_ARCH_BASE)_zicsr
else
RISCV_ARCH := $(RISCV_ARCH_BASE)
endif

# 编译选项
CFLAGS := -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -Iinclude -O0
LDFLAGS := -T linker.ld -nostdlib -nodefaultlibs -lgcc -no-pie -Wl,--build-id=none
ASFLAGS := -Iinclude -march=$(RISCV_ARCH) -mabi=lp64

# QEMU路径
QEMU := qemu-system-riscv64

# OpenSBI路径
# 默认
OPENSBI_BIOS := default
# 优先使用系统路径
ifeq ($(wildcard /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin),/usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin)
OPENSBI_BIOS := /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin
endif


# 源文件列表
C_SRCS := main.c sbi.c printf.c panic.c trap.c timer.c mem.c vm.c test_simple.c test_vm.c
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
	$(QEMU) -machine virt -nographic -bios $(OPENSBI_BIOS) -m 1024M -kernel ./kernel.elf -smp 1

clean:
	rm -f *.o kernel.elf kernel.bin

.PHONY: all clean run
