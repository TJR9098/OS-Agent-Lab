# 虚拟内存管理（VM）功能测试文档（Correctness First）

> 目标：验证虚拟内存机制“功能是否正确”，不评估性能、吞吐或周期数。  
> 默认背景：RISC-V RV64 + Sv39（如使用 Sv48/Sv57，请按对应 canonical 规则与页表层数调整）。  
> 关键硬件事实：  
> - Sv39 下 **64-bit 虚拟地址必须满足 canonical**：`va[63:39]` 全等于 `va[38]`，否则会触发 page-fault。
> - **写 satp 不保证 TLB/地址转换缓存失效，也不保证页表写入与后续转换之间的顺序**；如页表更新/ASID复用，需 `SFENCE.VMA` 同步。
> - page fault 类型可用 `scause` 区分：指令取指/读/写 page fault 常用编码分别为 12/13/15；fault VA 在 `stval`。
> - `copyin/copyout` 本质是对“用户虚拟地址”显式翻译并复制，因此应放在 VM 层验证。

---

## 1. 测试前置条件（必须具备）

### 1.1 测试环境
- 单核/多核均可；本文仅要求单核 correctness。
- 具备以下基础接口（名称可不同，语义需一致）：
  - `pagetable_create()/pagetable_destroy()`
  - `map_pages(pt, va, pa, size, perm) -> int`
  - `unmap_pages(pt, va, npages, do_free) -> int/void`
  - `walk(pt, va, alloc) -> pte*`
  - `switch_satp(pt)` 或 `w_satp()/r_satp()`
  - `sfence_vma([va],[asid])`（可用内联 asm）

### 1.2 必备“故障捕获（fault harness）”
若要测试权限、缺页、canonical 地址等，必须能**在不崩溃内核的前提下**捕获 fault 并返回测试流程。

建议实现：
- 全局结构 `vmtest_fault`：
  - `armed`：是否处于“期望 fault”模式
  - `hit`：是否发生 fault
  - `scause/stval/sepc`
- API：
  - `fault_arm(expected_mask, expected_va_or_range)`
  - `fault_disarm()`
  - `fault_expect_hit()` / `fault_expect_nohit()`

trap handler 行为：
- 若 `armed`：
  - 记录 `scause/stval/sepc`
  - 将 `sepc` 跳过当前 faulting 指令（或跳转到安全 label）
  - 返回到测试函数
- 否则：走正常内核 fault 处理（panic 或 kill）

> 说明：用 `scause` 区分取指/读/写 page fault，并用 `stval` 校验 fault VA。

### 1.3 测试 VA/PA 选择规则
- **避免低地址**（如 `0x0、0x1000`），很多系统会保留为 NULL guard。
- 选一个固定测试区间，例如：
  - `TEST_VA_BASE = 0x4000_0000`
  - `TEST_VA2 = TEST_VA_BASE + 0x2000`（避免互相覆盖）
- 物理页统一用 `kalloc()` 获取，测试结束时按约定释放。

---

## 2. PASS/FAIL 判定标准

- **PASS**：所有用例断言成立；无意外 trap；无资源泄漏（页表页/物理页计数回到初值）。
- **FAIL**：任一断言失败、trap 分类不符、TLB 未按预期同步、copy 数据不一致、出现 double-free/leak。

建议输出格式：
- `[VMTEST] T06 permissions ... PASS`
- `[VMTEST] T08 copyout cross-page ... FAIL: expected store-page-fault, got scause=... stval=...`

---

## 3. 测试用例清单（功能覆盖）

### T01：页表生命周期（create/destroy）
**目的**：验证页表创建/销毁、页表页分配与回收无泄漏/无 double-free。

**步骤**
1. `pt = pagetable_create()`
2. 分配 N 个物理页（N≥4），映射到不同 VA（跨不同 VPN）
3. `unmap_pages(..., do_free=0)` 与 `do_free=1` 两种路径分别测
4. `pagetable_destroy(pt)`

**断言**
- destroy 后页表页计数回到初值（需要 kalloc 计数或调试统计）
- `do_free=1` 时物理页被释放；`do_free=0` 时不会释放（防止 double free）

---

### T02：walk/map 基础正确性（PTE 内容一致）
**目的**：验证 `map_pages` 写入的 PTE 与 `walk` 读出的结果一致。

**步骤**
1. `pa = kalloc()` 清零
2. `map_pages(pt, VA, pa, 4096, perm)`
3. `pte = walk(pt, VA, alloc=0)`
4. 从 `pte` 解出 PPN -> `pa2`

