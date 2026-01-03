# Simple Ollama C++ Client

一个基于 C++ 实现的简单终端 AI 客户端，通过 HTTP 接口连接本地的 Ollama 服务。

项目初衷是为了练习 C++ 的面向对象编程、内存管理以及网络通信，同时也解决了一些在 Windows 环境下连接本地服务的常见问题（如代理拦截）。

## 功能实现

该代码实现了以下核心功能：

1.  **基础对话**：能够与本地 Ollama (如 qwen, llama3) 进行问答交互。
2.  **上下文记忆 (Sliding Window)**：
    - 程序在内存中维护一个 `m_context_buffer`。
    - 实现了简单的滑动窗口算法：当对话记录超过 2000 字符时，自动从头部裁剪旧记录，防止 Token 溢出。
3.  **对话日志持久化**：
    - 使用 `std::ofstream` 将所有对话记录实时追加保存到本地的 `chat_history.txt` 文件中。
4.  **RAII 资源管理**：
    - 使用 `std::unique_ptr` 管理 HTTP 客户端指针，避免手动 delete 带来的内存风险。
5.  **Windows 环境适配**：
    - 代码中内置了 `putenv` 逻辑，强制清空代理环境变量，解决本地连接被 VPN/代理软件拦截的问题。

## 依赖库

本项目使用了以下两个单头文件库（已包含在源码中）：
- **cpp-httplib**: 用于发送 HTTP POST 请求。
- **nlohmann/json**: 用于解析 Ollama 返回的 JSON 数据。

## 编译与运行

### 环境要求
- Windows (MinGW-w64)
- VS Code (推荐)

### 编译命令
如果在 VS Code 中已配置好 `tasks.json`，直接按 `Ctrl+Shift+B`。
或者在终端手动编译：

```bash
g++ main.cpp -o main.exe -lws2_32
```

### 运行

确保本地 Ollama 已启动 (`ollama serve`)，然后在终端运行：

```
.\main.ex
```

## 注意事项

- 默认连接的模型是 `qwen2.5-coder:7b`，如需更改请在 `main.cpp` 中修改 `bot` 对象的初始化参数。
    
- 对话记录保存在程序同级目录的 `chat_history.txt` 中。