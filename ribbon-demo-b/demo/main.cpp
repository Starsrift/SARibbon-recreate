#include "RibbonUi.h"
#include "SARibbonBar.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>

/**
 * \if ENGLISH
 * @brief Run the B-only demo with an explicitly labeled non-rendering dock.
 * \endif
 * \if CHINESE
 * @brief 运行 B 独立演示，使用明确标注、无渲染功能的窗口占位控件。
 * \endif
 */
int main(int argc, char* argv[])
{
    SARibbonBar::initHighDpi();
    QApplication app(argc, argv);
    QMainWindow host;
    host.setWindowTitle(QStringLiteral("Ribbon Demo — B 组件演示"));
    host.resize(1100, 750);
    auto* central = new QWidget(&host);
    central->setMaximumWidth(0);
    host.setCentralWidget(central);
    auto* ui = RibbonDemo::RibbonUi::install(&host);
    if (!ui)
        return 1;

    // This stand-in exercises only the binding interface; part A supplies the real dock.
    auto* placeholder = new QDockWidget(QStringLiteral("VTK View — 未接入 VTK"), &host);
    placeholder->setObjectName(QStringLiteral("vtkViewDock"));
    auto* label = new QLabel(QStringLiteral("VTK View\n\n未接入 VTK\n此窗口仅验证 B 部分的打开、关闭、停靠与浮动。"), placeholder);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setMinimumSize(260, 180);
    placeholder->setWidget(label);
    host.addDockWidget(Qt::LeftDockWidgetArea, placeholder);
    ui->bindVtkDock(placeholder);
    ui->restoreDefaultLayout();
    ui->appendInformation(QStringLiteral("B 独立演示：Control / Information 已就绪；VTK 为窗口占位控件。"));
    host.statusBar()->showMessage(QStringLiteral("B 部分 · Ribbon + 普通 Dock + 窗口控制"));
    host.show();
    return app.exec();
}
