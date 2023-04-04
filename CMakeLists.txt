cmake_minimum_required(VERSION 3.16)

IF (CMAKE_HOST_WIN32)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
ELSEIF (CMAKE_HOST_APPLE)
    set(CMAKE_TOOLCHAIN_FILE "/usr/local/share/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
ELSEIF (CMAKE_HOST_UNIX)
    set(CMAKE_TOOLCHAIN_FILE "/usr/local/share/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
ENDIF ()

IF (CMAKE_HOST_WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:\\Qt\\6.5.0\\msvc2019_64")
ENDIF ()

project(Qt-Ffmpeg VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#qt_standard_project_setup() 这个识别不了

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CURRENT_SOURCE_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX d)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CURRENT_PLATFORM "-64")
else()
    set(CURRENT_PLATFORM "-32")
endif()

message(STATUS "Current Platform is ${CURRENT_PLATFORM}")

#设定可执行二进制文件的目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin${CURRENT_PLATFORM}/${CMAKE_BUILD_TYPE})  #源文件目录
#SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)     #输出文件目录

#设定存放编译出来的库文件的目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin${CURRENT_PLATFORM}/${CMAKE_BUILD_TYPE})
#SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

#并且把该目录设为连接目录
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/bin${CURRENT_PLATFORM}/${CMAKE_BUILD_TYPE})
#LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)

include_directories( ${PROJECT_SOURCE_DIR} )

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core5Compat Concurrent Multimedia OpenGLWidgets)

find_package(unofficial-breakpad CONFIG REQUIRED)
if(unofficial-breakpad_FOUND)
  message(STATUS "found unofficial-breakpad")
endif()

find_package(PkgConfig REQUIRED)
if (PkgConfig_FOUND)
  message(STATUS "found PkgConfig")
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
endif()

pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
pkg_check_modules(ass REQUIRED IMPORTED_TARGET libass)

IF (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  pkg_check_modules(fontconfig REQUIRED IMPORTED_TARGET fontconfig)
  find_package(expat CONFIG REQUIRED)
  if(expat_FOUND)
    message(STATUS "found expat")
  endif()
ENDIF ()

find_package(harfbuzz CONFIG REQUIRED)
if(harfbuzz_FOUND)
  message(STATUS "found harfbuzz")
endif()
pkg_check_modules(fribidi REQUIRED IMPORTED_TARGET fribidi)
find_package(freetype CONFIG REQUIRED)
if(freetype_FOUND)
  message(STATUS "found freetype")
endif()
find_package(PNG REQUIRED)
if(PNG_FOUND)
  message(STATUS "found PNG")
endif()
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  message(STATUS "found ZLIB")
endif()
find_package(BZip2 REQUIRED)
if(BZip2_FOUND)
  message(STATUS "found BZip2")
endif()
find_package(unofficial-brotli CONFIG REQUIRED)
if(unofficial-brotli_FOUND)
  message(STATUS "found unofficial-brotli")
endif()

IF (CMAKE_HOST_APPLE)
    find_library(Foundation_LIBRARY Foundation)
    find_library(CoreAudio_LIBRARY CoreAudio)
    find_library(AVFoundation_LIBRARY AVFoundation)
    find_library(CoreGraphics_LIBRARY CoreGraphics)
    find_library(OpenGL_LIBRARY OpenGL)
    find_library(CoreText_LIBRARY CoreText)
    find_library(CoreImage_LIBRARY CoreImage)
    find_library(AppKit_LIBRARY AppKit)
    find_library(Security_LIBRARY Security)
    find_library(AudioToolBox_LIBRARY AudioToolBox)
    find_library(VideoToolBox_LIBRARY VideoToolBox)
    find_library(CoreFoundation_LIBRARY CoreFoundation)
    find_library(CoreMedia_LIBRARY CoreMedia)
    find_library(CoreVideo_LIBRARY CoreVideo)
    find_library(CoreServices_LIBRARY CoreServices)
ENDIF ()

add_subdirectory(utils)
add_subdirectory(3rdparty)
add_subdirectory(crashhandler)
add_subdirectory(ffmpeg)
add_subdirectory(tests)
add_subdirectory(examples)
