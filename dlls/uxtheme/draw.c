/*
 * Win32 5.1 Theme drawing
 *
 * Copyright (C) 2003 Kevin Koltzau
 * Copyright 2021 Zhiyi Zhang for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "commctrl.h"
#include "commoncontrols.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include "vssym32.h"

#include "msstyles.h"
#include "uxthemedll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

extern ATOM atDialogThemeEnabled;

/***********************************************************************/

/***********************************************************************
 *      EnableThemeDialogTexture                            (UXTHEME.@)
 */
HRESULT WINAPI EnableThemeDialogTexture(HWND hwnd, DWORD new_flag)
{
    DWORD old_flag = 0;
    BOOL res;

    TRACE("(%p,%#lx\n", hwnd, new_flag);

    new_flag &= ETDT_VALIDBITS;

    if (new_flag == 0)
        return S_OK;

    if (new_flag & ETDT_DISABLE)
    {
        new_flag = ETDT_DISABLE;
        old_flag = 0;
    }

    if (new_flag & ~ETDT_DISABLE)
    {
        old_flag = HandleToUlong(GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled)));
        old_flag &= ~ETDT_DISABLE;
    }

    new_flag = new_flag | old_flag;
    res = SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled), UlongToHandle(new_flag));
    return res ? S_OK : HRESULT_FROM_WIN32(GetLastError());
 }

/***********************************************************************
 *      IsThemeDialogTextureEnabled                         (UXTHEME.@)
 */
BOOL WINAPI IsThemeDialogTextureEnabled(HWND hwnd)
{
    DWORD dwDialogTextureFlags;

    TRACE("(%p)\n", hwnd);

    dwDialogTextureFlags = HandleToUlong( GetPropW( hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled) ));
    return dwDialogTextureFlags && !(dwDialogTextureFlags & ETDT_DISABLE);
}

/***********************************************************************
 *      DrawThemeParentBackground                           (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeParentBackground(HWND hwnd, HDC hdc, RECT *prc)
{
    RECT rt;
    POINT org;
    HWND hParent;
    HRGN clip = NULL;
    int hasClip = -1;
    
    TRACE("(%p,%p,%p)\n", hwnd, hdc, prc);
    hParent = GetParent(hwnd);
    if(!hParent)
        hParent = hwnd;
    if(prc) {
        rt = *prc;
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);

        clip = CreateRectRgn(0,0,1,1);
        hasClip = GetClipRgn(hdc, clip);
        if(hasClip == -1)
            TRACE("Failed to get original clipping region\n");
        else
            IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }
    else {
        GetClientRect(hwnd, &rt);
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);
    }

    OffsetViewportOrgEx(hdc, -rt.left, -rt.top, &org);

    SendMessageW(hParent, WM_ERASEBKGND, (WPARAM)hdc, 0);
    SendMessageW(hParent, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);

    SetViewportOrgEx(hdc, org.x, org.y, NULL);
    if(prc) {
        if(hasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if(hasClip == 1)
            SelectClipRgn(hdc, clip);
        DeleteObject(clip);
    }
    return S_OK;
}


/***********************************************************************
 *      DrawThemeBackground                                 (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, const RECT *pRect,
                                   const RECT *pClipRect)
{
    DTBGOPTS opts;
    opts.dwSize = sizeof(DTBGOPTS);
    opts.dwFlags = 0;
    if(pClipRect) {
        opts.dwFlags |= DTBG_CLIPRECT;
        opts.rcClip = *pClipRect;
    }
    return DrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, &opts);
}

/* Map integer 1..7 to TMT_MINDPI1..TMT_MINDPI7 */
static int mindpi_index_to_property(int index)
{
    return index <= 5 ? TMT_MINDPI1 + index - 1 : TMT_MINDPI6 + index - 6;
}

/* Map integer 1..7 to TMT_MINSIZE1..TMT_MINSIZE7 */
static int minsize_index_to_property(int index)
{
    return index <= 5 ? TMT_MINSIZE1 + index - 1 : TMT_MINSIZE6 + index - 6;
}

/* Map integer 1..7 to TMT_IMAGEFILE1..TMT_IMAGEFILE7 */
static int imagefile_index_to_property(int index)
{
    return index <= 5 ? TMT_IMAGEFILE1 + index - 1 : TMT_IMAGEFILE6 + index - 6;
}

/***********************************************************************
 *      UXTHEME_SelectImage
 *
 * Select the image to use
 */
static PTHEME_PROPERTY UXTHEME_SelectImage(HTHEME hTheme, int iPartId, int iStateId,
                                           const RECT *pRect, BOOL glyph, int *imageDpi)
{
    int imageselecttype = IST_NONE, glyphtype = GT_NONE;
    PTHEME_PROPERTY tp = NULL;
    int i;

    if (imageDpi)
        *imageDpi = 96;

    /* Try TMT_IMAGEFILE first when drawing part background and the part contains glyph images.
     * Otherwise, search TMT_IMAGEFILE1~7 and then TMT_IMAGEFILE or TMT_GLYPHIMAGEFILE */
    if (!glyph)
    {
        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_GLYPHTYPE, &glyphtype);
        if (glyphtype == GT_IMAGEGLYPH &&
            (tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE)))
                return tp;
    }

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGESELECTTYPE, &imageselecttype);

    if(imageselecttype == IST_DPI) {
        int reqdpi = 0;
        int dpi = MSSTYLES_GetThemeDPI(hTheme);
        for (i = 7; i >= 1; i--)
        {
            reqdpi = 0;
            if (SUCCEEDED(GetThemeInt(hTheme, iPartId, iStateId, mindpi_index_to_property(i),
                                      &reqdpi)))
            {
                if (reqdpi != 0 && dpi >= reqdpi)
                {
                    TRACE("Using %d DPI, image %d\n", reqdpi, imagefile_index_to_property(i));

                    if (imageDpi)
                        *imageDpi = reqdpi;

                    return MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME,
                                                 imagefile_index_to_property(i));
                }
            }
        }
        /* If an image couldn't be selected, choose the first one */
        tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }
    else if(imageselecttype == IST_SIZE) {
        POINT size = {pRect->right-pRect->left, pRect->bottom-pRect->top};
        POINT reqsize;
        for (i = 7; i >= 1; i--)
        {
            PTHEME_PROPERTY fileProp;

            fileProp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME,
                                             imagefile_index_to_property(i));
            if (!fileProp) continue;
            if (FAILED(GetThemePosition(hTheme, iPartId, iStateId, minsize_index_to_property(i),
                                        &reqsize)))
            {
                /* fall back to size of Nth image */
                WCHAR szPath[MAX_PATH];
                int imagelayout = IL_HORIZONTAL;
                int imagecount = 1;
                BITMAP bmp;
                HBITMAP hBmp;
                BOOL hasAlpha;

                lstrcpynW(szPath, fileProp->lpValue, min(fileProp->dwValueLen+1, ARRAY_SIZE(szPath)));
                hBmp = MSSTYLES_LoadBitmap(hTheme, szPath, &hasAlpha);
                if(!hBmp) continue;

                GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGELAYOUT, &imagelayout);
                GetThemeInt(hTheme, iPartId, iStateId, TMT_IMAGECOUNT, &imagecount);

                GetObjectW(hBmp, sizeof(bmp), &bmp);
                if(imagelayout == IL_VERTICAL) {
                    reqsize.x = bmp.bmWidth;
                    reqsize.y = bmp.bmHeight/imagecount;
                }
                else {
                    reqsize.x = bmp.bmWidth/imagecount;
                    reqsize.y = bmp.bmHeight;
                }
            }
            if(reqsize.x <= size.x && reqsize.y <= size.y) {
                TRACE("Using image size %ldx%ld, image %d\n", reqsize.x, reqsize.y,
                      imagefile_index_to_property(i));
                return fileProp;
            }
        }
        /* If an image couldn't be selected, choose the smallest one */
        tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }

    if (!tp)
        tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME,
                                   glyph ? TMT_GLYPHIMAGEFILE : TMT_IMAGEFILE);
    return tp;
}

/***********************************************************************
 *      UXTHEME_LoadImage
 *
 * Load image for part/state
 */
