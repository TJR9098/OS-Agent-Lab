CROSS ?= riscv64-unknown-elf-
PYTHON ?= python3
QEMU ?= qemu-system-riscv64
DTB ?= build/virt.dtb

RISCV_ARCH_ORIGIN := $(origin RISCV_ARCH)
ARCH_ARG :=
ifneq ($(RISCV_ARCH_ORIGIN), undefined)
ARCH_ARG := RISCV_ARCH=$(RISCV_ARCH)
endif

all: firmware kernel image user fs dtb

firmware:
	$(MAKE) -C firmware CROSS=$(CROSS) $(ARCH_ARG)

kernel:
	$(MAKE) -C kernel CROSS=$(CROSS) $(ARCH_ARG)

image: kernel
	$(PYTHON) tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img

user:
	$(MAKE) -C user install CROSS=$(CROSS) $(ARCH_ARG)

fs: user
	$(PYTHON) tools/mkfs_fat32.py --out build/fs.img --size 128M --root user/rootfs --force

dtb:
	mkdir -p build
	@QEMU_BIN="$(QEMU)"; \
	if ! command -v "$$QEMU_BIN" >/dev/null 2>&1; then \
		if [ -x /opt/qemu/bin/qemu-system-riscv64 ]; then \
			QEMU_BIN="/opt/qemu/bin/qemu-system-riscv64"; \
		else \
			echo "warn: $(QEMU) not found, skip dtb generation"; \
			exit 0; \
		fi; \
	fi; \
	-timeout 3s "$$QEMU_BIN" -machine virt,dumpdtb=$(DTB) -m 1024M -smp 4 -nographic -bios none >/dev/null 2>&1 || true

clean:
	$(MAKE) -C firmware clean CROSS=$(CROSS) $(ARCH_ARG)
	$(MAKE) -C kernel clean CROSS=$(CROSS) $(ARCH_ARG)
	$(MAKE) -C user clean CROSS=$(CROSS) $(ARCH_ARG)
	-rm -f build/disk.img build/fs.img $(DTB)

run:
	@QEMU_BIN="$(QEMU)"; \
	if ! command -v "$$QEMU_BIN" >/dev/null 2>&1; then \
		if [ -x /opt/qemu/bin/qemu-system-riscv64 ]; then \
			QEMU_BIN="/opt/qemu/bin/qemu-system-riscv64"; \
		else \
			echo "error: $(QEMU) not found; set QEMU=/path/to/qemu-system-riscv64"; \
			exit 127; \
		fi; \
	fi; \
	DTB_FLAG=""; \
	if [ -s "$(DTB)" ]; then \
		DTB_FLAG="-dtb $(DTB)"; \
	else \
		echo "warn: $(DTB) missing or empty, running without -dtb"; \
	fi; \
	"$$QEMU_BIN" -machine virt -m 1024M -smp 4 -nographic $$DTB_FLAG \
		-global virtio-mmio.force-legacy=false -bios firmware/firmware.bin \
		-drive file=build/disk.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 \
		-drive file=build/fs.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1

.PHONY: all clean firmware kernel image user fs dtb run
