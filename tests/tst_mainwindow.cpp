// tests/tst_mainwindow.cpp
#include <QtTest/QtTest>
#include <QMenuBar>
#include "mainwindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT
private slots:
    void constructs_and_has_menu();
};

void TestMainWindow::constructs_and_has_menu()
{
    MainWindow w;
    QVERIFY(w.menuBar() != nullptr);
    // Проверим, что меню есть и не пустое
    QVERIFY(!w.menuBar()->actions().isEmpty());
}

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
