include(../../Common.pri)

QT       += core gui network multimedia openglwidgets core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = QFfmpegPlayer

LIBS += -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(crashhandler) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../../3rdparty/3rdparty.pri)

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    openwebmediadialog.cc \
    slider.cpp

HEADERS += \
    mainwindow.h \
    openwebmediadialog.hpp \
    slider.h

DESTDIR = $$APP_OUTPUT_PATH

DEFINES += QT_DEPRECATED_WARNINGS
