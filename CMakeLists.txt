cmake_minimum_required(VERSION 3.5)

project(lab1_Qt20_MVC LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Widgets REQUIRED)


add_executable(lab1_Qt20_MVC
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    myrect.h
    mymodel.h
    mymodel.cpp
    mydelegate.cpp
    mydelegate.h
    )

target_link_libraries(lab1_Qt20_MVC PRIVATE Qt5::Widgets)
