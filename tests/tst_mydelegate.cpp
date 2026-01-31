/**
 * @file tst_mydelegate.cpp
 * @brief Юнит-тесты для класса MyDelegate.
 *
 * @details
 * Цели тестов:
 * 1) Проверить корректность поведения делегата в части редакторов:
 *    - createEditor(): для столбца PenStyle создаётся QComboBox, иначе — нет.
 *    - setEditorData(): ComboBox синхронизируется с данными модели (EditRole).
 *    - setModelData(): выбранное значение из ComboBox записывается обратно в модель (EditRole).
 *
 * 2) Проверить обработку событий (editorEvent) для столбца PenColor:
 *    - “ранние выходы” (не та колонка / не dblclick / не ЛКМ) не изменяют модель.
 *    - интеграционные GUI-сценарии с QColorDialog:
 *      - Cancel: модель не меняется, метод возвращает true;
 *      - Accept: модель меняется на выбранный цвет, метод возвращает true.
 *
 * @note
 * Тесты с QColorDialog запускают модальный диалог. На headless-платформах
 * (offscreen/minimal) это может быть нестабильно или приводить к зависанию,
 * поэтому такие тесты пропускаются через QSKIP.
 */

#include <QtTest/QtTest>

#include <QComboBox>
#include <QColor>
#include <QColorDialog>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTimer>
#include <QWidget>

#include "mydelegate.h"

namespace {

/**
 * @brief Проверяет, является ли текущая Qt-платформа “headless-подобной”.
 *
 * @details
 * Для GUI-тестов с модальными диалогами (QColorDialog::getColor) среда без
 * полноценного оконного сервера может приводить к зависаниям или нестабильному поведению.
 * В таких случаях тесты следует пропускать.
 *
 * @return true, если платформа содержит "offscreen" или "minimal".
 */
bool isHeadlessLikePlatform()
{
    const QString p = QGuiApplication::platformName().toLower();
    return (p.contains("offscreen") || p.contains("minimal"));
}

/**
 * @brief Утилита: читает цвет из модели по Qt::EditRole и сравнивает с ожидаемым.
 *
 * @details
 * Делегат MyDelegate предполагает, что модель для столбца PenColor в EditRole
 * возвращает QColor. Здесь мы дополнительно проверяем валидность QVariant и возможность
 * конвертации в QColor, чтобы падения происходили с понятным сообщением.
 *
 * @param model Модель, из которой читаем значение.
 * @param idx Индекс ячейки (ожидается столбец PenColor).
 * @param expected Ожидаемый QColor.
 */
void compareModelColor(const QAbstractItemModel& model, const QModelIndex& idx, const QColor& expected)
{
    const QVariant v = model.data(idx, Qt::EditRole);
    QVERIFY2(v.isValid(), "Model EditRole should be valid");
    QVERIFY2(v.canConvert<QColor>(), "Model EditRole should be convertible to QColor");
    QCOMPARE(v.value<QColor>(), expected);
}

} // namespace

/**
 * @brief Набор тестов для делегата MyDelegate.
 *
 * @details
 * Тестируем строго публичное поведение:
 * - создание редакторов;
 * - перенос данных из модели в редактор и обратно;
 * - корректность ранних выходов в editorEvent;
 * - интеграционные GUI-сценарии с QColorDialog без рефактора MyDelegate.
 */
class TestMyDelegate : public QObject
{
    Q_OBJECT
private slots:
    /// createEditor(): для PenStyle создаётся QComboBox.
    void createEditor_for_penStyle_is_combo();

    /// createEditor(): для других колонок не создаётся QComboBox (может быть nullptr или иной редактор).
    void createEditor_for_other_column_is_not_combo();

    /// createEditor(): на невалидном индексе не создаётся QComboBox и не происходит падения.
    void createEditor_for_invalid_index_does_not_return_combo();

    /// setEditorData(): ComboBox выставляется в значение модели (EditRole) для PenStyle.
    void setEditorData_sets_combo_to_model_value();

    /// setEditorData(): при неизвестном стиле ComboBox устанавливается на индекс 0.
    void setEditorData_unknown_style_sets_index0();

    /// setModelData(): выбранный в ComboBox стиль записывается в модель (EditRole).
    void setModelData_writes_combo_value_to_model();

    /// editorEvent(): событие в другой колонке не должно приводить к изменению модели.
    void editorEvent_other_column_does_not_change_model();

