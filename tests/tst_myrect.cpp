// tests/tst_myrect.cpp
#include <QtTest/QtTest>
#include "myrect.h"

/**
 * @brief Набор юнит-тестов для структуры MyRect.
 *
 * @details
 * Цель тестов — зафиксировать контракт структуры MyRect:
 * - значения полей по умолчанию должны совпадать с тем, что задано in-class инициализаторами;
 * - параметризованный конструктор должен корректно присваивать все входные параметры всем полям.
 *
 * @note
 * MyRect — value-object без логики, ветвлений и инвариантов.
 * Поэтому покрытие тестами сводится к проверке корректной инициализации полей.
 */
class TestMyRect : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief Проверяет значения MyRect по умолчанию.
     *
     * @details
     * Тест создаёт объект MyRect дефолтным конструктором и проверяет,
     * что все поля соответствуют спецификации (см. комментарии в myrect.h):
     * - penColor  == Qt::black
     * - penStyle  == Qt::SolidLine
     * - penWidth  == 1
     * - left/top  == 0/0
     * - width/height == 10/10
     *
     * @note
     * Этот тест защищает от случайных изменений in-class инициализации
     * или от добавления логики в default-конструктор.
     */
    void defaults();

    /**
     * @brief Проверяет, что параметризованный конструктор корректно инициализирует все поля.
     *
     * @details
     * Тест создаёт MyRect через конструктор со всеми аргументами и проверяет,
     * что каждое поле структуры получает соответствующее переданное значение:
     * - цвет пера (QColor)
     * - стиль пера (Qt::PenStyle)
     * - толщина пера
     * - геометрия прямоугольника (left, top, width, height)
     *
     * @note
     * Этот тест защищает от:
     * - перепутанного порядка присваиваний;
     * - забытых полей в списке инициализации;
     * - ошибок при будущих рефакторингах/расширениях структуры.
     */
    void ctor_sets_all_fields();
};

void TestMyRect::defaults()
{
    MyRect r;
    QCOMPARE(r.penColor, QColor(Qt::black));
    QCOMPARE(r.penStyle, Qt::SolidLine);
    QCOMPARE(r.penWidth, 1);
    QCOMPARE(r.left, 0);
    QCOMPARE(r.top, 0);
    QCOMPARE(r.width, 10);
    QCOMPARE(r.height, 10);
}

void TestMyRect::ctor_sets_all_fields()
{
    MyRect r(QColor(Qt::red), Qt::DotLine, 3, 1, 2, 30, 40);
    QCOMPARE(r.penColor, QColor(Qt::red));
    QCOMPARE(r.penStyle, Qt::DotLine);
    QCOMPARE(r.penWidth, 3);
    QCOMPARE(r.left, 1);
    QCOMPARE(r.top, 2);
    QCOMPARE(r.width, 30);
    QCOMPARE(r.height, 40);
}

/**
 * @brief Точка входа Qt Test.
 *
 * @details
 * Макрос QTEST_MAIN генерирует main() для запуска тестового набора.
 * Так как класс тестов содержит Q_OBJECT и объявлен в .cpp файле,
 * подключаем сгенерированный moc-файл.
 */
QTEST_MAIN(TestMyRect)
#include "tst_myrect.moc"
