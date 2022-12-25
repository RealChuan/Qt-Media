include(../Common.pri)

QT       += core gui network multimedia openglwidgets core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = QFfmpegPlayer

LIBS += -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(crashhandler) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../3rdparty/3rdparty.pri)

RC_ICONS = player.ico
#ICON     = player.icns

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    slider.cpp

OTHER_FILES += \
    app.ico

DESTDIR = $$APP_OUTPUT_PATH

RESOURCES += \
    resource.qrc

HEADERS += \
    mainwindow.h \
    slider.h
