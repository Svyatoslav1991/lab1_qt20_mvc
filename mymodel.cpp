// ======================= mymodel.cpp =======================
#include "mymodel.h"

#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QTextStream>
#include <QtGlobal>

/**
 * @brief Преобразует значение Qt::PenStyle в человекочитаемую строку.
 *
 * @details
 * Используется:
 * - для Qt::DisplayRole (чтобы в таблице не было “1/2/3”, а было "Qt::DotLine")
 * - при сохранении в файл (чтобы файл был читаемым)
 *
 * @param style Стиль пера.
 * @return Строка с именем перечисления или fallback-формат "Qt::PenStyle(<число>)".
 */
QString MyModel::penStyleToString(Qt::PenStyle style)
{
    switch (style)
    {
    case Qt::NoPen:          return "Qt::NoPen";
    case Qt::SolidLine:      return "Qt::SolidLine";
    case Qt::DashLine:       return "Qt::DashLine";
    case Qt::DotLine:        return "Qt::DotLine";
    case Qt::DashDotLine:    return "Qt::DashDotLine";
    case Qt::DashDotDotLine: return "Qt::DashDotDotLine";
    default:
        return QString("Qt::PenStyle(%1)").arg(static_cast<int>(style));
    }
}

/**
 * @brief Парсит Qt::PenStyle из строки.
 *
 * @details
 * Поддерживаем несколько форматов:
 * 1) чистое число (например "3")
 * 2) "Qt::PenStyle(3)"
 * 3) имя перечисления (например "Qt::DotLine")
 *
 * Это повышает устойчивость загрузки: файл может быть сохранён в разных вариантах
 * (или пользователь мог вручную поправить строку).
 *
 * @param s Строка.
 * @param ok Опционально: флаг успешности парсинга.
 * @return Значение Qt::PenStyle (если ok=false — возвращается Qt::SolidLine как fallback).
 */
Qt::PenStyle MyModel::penStyleFromString(const QString& s, bool* ok)
{
    const QString t = s.trimmed();

    // (1) Число
    bool numOk = false;
    const int asInt = t.toInt(&numOk);
    if (numOk)
    {
        if (ok) *ok = true;
        return static_cast<Qt::PenStyle>(asInt);
    }

    // (2) "Qt::PenStyle(3)"
    if (t.startsWith("Qt::PenStyle(") && t.endsWith(')'))
    {
        const int prefixLen = QString("Qt::PenStyle(").size();
        const QString inside = t.mid(prefixLen, t.size() - prefixLen - 1);
        const int v = inside.toInt(&numOk);
        if (numOk)
        {
            if (ok) *ok = true;
            return static_cast<Qt::PenStyle>(v);
        }
    }

    // (3) Имя перечисления
    struct MapItem { const char* name; Qt::PenStyle style; };
    static const MapItem kMap[] = {
        {"Qt::NoPen",          Qt::NoPen},
        {"Qt::SolidLine",      Qt::SolidLine},
        {"Qt::DashLine",       Qt::DashLine},
        {"Qt::DotLine",        Qt::DotLine},
        {"Qt::DashDotLine",    Qt::DashDotLine},
        {"Qt::DashDotDotLine", Qt::DashDotDotLine},
    };

    for (const auto& it : kMap)
    {
        if (t == QLatin1String(it.name))
        {
            if (ok) *ok = true;
            return it.style;
        }
    }

    if (ok) *ok = false;
    return Qt::SolidLine; // fallback
}

/**
 * @brief Конструктор табличной модели MyModel.
 *
 * @details
 * Заполняем заголовки в строгом соответствии с enum Column.
 * Q_ASSERT помогает поймать расхождения при рефакторинге.
 */
