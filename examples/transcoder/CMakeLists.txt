set(PROJECT_SOURCES main.cc mainwindow.cc mainwindow.hpp)

qt_add_executable(QFfmpegTranscoder MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(
  QFfmpegTranscoder PRIVATE ffmpeg thirdparty utils Qt6::Widgets
                            Qt6::Multimedia Qt6::OpenGLWidgets)
target_link_libraries(QFfmpegTranscoder PRIVATE PkgConfig::ffmpeg)

if(CMAKE_HOST_APPLE)
  target_link_libraries(
    QFfmpegTranscoder
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

qt_finalize_executable(QFfmpegTranscoder)
