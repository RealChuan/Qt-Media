TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    utils \
    dump \
    3rdparty \
    ffmpeg

win32 {
    exists("C:/3rd/x64/mpv/include"){
        SUBDIRS += mpv
    }
}

macx {
    exists("/opt/homebrew/include/mpv"){
        SUBDIRS += mpv
    }
}

unix:!macx{
    SUBDIRS += mpv
}
