include(../libs.pri)
include(../3rdparty/3rdparty.pri)
include(videooutput/videooutput.pri)
include(videorender/videorender.pri)
include(subtitle/subtitle.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    audioframeconverter.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    codeccontext.cpp \
    decoder.cc \
    decoderaudioframe.cpp \
    decodersubtitleframe.cc \
    decodervideoframe.cpp \
    formatcontext.cpp \
    frame.cc \
    hardwaredecode.cc \
    packet.cpp \
    player.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    videodecoder.cpp \
    videoformat.cc \
    videoframeconverter.cc

HEADERS += \
    audiodecoder.h \
    audioframeconverter.h \
    avcontextinfo.h \
    averror.h \
    codeccontext.h \
    colorspace.hpp \
    decoder.h \
    decoderaudioframe.h \
    decodersubtitleframe.hpp \
    decodervideoframe.h \
    ffmepg_global.h \
    formatcontext.h \
    frame.hpp \
    hardwaredecode.hpp \
    packet.h \
    player.h \
    subtitle.h \
    subtitledecoder.h \
    videodecoder.h \
    videoformat.hpp \
    videoframeconverter.hpp