static HRESULT UXTHEME_LoadImage(HTHEME hTheme, int iPartId, int iStateId, const RECT *pRect,
                                 BOOL glyph, HBITMAP *hBmp, RECT *bmpRect, BOOL *hasImageAlpha,
                                 int *imageDpi)
{
    int imagelayout = IL_HORIZONTAL;
    int imagecount = 1;
    int imagenum;
    BITMAP bmp;
    WCHAR szPath[MAX_PATH];
    PTHEME_PROPERTY tp;

    tp = UXTHEME_SelectImage(hTheme, iPartId, iStateId, pRect, glyph, imageDpi);
    if(!tp) {
        FIXME("Couldn't determine image for part/state %d/%d, invalid theme?\n", iPartId, iStateId);
        return E_PROP_ID_UNSUPPORTED;
    }
    lstrcpynW(szPath, tp->lpValue, min(tp->dwValueLen+1, ARRAY_SIZE(szPath)));
    *hBmp = MSSTYLES_LoadBitmap(hTheme, szPath, hasImageAlpha);
    if(!*hBmp) {
        TRACE("Failed to load bitmap %s\n", debugstr_w(szPath));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGELAYOUT, &imagelayout);
    GetThemeInt(hTheme, iPartId, iStateId, TMT_IMAGECOUNT, &imagecount);

    imagenum = max (min (imagecount, iStateId), 1) - 1;
    GetObjectW(*hBmp, sizeof(bmp), &bmp);

    if(imagecount < 1) imagecount = 1;

    if(imagelayout == IL_VERTICAL) {
        int height = bmp.bmHeight/imagecount;
        bmpRect->left = 0;
        bmpRect->right = bmp.bmWidth;
        bmpRect->top = imagenum * height;
        bmpRect->bottom = bmpRect->top + height;
    }
    else {
        int width = bmp.bmWidth/imagecount;
        bmpRect->left = imagenum * width;
        bmpRect->right = bmpRect->left + width;
        bmpRect->top = 0;
        bmpRect->bottom = bmp.bmHeight;
    }
    return S_OK;
}

/***********************************************************************
 *      UXTHEME_StretchBlt
 *
 * Pseudo TransparentBlt/StretchBlt
 */
static inline BOOL UXTHEME_StretchBlt(HDC hdcDst, int nXOriginDst, int nYOriginDst, int nWidthDst, int nHeightDst,
                                      HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                                      INT transparent, COLORREF transcolor)
{
    static const BLENDFUNCTION blendFunc = 
    {
      AC_SRC_OVER, /* BlendOp */
      0,           /* BlendFlag */
      255,         /* SourceConstantAlpha */
      AC_SRC_ALPHA /* AlphaFormat */
    };

    BOOL ret = TRUE;
    int old_stretch_mode;
    POINT old_brush_org;

    old_stretch_mode = SetStretchBltMode(hdcDst, HALFTONE);
    SetBrushOrgEx(hdcDst, nXOriginDst, nYOriginDst, &old_brush_org);

    if (transparent == ALPHABLEND_BINARY) {
        /* Ensure we don't pass any negative values to TransparentBlt */
        ret = TransparentBlt(hdcDst, nXOriginDst, nYOriginDst, abs(nWidthDst), abs(nHeightDst),
                              hdcSrc, nXOriginSrc, nYOriginSrc, abs(nWidthSrc), abs(nHeightSrc),
                              transcolor);
    } else if ((transparent == ALPHABLEND_NONE) ||
        !AlphaBlend(hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                    hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                    blendFunc))
    {
        ret = StretchBlt(hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                          hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                          SRCCOPY);
    }

    SetBrushOrgEx(hdcDst, old_brush_org.x, old_brush_org.y, NULL);
    SetStretchBltMode(hdcDst, old_stretch_mode);

    return ret;
}

/***********************************************************************
 *      UXTHEME_Blt
 *
 * Simplify sending same width/height for both source and dest
 */
static inline BOOL UXTHEME_Blt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                               HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
                               INT transparent, COLORREF transcolor)
{
    return UXTHEME_StretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                              hdcSrc, nXOriginSrc, nYOriginSrc, nWidthDest, nHeightDest,
                              transparent, transcolor);
}

/***********************************************************************
 *      UXTHEME_SizedBlt
 *
 * Stretches or tiles, depending on sizingtype.
 */
static inline BOOL UXTHEME_SizedBlt (HDC hdcDst, int nXOriginDst, int nYOriginDst, 
                                     int nWidthDst, int nHeightDst,
                                     HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, 
                                     int nWidthSrc, int nHeightSrc,
                                     int sizingtype, 
                                     INT transparent, COLORREF transcolor)
{
    if (sizingtype == ST_TILE)
    {
        HDC hdcTemp;
        BOOL result = FALSE;

        if (!nWidthSrc || !nHeightSrc) return TRUE;

        /* For destination width/height less than or equal to source
           width/height, do not bother with memory bitmap optimization */
        if (nWidthSrc >= nWidthDst && nHeightSrc >= nHeightDst)
        {
            int bltWidth = min (nWidthDst, nWidthSrc);
            int bltHeight = min (nHeightDst, nHeightSrc);

            return UXTHEME_Blt (hdcDst, nXOriginDst, nYOriginDst, bltWidth, bltHeight,
                                hdcSrc, nXOriginSrc, nYOriginSrc,
                                transparent, transcolor);
        }

        /* Create a DC with a bitmap consisting of a tiling of the source
           bitmap, with standard GDI functions. This is faster than an
           iteration with UXTHEME_Blt(). */
        hdcTemp = CreateCompatibleDC(hdcSrc);
        if (hdcTemp != 0)
        {
            HBITMAP bitmapTemp;
            HBITMAP bitmapOrig;
            int nWidthTemp, nHeightTemp;
            int xOfs, xRemaining;
            int yOfs, yRemaining;
            int growSize;

            /* Calculate temp dimensions of integer multiples of source dimensions */
            nWidthTemp = ((nWidthDst + nWidthSrc - 1) / nWidthSrc) * nWidthSrc;
            nHeightTemp = ((nHeightDst + nHeightSrc - 1) / nHeightSrc) * nHeightSrc;
            bitmapTemp = CreateCompatibleBitmap(hdcSrc, nWidthTemp, nHeightTemp);
            bitmapOrig = SelectObject(hdcTemp, bitmapTemp);

            /* Initial copy of bitmap */
            BitBlt(hdcTemp, 0, 0, nWidthSrc, nHeightSrc, hdcSrc, nXOriginSrc, nYOriginSrc, SRCCOPY);

            /* Extend bitmap in the X direction. Growth of width is exponential */
            xOfs = nWidthSrc;
            xRemaining = nWidthTemp - nWidthSrc;
            growSize = nWidthSrc;
            while (xRemaining > 0)
            {
                growSize = min(growSize, xRemaining);
                BitBlt(hdcTemp, xOfs, 0, growSize, nHeightSrc, hdcTemp, 0, 0, SRCCOPY);
                xOfs += growSize;
                xRemaining -= growSize;
                growSize *= 2;
            }

            /* Extend bitmap in the Y direction. Growth of height is exponential */
            yOfs = nHeightSrc;
            yRemaining = nHeightTemp - nHeightSrc;
            growSize = nHeightSrc;
            while (yRemaining > 0)
            {
                growSize = min(growSize, yRemaining);
                BitBlt(hdcTemp, 0, yOfs, nWidthTemp, growSize, hdcTemp, 0, 0, SRCCOPY);
                yOfs += growSize;
                yRemaining -= growSize;
                growSize *= 2;
            }

            /* Use temporary hdc for source */
            result = UXTHEME_Blt (hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                          hdcTemp, 0, 0,
                          transparent, transcolor);

            SelectObject(hdcTemp, bitmapOrig);
            DeleteObject(bitmapTemp);
        }
        DeleteDC(hdcTemp);
        return result;
    }
    else
    {
        return UXTHEME_StretchBlt (hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                                   hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                                   transparent, transcolor);
    }
}

/* Get transparency parameters passed to UXTHEME_StretchBlt() - the parameters 
 * depend on whether the image has full alpha  or whether it is 
 * color-transparent or just opaque. */
static inline void get_transparency (HTHEME hTheme, int iPartId, int iStateId, 
                                     BOOL hasImageAlpha, INT* transparent,
                                     COLORREF* transparentcolor, BOOL glyph)
{
    if (hasImageAlpha)
    {
        *transparent = ALPHABLEND_FULL;
        *transparentcolor = RGB (255, 0, 255);
    }
    else
    {
        BOOL trans = FALSE;
        GetThemeBool(hTheme, iPartId, iStateId, 
            glyph ? TMT_GLYPHTRANSPARENT : TMT_TRANSPARENT, &trans);
        if(trans) {
            *transparent = ALPHABLEND_BINARY;
            if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, 
                glyph ? TMT_GLYPHTRANSPARENTCOLOR : TMT_TRANSPARENTCOLOR, 
                transparentcolor))) {
                /* If image is transparent, but no color was specified, use magenta */
                *transparentcolor = RGB(255, 0, 255);
            }
        }
        else
            *transparent = ALPHABLEND_NONE;
    }
}

