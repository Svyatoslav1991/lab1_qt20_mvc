#include "mymodel.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QtGlobal>

/**
 * @brief Конструктор модели.
 *
 * Формирует заголовки столбцов (m_headerData) в порядке, соответствующем enum Column.
 * Проверяет согласованность количества заголовков с числом столбцов модели.
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

/**
 * @brief Возвращает количество строк модели.
 *
 * Для табличной модели дочерних элементов нет, поэтому при parent.isValid()
 * возвращаем 0.
 */
int MyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_vector.size();
}

/**
 * @brief Возвращает количество столбцов модели.
 *
 * Для табличной модели дочерних элементов нет, поэтому при parent.isValid()
 * возвращаем 0.
 */
int MyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return columnCountValue();
}

/**
 * @brief Возвращает данные по индексу и роли.
 *
 * Роли:
 * - Qt::DisplayRole / Qt::EditRole:
 *   - PenColor: строка "#RRGGBB"
 *   - PenStyle: int (static_cast<int>(Qt::PenStyle))
 *   - остальные параметры: int
 * - Qt::DecorationRole:
 *   - PenColor: QIcon с заливкой цветом пера (32x32)
 *
 * @param index Индекс ячейки (строка/столбец).
 * @param role Роль запрашиваемых данных.
 * @return Данные в виде QVariant или пустой QVariant при ошибке/неподдерживаемой роли.
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

    // Для редактирования обычно возвращаем то же значение, что и для отображения
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (column)
        {
        case Column::PenColor:
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

/**
 * @brief Устанавливает данные для ячейки (редактирование).
 *
 * Обрабатывается только роль Qt::EditRole.
 * Сохраняет значение в контейнер m_vector.
 * После изменения генерирует dataChanged(index, index) для обновления представлений.
 *
 * @param index Индекс ячейки.
 * @param value Новое значение (QVariant).
 * @param role Роль (должна быть Qt::EditRole).
 * @return true при успешном изменении данных, иначе false.
 */
bool MyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_vector.size())
        return false;
    if (col < 0 || col >= columnCountValue())
        return false;

    MyRect& r = m_vector[row];
    const Column column = static_cast<Column>(col);

    switch (column)
    {
    case Column::PenColor:
    {
        // Ожидаем строку "#RRGGBB" / "RRGGBB" / именованный цвет ("red") — QColor сам разбирает
        const QString s = value.toString().trimmed();
        const QColor c(s);
        if (!c.isValid())
            return false;

        r.penColor = c;
        break;
    }
    case Column::PenStyle:
    {
        // Пока редактируем через int (делегат по умолчанию даст SpinBox)
        const int styleInt = value.toInt();
        r.penStyle = static_cast<Qt::PenStyle>(styleInt);
        break;
    }
    case Column::PenWidth:
        r.penWidth = value.toInt();
        break;

    case Column::Left:
        r.left = value.toInt();
        break;
    case Column::Top:
        r.top = value.toInt();
        break;
    case Column::Width:
        r.width = value.toInt();
        break;
    case Column::Height:
        r.height = value.toInt();
        break;

    case Column::Count:
        return false;
    }

    // Изменился один элемент
    emit dataChanged(index, index);
    return true;
}

/**
 * @brief Возвращает заголовки строк/столбцов.
 *
 * - Для Qt::Horizontal: берётся строка из m_headerData по номеру секции.
 * - Для Qt::Vertical: возвращается номер строки в человекочитаемом виде (1..N).
 *
 * Заголовки предоставляются только для Qt::DisplayRole.
 */
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

/**
 * @brief Устанавливает заголовок столбца (только горизонтальный).
 *
 * Допускается только для:
 * - orientation == Qt::Horizontal
 * - role == Qt::EditRole
 *
 * При успешном изменении генерирует headerDataChanged().
 *
 * @return true, если заголовок изменён; иначе false.
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
 * @brief Вставляет строки в модель.
 *
 * Увеличивает контейнер данных m_vector на count элементов MyRect (по умолчанию),
 * начиная с позиции row.
 *
 * Важно: beginInsertRows()/endInsertRows() обязательны для сохранения целостности
 * модели и уведомления всех представлений.
 *
 * @param row Позиция вставки (нормализуется в диапазон [0..rowCount()]).
 * @param count Количество вставляемых строк (>0).
 * @param parent Родительский индекс (для таблицы должен быть невалидным).
 * @return true при успешной вставке, иначе false.
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

    // Вставляем count "пустых" элементов (MyRect по умолчанию)
    for (int i = 0; i < count; ++i)
        m_vector.insert(row, MyRect{});

    endInsertRows();
    return true;
}

/**
 * @brief Возвращает флаги элемента модели.
 *
 * Флаги описывают свойства элемента и определяют его поведение в View.
 * Для разрешения редактирования добавляем Qt::ItemIsEditable к флагам базового класса.
 *
 * Важно: флаги берём из QAbstractTableModel::flags(index), иначе возможна рекурсия.
 */
Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

/**
 * @brief Добавляет одну строку данных (rect) в конец модели.
 *
 * Алгоритм:
 * 1) insertRows(rowCount(), 1)
 * 2) запись rect в m_vector для добавленной строки
 * 3) dataChanged() для обновления отображения (в т.ч. иконки цвета)
 */
void MyModel::slotAddData(const MyRect& rect)
{
    const int row = m_vector.size();

    if (!insertRows(row, 1))
        return;

    m_vector[row] = rect;

    const QModelIndex leftTop = index(row, 0);
    const QModelIndex rightBottom = index(row, columnCountValue() - 1);
    emit dataChanged(leftTop, rightBottom);
}

/**
 * @brief Заполняет модель тестовыми данными.
 *
 * Используется для отладки корректности отображения (QTableView) и ролей
 * (DisplayRole/EditRole + DecorationRole для цвета).
 */
void MyModel::test()
{
    slotAddData(MyRect(QColor(Qt::red),   Qt::SolidLine, 2, 100, 100, 100, 100));
    slotAddData(MyRect(QColor(Qt::green), Qt::DotLine,   3,  10,  10, 100, 200));
    // Можно добавить ещё данных при желании
}
