set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        slider.cpp slider.h)

qt_add_resources(SOURCES resource.qrc)

qt_add_executable(QFfmpegPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(QFfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication crashhandler ffmpeg utils)
target_include_directories(QFfmpegPlayer PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${AVFILTER_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR}) 
target_link_libraries(QFfmpegPlayer PRIVATE ${SWSCALE_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY}  ${AVUTIL_LIBRARY} ${AVFILTER_LIBRARY} ${SWRESAMPLE_LIBRARY})

qt_finalize_executable(QFfmpegPlayer)
