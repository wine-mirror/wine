/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


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
