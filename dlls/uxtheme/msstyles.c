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
#define NO_SHLWAPI_REG
#include "shlwapi.h"
#include "winnls.h"
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
            if(tf->classes) {
                while(tf->classes) {
                    PTHEME_CLASS pcls = tf->classes;
                    tf->classes = pcls->next;
                    while(pcls->partstate) {
                        PTHEME_PARTSTATE ps = pcls->partstate;
                        pcls->partstate = ps->next;
                        HeapFree(GetProcessHeap(), 0, ps);
                    }
                    HeapFree(GetProcessHeap(), 0, pcls);
                }
            }
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
 *      MSSTYLES_GetActiveThemeIni
 *
 * Retrieve the ini file for the selected color/style
 */
PUXINI_FILE MSSTYLES_GetActiveThemeIni(PTHEME_FILE tf)
{
    WCHAR szFileResNamesResource[] = {
        'F','I','L','E','R','E','S','N','A','M','E','S','\0'
    };
    DWORD dwColorCount = 0;
    DWORD dwSizeCount = 0;
    DWORD dwColorNum = 0;
    DWORD dwSizeNum = 0;
    DWORD i;
    DWORD dwResourceIndex;
    LPWSTR tmp;
    HRSRC hrsc;

    /* Count the number of available colors & styles, and determine the index number
       of the color/style we are interested in
    */
    tmp = tf->pszAvailColors;
    while(*tmp) {
        if(!lstrcmpiW(tf->pszSelectedColor, tmp))
            dwColorNum = dwColorCount;
        tmp += lstrlenW(tmp)+1;
        dwColorCount++;
    }
    tmp = tf->pszAvailSizes;
    while(*tmp) {
        if(!lstrcmpiW(tf->pszSelectedSize, tmp))
            dwSizeNum = dwSizeCount;
        tmp += lstrlenW(tmp)+1;
        dwSizeCount++;
    }

    if(!(hrsc = FindResourceW(tf->hTheme, MAKEINTRESOURCEW(1), szFileResNamesResource))) {
        TRACE("FILERESNAMES map not found\n");
        return NULL;
    }
    tmp = (LPWSTR)LoadResource(tf->hTheme, hrsc);
    dwResourceIndex = (dwSizeCount * dwColorNum) + dwSizeNum;
    for(i=0; i < dwResourceIndex; i++) {
        tmp += lstrlenW(tmp)+1;
    }
    return UXINI_LoadINI(tf, tmp);
}


/***********************************************************************
 *      MSSTYLES_ParseIniSectionName
 *
 * Parse an ini section name into its component parts
 * Valid formats are:
 * [classname]
 * [classname(state)]
 * [classname.part]
 * [classname.part(state)]
 * [application::classname]
 * [application::classname(state)]
 * [application::classname.part]
 * [application::classname.part(state)]
 *
 * PARAMS
 *     lpSection           Section name
 *     dwLen               Length of section name
 *     szAppName           Location to store application name
 *     szClassName         Location to store class name
 *     iPartId             Location to store part id
 *     iStateId            Location to store state id
 */