    /// editorEvent(): событие не dblclick должно игнорироваться (модель не меняется).
    void editorEvent_non_dblclick_does_not_change_model();

    /// editorEvent(): dblclick, но не ЛКМ, должен игнорироваться (модель не меняется).
    void editorEvent_dblclick_not_left_button_does_not_change_model();

    /// editorEvent()+QColorDialog: Cancel не меняет модель и возвращает true.
    void editorEvent_penColor_cancel_does_not_change_model_and_returns_true();

    /// editorEvent()+QColorDialog: Accept меняет модель на выбранный цвет и возвращает true.
    void editorEvent_penColor_accept_sets_color_and_returns_true();
};

// -------------------- createEditor --------------------

/**
 * @brief Проверка createEditor(): в колонке PenStyle должен создаваться QComboBox.
 */
void TestMyDelegate::createEditor_for_penStyle_is_combo()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle column (1)

    QStyleOptionViewItem opt;
    QWidget parent;

    QWidget* ed = d.createEditor(&parent, opt, idx);
    QVERIFY(ed != nullptr);
    QVERIFY2(qobject_cast<QComboBox*>(ed) != nullptr, "Editor for PenStyle must be QComboBox");
}

/**
 * @brief Проверка createEditor(): в колонке, отличной от PenStyle, QComboBox создаваться не должен.
 */
void TestMyDelegate::createEditor_for_other_column_is_not_combo()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 0); // PenColor column (0) -> not PenStyle

    QStyleOptionViewItem opt;
    QWidget parent;

    QWidget* ed = d.createEditor(&parent, opt, idx);
    // Базовый делегат может вернуть nullptr или какой-то редактор — это ок.
    if (ed)
        QVERIFY2(qobject_cast<QComboBox*>(ed) == nullptr, "Editor for non-PenStyle must NOT be QComboBox");
}

/**
 * @brief Проверка createEditor(): для невалидного индекса делегат не должен создавать QComboBox.
 */
void TestMyDelegate::createEditor_for_invalid_index_does_not_return_combo()
{
    MyDelegate d;

    QStyleOptionViewItem opt;
    QWidget parent;

    const QModelIndex invalid;
    QWidget* ed = d.createEditor(&parent, opt, invalid);

    // Главное: не падать и не возвращать QComboBox.
    if (ed)
        QVERIFY2(qobject_cast<QComboBox*>(ed) == nullptr, "Invalid index must NOT produce QComboBox");
}

// -------------------- setEditorData --------------------

/**
 * @brief Проверка setEditorData(): ComboBox должен выбрать элемент, соответствующий EditRole модели.
 */
void TestMyDelegate::setEditorData_sets_combo_to_model_value()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    model.setData(idx, static_cast<int>(Qt::DotLine), Qt::EditRole);

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    d.setEditorData(combo, idx);

    QCOMPARE(combo->currentData().toInt(), static_cast<int>(Qt::DotLine));
}

/**
 * @brief Проверка setEditorData(): если стиль отсутствует в ComboBox, выбирается индекс 0.
 */
void TestMyDelegate::setEditorData_unknown_style_sets_index0()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    // Стиль, которого нет в combo -> findData == -1 -> setCurrentIndex(0)
    model.setData(idx, 999999, Qt::EditRole);

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    d.setEditorData(combo, idx);

    QCOMPARE(combo->currentIndex(), 0);
}

// -------------------- setModelData --------------------

/**
 * @brief Проверка setModelData(): выбранный стиль из ComboBox должен записаться в модель (EditRole).
 */
void TestMyDelegate::setModelData_writes_combo_value_to_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex idx = model.index(0, 1); // PenStyle

    QStyleOptionViewItem opt;
    QWidget parent;
    QWidget* ed = d.createEditor(&parent, opt, idx);
    auto* combo = qobject_cast<QComboBox*>(ed);
    QVERIFY(combo != nullptr);

    const int pos = combo->findData(static_cast<int>(Qt::DashLine));
    QVERIFY(pos >= 0);
    combo->setCurrentIndex(pos);

    d.setModelData(combo, &model, idx);

    QCOMPARE(model.data(idx, Qt::EditRole).toInt(), static_cast<int>(Qt::DashLine));
}

// -------------------- editorEvent: ранние выходы (без GUI) --------------------

