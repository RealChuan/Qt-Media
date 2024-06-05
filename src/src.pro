TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    utils \
    dump \
    3rdparty \
    ffmpeg

contains(CONFIG, BUILD_MPV) {
    SUBDIRS += mpv
}
