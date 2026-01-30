// ======================= mymodel.h =======================
#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>

#include <array>
#include <cstddef> // std::size_t

#include "MyRect.h"

class QIODevice;

/**
 * @brief Табличная модель Qt (QAbstractTableModel) для хранения/редактирования списка MyRect.
 *
 * @details
 * ## Архитектура
 * MyModel — это слой **Model** в паттерне Qt Model/View.
 * Она:
 * - хранит данные (QVector<MyRect>),
 * - предоставляет их представлению (QTableView) через data(),
 * - принимает правки через setData(),
 * - уведомляет представление через dataChanged()/beginInsertRows()/endInsertRows()/beginResetModel()/endResetModel().
 *
 * ## Единая схема колонок
 * Чтобы заголовки, порядок столбцов и порядок полей в TSV не расходились,
 * используется массив @ref kColumns. Он задаёт:
 * - число столбцов,
 * - заголовки,
 * - семантику (какой Column в каком индексе).
 *
 * ## Роли данных
 * - Qt::DisplayRole:
 *   - PenColor  -> "#RRGGBB"
 *   - PenStyle  -> "Qt::DotLine" и т.п.
 *   - числа     -> int
 * - Qt::EditRole:
 *   - PenColor  -> QColor
 *   - PenStyle  -> int(Qt::PenStyle)
 *   - числа     -> int
 * - Qt::DecorationRole:
 *   - PenColor  -> QIcon с заливкой цветом
 *
 * ## Сериализация TSV
 * Реализованы перегрузки:
 * - saveToTsv(QString) / loadFromTsv(QString) — работа с файлами
 * - saveToTsv(QIODevice&) / loadFromTsv(QIODevice&) — удобно тестировать через QBuffer
 */
class MyModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MyModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex& index,
                 const QVariant& value,
                 int role = Qt::EditRole) override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant& value,
                       int role = Qt::EditRole) override;

    bool insertRows(int row,
                    int count,
                    const QModelIndex& parent = QModelIndex()) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void test();

    // --- Файловые обёртки ---
    bool saveToTsv(const QString& fileName, QString* error = nullptr) const;
    bool loadFromTsv(const QString& fileName, QString* error = nullptr);

    // --- Тестируемые варианты через QIODevice ---
    bool saveToTsv(QIODevice& out, QString* error = nullptr) const;
    bool loadFromTsv(QIODevice& in, QString* error = nullptr);

public slots:
    void slotAddData(const MyRect& rect);

private:
    /**
     * @brief Семантические столбцы модели.
     *
     * @note Column::Count — служебный элемент “число столбцов”, не реальный столбец.
     */
    enum class Column : int
    {
        PenColor = 0,
        PenStyle,
        PenWidth,
        Left,
        Top,
        Width,
        Height,
        Count
    };

    /**
     * @brief Метаданные столбца: семантика + заголовок.
     */
    struct ColumnInfo
    {
        Column      col;
        const char* header;
    };

    /**
     * @brief Количество столбцов как size_t (нужно для std::array).
     *
     * @details
     * Важно: std::array требует размер типа size_t.
     * Здесь безопасно приводим Column::Count к size_t.
     */
    static constexpr std::size_t kColCount =
        static_cast<std::size_t>(Column::Count);

    /**
     * @brief Количество столбцов в терминах Qt (int).
     */
    static constexpr int kColCountInt =
        static_cast<int>(kColCount);

    /**
     * @brief Единый источник правды о колонках (порядок/заголовки/TSV).
     */
    static constexpr std::array<ColumnInfo, kColCount> kColumns = {{
        {Column::PenColor, "PenColor"},
        {Column::PenStyle, "PenStyle"},
        {Column::PenWidth, "PenWidth"},
        {Column::Left,     "Left"},
        {Column::Top,      "Top"},
        {Column::Width,    "Width"},
        {Column::Height,   "Height"},
    }};

private:
    static QString penStyleToString(Qt::PenStyle style);
    static Qt::PenStyle penStyleFromString(const QString& s, bool* ok = nullptr);

    /**
     * @brief Какие роли нужно пометить “изменившимися” для данного столбца.
     *
     * @details
     * В сигнале dataChanged роли передаются как QVector<int> (совместимо с Qt5/Qt6).
     */
    static QVector<int> changedRolesForColumn(Column c);

private:
    QVector<MyRect> m_items;
};

#endif // MYMODEL_H
