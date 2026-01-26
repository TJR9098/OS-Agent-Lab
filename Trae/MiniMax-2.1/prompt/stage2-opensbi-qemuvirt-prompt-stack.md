# Stage 2（OpenSBI + QEMU virt）基础内核库 / 启动 / Trap / Timer

## 目标与上下文（必须遵守）
- **目标**：在“仓库当前无任何代码文件”的前提下，从零创建 Stage 2 所需的最小内核代码与构建系统，使其能在 **QEMU virt + OpenSBI（`-bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin`）** 下启动、打印、处理中断/异常，并满足验收输出。

- **运行约束（固定）**
  - QEMU 使用 `virt` 机型，OpenSBI 作为固件：`-bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin`。
  - QEMU 使用 `-kernel kernel.elf` 将内核 **直接加载到物理地址 `0x80200000`**。
  - 默认 `-smp 1`，仅 **hart0** 继续运行（不做多核分流）。
  - 进入内核时寄存器约定：`a0 = hartid`；`a1/a2` **未定义**（内核不得读取）。
  - **禁止使用 `firmware/` 传参链路**（例如 fw_payload / bootinfo 等旁路传参方案一律禁用）。
  
- **强约束**：任何不明确处必须先提问；在未澄清前不得“拍脑袋补全”。

  - **Stop Condition（必须满足后停止运行）**

    **Agent 的“完成”仅以通过 Stage 2.5 验收为准。**

    1) **必须执行（不可跳过）**：在编译生成 `kernel.elf` 后，必须运行 `make run`（或等价命令）启动 QEMU，并采集控制台输出。
  
    2) **必须验证（不可跳过）**：必须逐项检查 Stage 2.5 中的“期望输出”是否出现、顺序是否正确（包括 5 秒 tick、异常三元组 `scause/sepc/stval`、以及 panic/停机行为）。
  
    3) **禁止提前停止**：仅“编译成功 / kernel.elf 存在 / 目标生成”不算完成；不得在未运行与未验证的情况下输出“任务完成/更新状态/总结已完成工作”。
  
    4) **无法继续时的唯一出口**：如果 run/验证失败且需要用户输入（例如 QEMU/OpenSBI 路径、平台选择、命令输出），只能输出“待用户回答的问题清单（含可复制粘贴命令）”并立即停止，等待用户回复后再继续。


---

## Clarification Gate（必须先问清楚的点）
1. **若用户主机是 Windows：先确认使用 WSL 还是原生 Windows 环境**；明确后再继续执行后续步骤。
2. **如有任何不明确或存在歧义，必须先提问并等待用户回答后再继续。**

## 总体交付物（必须创建）
你必须创建以下目录与文件（最小集），并确保可构建、可运行、可验收：

- 目录：`kernel/`、`kernel/include/`
- 启动与中断：
  - `kernel/entry.S`
  - `kernel/stack.S`
  - `kernel/trapvec.S`
  - `kernel/trap.c`
  - `kernel/main.c`
  - `kernel/timer.c`
- OpenSBI 封装：
  - `kernel/sbi.c`、`kernel/include/sbi.h`
- 打印：
  - `kernel/printf.c`、`kernel/include/printf.h`
- panic：
  - `kernel/panic.c`、`kernel/include/panic.h`
- 构建与链接：
  - `kernel/Makefile`
  - `linker.ld`（或 `kernel/linker.ld`，取决于 Clarification Gate 的确认结果）
- 基础头文件：
  - `kernel/include/riscv.h`
  - `kernel/include/memlayout.h`
  - `kernel/include/trap.h`
  - `kernel/include/timer.h`

---

# Stage 2 补充规范：内核栈编写与 linker 脚本编写

## 1) 内核栈（Kernel Stack / Boot Stack）编写规范（必须）

### 1.1 栈的基本语义（必须遵守）
- RISC-V 标准调用约定：栈向低地址增长；`sp` **必须保持 16 字节对齐**（跨函数调用必须满足）。否则可能触发 ABI 不兼容与未定义行为（例如编译器生成的栈访问假设被破坏）。
- Stage 2（无虚拟内存）下，栈是“普通物理内存中的一段连续区域”，因此必须由你在 `.bss`（或等价可写段）中显式预留。

