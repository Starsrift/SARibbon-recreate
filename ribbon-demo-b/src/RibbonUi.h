#pragma once

#include <QObject>
#include <memory>

class QDockWidget;
class QMainWindow;
class SARibbonBar;

namespace RibbonDemo {

/**
 * \if ENGLISH
 * @brief Installs part B's Ribbon and ordinary docks in an existing main window.
 * @note The host owns the UI; VTK rendering remains the responsibility of part A.
 * \endif
 * \if CHINESE
 * @brief 在已有主窗口中安装 B 部分的 Ribbon 和普通 Dock。
 * @note 由主窗口拥有组件；VTK 渲染仍归 A 部分负责。
 * \endif
 */
class RibbonUi final : public QObject
{
    Q_OBJECT
public:
    /// Install once per host, returning the existing instance on repeated calls.
    static RibbonUi* install(QMainWindow* host);
    /// Release private state; widget lifetime belongs to the host.
    ~RibbonUi() override;
    /// Bind or unbind the VTK dock without taking ownership or changing its content.
    void bindVtkDock(QDockWidget* dock);
    /// Return the Ribbon bar for optional host customization.
    SARibbonBar* ribbonBar() const;
    /// Return the Control dock owned by the host.
    QDockWidget* controlDock() const;
    /// Return the Information dock owned by the host.
    QDockWidget* informationDock() const;

public Q_SLOTS:
    /// Restore B docks and the bound VTK dock to the initial arrangement.
    void restoreDefaultLayout();
    /// Append plain text to the bounded information log.
    void appendInformation(const QString& message);

private:
    explicit RibbonUi(QMainWindow* host);
    class PrivateData;
    std::unique_ptr<PrivateData> d;
};

} // namespace RibbonDemo
