# 仿QQ聊天室(多客户端聊天，文件下载上传，Reactor服务器)

一个基于 C++、Linux Socket/epoll 和 Qt Widgets 的局域网即时聊天室项目。项目包含图形化客户端、epoll 服务端，以及一个用于学习事件驱动模型的 Reactor 服务端版本，支持用户登录、群聊、在线用户列表、文件上传和文件下载。

## 功能特性

- Qt 图形化聊天客户端
- TCP 长连接通信
- 用户名登录与在线用户列表同步
- 群聊消息广播
- 系统消息提示，如用户加入、离开、上传文件
- 文件上传、文件列表刷新和文件下载
- 自定义二进制应用层协议，支持粘包/半包处理和简单校验
- Linux epoll 非阻塞 I/O 服务端
- 可选 Reactor 事件分发版本，便于学习网络服务器架构

## 技术栈

- C++17
- CMake 3.16+
- Qt 5/Qt 6 Widgets、Network
- Linux Socket API
- Reactor网络模型
- epoll
- std::filesystem

## 项目结构

```text
IM_Chat/
├── Client/                 # Qt 客户端
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   ├── mainwindow.ui
│   └── protocol.h
├── Server/                 # 默认 epoll 服务端
│   ├── main.cpp
│   ├── chatserver.cpp
│   ├── chatserver.h
│   ├── filetransfer.h
│   └── protocol.h
├── Server_Reactor/         # Reactor 版本服务端
│   ├── include/
│   ├── src/
│   ├── chatserver.cpp
│   ├── chatserver.h
│   ├── filetransfer.h
│   └── protocol.h
├── CMakeLists.txt
└── readme.md
```

## 环境要求

服务端依赖 Linux 网络接口和 epoll，两个版本都可以使用，只是实现的方式不一样。

客户端依赖 Qt：

- Qt 5 或 Qt 6
- Qt Widgets 模块
- Qt Network 模块

如果只想运行服务端，可以不安装 Qt，并在 CMake 中关闭客户端构建。

## 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/<your-name>/IM_Chat.git
cd IM_Chat
```

### 2. 编译服务端(Linux系统)

适合没有 Qt 环境，或只想先启动聊天服务器的情况。

```bash
cmake -S . -B build -DBUILD_CLIENT=OFF -DBUILD_SERVER=ON
cmake --build build
```

启动服务端：

```bash
./build/Server/im_server
或者
./build/Server_Reactor/im_server
```

服务端默认监听 `8888` 端口，并在运行目录下创建 `uploads/` 目录保存上传文件。

### 3. 编译客户端

需要本机已安装 Qt，并且 CMake 能找到 Qt。

如果使用 Qt Creator，也可以直接打开根目录的 `CMakeLists.txt`，选择合适的 Kit 后构建运行。

![image-20260521095011639](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20260521095011639.png)

## 使用说明

1. 先启动服务端。
2. 启动一个或多个客户端。
3. 在客户端填写服务器 IP 和端口，默认端口为 `8888`。
4. 点击连接服务器。
5. 输入用户名后登录(任意都可以登录)。
6. 登录成功后即可发送群聊消息、查看在线用户、上传或下载文件。

客户端默认服务器地址可通过界面修改，也可以通过环境变量配置：

```bash
export IM_CHAT_SERVER_HOST=127.0.0.1
export IM_CHAT_SERVER_PORT=8888
```

在Client文件下的mainwindow.cpp可以修改服务器地址和端口

![image-20260521095220085](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20260521095220085.png)

## 协议设计

客户端和服务端使用统一的 `protocol.h`。每个数据包由包头、长度、命令字、负载和校验值组成。

```text
+------------+------------+------------+---------------+------------+
| PacketHead | Length     | Command    | Payload       | Checksum   |
| uint16     | uint32     | uint16     | variable      | uint16     |
+------------+------------+------------+---------------+------------+
```

- `PacketHead`：固定为 `0xFEFF`
- `Length`：`Command + Payload + Checksum` 的长度
- `Command`：业务命令类型
- `Payload`：业务数据
- `Checksum`：对 Payload 做简单累加校验

主要命令包括：

| 命令 | 含义 |
| --- | --- |
| `CmdLogin` | 客户端登录 |
| `CmdLoginOk` | 登录成功 |
| `CmdSystemMessage` | 系统消息 |
| `CmdChatMessage` | 聊天消息 |
| `CmdUserList` | 在线用户列表 |
| `CmdFileList` | 文件列表 |
| `CmdFileUploadBegin` | 开始上传文件 |
| `CmdFileUploadChunk` | 文件上传分片 |
| `CmdFileUploadEnd` | 文件上传结束 |
| `CmdFileDownloadRequest` | 请求下载文件 |
| `CmdFileDownloadBegin` | 开始下载文件 |
| `CmdFileDownloadChunk` | 文件下载分片 |
| `CmdFileDownloadEnd` | 文件下载结束 |
| `CmdError` | 错误消息 |

## 服务端说明

默认服务端位于 `Server/`，采用非阻塞 socket 和 epoll：

- 监听新连接
- 为每个客户端维护会话状态
- 解析自定义协议数据包
- 广播聊天消息和用户列表
- 处理文件上传和下载
- 清理断开连接和未完成上传

`Server_Reactor/` 目录提供了 Reactor 架构版本，将事件监听、事件分发和业务处理拆分为独立模块，适合对比学习 epoll 直接编程和 Reactor 模式。

## CMake 选项

根目录 `CMakeLists.txt` 提供以下选项：

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `BUILD_CLIENT` | `ON` | 构建 Qt 客户端 |
| `BUILD_SERVER` | `ON` | 构建默认 epoll 服务端 |
| `BUILD_SERVER_REACTOR` | `OFF` | 构建 Reactor 版本服务端 |

示例：

```bash
cmake -S . -B build \
  -DBUILD_CLIENT=OFF \
  -DBUILD_SERVER=ON \
  -DBUILD_SERVER_REACTOR=OFF
```

## 后续计划

- 增加账号注册和密码校验
- 支持私聊
- 增加聊天记录持久化
- 增加数据库存储用户和文件元信息
- 优化客户端界面布局
- 增加更完整的异常处理和日志系统

## License

本项目用于 C++ 网络编程和 Qt 客户端开发学习。发布到 GitHub 前可以根据需要补充开源协议，例如 MIT License。
