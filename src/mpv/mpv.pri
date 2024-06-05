QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig debug
#PKGCONFIG += mpv

win32 {
    INCLUDEPATH += C:/3rd/x64/mpv/include
    LIBS += C:/3rd/x64/mpv/libmpv.dll.a
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/lib -L/usr/local/lib
    LIBS += -lmpv
}

unix:!macx{
    LIBS += -lmpv
}