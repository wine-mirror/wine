/*
 *	PostScript driver object handling
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "psdrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(psdrv);

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
