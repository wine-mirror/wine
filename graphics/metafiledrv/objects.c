/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap.h"
#include "font.h"
#include "metafiledrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(metafile);


/***********************************************************************
 *           MFDRV_BITMAP_SelectObject
 */
static HBITMAP MFDRV_BITMAP_SelectObject( DC * dc, HBITMAP hbitmap )
{
    return 0;
}


/******************************************************************
 *         MFDRV_CreateBrushIndirect
 */

INT16 MFDRV_CreateBrushIndirect(DC *dc, HBRUSH hBrush )
{
    INT16 index = -1;
    DWORD size;
    METARECORD *mr;
    LOGBRUSH logbrush;

    if (!GetObjectA( hBrush, sizeof(logbrush), &logbrush )) return -1;

    switch(logbrush.lbStyle)
    {
    case BS_SOLID:
    case BS_NULL:
    case BS_HATCHED:
        {
	    LOGBRUSH16 lb16;

	    lb16.lbStyle = logbrush.lbStyle;
	    lb16.lbColor = logbrush.lbColor;
	    lb16.lbHatch = logbrush.lbHatch;
	    size = sizeof(METARECORD) + sizeof(LOGBRUSH16) - 2;
	    mr = HeapAlloc( GetProcessHeap(), 0, size );
	    mr->rdSize = size / 2;
	    mr->rdFunction = META_CREATEBRUSHINDIRECT;
	    memcpy( mr->rdParm, &lb16, sizeof(LOGBRUSH));
	    break;
	}
    case BS_PATTERN:
        {
	    BITMAP bm;
	    BYTE *bits;
	    BITMAPINFO *info;
	    DWORD bmSize;

	    GetObjectA(logbrush.lbHatch, sizeof(bm), &bm);
	    if(bm.bmBitsPixel != 1 || bm.bmPlanes != 1) {
	        FIXME("Trying to store a colour pattern brush\n");
		goto done;
	    }

	    bmSize = DIB_GetDIBImageBytes(bm.bmWidth, bm.bmHeight, 1);

	    size = sizeof(METARECORD) + sizeof(WORD) + sizeof(BITMAPINFO) + 
	      sizeof(RGBQUAD) + bmSize;

	    mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	    if(!mr) goto done;
	    mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	    mr->rdSize = size / 2;
	    mr->rdParm[0] = BS_PATTERN;
	    mr->rdParm[1] = DIB_RGB_COLORS;
	    info = (BITMAPINFO *)(mr->rdParm + 2);

	    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	    info->bmiHeader.biWidth = bm.bmWidth;
	    info->bmiHeader.biHeight = bm.bmHeight;
	    info->bmiHeader.biPlanes = 1;
	    info->bmiHeader.biBitCount = 1;
	    bits = ((BYTE *)info) + sizeof(BITMAPINFO) + sizeof(RGBQUAD);

	    GetDIBits(dc->hSelf, logbrush.lbHatch, 0, bm.bmHeight,
		      bits, info, DIB_RGB_COLORS);
	    *(DWORD *)info->bmiColors = 0;
	    *(DWORD *)(info->bmiColors + 1) = 0xffffff;
	    break;
	}

    case BS_DIBPATTERN:
        {
	      BITMAPINFO *info;
	      DWORD bmSize, biSize;

	      info = GlobalLock16((HGLOBAL16)logbrush.lbHatch);
	      if (info->bmiHeader.biCompression)
		  bmSize = info->bmiHeader.biSizeImage;
	      else
		  bmSize = DIB_GetDIBImageBytes(info->bmiHeader.biWidth,
						info->bmiHeader.biHeight,
						info->bmiHeader.biBitCount);
	      biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush.lbColor));
	      size = sizeof(METARECORD) + biSize + bmSize + 2;
	      mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	      if(!mr) goto done;
	      mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	      mr->rdSize = size / 2;
	      *(mr->rdParm) = logbrush.lbStyle;
	      *(mr->rdParm + 1) = LOWORD(logbrush.lbColor);
	      memcpy(mr->rdParm + 2, info, biSize + bmSize);
	      break;
	}
	default:
	    FIXME("Unkonwn brush style %x\n", logbrush.lbStyle);
	    return -1;
    }
    index = MFDRV_AddHandleDC( dc );
    if(!MFDRV_WriteRecord( dc, mr, mr->rdSize * 2))
        index = -1;
    HeapFree(GetProcessHeap(), 0, mr);
done:
    return index;
}


/***********************************************************************
 *           MFDRV_BRUSH_SelectObject
 */
static HBRUSH MFDRV_BRUSH_SelectObject( DC *dc, HBRUSH hbrush )
{
    INT16 index;
    METARECORD mr;

    index = MFDRV_CreateBrushIndirect( dc, hbrush );
    if(index == -1) return 0;

    mr.rdSize = sizeof(mr) / 2;
    mr.rdFunction = META_SELECTOBJECT;
    mr.rdParm[0] = index;
    return MFDRV_WriteRecord( dc, &mr, mr.rdSize * 2);
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
static HFONT MFDRV_FONT_SelectObject( DC * dc, HFONT hfont )
{
    LOGFONT16 lf16;

    if (!GetObject16( hfont, sizeof(lf16), &lf16 )) return GDI_ERROR;
    if (MFDRV_CreateFontIndirect(dc, hfont, &lf16))
        return FALSE;
    return GDI_ERROR;
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
static HPEN MFDRV_PEN_SelectObject( DC * dc, HPEN hpen )
{
    LOGPEN16 logpen;
    HPEN prevHandle = dc->hPen;

    if (!GetObject16( hpen, sizeof(logpen), &logpen )) return 0;
    if (MFDRV_CreatePenIndirect( dc, hpen, &logpen )) return prevHandle;
    return 0;
}


/***********************************************************************
 *           MFDRV_SelectObject
 */
HGDIOBJ MFDRV_SelectObject( DC *dc, HGDIOBJ handle )
{
    TRACE("hdc=%04x %04x\n", dc->hSelf, handle );

    switch(GetObjectType( handle ))
    {
    case OBJ_PEN:    return MFDRV_PEN_SelectObject( dc, handle );
    case OBJ_BRUSH:  return MFDRV_BRUSH_SelectObject( dc, handle );
    case OBJ_BITMAP: return MFDRV_BITMAP_SelectObject( dc, handle );
    case OBJ_FONT:   return MFDRV_FONT_SelectObject( dc, handle );
    case OBJ_REGION: return (HGDIOBJ)SelectClipRgn( dc->hSelf, handle );
    }
    return 0;
}
