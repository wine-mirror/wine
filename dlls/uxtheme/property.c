/*
 * Win32 5.1 Theme properties
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

BOOL UXTHEME_GetNextInteger(LPCWSTR lpStringStart, LPCWSTR lpStringEnd, LPCWSTR *lpValEnd, int *value)
{
    LPCWSTR cur = lpStringStart;
    int total = 0;
    BOOL gotNeg = FALSE;

    while(cur < lpStringEnd && (*cur < '0' || *cur > '9' || *cur == '-')) cur++;
    if(cur >= lpStringEnd) {
        return FALSE;
    }
    if(*cur == '-') {
        cur++;
        gotNeg = TRUE;
    }
    while(cur < lpStringEnd && (*cur >= '0' && *cur <= '9')) {
        total = total * 10 + (*cur - '0');
        cur++;
    }
    if(gotNeg) total = -total;
    *value = total;
    if(lpValEnd) *lpValEnd = cur;
    return TRUE;
}

/***********************************************************************
 *      GetThemeBool                                        (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBool(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, BOOL *pfVal)
{
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_BOOL, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    *pfVal = FALSE;
    if(*tp->lpValue == 't' || *tp->lpValue == 'T')
        *pfVal = TRUE;
    return S_OK;
}

/***********************************************************************
 *      GetThemeColor                                       (UXTHEME.@)
 */
HRESULT WINAPI GetThemeColor(HTHEME hTheme, int iPartId, int iStateId,
                             int iPropId, COLORREF *pColor)
{
    LPCWSTR lpEnd;
    LPCWSTR lpCur;
    int red, green, blue;
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_COLOR, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lpCur = tp->lpValue;
    lpEnd = tp->lpValue + tp->dwValueLen;

    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &red);
    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &green);
    if(!UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &blue)) {
        TRACE("Could not parse color property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    *pColor = RGB(red,green,blue);
    return S_OK;
}

/***********************************************************************
 *      GetThemeEnumValue                                   (UXTHEME.@)
 */
HRESULT WINAPI GetThemeEnumValue(HTHEME hTheme, int iPartId, int iStateId,
                                 int iPropId, int *piVal)
{
    WCHAR val[60];
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_ENUM, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lstrcpynW(val, tp->lpValue, min(tp->dwValueLen+1, sizeof(val)/sizeof(val[0])));
    if(!MSSTYLES_LookupEnum(val, iPropId, piVal))
        return E_PROP_ID_UNSUPPORTED;
    return S_OK;
}

/***********************************************************************
 *      GetThemeFilename                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemeFilename(HTHEME hTheme, int iPartId, int iStateId,
                                int iPropId, LPWSTR pszThemeFilename,
                                int cchMaxBuffChars)
{
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lstrcpynW(pszThemeFilename, tp->lpValue, min(tp->dwValueLen+1, cchMaxBuffChars));
    return S_OK;
}

/***********************************************************************
 *      GetThemeFont                                        (UXTHEME.@)
 */
HRESULT WINAPI GetThemeFont(HTHEME hTheme, HDC hdc, int iPartId,
                            int iStateId, int iPropId, LOGFONTW *pFont)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeInt                                         (UXTHEME.@)
 */
HRESULT WINAPI GetThemeInt(HTHEME hTheme, int iPartId, int iStateId,
                           int iPropId, int *piVal)
{
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_INT, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    if(!UXTHEME_GetNextInteger(tp->lpValue, (tp->lpValue + tp->dwValueLen), NULL, piVal)) {
        TRACE("Could not parse int property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeIntList                                     (UXTHEME.@)
 */
HRESULT WINAPI GetThemeIntList(HTHEME hTheme, int iPartId, int iStateId,
                               int iPropId, INTLIST *pIntList)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemePosition                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemePosition(HTHEME hTheme, int iPartId, int iStateId,
                                int iPropId, POINT *pPoint)
{
    LPCWSTR lpEnd;
    LPCWSTR lpCur;
    PTHEME_PROPERTY tp;
    int x,y;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_POSITION, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lpCur = tp->lpValue;
    lpEnd = tp->lpValue + tp->dwValueLen;

    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &x);
    if(!UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &y)) {
        TRACE("Could not parse position property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    pPoint->x = x;
    pPoint->y = y;
    return S_OK;
}

/***********************************************************************
 *      GetThemeRect                                        (UXTHEME.@)
 */
HRESULT WINAPI GetThemeRect(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, RECT *pRect)
{
    LPCWSTR lpEnd;
    LPCWSTR lpCur;
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_RECT, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lpCur = tp->lpValue;
    lpEnd = tp->lpValue + tp->dwValueLen;

    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, (int*)&pRect->left);
    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, (int*)&pRect->top);
    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, (int*)&pRect->right);
    if(!UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, (int*)&pRect->bottom)) {
        TRACE("Could not parse rect property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeString                                      (UXTHEME.@)
 */
HRESULT WINAPI GetThemeString(HTHEME hTheme, int iPartId, int iStateId,
                              int iPropId, LPWSTR pszBuff, int cchMaxBuffChars)
{
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lstrcpynW(pszBuff, tp->lpValue, min(tp->dwValueLen+1, cchMaxBuffChars));
    return S_OK;
}

/***********************************************************************
 *      GetThemeMargins                                     (UXTHEME.@)
 */
HRESULT WINAPI GetThemeMargins(HTHEME hTheme, HDC hdc, int iPartId,
                               int iStateId, int iPropId, RECT *prc,
                               MARGINS *pMargins)
{
    LPCWSTR lpEnd;
    LPCWSTR lpCur;
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_MARGINS, iPropId)))
        return E_PROP_ID_UNSUPPORTED;
    lpCur = tp->lpValue;
    lpEnd = tp->lpValue + tp->dwValueLen;

    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cxLeftWidth);
    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cxRightWidth);
    UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cyTopHeight);
    if(!UXTHEME_GetNextInteger(lpCur, lpEnd, &lpCur, &pMargins->cyBottomHeight)) {
        TRACE("Could not parse margins property\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeMetric                                      (UXTHEME.@)
 */
HRESULT WINAPI GetThemeMetric(HTHEME hTheme, HDC hdc, int iPartId,
                              int iStateId, int iPropId, int *piVal)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemePropertyOrigin                              (UXTHEME.@)
 */
HRESULT WINAPI GetThemePropertyOrigin(HTHEME hTheme, int iPartId, int iStateId,
                                      int iPropId, PROPERTYORIGIN *pOrigin)
{
    PTHEME_PROPERTY tp;

    TRACE("(%d, %d, %d)\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;

    if(!(tp = MSSTYLES_FindProperty(hTheme, iPartId, iStateId, 0, iPropId))) {
        *pOrigin = PO_NOTFOUND;
        return S_OK;
    }
    *pOrigin = tp->origin;
    return S_OK;
}
