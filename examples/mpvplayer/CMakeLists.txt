set(PROJECT_SOURCES
    ../common/controlwidget.cc
    ../common/controlwidget.hpp
    ../common/equalizerdialog.cpp
    ../common/equalizerdialog.h
    ../common/openwebmediadialog.cc
    ../common/openwebmediadialog.hpp
    ../common/playlistmodel.cpp
    ../common/playlistmodel.h
    ../common/playlistview.cc
    ../common/playlistview.hpp
    ../common/qmediaplaylist_p.h
    ../common/qmediaplaylist.cpp
    ../common/qmediaplaylist.h
    ../common/qplaylistfileparser_p.h
    ../common/qplaylistfileparser.cpp
    ../common/slider.cpp
    ../common/slider.h
    ../common/titlewidget.cc
    ../common/titlewidget.hpp
    main.cc
    mainwindow.cc
    mainwindow.hpp
    mpvlogwindow.cc
    mpvlogwindow.hpp
    subtitledelaydialog.cc
    subtitledelaydialog.hpp)

qt_add_executable(MpvPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_compile_definitions(MpvPlayer PRIVATE "MPV_ON")
target_link_libraries(
  MpvPlayer
  PRIVATE qmpv
          mediaconfig
          thirdparty
          dump
          utils
          Qt6::Widgets
          Qt6::Multimedia
          Qt6::Network
          Qt6::OpenGLWidgets)
if(CMAKE_HOST_WIN32)
  target_link_libraries(MpvPlayer PRIVATE C:\\3rd\\x64\\mpv\\libmpv.dll.a)
  file(COPY C:\\3rd\\x64\\mpv\\libmpv-2.dll
       DESTINATION ${EXECUTABLE_OUTPUT_PATH}/)
elseif(CMAKE_HOST_APPLE)
  target_link_directories(MpvPlayer PRIVATE "/opt/homebrew/lib")
  target_link_libraries(MpvPlayer PRIVATE mpv)
elseif(CMAKE_HOST_LINUX)
  target_link_libraries(MpvPlayer PRIVATE mpv)
endif()

qt_finalize_executable(MpvPlayer)
