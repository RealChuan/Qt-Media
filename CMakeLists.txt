cmake_minimum_required(VERSION 3.25.1)

include(cmake/macOSDeploymentTarget.cmake)
include(cmake/VcpkgToolchain.cmake)
include(cmake/QtSetup.cmake)

project(
  Qt-Media
  VERSION 0.0.1
  DESCRIPTION
    " A Qt-powered multimedia suite featuring FFmpeg and mpv for enhanced video playback and transcoding."
  HOMEPAGE_URL "https://github.com/RealChuan/Qt-Media"
  LANGUAGES CXX)

option(BUILD_MPV "build mpv" ON)

include(cmake/ProjectSettings.cmake)
include(cmake/InstallConfig.cmake)
include(cmake/PlatformLibraries.cmake)
include(cmake/InstallCrashpad.cmake)

find_package(unofficial-breakpad CONFIG REQUIRED)
if(unofficial-breakpad_FOUND)
  message(STATUS "found unofficial-breakpad")
endif()
find_package(crashpad CONFIG REQUIRED)
if(crashpad_FOUND)
  message(STATUS "found crashpad")
endif()
find_package(FFMPEG REQUIRED)
if(FFMPEG_FOUND)
  message(STATUS "found FFMPEG")
endif()
find_package(tl-expected CONFIG REQUIRED)
if(tl-expected_FOUND)
  message(STATUS "found tl-expected")
endif()

find_package(PkgConfig REQUIRED)
if(PkgConfig_FOUND)
  message(STATUS "found PkgConfig")
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

  # 仅在 macOS 下处理 Homebrew 的 libunibreak
  if(APPLE)
    find_program(BREW_EXECUTABLE brew)
    if(BREW_EXECUTABLE)
      execute_process(
        COMMAND ${BREW_EXECUTABLE} --prefix libunibreak
        OUTPUT_VARIABLE LIBUNIBREAK_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )
    endif()

    # 如果 brew 没找到，回退到常见路径
    if(NOT LIBUNIBREAK_PREFIX)
      if(EXISTS "/opt/homebrew/opt/libunibreak")
        set(LIBUNIBREAK_PREFIX "/opt/homebrew/opt/libunibreak")
      elseif(EXISTS "/usr/local/opt/libunibreak")
        set(LIBUNIBREAK_PREFIX "/usr/local/opt/libunibreak")
      endif()
    endif()

    if(LIBUNIBREAK_PREFIX AND EXISTS "${LIBUNIBREAK_PREFIX}/lib/pkgconfig/libunibreak.pc")
      set(ENV{PKG_CONFIG_PATH} "${LIBUNIBREAK_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
      message(STATUS "Added Homebrew libunibreak pkgconfig: ${LIBUNIBREAK_PREFIX}/lib/pkgconfig")
    else()
      message(WARNING "libunibreak.pc not found via Homebrew; pkg_check_modules(libass) may fail.")
    endif()
  endif()
endif()

pkg_check_modules(ass REQUIRED IMPORTED_TARGET libass)

find_package(
  Qt6 REQUIRED
  COMPONENTS Widgets
             Gui
             GuiPrivate
             Network
             Core5Compat
             Concurrent
             Multimedia
             OpenGLWidgets)

qt_standard_project_setup()

include(cmake/QtUtilities.cmake)

include_directories(src)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)

include(cmake/CMakeDebugInfo.cmake)
