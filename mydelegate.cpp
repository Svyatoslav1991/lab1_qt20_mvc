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
    // Работаем только с валидным индексом, существующей моделью и нужным столбцом (PenColor).
    if (!index.isValid() || !model || index.column() != kPenColorColumn)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    // Реагируем только на двойной клик мышью.
    if (event->type() != QEvent::MouseButtonDblClick)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    // Безопасно приводим к QMouseEvent (мы уже проверили type()).
    auto* me = static_cast<QMouseEvent*>(event);

    // Нас интересует только двойной клик ЛКМ.
    if (me->button() != Qt::LeftButton)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    // Текущий цвет лучше брать из EditRole: модель должна отдавать QColor (идеально),
    // но на всякий случай поддержим и строку "#RRGGBB".
    const QVariant cur = model->data(index, Qt::EditRole);

    QColor currentColor;
    if (cur.canConvert<QColor>())
        currentColor = cur.value<QColor>();
    else
        currentColor = QColor(cur.toString().trimmed());

    // Родитель для диалога: лучше привязать к виджету представления, если он известен.
    QWidget* parentWidget = option.widget ? const_cast<QWidget*>(option.widget) : nullptr;

    const QColor selectedColor =
        QColorDialog::getColor(currentColor, parentWidget, "Select pen color");

    // Если пользователь отменил диалог — событие считаем обработанным, но ничего не меняем.
    if (!selectedColor.isValid())
        return true;

    // Сохраняем выбранный цвет в модель (модель должна уметь принимать QColor в setData()).
    model->setData(index, selectedColor, Qt::EditRole);
    return true;
}

