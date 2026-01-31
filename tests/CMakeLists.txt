# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Если в корне уже есть include(CTest), можно не повторять, но не мешает:
include(CTest)

function(add_qt_test target source)
  add_executable(${target} ${source})

  target_link_libraries(${target} PRIVATE
    Qt5::Test
    Qt5::Widgets
    lab1_core
  )

  # Твой MinGW workaround
  set_target_properties(${target} PROPERTIES
    AUTOMOC_COMPILER_PREDEFINES OFF
  )

  # Фикс "WinMain": если сборка пошла в Windows subsystem, подтягиваем Qt5::WinMain (qtmain).
  # Это типовой рецепт для Qt+Windows. :contentReference[oaicite:1]{index=1}
  if (WIN32 AND TARGET Qt5::WinMain)
    target_link_libraries(${target} PRIVATE Qt5::WinMain)
  endif()

  add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction()

add_qt_test(tst_myrect      tst_myrect.cpp)
add_qt_test(tst_mymodel     tst_mymodel.cpp)
add_qt_test(tst_mydelegate  tst_mydelegate.cpp)
add_qt_test(tst_mainwindow  tst_mainwindow.cpp)
