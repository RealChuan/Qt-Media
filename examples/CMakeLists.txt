add_subdirectory(ffmpegplayer)
add_subdirectory(ffmpegtranscoder)

if(BUILD_MPV)
  add_subdirectory(mpvplayer)
endif()
