### for MinGW32+msys sh
### パスは適宜書き直してください
### Copyright (C) 2014 - 2016, guardiancrow

# mozjpegを使うときはUSE_MOZJPEG=1をmakeの引数で与えるか下の行を有効にしてください
# USE_MOZJPEG = 1

### ここから適宜変更
# zlib zopfli libpng-1.6.x ( libjpeg-9a | mozjpeg ) のソースコードを展開したディレクトリを指定してください
ZLIBDIR = D:/src/zlib
LIBPNGDIR = D:/src/libpng16
ZOPFLIDIR = D:/src/zopfli

ifdef USE_MOZJPEG
	LIBJPEGDIR = D:/src/mozjpeg
else
	LIBJPEGDIR = D:/src/jpeg-9b
endif
### ここまで適宜変更


CXX = g++
WINDRES = windres
SRC = clipss.cpp pngutil.cpp jpegutil.cpp optiondialog.cpp PNGParser.cpp pngrecomp.cpp
RC = clipss_res.rc

OUTPUT_DIR = ./out
RELEASE_DIR = ./release
OBJS = $(SRC:%.cpp=$(OUTPUT_DIR)/%.o)
RES = $(RC:%.rc=$(OUTPUT_DIR)/%.o)
TARGET = $(RELEASE_DIR)/clipss.exe

ZLIBINC = $(ZLIBDIR)
LIBPNGINC = $(LIBPNGDIR)
LIBJPEGINC = $(LIBJPEGDIR)

ZLIBLIB = $(ZLIBDIR)
LIBPNGDLL = libpng16.dll
LIBPNGBUILDDIR = $(OUTPUT_DIR)/libpngbuild
LIBPNGLIB = $(LIBPNGBUILDDIR)

ZOPFLIINC = $(ZOPFLIDIR)/src/zopfli
ZOPFLIBUILDDIR =  $(OUTPUT_DIR)/zopfli
ZOPFLILIB = $(ZOPFLIBUILDDIR)

ifdef USE_MOZJPEG
	LIBJPEGBUILDDIR = $(OUTPUT_DIR)/libjpegbuild
	LIBJPEGLIB = $(LIBJPEGBUILDDIR)/sharedlib
	LIBJPEGDLL = libjpeg-62.dll
	CXXFLAGS = -I$(ZLIBINC) -I$(LIBPNGINC) -I$(LIBPNGBUILDDIR) -I$(ZOPFLIINC) -I$(LIBJPEGINC) -I$(LIBJPEGBUILDDIR) --input-charset=utf-8 --exec-charset=CP932 -Wall -O3 -DUSE_MOZJPEG
else
	LIBJPEGBUILDDIR = $(LIBJPEGDIR)
	LIBJPEGLIB = $(LIBJPEGBUILDDIR)/.libs
	LIBJPEGDLL = libjpeg-9.dll
	CXXFLAGS = -I$(ZLIBINC) -I$(LIBPNGINC) -I$(LIBPNGBUILDDIR) -I$(ZOPFLIINC) -I$(LIBJPEGINC) --input-charset=utf-8 --exec-charset=CP932 -Wall -O3
endif

LIBS = -L$(ZLIBLIB) -L$(LIBPNGLIB) -L$(LIBJPEGLIB) -L$(ZOPFLILIB) -lpng16.dll -lzopfli.dll -lz -ljpeg -lrpcrt4 -lshlwapi -mwindows

C11FLAGS = -std=c++11 -U__STRICT_ANSI__


##########
##########


define create_dir
	@if [ ! -d $1 ]; then \
		echo "mkdir $1"; \
		mkdir $1; \
	fi
endef

all: zopfli libpng libjpeg exe readme

exe: $(TARGET)

$(TARGET): $(OBJS) $(RES)
	$(call create_dir, $(RELEASE_DIR))
	$(CXX) $(OBJS) $(RES) $(LIBS) -o $@

$(OUTPUT_DIR)/PNGParser.o: PNGParser.cpp
	$(call create_dir, $(OUTPUT_DIR))
	$(CXX) $(CXXFLAGS) $(C11FLAGS) -o $@ -c $<