### 1.2 栈的“最小可复现结构”（必须）
为满足“从汇编入口进入 C”与后续 trap 的稳定性，至少需要两段栈：
1) **Boot Stack**：`kernel_boot` 入口最早期使用（清 BSS 前也可用）。
2) **Kernel Stack**：进入 `kernel_main()` 后长期使用（后续 trap/中断也应使用此栈或其派生策略）。

> 由于本 Stage 固定 `-smp 1`，不需要 per-hart 栈数组，但**必须保留命名与布局扩展点**（未来多核可扩展）。

### 1.3 栈的放置与对齐（必须）
- 栈对象必须放入专用输入段（建议 `.bss.stack`），并在 linker 脚本中把该段放入 `.bss` 之前或之内，且对齐满足：
  - `boot_stack`/`kernel_stack` **至少 16 字节对齐**（建议 16 或更大）。
  - `boot_stack_top`/`kernel_stack_top` 作为“栈顶标号”必须同样满足 16 字节对齐（因为 `sp` 会被设为该地址）。 
- trap 入口地址（`stvec`）的 BASE **必须 4 字节对齐**；因此 `trapvec.S` 所在段与符号也必须保证至少 4 字节对齐。 :contentReference[oaicite:2]{index=2}

### 1.4 栈顶符号（label）对 C 的声明约定（必须）
- 所有由汇编 `.global <sym>` 或 linker script 导出的“标号符号”，在 C 里一律用 `extern char <sym>[];` 声明，仅表示地址（不可声明为 `long/int` 以免被误当作变量内容读取）。

### 1.5 启动汇编中对栈的使用顺序（必须按此顺序约束 Agent）
Agent 在 `entry.S` 中必须遵循以下顺序（关键点是“在调用任何 C 函数前保证 sp 对齐且稳定”）：
1. 入口 `kernel_boot`：立刻把 `sp` 设到 `boot_stack_top`（16B 对齐），并保存 `hartid`（a0）到临时寄存器或栈上。
2. 执行 BSS 清零（允许使用 `boot_stack`）。
3. BSS 清零完成后：把 `sp` 切换到 `kernel_stack_top`（16B 对齐）。
4. 恢复 `a0` 并跳转/调用 `kernel_main(hartid)`。

### 1.6 Trap 入口如何“保存内核栈”（必须）

> trap 发生时，必须保证在正确的内核栈上保存现场，并且可恢复。

#### 1.6.1 最小要求（Stage 2 只有 S-mode 也必须做）
- trap 入口必须在 **当前内核栈** 上分配 trapframe，并保存：
  - 全部 GPR（x1..x31）
  - `sepc`、`sstatus`
  - `usp`（即“trap 前的 sp 值/或为将来 U-mode 预留的 user sp 位”）
- 保存 `usp` 的关键点：你在 `sp -= TF_SIZE` 后，`sp` 已改变；因此必须先用临时寄存器保留“trap 前 sp”，再写入 trapframe。

#### 1.6.2 使用 `sscratch` 的“栈保存/交换”模式（建议写入提示词，便于后续 U-mode）
- `sscratch` 是“给 S-mode trap handler 用的 scratch CSR”，常用于保存/交换栈指针。
- 推荐固定约定（现在可只实现框架，Stage 2 不进 U-mode 也不冲突）：
  - **当未来进入 U-mode 前**：把 `sscratch` 设为 `kernel_stack_top`（表示“我的内核栈顶”）。
  - **trap 入口**：通过 `csrrw` 或“读 sscratch + 条件交换”的方式，确保最终 `sp` 落在内核栈。
  - **trap 退出**：恢复 sepc/sstatus，必要时交换回 user sp（未来阶段）。

> 关键点：`sscratch` 由软件约定其含义；硬件不会自动写它，因此必须在提示词里把“什么时候写 sscratch、保存什么值”讲清楚

### 1.7 最小自检与验收要求（强化 Agent 的“写对栈”能力）
在 Stage 2.5 验收中新增/强调以下检查（Agent 必须执行并验证）：
- `kernel_main` 启动后打印一次：`sp=<addr>`，并验证：
  - `sp % 16 == 0`
  - `boot_stack_top`、`kernel_stack_top` 与 `sp` 的关系符合预期（例如 `sp` 落在 kernel_stack 范围内）。
