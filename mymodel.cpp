#include "mymodel.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QtGlobal>

/**
 * @brief Преобразует значение Qt::PenStyle в человекочитаемую строку.
 *
 * @details
 * В таблице (QTableView) для роли Qt::DisplayRole мы хотим показывать пользователю
 * не числовое значение перечисления, а понятный текст вида "Qt::DotLine".
 *
 * При этом для редактирования (Qt::EditRole) мы продолжаем использовать int,
 * потому что:
 *  - QVariant не обязан “прозрачно” хранить Qt::PenStyle как отдельный тип,
 *  - наш делегат (QComboBox) опирается на userData/findData, где значения удобно хранить как int.
 *
 * @param style Значение стиля пера.
 * @return Строковое представление стиля:
 *         - для известных значений: строгое имя перечисления ("Qt::SolidLine" и т.п.);
 *         - для неизвестных/неожиданных: "Qt::PenStyle(<число>)".
 *
 * @note
 * Это **только** представление для отображения (DisplayRole). Для хранения/редактирования
 * используется исходное значение (EditRole).
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
        // На случай расширений/нестандартных значений — безопасный fallback.
        return QString("Qt::PenStyle(%1)").arg(static_cast<int>(style));
    }
}

/**
 * @brief Конструктор табличной модели MyModel.
 *
 * @details
 * 1) Инициализирует список заголовков столбцов @ref m_headerData.
 * 2) Проверяет, что число заголовков совпадает с количеством столбцов,
 *    определённым перечислением Column.
 *
 * @param parent Родительский QObject.
 *
 * @note
 * Проверка через Q_ASSERT — отладочная (работает в debug-сборках). Она помогает
 * быстро обнаружить рассогласование при изменениях Column/заголовков.
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
 * @brief Возвращает количество строк модели.
 *
 * @details
 * Для QAbstractTableModel мы используем “плоскую” таблицу без иерархии.
 * Поэтому при наличии валидного parent (т.е. когда View спрашивает о “дочерних”
 * элементах) возвращаем 0.
 *
 * @param parent Родительский индекс (должен быть невалидным для таблицы).
 * @return Количество элементов в @ref m_vector.
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
 * @details
 * Аналогично rowCount(): модель не имеет иерархии, поэтому при валидном parent
 * возвращаем 0.
 *
 * @param parent Родительский индекс (должен быть невалидным для таблицы).
 * @return Число столбцов, определяемое Column::Count.
 */
int MyModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return columnCountValue();
}

/**
 * @brief Возвращает данные ячейки по индексу и роли (основной “API” модели для View).
 *
 * @details
 * QTableView (и делегаты) многократно вызывают data() для разных ролей.
 * В рамках архитектуры Model/View Qt одна и та же ячейка может запрашиваться
 * в разных “представлениях”:
 *
 * - Qt::DisplayRole: то, что показываем пользователю (текст/числа).
 * - Qt::EditRole: то, что отдаём редактору/делегату при редактировании.
 * - Qt::DecorationRole: иконка/пиктограмма (например, для отображения цвета).
 *
 * В нашем приложении:
 * - Для PenColor:
 *   - DisplayRole: строка "#RRGGBB" (QColor::name()) — удобно видеть и копировать.
 *   - EditRole: QColor — удобно для QColorDialog в делегате.
 *   - DecorationRole: QIcon (32x32) закрашенный нужным цветом.
 *
 * - Для PenStyle:
 *   - DisplayRole: строка "Qt::DotLine" и т.п. (через penStyleToString()).
 *   - EditRole: int(Qt::PenStyle), потому что ComboBox хранит userData как int.
 *
 * - Для остальных числовых столбцов:
 *   - DisplayRole/EditRole: int.
 *
 * @param index Модельный индекс (строка/столбец).
 * @param role Роль данных.
 * @return QVariant с данными для указанного role или пустой QVariant ({}),
 *         если индекс некорректен или роль не поддерживается.
 *
 * @note
 * Важно различать DisplayRole и EditRole: делегат должен работать именно с EditRole,
 * иначе он может получить “красивую строку”, а не машинно-удобное значение.
 */
