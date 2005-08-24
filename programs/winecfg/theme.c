/*
 * Theme configuration code
 *
 * Copyright (c) 2005 by Frank Richter
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
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <shlobj.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/* UXTHEME functions not in the headers */
typedef void* HTHEMEFILE;
typedef BOOL (CALLBACK *EnumThemeProc)(LPVOID lpReserved, 
				       LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, 
				       LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

HRESULT WINAPI EnumThemeColors (LPWSTR pszThemeFileName, LPWSTR pszSizeName,
				DWORD dwColorNum, LPWSTR pszColorName);
HRESULT WINAPI EnumThemeSizes (LPWSTR pszThemeFileName, LPWSTR pszColorName,
			       DWORD dwSizeNum, LPWSTR pszSizeName);
HRESULT WINAPI ApplyTheme (HTHEMEFILE hThemeFile, char* unknown, HWND hWnd);
HRESULT WINAPI OpenThemeFile (LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
			      LPCWSTR pszSizeName, HTHEMEFILE* hThemeFile,
			      DWORD unknown);
HRESULT WINAPI CloseThemeFile (HTHEMEFILE hThemeFile);
HRESULT WINAPI EnumThemes (LPCWSTR pszThemePath, EnumThemeProc callback,
                           LPVOID lpData);

/* A struct to keep both the internal and "fancy" name of a color or size */
typedef struct
{
  WCHAR* name;
  WCHAR* fancyName;
} ThemeColorOrSize;

/* wrapper around DSA that also keeps an item count */
typedef struct
{
  HDSA dsa;
  int count;
} WrappedDsa;

/* Some helper functions to deal with ThemeColorOrSize structs in WrappedDSAs */

static void color_or_size_dsa_add (WrappedDsa* wdsa, const WCHAR* name,
				   const WCHAR* fancyName)
{
    ThemeColorOrSize item;
    
    item.name = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (name) + 1) * sizeof(WCHAR));
    lstrcpyW (item.name, name);

    item.fancyName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (fancyName) + 1) * sizeof(WCHAR));
    lstrcpyW (item.fancyName, fancyName);

    DSA_InsertItem (wdsa->dsa, wdsa->count, &item);
    wdsa->count++;
}

static int CALLBACK dsa_destroy_callback (LPVOID p, LPVOID pData)
{
    ThemeColorOrSize* item = (ThemeColorOrSize*)p;
    HeapFree (GetProcessHeap(), 0, item->name);
    HeapFree (GetProcessHeap(), 0, item->fancyName);
    return 1;
}

static void free_color_or_size_dsa (WrappedDsa* wdsa)
{
    DSA_DestroyCallback (wdsa->dsa, dsa_destroy_callback, NULL);
}

static void create_color_or_size_dsa (WrappedDsa* wdsa)
{
    wdsa->dsa = DSA_Create (sizeof (ThemeColorOrSize), 1);
    wdsa->count = 0;
}

static ThemeColorOrSize* color_or_size_dsa_get (WrappedDsa* wdsa, int index)
{
    return (ThemeColorOrSize*)DSA_GetItemPtr (wdsa->dsa, index);
}

static int color_or_size_dsa_find (WrappedDsa* wdsa, const WCHAR* name)
{
    int i = 0;
    for (; i < wdsa->count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (wdsa, i);
	if (lstrcmpiW (item->name, name) == 0) break;
    }
    return i;
}

/* A theme file, contains file name, display name, color and size scheme names */
typedef struct
{
    WCHAR* themeFileName;
    WCHAR* fancyName;
    WrappedDsa colors;
    WrappedDsa sizes;
} ThemeFile;

static HDSA themeFiles = NULL;
static int themeFilesCount = 0;

static int CALLBACK theme_dsa_destroy_callback (LPVOID p, LPVOID pData)
{
    ThemeFile* item = (ThemeFile*)p;
    HeapFree (GetProcessHeap(), 0, item->themeFileName);
    HeapFree (GetProcessHeap(), 0, item->fancyName);
    free_color_or_size_dsa (&item->colors);
    free_color_or_size_dsa (&item->sizes);
    return 1;
}

