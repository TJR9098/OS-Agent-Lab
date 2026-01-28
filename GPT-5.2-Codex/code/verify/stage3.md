# Stage3 验证（Pending）

## 阶段新增与修复（相对 Stage2）
- 新增 user/ 用户态目录：最小 libc、启动代码、示例程序与 user/Makefile。
- 内核作为 PID 1 自动启动 /bin/sh，初始 CWD 为 /，提示符显示 root@qemu:/path$。
- 提供基础用户命令：/bin/sh、ls、mkdir、touch、mkhello、cat、memtest、tcc。
- 引入 /bin/picoc 与 BusyBox vi（用于后续编辑/解释执行）。
- mkfs_fat32.py 支持从宿主机拷贝整个 rootfs 目录树到镜像根目录。
- 修复：/bin/sh 加载失败（根盘未包含 rootfs）的问题，并稳定 shebang 执行路径。

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

## 用户态构建与 rootfs 拷贝
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make -s -C kernel CROSS=/opt/riscv/bin/riscv64-unknown-elf-'
```
输出：
```bash
(no output)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && make -s -C user install'
```
输出：
```bash
(no output)
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
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && cp -f build/disk.img build/disk_stage3.img'
```
输出：
```bash
(no output)
```
预期/结论：
- user/ 程序构建完成，rootfs 已拷贝进 FAT32 镜像；复制内核盘用于测试。PASS。

代码片段（`tools/mkfs_fat32.py`）：
```python
def copy_tree(fatimg: Fat32Image, parent: DirState, host_dir: str, skip_name: str = None) -> None:
    names = sorted(os.listdir(host_dir), key=lambda s: s.lower())
    for name in names:
        if name in [".", ".."]:
            continue
        if skip_name and name.lower() == skip_name.lower():
            continue
        ensure_ascii_name(name)
        src = os.path.join(host_dir, name)
        if os.path.isdir(src):
            child = fatimg.mkdir(parent, name, parent.cluster)
            copy_tree(fatimg, child, src)
        else:
            with open(src, "rb") as f:
                data = f.read()
            fatimg.add_file(parent, name, data)
```

## /bin/sh 自动启动
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 8s stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1'
```
输出（节选）：
```bash
[INFO] vfs: root directory
[INFO] vfs: ./
[INFO] vfs: ../
[INFO] vfs: ram/
[INFO] vfs: bin/
[INFO] vfs: include/
[INFO] vfs: lib/
[INFO] vfs: root/
[INFO] vfs: tmp/
[INFO] init: exec /bin/sh
root@qemu:/$
```
预期/结论：
- 内核自动启动 /bin/sh，提示符出现。PASS。

代码片段（`kernel/main.c`）：
```c
  proc_init();
  if (root) {
    struct proc *init = proc_alloc();
    if (init) {
      char *argv[] = {"sh", NULL};
      int ret = proc_exec_on(init, "/bin/sh", argv);
      if (ret == 0) {
        init->state = PROC_RUNNABLE;
        log_info("init: exec /bin/sh");
        scheduler();
      }
    }
  }
```

## 内建 cd/pwd/getcwd
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 14s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_pwd.txt --delay 0.2) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ pwd
/
root@qemu:/$ cd /ram
root@qemu:/ram$ pwd
/ram
root@qemu:/ram$ cd
root@qemu:/$ pwd
/
root@qemu:/$ exit 0
```
预期/结论：
- pwd 输出当前路径，cd 无参数回到根目录；提示符带路径。PASS。

代码片段（`user/src/sh.c`）：
```c
static void print_prompt(void) {
  char cwd[256];
  char path[256];
  const char *out = "?";
  if (getcwd(cwd, sizeof(cwd))) {
    if (strncmp(cwd, "/root", 5) == 0 && (cwd[5] == '\0' || cwd[5] == '/')) {
      path[0] = '~';
      strncpy(path + 1, cwd + 5, sizeof(path) - 2);
      path[sizeof(path) - 1] = '\0';
      out = path;
    } else {
      strncpy(path, cwd, sizeof(path) - 1);
      path[sizeof(path) - 1] = '\0';
      out = path;
    }
  }
  printf("root@qemu:%s$ ", out);
}
```

## /bin/ls 目录/文件与多路径显示
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 18s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_ls_combo.txt --delay 0.2) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ ls /bin /
./ ../ cat ls memtest mkdir mkhello picoc sh tcc touch vi vim
./ ../ bin/ include/ lib/ ram/ root/ tmp/
root@qemu:/$ ls /bin/sh
sh
root@qemu:/$ exit 0
```
预期/结论：
- 目录项一行空格分隔、目录带 /，多路径分行输出；文件仅显示 basename。PASS。

代码片段（`user/src/ls.c`）：
```c
static void print_entry(const struct entry *ent, int *first) {
  if (!ent || !ent->name) {
    return;
  }
  if (!*first) {
    printf(" ");
  }
  printf("%s%s", ent->name, (ent->type == DT_DIR) ? "/" : "");
  *first = 0;
}
```