QVariant MyModel::data(const QModelIndex& index, int role) const
{
    // 1) Проверка индекса: View иногда запрашивает данные для невалидных индексов.
    if (!index.isValid())
        return {};

    // 2) Координаты ячейки.
    const int row = index.row();
    const int col = index.column();

    // 3) Проверка границ.
    if (row < 0 || row >= m_vector.size())
        return {};
    if (col < 0 || col >= columnCountValue())
        return {};

    // 4) Достаём объект строки и определяем столбец.
    const MyRect& r = m_vector[row];
    const Column column = static_cast<Column>(col);

    /**
     * @par Qt::EditRole
     * Значения, которые ожидают редакторы/делегаты.
     * Здесь мы намеренно возвращаем “удобные для редактирования” типы.
     */
    if (role == Qt::EditRole)
    {
        switch (column)
        {
        case Column::PenColor:  return r.penColor;                  // QColor
        case Column::PenStyle:  return static_cast<int>(r.penStyle);// int
        case Column::PenWidth:  return r.penWidth;
        case Column::Left:      return r.left;
        case Column::Top:       return r.top;
        case Column::Width:     return r.width;
        case Column::Height:    return r.height;
        case Column::Count:     break;
        }
        return {};
    }

    /**
     * @par Qt::DisplayRole
     * Значения, отображаемые пользователю.
     * Здесь мы делаем вывод “красивым”: стиль пера строкой, цвет — текст + иконка.
     */
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

    /**
     * @par Qt::DecorationRole
     * Декорация (иконки). В нашем случае это пиктограмма цвета.
     *
     * QTableView при отрисовке ячейки может дополнительно запросить DecorationRole.
     * Мы отдаём QIcon на основе QPixmap 32x32, заполненного цветом пера.
     */
    if (role == Qt::DecorationRole && column == Column::PenColor)
    {
        QPixmap px(32, 32);
        px.fill(r.penColor);
        return QIcon(px);
    }

    // Прочие роли не поддерживаем.
    return {};
}

/**
 * @brief Устанавливает данные для ячейки модели (обратный путь редактирования).
 *
 * @details
 * Когда пользователь редактирует ячейку, делегат (или стандартный редактор Qt)
 * в конечном итоге вызывает model->setData(index, value, role).
 *
 * Мы обрабатываем **только** Qt::EditRole.
 *
 * Алгоритм:
 * 1) проверить валидность индекса и границы;
 * 2) преобразовать QVariant к нужному типу в зависимости от столбца;
 * 3) сохранить значение в @ref m_vector;
 * 4) уведомить View сигналом dataChanged(index, index).
 *
 * @param index Индекс ячейки.
 * @param value Новое значение.
 * @param role Роль (должна быть Qt::EditRole).
 * @return true если значение принято (даже если оно совпало), иначе false.
 *
 * @note
 * Мы используем флаг @c changed, чтобы не посылать dataChanged() зря.
 * Это экономит перерисовку и “дребезг” сигналов при повторной установке того же значения.
 */
