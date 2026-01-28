# Stage4 ??

## 阶段新增与修复（相对 Stage3）
- 修复：`userret` 过程中可能被中断抢占导致内核崩溃的问题（清除 `SSTATUS_SIE`）。
- Shell 增强：支持 Tab 补全；未实现命令统一输出 `<instruction> not implemented`。
- 新增用户态命令：`/bin/echo`、`/bin/rm`、`/bin/poweroff`。
- 关机链路打通：内核 `reboot` → SBI SRST → 固件写 `sifive_test` 关机。
- DTB 回退路径增强：内核/固件在 DTB 缺失时改为固定 virtio-mmio 扫描，且顺序与 DTB 枚举一致。
- Makefile 改进（面向 Linux/WSL）：`clean` 使用 `rm -f`；自动检测 `-march` 是否支持 `_zicsr`；新增 `dtb` 规则；顶层 `run` 使用标准镜像名。

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

## Makefile 与工具链验证
### DTB 规则（make dtb）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make dtb QEMU=/opt/qemu/bin/qemu-system-riscv64 && ls -l build/virt.dtb'
```
输出：
```bash
mkdir -p build
timeout 3s /opt/qemu/bin/qemu-system-riscv64 -machine virt,dumpdtb=build/virt.dtb -m 1024M -smp 4 -nographic -bios none >/dev/null 2>&1 || true
-rwxrwxrwx 1 tjr tjr 1048576 Jan 27 22:17 build/virt.dtb
```
预期/结论：
- 成功生成 `build/virt.dtb`。PASS。

代码片段（`Makefile`）：
```makefile
dtb:
	mkdir -p build
	-timeout 3s $(QEMU) -machine virt,dumpdtb=$(DTB) -m 1024M -smp 4 -nographic -bios none >/dev/null 2>&1 || true
```

### `_zicsr` 自动检测（内核构建）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make -C kernel clean && make -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf-'
```
输出（节选）：
```bash
make: Entering directory '/mnt/d/Workspace/vide_coding/agent_os/kernel'
/opt/riscv/bin/riscv64-unknown-elf-gcc ... -march=rv64gc_zicsr -mabi=lp64 ... -c -o boot.o boot.c
/opt/riscv/bin/riscv64-unknown-elf-gcc ... -march=rv64gc_zicsr -mabi=lp64 ... -c -o main.o main.c
```
预期/结论：
- 构建命令中自动选择了 `-march=rv64gc_zicsr`，说明 `_zicsr` 检测生效。PASS。

代码片段（`kernel/Makefile`）：
```makefile
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
```

### 顶层 make（WSL 流程）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make CROSS=/opt/riscv/bin/riscv64-unknown-elf- PYTHON=python3'
```
输出（节选）：
```bash
make -C firmware CROSS=/opt/riscv/bin/riscv64-unknown-elf-
make -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf-
python3 tools/mkimage.py --kernel kernel/kernel.elf --out build/disk.img
make -C user install CROSS=/opt/riscv/bin/riscv64-unknown-elf-
python3 tools/mkfs_fat32.py --out build/fs.img --size 128M --root user/rootfs --force
mkfs: created build/fs.img (134217728 bytes)
```
预期/结论：
- 顶层构建流程在 WSL 下可直接运行完成。PASS。

代码片段（`Makefile`）：
```makefile
firmware:
	$(MAKE) -C firmware CROSS=$(CROSS) $(ARCH_ARG)

kernel:
	$(MAKE) -C kernel CROSS=$(CROSS) $(ARCH_ARG)

user:
	$(MAKE) -C user install CROSS=$(CROSS) $(ARCH_ARG)
```

## Shell 与系统调用验证
### 命令测试：echo / rm / 未实现提示 / poweroff
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 40s bash -c "(sleep 1; python3 verify/feed_input.py --input build/stage4_input_cmds.txt --delay 0.15) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage4.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage4.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
[INFO] init: exec /bin/sh
root@qemu:/$ pwd
/
root@qemu:/$ ls /bin
./ ../ cat echo ls memtest mkdir mkhello picoc poweroff rm sh tcc touch vi vim
root@qemu:/$ echo hello world
hello world
root@qemu:/$ touch /tmp/a
root@qemu:/$ ls /tmp
./ ../ a
root@qemu:/$ rm /tmp/a
root@qemu:/$ ls /tmp
./ ../
root@qemu:/$ foobar
foobar not implemented
root@qemu:/$ poweroff
[INFO] reboot: shutdown
```
预期/结论：
- `echo`、`rm` 行为正确。
- 未实现命令输出 `<instruction> not implemented`。
- `poweroff` 触发关机链路并退出 QEMU。PASS。

代码片段（`user/src/poweroff.c`，`kernel/syscall.c`，`firmware/mtrap.c`）：
```c
/* user/src/poweroff.c */
int main(void) {
  return reboot(0xfee1dead, 672274793U, 0x4321fedcU, 0);
}

/* kernel/syscall.c */
static long sys_reboot(uint64_t magic1, uint64_t magic2, uint64_t cmd, uint64_t arg) {
  (void)magic1; (void)magic2; (void)cmd; (void)arg;
  log_info("reboot: shutdown");
  sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_REASON_NONE);
  for (;;) { __asm__ volatile("wfi"); }
}

/* firmware/mtrap.c */
} else if (eid == SBI_EXT_SRST) {
  sbi_system_reset((uint32_t)tf->a0);
  tf->a0 = 0;
  tf->a1 = 0;
}
```

