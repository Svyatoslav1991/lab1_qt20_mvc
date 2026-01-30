#include "mymodel.h"

#include <QIcon>
#include <QPixmap>
#include <QtGlobal>

/**
 * @brief Конструктор модели.
 *
 * Формирует заголовки столбцов. Проверяет согласованность размера заголовков с числом столбцов.
 */
MyModel::MyModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Заголовки — как рекомендует методичка
    m_headerData
        << "PenColor"
        << "PenStyle"
        << "PenWidth"
        << "Left"
        << "Top"
        << "Width"
        << "Height";

    Q_ASSERT(m_headerData.size() == columnCountValue());
}

int MyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_vector.size();
}

int MyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return columnCountValue();
}

QVariant MyModel::data(const QModelIndex& index, int role) const
{
    // 1) проверка индекса
    if (!index.isValid())
        return {};

    // 2) координаты ячейки
    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_vector.size())
        return {};
    if (col < 0 || col >= columnCountValue())
        return {};

    const MyRect& r = m_vector[row];
    const Column column = static_cast<Column>(col);

    // DisplayRole — текст/числа
    if (role == Qt::DisplayRole)
    {
        switch (column)
        {
        case Column::PenColor:
            // Текстовое представление цвета (иконка даётся отдельно через DecorationRole)
            return r.penColor.name(); // "#RRGGBB"

        case Column::PenStyle:
            // QVariant не хранит Qt::PenStyle напрямую, поэтому отдаём int
            return static_cast<int>(r.penStyle);

        case Column::PenWidth: return r.penWidth;
        case Column::Left:     return r.left;
        case Column::Top:      return r.top;
        case Column::Width:    return r.width;
        case Column::Height:   return r.height;

        case Column::Count:
            break;
        }
        return {};
    }

    // DecorationRole — пиктограмма для цвета
    if (role == Qt::DecorationRole && column == Column::PenColor)
    {
        QPixmap px(32, 32);
        px.fill(r.penColor);
        return QIcon(px);
    }

    return {};
}

QVariant MyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal)
    {
        if (section >= 0 && section < m_headerData.size())
            return m_headerData.at(section);
        return {};
    }

    // Вертикальные заголовки: 1..N
    return section + 1;
}

bool MyModel::setHeaderData(int section,
                            Qt::Orientation orientation,
                            const QVariant& value,
                            int role)
{
    if (role != Qt::EditRole)
        return false;

    if (orientation != Qt::Horizontal)
        return false;

    if (section < 0 || section >= m_headerData.size())
        return false;

    m_headerData[section] = value.toString();
    emit headerDataChanged(orientation, section, section);
    return true;
}

bool MyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (count <= 0)
        return false;

    // Нормализуем позицию вставки
    if (row < 0)
        row = 0;
    if (row > m_vector.size())
        row = m_vector.size();

    beginInsertRows(QModelIndex(), row, row + count - 1);

    // Вставляем count "пустых" элементов (MyRect по умолчанию)
    for (int i = 0; i < count; ++i)
        m_vector.insert(row, MyRect{});

    endInsertRows();
    return true;
}

Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void MyModel::slotAddData(const MyRect& rect)
{
    const int row = m_vector.size();

    // 1) добавляем одну строку в конец
    if (!insertRows(row, 1))
        return;

    // 2) заполняем данными
    m_vector[row] = rect;

    // 3) уведомляем представления, что данные в строке обновились
    const QModelIndex leftTop = index(row, 0);
    const QModelIndex rightBottom = index(row, columnCountValue() - 1);
    emit dataChanged(leftTop, rightBottom);
}

void MyModel::test()
{
    slotAddData(MyRect(QColor(Qt::red),   Qt::SolidLine, 2, 100, 100, 100, 100));
    slotAddData(MyRect(QColor(Qt::green), Qt::DotLine,   3,  10,  10, 100, 200));
    // Можно добавить ещё данных при желании
}
