/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "dc.h"
#include "debug.h"

DECLARE_DEBUG_CHANNEL(bitblt)
DECLARE_DEBUG_CHANNEL(bitmap)
DECLARE_DEBUG_CHANNEL(gdi)


/***********************************************************************
 *           PatBlt16    (GDI.29)
 */
BOOL16 WINAPI PatBlt16( HDC16 hdc, INT16 left, INT16 top,
                        INT16 width, INT16 height, DWORD rop)
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pPatBlt) return FALSE;

    TRACE(bitblt, "%04x %d,%d %dx%d %06lx\n",
                    hdc, left, top, width, height, rop );
    return dc->funcs->pPatBlt( dc, left, top, width, height, rop );
}


/***********************************************************************
 *           PatBlt32    (GDI32.260)
 */
BOOL WINAPI PatBlt( HDC hdc, INT left, INT top,
                        INT width, INT height, DWORD rop)
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pPatBlt) return FALSE;

    TRACE(bitblt, "%04x %d,%d %dx%d %06lx\n",
                    hdc, left, top, width, height, rop );
    return dc->funcs->pPatBlt( dc, left, top, width, height, rop );
}


/***********************************************************************
 *           BitBlt16    (GDI.34)
 */
BOOL16 WINAPI BitBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst, INT16 width,
                        INT16 height, HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                        DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pBitBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    TRACE(bitblt, "hdcSrc=%04x %d,%d %d bpp -> hdcDest=%04x %d,%d %dx%dx%d rop=%06lx\n",
		 hdcSrc, xSrc, ySrc, dcSrc ? dcSrc->w.bitsPerPixel : 0,
		 hdcDst, xDst, yDst, width, height, dcDst->w.bitsPerPixel, rop);
    return dcDst->funcs->pBitBlt( dcDst, xDst, yDst, width, height,
                                  dcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           BitBlt32    (GDI32.10)
 */
BOOL WINAPI BitBlt( HDC hdcDst, INT xDst, INT yDst, INT width,
                        INT height, HDC hdcSrc, INT xSrc, INT ySrc,
                        DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pBitBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    TRACE(bitblt, "hdcSrc=%04x %d,%d %d bpp -> hdcDest=%04x %d,%d %dx%dx%d rop=%06lx\n",
		 hdcSrc, xSrc, ySrc, dcSrc ? dcSrc->w.bitsPerPixel : 0,
		 hdcDst, xDst, yDst, width, height, dcDst->w.bitsPerPixel, rop);
    return dcDst->funcs->pBitBlt( dcDst, xDst, yDst, width, height,
                                  dcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           StretchBlt16    (GDI.35)
 */
BOOL16 WINAPI StretchBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst,
                            INT16 widthDst, INT16 heightDst,
                            HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                            INT16 widthSrc, INT16 heightSrc, DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pStretchBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    TRACE(bitblt, "%04x %d,%d %dx%dx%d -> %04x %d,%d %dx%dx%d rop=%06lx\n",
		 hdcSrc, xSrc, ySrc, widthSrc, heightSrc,
		 dcSrc ? dcSrc->w.bitsPerPixel : 0, hdcDst, xDst, yDst,
		 widthDst, heightDst, dcDst->w.bitsPerPixel, rop );
    return dcDst->funcs->pStretchBlt( dcDst, xDst, yDst, widthDst, heightDst,
                                      dcSrc, xSrc, ySrc, widthSrc, heightSrc,
                                      rop );
}


/***********************************************************************
 *           StretchBlt32    (GDI32.350)
 */
BOOL WINAPI StretchBlt( HDC hdcDst, INT xDst, INT yDst,
                            INT widthDst, INT heightDst,
                            HDC hdcSrc, INT xSrc, INT ySrc,
                            INT widthSrc, INT heightSrc, DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pStretchBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    TRACE(bitblt, "%04x %d,%d %dx%dx%d -> %04x %d,%d %dx%dx%d rop=%06lx\n",
		 hdcSrc, xSrc, ySrc, widthSrc, heightSrc,
		 dcSrc ? dcSrc->w.bitsPerPixel : 0, hdcDst, xDst, yDst,
		 widthDst, heightDst, dcDst->w.bitsPerPixel, rop );
    return dcDst->funcs->pStretchBlt( dcDst, xDst, yDst, widthDst, heightDst,
                                      dcSrc, xSrc, ySrc, widthSrc, heightSrc,
                                      rop );
}


/***********************************************************************
 *           FastWindowFrame    (GDI.400)
 */
BOOL16 WINAPI FastWindowFrame16( HDC16 hdc, const RECT16 *rect,
                               INT16 width, INT16 height, DWORD rop )
{
    HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left - width, height, rop );
    PatBlt( hdc, rect->left, rect->top + height, width,
              rect->bottom - rect->top - height, rop );
    PatBlt( hdc, rect->left + width, rect->bottom,
              rect->right - rect->left - width, -height, rop );
    PatBlt( hdc, rect->right, rect->top, -width, 
              rect->bottom - rect->top - height, rop );
    SelectObject( hdc, hbrush );
    return TRUE;
}
/***********************************************************************
 *           MaskBlt32 [GDI32.252]
 */
BOOL WINAPI MaskBlt(HDC hdcDest, INT nXDest, INT nYDest,
                        INT nWidth, INT nHeight, HDC hdcSource,
			INT nXSrc, INT nYSrc, HBITMAP hbmMask,
			INT xMask, INT yMask, DWORD dwRop)
{
    FIXME(bitmap, "(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld): stub\n",
             hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSource,nXSrc,nYSrc,
             hbmMask,xMask,yMask,dwRop);
    return 1;
}
  
/*********************************************************************
 *      PlgBlt [GDI.267]
 *
 */
BOOL WINAPI PlgBlt( HDC hdcDest, const POINT *lpPoint,
                        HDC hdcSrc, INT nXDest, INT nYDest, INT nWidth,
                        INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask)
{
        FIXME(gdi, "PlgBlt, stub\n");
        return 1;
}
 
