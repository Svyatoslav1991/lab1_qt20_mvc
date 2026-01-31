// tests/tst_mydelegate.cpp
#include <QtTest/QtTest>

#include <QComboBox>
#include <QColor>
#include <QColorDialog>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTimer>
#include <QWidget>

#include "mydelegate.h"

namespace {

// Для GUI-тестов с модальными диалогами (QColorDialog::getColor).
// На headless (offscreen/minimal) такие тесты могут быть нестабильны или зависать.
bool isHeadlessLikePlatform()
{
    const QString p = QGuiApplication::platformName().toLower();
    return (p.contains("offscreen") || p.contains("minimal"));
}

// Утилита: проверяем, что QVariant действительно содержит QColor и сравниваем.
void compareModelColor(const QAbstractItemModel& model, const QModelIndex& idx, const QColor& expected)
{
    const QVariant v = model.data(idx, Qt::EditRole);
    QVERIFY2(v.isValid(), "Model EditRole should be valid");
    QVERIFY2(v.canConvert<QColor>(), "Model EditRole should be convertible to QColor");
    QCOMPARE(v.value<QColor>(), expected);
}

} // namespace

class TestMyDelegate : public QObject
{
    Q_OBJECT
private slots:
    // createEditor
    void createEditor_for_penStyle_is_combo();
    void createEditor_for_other_column_is_not_combo();
    void createEditor_for_invalid_index_does_not_return_combo();

    // setEditorData
    void setEditorData_sets_combo_to_model_value();
    void setEditorData_unknown_style_sets_index0();

    // setModelData
    void setModelData_writes_combo_value_to_model();

    // editorEvent (без проверки return базового класса — только побочные эффекты)
    void editorEvent_other_column_does_not_change_model();
    void editorEvent_non_dblclick_does_not_change_model();
    void editorEvent_dblclick_not_left_button_does_not_change_model();

    // editorEvent + QColorDialog (интеграционные GUI-тесты без рефактора)
    void editorEvent_penColor_cancel_does_not_change_model_and_returns_true();
    void editorEvent_penColor_accept_sets_color_and_returns_true();
};

// -------------------- createEditor --------------------

void TestMyDelegate::createEditor_for_penStyle_is_combo()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle column (1)

    QStyleOptionViewItem opt;
    QWidget parent;

    QWidget* ed = d.createEditor(&parent, opt, idx);
    QVERIFY(ed != nullptr);
    QVERIFY2(qobject_cast<QComboBox*>(ed) != nullptr, "Editor for PenStyle must be QComboBox");
}

void TestMyDelegate::createEditor_for_other_column_is_not_combo()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 0); // PenColor column (0) -> not PenStyle

    QStyleOptionViewItem opt;
    QWidget parent;

    QWidget* ed = d.createEditor(&parent, opt, idx);
    // Базовый делегат может вернуть nullptr или какой-то редактор — это ок.
    if (ed)
        QVERIFY2(qobject_cast<QComboBox*>(ed) == nullptr, "Editor for non-PenStyle must NOT be QComboBox");
}

void TestMyDelegate::createEditor_for_invalid_index_does_not_return_combo()
{
    MyDelegate d;

    QStyleOptionViewItem opt;
    QWidget parent;

    const QModelIndex invalid;
    QWidget* ed = d.createEditor(&parent, opt, invalid);

    // Главное: не падать и не возвращать QComboBox.
    if (ed)
        QVERIFY2(qobject_cast<QComboBox*>(ed) == nullptr, "Invalid index must NOT produce QComboBox");
}

// -------------------- setEditorData --------------------

void TestMyDelegate::setEditorData_sets_combo_to_model_value()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    model.setData(idx, static_cast<int>(Qt::DotLine), Qt::EditRole);

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    d.setEditorData(combo, idx);

    QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::DotLine));
}

void TestMyDelegate::setEditorData_unknown_style_sets_index0()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    // Стиль, которого нет в combo -> findData == -1 -> setCurrentIndex(0)
    model.setData(idx, 999999, Qt::EditRole);

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    d.setEditorData(combo, idx);

    QCOMPARE(combo->currentIndex(), 0);
}