MyModel::MyModel(QObject* parent)
    : QAbstractTableModel(parent)
{
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
 * @brief Количество строк модели.
 */
int MyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

/**
 * @brief Количество столбцов модели.
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
 * @details
 * См. документацию класса: DisplayRole / EditRole / DecorationRole.
 */
QVariant MyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_items.size())
        return {};
    if (col < 0 || col >= columnCountValue())
        return {};

    const MyRect& r = m_items[row];
    const Column column = static_cast<Column>(col);

    // --- EditRole: удобные для редактирования типы ---
    if (role == Qt::EditRole)
    {
        switch (column)
        {
        case Column::PenColor:  return r.penColor;                   // QColor
        case Column::PenStyle:  return static_cast<int>(r.penStyle); // int
        case Column::PenWidth:  return r.penWidth;
        case Column::Left:      return r.left;
        case Column::Top:       return r.top;
        case Column::Width:     return r.width;
        case Column::Height:    return r.height;
        case Column::Count:     break;
        }
        return {};
    }

    // --- DisplayRole: “красивое” отображение ---
    if (role == Qt::DisplayRole)
    {
        switch (column)
        {
        case Column::PenColor:  return r.penColor.name();            // "#RRGGBB"
        case Column::PenStyle:  return penStyleToString(r.penStyle); // "Qt::DotLine"
        case Column::PenWidth:  return r.penWidth;
        case Column::Left:      return r.left;
        case Column::Top:       return r.top;
        case Column::Width:     return r.width;
        case Column::Height:    return r.height;
        case Column::Count:     break;
        }
        return {};
    }

    // --- DecorationRole: иконка цвета для PenColor ---
    if (role == Qt::DecorationRole && column == Column::PenColor)
    {
        QPixmap px(32, 32);
        px.fill(r.penColor);
        return QIcon(px);
    }

    return {};
}

/**
 * @brief Устанавливает данные (редактирование).
 *
 * @details
 * Принимаем изменения только по Qt::EditRole.
 * После фактического изменения эмитим dataChanged(index,index),
 * чтобы View перерисовал ячейку и (при необходимости) иконку.
 */
bool MyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_items.size())
        return false;
    if (col < 0 || col >= columnCountValue())
        return false;

    MyRect& r = m_items[row];
    const Column column = static_cast<Column>(col);

    bool changed = false;

    switch (column)
    {
    case Column::PenColor:
    {
        // Поддерживаем QColor (нормально для делегата) и строку "#RRGGBB" (если ввод руками).
        QColor c;
        if (value.canConvert<QColor>())
            c = value.value<QColor>();
        else
            c = QColor(value.toString().trimmed());

        if (!c.isValid())
            return false;

        if (r.penColor != c)
        {
            r.penColor = c;
            changed = true;
        }
        break;
    }
    case Column::PenStyle:
    {
        // Ожидаем int, который интерпретируем как Qt::PenStyle.
        const int styleInt = value.toInt();
        const Qt::PenStyle style = static_cast<Qt::PenStyle>(styleInt);

        if (r.penStyle != style)
        {
            r.penStyle = style;
            changed = true;
        }
        break;
    }
    case Column::PenWidth:
    {
        const int v = value.toInt();
        if (r.penWidth != v)
        {
            r.penWidth = v;
            changed = true;
        }
        break;
    }
    case Column::Left:
    {
        const int v = value.toInt();
        if (r.left != v)
        {
            r.left = v;
            changed = true;
        }
        break;
    }
    case Column::Top:
    {
        const int v = value.toInt();
        if (r.top != v)
        {
            r.top = v;
            changed = true;
        }
        break;
    }
    case Column::Width:
    {
        const int v = value.toInt();
        if (r.width != v)
        {
            r.width = v;
            changed = true;
        }
        break;
    }
    case Column::Height:
    {
        const int v = value.toInt();
        if (r.height != v)
        {
            r.height = v;
            changed = true;
        }
        break;
    }
    case Column::Count:
        return false;
    }

    if (!changed)
        return true;

    emit dataChanged(index, index);
    return true;
}

/**
 * @brief Заголовки строк/столбцов.
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

    // Вертикальные заголовки: номера строк 1..N
    return section + 1;
}

/**
 * @brief Изменение заголовков.
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
 * @brief Вставка строк в модель.
 */
bool MyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (count <= 0)
        return false;

    if (row < 0)
        row = 0;
    if (row > m_items.size())
        row = m_items.size();

    beginInsertRows(QModelIndex(), row, row + count - 1);

    for (int i = 0; i < count; ++i)
        m_items.insert(row, MyRect{});

    endInsertRows();
    return true;
}

/**
 * @brief Флаги элемента.
 */
Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

/**
 * @brief Добавление строки в конец модели.
 */
