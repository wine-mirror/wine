/*
 * WineCfg app settings tabsheet
 *
 * Copyright 2004 Robert van Herk
 * Copyright 2004 Chris Morgan
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <wine/debug.h>
#include <stdlib.h>
#include <assert.h>
#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

struct win_version
{
    const WCHAR *szVersion;
    const WCHAR *szDescription;
    const WCHAR *szCurrentVersion;
    DWORD        dwMajorVersion;
    DWORD        dwMinorVersion;
    DWORD        dwBuildNumber;
    DWORD        dwPlatformId;
    const WCHAR *szCSDVersion;
    WORD         wServicePackMajor;
    WORD         wServicePackMinor;
    const WCHAR *szProductType;
};

static const struct win_version win_versions[] =
{
    { L"win11",     L"Windows 11",      L"6.3", 10,  0, 22000, VER_PLATFORM_WIN32_NT, L"", 0, 0, L"WinNT"},
    { L"win10",     L"Windows 10",      L"6.3", 10,  0, 19043, VER_PLATFORM_WIN32_NT, L"", 0, 0, L"WinNT"},
    { L"win81",     L"Windows 8.1",     NULL,    6,  3,  9600, VER_PLATFORM_WIN32_NT, L"", 0, 0, L"WinNT"},
    { L"win8",      L"Windows 8",       NULL,    6,  2,  9200, VER_PLATFORM_WIN32_NT, L"", 0, 0, L"WinNT"},
    { L"win2008r2", L"Windows 2008 R2", NULL,    6,  1,  7601, VER_PLATFORM_WIN32_NT, L"Service Pack 1", 1, 0, L"ServerNT"},
    { L"win7",      L"Windows 7",       NULL,    6,  1,  7601, VER_PLATFORM_WIN32_NT, L"Service Pack 1", 1, 0, L"WinNT"},
    { L"win2008",   L"Windows 2008",    NULL,    6,  0,  6002, VER_PLATFORM_WIN32_NT, L"Service Pack 2", 2, 0, L"ServerNT"},
    { L"vista",     L"Windows Vista",   NULL,    6,  0,  6002, VER_PLATFORM_WIN32_NT, L"Service Pack 2", 2, 0, L"WinNT"},
    { L"win2003",   L"Windows 2003",    NULL,    5,  2,  3790, VER_PLATFORM_WIN32_NT, L"Service Pack 2", 2, 0, L"ServerNT"},
    { L"winxp64",   L"Windows XP 64",   NULL,    5,  2,  3790, VER_PLATFORM_WIN32_NT, L"Service Pack 2", 2, 0, L"WinNT"},
    { L"winxp",     L"Windows XP",      NULL,    5,  1,  2600, VER_PLATFORM_WIN32_NT, L"Service Pack 3", 3, 0, L"WinNT"},
    { L"win2k",     L"Windows 2000",    NULL,    5,  0,  2195, VER_PLATFORM_WIN32_NT, L"Service Pack 4", 4, 0, L"WinNT"},
    { L"winme",     L"Windows ME",      NULL,    4, 90,  3000, VER_PLATFORM_WIN32_WINDOWS, L" ", 0, 0, L""},
    { L"win98",     L"Windows 98",      NULL,    4, 10,  2222, VER_PLATFORM_WIN32_WINDOWS, L" A ", 0, 0, L""},
    { L"win95",     L"Windows 95",      NULL,    4,  0,   950, VER_PLATFORM_WIN32_WINDOWS, L"", 0, 0, L""},
    { L"nt40",      L"Windows NT 4.0",  NULL,    4,  0,  1381, VER_PLATFORM_WIN32_NT, L"Service Pack 6a", 6, 0, L"WinNT"},
    { L"nt351",     L"Windows NT 3.51", NULL,    3, 51,  1057, VER_PLATFORM_WIN32_NT, L"Service Pack 5", 5, 0, L"WinNT"},
    { L"win31",     L"Windows 3.1",     NULL,    3, 10,     0, VER_PLATFORM_WIN32s, L"Win32s 1.3", 0, 0, L""},
    { L"win30",     L"Windows 3.0",     NULL,    3,  0,     0, VER_PLATFORM_WIN32s, L"Win32s 1.3", 0, 0, L""},
    { L"win20",     L"Windows 2.0",     NULL,    2,  0,     0, VER_PLATFORM_WIN32s, L"Win32s 1.3", 0, 0, L""}
};

#define DEFAULT_WIN_VERSION   L"win10"

static const WCHAR szKey9x[] = L"Software\\Microsoft\\Windows\\CurrentVersion";
static const WCHAR szKeyNT[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion";
static const WCHAR szKeyProdNT[] = L"System\\CurrentControlSet\\Control\\ProductOptions";

static DWORD get_reg_dword( HKEY root, const WCHAR *subkey, const WCHAR *value )
{
    HKEY hkey;
    DWORD ret, len = sizeof(ret), type;

    if (RegOpenKeyExW( root, subkey, 0, KEY_QUERY_VALUE, &hkey )) return 0;
    if (RegQueryValueExW( hkey, value, NULL, &type, (BYTE *)&ret, &len ) || type != REG_DWORD) ret = 0;
    RegCloseKey( hkey );
    return ret;
}

static int get_registry_version(void)
{
    int i, best = -1, platform, major = 0, minor = 0, build = 0;
    WCHAR *p, *ver, *type = NULL;

    if ((ver = get_reg_key( HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentVersion", NULL )))
    {
        WCHAR *build_str;

        platform = VER_PLATFORM_WIN32_NT;

        major = get_reg_dword( HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMajorVersionNumber" );
        minor = get_reg_dword( HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMinorVersionNumber" );

        build_str = get_reg_key( HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuildNumber", NULL );
        build = wcstol(build_str, NULL, 10);

        type = get_reg_key( HKEY_LOCAL_MACHINE, szKeyProdNT, L"ProductType", NULL );
    }
    else if ((ver = get_reg_key( HKEY_LOCAL_MACHINE, szKey9x, L"VersionNumber", NULL )))
        platform = VER_PLATFORM_WIN32_WINDOWS;
    else
        return -1;

    if (!major)
    {
        if ((p = wcschr( ver, '.' )))
        {
            WCHAR *minor_str = p;
            *minor_str++ = 0;
            if ((p = wcschr( minor_str, '.' )))
            {
                WCHAR *build_str = p;
                *build_str++ = 0;
                build = wcstol(build_str, NULL, 10);
            }
            minor = wcstol(minor_str, NULL, 10);
        }
        major = wcstol(ver, NULL, 10);
    }

    for (i = 0; i < ARRAY_SIZE(win_versions); i++)
    {
        if (win_versions[i].dwPlatformId != platform) continue;
        if (win_versions[i].dwMajorVersion != major) continue;
        if (type && wcsicmp(win_versions[i].szProductType, type)) continue;
        best = i;
        if ((win_versions[i].dwMinorVersion == minor) &&
            (win_versions[i].dwBuildNumber == build))
            return i;
    }
    return best;
}

static void update_comboboxes(HWND dialog)
{
    int i, ver;
    WCHAR *winver;

    /* retrieve the registry values */
    winver = get_reg_key(config_key, keypath(L""), L"Version", L"");
    ver = get_registry_version();

    if (!winver || !winver[0])
    {
        free(winver);

        if (current_app) /* no explicit setting */
        {
            WINE_TRACE("setting winver combobox to default\n");
            SendDlgItemMessageW(dialog, IDC_WINVER, CB_SETCURSEL, 0, 0);
            return;
        }
        if (ver != -1) winver = wcsdup(win_versions[ver].szVersion);
        else winver = wcsdup(DEFAULT_WIN_VERSION);
    }
    WINE_TRACE("winver is %s\n", debugstr_w(winver));

    /* normalize the version strings */
    for (i = 0; i < ARRAY_SIZE(win_versions); i++)
    {
        if (!wcsicmp(win_versions[i].szVersion, winver))
        {
            SendDlgItemMessageW(dialog, IDC_WINVER, CB_SETCURSEL,
                                i + (current_app?1:0), 0);
            WINE_TRACE("match with %s\n", debugstr_w(win_versions[i].szVersion));
            break;
	}
    }

    free(winver);
}

