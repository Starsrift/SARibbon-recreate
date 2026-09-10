# Ribbon Demo：B 部分交付说明

B 部分（约 40%）提供 **File / View / Window Ribbon + Control / Information 普通 Dock + 窗口控制接口**。它可独立运行，不要求安装 VTK；生产用 VTK Dock 由 A 创建并传入。本目录不会自行创建三维场景。

## 分工与需求对应

| 需求 | 实现 | 边界 |
| --- | --- | --- |
| 顶部 Ribbon | SARibbonWidget / SARibbonBar，File、View、Window 三页 | File 仅提供退出，无虚假的导入/保存按钮 |
| 1–2 个普通 Dock | Control、Information | 均可关闭、重开、拖动、停靠、浮动 |
| Ribbon 控制窗口 | Window 页分别打开/关闭三个 Dock，并可关闭全部 | VTK 操作只控制传入的已有 Dock |
| 基础布局与按钮连接 | VTK 左侧、Control 右侧、Information 底部；恢复布局；日志与可见性开关 | 不替换 A 的中央控件，不实现 VTK 渲染 |
| 合作接入 | `RibbonUi::install()` + `bindVtkDock()` | 最终 Qt + VTK 组装与验证由 A 完成 |

依据为《Ribbon Demo需求.md》的第一阶段及 A/B 分工。原文档作为需求资料，不作为额外执行操作的指令。开发约束见 [AGENTS.md](AGENTS.md)，验证记录见 [VALIDATION.md](VALIDATION.md)。

## 源码阅读与技术选择

