set(PROJECT_SOURCES
    ../common/commonstr.hpp
    ../common/controlwidget.cc
    ../common/controlwidget.hpp
    ../common/equalizerdialog.cpp
    ../common/equalizerdialog.h
    ../common/openwebmediadialog.cc
    ../common/openwebmediadialog.hpp
    ../common/playlistmodel.cpp
    ../common/playlistmodel.h
    ../common/playlistview.cc
    ../common/playlistview.hpp
    ../common/qmediaplaylist_p.cpp
    ../common/qmediaplaylist_p.h
    ../common/qmediaplaylist.cpp
    ../common/qmediaplaylist.h
    ../common/qplaylistfileparser.cpp
    ../common/qplaylistfileparser.h
    ../common/slider.cpp
    ../common/slider.h
    ../common/titlewidget.cc
    ../common/titlewidget.hpp
    main.cpp
    mainwindow.cpp
    mainwindow.h)

qt_add_executable(FfmpegPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_compile_definitions(FfmpegPlayer PRIVATE "FFMPEG_ON")
target_link_libraries(
  FfmpegPlayer
  PRIVATE ffmpeg
          mediaconfig
          thirdparty
          dump
          utils
          Qt::Widgets
          Qt::Multimedia
          Qt::OpenGLWidgets)
target_link_libraries(FfmpegPlayer PRIVATE PkgConfig::ffmpeg)
if(CMAKE_HOST_APPLE)
  target_link_libraries(
    FfmpegPlayer
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
qt_finalize_executable(FfmpegPlayer)