### Tab 补全（cd /bi<Tab>, ls /bi<Tab>）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 40s bash -c "(sleep 1; python3 verify/feed_input.py --input build/stage4_input_tab.txt --delay 0.15) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage4.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage4.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ cd /bin/
root@qemu:/bin$ pwd
/bin
root@qemu:/bin$ ls /bin/
./ ../ cat echo ls memtest mkdir mkhello picoc poweroff rm sh tcc touch vi vim
root@qemu:/bin$ poweroff
[INFO] reboot: shutdown
```
预期/结论：
- Tab 将 `/bi` 自动补全为 `/bin/`，`cd` 与 `ls` 都生效。PASS。

代码片段（`user/src/sh.c`）：
```c
if (c == '\t') {
  buf[len] = '\0';
  size_t start = len;
  while (start > 0 && !isspace((unsigned char)buf[start - 1])) {
    start--;
  }
  /* 解析 token，确定扫描目录（命令位默认 /bin），再做匹配与补全 */
  ...
}
```

## DTB 缺失回退（自测）
### 强制 dtb=0 构建（宏开关）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make -C firmware clean && make -C firmware CROSS=/opt/riscv/bin/riscv64-unknown-elf- EXTRA_CFLAGS=-DCONFIG_FORCE_DTB_NULL'
```
输出（节选）：
```bash
make: Entering directory '/mnt/d/Workspace/vide_coding/agent_os/firmware'
/opt/riscv/bin/riscv64-unknown-elf-gcc ... -DCONFIG_FORCE_DTB_NULL   -c -o main.o main.c
/opt/riscv/bin/riscv64-unknown-elf-gcc ... -DCONFIG_FORCE_DTB_NULL -T linker.ld ... -o firmware.elf ...
```
预期/结论：
- 固件可通过宏强制走“无 DTB”路径，便于回退逻辑验证。PASS。

代码片段（`firmware/main.c`）：
```c
#ifdef CONFIG_FORCE_DTB_NULL
  fw_printf("fw: forcing dtb=null for selftest\n");
  dtb_ptr = 0;
#endif
```

### 启动与挂载验证（dtb=0）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 40s bash -c "(sleep 1; python3 verify/feed_input.py --input build/stage4_input_cmds.txt --delay 0.15) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage4.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage4.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
[INFO] kernel: boot hart=0 dtb=0x0 bootinfo=0x80010000
[INFO] blk: dtb missing, using fixed virtio-mmio list
[INFO] blk: root virtio-blk base=0x10007000 capacity=262144 sectors
[INFO] fat32: part_lba=2048 part_sectors=260096 total=260096 spc=8 fat=254 root=2
[INFO] init: exec /bin/sh
```
预期/结论：
- 即使 DTB 不可用，内核仍能通过固定 virtio 枚举找到正确根盘并挂载 FAT32。PASS。
- 注：该测试需要先构建带宏的固件；测试完成后应重新构建正常固件。

代码片段（`firmware/main.c`，`kernel/blk.c`）：
```c
/* firmware/main.c */
for (int i = 7; i >= 0 && count < max_out; i--) {
  uint64_t base = VIRTIO0_BASE + (uint64_t)(uint32_t)i * 0x1000U;
  ...
}

/* kernel/blk.c */
for (int i = 7; i >= 0 && count < max_out; i--) {
  uint64_t base = VIRTIO0_BASE + (uint64_t)(uint32_t)i * 0x1000U;
  ...
}
```

## echo ??? + cp ??
???
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && (sleep 1; python3 verify/feed_input.py --input build/stage4_input_echo_cp.txt --delay 0.15) | /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=/tmp/agent_os_disk.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=/tmp/agent_os_fs.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1'
```
???????
```bash
root@qemu:/$ echo hello > /tmp/echo.txt
root@qemu:/$ echo world >> /tmp/echo.txt
root@qemu:/$ cat /tmp/echo.txt
hello
world
root@qemu:/$ cp /tmp/cpdir/a.txt /tmp/cpdir/b.txt /tmp/cpout
cp: overwrite '/tmp/cpout/a.txt'? [y/N] y
root@qemu:/$ cp /tmp/cpdir /tmp/cpdir2
root@qemu:/$ ls /tmp/cpout
./ ../ a.txt b.txt
root@qemu:/$ ls /tmp/cpdir2
./ ../ a.txt b.txt
root@qemu:/$ cat /tmp/cpout/a.txt
data1
```
??/???
- `echo >` ????`echo >>` ??????
- `cp` ????????????????????
- ??????????PASS?
????????????
```c
/* user/src/sh.c */
if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0) {
  redir_path = argv[i + 1];
  redir_append = (argv[i][1] == '>');
  ...
}

/* user/src/cp.c */
if (is_dir(src) && recursive) {
  return copy_dir_recursive(src, dst, overwrite);
}
...
if (prompt_overwrite(dst)) {
  return copy_file(src, dst);
}
```

## vim/vi ?????i / Esc / :w / :q / :wq?
???????????????? `Esc` ? `:wq`??
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && bash build/run_vim_scripted.sh'
```
???????
```bash
root@qemu:/$ vim /tmp/vi1.txt
...??? vi?...
:wq
'/tmp/vi1.txt' 2L, 12C
root@qemu:/$ cat /tmp/vi1.txt
Hello, vim
```
??/???
- ?? `i` ???`Esc` ?????`:w` ???`:q` ???`:wq` ??????
- ?????????????PASS?
????????????
```c
/* third_party/busybox/libbb/read_key.c */
if (n > 0) {
  unsigned char next = buffer[0];
  if (next != '[' && next != 'O') {
    buffer[-1] = n;
    return 27; /* ESC */
  }
}

/* third_party/busybox/editors/vi.c */
if (cmd[0] == 'x' || cmd[1] == 'q') {
  /* allow :wq/:q to exit without multi-file guard */
  editing = 0;
}

/* user/lib/getopt.c */
if (optind == 0) {
  optind = 1;
  optpos = 0;
}
```
