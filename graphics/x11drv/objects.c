/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "x11drv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           X11DRV_SelectObject
 */
HGDIOBJ X11DRV_SelectObject( DC *dc, HGDIOBJ handle )
{
    TRACE("hdc=%04x %04x\n", dc->hSelf, handle );

    switch(GetObjectType( handle ))
    {
    case OBJ_PEN:    return X11DRV_PEN_SelectObject( dc, handle );
    case OBJ_BRUSH:  return X11DRV_BRUSH_SelectObject( dc, handle );
    case OBJ_BITMAP: return X11DRV_BITMAP_SelectObject( dc, handle );
    case OBJ_FONT:   return X11DRV_FONT_SelectObject( dc, handle );
    case OBJ_REGION: return (HGDIOBJ)SelectClipRgn( dc->hSelf, handle );
    }
    return 0;
}


/***********************************************************************
 *           X11DRV_DeleteObject
 */
BOOL X11DRV_DeleteObject( HGDIOBJ handle )
{
    switch(GetObjectType( handle ))
    {
    case OBJ_BITMAP:
        return X11DRV_BITMAP_DeleteObject( handle );
    default:
        ERR("Shouldn't be here!\n");
        break;
    }
    return FALSE;
}
