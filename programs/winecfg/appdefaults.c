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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define NONAMELESSUNION
#include <windows.h>
#include <commdlg.h>
#include <wine/debug.h>
#include <stdio.h>
#include <assert.h>
#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

static void update_comboboxes(HWND dialog)
{
  int i;
  const VERSION_DESC *pVer = NULL;

  char *winver;
  
  /* retrieve the registry values */
  winver = get_reg_key(keypath("Version"), "Windows", "");

  /* empty winver means use automatic mode (ie the builtin dll linkage heuristics)  */
  WINE_TRACE("winver is %s\n", *winver != '\0' ? winver : "null (automatic mode)");

  /* normalize the version strings */
  if (*winver != '\0')
  {
    if ((pVer = getWinVersions ()))
    {
      for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
      {
	if (!strcasecmp (pVer->szVersion, winver))
	{
	  SendDlgItemMessage (dialog, IDC_WINVER, CB_SETCURSEL, (WPARAM) (i + 1), 0);
	  WINE_TRACE("match with %s\n", pVer->szVersion);
	}
      }
    }
  }
  else /* no explicit setting */
  {
    WINE_TRACE("setting winver combobox to automatic/default\n");
    SendDlgItemMessage (dialog, IDC_WINVER, CB_SETCURSEL, 0, 0);
  }

  HeapFree(GetProcessHeap(), 0, winver);
}

static void
init_comboboxes (HWND dialog)
{
  int i;
  const VERSION_DESC *ver = NULL;

  SendDlgItemMessage(dialog, IDC_WINVER, CB_RESETCONTENT, 0, 0);

  /* add the default entries (automatic) which correspond to no setting  */
  if (current_app)
  {
      SendDlgItemMessage(dialog, IDC_WINVER, CB_ADDSTRING, 0, (LPARAM) "Use global settings");
  }
  else
  {
      SendDlgItemMessage(dialog, IDC_WINVER, CB_ADDSTRING, 0, (LPARAM) "Automatically detect required version");
  }

  if ((ver = getWinVersions ()))
  {
    for (i = 0; *ver->szVersion || *ver->szDescription; i++, ver++)
    {
      SendDlgItemMessage (dialog, IDC_WINVER, CB_ADDSTRING,
			  0, (LPARAM) ver->szDescription);
    }
  }
}

static void add_listview_item(HWND listview, const char *text, void *association)
{
  LVITEM item;

  ZeroMemory(&item, sizeof(LVITEM));

  item.mask = LVIF_TEXT | LVIF_PARAM;
  item.pszText = (char*) text;
  item.cchTextMax = strlen(text);
  item.lParam = (LPARAM) association;
  item.iItem = ListView_GetItemCount(listview);

  ListView_InsertItem(listview, &item);
}

