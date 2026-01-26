# Stage1：RISC-V 交叉编译构建环境复现（含安装命令）

**目标**：在本机可复现地搭建并验证一个用于“交叉编译 RISC-V（riscv64）操作系统 + QEMU 运行”的构建环境。  
**强约束**：任何不明确的地方**必须提问**，并在用户答复后再继续执行。

## Clarification Gate（必须先问清楚的点）

1. **如果用户使用的是 Windows，必须先确认：期望使用 WSL 还是原生 Windows 环境。**
   - 若用户未明确选择：优先建议 **WSL**（可直接获得 Linux 发行版与 Bash/包管理器，降低工具链与依赖差异）。
   - 若选择 **WSL**：后续所有“Linux/Ubuntu 安装与命令”均在 WSL 发行版内执行；先检查当前是否已经安装 WSL 。若没有安装，继续追问发行版与版本（如 Ubuntu 22.04/24.04）。
   - 若选择 **原生 Windows**：后续步骤按 Windows 工具链（PowerShell/Chocolatey/手动安装）执行，并明确：QEMU/OpenSBI/交叉工具链需保证 PATH 与固件文件可见。

---

## 0) 记录基础信息（必须输出）

请先采集并输出以下信息（以“环境报告”的形式记录）：

- 主机 OS / 版本 / 内核版本（Windows/WSL/Linux/macOS）、CPU 架构
- 默认 shell（PowerShell/bash/zsh）
- `PATH` 是否已包含：交叉工具链、QEMU
- 仓库路径与当前工作目录（用于后续 `make`/`python` 执行位置核对）

---

## 1) 交叉工具链（必须）

需要提供 **riscv64 bare-metal** 工具链，满足以下可执行文件存在且可运行：

- `riscv64-unknown-elf-gcc`
- `riscv64-unknown-elf-objcopy`
- `riscv64-unknown-elf-ar`

**前缀处理**：

- 若工具链前缀不是 `riscv64-unknown-elf-`（例如 `riscv64-none-elf-` / `riscv-none-elf-`），必须设定并记录：
  - `CROSS=<实际前缀>`
- 后续所有 Makefile 调用应使用 `${CROSS}gcc` / `${CROSS}objcopy` / `${CROSS}ar`

**验证命令（必须执行并记录输出）**：

```bash
riscv64-unknown-elf-gcc --version
riscv64-unknown-elf-objcopy --version
riscv64-unknown-elf-ar --version
```

若前缀不同，改为：

```bash
${CROSS}gcc --version
${CROSS}objcopy --version
${CROSS}ar --version
```

---

## 2) 构建工具（必须）

需要具备：

- `make`（或 `gmake`）
- `Python 3`
  - 本仓库 Makefile 默认 `PYTHON=python`
  - 若 `python` 不是 3.x，请显式设置 `PYTHON=python3`
- 主机 C 编译器（BusyBox 构建需要 `gcc` 或 `clang`）

**验证命令（必须执行并记录输出）**：

```bash
make --version   # 或 gmake --version
python3 --version || python --version
gcc --version || clang --version
```

---

## 3) QEMU + OpenSBI 固件（（运行必需）

需要具备：

- `qemu-system-riscv64`

**验证命令（必须执行并记录输出）**：

```bash
qemu-system-riscv64 --version
```

**固件检查与兜底（必须写清楚）**：

- 先检查默认固件是否存在（Linux 常见路径）：

  ```bash
  ls -l /usr/share/qemu/opensbi-riscv64-virt-fw_jump.bin || true
  ```

- 若默认固件不存在或 `-bios default` 失败：安装 `opensbi` 并改用显式 `-bios`（Linux 常见路径）：

  ```bash
  ls -l /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin || true
  ```

---

## 4) 安装指引（根据实际系统选择；只安装缺少的包）

> 原则：先用 `command -v <tool>` / `where <tool>`（Windows）判断是否缺失；**只安装缺少项**。  
> 若你不确定用户系统类型（Windows 原生 vs WSL vs macOS vs Linux 发行版），必须先提问确认。

### 4.1 Ubuntu / Debian / WSL(Ubuntu)

**一次性安装（推荐起点；如需更精细请拆分安装缺失项）**：

