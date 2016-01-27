//Copyright (C) 2014 - 2016, guardiancrow

#include <iostream>
#include <iomanip>
#include <fstream>

#include "zlib.h"
#include "zopfli.h"
#include "PNGParser.h"
#include "pngrecomp.h"

using namespace std;

inline unsigned long _byteswap32(unsigned long ul)
{
    return (unsigned long)(((ul & 0xff000000) >> 24) | ((ul & 0x00ff0000) >> 8) | ((ul & 0x0000ff00) << 8) | ((ul & 0x000000ff) << 24));
}

int pngrecomp_zopfli_W(const wchar_t *srcfilename, wchar_t *destfilename, int numiterations)
{
	size_t _srcsize = wcslen(srcfilename);
	size_t _destsize = wcslen(destfilename);
	char* src = new char[_srcsize + 1];
	char* dest = new char[_destsize + 1];
	wcstombs(src, srcfilename, _srcsize);
	wcstombs(dest, destfilename, _destsize);
	int ret = pngrecomp_zopfli(src, dest, numiterations);
	delete [] src;
	delete [] dest;
	return ret;
}

int pngrecomp_zopfli(const char *srcfilename, char *destfilename, int numiterations)
{
    unsigned char PNGSig[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    std::ofstream ofs;
    CPNGParser * m_parser = NULL;
    m_parser = new CPNGParser();
    m_parser->Init(srcfilename);

    ofs.open(destfilename, ios::out|ios::binary|ios::trunc);
    ofs.write((char*)PNGSig, 8);
    vector<PNGChunkPtr>::iterator it;
    unsigned long chunk_crc = 0;
    unsigned long w = 0;
    unsigned long h = 0;
    unsigned char bitdepth = 0;
    unsigned char colortype = 0;
    unsigned long uncompress_buffer_size = 1000000;
    vector<unsigned char> idat_merge;
    for(it = m_parser->Chunks.begin();it != m_parser->Chunks.end();it++){
        unsigned long _size;
        unsigned long _crc;

        cout << (*it)->ChunkType << "|" << (*it)->Offset << "|" << (*it)->Size << endl;

        chunk_crc = 0;
        chunk_crc = crc32(chunk_crc, (unsigned char*)(*it)->ChunkType, 4);
        if((*it)->Size > 0){
            chunk_crc = crc32(chunk_crc, (unsigned char*)(*it)->ChunkData, (*it)->Size);
        }
        if((*it)->CRC - chunk_crc != 0){
            cout << "CRC NG:" << hex << (*it)->CRC << "|"<< chunk_crc << dec << endl;
        }

        string chunk = (*it)->ChunkType;
        if(chunk == "IHDR"){
            memcpy(&w, (*it)->ChunkData, 4);
            w = _byteswap32(w);
            memcpy(&h, (unsigned char*)&((*it)->ChunkData[4]), 4);
            h = _byteswap32(h);
            bitdepth = (*it)->ChunkData[8];
            colortype = (*it)->ChunkData[9];
            uncompress_buffer_size = w * h;
            if(bitdepth == 16){
                uncompress_buffer_size *= 2;
            }
            if(colortype & 0x4){
                uncompress_buffer_size *= 4;
            }else{
                uncompress_buffer_size *= 3;
            }
            uncompress_buffer_size += (w * 4 + h * 4); //margin
            cout << "buffer size:" << uncompress_buffer_size << endl;
        }

        if(chunk == "IDAT"){
            //IDATは複数ある場合は全て読み込まないとuncompressが成功しないのでここではしない
            std::copy((unsigned char*)&((*it)->ChunkData[0]),
            (unsigned char*)&((*it)->ChunkData[(*it)->Size]),
            std::back_inserter(idat_merge));
            cout << "IDAT (copy): " << idat_merge.size() << endl;
        }else if(chunk == "IEND"){
            //IDAT
            vector<unsigned char> uncompress_data;
            uLongf uncompress_size = uncompress_buffer_size;
            uncompress_data.resize(uncompress_buffer_size);
            cout << "  compress size : " << idat_merge.size() << endl;
            int ret = uncompress(&uncompress_data[0], &uncompress_size, &idat_merge[0], idat_merge.size());

            if(ret != 0){
                cout << "uncompress return : " << ret << endl;
                continue;
            }
            cout << "uncompress size : " << uncompress_size << endl;

            //最終書き込みデータなのでnewでやった方が確実かなと
            unsigned char *recompress_data = new unsigned char[uncompress_buffer_size];
            size_t recompress_size = 0;
            ZopfliOptions zoptions;
            ZopfliInitOptions(&zoptions);
            zoptions.verbose = 0;
            zoptions.numiterations = numiterations;
            ZopfliCompress(&zoptions, ZOPFLI_FORMAT_ZLIB, &uncompress_data[0], uncompress_size, &recompress_data, &recompress_size);
            cout << "recompress size : " << recompress_size << endl;
            chunk_crc = 0;
            chunk_crc = crc32(chunk_crc, (unsigned char*)"IDAT", 4);
            chunk_crc = crc32(chunk_crc, (unsigned char*)recompress_data, recompress_size);
            //chunk_crc = crc32(chunk_crc, p, recompress_size);
            cout << "new CRC : " << hex << chunk_crc << dec << endl;
            CRC32 crclass;
            cout << "new CRC : " << hex << crclass.pngchunkcrc((unsigned char*)"IDAT", 4, recompress_data, recompress_size) << dec << endl;

            _size = _byteswap32(recompress_size);
            ofs.write((char*)&_size, 4);
            ofs.write("IDAT", 4);
            ofs.write((char*)recompress_data, recompress_size);
            _crc = _byteswap32(chunk_crc);
            //_crc = _byteswap32(crcclass.pngchunkcrc((unsigned char*)(*it)->ChunkType, 4, recompress_data, recompress_size));
            ofs.write((char*)&_crc, 4);
            delete [] recompress_data;
            recompress_data = 0;

            //IEND
            _size = _byteswap32((*it)->Size);
            ofs.write((char*)&_size, 4);
            ofs.write((*it)->ChunkType, 4);
            ofs.write((char*)(*it)->ChunkData, (*it)->Size);
            _crc = _byteswap32((*it)->CRC);
            ofs.write((char*)&_crc, 4);
        }else{
            _size = _byteswap32((*it)->Size);
            ofs.write((char*)&_size, 4);
            ofs.write((*it)->ChunkType, 4);
            ofs.write((char*)(*it)->ChunkData, (*it)->Size);
            _crc = _byteswap32((*it)->CRC);
            ofs.write((char*)&_crc, 4);
        }
    }

    ofs.close();
    delete m_parser;

    return 0;
}
