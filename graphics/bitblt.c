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
#include "gdi_private.h"
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
        TRACE("hdcSrc=%p %d,%d -> hdcDest=%p %d,%d %dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, hdcDst, xDst, yDst, width, height, rop);
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

        TRACE("%p %d,%d %dx%d -> %p %d,%d %dx%d rop=%06lx\n",
              hdcSrc, xSrc, ySrc, widthSrc, heightSrc,
              hdcDst, xDst, yDst, widthDst, heightDst, rop );

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

static inline BYTE SwapROP3_SrcDst(BYTE bRop3)
{
    return (bRop3 & 0x99) | ((bRop3 & 0x22) << 1) | ((bRop3 & 0x44) >> 1);
}

#define FRGND_ROP3(ROP4)	((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)	(ROP3Table[(SwapROP3_SrcDst((ROP4)>>24)) & 0xFF])
#define DSTCOPY 		0x00AA0029
#define DSTERASE		0x00220326 /* dest = dest & (~src) : DSna */

/***********************************************************************
 *           MaskBlt [GDI32.@]
 */
BOOL WINAPI MaskBlt(HDC hdcDest, INT nXDest, INT nYDest,
                        INT nWidth, INT nHeight, HDC hdcSrc,
			INT nXSrc, INT nYSrc, HBITMAP hbmMask,
			INT xMask, INT yMask, DWORD dwRop)
{
    HBITMAP hOldMaskBitmap, hBitmap2, hOldBitmap2, hBitmap3, hOldBitmap3;
    HDC hDCMask, hDC1, hDC2;
    static const DWORD ROP3Table[256] = 
    {
        0x00000042, 0x00010289,
        0x00020C89, 0x000300AA,
        0x00040C88, 0x000500A9,
        0x00060865, 0x000702C5,
        0x00080F08, 0x00090245,
        0x000A0329, 0x000B0B2A,
        0x000C0324, 0x000D0B25,
        0x000E08A5, 0x000F0001,
        0x00100C85, 0x001100A6,
        0x00120868, 0x001302C8,
        0x00140869, 0x001502C9,
        0x00165CCA, 0x00171D54,
        0x00180D59, 0x00191CC8,
        0x001A06C5, 0x001B0768,
        0x001C06CA, 0x001D0766,
        0x001E01A5, 0x001F0385,
        0x00200F09, 0x00210248,
        0x00220326, 0x00230B24,
        0x00240D55, 0x00251CC5,
        0x002606C8, 0x00271868,
        0x00280369, 0x002916CA,
        0x002A0CC9, 0x002B1D58,
        0x002C0784, 0x002D060A,
        0x002E064A, 0x002F0E2A,
        0x0030032A, 0x00310B28,
        0x00320688, 0x00330008,
        0x003406C4, 0x00351864,
        0x003601A8, 0x00370388,
        0x0038078A, 0x00390604,
        0x003A0644, 0x003B0E24,
        0x003C004A, 0x003D18A4,
        0x003E1B24, 0x003F00EA,
        0x00400F0A, 0x00410249,
        0x00420D5D, 0x00431CC4,
        0x00440328, 0x00450B29,
        0x004606C6, 0x0047076A,
        0x00480368, 0x004916C5,
        0x004A0789, 0x004B0605,
        0x004C0CC8, 0x004D1954,
        0x004E0645, 0x004F0E25,
        0x00500325, 0x00510B26,
        0x005206C9, 0x00530764,
        0x005408A9, 0x00550009,
        0x005601A9, 0x00570389,
        0x00580785, 0x00590609,
        0x005A0049, 0x005B18A9,
        0x005C0649, 0x005D0E29,
        0x005E1B29, 0x005F00E9,
        0x00600365, 0x006116C6,
        0x00620786, 0x00630608,
        0x00640788, 0x00650606,
        0x00660046, 0x006718A8,
        0x006858A6, 0x00690145,
        0x006A01E9, 0x006B178A,
        0x006C01E8, 0x006D1785,
        0x006E1E28, 0x006F0C65,
        0x00700CC5, 0x00711D5C,
        0x00720648, 0x00730E28,
        0x00740646, 0x00750E26,
        0x00761B28, 0x007700E6,
        0x007801E5, 0x00791786,
        0x007A1E29, 0x007B0C68,
        0x007C1E24, 0x007D0C69,
        0x007E0955, 0x007F03C9,
        0x008003E9, 0x00810975,
        0x00820C49, 0x00831E04,
        0x00840C48, 0x00851E05,
        0x008617A6, 0x008701C5,
        0x008800C6, 0x00891B08,
        0x008A0E06, 0x008B0666,
        0x008C0E08, 0x008D0668,
        0x008E1D7C, 0x008F0CE5,
        0x00900C45, 0x00911E08,
        0x009217A9, 0x009301C4,
        0x009417AA, 0x009501C9,
        0x00960169, 0x0097588A,
        0x00981888, 0x00990066,
        0x009A0709, 0x009B07A8,
        0x009C0704, 0x009D07A6,
        0x009E16E6, 0x009F0345,
        0x00A000C9, 0x00A11B05,
        0x00A20E09, 0x00A30669,
        0x00A41885, 0x00A50065,
        0x00A60706, 0x00A707A5,
        0x00A803A9, 0x00A90189,
        0x00AA0029, 0x00AB0889,
        0x00AC0744, 0x00AD06E9,
        0x00AE0B06, 0x00AF0229,
        0x00B00E05, 0x00B10665,
        0x00B21974, 0x00B30CE8,
        0x00B4070A, 0x00B507A9,
        0x00B616E9, 0x00B70348,
        0x00B8074A, 0x00B906E6,
        0x00BA0B09, 0x00BB0226,
        0x00BC1CE4, 0x00BD0D7D,
        0x00BE0269, 0x00BF08C9,
        0x00C000CA, 0x00C11B04,
        0x00C21884, 0x00C3006A,
        0x00C40E04, 0x00C50664,
        0x00C60708, 0x00C707AA,
        0x00C803A8, 0x00C90184,
        0x00CA0749, 0x00CB06E4,
        0x00CC0020, 0x00CD0888,
        0x00CE0B08, 0x00CF0224,
        0x00D00E0A, 0x00D1066A,
        0x00D20705, 0x00D307A4,
        0x00D41D78, 0x00D50CE9,
        0x00D616EA, 0x00D70349,
        0x00D80745, 0x00D906E8,
        0x00DA1CE9, 0x00DB0D75,
        0x00DC0B04, 0x00DD0228,
        0x00DE0268, 0x00DF08C8,
        0x00E003A5, 0x00E10185,
        0x00E20746, 0x00E306EA,
        0x00E40748, 0x00E506E5,
        0x00E61CE8, 0x00E70D79,
        0x00E81D74, 0x00E95CE6,
        0x00EA02E9, 0x00EB0849,
        0x00EC02E8, 0x00ED0848,
        0x00EE0086, 0x00EF0A08,
        0x00F00021, 0x00F10885,
        0x00F20B05, 0x00F3022A,
        0x00F40B0A, 0x00F50225,
        0x00F60265, 0x00F708C5,
        0x00F802E5, 0x00F90845,
        0x00FA0089, 0x00FB0A09,
        0x00FC008A, 0x00FD0A0A,
        0x00FE02A9, 0x00FF0062,
    };

    if (!hbmMask)
	return BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop));

    /* 1. make mask bitmap's dc */
    hDCMask = CreateCompatibleDC(hdcDest);
    hOldMaskBitmap = (HBITMAP)SelectObject(hDCMask, hbmMask);

    /* 2. make masked Background bitmap */

    /* 2.1 make bitmap */
    hDC1 = CreateCompatibleDC(hdcDest);
    hBitmap2 = CreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    hOldBitmap2 = (HBITMAP)SelectObject(hDC1, hBitmap2);

    /* 2.2 draw dest bitmap and mask */
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY);
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, BKGND_ROP3(dwRop));
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hDCMask, xMask, yMask, DSTERASE);

    /* 3. make masked Foreground bitmap */

    /* 3.1 make bitmap */
    hDC2 = CreateCompatibleDC(hdcDest);
    hBitmap3 = CreateCompatibleBitmap(hdcDest, nWidth, nHeight);
    hOldBitmap3 = (HBITMAP)SelectObject(hDC2, hBitmap3);

    /* 3.2 draw src bitmap and mask */
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY);
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop));
    BitBlt(hDC2, 0, 0, nWidth, nHeight, hDCMask, xMask, yMask, SRCAND);

    /* 4. combine two bitmap and copy it to hdcDest */
    BitBlt(hDC1, 0, 0, nWidth, nHeight, hDC2, 0, 0, SRCPAINT);
    BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hDC1, 0, 0, SRCCOPY);

    /* 5. restore all object */
    SelectObject(hDCMask, hOldMaskBitmap);
    SelectObject(hDC1, hOldBitmap2);
    SelectObject(hDC2, hOldBitmap3);

    /* 6. delete all temp object */
    DeleteObject(hBitmap2);
    DeleteObject(hBitmap3);

    DeleteDC(hDC1);
    DeleteDC(hDC2);
    DeleteDC(hDCMask);

    return TRUE;
}

