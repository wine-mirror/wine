/*
 * WIN32S16
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dll);

/***********************************************************************
 *		BootTask16
 */
void WINAPI BootTask16()
{
	MESSAGE("BootTask(): should only be used by WIN32S.EXE.\n");
}

/***********************************************************************
 *           StackLinearToSegmented       (WIN32S16.43)
 *
 * Written without any docu.
 */
SEGPTR WINAPI StackLinearToSegmented16(WORD w1, WORD w2)
{
	FIXME("(%d,%d):stub.\n",w1,w2);
	return (SEGPTR)NULL;
}
