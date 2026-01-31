// tests/tst_mymodel.cpp
#include <QtTest/QtTest>
#include <QBuffer>
#include <QAbstractItemModelTester>
#include "mymodel.h"

class TestMyModel : public QObject
{
    Q_OBJECT
private slots:
    void init();                 // вызывается перед каждым тестом
    void basics_row_col();
    void insertRows_appends_defaults();
    void data_roles_for_penColor();
    void setData_emits_dataChanged_and_updates_roles();
    void setData_rejects_invalid_color();
    void tsv_roundtrip_via_buffer();

private:
    MyModel* m = nullptr;
};

void TestMyModel::init()
{
    delete m;
    m = new MyModel();

    // Автоматическая проверка корректности модели (не разрушает данные)
    // и перепроверяет состояние при изменениях. :contentReference[oaicite:5]{index=5}
    new QAbstractItemModelTester(m, QAbstractItemModelTester::FailureReportingMode::QtTest, m);
}

void TestMyModel::basics_row_col()
{
    QCOMPARE(m->rowCount(), 0);
    QCOMPARE(m->columnCount(), 7); // PenColor..Height
}

void TestMyModel::insertRows_appends_defaults()
{
    QVERIFY(m->insertRows(0, 2));
    QCOMPARE(m->rowCount(), 2);

    // Проверим одну ячейку по умолчанию (например PenWidth)
    const QModelIndex idx = m->index(0, 2); // PenWidth
    QCOMPARE(m->data(idx, Qt::EditRole).toInt(), 1);
    QCOMPARE(m->data(idx, Qt::DisplayRole).toInt(), 1);
}

void TestMyModel::data_roles_for_penColor()
{
    m->slotAddData(MyRect(QColor(Qt::green), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, 0); // PenColor

    // EditRole: QColor
    QVERIFY(m->data(idx, Qt::EditRole).canConvert<QColor>());
    QCOMPARE(m->data(idx, Qt::EditRole).value<QColor>(), QColor(Qt::green));

    // DisplayRole: "#RRGGBB"
    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QColor(Qt::green).name());

    // DecorationRole: иконка
    QVERIFY(m->data(idx, Qt::DecorationRole).canConvert<QIcon>());
}

void TestMyModel::setData_emits_dataChanged_and_updates_roles()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, 0); // PenColor

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(idx, QColor(Qt::blue), Qt::EditRole));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(m->data(idx, Qt::EditRole).value<QColor>(), QColor(Qt::blue));
    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QColor(Qt::blue).name());
}

void TestMyModel::setData_rejects_invalid_color()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, 0); // PenColor

    // Некорректная строка цвета -> QColor invalid -> ожидаем false
    QVERIFY(!m->setData(idx, QString("NOT_A_COLOR"), Qt::EditRole));
}

void TestMyModel::tsv_roundtrip_via_buffer()
{
    // Данные
    m->slotAddData(MyRect(QColor("#112233"), Qt::DotLine, 5, 10, 20, 30, 40));
    m->slotAddData(MyRect(QColor("#AABBCC"), Qt::DashLine, 1,  0,  0,  1,  2));

    // save -> QBuffer
    QByteArray bytes;
    QBuffer out(&bytes);
    QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Text));
    QString err;
    QVERIFY(m->saveToTsv(out, &err));
    QVERIFY(err.isEmpty());

    // load -> новая модель
    MyModel m2;
    QBuffer in(&bytes);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(m2.loadFromTsv(in, &err));
    QVERIFY(err.isEmpty());

    QCOMPARE(m2.rowCount(), 2);
    QCOMPARE(m2.data(m2.index(0, 0), Qt::DisplayRole).toString(), QString("#112233"));
    QCOMPARE(m2.data(m2.index(0, 1), Qt::DisplayRole).toString(), QString("Qt::DotLine"));
    QCOMPARE(m2.data(m2.index(1, 1), Qt::DisplayRole).toString(), QString("Qt::DashLine"));
}

QTEST_MAIN(TestMyModel)
#include "tst_mymodel.moc"
