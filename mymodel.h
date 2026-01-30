#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

#include "MyRect.h"

/**
 * @brief Табличная модель для хранения и отображения примитивов MyRect.
 *
 * Архитектура Qt Model/View:
 *  - модель хранит данные (m_vector),
 *  - представление (QTableView) отображает их и запрашивает через методы модели,
 *  - делегат (QStyledItemDelegate / MyDelegate) управляет редактированием и отрисовкой редакторов.
 *
 * В данной лабораторной:
 *  - Каждая строка соответствует одному MyRect.
 *  - Каждый столбец соответствует одному полю MyRect (атрибуты пера + геометрия).
 *
 * Поддерживается редактирование:
 *  - flags() добавляет Qt::ItemIsEditable,
 *  - setData() сохраняет значения в m_vector и генерирует dataChanged().
 *
 * Совместимость с MyDelegate:
 *  - PenStyle редактируется как int(Qt::PenStyle) через Qt::EditRole,
 *  - PenColor редактируется как QColor через Qt::EditRole (двойной клик → QColorDialog).
 */
class MyModel final : public QAbstractTableModel
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор.
     * @param parent Родительский объект.
     *
     * Формирует горизонтальные заголовки столбцов (m_headerData).
     * Порядок заголовков соответствует порядку значений enum Column.
     */
    explicit MyModel(QObject* parent = nullptr);

    /**
     * @brief Количество строк.
     * @param parent Для табличной модели дочерних индексов нет; если parent валиден, возвращаем 0.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Количество столбцов.
     * @param parent Для табличной модели дочерних индексов нет; если parent валиден, возвращаем 0.
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные ячейки по индексу и роли.
     *
     * Роли:
     *  - Qt::DisplayRole: данные для отображения пользователю
     *      - PenColor: строка "#RRGGBB"
     *      - PenStyle: строка "Qt::SolidLine" и т.п. (чтобы не видеть числа)
     *      - остальные: int
     *
     *  - Qt::EditRole: данные для редакторов/делегатов
     *      - PenColor: QColor (чтобы делегат мог корректно открыть QColorDialog)
     *      - PenStyle: int(Qt::PenStyle) (чтобы ComboBox работал через userData/findData)
     *      - остальные: int
     *
     *  - Qt::DecorationRole:
     *      - PenColor: иконка (QIcon) с заливкой цветом пера
     *
     * @param index Индекс ячейки.
     * @param role Роль данных.
     * @return QVariant с данными или пустой QVariant, если индекс/роль не поддержаны.
     */
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает данные в модели (используется при редактировании).
     *
     * Обрабатывается только Qt::EditRole.
     * Сохраняет значение в m_vector и уведомляет представления через dataChanged().
     *
     * Важные случаи:
     *  - PenColor: принимает QColor (от делегата) или строку (если редактировать вручную)
     *  - PenStyle: принимает int (Qt::PenStyle)
     *
     * @param index Индекс ячейки.
     * @param value Новое значение.
     * @param role Роль (должна быть Qt::EditRole).
     * @return true, если данные успешно применены.
     */
    bool setData(const QModelIndex& index,
                 const QVariant& value,
                 int role = Qt::EditRole) override;

    /**
     * @brief Возвращает заголовки строк/столбцов.
     *
     * Горизонтальные заголовки берутся из m_headerData.
     * Вертикальные заголовки — номера строк 1..N.
     *
     * @param section Номер секции.
     * @param orientation Ориентация заголовка.
     * @param role Должна быть Qt::DisplayRole.
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает заголовок столбца (горизонтальный).
     *
     * @param section Номер столбца.
     * @param orientation Должна быть Qt::Horizontal.
     * @param value Новое имя заголовка.
     * @param role Должна быть Qt::EditRole.
     */
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant& value,
                       int role = Qt::EditRole) override;

    /**
     * @brief Вставка строк в модель.
     *
     * Вставляет count элементов MyRect{} начиная с row.
     * Обязательно использует beginInsertRows/endInsertRows для корректного обновления View.
     */
    bool insertRows(int row,
                    int count,
                    const QModelIndex& parent = QModelIndex()) override;

    /**
     * @brief Флаги элемента.
     *
     * Добавляет Qt::ItemIsEditable к флагам базового класса,
     * чтобы разрешить редактирование всех ячеек.
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * @brief Тестовое заполнение модели (для отладки).
     */
    void test();

public slots:
    /**
     * @brief Добавляет одну строку (rect) в конец модели.
     *
     * Алгоритм:
     * 1) insertRows(rowCount(), 1)
     * 2) записываем rect в m_vector
     * 3) dataChanged по диапазону строки для обновления отображения
     */
    void slotAddData(const MyRect& rect);

private:
    /**
     * @brief Номера столбцов модели.
     *
     * @note Порядок значений должен соответствовать:
     *  - заголовкам m_headerData
     *  - логике в data()/setData()
     *  - константам в MyDelegate (kPenColorColumn=0, kPenStyleColumn=1)
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
     * @brief Преобразует Qt::PenStyle в человекочитаемую строку.
     * @param style Значение Qt::PenStyle.
     * @return Строка вида "Qt::SolidLine".
     */
    static QString penStyleToString(Qt::PenStyle style);

private:
    QVector<MyRect> m_vector;     ///< Данные модели (строка таблицы = один MyRect).
    QStringList     m_headerData; ///< Заголовки столбцов (горизонтальные).
};

#endif // MYMODEL_H
