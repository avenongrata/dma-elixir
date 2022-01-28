CFLAGS_dma_elixir.o := -DDEBUG
SDK_PATH = /opt/radiomodule-sdk
SDK_BIN = $(SDK_PATH)/bin
SDK_ARCH = arm
CROSS_COMPILE = arm-linux-

SDK_LINUX = $(SDK_PATH)/src/linux
SDK_LINUX_INC = $(SDK_LINUX)/include

SDK_PATHS = $(PATH):$(SDK_BIN)
SDK_INCS = -I . -I $(SDK_LINUX_INC)
SDK_CROSS = ARCH=$(SDK_ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
SDK_PARAMS = $(SDK_CROSS) -C $(SDK_LINUX) $(SDK_INCS) M=$(PWD)

TARGET = dma_elixir
obj-m := $(TARGET).o

all: distclean
	PATH=$(SDK_PATHS) make $(SDK_PARAMS) modules
clean:
	PATH=$(SDK_PATHS) make $(SDK_PARAMS) clean

distclean: clean
	rm -f $(TARGET)

