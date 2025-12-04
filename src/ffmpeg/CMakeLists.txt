set(PROJECT_SOURCES
    audiorender/audiooutput.cc
    audiorender/audiooutput.hpp
    audiorender/audiooutputthread.cc
    audiorender/audiooutputthread.hpp
    event/errorevent.hpp
    event/event.hpp
    event/seekevent.hpp
    event/trackevent.hpp
    event/valueevent.hpp
    filter/filter.cc
    filter/filter.hpp
    filter/filtercontext.cc
    filter/filtercontext.hpp
    filter/filtergraph.cc
    filter/filtergraph.hpp
    filter/filterinout.cc
    filter/filterinout.hpp
    gpu/bufferref.cc
    gpu/bufferref.hpp
    gpu/hardwaredecode.cc
    gpu/hardwaredecode.hpp
    gpu/hardwareencode.cc
    gpu/hardwareencode.hpp
    subtitle/ass.cc
    subtitle/ass.hpp
    subtitle/assdata.cc
    subtitle/assdata.hpp
    videorender/openglrender.cc
    videorender/openglrender.hpp
    videorender/openglshader.cc
    videorender/openglshader.hpp
    videorender/openglshaderprogram.cc
    videorender/openglshaderprogram.hpp
    videorender/shaderutils.cc
    videorender/shaderutils.hpp
    videorender/tonemapping.cc
    videorender/tonemapping.hpp
    videorender/videopreviewwidget.cc
    videorender/videopreviewwidget.hpp
    videorender/videorender.cc
    videorender/videorender.hpp
    videorender/videorendercreate.cc
    videorender/videorendercreate.hpp
    videorender/widgetrender.cc
    videorender/widgetrender.hpp
    widgets/mediainfodialog.cc
    widgets/mediainfodialog.hpp
    audiodecoder.cpp
    audiodecoder.h
    audiodisplay.cc
    audiodisplay.hpp
    audiofifo.cc
    audiofifo.hpp
    audioframeconverter.cpp
    audioframeconverter.h
    avcontextinfo.cpp
    avcontextinfo.h
    averror.cpp
    averror.h
    averrormanager.cc
    averrormanager.hpp
    clock.cc
    clock.hpp
    codeccontext.cpp
    codeccontext.h
    colorutils.cc
    colorutils.hpp
    decoder.cc
    decoder.h
    encodecontext.cc
    encodecontext.hpp
    ffmepg_global.h
    ffmpegutils.cc
    ffmpegutils.hpp
    formatcontext.cpp
    formatcontext.h
    frame.cc
    frame.hpp
    hdrmetadata.cc
    hdrmetadata.hpp
    mediainfo.cc
    mediainfo.hpp
    packet.cc
    packet.hpp
    player.cpp
    player.h
    previewtask.cc
    previewtask.hpp
    subtitle.cpp
    subtitle.h
    subtitledecoder.cpp
    subtitledecoder.h
    subtitledisplay.cc
    subtitledisplay.hpp
    transcoder.cc
    transcoder.hpp
    transcodercontext.cc
    transcodercontext.hpp
    videodecoder.cpp
    videodecoder.h
    videodisplay.cc
    videodisplay.hpp
    videoformat.cc
    videoformat.hpp
    videoframeconverter.cc
    videoframeconverter.hpp)

qt_add_resources(SOURCES videorender/shaders.qrc)

add_platform_library(ffmpeg ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(ffmpeg PRIVATE mediaconfig utils Qt::Widgets
                                     Qt::Multimedia Qt::OpenGLWidgets)
target_include_directories(ffmpeg PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg PRIVATE ${FFMPEG_LIBRARIES})
target_link_libraries(ffmpeg PRIVATE PkgConfig::ass)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(ffmpeg PRIVATE "FFMPEG_LIBRARY")
endif()
