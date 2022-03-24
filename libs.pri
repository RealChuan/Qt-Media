include(Common.pri)

TEMPLATE = lib

win32 {
DESTDIR = $$APP_OUTPUT_PATH/../libs
DLLDESTDIR = $$APP_OUTPUT_PATH
}

macx{
CONFIG += staticlib
DESTDIR = $$APP_OUTPUT_PATH/libs
#DESTDIR = $$APP_OUTPUT_PATH/FfmpegPlayer.app/Contents/Frameworks
}

unix:!macx {
CONFIG += c++14
DESTDIR = $$APP_OUTPUT_PATH
}

LIBS += -L$$DESTDIR
