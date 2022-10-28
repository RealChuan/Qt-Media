qt_add_executable(FfmpegPlayer MANUAL_FINALIZATION main.cpp)
target_link_libraries(FfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication utils crashhandler ffmpeg mainwindow)
target_include_directories(FfmpegPlayer PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${AVFILTER_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR}) 
target_link_libraries(ffmpeg PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFILTER_LIBRARY} ${SWRESAMPLE_LIBRARY} ${SWSCALE_LIBRARY})
qt_finalize_executable(FfmpegPlayer)
