/*
 * Win32 5.1 Theme drawing
 *
 * Copyright (C) 2003 Kevin Koltzau
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "uxtheme.h"
#include "tmschema.h"

#include "msstyles.h"
#include "uxthemedll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 *      EnableThemeDialogTexture                            (UXTHEME.@)
 */
HRESULT WINAPI EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags)
{
    FIXME("%p 0x%08lx: stub\n", hwnd, dwFlags);
    return ERROR_CALL_NOT_IMPLEMENTED;
 }

/***********************************************************************
 *      IsThemeDialogTextureEnabled                         (UXTHEME.@)
 */
BOOL WINAPI IsThemeDialogTextureEnabled(void)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *      DrawThemeParentBackground                           (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeParentBackground(HWND hwnd, HDC hdc, RECT *prc)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeBackground                                 (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, const RECT *pRect,
                                   const RECT *pClipRect)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeBackgroundEx                               (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, const RECT *pRect,
                                     const DTBGOPTS *pOptions)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeEdge                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId,
                             int iStateId, const RECT *pDestRect, UINT uEdge,
                             UINT uFlags, RECT *pContentRect)
{
    FIXME("%d %d 0x%08x 0x%08x: stub\n", iPartId, iStateId, uEdge, uFlags);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeIcon                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeText                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                             DWORD dwTextFlags2, const RECT *pRect)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
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

    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(FAILED(hr)) {
        TRACE("Margins not found\n");
        return hr;
    }
    pContentRect->left = pBoundingRect->left + margin.cxLeftWidth;
    pContentRect->top  = pBoundingRect->top + margin.cyTopHeight;
    pContentRect->right = pBoundingRect->right - margin.cxRightWidth;
    pContentRect->bottom = pBoundingRect->bottom - margin.cyBottomHeight;

    TRACE("left:%ld,top:%ld,right:%ld,bottom:%ld\n", pContentRect->left, pContentRect->top, pContentRect->right, pContentRect->bottom);

    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundExtent                            (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pContentRect,
                                        RECT *pExtentRect)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeBackgroundRegion                            (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pRect,
                                        HRGN *pRegion)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemePartSize                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId,
                                int iStateId, RECT *prc, THEMESIZE eSize,
                                SIZE *psz)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, eSize);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 *      GetThemeTextExtent                                  (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                  int iStateId, LPCWSTR pszText, int iCharCount,
                                  DWORD dwTextFlags, const RECT *pBoundingRect,
                                  RECT *pExtentRect)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeTextMetrics                                 (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextMetrics(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, TEXTMETRICW *ptm)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      IsThemeBackgroundPartiallyTransparent               (UXTHEME.@)
 */
BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId,
                                                  int iStateId)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    return FALSE;
}
