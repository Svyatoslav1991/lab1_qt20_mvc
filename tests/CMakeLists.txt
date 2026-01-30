cmake_minimum_required(VERSION 3.5)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Test Widgets)

# Чтобы тесты не требовали WinMain на Windows/MinGW
# (иначе словишь "undefined reference to WinMain")
# WIN32_EXECUTABLE=true => WinMain, false => main. :contentReference[oaicite:2]{index=2}
function(make_test_exe target_name)
    add_executable(${target_name} ${ARGN})
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
    target_link_libraries(${target_name} PRIVATE Qt5::Test Qt5::Widgets)
    set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE FALSE)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

# --- MyRect ---
make_test_exe(test_myrect
    test_myrect.cpp
)

# --- MyModel ---
make_test_exe(test_mymodel
    test_mymodel.cpp
    ../mymodel.cpp
)

# --- MyDelegate (без editorEvent) ---
make_test_exe(test_mydelegate
    test_mydelegate.cpp
    ../mydelegate.cpp
    ../mymodel.cpp
)

# --- MainWindow wiring (без вызова QFileDialog слотов) ---
make_test_exe(test_mainwindow
    test_mainwindow.cpp
    ../mainwindow.cpp
    ../mydelegate.cpp
    ../mymodel.cpp
    ../mainwindow.ui
)
