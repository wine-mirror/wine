/*
 *	PostScript driver object handling
 *
 *	Copyright 1998  Huw D M Davies
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

#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           PSDRV_BITMAP_SelectObject
 */
static HBITMAP PSDRV_BITMAP_SelectObject( DC * dc, HBITMAP hbitmap )
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *           PSDRV_SelectObject
 */
HGDIOBJ PSDRV_SelectObject( DC *dc, HGDIOBJ handle )
{
    HGDIOBJ ret = 0;

    TRACE("hdc=%04x %04x\n", dc->hSelf, handle );

    switch(GetObjectType( handle ))
    {
    case OBJ_PEN:
	  ret = PSDRV_PEN_SelectObject( dc, handle );
	  break;
    case OBJ_BRUSH:
	  ret = PSDRV_BRUSH_SelectObject( dc, handle );
	  break;
    case OBJ_BITMAP:
	  ret = PSDRV_BITMAP_SelectObject( dc, handle );
	  break;
    case OBJ_FONT:
	  ret = PSDRV_FONT_SelectObject( dc, handle );
	  break;
    case OBJ_REGION:
	  ret = (HGDIOBJ)SelectClipRgn( dc->hSelf, handle );
	  break;
    case 0:  /* invalid handle */
        break;
      default:
	  ERR("Unknown object type %ld\n", GetObjectType(handle) );
	  break;
    }
    return ret;
}