$(OUTPUT_DIR)/pngrecomp.o: pngrecomp.cpp
	$(call create_dir, $(OUTPUT_DIR))
	$(CXX) $(CXXFLAGS) $(C11FLAGS) -o $@ -c $<

$(OUTPUT_DIR)/%.o: %.cpp
	$(call create_dir, $(OUTPUT_DIR))
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(RES): $(RC) resource.h
	$(call create_dir, $(OUTPUT_DIR))
	$(WINDRES) $(RC) $(RES)

readme:
	cp clipss_ini_readme.txt $(RELEASE_DIR)/clipss_ini_readme.txt
	cp readme_binary.txt $(RELEASE_DIR)/readme.txt

zopfli:
	$(call create_dir, $(OUTPUT_DIR))
	$(call create_dir, $(ZOPFLIBUILDDIR))
	make -f makefile_zopfli libzopfli
	cp $(ZOPFLIBUILDDIR)/libzopfli.dll $(RELEASE_DIR)/libzopfli.dll
	cp $(ZOPFLIDIR)/COPYING $(RELEASE_DIR)/zopfli-COPYING.txt

libpng:
	$(call create_dir, $(RELEASE_DIR))
	$(call create_dir, $(OUTPUT_DIR))
	$(call create_dir, $(LIBPNGBUILDDIR))
	@cd $(ZLIBDIR); make -f win32/Makefile.gcc libz.a
	@cd $(LIBPNGBUILDDIR); cmake $(LIBPNGDIR) -G "MSYS Makefiles" -DZLIB_INCLUDE_DIR=$(ZLIBDIR) -DZLIB_LIBRARY=$(ZLIBDIR)/libz.a -DCMAKE_POLICY_DEFAULT_CMP0026=OLD; make png
	cp $(LIBPNGLIB)/$(LIBPNGDLL) $(RELEASE_DIR)/$(LIBPNGDLL)
	cp $(ZLIBDIR)/README $(RELEASE_DIR)/zlib-README.txt
	cp $(LIBPNGDIR)/LICENSE $(RELEASE_DIR)/libpng-LICENSE.txt

libjpeg:
	$(call create_dir, $(RELEASE_DIR))
	$(call create_dir, $(OUTPUT_DIR))
ifdef USE_MOZJPEG
	$(call create_dir, $(LIBJPEGBUILDDIR))
	@cd $(LIBJPEGBUILDDIR); cmake $(LIBJPEGDIR) -G "MSYS Makefiles"; make jpeg
	cp $(LIBJPEGDIR)/LICENSE.txt $(RELEASE_DIR)/mozjpeg-LICENSE.txt
else
	@cd $(LIBJPEGDIR); autoheader; ./configure; make libjpeg.la
	cp $(LIBJPEGDIR)/README $(RELEASE_DIR)/libjpeg-README.txt
endif
	cp $(LIBJPEGLIB)/$(LIBJPEGDLL) $(RELEASE_DIR)/$(LIBJPEGDLL)

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
	rm -f $(RES)
	rm -f $(RELEASE_DIR)/clipss_ini_readme.txt
	rm -f $(RELEASE_DIR)/readme.txt
	rm -f $(RELEASE_DIR)/$(LIBPNGDLL)
	rm -f $(RELEASE_DIR)/$(LIBJPEGDLL)
	rm -f $(RELEASE_DIR)/zlib-README.txt
	rm -f $(RELEASE_DIR)/libzopfli.dll
	rm -f $(RELEASE_DIR)/zopfli-COPYING.txt
	make -f makefile_zopfli clean
	rm -f $(RELEASE_DIR)/libpng-LICENSE.txt
	@cd $(ZLIBDIR); make clean -f win32/Makefile.gcc
	rm -fR $(LIBPNGBUILDDIR)
ifdef USE_MOZJPEG
	rm -fR $(LIBJPEGBUILDDIR)
	rm -f $(RELEASE_DIR)/mozjpeg-LICENSE.txt
else
	@cd $(LIBJPEGDIR); make clean
	rm -f $(RELEASE_DIR)/libjpeg-README.txt
endif

