TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    ffmpegplayer \
    ffmpegtranscoder

contains(CONFIG, BUILD_MPV) {
    SUBDIRS += mpvplayer
}
