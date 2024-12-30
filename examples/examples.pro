TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    ffmpegplayer \
    ffmpegtranscoder \
    qplayer

win32 {
    exists("C:/3rd/x64/mpv/include"){
        SUBDIRS += mpvplayer
    }
}

macx {
    exists("/opt/homebrew/include/mpv"){
        SUBDIRS += mpvplayer
    }
}

unix:!macx{
    SUBDIRS += mpvplayer
}
