ARCH ?= $(shell uname -m)

USE_MINGW ?= 0

ifeq ($(USE_MINGW),1)
  ifeq ($(ARCH),x86_64)
    CXX = x86_64-w64-mingw32-g++
    STRIP = x86_64-w64-mingw32-strip
    OPENCL_DIR = lib/OpenCL-SDK-v2024.10.24-Win-x64
  else ifeq ($(ARCH),i686)
    CXX = i686-w64-mingw32-g++
    STRIP = i686-w64-mingw32-strip
    OPENCL_DIR = lib/OpenCL-SDK-v2024.10.24-Win-x86
  else
    $(error Unsupported ARCH $(ARCH))
  endif

  CFLAGS = -std=c++17 -O2 -I/usr/$(ARCH)-w64-mingw32/include -I$(OPENCL_DIR)/include
  LDFLAGS = -L$(OPENCL_DIR)/bin -lOpenCL -static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
  OUT_EXT = .exe
else
  CXX = g++
  STRIP = strip
  OPENCL_INC ?= /usr/include/CL
  CFLAGS = -std=c++17 -O2 -I$(OPENCL_INC)
  LDFLAGS = -lOpenCL
  OUT_EXT =
endif

SRC = main.cpp
OUT_DIR = bin/$(ARCH)
OUT = $(OUT_DIR)/gpu-pcie-bench$(OUT_EXT)

all: $(OUT)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT): $(SRC) | $(OUT_DIR)
	$(CXX) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)
	$(STRIP) $(OUT)

clean:
	rm -f bin/*/gpu-pcie-bench.exe bin/*/gpu-pcie-bench
