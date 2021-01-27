include(../libs.pri)

QT += widgets

DEFINES += CRASHHANDLER_LIBRARY
TARGET = $$replaceLibName(crashhandler)

LIBS += -l$$replaceLibName(utils)
	
SOURCES += \
    crashhandler.cpp

HEADERS += \
    crashHandler_global.h \
    crashhandler.h
