/*
 * GDI brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
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

#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "bitmap.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

/* GDI logical brush object */
typedef struct
{
    GDIOBJHDR header;
    LOGBRUSH  logbrush;
} BRUSHOBJ;

#define NB_HATCH_STYLES  6

static HGDIOBJ BRUSH_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static INT BRUSH_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT BRUSH_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static BOOL BRUSH_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs brush_funcs =
{
    BRUSH_SelectObject,  /* pSelectObject */
    BRUSH_GetObject16,   /* pGetObject16 */
    BRUSH_GetObject,     /* pGetObjectA */
    BRUSH_GetObject,     /* pGetObjectW */
    NULL,                /* pUnrealizeObject */
    BRUSH_DeleteObject   /* pDeleteObject */
};

static HGLOBAL16 dib_copy(BITMAPINFO *info, UINT coloruse)
{
    BITMAPINFO  *newInfo;
    HGLOBAL16   hmem;
    INT         size;

    if (info->bmiHeader.biCompression)
        size = info->bmiHeader.biSizeImage;
    else
        size = DIB_GetDIBImageBytes(info->bmiHeader.biWidth,
                                    info->bmiHeader.biHeight,
                                    info->bmiHeader.biBitCount);
    size += DIB_BitmapInfoSize( info, coloruse );

    if (!(hmem = GlobalAlloc16( GMEM_MOVEABLE, size )))
    {
        return 0;
    }
    newInfo = (BITMAPINFO *) GlobalLock16( hmem );
    memcpy( newInfo, info, size );
    GlobalUnlock16( hmem );
    return hmem;
}


/***********************************************************************
 *           CreateBrushIndirect    (GDI32.@)
 *
 * BUGS
 *      As for Windows 95 and Windows 98:
 *      Creating brushes from bitmaps or DIBs larger than 8x8 pixels
 *      is not supported. If a larger bitmap is given, only a portion
 *      of the bitmap is used.
 */
HBRUSH WINAPI CreateBrushIndirect( const LOGBRUSH * brush )
{
    BRUSHOBJ * ptr;
    HBRUSH hbrush;

    if (!(ptr = GDI_AllocObject( sizeof(BRUSHOBJ), BRUSH_MAGIC,
				 (HGDIOBJ *)&hbrush, &brush_funcs ))) return 0;
    ptr->logbrush.lbStyle = brush->lbStyle;
    ptr->logbrush.lbColor = brush->lbColor;
    ptr->logbrush.lbHatch = brush->lbHatch;

    switch (ptr->logbrush.lbStyle)
    {
    case BS_PATTERN8X8:
        ptr->logbrush.lbStyle = BS_PATTERN;
        /* fall through */
    case BS_PATTERN:
        ptr->logbrush.lbHatch = (LONG)BITMAP_CopyBitmap( (HBITMAP) ptr->logbrush.lbHatch );
        if (!ptr->logbrush.lbHatch) goto error;
        break;

    case BS_DIBPATTERNPT:
        ptr->logbrush.lbStyle = BS_DIBPATTERN;
        ptr->logbrush.lbHatch = (LONG)dib_copy( (BITMAPINFO *) ptr->logbrush.lbHatch,
                                                ptr->logbrush.lbColor);
        if (!ptr->logbrush.lbHatch) goto error;
        break;

    case BS_DIBPATTERN8X8:
    case BS_DIBPATTERN:
       {
            BITMAPINFO* bmi;
            HGLOBAL h = (HGLOBAL)ptr->logbrush.lbHatch;

            ptr->logbrush.lbStyle = BS_DIBPATTERN;
            if (!(bmi = (BITMAPINFO *)GlobalLock( h ))) goto error;
            ptr->logbrush.lbHatch = dib_copy( bmi, ptr->logbrush.lbColor);
            GlobalUnlock( h );
            if (!ptr->logbrush.lbHatch) goto error;
            break;
       }

    default:
        if(ptr->logbrush.lbStyle > BS_MONOPATTERN) goto error;
        break;
    }

    GDI_ReleaseObj( hbrush );
    TRACE("%p\n", hbrush);
    return hbrush;

 error:
    GDI_FreeObject( hbrush, ptr );
    return 0;
}


/***********************************************************************
 *           CreateHatchBrush    (GDI32.@)
 */
