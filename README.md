# lab1_qt20_mvc

Учебный проект на **Qt Widgets** с архитектурой **Model/View (MVC)**: табличное представление прямоугольников (`MyRect`) с редактированием через **модель** (`QAbstractTableModel`) и **делегат**.

Проект демонстрирует:
- работу с Qt Model/View API (`data`, `setData`, `flags`, `headerData`, `insertRows`, сигналы);
- разделение ролей `DisplayRole / EditRole / DecorationRole`;
- сохранение/загрузка данных в **TSV**;
- покрытие ключевой логики **автотестами QtTest**.

---

## Возможности

### Таблица прямоугольников
Для каждого прямоугольника хранится:
- цвет пера (`PenColor`);
- стиль пера (`PenStyle`);
- толщина пера (`PenWidth`);
- геометрия (`Left`, `Top`, `Width`, `Height`).

Редактирование:
- цвет — через делегат (двойной клик → выбор цвета);
- стиль пера — через делегат/редактор (как `int(Qt::PenStyle)`);
- числа — как целые значения.

---

## Модель `MyModel`

`MyModel` — наследник `QAbstractTableModel`.

Реализовано:
- фиксированное число столбцов (7);
- `data()` для ролей:
  - `Qt::EditRole` — машинные значения (например `QColor`, `int(Qt::PenStyle)`, `int`);
  - `Qt::DisplayRole` — отображение (например `"#RRGGBB"`, `"Qt::DotLine"`, числа);
  - `Qt::DecorationRole` — иконка цвета для `PenColor`;
- `setData()` — изменение ячеек с корректным `dataChanged(..., roles)`:
  - `PenColor`: принимает `QColor` или строку `#RRGGBB`;
  - `PenStyle`: принимает `int`;
  - числовые поля: `toInt()`;
- `insertRows()` — вставка строк с `beginInsertRows/endInsertRows`;
- сериализация:
  - `saveToTsv/loadFromTsv` по имени файла;
  - `saveToTsv/loadFromTsv` через `QIODevice` (удобно для тестов через `QBuffer`).

### Формат TSV
- одна строка = один прямоугольник;
- разделитель `\t`;
- 7 полей в строке;
- `PenColor` хранится как `#RRGGBB`;
- `PenStyle` хранится как строка вида `Qt::DotLine` (поддерживается также парсинг `"3"` и `"Qt::PenStyle(3)"`);
- остальные поля — целые числа.

---

## Делегат `MyDelegate`

Делегат отвечает за UX редактирования:
- на двойной клик по ячейке цвета открывает стандартный `QColorDialog`;
- при выборе валидного цвета записывает его в модель через `setData()`.

---

## Главное окно `MainWindow`

В `MainWindow`:
- создаётся `MyModel` и привязывается к `QTableView`;
- создаётся `MyDelegate` и назначается таблице;
- включается растяжение столбцов (`QHeaderView::Stretch`);
- создаётся меню **"Файл"**:
  - **Открыть...** — загрузка TSV (`loadFromTsv`);
  - **Сохранить...** — сохранение TSV (`saveToTsv`);
- (опционально) вызывается `MyModel::test()` для заполнения тестовыми данными.

---

## Автотесты

Тесты написаны на **QtTest** и лежат в папке `tests/`:

- `tst_mymodel.cpp`:
  - проверка поведения модели: row/column count, headerData, flags;
  - проверка корректности `data()` и ролей;
  - проверка `setData()` (включая `dataChanged` и список ролей);
  - проверка TSV (roundtrip через `QBuffer`, ошибки формата, гарантия “не менять модель при ошибке”).

- `tst_mainwindow.cpp`:
  - smoke-тест конструктора;
  - проверка наличия `QTableView`, модели, делегата;
  - проверка заполнения тестовыми данными (если включён `MyModel::test()` в конструкторе);
  - проверка меню "Файл" и ожидаемых `QAction` + стандартных шорткатов.

> Примечание: диалоги `QFileDialog::get*` и `QMessageBox` обычно не покрывают unit-тестами без инъекции зависимостей, т.к. они вызываются статическими методами и требуют UI-взаимодействия.

---

## Структура репозитория

(Исходники лежат в корне проекта — папка `src/` не используется.)

- `tests/` — автотесты (`tst_mymodel.cpp`, `tst_mainwindow.cpp`)
- `main.cpp` — точка входа
- `mainwindow.h/.cpp` — главное окно
- `mainwindow.ui` — форма Qt Designer
- `mymodel.h/.cpp` — модель
- `mydelegate.h/.cpp` — делегат
- `myrect.h` — данные прямоугольника
- `CMakeLists.txt` — сборка CMake

---

## Сборка и запуск (CMake)

Пример для сборки out-of-source:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Debug
