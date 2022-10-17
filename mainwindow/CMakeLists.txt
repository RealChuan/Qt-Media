set(PROJECT_SOURCES
        mainwindow_global.h
        mainwindow.cpp
        mainwindow.h
        playerwidget.cpp
        playerwidget.h
        slider.cpp
        slider.h)

add_library(mainwindow SHARED ${PROJECT_SOURCES})
target_link_libraries(mainwindow PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::OpenGLWidgets utils ffmpeg)

if (WIN32)
  target_compile_definitions(mainwindow PRIVATE "MAINWINDOW_LIBRARY")
endif()