/* Free memory occupied by the theme list */
static void free_theme_files(void)
{
    if (themeFiles == NULL) return;
      
    DSA_DestroyCallback (themeFiles , theme_dsa_destroy_callback, NULL);
    themeFiles = NULL;
    themeFilesCount = 0;
}

typedef HRESULT (WINAPI * EnumTheme) (LPWSTR, LPWSTR, DWORD, LPWSTR);

/* fill a string list with either colors or sizes of a theme */
static void fill_theme_string_array (const WCHAR* filename, 
				     WrappedDsa* wdsa,
				     EnumTheme enumTheme)
{
    DWORD index = 0;
    WCHAR name[MAX_PATH];

    WINE_TRACE ("%s %p %p\n", wine_dbgstr_w (filename), wdsa, enumTheme);

    while (SUCCEEDED (enumTheme ((WCHAR*)filename, NULL, index++, name)))
    {
	WINE_TRACE ("%s\n", wine_dbgstr_w (name));
	color_or_size_dsa_add (wdsa, name, name);
    }
}

/* Theme enumeration callback, adds theme to theme list */
static BOOL CALLBACK myEnumThemeProc (LPVOID lpReserved, 
				      LPCWSTR pszThemeFileName,
				      LPCWSTR pszThemeName, 
				      LPCWSTR pszToolTip, 
				      LPVOID lpReserved2, LPVOID lpData)
{
    ThemeFile newEntry;

    /* fill size/color lists */
    create_color_or_size_dsa (&newEntry.colors);
    fill_theme_string_array (pszThemeFileName, &newEntry.colors, EnumThemeColors);
    create_color_or_size_dsa (&newEntry.sizes);
    fill_theme_string_array (pszThemeFileName, &newEntry.sizes, EnumThemeSizes);

    newEntry.themeFileName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (pszThemeFileName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.themeFileName, pszThemeFileName);
  
    newEntry.fancyName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (pszThemeName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.fancyName, pszThemeName);
  
    /*list_add_tail (&themeFiles, &newEntry->entry);*/
    DSA_InsertItem (themeFiles, themeFilesCount, &newEntry);
    themeFilesCount++;

    return TRUE;
}

/* Scan for themes */
static void scan_theme_files(void)
{
    static const WCHAR themesSubdir[] = { '\\','T','h','e','m','e','s',0 };
    WCHAR themesPath[MAX_PATH];

    free_theme_files();

    if (FAILED (SHGetFolderPathW (NULL, CSIDL_RESOURCES, NULL, 
        SHGFP_TYPE_CURRENT, themesPath))) return;

    themeFiles = DSA_Create (sizeof (ThemeFile), 1);
    lstrcatW (themesPath, themesSubdir);

    EnumThemes (themesPath, myEnumThemeProc, 0);
}

/* fill the color & size combo boxes for a given theme */
static void fill_color_size_combos (ThemeFile* theme, HWND comboColor, 
                                    HWND comboSize)
{
    int i;

    SendMessageW (comboColor, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < theme->colors.count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (&theme->colors, i);
	SendMessageW (comboColor, CB_ADDSTRING, 0, (LPARAM)item->fancyName);
    }

    SendMessageW (comboSize, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < theme->sizes.count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (&theme->sizes, i);
	SendMessageW (comboSize, CB_ADDSTRING, 0, (LPARAM)item->fancyName);
    }
}

/* Select the item of a combo box that matches a theme's color and size 
 * scheme. */
static void select_color_and_size (ThemeFile* theme, 
			    const WCHAR* colorName, HWND comboColor, 
			    const WCHAR* sizeName, HWND comboSize)
{
    SendMessageW (comboColor, CB_SETCURSEL, 
	color_or_size_dsa_find (&theme->colors, colorName), 0);
    SendMessageW (comboSize, CB_SETCURSEL, 
	color_or_size_dsa_find (&theme->sizes, sizeName), 0);
}

