/*
 * WineCfg app settings tabsheet
 *
 * Copyright 2004 Robert van Herk
 *                Chris Morgan
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
#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

typedef struct _APPL
{
  BOOL isGlobal;
  char* lpcApplication;
  char* lpcVersionSection;
  char* lpcWinelookSection;
} APPL, *LPAPPL;

static LPAPPL CreateAppl(BOOL isGlobal, char* application, char* version_section, char* winelook_section)
{
  LPAPPL out;
  WINE_TRACE("application: '%s', version_section: '%s', winelook_section: '%s'\n", application,
	     version_section, winelook_section);
  out = HeapAlloc(GetProcessHeap(),0,sizeof(APPL));
  out->lpcApplication = strdup(application);
  out->lpcVersionSection = strdup(version_section);
  out->lpcWinelookSection = strdup(winelook_section);
  out->isGlobal = isGlobal;
  return out;
}

static VOID FreeAppl(LPAPPL lpAppl)
{
 /* The strings were strdup-ped, so we use "free" */
  if (lpAppl->lpcApplication) free(lpAppl->lpcApplication);
  if (lpAppl->lpcVersionSection) free(lpAppl->lpcVersionSection);
  if (lpAppl->lpcWinelookSection) free(lpAppl->lpcWinelookSection);
  HeapFree(GetProcessHeap(),0,lpAppl);
}

/* section can be "Version" or "Tweak.Layout" */
/* key can be "Windows"/"Dos"/"WineLook" */
/* value can be appropriate values for the above keys */
typedef struct _APPSETTING
{
  char* section;
  char* lpcKey;
  char* value;
} APPSETTING, *LPAPPSETTING;

static LPAPPSETTING CreateAppSetting(char* lpcKey, char* value)
{
	LPAPPSETTING out = HeapAlloc(GetProcessHeap(),0,sizeof(APPSETTING));
	out->lpcKey = strdup (lpcKey);
	out->value = strdup(value);
	return out;
}

static VOID FreeAppSetting(LPAPPSETTING las)
{
  if (las->lpcKey) free(las->lpcKey);
  if (las->value) free(las->value);
  HeapFree(GetProcessHeap(),0,las);
}

typedef struct _ITEMTAG
{
  LPAPPL lpAppl;
  LPAPPSETTING lpSetting;
} ITEMTAG, *LPITEMTAG;

static LPITEMTAG CreateItemTag()
{
  LPITEMTAG out;
  out = HeapAlloc(GetProcessHeap(),0,sizeof(ITEMTAG));
  out->lpAppl = 0;
  out->lpSetting = 0;
  return out;
}

static VOID FreeItemTag(LPITEMTAG lpit)
{
  /* if child, only free the lpSetting, the lpAppl will be freed when we free the parents item tag */
  if (lpit->lpSetting)
  {
    FreeAppSetting(lpit->lpSetting);
  } else
  {
    if (lpit->lpAppl)
      FreeAppl(lpit->lpAppl);
  }
  HeapFree(GetProcessHeap(), 0, lpit);
}

