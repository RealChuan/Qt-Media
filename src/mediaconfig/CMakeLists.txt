set(PROJECT_SOURCES equalizer.cc equalizer.hpp mediaconfig_global.hpp)

add_platform_library(mediaconfig ${PROJECT_SOURCES})
target_link_libraries(mediaconfig PRIVATE utils Qt::Core)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(mediaconfig PRIVATE "MEDIACONFIG_LIBRARY")
endif()
