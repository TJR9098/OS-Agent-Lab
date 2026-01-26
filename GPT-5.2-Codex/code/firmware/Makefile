CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
RISCV_ARCH ?= rv64imac

CFLAGS := -Wall -Wextra -O2 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -msmall-data-limit=0 \
	-Iinclude -I../kernel/include
ASFLAGS := -Iinclude -I../kernel/include -march=$(RISCV_ARCH) -mabi=lp64
LDFLAGS := -T linker.ld -nostdlib -no-pie -Wl,--build-id=none
LDLIBS := -lgcc

C_SRCS := main.c uart.c string.c printf.c fdt.c virtio.c disk.c mtrap.c
S_SRCS := start.S trap.S
OBJS := $(C_SRCS:.c=.o) $(S_SRCS:.S=.o)

all: firmware.elf firmware.bin

firmware.elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@

clean:
	-cmd /c del /q $(OBJS) firmware.elf firmware.bin 2>NUL

.PHONY: all clean
