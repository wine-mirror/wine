/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include "bitmap.h"
#include "brush.h"
#include "font.h"
#include "metafiledrv.h"
#include "pen.h"
#include "debug.h"
#include "heap.h"

DECLARE_DEBUG_CHANNEL(gdi)
DECLARE_DEBUG_CHANNEL(metafile)

/***********************************************************************
 *           MFDRV_BITMAP_SelectObject
 */
static HBITMAP16 MFDRV_BITMAP_SelectObject( DC * dc, HBITMAP16 hbitmap,
                                            BITMAPOBJ * bmp )
{
    return 0;
}


/******************************************************************
 *         MFDRV_CreateBrushIndirect
 */

static BOOL MFDRV_CreateBrushIndirect(DC *dc, HBRUSH16 hBrush,
				      LOGBRUSH16 *logbrush)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logbrush)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logbrush) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParm), logbrush, sizeof(*logbrush));
    if (!(MFDRV_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MFDRV_CreatePatternBrush
 */
static BOOL MFDRV_CreatePatternBrush(DC *dc, HBRUSH16 hBrush,
				     LOGBRUSH16 *logbrush)
{
    DWORD len, bmSize, biSize;
    METARECORD *mr;
    BITMAPINFO *info;
    int index;
    char buffer[sizeof(METARECORD)];

    switch (logbrush->lbStyle)
    {
    case BS_PATTERN:
        {
	    BITMAP bm;
	    BYTE *bits;

	    GetObjectA(logbrush->lbHatch, sizeof(bm), &bm);
	    if(bm.bmBitsPixel != 1 || bm.bmPlanes != 1) {
	        FIXME(metafile, "Trying to store a colour pattern brush\n");
		return FALSE;
	    }

	    bmSize = DIB_GetDIBImageBytes(bm.bmWidth, bm.bmHeight, 1);

	    len = sizeof(METARECORD) +  sizeof(WORD) + sizeof(BITMAPINFO) + 
	      sizeof(RGBQUAD) + bmSize;
	     
	    mr = HeapAlloc(SystemHeap, HEAP_ZERO_MEMORY, len);
	    if(!mr) return FALSE;
	    mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	    mr->rdSize = len / 2;
	    mr->rdParm[0] = BS_PATTERN;
	    mr->rdParm[1] = DIB_RGB_COLORS;
	    info = (BITMAPINFO *)(mr->rdParm + 2);

	    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	    info->bmiHeader.biWidth = bm.bmWidth;
	    info->bmiHeader.biHeight = bm.bmHeight;
	    info->bmiHeader.biPlanes = 1;
	    info->bmiHeader.biBitCount = 1;
	    bits = ((BYTE *)info) + sizeof(BITMAPINFO) + sizeof(RGBQUAD);

	    GetDIBits(dc->hSelf, logbrush->lbHatch, 0, bm.bmHeight, bits,
		      info, DIB_RGB_COLORS);
	    *(DWORD *)info->bmiColors = 0;
	    *(DWORD *)(info->bmiColors + 1) = 0xffffff;
	    break;
	}

    case BS_DIBPATTERN:
	info = (BITMAPINFO *)GlobalLock16((HGLOBAL16)logbrush->lbHatch);
	if (info->bmiHeader.biCompression)
            bmSize = info->bmiHeader.biSizeImage;
        else
	    bmSize = DIB_GetDIBImageBytes(info->bmiHeader.biWidth,
					  info->bmiHeader.biHeight,
					  info->bmiHeader.biBitCount);
	biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush->lbColor)); 
	len = sizeof(METARECORD) + biSize + bmSize + 2;
	mr = HeapAlloc(SystemHeap, HEAP_ZERO_MEMORY, len);
	if(!mr) return FALSE;
	mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr->rdSize = len / 2;
	*(mr->rdParm) = logbrush->lbStyle;
	*(mr->rdParm + 1) = LOWORD(logbrush->lbColor);
	memcpy(mr->rdParm + 2, info, biSize + bmSize);
	break;
    default:
        return FALSE;
    }
    if (!(MFDRV_WriteRecord(dc, mr, len)))
    {
	HeapFree(SystemHeap, 0, mr);
	return FALSE;
    }

    HeapFree(SystemHeap, 0, mr);
    
    mr = (METARECORD *)&buffer;
    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dc, mr, mr->rdSize * 2);
}

