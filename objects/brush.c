/*
 * GDI brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include <string.h>
#include "winbase.h"
#include "brush.h"
#include "bitmap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi)

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



static BOOL create_brush_indirect(BRUSHOBJ *brushPtr, BOOL v16)
{
    LOGBRUSH *brush = &brushPtr->logbrush;

    switch (brush->lbStyle)
    {
       case BS_PATTERN8X8:
            brush->lbStyle = BS_PATTERN;
       case BS_PATTERN:
            brush->lbHatch = (LONG)BITMAP_CopyBitmap( (HBITMAP) brush->lbHatch );
            if (! brush->lbHatch)
               break;
            return TRUE;

       case BS_DIBPATTERNPT:
            brush->lbStyle = BS_DIBPATTERN;
            brush->lbHatch = (LONG)dib_copy( (BITMAPINFO *) brush->lbHatch,
                                             brush->lbColor);
            if (! brush->lbHatch)
               break;
            return TRUE;

       case BS_DIBPATTERN8X8:
       case BS_DIBPATTERN:
       {
            BITMAPINFO* bmi;
            HGLOBAL     h = brush->lbHatch;

            brush->lbStyle = BS_DIBPATTERN;
            if (v16)
            {
               if (!(bmi = (BITMAPINFO *)GlobalLock16( h )))
                  break;
            }
            else
            {
               if (!(bmi = (BITMAPINFO *)GlobalLock( h )))
                  break;
            }

            brush->lbHatch = dib_copy( bmi, brush->lbColor);

            if (v16)  GlobalUnlock16( h );
            else      GlobalUnlock( h );

            if (!brush->lbHatch)
               break;

            return TRUE;
       }

       default:
          if( brush->lbStyle <= BS_MONOPATTERN)
             return TRUE;
    }

    return FALSE;
}


/***********************************************************************
 *           CreateBrushIndirect16    (GDI.50)
 */
HBRUSH16 WINAPI CreateBrushIndirect16( const LOGBRUSH16 * brush )
{
    BOOL success;
    BRUSHOBJ * brushPtr;
    HBRUSH16 hbrush = GDI_AllocObject( sizeof(BRUSHOBJ), BRUSH_MAGIC );
    if (!hbrush) return 0;
    brushPtr = (BRUSHOBJ *) GDI_HEAP_LOCK( hbrush );
    brushPtr->logbrush.lbStyle = brush->lbStyle;
    brushPtr->logbrush.lbColor = brush->lbColor;
    brushPtr->logbrush.lbHatch = brush->lbHatch;
    success = create_brush_indirect(brushPtr, TRUE);
    GDI_HEAP_UNLOCK( hbrush );
    if(!success)
    {
       GDI_FreeObject(hbrush);
       hbrush = 0;
    }
    TRACE("%04x\n", hbrush);
    return hbrush;
}


/***********************************************************************
 *           CreateBrushIndirect    (GDI32.27)
 *
 * BUGS
 *      As for Windows 95 and Windows 98:
 *      Creating brushes from bitmaps or DIBs larger than 8x8 pixels
 *      is not supported. If a larger bitmap is given, only a portion
 *      of the bitmap is used.
 */
HBRUSH WINAPI CreateBrushIndirect( const LOGBRUSH * brush )
{
    BOOL success;
    BRUSHOBJ * brushPtr;
    HBRUSH hbrush = GDI_AllocObject( sizeof(BRUSHOBJ), BRUSH_MAGIC );
    if (!hbrush) return 0;
    brushPtr = (BRUSHOBJ *) GDI_HEAP_LOCK( hbrush );
    brushPtr->logbrush.lbStyle = brush->lbStyle;
    brushPtr->logbrush.lbColor = brush->lbColor;
    brushPtr->logbrush.lbHatch = brush->lbHatch;
    success = create_brush_indirect(brushPtr, FALSE);
    GDI_HEAP_UNLOCK( hbrush );
    if(!success)
    {
       GDI_FreeObject(hbrush);
       hbrush = 0;
    }
    TRACE("%08x\n", hbrush);
    return hbrush;
}


/***********************************************************************
 *           CreateHatchBrush16    (GDI.58)
 */
HBRUSH16 WINAPI CreateHatchBrush16( INT16 style, COLORREF color )
{
    return CreateHatchBrush( style, color );
}


/***********************************************************************
 *           CreateHatchBrush    (GDI32.48)
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
 *           CreatePatternBrush16    (GDI.60)
 */
HBRUSH16 WINAPI CreatePatternBrush16( HBITMAP16 hbitmap )
{
    return (HBRUSH16)CreatePatternBrush( hbitmap );
}


/***********************************************************************
 *           CreatePatternBrush    (GDI32.54)
 */
