// tests/tst_mymodel.cpp
/**
 * @file tst_mymodel.cpp
 * @brief Набор unit-тестов для класса MyModel (Qt QAbstractTableModel).
 *
 * @details
 * Цели этих тестов:
 * 1) Зафиксировать контракт MyModel как Qt-модели (Model/View):
 *    - rowCount/columnCount для плоской таблицы;
 *    - headerData, flags;
 *    - корректная работа data() и setData() по ролям;
 *    - корректные сигналы dataChanged и их роли.
 *
 * 2) Проверить бизнес-логику модели:
 *    - insertRows: валидация аргументов, clamping row, вставка дефолтных MyRect{};
 *    - slotAddData: добавление строки и обновление диапазона;
 *    - сериализация TSV (saveToTsv/loadFromTsv через QIODevice):
 *      - roundtrip;
 *      - валидация формата и гарантия "не менять модель при ошибке";
 *      - поддержка разных форматов PenStyle при чтении.
 *
 * 3) Минимизировать “ложноположительные” тесты:
 *    - сравниваем не только значения, но и роли в dataChanged,
 *      потому что View/Delegate зависят от корректного набора ролей;
 *    - негативные тесты проверяют, что при ошибке состояние модели не меняется.
 *
 * @note
 * Тесты намеренно опираются на публичный интерфейс MyModel.
 * Приватные детали (enum Column, kColumns) напрямую недоступны, поэтому
 * индексы колонок здесь заданы константами в соответствии с порядком столбцов.
 */

#include <QtTest/QtTest>

#include <QAbstractItemModelTester>
#include <QBuffer>
#include <QIcon>
#include <QSignalSpy>

#include "mymodel.h"

namespace {

/**
 * @name Константы индексов колонок
 * @brief Индексы столбцов в таблице MyModel (соответствуют порядку в MyModel::kColumns).
 *
 * @details
 * Важно держать их синхронизированными с моделью:
 * - PenColor  = 0
 * - PenStyle  = 1
 * - PenWidth  = 2
 * - Left      = 3
 * - Top       = 4
 * - Width     = 5
 * - Height    = 6
 */
///@{
constexpr int kColPenColor = 0;
constexpr int kColPenStyle = 1;
constexpr int kColPenWidth = 2;
constexpr int kColLeft     = 3;
constexpr int kColTop      = 4;
constexpr int kColWidth    = 5;
constexpr int kColHeight   = 6;
constexpr int kColCount    = 7;
///@}

/**
 * @brief Достаёт QVector<int> ролей из аргументов QSignalSpy для сигнала dataChanged.
 *
 * @details
 * Сигнал Qt:
 *   dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
 *
 * QSignalSpy сохраняет аргументы как QList<QVariant>, где:
 * - args[0] -> QModelIndex topLeft
 * - args[1] -> QModelIndex bottomRight
 * - args[2] -> QVector<int> roles
 *
 * @param args Аргументы одной записи QSignalSpy (spy.takeFirst()).
 * @return Вектор ролей, либо пустой вектор при несоответствии формата.
 */
QVector<int> rolesFromSpyArgs(const QList<QVariant>& args)
{
    if (args.size() < 3)
        return {};
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return qvariant_cast<QVector<int>>(args.at(2));
#else
    return {};
#endif
}

/**
 * @brief Проверяет наличие роли в QVector<int>.
 * @param roles Вектор ролей (например из rolesFromSpyArgs()).
 * @param role Конкретная роль Qt::ItemDataRole.
 * @return true если роль присутствует.
 */
bool vectorContainsRole(const QVector<int>& roles, int role)
{
    return std::find(roles.begin(), roles.end(), role) != roles.end();
}

/**
 * @brief Утилита для проверки цвета в модели по двум ролям.
 *
 * @details
 * Для PenColor по контракту MyModel:
 * - EditRole     -> QColor (машинное значение для делегата)
 * - DisplayRole  -> QString "#RRGGBB" (человекочитаемое значение)
 *
 * @param model Ссылка на модель.
 * @param idx Индекс ячейки PenColor.
 * @param expected Ожидаемый QColor.
 */
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

/**
 * @brief Тестовый набор для класса MyModel.
 *
 * @details
 * Организация:
 * - init/cleanup: создаём "чистую" модель на каждый тест
 *   и подключаем QAbstractItemModelTester, который ловит нарушения инвариантов Qt.
 *
 * - Группы тестов:
 *   - basics: row/col, headers, flags.
 *   - insertRows: валидация и дефолтные значения.
 *   - data(): корректные роли и обработка невалидных индексов.
 *   - setData(): валидация входа, обновления, dataChanged и роли.
 *   - slotAddData(): добавление строки и обновление диапазона.
 *   - TSV: требования к QIODevice режимам, roundtrip, парсинг, ошибки и неизменность модели при ошибке.
 */
class TestMyModel : public QObject
{
    Q_OBJECT
private slots:
    /**
     * @brief Вызывается перед каждым тестом.
     *
     * @details
     * Создаёт новую модель и подключает QAbstractItemModelTester,
     * который автоматически проверяет корректность реализации QAbstractItemModel:
     * - корректные begin/end* пары;
     * - корректность индексов;
     * - отсутствие "плохих" сигналов и нарушений инвариантов.
     */
    void init();

