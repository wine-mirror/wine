/*
 * WIN32S16
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "wine/winbase16.h"
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


/***********************************************************************
 *           UTSelectorOffsetToLinear16       (WIN32S16.48)
 *
 * rough guesswork, but seems to work (I had no "reasonable" docu)
 */
LPVOID WINAPI UTSelectorOffsetToLinear16(SEGPTR sptr)
{
        return MapSL(sptr);
}

/***********************************************************************
 *           UTLinearToSelectorOffset16       (WIN32S16.49)
 *
 * FIXME: I don't know if that's the right way to do linear -> segmented
 */
SEGPTR WINAPI UTLinearToSelectorOffset16(LPVOID lptr)
{
    return (SEGPTR)lptr;
}
