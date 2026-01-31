// tests/tst_mainwindow.cpp
#include <QtTest/QtTest>

#include <QAction>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QTableView>

#include "mainwindow.h"
#include "mymodel.h"
#include "mydelegate.h"

/**
 * @file tst_mainwindow.cpp
 * @brief Набор тестов для проверки поведения MainWindow.
 *
 * @details
 * Тестируем то, что можно стабильно проверить без рефакторинга MainWindow:
 *
 * 1) Smoke-тест:
 *    - окно создаётся без падений;
 *    - menuBar() существует.
 *
 * 2) Настройка таблицы:
 *    - QTableView найден (ожидаем objectName = "tableView" из .ui);
 *    - модель установлена и это MyModel;
 *    - делегат установлен и это MyDelegate;
 *    - horizontalHeader() настроен на Stretch.
 *
 * 3) Эффекты конструктора:
 *    - вызывается MyModel::test(), поэтому ожидаем 2 строки.
 *
 * 4) Меню "Файл":
 *    - меню "Файл" существует;
 *    - в меню есть действия "Открыть..." и "Сохранить...";
 *    - у действий стоят стандартные шорткаты Open/Save.
 *
 * @note
 * Мы НЕ тестируем тут QFileDialog/QMessageBox (slotSaveToFile/slotLoadFromFile),
 * потому что в коде используются статические методы QFileDialog::get*,
 * которые сложно корректно и стабильно тестировать без инъекции зависимостей.
 */

namespace {

/**
 * @brief Возвращает указатель на QTableView из MainWindow.
 *
 * @details
 * Никаких QtTest-макросов внутри — только поиск. Проверки делаем в тестах,
 * чтобы не получить "return without value" в функциях, возвращающих значение.
 */
QTableView* findTableView(MainWindow& w)
{
    return w.findChild<QTableView*>("tableView");
}

/**
 * @brief Находит QMenu по точному тексту среди QAction в менюбаре.
 */
QMenu* findMenuByTitle(QMenuBar* mb, const QString& title)
{
    if (!mb)
        return nullptr;

    for (QAction* a : mb->actions())
    {
        if (!a)
            continue;
        if (a->text() == title)
            return a->menu();
    }
    return nullptr;
}

/**
 * @brief Находит QAction по точному тексту внутри меню.
 */
QAction* findActionByText(QMenu* menu, const QString& text)
{
    if (!menu)
        return nullptr;

    for (QAction* a : menu->actions())
    {
        if (a && a->text() == text)
            return a;
    }
    return nullptr;
}

} // namespace

class TestMainWindow : public QObject
{
    Q_OBJECT
private slots:
    void constructs_and_has_menubar();
    void tableView_has_model_delegate_and_stretch_header();
    void model_is_filled_by_test_data();
    void file_menu_exists_and_has_expected_actions();
    void file_actions_have_standard_shortcuts();
};

void TestMainWindow::constructs_and_has_menubar()
{
    MainWindow w;
    QVERIFY2(w.menuBar() != nullptr, "MainWindow must have a menuBar()");
}

void TestMainWindow::tableView_has_model_delegate_and_stretch_header()
{
    MainWindow w;

    QTableView* tv = findTableView(w);
    QVERIFY2(tv != nullptr, "MainWindow must have QTableView child named 'tableView'");

    // Модель должна быть установлена и быть MyModel
    QVERIFY2(tv->model() != nullptr, "tableView must have a model");
    QVERIFY2(qobject_cast<MyModel*>(tv->model()) != nullptr, "tableView model must be MyModel");

    // Делегат должен быть установлен и быть MyDelegate
    QVERIFY2(tv->itemDelegate() != nullptr, "tableView must have an itemDelegate");
    QVERIFY2(qobject_cast<const MyDelegate*>(tv->itemDelegate()) != nullptr,
             "tableView itemDelegate must be MyDelegate");

    // Заголовок должен быть и режим Stretch должен быть включён
    QHeaderView* hh = tv->horizontalHeader();
    QVERIFY2(hh != nullptr, "tableView must have horizontalHeader()");
    QCOMPARE(hh->sectionResizeMode(0), QHeaderView::Stretch);
}

void TestMainWindow::model_is_filled_by_test_data()
{
    MainWindow w;

    QTableView* tv = findTableView(w);
    QVERIFY2(tv != nullptr, "MainWindow must have QTableView child named 'tableView'");

    auto* model = qobject_cast<MyModel*>(tv->model());
    QVERIFY2(model != nullptr, "tableView model must be MyModel");

    // По текущей реализации MyModel::test() добавляет ровно 2 строки.
    QCOMPARE(model->rowCount(), 2);

    // Минимальная sanity-проверка данных (PenColor DisplayRole должен быть "#RRGGBB").
    const QModelIndex idxColor0 = model->index(0, 0);
    QVERIFY(idxColor0.isValid());

    const QString colorText = model->data(idxColor0, Qt::DisplayRole).toString();
    QVERIFY2(!colorText.isEmpty(), "PenColor DisplayRole must not be empty");
    QVERIFY2(colorText.startsWith('#'), "PenColor DisplayRole is expected to be '#RRGGBB'");
}

void TestMainWindow::file_menu_exists_and_has_expected_actions()
{
    MainWindow w;

    QMenuBar* mb = w.menuBar();
    QVERIFY2(mb != nullptr, "MainWindow must have a menuBar()");

    QMenu* fileMenu = findMenuByTitle(mb, "Файл");
    QVERIFY2(fileMenu != nullptr, "MenuBar must contain menu titled 'Файл'");

    QAction* actOpen = findActionByText(fileMenu, "Открыть...");
    QAction* actSave = findActionByText(fileMenu, "Сохранить...");

    QVERIFY2(actOpen != nullptr, "File menu must contain action 'Открыть...'");
    QVERIFY2(actSave != nullptr, "File menu must contain action 'Сохранить...'");
}

void TestMainWindow::file_actions_have_standard_shortcuts()
{
    MainWindow w;

    QMenu* fileMenu = findMenuByTitle(w.menuBar(), "Файл");
    QVERIFY2(fileMenu != nullptr, "MenuBar must contain menu titled 'Файл'");

    QAction* actOpen = findActionByText(fileMenu, "Открыть...");
    QAction* actSave = findActionByText(fileMenu, "Сохранить...");

    QVERIFY2(actOpen != nullptr, "File menu must contain action 'Открыть...'");
    QVERIFY2(actSave != nullptr, "File menu must contain action 'Сохранить...'");

    QCOMPARE(actOpen->shortcut(), QKeySequence::Open);
    QCOMPARE(actSave->shortcut(), QKeySequence::Save);
}

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