HBRUSH WINAPI CreatePatternBrush( HBITMAP hbitmap )
{
    LOGBRUSH logbrush = { BS_PATTERN, 0, 0 };
    TRACE("%04x\n", hbitmap );

    logbrush.lbHatch = hbitmap;
        return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrush16    (GDI.445)
 */
HBRUSH16 WINAPI CreateDIBPatternBrush16( HGLOBAL16 hbitmap, UINT16 coloruse )
{
    LOGBRUSH16 logbrush;

    TRACE("%04x\n", hbitmap );

    logbrush.lbStyle = BS_DIBPATTERN;
    logbrush.lbColor = coloruse;
    logbrush.lbHatch = hbitmap;

    return CreateBrushIndirect16( &logbrush );
    }


/***********************************************************************
 *           CreateDIBPatternBrush    (GDI32.34)
 *
 *	Create a logical brush which has the pattern specified by the DIB
 *
 *	Function call is for compatability only.  CreateDIBPatternBrushPt should be used.   
 *
 * RETURNS
 *
 *	Handle to a logical brush on success, NULL on failure.
 *
 * BUGS
 *	
 */
HBRUSH WINAPI CreateDIBPatternBrush( 
		HGLOBAL hbitmap, 		/* Global object containg BITMAPINFO structure */
		UINT coloruse 		/* Specifies color format, if provided */
)
{
    LOGBRUSH logbrush;
    
    TRACE("%04x\n", hbitmap );

    logbrush.lbStyle = BS_DIBPATTERN;
    logbrush.lbColor = coloruse;

    logbrush.lbHatch = (LONG)hbitmap;

    return CreateBrushIndirect( &logbrush );
}


/***********************************************************************
 *           CreateDIBPatternBrushPt    (GDI32.35)
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
		const void* data,		/* Pointer to a BITMAPINFO structure followed by more data */ 
		UINT coloruse 		/* Specifies color format, if provided */
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
 *           CreateSolidBrush    (GDI.66)
 */
HBRUSH16 WINAPI CreateSolidBrush16( COLORREF color )
{
    return CreateSolidBrush( color );
}


/***********************************************************************
 *           CreateSolidBrush    (GDI32.64)
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
 *           SetBrushOrg    (GDI.148)
 */
DWORD WINAPI SetBrushOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    DWORD retval;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    retval = dc->w.brushOrgX | (dc->w.brushOrgY << 16);
    dc->w.brushOrgX = x;
    dc->w.brushOrgY = y;
    return retval;
}


/***********************************************************************
 *           SetBrushOrgEx    (GDI32.308)
 */
BOOL WINAPI SetBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!dc) return FALSE;
    if (oldorg)
    {
        oldorg->x = dc->w.brushOrgX;
        oldorg->y = dc->w.brushOrgY;
    }
    dc->w.brushOrgX = x;
    dc->w.brushOrgY = y;
    return TRUE;
}

/***********************************************************************
 *           FixBrushOrgEx    (GDI32.102)
 * SDK says discontinued, but in Win95 GDI32 this is the same as SetBrushOrgEx
 */
BOOL WINAPI FixBrushOrgEx( HDC hdc, INT x, INT y, LPPOINT oldorg )
{
    return SetBrushOrgEx(hdc,x,y,oldorg);
}


/***********************************************************************
 *           BRUSH_DeleteObject
 */
BOOL BRUSH_DeleteObject( HBRUSH16 hbrush, BRUSHOBJ * brush )
{
    switch(brush->logbrush.lbStyle)
    {
      case BS_PATTERN:
	  DeleteObject( (HGDIOBJ)brush->logbrush.lbHatch );
	  break;
      case BS_DIBPATTERN:
	  GlobalFree16( (HGLOBAL16)brush->logbrush.lbHatch );
	  break;
    }
    return GDI_FreeObject( hbrush );
}


/***********************************************************************
 *           BRUSH_GetObject16
 */
INT16 BRUSH_GetObject16( BRUSHOBJ * brush, INT16 count, LPSTR buffer )
{
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
INT BRUSH_GetObject( BRUSHOBJ * brush, INT count, LPSTR buffer )
{
    if (count > sizeof(brush->logbrush)) count = sizeof(brush->logbrush);
    memcpy( buffer, &brush->logbrush, count );
    return count;
}


/***********************************************************************
 *           SetSolidBrush16   (GDI.604)
 *
 *  If hBrush is a solid brush, change it's color to newColor.
 *
 *  RETURNS
 *           TRUE on success, FALSE on failure.
 *   FIXME: not yet implemented!
 */
BOOL16 WINAPI SetSolidBrush16(HBRUSH16 hBrush, COLORREF newColor )
{
     FIXME("(hBrush %04x, newColor %04x): stub!\n", hBrush, (int)newColor);

     return(FALSE);
}

