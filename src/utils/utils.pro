include(../slib.pri)

QT       += widgets core5compat

DEFINES += UTILS_LIBRARY
TARGET = $$replaceLibName(utils)

SOURCES += \
    countdownlatch.cc \
    fps.cc \
    hostosinfo.cpp \
    logasync.cpp \
    speed.cc \
    utils.cpp

HEADERS += \
    boundedblockingqueue.hpp \
    concurrentqueue.hpp \
    countdownlatch.hpp \
    fps.hpp \
    hostosinfo.h \
    logasync.h \
    osspecificaspects.h \
    range.hpp \
    singleton.hpp \
    speed.hpp \
    utils_global.h \
    utils.h