/* Fill theme, color and sizes combo boxes with the know themes and select
 * the entries matching the currently active theme. */
static BOOL fill_theme_list (HWND comboTheme, HWND comboColor, HWND comboSize)
{
    WCHAR textNoTheme[256];
    int themeIndex = 0;
    BOOL ret = TRUE;
    int i;
    WCHAR currentTheme[MAX_PATH];
    WCHAR currentColor[MAX_PATH];
    WCHAR currentSize[MAX_PATH];
    ThemeFile* theme = NULL;

    LoadStringW (GetModuleHandle (NULL), IDS_NOTHEME, textNoTheme,
	sizeof(textNoTheme) / sizeof(WCHAR));

    SendMessageW (comboTheme, CB_RESETCONTENT, 0, 0);
    SendMessageW (comboTheme, CB_ADDSTRING, 0, (LPARAM)textNoTheme);

    for (i = 0; i < themeFilesCount; i++)
    {
	ThemeFile* item = (ThemeFile*)DSA_GetItemPtr (themeFiles, i);
	SendMessageW (comboTheme, CB_ADDSTRING, 0, 
	    (LPARAM)item->fancyName);
    }
  
    if (IsThemeActive () && SUCCEEDED (GetCurrentThemeName (currentTheme, 
	    sizeof(currentTheme) / sizeof(WCHAR),
	    currentColor, sizeof(currentColor) / sizeof(WCHAR),
	    currentSize, sizeof(currentSize) / sizeof(WCHAR))))
    {
	/* Determine the index of the currently active theme. */
	BOOL found = FALSE;
	for (i = 0; i < themeFilesCount; i++)
	{
	    theme = (ThemeFile*)DSA_GetItemPtr (themeFiles, i);
	    if (lstrcmpiW (theme->themeFileName, currentTheme) == 0)
	    {
		found = TRUE;
		themeIndex = i+1;
		break;
	    }
	}
	if (!found)
	{
	    /* Current theme not found?... add to the list, then... */
	    WINE_TRACE("Theme %s not in list of enumerated themes",
		wine_dbgstr_w (currentTheme));
	    myEnumThemeProc (NULL, currentTheme, currentTheme, 
		currentTheme, NULL, NULL);
	    themeIndex = themeFilesCount;
	    theme = (ThemeFile*)DSA_GetItemPtr (themeFiles, 
		themeFilesCount-1);
	}
	fill_color_size_combos (theme, comboColor, comboSize);
	select_color_and_size (theme, currentColor, comboColor,
	    currentSize, comboSize);
    }
    else
    {
	/* No theme selected */
	ret = FALSE;
    }

    SendMessageW (comboTheme, CB_SETCURSEL, themeIndex, 0);
    return ret;
}

/* Update the color & size combo boxes when the selection of the theme
 * combo changed. Selects the current color and size scheme if the theme
 * is currently active, otherwise the first color and size. */
BOOL THEME_update_color_and_size (int themeIndex, HWND comboColor, 
				  HWND comboSize)
{
    if (themeIndex == 0)
    {
	return FALSE;
    }
    else
    {
	WCHAR currentTheme[MAX_PATH];
	WCHAR currentColor[MAX_PATH];
	WCHAR currentSize[MAX_PATH];
	ThemeFile* theme = 
	    (ThemeFile*)DSA_GetItemPtr (themeFiles, themeIndex - 1);
    
	fill_color_size_combos (theme, comboColor, comboSize);
      
	if ((SUCCEEDED (GetCurrentThemeName (currentTheme, 
	    sizeof(currentTheme) / sizeof(WCHAR),
	    currentColor, sizeof(currentColor) / sizeof(WCHAR),
	    currentSize, sizeof(currentSize) / sizeof(WCHAR))))
	    && (lstrcmpiW (currentTheme, theme->themeFileName) == 0))
	{
	    select_color_and_size (theme, currentColor, comboColor,
		currentSize, comboSize);
	}
	else
	{
	    SendMessageW (comboColor, CB_SETCURSEL, 0, 0);
	    SendMessageW (comboSize, CB_SETCURSEL, 0, 0);
	}
    }
    return TRUE;
}

