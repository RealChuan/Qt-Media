QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    player.cpp \
    playerwidget.cpp

HEADERS += \
    mainwindow.h \
    player.h \
    playerwidget.h

contains(QT_ARCH, i386) {
    BIN = bin-32
}else{
    BIN = bin-64
}

CONFIG(debug, debug|release) {
    APP_OUTPUT_PATH = $$PWD/$$BIN/debug
    LIBS += -LD:/Mine/CODE/vcpkg/packages/ffmpeg_x64-windows/debug/lib
}else{
    APP_OUTPUT_PATH = $$PWD/$$BIN/release
    LIBS += -LD:/Mine/CODE/vcpkg/packages/ffmpeg_x64-windows/lib
}

LIBS += -lavutil -lavformat -lavcodec -lavdevice -lavfilter -lpostproc -lswresample -lswscale

DESTDIR = $$APP_OUTPUT_PATH

INCLUDEPATH += D:/Mine/CODE/vcpkg/packages/ffmpeg_x64-windows/include
