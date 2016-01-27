//Copyright (C) 2014 - 2016, guardiancrow

#pragma once

int libjpegVersion(void);
void libjpegVersionString(char **ppszVer);

int DIBtoJPEG(const char *filename, void* lpBInfo, void* lpBm, int nQuality = 75);

