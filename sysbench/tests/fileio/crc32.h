#ifndef CRC32_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define CRC32_H

extern unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len);

#endif