    /**
     * @brief Вызывается после каждого теста.
     * @details Освобождает модель.
     */
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
    /// Текущая тестируемая модель (создаётся заново в init()).
    MyModel* m = nullptr;
};

// -------------------- fixture --------------------

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

/**
 * @brief Проверяет rowCount/columnCount и поведение при parent.isValid().
 *
 * @details
 * MyModel — плоская таблица, поэтому:
 * - при parent.isValid() rowCount() и columnCount() должны возвращать 0.
 * - при parent невалидном: columnCount() должен быть фиксированным (7),
 *   rowCount() зависит от m_items.
 */
void TestMyModel::basics_row_col_and_parent()
{
    QCOMPARE(m->rowCount(), 0);
    QCOMPARE(m->columnCount(), kColCount);

    // Чтобы получить валидный parent, вставим строку.
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex some = m->index(0, 0);
    QVERIFY(some.isValid());

    // Для плоской модели при валидном parent должно быть 0.
    QCOMPARE(m->rowCount(some), 0);
    QCOMPARE(m->columnCount(some), 0);
}

/**
 * @brief Проверяет headerData для горизонтальных/вертикальных заголовков и ошибок.
 *
 * @details
 * Ожидаемое поведение MyModel:
 * - Horizontal + DisplayRole -> текст заголовка ("PenColor", "PenStyle", ...).
 * - Vertical + DisplayRole -> section + 1.
 * - Любая другая роль -> invalid QVariant.
 * - Horizontal вне диапазона секций -> invalid QVariant.
 */
void TestMyModel::headerData_horizontal_vertical_and_invalids()
{
    // Горизонтальные заголовки (строки из kColumns)
    QCOMPARE(m->headerData(kColPenColor, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenColor"));
    QCOMPARE(m->headerData(kColPenStyle, Qt::Horizontal, Qt::DisplayRole).toString(), QString("PenStyle"));
    QCOMPARE(m->headerData(kColHeight,   Qt::Horizontal, Qt::DisplayRole).toString(), QString("Height"));

    // Вертикальные: section + 1 (Qt может спрашивать заголовки и при 0 строк)
    QCOMPARE(m->headerData(0, Qt::Vertical, Qt::DisplayRole).toInt(), 1);
    QCOMPARE(m->headerData(5, Qt::Vertical, Qt::DisplayRole).toInt(), 6);

    // Неподдерживаемая роль
    QVERIFY(!m->headerData(0, Qt::Horizontal, Qt::EditRole).isValid());

    // Выход за диапазон для горизонтальных
    QVERIFY(!m->headerData(-1, Qt::Horizontal, Qt::DisplayRole).isValid());
    QVERIFY(!m->headerData(kColCount, Qt::Horizontal, Qt::DisplayRole).isValid());
}

/**
 * @brief Проверяет flags() для валидного и невалидного индекса.
 *
 * @details
 * По контракту:
 * - для невалидного индекса -> Qt::NoItemFlags;
 * - для валидного -> базовые флаги + Qt::ItemIsEditable.
 */
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

/**
 * @brief Негативные сценарии insertRows: неправильные аргументы.
 *
 * @details
 * По реализации:
 * - count <= 0 -> false;
 * - parent.isValid() -> false (модель плоская).
 */
void TestMyModel::insertRows_rejects_bad_args()
{
    QVERIFY(!m->insertRows(0, 0));
    QVERIFY(!m->insertRows(0, -1));

    // parent.isValid() -> false
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex parent = m->index(0, 0);
    QVERIFY(parent.isValid());
    QVERIFY(!m->insertRows(0, 1, parent));
}

/**
 * @brief Проверяет clamping row и дефолтные значения вставляемых MyRect{}.
 *
 * @details
 * По реализации:
 * - row < 0 -> clamp to 0;
 * - row > size -> clamp to size (append).
 *
 * Также фиксируем "дефолты" MyRect{} (как они реально используются моделью),
 * чтобы изменения MyRect по умолчанию не проходили незаметно.
 */
void TestMyModel::insertRows_clamps_row_and_inserts_defaults()
{
    // row < 0 -> clamp to 0
    QVERIFY(m->insertRows(-100, 1));
    QCOMPARE(m->rowCount(), 1);

    // Дефолтная строка: ожидаем значения MyRect{} как в текущем проекте
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

/**
 * @brief Проверяет, что data() корректно возвращает invalid QVariant для невалидных индексов и выхода за диапазон.
 */
void TestMyModel::data_invalid_index_and_out_of_range_returns_invalid()
{
    QVERIFY(!m->data(QModelIndex(), Qt::DisplayRole).isValid());

    QVERIFY(m->insertRows(0, 1));

    QVERIFY(!m->data(m->index(-1, 0), Qt::DisplayRole).isValid());
    QVERIFY(!m->data(m->index( 1, 0), Qt::DisplayRole).isValid()); // row out
    QVERIFY(!m->data(m->index( 0,-1), Qt::DisplayRole).isValid());
    QVERIFY(!m->data(m->index( 0, kColCount), Qt::DisplayRole).isValid());
}

/**
 * @brief Проверяет роли data() для PenColor: EditRole/DisplayRole/DecorationRole.
 *
 * @details
 * PenColor — особый столбец:
 * - EditRole возвращает QColor (для делегата/редактора),
 * - DisplayRole возвращает строку "#RRGGBB",
 * - DecorationRole возвращает QIcon для визуализации цвета.
 */
void TestMyModel::data_roles_for_penColor()
{
    m->slotAddData(MyRect(QColor(Qt::green), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QVERIFY(m->data(idx, Qt::EditRole).canConvert<QColor>());
    QCOMPARE(m->data(idx, Qt::EditRole).value<QColor>(), QColor(Qt::green));

    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QColor(Qt::green).name());

    QVERIFY(m->data(idx, Qt::DecorationRole).canConvert<QIcon>());

    // Неизвестная роль -> invalid
    QVERIFY(!m->data(idx, 123456).isValid());
}

/**
 * @brief Проверяет роли data() для PenStyle и числовых столбцов.
 *
 * @details
 * - PenStyle: EditRole -> int(Qt::PenStyle), DisplayRole -> строка "Qt::DotLine"
 * - Numeric columns: DisplayRole и EditRole возвращают int
 */
void TestMyModel::data_roles_for_penStyle_and_numeric_columns()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::DotLine, 5, 10, 20, 30, 40));

    const QModelIndex penStyleIdx = m->index(0, kColPenStyle);
    QCOMPARE(m->data(penStyleIdx, Qt::EditRole).toInt(), static_cast<int>(Qt::DotLine));
    QCOMPARE(m->data(penStyleIdx, Qt::DisplayRole).toString(), QString("Qt::DotLine"));

    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::EditRole).toInt(), 5);
    QCOMPARE(m->data(m->index(0, kColPenWidth), Qt::DisplayRole).toInt(), 5);

