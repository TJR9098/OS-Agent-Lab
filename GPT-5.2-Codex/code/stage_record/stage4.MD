# Stage4 阶段改动记录

本文记录从 Stage4 开始到当前的所有主要改动与验证结果（以代码与行为为准）。

## 总览
- 完成 /bin/sh 的交互体验增强（提示符、重定向、内建命令），并补充常用用户态工具（echo/cp/rm/poweroff）。
- 让 BusyBox 的 vi/vim 可用（i 插入、Esc 退出插入、:w/:q/:wq）。
- 增加用户态最小 libc 支撑（poll/errno/getopt 等），便于编辑器与交互工具运行。
- 修复 /ram 挂载失效问题，使 /ram 真正变为 RAMFS（易失）。
- 增强 FDT 解析健壮性，避免异常 DTB 导致内核异常。

## 代码改动（按目录/模块）
### kernel/
- VFS 挂载匹配逻辑修复：让 /ram 目录正确命中 RAMFS（根目录下同名目录大小写不敏感匹配）。
  - 影响文件：`kernel/vfs.c`
- FDT 解析增加 totalsize/offset/strings 边界检查，避免非法 DTB 触发内核异常并保持 blk 回退路径可用。
  - 影响文件：`kernel/fdt.c`
- 新增/补充终端输入与轮询能力（供用户态 vi 等交互程序使用）。
  - 影响文件：`kernel/syscall.c`, `kernel/include/syscall.h`, `kernel/uart.c`, `kernel/include/uart.h`, `kernel/timer.c`, `kernel/include/timer.h`

### user/
- /bin/sh 支持重定向（> / >>）、命令执行流程优化，内建命令与提示符风格对齐需求。
  - 影响文件：`user/src/sh.c`
- 新增用户态工具：
  - `echo`：用于重定向测试与脚本输出。
  - `cp`：支持递归、多个源文件、覆盖提示。
  - `rm`：支持删除文件/空目录（与系统调用配合）。
  - `poweroff`：调用 reboot 实现关机。
  - 影响文件：`user/src/echo.c`, `user/src/cp.c`, `user/src/rm.c`, `user/src/poweroff.c`
- 最小 libc 支持增强：
  - `poll` 系统调用封装。
  - `getopt` 行为与 BusyBox 期望对齐（重置 optind）。
  - 影响文件：`user/lib/poll.c`, `user/lib/getopt.c`

### third_party/busybox/
- 修复 ESC 键处理逻辑，使 vi 的插入/普通模式切换可用。
- 放宽多文件编辑的 :q/:wq 退出限制，便于单文件编辑流程。
  - 影响文件：`third_party/busybox/libbb/read_key.c`, `third_party/busybox/editors/vi.c`

## 行为变化说明
- `/ram` 现在是真正的 RAMFS（易失），重启后文件会消失。
- 根文件系统 `/` 为 FAT32，若不重新生成 `fs.img` 则内容持久。
- vi/vim 支持基本编辑流程：`i` 进入插入，`Esc` 退出插入，`:w` 保存，`:q` 退出，`:wq` 保存并退出。

## 验证与日志
### 1) echo 重定向 + cp 功能验证
- 日志：`build/echo_cp2.log`
- 验证点：
  - `echo >`、`echo >>` 正常工作。
  - `cp` 支持多源复制、递归复制、覆盖提示。

### 2) vi/vim 功能验证
- 日志：`build/vim_scripted5.log`
- 验证点：
  - 进入插入、退出插入、保存与退出流程可用。

### 3) /ram 易失性验证
- 日志：`build/ram_test_run1.log`, `build/ram_test_run2.log`
- 验证点：
  - 第一次启动创建 `/ram/volatile.txt`，可列出。
  - 关机后重启 `/ram` 不再存在该文件（仅有 `.` 与 `..`）。

## 备注
- 如果 `make run` 不再重建 `fs.img`，则 `/` 内容会持久保存。
- 若需要将以上验证步骤写入 `verify/stage4.md`，可继续补充。
