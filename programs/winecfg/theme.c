/*
 * Desktop Integration
 * - Theme configuration code
 * - User Shell Folder mapping
 *
 * Copyright (c) 2005 by Frank Richter
 * Copyright (c) 2006 by Michael Jung
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
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#define COBJMACROS

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/* UXTHEME functions not in the headers */

typedef struct tagTHEMENAMES
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szTooltip[MAX_PATH+1];
} THEMENAMES, *PTHEMENAMES;

typedef void* HTHEMEFILE;
typedef BOOL (CALLBACK *EnumThemeProc)(LPVOID lpReserved, 
				       LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, 
				       LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

HRESULT WINAPI EnumThemeColors (LPCWSTR pszThemeFileName, LPWSTR pszSizeName,
				DWORD dwColorNum, PTHEMENAMES pszColorNames);
HRESULT WINAPI EnumThemeSizes (LPCWSTR pszThemeFileName, LPWSTR pszColorName,
			       DWORD dwSizeNum, PTHEMENAMES pszSizeNames);
HRESULT WINAPI ApplyTheme (HTHEMEFILE hThemeFile, char* unknown, HWND hWnd);
HRESULT WINAPI OpenThemeFile (LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
			      LPCWSTR pszSizeName, HTHEMEFILE* hThemeFile,
			      DWORD unknown);
HRESULT WINAPI CloseThemeFile (HTHEMEFILE hThemeFile);
HRESULT WINAPI EnumThemes (LPCWSTR pszThemePath, EnumThemeProc callback,
                           LPVOID lpData);

static void refresh_sysparams(HWND hDlg);
static void on_sysparam_change(HWND hDlg);

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

    item.name = malloc ((wcslen (name) + 1) * sizeof(WCHAR));
    lstrcpyW (item.name, name);

    item.fancyName = malloc ((wcslen (fancyName) + 1) * sizeof(WCHAR));
    lstrcpyW (item.fancyName, fancyName);

    DSA_InsertItem (wdsa->dsa, wdsa->count, &item);
    wdsa->count++;
}

