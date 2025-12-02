find_package(Qt6 REQUIRED COMPONENTS MultimediaWidgets)

set(PROJECT_SOURCES
    ../common/commonstr.hpp
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
    ../common/qmediaplaylist_p.cpp
    ../common/qmediaplaylist_p.h
    ../common/qmediaplaylist.cpp
    ../common/qmediaplaylist.h
    ../common/qplaylistfileparser.cpp
    ../common/qplaylistfileparser.h
    ../common/slider.cpp
    ../common/slider.h
    ../common/titlewidget.cc
    ../common/titlewidget.hpp
    main.cc
    mainwindow.cc
    mainwindow.hpp
    videowidget.cc
    videowidget.hpp)

qt_add_executable(QPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_include_directories(QPlayer PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(
  QPlayer
  PRIVATE mediaconfig
          thirdparty
          dump
          utils
          Qt::MultimediaWidgets
          Qt::Network
          Qt::GuiPrivate)
qt_finalize_executable(QPlayer)
