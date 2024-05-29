include(../../common.pri)

QT       += core gui network multimedia openglwidgets core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = Subtitle_unittest

LIBS += -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(utils)

include(../../src/3rdparty/3rdparty.pri)

SOURCES += \
    main.cc \
    mainwindow.cc \
    subtitlethread.cc

HEADERS += \
    mainwindow.hpp \
    subtitlethread.hpp

DESTDIR = $$APP_OUTPUT_PATH
