include(../libs.pri)
include(../3rdparty/3rdparty.pri)
include(videorender/videorender.pri)
include(subtitle/subtitle.pri)
include(filter/filter.pri)
include(gpu/gpu.pri)
include(event/event.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    audiofifo.cc \
    audioframeconverter.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    averrormanager.cc \
    clock.cc \
    codeccontext.cpp \
    decoder.cc \
    decoderaudioframe.cpp \
    decodersubtitleframe.cc \
    decodervideoframe.cpp \
    ffmpegutils.cc \
    formatcontext.cpp \
    frame.cc \
    packet.cpp \
    player.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    transcode.cc \
    videodecoder.cpp \
    videoformat.cc \
    videoframeconverter.cc

HEADERS += \
    audiodecoder.h \
    audiofifo.hpp \
    audioframeconverter.h \
    avcontextinfo.h \
    averror.h \
    averrormanager.hpp \
    clock.hpp \
    codeccontext.h \
    colorspace.hpp \
    decoder.h \
    decoderaudioframe.h \
    decodersubtitleframe.hpp \
    decodervideoframe.h \
    ffmepg_global.h \
    ffmpegutils.hpp \
    formatcontext.h \
    frame.hpp \
    mediainfo.hpp \
    packet.h \
    player.h \
    subtitle.h \
    subtitledecoder.h \
    transcode.hpp \
    videodecoder.h \
    videoformat.hpp \
    videoframeconverter.hpp
