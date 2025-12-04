set(PROJECT_SOURCES main.cc mainwindow.cc mainwindow.hpp subtitlethread.cc
                    subtitlethread.hpp)

qt_add_executable(subtitle_unittest MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(
  subtitle_unittest PRIVATE ffmpeg utils Qt::Widgets Qt::Multimedia
                            Qt::OpenGLWidgets)
target_link_libraries(subtitle_unittest PRIVATE tl::expected)
target_include_directories(subtitle_unittest PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(subtitle_unittest PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(subtitle_unittest PRIVATE ${FFMPEG_LIBRARIES})
