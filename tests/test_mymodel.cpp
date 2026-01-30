#include <QtTest>

#include "../mymodel.h"

/**
 * Unit-тесты для MyModel
 */
class TestMyModel : public QObject
{
    Q_OBJECT

private slots:

    void init();
    void cleanup();

    void testEmptyModel();
    void testInsert();
    void testSetData();
    void testSaveLoad();
};

MyModel* model = nullptr;


/**
 * Вызывается перед каждым тестом
 */
void TestMyModel::init()
{
    model = new MyModel();
}

/**
 * Вызывается после каждого теста
 */
void TestMyModel::cleanup()
{
    delete model;
    model = nullptr;
}


/**
 * Проверяем пустую модель
 */
void TestMyModel::testEmptyModel()
{
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 7);
}


/**
 * Проверка вставки строк
 */
void TestMyModel::testInsert()
{
    QVERIFY(model->insertRows(0, 2));

    QCOMPARE(model->rowCount(), 2);

    QModelIndex idx = model->index(0, 0);
    QVERIFY(idx.isValid());
}


/**
 * Проверяем setData / data
 */
void TestMyModel::testSetData()
{
    model->insertRows(0, 1);

    QModelIndex colorIdx = model->index(0, 0);
    QModelIndex widthIdx = model->index(0, 2);

    QVERIFY(model->setData(colorIdx, QColor(Qt::blue)));
    QVERIFY(model->setData(widthIdx, 5));

    QCOMPARE(
        model->data(colorIdx, Qt::EditRole).value<QColor>(),
        QColor(Qt::blue)
    );

    QCOMPARE(
        model->data(widthIdx, Qt::EditRole).toInt(),
        5
    );
}


/**
 * Проверка save/load через QBuffer
 */
void TestMyModel::testSaveLoad()
{
    model->slotAddData(
        MyRect(QColor(Qt::red), Qt::DotLine, 3, 1, 2, 3, 4)
    );

    QByteArray data;
    QBuffer buffer(&data);

    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(model->saveToTsv(buffer));
    buffer.close();


    MyModel loaded;

    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QVERIFY(loaded.loadFromTsv(buffer));
    buffer.close();


    QCOMPARE(loaded.rowCount(), 1);

    QModelIndex idx = loaded.index(0, 0);

    QColor c = loaded.data(idx, Qt::EditRole).value<QColor>();

    QCOMPARE(c, QColor(Qt::red));
}


QTEST_MAIN(TestMyModel)
#include "test_mymodel.moc"
