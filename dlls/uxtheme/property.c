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

#include "uxthemedll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 *      GetThemeBool                                        (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBool(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, BOOL *pfVal)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeColor                                       (UXTHEME.@)
 */
HRESULT WINAPI GetThemeColor(HTHEME hTheme, int iPartId, int iStateId,
                             int iPropId, COLORREF *pColor)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeEnumValue                                   (UXTHEME.@)
 */
HRESULT WINAPI GetThemeEnumValue(HTHEME hTheme, int iPartId, int iStateId,
                                 int iPropId, int *piVal)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeFilename                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemeFilename(HTHEME hTheme, int iPartId, int iStateId,
                                int iPropId, LPWSTR pszThemeFilename,
                                int cchMaxBuffChars)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeRect                                        (UXTHEME.@)
 */
HRESULT WINAPI GetThemeRect(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, RECT *pRect)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeString                                      (UXTHEME.@)
 */
HRESULT WINAPI GetThemeString(HTHEME hTheme, int iPartId, int iStateId,
                              int iPropId, LPWSTR pszBuff, int cchMaxBuffChars)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeMargins                                     (UXTHEME.@)
 */
HRESULT WINAPI GetThemeMargins(HTHEME hTheme, HDC hdc, int iPartId,
                               int iStateId, int iPropId, RECT *prc,
                               MARGINS *pMargins)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    FIXME("%d %d %d: stub\n", iPartId, iStateId, iPropId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}
