include(../libs.pri)
include(../3rdparty/3rdparty.pri)
include(videooutput/videooutput.pri)
include(videorender/videorender.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    avaudio.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    codeccontext.cpp \
    decoder.cc \
    decoderaudioframe.cpp \
    decodervideoframe.cpp \
    formatcontext.cpp \
    frame.cc \
    frameconverter.cc \
    hardwaredecode.cc \
    packet.cpp \
    player.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    videodecoder.cpp

HEADERS += \
    audiodecoder.h \
    avaudio.h \
    avcontextinfo.h \
    averror.h \
    codeccontext.h \
    decoder.h \
    decoderaudioframe.h \
    decodervideoframe.h \
    ffmepg_global.h \
    formatcontext.h \
    frame.hpp \
    frameconverter.hpp \
    hardwaredecode.hpp \
    packet.h \
    player.h \
    subtitle.h \
    subtitledecoder.h \
    videodecoder.h
