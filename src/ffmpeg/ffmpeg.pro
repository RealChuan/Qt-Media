include(../slib.pri)
include(../3rdparty/3rdparty.pri)
include(subtitle/subtitle.pri)
include(filter/filter.pri)
include(gpu/gpu.pri)
include(videorender/videorender.pri)
include(audiorender/audiorender.pri)
include(event/event.pri)
include(widgets/widgets.pri)

QT += widgets multimedia openglwidgets

DEFINES += FFMPEG_LIBRARY
TARGET = $$replaceLibName(ffmpeg)

LIBS += \
    -l$$replaceLibName(mediaconfig) \
    -l$$replaceLibName(utils)

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
    colorutils.cc \
    decoder.cc \
    encodecontext.cc \
    ffmpegutils.cc \
    formatcontext.cpp \
    frame.cc \
    hdrmetadata.cc \
    mediainfo.cc \
    packet.cc \
    player.cpp \
    previewtask.cc \
    subtitle.cpp \
    subtitledecoder.cpp \
    subtitledisplay.cc \
    transcoder.cc \
    transcodercontext.cc \
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
    colorutils.hpp \
    decoder.h \
    encodecontext.hpp \
    ffmepg_global.h \
    ffmpegutils.hpp \
    formatcontext.h \
    frame.hpp \
    hdrmetadata.hpp \
    mediainfo.hpp \
    packet.hpp \
    player.h \
    previewtask.hpp \
    subtitle.h \
    subtitledecoder.h \
    subtitledisplay.hpp \
    transcoder.hpp \
    transcodercontext.hpp \
    videodecoder.h \
    videodisplay.hpp \
    videoformat.hpp \
    videoframeconverter.hpp
