/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "gdi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(bitblt);


/***********************************************************************
 *           PatBlt    (GDI.29)
 */
BOOL16 WINAPI PatBlt16( HDC16 hdc, INT16 left, INT16 top,
                        INT16 width, INT16 height, DWORD rop)
{
    return PatBlt( hdc, left, top, width, height, rop );
}


/***********************************************************************
 *           PatBlt    (GDI32.@)
 */
BOOL WINAPI PatBlt( HDC hdc, INT left, INT top,
                        INT width, INT height, DWORD rop)
{
    DC * dc = DC_GetDCUpdate( hdc );
    BOOL bRet = FALSE;

    if (!dc) return FALSE;

    if (dc->funcs->pPatBlt)
    {
        TRACE("%04x %d,%d %dx%d %06lx\n", hdc, left, top, width, height, rop );
        bRet = dc->funcs->pPatBlt( dc, left, top, width, height, rop );
    }
    GDI_ReleaseObj( hdc );
    return bRet;
}


/***********************************************************************
 *           BitBlt    (GDI.34)
 */
BOOL16 WINAPI BitBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst, INT16 width,
                        INT16 height, HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                        DWORD rop )
{
    return BitBlt( hdcDst, xDst, yDst, width, height, hdcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           BitBlt    (GDI32.@)
 */
BOOL WINAPI BitBlt( HDC hdcDst, INT xDst, INT yDst, INT width,
                    INT height, HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop )
{
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    if ((dcSrc = DC_GetDCUpdate( hdcSrc ))) GDI_ReleaseObj( hdcSrc );
    /* FIXME: there is a race condition here */
    if ((dcDst = DC_GetDCUpdate( hdcDst )))
    {
        dcSrc = DC_GetDCPtr( hdcSrc );
        TRACE("hdcSrc=%04x %d,%d %d bpp->hdcDest=%04x %d,%d %dx%dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, dcSrc ? dcSrc->bitsPerPixel : 0,
              hdcDst, xDst, yDst, width, height, dcDst->bitsPerPixel, rop);
        if (dcDst->funcs->pBitBlt)
            ret = dcDst->funcs->pBitBlt( dcDst, xDst, yDst, width, height,
                                         dcSrc, xSrc, ySrc, rop );
        if (dcSrc) GDI_ReleaseObj( hdcSrc );
        GDI_ReleaseObj( hdcDst );
    }
    return ret;
}


/***********************************************************************
 *           StretchBlt    (GDI.35)
 */
BOOL16 WINAPI StretchBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst,
                            INT16 widthDst, INT16 heightDst,
                            HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                            INT16 widthSrc, INT16 heightSrc, DWORD rop )
{
    return StretchBlt( hdcDst, xDst, yDst, widthDst, heightDst,
                       hdcSrc, xSrc, ySrc, widthSrc, heightSrc, rop );
}


/***********************************************************************
 *           StretchBlt    (GDI32.@)
 */
BOOL WINAPI StretchBlt( HDC hdcDst, INT xDst, INT yDst,
                            INT widthDst, INT heightDst,
                            HDC hdcSrc, INT xSrc, INT ySrc,
                            INT widthSrc, INT heightSrc, 
			DWORD rop )
{
    BOOL ret = FALSE;
    DC *dcDst, *dcSrc;

    if ((dcSrc = DC_GetDCUpdate( hdcSrc ))) GDI_ReleaseObj( hdcSrc );
    /* FIXME: there is a race condition here */
    if ((dcDst = DC_GetDCUpdate( hdcDst )))
    {
        dcSrc = DC_GetDCPtr( hdcSrc );

        TRACE("%04x %d,%d %dx%dx%d -> %04x %d,%d %dx%dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, widthSrc, heightSrc,
              dcSrc ? dcSrc->bitsPerPixel : 0, hdcDst, xDst, yDst,
              widthDst, heightDst, dcDst->bitsPerPixel, rop );

	if (dcSrc) {
	    if (dcDst->funcs->pStretchBlt)
		ret = dcDst->funcs->pStretchBlt( dcDst, xDst, yDst, widthDst, heightDst,
						 dcSrc, xSrc, ySrc, widthSrc, heightSrc, rop );
	    GDI_ReleaseObj( hdcSrc );
	}
        GDI_ReleaseObj( hdcDst );
    }
    return ret;
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
    PatBlt( hdc, rect->left + width, rect->bottom - 1,
              rect->right - rect->left - width, -height, rop );
    PatBlt( hdc, rect->right - 1, rect->top, -width,
              rect->bottom - rect->top - height, rop );
    SelectObject( hdc, hbrush );
    return TRUE;

}


/***********************************************************************
 *           MaskBlt [GDI32.@]
 */
BOOL WINAPI MaskBlt(HDC hdcDest, INT nXDest, INT nYDest,
                        INT nWidth, INT nHeight, HDC hdcSource,
			INT nXSrc, INT nYSrc, HBITMAP hbmMask,
			INT xMask, INT yMask, DWORD dwRop)
{
    FIXME("(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld): stub\n",
             hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSource,nXSrc,nYSrc,
             hbmMask,xMask,yMask,dwRop);
    return 1;
}
  
/*********************************************************************
 *      PlgBlt [GDI32.@]
 *
 */
BOOL WINAPI PlgBlt( HDC hdcDest, const POINT *lpPoint,
                        HDC hdcSrc, INT nXDest, INT nYDest, INT nWidth,
                        INT nHeight, HBITMAP hbmMask, INT xMask, INT yMask)
{
    FIXME("PlgBlt, stub\n");
        return 1;
}
 
