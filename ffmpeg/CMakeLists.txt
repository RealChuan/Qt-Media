set(PROJECT_SOURCES
        videooutput/videoouputrenderwidget.cc videooutput/videoouputrenderwidget.hpp
        videooutput/videooutputrender.cc videooutput/videooutputrender.hpp
        videooutput/videooutputrenderopenglrender.cc videooutput/videooutputrenderopenglrender.hpp
        videooutput/videopreviewwidget.cc videooutput/videopreviewwidget.hpp

        videorender/openglrender.cc videorender/openglrender.hpp
        videorender/videorender.cc videorender/videorender.hpp

        audiodecoder.cpp audiodecoder.h
        avaudio.cpp avaudio.h
        avcontextinfo.cpp avcontextinfo.h
        averror.cpp averror.h
        codeccontext.cpp codeccontext.h
        decoder.h
        decoderaudioframe.cpp decoderaudioframe.h
        decodervideoframe.cpp decodervideoframe.h
        ffmepg_global.h
        formatcontext.cpp formatcontext.h
        frame.cc frame.hpp
        frameconverter.cc frameconverter.hpp
        hardwaredecode.cc hardwaredecode.hpp
        packet.cpp packet.h
        player.cpp player.h
        subtitle.cpp subtitle.h
        subtitledecoder.cpp subtitledecoder.h
        videodecoder.cpp videodecoder.h)

qt_add_resources(SOURCES videooutput/shader/shader.qrc)
qt_add_resources(SOURCES videorender/shaders.qrc)

if (CMAKE_HOST_WIN32)
  add_library(ffmpeg SHARED ${PROJECT_SOURCES} ${SOURCES})
else ()
  add_library(ffmpeg ${PROJECT_SOURCES} ${SOURCES})
endif ()
target_include_directories(ffmpeg PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${AVFILTER_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})
target_link_libraries(ffmpeg PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets utils
    ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFILTER_LIBRARY} ${SWRESAMPLE_LIBRARY} ${SWSCALE_LIBRARY})
if (CMAKE_HOST_APPLE)
  target_link_libraries(ffmpeg PRIVATE ${Security_LIBRARY} ${AudioToolBox_LIBRARY} ${VideoToolBox_LIBRARY} ${CoreVideo_LIBRARY} ${CoreMedia_LIBRARY})
endif ()

if (CMAKE_HOST_WIN32)
  target_compile_definitions(ffmpeg PRIVATE "FFMPEG_LIBRARY")
endif()