bool MyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    // Меняем данные только при редактировании.
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

    bool changed = false;

    switch (column)
    {
    case Column::PenColor:
    {
        /**
         * Делегат цвета пишет QColor напрямую.
         * Но если пользователь редактирует “вручную” (например, стандартным редактором),
         * сюда может прийти строка "#RRGGBB". Поддерживаем оба сценария.
         */
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
        /**
         * PenStyle редактируется как int, потому что:
         *  - ComboBox хранит userData = int(Qt::PenStyle),
         *  - setEditorData()/setModelData() работают через findData/currentData().
         */
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
        const int w = value.toInt();
        if (r.penWidth != w)
        {
            r.penWidth = w;
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

    // Если фактически ничего не изменилось — считаем операцию успешной.
    if (!changed)
        return true;

    /**
     * Уведомляем представления, что изменился один элемент.
     *
     * Важно: после dataChanged() View может заново запросить data() с разными ролями,
     * поэтому корректно обновятся и DisplayRole (текст), и DecorationRole (иконка).
     */
    emit dataChanged(index, index);
    return true;
}

/**
 * @brief Возвращает заголовок строки/столбца.
 *
 * @details
 * - Горизонтальные заголовки (столбцы) берутся из @ref m_headerData.
 * - Вертикальные заголовки (строки) — это номера строк (1..N).
 *
 * Заголовки запрашиваются View с ролью Qt::DisplayRole.
 *
 * @param section Номер секции (индекс столбца или строки).
 * @param orientation Ориентация (Qt::Horizontal / Qt::Vertical).
 * @param role Роль (используем только Qt::DisplayRole).
 * @return Заголовок или пустой QVariant, если секция вне диапазона/роль не та.
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
 * @brief Устанавливает заголовок столбца (опционально; в лабораторной может не требоваться).
 *
 * @details
 * Обрабатываем только:
 * - role == Qt::EditRole
 * - orientation == Qt::Horizontal
 *
 * При успешной установке генерируем headerDataChanged() для обновления View.
 *
 * @param section Номер столбца.
 * @param orientation Ориентация (должна быть Qt::Horizontal).
 * @param value Новый текст заголовка.
 * @param role Роль (должна быть Qt::EditRole).
 * @return true при успешной установке; иначе false.
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
 * @details
 * В табличных моделях Qt **обязательно** нужно оборачивать модификацию контейнера
 * вызовами beginInsertRows()/endInsertRows(). Это:
 *  - гарантирует целостность модели,
 *  - уведомляет все связанные представления об изменениях.
 *
 * Мы вставляем @p count “пустых” объектов MyRect (конструктор по умолчанию),
 * начиная с позиции @p row.
 *
 * @param row Позиция вставки (нормализуется в допустимый диапазон).
 * @param count Количество вставляемых строк (должно быть > 0).
 * @param parent Родительский индекс (для таблицы должен быть невалидным).
 * @return true при успешной вставке; иначе false.
 */
bool MyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (count <= 0)
        return false;

    // Нормализация позиции вставки.
    if (row < 0)
        row = 0;
    if (row > m_vector.size())
        row = m_vector.size();

    beginInsertRows(QModelIndex(), row, row + count - 1);

    // Вставляем count элементов MyRect{}.
    for (int i = 0; i < count; ++i)
        m_vector.insert(row, MyRect{});

    endInsertRows();
    return true;
}

/**
 * @brief Возвращает флаги поведения элемента (select/edit/enabled и т.п.).
 *
 * @details
 * Чтобы QTableView позволил редактирование, ячейка должна иметь флаг Qt::ItemIsEditable.
 *
 * Важно: флаги нужно брать из базового класса и дополнять, иначе легко получить
 * рекурсию при ошибочном вызове MyModel::flags() из MyModel::flags().
 *
 * @param index Индекс элемента.
 * @return Флаги базового класса + Qt::ItemIsEditable; либо Qt::NoItemFlags для невалидного индекса.
 */
Qt::ItemFlags MyModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

/**
 * @brief Добавляет одну строку данных в конец модели.
 *
 * @details
 * Используется как “удобный API” модели для заполнения извне (например, из UI или из файла).
 *
 * Алгоритм:
 * 1) добавить строку через insertRows();
 * 2) записать данные в соответствующий элемент m_vector;
 * 3) уведомить View о смене данных в диапазоне всей строки (чтобы обновились
 *    текст и иконка цвета).
 *
 * @param rect Прямоугольник для добавления.
 */
void MyModel::slotAddData(const MyRect& rect)
{
    const int row = m_vector.size();

    if (!insertRows(row, 1))
        return;

    m_vector[row] = rect;

    // Обновляем всю строку (включая иконку цвета).
    const QModelIndex leftTop = index(row, 0);
    const QModelIndex rightBottom = index(row, columnCountValue() - 1);
    emit dataChanged(leftTop, rightBottom);
}

/**
 * @brief Заполняет модель тестовыми данными.
 *
 * @details
 * Удобно вызывать из конструктора MainWindow для проверки:
 *  - отображения таблицы,
 *  - строкового вывода стиля пера (DisplayRole),
 *  - иконки цвета (DecorationRole),
 *  - редактирования (EditRole + setData()).
 */
void MyModel::test()
{
    slotAddData(MyRect(QColor(Qt::red),   Qt::SolidLine, 2, 100, 100, 100, 100));
    slotAddData(MyRect(QColor(Qt::green), Qt::DotLine,   3,  10,  10, 100, 200));
}
