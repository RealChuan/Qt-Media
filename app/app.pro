include(../Common.pri)

QT       += core gui network multimedia openglwidgets core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = FfmpegPlayer

LIBS += -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(mainwindow) \
    -l$$replaceLibName(crashhandler) \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../3rdparty/3rdparty.pri)

RC_ICONS = app.ico
#ICON     = app.icns

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp

OTHER_FILES += \
    app.ico

DESTDIR = $$APP_OUTPUT_PATH
