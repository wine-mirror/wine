/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "dc.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           PatBlt16    (GDI.29)
 */
BOOL16 WINAPI PatBlt16( HDC16 hdc, INT16 left, INT16 top,
                        INT16 width, INT16 height, DWORD rop)
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pPatBlt) return FALSE;

    dprintf_bitblt( stddeb, "PatBlt16: %04x %d,%d %dx%d %06lx\n",
                    hdc, left, top, width, height, rop );
    return dc->funcs->pPatBlt( dc, left, top, width, height, rop );
}


/***********************************************************************
 *           PatBlt32    (GDI32.260)
 */
BOOL32 WINAPI PatBlt32( HDC32 hdc, INT32 left, INT32 top,
                        INT32 width, INT32 height, DWORD rop)
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pPatBlt) return FALSE;

    dprintf_bitblt( stddeb, "PatBlt32: %04x %d,%d %dx%d %06lx\n",
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

    dprintf_bitblt(stddeb,
                "BitBlt16: hdcSrc=%04x %d,%d %d bpp -> hdcDest=%04x %d,%d %dx%dx%d rop=%06lx\n",
                hdcSrc, xSrc, ySrc, dcSrc ? dcSrc->w.bitsPerPixel : 0,
                hdcDst, xDst, yDst, width, height, dcDst->w.bitsPerPixel, rop);
    return dcDst->funcs->pBitBlt( dcDst, xDst, yDst, width, height,
                                  dcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           BitBlt32    (GDI32.10)
 */
BOOL32 WINAPI BitBlt32( HDC32 hdcDst, INT32 xDst, INT32 yDst, INT32 width,
                        INT32 height, HDC32 hdcSrc, INT32 xSrc, INT32 ySrc,
                        DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pBitBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    dprintf_bitblt(stddeb,
                "BitBlt32: hdcSrc=%04x %d,%d %d bpp -> hdcDest=%04x %d,%d %dx%dx%d rop=%06lx\n",
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

    dprintf_bitblt(stddeb,
        "StretchBlt16: %04x %d,%d %dx%dx%d -> %04x %d,%d %dx%dx%d rop=%06lx\n",
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
BOOL32 WINAPI StretchBlt32( HDC32 hdcDst, INT32 xDst, INT32 yDst,
                            INT32 widthDst, INT32 heightDst,
                            HDC32 hdcSrc, INT32 xSrc, INT32 ySrc,
                            INT32 widthSrc, INT32 heightSrc, DWORD rop )
{
    DC *dcDst, *dcSrc;

    if (!(dcDst = DC_GetDCPtr( hdcDst ))) return FALSE;
    if (!dcDst->funcs->pStretchBlt) return FALSE;
    dcSrc = DC_GetDCPtr( hdcSrc );

    dprintf_bitblt(stddeb,
        "StretchBlt32: %04x %d,%d %dx%dx%d -> %04x %d,%d %dx%dx%d rop=%06lx\n",
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
BOOL16 WINAPI FastWindowFrame( HDC16 hdc, const RECT16 *rect,
                               INT16 width, INT16 height, DWORD rop )
{
    HBRUSH32 hbrush = SelectObject32( hdc, GetStockObject32( GRAY_BRUSH ) );
    PatBlt32( hdc, rect->left, rect->top,
              rect->right - rect->left - width, height, rop );
    PatBlt32( hdc, rect->left, rect->top + height, width,
              rect->bottom - rect->top - height, rop );
    PatBlt32( hdc, rect->left + width, rect->bottom,
              rect->right - rect->left - width, -height, rop );
    PatBlt32( hdc, rect->right, rect->top, -width, 
              rect->bottom - rect->top - height, rop );
    SelectObject32( hdc, hbrush );
    return TRUE;
}
