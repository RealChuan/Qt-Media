set(PROJECT_SOURCES
    boundedblockingqueue.hpp
    countdownlatch.cc
    countdownlatch.hpp
    fps.cc
    fps.hpp
    hostosinfo.cpp
    hostosinfo.h
    logasync.cpp
    logasync.h
    osspecificaspects.h
    singleton.hpp
    threadsafequeue.hpp
    utils_global.h
    utils.cpp
    utils.h)

add_custom_library(utils ${PROJECT_SOURCES} ${SOURCES})
target_link_libraries(utils PRIVATE Qt6::Widgets Qt6::Core5Compat)

if(CMAKE_HOST_WIN32)
  target_compile_definitions(utils PRIVATE "UTILS_LIBRARY")
endif()