/* Apply a theme from a given theme, color and size combo box item index. */
static void do_apply_theme (int themeIndex, int colorIndex, int sizeIndex)
{
    static char b[] = "\0";

    if (themeIndex == 0)
    {
	/* no theme */
	ApplyTheme (NULL, b, NULL);
    }
    else
    {
	ThemeFile* theme = 
	    (ThemeFile*)DSA_GetItemPtr (themeFiles, themeIndex-1);
	const WCHAR* themeFileName = theme->themeFileName;
	const WCHAR* colorName = NULL;
	const WCHAR* sizeName = NULL;
	HTHEMEFILE hTheme;
	ThemeColorOrSize* item;
    
	item = color_or_size_dsa_get (&theme->colors, colorIndex);
	colorName = item->name;
	
	item = color_or_size_dsa_get (&theme->sizes, sizeIndex);
	sizeName = item->name;
	
	if (SUCCEEDED (OpenThemeFile (themeFileName, colorName, sizeName,
	    &hTheme, 0)))
	{
	    ApplyTheme (hTheme, b, NULL);
	    CloseThemeFile (hTheme);
	}
	else
	{
	    ApplyTheme (NULL, b, NULL);
	}
    }
}

int updating_ui;
BOOL theme_dirty;

static void enable_size_and_color_controls (HWND dialog, BOOL enable)
{
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORTEXT), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZETEXT), enable);
}
  
static void init_dialog (HWND dialog)
{
    updating_ui = TRUE;
    
    scan_theme_files();
    if (!fill_theme_list (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
    {
        SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        enable_size_and_color_controls (dialog, FALSE);
    }
    else
    {
        enable_size_and_color_controls (dialog, TRUE);
    }
    theme_dirty = FALSE;
    
    updating_ui = FALSE;
}

static void on_theme_changed(HWND dialog) {
    int index = SendMessageW (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        CB_GETCURSEL, 0, 0);
    if (!THEME_update_color_and_size (index, GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
    {
        SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        enable_size_and_color_controls (dialog, FALSE);
    }
    else
    {
        enable_size_and_color_controls (dialog, TRUE);
    }
    theme_dirty = TRUE;
}

static void apply_theme(HWND dialog)
{
    int themeIndex, colorIndex, sizeIndex;

    if (!theme_dirty) return;

    themeIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        CB_GETCURSEL, 0, 0);
    colorIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        CB_GETCURSEL, 0, 0);
    sizeIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO),
        CB_GETCURSEL, 0, 0);

    do_apply_theme (themeIndex, colorIndex, sizeIndex);
    theme_dirty = FALSE;
}

INT_PTR CALLBACK
ThemeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            break;
        
        case WM_DESTROY:
            free_theme_files();
            break;

        case WM_SHOWWINDOW:
            set_window_title(hDlg);
            break;
            
        case WM_COMMAND:
            switch(HIWORD(wParam)) {
                case CBN_SELCHANGE: {
                    if (updating_ui) break;
                    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                    switch (LOWORD(wParam))
                    {
                        case IDC_THEME_THEMECOMBO: on_theme_changed(hDlg); break;
                        case IDC_THEME_COLORCOMBO: /* fall through */
                        case IDC_THEME_SIZECOMBO: theme_dirty = TRUE; break;
                    }
                    break;
                }
                    
                default:
                    break;
            }
            break;
        
        
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_KILLACTIVE: {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;
                }
                case PSN_APPLY: {
                    apply();
                    apply_theme(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                }
                case PSN_SETACTIVE: {
                    init_dialog (hDlg);
                    break;
                }
            }
            break;

        default:
            break;
    }
    return FALSE;
}
