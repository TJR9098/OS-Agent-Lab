# Agent信息

- 模型名称：GPT-5.2-Codex
- Reasoning Effort：Extra High
- 上下文窗口大小：258k tokens
- Mode：Agent（full access）

# 项目概述

本项目的代码完全由Agent生成。

本项目是面向 QEMU virt（RV64）的 RISC-V 操作系统原型，包含 M 模式固件与 S 模式内核。固件支持 `-bios` 直接加载，读取磁盘镜像并解析内核 ELF，解析 DTB 后传递启动信息；内核完成 Sv39 分页初始化、UART 日志、定时器中断、virtio-blk 块设备、可写 FAT32 根文件系统与 RAMFS 挂载，并提供基础进程/系统调用与静态 ELF 用户程序加载路径。

## 目录结构
- `firmware/`: M 模式固件（bootloader），负责串口初始化、DTB 解析、virtio-blk 读取、内核 ELF 加载与 bootinfo 传递。
  - `firmware/include/`: 固件侧头文件（FDT、磁盘、virtio、字符串/printf 等）。
- `kernel/`: S 模式内核实现（内存管理、异常/中断、块设备、文件系统、VFS、进程/系统调用等）。
  - `kernel/include/`: 内核头文件（memlayout、vm、vfs、fat32、ramfs、syscall 等）。
- `tools/`: 主机侧工具脚本。
  - `tools/mkimage.py`: 生成内核磁盘镜像（包含镜像头与内核 ELF）。
  - `tools/mkfs_fat32.py`: 生成带 MBR 的 FAT32 镜像（默认 128MB，含 `/ram` 目录）。
- `verify/`: 验证记录与测试日志。
- `build/`: 构建与运行生成的中间产物/镜像（如 `disk.img`、`fs.img`、`virt.dtb` 等）。

## 如何运行（WSL）
### 依赖
请自行安装并确保以下命令在 `PATH` 中可用：
- RISC-V 交叉工具链（前缀 `riscv64-unknown-elf-`，含 `gcc/objcopy`）
- QEMU（`qemu-system-riscv64`）
- `make`、`python3`

可用以下命令快速检查：
```bash
riscv64-unknown-elf-gcc --version
qemu-system-riscv64 --version
python3 --version
```

### 构建与运行
在仓库根目录执行：
```bash
# 1) 构建固件（M 模式）
make -C firmware CROSS=riscv64-unknown-elf-

# 2) 构建内核（S 模式）
make -C kernel CROSS=riscv64-unknown-elf-

# 3) 生成内核磁盘镜像
python3 tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img

# 4) 生成/格式化 FAT32 镜像（作为第二块盘）
python3 tools/mkfs_fat32.py --out build/fs.img --size 128M --root user/rootfs --force

# 5) 构建用户态
make -C user install

# 6) 启动 QEMU
/opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1
```

说明：
- `disk.img` 是固件读取的内核镜像盘，`fs.img` 作为 FAT32 根文件系统盘。
- 如果只接入一块盘，内核会回退使用 `hd0` 作为根盘。
