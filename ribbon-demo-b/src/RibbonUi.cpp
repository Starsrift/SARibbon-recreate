#include "RibbonUi.h"

#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonWidget.h"

#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

/**
 * \if ENGLISH
 * @brief Initialize static-library resources outside any namespace.
 * \endif
 * \if CHINESE
 * @brief 在命名空间外初始化静态库资源。
 * \endif
 */
static void initializeRibbonResources()
{
#ifdef SA_RIBBON_BAR_NO_EXPORT
    Q_INIT_RESOURCE(SARibbonResource);
#endif
}

namespace RibbonDemo {

class RibbonUi::PrivateData
{
public:
    QMainWindow* host = nullptr;
    SARibbonBar* ribbon = nullptr;
    QDockWidget* control = nullptr;
    QDockWidget* information = nullptr;
    QPlainTextEdit* log = nullptr;
    QPointer<QDockWidget> vtk;
    QAction* openVtk = nullptr;
    QAction* closeVtk = nullptr;
    QAction* closeAll = nullptr;
    QMetaObject::Connection vtkVisibility;
    QMetaObject::Connection vtkDestroyed;

    /// Create an action with a stable identifier and native icon.
    QAction* action(QObject* owner, const char* id, const QString& text, QStyle::StandardPixmap icon)
    {
        auto* result = new QAction(host->style()->standardIcon(icon), text, owner);
        result->setObjectName(QString::fromLatin1(id));
        result->setToolTip(text);
        return result;
    }

