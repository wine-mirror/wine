/*
 * GDI bit-blit operations
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

#include "gdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);


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
        TRACE("%p %d,%d %dx%d %06lx\n", hdc, left, top, width, height, rop );
        bRet = dc->funcs->pPatBlt( dc->physDev, left, top, width, height, rop );
    }
    GDI_ReleaseObj( hdc );
    return bRet;
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
        TRACE("hdcSrc=%p %d,%d %d bpp->hdcDest=%p %d,%d %dx%dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, dcSrc ? dcSrc->bitsPerPixel : 0,
              hdcDst, xDst, yDst, width, height, dcDst->bitsPerPixel, rop);
        if (dcDst->funcs->pBitBlt)
            ret = dcDst->funcs->pBitBlt( dcDst->physDev, xDst, yDst, width, height,
                                         dcSrc ? dcSrc->physDev : NULL, xSrc, ySrc, rop );
        if (dcSrc) GDI_ReleaseObj( hdcSrc );
        GDI_ReleaseObj( hdcDst );
    }
    return ret;
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

        TRACE("%p %d,%d %dx%dx%d -> %p %d,%d %dx%dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, widthSrc, heightSrc,
              dcSrc ? dcSrc->bitsPerPixel : 0, hdcDst, xDst, yDst,
              widthDst, heightDst, dcDst->bitsPerPixel, rop );

	if (dcSrc) {
	    if (dcDst->funcs->pStretchBlt)
		ret = dcDst->funcs->pStretchBlt( dcDst->physDev, xDst, yDst, widthDst, heightDst,
                                                 dcSrc->physDev, xSrc, ySrc, widthSrc, heightSrc,
                                                 rop );
	    GDI_ReleaseObj( hdcSrc );
	}
        GDI_ReleaseObj( hdcDst );
    }
    return ret;
}


/***********************************************************************
 *           MaskBlt [GDI32.@]
 */
BOOL WINAPI MaskBlt(HDC hdcDest, INT nXDest, INT nYDest,
                        INT nWidth, INT nHeight, HDC hdcSource,
			INT nXSrc, INT nYSrc, HBITMAP hbmMask,
			INT xMask, INT yMask, DWORD dwRop)
{
    FIXME("(%p,%d,%d,%d,%d,%p,%d,%d,%p,%d,%d,%ld): stub\n",
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