**断言**
- `pte != NULL`
- `PTE_V=1`
- `pa2 == pa`
- PTE 权限位与 `perm` 一致

---

### T03：unmap 正确性（映射消失 + 可选 free 行为）
**目的**：验证 unmap 后该 VA 不再有效。

**步骤**
1. 先执行 T02 完成映射
2. `unmap_pages(pt, VA, 1, do_free=0)`
3. `pte = walk(pt, VA, 0)`

**断言**
- `pte == NULL` 或 `PTE_V=0`（按实现约定）
- 再次访问 VA（load/store/exec）应触发 page fault（结合 fault harness）

---

### T04：重复映射（remap policy 明确化）
**目的**：明确同一 VA 再次 `map_pages` 时的语义：拒绝/覆盖。

**步骤**
1. map `VA -> PA1`
2. 再 map `VA -> PA2`（不 unmap）
3. 读返回值/最终 PTE 指向

**断言（必须二选一并固化）**
- **策略 A：拒绝 remap**
  - 第二次 map 返回失败；PTE 仍指向 PA1
- **策略 B：允许覆盖**
  - 第二次 map 成功；PTE 指向 PA2；并要求执行/内部执行 `SFENCE.VMA`

> 页表更新后是否需要 `SFENCE.VMA`：RISC-V 明确要求用它同步页表写入与地址转换缓存；写 satp 也不隐含 flush。:contentReference[oaicite:5]{index=5}

---

### T05：Sv39 canonical VA 检查（必须是“功能测试”）
**目的**：对 Sv39 规则进行正反例验证：非 canonical VA 必须 fault。:contentReference[oaicite:6]{index=6}

**测试点**
- canonical（低半区）：`0x0000_0000_0000_0000`、`0x0000_0007_FFFF_FFFF`
- canonical（高半区）：`0xFFFF_FFFF_8000_0000`、`0xFFFF_FFFF_FFFF_FFFF`
- 非 canonical：例如 `0x0000_0008_0000_0000`（bit38=0 但上位非 0）

**步骤**
1. 对每个 VA：执行一次 load（或 walk+访问组合）
2. 对非 canonical：`fault_arm(expected=load-page-fault, expected_va=VA)` 后访问

**断言**
- canonical VA：若未映射应产生 “not present” 的 load-page-fault（或你的内核定义的缺页路径）
- 非 canonical VA：必须 page-fault（RISC-V 规定）:contentReference[oaicite:7]{index=7}

---

### T06：权限矩阵（真实访问验证：load/store/exec）
**目的**：不要只检查 PTE 位，必须用真实指令触发访问，验证 fault 分类（scause）与 stval。:contentReference[oaicite:8]{index=8}

**用例**
1. `R=1 W=0 X=0`：load 成功；store fault；exec fault
2. `R=1 W=1 X=0`：load/store 成功；exec fault
3. `R=0 W=0 X=1`：exec 成功；load/store fault（若实现禁止读写）

**实现提示**
- exec 测试需要在被映射页上放置一个小 stub（或跳板函数），并通过函数指针调用。
- fault 分类：
  - 取指 page fault：scause=12
  - 读 page fault：scause=13
  - 写/AMO page fault：scause=15 :contentReference[oaicite:9]{index=9}

**断言**
- fault 必须命中，且 `stval == fault_va`
- 非 fault 路径：读写值正确、不会误触发 trap

---

### T07：copyin/copyout 正常路径（用户虚拟地址翻译正确）
**目的**：验证 “用户 VA → PA 翻译 + 复制” 的 correctness。`copyin/copyout` 应基于给定 pagetable 显式翻译用户虚拟地址。:contentReference[oaicite:10]{index=10}

**步骤**
1. 构造“用户页表”（或你的 copyin/out 期望的 pt），映射 `UVA` 一页，权限满足读写（通常需 `PTE_U|PTE_R|PTE_W|PTE_V`）
2. `copyout(pt, UVA, src_buf, len)` -> 期望成功
3. `copyin(pt, dst_buf, UVA, len)` -> 期望成功
4. 比对 `src_buf == dst_buf`

**断言**
- 返回值为成功（0）
- 数据一致
- 无 fault

---

### T08：copyin/copyout 跨页边界（第二页缺失）
**目的**：验证跨页复制时遇到缺页，函数能正确失败并给出一致行为。

**步骤**
1. 映射 `UVA` 的第一页；**不映射** `UVA+4096` 的第二页
2. `len = 4096 + 16`
3. 执行 `copyout`（或 `copyin`）

**断言**
- 返回失败（非 0 或 -1，按实现）
- 不会写越界到未映射页
- 若实现会记录 fault：`stval` 应落在第二页范围

