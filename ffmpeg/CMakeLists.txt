set(PROJECT_SOURCES
    event/errorevent.hpp
    event/event.hpp
    event/pauseevent.hpp
    event/seekevent.hpp
    event/trackevent.hpp
    event/valueevent.hpp
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
    videorender/videopreviewwidget.cc
    videorender/videopreviewwidget.hpp
    videorender/videorender.cc
    videorender/videorender.hpp
    videorender/videorendercreate.cc
    videorender/videorendercreate.hpp
    videorender/widgetrender.cc
    videorender/widgetrender.hpp
    audiodecoder.cpp
    audiodecoder.h
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
    colorspace.hpp
    decoder.cc
    decoder.h
    decoderaudioframe.cpp
    decoderaudioframe.h
    decodersubtitleframe.cc
    decodersubtitleframe.hpp
    decodervideoframe.cpp
    decodervideoframe.h
    ffmepg_global.h
    ffmpegutils.cc
    ffmpegutils.hpp
    formatcontext.cpp
    formatcontext.h
    frame.cc
    frame.hpp
    mediainfo.cc
    mediainfo.hpp
    packet.cpp
    packet.h
    player.cpp
    player.h
    subtitle.cpp
    subtitle.h
    subtitledecoder.cpp
    subtitledecoder.h
    transcode.cc
    transcode.hpp
    videodecoder.cpp
    videodecoder.h
    videoformat.cc
    videoformat.hpp
    videoframeconverter.cc
    videoframeconverter.hpp)

qt_add_resources(SOURCES videorender/shaders.qrc)

add_custom_library(ffmpeg ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(ffmpeg PRIVATE utils Qt6::Widgets Qt6::Multimedia
                                     Qt6::OpenGLWidgets)
target_link_libraries(ffmpeg PRIVATE PkgConfig::ffmpeg PkgConfig::ass)
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(ffmpeg PRIVATE PkgConfig::fontconfig expat::expat)
endif()
target_link_libraries(
  ffmpeg
  PRIVATE harfbuzz
          harfbuzz::harfbuzz
          PkgConfig::fribidi
          freetype
          PNG::PNG
          ZLIB::ZLIB
          BZip2::BZip2
          unofficial::brotli::brotlidec
          unofficial::brotli::brotlienc)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(ffmpeg PRIVATE "FFMPEG_LIBRARY")
elseif(CMAKE_HOST_APPLE)
  target_link_libraries(
    ffmpeg
    PRIVATE ${Foundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${OpenGL_LIBRARY}
            ${CoreText_LIBRARY}
            ${CoreImage_LIBRARY}
            ${AppKit_LIBRARY}
            ${Security_LIBRARY}
            ${AudioToolBox_LIBRARY}
            ${VideoToolBox_LIBRARY}
            ${CoreFoundation_LIBRARY}
            ${CoreMedia_LIBRARY}
            ${CoreVideo_LIBRARY}
            ${CoreServices_LIBRARY})
endif()
