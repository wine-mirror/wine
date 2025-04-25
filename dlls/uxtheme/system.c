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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include "vssym32.h"

#include "uxthemedll.h"
#include "msstyles.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

static const WCHAR szThemeManager[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";

ATOM atDialogThemeEnabled;

static DWORD dwThemeAppProperties = STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS;
static ATOM atWindowTheme;
static ATOM atSubAppName;
static ATOM atSubIdList;

static BOOL bThemeActive = FALSE;
static WCHAR szCurrentTheme[MAX_PATH];
static WCHAR szCurrentColor[64];
static WCHAR szCurrentSize[64];

struct user_api_hook user_api = {0};

/***********************************************************************/

static BOOL CALLBACK UXTHEME_broadcast_msg_enumchild (HWND hWnd, LPARAM msg)
{
    PostMessageW(hWnd, msg, 0, 0);
    return TRUE;
}

/* Broadcast a message to *all* windows, including children */
static BOOL CALLBACK UXTHEME_broadcast_msg (HWND hWnd, LPARAM msg)
{
    if (hWnd == NULL)
    {
	EnumWindows (UXTHEME_broadcast_msg, msg);
    }
    else
    {
	PostMessageW(hWnd, msg, 0, 0);
	EnumChildWindows (hWnd, UXTHEME_broadcast_msg_enumchild, msg);
    }
    return TRUE;
}

/* At the end of the day this is a subset of what SHRegGetPath() does - copied
 * here to avoid linking against shlwapi. */
static DWORD query_reg_path (HKEY hKey, LPCWSTR lpszValue,
                             LPVOID pvData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = MAX_PATH * sizeof(WCHAR), dwExpDataLen;

  TRACE("(hkey=%p,%s,%p)\n", hKey, debugstr_w(lpszValue),
        pvData);

  dwRet = RegQueryValueExW(hKey, lpszValue, 0, &dwType, pvData, &dwUnExpDataLen);
  if (dwRet!=ERROR_SUCCESS && dwRet!=ERROR_MORE_DATA)
      return dwRet;

  if (dwType == REG_EXPAND_SZ)
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPWSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if (dwRet == ERROR_MORE_DATA)
    {
      WCHAR emptyW[] = L"";
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, 0, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, emptyW, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenW(pvData) + 1) * sizeof(WCHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc );
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, MAX_PATH );
      if (dwExpDataLen > MAX_PATH) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  return dwRet;
}

/***********************************************************************
 *      UXTHEME_LoadTheme
 *
 * Set the current active theme from the registry
 */
static void UXTHEME_LoadTheme(void)
{
    HKEY hKey;
    DWORD buffsize;
    HRESULT hr;
    WCHAR tmp[10];
    PTHEME_FILE pt;

    /* Get current theme configuration */
    if(!RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
        TRACE("Loading theme config\n");
        buffsize = sizeof(tmp);
        if (!RegQueryValueExW(hKey, L"ThemeActive", NULL, NULL, (BYTE*)tmp, &buffsize)) {
            bThemeActive = (tmp[0] != '0');
        }
        else {
            bThemeActive = FALSE;
            TRACE("Failed to get ThemeActive: %ld\n", GetLastError());
        }
        buffsize = sizeof(szCurrentColor);
        if (RegQueryValueExW(hKey, L"ColorName", NULL, NULL, (BYTE*)szCurrentColor, &buffsize))
            szCurrentColor[0] = '\0';
        buffsize = sizeof(szCurrentSize);
        if (RegQueryValueExW(hKey, L"SizeName", NULL, NULL, (BYTE*)szCurrentSize, &buffsize))
            szCurrentSize[0] = '\0';
        if (query_reg_path (hKey, L"DllName", szCurrentTheme))
            szCurrentTheme[0] = '\0';
        RegCloseKey(hKey);
    }
    else
        TRACE("Failed to open theme registry key\n");

    if(bThemeActive) {
        /* Make sure the theme requested is actually valid */
        hr = MSSTYLES_OpenThemeFile(szCurrentTheme,
                                    szCurrentColor[0]?szCurrentColor:NULL,
                                    szCurrentSize[0]?szCurrentSize:NULL,
                                    &pt);
        if(FAILED(hr)) {
            bThemeActive = FALSE;
            szCurrentTheme[0] = '\0';
            szCurrentColor[0] = '\0';
            szCurrentSize[0] = '\0';
        }
        else {
            /* Make sure the global color & size match the theme */
            lstrcpynW(szCurrentColor, pt->pszSelectedColor, ARRAY_SIZE(szCurrentColor));
            lstrcpynW(szCurrentSize, pt->pszSelectedSize, ARRAY_SIZE(szCurrentSize));

            UXTHEME_SetActiveTheme(pt);
            TRACE("Theme active: %s %s %s\n", debugstr_w(szCurrentTheme),
                debugstr_w(szCurrentColor), debugstr_w(szCurrentSize));
            MSSTYLES_CloseThemeFile(pt);
        }
    }
    if(!bThemeActive) {
        MSSTYLES_SetActiveTheme(NULL, FALSE);
        TRACE("Theming not active\n");
    }
}

/***********************************************************************/

