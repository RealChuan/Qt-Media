set(PROJECT_SOURCES
    boundedblockingqueue.hpp
    concurrentqueue.hpp
    countdownlatch.cc
    countdownlatch.hpp
    fps.cc
    fps.hpp
    hostosinfo.cpp
    hostosinfo.h
    logasync.cpp
    logasync.h
    logfile.cc
    logfile.hpp
    osspecificaspects.h
    range.hpp
    singleton.hpp
    speed.cc
    speed.hpp
    utils_global.h
    utils.cpp
    utils.h
    utilstr.h)

add_custom_library(utils ${PROJECT_SOURCES})
target_link_libraries(utils PRIVATE Qt::Widgets Qt::Core5Compat)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(utils PRIVATE "UTILS_LIBRARY")
endif()
