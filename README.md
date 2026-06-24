# SonyHeadphoneControl_WH-1000XM4

在 Windows 11 系统托盘显示 Sony WH-1000XM4 耳机连接状态与电量，单击图标打开控制界面，一键切换降噪 / 环境声 / 语音 / 关闭。

> 纯 Windows 桌面工具，无后端，无遥测。

## 特性

- 🎧 托盘图标显示耳机连接状态 + 电量百分比（tooltip 悬停两行：电量 + 当前模式）
- 🔇 控制界面 4 模式圆形按钮：降噪 / 环境声音 / 语音 / 关闭，蓝色高亮互斥
- 🔋 单一电量数据源架构：后台重试踩中断连窗口，瞬断保留上一条有效电量不闪烁
- 🌗 自适应浅色/深色主题
- 🇨🇳 全中文界面
- ⚡ 零蓝牙开销刷新：托盘读缓存，查询仅在控制界面进行（60s 轮询）

## 截图

<!-- TODO: 补充托盘 tooltip + 控制界面截图 -->

## 免责声明

**本项目与 Sony Corporation 无任何关联，非官方项目。** Sony、WH-1000XM4 等名称与商标归 Sony Corporation 所有，本项目仅作功能描述性使用。

使用本项目时可能发生的任何损害（包括但不限于耳机固件异常、蓝牙连接问题），由使用者自行负责。Sony 蓝牙协议为逆向所得，非官方公开规范。

## 致谢

本项目基于以下开源项目改造：

- **[WinUI-SonyHeadphonesClient](https://github.com/dustbin1415/WinUI-SonyHeadphonesClient)** by dustbin1415 — WinUI 3 托盘前端（MIT），本项目的直接改造基础
- **[SonyHeadphonesClient](https://github.com/Plutoberth/SonyHeadphonesClient)** by Plutoberth — 跨平台协议核心（MIT，已归档）
- **[Sony Headphones Bluetooth Documentation](https://github.com/ohm-app/sony-headphones-bluetooth-documentation)** by ohm-app — 协议文档（CC0 1.0）

电量查询协议字节来自反编译的 Sony 官方 Songpal 安卓应用，与上游 `Constants.h` 交叉验证一致。详见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

## 安装与构建

### 前置要求
- Windows 11
- Visual Studio 2022（.NET 桌面 + C++ 桌面 + Windows 11 SDK 工作负载）
- .NET SDK 6.0+（项目目标 `net6.0-windows10.0.19041.0`）

### 克隆（含上游 submodule）
```bash
git clone --recurse-submodules <本仓库地址>
cd SonyHeadphoneControl_WH-1000XM4
```
若已克隆未带 submodule：
```bash
git submodule update --init --recursive
```

### 编译
```bash
# Git Bash
export PATH="/c/Program Files/dotnet:$PATH"
vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
msb=$("$vswhere" -latest -requires Microsoft.Component.MSBuild -find "MSBuild/**/Bin/MSBuild.exe" | head -1)
"$msb" src/WinUI-SonyHeadphonesClient.sln -t:Restore -p:Configuration=Debug -p:Platform=x64 -nologo -v:minimal
"$msb" src/WinUI-SonyHeadphonesClient.sln -p:Configuration=Debug -p:Platform=x64 -nologo -v:minimal
```

产物：`src/SonyHeadphonesClient/bin/x64/Debug/net6.0-windows10.0.19041.0/SonyHeadphonesClient.exe`

详细构建说明与改造要点见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

## 使用

1. 运行 `SonyHeadphonesClient.exe`，托盘出现图标。
2. **先在 Windows 蓝牙配对 WH-1000XM4**，再在程序里点连接（左键托盘 → 选 WH-1000XM4）。光连 Windows 蓝牙不够，需程序内建立 RFCOMM 连接。
3. 连接成功后 tooltip 显示电量 + 模式。
4. 左键托盘打开控制界面，切换 4 种模式。
5. 使用时**关掉 Sony 官方 Headphones Connect App**，避免抢连接。

> ⚠️ 上游 SettingPage 后台每秒断开重连蓝牙 socket 做健康检查，是固有设计、耗电主因，暂未修改。详见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

## 项目结构

```
SonyHeadphoneControl_WH-1000XM4/
├─ README.md                      本文件（面向用户）
├─ LICENSE                        MIT（含上游署名）
├─ docs/
│  ├─ 设计文档.md                  方案、可行性、改造清单
│  ├─ DEVELOPMENT.md              开发笔记（工具链/构建/改造要点）
│  └─ modify_vs.ps1               VS 工作负载补装脚本（参考）
├─ src/                           改造后的工程
│  ├─ SonyHeadphonesClient/       C# WinUI3 前端
│  └─ SonyHeadphonesClientDll/    C++ 协议核心 DLL
└─ reference/                     上游参考（git submodule）
   ├─ WinUI-SonyHeadphonesClient-upstream/
   ├─ Plutoberth-SonyHeadphonesClient-upstream/
   └─ sony-headphones-bluetooth-documentation/
```

## 许可证

[MIT](LICENSE)。本项目基于 MIT 许可的上游项目改造，已保留上游版权声明。

## 开发文档

- [docs/设计文档.md](docs/设计文档.md) — 方案设计、可行性分析、改造清单
- [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) — 工具链、构建命令、改造要点、进度
