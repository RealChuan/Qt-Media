set(PROJECT_SOURCES
        mainwindow_global.h
        mainwindow.cpp
        mainwindow.h
        playerwidget.cpp
        playerwidget.h
        slider.cpp
        slider.h)

if (CMAKE_HOST_WIN32)
  add_library(mainwindow SHARED ${PROJECT_SOURCES} ${SOURCES})
else ()
  add_library(mainwindow ${PROJECT_SOURCES} ${SOURCES})
endif ()
target_link_libraries(mainwindow PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::OpenGLWidgets utils ffmpeg)

if (CMAKE_HOST_WIN32)
  target_compile_definitions(mainwindow PRIVATE "MAINWINDOW_LIBRARY")
endif()
