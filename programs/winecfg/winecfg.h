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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#define return_if_fail(try) \
    if (!(try)) { \
        WINE_ERR("check (" #try ") at %s:%d failed, returning\n", __FILE__,  __LINE__ - 1); \
	return; \
    }

#define WRITEME(owner) MessageBox(owner, "Write me!", "", MB_OK | MB_ICONEXCLAMATION);

extern char *currentApp; /* NULL means editing global settings  */

/* Use get and set to alter registry settings. The changes made through set
   won't be committed to the registry until process_all_settings is called,
   however get will still return accurate information.

   You are expected to release the result of get. The parameters to set will
   be copied, so release them too when necessary.
 */
void set(char *path, char *name, char *value);
char *get(char *path, char *name, char *def);
BOOL exists(char *path, char *name);
void apply(void);
char **enumerate_values(char *path);

/* returns a string of the form "AppDefaults\\appname.exe\\section", or just "section" if
 * the user is editing the global settings.
 *
 * no explicit free is needed of the string returned by this function
 */
char *keypath(char *section); 

/* Initializes the transaction system */
int initialize(void);
extern HKEY config_key;

void set_window_title(HWND dialog);

/* Graphics */

void initGraphDlg (HWND hDlg);
void saveGraphDlgSettings (HWND hDlg);
INT_PTR CALLBACK GraphDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Drive management */
void initDriveDlg (HWND hDlg);
void saveDriveSettings (HWND hDlg);

INT_PTR CALLBACK DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LibrariesDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Audio config dialog */
INT_PTR CALLBACK AudioDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* some basic utilities to make win32 suck less */
#define disable(id) EnableWindow(GetDlgItem(dialog, id), 0);
#define enable(id) EnableWindow(GetDlgItem(dialog, id), 1);
void PRINTERROR(void); /* WINE_TRACE() the plaintext error message from GetLastError() */

/* returns a string in the win32 heap  */
static inline char *strdupA(char *s)
{
    char *r = HeapAlloc(GetProcessHeap(), 0, strlen(s));
    return strcpy(r, s);
}

static inline char *get_control_text(HWND dialog, WORD id)
{
    HWND item = GetDlgItem(dialog, id);
    int len = GetWindowTextLength(item) + 1;
    char *result = HeapAlloc(GetProcessHeap(), 0, len);
    if (GetWindowText(item, result, len) == 0) return NULL;
    return result;
}

#define WINE_KEY_ROOT "Software\\Wine\\Wine\\Config"

#endif
