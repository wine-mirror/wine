/*
 * Win32 5.1 Theme system
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
 * Defines and global variables
 */

DWORD dwThemeAppProperties = STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS;
ATOM atWindowTheme;
ATOM atSubAppName;
ATOM atSubIdList;

/***********************************************************************/

/***********************************************************************
 *      UXTHEME_InitSystem
 */
void UXTHEME_InitSystem()
{
    const WCHAR szWindowTheme[] = {
        'u','x','_','t','h','e','m','e','\0'
    };
    const WCHAR szSubAppName[] = {
        'u','x','_','s','u','b','a','p','p','\0'
    };
    const WCHAR szSubIdList[] = {
        'u','x','_','s','u','b','i','d','l','s','t','\0'
    };

    atWindowTheme = GlobalAddAtomW(szWindowTheme);
    atSubAppName  = GlobalAddAtomW(szSubAppName);
    atSubIdList   = GlobalAddAtomW(szSubIdList);
}

/***********************************************************************
 *      IsAppThemed                                         (UXTHEME.@)
 */
BOOL WINAPI IsAppThemed(void)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *      IsThemeActive                                       (UXTHEME.@)
 */
BOOL WINAPI IsThemeActive(void)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *      EnableTheming                                       (UXTHEME.@)
 *
 * NOTES
 * This is a global and persistant change
 */
HRESULT WINAPI EnableTheming(BOOL fEnable)
{
    FIXME("%d: stub\n", fEnable);
    return ERROR_CALL_NOT_IMPLEMENTED;
 }

/***********************************************************************
 *      OpenThemeData                                       (UXTHEME.@)
 */
HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
    FIXME("%p %s: stub\n", hwnd, debugstr_w(pszClassList));
    return NULL;
}

/***********************************************************************
 *      GetWindowTheme                                      (UXTHEME.@)
 *
 * Retrieve the last theme opened for a window
 */
HTHEME WINAPI GetWindowTheme(HWND hwnd)
{
    TRACE("(%p)\n", hwnd);
    return GetPropW(hwnd, MAKEINTATOMW(atWindowTheme));
}

/***********************************************************************
 *      UXTHEME_SetWindowProperty
 *
 * I'm using atoms as there may be large numbers of duplicated strings
 * and they do the work of keeping memory down as a cause of that quite nicely
 */
HRESULT UXTHEME_SetWindowProperty(HWND hwnd, ATOM aProp, LPCWSTR pszValue)
{
    ATOM oldValue = (ATOM)(size_t)RemovePropW(hwnd, MAKEINTATOMW(aProp));
    if(oldValue)
        DeleteAtom(oldValue);
    if(pszValue) {
        ATOM atValue = AddAtomW(pszValue);
        if(!atValue
           || !SetPropW(hwnd, MAKEINTATOMW(aProp), (LPWSTR)MAKEINTATOMW(atValue))) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            if(atValue) DeleteAtom(atValue);
            return hr;
        }
    }
    return S_OK;
}

/***********************************************************************
 *      SetWindowTheme                                      (UXTHEME.@)
 *
 * Persistant through the life of the window, even after themes change
 */
HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName,
                              LPCWSTR pszSubIdList)
{
    HRESULT hr;
    TRACE("(%p,%s,%s)\n", hwnd, debugstr_w(pszSubAppName),
          debugstr_w(pszSubIdList));
    hr = UXTHEME_SetWindowProperty(hwnd, atSubAppName, pszSubAppName);
    if(SUCCEEDED(hr))
      hr = UXTHEME_SetWindowProperty(hwnd, atSubIdList, pszSubIdList);
    return hr;
}

/***********************************************************************
 *      GetCurrentThemeName                                 (UXTHEME.@)
 */
HRESULT WINAPI GetCurrentThemeName(LPWSTR pszThemeFileName, int dwMaxNameChars,
                                   LPWSTR pszColorBuff, int cchMaxColorChars,
                                   LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      GetThemeAppProperties                               (UXTHEME.@)
 */
DWORD WINAPI GetThemeAppProperties(void)
{
    return dwThemeAppProperties;
}

/***********************************************************************
 *      SetThemeAppProperties                               (UXTHEME.@)
 */
void WINAPI SetThemeAppProperties(DWORD dwFlags)
{
    TRACE("(0x%08lx)\n", dwFlags);
    dwThemeAppProperties = dwFlags;
}

/***********************************************************************
 *      CloseThemeData                                      (UXTHEME.@)
 */
HRESULT WINAPI CloseThemeData(HTHEME hTheme)
{
    FIXME("stub\n");
    if(!hTheme)
        return E_HANDLE;
    return S_OK;
}

/***********************************************************************
 *      HitTestThemeBackground                              (UXTHEME.@)
 */
HRESULT WINAPI HitTestThemeBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, DWORD dwOptions,
                                     const RECT *pRect, HRGN hrgn,
                                     POINT ptTest, WORD *pwHitTestCode)
{
    FIXME("%d %d 0x%08lx: stub\n", iPartId, iStateId, dwOptions);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      IsThemePartDefined                                  (UXTHEME.@)
 */
BOOL WINAPI IsThemePartDefined(HTHEME hTheme, int iPartId, int iStateId)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    return FALSE;
}

