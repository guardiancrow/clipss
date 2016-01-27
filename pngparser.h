//Copyright (C) 2014 - 2016, guardiancrow
// g++ -std=c++11

#pragma once

#ifndef _PNG_PARSER_H_
#define _PNG_PARSER_H_

#include <vector>
#include "PNGChunk.h"

typedef std::shared_ptr<PNGChunk> PNGChunkPtr;

class CPNGParser
{
public:
	CPNGParser();
	virtual ~CPNGParser();

	void Init(const wchar_t *filename);
	void Init(const char *filename);
	void Init(std::istream& is);

	std::vector<PNGChunkPtr> Chunks;
};

#endif //_PNG_PARSER_H_
