# Stage2 验证（Pending）

注意：WSL 启动噪声已省略。QEMU 日志仅保留相关片段。

## 环境
命令：
```bash
wsl -- bash -lc 'uname -a'
```
输出：
```bash
Linux tjr 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
```
预期/结论：
- WSL 内核版本已记录。

命令：
```bash
wsl -- bash -lc 'cat /etc/os-release'
```
输出：
```bash
NAME="Ubuntu"
VERSION="20.04.6 LTS (Focal Fossa)"
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu 20.04.6 LTS"
VERSION_ID="20.04"
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
VERSION_CODENAME=focal
UBUNTU_CODENAME=focal
```
预期/结论：
- WSL 发行版信息已记录。

命令：
```bash
wsl -- bash -lc '/opt/riscv/bin/riscv64-unknown-elf-gcc --version | head -n 1'
```
输出：
```bash
riscv64-unknown-elf-gcc (gc891d8dc23e) 13.2.0
```
预期/结论：
- 工具链版本已记录。

命令：
```bash
wsl -- bash -lc '/opt/qemu/bin/qemu-system-riscv64 --version | head -n 1'
```
输出：
```bash
QEMU emulator version 7.2.0
```
预期/结论：
- QEMU 版本已记录。

## DTB 导出
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && /opt/qemu/bin/qemu-system-riscv64 -machine virt,dumpdtb=build/virt.dtb -m 1024M -smp 4 -nographic -bios none'
```
输出：
```bash
qemu-system-riscv64: info: dtb dumped to build/virt.dtb. Exiting.
```
预期/结论：
- 已生成 virt 的 DTB，并用于后续 QEMU 运行。

## 根盘回退（root_disk_index=1，仅挂载 hd0）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && rm -f kernel/*.o kernel/kernel.elf kernel/kernel.bin && make -s -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf- EXTRA_CFLAGS=-DCONFIG_STAGE2_TEST'
```
输出：
```bash
/opt/riscv/lib/gcc/riscv64-unknown-elf/13.2.0/../../../../riscv64-unknown-elf/bin/ld: warning: kernel.elf has a LOAD segment with RWX permissions
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img'
```
输出：
```bash
(no output)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 5s /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb /mnt/d/Workspace/vide_coding/agent_os/build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0'
```
输出（节选）：
```bash
[INFO] blk: devices=1 requested=1 selected=0 fallback
[INFO] blk: root virtio-blk base=0x10008000 capacity=274 sectors
```
预期/结论：
- 仅有一个设备时，根盘选择回退到索引 0。PASS。

代码片段（`kernel/blk.c`）：
```c
  uint32_t selected = root_index;
  int fallback = 0;
  if (selected >= blk_count) {
    selected = 0;
    fallback = 1;
  }
#ifdef CONFIG_STAGE2_TEST
  log_info("blk: devices=%lu requested=%u selected=%u%s",
           (uint64_t)blk_count,
           root_index,
           selected,
           fallback ? " fallback" : "");
#endif
```