static const WCHAR * const SysColorsNames[] =
{
    L"Scrollbar",               /* COLOR_SCROLLBAR */
    L"Background",              /* COLOR_BACKGROUND */
    L"ActiveTitle",             /* COLOR_ACTIVECAPTION */
    L"InactiveTitle",           /* COLOR_INACTIVECAPTION */
    L"Menu",                    /* COLOR_MENU */
    L"Window",                  /* COLOR_WINDOW */
    L"WindowFrame",             /* COLOR_WINDOWFRAME */
    L"MenuText",                /* COLOR_MENUTEXT */
    L"WindowText",              /* COLOR_WINDOWTEXT */
    L"TitleText",               /* COLOR_CAPTIONTEXT */
    L"ActiveBorder",            /* COLOR_ACTIVEBORDER */
    L"InactiveBorder",          /* COLOR_INACTIVEBORDER */
    L"AppWorkSpace",            /* COLOR_APPWORKSPACE */
    L"Hilight",                 /* COLOR_HIGHLIGHT */
    L"HilightText",             /* COLOR_HIGHLIGHTTEXT */
    L"ButtonFace",              /* COLOR_BTNFACE */
    L"ButtonShadow",            /* COLOR_BTNSHADOW */
    L"GrayText",                /* COLOR_GRAYTEXT */
    L"ButtonText",              /* COLOR_BTNTEXT */
    L"InactiveTitleText",       /* COLOR_INACTIVECAPTIONTEXT */
    L"ButtonHilight",           /* COLOR_BTNHIGHLIGHT */
    L"ButtonDkShadow",          /* COLOR_3DDKSHADOW */
    L"ButtonLight",             /* COLOR_3DLIGHT */
    L"InfoText",                /* COLOR_INFOTEXT */
    L"InfoWindow",              /* COLOR_INFOBK */
    L"ButtonAlternateFace",     /* COLOR_ALTERNATEBTNFACE */
    L"HotTrackingColor",        /* COLOR_HOTLIGHT */
    L"GradientActiveTitle",     /* COLOR_GRADIENTACTIVECAPTION */
    L"GradientInactiveTitle",   /* COLOR_GRADIENTINACTIVECAPTION */
    L"MenuHilight",             /* COLOR_MENUHILIGHT */
    L"MenuBar",                 /* COLOR_MENUBAR */
};

static const WCHAR strColorKey[] = L"Control Panel\\Colors";

#define NUM_SYS_COLORS     (COLOR_MENUBAR+1)

struct system_metrics
{
    COLORREF system_colors[NUM_SYS_COLORS];
    NONCLIENTMETRICSW non_client_metrics;
    LOGFONTW icon_title_font;
    DWORD gradient_caption;
    DWORD flat_menu;
};

static BOOL UXTHEME_GetSystemMetrics(struct system_metrics *metrics)
{
    DPI_AWARENESS_CONTEXT old_context;
    BOOL ret = FALSE;
    int i;

    for (i = 0; i < NUM_SYS_COLORS; ++i)
        metrics->system_colors[i] = GetSysColor(i);

    old_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);

    memset(&metrics->non_client_metrics, 0, sizeof(metrics->non_client_metrics));
    metrics->non_client_metrics.cbSize = sizeof(metrics->non_client_metrics);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics->non_client_metrics),
                               &metrics->non_client_metrics, 0))
        goto done;
    memset(&metrics->icon_title_font, 0, sizeof(metrics->icon_title_font));
    if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(metrics->icon_title_font),
                               &metrics->icon_title_font, 0))
        goto done;
    if (!SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &metrics->gradient_caption, 0))
        goto done;
    if (!SystemParametersInfoW(SPI_GETFLATMENU, 0, &metrics->flat_menu, 0))
        goto done;

    ret = TRUE;
done:
    SetThreadDpiAwarenessContext(old_context);
    return ret;
}

/* Read back old settings after a theme was deactivated */
static BOOL UXTHEME_GetUnthemedSystemMetrics(struct system_metrics *metrics)
{
    HKEY theme_manager_key = NULL, color_key = NULL;
    BOOL ret = FALSE;
    WCHAR string[13];
    int i, r, g, b;
    DWORD size;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, szThemeManager, 0, KEY_QUERY_VALUE, &theme_manager_key))
        goto done;

    if (RegOpenKeyExW(theme_manager_key, strColorKey, 0, KEY_QUERY_VALUE, &color_key))
        goto done;

    for (i = 0; i < NUM_SYS_COLORS; ++i)
    {
        size = sizeof(string);
        if (RegQueryValueExW(color_key, SysColorsNames[i], 0, NULL, (BYTE *)string, &size))
            goto done;

        if (swscanf(string, L"%d %d %d", &r, &g, &b) != 3)
            goto done;

        metrics->system_colors[i] = RGB(r, g, b);
    }

    size = sizeof(metrics->non_client_metrics);
    if (RegQueryValueExW(theme_manager_key, L"NonClientMetrics", 0, NULL,
                         (BYTE *)&metrics->non_client_metrics, &size))
        goto done;
    size = sizeof(metrics->icon_title_font);
    if (RegQueryValueExW(theme_manager_key, L"IconTitleFont", 0, NULL,
                         (BYTE *)&metrics->icon_title_font, &size))
        goto done;
    size = sizeof(metrics->gradient_caption);
    if (RegQueryValueExW(theme_manager_key, L"GradientCaption", 0, NULL,
                         (BYTE *)&metrics->gradient_caption, &size))
        goto done;
    size = sizeof(metrics->flat_menu);
    if (RegQueryValueExW(theme_manager_key, L"FlatMenu", 0, NULL, (BYTE *)&metrics->flat_menu,
                         &size))
        goto done;

    ret = TRUE;