static VOID LoadAppSettings(LPAPPL appl /*DON'T FREE, treeview will own this*/, HWND hDlg, HWND hwndTV)
{
  HKEY key;
  int i;
  DWORD size;
  DWORD readSize;
  char name [255];
  char read [255];
  char *description;
  LPITEMTAG lpIt;
  TVINSERTSTRUCT tis;	
  HTREEITEM hParent;
  LPAPPSETTING lpas;
	
  WINE_TRACE("opening '%s' and '%s'\n", appl->lpcVersionSection, appl->lpcWinelookSection);
  if ((RegOpenKey (configKey, appl->lpcVersionSection, &key) == ERROR_SUCCESS) ||
      (RegOpenKey (configKey, appl->lpcWinelookSection, &key) == ERROR_SUCCESS))
  {
     i = 0;
     size = 255;
     readSize = 255;

     lpIt = CreateItemTag();
     lpIt->lpAppl = appl;

     tis.hParent = NULL;
     tis.hInsertAfter = TVI_LAST;
     tis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
     tis.u.item.pszText = appl->lpcApplication;
     tis.u.item.lParam = (LPARAM)lpIt;
     hParent = TreeView_InsertItem(hwndTV,&tis);
     tis.hParent = hParent;

     /* insert version entries */
     if(RegOpenKey (configKey, appl->lpcVersionSection, &key) == ERROR_SUCCESS)
     {
       while (RegEnumValue(key, i, name, &size, NULL, NULL, read, &readSize) == ERROR_SUCCESS)
       {
	 char itemtext[128];

	 WINE_TRACE("Reading value %s, namely %s\n", name, read);
			
	 lpIt = CreateItemTag();
	 lpas = CreateAppSetting(name, read);
	 lpIt->lpSetting = lpas;
	 lpIt->lpAppl = appl;
	 tis.u.item.lParam = (LPARAM)lpIt;

	 /* convert the value for 'dosver or winver' to human readable form */
	 description = getDescriptionFromVersion(getWinVersions(), read);
	 if(!description)
	 {
	   description = getDescriptionFromVersion(getDOSVersions(), read);
	   if(!description)
	     description = "Not found";
	 }

	 sprintf(itemtext, "%s - %s", name, description);
	 tis.u.item.pszText = itemtext;

	 TreeView_InsertItem(hwndTV,&tis);
	 i++; size = 255; readSize = 255;
       }
       RegCloseKey(key);
     } else
     {
       WINE_TRACE("no version section found\n");
     }

     i = 0; /* reset i to 0 before calling RegEnumValue() again */

     /* insert winelook entries */
     if(RegOpenKey (configKey, appl->lpcWinelookSection, &key) == ERROR_SUCCESS)
     {
       while (RegEnumValue(key, i, name, &size, NULL, NULL, read, &readSize) == ERROR_SUCCESS)
       {
	 char itemtext[128];

	 WINE_TRACE("Reading value %s, namely %s\n", name, read);
			
	 lpIt = CreateItemTag();
	 lpas = CreateAppSetting(name, read);
	 lpIt->lpSetting = lpas;
	 lpIt->lpAppl = appl;
	 tis.u.item.lParam = (LPARAM)lpIt;

	 /* convert the value for 'winelook' to human readable form */
	 description = getDescriptionFromVersion(getWinelook(), read);
	 if(!description) description = "Not found";
	 
	 sprintf(itemtext, "%s - %s", name, description);
	 tis.u.item.pszText = itemtext;

	 TreeView_InsertItem(hwndTV,&tis);
	 i++; size = 255; readSize = 255;
       }
       RegCloseKey(key);
     } else
     {
       WINE_TRACE("no winelook section found\n");
     }
  }
}

static VOID SetEnabledAppControls(HWND dialog)
{
}

static VOID UpdateComboboxes(HWND hDlg, LPAPPL lpAppl)
{
  int i;
  const VERSION_DESC *pVer = NULL;

  /* retrieve the registry values for this application */
  char *curWinVer = getConfigValue(lpAppl->lpcVersionSection, "Windows", "");
  char *curDOSVer = getConfigValue(lpAppl->lpcVersionSection, "DOS", "");
  char *curWineLook = getConfigValue(lpAppl->lpcWinelookSection, "WineLook", "");

  if(curWinVer) WINE_TRACE("curWinVer is '%s'\n", curWinVer);
  else WINE_TRACE("curWinVer is null\n");
  if(curDOSVer) WINE_TRACE("curDOSVer is '%s'\n", curDOSVer);
  else WINE_TRACE("curDOSVer is null\n");
  if(curWineLook) WINE_TRACE("curWineLook is '%s'\n", curWineLook);
  else WINE_TRACE("curWineLook is null\n");

  /* normalize the version strings */
  if(strlen(curWinVer) != 0)
  {
    if ((pVer = getWinVersions ()))
    {
      WINE_TRACE("Windows version\n");
      for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
      {
	WINE_TRACE("pVer->szVersion == %s\n", pVer->szVersion);
	if (!strcasecmp (pVer->szVersion, curWinVer))
	{
	  SendDlgItemMessage (hDlg, IDC_WINVER, CB_SETCURSEL,
			      (WPARAM) i, 0);
	  WINE_TRACE("match with %s\n", pVer->szVersion);
	}
      }
    }
  } else /* clear selection */
  {
    WINE_TRACE("setting winver to nothing\n");
    SendDlgItemMessage (hDlg, IDC_WINVER, CB_SETCURSEL,
			-1, 0);
  }

  if(strlen(curDOSVer) != 0)
  {
    if ((pVer = getDOSVersions ()))
    {
      WINE_TRACE("DOS version\n");
      for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
      {
	WINE_TRACE("pVer->szVersion == %s\n", pVer->szVersion);
	if (!strcasecmp (pVer->szVersion, curDOSVer))
	{
	  SendDlgItemMessage (hDlg, IDC_DOSVER, CB_SETCURSEL,
			      (WPARAM) i, 0);
	  WINE_TRACE("match with %s\n", pVer->szVersion);
	}
      }
    }
  } else
  {
    WINE_TRACE("setting dosver to nothing\n");
    SendDlgItemMessage (hDlg, IDC_DOSVER, CB_SETCURSEL,
			-1, 0);
  }

  if(strlen(curWineLook) != 0)
  {
    if ((pVer = getWinelook ()))
    {
      WINE_TRACE("Winelook\n");

      for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
      {
	WINE_TRACE("pVer->szVersion == %s\n", pVer->szVersion);
	if (!strcasecmp (pVer->szVersion, curWineLook))
	{
	  SendDlgItemMessage (hDlg, IDC_WINELOOK, CB_SETCURSEL,
			      (WPARAM) i, 0);
	  WINE_TRACE("match with %s\n", pVer->szVersion);
	}
      }
    }
  } else
  {
    WINE_TRACE("setting winelook to nothing\n");
    SendDlgItemMessage (hDlg, IDC_WINELOOK, CB_SETCURSEL,
			      -1, 0);
  }

  if(curWinVer) free(curWinVer);
  if(curDOSVer) free(curDOSVer);
  if(curWineLook) free(curWineLook);
}

