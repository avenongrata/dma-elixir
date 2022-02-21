SOURCES += \
    xilinx_dma.c

ARCH         = arm64
SRC_PATH     = $$system(pwd)
HEADERS_PATH = /usr/src/linux-headers-$$system(uname -r)

INCLUDEPATH += $$system(find -L $$SRC_PATH -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/arch/$$ARCH/include -type d)
