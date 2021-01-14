include(../libs.pri)

QT += widgets

DEFINES += UTILS_LIBRARY
TARGET = $$replaceLibName(utils)

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    hostosinfo.cpp \
    json.cpp \
    logasync.cpp \
    utils.cpp

HEADERS += \
    hostosinfo.h \
    json.h \
    logasync.h \
    osspecificaspects.h \
    taskqueue.h \
    utils_global.h \
    utils.h
