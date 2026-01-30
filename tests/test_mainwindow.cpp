#include <QtTest>
#include <QTableView>
#include <QMenuBar>
#include <QMenu>

#include "../mainwindow.h"
#include "../mymodel.h"
#include "../mydelegate.h"

class TestMainWindow : public QObject {
    Q_OBJECT
private slots:
    void ctor_setsModelAndDelegateAndMenu() {
        MainWindow w; // НЕ вызываем слоты open/save

        auto* view = w.findChild<QTableView*>("tableView");
        QVERIFY2(view != nullptr, "tableView not found (check objectName in .ui)");

        QVERIFY(view->model() != nullptr);
        QVERIFY(qobject_cast<MyModel*>(view->model()) != nullptr);

        QVERIFY(view->itemDelegate() != nullptr);
        QVERIFY(qobject_cast<MyDelegate*>(view->itemDelegate()) != nullptr);

        // Проверка меню "Файл" и действий
        QMenuBar* mb = w.menuBar();
        QVERIFY(mb != nullptr);

        QMenu* fileMenu = nullptr;
        for (QAction* a : mb->actions()) {
            if (a->text() == QStringLiteral("Файл")) {
                fileMenu = a->menu();
                break;
            }
        }
        QVERIFY2(fileMenu != nullptr, "Menu 'Файл' not found");

        bool hasOpen = false, hasSave = false;
        for (QAction* a : fileMenu->actions()) {
            if (a->text() == QStringLiteral("Открыть...")) hasOpen = true;
            if (a->text() == QStringLiteral("Сохранить...")) hasSave = true;
        }
        QVERIFY(hasOpen);
        QVERIFY(hasSave);
    }
};

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"
