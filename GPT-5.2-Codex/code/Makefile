CROSS ?= riscv64-unknown-elf-
PYTHON ?= python3

all: firmware kernel image user fs

firmware:
	make -C firmware

kernel:
	make -C kernel

image: kernel
	$(PYTHON) tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img

user:
	make -C user install

fs: user
	$(PYTHON) tools/mkfs_fat32.py --out build/fs.img --size 128M --root user/rootfs --force

clean:
	make -C firmware clean
	make -C kernel clean
	make -C user clean
	-cmd /c del /q build\disk.img 2>NUL
	-cmd /c del /q build\fs.img 2>NUL

run:
	qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1

.PHONY: all clean firmware kernel image user fs
