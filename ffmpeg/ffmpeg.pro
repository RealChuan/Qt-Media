include(../libs.pri)
include(../ffmpeg.pri)

QT += widgets multimedia

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    avaudio.cpp \
    avcontextinfo.cpp \
    avimage.cpp \
    codeccontext.cpp \
    decoderaudioframe.cpp \
    decodervideoframe.cpp \
    formatcontext.cpp \
    packet.cpp \
    player.cpp \
    playframe.cpp \
    subtitledecoder.cpp \
    videodecoder.cpp

HEADERS += \
    audiodecoder.h \
    avaudio.h \
    avcontextinfo.h \
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
    subtitledecoder.h \
    videodecoder.h
