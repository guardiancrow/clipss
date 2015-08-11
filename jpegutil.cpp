//JPEGへの書き出しを扱います
//基本的にlibjpegの書き方を踏襲します
//libjpegの他にturboもmozjpegも扱えると思います
//
//ということでCopyrightもModificationsとしておきます
//
//Modifications:
//Copyright (C) 2014 - 2015, guardiancrow

#include "jpegutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "jpeglib.h"

///内部エラー告知用
#define ERROR_INTERNAL 1001

///エラー終了用
#define ERREXIT(cinfo,code)  \
		((cinfo)->err->msg_code = (code), \
		(*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))

#define UCH(x)	((int) (x))
#define GET_2B(array,offset)  ((unsigned int) UCH(array[offset]) + \
		(((unsigned int) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + \
		(((INT32) UCH(array[offset+1])) << 8) + \
		(((INT32) UCH(array[offset+2])) << 16) + \
		(((INT32) UCH(array[offset+3])) << 24))


typedef struct cjpeg_source_struct * cjpeg_source_ptr;

struct cjpeg_source_struct {
	void (*start_input) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);
	JDIMENSION (*get_pixel_rows) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);
	void (*finish_input) (j_compress_ptr cinfo, cjpeg_source_ptr sinfo);

	void *lpBInfo;
	void *lpBm;

	JSAMPARRAY buffer;
	JDIMENSION buffer_height;
};

typedef struct{
	struct cjpeg_source_struct pub;
	j_compress_ptr cinfo;
	JSAMPARRAY colormap;
	jvirt_sarray_ptr whole_image;
	JDIMENSION source_row;
	JDIMENSION row_width;
	int bits_per_pixel;
}dib_source_struct,* dib_source_ptr;

///パレットを読み込み
static void read_colormap (dib_source_ptr sinfo, int cmaplen, int mapentrysize)
{
	int i;
	char *p = (char*)(sinfo->pub.lpBInfo) + 40;

	switch (mapentrysize) {
	case 3:
		for (i = 0; i < cmaplen; i++) {
			sinfo->colormap[2][i] = (JSAMPLE) *p++;
			sinfo->colormap[1][i] = (JSAMPLE) *p++;
			sinfo->colormap[0][i] = (JSAMPLE) *p++;
		}
		break;
	case 4:
		for (i = 0; i < cmaplen; i++) {
			sinfo->colormap[2][i] = (JSAMPLE) *p++;
			sinfo->colormap[1][i] = (JSAMPLE) *p++;
			sinfo->colormap[0][i] = (JSAMPLE) *p++;
			p++;
		}
		break;
	default:
		ERREXIT(sinfo->cinfo, ERROR_INTERNAL);
		break;
	}
}

///8ビットDIBを一列分読み込む
METHODDEF(JDIMENSION)
get_8bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
	dib_source_ptr source = (dib_source_ptr)sinfo;
	register JSAMPARRAY colormap = source->colormap;
	JSAMPARRAY image_ptr;
	register int t;
	register JSAMPROW inptr, outptr;
	register JDIMENSION col;

	source->source_row--;
	image_ptr = (*cinfo->mem->access_virt_sarray)
		((j_common_ptr) cinfo, source->whole_image,
		 source->source_row, (JDIMENSION) 1, FALSE);

	inptr = image_ptr[0];
	outptr = source->pub.buffer[0];
	for (col = cinfo->image_width; col > 0; col--) {
		t = GETJSAMPLE(*inptr++);
		*outptr++ = colormap[0][t];
		*outptr++ = colormap[1][t];
		*outptr++ = colormap[2][t];
	}

	return 1;
}

///24ビットDIBを一列分読み込む
METHODDEF(JDIMENSION)
get_24bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
	dib_source_ptr source = (dib_source_ptr)sinfo;
	JSAMPARRAY image_ptr;
	register JSAMPROW inptr, outptr;
	register JDIMENSION col;

	source->source_row--;
	image_ptr = (*cinfo->mem->access_virt_sarray)
		((j_common_ptr) cinfo, source->whole_image,
		 source->source_row, (JDIMENSION) 1, FALSE);

	inptr = image_ptr[0];
	outptr = source->pub.buffer[0];
	for (col = cinfo->image_width; col > 0; col--) {
		outptr[2] = *inptr++;
		outptr[1] = *inptr++;
		outptr[0] = *inptr++;
		outptr += 3;
	}

	return 1;
}

