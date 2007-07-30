/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2004 Mike Hearn
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

#ifndef WINE_CFG_H
#define WINE_CFG_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "properties.h"

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

extern WCHAR* current_app; /* NULL means editing global settings  */

/* Use get_reg_key and set_reg_key to alter registry settings. The changes made through
   set_reg_key won't be committed to the registry until process_all_settings is called,
   however get_reg_key will still return accurate information.

   The root HKEY has to be non-ambiguous. So only the registry roots (HKCU, HKLM, ...) or
   the global config_key are allowed here.
   
   You are expected to HeapFree the result of get_reg_key. The parameters to set_reg_key will
   be copied, so free them too when necessary.
 */

void set_reg_key(HKEY root, const char *path, const char *name, const char *value);
void set_reg_key_dword(HKEY root, const char *path, const char *name, DWORD value);
char *get_reg_key(HKEY root, const char *path, const char *name, const char *def);
BOOL reg_key_exists(HKEY root, const char *path, const char *name);
void apply(void);
char **enumerate_values(HKEY root, char *path);

/* Load a string from the resources. Allocated with HeapAlloc (GetProcessHeap()) */
WCHAR* load_string (UINT id);

/* returns a string of the form "AppDefaults\\appname.exe\\section", or just "section" if
   the user is editing the global settings.
 
   no explicit free is needed of the string returned by this function
 */
char *keypath(const char *section); 

int initialize(HINSTANCE hInstance);
extern HKEY config_key;

/* hack for the property sheet control  */
void set_window_title(HWND dialog);

/* Window procedures */
INT_PTR CALLBACK GraphDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LibrariesDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AudioDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ThemeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Drive management  */
void load_drives(void);
int autodetect_drives(void);

struct drive
{
    char letter;
    char *unixpath;
    char *label;
    char *serial;
    DWORD type; /* one of the DRIVE_ constants from winbase.h  */

    BOOL in_use;
};

#define DRIVE_MASK_BIT(B) (1 << (toupper(B) - 'A'))

long drive_available_mask(char letter);
BOOL add_drive(const char letter, const char *targetpath, const char *label, const char *serial, unsigned int type);
void delete_drive(struct drive *pDrive);
void apply_drive_changes(void);
BOOL browse_for_unix_folder(HWND dialog, char *pszPath);
extern struct drive drives[26]; /* one for each drive letter */

BOOL gui_mode;

/* Some basic utilities to make win32 suck less */
#define disable(id) EnableWindow(GetDlgItem(dialog, id), 0);
#define enable(id) EnableWindow(GetDlgItem(dialog, id), 1);
void PRINTERROR(void); /* WINE_TRACE() the plaintext error message from GetLastError() */

/* returns a string in the win32 heap  */
static inline char *strdupA(const char *s)
{
    char *r = HeapAlloc(GetProcessHeap(), 0, strlen(s)+1);
    return strcpy(r, s);
}

static inline WCHAR *strdupW(const WCHAR *s)
{
    WCHAR *r = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(s)+1)*sizeof(WCHAR));
    return lstrcpyW(r, s);
}

static inline char *get_text(HWND dialog, WORD id)
{
    HWND item = GetDlgItem(dialog, id);
    int len = GetWindowTextLength(item) + 1;
    char *result = len ? HeapAlloc(GetProcessHeap(), 0, len) : NULL;
    if (!result || GetWindowText(item, result, len) == 0) return NULL;
    return result;
}

static inline void set_text(HWND dialog, WORD id, const char *text)
{
    SetWindowText(GetDlgItem(dialog, id), text);
}

#define WINE_KEY_ROOT "Software\\Wine"
#define MAXBUFLEN 256

extern HMENU     hPopupMenus;

#endif
