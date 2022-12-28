win32{
    contains(QT_ARCH, i386) {
        vcpkg_path = C:/vcpkg/installed/x86-windows
    }else{
        vcpkg_path = C:/vcpkg/installed/x64-windows
    }
}

macx{
    vcpkg_path = /usr/local/share/vcpkg/installed/x64-osx
}

unix:!macx{
    vcpkg_path = /usr/local/share/vcpkg/installed/x64-linux
}

CONFIG(debug, debug|release) {
    LIBS += -L$$vcpkg_path/debug/lib \
            -llibbreakpad_clientd -llibbreakpadd
}else{
    LIBS += -L$$vcpkg_path/lib \
            -llibbreakpad_client -llibbreakpad
}

LIBS += -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil \
        -lass -lharfbuzz -lfribidi

win32{

CONFIG(debug, debug|release) {
    LIBS += -lfreetyped -llibpng16d -lzlibd -lbz2d -lbrotlidec -lbrotlienc -lbrotlicommon
}else{
    LIBS += -lfreetype -llibpng16 -lzlib -lbz2 -lbrotlidec -lbrotlienc -lbrotlicommon
}

}

unix:!macx{
LIBS += -lfontconfig -lexpat
}

unix{

CONFIG(debug, debug|release) {
    LIBS += -lfreetyped -lpng -lz -lbz2d -lbrotlidec-static -lbrotlienc-static -lbrotlicommon-static
}else{
    LIBS += -lfreetype -lpng -lz -lbz2 -lbrotlidec-static -lbrotlienc-static -lbrotlicommon-static
}

}

INCLUDEPATH += $$vcpkg_path/include

macx{
LIBS += \
    -framework Foundation \
    -framework CoreAudio \
    -framework AVFoundation \
    -framework CoreGraphics \
    -framework OpenGL \
    -framework CoreText \
    -framework CoreImage \
    -framework AppKit \
    -framework Security \
    -framework AudioToolBox \
    -framework VideoToolBox \
    -framework CoreFoundation \
    -framework CoreMedia \
    -framework CoreVideo \
    -framework CoreServices
#    -framework QuartzCore \
#    -framework Cocoa \
#    -framework VideoDecodeAcceleration
}
