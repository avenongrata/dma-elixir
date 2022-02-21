TEMPLATE = subdirs
SUBDIRS += \
    api/dma_elixir_speed_test \
    api/dma_elixir_test \
    api/dma_elixir_udp_test \
    xilinx \
    udp-echo-server \
    argument-parser \
    debug

api/dma_elixir_speed_test.file = \
    api/dma_elixir_speed_test/dma_elixir_speed_test.pro

api/dma_elixir_test.file = \
    api/dma_elixir_test/dma_elixir_test.pro

api/dma_elixir_udp_test = \
    api/dma_elixir_udp_test/dma_elixir_udp_test.pro

xilinx.file = \
    xilinx/xilinx_dma.pro

udp-echo-server.file = \
    udp-echo-server/udp_echo_server.pro

argument-parser.file = \
    argument-parser/argument_parser.pro

debug.file = \
    debug/dma_debug.pro

ARCH         = arm64
SRC_PATH     = $$system(pwd)
HEADERS_PATH = /usr/src/linux-headers-$$system(uname -r)

SOURCES     += dma_elixir.c

HEADERS     +=

OTHER_FILES += Makefile

INCLUDEPATH += $$system(find -L $$SRC_PATH -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/arch/$$ARCH/include -type d)

#------------------------------------------------------------------------------
#INCLUDEPATH += \
#    /opt/radiomodule-sdk/arm-buildroot-linux-musleabihf/include \
#    /opt/radiomodule-sdk/arm-buildroot-linux-musleabihf/sysroot/usr/include \
#    /opt/radiomodule-sdk/src/linux/arch/arm/include \
#    /opt/radiomodule-sdk/src/linux/include