/******************************************************************************
 *           GdiTransparentBlt [GDI32.@]
 */
BOOL WINAPI GdiTransparentBlt( HDC hdcDest, int xDest, int yDest, int widthDest, int heightDest,
                            HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                            UINT crTransparent )
{
    BOOL ret = FALSE;
    HDC hdcWork;
    HBITMAP bmpWork;
    HGDIOBJ oldWork;
    HDC hdcMask = NULL;
    HBITMAP bmpMask = NULL;
    HBITMAP oldMask = NULL;
    COLORREF oldBackground;
    COLORREF oldForeground;
    int oldStretchMode;

    if(widthDest < 0 || heightDest < 0 || widthSrc < 0 || heightSrc < 0) {
        TRACE("Can not mirror\n");
        return FALSE;
    }

    oldBackground = SetBkColor(hdcDest, RGB(255,255,255));
    oldForeground = SetTextColor(hdcDest, RGB(0,0,0));

    /* Stretch bitmap */
    oldStretchMode = GetStretchBltMode(hdcSrc);
    if(oldStretchMode == BLACKONWHITE || oldStretchMode == WHITEONBLACK)
        SetStretchBltMode(hdcSrc, COLORONCOLOR);
    hdcWork = CreateCompatibleDC(hdcDest);
    bmpWork = CreateCompatibleBitmap(hdcDest, widthDest, heightDest);
    oldWork = SelectObject(hdcWork, bmpWork);
    if(!StretchBlt(hdcWork, 0, 0, widthDest, heightDest, hdcSrc, xSrc, ySrc, widthSrc, heightSrc, SRCCOPY)) {
        TRACE("Failed to stretch\n");
        goto error;
    }
    SetBkColor(hdcWork, crTransparent);

    /* Create mask */
    hdcMask = CreateCompatibleDC(hdcDest);
    bmpMask = CreateCompatibleBitmap(hdcMask, widthDest, heightDest);
    oldMask = SelectObject(hdcMask, bmpMask);
    if(!BitBlt(hdcMask, 0, 0, widthDest, heightDest, hdcWork, 0, 0, SRCCOPY)) {
        TRACE("Failed to create mask\n");
        goto error;
    }

    /* Replace transparent color with black */
    SetBkColor(hdcWork, RGB(0,0,0));
    SetTextColor(hdcWork, RGB(255,255,255));
    if(!BitBlt(hdcWork, 0, 0, widthDest, heightDest, hdcMask, 0, 0, SRCAND)) {
        TRACE("Failed to mask out background\n");
        goto error;
    }

    /* Replace non-transparent area on destination with black */
    if(!BitBlt(hdcDest, xDest, yDest, widthDest, heightDest, hdcMask, 0, 0, SRCAND)) {
        TRACE("Failed to clear destination area\n");
        goto error;
    }

    /* Draw the image */
    if(!BitBlt(hdcDest, xDest, yDest, widthDest, heightDest, hdcWork, 0, 0, SRCPAINT)) {
        TRACE("Failed to paint image\n");
        goto error;
    }

    ret = TRUE;
error:
    SetStretchBltMode(hdcSrc, oldStretchMode);
    SetBkColor(hdcDest, oldBackground);
    SetTextColor(hdcDest, oldForeground);
    if(hdcWork) {
        SelectObject(hdcWork, oldWork);
        DeleteDC(hdcWork);
    }
    if(bmpWork) DeleteObject(bmpWork);
    if(hdcMask) {
        SelectObject(hdcMask, oldMask);
        DeleteDC(hdcMask);
    }
    if(bmpMask) DeleteObject(bmpMask);
    return ret;
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
