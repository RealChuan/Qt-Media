include(../../common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = FfmpegTranscoder

LIBS += \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(mediaconfig) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(dump) \
    -l$$replaceLibName(utils)

include(../../src/3rdparty/3rdparty.pri)

SOURCES += \
    audioencodermodel.cc \
    audioencodertableview.cc \
    audioencoderwidget.cc \
    commonwidgets.cc \
    main.cc \
    mainwindow.cc \
    outputwidget.cc \
    previewwidget.cc \
    sourcewidget.cc \
    stautuswidget.cc \
    styleditemdelegate.cc \
    subtitleencodermodel.cc \
    subtitleencodertableview.cc \
    subtitleencoderwidget.cc \
    videoencoderwidget.cc

HEADERS += \
    audioencodermodel.hpp \
    audioencodertableview.hpp \
    audioencoderwidget.hpp \
    commonwidgets.hpp \
    mainwindow.hpp \
    outputwidget.hpp \
    previewwidget.hpp \
    sourcewidget.hpp \
    stautuswidget.hpp \
    styleditemdelegate.hpp \
    subtitleencodermodel.hpp \
    subtitleencodertableview.hpp \
    subtitleencoderwidget.hpp \
    videoencoderwidget.hpp

DESTDIR = $$APP_OUTPUT_PATH

DEFINES += QT_DEPRECATED_WARNINGS
