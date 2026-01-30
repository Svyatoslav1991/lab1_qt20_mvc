#ifndef MYDELEGATE_H
#define MYDELEGATE_H

#include <QStyledItemDelegate>

class QComboBox;

/**
 * @brief Пользовательский делегат для табличного представления QTableView (Model/View).
 *
 * Делегат в Qt отвечает за:
 *  1) создание редактора для ячейки (createEditor),
 *  2) загрузку текущего значения из модели в редактор (setEditorData),
 *  3) запись изменённого значения из редактора обратно в модель (setModelData),
 *  4) (опционально) обработку пользовательских событий редактирования (editorEvent).
 *
 * В данной лабораторной делегат реализует два разных подхода настройки редактирования:
 *
 * 1) **Создание виджета-редактора** для столбца PenStyle:
 *    - при редактировании в столбце PenStyle показывается QComboBox
 *      со списком значений Qt::PenStyle.
 *    - в QComboBox хранится:
 *        - отображаемый текст (например, "Qt::DotLine")
 *        - userData (int), равный числовому значению Qt::PenStyle
 *
 * 2) **Обработка событий** для столбца PenColor:
 *    - по двойному клику ЛКМ (MouseButtonDblClick) открывается стандартный диалог QColorDialog
 *    - выбранный QColor записывается в модель через model->setData(index, QColor, Qt::EditRole)
 *
 * @note Этот делегат предполагает, что модель:
 *  - возвращает для PenStyle в Qt::EditRole целое число (int),
 *  - умеет принимать QColor в setData() для столбца PenColor.
 *
 * @warning Номера столбцов (kPenColorColumn, kPenStyleColumn) должны совпадать
 *          с порядком столбцов в перечислении MyModel::Column.
 */
class MyDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор делегата.
     * @param parent Родительский QObject (обычно ui->tableView).
     *
     * Родитель задаётся для корректного управления временем жизни делегата
     * (удаление вместе с родителем).
     */
    explicit MyDelegate(QObject* parent = nullptr);

    /**
     * @brief Создаёт редактор (виджет) для редактирования ячейки.
     *
     * Вызывается QTableView, когда пользователь начинает редактирование.
     *
     * Логика:
     *  - если индекс невалидный или редактируем не PenStyle → используем стандартный редактор базового класса
     *  - если столбец PenStyle → создаём QComboBox и заполняем списком стилей пера
     *
     * @param parent Родитель для редактора (его задаёт представление).
     * @param option Параметры отрисовки/стиля (обычно не используем, но передаём в базовый класс).
     * @param index Индекс редактируемой ячейки.
     * @return Указатель на созданный QWidget-редактор.
     */
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;

    /**
     * @brief Загружает текущее значение из модели и отображает его в редакторе.
     *
     * Вызывается сразу после createEditor().
     *
     * Для PenStyle:
     *  - берём значение из модели через index.model()->data(index, Qt::EditRole).toInt()
     *  - ищем в QComboBox элемент с таким userData (findData)
     *  - устанавливаем текущий индекс ComboBox
     *
     * Для остальных столбцов — используем реализацию базового класса.
     *
     * @param editor Редактор, созданный createEditor().
     * @param index Индекс ячейки.
     */
    void setEditorData(QWidget* editor,
                       const QModelIndex& index) const override;

    /**
     * @brief Записывает значение из редактора обратно в модель.
     *
     * Вызывается, когда редактирование завершается (переход в другую ячейку, Enter и т.п.).
     *
     * Для PenStyle:
     *  - берём userData из QComboBox (currentData) — это int(Qt::PenStyle)
     *  - передаём в model->setData(index, value, Qt::EditRole)
     *
     * Для остальных столбцов — используем реализацию базового класса.
     *
     * @param editor Редактор, содержащий новое значение.
     * @param model Модель, связанная с представлением.
     * @param index Индекс ячейки, куда сохраняем результат.
     */
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    /**
     * @brief Обрабатывает пользовательские события для ячейки (альтернативный путь редактирования).
     *
     * В этой лабораторной используется для PenColor:
     *  - при событии MouseButtonDblClick и нажатии ЛКМ
     *    открываем QColorDialog и сохраняем выбранный цвет в модель.
     *
     * Для других столбцов/событий — передаём обработку базовому классу.
     *
     * @param event Указатель на событие (QEvent).
     * @param model Модель, в которую можно записывать данные через setData().
     * @param option Параметры стиля/отрисовки.
     * @param index Индекс элемента, по которому пришло событие.
     * @return true если событие обработано и дальнейшая обработка не нужна; иначе результат базового класса.
     */
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

private:
    /**
     * @brief Номер столбца "цвет пера".
     *
     * @warning Должен совпадать с MyModel::Column::PenColor.
     */
    static constexpr int kPenColorColumn = 0;

    /**
     * @brief Номер столбца "стиль пера".
     *
     * @warning Должен совпадать с MyModel::Column::PenStyle.
     */
    static constexpr int kPenStyleColumn = 1;

private:
    /**
     * @brief Заполняет QComboBox доступными стилями пера.
     *
     * В combo добавляются элементы:
     *  - text  = строковое имя стиля (например, "Qt::SolidLine")
     *  - userData = int(Qt::PenStyle) — числовое значение, которое сохраняем в модель
     *
     * @param combo Указатель на QComboBox, который нужно заполнить.
     */
    static void fillPenStyleCombo(QComboBox* combo);
};

#endif // MYDELEGATE_H
