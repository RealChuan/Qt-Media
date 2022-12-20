set(PROJECT_SOURCES
        mainwindow_global.h
        mainwindow.cpp mainwindow.h
        slider.cpp slider.h)

if (CMAKE_HOST_WIN32)
  add_library(mainwindow SHARED ${PROJECT_SOURCES} ${SOURCES})
else ()
  add_library(mainwindow ${PROJECT_SOURCES} ${SOURCES})
endif ()
target_link_libraries(mainwindow PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::OpenGLWidgets utils ffmpeg)
target_include_directories(mainwindow PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${AVFILTER_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR}) 
target_link_libraries(mainwindow PRIVATE ${SWSCALE_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY}  ${AVUTIL_LIBRARY} ${AVFILTER_LIBRARY} ${SWRESAMPLE_LIBRARY})

if (CMAKE_HOST_WIN32)
  target_compile_definitions(mainwindow PRIVATE "MAINWINDOW_LIBRARY")
endif()
