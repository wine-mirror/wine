/*
 * WINDEBUG.DLL
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "module.h"

/***********************************************************************
 *           WinNotify       (WINDEBUG.1)
 *  written without _any_ docu
 */
DWORD WinNotify() {
	fprintf(stderr, "WinNotify(): stub !\n");
	return NULL;
}