void
initAppDlgComboboxes (HWND hDlg)
{
  int i;
  const VERSION_DESC *pVer = NULL;

  if ((pVer = getWinVersions ()))
  {
    for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
    {
      SendDlgItemMessage (hDlg, IDC_WINVER, CB_ADDSTRING,
			  0, (LPARAM) pVer->szDescription);
    }
  }
  if ((pVer = getDOSVersions ()))
  {
    for (i = 0; *pVer->szVersion || *pVer->szDescription ; i++, pVer++)
    {
      SendDlgItemMessage (hDlg, IDC_DOSVER, CB_ADDSTRING,
			  0, (LPARAM) pVer->szDescription);
    }
  }
  if ((pVer = getWinelook ()))
  {
    for (i = 0; *pVer->szVersion || *pVer->szDescription; i++, pVer++)
    {
      SendDlgItemMessage (hDlg, IDC_WINELOOK, CB_ADDSTRING,
			  0, (LPARAM) pVer->szDescription);
    }
  }
}


static VOID OnInitAppDlg(HWND hDlg)
{
  HWND hwndTV;
  LPAPPL lpAppl;
  HKEY applKey;
  int i;
  DWORD size;
  char appl [255];
  char lpcVersionKey [255];
  char lpcWinelookKey [255];
  FILETIME ft;

  hwndTV = GetDlgItem(hDlg,IDC_APP_TREEVIEW);
  lpAppl = CreateAppl(TRUE, "Global Settings", "Version", "Tweak.Layout");
  LoadAppSettings(lpAppl, hDlg, hwndTV);
	
  /*And now the application specific stuff:*/
  if (RegOpenKey(configKey, "AppDefaults", &applKey) == ERROR_SUCCESS)
  {
    i = 0;
    size = 255;
    while (RegEnumKeyEx(applKey, i, appl, &size, NULL, NULL, NULL, &ft) == ERROR_SUCCESS)
    {
      sprintf(lpcVersionKey, "AppDefaults\\%s\\Version", appl);
      sprintf(lpcWinelookKey, "AppDefaults\\%s\\Tweak.Layout", appl);
      lpAppl = CreateAppl(FALSE, appl, lpcVersionKey, lpcWinelookKey);
      LoadAppSettings(lpAppl, hDlg, hwndTV);
      i++; size = 255;
    }
    RegCloseKey(applKey);
  }
  SetEnabledAppControls(hDlg);
}

static VOID OnTreeViewChangeItem(HWND hDlg, HWND hTV)
{
  TVITEM ti;
  LPITEMTAG lpit;

  ti.mask = TVIF_PARAM;
  ti.hItem = TreeView_GetSelection(hTV);
  if (TreeView_GetItem (hTV, &ti))
  {
    lpit = (LPITEMTAG) ti.lParam;
    if (lpit->lpAppl)
    {
      WINE_TRACE("lpit->lpAppl is non-null\n");
      /* update comboboxes to reflect settings for this app */
      UpdateComboboxes(hDlg, lpit->lpAppl);
    } else
    {
      WINE_TRACE("lpit->lpAppl is null\n");
    }
  }
}

