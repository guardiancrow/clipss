//Copyright (C) 2014 - 2016, guardiancrow

#pragma once

#ifndef _PNGCHUNK_H_
#define _PNGCHUNK_H_

#include "CRC32.hpp"

class PNGChunk
{
public:
	PNGChunk(){
		ChunkType = new char[5];
		memset(ChunkType, 0, 5);
		ChunkData = NULL;
		Size = 0;
		CRC = 0;
		Offset = 0;

		isCritial = false;
		isPublic = false;
		isReserved = false;
		isCopySafe = false;

		crcStatus = false;
	}
	virtual ~PNGChunk(){
		delete [] ChunkType;
		ChunkType = NULL;
		if(ChunkData){
			delete [] ChunkData;
			ChunkData = NULL;
		}
		Size = 0;
		CRC = 0;
		Offset = 0;

		isCritial = false;
		isPublic = false;
		isReserved = false;
		isCopySafe = false;

		crcStatus = false;
	}

	bool Check_CRC(){
		CRC32 crc;
		unsigned long newcrc = 0;
		newcrc = crc.pngchunkcrc((unsigned char*)ChunkType, 4, (unsigned char*)ChunkData, Size);

		return (newcrc - CRC == 0)?true:false;
	}

	unsigned long Size;
	unsigned long Offset;
	unsigned long CRC;
	char *ChunkType;
	unsigned char *ChunkData;

	bool isCritial;
	bool isPublic;
	bool isReserved;
	bool isCopySafe;

	bool crcStatus;
};

#endif