done:
    RegCloseKey(color_key);
    RegCloseKey(theme_manager_key);
    return ret;
}

static void UXTHEME_SaveUnthemedSystemMetrics(struct system_metrics *metrics)
{
    HKEY theme_manager_key, color_key;
    WCHAR string[13];
    DWORD length;
    int i;

    if (!RegCreateKeyExW(HKEY_CURRENT_USER, szThemeManager, 0, 0, 0, KEY_ALL_ACCESS, 0,
                         &theme_manager_key, 0))
    {
        if (!RegCreateKeyExW(theme_manager_key, strColorKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &color_key,
                             0))
        {
            for (i = 0; i < NUM_SYS_COLORS; ++i)
            {
                length = swprintf(string, ARRAY_SIZE(string), L"%d %d %d",
                                  GetRValue(metrics->system_colors[i]),
                                  GetGValue(metrics->system_colors[i]),
                                  GetBValue(metrics->system_colors[i]));
                RegSetValueExW(color_key, SysColorsNames[i], 0, REG_SZ, (BYTE *)string,
                               (length + 1) * sizeof(WCHAR));
            }

            RegCloseKey(color_key);
        }

        RegSetValueExW(theme_manager_key, L"NonClientMetrics", 0, REG_BINARY,
                       (BYTE *)&metrics->non_client_metrics, sizeof(metrics->non_client_metrics));
        RegSetValueExW(theme_manager_key, L"IconTitleFont", 0, REG_BINARY,
                       (BYTE *)&metrics->icon_title_font, sizeof(metrics->icon_title_font));
        RegSetValueExW(theme_manager_key, L"GradientCaption", 0, REG_DWORD,
                       (BYTE *)&metrics->gradient_caption, sizeof(metrics->gradient_caption));
        RegSetValueExW(theme_manager_key, L"FlatMenu", 0, REG_DWORD, (BYTE *)&metrics->flat_menu,
                       sizeof(metrics->flat_menu));
        RegCloseKey(theme_manager_key);
    }
}

/* Make system settings persistent, so they're in effect even w/o uxtheme 
 * loaded.
 * For efficiency reasons, only the last SystemParametersInfoW sets
 * SPIF_SENDWININICHANGE */
static void UXTHEME_SaveSystemMetrics(struct system_metrics *metrics, BOOL send_syscolor_change)
{
    int i, length, index[NUM_SYS_COLORS];
    DPI_AWARENESS_CONTEXT old_context;
    WCHAR string[13];
    HKEY hkey;

    if (!RegCreateKeyExW(HKEY_CURRENT_USER, strColorKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, 0))
    {
        for (i = 0; i < NUM_SYS_COLORS; ++i)
        {
            length = swprintf(string, ARRAY_SIZE(string), L"%d %d %d",
                              GetRValue(metrics->system_colors[i]),
                              GetGValue(metrics->system_colors[i]),
                              GetBValue(metrics->system_colors[i]));
            RegSetValueExW(hkey, SysColorsNames[i], 0, REG_SZ, (BYTE *)string,
                           (length + 1) * sizeof(WCHAR));
        }
        RegCloseKey(hkey);
    }

    if (send_syscolor_change)
    {
        for (i = 0; i < NUM_SYS_COLORS; ++i)
            index[i] = i;

        SetSysColors(NUM_SYS_COLORS, index, metrics->system_colors);
    }

    old_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);

    SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(metrics->non_client_metrics),
                          &metrics->non_client_metrics, SPIF_UPDATEINIFILE);
    SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(metrics->icon_title_font),
                          &metrics->icon_title_font, SPIF_UPDATEINIFILE);
    SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0, (void *)(INT_PTR)metrics->gradient_caption,
                          SPIF_UPDATEINIFILE);
    SystemParametersInfoW(SPI_SETFLATMENU, 0, (void *)(INT_PTR)metrics->flat_menu,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    SetThreadDpiAwarenessContext(old_context);
}

/***********************************************************************
 *      UXTHEME_SetActiveTheme
 *
 * Change the current active theme
 */
