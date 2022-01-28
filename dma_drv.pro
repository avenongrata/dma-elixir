TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

ARCH=arm64
SRC_PROJECT_PATH = $$system(pwd)
LINUX_HEADERS_PATH = /usr/src/linux-headers-$$system(uname -r)

SOURCES += $$system(find -L $$SRC_PROJECT_PATH -type f -name ".c" -o -name ".S" )
HEADERS += $$system(find -L $$SRC_PROJECT_PATH -type f -name ".h" )
OTHER_FILES += $$system(find -L $$SRC_PROJECT_PATH -type f -not -name ".h" -not -name ".c" -not -name ".S" )

INCLUDEPATH += $$system(find -L $$SRC_PROJECT_PATH -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$LINUX_HEADERS_PATH/arch/$$ARCH/include -type d)

SOURCES += \
        dma_elixir.c \
