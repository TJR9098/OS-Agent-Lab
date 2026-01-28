# Stage 4：Sv39 虚拟内存（内核映射 + 用户空间雏形）

>  前置：已完成 Stage 3 物理页分配器（`kalloc/kfree` 可用），且 Stage 2/3 仍可在 QEMU virt 上稳定运行与打印。

------

## 目标

1. **实现 Sv39 页表机制**

   - 页表页分配（使用 Stage 3 `kalloc`）、`walk`、`map/unmap`。
   - 支持 PTE 权限位：`R/W/X/U`（允许附带实现/固定 `A/D` 策略，但核心验收以 R/W/X/U 为准）。
   - 在修改映射/切换地址空间后，使用 `sfence.vma` 刷新相关 TLB。

2. **建立内核地址空间方案**

   **恒等映射（VA==PA）**


    要求把关键区域映射正确：内核文本、数据、设备 MMIO、内核栈（以及 trap 向量页）。

3. **安全开启 satp（强制自检）**

   - 在写 `satp` 前必须做“关键映射自检”：**PC 所在页、当前栈页、trap 向量页**必须可翻译且权限正确；否则不得写 `satp`，必须 `panic`，避免“切换后无输出”。

4. **提供用户地址空间雏形**

   - 创建用户页表、加载最小用户程序（最小 ELF 或内嵌 initcode 均可）。
   - 具备 `copyin/copyout`（必须覆盖跨页边界）。

5. **实现 page fault 处理（可诊断）**

   - 至少能区分 **用户 fault / 内核 fault**。
   - 用户 fault：终止“当前用户程序/进程雏形”，输出可诊断信息并返回到内核测试流程。
   - 内核 fault：直接 `panic`。

**强约束**：

- **Stop Condition（必须满足后停止运行）**

  - 只有当以下全部满足，Agent 才能停止：

    - satp 回归通过（开分页后仍打印/仍能 trap/能继续执行）。
    - 权限测试、隔离、copyin/copyout、TLB 刷新回归全部通过。
    - page fault 打印包含 `scause/sepc/stval`，且用户 fault 能 kill、内核 fault 会 panic。
    - 性能指标输出包含：测量方法 + 数据（avg/P95/MB/s/us）。

    否则不得输出“任务完成/总结已完成工作”；必须修复、重跑、再验证后再停止。

------

## Clarification Gate（必须先问清楚的点）

1. **平台选择（必须）**：如果用户是 Windows，先确认用 **WSL** 还是 **原生 Windows**，再继续。
2. **如有任何不明确或存在歧义，必须先提问并等待用户回答后再继续。**

> 未完成 Clarification Gate 回答前：只能输出问题清单并停止。

------

## 输入与约束（必须）

1. **不依赖 DTB**：本 Stage 不解析 DTB。设备 MMIO 使用 **QEMU virt 固定内存映射常量**（UART/virtio/PLIC/CLINT 等），写入 `memlayout.h` 固化。
2. 内核地址空间选择**恒等映射**。
3. 用户地址空间范围采用 **xv6 风格**。
4. **TLB 刷新粒度**：仅用全局 `sfence.vma`。
5. **缺页策略（本 Stage 固定）**：用户 page fault 直接 `kill`。
6. **Sv39 结构约束**：
   - Sv39：3 级页表，每级 512 项（9-bit VPN），PTE 8 字节；页表页必须 4KB 对齐。
7. **satp 约束**：
   - `satp.MODE=Sv39`，`satp.PPN` 指向根页表物理页号（根页表物理地址 >> 12）。
8. **一致性约束**：所有页表写入后，必须按约定执行 `sfence.vma`，保证映射立刻生效。

------

## 交付物（文件清单，必须创建/修改）

- `kernel/include/vm.h`：PTE 定义、权限位、API 声明
- `kernel/vm.c`：页表分配、walk、map/unmap、copyin/copyout、切换 satp、关键自检
- `kernel/trap.c`：page fault 分流与诊断输出（用户 kill / 内核 panic）
- `kernel/include/memlayout.h`：QEMU virt 设备 MMIO 映射常量（UART/virtio/PLIC/CLINT）
- `kernel/main.c`：Stage 4 测试入口与日志顺序（satp 回归、权限回归、copyin/out 回归）
- （可选）`user/`：最小用户程序与构建脚本；或内嵌 `initcode` 二进制数组

------

## 设计与实现要求（Agent 必须按条落实）