static int CALLBACK dsa_destroy_callback (LPVOID p, LPVOID pData)
{
    ThemeColorOrSize* item = p;
    free (item->name);
    free (item->fancyName);
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
    return DSA_GetItemPtr (wdsa->dsa, index);
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
    ThemeFile* item = p;
    free (item->themeFileName);
    free (item->fancyName);
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

typedef HRESULT (WINAPI * EnumTheme) (LPCWSTR, LPWSTR, DWORD, PTHEMENAMES);

/* fill a string list with either colors or sizes of a theme */
static void fill_theme_string_array (const WCHAR* filename, 
				     WrappedDsa* wdsa,
				     EnumTheme enumTheme)
{
    DWORD index = 0;
    THEMENAMES names;

    WINE_TRACE ("%s %p %p\n", wine_dbgstr_w (filename), wdsa, enumTheme);

    while (SUCCEEDED (enumTheme (filename, NULL, index++, &names)))
    {
	WINE_TRACE ("%s: %s\n", wine_dbgstr_w (names.szName), 
            wine_dbgstr_w (names.szDisplayName));
	color_or_size_dsa_add (wdsa, names.szName, names.szDisplayName);
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

    newEntry.themeFileName = malloc ((wcslen (pszThemeFileName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.themeFileName, pszThemeFileName);

    newEntry.fancyName = malloc ((wcslen (pszThemeName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.fancyName, pszThemeName);
  
    /*list_add_tail (&themeFiles, &newEntry->entry);*/
    DSA_InsertItem (themeFiles, themeFilesCount, &newEntry);
    themeFilesCount++;

    return TRUE;
}

/* Scan for themes */
static void scan_theme_files(void)
{
    WCHAR themesPath[MAX_PATH];

    free_theme_files();

    if (FAILED (SHGetFolderPathW (NULL, CSIDL_RESOURCES, NULL, 
        SHGFP_TYPE_CURRENT, themesPath))) return;

    themeFiles = DSA_Create (sizeof (ThemeFile), 1);
    lstrcatW (themesPath, L"\\Themes");

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

    LoadStringW(GetModuleHandleW(NULL), IDS_NOTHEME, textNoTheme, ARRAY_SIZE(textNoTheme));

    SendMessageW (comboTheme, CB_RESETCONTENT, 0, 0);
    SendMessageW (comboTheme, CB_ADDSTRING, 0, (LPARAM)textNoTheme);

    for (i = 0; i < themeFilesCount; i++)
    {
        ThemeFile* item = DSA_GetItemPtr (themeFiles, i);
	SendMessageW (comboTheme, CB_ADDSTRING, 0, 
	    (LPARAM)item->fancyName);
    }

    if (IsThemeActive() && SUCCEEDED(GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme),
                currentColor, ARRAY_SIZE(currentColor), currentSize, ARRAY_SIZE(currentSize))))
    {
	/* Determine the index of the currently active theme. */
	BOOL found = FALSE;
	for (i = 0; i < themeFilesCount; i++)
	{
            theme = DSA_GetItemPtr (themeFiles, i);
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
	    WINE_TRACE("Theme %s not in list of enumerated themes\n",
		wine_dbgstr_w (currentTheme));
	    myEnumThemeProc (NULL, currentTheme, currentTheme, 
		currentTheme, NULL, NULL);
	    themeIndex = themeFilesCount;
            theme = DSA_GetItemPtr (themeFiles, themeFilesCount-1);
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
static BOOL update_color_and_size (int themeIndex, HWND comboColor, 
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
	ThemeFile* theme = DSA_GetItemPtr (themeFiles, themeIndex - 1);
    
	fill_color_size_combos (theme, comboColor, comboSize);

        if ((SUCCEEDED(GetCurrentThemeName (currentTheme, ARRAY_SIZE(currentTheme),
            currentColor, ARRAY_SIZE(currentColor), currentSize, ARRAY_SIZE(currentSize))))
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
static void do_apply_theme (HWND dialog, int themeIndex, int colorIndex, int sizeIndex)
{
    static char b[] = "\0";

    if (themeIndex == 0)
    {
	/* no theme */
	ApplyTheme (NULL, b, NULL);
    }
    else
    {
        ThemeFile* theme = DSA_GetItemPtr (themeFiles, themeIndex-1);
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

    refresh_sysparams(dialog);
}

static BOOL updating_ui;
static BOOL theme_dirty;

static void enable_size_and_color_controls (HWND dialog, BOOL enable)
{
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORTEXT), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZETEXT), enable);
}

static DWORD get_app_theme(void)
{
    DWORD ret = 0, len = sizeof(ret), type;
    HKEY hkey;

    if (RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_QUERY_VALUE, &hkey ))
        return 1;
    if (RegQueryValueExW( hkey, L"AppsUseLightTheme", NULL, &type, (BYTE *)&ret, &len ) || type != REG_DWORD)
        ret = 1;

    RegCloseKey( hkey );
    return ret;
}
  
static void init_dialog (HWND dialog)
{
    DWORD apps_use_light_theme;
    WCHAR apps_theme_str[256];

    static const struct
    {
        int id;
        DWORD value;
    }
    app_themes[] =
    {
        { IDC_THEME_APPCOMBO_LIGHT, 1 },
        { IDC_THEME_APPCOMBO_DARK,  0 },
    };

    SendDlgItemMessageW( dialog, IDC_THEME_APPCOMBO, CB_RESETCONTENT, 0, 0 );

    LoadStringW( GetModuleHandleW(NULL), app_themes[0].id, apps_theme_str, ARRAY_SIZE(apps_theme_str) );
    SendDlgItemMessageW( dialog, IDC_THEME_APPCOMBO, CB_ADDSTRING, 0, (LPARAM)apps_theme_str );
    LoadStringW( GetModuleHandleW(NULL), app_themes[1].id, apps_theme_str, ARRAY_SIZE(apps_theme_str) );
    SendDlgItemMessageW( dialog, IDC_THEME_APPCOMBO, CB_ADDSTRING, 0, (LPARAM)apps_theme_str );

    apps_use_light_theme = get_app_theme();
    SendDlgItemMessageW( dialog, IDC_THEME_APPCOMBO, CB_SETCURSEL, app_themes[apps_use_light_theme].value, 0 );

    SendDlgItemMessageW( dialog, IDC_SYSPARAM_SIZE_UD, UDM_SETBUDDY, (WPARAM)GetDlgItem(dialog, IDC_SYSPARAM_SIZE), 0 );
}

static void update_dialog (HWND dialog)
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

    SendDlgItemMessageW(dialog, IDC_SYSPARAM_SIZE_UD, UDM_SETRANGE, 0, MAKELONG(100, 8));

    updating_ui = FALSE;
}

static void on_theme_changed(HWND dialog) {
    int index;

    index = SendMessageW (GetDlgItem (dialog, IDC_THEME_THEMECOMBO), CB_GETCURSEL, 0, 0);
    if (!update_color_and_size (index, GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
    {
        SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, -1, 0);
        SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, -1, 0);
        enable_size_and_color_controls (dialog, FALSE);
    }
    else
    {
        enable_size_and_color_controls (dialog, TRUE);
    }

    index = SendMessageW (GetDlgItem (dialog, IDC_THEME_APPCOMBO), CB_GETCURSEL, 0, 0);
    set_reg_key_dword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      L"AppsUseLightTheme", !index);
    set_reg_key_dword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      L"SystemUsesLightTheme", !index);

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

    do_apply_theme (dialog, themeIndex, colorIndex, sizeIndex);
    theme_dirty = FALSE;
}

static struct
{
    int sm_idx, color_idx;
    const WCHAR *color_reg;
    int size;
    COLORREF color;
    LOGFONTW lf;
} metrics[] =
{
    {-1,                COLOR_BTNFACE,          L"ButtonFace"    }, /* IDC_SYSPARAMS_BUTTON */
    {-1,                COLOR_BTNTEXT,          L"ButtonText"    }, /* IDC_SYSPARAMS_BUTTON_TEXT */
    {-1,                COLOR_BACKGROUND,       L"Background"    }, /* IDC_SYSPARAMS_DESKTOP */
    {SM_CXMENUSIZE,     COLOR_MENU,             L"Menu"          }, /* IDC_SYSPARAMS_MENU */
    {-1,                COLOR_MENUTEXT,         L"MenuText"      }, /* IDC_SYSPARAMS_MENU_TEXT */
    {SM_CXVSCROLL,      COLOR_SCROLLBAR,        L"Scrollbar"     }, /* IDC_SYSPARAMS_SCROLLBAR */
    {-1,                COLOR_HIGHLIGHT,        L"Hilight"       }, /* IDC_SYSPARAMS_SELECTION */
    {-1,                COLOR_HIGHLIGHTTEXT,    L"HilightText"   }, /* IDC_SYSPARAMS_SELECTION_TEXT */
    {-1,                COLOR_INFOBK,           L"InfoWindow"    }, /* IDC_SYSPARAMS_TOOLTIP */
    {-1,                COLOR_INFOTEXT,         L"InfoText"      }, /* IDC_SYSPARAMS_TOOLTIP_TEXT */
    {-1,                COLOR_WINDOW,           L"Window"        }, /* IDC_SYSPARAMS_WINDOW */
    {-1,                COLOR_WINDOWTEXT,       L"WindowText"    }, /* IDC_SYSPARAMS_WINDOW_TEXT */
    {SM_CYSIZE,         COLOR_ACTIVECAPTION,    L"ActiveTitle"   }, /* IDC_SYSPARAMS_ACTIVE_TITLE */
    {-1,                COLOR_CAPTIONTEXT,      L"TitleText"     }, /* IDC_SYSPARAMS_ACTIVE_TITLE_TEXT */
    {-1,                COLOR_INACTIVECAPTION,  L"InactiveTitle" }, /* IDC_SYSPARAMS_INACTIVE_TITLE */
    {-1,                COLOR_INACTIVECAPTIONTEXT,L"InactiveTitleText" }, /* IDC_SYSPARAMS_INACTIVE_TITLE_TEXT */
    {-1,                -1,                     L"MsgBoxText"    }, /* IDC_SYSPARAMS_MSGBOX_TEXT */
    {-1,                COLOR_APPWORKSPACE,     L"AppWorkSpace"  }, /* IDC_SYSPARAMS_APPWORKSPACE */
    {-1,                COLOR_WINDOWFRAME,      L"WindowFrame"   }, /* IDC_SYSPARAMS_WINDOW_FRAME */
    {-1,                COLOR_ACTIVEBORDER,     L"ActiveBorder"  }, /* IDC_SYSPARAMS_ACTIVE_BORDER */
    {-1,                COLOR_INACTIVEBORDER,   L"InactiveBorder" }, /* IDC_SYSPARAMS_INACTIVE_BORDER */
    {-1,                COLOR_BTNSHADOW,        L"ButtonShadow"  }, /* IDC_SYSPARAMS_BUTTON_SHADOW */
    {-1,                COLOR_GRAYTEXT,         L"GrayText"      }, /* IDC_SYSPARAMS_GRAY_TEXT */
    {-1,                COLOR_BTNHIGHLIGHT,     L"ButtonHilight" }, /* IDC_SYSPARAMS_BUTTON_HIGHLIGHT */
    {-1,                COLOR_3DDKSHADOW,       L"ButtonDkShadow" }, /* IDC_SYSPARAMS_BUTTON_DARK_SHADOW */
    {-1,                COLOR_3DLIGHT,          L"ButtonLight"   }, /* IDC_SYSPARAMS_BUTTON_LIGHT */
    {-1,                COLOR_ALTERNATEBTNFACE, L"ButtonAlternateFace" }, /* IDC_SYSPARAMS_BUTTON_ALTERNATE */
    {-1,                COLOR_HOTLIGHT,         L"HotTrackingColor" }, /* IDC_SYSPARAMS_HOT_TRACKING */
    {-1,                COLOR_GRADIENTACTIVECAPTION, L"GradientActiveTitle" }, /* IDC_SYSPARAMS_ACTIVE_TITLE_GRADIENT */
    {-1,                COLOR_GRADIENTINACTIVECAPTION, L"GradientInactiveTitle" }, /* IDC_SYSPARAMS_INACTIVE_TITLE_GRADIENT */
    {-1,                COLOR_MENUHILIGHT,      L"MenuHilight"   }, /* IDC_SYSPARAMS_MENU_HIGHLIGHT */
    {-1,                COLOR_MENUBAR,          L"MenuBar"       }, /* IDC_SYSPARAMS_MENUBAR */
};

static void save_sys_color(int idx, COLORREF clr)
{
    WCHAR buffer[13];

    swprintf(buffer, ARRAY_SIZE(buffer), L"%d %d %d",  GetRValue (clr), GetGValue (clr), GetBValue (clr));
    set_reg_key(HKEY_CURRENT_USER, L"Control Panel\\Colors", metrics[idx].color_reg, buffer);
}

static void set_color_from_theme(const WCHAR *keyName, COLORREF color)
{
    int i;

    for (i=0; i < ARRAY_SIZE(metrics); i++)
    {
        if (wcsicmp(metrics[i].color_reg, keyName)==0)
        {
            metrics[i].color = color;
            save_sys_color(i, color);
            break;
        }
    }
}

static void do_parse_theme(WCHAR *file)
{
    WCHAR *keyName, keyNameValue[MAX_PATH];
    DWORD len, allocLen = 512;
    WCHAR *keyNamePtr = NULL;
    int red = 0, green = 0, blue = 0;
    COLORREF color;

    WINE_TRACE("%s\n", wine_dbgstr_w(file));
    keyName = malloc(sizeof(*keyName) * allocLen);
    for (;;)
    {
        assert(keyName);
        len = GetPrivateProfileStringW(L"Control Panel\\Colors", NULL, NULL, keyName,
                                       allocLen, file);
        if (len < allocLen - 2)
            break;

        allocLen *= 2;
        keyName = realloc(keyName, sizeof(*keyName) * allocLen);
    }

    keyNamePtr = keyName;
    while (*keyNamePtr!=0) {
        GetPrivateProfileStringW(L"Control Panel\\Colors", keyNamePtr, NULL, keyNameValue,
                                 MAX_PATH, file);

        WINE_TRACE("parsing key: %s with value: %s\n",
                   wine_dbgstr_w(keyNamePtr), wine_dbgstr_w(keyNameValue));

        swscanf(keyNameValue, L"%d %d %d", &red, &green, &blue);

        color = RGB((BYTE)red, (BYTE)green, (BYTE)blue);
        set_color_from_theme(keyNamePtr, color);

        keyNamePtr+=lstrlenW(keyNamePtr);
        keyNamePtr++;
    }
    free(keyName);
}

static void on_theme_install(HWND dialog)
{
  static const WCHAR filterMask[] = L"\0*.msstyles;*.theme\0";
  OPENFILENAMEW ofn;
  WCHAR filetitle[MAX_PATH];
  WCHAR file[MAX_PATH];
  WCHAR filter[100];
  WCHAR title[100];

  LoadStringW(GetModuleHandleW(NULL), IDS_THEMEFILE, filter, ARRAY_SIZE(filter) - ARRAY_SIZE(filterMask));
  memcpy(filter + lstrlenW (filter), filterMask, sizeof(filterMask));
  LoadStringW(GetModuleHandleW(NULL), IDS_THEMEFILE_SELECT, title, ARRAY_SIZE(title));

  ofn.lStructSize = sizeof(OPENFILENAMEW);
  ofn.hwndOwner = dialog;
  ofn.hInstance = 0;
  ofn.lpstrFilter = filter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = file;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = ARRAY_SIZE(file);
  ofn.lpstrFileTitle = filetitle;
  ofn.lpstrFileTitle[0] = '\0';
  ofn.nMaxFileTitle = ARRAY_SIZE(filetitle);
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lCustData = 0;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;

  if (GetOpenFileNameW(&ofn))
  {
      WCHAR themeFilePath[MAX_PATH];
      SHFILEOPSTRUCTW shfop;

      if (FAILED (SHGetFolderPathW (NULL, CSIDL_RESOURCES|CSIDL_FLAG_CREATE, NULL, 
          SHGFP_TYPE_CURRENT, themeFilePath))) return;

      if (lstrcmpiW(PathFindExtensionW(filetitle), L".theme")==0)
      {
          do_parse_theme(file);
          SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
          return;
      }

      PathRemoveExtensionW (filetitle);

      /* Construct path into which the theme file goes */
      lstrcatW (themeFilePath, L"\\themes\\");
      lstrcatW (themeFilePath, filetitle);

      /* Create the directory */
      SHCreateDirectoryExW (dialog, themeFilePath, NULL);

      /* Append theme file name itself */
      lstrcatW (themeFilePath, L"\\");
      lstrcatW (themeFilePath, PathFindFileNameW (file));
      /* SHFileOperation() takes lists as input, so double-nullterminate */
      themeFilePath[lstrlenW (themeFilePath)+1] = 0;
      file[lstrlenW (file)+1] = 0;

      /* Do the copying */
      WINE_TRACE("copying: %s -> %s\n", wine_dbgstr_w (file), 
          wine_dbgstr_w (themeFilePath));
      shfop.hwnd = dialog;
      shfop.wFunc = FO_COPY;
      shfop.pFrom = file;
      shfop.pTo = themeFilePath;
      shfop.fFlags = FOF_NOCONFIRMMKDIR;
      if (SHFileOperationW (&shfop) == 0)
      {
          scan_theme_files();
          if (!fill_theme_list (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
              GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
              GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
          {
              SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, -1, 0);
              SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, -1, 0);
              enable_size_and_color_controls (dialog, FALSE);
          }
          else
          {
              enable_size_and_color_controls (dialog, TRUE);
          }
      }
      else
          WINE_TRACE("copy operation failed\n");
  }
  else WINE_TRACE("user cancelled\n");
}

/* Information about symbolic link targets of certain User Shell Folders. */
struct ShellFolderInfo {
    int nFolder;
    char szLinkTarget[FILENAME_MAX]; /* in unix locale */
};

#define CSIDL_DOWNLOADS 0x0047

static struct ShellFolderInfo asfiInfo[] = {
    { CSIDL_DESKTOP,  "" },
    { CSIDL_PERSONAL, "" },
    { CSIDL_MYPICTURES, "" },
    { CSIDL_MYMUSIC, "" },
    { CSIDL_MYVIDEO, "" },
    { CSIDL_DOWNLOADS, "" },
    { CSIDL_TEMPLATES, "" }
};

static struct ShellFolderInfo *psfiSelected = NULL;

static void init_shell_folder_listview_headers(HWND dialog) {
    LVCOLUMNW listColumn;
    RECT viewRect;
    WCHAR szShellFolder[64] = L"Shell Folder";
    WCHAR szLinksTo[64] = L"Links to";
    int width;

    LoadStringW(GetModuleHandleW(NULL), IDS_SHELL_FOLDER, szShellFolder, ARRAY_SIZE(szShellFolder));
    LoadStringW(GetModuleHandleW(NULL), IDS_LINKS_TO, szLinksTo, ARRAY_SIZE(szLinksTo));

    GetClientRect(GetDlgItem(dialog, IDC_LIST_SFPATHS), &viewRect);
    width = (viewRect.right - viewRect.left) / 3;

    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = szShellFolder;
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    listColumn.pszText = szLinksTo;
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);
    listColumn.cx = viewRect.right - viewRect.left - width - 1;

    SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_INSERTCOLUMNW, 1, (LPARAM) &listColumn);
}

/* Reads the currently set shell folder symbol link targets into asfiInfo. */
static void read_shell_folder_link_targets(void) {
    WCHAR wszPath[MAX_PATH];
    int i;

    for (i=0; i<ARRAY_SIZE(asfiInfo); i++) {
        asfiInfo[i].szLinkTarget[0] = '\0';
        if (SUCCEEDED( SHGetFolderPathW( NULL, asfiInfo[i].nFolder | CSIDL_FLAG_DONT_VERIFY, NULL,
                                         SHGFP_TYPE_CURRENT, wszPath )))
            query_shell_folder( wszPath, asfiInfo[i].szLinkTarget, FILENAME_MAX );
    }
}

static void update_shell_folder_listview(HWND dialog) {
    int i;
    LVITEMW item;
    LONG lSelected = SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_GETNEXTITEM, -1,
                                        MAKELPARAM(LVNI_SELECTED,0));

    SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_DELETEALLITEMS, 0, 0);

    for (i=0; i<ARRAY_SIZE(asfiInfo); i++) {
        WCHAR buffer[MAX_PATH];
        HRESULT hr;
        LPITEMIDLIST pidlCurrent;

        /* Some acrobatic to get the localized name of the shell folder */
        hr = SHGetFolderLocation(dialog, asfiInfo[i].nFolder, NULL, 0, &pidlCurrent);
        if (SUCCEEDED(hr)) { 
            LPSHELLFOLDER psfParent;
            LPCITEMIDLIST pidlLast;
            hr = SHBindToParent(pidlCurrent, &IID_IShellFolder, (LPVOID*)&psfParent, &pidlLast);
            if (SUCCEEDED(hr)) {
                STRRET strRet;
                hr = IShellFolder_GetDisplayNameOf(psfParent, pidlLast, SHGDN_FORADDRESSBAR, &strRet);
                if (SUCCEEDED(hr)) {
                    hr = StrRetToBufW(&strRet, pidlLast, buffer, MAX_PATH);
                }
                IShellFolder_Release(psfParent);
            }
            ILFree(pidlCurrent);
        }

        /* If there's a dangling symlink for the current shell folder, SHGetFolderLocation
         * will fail above. We fall back to the (non-verified) path of the shell folder. */
        if (FAILED(hr)) {
            hr = SHGetFolderPathW(dialog, asfiInfo[i].nFolder|CSIDL_FLAG_DONT_VERIFY, NULL,
                                 SHGFP_TYPE_CURRENT, buffer);
        }
    
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = buffer;
        item.lParam = (LPARAM)&asfiInfo[i];
        SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_INSERTITEMW, 0, (LPARAM)&item);

        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 1;
        item.pszText = strdupU2W(asfiInfo[i].szLinkTarget);
        SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_SETITEMW, 0, (LPARAM)&item);
        free(item.pszText);
    }

    /* Ensure that the previously selected item is selected again. */
    if (lSelected >= 0) {
        item.mask = LVIF_STATE;
        item.state = LVIS_SELECTED;
        item.stateMask = LVIS_SELECTED;
        SendDlgItemMessageW(dialog, IDC_LIST_SFPATHS, LVM_SETITEMSTATE, lSelected, (LPARAM)&item);
    }
}

