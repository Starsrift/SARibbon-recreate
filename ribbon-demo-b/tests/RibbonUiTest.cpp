#include "RibbonUi.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"

#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTest>
#include <QToolButton>

using RibbonDemo::RibbonUi;

class RibbonUiTest : public QObject
{
    Q_OBJECT
private:
    static QAction* action(RibbonUi* ui, const char* id)
    {
        return ui->findChild<QAction*>(QString::fromLatin1(id));
    }

private Q_SLOTS:
    void installPreservesHostAndIsIdempotent()
    {
        QMainWindow host;
        auto* center = new QLabel(QStringLiteral("A owns the central widget"), &host);
        host.setCentralWidget(center);
        auto* ui = RibbonUi::install(&host);
        QVERIFY(ui);
        QCOMPARE(RibbonUi::install(&host), ui);
        QCOMPARE(host.centralWidget(), center);
        QCOMPARE(host.findChildren<QDockWidget*>().size(), 2);
        QCOMPARE(ui->ribbonBar()->categoryPages().size(), 3);
        QCOMPARE(ui->ribbonBar()->categoryPages().at(0)->categoryName(), QStringLiteral("File"));
        QCOMPARE(ui->ribbonBar()->categoryPages().at(1)->categoryName(), QStringLiteral("View"));
        QCOMPARE(ui->ribbonBar()->categoryPages().at(2)->categoryName(), QStringLiteral("Window"));
        QVERIFY(!action(ui, "vtkViewDockOpen")->isEnabled());
        QVERIFY(!action(ui, "vtkViewDockClose")->isEnabled());
        QVERIFY(!RibbonUi::install(nullptr));
    }

    void existingMenuIsPreserved()
    {
        QMainWindow host;
        auto* original = new QLabel(QStringLiteral("Existing menu"), &host);
        host.setMenuWidget(original);
        QTest::ignoreMessage(QtWarningMsg, "RibbonUi requires a host without an existing menu widget");
        QVERIFY(!RibbonUi::install(&host));
        QCOMPARE(host.menuWidget(), original);
        QVERIFY(host.findChildren<QDockWidget*>().isEmpty());
    }

    void closeReopenAndToggleStayConsistent()
    {
        QMainWindow host;
        host.resize(1100, 750);
        auto* ui = RibbonUi::install(&host);
        host.show();
        auto* control = ui->controlDock();
        QTRY_VERIFY(control->isVisible());
        action(ui, "controlDockClose")->trigger();
        QTRY_VERIFY(control->isHidden());
        QVERIFY(!control->toggleViewAction()->isChecked());
        for (int i = 0; i < 3; ++i) {
            action(ui, "controlDockOpen")->trigger();
            QTRY_VERIFY(control->isVisible());
            QVERIFY(control->toggleViewAction()->isChecked());
            control->close(); // Same close path as the title-bar close button.
        }
        control->toggleViewAction()->trigger();
        QTRY_VERIFY(control->isVisible());
        QCOMPARE(host.findChildren<QDockWidget*>().size(), 2);
        QCOMPARE(ui->controlDock(), control);
        action(ui, "closeAllDocks")->trigger();
        QTRY_VERIFY(control->isHidden());
        QVERIFY(ui->informationDock()->isHidden());
        action(ui, "restoreDefaultLayout")->trigger();
        QTRY_VERIFY(control->isVisible());
        QVERIFY(ui->informationDock()->isVisible());
    }

    void floatTabifyAndRestore()
    {
        QMainWindow host;
        host.resize(1100, 750);
        auto* ui = RibbonUi::install(&host);
        host.show();
        auto* control = ui->controlDock();
        QVERIFY(control->features().testFlag(QDockWidget::DockWidgetMovable));
        QVERIFY(control->features().testFlag(QDockWidget::DockWidgetFloatable));
        control->setFloating(true);
        QTRY_VERIFY(control->isFloating());
        control->close();
        action(ui, "controlDockOpen")->trigger();
        QTRY_VERIFY(control->isVisible());
        QVERIFY(control->isFloating());
        ui->restoreDefaultLayout();
        QTRY_VERIFY(!control->isFloating());
        QCOMPARE(host.dockWidgetArea(control), Qt::RightDockWidgetArea);
        QCOMPARE(host.dockWidgetArea(ui->informationDock()), Qt::BottomDockWidgetArea);
        host.tabifyDockWidget(control, ui->informationDock());
        QVERIFY(host.tabifiedDockWidgets(control).contains(ui->informationDock()));
        action(ui, "controlDockOpen")->trigger();
        ui->restoreDefaultLayout();
        QVERIFY(host.tabifiedDockWidgets(control).isEmpty());
        QCOMPARE(host.dockWidgetArea(ui->informationDock()), Qt::BottomDockWidgetArea);
    }

