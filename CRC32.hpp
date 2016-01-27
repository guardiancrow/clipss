//Copyright (C) 2014 - 2016, guardiancrow

#pragma once

#ifndef _CRC32_H_
#define _CRC32_H_

#include <memory>
#include <cstring>

class CRC32
{
public:
	CRC32(){
		crc_table = new unsigned long[256];
		make_crc_table();
	}

	virtual ~CRC32(){
		delete [] crc_table;
	}

private:
	void make_crc_table(void){
		unsigned long c;
		int n, k;

		for (n = 0; n < 256; n++) {
			c = (unsigned long) n;
			for (k = 0; k < 8; k++) {
				if (c & 1)
					c = 0xedb88320L ^ (c >> 1);
				else
					c = c >> 1;
			}
			crc_table[n] = c;
		}
	}

	unsigned long *crc_table;

public:
	unsigned long update_crc(unsigned long crc, unsigned char *buf, size_t len){
		unsigned long c = crc;
		size_t n;

		for (n = 0; n < len; n++) {
			c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
		}
		return c;
	}

	unsigned long update_crc_xor(unsigned long crc, unsigned char *buf, size_t len){
		return update_crc(crc ^ 0xffffffffL, buf, len) ^ 0xffffffffL;
	}

public:
	unsigned long crc32(unsigned long crc, unsigned char *buf, size_t len){
		return update_crc_xor(crc, buf, len);
	}

	unsigned long pngchunkcrc(unsigned char *name, size_t namelen, unsigned char *buf, size_t len){
		if(namelen < 4){
			return 0xffffffffL;
		}
		unsigned long ul = crc32(0, name, 4);
		return (len > 0) ? crc32(ul, buf, len) : ul;
	}
};

#endif