static void on_shell_folder_selection_changed(HWND hDlg, LPNMLISTVIEW lpnm) {
    if (lpnm->uNewState & LVIS_SELECTED) {
        psfiSelected = (struct ShellFolderInfo *)lpnm->lParam;
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_SFPATH), 1);
        if (*psfiSelected->szLinkTarget) {
            WCHAR *link;
            CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 1);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 1);
            link = strdupU2W(psfiSelected->szLinkTarget);
            set_textW(hDlg, IDC_EDIT_SFPATH, link);
            free(link);
        } else {
            CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 0);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 0);
            set_text(hDlg, IDC_EDIT_SFPATH, "");
        }
    } else {
        psfiSelected = NULL;
        CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
        set_text(hDlg, IDC_EDIT_SFPATH, "");
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_SFPATH), 0);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 0);
        EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 0);
    }
}

/* Keep the contents of the edit control, the listview control and the symlink 
 * information in sync. */
static void on_shell_folder_edit_changed(HWND hDlg) {
    LVITEMW item;
    WCHAR *text = get_text(hDlg, IDC_EDIT_SFPATH);
    LONG iSel = SendDlgItemMessageW(hDlg, IDC_LIST_SFPATHS, LVM_GETNEXTITEM, -1,
                                    MAKELPARAM(LVNI_SELECTED,0));

    if (!text || !psfiSelected || iSel < 0) {
        free(text);
        return;
    }

    WideCharToMultiByte(CP_UNIXCP, 0, text, -1,
                        psfiSelected->szLinkTarget, FILENAME_MAX, NULL, NULL);

    item.mask = LVIF_TEXT;
    item.iItem = iSel;
    item.iSubItem = 1;
    item.pszText = text;
    SendDlgItemMessageW(hDlg, IDC_LIST_SFPATHS, LVM_SETITEMW, 0, (LPARAM)&item);

    free(text);

    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

static void apply_shell_folder_changes(void) {
    WCHAR wszPath[MAX_PATH];
    int i;

    for (i=0; i<ARRAY_SIZE(asfiInfo); i++) {
        if (SUCCEEDED( SHGetFolderPathW( NULL, asfiInfo[i].nFolder | CSIDL_FLAG_CREATE, NULL,
                                         SHGFP_TYPE_CURRENT, wszPath )))
            set_shell_folder( wszPath, asfiInfo[i].szLinkTarget );
    }
}

static void refresh_sysparams(HWND hDlg)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(metrics); i++)
    {
        if (metrics[i].sm_idx != -1)
            metrics[i].size = GetSystemMetrics(metrics[i].sm_idx);
        if (metrics[i].color_idx != -1)
            metrics[i].color = GetSysColor(metrics[i].color_idx);
    }

    on_sysparam_change(hDlg);
}

