add_subdirectory(ffmpegplayer)
add_subdirectory(ffmpegtranscoder)
add_subdirectory(qplayer)

if(BUILD_MPV)
  add_subdirectory(mpvplayer)
endif()