///32ビットDIBを一列分読み込む
METHODDEF(JDIMENSION)
get_32bit_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
	dib_source_ptr source = (dib_source_ptr) sinfo;
	JSAMPARRAY image_ptr;
	register JSAMPROW inptr, outptr;
	register JDIMENSION col;

	source->source_row--;
	image_ptr = (*cinfo->mem->access_virt_sarray)
		((j_common_ptr) cinfo, source->whole_image,
		 source->source_row, (JDIMENSION) 1, FALSE);

	inptr = image_ptr[0];
	outptr = source->pub.buffer[0];
	for (col = cinfo->image_width; col > 0; col--) {
		outptr[2] = *inptr++;
		outptr[1] = *inptr++;
		outptr[0] = *inptr++;
		inptr++;
		outptr += 3;
	}

	return 1;
}

///画像変換を行う前の下準備を行う（主に画像データ部）
METHODDEF(JDIMENSION)
preload_image (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
	dib_source_ptr source = (dib_source_ptr)sinfo;
	register char *p = (char*)(source->pub.lpBm);
	register JSAMPROW out_ptr;
	JSAMPARRAY image_ptr;
	JDIMENSION row;
	//JDIMENSION col;
	//ProgressPtr progress = (ProgressPtr) cinfo->progress;

	for (row = 0; row < cinfo->image_height; row++) {
		//if (progress != NULL) {
		//	progress->pub.pass_counter = (long) row;
		//	progress->pub.pass_limit = (long) cinfo->image_height;
		//	(*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
		//}
		image_ptr = (*cinfo->mem->access_virt_sarray)
			((j_common_ptr) cinfo, source->whole_image,
			 row, (JDIMENSION) 1, TRUE);
		out_ptr = image_ptr[0];

		//for (col = source->row_width; col > 0; col--) {
		//	*out_ptr++ = (JSAMPLE) *p++;
		//}
		memcpy(out_ptr, p, source->row_width);
		out_ptr += source->row_width;
		p += source->row_width;
	}
	//if (progress != NULL)
	//	progress->completed_extra_passes++;

	switch (source->bits_per_pixel) {
	case 8:
		source->pub.get_pixel_rows = get_8bit_row;
		break;
	case 24:
		source->pub.get_pixel_rows = get_24bit_row;
		break;
	case 32:
		source->pub.get_pixel_rows = get_32bit_row;
		break;
	}
	source->source_row = cinfo->image_height;

	return (*source->pub.get_pixel_rows) (cinfo, sinfo);
}

