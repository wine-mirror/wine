/*
 * Win32 5.1 Theme metrics
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

#include "msstyles.h"
#include "uxthemedll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 *      GetThemeSysBool                                     (UXTHEME.@)
 */
BOOL WINAPI GetThemeSysBool(HTHEME hTheme, int iBoolID)
{
    FIXME("%d: stub\n", iBoolID);
    return FALSE;
}

/***********************************************************************
 *      GetThemeSysColor                                    (UXTHEME.@)
 */
COLORREF WINAPI GetThemeSysColor(HTHEME hTheme, int iColorID)
{
    FIXME("%d: stub\n", iColorID);
    return FALSE;
}

/***********************************************************************
 *      GetThemeSysColorBrush                               (UXTHEME.@)
 */
HBRUSH WINAPI GetThemeSysColorBrush(HTHEME hTheme, int iColorID)
{
    FIXME("%d: stub\n", iColorID);
    return FALSE;
}

/***********************************************************************
 *      GetThemeSysFont                                     (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysFont(HTHEME hTheme, int iFontID, LOGFONTW *plf)
{
    FIXME("%d: stub\n", iFontID);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeSysInt                                      (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysInt(HTHEME hTheme, int iIntID, int *piValue)
{
    FIXME("%d: stub\n", iIntID);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeSysSize                                     (UXTHEME.@)
 */
int WINAPI GetThemeSysSize(HTHEME hTheme, int iSizeID)
{
    FIXME("%d: stub\n", iSizeID);
    return 0;
}

/***********************************************************************
 *      GetThemeSysString                                   (UXTHEME.@)
 */
HRESULT WINAPI GetThemeSysString(HTHEME hTheme, int iStringID,
                                 LPWSTR pszStringBuff, int cchMaxStringChars)
{
    FIXME("%d: stub\n", iStringID);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}
