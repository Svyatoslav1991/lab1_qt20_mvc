// tests/tst_mymodel.cpp
#include <QtTest/QtTest>

#include <QAbstractItemModelTester>
#include <QBuffer>
#include <QIcon>
#include <QSignalSpy>

#include "mymodel.h"

namespace {

// Колонки по твоей модели (см. MyModel::kColumns).
constexpr int kColPenColor = 0;
constexpr int kColPenStyle = 1;
constexpr int kColPenWidth = 2;
constexpr int kColLeft     = 3;
constexpr int kColTop      = 4;
constexpr int kColWidth    = 5;
constexpr int kColHeight   = 6;
constexpr int kColCount    = 7;

QVector<int> rolesFromSpyArgs(const QList<QVariant>& args)
{
    // dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)
    // В QSignalSpy аргументы лежат в QVariant.
    if (args.size() < 3)
        return {};
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return qvariant_cast<QVector<int>>(args.at(2));
#else
    return {};
#endif
}

bool vectorContainsRole(const QVector<int>& roles, int role)
{
    return std::find(roles.begin(), roles.end(), role) != roles.end();
}

void compareModelColor(const QAbstractItemModel& model, const QModelIndex& idx, const QColor& expected)
{
    const QVariant v = model.data(idx, Qt::EditRole);
    QVERIFY2(v.isValid(), "EditRole must be valid");
    QVERIFY2(v.canConvert<QColor>(), "EditRole must be convertible to QColor");
    QCOMPARE(v.value<QColor>(), expected);

    const QString disp = model.data(idx, Qt::DisplayRole).toString();
    QCOMPARE(disp, expected.name());
}

} // namespace

class TestMyModel : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    // basics
    void basics_row_col_and_parent();
    void headerData_horizontal_vertical_and_invalids();
    void flags_valid_and_invalid();

    // insertRows
    void insertRows_rejects_bad_args();
    void insertRows_clamps_row_and_inserts_defaults();

    // data()
    void data_invalid_index_and_out_of_range_returns_invalid();
    void data_roles_for_penColor();
    void data_roles_for_penStyle_and_numeric_columns();

    // setData()
    void setData_rejects_wrong_role_invalid_index_out_of_range();
    void setData_penColor_accepts_qcolor_and_emits_roles();
    void setData_penColor_accepts_string_color();
    void setData_rejects_invalid_color();
    void setData_no_change_returns_true_and_emits_nothing();
    void setData_penStyle_changes_and_updates_display();
    void setData_numeric_columns_change();

    // slotAddData()
    void slotAddData_appends_row_sets_values_and_emits_range_roles();

    // TSV: QIODevice mode checks
    void tsv_save_requires_open_writeonly();
    void tsv_load_requires_open_readonly();

    // TSV: roundtrip + parsing variants
    void tsv_roundtrip_via_buffer();
    void tsv_load_parses_penStyle_as_number_and_qtpenstyle_n();

    // TSV: errors + "model not modified on failure"
    void tsv_load_wrong_field_count_does_not_modify_model();
    void tsv_load_invalid_color_does_not_modify_model();
    void tsv_load_invalid_style_does_not_modify_model();
    void tsv_load_invalid_integer_does_not_modify_model();

private:
    MyModel* m = nullptr;
};

void TestMyModel::init()
{
    m = new MyModel();

    // Автоматический тест корректности модели (инварианты Qt Model/View).
    // Родитель = m, чтобы корректно освободилось при удалении модели.
    new QAbstractItemModelTester(m, QAbstractItemModelTester::FailureReportingMode::QtTest, m);
}

void TestMyModel::cleanup()
{
    delete m;
    m = nullptr;
}

// -------------------- basics --------------------

void TestMyModel::basics_row_col_and_parent()
{
    QCOMPARE(m->rowCount(), 0);
    QCOMPARE(m->columnCount(), kColCount);

    // Для плоской модели при валидном parent должно быть 0.
    const QModelIndex parent = m->index(0, 0); // даже если строк нет, index будет невалидный
    // Но мы проверим именно "валидный parent": вставим строку.
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex some = m->index(0, 0);
    QVERIFY(some.isValid());

    QCOMPARE(m->rowCount(some), 0);
    QCOMPARE(m->columnCount(some), 0);
}

