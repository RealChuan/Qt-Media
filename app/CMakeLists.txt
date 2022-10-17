qt_add_executable(FfmpegPlayer MANUAL_FINALIZATION main.cpp)
target_link_libraries(FfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication utils crashhandler ffmpeg mainwindow)
qt_finalize_executable(FfmpegPlayer)
