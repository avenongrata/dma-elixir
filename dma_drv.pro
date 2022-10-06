TEMPLATE = subdirs
SUBDIRS += \
    api/dma_elixir_speed_test \
    api/dma_elixir_test \
    api/dma_elixir_udp_test \
    xilinx \
    api/udp-echo-server \
    argument-parser \
    debug \
    debug/kernel

api/dma_elixir_speed_test.file = \
    api/dma_elixir_speed_test/dma_elixir_speed_test.pro

api/dma_elixir_test.file = \
    api/dma_elixir_test/dma_elixir_test.pro

api/dma_elixir_udp_test = \
    api/dma_elixir_udp_test/dma_elixir_udp_test.pro

xilinx.file = \
    xilinx/xilinx_dma.pro

api/udp-echo-server.file = \
    api/udp-echo-server/udp_echo_server.pro

argument-parser.file = \
    argument-parser/argument_parser.pro

debug.file = \
    debug/dma_debug.pro

debug/kernel.file = \
    debug/kernel/mem_module.pro

ARCH         = arm64
SRC_PATH     = $$system(pwd)
HEADERS_PATH = /usr/src/linux-headers-$$system(uname -r)

SOURCES     += dma_elixir.c

HEADERS     +=

OTHER_FILES += Makefile

INCLUDEPATH += $$system(find -L $$SRC_PATH -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/include -type d)
INCLUDEPATH += $$system(find -L $$HEADERS_PATH/arch/$$ARCH/include -type d)
