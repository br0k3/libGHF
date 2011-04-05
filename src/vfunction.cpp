// GPL License - see http://opensource.org/licenses/gpl-license.php
// Copyright 2006 *nixCoders team - don't forget to credits us

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ghf.h"

// Return vtable index of a virtual function
int getVTblIndex(const char *fmt, ...) {
	va_list va_alist;
	char buf[8];
	memset(buf, 0, sizeof(buf));
	
	va_start(va_alist, fmt);
		vsnprintf(buf, sizeof(buf), fmt, va_alist);
	va_end(va_alist);

	return (strtoul(buf, NULL, 16) - 1) / 4;
}
