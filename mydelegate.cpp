#include "mydelegate.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

/**
 * @brief Конструктор MyDelegate.
 *
 * Делегат не хранит собственного состояния, поэтому конструктор пустой.
 * Родитель нужен для управления временем жизни делегата.
 */
MyDelegate::MyDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

/**
 * @brief Заполняет ComboBox вариантами Qt::PenStyle.
 *
 * Мы добавляем элементы в виде пары:
 *  - text: строка, видимая пользователю
 *  - userData: числовое значение стиля (int)
 *
 * Это удобно, потому что:
 *  - при отображении пользователь видит читаемый текст
 *  - в модель можно писать int (QVariant это понимает)
 *
 * @param combo Комбобокс, который нужно заполнить.
 */
void MyDelegate::fillPenStyleCombo(QComboBox* combo)
{
    // Текст + userData (int), чтобы потом setData() получал int
    combo->addItem("Qt::NoPen",           static_cast<int>(Qt::NoPen));
    combo->addItem("Qt::SolidLine",       static_cast<int>(Qt::SolidLine));
    combo->addItem("Qt::DashLine",        static_cast<int>(Qt::DashLine));
    combo->addItem("Qt::DotLine",         static_cast<int>(Qt::DotLine));
    combo->addItem("Qt::DashDotLine",     static_cast<int>(Qt::DashDotLine));
    combo->addItem("Qt::DashDotDotLine",  static_cast<int>(Qt::DashDotDotLine));
}

/**
 * @brief Создание специфического редактора для элемента таблицы.
 *
 * Редактор создаём только для столбца PenStyle.
 * Для остальных столбцов используем стандартные редакторы QStyledItemDelegate:
 *  - строки → QLineEdit
 *  - целые → QSpinBox
 *  - и т.п.
 *
 * @param parent Родитель редактора (задаёт представление).
 * @param option Параметры стиля (не используем, но должны корректно передать базовому классу).
 * @param index Индекс ячейки.
 * @return Виджет-редактор.
 */
QWidget* MyDelegate::createEditor(QWidget* parent,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    // Если индекс неверный — делегируем базовому классу
    if (!index.isValid())
        return QStyledItemDelegate::createEditor(parent, option, index);

    // Если это не PenStyle — делегируем базовому классу
    if (index.column() != kPenStyleColumn)
        return QStyledItemDelegate::createEditor(parent, option, index);

    // Для PenStyle используем ComboBox
    auto* combo = new QComboBox(parent);
    fillPenStyleCombo(combo);
    combo->setEditable(false);
    return combo;
}

/**
 * @brief Отображение текущего значения в редакторе.
 *
 * Для PenStyle мы должны:
 *  1) запросить у модели значение именно через Qt::EditRole,
 *     т.к. DisplayRole может возвращать строку (для красивого отображения)
 *  2) найти в combo элемент с userData == styleInt
 *  3) установить combo->setCurrentIndex(...)
 *
 * @param editor Редактор (ожидаем QComboBox для PenStyle).
 * @param index Индекс ячейки.
 */
void MyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    // Если не PenStyle — базовая логика
    if (!index.isValid() || index.column() != kPenStyleColumn)
    {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }

    // Безопасное приведение к QComboBox (редактор мы сами создали в createEditor)
    auto* combo = static_cast<QComboBox*>(editor);

    // Важно: просим именно EditRole (там int)
    const int styleInt = index.model()->data(index, Qt::EditRole).toInt();

    // Ищем элемент в ComboBox по userData
    const int pos = combo->findData(styleInt);

    // Если не найден — берём 0
    combo->setCurrentIndex(pos >= 0 ? pos : 0);
}

/**
 * @brief Сохранение выбранного значения из редактора в модель.
 *
 * Для PenStyle берём combo->currentData() (int) и пишем его в модель через setData().
 *
 * @param editor Редактор (QComboBox).
 * @param model Модель, в которую записываем данные.
 * @param index Индекс ячейки.
 */