/**********************************************************************
 *      Undocumented functions
 */

/**********************************************************************
 *      QueryThemeServices                                 (UXTHEME.1)
 *
 * RETURNS
 *     some kind of status flag
 */
DWORD WINAPI QueryThemeServices()
{
    FIXME("stub\n");
    return 3; /* This is what is returned under XP in most cases */
}


/**********************************************************************
 *      OpenThemeFile                                      (UXTHEME.2)
 *
 * Opens a theme file, which can be used to change the current theme, etc
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Color defined in the theme, eg. NormalColor
 *     pszSizeName         Size defined in the theme, eg. NormalSize
 *     hThemeFile          Handle to theme file
 */
HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE *hThemeFile,
                             DWORD unknown)
{
    FIXME("%s,%s,%s,%ld: stub\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszColorName),
          debugstr_w(pszSizeName), unknown);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**********************************************************************
 *      CloseThemeFile                                     (UXTHEME.3)
 *
 * Releases theme file handle returned by OpenThemeFile
 *
 * PARAMS
 *     hThemeFile           Handle to theme file
 */
HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile)
{
    FIXME("stub\n");
    return S_OK;
}

/**********************************************************************
 *      ApplyTheme                                         (UXTHEME.4)
 *
 * Set a theme file to be the currently active theme
 *
 * PARAMS
 *     hThemeFile           Handle to theme file
 *     unknown              See notes
 *     hWnd                 Window requesting the theme change
 *
 * NOTES
 * I'm not sure what the second parameter is (the datatype is likely wrong), other then this:
 * Under XP if I pass
 * char b[] = "";
 *   the theme is applied with the screen redrawing really badly (flickers)
 * char b[] = "\0"; where \0 can be one or more of any character, makes no difference
 *   the theme is applied smoothly (screen does not flicker)
 * char *b = "\0" or NULL; where \0 can be zero or more of any character, makes no difference
 *   the function fails returning invalid parameter...very strange
 */
HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, char *unknown, HWND hWnd)
{
    FIXME("%s: stub\n", unknown);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**********************************************************************
 *      GetThemeDefaults                                   (UXTHEME.7)
 *
 * Get the default color & size for a theme
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Buffer to recieve the default color name
 *     dwColorNameLen      Length, in characters, of color name buffer
 *     pszSizeName         Buffer to recieve the default size name
 *     dwSizeNameLen       Length, in characters, of size name buffer
 */
HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen)
{
    FIXME("%s: stub\n", debugstr_w(pszThemeFileName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**********************************************************************
 *      EnumThemes                                         (UXTHEME.8)
 *
 * Enumerate available themes, calls specified EnumThemeProc for each
 * theme found. Passes lpData through to callback function.
 *
 * PARAMS
 *     pszThemePath        Path containing themes
 *     callback            Called for each theme found in path
 *     lpData              Passed through to callback
 */
HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, EnumThemeProc callback,
                          LPVOID lpData)
{
    FIXME("%s: stub\n", debugstr_w(pszThemePath));
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/**********************************************************************
 *      EnumThemeColors                                    (UXTHEME.9)
 *
 * Enumerate theme colors available with a particular size
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszSizeName         Theme size to enumerate available colors
 *                         If NULL the default theme size is used
 *     dwColorNum          Color index to retrieve, increment from 0
 *     pszColorName        Output color name
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwColorName does not refer to a color
 *          or when pszSizeName does not refer to a valid size
 *
 * NOTES
 * XP fails with E_POINTER when pszColorName points to a buffer smaller then 605
 * characters
 */
HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, LPWSTR pszColorName)
{
    FIXME("%s %s %ld: stub\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszSizeName), dwColorNum);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**********************************************************************
 *      EnumThemeSizes                                     (UXTHEME.10)
 *
 * Enumerate theme colors available with a particular size
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Theme color to enumerate available sizes
 *                         If NULL the default theme color is used
 *     dwSizeNum           Size index to retrieve, increment from 0
 *     pszSizeName         Output size name
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwSizeName does not refer to a size
 *          or when pszColorName does not refer to a valid color
 *
 * NOTES
 * XP fails with E_POINTER when pszSizeName points to a buffer smaller then 605
 * characters
 */
HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, LPWSTR pszSizeName)
{
    FIXME("%s %s %ld: stub\n", debugstr_w(pszThemeFileName), debugstr_w(pszColorName), dwSizeNum);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**********************************************************************
 *      ParseThemeIniFile                                  (UXTHEME.11)
 *
 * Enumerate data in a theme INI file.
 *
 * PARAMS
 *     pszIniFileName      Path to a theme ini file
 *     pszUnknown          Cannot be NULL, L"" is valid
 *     callback            Called for each found entry
 *     lpData              Passed through to callback
 *
 * RETURNS
 *     S_OK on success
 *     0x800706488 (Unknown property) when enumeration is canceled from callback
 *
 * NOTES
 * When pszUnknown is NULL the callback is never called, the value does not seem to surve
 * any other purpose
 */
HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 ParseThemeIniFileProc callback, LPVOID lpData)
{
    FIXME("%s %s: stub\n", debugstr_w(pszIniFileName), debugstr_w(pszUnknown));
    return ERROR_CALL_NOT_IMPLEMENTED;
}
