/*
 * metafile driver text functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include "windows.h"
#include "dc.h"
#include "gdi.h"
#include "callback.h"
#include "heap.h"
#include "metafile.h"
#include "metafiledrv.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"


/***********************************************************************
 *           MFDRV_ExtTextOut
 */
BOOL32
MFDRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                  const RECT32 *lprect, LPCSTR str, UINT32 count,
                  const INT32 *lpDx )
{
    RECT16	rect16;
    LPINT16	lpdx16 = lpDx?(LPINT16)xmalloc(sizeof(INT16)*count):NULL;
    BOOL32	ret;
    int		i;

    if (lprect)	CONV_RECT32TO16(lprect,&rect16);
    if (lpdx16)	for (i=count;i--;) lpdx16[i]=lpDx[i];
    ret=MF_ExtTextOut(dc,x,y,flags,lprect?&rect16:NULL,str,count,lpdx16);
    if (lpdx16)	free(lpdx16);
    return ret;
}