HRESULT UXTHEME_SetActiveTheme(PTHEME_FILE tf)
{
    BOOL ret, loaded_before = FALSE, same_theme = FALSE;
    struct system_metrics metrics;
    DWORD size;
    HKEY hKey;
    WCHAR tmp[2];
    HRESULT hr;

    if(tf) {
        bThemeActive = TRUE;
        same_theme = !lstrcmpW(szCurrentTheme, tf->szThemeFile)
                     && !lstrcmpW(szCurrentColor, tf->pszSelectedColor)
                     && !lstrcmpW(szCurrentSize, tf->pszSelectedSize);
        lstrcpynW(szCurrentTheme, tf->szThemeFile, ARRAY_SIZE(szCurrentTheme));
        lstrcpynW(szCurrentColor, tf->pszSelectedColor, ARRAY_SIZE(szCurrentColor));
        lstrcpynW(szCurrentSize, tf->pszSelectedSize, ARRAY_SIZE(szCurrentSize));

        ret = UXTHEME_GetSystemMetrics(&metrics);

        /* Check if theming is already active */
        if (!RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey))
        {
            size = sizeof(tmp);
            if (!RegQueryValueExW(hKey, L"LoadedBefore", NULL, NULL, (BYTE *)tmp, &size))
                loaded_before = (tmp[0] != '0');
            else
                WARN("Failed to get LoadedBefore: %ld\n", GetLastError());
            RegCloseKey(hKey);
        }
        if (loaded_before && same_theme)
            return MSSTYLES_SetActiveTheme(tf, FALSE);

        if (!loaded_before && ret)
            UXTHEME_SaveUnthemedSystemMetrics(&metrics);
    }
    else {
        bThemeActive = FALSE;
        szCurrentTheme[0] = '\0';
        szCurrentColor[0] = '\0';
        szCurrentSize[0] = '\0';
    }

    TRACE("Writing theme config to registry\n");
    if(!RegCreateKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
        tmp[0] = bThemeActive?'1':'0';
        tmp[1] = '\0';
        RegSetValueExW(hKey, L"ThemeActive", 0, REG_SZ, (const BYTE*)tmp, sizeof(WCHAR)*2);
        if(bThemeActive) {
            RegSetValueExW(hKey, L"ColorName", 0, REG_SZ, (const BYTE*)szCurrentColor,
		(lstrlenW(szCurrentColor)+1)*sizeof(WCHAR));
            RegSetValueExW(hKey, L"SizeName", 0, REG_SZ, (const BYTE*)szCurrentSize,
		(lstrlenW(szCurrentSize)+1)*sizeof(WCHAR));
            RegSetValueExW(hKey, L"DllName", 0, REG_SZ, (const BYTE*)szCurrentTheme,
		(lstrlenW(szCurrentTheme)+1)*sizeof(WCHAR));
            RegSetValueExW(hKey, L"LoadedBefore", 0, REG_SZ, (const BYTE *)tmp, sizeof(WCHAR) * 2);
        }
        else {
            RegDeleteValueW(hKey, L"ColorName");
            RegDeleteValueW(hKey, L"SizeName");
            RegDeleteValueW(hKey, L"DllName");
            RegDeleteValueW(hKey, L"LoadedBefore");
        }
        RegCloseKey(hKey);
    }
    else
        TRACE("Failed to open theme registry key\n");

    hr = MSSTYLES_SetActiveTheme(tf, TRUE);
    if (bThemeActive)
    {
        if (UXTHEME_GetSystemMetrics(&metrics))
            UXTHEME_SaveSystemMetrics(&metrics, FALSE);
    }
    else
    {
        if (UXTHEME_GetUnthemedSystemMetrics(&metrics))
            UXTHEME_SaveSystemMetrics(&metrics, TRUE);
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_InitSystem
 */
void UXTHEME_InitSystem(HINSTANCE hInst)
{
    atWindowTheme        = GlobalAddAtomW(L"ux_theme");
    atSubAppName         = GlobalAddAtomW(L"ux_subapp");
    atSubIdList          = GlobalAddAtomW(L"ux_subidlst");
    atDialogThemeEnabled = GlobalAddAtomW(L"ux_dialogtheme");

    UXTHEME_LoadTheme();
    ThemeHooksInstall();
}

void UXTHEME_UninitSystem(void)
{
    ThemeHooksRemove();
    MSSTYLES_SetActiveTheme(NULL, FALSE);

    GlobalDeleteAtom(atWindowTheme);
    GlobalDeleteAtom(atSubAppName);
    GlobalDeleteAtom(atSubIdList);
    GlobalDeleteAtom(atDialogThemeEnabled);
}

/***********************************************************************
 *      IsAppThemed                                         (UXTHEME.@)
 */
BOOL WINAPI IsAppThemed(void)
{
    return IsThemeActive();
}

/***********************************************************************
 *      IsThemeActive                                       (UXTHEME.@)
 */
BOOL WINAPI IsThemeActive(void)
{
    TRACE("\n");
    SetLastError(ERROR_SUCCESS);
    return bThemeActive;
}

/************************************************************
*       IsCompositionActive   (UXTHEME.@)
*/
BOOL WINAPI IsCompositionActive(void)
{
    FIXME(": stub\n");

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *      EnableTheming                                       (UXTHEME.@)
 *
 * NOTES
 * This is a global and persistent change
 */
HRESULT WINAPI EnableTheming(BOOL fEnable)
{
    HKEY hKey;

    TRACE("(%d)\n", fEnable);

    if (bThemeActive && !fEnable)
    {
        bThemeActive = fEnable;
        if(!RegOpenKeyW(HKEY_CURRENT_USER, szThemeManager, &hKey)) {
            RegSetValueExW(hKey, L"ThemeActive", 0, REG_SZ, (BYTE *)L"0", 2 * sizeof(WCHAR));
            RegCloseKey(hKey);
        }
	UXTHEME_broadcast_msg (NULL, WM_THEMECHANGED);
    }
    return S_OK;
}

/***********************************************************************
 *      UXTHEME_SetWindowProperty
 *
 * I'm using atoms as there may be large numbers of duplicated strings
 * and they do the work of keeping memory down as a cause of that quite nicely
 */
static HRESULT UXTHEME_SetWindowProperty(HWND hwnd, ATOM aProp, LPCWSTR pszValue)
{
    ATOM oldValue = (ATOM)(size_t)RemovePropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp));
    if(oldValue)
        DeleteAtom(oldValue);
    if(pszValue) {
        ATOM atValue = AddAtomW(pszValue);
        if(!atValue
           || !SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp), (LPWSTR)MAKEINTATOM(atValue))) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            if(atValue) DeleteAtom(atValue);
            return hr;
        }
    }
    return S_OK;
}

