TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    src \
    tests \
    examples

DISTFILES += \
    doc/** \
    .clang* \
    LICENSE \
    README*
