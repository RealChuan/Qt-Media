include(../libs.pri)
include(../3rdparty/3rdparty.pri)

QT += widgets concurrent openglwidgets

DEFINES += MAINWINDOW_LIBRARY
TARGET = $$replaceLibName(mainwindow)

LIBS += \
    -l$$replaceLibName(utils) \
    -l$$replaceLibName(ffmpeg)

SOURCES += \
    mainwindow.cpp \
    playerwidget.cpp \
    slider.cpp

HEADERS += \
    mainwindow_global.h \
    mainwindow.h \
    playerwidget.h \
    slider.h