// -------------------- setModelData --------------------

void TestMyDelegate::setModelData_writes_combo_value_to_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    const int pos = combo->findData(static_cast<int>(Qt::DashLine));
    QVERIFY(pos >= 0);
    combo->setCurrentIndex(pos);

    d.setModelData(combo, &model, idx);

    QCOMPARE(model.data(idx, Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
}

// -------------------- editorEvent: ранние выходы (без GUI) --------------------

void TestMyDelegate::editorEvent_other_column_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penStyleIdx = model.index(0, 1); // НЕ PenColor

    // Инициализация: в PenStyle лежит DotLine
    model.setData(penStyleIdx, static_cast<int>(Qt::DotLine), Qt::EditRole);
    const int before = model.data(penStyleIdx, Qt::EditRole).toInt();

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Вызов (должен уйти в базовый обработчик, но главное — без побочных эффектов)
    d.editorEvent(&dbl, &model, opt, penStyleIdx);

    const int after = model.data(penStyleIdx, Qt::EditRole).toInt();
    QCOMPARE(after, before);
}

void TestMyDelegate::editorEvent_non_dblclick_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor

    model.setData(penColorIdx, QColor(Qt::red), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::red));

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    d.editorEvent(&press, &model, opt, penColorIdx);

    // Цвет не должен меняться
    compareModelColor(model, penColorIdx, QColor(Qt::red));
}

void TestMyDelegate::editorEvent_dblclick_not_left_button_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor

    model.setData(penColorIdx, QColor(Qt::green), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::green));

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    // DblClick, но правая кнопка
    QMouseEvent dblRight(QEvent::MouseButtonDblClick, QPointF(1, 1),
                         Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    d.editorEvent(&dblRight, &model, opt, penColorIdx);

    // Цвет не должен меняться
    compareModelColor(model, penColorIdx, QColor(Qt::green));
}

// -------------------- editorEvent + QColorDialog (GUI) --------------------

void TestMyDelegate::editorEvent_penColor_cancel_does_not_change_model_and_returns_true()
{
    if (isHeadlessLikePlatform())
        QSKIP("Skipping QColorDialog GUI test on headless/minimal platform.");

    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor
    model.setData(penColorIdx, QColor(Qt::blue), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::blue));

    QTableView view;
    view.setModel(&model);
    view.show(); // помогает корректно создать родительскую иерархию для диалога

    QStyleOptionViewItem opt;
    opt.widget = &view;

    // Таймер: как только появится модальный QColorDialog — закрываем Cancel (reject)
    QTimer::singleShot(0, []() {
        QWidget* w = QApplication::activeModalWidget();
        if (auto* dlg = qobject_cast<QColorDialog*>(w))
            dlg->reject();
    });

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    const bool handled = d.editorEvent(&dbl, &model, opt, penColorIdx);

    // При отмене диалога твой код возвращает true и не меняет модель
    QVERIFY(handled);
    compareModelColor(model, penColorIdx, QColor(Qt::blue));
}

void TestMyDelegate::editorEvent_penColor_accept_sets_color_and_returns_true()
{
    if (isHeadlessLikePlatform())
        QSKIP("Skipping QColorDialog GUI test on headless/minimal platform.");

    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor
    model.setData(penColorIdx, QColor(Qt::black), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::black));

    QTableView view;
    view.setModel(&model);
    view.show();

    QStyleOptionViewItem opt;
    opt.widget = &view;

    const QColor chosen = QColor(Qt::magenta);

    // Таймер: выставляем цвет и жмём OK (accept)
    QTimer::singleShot(0, [chosen]() {
        QWidget* w = QApplication::activeModalWidget();
        if (auto* dlg = qobject_cast<QColorDialog*>(w))
        {
            dlg->setCurrentColor(chosen);
            dlg->accept();
        }
    });

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    const bool handled = d.editorEvent(&dbl, &model, opt, penColorIdx);

    QVERIFY(handled);
    compareModelColor(model, penColorIdx, chosen);
}

QTEST_MAIN(TestMyDelegate)
#include "tst_mydelegate.moc"
