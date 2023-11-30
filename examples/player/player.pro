include(../../Common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = QFfmpegPlayer

LIBS += \
    -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../../3rdparty/3rdparty.pri)

SOURCES += \
    colorspacedialog.cc \
    controlwidget.cc \
    main.cpp \
    mainwindow.cpp \
    mediainfodialog.cc \
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
    mediainfodialog.hpp \
    openwebmediadialog.hpp \
    playlistmodel.h \
    playlistview.hpp \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser_p.h \
    slider.h \
    titlewidget.hpp

DESTDIR = $$APP_OUTPUT_PATH
