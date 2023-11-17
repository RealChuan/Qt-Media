include(../libs.pri)
include(../3rdparty/3rdparty.pri)
include(subtitle/subtitle.pri)
include(filter/filter.pri)
include(gpu/gpu.pri)
include(videorender/videorender.pri)
include(audiorender/audiorender.pri)
include(event/event.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += -l$$replaceLibName(utils)

SOURCES += \
    audiodecoder.cpp \
    audiodisplay.cc \
    audiofifo.cc \
    audioframeconverter.cpp \
    avcontextinfo.cpp \
    averror.cpp \
    averrormanager.cc \
    clock.cc \
    codeccontext.cpp \
    colorspace.cc \
    decoder.cc \
    ffmpegutils.cc \
    formatcontext.cpp \
    frame.cc \
    hdrmetadata.cc \
    mediainfo.cc \
    packet.cpp \
    player.cpp \
    subtitle.cpp \
    subtitledecoder.cpp \
    subtitledisplay.cc \
    transcode.cc \
    videodecoder.cpp \
    videodisplay.cc \
    videoformat.cc \
    videoframeconverter.cc

HEADERS += \
    audiodecoder.h \
    audiodisplay.hpp \
    audiofifo.hpp \
    audioframeconverter.h \
    avcontextinfo.h \
    averror.h \
    averrormanager.hpp \
    clock.hpp \
    codeccontext.h \
    colorspace.hpp \
    decoder.h \
    ffmepg_global.h \
    ffmpegutils.hpp \
    formatcontext.h \
    frame.hpp \
    hdrmetadata.hpp \
    mediainfo.hpp \
    packet.h \
    player.h \
    subtitle.h \
    subtitledecoder.h \
    subtitledisplay.hpp \
    transcode.hpp \
    videodecoder.h \
    videodisplay.hpp \
    videoformat.hpp \
    videoframeconverter.hpp