/* Called when the application is initialized (cannot reinit!)  */
static void init_appsheet(HWND dialog)
{
  HWND listview;
  HKEY key;
  int i;
  DWORD size;
  char appname[1024];

  WINE_TRACE("()\n");

  listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);

  /* we use the lparam field of the item so we can alter the presentation later and not change code
   * for instance, to use the tile view or to display the EXEs embedded 'display name' */
  add_listview_item(listview, "Default Settings", NULL);

  /* because this list is only populated once, it's safe to bypass the settings list here  */
  if (RegOpenKey(config_key, "AppDefaults", &key) == ERROR_SUCCESS)
  {
      i = 0;
      size = sizeof(appname);
      while (RegEnumKeyEx(key, i, appname, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
      {
          add_listview_item(listview, appname, strdupA(appname));

          i++;
          size = sizeof(appname);
      }

      RegCloseKey(key);
  }

  init_comboboxes(dialog);
  
  /* Select the default settings listview item  */
  {
      LVITEM item;
      
      ZeroMemory(&item, sizeof(item));
      
      item.mask = LVIF_STATE;
      item.iItem = 0;
      item.state = LVIS_SELECTED | LVIS_FOCUSED;
      item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

      ListView_SetItem(listview, &item);
  }
  
}

/* there has to be an easier way than this  */
static int get_listview_selection(HWND listview)
{
  int count = ListView_GetItemCount(listview);
  int i;
  
  for (i = 0; i < count; i++)
  {
    if (ListView_GetItemState(listview, i, LVIS_SELECTED)) return i;
  }

  return -1;
}


/* called when the user selects a different application */
static void on_selection_change(HWND dialog, HWND listview)
{
  LVITEM item;
  char *oldapp = current_app;

  WINE_TRACE("()\n");

  item.iItem = get_listview_selection(listview);
  item.mask = LVIF_PARAM;

  WINE_TRACE("item.iItem=%d\n", item.iItem);
  
  if (item.iItem == -1) return;
  
  ListView_GetItem(listview, &item);

  current_app = (char *) item.lParam;

  if (current_app)
  {
      WINE_TRACE("current_app is now %s\n", current_app);
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

static BOOL list_contains_file(HWND listview, char *filename)
{
  LVFINDINFO find_info = { LVFI_STRING, filename, 0, {0, 0}, 0 };
  int index;

  index = ListView_FindItem(listview, -1, &find_info);

  return (index != -1);
}

static void on_add_app_click(HWND dialog)
{
  char filetitle[MAX_PATH];
  char file[MAX_PATH];

  OPENFILENAME ofn = { sizeof(OPENFILENAME),
		       0, /*hInst*/0, "Wine Programs (*.exe,*.exe.so)\0*.exe;*.exe.so\0", NULL, 0, 0, NULL,
		       0, NULL, 0, "c:\\", "Select a Windows executable file",
		       OFN_SHOWHELP | OFN_HIDEREADONLY, 0, 0, NULL, 0, NULL };

  ofn.lpstrFileTitle = filetitle;
  ofn.lpstrFileTitle[0] = '\0';
  ofn.nMaxFileTitle = sizeof(filetitle);
  ofn.lpstrFile = file;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(file);

  if (GetOpenFileName(&ofn))
  {
      HWND listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);
      int count = ListView_GetItemCount(listview);
      char* new_app;
      
      new_app = strdupA(filetitle);

      if (list_contains_file(listview, new_app))
          return;
      
      WINE_TRACE("adding %s\n", new_app);
      
      add_listview_item(listview, new_app, new_app);

      ListView_SetItemState(listview, count, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

      SetFocus(listview);
  }
  else WINE_TRACE("user cancelled\n");
}

static void on_remove_app_click(HWND dialog)
{
    HWND listview = GetDlgItem(dialog, IDC_APP_LISTVIEW);
    int selection = get_listview_selection(listview);
    char *section = keypath(""); /* AppDefaults\\whatever.exe\\ */

    WINE_TRACE("selection=%d, section=%s\n", selection, section);
    
    assert( selection != 0 ); /* user cannot click this button when "default settings" is selected  */

    section[strlen(section)] = '\0'; /* remove last backslash  */
    set_reg_key(section, NULL, NULL); /* delete the section  */
    ListView_DeleteItem(listview, selection);
    ListView_SetItemState(listview, selection - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    SetFocus(listview);
    
    SendMessage(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);        
}

static void on_winver_change(HWND dialog)
{
    int selection = SendDlgItemMessage(dialog, IDC_WINVER, CB_GETCURSEL, 0, 0);
    VERSION_DESC *ver = getWinVersions();

    if (selection == 0)
    {
        WINE_TRACE("automatic/default selected so removing current setting\n");
        set_reg_key(keypath("Version"), "Windows", NULL);
    }
    else
    {
        WINE_TRACE("setting Version\\Windows key to value '%s'\n", ver[selection - 1].szVersion);
        set_reg_key(keypath("Version"), "Windows", ver[selection - 1].szVersion);
    }

    /* enable the apply button  */
    SendMessage(GetParent(dialog), PSM_CHANGED, (WPARAM) dialog, 0);
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
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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
