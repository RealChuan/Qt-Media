include(../libs.pri)

QT += widgets

greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

DEFINES += UTILS_LIBRARY
TARGET = $$replaceLibName(utils)

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    countdownlatch.cc \
    fps.cc \
    hostosinfo.cpp \
    logasync.cpp \
    utils.cpp

HEADERS += \
    boundedblockingqueue.hpp \
    countdownlatch.hpp \
    fps.hpp \
    hostosinfo.h \
    logasync.h \
    osspecificaspects.h \
    singleton.hpp \
    utils_global.h \
    utils.h
