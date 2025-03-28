include(../../common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = FfmpegPlayer

LIBS += \
    -l$$replaceLibName(ffmpeg) \
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
    ../common/qmediaplaylist.cpp \
    ../common/qplaylistfileparser.cpp \
    ../common/slider.cpp \
    ../common/titlewidget.cc \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../common/controlwidget.hpp \
    ../common/equalizerdialog.h \
    ../common/openwebmediadialog.hpp \
    ../common/playlistmodel.h \
    ../common/playlistview.hpp \
    ../common/qmediaplaylist.h \
    ../common/qmediaplaylist_p.h \
    ../common/qplaylistfileparser_p.h \
    ../common/slider.h \
    ../common/titlewidget.hpp \
    mainwindow.h

DESTDIR = $$APP_OUTPUT_PATH
