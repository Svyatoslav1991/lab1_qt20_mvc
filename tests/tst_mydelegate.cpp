// tests/tst_mydelegate.cpp
#include <QtTest/QtTest>
#include <QComboBox>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QEvent>
#include <QMouseEvent>
#include "mydelegate.h"

class TestMyDelegate : public QObject
{
    Q_OBJECT
private slots:
    void createEditor_for_penStyle_is_combo();
    void setEditorData_sets_combo_to_model_value();
    void setModelData_writes_combo_value_to_model();
    void editorEvent_ignores_non_dblclick_or_other_column();
};

void TestMyDelegate::createEditor_for_penStyle_is_combo()
{
    MyDelegate d;
    QStandardItemModel model(1, 2);
    QModelIndex idx = model.index(0, 1); // PenStyle column by your convention

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    QVERIFY(ed != nullptr);
    QVERIFY(qobject_cast<QComboBox*>(ed) != nullptr);
}

void TestMyDelegate::setEditorData_sets_combo_to_model_value()
{
    MyDelegate d;
    QStandardItemModel model(1, 2);

    // В модель кладём int(Qt::PenStyle) как EditRole
    model.setData(model.index(0, 1), static_cast<int>(Qt::DotLine), Qt::EditRole);

    QStyleOptionViewItem opt;
    QWidget parent;
    auto* combo = qobject_cast<QComboBox*>(d.createEditor(&parent, opt, model.index(0, 1)));
    QVERIFY(combo);

    d.setEditorData(combo, model.index(0, 1));

    QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::DotLine));
}

void TestMyDelegate::setModelData_writes_combo_value_to_model()
{
    MyDelegate d;
    QStandardItemModel model(1, 2);

    QStyleOptionViewItem opt;
    QWidget parent;
    auto* combo = qobject_cast<QComboBox*>(d.createEditor(&parent, opt, model.index(0, 1)));
    QVERIFY(combo);

    // Выбираем, например, DashLine (ищем по userData)
    const int pos = combo->findData(static_cast<int>(Qt::DashLine));
    QVERIFY(pos >= 0);
    combo->setCurrentIndex(pos);

    d.setModelData(combo, &model, model.index(0, 1));
    QCOMPARE(model.data(model.index(0, 1), Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
}

void TestMyDelegate::editorEvent_ignores_non_dblclick_or_other_column()
{
    MyDelegate d;
    QStandardItemModel model(1, 2);
    QStyleOptionViewItem opt;

    // Событие НЕ dblclick -> должно уйти в базовую обработку (нам важно, что не падает)
    QMouseEvent click(QEvent::MouseButtonPress, QPointF(1,1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Столбец 1 (PenStyle) — не PenColor, значит диалог цвета открываться не должен
    QVERIFY(!d.editorEvent(&click, &model, opt, model.index(0, 1)));
}

QTEST_MAIN(TestMyDelegate)
#include "tst_mydelegate.moc"
