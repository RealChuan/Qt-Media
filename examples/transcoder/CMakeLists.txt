set(PROJECT_SOURCES
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

qt_add_executable(Transcoder MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(
  Transcoder
  PRIVATE ffmpeg
          thirdparty
          dump
          utils
          Qt6::Widgets
          Qt6::Multimedia
          Qt6::OpenGLWidgets)
target_link_libraries(Transcoder PRIVATE PkgConfig::ffmpeg)

if(CMAKE_HOST_APPLE)
  target_link_libraries(
    Transcoder
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

qt_finalize_executable(Transcoder)