BOOL MSSTYLES_ParseIniSectionName(LPCWSTR lpSection, DWORD dwLen, LPWSTR szAppName, LPWSTR szClassName, int *iPartId, int *iStateId)
{
    WCHAR sec[255];
    WCHAR part[60] = {'\0'};
    WCHAR state[60] = {'\0'};
    LPWSTR tmp;
    LPWSTR comp;
    lstrcpynW(sec, lpSection, min(dwLen+1, sizeof(sec)/sizeof(sec[0])));

    *szAppName = 0;
    *szClassName = 0;
    *iPartId = 0;
    *iStateId = 0;
    comp = sec;
    /* Get the application name */
    tmp = StrChrW(comp, ':');
    if(tmp) {
        *tmp++ = 0;
        tmp++;
        lstrcpynW(szAppName, comp, MAX_THEME_APP_NAME);
        comp = tmp;
    }

    tmp = StrChrW(comp, '.');
    if(tmp) {
        *tmp++ = 0;
        lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
        comp = tmp;
        /* now get the part & state */
        tmp = StrChrW(comp, '(');
        if(tmp) {
            *tmp++ = 0;
            lstrcpynW(part, comp, sizeof(part)/sizeof(part[0]));
            comp = tmp;
            /* now get the state */
            *StrChrW(comp, ')') = 0;
            lstrcpynW(state, comp, sizeof(state)/sizeof(state[0]));
        }
        else {
            lstrcpynW(part, comp, sizeof(part)/sizeof(part[0]));
        }
    }
    else {
        tmp = StrChrW(comp, '(');
        if(tmp) {
            *tmp++ = 0;
            lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
            comp = tmp;
            /* now get the state */
            *StrChrW(comp, ')') = 0;
            lstrcpynW(state, comp, sizeof(state)/sizeof(state[0]));
        }
        else {
            lstrcpynW(szClassName, comp, MAX_THEME_CLASS_NAME);
        }
    }
    if(!*szClassName) return FALSE;
    return MSSTYLES_LookupPartState(szClassName, part[0]?part:NULL, state[0]?state:NULL, iPartId, iStateId);
}

/***********************************************************************
 *      MSSTYLES_FindClass
 *
 * Find a class
 *
 * PARAMS
 *     tf                  Theme file
 *     pszAppName          App name to find
 *     pszClassName        Class name to find
 *
 * RETURNS
 *  The class found, or NULL
 */
PTHEME_CLASS MSSTYLES_FindClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassName)
{
    PTHEME_CLASS cur = tf->classes;
    while(cur) {
        if(!lstrcmpiW(pszAppName, cur->szAppName) && !lstrcmpiW(pszClassName, cur->szClassName))
            return cur;
        cur = cur->next;
    }
    return NULL;
}

/***********************************************************************
 *      MSSTYLES_AddClass
 *
 * Add a class to a theme file
 *
 * PARAMS
 *     tf                  Theme file
 *     pszAppName          App name to add
 *     pszClassName        Class name to add
 *
 * RETURNS
 *  The class added, or a class previously added with the same name
 */
PTHEME_CLASS MSSTYLES_AddClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassName)
{
    PTHEME_CLASS cur = MSSTYLES_FindClass(tf, pszAppName, pszClassName);
    if(cur) return cur;

    cur = HeapAlloc(GetProcessHeap(), 0, sizeof(THEME_CLASS));
    lstrcpyW(cur->szAppName, pszAppName);
    lstrcpyW(cur->szClassName, pszClassName);
    cur->next = tf->classes;
    cur->partstate = NULL;
    cur->overrides = NULL;
    tf->classes = cur;
    return cur;
}

/***********************************************************************
 *      MSSTYLES_FindPartState
 *
 * Find a part/state
 *
 * PARAMS
 *     tc                  Class to search
 *     iPartId             Part ID to find
 *     iStateId            State ID to find
 *
 * RETURNS
 *  The part/state found, or NULL
 */
PTHEME_PARTSTATE MSSTYLES_FindPartState(PTHEME_CLASS tc, int iPartId, int iStateId)
{
    PTHEME_PARTSTATE cur = tc->partstate;
    while(cur) {
        if(cur->iPartId == iPartId && cur->iStateId == iStateId)
            return cur;
        cur = cur->next;
    }
    if(tc->overrides) return MSSTYLES_FindPartState(tc->overrides, iPartId, iStateId);
    return NULL;
}

/***********************************************************************
 *      MSSTYLES_AddPartState
 *
 * Add a part/state to a class
 *
 * PARAMS
 *     tc                  Theme class
 *     iPartId             Part ID to add
 *     iStateId            State ID to add
 *
 * RETURNS
 *  The part/state added, or a part/state previously added with the same IDs
 */
PTHEME_PARTSTATE MSSTYLES_AddPartState(PTHEME_CLASS tc, int iPartId, int iStateId)
{
    PTHEME_PARTSTATE cur = MSSTYLES_FindPartState(tc, iPartId, iStateId);
    if(cur) return cur;

    cur = HeapAlloc(GetProcessHeap(), 0, sizeof(THEME_PARTSTATE));
    cur->iPartId = iPartId;
    cur->iStateId = iStateId;
    cur->next = tc->partstate;
    tc->partstate = cur;
    return cur;
}