static void
init_comboboxes (HWND dialog)
{
    int i;

    SendDlgItemMessageW(dialog, IDC_WINVER, CB_RESETCONTENT, 0, 0);

    /* add the default entries (automatic) which correspond to no setting  */
    if (current_app)
    {
        WCHAR str[256];
        LoadStringW(GetModuleHandleW(NULL), IDS_USE_GLOBAL_SETTINGS, str, ARRAY_SIZE(str));
        SendDlgItemMessageW (dialog, IDC_WINVER, CB_ADDSTRING, 0, (LPARAM)str);
    }

    for (i = 0; i < ARRAY_SIZE(win_versions); i++)
    {
      SendDlgItemMessageW(dialog, IDC_WINVER, CB_ADDSTRING,
                          0, (LPARAM) win_versions[i].szDescription);
    }
}

static void add_listview_item(HWND listview, WCHAR *text, void *association)
{
  LVITEMW item;

  item.mask = LVIF_TEXT | LVIF_PARAM;
  item.pszText = text;
  item.cchTextMax = lstrlenW(text);
  item.lParam = (LPARAM) association;
  item.iItem = SendMessageW( listview, LVM_GETITEMCOUNT, 0, 0 );
  item.iSubItem = 0;

  SendMessageW(listview, LVM_INSERTITEMW, 0, (LPARAM) &item);
}