    void vtkBindingRebindingAndDestruction()
    {
        QMainWindow host;
        host.resize(1100, 750);
        auto* ui = RibbonUi::install(&host);
        auto* first = new QDockWidget(QStringLiteral("A's VTK dock"), &host);
        auto* payload = new QLabel(QStringLiteral("Owned by A"), first);
        first->setWidget(payload);
        host.addDockWidget(Qt::LeftDockWidgetArea, first);
        ui->bindVtkDock(first);
        host.show();
        QVERIFY(action(ui, "vtkViewDockOpen")->isEnabled());
        action(ui, "vtkViewDockClose")->trigger();
        QTRY_VERIFY(first->isHidden());
        action(ui, "vtkViewDockOpen")->trigger();
        QTRY_VERIFY(first->isVisible());
        QCOMPARE(first->widget(), payload);
        QCOMPARE(first->parentWidget(), &host);
        auto* second = new QDockWidget(QStringLiteral("Replacement"), &host);
        host.addDockWidget(Qt::LeftDockWidgetArea, second);
        ui->bindVtkDock(second);
        delete first; // Old destruction connection must not disable the new binding.
        QVERIFY(action(ui, "vtkViewDockOpen")->isEnabled());
        action(ui, "closeAllDocks")->trigger();
        QTRY_VERIFY(second->isHidden());
        ui->restoreDefaultLayout();
        QTRY_VERIFY(second->isVisible());
        second->setAttribute(Qt::WA_DeleteOnClose);
        action(ui, "vtkViewDockClose")->trigger();
        QTRY_VERIFY(!action(ui, "vtkViewDockOpen")->isEnabled());
        QVERIFY(!action(ui, "vtkViewDockClose")->isEnabled());
        ui->restoreDefaultLayout(); // Null binding must remain safe.
        QVERIFY(ui->controlDock()->isVisible());
    }

    void explicitUnbindAndInvalidBinding()
    {
        QMainWindow host;
        QMainWindow other;
        auto* ui = RibbonUi::install(&host);
        auto* dock = new QDockWidget(&host);
        host.addDockWidget(Qt::LeftDockWidgetArea, dock);
        ui->bindVtkDock(dock);
        auto* wrongHostDock = new QDockWidget(&other);
        QTest::ignoreMessage(QtWarningMsg, "VTK dock must be a distinct dock owned by the same host");
        ui->bindVtkDock(wrongHostDock);
        QVERIFY(action(ui, "vtkViewDockOpen")->isEnabled());
        QTest::ignoreMessage(QtWarningMsg, "VTK dock must be a distinct dock owned by the same host");
        ui->bindVtkDock(ui->controlDock());
        ui->bindVtkDock(nullptr);
        QVERIFY(!action(ui, "vtkViewDockOpen")->isEnabled());
        QVERIFY(dock->parentWidget() == &host);
    }

    void realToolButtonsDriveDocksAndInformation()
    {
        QMainWindow host;
        host.resize(1100, 750);
        auto* ui = RibbonUi::install(&host);
        host.show();
        QTRY_VERIFY(ui->controlDock()->isVisible());
        QToolButton* closeButton = nullptr;
        for (auto* button : host.findChildren<QToolButton*>()) {
            if (button->defaultAction() == action(ui, "controlDockClose"))
                closeButton = button;
        }
        QVERIFY(closeButton);
        QTRY_VERIFY(closeButton->isVisible());
        QTest::mouseClick(closeButton, Qt::LeftButton);
        QTRY_VERIFY(ui->controlDock()->isHidden());
        action(ui, "controlDockOpen")->trigger();
        QTRY_VERIFY(ui->controlDock()->isVisible());
        auto* log = ui->informationDock()->findChild<QPlainTextEdit*>();
        QVERIFY(log && log->isReadOnly());
        ui->appendInformation(QStringLiteral("<b>Plain text</b>"));
        QVERIFY(log->toPlainText().contains(QStringLiteral("<b>Plain text</b>")));
        QToolButton* clearButton = nullptr;
        for (auto* button : ui->controlDock()->findChildren<QToolButton*>()) {
            if (button->defaultAction() == action(ui, "clearInformation"))
                clearButton = button;
        }
        QVERIFY(clearButton);
        QTest::mouseClick(clearButton, Qt::LeftButton);
        QVERIFY(log->toPlainText().isEmpty());
        for (int i = 0; i < 350; ++i)
            ui->appendInformation(QString::number(i));
        QVERIFY(log->blockCount() <= 300);
        ui->informationDock()->close();
        action(ui, "showInformation")->trigger();
        QTRY_VERIFY(ui->informationDock()->isVisible());
        action(ui, "exitApplication")->trigger();
        QTRY_VERIFY(host.isHidden());
    }
};

QTEST_MAIN(RibbonUiTest)
#include "RibbonUiTest.moc"
