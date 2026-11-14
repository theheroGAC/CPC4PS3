PS3DEV ?= /mnt/d/ps3dev-sdk
PSL1GHT := $(PS3DEV)
PORTLIBS := $(PS3DEV)/portlibs/ppu

export PS3DEV
export PSL1GHT
export PORTLIBS

CXX := $(PS3DEV)/ppu/bin/ppu-g++
STRIP := $(PS3DEV)/ppu/bin/ppu-strip
SPRX := $(PS3DEV)/bin/sprxlinker
SELF := $(PS3DEV)/bin/make_self

TARGET := cpc_ps3

CAPS_SRCS := \
	src/capsimg/CAPSImg/CapsAPI.cpp \
	src/capsimg/CAPSImg/CapsFDCEmulator.cpp \
	src/capsimg/CAPSImg/CapsFile.cpp \
	src/capsimg/CAPSImg/CapsFormatMFM.cpp \
	src/capsimg/CAPSImg/CapsImage.cpp \
	src/capsimg/CAPSImg/CapsImageStd.cpp \
	src/capsimg/CAPSImg/CapsLoader.cpp \
	src/capsimg/CAPSImg/DiskImage.cpp \
	src/capsimg/CAPSImg/DiskImageFactory.cpp \
	src/capsimg/CAPSImg/StreamCueImage.cpp \
	src/capsimg/CAPSImg/StreamImage.cpp \
	src/capsimg/Codec/CTRawCodec.cpp \
	src/capsimg/Codec/CTRawCodecDecompressor.cpp \
	src/capsimg/Codec/DiskEncoding.cpp \
	src/capsimg/Core/BaseFile.cpp \
	src/capsimg/Core/BitBuffer.cpp \
	src/capsimg/Core/CRC.cpp \
	src/capsimg/Core/DiskFile.cpp \
	src/capsimg/Core/MemoryFile.cpp

CORE_SRCS := $(wildcard src/cap32/*.cpp)
PS3_SRCS := $(wildcard src/ps3/*.cpp)
SRCS := $(CORE_SRCS) $(PS3_SRCS) $(CAPS_SRCS)
PREBUILT_OBJS := src/ps3/prebuilt/SDL_PSL1GHTrender.o
OBJS := $(patsubst %.cpp,build/%.o,$(SRCS)) $(PREBUILT_OBJS)

INCLUDES := -Isrc/cap32 -Isrc/ps3 -Isrc/capsimg/CAPSImg -Isrc/capsimg/LibIPF \
	-Isrc/capsimg/Core -Isrc/capsimg/Codec -Isrc/capsimg/Device \
	-I$(PS3DEV)/ppu/include -I$(PS3DEV)/ppu/ppu/include \
	-I$(PORTLIBS)/include -I$(PORTLIBS)/include/SDL2 -I$(PORTLIBS)/include/freetype2
DEFS := -D__PS3__ -D__PSL1GHT__ -DPS3_PORT -DSDL_MAIN_HANDLED -DDllExport= -DDllImport= -D__cdecl=
CXXFLAGS := -O2 -mcpu=cell -m64 -std=gnu++17 -fno-strict-aliasing -fsigned-char \
	-fno-tree-vectorize -fno-tree-slp-vectorize -fcommon \
	-Wno-write-strings -Wno-register $(DEFS) $(INCLUDES)
LDFLAGS := -m64 -L$(PS3DEV)/ppu/lib -L$(PS3DEV)/ppu/ppu/lib -L$(PORTLIBS)/lib
LIBS := -lSDL2_ttf -lfreetype -lpng -lSDL2 -lgcm_sys -lio -laudio -lsysutil -lsysmodule -lrsx -lrt -llv2 -lz -lm -lc

PS3_CXXFLAGS := $(CXXFLAGS) -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-optimize-sibling-calls

.PHONY: all clean self

all: $(TARGET).elf

build/src/ps3/%.o: src/ps3/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX-PS3] $<"
	@$(CXX) $(PS3_CXXFLAGS) -c $< -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	@echo "[LD] $@"
	@$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

self: $(TARGET).elf
	@$(STRIP) $(TARGET).elf -o $(TARGET).stripped.elf
	@$(SPRX) $(TARGET).stripped.elf
	@$(SELF) $(TARGET).stripped.elf $(TARGET).self

clean:
	rm -rf build $(TARGET).elf $(TARGET).stripped.elf $(TARGET).self
