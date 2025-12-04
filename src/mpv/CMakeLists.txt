set(PROJECT_SOURCES
    mediainfo.cc
    mediainfo.hpp
    mpv_global.h
    mpvopenglwidget.cc
    mpvopenglwidget.hpp
    mpvplayer.cc
    mpvplayer.hpp
    mpvwidget.cc
    mpvwidget.hpp
    previewwidget.cc
    previewwidget.hpp
    qthelper.hpp)

add_platform_library(qmpv ${PROJECT_SOURCES})
target_link_libraries(qmpv PRIVATE Qt::Widgets Qt::OpenGLWidgets)

if(CMAKE_HOST_WIN32)
  target_include_directories(qmpv PRIVATE "C:\\3rd\\x64\\mpv\\include")
  target_link_libraries(qmpv PRIVATE C:\\3rd\\x64\\mpv\\libmpv.dll.a)
elseif(CMAKE_HOST_APPLE)
  target_include_directories(qmpv PRIVATE "/opt/homebrew/include")
  target_link_directories(qmpv PRIVATE "/opt/homebrew/lib")
  target_link_libraries(qmpv PRIVATE mpv)
elseif(CMAKE_HOST_LINUX)
  target_link_libraries(qmpv PRIVATE mpv)
endif()

if(CMAKE_HOST_WIN32)
  target_compile_definitions(qmpv PRIVATE "MPV_LIBRARY")
endif()