## virtio-blk 读写自测
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && rm -f build/blk_test.img && dd if=/dev/zero of=build/blk_test.img bs=1M count=4'
```
输出：
```bash
4+0 records in
4+0 records out
4194304 bytes (4.2 MB, 4.0 MiB) copied, 0.0192829 s, 218 MB/s
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && rm -f kernel/*.o kernel/kernel.elf kernel/kernel.bin && make -s -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf- EXTRA_CFLAGS=-DCONFIG_STAGE2_TEST\ -DCONFIG_STAGE2_TEST_BLK'
```
输出：
```bash
/opt/riscv/lib/gcc/riscv64-unknown-elf/13.2.0/../../../../riscv64-unknown-elf/bin/ld: warning: kernel.elf has a LOAD segment with RWX permissions
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img'
```
输出：
```bash
(no output)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 5s /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb /mnt/d/Workspace/vide_coding/agent_os/build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/blk_test.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1'
```
输出（节选）：
```bash
[INFO] blk: devices=2 requested=1 selected=1
[INFO] selftest: blk rw start
[INFO] selftest: blk rw pass
```
预期/结论：
- 块读写自测通过。PASS。

代码片段（`kernel/selftest.c`）：
```c
void selftest_run_blk(void) {
  log_info("selftest: blk rw start");
  if (selftest_blk_rw() == 0) {
    log_info("selftest: blk rw pass");
  } else {
    log_error("selftest: blk rw fail");
  }
}

static int selftest_blk_rw(void) {
  uint8_t orig[512];
  uint8_t pattern[512];
  uint8_t readback[512];

  if (blk_read(1, orig, 1) != 0) {
    return -1;
  }
  for (size_t i = 0; i < sizeof(pattern); i++) {
    pattern[i] = (uint8_t)(0xA5U ^ (uint8_t)i);
  }
  if (blk_write(1, pattern, 1) != 0) {
    return -1;
  }
  if (blk_read(1, readback, 1) != 0) {
    return -1;
  }
  if (memcmp(pattern, readback, sizeof(pattern)) != 0) {
    return -1;
  }
  if (blk_write(1, orig, 1) != 0) {
    return -1;
  }
  return 0;
}
```

## FAT32 镜像工具（MBR + /ram）
注意：辅助脚本位于 `verify/verify_fs.py`。
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && rm -f build/fs_test.img && python3 tools/mkfs_fat32.py --out build/fs_test.img --size 128M'
```
输出：
```bash
mkfs: created build/fs_test.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 verify/verify_fs.py --image build/fs_test.img'
```
输出：
```bash
MBR_SIG=0xaa55 TYPE=0x0c LBA_START=2048 SECTORS=260096
BPB_SIG=0xaa55 BPS=512 SPC=8 RESERVED=32 FATS=2 SPF=254 ROOT=2 DATA_LBA=2588
ROOT_ENTRY_NAME=RAM         ATTR=0x10 CLUSTER=3
```
预期/结论：
- MBR 签名、FAT32 BPB、根目录 /ram 条目验证通过。PASS。

## FAT32 读写 + VFS + RAMFS
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && rm -f kernel/*.o kernel/kernel.elf kernel/kernel.bin && make -s -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf- EXTRA_CFLAGS=-DCONFIG_STAGE2_TEST\ -DCONFIG_STAGE2_TEST_FS'
```
输出：
```bash
/opt/riscv/lib/gcc/riscv64-unknown-elf/13.2.0/../../../../riscv64-unknown-elf/bin/ld: warning: kernel.elf has a LOAD segment with RWX permissions
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img'
```
输出：
```bash
(no output)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 5s /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb /mnt/d/Workspace/vide_coding/agent_os/build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_test.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1'
```
输出（节选）：
```bash
[INFO] selftest: vfs basic pass
[INFO] selftest: fat32 ops pass
[INFO] selftest: ramfs ops pass
```
预期/结论：
- FAT32 读写 + VFS 基础 + RAMFS 操作通过。PASS。

代码片段（`kernel/selftest.c`）：
```c
void selftest_run_fs(void) {
  log_info("selftest: vfs basic start");
  if (selftest_vfs_basic() == 0) {
    log_info("selftest: vfs basic pass");
  }

  log_info("selftest: fat32 ops start");
  if (selftest_fat32_ops() == 0) {
    log_info("selftest: fat32 ops pass");
  }

  log_info("selftest: ramfs ops start");
  if (selftest_ramfs_ops() == 0) {
    log_info("selftest: ramfs ops pass");
  }
}

static int selftest_vfs_basic(void) {
  if (selftest_expect(vfs_mkdir("/TestDir/Sub", NULL) == 0, "vfs mkdir nested") != 0) {
    return -1;
  }
  if (selftest_expect(vfs_create("/AutoDir/Sub/auto.txt", NULL) == 0, "vfs create auto parents") != 0) {
    return -1;
  }
  return 0;
}
```

## Pending
- 进程/VM/syscall 基础验证（Pending；等待 /bin/sh 就绪）。

## Skipped
- stdin 回显/退格验证。