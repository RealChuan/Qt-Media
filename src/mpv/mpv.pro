include(../slib.pri)
include(mpv.pri)

QT       += core gui network widgets openglwidgets

DEFINES += MPV_LIBRARY
TARGET = $$replaceLibName(custommpv)

SOURCES += \
    mediainfo.cc \
    mpvopenglwidget.cc \
    mpvplayer.cc \
    mpvwidget.cc \
    previewwidget.cc

HEADERS += \
    mediainfo.hpp \
    mpv_global.h \
    mpvopenglwidget.hpp \
    mpvplayer.hpp \
    mpvwidget.hpp \
    previewwidget.hpp \
    qthelper.hpp
