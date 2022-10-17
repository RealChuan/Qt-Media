set(PROJECT_SOURCES
        breakpad.cc
        breakpad.hpp
        crashhandler_global.h
        crashhandler.cpp
        crashhandler.h)

add_library(crashhandler SHARED ${PROJECT_SOURCES})
target_link_libraries(crashhandler PRIVATE Qt6::Widgets utils 
    unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client)

if (WIN32)
  target_compile_definitions(crashhandler PRIVATE "CRASHHANDLER_LIBRARY")
endif()
