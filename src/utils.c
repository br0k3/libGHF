// GPL License - see http://opensource.org/licenses/gpl-license.php
// Copyright 2006 *nixCoders team - don't forget to credit us

#include "ghf.h"

#include <stdio.h>
#include <zlib.h>

unsigned long crc32FromFile(const char* filename) {
	FILE *file;
	
	if ((file = fopen(filename, "rb")) == NULL)
		return 0;
	
	unsigned char buffer[512];
	int length;
	unsigned long crc = crc32(0L, Z_NULL, 0);

	while ((length = fread(buffer, 1, sizeof(buffer), file)) != 0)
		crc = crc32(crc, buffer, length);

	fclose(file);
	return crc;
}
