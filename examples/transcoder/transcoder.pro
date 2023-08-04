include(../../Common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = QFfmpegTranscoder

LIBS += \
    -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../../3rdparty/3rdparty.pri)

SOURCES += \
    main.cc \
    mainwindow.cc

HEADERS += \
    mainwindow.hpp

DESTDIR = $$APP_OUTPUT_PATH

DEFINES += QT_DEPRECATED_WARNINGS
