vcpkg_path = D:/Mine/CODE/vcpkg

contains(QT_ARCH, i386) {
    ffmpeg_arch = ffmpeg_x86-windows
}else{
    ffmpeg_arch = ffmpeg_x64-windows
}

CONFIG(debug, debug|release) {
    LIBS += -L$$vcpkg_path/packages/$$ffmpeg_arch/debug/lib
}else{
    LIBS += -L$$vcpkg_path/packages/$$ffmpeg_arch/lib
}

LIBS += -lavutil -lavformat -lavcodec -lavdevice -lavfilter -lswresample -lswscale

INCLUDEPATH += $$vcpkg_path/packages/$$ffmpeg_arch/include
