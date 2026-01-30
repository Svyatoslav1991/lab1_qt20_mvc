// ======================= mymodel.h =======================
#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>

#include <array>
#include <cstddef> // std::size_t

#include "myrect.h"

class QIODevice;

/**
 * @brief Табличная модель Qt (QAbstractTableModel) для хранения/редактирования списка MyRect.
 *
 * @details
 * # Назначение
 * MyModel — реализация интерфейса Qt Model/View для отображения и редактирования
 * коллекции объектов @ref MyRect в представлениях типа QTableView.
 *
 * Модель выступает “источником данных” и обеспечивает:
 * - количество строк/столбцов;
 * - получение значения ячейки по индексу и роли (data());
 * - установку значения ячейки (setData());
 * - заголовки столбцов/строк (headerData());
 * - флаги редактируемости (flags());
 * - корректные уведомления View через сигналы dataChanged и begin/end* методы.
 *
 * # Архитектурные решения
 * ## 1) Единый источник правды по колонкам
 * Для предотвращения рассинхронизации:
 * - enum Column (семантика),
 * - заголовки,
 * - порядок полей в TSV,
 * используется массив @ref kColumns.
 *
 * Таким образом:
 * - индекс столбца в таблице = индекс элемента в kColumns,
 * - столбец Column берётся из kColumns[col].col,
 * - заголовок столбца берётся из kColumns[col].header,
 * - порядок сохранения/загрузки TSV соответствует kColumns.
 *
 * ## 2) Разделение ролей Display/Edit/Decoration
 * В Qt принято:
 * - DisplayRole — удобочитаемое отображение (строки/числа),
 * - EditRole — “машинное” значение для редакторов (например QColor/int),
 * - DecorationRole — иконка/декорация.
 *
 * Это упрощает совместимость с делегатами:
 * - цвет редактируем как QColor (EditRole),
 * - стиль пера редактируем как int(Qt::PenStyle),
 * - для цвета дополнительно даём иконку (DecorationRole).
 *
 * ## 3) Сериализация TSV и тестируемость
 * Есть 2 группы методов:
 * - saveToTsv/loadFromTsv по имени файла (QString) — удобство для UI;
 * - saveToTsv/loadFromTsv через QIODevice — удобство для тестов (QBuffer).
 *
 * # Формат TSV
 * - Одна строка = один MyRect.
 * - Разделитель = '\t'.
 * - Количество полей = числу столбцов (kColCountInt).
 * - Порядок полей соответствует kColumns.
 *
 * Поля:
 * - PenColor  сохраняется как "#RRGGBB" (QColor::name()).
 * - PenStyle  сохраняется как "Qt::DotLine" и т.п.
 * - Остальные поля — целые числа.
 */
class MyModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели.
     *
     * @details
     * - Инициализирует базовый класс QAbstractTableModel.
     * - Проверяет на этапе компиляции (static_assert), что kColumns согласован с Column::Count.
     *
     * @param parent Родительский QObject (для управления временем жизни модели).
     */
    explicit MyModel(QObject* parent = nullptr);

    /**
     * @brief Возвращает количество строк.
     *
     * @details
     * - Если @p parent валиден (иерархические модели), возвращаем 0,
     *   потому что это плоская таблица.
     * - Иначе возвращаем количество элементов в контейнере @ref m_items.
     *
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return Число строк в модели.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает количество столбцов.
     *
     * @details
     * - Если @p parent валиден — возвращаем 0.
     * - Иначе — фиксированное число столбцов @ref kColCountInt.
     *
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return Число столбцов в модели.
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные ячейки по индексу и роли.
     *
     * @details
     * Поддерживаемые роли:
     * - Qt::EditRole:
     *   - PenColor -> QColor
     *   - PenStyle -> int(Qt::PenStyle)
     *   - остальные поля -> int
     * - Qt::DisplayRole:
     *   - PenColor -> "#RRGGBB"
     *   - PenStyle -> "Qt::DotLine" и т.п.
     *   - остальные поля -> int
     * - Qt::DecorationRole:
     *   - только для PenColor -> QIcon с заливкой текущим цветом
     *
     * Если индекс невалиден или вне диапазонов — возвращается пустой QVariant.
     *
     * @param index Индекс ячейки (row/column).
     * @param role Роль данных.
     * @return QVariant со значением для указанной роли.
     */
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает значение ячейки (редактирование).
     *
     * @details
     * Обрабатывается только Qt::EditRole.
     *
     * Правила по столбцам:
     * - PenColor:
     *   - принимает QColor или строку "#RRGGBB";
     *   - при невалидном цвете возвращает false.
     * - PenStyle:
     *   - принимает int, интерпретируемый как Qt::PenStyle.
     * - остальные поля:
     *   - принимают int (через QVariant::toInt()).
     *
     * При фактическом изменении значения:
     * - эмитится dataChanged(index, index, roles),
     *   где roles вычисляется через @ref changedRolesForColumn().
     *
     * Если новое значение совпадает со старым — возвращаем true без сигналов.
     *
     * @param index Индекс ячейки.
     * @param value Новое значение.
     * @param role Роль (должна быть Qt::EditRole).
     * @return true если значение принято (или совпало), false при ошибке/невалидных данных.
     */
    bool setData(const QModelIndex& index,
                 const QVariant& value,
                 int role = Qt::EditRole) override;

    /**
     * @brief Заголовки столбцов/строк.
     *
     * @details
     * - Для горизонтальных заголовков возвращается текст из @ref kColumns[].header.
     * - Для вертикальных — номера строк, начиная с 1.
     * - Поддерживается только Qt::DisplayRole.
     *
     * @param section Номер секции (столбца/строки).
     * @param orientation Ориентация (Horizontal/Vertical).
     * @param role Роль (ожидается Qt::DisplayRole).
     * @return Заголовок или пустой QVariant.
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /**
     * @brief Изменение заголовков.
     *
     * @details
     * В данной реализации заголовки фиксированы через @ref kColumns и не изменяются.
     * Поэтому функция всегда возвращает false.
     *
     * @return false всегда.
     */
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant& value,
                       int role = Qt::EditRole) override;

    /**
     * @brief Вставляет строки в модель.
     *
     * @details
     * - Модель плоская: если @p parent валиден — возвращаем false.
     * - Корректируем @p row в диапазон [0..m_items.size()].
     * - Выполняем beginInsertRows/endInsertRows.
     * - Добавляем @p count объектов MyRect{} в позицию row.
     *
     * @param row Позиция вставки.
     * @param count Количество строк для вставки (>0).
     * @param parent Родительский индекс (для таблицы должен быть невалидным).
     * @return true при успехе, false при ошибочных аргументах.
     */
    bool insertRows(int row,
                    int count,
                    const QModelIndex& parent = QModelIndex()) override;

    /**
     * @brief Возвращает флаги для элемента.
     *
     * @details
     * Все валидные элементы:
     * - доступны,
     * - выделяемы,
     * - редактируемы (Qt::ItemIsEditable).
     *
     * @param index Индекс ячейки.
     * @return Qt::ItemFlags.
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * @brief Заполняет модель тестовыми данными.
     *
     * @details
     * Используется для быстрой проверки:
     * - отображения таблицы,
     * - работы делегата,
     * - сериализации/десериализации.
     *
     * В реальном приложении можно убрать вызов test().
     */
    void test();

    /**
     * @brief Сохраняет модель в TSV по имени файла.
     *
     * @details
     * Открывает QFile на запись и делегирует в saveToTsv(QIODevice&).
     *
     * @param fileName Путь к файлу.
     * @param error Опционально: строка ошибки.
     * @return true при успехе, false при ошибке открытия/записи.
     */
    bool saveToTsv(const QString& fileName, QString* error = nullptr) const;

    /**
     * @brief Загружает модель из TSV по имени файла.
     *
     * @details
     * Открывает QFile на чтение и делегирует в loadFromTsv(QIODevice&).
     *
     * @param fileName Путь к файлу.
     * @param error Опционально: строка ошибки.
     * @return true при успехе, false при ошибке открытия/формата.
     */
    bool loadFromTsv(const QString& fileName, QString* error = nullptr);

    /**
     * @brief Сохраняет модель в TSV в абстрактное устройство вывода.
     *
     * @details
     * Устройство должно быть:
     * - открыто,
     * - иметь режим WriteOnly.
     *
     * Формирует TSV построчно (одна строка = один MyRect).
     *
     * @param out Устройство вывода (QFile/QBuffer/и т.п.).
     * @param error Опционально: строка ошибки.
     * @return true при успехе, false при ошибке состояния устройства/потока.
     */
    bool saveToTsv(QIODevice& out, QString* error = nullptr) const;

    /**
     * @brief Загружает модель из TSV из абстрактного устройства ввода.
     *
     * @details
     * Устройство должно быть:
     * - открыто,
     * - иметь режим ReadOnly.
     *
     * Алгоритм:
     * 1) читаем строки,
     * 2) парсим поля,
     * 3) при первой же ошибке возвращаем false и НЕ меняем текущую модель,
     * 4) при полном успехе делаем beginResetModel/endResetModel и заменяем m_items.
     *
     * @param in Устройство ввода (QFile/QBuffer/и т.п.).
     * @param error Опционально: строка ошибки.
     * @return true при успехе, false при ошибке формата/чтения.
     */
    bool loadFromTsv(QIODevice& in, QString* error = nullptr);

public slots:
    /**
     * @brief Добавляет одну строку в конец модели.
     *
     * @details
     * - Вставляет строку через insertRows(rowCount(), 1).
     * - Записывает значения в последнюю строку.
     * - Эмитит dataChanged по всей строке с основными ролями, чтобы обновились:
     *   - текст (DisplayRole),
     *   - данные редакторов (EditRole),
     *   - иконки (DecorationRole).
     *
     * @param rect Объект, который будет добавлен в модель.
     */
    void slotAddData(const MyRect& rect);

private:
    /**
     * @brief Семантические столбцы модели.
     *
     * @note Column::Count — служебный элемент “число столбцов”.
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
     * @brief Метаданные одного столбца: семантика и текст заголовка.
     */
    struct ColumnInfo
    {
        Column      col;     ///< Семантика столбца.
        const char* header;  ///< Текст заголовка (ASCII/Latin1).
    };

    /**
     * @brief Количество столбцов как std::size_t (нужно для std::array).
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
     *
     * @details
     * Индекс в массиве = индекс столбца в QTableView.
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
    /**
     * @brief Преобразует Qt::PenStyle в строку (для DisplayRole и TSV).
     */
    static QString penStyleToString(Qt::PenStyle style);

    /**
     * @brief Разбирает Qt::PenStyle из строки.
     *
     * @details
     * Поддерживаем форматы:
     * - "3"
     * - "Qt::PenStyle(3)"
     * - "Qt::DotLine"
     */
    static Qt::PenStyle penStyleFromString(const QString& s, bool* ok = nullptr);

    /**
     * @brief Возвращает набор ролей, которые надо указать в dataChanged для столбца.
     *
     * @details
     * В Qt5/Qt6 третьим параметром dataChanged является QVector<int>.
     */
    static QVector<int> changedRolesForColumn(Column c);

private:
    /**
     * @brief Контейнер данных модели.
     *
     * @details
     * Каждая строка таблицы соответствует одному элементу MyRect.
     */
    QVector<MyRect> m_items;
};

#endif // MYMODEL_H