/* Called when the application is initialized (cannot reinit!)  */
static void init_appsheet(HWND dialog)
{
  HWND listview;
  LVITEMW item;
  HKEY key;
  int i;
  DWORD size;
  WCHAR appname[1024];

  WINE_TRACE("()\n");

  listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);

  /* we use the lparam field of the item so we can alter the presentation later and not change code
   * for instance, to use the tile view or to display the EXEs embedded 'display name' */
  LoadStringW(GetModuleHandleW(NULL), IDS_DEFAULT_SETTINGS, appname, ARRAY_SIZE(appname));
  add_listview_item(listview, appname, NULL);

  /* because this list is only populated once, it's safe to bypass the settings list here  */
  if (RegOpenKeyW(config_key, L"AppDefaults", &key) == ERROR_SUCCESS)
  {
      i = 0;
      size = ARRAY_SIZE(appname);
      while (RegEnumKeyExW (key, i, appname, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
      {
          add_listview_item(listview, appname, wcsdup(appname));

          i++;
          size = ARRAY_SIZE(appname);
      }

      RegCloseKey(key);
  }

  init_comboboxes(dialog);
  
  /* Select the default settings listview item  */
  item.iItem = 0;
  item.iSubItem = 0;
  item.mask = LVIF_STATE;
  item.state = LVIS_SELECTED | LVIS_FOCUSED;
  item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

  SendMessageW(listview, LVM_SETITEMW, 0, (LPARAM) &item);
}

/* there has to be an easier way than this  */
static int get_listview_selection(HWND listview)
{
  int count = SendMessageW(listview, LVM_GETITEMCOUNT, 0, 0);
  int i;

  for (i = 0; i < count; i++)
  {
      if (SendMessageW( listview, LVM_GETITEMSTATE, i, LVIS_SELECTED )) return i;
  }

  return -1;
}


/* called when the user selects a different application */
static void on_selection_change(HWND dialog, HWND listview)
{
  LVITEMW item;
  WCHAR* oldapp = current_app;

  WINE_TRACE("()\n");

  item.iItem = get_listview_selection(listview);
  item.iSubItem = 0;
  item.mask = LVIF_PARAM;

  WINE_TRACE("item.iItem=%d\n", item.iItem);
  
  if (item.iItem == -1) return;

  SendMessageW(listview, LVM_GETITEMW, 0, (LPARAM) &item);

  current_app = (WCHAR*) item.lParam;

  if (current_app)
  {
      WINE_TRACE("current_app is now %s\n", wine_dbgstr_w (current_app));
      enable(IDC_APP_REMOVEAPP);
  }
  else
  {
      WINE_TRACE("current_app=NULL, editing global settings\n");
      /* focus will never be on the button in this callback so it's safe  */
      disable(IDC_APP_REMOVEAPP);
  }

  /* reset the combo boxes if we changed from/to global/app-specific  */

  if ((oldapp && !current_app) || (!oldapp && current_app))
      init_comboboxes(dialog);
  
  update_comboboxes(dialog);

  set_window_title(dialog);
}

static BOOL list_contains_file(HWND listview, WCHAR *filename)
{
  LVFINDINFOW find_info = { LVFI_STRING, filename, 0, {0, 0}, 0 };
  int index;

  index = ListView_FindItemW(listview, -1, &find_info);

  return (index != -1);
}

static void on_add_app_click(HWND dialog)
{
  WCHAR filetitle[MAX_PATH];
  WCHAR file[MAX_PATH];
  WCHAR programsFilter[100], filter[MAX_PATH];
  WCHAR selectExecutableStr[100];

  OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW),
		       dialog, /*hInst*/0, 0, NULL, 0, 0, NULL,
		       0, NULL, 0, L"C:\\", 0,
		       OFN_SHOWHELP | OFN_HIDEREADONLY | OFN_ENABLESIZING,
                       0, 0, NULL, 0, NULL };

  LoadStringW (GetModuleHandleW(NULL), IDS_SELECT_EXECUTABLE, selectExecutableStr,
      ARRAY_SIZE(selectExecutableStr));
  LoadStringW (GetModuleHandleW(NULL), IDS_EXECUTABLE_FILTER, programsFilter,
      ARRAY_SIZE(programsFilter));
  swprintf( filter, MAX_PATH, L"%s%c*.exe;*.exe.so%c", programsFilter, 0, 0 );

  ofn.lpstrTitle = selectExecutableStr;
  ofn.lpstrFilter = filter;
  ofn.lpstrFileTitle = filetitle;
  ofn.lpstrFileTitle[0] = '\0';
  ofn.nMaxFileTitle = ARRAY_SIZE(filetitle);
  ofn.lpstrFile = file;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = ARRAY_SIZE(file);

  if (GetOpenFileNameW (&ofn))
  {
      HWND listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);
      int count = SendMessageW(listview, LVM_GETITEMCOUNT, 0, 0);
      WCHAR* new_app;
      LVITEMW item;

      if (list_contains_file(listview, filetitle))
          return;

      new_app = wcsdup(filetitle);

      WINE_TRACE("adding %s\n", wine_dbgstr_w (new_app));

      add_listview_item(listview, new_app, new_app);

      item.mask = LVIF_STATE;
      item.state = LVIS_SELECTED | LVIS_FOCUSED;
      item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
      SendMessageW(listview, LVM_SETITEMSTATE, count, (LPARAM)&item );

      SetFocus(listview);
  }
  else WINE_TRACE("user cancelled\n");
}