void MyDelegate::setModelData(QWidget* editor,
                             QAbstractItemModel* model,
                             const QModelIndex& index) const
{
    if (!index.isValid() || index.column() != kPenStyleColumn)
    {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    auto* combo = static_cast<QComboBox*>(editor);

    // Берём userData = int(Qt::PenStyle) и пишем в модель
    // Роль Qt::EditRole можно передать явно для ясности.
    model->setData(index, combo->currentData(), Qt::EditRole);
}

/**
 * @brief Обработка событий редактирования (подход через event handling).
 *
 * Здесь настраиваем редактирование цвета пера (PenColor) так:
 *  - по двойному клику левой кнопкой мыши открываем QColorDialog
 *  - если пользователь выбрал валидный цвет — сохраняем его через model->setData(...)
 *
 * @param event Событие (ожидаем MouseButtonDblClick).
 * @param model Модель, которую можно модифицировать.
 * @param option Параметры отрисовки.
 * @param index Индекс элемента, по которому пришло событие.
 * @return true, если событие обработано; иначе — результат базового класса.
 */
bool MyDelegate::editorEvent(QEvent* event,
                            QAbstractItemModel* model,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index)
{
    /**
     * Шаг 1. Ранний выход для всех случаев, которые нас не интересуют:
     * - невалидный индекс;
     * - отсутствует модель (nullptr);
     * - клик не в столбце PenColor.
     *
     * В этих случаях отдаём обработку базовому классу.
     */
    if (!index.isValid() || !model || index.column() != kPenColorColumn)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    /**
     * Шаг 2. Нас интересует только двойной клик мышью.
     * Все остальные события (наведение, одиночный клик и т.п.) — базовой обработке.
     */
    if (event->type() != QEvent::MouseButtonDblClick)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    /**
     * Шаг 3. Приведение QEvent к QMouseEvent безопасно, так как мы уже проверили type().
     * Затем проверяем, что двойной клик был ЛКМ (LeftButton).
     */
    auto* me = static_cast<QMouseEvent*>(event);
    if (me->button() != Qt::LeftButton)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    /**
     * Шаг 4. Получаем текущий цвет из модели.
     * Мы просим Qt::EditRole, потому что это “машинное” значение для редакторов/делегатов.
     * В идеале модель вернёт QColor, но на всякий случай поддержим и строковый вариант.
     */
    const QVariant cur = model->data(index, Qt::EditRole);

    QColor currentColor;
    if (cur.canConvert<QColor>())
        currentColor = cur.value<QColor>();
    else
        currentColor = QColor(cur.toString().trimmed());

    /**
     * Шаг 5. Определяем родителя для диалога.
     * option.widget — это виджет представления, через который пришло событие (обычно QTableView).
     * Привязка диалога к этому родителю улучшает UX:
     * - диалог будет поверх правильного окна,
     * - корректная модальность,
     * - корректное позиционирование.
     */
    QWidget* parentWidget = option.widget ? const_cast<QWidget*>(option.widget) : nullptr;

    /**
     * Шаг 6. Открываем стандартный диалог выбора цвета.
     * В качестве “начального” цвета передаём текущий цвет, чтобы пользователь видел текущее значение.
     */
    const QColor selectedColor =
        QColorDialog::getColor(currentColor, parentWidget, "Select pen color");

    /**
     * Шаг 7. Если диалог отменён (цвет невалидный) — считаем событие обработанным,
     * но ничего в модель не записываем.
     */
    if (!selectedColor.isValid())
        return true;

    /**
     * Шаг 8. Записываем выбранный цвет в модель.
     * Модель должна обработать Qt::EditRole и сохранить QColor в соответствующее поле.
     */
    model->setData(index, selectedColor, Qt::EditRole);

    /**
     * Возвращаем true: мы обработали событие и не хотим, чтобы Qt делал что-то ещё с этим кликом.
     */
    return true;
}

