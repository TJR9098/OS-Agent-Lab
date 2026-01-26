# Agent OS Prompt Pack（基于 `export.csv` 提取整理）

**Session ID**：`019bd519-f56b-76a3-b2f1-4c4b5daa6ab4`  
**来源**：Codex 交互日志（CSV 导出）  
**目标**：把该 Session 中与实现进度相关的需求与约束，按 **Stage** 归档为可直接复制使用的 Prompt。

> 统一原则：每个 Stage 结束前必须 **编译 + 运行 + 验证通过**，否则不得宣告完成，也不得进入下一阶段。

---

## 全局约束（所有 Stage 通用）

### 目标与环境

- 目标环境：**QEMU `virt`**
- ISA：**RV64**
- 支持多核（SMP）
- 固件可由 QEMU `-bios` 直接加载
- 早期阶段允许依赖 DTB 解析设备地址
- 优先支持：**计时器、块设备**
- 文件系统路线：**先 RAMFS，后 FAT32**
- 需要可控的日志级别

### 交付要求

- 每个 Stage 必须提供：
  1. **实现内容**（代码）
  2. **可复现的运行命令**
  3. **验证证据**（日志/脚本输出/文档记录）
- 若运行验证失败，必须先修复，再继续推进。

### 强制停止条件（Stop Condition）

只有在满足以下条件时，才能回复“完成”并停止：

- `make` 成功
- `make run` 能启动并得到**预期输出**
- 若存在 `verify/` 验证脚本/文档，必须执行并通过（输出 PASS / 结果为 0）

---

# Stage 1：VFS + RAMFS + FAT32（可写）基础能力

> 该 Stage 的目标是让内核具备“可用文件系统”的最小闭环：**块设备 → FAT32/RAMFS → VFS 路径解析 → 基本文件操作**。

## Stage 1 Prompt（可直接复制给 Agent）

```text
你是一个 RISC-V OS 工程 Agent。请在现有仓库基础上实现 Stage 1：VFS + RAMFS + FAT32（可写）基础能力。

【目标】
1) 块设备：能通过 virtio-blk 完成基础读写，支持选择根盘：
   - 使用 bootinfo.root_disk_index 选择根盘
   - 越界时回退到 0
2) FAT32（可写）：从只读扩展到可写，至少实现：
   - LFN（长文件名）与 8.3 名称生成/兼容
   - create / mkdir / unlink / truncate / write / read
   - 创建目录时自动写入 ./.. 
   - lookup 不区分大小写
3) VFS（简化但可用）：
   - 支持挂载点、CWD、路径解析
   - 支持 *_at 接口（openat/mkdirat/unlinkat 等风格）
   - 支持“自动创建父目录”（按需求约定）
   - readdir 输出注入 ./..
   - 支持 getcwd
4) RAMFS：
   - 支持读写、mkdir/create/unlink/truncate

【实现约束】
- 修改应集中在 kernel/fs 相关模块（如 fat32.c、vfs.c、file.h/vfs.h 等）
- 尽量保持接口清晰：FAT32/RAMFS 作为具体 FS，VFS 作为统一抽象
- 需要提供可复现的验证步骤与输出关键字

【验证要求（必须执行）】
- 运行：
  - make
  - make run
- 验证：
  - 至少证明：blk rw 正常、FAT32 写入可读回、RAMFS 正常、路径解析正常
  - 输出中必须出现明确 PASS 标记（例如 STAGE1 PASS），或提供 verify 脚本输出为 0

【停止条件】
只有当上述验证全部通过，且你提供了：运行命令 + 关键输出（或日志路径）后，才允许回复“Stage 1 完成”。
否则继续排查与修复，不得停止。
```

---

# Stage 2：自测框架 + Stage2 验证文档固化

> 该 Stage 的目标是让 Stage1 的文件系统能力“可证明、可回归”。核心交付是：**自测框架 + verify 脚本 + 文档归档**。

## Stage 2 Prompt（可直接复制给 Agent）