/* Reset alpha values in hdc to 0xFF if the background is opaque */
static void reset_dc_alpha_values(HTHEME htheme, HDC hdc, int part_id, int state_id,
                                  const RECT *rect)
{
    static const RGBQUAD bitmap_bits = {0x0, 0x0, 0x0, 0xFF};
    BITMAPINFO bitmap_info = {{0}};
    RECT image_rect;
    BOOL has_alpha;
    HBITMAP hbmp;
    int bg_type;

    if (GetDeviceCaps(hdc, BITSPIXEL) != 32)
        return;

    if (FAILED(GetThemeEnumValue(htheme, part_id, state_id, TMT_BGTYPE, &bg_type))
        || bg_type != BT_IMAGEFILE)
        return;

    if (FAILED(UXTHEME_LoadImage(htheme, part_id, state_id, rect, FALSE, &hbmp, &image_rect,
                                 &has_alpha, NULL)) || has_alpha)
        return;

    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = 1;
    bitmap_info.bmiHeader.biHeight = 1;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, rect->left, rect->top, abs(rect->right - rect->left),
                  abs(rect->bottom - rect->top), 0, 0, 1, 1, &bitmap_bits, &bitmap_info,
                  DIB_RGB_COLORS, SRCPAINT);
}

/***********************************************************************
 *      UXTHEME_DrawImageGlyph
 *
 * Draw an imagefile glyph
 */
