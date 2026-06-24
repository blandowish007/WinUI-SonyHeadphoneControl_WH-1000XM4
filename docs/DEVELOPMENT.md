# 开发文档

面向二次开发者的内部笔记：工具链、构建命令、改造要点、接续指引。
公开用户文档见根目录 [README.md](../README.md)。

## 工具链

| 工具 | 版本 / 路径 |
|---|---|
| Visual Studio 2022 Community | 17.14（`C:\Program Files\Microsoft Visual Studio\2022\Community`）|
| .NET SDK | 9.0.315（`C:\Program Files\dotnet`）|
| MSVC | 14.44 |
| Windows SDK | 22621 |
| Git | 2.54.0 |
| 工作负载 | .NET 桌面 + C++ 桌面 + Win11 SDK + Roslyn |

无需再装任何东西。若 MSBuild 找不到 .NET SDK，手动加 PATH：
```powershell
$env:Path = "C:\Program Files\dotnet;" + $env:Path
```

## 编译命令

Git Bash 用 `-` 前缀开关（`/nologo` 会被转成路径）：
```bash
base="c:/Users/XUXIANKUN/Desktop/claudecode_work/2026-06-22-工具开发-XM4状态栏控制"
export PATH="/c/Program Files/dotnet:$PATH"
vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
msb=$("$vswhere" -latest -requires Microsoft.Component.MSBuild -find "MSBuild/**/Bin/MSBuild.exe" | head -1)
"$msb" "$base/src/WinUI-SonyHeadphonesClient.sln" -t:Restore -p:Configuration=Debug -p:Platform=x64 -nologo -v:minimal
"$msb" "$base/src/WinUI-SonyHeadphonesClient.sln" -p:Configuration=Debug -p:Platform=x64 -nologo -v:minimal
```

产物：
- `src/SonyHeadphonesClient/bin/x64/Debug/net6.0-windows10.0.19041.0/SonyHeadphonesClient.exe`
- `src/SonyHeadphonesClient/bin/x64/Debug/net6.0-windows10.0.19041.0/SonyHeadphonesClientDll.dll`

## 改造点（相对上游 WinUI-SonyHeadphonesClient）

### C++ DLL 侧（`src/SonyHeadphonesClientDll/`）
- `Constants.h`：新增 `COMMAND_TYPE::COMMON_GET_BATTERY_LEVEL=0x10` / `COMMON_RET_BATTERY_LEVEL=0x11` / `COMMON_NTFY_BATTERY_LEVEL=0x13`，`BATTERY_INQUIRED_TYPE::BATTERY=0`。
- `CommandSerializer`：`serializeBatteryLevelQuery()` → `[0x10,0x00]`；`extractPayload(frame)` 按 sizeBE4 切 payload。
- `BluetoothWrapper`：新增 `sendCommandWithResponse()` + `_recvFrame()`（持久缓冲，正确处理分片/多帧/转义，修复原 `_waitForAck` 对含 60/61/62 字节帧的潜在解析缺陷，仅作用于电量路径，不改 NC 路径）。
- `Headphones`：`queryBatteryLevel()` → 0-100，-1=失败。
- `dllmain`：导出 `int GetBatteryLevel()`。

### C# 前端侧（`src/SonyHeadphonesClient/`）
- `API/Headphone.cs`：P/Invoke `int GetBatteryLevel()`。
- `API/BatteryState.cs`（新建）：全局电量单一数据源。`Query(retries)` 唯一查询入口（后台重试踩中上游断连窗口）；`Current`/`LastValid` 缓存，瞬断保留上一条有效电量不闪烁。
- `Control/TaskbarIcon.xaml/.cs`：2s `DispatcherTimer` 读缓存刷新 tooltip（不查询，零蓝牙开销）。
- `Pages/SettingPage.xaml/.cs`：唯一查询入口——进入立即查（重试 8 次）+ 60s 轮询（重试 4 次）；4 模式圆形图标按钮（降噪/环境声音/语音/关闭）。

### 编码教训
- MSVC 默认按 GBK(936) 解析源文件，C++ 源文件中文注释触发 C4819 + C2447。**C++ 侧注释一律用英文**；中文文案放 C# 前端资源文件（.resw）。
- H.NotifyIcon.WinUI 2.0.108 在 Win11 WinUI3 tooltip 不弹（issue #94 open），**升级到 2.0.115 修复**（WindowsAppSDK 同步 1.3.230724）。
- unpackaged 应用 `SvgImageSource`/`BitmapImage` 加载本地 svg 失败，改用 XAML `Path` 控件直接绘制矢量路径。

### 电量协议（字节已验证）
来源：反编译的 Sony 官方 Songpal 安卓应用（`egaebel/sony-headphones-hack`），与上游 `Constants.h` 的 `NCASM_SET_PARAM=0x68` 交叉验证一致。

| 项 | 值 |
|---|---|
| 通道 DATA_TYPE | `0x0C` (DATA_MDR) |
| 查询载荷（转义前） | `[0x10, 0x00]` |
| 响应命令字节 | `0x11`（RET）/ `0x13`（NTFY）|
| 电量百分比位置 | 响应载荷 byte[2]，0–100 |
| 充电状态位置 | byte[3]：0=未充电 / 1=充电中 / 0xF0=未知 |

帧封装（`packageDataForBt`，上游已有）：
```
0x3E | DATA_TYPE | seq | BE4字节长度 | 载荷 | 累加校验和 | 0x3C
       （中间整段对 60/61/62 转义：sentry=0x3D，60→3D 2C，61→3D 2D，62→3D 2E）
```
源码中 START_MARKER=0x3E '>', END_MARKER=0x3C '<'。

NC/ASM 无 GET 命令（已调研确认），连接默认设降噪模式，不读耳机状态。

### 已知限制（非 bug）
上游 SettingPage 后台循环每轮 `setChanges()`→`_conn.refresh()` 会每秒断开重连蓝牙 socket（disconnect+Sleep500+connect+Sleep500），是上游固有设计（兼做连接健康检查），耗电主因。暂不改（触及上游核心逻辑，风险高）。电量轮询 60s，对整体耗电影响小。

## 进度

- [x] 阶段1：基线编译验证通过
- [x] 阶段2：C++ DLL 加 `GetBatteryLevel` 导出（真机验证）
- [x] 阶段3：C# 托盘电量显示（真机验证）
- [x] 阶段4：控制界面 4 模式圆形图标按钮（真机验证）
- [ ] 阶段5：自启动、图标资源、异常处理、GUI 全中文优化

## 扩展指引
- 加更多命令（环境声等级、Speak-to-Chat）→ 参考 `reference/sony-headphones-bluetooth-documentation/docs/Devices/LinkBuds S.md` 字节表，按 `serializeNcAndAsmSetting` 模式新增。
- 换设备（XM3/XM5）→ 改 `CommandSerializer.cpp` 的 `MAX_STEPS` 与命令常量；电量查询 `[0x10,0x00]` 对单电池设备通用。
- 电量取不到 → 回退 Windows WinRT `GattBatteryService.GetBatteryLevelPercentageAsync`。

## 更新记录
- 2026-06-22：立项，调研与设计文档，装 Git + VS 2022 + .NET SDK 9。阶段1 基线编译通过。
- 2026-06-22：阶段2 完成。C++ DLL 加 `GetBatteryLevel` 导出。
- 2026-06-23：阶段3 完成。C# 托盘电量显示。tooltip 库升级 2.0.108→2.0.115。
- 2026-06-23：阶段4 完成。4 模式圆形图标按钮，自适应主题，字号统一。真机验证通过。
