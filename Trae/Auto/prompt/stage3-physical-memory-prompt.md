# Stage 3：内存管理（Physical Memory Only，无虚拟内存）

## 目标（必须实现）
1. 在**不使用虚拟内存**（`satp=0`，`KERNEL_VIRT_OFFSET=0`）前提下，提供**可复用、可验证、可复现**的**物理页**分配与释放接口。
   - 页大小固定：`PAGE_SIZE = 4096`。
   - 内核运行地址与物理地址一致：所有指针/地址均按“物理地址”语义处理。
2. 行为必须可稳定复现：分配范围、边界、失败模式与日志输出均固定。
   - 当内存范围无效/越界/对齐错误时：必须触发 `panic()`（不可静默失败）。

**强约束**：

- **Stop Condition（必须满足后停止运行）**
  - 当且仅当：日志与自测全部满足“**3.7 验收**”要求，输出最终结果后立即停止，不做额外扩展。
- 任何不明确处必须先提问；在未澄清前不得“拍脑袋补全”。
- 若因信息缺失/不一致无法继续：输出问题清单并停止。

---

## Clarification Gate（必须先问清楚的点）
1. **若用户主机是 Windows：先确认使用 WSL 还是原生 Windows 环境**；明确后再继续执行后续步骤。
2. **QEMU 内存参数**：用户运行 QEMU 的 `-m` 是多少？需要据此设定 `DRAM_SIZE`（单位：字节）。
3. **如有任何不明确或存在歧义，必须先提问并等待用户回答后再继续。**

> 未完成 Clarification Gate 的回答前：只输出问题清单并停止。

---

## 3.1 输入与约束（必须遵守）
### 1) 内存规模来源
- **不解析 DTB**、不从固件读取内存布局。
- 使用编译期常量：`DRAM_SIZE`（单位：字节）。
- 若 `DRAM_SIZE` 与用户 QEMU `-m` 不匹配：必须提示用户调整其中之一，并停止等待确认。

### 2) 地址约束（固定）
- `DRAM_BASE = 0x80000000`（QEMU virt DRAM 起始地址）。
- `KERNEL_PHYS_BASE = 0x80200000`（内核加载地址固定）。
- `KERNEL_VIRT_OFFSET = 0`（无虚拟内存）。

---

## 3.2 内存布局与边界（必须完全一致）
### 1) 可分配区间（页粒度）
- `alloc_start = align_up(max(__kernel_end, DRAM_BASE), PAGE_SIZE)`
- `alloc_end   = DRAM_BASE + DRAM_SIZE`

### 2) 排除区间（不可释放）
- `[DRAM_BASE, __kernel_end)`：内核镜像 + BSS + 启动/内核栈等占用内存。

### 3) 合法性检查（必须）
- 若 `alloc_start >= alloc_end`：必须 `panic("bad memory range")`（或等价信息，需固定）。

---

## 3.3 符号与类型约定（必须）
- 所有 linker/汇编导出的“标号符号”在 C 里统一用 `char[]` 声明，仅表示地址：
  - 例如：`extern char __kernel_end[];`
- 禁止将上述符号声明为 `int/long` 并“读取其内容”。

---

## 3.4 数据结构（必须）
### 1) 空闲链表（O(1)）
- `struct run { struct run *next; }`
- 全局：`g_freelist` 指向空闲页链表头
- `kalloc/kfree` 时间复杂度：O(1)

### 2) 非目标（本 Stage 不实现）
- 不实现 SLAB / 伙伴系统 / 多级缓存；仅单链表页分配器。

---

## 3.5 接口与行为（必须）
### 1) 初始化：`kinit(uint64_t mem_base, uint64_t mem_size)`
- **仅允许**：`mem_base == DRAM_BASE` 且 `mem_size == DRAM_SIZE`；否则 `panic("kinit args")`。
- 初始化步骤（顺序固定）：
  1) `g_freelist = NULL`
  2) 计算 `alloc_start/alloc_end` 并进行合法性检查
  3) 对 `[alloc_start, alloc_end)` 按页遍历并调用 `kfree()`
- 初始化完成后：计算并打印可分配页数 `free_pages`（算法需固定）。

> 复现性要求：必须确保“最小自测”能观测到**连续三次 `kalloc()` 返回地址递增**（见验收）。因此，空闲链表的构建顺序/入链策略必须与此要求一致。

### 2) 分配：`kalloc(void)`
- 从 `g_freelist` 取出一页并返回其物理地址指针。
- 若链表为空：按 Clarification Gate 的 OOM 策略执行（返回 NULL 或 panic；二者选其一并固定）。
- 分配出的页必须清零（`memset(page, 0, PAGE_SIZE)`）。
- `kalloc()` 在空闲链表为空时，返回 `NULL`。

### 3) 释放：`kfree(void *pa)`
- 约束检查（任一不满足必须 panic）：
  - `pa` 4K 对齐（`(uintptr_t)pa % PAGE_SIZE == 0`）
  - `pa >= alloc_start && pa < alloc_end`
- 释放页可选填充调试字节（如 0xCC，需固定是否启用与填充值），再入链表。

---

## 3.6 调用点（内核启动顺序，必须）
在 `main.c` 的固定流程中：
1. 打印 banner 后，调用 `kinit(DRAM_BASE, DRAM_SIZE)`
2. 打印：`mem: kinit done, free_pages=<N>`

---

## 3.7 验收（必须可复现）
### 1) 启动日志（固定输出内容）
必须包含并保持顺序：
- `mem: base=0x80000000 size=<DRAM_SIZE>`
- `mem: kinit done, free_pages=<N>`

### 2) 最小自测（必须）
- 连续 `kalloc()` 三页：必须均非空，且地址严格递增（`p0 < p1 < p2`）。
- 释放后再次分配：必须可复用（至少能再次分配到先前释放过的页）。
- 任一自测失败：必须 `panic("mem selftest failed")`（或等价信息，需固定）。
