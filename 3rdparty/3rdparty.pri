win32{
    vcpkg_path = C:/vcpkg
    contains(QT_ARCH, i386) {
        arch = x86-windows
    }else{
        arch = x64-windows
    }
}

macx{
    vcpkg_path = /usr/local/share/vcpkg
    arch = x64-osx

LIBS += \
#    -framework Foundation \
#    -framework CoreFoundation \
#    -framework AVFoundation \
#    -framework OpenGL \
#    -framework AppKit \
#    -framework QuartzCore \
#    -framework Cocoa \
#    -framework VideoDecodeAcceleration \
    -framework Security \
    -framework AudioToolBox \
    -framework VideoToolBox \
    -framework CoreVideo \
    -framework CoreMedia
}

unix:!macx{
    vcpkg_path = /usr/local/share/vcpkg
    arch = x64-linux
}

CONFIG(debug, debug|release) {
    LIBS += -L$$vcpkg_path/installed/$$arch/debug/lib \
            -llibbreakpad_clientd -llibbreakpadd
}else{
    LIBS += -L$$vcpkg_path/installed/$$arch/lib \
            -llibbreakpad_client -llibbreakpad
}

LIBS += -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil \
        -lass -lharfbuzz -lfribidi

CONFIG(debug, debug|release) {
    LIBS += -lfreetyped -llibpng16d -lzlibd -lbz2d -lbrotlidec -lbrotlicommon
}else{
    LIBS += -lfreetype -llibpng16 -lzlib -lbz2 -lbrotlidec -lbrotlicommon
}

INCLUDEPATH += $$vcpkg_path/installed/$$arch/include