    QCOMPARE(m->data(m->index(0, kColLeft), Qt::EditRole).toInt(), 10);
    QCOMPARE(m->data(m->index(0, kColTop), Qt::EditRole).toInt(), 20);
    QCOMPARE(m->data(m->index(0, kColWidth), Qt::EditRole).toInt(), 30);
    QCOMPARE(m->data(m->index(0, kColHeight), Qt::EditRole).toInt(), 40);
}

// -------------------- setData() --------------------

/**
 * @brief Негативные сценарии setData: неправильная роль, невалидный индекс, выход за диапазон.
 */
void TestMyModel::setData_rejects_wrong_role_invalid_index_out_of_range()
{
    QVERIFY(m->insertRows(0, 1));
    const QModelIndex idx = m->index(0, 0);
    QVERIFY(idx.isValid());

    // role != EditRole -> false
    QVERIFY(!m->setData(idx, QColor(Qt::blue), Qt::DisplayRole));

    // invalid index -> false
    QVERIFY(!m->setData(QModelIndex(), 123, Qt::EditRole));

    // out of range -> false
    QVERIFY(!m->setData(m->index(1, 0), 123, Qt::EditRole));
    QVERIFY(!m->setData(m->index(0, kColCount), 123, Qt::EditRole));
}

/**
 * @brief Проверяет setData для PenColor при передаче QColor + корректные роли dataChanged.
 *
 * @details
 * Важный момент: для PenColor в dataChanged должны быть
 * DisplayRole + EditRole + DecorationRole, иначе View может не обновить иконку.
 */