void TestMyModel::headerData_horizontal_vertical_and_invalids()
{
    // Горизонтальные заголовки (строки из kColumns)
    QCOMPARE(m->headerData(kColPenColor, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenColor"));
    QCOMPARE(m->headerData(kColPenStyle, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenStyle"));
    QCOMPARE(m->headerData(kColHeight,   Qt::Horizontal, Qt::DisplayRole).toString(), QString("Height"));

    // Вертикальные: section + 1 (даже без строк Qt может спрашивать заголовки)
    QCOMPARE(m->headerData(0, Qt::Vertical, Qt::DisplayRole).toInt(), 1);
    QCOMPARE(m->headerData(5, Qt::Vertical, Qt::DisplayRole).toInt(), 6);

    // Неподдерживаемая роль
    QVERIFY(!m->headerData(0, Qt::Horizontal, Qt::EditRole).isValid());

    // Выход за диапазон для горизонтальных
    QVERIFY(!m->headerData(-1, Qt::Horizontal, Qt::DisplayRole).isValid());
    QVERIFY(!m->headerData(kColCount, Qt::Horizontal, Qt::DisplayRole).isValid());
}

void TestMyModel::flags_valid_and_invalid()
{
    // Невалидный индекс -> NoItemFlags
    const QModelIndex invalid;
    QCOMPARE(m->flags(invalid), Qt::NoItemFlags);

    // Вставим строку и проверим флаги валидного
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex idx = m->index(0, 0);
    QVERIFY(idx.isValid());

    const Qt::ItemFlags f = m->flags(idx);
    QVERIFY(f.testFlag(Qt::ItemIsEditable));
    QVERIFY(f.testFlag(Qt::ItemIsEnabled));
    QVERIFY(f.testFlag(Qt::ItemIsSelectable));
}

// -------------------- insertRows --------------------

void TestMyModel::insertRows_rejects_bad_args()
{
    // count <= 0 -> false
    QVERIFY(!m->insertRows(0, 0));
    QVERIFY(!m->insertRows(0, -1));

    // parent.isValid() -> false
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex parent = m->index(0, 0);
    QVERIFY(parent.isValid());
    QVERIFY(!m->insertRows(0, 1, parent));
}

void TestMyModel::insertRows_clamps_row_and_inserts_defaults()
{
    // row < 0 -> clamp to 0
    QVERIFY(m->insertRows(-100, 1));
    QCOMPARE(m->rowCount(), 1);

    // Проверим дефолтную строку (PenWidth == 1, PenColor == black, PenStyle == SolidLine, геометрия 0,0,10,10)
    compareModelColor(*m, m->index(0, kColPenColor), QColor(Qt::black));
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::EditRole).toInt(), static_cast<int>(Qt::SolidLine));
    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::EditRole).toInt(), 1);
    QCOMPARE(m->data(m->index(0, kColLeft), Qt::EditRole).toInt(), 0);
    QCOMPARE(m->data(m->index(0, kColTop), Qt::EditRole).toInt(), 0);
    QCOMPARE(m->data(m->index(0, kColWidth), Qt::EditRole).toInt(), 10);
    QCOMPARE(m->data(m->index(0, kColHeight), Qt::EditRole).toInt(), 10);

    // row > size -> clamp to size (append)
    QVERIFY(m->insertRows(999, 2));
    QCOMPARE(m->rowCount(), 3);
}

// -------------------- data() --------------------

void TestMyModel::data_invalid_index_and_out_of_range_returns_invalid()
{
    // invalid index
    QVERIFY(!m->data(QModelIndex(), Qt::DisplayRole).isValid());

    // out of range row/col -> invalid
    QVERIFY(m->insertRows(0, 1));

    QVERIFY(!m->data(m->index(-1, 0), Qt::DisplayRole).isValid());
    QVERIFY(!m->data(m->index( 1, 0), Qt::DisplayRole).isValid()); // rowCount()==1, row 1 out
    QVERIFY(!m->data(m->index( 0,-1), Qt::DisplayRole).isValid());
    QVERIFY(!m->data(m->index( 0, kColCount), Qt::DisplayRole).isValid());
}