static LPWSTR UXTHEME_GetWindowProperty(HWND hwnd, ATOM aProp, LPWSTR pszBuffer, int dwLen)
{
    ATOM atValue = (ATOM)(size_t)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(aProp));
    if(atValue) {
        if(GetAtomNameW(atValue, pszBuffer, dwLen))
            return pszBuffer;
        TRACE("property defined, but unable to get value\n");
    }
    return NULL;
}

static HTHEME open_theme_data(HWND hwnd, LPCWSTR pszClassList, DWORD flags, UINT dpi)
{
    WCHAR szAppBuff[256];
    WCHAR szClassBuff[256];
    LPCWSTR pszAppName;
    LPCWSTR pszUseClassList;
    HTHEME hTheme = NULL;
    TRACE("(%p,%s, %lx)\n", hwnd, debugstr_w(pszClassList), flags);

    if(!pszClassList)
    {
        SetLastError(E_POINTER);
        return NULL;
    }

    if(flags)
        FIXME("unhandled flags: %lx\n", flags);

    if(bThemeActive)
    {
        pszAppName = UXTHEME_GetWindowProperty(hwnd, atSubAppName, szAppBuff, ARRAY_SIZE(szAppBuff));
        /* If SetWindowTheme was used on the window, that overrides the class list passed to this function */
        pszUseClassList = UXTHEME_GetWindowProperty(hwnd, atSubIdList, szClassBuff, ARRAY_SIZE(szClassBuff));
        if(!pszUseClassList)
            pszUseClassList = pszClassList;

        if (pszUseClassList)
            hTheme = MSSTYLES_OpenThemeClass(pszAppName, pszUseClassList, dpi);
    }
    if(IsWindow(hwnd))
        SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme), hTheme);
    TRACE(" = %p\n", hTheme);

    SetLastError(hTheme ? ERROR_SUCCESS : E_PROP_ID_UNSUPPORTED);
    return hTheme;
}

/***********************************************************************
 *      OpenThemeDataEx                                     (UXTHEME.61)
 */
HTHEME WINAPI OpenThemeDataEx(HWND hwnd, LPCWSTR pszClassList, DWORD flags)
{
    UINT dpi;

    dpi = GetDpiForWindow(hwnd);
    if (!dpi)
        dpi = GetDpiForSystem();

    return open_theme_data(hwnd, pszClassList, flags, dpi);
}

/***********************************************************************
 *      OpenThemeDataForDpi                                 (UXTHEME.@)
 */
HTHEME WINAPI OpenThemeDataForDpi(HWND hwnd, LPCWSTR class_list, UINT dpi)
{
    return open_theme_data(hwnd, class_list, 0, dpi);
}

/***********************************************************************
 *      OpenThemeData                                       (UXTHEME.@)
 */
HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR classlist)
{
    return OpenThemeDataEx(hwnd, classlist, 0);
}

/***********************************************************************
 *      GetWindowTheme                                      (UXTHEME.@)
 *
 * Retrieve the last theme opened for a window.
 *
 * PARAMS
 *  hwnd  [I] window to retrieve the theme for
 *
 * RETURNS
 *  The most recent theme.
 */
HTHEME WINAPI GetWindowTheme(HWND hwnd)
{
    TRACE("(%p)\n", hwnd);

    if (!hwnd)
    {
        SetLastError(E_HANDLE);
        return NULL;
    }

    return GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme));
}

/***********************************************************************
 *      SetWindowTheme                                      (UXTHEME.@)
 *
 * Persistent through the life of the window, even after themes change
 */
HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName,
                              LPCWSTR pszSubIdList)
{
    HRESULT hr;
    TRACE("(%p,%s,%s)\n", hwnd, debugstr_w(pszSubAppName),
          debugstr_w(pszSubIdList));

    if (!hwnd)
        return E_HANDLE;

    hr = UXTHEME_SetWindowProperty(hwnd, atSubAppName, pszSubAppName);
    if(SUCCEEDED(hr))
        hr = UXTHEME_SetWindowProperty(hwnd, atSubIdList, pszSubIdList);
    if(SUCCEEDED(hr))
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    return hr;
}

/***********************************************************************
 *      SetWindowThemeAttribute                             (UXTHEME.@)
 */
HRESULT WINAPI SetWindowThemeAttribute(HWND hwnd, enum WINDOWTHEMEATTRIBUTETYPE type,
                                       PVOID attribute, DWORD size)
{
   FIXME("(%p,%d,%p,%ld): stub\n", hwnd, type, attribute, size);
   return E_NOTIMPL;
}

/***********************************************************************
 *      GetCurrentThemeName                                 (UXTHEME.@)
 */
