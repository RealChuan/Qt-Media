include(Common.pri)

TEMPLATE = lib

win32 {
DESTDIR = $$APP_OUTPUT_PATH/../libs
DLLDESTDIR = $$APP_OUTPUT_PATH
}

unix {
CONFIG += c++14
DESTDIR = $$APP_OUTPUT_PATH
}

LIBS += -L$$DESTDIR