- 若对齐不满足或 `sp` 不在预期范围内：必须 `panic("bad stack")`，不得继续跑“看似正常”的日志。

---

## 2) linker 脚本（linker.ld）编写规范（必须）

### 2.1 总体目标（必须）
- 生成的 `kernel.elf` 必须满足：
  - 入口符号为 `kernel_boot`（由 `ENTRY(kernel_boot)` 指定）。
  - 链接地址（VMA）即运行物理地址：`KERNEL_PHYS_BASE = 0x80200000`，不使用 `AT(...)` 偏移（无虚拟内存、无 LMA/VMA 分离）。
  - 段按顺序布局：`.text → .rodata → .data → .bss`，并导出 `__bss_start/__bss_end/__kernel_end`。

> linker script 的核心作用是“控制输入段如何映射到输出段，以及最终镜像的内存布局”。

### 2.2 必须包含的指令与语义（Agent 必须理解并正确使用）
- `OUTPUT_ARCH(riscv)`：声明目标架构（便于工具链一致性）。
- `ENTRY(kernel_boot)`：指定 ELF entry point。
- 位置计数器 `. = KERNEL_PHYS_BASE;`：把第一个输出段放到 `0x80200000`。
- `SECTIONS { ... }`：定义输出段布局。
- `ALIGN(n)`：控制输出段/地址对齐（例如 4K 对齐段边界；至少保证 `.text.boot`、trap 向量相关段满足 4B 对齐）。GNU ld 与 lld 对输出段对齐的行为有明确规则，Agent 不得随意省略对齐。
- `KEEP(*(.text.boot))`（或等价入口段）：若启用 `--gc-sections`，必须用 `KEEP` 防止启动段被回收。 
- `PROVIDE(__bss_start = .);`/直接符号赋值：导出 BSS 边界与内核结束符号。

### 2.3 推荐的段组织（模板级约束，Agent 可据此生成脚本）
> 以下是“结构模板”，用于提示词约束；Agent 可生成等价脚本，但必须满足同样语义。

- `.text` 段内必须优先放置启动入口段（例如 `.text.boot` / `.text.entry`），并使用 `KEEP()` 包裹。
- `.bss` 段内必须包含 `.bss.stack`（内核栈/boot 栈）并导出 BSS 边界符号。
- 必须导出：
  - `__bss_start`：BSS 起始
  - `__bss_end`：BSS 结束
  - `__kernel_end`：镜像末尾（用于后续物理内存分配起点）

### 2.4 典型坑位（必须写入提示词以约束 Agent）
- **坑 1：入口段被回收** 
  若使用 `-ffunction-sections -fdata-sections -Wl,--gc-sections`，没有 `KEEP()` 可能导致 `.text.boot` 被裁剪，表现为“能链接但无法启动”。 
- **坑 2：stvec 未对齐** 
  `stvec` BASE 必须 4B 对齐，MODE 可能带来更强约束；因此 trap 向量符号与其输出段必须保证至少 4B 对齐。
- **坑 3：误用 AT(...) 或分离 LMA/VMA** 
  本 Stage 明确“无虚拟内存”，因此不得引入 LMA/VMA 分离，避免运行地址与链接地址不一致。
- **坑 4：忘记导出符号导致启动/清零错误** 
  `__bss_start/__bss_end/__kernel_end` 缺失会直接破坏 BSS 清零与后续内存管理 Stage 的边界计算。

### 2.5 链接产物的强制验证（Agent 必须执行并记录）
在“编译成功”之后，Agent 必须执行并记录至少以下验证（作为 Stage 2.5 的一部分）：
1. **检查入口点**：确认 `Entry point address` 指向 `kernel_boot` 且位于 `0x80200000` 附近。
2. **检查段地址**：确认 `.text/.rodata/.data/.bss` 的 VMA 从 `0x80200000` 开始单调递增，且 `.bss` 含栈段。
3. **检查导出符号**：确认 `__bss_start/__bss_end/__kernel_end/boot_stack_top/kernel_stack_top` 均存在且地址合理。