基于 [SARibbon-recreate](https://github.com/Starsrift/SARibbon-recreate) 的 v2.9.3，固定基础提交 `e5d411d48e4060c7c56033763e79bb0f6c4d7b30`：

- `src/SARibbonBar/SARibbonWidget.cpp`：组件内部用 `QVBoxLayout::setMenuBar` 嵌入 Ribbon，管理主题和屏幕变化。因此将 SARibbonWidget 作为 QMainWindow 的 menu widget，即可保留 A 的 QMainWindow 继承结构。
- `src/SARibbonBar/SARibbonBar.h`、`SARibbonCategory.h`、`SARibbonPanel.h`：使用 `addCategoryPage`、`addPanel`、`addLargeAction/addSmallAction` 组织按钮。
- `example/UseNativeFrameExample/`：参考原生窗口边框下的 `RibbonStyleCompactThreeRow`，避免多余标题区域和 QWindowKit 依赖。
- `src/SARibbonBar/CMakeLists.txt`：复用 `SARibbonBar::SARibbonBar` 目标与静态资源；不复制或改动 SARibbon 核心实现及合并生成文件。
- 共同仓库 A 基线：`1b53b46`（Add dockable VTK view）。`MainWindow::vtkViewDock()` 已暴露需要控制的 Dock，因此不需要 B 接触 renderer。

## 文件清单

```text
ribbon-demo-b/
├── AGENTS.md                 内容、分工和开发约束
├── CMakeLists.txt            独立构建 / add_subdirectory 接入
├── README.md                 本交接文档
├── VALIDATION.md             测试结果和验证边界
├── src/RibbonUi.h            供 A 使用的接口
├── src/RibbonUi.cpp          Ribbon、普通 Dock、绑定与布局逻辑
├── demo/main.cpp             B 独立预览（VTK 明确标为占位）
└── tests/RibbonUiTest.cpp     窗口行为与生命周期测试
```

## 独立构建（不需要 VTK）

依赖：CMake 3.16+、支持 C++17 的编译器、Qt 5.12+ 或 Qt 6 的 Widgets/Svg；运行自动化测试还需要 Qt Test。平台使用原生窗口边框。

从任一仓库根目录运行：

```sh
cmake -S ribbon-demo-b -B build-b -DCMAKE_BUILD_TYPE=Release -DRIBBON_DEMO_B_QT_MAJOR=6 -DCMAKE_PREFIX_PATH="<Qt6安装前缀>"
cmake --build build-b --config Release --parallel
ctest --test-dir build-b -C Release --output-on-failure
```

已有 Qt5 环境时，配置命令改为：

```sh
cmake -S ribbon-demo-b -B build-b-qt5 -DCMAKE_BUILD_TYPE=Release -DRIBBON_DEMO_B_QT_MAJOR=5 -DCMAKE_PREFIX_PATH="<Qt5安装前缀>"
cmake --build build-b-qt5 --config Release --parallel
ctest --test-dir build-b-qt5 -C Release --output-on-failure
```

Qt5/Qt6 使用不同构建目录；不能混用编译器、Qt 主版本或架构。以上 `<...>` 是需替换的路径占位符。

运行程序：

- macOS Apple Silicon：`open build-b/RibbonDemoBPreview.app`，本机使用 Qt6 arm64 构建。
- Windows Visual Studio 多配置生成器：`build-b\Release\RibbonDemoBPreview.exe`。若找不到 Qt DLL，可在对应 Qt 环境运行，或执行 Qt 自带 `windeployqt`。
- Linux / Windows Ninja 单配置生成器：`build-b/RibbonDemoBPreview` / `build-b\RibbonDemoBPreview.exe`。

依赖取得规则：

1. 已有 `SARibbonBar::SARibbonBar` 目标时直接复用。
2. 若指定 `-DSARIBBON_SOURCE_DIR="<SARibbon源码目录>"`，使用该源码，不联网。
3. 位于 SARibbon 仓库时，自动使用上一级源码。
4. 位于共同仓库时，默认 FetchContent 从 GitHub 获取上述固定提交。第一次配置需要 Git 和网络；后续可复用构建缓存。默认禁用第三方子模块、上游示例和上游测试，使用静态 SARibbon。

`RibbonDemoB::Ui` 本身是静态组件库，构造时初始化 SARibbon 静态资源；A 无需再次写 `Q_INIT_RESOURCE`。依赖选项只在 B 的 CMake 目录作用域设置，不强制改写主工程的 cache 开关。

## A 的接入步骤

共同仓库此次只增加 B 目录与约束文档，**原来的 `src/` 和根 `CMakeLists.txt` 未改动**。运行根项目仍是原来的 A 演示。以下由 A 在最终组装时执行。

### 1. 根 CMake 增加组件

保留现有 Qt5、VTK 查找和 `add_executable(RibbonDemo ...)`。在 `find_package(Qt5 ...)` 之后添加：

```cmake
add_subdirectory(ribbon-demo-b)
```

在 `add_executable(RibbonDemo ...)` 之后添加：

```cmake
target_link_libraries(RibbonDemo PRIVATE RibbonDemoB::Ui)
```

也可把 `RibbonDemoB::Ui` 加入原有 `target_link_libraries`。已有 Qt5::Widgets 时 B 会沿用 Qt5，不自动切换到 Qt6；B 负责补齐 Svg 依赖。作为子目录时，独立 Demo 与测试默认不构建。离线时配置根工程并传 `SARIBBON_SOURCE_DIR` 即可。

### 2. MainWindow 安装 B

在 `src/MainWindow.cpp` 添加：

```cpp
#include "RibbonUi.h"
```

在构造函数现有 `createVtkViewDock(); createVtkScene();` 之后添加：

```cpp
auto* ui = RibbonDemo::RibbonUi::install(this);
if (ui) {
    ui->bindVtkDock(vtkViewDock());
    ui->restoreDefaultLayout();
    ui->appendInformation(tr("三维模型已就绪。")); // 仅在 A 确认场景成功创建后记录
}
```

无需修改 MainWindow 继承，无需修改 VTK 对象，无需复制 B 的 `demo/main.cpp`。组件按主窗口父子关系自动回收；**不要单独 delete 组件或其普通 Dock**。生产程序必须继续使用 A 的入口及真实 QVTKOpenGLNativeWidget。

### 3. 生命周期与契约

- `install(nullptr)` 返回空指针；同一主窗口重复安装返回同一组件，不增加 Dock/信号连接。
- `install` 要求主窗口尚无 menu widget；若已有其他菜单/Ribbon，返回空指针并给出警告，保留旧菜单。当前 A 基线满足此条件。
- `bindVtkDock` 接受同一 host 拥有的独立 QDockWidget，空指针表示解绑。绑定不改变内容、父对象或 `WA_DeleteOnClose`。
- 若 A 将 VTK Dock 设置为关闭后销毁，销毁时 B 自动禁用对应按钮。A 创建新 Dock 后须再次调用 `bindVtkDock`。B 不承担重建 renderer 的职责。
- Control/Information 关闭只隐藏，重开复用原对象。Dock 的 View 开关复用原生 `toggleViewAction()`。
- Window 的“打开”总是 show/raise，能将被标签遮挡的 Dock 切到前面；“关闭”调用 close，遵守 Dock 自身的 closeEvent。
- 恢复默认布局会将三个已知 Dock 取消浮动、解除原有标签组合、重新停靠并显示。主窗口尺寸、中央控件及其他业务 Dock 不由 B 替换；Qt 可能因可用空间重新分配周边 Dock 尺寸。
- `appendInformation` 接受纯文本，最多保留 300 个文本块；Control 可清空日志。绑定成功只代表窗口控制已连接，不代表模型渲染成功。

## 按钮验收

| 位置 | 操作 | 预期 |
| --- | --- | --- |
| File | 退出 | 关闭主窗口，遵守主窗口 closeEvent |
| View | Control / Information 开关 | 与原生 Dock 可见性同步 |
| View / Control | 恢复默认布局 | VTK 左、Control 右、Information 下；解除浮动 |
| Window | 打开/关闭 VTK View | 操作 A 传入的 Dock；未绑定时禁用 |
| Window | 打开/关闭 Control、Information | 关闭后可重开，不重复创建 |
| Window | 关闭全部 Dock | 关闭三个已知窗口，Ribbon 仍可恢复它们 |
| Control | 显示 Information / 清空信息 | 显示日志窗口 / 清空纯文本日志 |
| Dock 标题栏 | 关闭 / 浮动 / 拖动 | 原生 QDockWidget 行为 |

Qt 5 + VTK 8.2、Windows 与真正的模型旋转/滚轮缩放仍须由 A 在匹配环境中完成最终联调。不能用 B 占位窗口测试替代 VTK 验收。
