include(../slib.pri)

QT       += core

DEFINES += MEDIACONFIG_LIBRARY
TARGET = $$replaceLibName(mediaconfig)

LIBS += -l$$replaceLibName(utils)

HEADERS += \
    equalizer.hpp \
    mediaconfig_global.hpp

SOURCES += \
    equalizer.cc
