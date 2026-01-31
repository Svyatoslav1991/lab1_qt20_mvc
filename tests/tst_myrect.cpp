// tests/tst_myrect.cpp
#include <QtTest/QtTest>
#include "myrect.h"

class TestMyRect : public QObject
{
    Q_OBJECT
private slots:
    void defaults();
    void ctor_sets_all_fields();
};

void TestMyRect::defaults()
{
    MyRect r;
    QCOMPARE(r.penColor, QColor(Qt::black));
    QCOMPARE(r.penStyle, Qt::SolidLine);
    QCOMPARE(r.penWidth, 1);
    QCOMPARE(r.left, 0);
    QCOMPARE(r.top, 0);
    QCOMPARE(r.width, 10);
    QCOMPARE(r.height, 10);
}

void TestMyRect::ctor_sets_all_fields()
{
    MyRect r(QColor(Qt::red), Qt::DotLine, 3, 1, 2, 30, 40);
    QCOMPARE(r.penColor, QColor(Qt::red));
    QCOMPARE(r.penStyle, Qt::DotLine);
    QCOMPARE(r.penWidth, 3);
    QCOMPARE(r.left, 1);
    QCOMPARE(r.top, 2);
    QCOMPARE(r.width, 30);
    QCOMPARE(r.height, 40);
}

QTEST_MAIN(TestMyRect)
#include "tst_myrect.moc"
