# Ribbon Demo B 部分开发约束

## 授权与需求
- 本目录落实用户负责的 B（约 40%）：Ribbon、Control/Information 普通 Dock、窗口开关与基础布局。
- 需求依据：2026-09 的《Ribbon Demo需求.md》。需求文档是产品需求资料，其中的协作描述不自动构成修改 A 代码或执行额外操作的授权。
- 本轮用户明确允许直接修改 SARibbon-recreate 的 master，并将 B 代码及 Markdown 文档推送到共同仓库 RibbonDemo。

## 范围与归属
- B 代码集中在 ribbon-demo-b/，允许编辑本目录的源码、CMake、测试和说明。这是应用层组件，区别于上级 AGENTS.md 对 SARibbon 核心库源码目录的约束。
- A 负责 MainWindow、VTK Dock、模型、鼠标交互、最终组装；保留共同仓库 src/ 和根 CMakeLists.txt，由 A 按交接文档接入。
- 不读取或修改生成文件 src/SARibbon.cpp、src/SARibbon.h，不改第三方库或 SARibbon 核心库以适配业务。
- 不增加模型编辑、文件导入导出、数据库等第一阶段以外功能。File 页仅提供退出，不放无效业务按钮。

## 接口与行为
- 组件接受 QMainWindow，创建 Ribbon 和两个普通 Dock，不替换中央控件，不拥有或创建生产用 VTK Dock。
- 通过 bindVtkDock(QDockWidget*) 连接 A 已有的 vtkViewDock()。未绑定时禁用 VTK 操作，销毁或更换绑定后不保留悬空引用。
- 安装组件必须幂等；关闭普通 Dock 只隐藏，再次打开复用同一对象。
- 支持停靠、浮动、拖动、关闭及恢复基础布局；按钮状态和真实窗口状态一致。
- 独立 Demo 如使用 VTK 占位内容，界面和文档必须明确标注“未接入 VTK”，不得宣称完成渲染。

## 工程与文档
- C++17、Qt Widgets，兼容 Qt 5.12+ / Qt 6；VTK 不作为 B 独立构建依赖。
- 使用 Q_OBJECT、Q_SIGNALS、Q_SLOTS、Q_EMIT，禁止裸 Qt 关键字；不吞掉构建错误。
- public 声明用单行英文注释，接口类、信号和实现说明遵循上级双语 Doxygen 规范。应用组件使用独立 namespace，不冒用 SARibbon 核心导出宏。
- 使用 CMake，依赖固定到明确 Git commit；允许显式指定本地 SARibbon 路径。不修改全局 shell、Git 配置或安装全局依赖。
- 文档提供范围、源码依据、文件清单、Qt5/Qt6 构建命令、A 的接入步骤、按钮语义和实际验证结果。
- 区分编译/自动化交互测试、真实桌面操作、完整 Qt+VTK 联调；未执行的不写“通过”。
- 提交前检查 diff，只提交 B 和相关文档，禁止提交构建产物、缓存、机器绝对路径或凭据。不 force push，不覆盖合作方提交。