void TestMyModel::data_roles_for_penColor()
{
    m->slotAddData(MyRect(QColor(Qt::green), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    // EditRole: QColor
    QVERIFY(m->data(idx, Qt::EditRole).canConvert<QColor>());
    QCOMPARE(m->data(idx, Qt::EditRole).value<QColor>(), QColor(Qt::green));

    // DisplayRole: "#RRGGBB"
    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QColor(Qt::green).name());

    // DecorationRole: QIcon
    QVERIFY(m->data(idx, Qt::DecorationRole).canConvert<QIcon>());

    // Неизвестная роль -> invalid
    QVERIFY(!m->data(idx, 123456).isValid());
}

void TestMyModel::data_roles_for_penStyle_and_numeric_columns()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::DotLine, 5, 10, 20, 30, 40));

    const QModelIndex penStyleIdx = m->index(0, kColPenStyle);
    QCOMPARE(m->data(penStyleIdx, Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));
    QCOMPARE(m->data(penStyleIdx, Qt::DisplayRole).toString(), QString("Qt::DotLine"));

    // Numeric columns: DisplayRole и EditRole одинаково int
    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::EditRole).toInt(), 5);
    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::DisplayRole).toInt(), 5);

    QCOMPARE(m->data(m->index(0, kColLeft), Qt::EditRole).toInt(), 10);
    QCOMPARE(m->data(m->index(0, kColTop), Qt::EditRole).toInt(), 20);
    QCOMPARE(m->data(m->index(0, kColWidth), Qt::EditRole).toInt(), 30);
    QCOMPARE(m->data(m->index(0, kColHeight), Qt::EditRole).toInt(), 40);
}

// -------------------- setData() --------------------

void TestMyModel::setData_rejects_wrong_role_invalid_index_out_of_range()
{
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex idx = m->index(0, 0);
    QVERIFY(idx.isValid());

    // wrong role
    QVERIFY(!m->setData(idx, QColor(Qt::blue), Qt::DisplayRole));

    // invalid index
    QVERIFY(!m->setData(QModelIndex(), 123, Qt::EditRole));

    // out of range
    QVERIFY(!m->setData(m->index(1, 0), 123, Qt::EditRole));            // row out
    QVERIFY(!m->setData(m->index(0, kColCount), 123, Qt::EditRole));    // col out
}

void TestMyModel::setData_penColor_accepts_qcolor_and_emits_roles()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(idx, QColor(Qt::blue), Qt::EditRole));
    QCOMPARE(spy.count(), 1);

    // Проверяем роли в dataChanged: для PenColor должны быть Display/Edit/Decoration
    const QVector<int> roles = rolesFromSpyArgs(spy.takeFirst());
    QVERIFY(vectorContainsRole(roles, Qt::DisplayRole));
    QVERIFY(vectorContainsRole(roles, Qt::EditRole));
    QVERIFY(vectorContainsRole(roles, Qt::DecorationRole));

    compareModelColor(*m, idx, QColor(Qt::blue));
}