HRESULT WINAPI GetCurrentThemeName(LPWSTR pszThemeFileName, int dwMaxNameChars,
                                   LPWSTR pszColorBuff, int cchMaxColorChars,
                                   LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
    if(!bThemeActive)
        return E_PROP_ID_UNSUPPORTED;
    if(pszThemeFileName) lstrcpynW(pszThemeFileName, szCurrentTheme, dwMaxNameChars);
    if(pszColorBuff) lstrcpynW(pszColorBuff, szCurrentColor, cchMaxColorChars);
    if(pszSizeBuff) lstrcpynW(pszSizeBuff, szCurrentSize, cchMaxSizeChars);
    return S_OK;
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
    TRACE("(%p)\n", hTheme);
    if(!hTheme || hTheme == INVALID_HANDLE_VALUE)
        return E_HANDLE;
    return MSSTYLES_CloseThemeClass(hTheme);
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
    return E_NOTIMPL;
}

/***********************************************************************
 *      IsThemePartDefined                                  (UXTHEME.@)
 */
BOOL WINAPI IsThemePartDefined(HTHEME hTheme, int iPartId, int iStateId)
{
    TRACE("(%p,%d,%d)\n", hTheme, iPartId, iStateId);
    if(!hTheme) {
        SetLastError(E_HANDLE);
        return FALSE;
    }

    SetLastError(NO_ERROR);
    return !iStateId && MSSTYLES_FindPart(hTheme, iPartId);
}

/***********************************************************************
 *      GetThemeDocumentationProperty                       (UXTHEME.@)
 *
 * Try and retrieve the documentation property from string resources
 * if that fails, get it from the [documentation] section of themes.ini
 */
HRESULT WINAPI GetThemeDocumentationProperty(LPCWSTR pszThemeName,
                                             LPCWSTR pszPropertyName,
                                             LPWSTR pszValueBuff,
                                             int cchMaxValChars)
{
    static const WORD wDocToRes[] = {
        TMT_DISPLAYNAME,5000,
        TMT_TOOLTIP,5001,
        TMT_COMPANY,5002,
        TMT_AUTHOR,5003,
        TMT_COPYRIGHT,5004,
        TMT_URL,5005,
        TMT_VERSION,5006,
        TMT_DESCRIPTION,5007
    };

    PTHEME_FILE pt;
    HRESULT hr;
    unsigned int i;
    int iDocId;
    TRACE("(%s,%s,%p,%d)\n", debugstr_w(pszThemeName), debugstr_w(pszPropertyName),
          pszValueBuff, cchMaxValChars);

    hr = MSSTYLES_OpenThemeFile(pszThemeName, NULL, NULL, &pt);
    if(FAILED(hr)) return hr;

    /* Try to load from string resources */
    hr = E_PROP_ID_UNSUPPORTED;
    if(MSSTYLES_LookupProperty(pszPropertyName, NULL, &iDocId)) {
        for(i=0; i<ARRAY_SIZE(wDocToRes); i+=2) {
            if(wDocToRes[i] == iDocId) {
                if(LoadStringW(pt->hTheme, wDocToRes[i+1], pszValueBuff, cchMaxValChars)) {
                    hr = S_OK;
                    break;
                }
            }
        }
    }
    /* If loading from string resource failed, try getting it from the theme.ini */
    if(FAILED(hr)) {
        PUXINI_FILE uf = MSSTYLES_GetThemeIni(pt);
        if(UXINI_FindSection(uf, L"documentation")) {
            LPCWSTR lpValue;
            DWORD dwLen;
            if(UXINI_FindValue(uf, pszPropertyName, &lpValue, &dwLen)) {
                lstrcpynW(pszValueBuff, lpValue, min(dwLen+1,cchMaxValChars));
                hr = S_OK;
            }
        }
        UXINI_CloseINI(uf);
    }

    MSSTYLES_CloseThemeFile(pt);
    return hr;
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
DWORD WINAPI QueryThemeServices(void)
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
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE *hThemeFile,
                             DWORD unknown)
{
    TRACE("(%s,%s,%s,%p,%ld)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszColorName), debugstr_w(pszSizeName),
          hThemeFile, unknown);
    return MSSTYLES_OpenThemeFile(pszThemeFileName, pszColorName, pszSizeName, (PTHEME_FILE*)hThemeFile);
}

/**********************************************************************
 *      CloseThemeFile                                     (UXTHEME.3)
 *
 * Releases theme file handle returned by OpenThemeFile
 *
 * PARAMS
 *     hThemeFile           Handle to theme file
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile)
{
    TRACE("(%p)\n", hThemeFile);
    MSSTYLES_CloseThemeFile(hThemeFile);
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
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 *
 * NOTES
 * I'm not sure what the second parameter is (the datatype is likely wrong), other then this:
 * Under XP if I pass
 * char b[] = "";
 *   the theme is applied with the screen redrawing really badly (flickers)
 * char b[] = "\0"; where \0 can be one or more of any character, makes no difference
 *   the theme is applied smoothly (screen does not flicker)
 * char *b = "\0" or NULL; where \0 can be zero or more of any character, makes no difference
 *   the function fails returning invalid parameter... very strange
 */
HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, char *unknown, HWND hWnd)
{
    HRESULT hr;
    TRACE("(%p,%s,%p)\n", hThemeFile, unknown, hWnd);
    hr = UXTHEME_SetActiveTheme(hThemeFile);
    UXTHEME_broadcast_msg (NULL, WM_THEMECHANGED);
    return hr;
}

/**********************************************************************
 *      GetThemeDefaults                                   (UXTHEME.7)
 *
 * Get the default color & size for a theme
 *
 * PARAMS
 *     pszThemeFileName    Path to a msstyles theme file
 *     pszColorName        Buffer to receive the default color name
 *     dwColorNameLen      Length, in characters, of color name buffer
 *     pszSizeName         Buffer to receive the default size name
 *     dwSizeNameLen       Length, in characters, of size name buffer
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen)
{
    PTHEME_FILE pt;
    HRESULT hr;
    TRACE("(%s,%p,%ld,%p,%ld)\n", debugstr_w(pszThemeFileName),
          pszColorName, dwColorNameLen,
          pszSizeName, dwSizeNameLen);

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, NULL, &pt);
    if(FAILED(hr)) return hr;

    lstrcpynW(pszColorName, pt->pszSelectedColor, dwColorNameLen);
    lstrcpynW(pszSizeName, pt->pszSelectedSize, dwSizeNameLen);

    MSSTYLES_CloseThemeFile(pt);
    return S_OK;
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
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, EnumThemeProc callback,
                          LPVOID lpData)
{
    WCHAR szDir[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    WCHAR szName[60];
    WCHAR szTip[60];
    HANDLE hFind;
    WIN32_FIND_DATAW wfd;
    HRESULT hr;
    size_t pathLen;

    TRACE("(%s,%p,%p)\n", debugstr_w(pszThemePath), callback, lpData);

    if(!pszThemePath || !callback)
        return E_POINTER;

    lstrcpyW(szDir, pszThemePath);
    pathLen = lstrlenW (szDir);
    if ((pathLen > 0) && (pathLen < MAX_PATH-1) && (szDir[pathLen - 1] != '\\'))
    {
        szDir[pathLen] = '\\';
        szDir[pathLen+1] = 0;
    }

    lstrcpyW(szPath, szDir);
    lstrcatW(szPath, L"*.*");
    TRACE("searching %s\n", debugstr_w(szPath));

    hFind = FindFirstFileW(szPath, &wfd);
    if(hFind != INVALID_HANDLE_VALUE) {
        do {
            if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
               && !(wfd.cFileName[0] == '.' && ((wfd.cFileName[1] == '.' && wfd.cFileName[2] == 0) || wfd.cFileName[1] == 0))) {
                wsprintfW(szPath, L"%s%s\\%s.msstyles", szDir, wfd.cFileName, wfd.cFileName);

                hr = GetThemeDocumentationProperty(szPath, L"displayname", szName, ARRAY_SIZE(szName));
                if(SUCCEEDED(hr))
                    hr = GetThemeDocumentationProperty(szPath, L"tooltip", szTip, ARRAY_SIZE(szTip));
                if(SUCCEEDED(hr)) {
                    TRACE("callback(%s,%s,%s,%p)\n", debugstr_w(szPath), debugstr_w(szName), debugstr_w(szTip), lpData);
                    if(!callback(NULL, szPath, szName, szTip, NULL, lpData)) {
                        TRACE("callback ended enum\n");
                        break;
                    }
                }
            }
        } while(FindNextFileW(hFind, &wfd));
        FindClose(hFind);
    }
    return S_OK;
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
 *     pszColorNames       Output color names
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwColorName does not refer to a color
 *          or when pszSizeName does not refer to a valid size
 *
 * NOTES
 * XP fails with E_POINTER when pszColorNames points to a buffer smaller than 
 * sizeof(THEMENAMES).
 *
 * Not very efficient that I'm opening & validating the theme every call, but
 * this is undocumented and almost never called..
 * (and this is how windows works too)
 */
HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, PTHEMENAMES pszColorNames)
{
    PTHEME_FILE pt;
    HRESULT hr;
    LPWSTR tmp;
    UINT resourceId = dwColorNum + 1000;
    TRACE("(%s,%s,%ld)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszSizeName), dwColorNum);

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, pszSizeName, &pt);
    if(FAILED(hr)) return hr;

    tmp = pt->pszAvailColors;
    while(dwColorNum && *tmp) {
        dwColorNum--;
        tmp += lstrlenW(tmp)+1;
    }
    if(!dwColorNum && *tmp) {
        TRACE("%s\n", debugstr_w(tmp));
        lstrcpyW(pszColorNames->szName, tmp);
        LoadStringW(pt->hTheme, resourceId, pszColorNames->szDisplayName,
            ARRAY_SIZE(pszColorNames->szDisplayName));
        LoadStringW(pt->hTheme, resourceId+1000, pszColorNames->szTooltip,
            ARRAY_SIZE(pszColorNames->szTooltip));
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

    MSSTYLES_CloseThemeFile(pt);
    return hr;
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
 *     pszSizeNames        Output size names
 *
 * RETURNS
 *     S_OK on success
 *     E_PROP_ID_UNSUPPORTED when dwSizeName does not refer to a size
 *          or when pszColorName does not refer to a valid color
 *
 * NOTES
 * XP fails with E_POINTER when pszSizeNames points to a buffer smaller than 
 * sizeof(THEMENAMES).
 *
 * Not very efficient that I'm opening & validating the theme every call, but
 * this is undocumented and almost never called..
 * (and this is how windows works too)
 */
HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, PTHEMENAMES pszSizeNames)
{
    PTHEME_FILE pt;
    HRESULT hr;
    LPWSTR tmp;
    UINT resourceId = dwSizeNum + 3000;
    TRACE("(%s,%s,%ld)\n", debugstr_w(pszThemeFileName),
          debugstr_w(pszColorName), dwSizeNum);

    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, pszColorName, NULL, &pt);
    if(FAILED(hr)) return hr;

    tmp = pt->pszAvailSizes;
    while(dwSizeNum && *tmp) {
        dwSizeNum--;
        tmp += lstrlenW(tmp)+1;
    }
    if(!dwSizeNum && *tmp) {
        TRACE("%s\n", debugstr_w(tmp));
        lstrcpyW(pszSizeNames->szName, tmp);
        LoadStringW(pt->hTheme, resourceId, pszSizeNames->szDisplayName,
            ARRAY_SIZE(pszSizeNames->szDisplayName));
        LoadStringW(pt->hTheme, resourceId+1000, pszSizeNames->szTooltip,
            ARRAY_SIZE(pszSizeNames->szTooltip));
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

    MSSTYLES_CloseThemeFile(pt);
    return hr;
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
 * When pszUnknown is NULL the callback is never called, the value does not seem to serve
 * any other purpose
 */
HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 ParseThemeIniFileProc callback, LPVOID lpData)
{
    FIXME("%s %s: stub\n", debugstr_w(pszIniFileName), debugstr_w(pszUnknown));
    return E_NOTIMPL;
}

/**********************************************************************
 *      CheckThemeSignature                                (UXTHEME.29)
 *
 * Validates the signature of a theme file
 *
 * PARAMS
 *     pszIniFileName      Path to a theme file
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: HRESULT error-code
 */
HRESULT WINAPI CheckThemeSignature(LPCWSTR pszThemeFileName)
{
    PTHEME_FILE pt;
    HRESULT hr;
    TRACE("(%s)\n", debugstr_w(pszThemeFileName));
    hr = MSSTYLES_OpenThemeFile(pszThemeFileName, NULL, NULL, &pt);
    if(FAILED(hr))
        return hr;
    MSSTYLES_CloseThemeFile(pt);
    return S_OK;
}

BOOL WINAPI ThemeHooksInstall(void)
{
    struct user_api_hook hooks;

    hooks.pDefDlgProc = UXTHEME_DefDlgProc;
    hooks.pNonClientButtonDraw = UXTHEME_NonClientButtonDraw;
    hooks.pScrollBarDraw = UXTHEME_ScrollBarDraw;
    hooks.pScrollBarWndProc = UXTHEME_ScrollbarWndProc;
    return RegisterUserApiHook(&hooks, &user_api);
}

BOOL WINAPI ThemeHooksRemove(void)
{
    UnregisterUserApiHook();
    return TRUE;
}

/**********************************************************************
 *      RefreshImmersiveColorPolicyState                  (UXTHEME.104)
 *
 */
void WINAPI RefreshImmersiveColorPolicyState(void)
{
    FIXME("stub\n");
}

/**********************************************************************
 *      ShouldSystemUseDarkMode                           (UXTHEME.138)
 *
 * RETURNS
 *     Whether or not the system should use dark mode.
 */
BOOLEAN WINAPI ShouldSystemUseDarkMode(void)
{
    DWORD light_theme = TRUE, light_theme_size = sizeof(light_theme);

    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"SystemUsesLightTheme", RRF_RT_REG_DWORD, NULL, &light_theme, &light_theme_size);

    return !light_theme;
}

/**********************************************************************
 *      ShouldAppsUseDarkMode                           (UXTHEME.132)
 *
 * RETURNS
 *     Whether or not apps should use dark mode.
 */
BOOLEAN WINAPI ShouldAppsUseDarkMode(void)
{
    DWORD light_theme = TRUE, light_theme_size = sizeof(light_theme);

    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light_theme, &light_theme_size);

    return !light_theme;
}

/**********************************************************************
 *      AllowDarkModeForWindow                          (UXTHEME.133)
 *
 */
BOOLEAN WINAPI AllowDarkModeForWindow(HWND hwnd, BOOLEAN allow)
{
    FIXME("%p %d: stub\n", hwnd, allow);
    return FALSE;
}

/**********************************************************************
 *      SetPreferredAppMode                             (UXTHEME.135)
 *
 */
int WINAPI SetPreferredAppMode(int app_mode)
{
    FIXME("%d: stub\n", app_mode);
    return 0;
}

/**********************************************************************
 *      FlushMenuThemes                                 (UXTHEME.136)
 *
 */
void WINAPI FlushMenuThemes(void)
{
    FIXME("stub\n");
}

/**********************************************************************
 *      IsDarkModeAllowedForWindow                        (UXTHEME.137)
 *
 */
BOOLEAN WINAPI IsDarkModeAllowedForWindow(HWND hwnd)
{
    FIXME("%p: stub\n", hwnd);
    return FALSE;
}