static VOID OnTreeViewDeleteItem(NMTREEVIEW* nmt)
{
  FreeItemTag((LPITEMTAG)(nmt->itemOld.lParam));
}

static VOID OnAddApplicationClick(HWND hDlg)
{
  char szFileTitle [255];
  char szFile [255];
  char lpcVersionKey [255];
  char lpcWinelookKey [255];

  TVINSERTSTRUCT tis;
  LPITEMTAG lpit;
  OPENFILENAME ofn = { sizeof(OPENFILENAME),
		       0, /*hInst*/0, "Wine Programs (*.exe,*.exe.so)\0*.exe;*.exe.so\0", NULL, 0, 0, NULL,
		       0, NULL, 0, NULL, NULL,
		       OFN_SHOWHELP, 0, 0, NULL, 0, NULL };

  ofn.lpstrFileTitle = szFileTitle;
  ofn.lpstrFileTitle[0] = '\0';
  ofn.nMaxFileTitle = sizeof(szFileTitle);
  ofn.lpstrFile = szFile;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);

  if (GetOpenFileName(&ofn))
  {
    tis.hParent = NULL;
    tis.hInsertAfter = TVI_LAST;
    tis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
    tis.u.item.pszText = szFileTitle;
    lpit = CreateItemTag();
    sprintf(lpcVersionKey, "AppDefaults\\%s\\Version", szFileTitle);
    sprintf(lpcWinelookKey, "AppDefaults\\%s\\Tweak.Layout", szFileTitle);
    lpit->lpAppl = CreateAppl(FALSE, szFileTitle, lpcVersionKey, lpcWinelookKey);
    tis.u.item.lParam = (LPARAM)lpit;
    TreeView_InsertItem(GetDlgItem(hDlg, IDC_APP_TREEVIEW), &tis);

    /* add the empty entries for the Version and Winelook sections for this app */
    setConfigValue(lpcVersionKey,NULL,NULL);
    setConfigValue(lpcWinelookKey,NULL,NULL);
  }
}

static VOID OnRemoveApplicationClick(HWND hDlg)
{
  HWND hTV;
  TVITEM ti;
  LPITEMTAG lpit;

  hTV = GetDlgItem(hDlg, IDC_APP_TREEVIEW);
  ti.mask = TVIF_PARAM;
  ti.hItem = TreeView_GetSelection(hTV);
  if (TreeView_GetItem (hTV, &ti))
  {
    lpit = (LPITEMTAG) ti.lParam;
    if (lpit->lpAppl)
    {
      /* add transactions to remove all entries for this application */
      addTransaction(lpit->lpAppl->lpcVersionSection, NULL, ACTION_REMOVE, NULL);
      addTransaction(lpit->lpAppl->lpcWinelookSection, NULL, ACTION_REMOVE, NULL);
      TreeView_DeleteItem(hTV,ti.hItem);
    }
  }
}

static void UpdateWinverSelection(HWND hDlg, int selection)
{
  TVITEM ti;
  LPITEMTAG lpit;
  VERSION_DESC *pVer = NULL;
  HWND hTV = GetDlgItem(hDlg, IDC_APP_TREEVIEW);

  ti.mask = TVIF_PARAM;
  ti.hItem = TreeView_GetSelection(hTV);
  if (TreeView_GetItem (hTV, &ti))
  {
    lpit = (LPITEMTAG) ti.lParam;

    if(lpit->lpAppl)
    {
      pVer = getWinVersions();

      /* if no item is selected OR if our version string is null */
      /* remove this applications setting */
      if((selection == CB_ERR) || !(*pVer[selection].szVersion))
      {
	WINE_TRACE("removing section '%s'\n", lpit->lpAppl->lpcWinelookSection);
	addTransaction(lpit->lpAppl->lpcVersionSection, "Windows", ACTION_REMOVE, NULL); /* change registry entry */
      } else
      {
	WINE_TRACE("setting section '%s', key '%s', value '%s'\n", lpit->lpAppl->lpcVersionSection, "Windows", pVer[selection].szVersion);
	addTransaction(lpit->lpAppl->lpcVersionSection, "Windows", ACTION_SET, pVer[selection].szVersion); /* change registry entry */
      }

      TreeView_DeleteAllItems(hTV); /* delete all items from the treeview */
      OnInitAppDlg(hDlg);
    }
  }
}

