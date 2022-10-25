set(PROJECT_SOURCES
        hostosinfo.cpp hostosinfo.h
        logasync.cpp logasync.h
        osspecificaspects.h
        singleton.hpp
        taskqueue.h
        utils_global.h
        utils.cpp utils.h)

add_library(utils SHARED ${PROJECT_SOURCES})
target_link_libraries(utils PRIVATE Qt6::Widgets Qt6::Core5Compat)

if (WIN32)
  target_compile_definitions(utils PRIVATE "UTILS_LIBRARY")
endif()