static void read_sysparams(HWND hDlg)
{
    WCHAR buffer[256];
    HWND list = GetDlgItem(hDlg, IDC_SYSPARAM_COMBO);
    NONCLIENTMETRICSW nonclient_metrics;
    int i, idx;

    for (i = 0; i < ARRAY_SIZE(metrics); i++)
    {
        LoadStringW(GetModuleHandleW(NULL), i + IDC_SYSPARAMS_BUTTON, buffer, ARRAY_SIZE(buffer));
        idx = SendMessageW(list, CB_ADDSTRING, 0, (LPARAM)buffer);
        if (idx != CB_ERR) SendMessageW(list, CB_SETITEMDATA, idx, i);

        if (metrics[i].sm_idx != -1)
            metrics[i].size = GetSystemMetrics(metrics[i].sm_idx);
        if (metrics[i].color_idx != -1)
            metrics[i].color = GetSysColor(metrics[i].color_idx);
    }

    nonclient_metrics.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nonclient_metrics, 0);

    memcpy(&(metrics[IDC_SYSPARAMS_MENU_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfMenuFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_ACTIVE_TITLE_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfCaptionFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_TOOLTIP_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfStatusFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_MSGBOX_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfMessageFont), sizeof(LOGFONTW));
}

static void apply_sysparams(void)
{
    NONCLIENTMETRICSW ncm;
    int i, cnt = 0;
    int colors_idx[ARRAY_SIZE(metrics)];
    COLORREF colors[ARRAY_SIZE(metrics)];
    HDC hdc;
    int dpi;

    hdc = GetDC( 0 );
    dpi = GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( 0, hdc );

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    /* convert metrics back to twips */
    ncm.iMenuWidth = ncm.iMenuHeight =
        MulDiv( metrics[IDC_SYSPARAMS_MENU - IDC_SYSPARAMS_BUTTON].size, -1440, dpi );
    ncm.iCaptionWidth = ncm.iCaptionHeight =
        MulDiv( metrics[IDC_SYSPARAMS_ACTIVE_TITLE - IDC_SYSPARAMS_BUTTON].size, -1440, dpi );
    ncm.iScrollWidth = ncm.iScrollHeight =
        MulDiv( metrics[IDC_SYSPARAMS_SCROLLBAR - IDC_SYSPARAMS_BUTTON].size, -1440, dpi );
    ncm.iSmCaptionWidth = MulDiv( ncm.iSmCaptionWidth, -1440, dpi );
    ncm.iSmCaptionHeight = MulDiv( ncm.iSmCaptionHeight, -1440, dpi );

    ncm.lfMenuFont    = metrics[IDC_SYSPARAMS_MENU_TEXT - IDC_SYSPARAMS_BUTTON].lf;
    ncm.lfCaptionFont = metrics[IDC_SYSPARAMS_ACTIVE_TITLE_TEXT - IDC_SYSPARAMS_BUTTON].lf;
    ncm.lfStatusFont  = metrics[IDC_SYSPARAMS_TOOLTIP_TEXT - IDC_SYSPARAMS_BUTTON].lf;
    ncm.lfMessageFont = metrics[IDC_SYSPARAMS_MSGBOX_TEXT - IDC_SYSPARAMS_BUTTON].lf;

    SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(ncm), &ncm,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    for (i = 0; i < ARRAY_SIZE(metrics); i++)
        if (metrics[i].color_idx != -1)
        {
            colors_idx[cnt] = metrics[i].color_idx;
            colors[cnt++] = metrics[i].color;
        }
    SetSysColors(cnt, colors_idx, colors);
}