---

### T09：copyin/copyout 权限失败（只读页写入/不可读页读取）
**目的**：验证权限 enforcement：copyout 本质写，copyin 本质读。

**步骤**
1. 映射 UVA，设置 `R=1 W=0`，执行 `copyout` -> 应失败
2. 映射 UVA，设置 `R=0`（或清 R），执行 `copyin` -> 应失败

**断言**
- 返回失败
- fault 分类符合读/写 page fault（scause=13/15）:contentReference[oaicite:11]{index=11}

---

### T10：TLB 一致性（映射更新后必须可见）
**目的**：验证映射改变后，访问能看到最新映射；并验证 `SFENCE.VMA` 的必要性/实现策略。

**背景**
- RISC-V 要求用 `SFENCE.VMA` 同步页表更新与地址转换缓存；且写 satp 不隐含 flush。:contentReference[oaicite:12]{index=12}  
- CPU 会缓存 PTE 到 TLB，页表变化后需使缓存失效。:contentReference[oaicite:13]{index=13}

**步骤**
1. map `VA->PA1`，向 `PA1` 写入 A
2. 访问 `*(VA)` 读到 A（保证 TLB 填充）
3. 更新映射 `VA->PA2`，向 `PA2` 写入 B
4. 执行 `sfence.vma`（全局或按 VA/ASID 细粒度）
5. 再读 `*(VA)`，应读到 B

**断言**
- flush 后必须读到 B
- 若你的 `map_pages/unmap_pages` 内部自动 flush，也要在文档注明（并仍建议测“是否确实刷新”）

---

### T11：satp 切换回归（切换前后地址空间行为正确）
**目的**：验证切换页表后，访问语义符合新页表；并验证是否需要显式 `SFENCE.VMA`。

**步骤**
1. 准备 `ptA`、`ptB`：
   - `ptA`：`VA->PA1 (A)`
   - `ptB`：`VA->PA2 (B)`
2. `switch_satp(ptA)`；必要时 `sfence.vma`
3. 读 `VA` -> A
4. `switch_satp(ptB)`；必要时 `sfence.vma`
5. 读 `VA` -> B

**断言**
- 切换后读值随页表变化而变化
- 不出现“旧翻译残留”（若出现，说明缺少/错误使用 `SFENCE.VMA`）:contentReference[oaicite:14]{index=14}

---

### T12：Page fault 诊断输出（分类与关键信息一致）
**目的**：验证 trap 诊断“不是打印演示”，而是和硬件寄存器一致。

**步骤**
触发三类 fault：
- 未映射取指 / 读 / 写
- 权限不足的取指 / 读 / 写（通过 T06 的权限矩阵触发）

**断言**
- `scause` 与访问类型一致（12/13/15）:contentReference[oaicite:15]{index=15}
- `stval` 等于 fault VA（或落在 fault VA 范围内）:contentReference[oaicite:16]{index=16}
- 诊断输出/结构化结果（推荐结构化）与上述寄存器一致

---

## 4. 常见“假通过”风险清单（务必规避）
- 只检查 PTE 位就声称“权限 OK” → 必须真实访问触发/不触发 fault。
- 做纯算术循环就声称“TLB miss/flush OK” → 必须通过映射变化 + 访问结果验证。
- remap 不检查返回值 → 测试结果随机、且易掩盖 bug。
- 选用低 VA（0x0/0x1000）→ 可能踩 NULL guard 或保留区导致误判。
- 认为写 satp 会自动 flush → RISC-V 明确不保证，需要 `SFENCE.VMA`。:contentReference[oaicite:17]{index=17}

---

## 5. 建议的最小化测试执行顺序（从“基础到危险”）
1. T01 生命周期
2. T02/T03 map/walk/unmap
3. T04 remap 策略
4. T05 canonical VA（Sv39）
5. T10 TLB flush
6. T11 satp 切换
7. T06 权限矩阵（需要 fault harness）
8. T07–T09 copyin/copyout
9. T12 诊断一致性

---

## 6. 参考依据（用于对齐实现/断言）
- RISC-V Privileged Spec（Sv39 canonical 地址规则；SFENCE.VMA 语义；satp 写入不隐含 flush）。:contentReference[oaicite:18]{index=18}  
- RISC-V page fault 的 scause/stval 使用方式（12/13/15 与 fault VA）。:contentReference[oaicite:19]{index=19}  
- xv6-riscv 教材对 copyin/copyout 的定位：它们需要显式翻译用户虚拟地址，属于 VM correctness 范畴。:contentReference[oaicite:20]{index=20}
