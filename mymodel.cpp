#include "mymodel.h"

#include <QPixmap>
#include <QtGlobal>

/**
 * @brief Конструктор модели.
 *
 * Заполняет список заголовков столбцов.
 * Дополнительно проверяет, что число заголовков соответствует числу столбцов.
 */
MyModel::MyModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_headerData
        << "Цвет"
        << "Стиль"
        << "Толщина"
        << "Left"
        << "Top"
        << "Ширина"
        << "Высота";

    Q_ASSERT(m_headerData.size() == columnCountValue());
}

/**
 * @brief Количество строк (прямоугольников) в модели.
 *
 * Для табличной модели родительский индекс не используется.
 */
int MyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_vector.size();
}

/**
 * @brief Количество столбцов в модели.
 *
 * Для табличной модели родительский индекс не используется.
 */
int MyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return columnCountValue();
}

/**
 * @brief Возврат данных по индексу и роли.
 *
 * Qt::DisplayRole:
 *  - Возвращает значения полей MyRect (цвет — как строка "#RRGGBB").
 * Qt::DecorationRole:
 *  - Для столбца PenColor возвращает цветную пиктограмму 16x16.
 */
QVariant MyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_vector.size())
        return {};

    if (col < 0 || col >= columnCountValue())
        return {};

    const MyRect& r = m_vector[row];
    const Column column = static_cast<Column>(col);

    if (role == Qt::DisplayRole)
    {
        switch (column)
        {
        case Column::PenColor:
            return r.penColor.name(); // "#RRGGBB"
        case Column::PenStyle:
            // Сейчас возвращаем числовое значение.
            // Позже можно вернуть строковое имя стиля или использовать делегат.
            return static_cast<int>(r.penStyle);
        case Column::PenWidth:
            return r.penWidth;
        case Column::Left:
            return r.left;
        case Column::Top:
            return r.top;
        case Column::Width:
            return r.width;
        case Column::Height:
            return r.height;
        case Column::Count:
            break;
        }
        return {};
    }

    if (role == Qt::DecorationRole && column == Column::PenColor)
    {
        QPixmap pix(16, 16);
        pix.fill(r.penColor);
        return pix;
    }

    return {};
}

/**
 * @brief Заголовки строк/столбцов.
 *
 * Горизонтальные: m_headerData[section]
 * Вертикальные: номера строк 1..N
 */
QVariant MyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal)
    {
        if (section >= 0 && section < m_headerData.size())
            return m_headerData[section];
        return {};
    }

    return section + 1;
}

/**
 * @brief Установка заголовка столбца.
 *
 * Разрешено только для orientation == Qt::Horizontal и role == Qt::EditRole.
 * При изменении заголовка уведомляет представления сигналом headerDataChanged().
 */
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

/**
 * @brief Вставка новых строк в модель.
 *
 * Вставляет count строк начиная с позиции row.
 * Для корректного обновления связанных представлений обязательно вызывает
 * beginInsertRows() / endInsertRows().
 *
 * Новые элементы инициализируются значениями MyRect по умолчанию.
 */
bool MyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (count <= 0)
        return false;

    if (row < 0)
        row = 0;
    if (row > m_vector.size())
        row = m_vector.size();

    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        m_vector.insert(row, MyRect{});
    }
    endInsertRows();

    return true;
}

/**
 * @brief Флаги элемента.
 *
 * Сейчас элементы доступны и выбираемы.
 * Для редактирования в таблице позже можно добавить Qt::ItemIsEditable.
 */
Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