static void on_sysparam_change(HWND hDlg)
{
    int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

    index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

    updating_ui = TRUE;

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR_TEXT), metrics[index].color_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), metrics[index].color_idx != -1);
    InvalidateRect(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), NULL, TRUE);

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE_TEXT), metrics[index].sm_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE), metrics[index].sm_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE_UD), metrics[index].sm_idx != -1);
    if (metrics[index].sm_idx != -1)
        SendDlgItemMessageW(hDlg, IDC_SYSPARAM_SIZE_UD, UDM_SETPOS, 0, MAKELONG(metrics[index].size, 0));
    else
        set_text(hDlg, IDC_SYSPARAM_SIZE, "");

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_FONT),
        index == IDC_SYSPARAMS_MENU_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_ACTIVE_TITLE_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_TOOLTIP_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_MSGBOX_TEXT-IDC_SYSPARAMS_BUTTON
    );

    updating_ui = FALSE;
}

static void on_draw_item(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH black_brush = 0;
    LPDRAWITEMSTRUCT draw_info = (LPDRAWITEMSTRUCT)lParam;

    if (!black_brush) black_brush = CreateSolidBrush(0);

    if (draw_info->CtlID == IDC_SYSPARAM_COLOR)
    {
        UINT state;
        HTHEME theme;
        RECT buttonrect;

        theme = OpenThemeDataForDpi(NULL, WC_BUTTONW, GetDpiForWindow(hDlg));

        if (theme) {
            MARGINS margins;

            if (draw_info->itemState & ODS_DISABLED)
                state = PBS_DISABLED;
            else if (draw_info->itemState & ODS_SELECTED)
                state = PBS_PRESSED;
            else
                state = PBS_NORMAL;

            if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
                DrawThemeParentBackground(draw_info->hwndItem, draw_info->hDC, NULL);

            DrawThemeBackground(theme, draw_info->hDC, BP_PUSHBUTTON, state, &draw_info->rcItem, NULL);

            buttonrect = draw_info->rcItem;

            GetThemeMargins(theme, draw_info->hDC, BP_PUSHBUTTON, state, TMT_CONTENTMARGINS, &draw_info->rcItem, &margins);

            buttonrect.left += margins.cxLeftWidth;
            buttonrect.top += margins.cyTopHeight;
            buttonrect.right -= margins.cxRightWidth;
            buttonrect.bottom -= margins.cyBottomHeight;

            if (draw_info->itemState & ODS_FOCUS)
                DrawFocusRect(draw_info->hDC, &buttonrect);

            CloseThemeData(theme);
        } else {
            state = DFCS_ADJUSTRECT | DFCS_BUTTONPUSH;

            if (draw_info->itemState & ODS_DISABLED)
                state |= DFCS_INACTIVE;
            else
                state |= draw_info->itemState & ODS_SELECTED ? DFCS_PUSHED : 0;

            DrawFrameControl(draw_info->hDC, &draw_info->rcItem, DFC_BUTTON, state);

            buttonrect = draw_info->rcItem;
        }

        if (!(draw_info->itemState & ODS_DISABLED))
        {
            HBRUSH brush;
            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);
            brush = CreateSolidBrush(metrics[index].color);

            InflateRect(&buttonrect, -1, -1);
            FrameRect(draw_info->hDC, &buttonrect, black_brush);
            InflateRect(&buttonrect, -1, -1);
            FillRect(draw_info->hDC, &buttonrect, brush);
            DeleteObject(brush);
        }
    }
}

