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

typedef struct _THEME_FILE {
    HMODULE hTheme;
    LPWSTR pszAvailColors;
    LPWSTR pszAvailSizes;

    LPWSTR pszSelectedColor;
    LPWSTR pszSelectedSize;
} THEME_FILE, *PTHEME_FILE;

HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf);
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf);


#endif
