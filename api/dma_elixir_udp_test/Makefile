#======================================================================
# Makefile для ручной сборки.
#======================================================================

SDK_PATH    = /opt/radiomodule-sdk
SDK_BINS    = $(SDK_PATH)/bin
SDK_SYSROOT = $(SDK_PATH)/arm-buildroot-linux-musleabihf/sysroot
SDK_LIBS    = $(SDK_PATH)/arm-buildroot-linux-musleabihf/lib \
              $(SDK_SYSROOT)/usr/include
SDK_PREF    = arm-linux-
SDK_CROSS   = $(SDK_BINS)/$(SDK_PREF)

SHELL       = /bin/bash
#DEFINES     = -D_REENTRANT
#CFLAGS      = -O2 -Wall -Wextra -fPIC $(DEFINES)
CFLAGS      = -O2 -Wall -Wextra $(DEFINES)
CXXFLAGS    = -std=gnu++1z -std=gnu++17 $(CFLAGS)
INCPATH     = -I.
LIBS        = $(SUBLIBS)
LFLAGS      = -O1

SDK_PATH    = /opt/radiomodule-sdk/bin
SDK_PREF    = arm-linux-
SDK_CROSS   = $(SDK_PATH)/$(SDK_PREF)

SDK_GCC     = $(SDK_CROSS)gcc
SDK_GXX     = $(SDK_CROSS)g++
SDK_LINK    = $(SDK_CROSS)g++

SDK_MAKE    = $(SDK_GCC) $(CFLAGS) $(INCPATH)

#======================================================================

TARGET = udp_dma_client

OBJECTS_DIR = ./objects

OBJECTS = 

#======================================================================

first: all

all: $(OBJECTS) $(TARGET)

clean:
	rm -f $(OBJECTS)

distclean:
	rm -f $(OBJECTS) $(TARGET)

#======================================================================

$(TARGET):
	$(SDK_GCC) $(CFLAGS) -o $(TARGET) udp_dma_client.c

#======================================================================