static void on_select_font(HWND hDlg)
{
    CHOOSEFONTW cf;
    int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);
    index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(CHOOSEFONTW);
    cf.hwndOwner = hDlg;
    cf.lpLogFont = &(metrics[index].lf);
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_NOVERTFONTS;

    if (ChooseFontW(&cf))
        SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

static void init_mime_types(HWND hDlg)
{
    WCHAR *buf = get_reg_key(config_key, keypath(L"FileOpenAssociations"), L"Enable", L"Y");
    int state = IS_OPTION_TRUE(*buf) ? BST_CHECKED : BST_UNCHECKED;

    CheckDlgButton(hDlg, IDC_ENABLE_FILE_ASSOCIATIONS, state);

    free(buf);
}

static void update_mime_types(HWND hDlg)
{
    const WCHAR *state = L"Y";

    if (IsDlgButtonChecked(hDlg, IDC_ENABLE_FILE_ASSOCIATIONS) != BST_CHECKED)
        state = L"N";

    set_reg_key(config_key, keypath(L"FileOpenAssociations"), L"Enable", state);
}

static BOOL CALLBACK update_window_pos_proc(HWND hwnd, LPARAM lp)
{
    RECT rect;

    GetClientRect(hwnd, &rect);
    AdjustWindowRectEx(&rect, GetWindowLongW(hwnd, GWL_STYLE), !!GetMenu(hwnd),
                       GetWindowLongW(hwnd, GWL_EXSTYLE));
    SetWindowPos(hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    return TRUE;
}

/* Adjust the rectangle for top-level windows because the new non-client metrics may be different */
static void update_window_pos(void)
{
    EnumWindows(update_window_pos_proc, 0);
}

INT_PTR CALLBACK
ThemeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            read_shell_folder_link_targets();
            init_shell_folder_listview_headers(hDlg);
            update_shell_folder_listview(hDlg);
            read_sysparams(hDlg);
            init_mime_types(hDlg);
            init_dialog(hDlg);
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
                    switch (LOWORD(wParam))
                    {
                        case IDC_THEME_APPCOMBO: /* fall through */
                        case IDC_THEME_THEMECOMBO: on_theme_changed(hDlg); break;
                        case IDC_THEME_COLORCOMBO: /* fall through */
                        case IDC_THEME_SIZECOMBO: theme_dirty = TRUE; break;
                        case IDC_SYSPARAM_COMBO: on_sysparam_change(hDlg); return FALSE;
                    }
                    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                    break;
                }
                case EN_CHANGE: {
                    if (updating_ui) break;
                    switch (LOWORD(wParam))
                    {
                        case IDC_EDIT_SFPATH: on_shell_folder_edit_changed(hDlg); break;
                        case IDC_SYSPARAM_SIZE:
                        {
                            WCHAR *text = get_text(hDlg, IDC_SYSPARAM_SIZE);
                            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

                            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

                            if (text)
                            {
                                metrics[index].size = wcstol(text, NULL, 10);
                                free(text);
                            }
                            else
                            {
                                /* for empty string set to minimum value */
                                SendDlgItemMessageW(hDlg, IDC_SYSPARAM_SIZE_UD, UDM_GETRANGE32, (WPARAM)&metrics[index].size, 0);
                            }

                            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            break;
                        }
                    }
                    break;
                }
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_THEME_INSTALL:
                            on_theme_install (hDlg);
                            break;

                        case IDC_SYSPARAM_FONT:
                            on_select_font(hDlg);
                            break;

                        case IDC_BROWSE_SFPATH:
                        {
                            WCHAR link[FILENAME_MAX];
                            if (browse_for_unix_folder(hDlg, link)) {
                                WideCharToMultiByte(CP_UNIXCP, 0, link, -1,
                                                    psfiSelected->szLinkTarget, FILENAME_MAX,
                                                    NULL, NULL);
                                update_shell_folder_listview(hDlg);
                                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;
                        }

                        case IDC_LINK_SFPATH:
                            if (IsDlgButtonChecked(hDlg, IDC_LINK_SFPATH)) {
                                WCHAR link[FILENAME_MAX];
                                if (browse_for_unix_folder(hDlg, link)) {
                                    WideCharToMultiByte(CP_UNIXCP, 0, link, -1,
                                                        psfiSelected->szLinkTarget, FILENAME_MAX,
                                                        NULL, NULL);
                                    update_shell_folder_listview(hDlg);
                                    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                                } else {
                                    CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
                                }
                            } else {
                                psfiSelected->szLinkTarget[0] = '\0';
                                update_shell_folder_listview(hDlg);
                                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;    

                        case IDC_SYSPARAM_COLOR:
                        {
                            static COLORREF user_colors[16];
                            CHOOSECOLORW c_color;
                            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

                            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

                            memset(&c_color, 0, sizeof(c_color));
                            c_color.lStructSize = sizeof(c_color);
                            c_color.lpCustColors = user_colors;
                            c_color.rgbResult = metrics[index].color;
                            c_color.Flags = CC_ANYCOLOR | CC_RGBINIT;
                            c_color.hwndOwner = hDlg;
                            if (ChooseColorW(&c_color))
                            {
                                metrics[index].color = c_color.rgbResult;
                                save_sys_color(index, metrics[index].color);
                                InvalidateRect(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), NULL, TRUE);
                                SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;
                        }

                        case IDC_ENABLE_FILE_ASSOCIATIONS:
                            update_mime_types(hDlg);
                            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            break;
                    }
                    break;
            }
            break;
        
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_KILLACTIVE: {
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, FALSE);
                    break;
                }
                case PSN_APPLY: {
                    apply();
                    apply_theme(hDlg);
                    apply_shell_folder_changes();
                    apply_sysparams();
                    read_shell_folder_link_targets();
                    update_shell_folder_listview(hDlg);
                    update_mime_types(hDlg);
                    update_window_pos();
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                }
                case LVN_ITEMCHANGED: { 
                    if (wParam == IDC_LIST_SFPATHS)  
                        on_shell_folder_selection_changed(hDlg, (LPNMLISTVIEW)lParam);
                    break;
                }
                case PSN_SETACTIVE: {
                    update_dialog(hDlg);
                    break;
                }
            }
            break;

        case WM_DRAWITEM:
            on_draw_item(hDlg, wParam, lParam);
            break;

        default:
            break;
    }
    return FALSE;
}
