set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        slider.cpp slider.h)

qt_add_resources(SOURCES resource.qrc)

qt_add_executable(QFfmpegPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(QFfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication crashhandler ffmpeg utils)
target_include_directories(QFfmpegPlayer PRIVATE ${AVDEVICE_INCLUDE_DIR} ${AVFILTER_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR}) 
target_link_libraries(QFfmpegPlayer PRIVATE ${AVDEVICE_LIBRARY} ${AVFILTER_LIBRARY} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${SWRESAMPLE_LIBRARY} ${SWSCALE_LIBRARY} ${AVUTIL_LIBRARY})

if (CMAKE_HOST_APPLE)
  target_link_libraries(QFfmpegPlayer PRIVATE ${Foundation_LIBRARY} ${CoreAudio_LIBRARY} ${AVFoundation_LIBRARY} ${CoreGraphics_LIBRARY} ${OpenGL_LIBRARY} ${CoreText_LIBRARY} ${CoreImage_LIBRARY} ${AppKit_LIBRARY} ${Security_LIBRARY} ${AudioToolBox_LIBRARY} ${VideoToolBox_LIBRARY} ${CoreFoundation_LIBRARY} ${CoreMedia_LIBRARY} ${CoreVideo_LIBRARY} ${CoreServices_LIBRARY})
endif ()

qt_finalize_executable(QFfmpegPlayer)
