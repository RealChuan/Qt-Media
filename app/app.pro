include(../Common.pri)

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = FfmpegPlayer

win32 {
LIBS += -L$$APP_OUTPUT_PATH/../libs
}

unix {
LIBS += -L$$APP_OUTPUT_PATH
}

LIBS += \
    -l$$replaceLibName(utils) \
    -l$$replaceLibName(crashhandler) \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(mainwindow) \

RC_ICONS = app.ico
#ICON     = app.icns

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp

OTHER_FILES += \
    app.ico

DESTDIR = $$APP_OUTPUT_PATH
