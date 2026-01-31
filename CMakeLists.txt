cmake_minimum_required(VERSION 3.10)
project(lab1_Qt20_MVC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Test)

add_library(lab1_core STATIC
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    myrect.h
    mymodel.cpp
    mymodel.h
    mydelegate.cpp
    mydelegate.h
)

target_include_directories(lab1_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lab1_core PUBLIC Qt5::Widgets)

set_target_properties(lab1_core PROPERTIES
    AUTOMOC_COMPILER_PREDEFINES OFF
)

add_executable(lab1_Qt20_MVC main.cpp)
target_link_libraries(lab1_Qt20_MVC PRIVATE lab1_core)

set_target_properties(lab1_Qt20_MVC PROPERTIES
    AUTOMOC_COMPILER_PREDEFINES OFF
)

include(CTest)
add_subdirectory(tests)