### 4.1 页表与 PTE（必须）

- 定义 `pagetable_t`：根页表指针（指向 4KB 页表页）。
- `walk(pagetable, va, alloc)`：按 Sv39 三层索引遍历；`alloc=1` 时可按需分配中间页表页（使用 `kalloc`）。
- `map_pages(pagetable, va, pa, size, perm)`：按页映射，设置 `R/W/X/U`。
- `unmap_pages(pagetable, va, npages, do_free)`：解除映射；必要时释放叶子页（用户页）。
- 权限语义：读/写/取指非法时必须触发 page fault（由硬件 + trap 处理保证）。

### 4.2 内核地址空间（必须）

- 方案 A（恒等映射）：将内核 `.text/.rodata/.data/.bss`、内核栈、MMIO 以 `VA=PA` 映射。
- 方案 B（高半区）：固定一个 `KERNEL_VA_BASE`，将上述区域映射到高半区 VA；并确保 `stvec`、`sp`、`pc` 使用的地址与映射一致。
- 设备 MMIO：至少映射 UART 与 virtio-mmio（后续设备驱动会用），PLIC/CLINT 可映射为“将来用”。

### 4.3 安全开启 satp（必须：自检 + 刷新）

在写 `satp` 前，必须完成并输出自检结果（任一失败必须 `panic`）：

1. **PC 页可翻译**：当前执行地址所在页在新页表下可 `X`（可执行）。
2. **当前栈页可翻译**：当前 `sp` 所在页在新页表下可 `R/W`。
3. **trap 向量页可翻译**：`stvec` 指向的地址所在页在新页表下可 `X`（且地址满足对齐要求）。
    完成后按固定顺序执行：

- 写 `satp`（切换到新根页表）
- 执行 `sfence.vma`（建议全局）
- 立即打印 `vm: satp enabled`（用于回归定位）

### 4.4 用户地址空间雏形（必须）

- `uvm_create()`：创建用户根页表（空）。
- `uvm_load_minimal()`：加载最小用户程序（ELF 或内嵌 initcode）到用户 VA（例如 0x0 或固定 USER_BASE）。
- 用户页必须设置 `U=1`；内核映射不得设置 `U=1`（保证隔离）。

### 4.5 copyin/copyout（必须）

- 必须“软件 walk 用户页表”实现 copyin/copyout（不依赖内核把用户页恒等映射到内核地址）。
- 必须覆盖跨页边界：长度为 0、正好跨 1 页、跨多页。

### 4.6 page fault 处理（必须）

- trap 中识别 load/store/inst page fault（按 `scause` 分类），并打印至少：`scause/sepc/stval`。
- 区分 fault 来源：
  - 若来自用户态：打印 `user fault, kill` + 诊断信息，并结束用户程序回到内核测试。
  - 若来自内核态：直接 `panic_trap()`。

------

## 测试点（必须全部执行并验证）

1. **satp 开启回归**：开启 satp 后仍能打印、仍能处理 trap，并继续后续测试。
2. **权限测试**：对同一 VA 分别设置 `R/W/X/U`，验证非法访问触发 page fault（并打印三元组）。
3. **用户-内核隔离**：用户态访问内核地址必须失败（fault 或 kill）。
4. **copyin/copyout 跨页**：三类用例必须通过（0、跨 1 页、跨多页）。
5. **TLB 刷新回归**：修改映射后立即生效；若启用 debug，可检测“未 sfence.vma”导致的错误路径（可选）。

------

## 性能指标（必须输出测量方法与结果）

1. **TLB miss 代价（cycles）**：构造触发 `walk` 的访问模式，测平均与 P95。
2. **缺页处理开销（us）**：触发用户 page fault 并 kill 的路径耗时（教学分析）。
3. **copyin/copyout 带宽（MB/s）**：4KB / 64KB / 1MB 三档，对比 Stage2 `memcpy` 基线比值。
4. **地址空间切换开销（us）**：切换到另一用户页表（写 satp + flush）成本，为后续进程切换做基线。

------

## 执行与验证步骤（必须；防止“只编译就停”）

1. **构建**：生成 `kernel.elf`。
2. **运行**：必须启动 QEMU 并采集完整控制台输出。
3. **逐项验证**：对照“测试点”与“性能指标”输出；任一缺失即判定未完成，必须修复后重跑。
4. **失败唯一出口**：若需要用户输入（平台/路径/命令输出/参数确认），只输出问题清单并停止。

