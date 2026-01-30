#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

#include "MyRect.h"

class QString;

/**
 * @brief Табличная модель для хранения и отображения примитивов MyRect.
 *
 * @details
 * ## Роль модели в архитектуре Qt Model/View
 * В Qt (Model/View) модель:
 *  - хранит данные (в нашем случае в контейнере @ref m_items),
 *  - предоставляет данные представлению (QTableView) через методы rowCount/columnCount/data,
 *  - принимает изменения от редакторов/делегатов через setData,
 *  - уведомляет представление о изменениях через сигналы dataChanged/rowsInserted/modelReset.
 *
 * ## Структура таблицы
 * - Одна строка = один объект MyRect.
 * - Столбцы соответствуют атрибутам пера и геометрии.
 *
 * ## Роли данных (roles)
 * Модель поддерживает ключевые роли:
 * - Qt::DisplayRole:
 *   - PenColor  -> "#RRGGBB" (читаемо)
 *   - PenStyle  -> "Qt::DotLine" и т.п. (читаемо)
 *   - остальное -> числа
 * - Qt::EditRole:
 *   - PenColor  -> QColor (для QColorDialog и корректного редактирования)
 *   - PenStyle  -> int (значение Qt::PenStyle), удобно для QComboBox userData/findData
 *   - остальное -> числа
 * - Qt::DecorationRole:
 *   - PenColor -> иконка цвета (QIcon), чтобы было видно цвет “квадратиком”
 *
 * ## Совместимость с MyDelegate
 * - MyDelegate::createEditor / setEditorData / setModelData используют Qt::EditRole.
 * - PenStyle: редактирование через QComboBox (int).
 * - PenColor: редактирование через QColorDialog (QColor).
 *
 * ## Сохранение/загрузка в TSV
 * Реализовано форматированное сохранение/чтение в текстовый файл.
 * - Разделитель полей: '\t'
 * - Одна строка файла соответствует одной строке модели (одному MyRect).
 * - Поля сохраняются в порядке столбцов (см. enum Column).
 */
class MyModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели.
     *
     * @details
     * - Инициализирует заголовки столбцов @ref m_headerData.
     * - Модель стартует пустой (без строк).
     *
     * @param parent Родительский QObject.
     */
    explicit MyModel(QObject* parent = nullptr);

    /**
     * @brief Количество строк модели.
     *
     * @details
     * Для QAbstractTableModel иерархия не используется, поэтому если @p parent валиден,
     * возвращаем 0.
     *
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return Число строк (элементов @ref m_items).
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Количество столбцов модели.
     *
     * @details
     * Для QAbstractTableModel иерархия не используется, поэтому если @p parent валиден,
     * возвращаем 0.
     *
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return Число столбцов (Column::Count).
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные ячейки по индексу и роли.
     *
     * @details
     * Поддерживаются роли:
     * - Qt::DisplayRole: данные для отображения (текст/числа)
     * - Qt::EditRole:   данные для редактирования (QColor, int(Qt::PenStyle), int...)
     * - Qt::DecorationRole: иконка для PenColor
     *
     * @param index Индекс ячейки.
     * @param role Роль данных (по умолчанию Qt::DisplayRole).
     * @return QVariant с данными или пустой QVariant при невалидном индексе/неподдержанной роли.
     */
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает данные в модели (редактирование).
     *
     * @details
     * Обрабатывается только роль Qt::EditRole.
     * В зависимости от столбца ожидаются разные типы:
     * - PenColor: QColor или строка "#RRGGBB"
     * - PenStyle: int (значение Qt::PenStyle)
     * - остальные: int
     *
     * При успешной установке генерируется dataChanged(index, index).
     *
     * @param index Индекс ячейки.
     * @param value Новое значение.
     * @param role Роль (должна быть Qt::EditRole).
     * @return true если значение принято (даже если совпало); иначе false.
     */
    bool setData(const QModelIndex& index,
                 const QVariant& value,
                 int role = Qt::EditRole) override;

    /**
     * @brief Возвращает заголовки строк/столбцов.
     *
     * @details
     * - Qt::Horizontal: берём из @ref m_headerData
     * - Qt::Vertical:   показываем номера строк (1..N)
     *
     * @param section Номер секции (столбца/строки).
     * @param orientation Ориентация заголовка.
     * @param role Роль (ожидается Qt::DisplayRole).
     * @return Заголовок или пустой QVariant, если не поддержано.
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает заголовок столбца (горизонтальный).
     *
     * @details
     * Поддерживается только:
     * - orientation == Qt::Horizontal
     * - role == Qt::EditRole
     *
     * После изменения генерируется headerDataChanged().
     *
     * @param section Номер столбца.
     * @param orientation Ориентация (должна быть Qt::Horizontal).
     * @param value Новый текст.
     * @param role Роль (должна быть Qt::EditRole).
     * @return true при успехе; иначе false.
     */
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant& value,
                       int role = Qt::EditRole) override;

    /**
     * @brief Вставляет строки в модель.
     *
     * @details
     * Вставка выполняется через beginInsertRows()/endInsertRows().
     * Вставляем @p count объектов MyRect{} начиная с @p row.
     *
     * @param row Позиция вставки.
     * @param count Количество вставляемых строк (>0).
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return true при успехе; иначе false.
     */
    bool insertRows(int row,
                    int count,
                    const QModelIndex& parent = QModelIndex()) override;

    /**
     * @brief Возвращает флаги элемента (selectable/editable/enabled).
     *
     * @details
     * Добавляем Qt::ItemIsEditable для всех валидных индексов,
     * чтобы QTableView разрешил редактирование.
     *
     * @param index Индекс элемента.
     * @return Флаги элемента.
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * @brief Тестовое заполнение модели.
     *
     * @details
     * Полезно для быстрой проверки:
     * - отображения таблицы,
     * - строкового вывода стилей,
     * - иконки цвета,
     * - редактирования.
     */
    void test();

    /**
     * @brief Сохранить модель в TSV-файл.
     *
     * @details
     * - Формат: текстовый файл, одна строка = один MyRect.
     * - Поля разделяются символом '\t' (tab).
     * - Порядок полей соответствует столбцам (см. enum Column).
     *
     * @param fileName Путь к файлу.
     * @param error Опционально: сюда кладём диагностическое сообщение при ошибке.
     * @return true при успешной записи; иначе false.
     */
    bool saveToTsv(const QString& fileName, QString* error = nullptr) const;

    /**
     * @brief Загрузить модель из TSV-файла (полная замена данных).
     *
     * @details
     * - Данные читаются в том же порядке, в котором были записаны.
     * - Пустые строки пропускаются.
     * - Если формат некорректен — загрузка отменяется и модель не изменяется.
     *
     * При успешной загрузке выполняется beginResetModel()/endResetModel().
     *
     * @param fileName Путь к файлу.
     * @param error Опционально: сюда кладём диагностическое сообщение при ошибке.
     * @return true при успехе; иначе false.
     */
    bool loadFromTsv(const QString& fileName, QString* error = nullptr);

public slots:
    /**
     * @brief Добавляет одну строку в конец модели.
     *
     * @details
     * Реализация через insertRows():
     * 1) insertRows(rowCount(), 1)
     * 2) присваиваем данные в последний элемент
     * 3) dataChanged по всей строке, чтобы обновились и текст, и иконки
     *
     * @param rect Объект, который добавляем.
     */
    void slotAddData(const MyRect& rect);

private:
    /**
     * @brief Перечисление столбцов модели.
     *
     * @details
     * Важно, чтобы:
     * - порядок здесь совпадал с @ref m_headerData,
     * - логика data()/setData() соответствовала этому порядку,
     * - константы в делегате совпадали по номерам (PenColor=0, PenStyle=1 и т.д.).
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

    static constexpr int toInt(Column c) noexcept { return static_cast<int>(c); }
    static constexpr int columnCountValue() noexcept { return toInt(Column::Count); }

    /**
     * @brief Преобразует Qt::PenStyle в читаемую строку (для DisplayRole / файла).
     * @param style Значение Qt::PenStyle.
     * @return Строка вида "Qt::SolidLine" или "Qt::PenStyle(<число>)".
     */
    static QString penStyleToString(Qt::PenStyle style);

    /**
     * @brief Разобрать Qt::PenStyle из строки.
     *
     * @details
     * Поддерживаем несколько форматов входа:
     * - "Qt::DotLine" (и другие имена)
     * - "3" (число)
     * - "Qt::PenStyle(3)"
     *
     * @param s Входная строка.
     * @param ok Опционально: сюда пишется результат парсинга.
     * @return Значение стиля (если ok=false — возвращается fallback).
     */
    static Qt::PenStyle penStyleFromString(const QString& s, bool* ok = nullptr);

private:
    QVector<MyRect> m_items;      ///< Контейнер данных: строка таблицы = один MyRect.
    QStringList     m_headerData; ///< Заголовки столбцов (горизонтальные).
};

#endif // MYMODEL_H
