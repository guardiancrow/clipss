//Copyright (C) 2014, guardiancrow

#include "pngutil.h"
#include "strutil.hpp"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int PngWrite(const char * filename, unsigned int *pwrittensize, 
		int memlevel, int strategy, int filter,
		LPBITMAPINFO lpBInfo, LPBYTE lpBm);

///libpngのバージョンを得ます
int libpngVersion(void)
{
	return png_access_version_number();
}

///PNGフィルタ定義用
const int pngfilters[] = {
	PNG_FILTER_NONE,
	PNG_FILTER_SUB,
	PNG_FILTER_UP,
	PNG_FILTER_AVG,
	PNG_FILTER_PAETH,
	PNG_ALL_FILTERS
};

///普通書き出し用
static void writefn_write(png_structp png_ptr, png_bytep pdata, png_size_t length)
{
	ofstream *ofs =  reinterpret_cast<ofstream*>(png_ptr->io_ptr);
	ofs->write((char*)pdata, length);
}

///ファイルに書き込まない時用
static void writefn_seek(png_structp png_ptr, png_bytep pdata, png_size_t length)
{
	unsigned int *nCount = reinterpret_cast<unsigned int*>(png_ptr->io_ptr);
	(*nCount) += length;
}

///PNG形式を書き出します
///
///deflateInit2のオプションとPNGのフィルタを指定して出力ファイルサイズを計算します
///levelはZ_BEST_COMPRESSION、windowBitsは15、buffersizeは8192に固定です
///filenameがNULLのときは出力ファイルサイズを返します
int PngWrite(const char * filename, unsigned int *pwrittensize, 
		int memlevel, int strategy, int filter,
		LPBITMAPINFO lpBInfo, LPBYTE lpBm)
{
	png_structp write_ptr;
	png_infop write_info_ptr;

	const int level = Z_BEST_COMPRESSION;
	const int windowbits = 15;
	const int buffersize = 8192;

	ofstream ofs;

	if(lpBInfo == NULL || lpBm == NULL){
		return -1;
	}
	if(lpBInfo->bmiHeader.biBitCount != 24 && lpBInfo->bmiHeader.biBitCount != 32){
		return -1;
	}

	unsigned int outputcount = 0;

	if(filename){
		ofs.open(filename, ios::out | ios::binary);
	}

	write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(write_ptr == NULL){
		if(filename){
			ofs.close();
		}
		return -1;
	}

	write_info_ptr = png_create_info_struct(write_ptr);
	if(write_info_ptr == NULL){
		png_destroy_write_struct(&write_ptr, NULL);
		if(filename){
			ofs.close();
		}
		return -1;
	}

	if(filename){
		png_set_write_fn(write_ptr, &ofs, writefn_write, NULL);
	}else{
		png_set_write_fn(write_ptr, &outputcount, writefn_seek, NULL);
	}

	if(memlevel < 1){
		memlevel = 1;
	}else if(memlevel > 9){
		memlevel = 9;
	}

	if(strategy < 0){
		strategy = 0;
	}else if(strategy > 3){
		strategy = 3;
	}

	if(filter < PNG_FILTER_VALUE_NONE){
		filter = PNG_FILTER_VALUE_NONE;
	}else if(filter > PNG_FILTER_VALUE_LAST){
		filter = PNG_FILTER_VALUE_LAST;
	}

	//この３つは固定値
	png_set_compression_level(write_ptr, level);
	png_set_compression_window_bits(write_ptr, windowbits);
	png_set_compression_buffer_size(write_ptr, buffersize);

	png_set_compression_mem_level(write_ptr, memlevel);
	png_set_compression_strategy(write_ptr, strategy);
	png_set_filter(write_ptr, 0, pngfilters[filter]);

	if(lpBInfo->bmiHeader.biBitCount == 24){
		png_set_IHDR(write_ptr, write_info_ptr, lpBInfo->bmiHeader.biWidth, lpBInfo->bmiHeader.biHeight, 8,
				PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	}else{
		png_set_IHDR(write_ptr, write_info_ptr, lpBInfo->bmiHeader.biWidth, lpBInfo->bmiHeader.biHeight, 8,
				PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	}

	png_write_info(write_ptr, write_info_ptr);
	if(lpBInfo->bmiHeader.biBitCount == 24){
		png_set_bgr(write_ptr);
	}

	LPBYTE lpBmHead = lpBm;
	unsigned long rowstride = (lpBInfo->bmiHeader.biWidth * lpBInfo->bmiHeader.biBitCount + 31L) / 32 * 4;
	int num_pass = png_set_interlace_handling(write_ptr);
	for(int pass=0;pass<num_pass;pass++){
		for(int y=0;y<lpBInfo->bmiHeader.biHeight;y++){
			lpBmHead = lpBm + (lpBInfo->bmiHeader.biHeight - (y + 1)) * rowstride;
			png_write_row(write_ptr, lpBmHead);
		}
	}

	png_write_end(write_ptr, write_info_ptr);

	png_destroy_write_struct(&write_ptr, &write_info_ptr);

	if(filename){
		ofs.close();
	}

	if(pwrittensize){
		if(filename){
			ifstream ifs(filename);
			*pwrittensize = (unsigned int)ifs.seekg(0, ios::end).tellg();
			ifs.close();
		}else{
			*pwrittensize = outputcount;
		}
	}

	return 0;
}
/*
int BMPtoPNG(const char *bmpfilename, const char *pngfilename)
{
	return 0;
}
*/

///
/// DIBからPNGファイルを出力します
///
int DIBtoPNG(const char *filename, LPBITMAPINFO lpBInfo, LPBYTE lpBm)
{
	return PngWrite(filename, NULL, 9, Z_DEFAULT_STRATEGY, PNG_FILTER_VALUE_NONE, lpBInfo, lpBm);
}

/// スレッドで使用する構造体です
typedef struct{
	int bestmemlevel;
	int beststrategy;
	int bestfilter;
	unsigned int bestsize;
	LPBITMAPINFO lpBInfo;
	LPBYTE lpBm;
}ThreadResult;

///スレッドハンドル
HANDLE hThreads[5];

///スレッドID
DWORD dwThreadIDs[5];

///スレッド管理用
ThreadResult pthRes[5];

///
/// DIBtoPNG_Optimizeのスレッド用関数です
///
DWORD WINAPI OptimizeThreadProc(LPVOID lpParam)
{
	int memlevel, strategy, filter;
	unsigned int size = 0x7fffffff;
	ThreadResult *pthRes = NULL;
	pthRes = reinterpret_cast<ThreadResult *>(lpParam);
	
	memlevel = 9;
	filter = PNG_FILTER_VALUE_NONE;
	strategy = pthRes->beststrategy;
	LPBITMAPINFO lpBInfo = pthRes->lpBInfo;
	LPBYTE lpBm = pthRes->lpBm;

#ifdef _DEBUG
	string str = "png";
	str += toString(strategy);
	str += ".log";
	fstream fs(str.c_str(), ios::out);
#endif

	//4ぐらいから始める？
	for(memlevel=4;memlevel<=9;memlevel++){
		for(filter=PNG_FILTER_VALUE_NONE;filter<=PNG_FILTER_VALUE_LAST;filter++){
			DIBtoPNG_Simulate(&size, memlevel, strategy, filter, lpBInfo, lpBm);
#ifdef _DEBUG
			fs <<
				"memlevel:" << memlevel <<
				" strategy:" << strategy <<
				" filter:" << filter <<
				" size:" << size << endl;
#endif
			if(pthRes->bestsize >= size){
				pthRes->bestsize = size;
				pthRes->bestmemlevel = memlevel;
				pthRes->beststrategy = strategy;
				pthRes->bestfilter = filter;
			}
		}
	}
#ifdef _DEBUG
	fs <<
		"bestmemlevel:" << pthRes->bestmemlevel <<
		" beststrategy:" << pthRes->beststrategy <<
		" bestfilter:" << pthRes->bestfilter <<
		" bestsize:" << pthRes->bestsize << endl;
	fs.flush();
	fs.close();
#endif
	return 0;
}

///
///最適化は時間かかりますのでマルチスレッド化しています
///スレッドはストラテジごとに割り当てます
///
///memlevelは9にすれば最高圧縮になるとは限りません　4ぐらいまでは現実的な可能性があります
///特にSSの場合は文字と単一色の背景の組み合わせのような時にそうなる可能性が高いです
///流石に1が最適な画像は見たことがありません
///
int DIBtoPNG_Optimize(const char *filename, LPBITMAPINFO lpBInfo, LPBYTE lpBm)
{
	//int memlevel;
	int strategy;
	//int filter;
	int bestmemlevel=0, beststrategy=Z_DEFAULT_STRATEGY, bestfilter=PNG_FILTER_VALUE_NONE;
	unsigned int size = 0x7fffffff;
	unsigned int bestsize = 0x7fffffff;
#ifdef _DEBUG
	fstream fs("png.log", ios::out);
#endif

	//memlevel = 9;
	strategy = Z_DEFAULT_STRATEGY;
	//filter = PNG_FILTER_VALUE_NONE;
	for(strategy=0;strategy<=4;strategy++){
		hThreads[strategy] = NULL;
		dwThreadIDs[strategy] = 0;
		pthRes[strategy].bestmemlevel = 9;
		pthRes[strategy].beststrategy = strategy;
		pthRes[strategy].bestfilter = PNG_FILTER_VALUE_NONE;
		pthRes[strategy].bestsize = 0x7fffffff;
		pthRes[strategy].lpBInfo = lpBInfo;
		pthRes[strategy].lpBm = lpBm;

		hThreads[strategy] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)OptimizeThreadProc, (LPVOID)&pthRes[strategy],
				0, &dwThreadIDs[strategy]);
		/*for(memlevel=1;memlevel<=9;memlevel++){
			for(filter=PNG_FILTER_VALUE_NONE;filter<=PNG_FILTER_VALUE_LAST;filter++){
				DIBtoPNG_Simulate(&size, memlevel, strategy, filter, lpBInfo, lpBm);
#ifdef _DEBUG
				fs <<
					"memlevel:" << memlevel <<
					" strategy:" << strategy <<
					" filter:" << filter <<
					" size:" << size << endl;
#endif
				if(bestsize > size){
					bestsize = size;
					bestmemlevel = memlevel;
					beststrategy = strategy;
					bestfilter = filter;
				}
			}
		}*/
	}
	
	WaitForMultipleObjects(5, hThreads, TRUE, INFINITE);

	for(strategy=0;strategy<=4;strategy++){
#ifdef _DEBUG
		fs <<
			"memlevel:" << pthRes[strategy].bestmemlevel <<
			" strategy:" << pthRes[strategy].beststrategy <<
			" filter:" << pthRes[strategy].bestfilter <<
			" size:" << pthRes[strategy].bestsize << endl;
#endif
		if(bestsize > pthRes[strategy].bestsize){
			bestsize = pthRes[strategy].bestsize;
			bestmemlevel = pthRes[strategy].bestmemlevel;
			beststrategy = pthRes[strategy].beststrategy;
			bestfilter = pthRes[strategy].bestfilter;
		}
		CloseHandle(hThreads[strategy]);
	}
	
#ifdef _DEBUG
	fs <<
		"bestmemlevel:" << bestmemlevel <<
		" beststrategy:" << beststrategy <<
		" bestfilter:" << bestfilter <<
		" bestsize:" << bestsize << endl;

	fs.flush();
	fs.close();
#endif

	return PngWrite(filename, &size, bestmemlevel, beststrategy, bestfilter, lpBInfo, lpBm);
}

///
/// DIBからPNGファイルを作成するテストを行います。ファイルは作成されません
///
int DIBtoPNG_Simulate(unsigned int *pwrittensize, 
		int memlevel, int strategy, int filter,
		LPBITMAPINFO lpBInfo, LPBYTE lpBm)
{
	return PngWrite(NULL, pwrittensize, memlevel, strategy, filter, lpBInfo, lpBm);
}

