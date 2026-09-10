# B 部分验证记录

验证日期：2026-09-10。范围为本目录的 Ribbon、普通 Dock 和已有 Dock 的窗口控制。**不把 VTK 占位窗口当作真实渲染验证。**

## 环境与基础版本

- macOS 27.0，Apple Silicon / arm64。
- Qt 6.11.2、CMake 4.4.3、Apple Clang 21.0.0，Release / C++17。
- SARibbon 基础版本 v2.9.3，`e5d411d48e4060c7c56033763e79bb0f6c4d7b30`。
- 共同仓库 A 基线 `1b53b46`；本次 B 交付未修改原有 `src/` 和根 `CMakeLists.txt`。

## 构建与自动化交互

| 路径 | 方法 | 结果 |
| --- | --- | --- |
| SARibbon 仓库中的 B | 使用相邻 SARibbon 源码，Qt6，构建静态组件、预览程序、测试 | 通过 |
| 共同仓库中的 B | 独立配置，FetchContent 自动拉取固定提交，不指定本地 SARibbon 路径 | 通过；实际取得的 HEAD 与上述固定提交一致 |
| 两个构建目录的 CTest | `ctest --test-dir <build> --output-on-failure`，offscreen 平台 | 各 1/1 CTest 套件通过；7 个行为用例通过，另有 init/cleanup，共 9 passed、0 failed |
| A 风格的宿主接入 | 临时 Qt6 QMainWindow 宿主，`add_subdirectory` + `RibbonDemoB::Ui` | 编译和 offscreen 运行成功；中央控件和外部 Dock 的父对象保持不变；宿主 BUILD_TESTS 开关保留，B 独立目标默认关闭 |

7 个行为用例：

1. 安装幂等、保留中央控件、三个页面、未绑定 VTK 时禁用按钮。
2. 存在其他菜单时拒绝安装并保留原菜单。
3. 关闭/重开复用对象、View 勾选状态同步、关闭全部后恢复。
4. 浮动、关闭后重开、程序化标签组合、恢复停靠区域。
5. 绑定/重绑 VTK Dock、不改变内容与所有权、旧对象销毁不影响新绑定、`WA_DeleteOnClose` 后禁用操作。
6. 显式解绑、拒绝其他宿主或普通 Dock 的错误绑定。
7. QTest 点击真实按钮控件、纯文本日志、日志清空与 300 块上限、退出主窗口。

测试使用 offscreen 插件时会输出不支持 propagateSizeHints/raise 等平台提示；这些自动化结果不能独立证明原生桌面布局与拖动。因此另外进行了下面的真实桌面检查。

## 真实桌面操作（CUA）

针对编译后的 `RibbonDemoBPreview.app` 实际操作并检查可访问性状态与截图：

- 查看 File / View / Window 页面，确认标签和功能按钮显示。
- 点击 Ribbon 关闭 VTK 占位 Dock，再点击打开，确认窗口消失及重新出现。
- 点击 Control 标题栏浮动按钮，确认出现独立 Control 窗口；点击恢复布局后确认重新停靠。
- 拖动 Control 标题栏进入 VTK 区域，确认形成 Control / VTK 标签组合；点击恢复布局后解除组合。
- 在 View 页隐藏并重新显示 Information，确认窗口状态变化。
- 复查默认布局，保留较大的左侧工作区；修正 View 长标签挤压后再次查看，Information 文字完整显示。

这不是所有屏幕尺寸、缩放率或原生操作系统的穷举测试。独立预览中 VTK 区域始终明确显示“未接入 VTK”。

## 警告与未验证项

- SARibbon 上游 `SARibbonBar.cpp` 在 Qt6.11.2 下产生 2 处 QHoverEvent 过时构造函数警告，B 新代码未产生编译警告。没有为本次业务改动上游库。
- 本机缺少 Qt5 和 VTK 开发环境，**未编译/运行共同仓库原来的 Qt5 + VTK8.2 程序**。B 使用 Qt5/Qt6 共有 API 并提供显式选择与沿用宿主 Qt 的路径，但 Qt5 二进制兼容结果待相应环境验证。
- Windows/MSVC、Linux、Qt5 路径未实际运行；文档命令是复现步骤，不代表已经跨平台验收。
- A 必须在匹配的 Qt/VTK 环境按 README 接入真实 `vtkViewDock()`，验证三维模型、鼠标旋转、滚轮缩放，以及真实 VTK Dock 浮动/重开后的 OpenGL 渲染。
- 最终 Qt+VTK 主窗口组装仍属于 A；B 独立预览及临时宿主测试不代替该验收。