void TestMyModel::setData_penColor_accepts_qcolor_and_emits_roles()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(idx, QColor(Qt::blue), Qt::EditRole));
    QCOMPARE(spy.count(), 1);

    const QVector<int> roles = rolesFromSpyArgs(spy.takeFirst());
    QVERIFY(vectorContainsRole(roles, Qt::DisplayRole));
    QVERIFY(vectorContainsRole(roles, Qt::EditRole));
    QVERIFY(vectorContainsRole(roles, Qt::DecorationRole));

    compareModelColor(*m, idx, QColor(Qt::blue));
}

/**
 * @brief Проверяет setData для PenColor при передаче строки "#RRGGBB".
 */
void TestMyModel::setData_penColor_accepts_string_color()
{
    m->slotAddData(MyRect(QColor(Qt::black), Qt::SolidLine, 1, 0, 0, 10, 10));
    const QModelIndex idx = m->index(0, kColPenColor);

    QVERIFY(m->setData(idx, QString("#112233"), Qt::EditRole));
    compareModelColor(*m, idx, QColor("#112233"));
}

/**
 * @brief Проверяет, что setData отвергает невалидный цвет и не меняет модель/не эмитит dataChanged.
 */
void TestMyModel::setData_rejects_invalid_color()
{
    m->slotAddData(MyRect(QColor(Qt::red), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(!m->setData(idx, QString("NOT_A_COLOR"), Qt::EditRole));
    QCOMPARE(spy.count(), 0);
    compareModelColor(*m, idx, QColor(Qt::red));
}

/**
 * @brief Проверяет оптимизацию "без изменений": setData возвращает true, но не эмитит dataChanged.
 */
void TestMyModel::setData_no_change_returns_true_and_emits_nothing()
{
    m->slotAddData(MyRect(QColor("#010203"), Qt::SolidLine, 2, 1, 2, 3, 4));
    const QModelIndex idx = m->index(0, kColPenColor);

    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    QVERIFY(m->setData(idx, QColor("#010203"), Qt::EditRole));
    QCOMPARE(spy.count(), 0);
}

/**
 * @brief Проверяет setData для PenStyle: изменение значения, сигнал dataChanged и строковое отображение.
 */
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
    QVERIFY(!vectorContainsRole(roles, Qt::DecorationRole));

    QCOMPARE(m->data(idx, Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
    QCOMPARE(m->data(idx, Qt::DisplayRole).toString(), QString("Qt::DashLine"));
}

/**
 * @brief Проверяет setData для числовых столбцов (на примере Left).
 */
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

/**
 * @brief Проверяет slotAddData:
 *  - добавляет строку в конец,
 *  - значения действительно записаны в модель,
 *  - dataChanged эмитится по диапазону всей строки и содержит ключевые роли.
 *
 * @details
 * slotAddData после insertRows() записывает данные в m_items[row],
 * и затем эмитит dataChanged по всей строке:
 * (row, 0) .. (row, lastCol) с ролями Display/Edit/Decoration,
 * чтобы обновились и текст, и значения редакторов, и иконка для цвета.
 */
void TestMyModel::slotAddData_appends_row_sets_values_and_emits_range_roles()
{
    QSignalSpy spy(m, &MyModel::dataChanged);
    QVERIFY(spy.isValid());

    const MyRect r(QColor("#AABBCC"), Qt::DotLine, 9, 1, 2, 3, 4);
    m->slotAddData(r);

    QCOMPARE(m->rowCount(), 1);

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

/**
 * @brief Проверяет, что saveToTsv(QIODevice&) требует open + WriteOnly.
 *
 * @details
 * Модель должна корректно отказывать:
 * - если устройство закрыто,
 * - если оно открыто, но не WriteOnly.
 */
void TestMyModel::tsv_save_requires_open_writeonly()
{
    m->slotAddData(MyRect(QColor("#112233"), Qt::DotLine, 5, 10, 20, 30, 40));

    QString err;

    // Закрытое устройство
    QByteArray bytes;
    QBuffer closed(&bytes);
    QVERIFY(!closed.isOpen());
    QVERIFY(!m->saveToTsv(closed, &err));
    QVERIFY(!err.isEmpty());

    // Открыто, но ReadOnly
    err.clear();
    QBuffer ro(&bytes);
    QVERIFY(ro.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(!m->saveToTsv(ro, &err));
    QVERIFY(!err.isEmpty());
}

/**
 * @brief Проверяет, что loadFromTsv(QIODevice&) требует open + ReadOnly.
 *
 * @details
 * Модель должна корректно отказывать:
 * - если устройство закрыто,
 * - если оно открыто, но не ReadOnly.
 */
void TestMyModel::tsv_load_requires_open_readonly()
{
    QString err;

    // Закрытое устройство
    QByteArray bytes("something");
    QBuffer closed(&bytes);
    QVERIFY(!closed.isOpen());
    QVERIFY(!m->loadFromTsv(closed, &err));
    QVERIFY(!err.isEmpty());

    // Открыто, но WriteOnly
    err.clear();
    QBuffer wo(&bytes);
    QVERIFY(wo.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(!m->loadFromTsv(wo, &err));
    QVERIFY(!err.isEmpty());
}

// -------------------- TSV: roundtrip + parsing variants --------------------

/**
 * @brief Roundtrip тест: saveToTsv -> bytes -> loadFromTsv в новую модель.
 *
 * @details
 * Проверяем, что:
 * - сериализация не возвращает ошибку,
 * - десериализация не возвращает ошибку,
 * - количество строк совпадает,
 * - ключевые поля корректно восстанавливаются.
 */
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

/**
 * @brief Проверяет поддержку альтернативных форматов PenStyle при чтении TSV.
 *
 * @details
 * MyModel::penStyleFromString() поддерживает:
 * - "3" (число)
 * - "Qt::PenStyle(3)"
 * - "Qt::DotLine" (полное имя)
 *
 * Здесь проверяем первые два варианта и ожидаем, что стиль станет DotLine.
 */
void TestMyModel::tsv_load_parses_penStyle_as_number_and_qtpenstyle_n()
{
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

/**
 * @brief Ошибка формата: неправильное число полей.
 *
 * @details
 * Важно проверить гарантию loadFromTsv():
 * - при первой ошибке возвращает false,
 * - и НЕ меняет текущую модель (используется временный QVector<MyRect> tmp).
 */
void TestMyModel::tsv_load_wrong_field_count_does_not_modify_model()
{
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

    QCOMPARE(m->rowCount(), beforeRows);
    QCOMPARE(m->data(m->index(0, kColPenColor), Qt::DisplayRole).toString(), beforeColor);
    QCOMPARE(m->data(m->index(0, kColPenStyle), Qt::DisplayRole).toString(), beforeStyle);
}

/**
 * @brief Ошибка формата: невалидный цвет в первом поле.
 *
 * @details
 * Проверяем, что:
 * - loadFromTsv возвращает false,
 * - error непустой,
 * - существующая модель не меняется.
 */
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

/**
 * @brief Ошибка формата: невалидный стиль пера.
 *
 * @details
 * По реализации penStyleFromString():
 * - если стиль не распознан, styleOk=false => loadFromTsv возвращает false.
 * Также проверяем гарантию “модель не меняется при ошибке”.
 */
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

/**
 * @brief Ошибка формата: одно из числовых полей не парсится в int.
 *
 * @details
 * В loadFromTsv() есть локальная parseInt(), которая при ошибке:
 * - возвращает false,
 * - пишет error,
 * - и не меняет текущую модель (tmp не применяется).
 */
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
