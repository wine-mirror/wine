/*
 * Win32 5.1 msstyles theme format
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
#include "msstyles.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

extern HINSTANCE hDllInst;

#define MSSTYLES_VERSION 0x0003

static const WCHAR szThemesIniResource[] = {
    't','h','e','m','e','s','_','i','n','i','\0'
};

PTHEME_FILE tfActiveTheme = NULL;

/***********************************************************************/

/**********************************************************************
 *      MSSTYLES_OpenThemeFile
 *
 * Load and validate a theme
 *
 * PARAMS
 *     lpThemeFile         Path to theme file to load
 *     pszColorName        Color name wanted, can be NULL
 *     pszSizeName         Size name wanted, can be NULL
 *
 * NOTES
 * If pszColorName or pszSizeName are NULL, the default color/size will be used.
 * If one/both are provided, they are validated against valid color/sizes and if
 * a match is not found, the function fails.
 */
HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf)
{
    HMODULE hTheme;
    HRSRC hrsc;
    HRESULT hr = S_OK;
    WCHAR szPackThemVersionResource[] = {
        'P','A','C','K','T','H','E','M','_','V','E','R','S','I','O','N', '\0'
    };
    WCHAR szColorNamesResource[] = {
        'C','O','L','O','R','N','A','M','E','S','\0'
    };
    WCHAR szSizeNamesResource[] = {
        'S','I','Z','E','N','A','M','E','S','\0'
    };

    WORD version;
    DWORD versize;
    LPWSTR pszColors;
    LPWSTR pszSelectedColor = NULL;
    LPWSTR pszSizes;
    LPWSTR pszSelectedSize = NULL;
    LPWSTR tmp;

    TRACE("Opening %s\n", debugstr_w(lpThemeFile));

    hTheme = LoadLibraryExW(lpThemeFile, NULL, LOAD_LIBRARY_AS_DATAFILE);

    /* Validate that this is really a theme */
    if(!hTheme) goto invalid_theme;
    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), szPackThemVersionResource))) {
        TRACE("No version resource found\n");
        goto invalid_theme;
    }
    if((versize = SizeofResource(hTheme, hrsc)) != 2)
    {
        TRACE("Version resource found, but wrong size: %ld\n", versize);
        goto invalid_theme;
    }
    version = *(WORD*)LoadResource(hTheme, hrsc);
    if(version != MSSTYLES_VERSION)
    {
        TRACE("Version of theme file is unsupported: 0x%04x\n", version);
        goto invalid_theme;
    }

    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), szColorNamesResource))) {
        TRACE("Color names resource not found\n");
        goto invalid_theme;
    }
    pszColors = (LPWSTR)LoadResource(hTheme, hrsc);

    if(!(hrsc = FindResourceW(hTheme, MAKEINTRESOURCEW(1), szSizeNamesResource))) {
        TRACE("Size names resource not found\n");
        goto invalid_theme;
    }
    pszSizes = (LPWSTR)LoadResource(hTheme, hrsc);

    /* Validate requested color against whats available from the theme */
    if(pszColorName) {
        tmp = pszColors;
        while(*tmp) {
            if(!lstrcmpiW(pszColorName, tmp)) {
                pszSelectedColor = tmp;
                break;
            }
            tmp += lstrlenW(tmp)+1;
        }
    }
    else
        pszSelectedColor = pszColors; /* Use the default color */

    /* Validate requested size against whats available from the theme */
    if(pszSizeName) {
        tmp = pszSizes;
        while(*tmp) {
            if(!lstrcmpiW(pszSizeName, tmp)) {
                pszSelectedSize = tmp;
                break;
            }
            tmp += lstrlenW(tmp)+1;
        }
    }
    else
        pszSelectedSize = pszSizes; /* Use the default size */

    if(!pszSelectedColor || !pszSelectedSize) {
        TRACE("Requested color/size (%s/%s) not found in theme\n",
              debugstr_w(pszColorName), debugstr_w(pszSizeName));
        hr = E_PROP_ID_UNSUPPORTED;
        goto invalid_theme;
    }

    *tf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THEME_FILE));
    (*tf)->hTheme = hTheme;
    (*tf)->pszAvailColors = pszColors;
    (*tf)->pszAvailSizes = pszSizes;
    (*tf)->pszSelectedColor = pszSelectedColor;
    (*tf)->pszSelectedSize = pszSelectedSize;
    (*tf)->dwRefCount = 1;
    return S_OK;

invalid_theme:
    if(hTheme) FreeLibrary(hTheme);
    if(!hr) hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

/***********************************************************************
 *      MSSTYLES_CloseThemeFile
 *
 * Close theme file and free resources
 */
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf)
{
    if(tf) {
        tf->dwRefCount--;
        if(!tf->dwRefCount) {
            if(tf->hTheme) FreeLibrary(tf->hTheme);
            HeapFree(GetProcessHeap(), 0, tf);
        }
    }
}

/***********************************************************************
 *      MSSTYLES_SetActiveTheme
 *
 * Set the current active theme
 */
HRESULT MSSTYLES_SetActiveTheme(PTHEME_FILE tf)
{
    if(tfActiveTheme)
        MSSTYLES_CloseThemeFile(tfActiveTheme);
    tfActiveTheme = tf;
    tfActiveTheme->dwRefCount++;
    FIXME("Process theme resources\n");
    return S_OK;
}

/***********************************************************************
 *      MSSTYLES_GetThemeIni
 *
 * Retrieves themes.ini from a theme
 */
PUXINI_FILE MSSTYLES_GetThemeIni(PTHEME_FILE tf)
{
    return UXINI_LoadINI(tf, szThemesIniResource);
}

/***********************************************************************
 *      MSSTYLES_OpenThemeClass
 *
 * Open a theme class
 *
 * PARAMS
 *     tf                  Previously opened theme file
 *     pszClassList        List of requested classes, semicolon delimited
 */
PTHEME_CLASS MSSTYLES_OpenThemeClass(LPCWSTR pszClassList)
{
    FIXME("%s\n", debugstr_w(pszClassList));
    return NULL;
}

HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc)
{
    FIXME("%p\n", tc);
    return S_OK;
}
