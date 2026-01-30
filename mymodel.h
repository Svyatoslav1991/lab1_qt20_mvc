#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

#include "MyRect.h"

/**
 * @brief Табличная модель для хранения и отображения примитивов MyRect.
 *
 * Модель предназначена для использования с QTableView (паттерн Model/View).
 * Каждая строка таблицы соответствует одному объекту MyRect,
 * каждый столбец — одному параметру прямоугольника (атрибуты пера + геометрия).
 *
 * Данные хранятся внутри модели в контейнере QVector<MyRect>.
 */
class MyModel final : public QAbstractTableModel
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор модели.
     * @param parent Родительский QObject.
     *
     * Инициализирует заголовки столбцов (m_headerData).
     */
    explicit MyModel(QObject* parent = nullptr);

    /**
     * @brief Возвращает число строк модели.
     * @param parent Родительский индекс (для табличной модели должен быть невалидным).
     * @return Количество прямоугольников в контейнере (m_vector.size()).
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает число столбцов модели.
     * @param parent Родительский индекс (для табличной модели должен быть невалидным).
     * @return Количество столбцов (Column::Count).
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные ячейки.
     * @param index Индекс (строка/столбец).
     * @param role Роль данных (Qt::DisplayRole, Qt::DecorationRole и т.д.).
     *
     * Реализовано:
     * - Qt::DisplayRole: текстовые значения параметров MyRect.
     * - Qt::DecorationRole (только для столбца PenColor): цветная пиктограмма 16x16.
     *
     * @return QVariant с данными или пустой QVariant при некорректном индексе/роли.
     */
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    /**
     * @brief Возвращает заголовок для строки/столбца.
     * @param section Номер секции.
     * @param orientation Qt::Horizontal или Qt::Vertical.
     * @param role Роль данных (используется Qt::DisplayRole).
     *
     * Горизонтальные заголовки берутся из m_headerData,
     * вертикальные — нумерация строк 1..N.
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает заголовок столбца.
     * @param section Номер секции.
     * @param orientation Ориентация (допускается только Qt::Horizontal).
     * @param value Новое значение заголовка.
     * @param role Роль (используется Qt::EditRole).
     *
     * При успехе генерирует сигнал headerDataChanged().
     * @return true, если заголовок был изменён; иначе false.
     */
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant& value,
                       int role = Qt::EditRole) override;

    /**
     * @brief Вставляет строки в модель.
     * @param row Позиция вставки.
     * @param count Количество вставляемых строк.
     * @param parent Родительский индекс (для табличной модели должен быть невалидным).
     *
     * Обязательно вызывает beginInsertRows() и endInsertRows()
     * для корректного оповещения представлений.
     *
     * Вставляемые элементы инициализируются значениями MyRect по умолчанию.
     *
     * @return true при успешной вставке, иначе false.
     */
    bool insertRows(int row,
                    int count,
                    const QModelIndex& parent = QModelIndex()) override;

    /**
     * @brief Возвращает флаги элемента модели.
     * @param index Индекс элемента.
     *
     * Сейчас элементы только выбираемые и доступные (без редактирования).
     * Позже можно добавить Qt::ItemIsEditable.
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    /**
     * @brief Перечисление столбцов табличной модели.
     *
     * Используется для удобства и типобезопасности при обращении к столбцам.
     * Column::Count — служебное значение, равное числу столбцов.
     */
    enum class Column : int
    {
        PenColor = 0, ///< Цвет пера (QColor)
        PenStyle,     ///< Стиль пера (Qt::PenStyle)
        PenWidth,     ///< Толщина пера (int)
        Left,         ///< X координата левого верхнего угла (int)
        Top,          ///< Y координата левого верхнего угла (int)
        Width,        ///< Ширина (int)
        Height,       ///< Высота (int)
        Count         ///< Количество столбцов
    };

    /**
     * @brief Преобразование Column в int.
     */
    static constexpr int toInt(Column c) noexcept
    {
        return static_cast<int>(c);
    }

    /**
     * @brief Возвращает количество столбцов модели.
     */
    static constexpr int columnCountValue() noexcept
    {
        return toInt(Column::Count);
    }

private:
    /// Контейнер примитивов (данные модели). Одна строка таблицы = один MyRect.
    QVector<MyRect> m_vector;

    /// Заголовки столбцов для горизонтальной ориентации.
    QStringList     m_headerData;
};

#endif // MYMODEL_H
