TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    utils \
    ffmpeg \
    mainwindow \
    app

TRANSLATIONS += \
    translations/language.zh_cn.ts \
    translations/language.zh_en.ts

message("1.Build;")
message("2.Copy the file in the bin/ directory to the executable program directory;")
message("3.Run the program.");
