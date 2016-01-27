//Copyright (C) 2014 - 2016, guardiancrow

#include <png.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int libpngVersion(void);
void libpngVersionString(char **ppszVer);

int BMPtoPNG(const char *bmpfilename, const char *pngfilename);
int DIBtoPNG(const char *filename, LPBITMAPINFO lpBInfo, LPBYTE lpBm);
int DIBtoPNG_Optimize(const char *filename, LPBITMAPINFO lpBInfo, LPBYTE lpBm);
int DIBtoPNG_Simulate(unsigned int *pwrittensize, 
		int memlevel, int strategy, int filter,
		LPBITMAPINFO lpBInfo, LPBYTE lpBm);

