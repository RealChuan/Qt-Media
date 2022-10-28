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
            -llibbreakpad_clientd \
            -llibbreakpadd
}else{
    LIBS += -L$$vcpkg_path/installed/$$arch/lib \
            -llibbreakpad_client \
            -llibbreakpad
}

win32{
LIBS += -lavutil -lavformat -lavcodec -lavdevice -lavfilter -lswresample -lswscale
}

unix{
# 这个顺序在Linux下非常重要！！！！！！！！！！！！！！！！！！
LIBS += -lswscale -lavdevice -lavformat -lavcodec -lavutil -lavfilter -lswresample
}

INCLUDEPATH += $$vcpkg_path/installed/$$arch/include