static HRESULT UXTHEME_DrawImageGlyph(HTHEME hTheme, HDC hdc, int iPartId,
                               int iStateId, RECT *pRect,
                               const DTBGOPTS *pOptions)
{
    HRESULT hr;
    HBITMAP bmpSrc = NULL;
    HDC hdcSrc = NULL;
    HGDIOBJ oldSrc = NULL;
    RECT rcSrc;
    INT transparent = 0;
    COLORREF transparentcolor;
    int valign = VA_CENTER;
    int halign = HA_CENTER;
    POINT dstSize;
    POINT srcSize;
    POINT topleft;
    BOOL hasAlpha;

    hr = UXTHEME_LoadImage(hTheme, iPartId, iStateId, pRect, TRUE, &bmpSrc, &rcSrc, &hasAlpha,
                           NULL);
    if(FAILED(hr)) return hr;
    hdcSrc = CreateCompatibleDC(hdc);
    if(!hdcSrc) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    dstSize.x = pRect->right-pRect->left;
    dstSize.y = pRect->bottom-pRect->top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent,
        &transparentcolor, TRUE);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_VALIGN, &valign);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_HALIGN, &halign);

    topleft.x = pRect->left;
    topleft.y = pRect->top;
    if(halign == HA_CENTER)      topleft.x += (dstSize.x/2)-(srcSize.x/2);
    else if(halign == HA_RIGHT)  topleft.x += dstSize.x-srcSize.x;
    if(valign == VA_CENTER)      topleft.y += (dstSize.y/2)-(srcSize.y/2);
    else if(valign == VA_BOTTOM) topleft.y += dstSize.y-srcSize.y;

    if(!UXTHEME_Blt(hdc, topleft.x, topleft.y, srcSize.x, srcSize.y,
                    hdcSrc, rcSrc.left, rcSrc.top,
                    transparent, transparentcolor)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    SelectObject(hdcSrc, oldSrc);
    DeleteDC(hdcSrc);

    /* Don't transfer alpha values from the glyph when drawing opaque background */
    if (SUCCEEDED(hr) && hasAlpha)
        reset_dc_alpha_values(hTheme, hdc, iPartId, iStateId, pRect);

    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawGlyph
 *
 * Draw glyph on top of background, if appropriate
 */
static HRESULT UXTHEME_DrawGlyph(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    int glyphtype = GT_NONE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_GLYPHTYPE, &glyphtype);

    if(glyphtype == GT_IMAGEGLYPH) {
        return UXTHEME_DrawImageGlyph(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
    }
    else if(glyphtype == GT_FONTGLYPH) {
        /* I don't know what a font glyph is, I've never seen it used in any themes */
        FIXME("Font glyph\n");
    }
    return S_OK;
}

/***********************************************************************
 * get_image_part_size
 *
 * Used by GetThemePartSize and UXTHEME_DrawImageBackground
 */
static HRESULT get_image_part_size(HTHEME hTheme, int iPartId, int iStateId, RECT *prc,
                                   THEMESIZE eSize, POINT *psz)
{
    int imageDpi, dstDpi;
    HRESULT hr = S_OK;
    HBITMAP bmpSrc;
    RECT rcSrc;
    BOOL hasAlpha;

    hr = UXTHEME_LoadImage(hTheme, iPartId, iStateId, prc, FALSE, &bmpSrc, &rcSrc, &hasAlpha,
                           &imageDpi);
    if (FAILED(hr)) return hr;

    switch (eSize)
    {
        case TS_DRAW:
        {
            int sizingType = ST_STRETCH, scalingType = TSST_NONE, stretchMark = 0;
            POINT srcSize;

            srcSize.x = rcSrc.right - rcSrc.left;
            srcSize.y = rcSrc.bottom - rcSrc.top;

            GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_SIZINGTYPE, &sizingType);
            if (sizingType == ST_TRUESIZE)
            {
                GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_TRUESIZESCALINGTYPE, &scalingType);
                GetThemeInt(hTheme, iPartId, iStateId, TMT_TRUESIZESTRETCHMARK, &stretchMark);
                if (scalingType == TSST_DPI)
                {
                    /* Scale to DPI only if the destination DPI exceeds the source DPI by
                     * stretchMark percent */
                    dstDpi = MSSTYLES_GetThemeDPI(hTheme);
                    if (dstDpi != imageDpi && MulDiv(100, dstDpi, imageDpi) >= stretchMark + 100)
                    {
                        srcSize.x = MulDiv(srcSize.x, dstDpi, imageDpi);
                        srcSize.y = MulDiv(srcSize.y, dstDpi, imageDpi);
                    }
                }
            }
            *psz = srcSize;

            if (prc != NULL)
            {
                POINT dstSize;
                BOOL uniformsizing = FALSE;

                dstSize.x = prc->right - prc->left;
                dstSize.y = prc->bottom - prc->top;

                GetThemeBool(hTheme, iPartId, iStateId, TMT_UNIFORMSIZING, &uniformsizing);
                if(uniformsizing) {
                    /* Scale height and width equally */
                    if (dstSize.x*srcSize.y < dstSize.y*srcSize.x)
                        dstSize.y = MulDiv (srcSize.y, dstSize.x, srcSize.x);
                    else
                        dstSize.x = MulDiv (srcSize.x, dstSize.y, srcSize.y);
                }

                if (sizingType == ST_TRUESIZE)
                {
                    if ((dstSize.x < 0 || dstSize.y < 0)
                        || (dstSize.x < srcSize.x && dstSize.y < srcSize.y)
                        || (scalingType == TSST_SIZE
                            && MulDiv(100, dstSize.x, srcSize.x) >= stretchMark + 100
                            && MulDiv(100, dstSize.y, srcSize.y) >= stretchMark + 100))
                    {
                        *psz = dstSize;
                    }
                }
                else
                {
                    psz->x = abs(dstSize.x);
                    psz->y = abs(dstSize.y);
                }
            }

            break;
        }
        case TS_MIN:
            /* FIXME: couldn't figure how native uxtheme computes min size */
        case TS_TRUE:
            psz->x = rcSrc.right - rcSrc.left;
            psz->y = rcSrc.bottom - rcSrc.top;
            break;
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawImageBackground
 *
 * Draw an imagefile background
 */
static HRESULT UXTHEME_DrawImageBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    int destCenterWidth, srcCenterWidth, destCenterHeight, srcCenterHeight;
    HRESULT hr = S_OK;
    HBITMAP bmpSrc;
    HGDIOBJ oldSrc;
    HDC hdcSrc;
    RECT rcSrc;
    RECT rcDst;
    POINT dstSize;
    POINT srcSize;
    POINT drawSize;
    int sizingtype = ST_STRETCH;
    INT transparent;
    COLORREF transparentcolor = 0;
    BOOL hasAlpha;

    hr = UXTHEME_LoadImage(hTheme, iPartId, iStateId, pRect, FALSE, &bmpSrc, &rcSrc, &hasAlpha,
                           NULL);
    if(FAILED(hr)) return hr;
    hdcSrc = CreateCompatibleDC(hdc);
    if(!hdcSrc) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    rcDst = *pRect;
    
    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent,
        &transparentcolor, FALSE);

    dstSize.x = rcDst.right-rcDst.left;
    dstSize.y = rcDst.bottom-rcDst.top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_SIZINGTYPE, &sizingtype);
    if(sizingtype == ST_TRUESIZE) {
        int valign = VA_CENTER, halign = HA_CENTER;

        get_image_part_size(hTheme, iPartId, iStateId, pRect, TS_DRAW, &drawSize);
        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_VALIGN, &valign);
        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_HALIGN, &halign);

        if (halign == HA_CENTER)
            rcDst.left += (dstSize.x/2)-(drawSize.x/2);
        else if (halign == HA_RIGHT)
            rcDst.left = rcDst.right - drawSize.x;
        if (valign == VA_CENTER)
            rcDst.top  += (dstSize.y/2)-(drawSize.y/2);
        else if (valign == VA_BOTTOM)
            rcDst.top = rcDst.bottom - drawSize.y;
        rcDst.right = rcDst.left + drawSize.x;
        rcDst.bottom = rcDst.top + drawSize.y;
        if(!UXTHEME_StretchBlt(hdc, rcDst.left, rcDst.top, drawSize.x, drawSize.y,
                                hdcSrc, rcSrc.left, rcSrc.top, srcSize.x, srcSize.y,
                                transparent, transparentcolor))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        HDC hdcDst = NULL;
        MARGINS sm;
        POINT org;

        dstSize.x = abs(dstSize.x);
        dstSize.y = abs(dstSize.y);

        GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_SIZINGMARGINS, NULL, &sm);

        /* Resize sizing margins if destination is smaller */
        if (sm.cyTopHeight + sm.cyBottomHeight > dstSize.y || sm.cxLeftWidth + sm.cxRightWidth > dstSize.x) {
            if (sm.cyTopHeight + sm.cyBottomHeight > dstSize.y) {
                sm.cyTopHeight = MulDiv(sm.cyTopHeight, dstSize.y, srcSize.y);
                sm.cyBottomHeight = dstSize.y - sm.cyTopHeight;
            }

            if (sm.cxLeftWidth + sm.cxRightWidth > dstSize.x) {
                sm.cxLeftWidth = MulDiv(sm.cxLeftWidth, dstSize.x, srcSize.x);
                sm.cxRightWidth = dstSize.x - sm.cxLeftWidth;
            }
        }

        hdcDst = hdc;
        OffsetViewportOrgEx(hdcDst, rcDst.left, rcDst.top, &org);

        /* Upper left corner */
        if(!UXTHEME_Blt(hdcDst, 0, 0, sm.cxLeftWidth, sm.cyTopHeight,
                        hdcSrc, rcSrc.left, rcSrc.top, 
                        transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Upper right corner */
        if(!UXTHEME_Blt (hdcDst, dstSize.x-sm.cxRightWidth, 0, 
                         sm.cxRightWidth, sm.cyTopHeight,
                         hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top, 
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Lower left corner */
        if(!UXTHEME_Blt (hdcDst, 0, dstSize.y-sm.cyBottomHeight, 
                         sm.cxLeftWidth, sm.cyBottomHeight,
                         hdcSrc, rcSrc.left, rcSrc.bottom-sm.cyBottomHeight, 
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Lower right corner */
        if(!UXTHEME_Blt (hdcDst, dstSize.x-sm.cxRightWidth, dstSize.y-sm.cyBottomHeight, 
                         sm.cxRightWidth, sm.cyBottomHeight,
                         hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.bottom-sm.cyBottomHeight, 
                         transparent, transparentcolor)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }

        destCenterWidth  = dstSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
        srcCenterWidth   = srcSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
        destCenterHeight = dstSize.y - (sm.cyTopHeight + sm.cyBottomHeight);
        srcCenterHeight  = srcSize.y - (sm.cyTopHeight + sm.cyBottomHeight);

        if(destCenterWidth > 0) {
            /* Center top */
            if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, 0,
                                  destCenterWidth, sm.cyTopHeight,
                                  hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top,
                                  srcCenterWidth, sm.cyTopHeight,
                                  sizingtype, transparent, transparentcolor)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto draw_error;
            }
            /* Center bottom */
            if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, dstSize.y-sm.cyBottomHeight,
                                  destCenterWidth, sm.cyBottomHeight,
                                  hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.bottom-sm.cyBottomHeight,
                                  srcCenterWidth, sm.cyBottomHeight,
                                  sizingtype, transparent, transparentcolor)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto draw_error;
            }
        }
        if(destCenterHeight > 0) {
            /* Left center */
            if(!UXTHEME_SizedBlt (hdcDst, 0, sm.cyTopHeight,
                                  sm.cxLeftWidth, destCenterHeight,
                                  hdcSrc, rcSrc.left, rcSrc.top+sm.cyTopHeight,
                                  sm.cxLeftWidth, srcCenterHeight,
                                  sizingtype,
                                  transparent, transparentcolor)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto draw_error;
            }
            /* Right center */
            if(!UXTHEME_SizedBlt (hdcDst, dstSize.x-sm.cxRightWidth, sm.cyTopHeight,
                                  sm.cxRightWidth, destCenterHeight,
                                  hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top+sm.cyTopHeight,
                                  sm.cxRightWidth, srcCenterHeight,
                                  sizingtype, transparent, transparentcolor)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto draw_error;
            }
        }
        if(destCenterHeight > 0 && destCenterWidth > 0) {
            BOOL borderonly = FALSE;
            GetThemeBool(hTheme, iPartId, iStateId, TMT_BORDERONLY, &borderonly);
            if(!borderonly) {
                /* Center */
                if(!UXTHEME_SizedBlt (hdcDst, sm.cxLeftWidth, sm.cyTopHeight,
                                      destCenterWidth, destCenterHeight,
                                      hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top+sm.cyTopHeight,
                                      srcCenterWidth, srcCenterHeight,
                                      sizingtype, transparent, transparentcolor)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error; 
                }
            }
        }

draw_error:
        SetViewportOrgEx (hdcDst, org.x, org.y, NULL);
    }
    SelectObject(hdcSrc, oldSrc);
    DeleteDC(hdcSrc);
    *pRect = rcDst;
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBorderRectangle
 *
 * Draw the bounding rectangle for a borderfill background
 */
static HRESULT UXTHEME_DrawBorderRectangle(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    HPEN hPen;
    HGDIOBJ oldPen;
    COLORREF bordercolor = RGB(0,0,0);
    int bordersize = 1;

    GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
    if(bordersize > 0) {
        POINT ptCorners[5];
        ptCorners[0].x = pRect->left;
        ptCorners[0].y = pRect->top;
        ptCorners[1].x = pRect->right-1;
        ptCorners[1].y = pRect->top;
        ptCorners[2].x = pRect->right-1;
        ptCorners[2].y = pRect->bottom-1;
        ptCorners[3].x = pRect->left;
        ptCorners[3].y = pRect->bottom-1;
        ptCorners[4].x = pRect->left;
        ptCorners[4].y = pRect->top;

        InflateRect(pRect, -bordersize, -bordersize);
        if(pOptions->dwFlags & DTBG_OMITBORDER)
            return S_OK;
        GetThemeColor(hTheme, iPartId, iStateId, TMT_BORDERCOLOR, &bordercolor);
        hPen = CreatePen(PS_SOLID, bordersize, bordercolor);
        if(!hPen)
            return HRESULT_FROM_WIN32(GetLastError());
        oldPen = SelectObject(hdc, hPen);

        if(!Polyline(hdc, ptCorners, 5))
            hr = HRESULT_FROM_WIN32(GetLastError());

        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBackgroundFill
 *
 * Fill a borderfill background rectangle
 */
static HRESULT UXTHEME_DrawBackgroundFill(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, RECT *pRect,
                                   const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    int filltype = FT_SOLID;

    TRACE("(%d,%d,%ld)\n", iPartId, iStateId, pOptions->dwFlags);

    if(pOptions->dwFlags & DTBG_OMITCONTENT)
        return S_OK;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_FILLTYPE, &filltype);

    if(filltype == FT_SOLID) {
        HBRUSH hBrush;
        COLORREF fillcolor = RGB(255,255,255);

        GetThemeColor(hTheme, iPartId, iStateId, TMT_FILLCOLOR, &fillcolor);
        hBrush = CreateSolidBrush(fillcolor);
        if(!FillRect(hdc, pRect, hBrush))
            hr = HRESULT_FROM_WIN32(GetLastError());
        DeleteObject(hBrush);
    }
    else if(filltype == FT_VERTGRADIENT || filltype == FT_HORZGRADIENT) {
        /* FIXME: This only accounts for 2 gradient colors (out of 5) and ignores
            the gradient ratios (no idea how those work)
            Few themes use this, and the ones I've seen only use 2 colors with
            a gradient ratio of 0 and 255 respectively
        */

        COLORREF gradient1 = RGB(0,0,0);
        COLORREF gradient2 = RGB(255,255,255);
        TRIVERTEX vert[2];
        GRADIENT_RECT gRect;

        FIXME("Gradient implementation not complete\n");

        GetThemeColor(hTheme, iPartId, iStateId, TMT_GRADIENTCOLOR1, &gradient1);
        GetThemeColor(hTheme, iPartId, iStateId, TMT_GRADIENTCOLOR2, &gradient2);

        vert[0].x     = pRect->left;
        vert[0].y     = pRect->top;
        vert[0].Red   = GetRValue(gradient1) << 8;
        vert[0].Green = GetGValue(gradient1) << 8;
        vert[0].Blue  = GetBValue(gradient1) << 8;
        vert[0].Alpha = 0xff00;

        vert[1].x     = pRect->right;
        vert[1].y     = pRect->bottom;
        vert[1].Red   = GetRValue(gradient2) << 8;
        vert[1].Green = GetGValue(gradient2) << 8;
        vert[1].Blue  = GetBValue(gradient2) << 8;
        vert[1].Alpha = 0xff00;

        gRect.UpperLeft  = 0;
        gRect.LowerRight = 1;
        GradientFill(hdc,vert,2,&gRect,1,filltype==FT_HORZGRADIENT?GRADIENT_FILL_RECT_H:GRADIENT_FILL_RECT_V);
    }
    else if(filltype == FT_RADIALGRADIENT) {
        /* I've never seen this used in a theme */
        FIXME("Radial gradient\n");
    }
    else if(filltype == FT_TILEIMAGE) {
        /* I've never seen this used in a theme */
        FIXME("Tile image\n");
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBorderBackground
 *
 * Draw an imagefile background
 */
static HRESULT UXTHEME_DrawBorderBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, const RECT *pRect,
                                     const DTBGOPTS *pOptions)
{
    HRESULT hr;
    RECT rt;

    rt = *pRect;

    hr = UXTHEME_DrawBorderRectangle(hTheme, hdc, iPartId, iStateId, &rt, pOptions);
    if(FAILED(hr))
        return hr;
    return UXTHEME_DrawBackgroundFill(hTheme, hdc, iPartId, iStateId, &rt, pOptions);
}

/***********************************************************************
 *      DrawThemeBackgroundEx                               (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, const RECT *pRect,
                                     const DTBGOPTS *pOptions)
{
    HRESULT hr;
    const DTBGOPTS defaultOpts = {sizeof(DTBGOPTS), 0, {0,0,0,0}};
    const DTBGOPTS *opts;
    HRGN clip = NULL;
    int hasClip = -1;
    int bgtype = BT_BORDERFILL;
    RECT rt;

    TRACE("(%p,%p,%d,%d,%ld,%ld)\n", hTheme, hdc, iPartId, iStateId,pRect->left,pRect->top);
    if(!hTheme)
        return E_HANDLE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if (bgtype == BT_NONE) return S_OK;

    /* Ensure we have a DTBGOPTS structure available, simplifies some of the code */
    opts = pOptions;
    if(!opts) opts = &defaultOpts;

    if(opts->dwFlags & DTBG_CLIPRECT) {
        clip = CreateRectRgn(0,0,1,1);
        hasClip = GetClipRgn(hdc, clip);
        if(hasClip == -1)
            TRACE("Failed to get original clipping region\n");
        else
            IntersectClipRect(hdc, opts->rcClip.left, opts->rcClip.top, opts->rcClip.right, opts->rcClip.bottom);
    }
    rt = *pRect;

    if(bgtype == BT_IMAGEFILE)
        hr = UXTHEME_DrawImageBackground(hTheme, hdc, iPartId, iStateId, &rt, opts);
    else if(bgtype == BT_BORDERFILL)
        hr = UXTHEME_DrawBorderBackground(hTheme, hdc, iPartId, iStateId, pRect, opts);
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    if(SUCCEEDED(hr))
        hr = UXTHEME_DrawGlyph(hTheme, hdc, iPartId, iStateId, &rt, opts);
    if(opts->dwFlags & DTBG_CLIPRECT) {
        if(hasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if(hasClip == 1)
            SelectClipRgn(hdc, clip);
        DeleteObject(clip);
    }
    return hr;
}

/*
 * DrawThemeEdge() implementation
 *
 * Since it basically is DrawEdge() with different colors, I copied its code
 * from user32's uitools.c.
 */

enum
{
    EDGE_LIGHT,
    EDGE_HIGHLIGHT,
    EDGE_SHADOW,
    EDGE_DARKSHADOW,
    EDGE_FILL,

    EDGE_WINDOW,
    EDGE_WINDOWFRAME,

    EDGE_NUMCOLORS
};

static const struct 
{
    int themeProp;
    int sysColor;
} EdgeColorMap[EDGE_NUMCOLORS] = {
    {TMT_EDGELIGHTCOLOR,                  COLOR_3DLIGHT},
    {TMT_EDGEHIGHLIGHTCOLOR,              COLOR_BTNHIGHLIGHT},
    {TMT_EDGESHADOWCOLOR,                 COLOR_BTNSHADOW},
    {TMT_EDGEDKSHADOWCOLOR,               COLOR_3DDKSHADOW},
    {TMT_EDGEFILLCOLOR,                   COLOR_BTNFACE},
    {-1,                                  COLOR_WINDOW},
    {-1,                                  COLOR_WINDOWFRAME}
};

static const signed char LTInnerNormal[] = {
    -1,           -1,                 -1,                 -1,
    -1,           EDGE_HIGHLIGHT,     EDGE_HIGHLIGHT,     -1,
    -1,           EDGE_DARKSHADOW,    EDGE_DARKSHADOW,    -1,
    -1,           -1,                 -1,                 -1
};

static const signed char LTOuterNormal[] = {
    -1,                 EDGE_LIGHT,     EDGE_SHADOW, -1,
    EDGE_HIGHLIGHT,     EDGE_LIGHT,     EDGE_SHADOW, -1,
    EDGE_DARKSHADOW,    EDGE_LIGHT,     EDGE_SHADOW, -1,
    -1,                 EDGE_LIGHT,     EDGE_SHADOW, -1
};

static const signed char RBInnerNormal[] = {
    -1,           -1,                 -1,               -1,
    -1,           EDGE_SHADOW,        EDGE_SHADOW,      -1,
    -1,           EDGE_LIGHT,         EDGE_LIGHT,       -1,
    -1,           -1,                 -1,               -1
};

static const signed char RBOuterNormal[] = {
    -1,               EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    EDGE_SHADOW,      EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    EDGE_LIGHT,       EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    -1,               EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1
};

static const signed char LTInnerSoft[] = {
    -1,                  -1,                -1,               -1,
    -1,                  EDGE_LIGHT,        EDGE_LIGHT,       -1,
    -1,                  EDGE_SHADOW,       EDGE_SHADOW,      -1,
    -1,                  -1,                -1,               -1
};

static const signed char LTOuterSoft[] = {
    -1,               EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    EDGE_LIGHT,       EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    EDGE_SHADOW,      EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    -1,               EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1
};

#define RBInnerSoft RBInnerNormal   /* These are the same */
#define RBOuterSoft RBOuterNormal

static const signed char LTRBOuterMono[] = {
    -1,           EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
};

static const signed char LTRBInnerMono[] = {
    -1, -1,           -1,           -1,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
};

static const signed char LTRBOuterFlat[] = {
    -1,                 EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
};

static const signed char LTRBInnerFlat[] = {
    -1, -1,               -1,               -1,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
};

static COLORREF get_edge_color (int edgeType, HTHEME theme, int part, int state)
{
    COLORREF col;
    if ((EdgeColorMap[edgeType].themeProp == -1)
        || FAILED (GetThemeColor (theme, part, state, 
            EdgeColorMap[edgeType].themeProp, &col)))
        col = GetSysColor (EdgeColorMap[edgeType].sysColor);
    return col;
}

static inline HPEN get_edge_pen (int edgeType, HTHEME theme, int part, int state)
{
    return CreatePen (PS_SOLID, 1, get_edge_color (edgeType, theme, part, state));
}

static inline HBRUSH get_edge_brush (int edgeType, HTHEME theme, int part, int state)
{
    return CreateSolidBrush (get_edge_color (edgeType, theme, part, state));
}

/***********************************************************************
 *           draw_diag_edge
 *
 * Same as DrawEdge invoked with BF_DIAGONAL
 */
static HRESULT draw_diag_edge (HDC hdc, HTHEME theme, int part, int state,
                               const RECT* rc, UINT uType, 
                               UINT uFlags, LPRECT contentsRect)
{
    POINT Points[4];
    signed char InnerI, OuterI;
    HPEN InnerPen, OuterPen;
    POINT SavePoint;
    HPEN SavePen;
    int spx, spy;
    int epx, epy;
    int Width = rc->right - rc->left;
    int Height= rc->bottom - rc->top;
    int SmallDiam = Width > Height ? Height : Width;
    HRESULT retval = (((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) ) ? E_FAIL : S_OK;
    int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
            + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

    /* Init some vars */
    OuterPen = InnerPen = GetStockObject(NULL_PEN);
    SavePen = SelectObject(hdc, InnerPen);
    spx = spy = epx = epy = 0; /* Satisfy the compiler... */

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        InnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        InnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_SOFT)
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
    }
    else
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
    }

    if(InnerI != -1) InnerPen = get_edge_pen (InnerI, theme, part, state);
    if(OuterI != -1) OuterPen = get_edge_pen (OuterI, theme, part, state);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Don't ask me why, but this is what is visible... */
    /* This must be possible to do much simpler, but I fail to */
    /* see the logic in the MS implementation (sigh...). */
    /* So, this might look a bit brute force here (and it is), but */
    /* it gets the job done;) */

    switch(uFlags & BF_RECT)
    {
    case 0:
    case BF_LEFT:
    case BF_BOTTOM:
    case BF_BOTTOMLEFT:
        /* Left bottom endpoint */
        epx = rc->left-1;
        spx = epx + SmallDiam;
        epy = rc->bottom;
        spy = epy - SmallDiam;
        break;

    case BF_TOPLEFT:
    case BF_BOTTOMRIGHT:
        /* Left top endpoint */
        epx = rc->left-1;
        spx = epx + SmallDiam;
        epy = rc->top-1;
        spy = epy + SmallDiam;
        break;

    case BF_TOP:
    case BF_RIGHT:
    case BF_TOPRIGHT:
    case BF_RIGHT|BF_LEFT:
    case BF_RIGHT|BF_LEFT|BF_TOP:
    case BF_BOTTOM|BF_TOP:
    case BF_BOTTOM|BF_TOP|BF_LEFT:
    case BF_BOTTOMRIGHT|BF_LEFT:
    case BF_BOTTOMRIGHT|BF_TOP:
    case BF_RECT:
        /* Right top endpoint */
        spx = rc->left;
        epx = spx + SmallDiam;
        spy = rc->bottom-1;
        epy = spy - SmallDiam;
        break;
    }

    MoveToEx(hdc, spx, spy, NULL);
    SelectObject(hdc, OuterPen);
    LineTo(hdc, epx, epy);

    SelectObject(hdc, InnerPen);

    switch(uFlags & (BF_RECT|BF_DIAGONAL))
    {
    case BF_DIAGONAL_ENDBOTTOMLEFT:
    case (BF_DIAGONAL|BF_BOTTOM):
    case BF_DIAGONAL:
    case (BF_DIAGONAL|BF_LEFT):
        MoveToEx(hdc, spx-1, spy, NULL);
        LineTo(hdc, epx, epy-1);
        Points[0].x = spx-add;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->top;
        Points[2].x = epx+1;
        Points[2].y = epy-1-add;
        Points[3] = Points[2];
        break;

    case BF_DIAGONAL_ENDBOTTOMRIGHT:
        MoveToEx(hdc, spx-1, spy, NULL);
        LineTo(hdc, epx, epy+1);
        Points[0].x = spx-add;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->bottom-1;
        Points[2].x = epx+1;
        Points[2].y = epy+1+add;
        Points[3] = Points[2];
        break;

    case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP|BF_LEFT):
    case BF_DIAGONAL_ENDTOPRIGHT:
    case (BF_DIAGONAL|BF_RIGHT|BF_TOP|BF_LEFT):
        MoveToEx(hdc, spx+1, spy, NULL);
        LineTo(hdc, epx, epy+1);
        Points[0].x = epx-1;
        Points[0].y = epy+1+add;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top+add;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1;
        Points[3].x = spx+add;
        Points[3].y = spy;
        break;

    case BF_DIAGONAL_ENDTOPLEFT:
        MoveToEx(hdc, spx, spy-1, NULL);
        LineTo(hdc, epx+1, epy);
        Points[0].x = epx+1+add;
        Points[0].y = epy+1;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1-add;
        Points[3].x = spx;
        Points[3].y = spy-add;
        break;

    case (BF_DIAGONAL|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_TOP|BF_LEFT):
        MoveToEx(hdc, spx+1, spy-1, NULL);
        LineTo(hdc, epx, epy);
        Points[0].x = epx-1;
        Points[0].y = epy+1;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1-add;
        Points[3].x = spx+add;
        Points[3].y = spy-add;
        break;

    case (BF_DIAGONAL|BF_RIGHT):
    case (BF_DIAGONAL|BF_RIGHT|BF_LEFT):
    case (BF_DIAGONAL|BF_RIGHT|BF_LEFT|BF_BOTTOM):
        MoveToEx(hdc, spx, spy, NULL);
        LineTo(hdc, epx-1, epy+1);
        Points[0].x = spx;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->top+add;
        Points[2].x = epx-1-add;
        Points[2].y = epy+1+add;
        Points[3] = Points[2];
        break;
    }

    /* Fill the interior if asked */
    if((uFlags & BF_MIDDLE) && retval)
    {
        HBRUSH hbsave;
        HBRUSH hb = get_edge_brush ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
            theme, part, state);
        HPEN hpsave;
        HPEN hp = get_edge_pen ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
            theme, part, state);
        hbsave = SelectObject(hdc, hb);
        hpsave = SelectObject(hdc, hp);
        Polygon(hdc, Points, 4);
        SelectObject(hdc, hbsave);
        SelectObject(hdc, hpsave);
        DeleteObject (hp);
        DeleteObject (hb);
    }

    /* Adjust rectangle if asked */
    if(uFlags & BF_ADJUST)
    {
        *contentsRect = *rc;
        if(uFlags & BF_LEFT)   contentsRect->left   += add;
        if(uFlags & BF_RIGHT)  contentsRect->right  -= add;
        if(uFlags & BF_TOP)    contentsRect->top    += add;
        if(uFlags & BF_BOTTOM) contentsRect->bottom -= add;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
    if(InnerI != -1) DeleteObject (InnerPen);
    if(OuterI != -1) DeleteObject (OuterPen);

    return retval;
}

