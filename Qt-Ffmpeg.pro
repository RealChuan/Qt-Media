TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    utils \
    crashhandler\
    ffmpeg \
    tests \
    examples

TRANSLATIONS += \
    translations/language.zh_cn.ts \
    translations/language.zh_en.ts

DISTFILES += \
    $$files(docs/*.png)\
    LICENSE \
    README.md
