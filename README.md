<div align="center">
<img width="256" height="256" alt="image" src="https://github.com/user-attachments/assets/5859124e-af85-4712-8601-150a99383969" />

# ArWindow

> 轻量级 Windows 窗口管理器 — 列出、查看、强制控制所有可见窗口

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-0078D6.svg?logo=windows&logoColor=white)
![Qt](https://img.shields.io/badge/Qt6-gray?logo=qt)
![C++](https://img.shields.io/badge/C%2B%2B%2017-gray?logo=c%2B%2B&logoColor=%2300599C)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C.svg)

</div>

ArWindow 是一个基于 Qt 6 的 Windows 桌面工具，可以枚举系统中所有可见窗口，并对其执行最大化 / 最小化 / 还原 / 关闭等操作。针对那些禁用了最大化按钮、禁用关闭、甚至置顶不可关闭的"顽固窗口"，提供**强制操作模式**，绕过窗口自身的样式限制。

## 功能特性

- 🪟 **窗口枚举**：列出所有可见窗口，显示图标、标题、进程 PID、窗口句柄
- ⛶ **窗口控制**：一键最大化 / 最小化 / 向下还原 / 关闭
- 💪 **强制模式**：修改目标窗口的 `WS_MAXIMIZEBOX` / `WS_MINIMIZEBOX` 样式，强制启用被禁用的按钮
- 🛡️ **安全关闭**：强制模式下也仅发送 `WM_CLOSE`，不直接 `TerminateProcess`，给程序留出保存数据的机会
- ⏱️ **超时保护**：使用 `SendMessageTimeoutW` 查询窗口图标，避免目标窗口无响应时拖死本程序
- 🎨 **现代化 UI**：浅色 Material 风格，侧边栏设置页，等待光标反馈
- ⚙️ **持久化设置**：强制操作开关通过 `QSettings` 保存

## 截图

<img width="902" height="632" alt="image" src="https://github.com/user-attachments/assets/c0107d14-e45e-459d-8b94-c1ef65cd91c0" />
<img width="502" height="432" alt="image" src="https://github.com/user-attachments/assets/2e231b03-b3e8-4deb-a522-e202341841e5" />
<img width="502" height="432" alt="image" src="https://github.com/user-attachments/assets/37365317-ee82-4564-b825-f8dfe5b7d86e" />

## 环境要求

| 项 | 版本 |
|---|---|
| 操作系统 | Windows 7 及以上 |
| Qt | 6.0 及以上（开发基于 6.11.1） |
| 编译器 | MinGW / Clang-MinGW / MSVC（推荐 llvm-mingw） |
| C++ 标准 | C++17 |

## 构建方法

### 方式一：Qt Creator（推荐）

1. 打开 Qt Creator
2. `File → Open File or Project`，选择 `ArWindow.pro`
3. 选择对应的 Kit（如 `Desktop_Qt_6_11_1_llvm_mingw_64_bit`）
4. 点击 **Run** 或 `Ctrl+R` 即可编译运行

### 方式二：命令行

```bash
# 进入项目目录
cd ArWindow

# 生成 Makefile
qmake ArWindow.pro

# 编译（Release）
mingw32-make -j4

# 生成的可执行文件位于
# ./bin/ArWindow.exe  （或 ./build/<kit>-Release/bin/ArWindow.exe）
```

## 使用说明

1. 启动 ArWindow，主窗口自动枚举当前所有可见窗口
2. 在列表中选中目标窗口
3. 点击下方按钮执行操作：
   - **刷新列表**：重新枚举窗口
   - **最大化 / 最小化 / 向下还原**：常规窗口操作
   - **关闭窗口**：发送 `WM_CLOSE`（会弹确认对话框）
   - **设置**：打开设置对话框
4. 如果目标窗口禁用了最大化/最小化按钮，进入 **设置 → 通用**，勾选 **强制操作**，再重新执行操作
5. 强制模式下点击"关闭窗口"会有二次确认，避免误操作

## 项目结构

```
ArWindow/
├── ArWindow.pro          # qmake 工程文件
├── main.cpp              # 程序入口
├── mainwindow.{h,cpp,ui} # 主窗口（窗口列表 + 操作按钮）
├── settingsdialog.{h,cpp,ui} # 设置对话框（通用/关于）
├── appicon.ico           # 应用图标
└── LICENSE               # MIT 许可证
```

## 技术实现

- **窗口枚举**：`EnumWindows` + 回调 `EnumWindowsProc`，过滤不可见窗口、空标题窗口和自身进程窗口
- **图标获取**：依次尝试 `WM_GETICON` → `GetClassLongPtr(GCLP_HICON)` → `SHGetFileInfoW`（按进程可执行文件路径），仅销毁 `SHGetFileInfo` 返回的图标，避免破坏目标窗口的图标资源
- **强制操作**：`GetWindowLongPtr(GWL_STYLE)` 修改样式位 → `SetWindowLongPtr` 写回 → `SetWindowPos(SWP_FRAMECHANGED)` 让新样式生效 → `ShowWindow` 执行操作
- **样式刷新**：MSDN 明确要求 `SetWindowLongPtr` 修改样式后必须调用 `SetWindowPos` 重新刷新窗口框架
- **设置持久化**：`QSettings("ArWindow", "Settings")` 写入注册表 `HKEY_CURRENT_USER\Software\ArWindow\Settings`

## 开发说明

- 项目使用 Qt 的自动信号槽连接（`on_<objectName>_<signal>` 命名约定），无需手动 `connect`
- 全局样式表定义在 `MainWindow` 和 `SettingsDialog` 构造函数中，可通过修改 `setStyleSheet` 调整主题
- 非强制模式下的"最大化/最小化"会受目标窗口样式限制，这是预期行为，不是 Bug

## 已知限制

- 仅支持 Windows 平台（依赖 Win32 API）
- 强制模式无法关闭被 `SetWindowPos(HWND_TOPMOST)` 置顶且拦截了 `WM_CLOSE` 的窗口（这种窗口需要更强力的注入手段）
- 窗口图标采用同步获取，目标窗口超过几十个时刷新会有轻微卡顿

## 开源协议

本项目基于 [MIT License](./LICENSE) 开源。

## 版权声明

Copyright © 2026 Ar Studio. All Rights Reserved.

基于 Qt 开发，Qt 的版权归 The Qt Company 所有。