/***********************************************************************
 *           MFDRV_BRUSH_SelectObject
 */
static HBRUSH MFDRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush,
                                          BRUSHOBJ * brush )
{
    LOGBRUSH16 logbrush = { brush->logbrush.lbStyle,
                            brush->logbrush.lbColor,
                            brush->logbrush.lbHatch };
    switch (brush->logbrush.lbStyle)
    {
    case BS_SOLID:
    case BS_HATCHED:
    case BS_HOLLOW:
        if (!MFDRV_CreateBrushIndirect( dc, hbrush, &logbrush )) return 0;
        break;
    case BS_PATTERN:
    case BS_DIBPATTERN:
        if (!MFDRV_CreatePatternBrush( dc, hbrush, &logbrush )) return 0;
        break;
    }
    return 1;  /* FIXME? */
}

/******************************************************************
 *         MFDRV_CreateFontIndirect
 */

static BOOL MFDRV_CreateFontIndirect(DC *dc, HFONT16 hFont, LOGFONT16 *logfont)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT16)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT16) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParm), logfont, sizeof(LOGFONT16));
    if (!(MFDRV_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dc, mr, mr->rdSize * 2);
}


/***********************************************************************
 *           MFDRV_FONT_SelectObject
 */
static HFONT16 MFDRV_FONT_SelectObject( DC * dc, HFONT16 hfont,
                                        FONTOBJ * font )
{
    HFONT16 prevHandle = dc->w.hFont;
    if (MFDRV_CreateFontIndirect(dc, hfont, &(font->logfont)))
        return prevHandle;
    return 0;
}

/******************************************************************
 *         MFDRV_CreatePenIndirect
 */
static BOOL MFDRV_CreatePenIndirect(DC *dc, HPEN16 hPen, LOGPEN16 *logpen)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logpen)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logpen) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParm), logpen, sizeof(*logpen));
    if (!(MFDRV_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MFDRV_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MFDRV_WriteRecord( dc, mr, mr->rdSize * 2);
}


/***********************************************************************
 *           MFDRV_PEN_SelectObject
 */
static HPEN MFDRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen )
{
    HPEN prevHandle = dc->w.hPen;
    LOGPEN16 logpen = { pen->logpen.lopnStyle,
                        { pen->logpen.lopnWidth.x, pen->logpen.lopnWidth.y },
                        pen->logpen.lopnColor };
    if (MFDRV_CreatePenIndirect( dc, hpen, &logpen )) return prevHandle;
    return 0;
}


/***********************************************************************
 *           MFDRV_SelectObject
 */
HGDIOBJ MFDRV_SelectObject( DC *dc, HGDIOBJ handle )
{
    GDIOBJHDR * ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    HGDIOBJ ret = 0;

    if (!ptr) return 0;
    TRACE(gdi, "hdc=%04x %04x\n", dc->hSelf, handle );
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  ret = MFDRV_PEN_SelectObject( dc, handle, (PENOBJ *)ptr );
	  break;
      case BRUSH_MAGIC:
	  ret = MFDRV_BRUSH_SelectObject( dc, handle, (BRUSHOBJ *)ptr );
	  break;
      case BITMAP_MAGIC:
	  ret = MFDRV_BITMAP_SelectObject( dc, handle, (BITMAPOBJ *)ptr );
	  break;
      case FONT_MAGIC:
	  ret = MFDRV_FONT_SelectObject( dc, handle, (FONTOBJ *)ptr );	  
	  break;
      case REGION_MAGIC:
	  ret = (HGDIOBJ16)SelectClipRgn16( dc->hSelf, handle );
	  break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}


