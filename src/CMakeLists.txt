add_subdirectory(utils)
add_subdirectory(dump)
add_subdirectory(3rdparty)
add_subdirectory(mediaconfig)
add_subdirectory(ffmpeg)

if(BUILD_MPV)
  add_subdirectory(mpv)
endif()
