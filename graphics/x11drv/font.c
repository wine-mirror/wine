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
		    * dc->wndExtX / dc->vportExtX);
    size->cy = abs((dc->u.x.font.fstruct->ascent+dc->u.x.font.fstruct->descent)
		    * dc->wndExtY / dc->vportExtY);
    return TRUE;
}

BOOL32 X11DRV_GetTextMetrics(DC *dc, TEXTMETRIC32A *metrics)
{
    
    metrics->tmWeight           = dc->u.x.font.metrics.tmWeight;
    metrics->tmOverhang         = dc->u.x.font.metrics.tmOverhang;
    metrics->tmDigitizedAspectX = dc->u.x.font.metrics.tmDigitizedAspectX;
    metrics->tmDigitizedAspectY = dc->u.x.font.metrics.tmDigitizedAspectY;
    metrics->tmFirstChar        = dc->u.x.font.metrics.tmFirstChar;
    metrics->tmLastChar         = dc->u.x.font.metrics.tmLastChar;
    metrics->tmDefaultChar      = dc->u.x.font.metrics.tmDefaultChar;
    metrics->tmBreakChar        = dc->u.x.font.metrics.tmBreakChar;
    metrics->tmItalic           = dc->u.x.font.metrics.tmItalic;
    metrics->tmUnderlined       = dc->u.x.font.metrics.tmUnderlined;
    metrics->tmStruckOut        = dc->u.x.font.metrics.tmStruckOut;
    metrics->tmPitchAndFamily   = dc->u.x.font.metrics.tmPitchAndFamily;
    metrics->tmCharSet          = dc->u.x.font.metrics.tmCharSet;

    metrics->tmAscent  = abs( dc->u.x.font.metrics.tmAscent
			      * dc->wndExtY / dc->vportExtY );
    metrics->tmDescent = abs( dc->u.x.font.metrics.tmDescent
			      * dc->wndExtY / dc->vportExtY );
    metrics->tmHeight  = dc->u.x.font.metrics.tmAscent + dc->u.x.font.metrics.tmDescent;
    metrics->tmInternalLeading = abs( dc->u.x.font.metrics.tmInternalLeading
				      * dc->wndExtY / dc->vportExtY );
    metrics->tmExternalLeading = abs( dc->u.x.font.metrics.tmExternalLeading
				      * dc->wndExtY / dc->vportExtY );
    metrics->tmMaxCharWidth    = abs( dc->u.x.font.metrics.tmMaxCharWidth 
				      * dc->wndExtX / dc->vportExtX );
    metrics->tmAveCharWidth    = abs( dc->u.x.font.metrics.tmAveCharWidth
				      * dc->wndExtX / dc->vportExtX );

    return TRUE;
    
}



