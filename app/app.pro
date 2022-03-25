include(../Common.pri)

QT       += core gui network multimedia openglwidgets core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

TARGET = FfmpegPlayer

win32 {
LIBS += -L$$APP_OUTPUT_PATH/../libs
}

macx{
include(../3rdparty/3rdparty.pri)
LIBS += -L$$APP_OUTPUT_PATH/libs
#LIBS += -L$$APP_OUTPUT_PATH/FfmpegPlayer.app/Contents/Frameworks
}

unix!:macx {
LIBS += -L$$APP_OUTPUT_PATH
}

LIBS += \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils) \
    -l$$replaceLibName(crashhandler) \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(mainwindow)

RC_ICONS = app.ico
#ICON     = app.icns

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp

OTHER_FILES += \
    app.ico

DESTDIR = $$APP_OUTPUT_PATH