```bash
sudo apt-get update

# 基础构建工具 + Python3 + 主机编译器
sudo apt-get install -y build-essential make python3 python3-pip

# QEMU（若系统提供 qemu-system-riscv64，优先用它）
sudo apt-get install -y qemu-system-riscv64 || sudo apt-get install -y qemu-system-misc

# OpenSBI 固件（供 -bios default 缺失时显式指定）
sudo apt-get install -y opensbi

# bare-metal RISC-V 交叉工具链（riscv64-unknown-elf- 前缀）
sudo apt-get install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

**常见补充（可选）**：

```bash
# 便于排查版本与路径（可选）
sudo apt-get install -y file git curl
```

### 4.2 macOS（Homebrew）

**安装 Xcode 命令行工具（提供 clang/gcc 等工具链支撑）**：

```bash
xcode-select --install
```

**安装 Homebrew（若未安装）**：

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**安装依赖**：

```bash
brew update

# QEMU（含 qemu-system-riscv64）
brew install qemu

# Python 3
brew install python

# make（Homebrew 的 make 通常以 gmake 提供）
brew install make
```

**RISC-V bare-metal 交叉工具链（两种方式，选其一）**：

- 方式 A：使用 xPack（xpm）安装（需要 Node.js/npm）
  ```bash
  brew install node
  npm install --global xpm@latest
  xpm install --global @xpack-dev-tools/riscv-none-elf-gcc@latest
  ```
  - 说明：该工具链前缀通常为 `riscv-none-elf-`，需设置 `CROSS=riscv-none-elf-`

- 方式 B：手动下载 xPack 发行包并加入 PATH（适合不想装 Node 的场景）
  - 下载对应平台的压缩包 → 解压 → `export PATH=<toolchain>/bin:$PATH`
  - 前缀同样可能不是 `riscv64-unknown-elf-`，需按实际设置 `CROSS=...`

### 4.3 Windows（推荐：WSL）

**安装 WSL 与 Ubuntu（需要管理员权限）**：

```powershell
wsl --install -d Ubuntu
```

安装完成后，进入 Ubuntu（WSL）终端，按 **4.1** 执行 `apt-get` 安装命令。

### 4.4 Windows 原生（不推荐，但可用）

> 注意：Windows 原生环境常见问题是 `make`、路径与类 Unix 工具差异；如遇到大量兼容性问题，应回退到 WSL。

**安装 Chocolatey（管理员 PowerShell）**：

```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force; `
  [System.Net.ServicePointManager]::SecurityProtocol = `
    [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
  iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

**安装依赖**：

```powershell
# Python（任选：python / python3 / python312 等具体包名，以你选择的策略为准）
choco install python -y

# GNU make
choco install make -y

# QEMU（提供 qemu-system-riscv64）
choco install qemu -y

# OpenSBI：Windows 原生通常缺少配套固件文件。
# 建议使用 WSL(Ubuntu) 安装 opensbi 并通过 -bios <fw_jump.bin> 显式指定。

# clang/llvm（用于 BusyBox 等本机构建）
choco install llvm -y
```

**交叉工具链（建议 xPack 手动安装）**：

- 下载 xPack 的 RISC-V bare-metal 工具链 zip/tar.gz
- 解压到固定目录（例如 `C:\toolchains\xpack-riscv\...`）
- 将 `<toolchain>\bin` 加入系统 `PATH`
- 根据实际前缀设置 `CROSS=...`（如 `riscv-none-elf-`）

---

## 5) 环境变量设置（必须输出）

根据用户系统提供示例并要求用户确认其生效：

### Windows PowerShell 示例

```powershell
setx CROSS "riscv64-unknown-elf-"
setx PYTHON "python3"
```

### bash/zsh 示例

```bash
export CROSS=riscv64-unknown-elf-
export PYTHON=python3
```

**PATH 处理**：

- 若工具链路径不在 `PATH`，必须显式追加并记录追加后的关键项：
  - Windows：系统环境变量 PATH
  - Linux/macOS：`~/.bashrc` / `~/.zshrc` / `~/.profile`

---

## 6) 输出最终“环境报告”

必须输出一个可复制的，MD格式的，“环境报告”，内容至少包括：

- OS/版本/CPU 架构/默认 shell
- 所有工具版本（gcc/clang、make、python、qemu、交叉工具链三件套）
- `PATH` 关键项（至少列出与工具链/QEMU相关的目录）
- `CROSS`、`PYTHON` 的最终值
- 是否通过工具可用性验证（逐条列出验证命令与结果）

**验证步骤（必须写清楚并要求用户执行/或由 Agent 执行后记录）**：

1. 进入仓库根目录  
2. 逐项确认工具链与依赖可用（参考命令如下）：

```bash
${CROSS}gcc --version
${CROSS}objcopy --version
${CROSS}ar --version
python3 --version
make --version
qemu-system-riscv64 --version
```

- 若仍有未解决问题或环境差异：提出**明确、可操作**的问题给用户（例如“你的工具链前缀实际是什么？请粘贴 `ls <toolchain>/bin | grep riscv` 输出”）。
