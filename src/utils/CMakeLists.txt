set(PROJECT_SOURCES
    boundedblockingqueue.hpp
    concurrentqueue.hpp
    expected.h
    fps.cc
    fps.hpp
    hostosinfo.cpp
    hostosinfo.h
    logasync.cpp
    logasync.h
    logfile.cc
    logfile.hpp
    osspecificaspects.h
    qtcassert.cpp
    qtcassert.h
    range.hpp
    result.cpp
    result.h
    singleton.hpp
    speed.cc
    speed.hpp
    utils_global.h
    utils.cc
    utils.hpp
    utilstr.h)

add_platform_library(utils ${PROJECT_SOURCES})
target_link_libraries(utils PRIVATE Qt::Widgets Qt::Core5Compat tl::expected)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(utils PRIVATE "UTILS_LIBRARY")
endif()