> 目的：避免 Agent 出现“能编译但不运行/不验证”的提前停止行为。

# Stage 2.1 基础内核库构建

## A) OpenSBI 封装（`sbi.c`, `sbi.h`）
### A1. 通用 ECALL 封装（必须）
- 提供：
  - `struct sbi_ret { long error; long value; };`
  - `struct sbi_ret sbi_call(long eid, long fid, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);`
- 参数约定（必须）：
  - `a7 = eid`、`a6 = fid`、`a0..a5 = arg0..arg5`
  - 返回：`a0 -> error`、`a1 -> value`（映射到 `struct sbi_ret`）
- 说明：除 a0/a1 外，其余寄存器跨 SBI 调用需按规范保留（实现时避免破坏不必要寄存器）。

### A2. Console Putchar（必须）
- 提供：`void sbi_console_putchar(int ch);`
- 使用 **legacy console putchar**：
  - `eid = 0x1`（Console Putchar）
  - `a0 = ch`
- 不依赖 UART 设备，所有输出必须经由 OpenSBI。

### A3. Timer（必须）
- 保留接口：`void sbi_set_timer(uint64_t stime);`
- 使用：`SBI_EXT_TIME (0x54494D45)`，`fid = 0`

### A4. Shutdown（必须完成）

- **新增对外接口**：提供 `sbi_shutdown()`（语义：请求系统关机；**成功则不返回**）。
- **优先使用 SRST（System Reset Extension）**：
  - 使用 `sbi_call` 调用 **System Reset Extension**：`EID = 0x53525354 ("SRST")`，`FID = 0`。
  - 参数约定：`a0 = reset_type`，`a1 = reset_reason`；其中 **关机**对应 `reset_type = 0x00000000`，`reset_reason` 可用 `0x00000000`（No reason）。
  - 行为要求：该调用为同步调用，**成功不返回**；若返回表示未成功或不支持，需要进入回退路径。
- **回退路径：legacy shutdown（兼容性必须）**：
  - 若 SRST 调用返回错误（例如 NOT_SUPPORTED / INVALID_PARAM / FAILED），必须回退调用 **legacy System Shutdown**：`EID = 0x08`。
  - legacy shutdown 语义：从 S-mode 视角将系统置于关机态，**无论成功或失败均不返回**。
  - 规范关系：`EID 0x08` 的 legacy shutdown 在函数列表中明确标注其替代为 `EID 0x53525354 (SRST)`。
- **健壮性要求（必须）**：
  - 若两种路径意外返回（不符合预期实现的极端情况），不得继续执行普通流程：必须关闭中断并进入不可返回的停机循环（例如 `wfi`）。
  - `sbi_shutdown()` 必须被视为“终止性 API”（上层调用点要明确：一旦调用，后续逻辑不可依赖其返回）。

---

## B) printf（`printf.c`, `printf.h`）
### B1. 接口（必须）
- `void kprintf(const char *fmt, ...);`
- `void kvprintf(const char *fmt, va_list ap);`

### B2. 输出路径（必须）
- `kprintf/kvprintf` 必须通过 `sbi_console_putchar` 输出字符。
- `\n` 自动转换为 `\r\n`（控制台友好）。

### B3. 能力范围（必须）
- 支持：`%c` / `%s` / `%d` / `%u` / `%x` / `%p`
- 支持 `l` 前缀：`%ld` / `%lx` / `%lu`
- 不要求 `%lld`
- 不支持浮点、宽度/精度控制

> 注意：`NULL` 定义来自 `stddef.h`，不得自定义“伪 NULL”。

---

## C) panic（`panic.c`, `panic.h`）
### C1. 接口（必须）
- `void panic(const char *fmt, ...);`
- 可选：`void panic_trap(uint64_t scause, uint64_t sepc, uint64_t stval, const char *msg);`

### C2. 行为（必须）
- 输出 `panic: ` 前缀 + 格式化信息。
- trap 版本必须额外输出：`scause / sepc / stval`
- 关闭中断并调用`sbi.c`的`sbi_shutdown()`进行关机。

---

# Stage 2.2 启动（Startup）

