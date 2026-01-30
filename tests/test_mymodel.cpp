#include <QtTest>
#include <QBuffer>
#include <QByteArray>

#include "../mymodel.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  #include <QAbstractItemModelTester>
#endif

class TestMyModel : public QObject {
    Q_OBJECT
private slots:
    void smoke_countsAndHeaders() {
        MyModel m;
        QCOMPARE(m.rowCount(), 0);
        QCOMPARE(m.columnCount(), 7);

        QCOMPARE(m.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenColor"));
        QCOMPARE(m.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenStyle"));
        QCOMPARE(m.headerData(0, Qt::Vertical, Qt::DisplayRole).toInt(), 1);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    void modelTester_consistency() {
        MyModel m;
        QAbstractItemModelTester tester(&m, QAbstractItemModelTester::FailureReportingMode::QtTest);
        Q_UNUSED(tester);
        QVERIFY(true);
    }
#endif

    void insertRows_basic() {
        MyModel m;
        QVERIFY(m.insertRows(0, 2));
        QCOMPARE(m.rowCount(), 2);

        // Проверим, что индексы валидны
        QVERIFY(m.index(0, 0).isValid());
        QVERIFY(m.index(1, 6).isValid());
    }

    void setData_roundTrip_editRole() {
        MyModel m;
        QVERIFY(m.insertRows(0, 1));

        const QModelIndex colorIdx = m.index(0, 0); // PenColor
        const QModelIndex styleIdx = m.index(0, 1); // PenStyle
        const QModelIndex widthIdx = m.index(0, 2); // PenWidth

        QVERIFY(m.setData(colorIdx, QColor(Qt::blue), Qt::EditRole));
        QVERIFY(m.setData(styleIdx, static_cast<int>(Qt::DashLine), Qt::EditRole));
        QVERIFY(m.setData(widthIdx, 5, Qt::EditRole));

        QCOMPARE(m.data(colorIdx, Qt::EditRole).value<QColor>(), QColor(Qt::blue));
        QCOMPARE(m.data(styleIdx, Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
        QCOMPARE(m.data(widthIdx, Qt::EditRole).toInt(), 5);

        // DisplayRole: цвет строкой, стиль человекочитаемо
        QCOMPARE(m.data(colorIdx, Qt::DisplayRole).toString(), QColor(Qt::blue).name());
        QVERIFY(m.data(styleIdx, Qt::DisplayRole).toString().startsWith("Qt::"));
    }

    void saveLoad_viaQBuffer() {
        MyModel m;
        m.slotAddData(MyRect(QColor(Qt::red), Qt::DotLine, 3, 1, 2, 3, 4));
        m.slotAddData(MyRect(QColor(Qt::green), Qt::SolidLine, 2, 10, 20, 30, 40));

        QByteArray bytes;
        QBuffer out(&bytes);
        QVERIFY(out.open(QIODevice::WriteOnly));
        QVERIFY(m.saveToTsv(out));
        out.close();

        MyModel loaded;
        QBuffer in(&bytes);
        QVERIFY(in.open(QIODevice::ReadOnly));
        QVERIFY(loaded.loadFromTsv(in));
        in.close();

        QCOMPARE(loaded.rowCount(), 2);
        QCOMPARE(loaded.data(loaded.index(0,0), Qt::EditRole).value<QColor>(), QColor(Qt::red));
        QCOMPARE(loaded.data(loaded.index(0,1), Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));
        QCOMPARE(loaded.data(loaded.index(1,0), Qt::EditRole).value<QColor>(), QColor(Qt::green));
    }

    void load_invalidFormat_failsAndDoesNotChangeModel() {
        MyModel m;
        m.slotAddData(MyRect(QColor(Qt::red), Qt::DotLine, 3, 1, 2, 3, 4));
        QCOMPARE(m.rowCount(), 1);

        QByteArray bad("only\t2fields\n"); // должно быть 7 полей
        QBuffer in(&bad);
        QVERIFY(in.open(QIODevice::ReadOnly));

        QString err;
        QVERIFY(!m.loadFromTsv(in, &err));
        QVERIFY(!err.isEmpty());

        // модель не должна измениться
        QCOMPARE(m.rowCount(), 1);
    }
};

QTEST_MAIN(TestMyModel)
#include "test_mymodel.moc"