## mkdir/touch 行为
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 20s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_mkdir_touch.txt --delay 0.2) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ mkdir /demo/dir1
root@qemu:/$ mkdir /demo/dir1
root@qemu:/$ touch /demo/dir1/file1
root@qemu:/$ touch /demo/dir1/file1
root@qemu:/$ mkdir /demo/dir1/file1
mkdir: /demo/dir1/file1 failed (-1)
root@qemu:/$ touch /demo
touch: /demo is a directory
root@qemu:/$ touch /no_parent/aaa
touch: parent missing for /no_parent/aaa
root@qemu:/$ ls /demo/dir1
./ ../ file1
root@qemu:/$ exit 0
```
预期/结论：
- mkdir 已存在目录成功；目标为文件时报错。touch 对目录与缺失父目录报错。PASS。

代码片段（`user/src/mkdir.c`）：
```c
  for (int i = 1; i < argc; i++) {
    int ret = mkdirat(AT_FDCWD, argv[i]);
    if (ret != 0) {
      dprintf(2, "mkdir: %s failed (%d)\n", argv[i], ret);
      return 1;
    }
  }
```

代码片段（`user/src/touch.c`）：
```c
    if (parent_exists(argv[i]) != 0) {
      dprintf(2, "touch: parent missing for %s\n", argv[i]);
      return 1;
    }

    int fd = openat(AT_FDCWD, argv[i], O_WRONLY | O_CREAT);
    if (fd < 0) {
      dprintf(2, "touch: %s failed (%d)\n", argv[i], fd);
      return 1;
    }
```

## mkhello + cat
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 20s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_mkhello.txt --delay 0.3) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ mkhello
mkhello: wrote /test/hello_world.c
root@qemu:/$ mkhello
mkhello: wrote /test/hello_world.c
root@qemu:/$ cat /test/hello_world.c
#include <stdio.h>
int main(){ printf("Hello, world!\n"); return 0; }
root@qemu:/$ exit 0
```
预期/结论：
- mkhello 覆盖写入示例内容，cat 输出与示例一致。PASS。

代码片段（`user/src/mkhello.c`）：
```c
  const char *dir = "/test";
  const char *path = "/test/hello_world.c";
  const char *content = "#include <stdio.h>\n"
                        "int main(){ printf(\"Hello, world!\\n\"); return 0; }\n";

  if (ensure_dir(dir) != 0) {
    dprintf(2, "mkhello: create %s failed\n", dir);
    return 1;
  }

  int fd = openat(AT_FDCWD, path, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd < 0) {
    dprintf(2, "mkhello: open %s failed (%d)\n", path, fd);
    return 1;
  }
```

## memtest（malloc/free/realloc）
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 16s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_memtest.txt --delay 0.2) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ memtest
memtest: ok
root@qemu:/$ exit 0
```
预期/结论：
- 用户态 malloc/free/realloc 工作正常。PASS。

代码片段（`user/src/memtest.c`）：
```c
  char *a = (char *)malloc(a_len);
  char *b = (char *)malloc(b_len);
  char *c = (char *)malloc(c_len);
  if (!a || !b || !c) {
    dprintf(2, "memtest: malloc failed\n");
    return 1;
  }
```

## /bin/tcc 运行与 -o 输出
命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && python3 tools/mkfs_fat32.py --out build/fs_stage3.img --size 128M --root user/rootfs --force'
```
输出：
```bash
mkfs: created build/fs_stage3.img (134217728 bytes)
```

命令：
```bash
wsl -- bash -lc 'cd /mnt/d/Workspace/vide_coding/agent_os && timeout 30s bash -c "(sleep 2; python3 verify/feed_input.py --input build/stage3_input_tcc.txt --delay 0.4) | stdbuf -oL -eL /opt/qemu/bin/qemu-system-riscv64 -machine virt -m 1024M -smp 4 -nographic -dtb build/virt.dtb -global virtio-mmio.force-legacy=false -bios firmware/firmware.bin -drive file=build/disk_stage3.img,format=raw,if=none,id=hd0 -device virtio-blk-device,drive=hd0 -drive file=build/fs_stage3.img,format=raw,if=none,id=hd1 -device virtio-blk-device,drive=hd1"'
```
输出（节选）：
```bash
root@qemu:/$ mkhello
mkhello: wrote /test/hello_world.c
root@qemu:/$ tcc -I /include /test/hello_world.c
Hello, world!
root@qemu:/$ tcc -I /include -o /test/hello /test/hello_world.c
root@qemu:/$ ls /test
./ ../ hello hello_world.c
root@qemu:/$ cat /test/hello
#!/bin/picoc -I/include
#include <stdio.h>
int main(){ printf("Hello, world!\n"); return 0; }
root@qemu:/$ exit 0
```
预期/结论：
- tcc 可直接运行 .c（通过 picoc），-o 生成可执行脚本文件。PASS。

代码片段（`user/src/tcc.c`）：
```c
  if (out) {
    int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
      dprintf(2, "tcc: open %s failed (%d)\n", out, fd);
      return 1;
    }
    const char *interp = "/bin/picoc";
    int n = snprintf(shebang, sizeof(shebang), "#!%s\n", interp);
    if (write(fd, shebang, strlen(shebang)) < 0) {
      close(fd);
      return 1;
    }
    int ret = copy_file(fd, input);
    close(fd);
    return ret < 0 ? 1 : 0;
  }

  char *exec_argv[MAX_ARGS];
  exec_argv[0] = "/bin/picoc";
  execve("/bin/picoc", exec_argv, NULL);
```

## Pending
- 抢占式 round-robin 的长期公平性压力验证（需更多并发样例）。

## Skipped
- unlinkat 行为验证（无 rm 工具）。
- fstat/fstatat 独立验证（未提供单独测试程序）。
- vi/vim 交互验证（需人工交互）。
- stdin 退格/删除回显验证（需人工交互）。