/***********************************************************************
 *           draw_rect_edge
 *
 * Same as DrawEdge invoked without BF_DIAGONAL
 */
static HRESULT draw_rect_edge (HDC hdc, HTHEME theme, int part, int state,
                               const RECT* rc, UINT uType, 
                               UINT uFlags, LPRECT contentsRect)
{
    signed char LTInnerI, LTOuterI;
    signed char RBInnerI, RBOuterI;
    HPEN LTInnerPen, LTOuterPen;
    HPEN RBInnerPen, RBOuterPen;
    RECT InnerRect = *rc;
    POINT SavePoint;
    HPEN SavePen;
    int LBpenplus = 0;
    int LTpenplus = 0;
    int RTpenplus = 0;
    int RBpenplus = 0;
    HRESULT retval = (((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) ) ? E_FAIL : S_OK;

    /* Init some vars */
    LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = GetStockObject(NULL_PEN);
    SavePen = SelectObject(hdc, LTInnerPen);

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        LTInnerI = RBInnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        LTInnerI = RBInnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];

        if( LTInnerI != -1 ) LTInnerI = RBInnerI = EDGE_FILL;
    }
    else if(uFlags & BF_SOFT)
    {
        LTInnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
    }
    else
    {
        LTInnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
    }

    if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)   LBpenplus = 1;
    if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)       RTpenplus = 1;
    if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT) RBpenplus = 1;
    if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)         LTpenplus = 1;

    if(LTInnerI != -1) LTInnerPen = get_edge_pen (LTInnerI, theme, part, state);
    if(LTOuterI != -1) LTOuterPen = get_edge_pen (LTOuterI, theme, part, state);
    if(RBInnerI != -1) RBInnerPen = get_edge_pen (RBInnerI, theme, part, state);
    if(RBOuterI != -1) RBOuterPen = get_edge_pen (RBOuterI, theme, part, state);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Draw the outer edge */
    SelectObject(hdc, LTOuterPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.right, InnerRect.top);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.left, InnerRect.bottom);
    }
    SelectObject(hdc, RBOuterPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.left-1, InnerRect.bottom-1);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.right-1, InnerRect.top-1);
    }

    /* Draw the inner edge */
    SelectObject(hdc, LTInnerPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
        LineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
        LineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
    }
    SelectObject(hdc, RBInnerPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1-RBpenplus, InnerRect.bottom-2, NULL);
        LineTo(hdc, InnerRect.left-1+LBpenplus, InnerRect.bottom-2);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-2, InnerRect.bottom-1-RBpenplus, NULL);
        LineTo(hdc, InnerRect.right-2, InnerRect.top-1+RTpenplus);
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

        if(uFlags & BF_LEFT)   InnerRect.left   += add;
        if(uFlags & BF_RIGHT)  InnerRect.right  -= add;
        if(uFlags & BF_TOP)    InnerRect.top    += add;
        if(uFlags & BF_BOTTOM) InnerRect.bottom -= add;

        if((uFlags & BF_MIDDLE) && retval)
        {
            HBRUSH br = get_edge_brush ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
                theme, part, state);
            FillRect(hdc, &InnerRect, br);
            DeleteObject (br);
        }

        if(uFlags & BF_ADJUST)
            *contentsRect = InnerRect;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
    if(LTInnerI != -1) DeleteObject (LTInnerPen);
    if(LTOuterI != -1) DeleteObject (LTOuterPen);
    if(RBInnerI != -1) DeleteObject (RBInnerPen);
    if(RBOuterI != -1) DeleteObject (RBOuterPen);
    return retval;
}


