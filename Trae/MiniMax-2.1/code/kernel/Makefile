CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

CFLAGS = -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude
ASFLAGS = -Iinclude -march=rv64imac -mabi=lp64
LDFLAGS = -T linker.ld -nostdlib -no-pie -Wl,--build-id=none

SRCS_C = main.c sbi.c printf.c panic.c trap.c timer.c kalloc.c
SRCS_S = entry.S stack.S trapvec.S
OBJS = $(SRCS_C:.c=.o) $(SRCS_S:.S=.o)

all: kernel.elf kernel.bin

kernel.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary -S $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<

run: all
	qemu-system-riscv64 -machine virt -nographic -bios default -kernel ./kernel.elf -smp 1 -m 4096

clean:
	rm -f $(OBJS) kernel.elf kernel.bin

.PHONY: all clean run
