include(../../common.pri)

QT       += core gui widgets network multimediawidgets core5compat gui-private

TEMPLATE = app

TARGET = QPlayer

LIBS += \
    -l$$replaceLibName(mediaconfig) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(dump) \
    -l$$replaceLibName(utils)

include(../../src/3rdparty/3rdparty.pri)

SOURCES += \
    ../common/controlwidget.cc \
    ../common/equalizerdialog.cpp \
    ../common/openwebmediadialog.cc \
    ../common/playlistmodel.cpp \
    ../common/playlistview.cc \
    ../common/qmediaplaylist_p.cpp \
    ../common/qmediaplaylist.cpp \
    ../common/qplaylistfileparser.cpp \
    ../common/slider.cpp \
    ../common/titlewidget.cc \
    main.cc \
    mainwindow.cc \
    videowidget.cc

HEADERS += \
    ../common/commonstr.hpp \
    ../common/controlwidget.hpp \
    ../common/equalizerdialog.h \
    ../common/openwebmediadialog.hpp \
    ../common/playlistmodel.h \
    ../common/playlistview.hpp \
    ../common/qmediaplaylist.h \
    ../common/qmediaplaylist_p.h \
    ../common/qplaylistfileparser.h \
    ../common/slider.h \
    ../common/titlewidget.hpp \
    mainwindow.hpp \
    videowidget.hpp

DESTDIR = $$APP_OUTPUT_PATH
