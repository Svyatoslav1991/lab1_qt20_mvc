#include <QtTest>
#include <QComboBox>
#include "../mydelegate.h"
#include "../mymodel.h"

class TestMyDelegate : public QObject {
    Q_OBJECT
private slots:
    void penStyle_createEditor_isComboBox() {
        MyModel model;
        QVERIFY(model.insertRows(0, 1));

        MyDelegate delegate;
        QWidget parent;
        const QModelIndex idx = model.index(0, 1); // PenStyle column

        QWidget* ed = delegate.createEditor(&parent, QStyleOptionViewItem{}, idx);
        QVERIFY(ed != nullptr);
        QVERIFY(qobject_cast<QComboBox*>(ed) != nullptr);
        delete ed;
    }

    void penStyle_setEditorData_selectsCorrectItem() {
        MyModel model;
        QVERIFY(model.insertRows(0, 1));
        const QModelIndex idx = model.index(0, 1); // PenStyle

        QVERIFY(model.setData(idx, static_cast<int>(Qt::DashDotLine), Qt::EditRole));

        MyDelegate delegate;
        QWidget parent;
        auto* combo = new QComboBox(&parent);

        // Имитируем создание combo как в делегате
        // (createEditor делает fillPenStyleCombo, но он private; проще вызвать createEditor)
        delete combo;
        QWidget* ed = delegate.createEditor(&parent, QStyleOptionViewItem{}, idx);
        combo = qobject_cast<QComboBox*>(ed);
        QVERIFY(combo);

        delegate.setEditorData(combo, idx);
        QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::DashDotLine));

        delete ed;
    }

    void penStyle_setModelData_writesToModel() {
        MyModel model;
        QVERIFY(model.insertRows(0, 1));
        const QModelIndex idx = model.index(0, 1); // PenStyle

        MyDelegate delegate;
        QWidget parent;
        QWidget* ed = delegate.createEditor(&parent, QStyleOptionViewItem{}, idx);
        auto* combo = qobject_cast<QComboBox*>(ed);
        QVERIFY(combo);

        // Выбираем Qt::DotLine из списка
        const int pos = combo->findData(static_cast<int>(Qt::DotLine));
        QVERIFY(pos >= 0);
        combo->setCurrentIndex(pos);

        delegate.setModelData(combo, &model, idx);
        QCOMPARE(model.data(idx, Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));

        delete ed;
    }
};

QTEST_MAIN(TestMyDelegate)
#include "test_mydelegate.moc"
