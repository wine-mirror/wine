/*
 * Internal msstyles related defines & declarations
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

#ifndef __WINE_MSSTYLES_H
#define __WINE_MSSTYLES_H

#define TMT_ENUM 200

#define MAX_THEME_APP_NAME 60
#define MAX_THEME_CLASS_NAME 60

typedef struct _THEME_PARTSTATE {
    int iPartId;
    int iStateId;
    /* TODO: define part/state properties */

    struct _THEME_PARTSTATE *next;
} THEME_PARTSTATE, *PTHEME_PARTSTATE;

typedef struct _THEME_CLASS {
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    PTHEME_PARTSTATE partstate;
    struct _THEME_CLASS *overrides;

    struct _THEME_CLASS *next;
} THEME_CLASS, *PTHEME_CLASS;

typedef struct _THEME_FILE {
    DWORD dwRefCount;
    HMODULE hTheme;
    LPWSTR pszAvailColors;
    LPWSTR pszAvailSizes;

    LPWSTR pszSelectedColor;
    LPWSTR pszSelectedSize;

    PTHEME_CLASS classes;
} THEME_FILE, *PTHEME_FILE;

typedef struct _UXINI_FILE {
    LPCWSTR lpIni;
    LPCWSTR lpCurLoc;
    LPCWSTR lpEnd;
} UXINI_FILE, *PUXINI_FILE;

HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf);
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf);
HRESULT MSSTYLES_SetActiveTheme(PTHEME_FILE tf);
PTHEME_CLASS MSSTYLES_OpenThemeClass(LPCWSTR pszAppName, LPCWSTR pszClassList);
HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc);
BOOL MSSTYLES_LookupProperty(LPCWSTR pszPropertyName, DWORD *dwPrimitive, DWORD *dwId);
BOOL MSSTYLES_LookupEnum(LPCWSTR pszValueName, DWORD dwEnum, DWORD *dwValue);
BOOL MSSTYLES_LookupPartState(LPCWSTR pszClass, LPCWSTR pszPart, LPCWSTR pszState, int *iPartId, int *iStateId);
PUXINI_FILE MSSTYLES_GetThemeIni(PTHEME_FILE tf);
PTHEME_PARTSTATE MSSTYLES_FindPartState(PTHEME_CLASS tc, int iPartId, int iStateId);
PTHEME_CLASS MSSTYLES_FindClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassName);

PUXINI_FILE UXINI_LoadINI(PTHEME_FILE tf, LPCWSTR lpName);
void UXINI_CloseINI(PUXINI_FILE uf);
void UXINI_ResetINI(PUXINI_FILE uf);
LPCWSTR UXINI_GetNextSection(PUXINI_FILE uf, DWORD *dwLen);
BOOL UXINI_FindSection(PUXINI_FILE uf, LPCWSTR lpName);
LPCWSTR UXINI_GetNextValue(PUXINI_FILE uf, DWORD *dwNameLen, LPCWSTR *lpValue, DWORD *dwValueLen);
BOOL UXINI_FindValue(PUXINI_FILE uf, LPCWSTR lpName, LPCWSTR *lpValue, DWORD *dwValueLen);

#endif
