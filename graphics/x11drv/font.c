/*
 * X11 driver font functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "windows.h"
#include "x11drv.h"
#include "gdi.h"

/***********************************************************************
 *           X11DRV_GetTextExtentPoint
 */
BOOL32 X11DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                  LPSIZE32 size )
{
    int dir, ascent, descent;
    XCharStruct info;

    XTextExtents( dc->u.x.font.fstruct, str, count, &dir,
		  &ascent, &descent, &info );
    size->cx = abs((info.width + dc->w.breakRem + count * dc->w.charExtra)
		    * dc->w.WndExtX / dc->w.VportExtX);
    size->cy = abs((dc->u.x.font.fstruct->ascent+dc->u.x.font.fstruct->descent)
		    * dc->w.WndExtY / dc->w.VportExtY);
    return TRUE;
}