/**
 * @brief Проверка editorEvent(): событие в колонке, отличной от PenColor, не должно менять модель.
 */
void TestMyDelegate::editorEvent_other_column_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penStyleIdx = model.index(0, 1); // НЕ PenColor

    // Инициализация: в PenStyle лежит DotLine
    model.setData(penStyleIdx, static_cast<int>(Qt::DotLine), Qt::EditRole);
    const int before = model.data(penStyleIdx, Qt::EditRole).toInt();

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    d.editorEvent(&dbl, &model, opt, penStyleIdx);

    const int after = model.data(penStyleIdx, Qt::EditRole).toInt();
    QCOMPARE(after, before);
}

/**
 * @brief Проверка editorEvent(): событие MouseButtonPress (не dblclick) должно игнорироваться.
 */
void TestMyDelegate::editorEvent_non_dblclick_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor

    model.setData(penColorIdx, QColor(Qt::red), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::red));

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    d.editorEvent(&press, &model, opt, penColorIdx);

    compareModelColor(model, penColorIdx, QColor(Qt::red));
}

/**
 * @brief Проверка editorEvent(): dblclick правой кнопкой должен игнорироваться.
 */
void TestMyDelegate::editorEvent_dblclick_not_left_button_does_not_change_model()
{
    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor

    model.setData(penColorIdx, QColor(Qt::green), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::green));

    QTableView view;
    view.setModel(&model);

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QMouseEvent dblRight(QEvent::MouseButtonDblClick, QPointF(1, 1),
                         Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    d.editorEvent(&dblRight, &model, opt, penColorIdx);

    compareModelColor(model, penColorIdx, QColor(Qt::green));
}

// -------------------- editorEvent + QColorDialog (GUI) --------------------

/**
 * @brief Интеграционный тест: Cancel в QColorDialog не меняет модель, метод возвращает true.
 *
 * @details
 * Внутри editorEvent() вызывается QColorDialog::getColor().
 * Мы закрываем модальный диалог программно через QTimer и dlg->reject().
 */
void TestMyDelegate::editorEvent_penColor_cancel_does_not_change_model_and_returns_true()
{
    if (isHeadlessLikePlatform())
        QSKIP("Skipping QColorDialog GUI test on headless/minimal platform.");

    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor
    model.setData(penColorIdx, QColor(Qt::blue), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::blue));

    QTableView view;
    view.setModel(&model);
    view.show();

    QStyleOptionViewItem opt;
    opt.widget = &view;

    QTimer::singleShot(0, []() {
        QWidget* w = QApplication::activeModalWidget();
        if (auto* dlg = qobject_cast<QColorDialog*>(w))
            dlg->reject();
    });

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    const bool handled = d.editorEvent(&dbl, &model, opt, penColorIdx);

    QVERIFY(handled);
    compareModelColor(model, penColorIdx, QColor(Qt::blue));
}

/**
 * @brief Интеграционный тест: Accept в QColorDialog меняет модель на выбранный цвет, метод возвращает true.
 *
 * @details
 * Мы программно устанавливаем текущий цвет в QColorDialog и вызываем dlg->accept().
 */
void TestMyDelegate::editorEvent_penColor_accept_sets_color_and_returns_true()
{
    if (isHeadlessLikePlatform())
        QSKIP("Skipping QColorDialog GUI test on headless/minimal platform.");

    MyDelegate d;

    QStandardItemModel model(1, 2);
    const QModelIndex penColorIdx = model.index(0, 0); // PenColor
    model.setData(penColorIdx, QColor(Qt::black), Qt::EditRole);
    compareModelColor(model, penColorIdx, QColor(Qt::black));

    QTableView view;
    view.setModel(&model);
    view.show();

    QStyleOptionViewItem opt;
    opt.widget = &view;

    const QColor chosen = QColor(Qt::magenta);

    QTimer::singleShot(0, [chosen]() {
        QWidget* w = QApplication::activeModalWidget();
        if (auto* dlg = qobject_cast<QColorDialog*>(w))
        {
            dlg->setCurrentColor(chosen);
            dlg->accept();
        }
    });

    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(1, 1),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    const bool handled = d.editorEvent(&dbl, &model, opt, penColorIdx);

    QVERIFY(handled);
    compareModelColor(model, penColorIdx, chosen);
}

QTEST_MAIN(TestMyDelegate)
#include "tst_mydelegate.moc"