## A) 固件启动（OpenSBI）
### A1. 入口初始化（固定）
- 使用 OpenSBI：QEMU `-bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin`
- 内核运行方式：`-kernel kernel.elf`
- 加载地址固定：`0x80200000`

### A2. 链接器脚本（必须创建新的 `linker.ld`）

`linker.ld` 放置在 `kernel/` 目录

- `OUTPUT_ARCH(riscv)`
- `ENTRY(kernel_boot)`
- `KERNEL_PHYS_BASE = 0x80200000`（固定）
- 全部段按**物理地址**布局，不使用 `AT(...)` 偏移
- 必须导出符号：`__bss_start`、`__bss_end`、`__kernel_end`
- 无虚拟内存前提：不启用分页（Bare 模式）
  - 若保留 `KERNEL_VIRT_BASE`，则必须等于 `KERNEL_PHYS_BASE`
  - `KERNEL_VIRT_OFFSET = 0`，确保所有符号均为物理地址

### A3. 参数传递约定（必须严格遵守）
- 仅使用：`a0 = hartid`
- `a1/a2` 未定义，内核不得读取
- 不再使用 `bootinfo` 结构：相关逻辑必须移除或旁路
- **禁止使用 `firmware/` 传参链路**

---

## B) 内核启动（S 模式）

### B1. 入口与栈初始化（`kernel/entry.S`）
- 不考虑多核：默认 `-smp 1`，入口不做 hart 分流
- 设置 boot 栈：`boot_stack_top`
  - 建议：`BOOT_STACK_SIZE = 16KB`、`KERNEL_STACK_SIZE = 16KB`
- 仅保存 `a0`（不得依赖 `a1/a2`）
- **不启用分页**：不写 `satp`，不 `sfence.vma`
- 清零 BSS
- 恢复 `a0`
- 切换到 `kernel_stack_top`
- 跳转：`kernel_main(hartid)`

### B2. 物理地址链接（`memlayout.h` 等）
- 取消高半区映射：内核链接地址全为物理地址
- `KERNEL_PHYS_BASE = 0x80200000`
- `.text/.rodata/.data/.bss` 直接从物理地址顺序布局，不用 `AT(...)` 偏移

### B3. 早期初始化（`kernel/main.c`）
- 进入 `kernel_main` 后立即打印 banner（必须使用 OpenSBI console putchar）
- 记录并打印：
  - `hartid`
  - `mem_base / mem_size`（允许使用固定值或占位值；不得依赖任何外部描述输入）
  - `timebase`（允许使用固定值或占位值；不得依赖任何外部描述输入）
  - `tick_hz`
- 初始化顺序固定：`trap_init -> timer_init -> trap_enable`
- 不触发任何虚拟内存相关逻辑

---

# Stage 2.3 Trap / 中断 / Timer

## A) Trap 向量与现场保存
### A1. trapframe 定义（`kernel/include/trap.h`）
- `trapframe` 字段顺序必须与 `trapvec` 保存顺序**严格一致**

### A2. trap 入口（`kernel/trapvec.S`）
- `stvec` 使用 **direct mode**
- `stevc` 严格**四字节对齐**
- 保存/恢复：完整 GPR + `sepc` + `sstatus` + `usp`
- 不保存浮点寄存器；返回 `sret`
- `sscratch` 用途明确：初始化为 `0` 或内核栈指针，用于 trap 入口判断/交换栈

## B) Trap 初始化与开关（`kernel/trap.c`）
- `trap_init`：`stvec = &kernel_trap_vector`，写入时低 2 位清零
- `trap_enable`：打开 `sstatus.SIE` 与 `sie.STIE`

## C) 最小异常集支持（`kernel/trap.c`）
- 非中断异常（`scause` 高位=0）必须支持：
  - `2` illegal instruction
  - `5` load access fault
  - `7` store/amo access fault
  - `8` ecall from U-mode（若仅内核态，可记录后返回或进入 `syscall_stub` 占位）
- 处理策略：
  - `2/5/7`：打印并 `panic_trap`（不可恢复）
  - `8`：打印 `scause/sepc/stval`，`sepc += 4`，可调用 `syscall_dispatch` 或占位返回
- 输出要求：以上异常必须打印 `scause/sepc/stval`