/***********************************************************************
 *      DrawThemeEdge                                       (UXTHEME.@)
 *
 * DrawThemeEdge() is pretty similar to the vanilla DrawEdge() - the
 * difference is that it does not rely on the system colors alone, but
 * also allows color specification in the theme.
 */
HRESULT WINAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId,
                             int iStateId, const RECT *pDestRect, UINT uEdge,
                             UINT uFlags, RECT *pContentRect)
{
    TRACE("%d %d 0x%08x 0x%08x\n", iPartId, iStateId, uEdge, uFlags);
    if(!hTheme)
        return E_HANDLE;
     
    if(uFlags & BF_DIAGONAL)
        return draw_diag_edge (hdc, hTheme, iPartId, iStateId, pDestRect,
            uEdge, uFlags, pContentRect);
    else
        return draw_rect_edge (hdc, hTheme, iPartId, iStateId, pDestRect,
            uEdge, uFlags, pContentRect);
}


/***********************************************************************
 *      DrawThemeIcon                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
    INT effect = ICE_NONE, saturation = 0, alpha = 128;
    IImageList *image_list = (IImageList *)himl;
    IMAGELISTDRAWPARAMS params = {0};
    COLORREF color = 0;

    TRACE("%p %p %d %d %s %p %d\n", hTheme, hdc, iPartId, iStateId, wine_dbgstr_rect(pRect), himl,
          iImageIndex);

    if (!hTheme)
        return E_HANDLE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_ICONEFFECT, &effect);
    switch (effect)
    {
    case ICE_NONE:
        params.fState = ILS_NORMAL;
        break;
    case ICE_GLOW:
        GetThemeColor(hTheme, iPartId, iStateId, TMT_GLOWCOLOR, &color);
        params.fState = ILS_GLOW;
        params.crEffect = color;
        break;
    case ICE_SHADOW:
        GetThemeColor(hTheme, iPartId, iStateId, TMT_SHADOWCOLOR, &color);
        params.fState = ILS_SHADOW;
        params.crEffect = color;
        break;
    case ICE_PULSE:
        GetThemeInt(hTheme, iPartId, iStateId, TMT_SATURATION, &saturation);
        params.fState = ILS_SATURATE;
        params.Frame = saturation;
        break;
    case ICE_ALPHA:
        GetThemeInt(hTheme, iPartId, iStateId, TMT_ALPHALEVEL, &alpha);
        params.fState = ILS_ALPHA;
        params.Frame = alpha;
        break;
    }

    params.cbSize = sizeof(params);
    params.himl = himl;
    params.i = iImageIndex;
    params.hdcDst = hdc;
    params.x = pRect->left;
    params.y = pRect->top;
    params.cx = pRect->right - pRect->left;
    params.cy = pRect->bottom - pRect->top;
    params.rgbBk = CLR_NONE;
    params.rgbFg = CLR_NONE;
    params.fStyle = ILD_TRANSPARENT;
    return IImageList_Draw(image_list, &params);
}

/***********************************************************************
 *      DrawThemeText                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD flags,
                             DWORD flags2, const RECT *pRect)
{
    DTTOPTS opts = { 0 };
    RECT rt;

    TRACE("%d %d\n", iPartId, iStateId);

    rt = *pRect;

    opts.dwSize = sizeof(opts);
    if (flags2 & DTT_GRAYED) {
        opts.dwFlags = DTT_TEXTCOLOR;
        opts.crText = GetSysColor(COLOR_GRAYTEXT);
    }
    return DrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, iCharCount, flags, &rt, &opts);
}

/***********************************************************************
 *      DrawThemeTextEx                                     (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
    LPCWSTR pszText, int iCharCount, DWORD flags, RECT *rect, const DTTOPTS *options)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    int oldBkMode;
    int fontProp;

    TRACE("%p %p %d %d %s:%d 0x%08lx %p %p\n", hTheme, hdc, iPartId, iStateId,
        debugstr_wn(pszText, iCharCount), iCharCount, flags, rect, options);

    if(!hTheme)
        return E_HANDLE;

    if (options->dwFlags & ~(DTT_TEXTCOLOR | DTT_FONTPROP))
        FIXME("unsupported flags 0x%08lx\n", options->dwFlags);

    if (options->dwFlags & DTT_FONTPROP)
        fontProp = options->iFontPropId;
    else
        fontProp = TMT_FONT;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, fontProp, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }

    if(hFont)
        oldFont = SelectObject(hdc, hFont);

    if (options->dwFlags & DTT_TEXTCOLOR)
        textColor = options->crText;
    else {
        if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTCOLOR, &textColor)))
            textColor = GetTextColor(hdc);
    }
    oldTextColor = SetTextColor(hdc, textColor);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, pszText, iCharCount, rect, flags);
    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldTextColor);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundContentRect                       (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId,
                                             int iStateId,
                                             const RECT *pBoundingRect,
                                             RECT *pContentRect)
{
    MARGINS margin;
    HRESULT hr;

    TRACE("(%d,%d)\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    /* try content margins property... */
    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(SUCCEEDED(hr)) {
        pContentRect->left = pBoundingRect->left + margin.cxLeftWidth;
        pContentRect->top  = pBoundingRect->top + margin.cyTopHeight;
        pContentRect->right = pBoundingRect->right - margin.cxRightWidth;
        pContentRect->bottom = pBoundingRect->bottom - margin.cyBottomHeight;
    } else {
        /* otherwise, try to determine content rect from the background type and props */
        int bgtype = BT_BORDERFILL;
        *pContentRect = *pBoundingRect;

        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
        if(bgtype == BT_BORDERFILL) {
            int bordersize = 1;

            GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
            InflateRect(pContentRect, -bordersize, -bordersize);
        } else if ((bgtype == BT_IMAGEFILE)
                && (SUCCEEDED(hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, 
                TMT_SIZINGMARGINS, NULL, &margin)))) {
            pContentRect->left = pBoundingRect->left + margin.cxLeftWidth;
            pContentRect->top  = pBoundingRect->top + margin.cyTopHeight;
            pContentRect->right = pBoundingRect->right - margin.cxRightWidth;
            pContentRect->bottom = pBoundingRect->bottom - margin.cyBottomHeight;
        }
        /* If nothing was found, leave unchanged */
    }

    TRACE("%s\n", wine_dbgstr_rect(pContentRect));

    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundExtent                            (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pContentRect,
                                        RECT *pExtentRect)
{
    MARGINS margin;
    HRESULT hr;

    TRACE("(%d,%d)\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    /* try content margins property... */
    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(SUCCEEDED(hr)) {
        pExtentRect->left = pContentRect->left - margin.cxLeftWidth;
        pExtentRect->top  = pContentRect->top - margin.cyTopHeight;
        pExtentRect->right = pContentRect->right + margin.cxRightWidth;
        pExtentRect->bottom = pContentRect->bottom + margin.cyBottomHeight;
    } else {
        /* otherwise, try to determine content rect from the background type and props */
        int bgtype = BT_BORDERFILL;
        *pExtentRect = *pContentRect;

        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
        if(bgtype == BT_BORDERFILL) {
            int bordersize = 1;

            GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
            InflateRect(pExtentRect, bordersize, bordersize);
        } else if ((bgtype == BT_IMAGEFILE)
                && (SUCCEEDED(hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, 
                TMT_SIZINGMARGINS, NULL, &margin)))) {
            pExtentRect->left = pContentRect->left - margin.cxLeftWidth;
            pExtentRect->top  = pContentRect->top - margin.cyTopHeight;
            pExtentRect->right = pContentRect->right + margin.cxRightWidth;
            pExtentRect->bottom = pContentRect->bottom + margin.cyBottomHeight;
        }
        /* If nothing was found, leave unchanged */
    }

    TRACE("%s\n", wine_dbgstr_rect(pExtentRect));

    return S_OK;
}

