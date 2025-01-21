cmake_minimum_required(VERSION 3.25.1)

include(cmake/vcpkg.cmake)
include(cmake/qt.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(utils)
include(qt_utils)

project(
  Qt-Media
  VERSION 0.0.1
  DESCRIPTION
    " A Qt-powered multimedia suite featuring FFmpeg and mpv for enhanced video playback and transcoding."
  HOMEPAGE_URL "https://github.com/RealChuan/Qt-Media"
  LANGUAGES CXX)

option(BUILD_MPV "build mpv" ON)

include(cmake/common.cmake)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core5Compat Concurrent
                                     Multimedia OpenGLWidgets)

qt_standard_project_setup()
# qt_standard_project_setup will set CMAKE_RUNTIME_OUTPUT_DIRECTORY, we need to
# set it back, and use EXECUTABLE_OUTPUT_PATH
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

find_package(unofficial-breakpad CONFIG REQUIRED)
if(unofficial-breakpad_FOUND)
  message(STATUS "found unofficial-breakpad")
endif()
find_package(crashpad CONFIG REQUIRED)
if(crashpad_FOUND)
  message(STATUS "found crashpad")
endif()

find_package(PkgConfig REQUIRED)
if(PkgConfig_FOUND)
  message(STATUS "found PkgConfig")
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
endif()

pkg_check_modules(
  ffmpeg
  REQUIRED
  IMPORTED_TARGET
  libavdevice
  libavfilter
  libavformat
  libavcodec
  libswresample
  libswscale
  libavutil)
pkg_check_modules(ass REQUIRED IMPORTED_TARGET libass)

if(CMAKE_HOST_LINUX)
  pkg_check_modules(fontconfig REQUIRED IMPORTED_TARGET fontconfig)
  find_package(expat CONFIG REQUIRED)
  if(expat_FOUND)
    message(STATUS "found expat")
  endif()
endif()

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

if(CMAKE_HOST_APPLE)
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
endif()

include_directories(src)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)

include(cmake/build_info.cmake)