HBRUSH WINAPI CreateHatchBrush( INT style, COLORREF color )
{
    LOGBRUSH logbrush;

    TRACE("%d %06lx\n", style, color );

    logbrush.lbStyle = BS_HATCHED;
    logbrush.lbColor = color;
    logbrush.lbHatch = style;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreatePatternBrush    (GDI32.@)
 */
HBRUSH WINAPI CreatePatternBrush( HBITMAP hbitmap )
{
    LOGBRUSH logbrush = { BS_PATTERN, 0, 0 };
    TRACE("%p\n", hbitmap );

    logbrush.lbHatch = (ULONG_PTR)hbitmap;
    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrush    (GDI32.@)
 *
 *	Create a logical brush which has the pattern specified by the DIB
 *
 *	Function call is for compatibility only.  CreateDIBPatternBrushPt should be used.
 *
 * RETURNS
 *
 *	Handle to a logical brush on success, NULL on failure.
 *
 * BUGS
 *
 */
HBRUSH WINAPI CreateDIBPatternBrush(
		HGLOBAL hbitmap, /* [in] Global object containg BITMAPINFO structure */
		UINT coloruse 	 /* [in] Specifies color format, if provided */
)
{
    LOGBRUSH logbrush;

    TRACE("%p\n", hbitmap );

    logbrush.lbStyle = BS_DIBPATTERN;
    logbrush.lbColor = coloruse;

    logbrush.lbHatch = (LONG)hbitmap;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrushPt    (GDI32.@)
 *
 *	Create a logical brush which has the pattern specified by the DIB
 *
 * RETURNS
 *
 *	Handle to a logical brush on success, NULL on failure.
 *
 * BUGS
 *
 */
HBRUSH WINAPI CreateDIBPatternBrushPt(
		const void* data, /* [in] Pointer to a BITMAPINFO structure followed by more data */
		UINT coloruse 	  /* [in] Specifies color format, if provided */
)
{
    BITMAPINFO *info=(BITMAPINFO*)data;
    LOGBRUSH logbrush;

    TRACE("%p %ldx%ld %dbpp\n", info, info->bmiHeader.biWidth,
	  info->bmiHeader.biHeight,  info->bmiHeader.biBitCount);

    logbrush.lbStyle = BS_DIBPATTERNPT;
    logbrush.lbColor = coloruse;
    logbrush.lbHatch = (LONG) data;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateSolidBrush    (GDI32.@)
 */
HBRUSH WINAPI CreateSolidBrush( COLORREF color )
{
    LOGBRUSH logbrush;

    TRACE("%06lx\n", color );

    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = color;
    logbrush.lbHatch = 0;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           SetBrushOrgEx    (GDI32.@)
 */
BOOL WINAPI SetBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return FALSE;
    if (oldorg)
    {
        oldorg->x = dc->brushOrgX;
        oldorg->y = dc->brushOrgY;
    }
    dc->brushOrgX = x;
    dc->brushOrgY = y;
    GDI_ReleaseObj( hdc );
    return TRUE;
}

/***********************************************************************
 *           FixBrushOrgEx    (GDI32.@)
 * SDK says discontinued, but in Win95 GDI32 this is the same as SetBrushOrgEx
 */
BOOL WINAPI FixBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    return SetBrushOrgEx(hdc,x,y,oldorg);
}


/***********************************************************************
 *           BRUSH_SelectObject
 */
static HGDIOBJ BRUSH_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    BRUSHOBJ *brush = obj;
    HGDIOBJ ret;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;

    if (brush->logbrush.lbStyle == BS_PATTERN)
        BITMAP_SetOwnerDC( (HBITMAP)brush->logbrush.lbHatch, dc );

    ret = dc->hBrush;
    if (dc->funcs->pSelectBrush) handle = dc->funcs->pSelectBrush( dc->physDev, handle );
    if (handle) dc->hBrush = handle;
    else ret = 0;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           BRUSH_DeleteObject
 */
static BOOL BRUSH_DeleteObject( HGDIOBJ handle, void *obj )
{
    BRUSHOBJ *brush = obj;

    switch(brush->logbrush.lbStyle)
    {
      case BS_PATTERN:
	  DeleteObject( (HGDIOBJ)brush->logbrush.lbHatch );
	  break;
      case BS_DIBPATTERN:
	  GlobalFree16( (HGLOBAL16)brush->logbrush.lbHatch );
	  break;
    }
    return GDI_FreeObject( handle, obj );
}


/***********************************************************************
 *           BRUSH_GetObject16
 */
static INT BRUSH_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    BRUSHOBJ *brush = obj;
    LOGBRUSH16 logbrush;

    logbrush.lbStyle = brush->logbrush.lbStyle;
    logbrush.lbColor = brush->logbrush.lbColor;
    logbrush.lbHatch = brush->logbrush.lbHatch;
    if (count > sizeof(logbrush)) count = sizeof(logbrush);
    memcpy( buffer, &logbrush, count );
    return count;
}


/***********************************************************************
 *           BRUSH_GetObject
 */
static INT BRUSH_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    BRUSHOBJ *brush = obj;

    if (count > sizeof(brush->logbrush)) count = sizeof(brush->logbrush);
    memcpy( buffer, &brush->logbrush, count );
    return count;
}


/***********************************************************************
 *           SetSolidBrush   (GDI.604)
 *
 *  If hBrush is a solid brush, change its color to newColor.
 *
 *  RETURNS
 *           TRUE on success, FALSE on failure.
 *
 *  FIXME: untested, not sure if correct.
 */
BOOL16 WINAPI SetSolidBrush16(HBRUSH16 hBrush, COLORREF newColor )
{
    BRUSHOBJ * brushPtr;
    BOOL16 res = FALSE;

    TRACE("(hBrush %04x, newColor %08lx)\n", hBrush, (DWORD)newColor);
    if (!(brushPtr = (BRUSHOBJ *) GDI_GetObjPtr( HBRUSH_32(hBrush), BRUSH_MAGIC )))
	return FALSE;

    if (brushPtr->logbrush.lbStyle == BS_SOLID)
    {
        brushPtr->logbrush.lbColor = newColor;
	res = TRUE;
    }

     GDI_ReleaseObj( HBRUSH_32(hBrush) );
     return res;
}
