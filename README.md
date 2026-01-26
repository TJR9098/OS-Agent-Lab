# OS-Agent Lab

本仓库用于对比不同 Agent（例如 GPT-5.2-Codex、Trae 系列）在不同 Prompt 下编写操作系统代码的效果。

仓库包含：
- 每个 Agent 的 Prompt（用于驱动生成）
- 每个 Agent 产出的代码（可构建、可运行、可验证）

---

## 目录结构总览

```text
.
├── GPT-5.2-Codex/        # Agent：GPT-5.2-Codex
│   ├── Prompt/           # 该 Agent 的提示词
│   └── code/             # 该 Agent 生成的 OS 代码与工具链
└── Trae/                 # 使用 Trae IDE（包含：Auto / MiniMax-2.1 等Agent）
    ├── Auto/
    └── MiniMax-2.1/
```

# 如何运行

本项目的每个代码目录均提供对应的 **Makefile**。推荐按以下步骤执行：

1. **编译**
   
   ```bash
   make

2. **运行**

   ```bash
   make run
   ```

如需从零开始重新构建，可先清理构建产物：

```bash
make clean
```