static void on_remove_app_click(HWND dialog)
{
    HWND listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);
    int selection = get_listview_selection(listview);
    LVITEMW item;

    item.iItem = selection;
    item.iSubItem = 0;
    item.mask = LVIF_PARAM;

    WINE_TRACE("selection=%d\n", selection);

    assert( selection != 0 ); /* user cannot click this button when "default settings" is selected  */

    set_reg_key(config_key, keypath(L""), NULL, NULL); /* delete the section  */
    SendMessageW(listview, LVM_GETITEMW, 0, (LPARAM) &item);
    free((void*)item.lParam);
    SendMessageW(listview, LVM_DELETEITEM, selection, 0);
    item.mask = LVIF_STATE;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    SendMessageW(listview, LVM_SETITEMSTATE, 0, (LPARAM)&item);

    SetFocus(listview);
    SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
}

static void set_winver(const struct win_version *version)
{
    static const WCHAR szKeyWindNT[] = L"System\\CurrentControlSet\\Control\\Windows";
    static const WCHAR szKeyEnvNT[]  = L"System\\CurrentControlSet\\Control\\Session Manager\\Environment";
    WCHAR buffer[40];

    switch (version->dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            swprintf(buffer, ARRAY_SIZE(buffer), L"%d.%d.%d", version->dwMajorVersion,
                        version->dwMinorVersion, version->dwBuildNumber);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"VersionNumber", buffer);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"SubVersionNumber", version->szCSDVersion);
            swprintf(buffer, ARRAY_SIZE(buffer), L"Microsoft %s", version->szDescription);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"ProductName", buffer);

            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CSDVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMajorVersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMinorVersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuild", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuildNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"ProductName", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyProdNT, L"ProductType", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyWindNT, L"CSDVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyEnvNT, L"OS", NULL);
            set_reg_key(config_key, keypath(L""), L"Version", NULL);
            break;

        case VER_PLATFORM_WIN32_NT:
            if (version->szCurrentVersion)
                set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentVersion", version->szCurrentVersion);
            else
            {
                swprintf(buffer, ARRAY_SIZE(buffer), L"%d.%d", version->dwMajorVersion, version->dwMinorVersion);
                set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentVersion", buffer);
            }
            set_reg_key_dword(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMajorVersionNumber", version->dwMajorVersion);
            set_reg_key_dword(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentMinorVersionNumber", version->dwMinorVersion);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CSDVersion", version->szCSDVersion);
            swprintf(buffer, ARRAY_SIZE(buffer), L"%d", version->dwBuildNumber);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuild", buffer);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuildNumber", buffer);
            swprintf(buffer, ARRAY_SIZE(buffer), L"Microsoft %s", version->szDescription);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"ProductName", buffer);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyProdNT, L"ProductType", version->szProductType);
            set_reg_key_dword(HKEY_LOCAL_MACHINE, szKeyWindNT, L"CSDVersion",
                                MAKEWORD( version->wServicePackMinor,
                                        version->wServicePackMajor ));
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyEnvNT, L"OS", L"Windows_NT");

            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"VersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"SubVersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"ProductName", NULL);
            set_reg_key(config_key, keypath(L""), L"Version", NULL);
            break;

        case VER_PLATFORM_WIN32s:
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CSDVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuild", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"CurrentBuildNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyNT, L"ProductName", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyProdNT, L"ProductType", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyWindNT, L"CSDVersion", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKeyEnvNT, L"OS", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"VersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"SubVersionNumber", NULL);
            set_reg_key(HKEY_LOCAL_MACHINE, szKey9x, L"ProductName", NULL);
            set_reg_key(config_key, keypath(L""), L"Version", version->szVersion);
            break;
    }
}