static void UpdateDosverSelection(HWND hDlg, int selection)
{
  TVITEM ti;
  LPITEMTAG lpit;
  VERSION_DESC *pVer = NULL;
  HWND hTV = GetDlgItem(hDlg, IDC_APP_TREEVIEW);

  ti.mask = TVIF_PARAM;
  ti.hItem = TreeView_GetSelection(hTV);
  if (TreeView_GetItem (hTV, &ti))
  {
    lpit = (LPITEMTAG) ti.lParam;

    if(lpit->lpAppl)
    {
      pVer = getDOSVersions();

      /* if no item is selected OR if our version string is null */
      /* remove this applications setting */
      if((selection == CB_ERR) || !(*pVer[selection].szVersion))
      {
	WINE_TRACE("removing section '%s'\n", lpit->lpAppl->lpcWinelookSection);
	addTransaction(lpit->lpAppl->lpcVersionSection, "DOS", ACTION_REMOVE, NULL); /* change registry entry */
      } else
      {
	WINE_TRACE("setting section '%s', key '%s', value '%s'\n", lpit->lpAppl->lpcVersionSection, "DOS", pVer[selection].szVersion);
	addTransaction(lpit->lpAppl->lpcVersionSection, "DOS", ACTION_SET, pVer[selection].szVersion); /* change registry entry */
      }

      TreeView_DeleteAllItems(hTV); /* delete all items from the treeview */
      OnInitAppDlg(hDlg);
    }
  }
}

static void UpdateWinelookSelection(HWND hDlg, int selection)
{
  TVITEM ti;
  LPITEMTAG lpit;
  VERSION_DESC *pVer = NULL;
  HWND hTV = GetDlgItem(hDlg, IDC_APP_TREEVIEW);

  ti.mask = TVIF_PARAM;
  ti.hItem = TreeView_GetSelection(hTV);
  if (TreeView_GetItem (hTV, &ti))
  {
    lpit = (LPITEMTAG) ti.lParam;

    if(lpit->lpAppl)
    {
      pVer = getWinelook();

      /* if no item is selected OR if our version string is null */
      /* remove this applications setting */
      if((selection == CB_ERR) || !(*pVer[selection].szVersion))
      {
	WINE_TRACE("removing section '%s'\n", lpit->lpAppl->lpcWinelookSection);
	addTransaction(lpit->lpAppl->lpcWinelookSection, "WineLook", ACTION_REMOVE, NULL); /* change registry entry */
      } else
      {
	WINE_TRACE("setting section '%s', key '%s', value '%s'\n", lpit->lpAppl->lpcWinelookSection, "WineLook", pVer[selection].szVersion);
	addTransaction(lpit->lpAppl->lpcWinelookSection, "WineLook", ACTION_SET, pVer[selection].szVersion); /* change registry entry */
      }

      TreeView_DeleteAllItems(hTV); /* delete all items from the treeview */
      OnInitAppDlg(hDlg);
    }
  }
}



INT_PTR CALLBACK
AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int selection;
  switch (uMsg)
  {
  case WM_INITDIALOG:
    OnInitAppDlg(hDlg);
    initAppDlgComboboxes(hDlg);
    break;  
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code) {
    case TVN_SELCHANGED: {
      switch(LOWORD(wParam)) {
      case IDC_APP_TREEVIEW:
	OnTreeViewChangeItem(hDlg, GetDlgItem(hDlg,IDC_APP_TREEVIEW));
	break;
      }
    }
      break;
    case TVN_DELETEITEM:
      OnTreeViewDeleteItem ((LPNMTREEVIEW)lParam);
      break;
    }
    break;
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case CBN_SELCHANGE:
      switch(LOWORD(wParam)) {
      case IDC_WINVER:
	selection = SendDlgItemMessage( hDlg, IDC_WINVER, CB_GETCURSEL, 0, 0);
	UpdateWinverSelection(hDlg, selection);
	break;
      case IDC_DOSVER:
	selection = SendDlgItemMessage( hDlg, IDC_DOSVER, CB_GETCURSEL, 0, 0);
	UpdateDosverSelection(hDlg, selection);
	break;
      case IDC_WINELOOK:
	selection = SendDlgItemMessage( hDlg, IDC_WINELOOK, CB_GETCURSEL, 0, 0);
	UpdateWinelookSelection(hDlg, selection);
	break;
      }
    case BN_CLICKED:
      switch(LOWORD(wParam)) {
      case IDC_APP_ADDAPP:
	OnAddApplicationClick(hDlg);
	break;
      case IDC_APP_REMOVEAPP:
	OnRemoveApplicationClick(hDlg);
	break;
      }
      break;
    }
    break;
  }

  return 0;
}