void TestMyModel::setData_penColor_accepts_string_color()
{
    m->slotAddData(MyRect(QColor(Qt::black), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QModelIndex idx = m->index(0, kColPenColor);

    QVERIFY(m->setData(idx, QString("#112233"), Qt::EditRole));
    compareModelColor(*m, idx, QColor("#112233"));
}

void TestMyModel::setData_rejects_invalid_color()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(!m->setData(idx, QString("NOT_A_COLOR"), Qt::EditRole));
    QCOMPARE(spy.count(), 0); // не должно быть сигналов
    compareModelColor(*m, idx, QColor(Qt::red)); // осталось старое
}

void TestMyModel::setData_no_change_returns_true_and_emits_nothing()
{
    m->slotAddData(MyRect(QColor("#010203"), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    // Ставим то же значение — должно вернуть true, но не эмитить dataChanged.
    QVERIFY(m->setData(idx, QColor("#010203"), Qt::EditRole));
    QCOMPARE(spy.count(), 0);
}

void TestMyModel::setData_penStyle_changes_and_updates_display()
{
    m->slotAddData(MyRect(QColor(Qt::black), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QModelIndex idx = m->index(0, kColPenStyle);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(idx, static_cast<int>(Qt::DashLine), Qt::EditRole));
    QCOMPARE(spy.count(), 1);

    const QVector<int> roles = rolesFromSpyArgs(spy.takeFirst());
    QVERIFY(vectorContainsRole(roles, Qt::DisplayRole));
    QVERIFY(vectorContainsRole(roles, Qt::EditRole));
    QVERIFY(!vectorContainsRole(roles, Qt::DecorationRole)); // не PenColor

    QCOMPARE(m->data(idx, Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QString("Qt::DashLine"));
}

void TestMyModel::setData_numeric_columns_change()
{
    m->slotAddData(MyRect(QColor(Qt::black), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QModelIndex leftIdx = m->index(0, kColLeft);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(leftIdx, 777, Qt::EditRole));
    QCOMPARE(spy.count(), 1);

    const QVector<int> roles = rolesFromSpyArgs(spy.takeFirst());
    QVERIFY(vectorContainsRole(roles, Qt::DisplayRole));
    QVERIFY(vectorContainsRole(roles, Qt::EditRole));
    QVERIFY(!vectorContainsRole(roles, Qt::DecorationRole));

    QCOMPARE(m->data(leftIdx, Qt::EditRole).toInt(), 777);
    QCOMPARE(m->data(leftIdx, Qt::DisplayRole).toInt(), 777);
}

// -------------------- slotAddData() --------------------

void TestMyModel::slotAddData_appends_row_sets_values_and_emits_range_roles()
{
    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    const MyRect r(QColor("#AABBCC"), Qt::DotLine, 9, 1, 2, 3, 4);
    m->slotAddData(r);

    QCOMPARE(m->rowCount(), 1);

    // Значения записаны
    compareModelColor(*m, m->index(0, kColPenColor), QColor("#AABBCC"));
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString(), QString("Qt::DotLine"));
    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::EditRole).toInt(), 9);
    QCOMPARE(m->data(m->index(0, kColLeft), Qt::EditRole).toInt(), 1);
    QCOMPARE(m->data(m->index(0, kColTop), Qt::EditRole).toInt(), 2);
    QCOMPARE(m->data(m->index(0, kColWidth), Qt::EditRole).toInt(), 3);
    QCOMPARE(m->data(m->index(0, kColHeight), Qt::EditRole).toInt(), 4);

    // Должен быть dataChanged по диапазону всей строки
    QVERIFY(spy.count() >= 1);
    const QList<QVariant> args = spy.takeLast();
    const QModelIndex topLeft = qvariant_cast<QModelIndex>(args.at(0));
    const QModelIndex botRight = qvariant_cast<QModelIndex>(args.at(1));
    QCOMPARE(topLeft, m->index(0, 0));
    QCOMPARE(botRight, m->index(0, kColCount - 1));

    const QVector<int> roles = rolesFromSpyArgs(args);
    QVERIFY(vectorContainsRole(roles, Qt::DisplayRole));
    QVERIFY(vectorContainsRole(roles, Qt::EditRole));
    QVERIFY(vectorContainsRole(roles, Qt::DecorationRole));
}

// -------------------- TSV: QIODevice mode checks --------------------

void TestMyModel::tsv_save_requires_open_writeonly()
{
    m->slotAddData(MyRect(QColor("#112233"), Qt::DotLine, 5, 10, 20, 30, 40));

    QString err;

    // закрытое устройство
    QByteArray bytes;
    QBuffer closed(&bytes);
    QVERIFY(!closed.isOpen());
    QVERIFY(!m->saveToTsv(closed, &err));
    QVERIFY(!err.isEmpty());

    // открыто, но ReadOnly
    err.clear();
    QBuffer ro(&bytes);
    QVERIFY(ro.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(!m->saveToTsv(ro, &err));
    QVERIFY(!err.isEmpty());
}

void TestMyModel::tsv_load_requires_open_readonly()
{
    QString err;

    // закрытое устройство
    QByteArray bytes("something");
    QBuffer closed(&bytes);
    QVERIFY(!closed.isOpen());
    QVERIFY(!m->loadFromTsv(closed, &err));
    QVERIFY(!err.isEmpty());

    // открыто, но WriteOnly
    err.clear();
    QBuffer wo(&bytes);
    QVERIFY(wo.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(!m->loadFromTsv(wo, &err));
    QVERIFY(!err.isEmpty());
}

// -------------------- TSV: roundtrip + parsing variants --------------------

void TestMyModel::tsv_roundtrip_via_buffer()
{
    m->slotAddData(MyRect(QColor("#112233"), Qt::DotLine, 5, 10, 20, 30, 40));
    m->slotAddData(MyRect(QColor("#AABBCC"), Qt::DashLine, 1,  0,  0,  1,  2));

    QByteArray bytes;
    QBuffer out(&bytes);
    QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Text));
    QString err;
    QVERIFY(m->saveToTsv(out, &err));
    QVERIFY(err.isEmpty());

    MyModel m2;
    QBuffer in(&bytes);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(m2.loadFromTsv(in, &err));
    QVERIFY(err.isEmpty());

    QCOMPARE(m2.rowCount(), 2);
    QCOMPARE(m2.data(m2.index(0, kColPenColor), Qt::DisplayRole).toString(), QString("#112233"));
    QCOMPARE(m2.data(m2.index(0, kColPenStyle), Qt::DisplayRole).toString(), QString("Qt::DotLine"));
    QCOMPARE(m2.data(m2.index(1, kColPenStyle), Qt::DisplayRole).toString(), QString("Qt::DashLine"));
}

void TestMyModel::tsv_load_parses_penStyle_as_number_and_qtpenstyle_n()
{
    // style "3" и "Qt::PenStyle(3)" должны распарситься как DotLine
    const QByteArray bytes =
        "#112233\t3\t1\t0\t0\t10\t10\n"
        "#445566\tQt::PenStyle(3)\t2\t1\t2\t3\t4\n";

    QBuffer in(const_cast<QByteArray*>(&bytes));
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QString err;
    QVERIFY(m->loadFromTsv(in, &err));
    QVERIFY(err.isEmpty());

    QCOMPARE(m->rowCount(), 2);
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString(), QString("Qt::DotLine"));
    QCOMPARE(m->data(m->index(1, kColPenStyle), Qt::DisplayRole).toString(), QString("Qt::DotLine"));
}

// -------------------- TSV errors + non-modification guarantee --------------------

void TestMyModel::tsv_load_wrong_field_count_does_not_modify_model()
{
    // Исходное состояние
    m->slotAddData(MyRect(QColor("#ABCDEF"), Qt::DashLine, 7, 1, 2, 3, 4));
    const int beforeRows = m->rowCount();
    const QString beforeColor = m->data(m->index(0, kColPenColor), Qt::DisplayRole).toString();
    const QString beforeStyle = m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString();

    // 6 полей вместо 7
    QByteArray bad = "#112233\tQt::DotLine\t1\t0\t0\t10\n";
    QBuffer in(&bad);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QString err;
    QVERIFY(!m->loadFromTsv(in, &err));
    QVERIFY(!err.isEmpty());

    // Модель не изменилась
    QCOMPARE(m->rowCount(), beforeRows);
    QCOMPARE(m->data(m->index(0, kColPenColor), Qt::DisplayRole).toString(), beforeColor);
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString(), beforeStyle);
}

void TestMyModel::tsv_load_invalid_color_does_not_modify_model()
{
    m->slotAddData(MyRect(QColor("#010203"), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QString before = m->data(m->index(0, kColPenColor), Qt::DisplayRole).toString();

    QByteArray bad = "NOT_A_COLOR\tQt::DotLine\t1\t0\t0\t10\t10\n";
    QBuffer in(&bad);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QString err;
    QVERIFY(!m->loadFromTsv(in, &err));
    QVERIFY(!err.isEmpty());

    QCOMPARE(m->data(m->index(0, kColPenColor), Qt::DisplayRole).toString(), before);
}

void TestMyModel::tsv_load_invalid_style_does_not_modify_model()
{
    m->slotAddData(MyRect(QColor("#010203"), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QString before = m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString();

    QByteArray bad = "#112233\tQt::SomeUnknownStyle\t1\t0\t0\t10\t10\n";
    QBuffer in(&bad);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QString err;
    QVERIFY(!m->loadFromTsv(in, &err));
    QVERIFY(!err.isEmpty());

    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString(), before);
}

void TestMyModel::tsv_load_invalid_integer_does_not_modify_model()
{
    m->slotAddData(MyRect(QColor("#010203"), Qt::SolidLine, 1, 0, 0, 10, 10));
    const int beforeLeft = m->data(m->index(0, kColLeft), Qt::EditRole).toInt();

    // Left = "NOPE"
    QByteArray bad = "#112233\tQt::DotLine\t1\tNOPE\t0\t10\t10\n";
    QBuffer in(&bad);
    QVERIFY(in.open(QIODevice::ReadOnly | QIODevice::Text));
    QString err;
    QVERIFY(!m->loadFromTsv(in, &err));
    QVERIFY(!err.isEmpty());

    QCOMPARE(m->data(m->index(0, kColLeft), Qt::EditRole).toInt(), beforeLeft);
}

QTEST_MAIN(TestMyModel)
#include "tst_mymodel.moc"