///読み込み開始（ヘッダから読み込み）
METHODDEF(void)
start_input_dib (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
	dib_source_ptr source = (dib_source_ptr)sinfo;
	unsigned char bmpinfoheader[64];
	INT32 headerSize=0;
	INT32 biWidth = 0;
	INT32 biHeight = 0;
	unsigned int biPlanes=0;
	INT32 biCompression=0;
	INT32 biXPelsPerMeter=0,biYPelsPerMeter=0;
	INT32 biClrUsed = 0;
	int mapentrysize = 0;
	JDIMENSION row_width=0;

	memset(bmpinfoheader, 0, 63);
	memcpy(bmpinfoheader, (unsigned char*)(source->pub.lpBInfo), 40);
	headerSize = (INT32)GET_4B(bmpinfoheader,0);
	if(headerSize != 40)
		ERREXIT(cinfo, ERROR_INTERNAL);

	biWidth = GET_4B(bmpinfoheader,4);
	biHeight = GET_4B(bmpinfoheader, 8);
	biPlanes = GET_2B(bmpinfoheader, 12);
	source->bits_per_pixel = (int)GET_2B(bmpinfoheader, 14);
	biCompression = GET_4B(bmpinfoheader, 16);
	biXPelsPerMeter = GET_4B(bmpinfoheader, 24);
	biYPelsPerMeter = GET_4B(bmpinfoheader, 28);
	biClrUsed = GET_4B(bmpinfoheader, 32);

	switch(source->bits_per_pixel){
	case 8:
		mapentrysize = 4;
		break;
	case 24:
	case 32:
		break;
	default:
		ERREXIT(cinfo, ERROR_INTERNAL);
		break;
	}
	if(biPlanes != 1)
		ERREXIT(cinfo, ERROR_INTERNAL);
	if(biCompression != 0)
		ERREXIT(cinfo, ERROR_INTERNAL);

	if(biXPelsPerMeter > 0 && biYPelsPerMeter > 0){
		cinfo->X_density = (UINT16)(biXPelsPerMeter/100);
		cinfo->Y_density = (UINT16)(biYPelsPerMeter/100);
		cinfo->density_unit = 2;
	}

	if (mapentrysize > 0) {
		if (biClrUsed <= 0)
			biClrUsed = 256;
		else if (biClrUsed > 256)
			ERREXIT(cinfo, ERROR_INTERNAL);
		source->colormap = (*cinfo->mem->alloc_sarray)
			((j_common_ptr) cinfo, JPOOL_IMAGE,
			 (JDIMENSION) biClrUsed, (JDIMENSION) 3);
		read_colormap(source, (int) biClrUsed, mapentrysize);
	}

	if (source->bits_per_pixel == 24)
		row_width = (JDIMENSION) (biWidth * 3);
	else if(source->bits_per_pixel == 32)
		row_width = (JDIMENSION) (biWidth * 4);
	else
		row_width = (JDIMENSION) biWidth;
	while ((row_width & 3) != 0) row_width++;
	source->row_width = row_width;

	source->whole_image = (*cinfo->mem->request_virt_sarray)
		((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
		 row_width, (JDIMENSION) biHeight, (JDIMENSION) 1);
	source->pub.get_pixel_rows = preload_image;
//  if (cinfo->progress != NULL) {
//    BQProgressPtr progress = (BQProgressPtr) cinfo->progress;
//    progress->total_extra_passes++; /* count file input as separate pass */
//  }

	source->pub.buffer = (*cinfo->mem->alloc_sarray)
		((j_common_ptr) cinfo, JPOOL_IMAGE,
		 (JDIMENSION) (biWidth * 3), (JDIMENSION) 1);
	source->pub.buffer_height = 1;

	cinfo->in_color_space = JCS_RGB;
	cinfo->input_components = 3;
	cinfo->data_precision = 8;
	cinfo->image_width = (JDIMENSION) biWidth;
	cinfo->image_height = (JDIMENSION) biHeight;
}

///読み込み終了（何も行いません）
METHODDEF(void)
finish_input_dib (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
}

///読み込み前の初期化
cjpeg_source_ptr jinit_read_dib (j_compress_ptr cinfo, void* lpBInfo, void* lpBm)
{
	dib_source_ptr source;

	source = (dib_source_ptr)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
			sizeof(dib_source_struct));
	source->cinfo = cinfo;
	source->pub.start_input = start_input_dib;
	source->pub.finish_input = finish_input_dib;
	source->pub.lpBInfo = (void*)lpBInfo;
	source->pub.lpBm = (void*)lpBm;

	return (cjpeg_source_ptr) source;
}


/// DIBからJPEGファイルを作成します
///
/// @param filename 出力ファイルネーム
/// @param lpBInfo BITMAPINFOへのポインタ
/// @param lpBm ビットマップデータへのポインタ
/// @param nQuality JPEGの品質設定(1-100)
///
/// @return 0:正常終了 0以外:不正終了
int DIBtoJPEG(const char *filename, void* lpBInfo, void* lpBm, int nQuality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *output_file;
	JDIMENSION num_scanlines;
	cjpeg_source_ptr src_mgr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	cinfo.in_color_space = JCS_RGB;

//#ifdef USE_MOZJPEG
	//cinfo.use_moz_defaults = TRUE;
//	if (jpeg_c_bool_param_supported(&cinfo, JBOOLEAN_USE_MOZ_DEFAULTS)){
//		jpeg_c_set_bool_param(&cinfo, JBOOLEAN_USE_MOZ_DEFAULTS, TRUE);
//	}
//#endif
	jpeg_set_defaults(&cinfo);

	output_file = fopen(filename, "wb");
	src_mgr = jinit_read_dib(&cinfo, lpBInfo, lpBm);

	(*src_mgr->start_input) (&cinfo, src_mgr);

	jpeg_default_colorspace(&cinfo);

	//if(grayscale)
	//	jpeg_set_colorspace(&cinfo, JCS_GRAYSCALE);
	if(nQuality < 0){
		nQuality = 0;
	}else if(nQuality > 100){
		nQuality = 100;
	}
	jpeg_set_quality(&cinfo, nQuality, TRUE);
	//if(progressive)
	//	jpeg_simple_progression(&cinfo);
	cinfo.dct_method = JDCT_FLOAT;

	jpeg_stdio_dest(&cinfo, output_file);

	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
		(void) jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
	}

	(*src_mgr->finish_input) (&cinfo, src_mgr);

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fclose(output_file);

	return 0;
}
