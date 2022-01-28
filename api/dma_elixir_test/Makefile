#======================================================================
# Makefile для ручной сборки.
#======================================================================

TARGET     = dma_elixir_test
DESTDIR    = .

SDK_PATH   = /opt/radiomodule-sdk
SDK_BINS   = $(SDK_PATH)/bin
SDK_LIBS   = $(SDK_PATH)/arm-buildroot-linux-musleabihf/lib
SDK_INCS   = $(SDK_PATH)/arm-buildroot-linux-musleabihf/include
SDK_PREF   = $(SDK_BINS)/arm-linux-

SHELL      = /bin/bash
DEFINES    =
CFLAGS     = -O2 -Wall -Wextra $(DEFINES)
CXXFLAGS   = -std=gnu++1z -std=gnu++17 $(CFLAGS)
INCPATH    = -I. -I$(SDK_INCS)
LIBS       = -L./libs -L$(SDK_LIBS) 
LFLAGS     = -O1 $(CXXFLAGS)

SDK_GCC    = $(SDK_PREF)gcc    $(CFLAGS)   $(LIBS) $(INCPATH)
SDK_GXX    = $(SDK_PREF)g++ -c $(CXXFLAGS) $(LIBS) $(INCPATH)
SDK_LINK   = $(SDK_PREF)g++    $(LFLAGS)   $(LIBS) $(INCPATH)

#======================================================================

OBJECTS = 

#======================================================================

first: all

all: distclean \
     application

application: $(OBJECTS)
	$(SDK_LINK) -o $(DESTDIR)/$(TARGET) $(OBJECTS) main.cpp

clean:
	rm -f $(OBJECTS)

distclean: clean
	rm -f $(DESTDIR)/$(TARGET)

#======================================================================

#======================================================================
