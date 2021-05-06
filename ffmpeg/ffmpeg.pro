include(../libs.pri)
include(ffmpeg.pri)

QT += widgets multimedia

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    avaudio.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    avimage.cpp \
    codeccontext.cpp \
    decoderaudioframe.cpp \
    decodervideoframe.cpp \
    formatcontext.cpp \
    packet.cpp \
    player.cpp \
    playframe.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    videodecoder.cpp \
    videooutputwidget.cpp

HEADERS += \
    audiodecoder.h \
    avaudio.h \
    avcontextinfo.h \
    averror.h \
    avimage.h \
    codeccontext.h \
    decoder.h \
    decoderaudioframe.h \
    decodervideoframe.h \
    ffmepg_global.h \
    formatcontext.h \
    packet.h \
    player.h \
    playframe.h \
    subtitle.h \
    subtitledecoder.h \
    videodecoder.h \
    videooutputwidget.h
