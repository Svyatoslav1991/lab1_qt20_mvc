// ======================= mymodel.cpp =======================
#include "mymodel.h"

#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QTextStream>
#include <QIODevice>
#include <QtGlobal>

// -------------------- helpers: PenStyle <-> string --------------------

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

Qt::PenStyle MyModel::penStyleFromString(const QString& s, bool* ok)
{
    const QString t = s.trimmed();

    // (1) число
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

    // (3) имена
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
    return Qt::SolidLine;
}

QVector<int> MyModel::changedRolesForColumn(Column c)
{
    if (c == Column::PenColor)
        return { Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole };

    return { Qt::DisplayRole, Qt::EditRole };
}

// -------------------- ctor / basic --------------------

MyModel::MyModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    static_assert(kColumns.size() == kColCount, "kColumns must match Column::Count");
}

int MyModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

int MyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return kColCountInt;
}

QVariant MyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal)
    {
        if (section < 0 || section >= kColCountInt)
            return {};
        return QLatin1String(kColumns[static_cast<std::size_t>(section)].header);
    }

    return section + 1;
}

bool MyModel::setHeaderData(int, Qt::Orientation, const QVariant&, int)
{
    // Заголовки фиксированы (kColumns).
    return false;
}

Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool MyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid() || count <= 0)
        return false;

    if (row < 0) row = 0;
    if (row > m_items.size()) row = m_items.size();

    beginInsertRows(QModelIndex(), row, row + count - 1);
    m_items.insert(row, count, MyRect{});
    endInsertRows();

    return true;
}

// -------------------- data / setData --------------------

QVariant MyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_items.size())
        return {};
    if (col < 0 || col >= kColCountInt)
        return {};

    const MyRect& r = m_items[row];
    const Column column = kColumns[static_cast<std::size_t>(col)].col;

    if (role == Qt::EditRole)
    {
        switch (column)
        {
        case Column::PenColor:  return r.penColor;
        case Column::PenStyle:  return static_cast<int>(r.penStyle);
        case Column::PenWidth:  return r.penWidth;
        case Column::Left:      return r.left;
        case Column::Top:       return r.top;
        case Column::Width:     return r.width;
        case Column::Height:    return r.height;
        case Column::Count:     break;
        }
        return {};
    }

    if (role == Qt::DisplayRole)
    {
        switch (column)
        {
        case Column::PenColor:  return r.penColor.name();
        case Column::PenStyle:  return penStyleToString(r.penStyle);
        case Column::PenWidth:  return r.penWidth;
        case Column::Left:      return r.left;
        case Column::Top:       return r.top;
        case Column::Width:     return r.width;
        case Column::Height:    return r.height;
        case Column::Count:     break;
        }
        return {};
    }

    if (role == Qt::DecorationRole && column == Column::PenColor)
    {
        QPixmap px(32, 32);
        px.fill(r.penColor);
        return QIcon(px);
    }

    return {};
}

bool MyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= m_items.size())
        return false;
    if (col < 0 || col >= kColCountInt)
        return false;

    MyRect& r = m_items[row];
    const Column column = kColumns[static_cast<std::size_t>(col)].col;

    bool changed = false;

    switch (column)
    {
    case Column::PenColor:
    {
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
        const Qt::PenStyle style = static_cast<Qt::PenStyle>(value.toInt());
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

    emit dataChanged(index, index, changedRolesForColumn(column));
    return true;
}

// -------------------- slotAddData / test --------------------

void MyModel::slotAddData(const MyRect& rect)
{
    const int row = m_items.size();

    if (!insertRows(row, 1))
        return;

    m_items[row] = rect;

    const QModelIndex leftTop = index(row, 0);
    const QModelIndex rightBottom = index(row, kColCountInt - 1);

    emit dataChanged(leftTop, rightBottom,
                     {Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole});
}

void MyModel::test()
{
    slotAddData(MyRect(QColor(Qt::red),   Qt::SolidLine, 2, 100, 100, 100, 100));
    slotAddData(MyRect(QColor(Qt::green), Qt::DotLine,   3,  10,  10, 100, 200));
}

// -------------------- save/load wrappers by filename --------------------

bool MyModel::saveToTsv(const QString& fileName, QString* error) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }
    return saveToTsv(static_cast<QIODevice&>(file), error);
}

bool MyModel::loadFromTsv(const QString& fileName, QString* error)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }
    return loadFromTsv(static_cast<QIODevice&>(file), error);
}

// -------------------- save/load via QIODevice --------------------

bool MyModel::saveToTsv(QIODevice& out, QString* error) const
{
    if (!out.isOpen() || !(out.openMode() & QIODevice::WriteOnly))
    {
        if (error) *error = "Output device is not opened for writing";
        return false;
    }

    QTextStream stream(&out);

    for (const MyRect& r : m_items)
    {
        QStringList fields;
        fields.reserve(kColCountInt);

        fields << r.penColor.name();
        fields << penStyleToString(r.penStyle);
        fields << QString::number(r.penWidth);
        fields << QString::number(r.left);
        fields << QString::number(r.top);
        fields << QString::number(r.width);
        fields << QString::number(r.height);

        stream << fields.join('\t') << '\n';
    }

    if (stream.status() != QTextStream::Ok)
    {
        if (error) *error = "Failed to write TSV stream";
        return false;
    }

    return true;
}

bool MyModel::loadFromTsv(QIODevice& in, QString* error)
{
    if (!in.isOpen() || !(in.openMode() & QIODevice::ReadOnly))
    {
        if (error) *error = "Input device is not opened for reading";
        return false;
    }

    QTextStream stream(&in);

    QVector<MyRect> tmp;
    int lineNo = 0;

    while (!stream.atEnd())
    {
        const QString line = stream.readLine();
        ++lineNo;

        if (line.trimmed().isEmpty())
            continue;

        const QStringList parts = line.split('\t', Qt::KeepEmptyParts);
        if (parts.size() != kColCountInt)
        {
            if (error)
            {
                *error = QString("Line %1: expected %2 fields, got %3")
                             .arg(lineNo)
                             .arg(kColCountInt)
                             .arg(parts.size());
            }
            return false;
        }

        const QColor color(parts[0].trimmed());
        if (!color.isValid())
        {
            if (error) *error = QString("Line %1: invalid color '%2'").arg(lineNo).arg(parts[0]);
            return false;
        }

        bool styleOk = false;
        const Qt::PenStyle style = penStyleFromString(parts[1], &styleOk);
        if (!styleOk)
        {
            if (error) *error = QString("Line %1: invalid pen style '%2'").arg(lineNo).arg(parts[1]);
            return false;
        }

        auto parseInt = [&](const QString& s, const char* fieldName, int& outVal) -> bool
        {
            bool ok = false;
            outVal = s.trimmed().toInt(&ok);
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
