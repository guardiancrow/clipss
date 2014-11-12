### for MinGW32
### パスは各自書き直してください
### Copyright (C) 2014, guardiancrow

CXX = g++
SRC = clipss.cpp pngutil.cpp jpegutil.cpp

OUTPUT_DIR = ./out
OBJS = $(SRC:%.cpp=$(OUTPUT_DIR)/%.o)
TARGET = $(OUTPUT_DIR)/clipss.exe

ZLIBINC = D:/lib/zlib
LIBPNGINC = D:/lib/libpng12
LIBJPEGINC = D:/lib/mozjpeg

ZLIBLIB = D:/lib/zlib
LIBPNGLIB = D:/lib/libpng12
LIBJPEGLIB = D:/lib/mozjpeg/sharedlib

LIBPNGDLL = libpng12.dll
LIBJPEGDLL = libjpeg-62.dll

CXXFLAGS = -I$(ZLIBINC) -I$(LIBPNGINC) -I$(LIBJPEGINC) -Wall -O3 -DUSE_MOZJPEG #-D_DEBUG
LIBS = -L$(ZLIBLIB) -L$(LIBPNGLIB) -L$(LIBJPEGLIB) -lpng -lz -ljpeg -lrpcrt4 -mwindows

$(TARGET): $(OBJS) dllcopy
	@if [ ! -d $(OUTPUT_DIR) ]; then \
		echo "mkdir $(OUTPUT_DIR)"; \
		mkdir $(OUTPUT_DIR); \
	fi
	$(CXX) $(OBJS) $(LIBS) -o $@

$(OUTPUT_DIR)/%.o: %.cpp
	@if [ ! -d $(OUTPUT_DIR) ]; then \
		echo "mkdir $(OUTPUT_DIR)"; \
		mkdir $(OUTPUT_DIR); \
	fi
	$(CXX) $(CXXFLAGS) -o $@ -c $<

dllcopy:
	@if [ ! -d $(OUTPUT_DIR) ]; then \
		echo "mkdir $(OUTPUT_DIR)"; \
		mkdir $(OUTPUT_DIR); \
	fi
	cp $(LIBPNGLIB)/$(LIBPNGDLL) $(OUTPUT_DIR)/$(LIBPNGDLL)
	cp $(LIBJPEGLIB)/$(LIBJPEGDLL) $(OUTPUT_DIR)/$(LIBJPEGDLL)

clean:
	rm $(TARGET)
	rm $(OBJS)
	rm $(OUTPUT_DIR)/$(LIBPNGDLL)

