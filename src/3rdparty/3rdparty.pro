include(3rdparty.pri)
include(../slib.pri)
include(qtsingleapplication/qtsingleapplication.pri)

DEFINES += THRIDPARTY_LIBRARY
TARGET = $$replaceLibName(thirdparty)

LIBS += -l$$replaceLibName(utils)

HEADERS += thirdparty_global.hpp
