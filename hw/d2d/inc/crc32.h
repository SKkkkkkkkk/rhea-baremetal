#ifndef _CRC_H
#define _CRC_H
#include <stdint.h>
#include <stdio.h>
uint32_t crc32(uint32_t crc, const unsigned char *p, unsigned int len);
#endif