void MyModel::slotAddData(const MyRect& rect)
{
    const int row = m_items.size();

    if (!insertRows(row, 1))
        return;

    m_items[row] = rect;

    const QModelIndex leftTop = index(row, 0);
    const QModelIndex rightBottom = index(row, columnCountValue() - 1);
    emit dataChanged(leftTop, rightBottom);
}

/**
 * @brief Тестовое заполнение.
 */
void MyModel::test()
{
    slotAddData(MyRect(QColor(Qt::red),   Qt::SolidLine, 2, 100, 100, 100, 100));
    slotAddData(MyRect(QColor(Qt::green), Qt::DotLine,   3,  10,  10, 100, 200));
}

/**
 * @brief Сохранение данных модели в TSV.
 *
 * @details
 * Каждая строка файла:
 * PenColor \t PenStyle \t PenWidth \t Left \t Top \t Width \t Height
 *
 * PenColor сохраняем как "#RRGGBB" (QColor::name()).
 * PenStyle сохраняем как читаемое имя "Qt::DotLine" (penStyleToString()).
 */
bool MyModel::saveToTsv(const QString& fileName, QString* error) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }

    QTextStream stream(&file);

    for (const MyRect& r : m_items)
    {
        QStringList fields;
        fields
            << r.penColor.name()
            << penStyleToString(r.penStyle)
            << QString::number(r.penWidth)
            << QString::number(r.left)
            << QString::number(r.top)
            << QString::number(r.width)
            << QString::number(r.height);

        stream << fields.join('\t') << '\n';
    }

    return true;
}

/**
 * @brief Загрузка данных модели из TSV.
 *
 * @details
 * - Читаем файл построчно.
 * - Каждая непустая строка разбивается по '\t'.
 * - Ожидаем ровно 7 полей (Column::Count без Count).
 * - Если встречается ошибка формата, возвращаем false и НЕ меняем текущую модель.
 * - При успешной загрузке заменяем данные целиком через beginResetModel()/endResetModel().
 */
bool MyModel::loadFromTsv(const QString& fileName, QString* error)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }

    QTextStream stream(&file);

    QVector<MyRect> tmp;
    int lineNo = 0;

    while (!stream.atEnd())
    {
        const QString line = stream.readLine();
        ++lineNo;

        if (line.trimmed().isEmpty())
            continue;

        const QStringList parts = line.split('\t', Qt::KeepEmptyParts);
        if (parts.size() != columnCountValue())
        {
            if (error)
            {
                *error = QString("Line %1: expected %2 fields, got %3")
                             .arg(lineNo)
                             .arg(columnCountValue())
                             .arg(parts.size());
            }
            return false;
        }

        // PenColor: "#RRGGBB"
        const QColor color(parts[0].trimmed());
        if (!color.isValid())
        {
            if (error) *error = QString("Line %1: invalid color '%2'").arg(lineNo).arg(parts[0]);
            return false;
        }

        // PenStyle: "Qt::DotLine" / "3" / "Qt::PenStyle(3)"
        bool styleOk = false;
        const Qt::PenStyle style = penStyleFromString(parts[1], &styleOk);
        if (!styleOk)
        {
            if (error) *error = QString("Line %1: invalid pen style '%2'").arg(lineNo).arg(parts[1]);
            return false;
        }

        auto parseInt = [&](const QString& s, const char* fieldName, int& out) -> bool
        {
            bool ok = false;
            out = s.trimmed().toInt(&ok);
            if (!ok)
            {
                if (error)
                {
                    *error = QString("Line %1: invalid %2 '%3'")
                                 .arg(lineNo)
                                 .arg(fieldName)
                                 .arg(s);
                }
                return false;
            }
            return true;
        };

        int penWidth = 0, left = 0, top = 0, width = 0, height = 0;
        if (!parseInt(parts[2], "PenWidth", penWidth)) return false;
        if (!parseInt(parts[3], "Left",     left))     return false;
        if (!parseInt(parts[4], "Top",      top))      return false;
        if (!parseInt(parts[5], "Width",    width))    return false;
        if (!parseInt(parts[6], "Height",   height))   return false;

        tmp.push_back(MyRect(color, style, penWidth, left, top, width, height));
    }

    beginResetModel();
    m_items = std::move(tmp);
    endResetModel();

    return true;
}
