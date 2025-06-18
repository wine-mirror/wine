/*
 * Internal uxtheme defines & declarations
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

#ifndef __WINE_UXTHEMEDLL_H
#define __WINE_UXTHEMEDLL_H

#include <wingdi.h>
#include <winuser.h>
#include <uxtheme.h>
#include <msstyles.h>

typedef HANDLE HTHEMEFILE;

/**********************************************************************
 *              EnumThemeProc
 *
 * Callback function for EnumThemes.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     lpReserved          Always 0
 *     pszThemeFileName    Full path to theme msstyles file
 *     pszThemeName        Display name for theme
 *     pszToolTip          Tooltip name for theme
 *     lpReserved2         Always 0
 *     lpData              Value passed through lpData from EnumThemes
 */
typedef BOOL (CALLBACK *EnumThemeProc)(LPVOID lpReserved, LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

/**********************************************************************
 *              ParseThemeIniFileProc
 *
 * Callback function for ParseThemeIniFile.
 *
 * RETURNS
 *     TRUE to continue enumeration, FALSE to stop
 *
 * PARAMS
 *     dwType              Entry type
 *     pszParam1           Use defined by entry type
 *     pszParam2           Use defined by entry type
 *     pszParam3           Use defined by entry type
 *     dwParam             Use defined by entry type
 *     lpData              Value passed through lpData from ParseThemeIniFile
 *
 * NOTES
 * I don't know what the valid entry types are
 */
typedef BOOL (CALLBACK*ParseThemeIniFileProc)(DWORD dwType, LPWSTR pszParam1,
                                              LPWSTR pszParam2, LPWSTR pszParam3,
                                              DWORD dwParam, LPVOID lpData);

/* Structure filled in by EnumThemeColors() and EnumeThemeSizes() with the
 * various strings for a theme color or size. */
typedef struct tagTHEMENAMES
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szTooltip[MAX_PATH+1];
} THEMENAMES, *PTHEMENAMES;

/* Declarations for undocumented functions for use internally */
DWORD WINAPI QueryThemeServices(void);
HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE *hThemeFile,
                             DWORD unknown);
HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile);
HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, char *unknown, HWND hWnd);
HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen);
HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, EnumThemeProc callback,
                          LPVOID lpData);
HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, PTHEMENAMES pszColorNames);
HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, PTHEMENAMES pszColorNames);
HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 ParseThemeIniFileProc callback, LPVOID lpData);
BOOL WINAPI ThemeHooksInstall(void);
BOOL WINAPI ThemeHooksRemove(void);

extern void UXTHEME_InitSystem(HINSTANCE hInst);
extern HRESULT UXTHEME_SetActiveTheme(PTHEME_FILE tf);
extern void UXTHEME_UninitSystem(void);

extern struct user_api_hook user_api;
LRESULT WINAPI UXTHEME_DefDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL unicode);
void WINAPI UXTHEME_NonClientButtonDraw(HWND hwnd, HDC hdc, enum NONCLIENT_BUTTON_TYPE type,
                                        RECT rect, BOOL down, BOOL grayed);
void WINAPI UXTHEME_ScrollBarDraw(HWND hwnd, HDC dc, INT bar, enum SCROLL_HITTEST hit_test,
                                  const struct SCROLL_TRACKING_INFO *tracking_info,
                                  BOOL draw_arrows, BOOL draw_interior, RECT *rect, UINT enable_flags,
                                  INT arrowsize, INT thumbpos, INT thumbsize, BOOL vertical);
LRESULT WINAPI UXTHEME_ScrollbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                        BOOL unicode);

/* No alpha blending */
#define ALPHABLEND_NONE             0
/* "Cheap" binary alpha blending - but possibly faster */
#define ALPHABLEND_BINARY           1
/* Full alpha blending */
#define ALPHABLEND_FULL             2

#endif /* __WINE_UXTHEMEDLL_H */
