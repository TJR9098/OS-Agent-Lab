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
	-march=$(RISCV_ARCH) -mabi=lp64 -mcmodel=medany -msmall-data-limit=0 \
	-Iinclude -I../kernel/include $(EXTRA_CFLAGS)
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
	-rm -f $(OBJS) firmware.elf firmware.bin

.PHONY: all clean
