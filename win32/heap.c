/*
 * Win32 kernel functions
 *
 * Copyright 1995 Thomas Sandford <t.d.g.sandford@prds-grn.demon.co.uk>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "winbase.h"
#include "stddebug.h"
#include "debug.h"

#define HEAP_ZERO_MEMORY	0x8

/* FIXME: these functions do *not* implement the win32 API properly. They
are here merely as "get you going" aids */

/***********************************************************************
 *           HeapAlloc			(KERNEL32.222)
 *
 */
LPVOID HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes)

{
	void *result;

	result = malloc(dwBytes);
	if(result && (dwFlags & HEAP_ZERO_MEMORY))
		memset(result, 0, dwBytes);
	return result;
}
