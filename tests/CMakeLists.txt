cmake_minimum_required(VERSION 3.5)

# В tests/ нет project() — он уже объявлен в корне.
# Но можно оставить без вреда; я обычно не дублирую.

# На всякий случай включим автомок для Q_OBJECT в тестах
set(CMAKE_AUTOMOC ON)

# Нужны QtTest + (опционально) Widgets, т.к. модель использует QColor/QIcon/QPixmap и т.п.
find_package(Qt5 REQUIRED COMPONENTS Test Widgets)

add_executable(test_mymodel
    test_mymodel.cpp
    # Подключаем реализацию модели напрямую (простой вариант без выделения библиотеки)
    ../mymodel.cpp
)

set_target_properties(test_mymodel PROPERTIES WIN32_EXECUTABLE FALSE)

# Чтобы тест видел заголовки из корня проекта
target_include_directories(test_mymodel PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(test_mymodel PRIVATE
    Qt5::Test
    Qt5::Widgets
)

# Регистрируем тест для ctest
add_test(NAME MyModelTests COMMAND test_mymodel)