/***********************************************************************
 *      MSSTYLES_ParseThemeIni
 *
 * Parse the theme ini for the selected color/style
 *
 * PARAMS
 *     tf                  Theme to parse
 */
void MSSTYLES_ParseThemeIni(PTHEME_FILE tf)
{
    WCHAR szSysMetrics[] = {'S','y','s','M','e','t','r','i','c','s','\0'};
    PTHEME_CLASS cls;
    PTHEME_PARTSTATE ps;
    PUXINI_FILE ini;
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    int iPartId;
    int iStateId;
    DWORD dwLen;
    LPCWSTR lpName;

    ini = MSSTYLES_GetActiveThemeIni(tf);

    while((lpName=UXINI_GetNextSection(ini, &dwLen))) {
        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpName, dwLen, szSysMetrics, -1) == CSTR_EQUAL) {
            FIXME("Process system metrics\n");
            continue;
        }
        if(MSSTYLES_ParseIniSectionName(lpName, dwLen, szAppName, szClassName, &iPartId, &iStateId)) {
            cls = MSSTYLES_AddClass(tf, szAppName, szClassName);
            ps = MSSTYLES_AddPartState(cls, iPartId, iStateId);
            FIXME("Parse properties\n");
        }
    }

    /* App/Class combos override values defined by the base class, map these overrides */
    cls = tf->classes;
    while(cls) {
        if(*cls->szAppName) {
            cls->overrides = MSSTYLES_FindClass(tf, NULL, cls->szClassName);
            if(!cls->overrides) {
                TRACE("No overrides found for app %s class %s\n", debugstr_w(cls->szAppName), debugstr_w(cls->szClassName));
            }
        }
        cls = cls->next;
    }
    UXINI_CloseINI(ini);

    if(!tf->classes) {
        ERR("Failed to parse theme ini\n");
    }
}

/***********************************************************************
 *      MSSTYLES_OpenThemeClass
 *
 * Open a theme class, uses the current active theme
 *
 * PARAMS
 *     pszAppName          Application name, for theme styles specific
 *                         to a particular application
 *     pszClassList        List of requested classes, semicolon delimited
 */
PTHEME_CLASS MSSTYLES_OpenThemeClass(LPCWSTR pszAppName, LPCWSTR pszClassList)
{
    PTHEME_CLASS cls = NULL;
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    LPCWSTR start;
    LPCWSTR end;
    DWORD len;
    if(!tfActiveTheme) {
        TRACE("there is no active theme\n");
        return NULL;
    }
    if(!tfActiveTheme->classes) {
        MSSTYLES_ParseThemeIni(tfActiveTheme);
        if(!tfActiveTheme->classes)
            return NULL;
    }

    start = pszClassList;
    while((end = StrChrW(start, ';'))) {
        len = end-start;
        lstrcpynW(szClassName, start, min(len+1, sizeof(szClassName)/sizeof(szClassName[0])));
        start = end+1;
        cls = MSSTYLES_FindClass(tfActiveTheme, pszAppName, szClassName);
        if(cls) break;
    }
    if(!cls && *start) {
        lstrcpynW(szClassName, start, sizeof(szClassName)/sizeof(szClassName[0]));
        cls = MSSTYLES_FindClass(tfActiveTheme, pszAppName, szClassName);
    }
    if(cls) {
        TRACE("Opened class %s from list %s\n", debugstr_w(cls->szClassName), debugstr_w(pszClassList));
    }
    return cls;
}

/***********************************************************************
 *      MSSTYLES_CloseThemeClass
 *
 * Close a theme class
 *
 * PARAMS
 *     tc                  Theme class to close
 *
 * NOTES
 *  There is currently no need clean anything up for theme classes,
 *  so do nothing for now
 */
HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc)
{
    return S_OK;
}
