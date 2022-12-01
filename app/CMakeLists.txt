qt_add_resources(SOURCES resource.qrc)

qt_add_executable(QFfmpegPlayer MANUAL_FINALIZATION main.cpp ${SOURCES})
target_link_libraries(QFfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication crashhandler mainwindow ffmpeg utils)
qt_finalize_executable(QFfmpegPlayer)
