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
target_include_directories(FfmpegPlayer PRIVATE ${CMAKE_SOURCE_DIR})

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
target_include_directories(FfmpegPlayer PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(FfmpegPlayer PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(FfmpegPlayer PRIVATE ${FFMPEG_LIBRARIES})
qt_finalize_executable(FfmpegPlayer)
