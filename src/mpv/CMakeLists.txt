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

add_custom_library(custommpv ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(custommpv PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)

if(CMAKE_HOST_WIN32)
  target_include_directories(custommpv PRIVATE "C:\\3rd\\x64\\mpv\\include")
  target_link_libraries(custommpv PRIVATE C:\\3rd\\x64\\mpv\\libmpv.dll.a)
elseif(CMAKE_HOST_APPLE)
  target_include_directories(custommpv PRIVATE "/usr/local/include")
  target_link_directories(custommpv PRIVATE /usr/lib)
  target_link_directories(custommpv PRIVATE /usr/local/lib)
  target_link_libraries(custommpv PRIVATE mpv)
elseif(CMAKE_HOST_LINUX)
  target_link_libraries(custommpv PRIVATE mpv)
endif()

if(CMAKE_HOST_WIN32)
  target_compile_definitions(custommpv PRIVATE "MPV_LIBRARY")
endif()
