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
#include "debug.h"
#include "xmalloc.h"


/***********************************************************************
 *           MFDRV_ExtTextOut
 */
BOOL
MFDRV_ExtTextOut( DC *dc, INT x, INT y, UINT flags,
                  const RECT *lprect, LPCSTR str, UINT count,
                  const INT *lpDx )
{
    RECT16	rect16;
    LPINT16	lpdx16 = lpDx?(LPINT16)xmalloc(sizeof(INT16)*count):NULL;
    BOOL	ret;
    int		i;

    if (lprect)	CONV_RECT32TO16(lprect,&rect16);
    if (lpdx16)	for (i=count;i--;) lpdx16[i]=lpDx[i];
    ret=MF_ExtTextOut(dc,x,y,flags,lprect?&rect16:NULL,str,count,lpdx16);
    if (lpdx16)	free(lpdx16);
    return ret;
}
