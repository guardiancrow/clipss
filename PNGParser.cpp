//Copyright (C) 2014 - 2016, guardiancrow
//MinGW g++ : -std=c++11 -U__STRICT_ANSI__

#include "PNGParser.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

//Q. なんでわざわざ低水準IOをつかうの？fseek()とftell()でいいんじゃないの？
//A. https://www.jpcert.or.jp/sc-rules/c-fio19-c.html
//新たな疑問：C++の場合はその限りではない？
//for MinGW g++ : need -U__STRICT_ANSI__
inline size_t __getfilesize(const char* filename)
{
    int fd = 0;
    int ret = 0;
    struct _stat stbuf;

    if((fd = _open(filename, O_RDONLY)) == -1){
        return -1;
    }
    ret = _fstat(fd, &stbuf);
    _close(fd);
    return (ret == 0) ? stbuf.st_size : -1;
}

// C++ならばtellg() seekg()でも良い？
inline size_t __getfilesize2(const char* filename)
{
    ifstream ifs (filename, ios::in | ios::binary);
    if(!ifs){
        return -1;
    }
    size_t ret = ifs.seekg(0, ios::end).tellg();
    ifs.close();
    return ret;
}

inline size_t __getfilesize(std::istream& is)
{
    size_t current = is.tellg();
    size_t ret = is.seekg(0, ios::end).tellg();
    is.seekg(current, ios::beg);
    return ret;
}

CPNGParser::CPNGParser()
{
}

CPNGParser::~CPNGParser()
{
}

void CPNGParser::Init(const wchar_t *filename)
{
	char *str = NULL;
	size_t _size = wcslen(filename);
	str = new char[_size + 1];
	wcstombs(str, filename, _size);
	Init(str);
	delete [] str;
}

void CPNGParser::Init(const char* filename)
{
	ifstream fin (filename, ios::in | ios::binary);
	if(!fin){
		return;
	}

    Init(fin);
    fin.close();
}

void CPNGParser::Init(std::istream& fin)
{
    char *buffer = NULL;
	unsigned char PNGSig[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    unsigned long filesize = __getfilesize(fin);

	buffer = new char[8];
	memset(buffer, 0, 8);
	fin.read(buffer, 8);

	if(memcmp(buffer, PNGSig, 8) != 0){
		delete [] buffer;
		buffer = 0;
		return;
	}

	while(!fin.eof()){
		unsigned long size = 0;
		unsigned long crc = 0;
		unsigned long offset = 0;
		char *chunktype = new char[4];
		char *chunkdata = NULL;
		memset(chunktype, 0, 4);

		offset = (unsigned long)fin.tellg();
		fin.read(buffer, 4);
		size = (unsigned long)(((unsigned char)buffer[0] << 24) + ((unsigned char)buffer[1] << 16) + ((unsigned char)buffer[2] << 8) + (unsigned char)buffer[3]);

		if(offset + size + 12 > filesize){
			delete [] chunktype;
			break;
		}

		fin.read(chunktype, 4);

		if(size <= 0){
			chunkdata = NULL;
		}else{
			chunkdata = new char[size];
			fin.read(chunkdata, size);
		}

		fin.read(buffer, 4);
		crc = (unsigned long)(((unsigned char)buffer[0] << 24) + ((unsigned char)buffer[1] << 16) + ((unsigned char)buffer[2] << 8) + (unsigned char)buffer[3]);

		PNGChunk *pngchunk = new PNGChunk();

		pngchunk->Size = size;
		pngchunk->CRC = crc;
		pngchunk->Offset = offset;

		memcpy(pngchunk->ChunkType, chunktype, 4);
		if(size > 0){
			pngchunk->ChunkData = new unsigned char[pngchunk->Size];
			memcpy(pngchunk->ChunkData, chunkdata, pngchunk->Size);
		}

		pngchunk->isCritial = (pngchunk->ChunkType[0] & 0x20)?false:true;
		pngchunk->isPublic = (pngchunk->ChunkType[1] & 0x20)?false:true;
		pngchunk->isReserved = (pngchunk->ChunkType[2] & 0x20)?false:true;
		pngchunk->isCopySafe = (pngchunk->ChunkType[3] & 0x20)?true:false;

		CRC32 crcclass;
		pngchunk->crcStatus = (crcclass.pngchunkcrc((unsigned char*)pngchunk->ChunkType, 4, (unsigned char*)pngchunk->ChunkData, pngchunk->Size) - pngchunk->CRC == 0)?true:false;

		Chunks.push_back(PNGChunkPtr(pngchunk));

		if(chunktype){
			delete [] chunktype;
			chunktype = NULL;
		}
		if(chunkdata){
			delete [] chunkdata;
			chunkdata = NULL;
		}
	}

	delete [] buffer;
	buffer = 0;
}