static inline void flush_rgn_data( HRGN rgn, RGNDATA *data )
{
    HRGN tmp = ExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );

    CombineRgn( rgn, rgn, tmp, RGN_OR );
    DeleteObject( tmp );
    data->rdh.nCount = 0;
}

static inline void add_row( HRGN rgn, RGNDATA *data, int x, int y, int len )
{
    RECT *rect = (RECT *)data->Buffer + data->rdh.nCount;

    if (len <= 0) return;
    rect->left   = x;
    rect->top    = y;
    rect->right  = x + len;
    rect->bottom = y + 1;
    data->rdh.nCount++;
    if (data->rdh.nCount * sizeof(RECT) > data->rdh.nRgnSize - sizeof(RECT))
        flush_rgn_data( rgn, data );
}

static HRESULT create_image_bg_region(HTHEME theme, int part, int state, const RECT *rect, HRGN *rgn)
{
    RECT r;
    HDC dc;
    HBITMAP bmp;
    HRGN hrgn;
    BOOL istrans;
    COLORREF transcolour;
    HBRUSH transbrush;
    unsigned int x, y, start;
    BITMAPINFO bitmapinfo;
    DWORD *bits;
    char buffer[4096];
    RGNDATA *data = (RGNDATA *)buffer;

    if (FAILED(GetThemeBool(theme, part, state, TMT_TRANSPARENT, &istrans)) || !istrans) {
        *rgn = CreateRectRgn(rect->left, rect->top, rect->right, rect->bottom);
        return S_OK;
    }

    r = *rect;
    OffsetRect(&r, -r.left, -r.top);

    if (FAILED(GetThemeColor(theme, part, state, TMT_TRANSPARENTCOLOR, &transcolour)))
        transcolour = RGB(255, 0, 255); /* defaults to magenta */

    dc = CreateCompatibleDC(NULL);
    if (!dc) {
        WARN("CreateCompatibleDC failed\n");
        return E_FAIL;
    }

    bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = rect->right - rect->left;
    bitmapinfo.bmiHeader.biHeight = -(rect->bottom - rect->top);
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 32;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = bitmapinfo.bmiHeader.biWidth * bitmapinfo.bmiHeader.biHeight * 4;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;

    bmp = CreateDIBSection(dc, &bitmapinfo, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    if (!bmp) {
        WARN("CreateDIBSection failed\n");
        DeleteDC(dc);
        return E_FAIL;
    }

    SelectObject(dc, bmp);

    transbrush = CreateSolidBrush(transcolour);
    FillRect(dc, &r, transbrush);
    DeleteObject(transbrush);

    if (FAILED(DrawThemeBackground(theme, dc, part, state, &r, NULL))) {
        WARN("DrawThemeBackground failed\n");
        DeleteObject(bmp);
        DeleteDC(dc);
        return E_FAIL;
    }

    data->rdh.dwSize = sizeof(data->rdh);
    data->rdh.iType  = RDH_RECTANGLES;
    data->rdh.nCount = 0;
    data->rdh.nRgnSize = sizeof(buffer) - sizeof(data->rdh);

    hrgn = CreateRectRgn(0, 0, 0, 0);

    for (y = 0; y < r.bottom; y++, bits += r.right) {
        x = 0;
        while (x < r.right) {
            while (x < r.right && (bits[x] & 0xffffff) == transcolour) x++;
            start = x;
            while (x < r.right && !((bits[x] & 0xffffff) == transcolour)) x++;
            add_row( hrgn, data, rect->left + start, rect->top + y, x - start );
        }
    }

    if (data->rdh.nCount > 0) flush_rgn_data(hrgn, data);

    *rgn = hrgn;

    DeleteObject(bmp);
    DeleteDC(dc);

    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundRegion                            (UXTHEME.@)
 *
 * Calculate the background region, taking into consideration transparent areas
 * of the background image.
 */
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pRect,
                                        HRGN *pRegion)
{
    HRESULT hr = S_OK;
    int bgtype = BT_BORDERFILL;

    TRACE("(%p,%p,%d,%d)\n", hTheme, hdc, iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    if(!pRect || !pRegion)
        return E_POINTER;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if(bgtype == BT_IMAGEFILE) {
        hr = create_image_bg_region(hTheme, iPartId, iStateId, pRect, pRegion);
    }
    else if(bgtype == BT_BORDERFILL) {
        *pRegion = CreateRectRgn(pRect->left, pRect->top, pRect->right, pRect->bottom);
        if(!*pRegion)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (bgtype == BT_NONE)
    {
        hr = E_UNEXPECTED;
    }
    else {
        FIXME("Unknown background type %d\n", bgtype);
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    return hr;
}

/* compute part size for "borderfill" backgrounds */
static HRESULT get_border_background_size (HTHEME hTheme, int iPartId,
                                           int iStateId, THEMESIZE eSize, POINT* psz)
{
    HRESULT hr = S_OK;
    int bordersize = 1;

    if (SUCCEEDED (hr = GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, 
        &bordersize)))
    {
        psz->x = psz->y = 2*bordersize;
        if (eSize != TS_MIN)
        {
            psz->x++;
            psz->y++; 
        }
    }
    return hr;
}

/***********************************************************************
 *      GetThemePartSize                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId,
                                int iStateId, RECT *prc, THEMESIZE eSize,
                                SIZE *psz)
{
    int bgtype = BT_BORDERFILL;
    HRESULT hr = S_OK;
    POINT size = {1, 1};

    if(!hTheme)
        return E_HANDLE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if (bgtype == BT_NONE)
        /* do nothing */;
    else if(bgtype == BT_IMAGEFILE)
        hr = get_image_part_size(hTheme, iPartId, iStateId, prc, eSize, &size);
    else if(bgtype == BT_BORDERFILL)
        hr = get_border_background_size (hTheme, iPartId, iStateId, eSize, &size);
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    psz->cx = size.x;
    psz->cy = size.y;
    return hr;
}


/***********************************************************************
 *      GetThemeTextExtent                                  (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                  int iStateId, LPCWSTR pszText, int iCharCount,
                                  DWORD dwTextFlags, const RECT *pBoundingRect,
                                  RECT *pExtentRect)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    RECT rt = {0,0,0xFFFF,0xFFFF};
    
    TRACE("%d %d\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    if(pBoundingRect)
        rt = *pBoundingRect;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    if(hFont)
        oldFont = SelectObject(hdc, hFont);
        
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags|DT_CALCRECT);
    *pExtentRect = rt;

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeTextMetrics                                 (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextMetrics(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, TEXTMETRICW *ptm)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;

    TRACE("(%p, %p, %d, %d)\n", hTheme, hdc, iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    if(hFont)
        oldFont = SelectObject(hdc, hFont);

    if(!GetTextMetricsW(hdc, ptm))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return hr;
}

/***********************************************************************
 *      IsThemeBackgroundPartiallyTransparent               (UXTHEME.@)
 */
BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId,
                                                  int iStateId)
{
    int bgtype = BT_BORDERFILL;
    RECT rect = {0, 0, 0, 0};
    HBITMAP bmpSrc;
    RECT rcSrc;
    BOOL hasAlpha;
    INT transparent;
    COLORREF transparentcolor;

    TRACE("(%d,%d)\n", iPartId, iStateId);

    if(!hTheme)
        return FALSE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);

    if (bgtype != BT_IMAGEFILE) return FALSE;

    if (FAILED(UXTHEME_LoadImage(hTheme, iPartId, iStateId, &rect, FALSE, &bmpSrc, &rcSrc,
                                 &hasAlpha, NULL)))
        return FALSE;

    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent,
        &transparentcolor, FALSE);
    return (transparent != ALPHABLEND_NONE);
}