```text
你是一个 RISC-V OS 工程 Agent。请在现有仓库基础上实现 Stage 2：自测框架 + Stage2 验证文档固化，并修复所有阻塞验证的问题。

【目标】
1) 自测框架：
   - 新增并接入最小 selftest 框架（例如 kernel/selftest.c + include/selftest.h）
   - 可通过编译开关启用/关闭（例如 CONFIG_STAGE2_TEST）
   - 自测失败时必须输出足够诊断信息（返回值、关键缓冲内容等）
2) 修复编译/链接与行为缺陷：
   - 修复 FAT32 读写返回值、LFN 结构体打包、元数据刷新等问题
   - 补齐缺失头文件/常量引用，确保 clean build
   - 补齐构建项（firmware/kernel 的 march/abi、zicsr 等）
3) 验证归档（强制）：
   - 新增或完善 verify/stage2.md（记录环境、命令、输出、结论）
   - 提供 stage2 中文版（verify/stage2_cn.md）
   - 将验证脚本收敛到 verify/ 目录（例如 verify/verify_fs.py）

【必须覆盖的验证项】
- DTB dump（或等价方式证明 DTB 可用）
- root disk index 越界回退行为
- virtio-blk 读写
- FAT32/VFS/RAMFS 自测全部 PASS

【验证要求（必须执行）】
- make
- make run
- 运行 verify 脚本（若存在）并确保结果为 PASS/退出码 0
- 将所有命令与输出写入 verify/stage2.md（含结论）

【停止条件】
只有当：
- Stage2 自测通过
- 文档与脚本已归档在 verify/
- 你给出可复现命令与 PASS 证据
才允许回复“Stage 2 完成”。
否则继续修复。
```

---

# Stage 3：用户态 + `/bin/sh` + Stage3 验证文档

> 该 Stage 目标是完成“用户态可用闭环”：**内核启动 → PID 1 shell → fork/exec/wait → /bin 工具集 → FAT32 镜像携带 rootfs**。

## Stage 3 Prompt（可直接复制给 Agent）

```text
你是一个 RISC-V OS 工程 Agent。请实现 Stage 3：用户态 + /bin/sh + Stage3 验证文档，并确保全流程可复现与可回归。

【目标】
1) FAT32 镜像 rootfs 注入：
   - 扩展 tools/mkfs_fat32.py，支持 --root 把宿主机 rootfs 拷贝进镜像
   - 镜像内至少包含 /bin 目录与可执行程序
2) 内核自动启动用户态：
   - 内核启动后自动 exec /bin/sh（作为 PID 1）
   - 启动过程应打印明确日志，便于 verify 判定
3) /bin/sh 基本功能（最小可用交互）
   - 固定提示符：`$ `
   - 空白分隔（无需引号/管道/重定向）
   - PATH 固定为 /bin
   - 内建命令：cd / pwd / exit
   - 外部命令通过 fork/exec/wait 运行
4) /bin 工具集（按需实现）
   - /bin/ls：支持可选路径；输出包含 `.` 与 `..`；目录名后加 `/`；按显示名不区分大小写排序
   - /bin/mkdir：多参数；自动创建父目录；目录已存在返回成功；遇错停止
   - /bin/touch：多参数；文件已存在返回成功；目录报错；父目录不存在报错
   - /bin/mkhello：创建/覆盖写入 `/test/hello_world.c`
   - 可选：显式验证 malloc/free/realloc（可通过专用小程序或间接验证）
5) Stage3 文档归档：
   - 编写 verify/stage3.md（格式可模仿 stage2_cn）
   - 每个测试段落包含：目的、命令、预期输出、实际输出、结论

【验证要求（必须执行）】
- make
- make run
- 验证点至少包含：
  - 内核自动启动 /bin/sh（PID 1）
  - shell 可交互、可执行外部命令
  - mkdir/touch/ls/mkhello 行为符合预期
  - mkhello 写入内容若要求可读回验证
- 若允许自动化测试，可使用脚本向 QEMU stdin 投喂命令并记录输出（例如 here-doc / expect / pexpect）

【停止条件】
只有当：
- Stage3 全部测试 PASS
- verify/stage3.md 写明命令 + 输出证据
- 你给出完整复现步骤
才允许回复“Stage 3 完成”。
否则继续修复与补齐验证。
```

---

## 参考（方法论）

- GitHub README 组织建议：项目用途、如何使用、运行指南等应明确写在 README 中。
- OpenAI Cookbook：Eval + Run 的结构适合做 Prompt/Agent 实验与回归验证。  
- Prompt 鲁棒性与评测闭环（Evaluation Flywheel）：适合把“必须验证才算完成”固化成流程。