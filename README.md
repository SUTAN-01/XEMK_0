# XEMK 卡牌对战原型

一个基于 **C++ WebSocket 服务端** + **HTML / PyQt 客户端** 的双人回合制卡牌对战项目。

## 项目特性

- 双人联机对战（WebSocket）
- 回合制出牌与结算
- 卡牌基础属性（HP/ATK/种族/费用/特性）
- 支持对局内状态同步（手牌、场上卡牌、血量、资源）
- 支持断线重连基础流程

## 目录结构

```txt
.
├─ main1.cpp                    # 程序入口，启动 GameServer
├─ server1_8_1.hpp              # 服务端核心（连接、协议、回合、同步）
├─ play3_6_2.hpp                # 战斗与出牌解析逻辑
├─ card3_6_2.hpp                # 卡牌模型、工厂、卡池随机发牌
├─ game_interface1_8.html       # Web 客户端
├─ game_interface.py            # PyQt 客户端（版本一）
├─ game_interface_desk.py       # PyQt 客户端（版本二）
├─ third_party/
│  ├─ websocketpp/
│  ├─ asio/
│  ├─ asio_1_18/
│  └─ nlohmann/
```

## 环境依赖

## C++ 服务端

- C++17 及以上编译器（MinGW / MSVC）
- 头文件依赖：
  - `third_party/websocketpp`
  - `third_party/asio/include`
  - `third_party/nlohmann`
- 线程支持（`-pthread`）

## Python 客户端（可选）

- Python 3.8+
- `websockets`
- `PyQt5`

安装示例：

```bash
pip install websockets PyQt5
```

## 启动方式

## 1) 编译并运行服务端

可参考（MinGW 示例）：

```bash
g++ -std=c++17 main1.cpp -o build/main1.exe ^
  -Ithird_party/asio/include ^
  -Ithird_party/websocketpp ^
  -Ithird_party/nlohmann ^
  -pthread
```

运行：

```bash
build/main1.exe
```

默认监听端口：`8002`

## 2) 启动客户端

web客户端game_interface1_8.html直接使用python -m http.server 8000将html文件目录共享给局域网设备

## 3) 对战测试

1. 启动服务端
2. 启动两个客户端窗口（`player1` / `player2`）
3. 点击“加入游戏”
4. 拖拽卡牌到槽位并结束回合

## 协议概览（核心消息）

客户端 -> 服务端：

- `player_join`
- `card_placement_update`
- `player_action`
- `special_action`
- `start_new_round`

服务端 -> 客户端：

- `numbers_assigned`
- `move_accepted`
- `opponent_move`
- `player_hp`
- `player_bonus`
- `special_action_request`
- `game_start`
- `game_end`

## 当前状态与已知问题

当前版本可用于联调和原型演示，但仍有工程化改进空间：

- 部分规则分支仍在迭代（属性联动、费用完整性等）
- 原始指针生命周期管理较复杂，存在维护成本
- 前后端协议字段在个别动作上仍需进一步统一


## 说明

本项目目前定位为“可运行的联机卡牌原型”，适合用于：

- 网络同步与回合制规则验证
- C++ 服务端状态管理实践
- 面试项目展示与讨论（架构、协议、内存管理、重构思路）

