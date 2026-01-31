# lab1_qt20_mvc

[![ci-windows-mingw](https://github.com/Svyatoslav1991/lab1_qt20_mvc/actions/workflows/ci-windows-mingw.yml/badge.svg)](https://github.com/Svyatoslav1991/lab1_qt20_mvc/actions/workflows/ci-windows-mingw.yml)

Учебный проект на **Qt Widgets** с архитектурой **Model/View (MVC)**: табличное представление прямоугольников (`MyRect`) с редактированием через **модель** (`QAbstractTableModel`) и **делегат**.

Проект демонстрирует:
- работу с Qt Model/View API (`data`, `setData`, `flags`, `headerData`, `insertRows`, сигналы);
- разделение ролей `DisplayRole / EditRole / DecorationRole`;
- сохранение/загрузка данных в **TSV**;
- покрытие ключевой логики **автотестами QtTest**.

---

## Требования

- CMake (минимальная версия в проекте: **3.10**, см. `CMakeLists.txt`)
- Компилятор с поддержкой **C++17** (см. `CMakeLists.txt`)
- Qt 5: модули **Widgets** и **Test** (см. `find_package(Qt5 REQUIRED COMPONENTS Widgets Test)`)
- Windows + MinGW (локально) / GitHub Actions (CI)

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

## CI

В репозитории настроен GitHub Actions workflow: **сборка + прогон тестов на Windows (MinGW)**.

Файл workflow: `.github/workflows/ci-windows-mingw.yml`.

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

## Сборка и запуск (Windows + MinGW)

Ниже — самый простой вариант через консоль (Git Bash / PowerShell).

### 1) Подготовь Qt для CMake
CMake должен видеть Qt5-пакеты.

Вариант A (рекомендуется): укажи `Qt5_DIR` на папку с `Qt5Config.cmake`.

Пример пути (у тебя Qt 5.14.2 MinGW 7.3):
- `C:\Qt\5.14.2\mingw73_64\lib\cmake\Qt5`

Вариант B: укажи `CMAKE_PREFIX_PATH` на корень установленного Qt:
- `C:\Qt\5.14.2\mingw73_64`

> Если собираешь из **Qt Creator**, то обычно ничего дополнительно указывать не надо:
> Qt Creator сам подставляет пути из выбранного Kit (MinGW + Qt).

### 2) Сборка проекта (out-of-source)

**Git Bash (пример):**
```bash
mkdir -p build
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug \
  -DQt5_DIR="C:/Qt/5.14.2/mingw73_64/lib/cmake/Qt5"

cmake --build build -- -j 2
```

**PowerShell (пример):**
```powershell
mkdir build
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug `
  -DQt5_DIR="C:/Qt/5.14.2/mingw73_64/lib/cmake/Qt5"

cmake --build build -- -j 2
```

> Если используешь `CMAKE_PREFIX_PATH`, то замени `-DQt5_DIR=...` на:
> `-DCMAKE_PREFIX_PATH="C:/Qt/5.14.2/mingw73_64"`

### 3) Запуск приложения

После сборки исполняемый файл будет в `build/` (для MinGW Makefiles — обычно прямо там):

```bash
./build/lab1_Qt20_MVC.exe
```

---

## Запуск тестов

Тесты регистрируются через CTest, поэтому их удобнее запускать так.

### 1) Прогон всех тестов
```bash
ctest --test-dir build --output-on-failure
```

### 2) Прогон одного теста по имени
Имена тестов заданы в `tests/CMakeLists.txt`:
- `tst_myrect`
- `tst_mymodel`
- `tst_mydelegate`
- `tst_mainwindow`

Пример:
```bash
ctest --test-dir build -R tst_mymodel --output-on-failure
```

### 3) Запуск тестов напрямую (без CTest)
Путь к бинарникам тестов обычно build/tests/ (если используешь MinGW Makefiles). 
Если не нашёл — проверь, где CMake создал .exe (поиск по tst_*.exe).
Можно запускать `.exe` из `build/tests/`:

```bash
./build/tests/tst_mymodel.exe
./build/tests/tst_mainwindow.exe
```

---