BOOL set_winver_from_string(const WCHAR *version)
{
    int i;

    WINE_TRACE("desired winver: %s\n", debugstr_w(version));

    for (i = 0; i < ARRAY_SIZE(win_versions); i++)
    {
        if (!wcsicmp(win_versions[i].szVersion, version))
        {
            WINE_TRACE("match with %s\n", debugstr_w(win_versions[i].szVersion));
            set_winver(&win_versions[i]);
            apply();
            return TRUE;
        }
    }

    return FALSE;
}

void print_windows_versions(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(win_versions); i++)
    {
        MESSAGE("  %10ls  %ls\n", win_versions[i].szVersion, win_versions[i].szDescription);
    }
}

void print_current_winver(void)
{
    WCHAR *winver = get_reg_key(config_key, keypath(L""), L"Version", L"");

    if (!winver || !winver[0])
    {
        int ver = get_registry_version();
        MESSAGE("%ls\n", ver == -1 ? DEFAULT_WIN_VERSION : win_versions[ver].szVersion);
    }
    else
        MESSAGE("%ls\n", winver);

    free(winver);
}

static void on_winver_change(HWND dialog)
{
    int selection = SendDlgItemMessageW(dialog, IDC_WINVER, CB_GETCURSEL, 0, 0);

    if (current_app)
    {
        if (!selection)
        {
            WINE_TRACE("default selected so removing current setting\n");
            set_reg_key(config_key, keypath(L""), L"Version", NULL);
        }
        else
        {
            WINE_TRACE("setting Version key to value %s\n", debugstr_w(win_versions[selection-1].szVersion));
            set_reg_key(config_key, keypath(L""), L"Version", win_versions[selection-1].szVersion);
        }
    }
    else /* global version only */
    {
        set_winver(&win_versions[selection]);
    }

    /* enable the apply button  */
    SendMessageW(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
}

INT_PTR CALLBACK
AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
        init_appsheet(hDlg);
        break;

    case WM_SHOWWINDOW:
        set_window_title(hDlg);
        break;

    case WM_NOTIFY:
      switch (((LPNMHDR)lParam)->code)
      {
        case LVN_ITEMCHANGED:
            on_selection_change(hDlg, GetDlgItem(hDlg, IDC_APP_LISTVIEW));
            break;
        case PSN_APPLY:
            apply();
            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            break;
      }
      
      break;
    
    case WM_COMMAND:
      switch(HIWORD(wParam))
      {
        case CBN_SELCHANGE:
          switch(LOWORD(wParam))
          {
            case IDC_WINVER:
              on_winver_change(hDlg);
              break;
          }
        case BN_CLICKED:
          switch(LOWORD(wParam))
          {
            case IDC_APP_ADDAPP:
              on_add_app_click(hDlg);
              break;
            case IDC_APP_REMOVEAPP:
              on_remove_app_click(hDlg);
              break;
          }
          break;
      }

      break;
  }
  
  return 0;
}