    /// Show and activate an existing dock, including an inactive tab.
    static void openDock(QDockWidget* dock)
    {
        if (!dock)
            return;
        dock->show();
        dock->raise();
        if (dock->isFloating())
            dock->activateWindow();
    }
};

/**
 * \if ENGLISH
 * @brief Install once without replacing an existing unrelated menu widget.
 * \endif
 * \if CHINESE
 * @brief 幂等安装，避免覆盖已有且不属于本组件的菜单控件。
 * \endif
 */
RibbonUi* RibbonUi::install(QMainWindow* host)
{
    if (!host)
        return nullptr;
    if (auto* existing = host->findChild<RibbonUi*>(QStringLiteral("ribbonDemoBUi"), Qt::FindDirectChildrenOnly))
        return existing;
    if (host->menuWidget()) {
        qWarning("RibbonUi requires a host without an existing menu widget");
        return nullptr;
    }
    return new RibbonUi(host);
}

/**
 * \if ENGLISH
 * @brief Build File/View/Window pages and reusable ordinary docks.
 * \endif
 * \if CHINESE
 * @brief 创建 File/View/Window 页面及可反复打开的普通 Dock。
 * \endif
 */
RibbonUi::RibbonUi(QMainWindow* host) : QObject(host), d(new PrivateData)
{
    setObjectName(QStringLiteral("ribbonDemoBUi"));
    d->host = host;
    initializeRibbonResources();
    auto* ribbonWidget = new SARibbonWidget(host);
    ribbonWidget->setObjectName(QStringLiteral("ribbonDemoBMenu"));
    d->ribbon = ribbonWidget->ribbonBar();
    d->ribbon->setRibbonStyle(SARibbonBar::RibbonStyleCompactThreeRow);
    d->ribbon->setApplicationButton(nullptr);
    host->setMenuWidget(ribbonWidget);
    host->setDockNestingEnabled(true);
    host->setDockOptions(host->dockOptions() | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    auto makeDock = [host](const QString& title, const char* name) {
        auto* dock = new QDockWidget(title, host);
        dock->setObjectName(QString::fromLatin1(name));
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
                          | QDockWidget::DockWidgetFloatable);
        dock->setMinimumWidth(220);
        return dock;
    };
    d->control = makeDock(tr("Control"), "controlDock");
    d->information = makeDock(tr("Information"), "informationDock");
    d->log = new QPlainTextEdit(d->information);
    d->log->setObjectName(QStringLiteral("informationLog"));
    d->log->setReadOnly(true);
    d->log->setMaximumBlockCount(300);
    d->information->setWidget(d->log);

    auto* file = d->ribbon->addCategoryPage(tr("File"));
    file->setObjectName(QStringLiteral("fileCategory"));
    auto* view = d->ribbon->addCategoryPage(tr("View"));
    view->setObjectName(QStringLiteral("viewCategory"));
    auto* window = d->ribbon->addCategoryPage(tr("Window"));
    window->setObjectName(QStringLiteral("windowCategory"));
    auto* filePanel = file->addPanel(tr("Application"));
    auto* exit = d->action(this, "exitApplication", tr("退出"), QStyle::SP_DialogCloseButton);
    filePanel->addLargeAction(exit);
    connect(exit, &QAction::triggered, host, &QWidget::close);

    auto* visibility = view->addPanel(tr("Dock 可见性"));
    for (auto* dock : {d->control, d->information}) {
        auto* toggle = dock->toggleViewAction();
        toggle->setObjectName(dock->objectName() + QStringLiteral("Toggle"));
        toggle->setIcon(host->style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        visibility->addSmallAction(toggle);
        connect(dock, &QDockWidget::visibilityChanged, this, [this, dock](bool visible) {
            appendInformation(tr("%1：%2").arg(dock->windowTitle(), visible ? tr("显示") : tr("隐藏或位于其他标签后")));
        });
    }
    auto* layout = view->addPanel(tr("布局"));
    auto* restore = d->action(this, "restoreDefaultLayout", tr("恢复默认布局"), QStyle::SP_BrowserReload);
    layout->addLargeAction(restore);
    connect(restore, &QAction::triggered, this, &RibbonUi::restoreDefaultLayout);

    auto addWindowPanel = [this, window](QDockWidget* dock) {
        auto* panel = window->addPanel(dock->windowTitle());
        const QByteArray id = dock->objectName().toLatin1();
        auto* open = d->action(this, (id + "Open").constData(), tr("打开 %1").arg(dock->windowTitle()),
                               QStyle::SP_DirOpenIcon);
        auto* close = d->action(this, (id + "Close").constData(), tr("关闭 %1").arg(dock->windowTitle()),
                                QStyle::SP_DialogCloseButton);
        panel->addLargeAction(open);
        panel->addSmallAction(close);
        connect(open, &QAction::triggered, this, [dock] { PrivateData::openDock(dock); });
        connect(close, &QAction::triggered, dock, &QWidget::close);
    };
    auto* vtkPanel = window->addPanel(tr("VTK View"));
    d->openVtk = d->action(this, "vtkViewDockOpen", tr("打开 VTK View"), QStyle::SP_DirOpenIcon);
    d->closeVtk = d->action(this, "vtkViewDockClose", tr("关闭 VTK View"), QStyle::SP_DialogCloseButton);
    vtkPanel->addLargeAction(d->openVtk);
    vtkPanel->addSmallAction(d->closeVtk);
    connect(d->openVtk, &QAction::triggered, this, [this] { PrivateData::openDock(d->vtk.data()); });
    connect(d->closeVtk, &QAction::triggered, this, [this] {
        if (d->vtk)
            d->vtk->close();
    });
    addWindowPanel(d->control);
    addWindowPanel(d->information);
    auto* all = window->addPanel(tr("全部窗口"));
    d->closeAll = d->action(this, "closeAllDocks", tr("关闭全部 Dock"), QStyle::SP_DialogCloseButton);
    all->addLargeAction(d->closeAll);
    connect(d->closeAll, &QAction::triggered, this, [this] {
        d->control->close();
        d->information->close();
        if (d->vtk)
            d->vtk->close();
    });

    auto* controls = new QWidget(d->control);
    auto* controlsLayout = new QVBoxLayout(controls);
    auto* hint = new QLabel(tr("窗口控制\n拖动标题栏可停靠或浮动。"), controls);
    hint->setWordWrap(true);
    controlsLayout->addWidget(hint);
    auto* showInfo = d->action(this, "showInformation", tr("显示 Information"), QStyle::SP_MessageBoxInformation);
    connect(showInfo, &QAction::triggered, this, [this] { PrivateData::openDock(d->information); });
    auto* clearInfo = d->action(this, "clearInformation", tr("清空信息"), QStyle::SP_DialogResetButton);
    connect(clearInfo, &QAction::triggered, d->log, &QPlainTextEdit::clear);
    for (auto* action : {d->openVtk, showInfo, restore, clearInfo}) {
        auto* button = new QToolButton(controls);
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        controlsLayout->addWidget(button);
    }
    controlsLayout->addStretch();
    d->control->setWidget(controls);
    bindVtkDock(nullptr);
    restoreDefaultLayout();
    d->ribbon->setCurrentIndex(2);
}

/**
 * \if ENGLISH
 * @brief Destroy the private implementation after disconnecting VTK callbacks.
 * \endif
 * \if CHINESE
 * @brief 断开 VTK 回调后销毁私有实现。
 * \endif
 */
RibbonUi::~RibbonUi()
{
    disconnect(d->vtkVisibility);
    disconnect(d->vtkDestroyed);
}

/**
 * \if ENGLISH
 * @brief Bind the host-owned dock; null and destroyed docks disable VTK actions.
 * @note Rebinding never deletes the previous dock or transfers widget ownership.
 * \endif
 * \if CHINESE
 * @brief 绑定主窗口拥有的 Dock；空绑定或对象销毁后禁用 VTK 按钮。
 * @note 重绑不会删除旧 Dock 或转移其控件所有权。
 * \endif
 */
void RibbonUi::bindVtkDock(QDockWidget* dock)
{
    if (dock && (dock == d->control || dock == d->information || dock->parentWidget() != d->host)) {
        qWarning("VTK dock must be a distinct dock owned by the same host");
        return;
    }
    disconnect(d->vtkVisibility);
    disconnect(d->vtkDestroyed);
    d->vtk = dock;
    d->openVtk->setEnabled(dock != nullptr);
    d->closeVtk->setEnabled(dock != nullptr);
    d->openVtk->setToolTip(dock ? tr("显示已有的 VTK Dock") : tr("尚未绑定 VTK Dock"));
    if (!dock) {
        appendInformation(tr("尚未绑定 VTK Dock。"));
        return;
    }
    d->vtkVisibility = connect(dock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        appendInformation(visible ? tr("VTK View：显示") : tr("VTK View：隐藏或位于其他标签后"));
    });
    d->vtkDestroyed = connect(dock, &QObject::destroyed, this, [this] { bindVtkDock(nullptr); });
    appendInformation(tr("已绑定 VTK Dock 的窗口控制。"));
}

/**
 * \if ENGLISH
 * @brief Place Control right, Information bottom and an optional VTK dock left.
 * \endif
 * \if CHINESE
 * @brief 将 Control 放右侧、Information 放底部，已绑定的 VTK Dock 放左侧。
 * \endif
 */
void RibbonUi::restoreDefaultLayout()
{
    const auto place = [this](QDockWidget* dock, Qt::DockWidgetArea area) {
        if (!dock)
            return;
        dock->setFloating(false);
        d->host->removeDockWidget(dock);
        d->host->addDockWidget(area, dock);
        dock->show();
    };
    place(d->vtk.data(), Qt::LeftDockWidgetArea);
    place(d->control, Qt::RightDockWidgetArea);
    place(d->information, Qt::BottomDockWidgetArea);
    // Defer sizing until the host layout has processed the dock placement.
    QTimer::singleShot(0, this, [this] {
        if (d->vtk)
            d->host->resizeDocks({d->vtk.data(), d->control}, {740, 260}, Qt::Horizontal);
        else
            d->host->resizeDocks({d->control}, {260}, Qt::Horizontal);
        d->host->resizeDocks({d->information}, {150}, Qt::Vertical);
    });
    appendInformation(tr("已恢复默认布局。"));
}

/**
 * \if ENGLISH
 * @brief Append host-supplied plain text without interpreting HTML.
 * \endif
 * \if CHINESE
 * @brief 追加主窗口提供的纯文本信息，不解释 HTML。
 * \endif
 */
void RibbonUi::appendInformation(const QString& message)
{
    d->log->appendPlainText(message);
}

/**
 * \if ENGLISH
 * @brief Return the installed Ribbon bar.
 * \endif
 * \if CHINESE
 * @brief 返回已安装的 Ribbon。
 * \endif
 */
SARibbonBar* RibbonUi::ribbonBar() const { return d->ribbon; }

/**
 * \if ENGLISH
 * @brief Return the Control dock.
 * \endif
 * \if CHINESE
 * @brief 返回 Control Dock。
 * \endif
 */
QDockWidget* RibbonUi::controlDock() const { return d->control; }

/**
 * \if ENGLISH
 * @brief Return the Information dock.
 * \endif
 * \if CHINESE
 * @brief 返回 Information Dock。
 * \endif
 */
QDockWidget* RibbonUi::informationDock() const { return d->information; }

} // namespace RibbonDemo
