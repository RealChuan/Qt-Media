include(../../common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = Player

LIBS += \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(dump) \
    -l$$replaceLibName(utils)

include(../../src/3rdparty/3rdparty.pri)

SOURCES += \
    colorspacedialog.cc \
    controlwidget.cc \
    main.cpp \
    mainwindow.cpp \
    openwebmediadialog.cc \
    playlistmodel.cpp \
    playlistview.cc \
    qmediaplaylist.cpp \
    qplaylistfileparser.cpp \
    slider.cpp \
    titlewidget.cc

HEADERS += \
    colorspacedialog.hpp \
    controlwidget.hpp \
    mainwindow.h \
    openwebmediadialog.hpp \
    playlistmodel.h \
    playlistview.hpp \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser_p.h \
    slider.h \
    titlewidget.hpp

DESTDIR = $$APP_OUTPUT_PATH