## D) 定时器中断（`kernel/timer.c`, `kernel/sbi.c`）
- QEMU `virt` 的设备树里 `/cpus` 的 `timebase-frequency` 常见是 **10,000,000 (10MHz)**（十六进制 `0x00989680`）
- `timer_init`：`tick_hz=100`，用 SBI `set_timer` 设置首次 deadline（不依赖外部描述输入）
- `timer_handle`：`tick++`，设置下一次 deadline；每秒打印一次 `timer: tick=...`

## E) Trap 行为

- **必须先打印 scause/sepc/stval**

---

# Stage 2.4（补充）panic 机制（必须完善）
- `panic(...)`：打印 `panic:` + 信息；输出 `hartid`（启动来自 `a0`，之后保存到 `tp`）；关闭中断（清 `sstatus.SIE`）；调用`sbi_shutdown()`关机。
- 接入点：trap handler 对 `2/5/7` 调用 `panic_trap`；其他不可恢复错误调用 `panic`

---

# Stage 2.5 验收程序（打印输出要求）
1. **Banner**：进入 `kernel_main` 立即打印一次 

   示例：`AgentOS RV64 - OpenSBI boot`

2. **初始化信息**（顺序必须正确）：`hartid`、`tick_hz`、`trap_init ok`、`timer_init ok`、`trap_enable ok`

3. **定时器可见性**：每秒输出一次 `timer: tick=...`，输出5秒，紧接着进行**异常打印**。

4. **异常打印**：必须打印 `scause/sepc/stval`

   - 触发 illegal instruction：在 S-mode 下执行一次对 **M-mode CSR** 的读访问（例如读取 `mstatus` 等 Machine 级 CSR）

   - 手动触发 panic：在初始化日志完成后，显式调用一次 `panic("manual panic")`（或等价信息）。

5. **通过判定**：日志顺序正确；约 1 秒后出现 tick；不可恢复异常能打印三元组并进入 panic

---

# Stage 2.6 Makefile 规范（必须遵守）

- 工具链：
  ```makefile
  CROSS ?= riscv64-unknown-elf-
  CC := $(CROSS)gcc
  OBJCOPY := $(CROSS)objcopy
  ```
  
- CFLAGS：
  `-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -march=rv64imac -mabi=lp64 -mcmodel=medany -Iinclude`
  
- LDFLAGS：
  `-T linker.ld -nostdlib -no-pie -Wl,--build-id=none`
  
- ASFLAGS：
  `-Iinclude -march=rv64imac -mabi=lp64`
  
- 源文件列表（必须包含）：
  - C：`main.c sbi.c printf.c log.c panic.c trap.c timer.c`
  - S：`entry.S stack.S trapvec.S`
  
- 目标：
  - `all: kernel.elf kernel.bin`
  - `kernel.elf` 链接所有对象 + `linker.ld`
  - `kernel.bin` 由 `objcopy -O binary` 生成
  
- `run`（必须提供一键验收）：

  - `make -C kernel run` 或在根目录执行等价命令
  - 启动命令固定为：`qemu-system-riscv64 -machine virt -nographic -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin -kernel ./kernel.elf -smp 1`

- 清理：删除 `*.o`、`kernel.elf`、`kernel.bin`

- `.PHONY: all clean`

---

# Stage 2.7 链接文件规范（必须遵守）
- `.section .text.boot` 与 `linker.ld` 对应
- `OUTPUT_ARCH(riscv)`、`ENTRY(kernel_boot)`、`KERNEL_PHYS_BASE=0x80200000`
- 段布局：从 `KERNEL_PHYS_BASE` 开始，`.text -> .rodata -> .data -> .bss`
- 必须导出符号：`__bss_start`、`__bss_end`、`__kernel_end`

---

# Stage 2.8 注意事项（必须逐条遵循）
1. 汇编对齐：trap 向量写入 `stvec` 时必须先确认 direct/vectored 对齐要求再写代码（至少 4B 对齐；vectored 可能更严格）。
2. `.section` 必须与 `linker.ld` 段匹配。
3. 启动汇编必须放置到 `0x80200000`（通过链接脚本 + `.text.boot` 保证）。
4. `printf` 实现参考现有项目；`NULL` 来自 `stddef.h`，不要自定义。
