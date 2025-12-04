set(PROJECT_SOURCES
    ../common/commonstr.hpp
    audioencodermodel.cc
    audioencodermodel.hpp
    audioencodertableview.cc
    audioencodertableview.hpp
    audioencoderwidget.cc
    audioencoderwidget.hpp
    commonwidgets.cc
    commonwidgets.hpp
    main.cc
    mainwindow.cc
    mainwindow.hpp
    outputwidget.cc
    outputwidget.hpp
    previewwidget.cc
    previewwidget.hpp
    sourcewidget.cc
    sourcewidget.hpp
    stautuswidget.cc
    stautuswidget.hpp
    styleditemdelegate.cc
    styleditemdelegate.hpp
    subtitleencodermodel.cc
    subtitleencodermodel.hpp
    subtitleencodertableview.cc
    subtitleencodertableview.hpp
    subtitleencoderwidget.cc
    subtitleencoderwidget.hpp
    videoencoderwidget.cc
    videoencoderwidget.hpp)

qt_add_executable(FfmpegTranscoder MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_include_directories(FfmpegTranscoder PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(
  FfmpegTranscoder
  PRIVATE ffmpeg
          mediaconfig
          thirdparty
          dump
          utils
          Qt::Widgets
          Qt::Multimedia
          Qt::OpenGLWidgets)
target_link_libraries(FfmpegTranscoder PRIVATE tl::expected)
target_include_directories(FfmpegTranscoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(FfmpegTranscoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(FfmpegTranscoder PRIVATE ${FFMPEG_LIBRARIES})
