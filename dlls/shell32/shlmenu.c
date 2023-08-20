/*
 * see www.geocities.com/SiliconValley/4942/filemenu.html
 *
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2011 Jay Yang
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
 */

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "shell32_main.h"

#include "pidl.h"
#include "wine/debug.h"
#include "debughlp.h"

/* FileMenu_Create nSelHeight constants */
#define FM_DEFAULT_SELHEIGHT  -1
#define FM_FULL_SELHEIGHT     0

/* FileMenu_Create flags */
#define FMF_SMALL_ICONS      0x00
#define FMF_LARGE_ICONS      0x08
#define FMF_NO_COLUMN_BREAK  0x10

/* FileMenu_AppendItem constants */
#define FM_SEPARATOR       ((const WCHAR *)1)
#define FM_BLANK_ICON      -1
#define FM_DEFAULT_HEIGHT  0

/* FileMenu_InsertUsingPidl flags */
#define FMF_NO_EMPTY_ITEM      0x01
#define FMF_NO_PROGRAM_GROUPS  0x04

/* FileMenu_InsertUsingPidl callback function */
typedef void (CALLBACK *LPFNFMCALLBACK)(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlFile);

static BOOL FileMenu_AppendItemW(HMENU hMenu, LPCWSTR lpText, UINT uID, int icon,
                                 HMENU hMenuPopup, int nItemHeight);
BOOL WINAPI FileMenu_DeleteAllItems(HMENU hMenu);

typedef struct
{
	BOOL		bInitialized;
	BOOL		bFixedItems;
	/* create */
	COLORREF	crBorderColor;
	int		nBorderWidth;
	HBITMAP		hBorderBmp;

	/* insert using pidl */
	LPITEMIDLIST	pidl;
	UINT		uID;
	UINT		uFlags;
	UINT		uEnumFlags;
	LPFNFMCALLBACK lpfnCallback;
} FMINFO, *LPFMINFO;

typedef struct
{	int	cchItemText;
	int	iIconIndex;
	HMENU	hMenu;
	WCHAR	szItemText[1];
} FMITEM, * LPFMITEM;

static BOOL bAbortInit;

#define	CCH_MAXITEMTEXT 256

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static LPFMINFO FM_GetMenuInfo(HMENU hmenu)
{
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return NULL;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;

	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %lu\n", menudata, MenuInfo.cbSize);
	  return 0;
	}

	return menudata;

}
/*************************************************************************
 * FM_SetMenuParameter				[internal]
 *
 */
static LPFMINFO FM_SetMenuParameter(
	HMENU hmenu,
	UINT uID,
	LPCITEMIDLIST pidl,
	UINT uFlags,
	UINT uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	LPFMINFO	menudata;

	TRACE("\n");

	menudata = FM_GetMenuInfo(hmenu);

	SHFree(menudata->pidl);

	menudata->uID = uID;
	menudata->pidl = ILClone(pidl);
	menudata->uFlags = uFlags;
	menudata->uEnumFlags = uEnumFlags;
	menudata->lpfnCallback = lpfnCallback;

	return menudata;
}

/*************************************************************************
 * FM_InitMenuPopup				[internal]
 *
 */
static int FM_InitMenuPopup(HMENU hmenu, LPCITEMIDLIST pAlternatePidl)
{	IShellFolder	*lpsf, *lpsf2;
	ULONG		ulItemAttr = SFGAO_FOLDER;
	UINT		uID, uEnumFlags;
	LPFNFMCALLBACK	lpfnCallback;
	LPCITEMIDLIST	pidl;
	WCHAR		sTemp[MAX_PATH];
	int		NumberOfItems = 0, iIcon;
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	TRACE("%p %p\n", hmenu, pAlternatePidl);

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return FALSE;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;

	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %lu\n", menudata, MenuInfo.cbSize);
	  return 0;
	}

	if (menudata->bInitialized)
	  return 0;

	pidl = (pAlternatePidl? pAlternatePidl: menudata->pidl);
	if (!pidl)
	  return 0;

	uID = menudata->uID;
	uEnumFlags = menudata->uEnumFlags;
	lpfnCallback = menudata->lpfnCallback;
	menudata->bInitialized = FALSE;

	SetMenuInfo(hmenu, &MenuInfo);

	if (SUCCEEDED (SHGetDesktopFolder(&lpsf)))
	{
	  if (SUCCEEDED(IShellFolder_BindToObject(lpsf, pidl,0,&IID_IShellFolder,(LPVOID *)&lpsf2)))
	  {
	    IEnumIDList	*lpe = NULL;

	    if (SUCCEEDED (IShellFolder_EnumObjects(lpsf2, 0, uEnumFlags, &lpe )))
	    {

	      LPITEMIDLIST pidlTemp = NULL;
	      ULONG ulFetched;

	      while ((!bAbortInit) && (S_OK == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched)))
	      {
		if (SUCCEEDED (IShellFolder_GetAttributesOf(lpsf, 1, (LPCITEMIDLIST*)&pidlTemp, &ulItemAttr)))
		{
		  ILGetDisplayNameExW(NULL, pidlTemp, sTemp, ILGDN_FORPARSING);
		  if (! (PidlToSicIndex(lpsf, pidlTemp, FALSE, 0, &iIcon)))
		    iIcon = FM_BLANK_ICON;
		  if ( SFGAO_FOLDER & ulItemAttr)
		  {
		    LPFMINFO lpFmMi;
		    MENUINFO MenuInfo;
		    HMENU hMenuPopup = CreatePopupMenu();

		    lpFmMi = calloc(1, sizeof(*lpFmMi));

		    lpFmMi->pidl = ILCombine(pidl, pidlTemp);
		    lpFmMi->uEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

		    MenuInfo.cbSize = sizeof(MENUINFO);
		    MenuInfo.fMask = MIM_MENUDATA;
		    MenuInfo.dwMenuData = (ULONG_PTR) lpFmMi;
		    SetMenuInfo (hMenuPopup, &MenuInfo);

		    FileMenu_AppendItemW (hmenu, sTemp, uID, iIcon, hMenuPopup, FM_DEFAULT_HEIGHT);
		  }
		  else
		  {
		    LPWSTR pExt = PathFindExtensionW(sTemp);
		    if (pExt)
		      *pExt = 0;
		    FileMenu_AppendItemW (hmenu, sTemp, uID, iIcon, 0, FM_DEFAULT_HEIGHT);
		  }
		}

		if (lpfnCallback)
		{
		  TRACE("enter callback\n");
		  lpfnCallback ( pidl, pidlTemp);
		  TRACE("leave callback\n");
		}

		NumberOfItems++;
	      }
	      IEnumIDList_Release (lpe);
	    }
	    IShellFolder_Release(lpsf2);
	  }
	  IShellFolder_Release(lpsf);
	}

	if ( GetMenuItemCount (hmenu) == 0 )
	{
	  FileMenu_AppendItemW (hmenu, L"(empty)", uID, FM_BLANK_ICON, 0, FM_DEFAULT_HEIGHT);
	  NumberOfItems++;
	}

	menudata->bInitialized = TRUE;
	SetMenuInfo(hmenu, &MenuInfo);

	return NumberOfItems;
}
/*************************************************************************
 * FileMenu_Create				[SHELL32.114]
 *
 * NOTES
 *  for non-root menus values are
 *  (ffffffff,00000000,00000000,00000000,00000000)
 */
HMENU WINAPI FileMenu_Create (
	COLORREF crBorderColor,
	int nBorderWidth,
	HBITMAP hBorderBmp,
	int nSelHeight,
	UINT uFlags)
{
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	HMENU hMenu = CreatePopupMenu();

	TRACE("0x%08lx 0x%08x %p 0x%08x 0x%08x  hMenu=%p\n",
	crBorderColor, nBorderWidth, hBorderBmp, nSelHeight, uFlags, hMenu);

	menudata = calloc(1, sizeof(*menudata));
	menudata->crBorderColor = crBorderColor;
	menudata->nBorderWidth = nBorderWidth;
	menudata->hBorderBmp = hBorderBmp;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;
	MenuInfo.dwMenuData = (ULONG_PTR) menudata;
	SetMenuInfo (hMenu, &MenuInfo);

	return hMenu;
}

/*************************************************************************
 * FileMenu_Destroy				[SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
void WINAPI FileMenu_Destroy (HMENU hmenu)
{
	LPFMINFO	menudata;

	TRACE("%p\n", hmenu);

	FileMenu_DeleteAllItems (hmenu);

	menudata = FM_GetMenuInfo(hmenu);

	SHFree( menudata->pidl);
	free(menudata);

	DestroyMenu (hmenu);
}

/*************************************************************************
 * FileMenu_AppendItem			[SHELL32.115]
 *
 */
static BOOL FileMenu_AppendItemW(
	HMENU hMenu,
	LPCWSTR lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	MENUITEMINFOW	mii;
	LPFMITEM	myItem;
	LPFMINFO	menudata;
	MENUINFO        MenuInfo;


	TRACE("%p %s 0x%08x 0x%08x %p 0x%08x\n",
	  hMenu, (lpText!=FM_SEPARATOR) ? debugstr_w(lpText) : NULL,
	  uID, icon, hMenuPopup, nItemHeight);

	ZeroMemory (&mii, sizeof(MENUITEMINFOW));

	mii.cbSize = sizeof(MENUITEMINFOW);

	if (lpText != FM_SEPARATOR)
	{
	  int len = lstrlenW (lpText);
          myItem = SHAlloc(sizeof(FMITEM) + len*sizeof(WCHAR));
	  lstrcpyW (myItem->szItemText, lpText);
	  myItem->cchItemText = len;
	  myItem->iIconIndex = icon;
	  myItem->hMenu = hMenu;
	  mii.fMask = MIIM_DATA;
	  mii.dwItemData = (ULONG_PTR) myItem;
	}

	if ( hMenuPopup )
	{ /* sub menu */
	  mii.fMask |= MIIM_TYPE | MIIM_SUBMENU;
	  mii.fType = MFT_OWNERDRAW;
	  mii.hSubMenu = hMenuPopup;
	}
	else if (lpText == FM_SEPARATOR )
	{ mii.fMask |= MIIM_ID | MIIM_TYPE;
	  mii.fType = MFT_SEPARATOR;
	}
	else
	{ /* normal item */
	  mii.fMask |= MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.fState = MFS_ENABLED | MFS_DEFAULT;
	  mii.fType = MFT_OWNERDRAW;
	}
	mii.wID = uID;

	InsertMenuItemW (hMenu, (UINT)-1, TRUE, &mii);

	/* set bFixedItems to true */
	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hMenu, &MenuInfo))
	  return FALSE;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;
	if ((menudata == 0) || (MenuInfo.cbSize != sizeof(MENUINFO)))
	{
	  ERR("menudata corrupt: %p %lu\n", menudata, MenuInfo.cbSize);
	  return FALSE;
	}

	menudata->bFixedItems = TRUE;
	SetMenuInfo(hMenu, &MenuInfo);

	return TRUE;

}

/**********************************************************************/

BOOL WINAPI FileMenu_AppendItemAW(
	HMENU hMenu,
	LPCVOID lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	BOOL ret;

        if (!lpText) return FALSE;

	if (SHELL_OsIsUnicode() || lpText == FM_SEPARATOR)
	  ret = FileMenu_AppendItemW(hMenu, lpText, uID, icon, hMenuPopup, nItemHeight);
        else
	{
	  DWORD len = MultiByteToWideChar( CP_ACP, 0, lpText, -1, NULL, 0 );
	  WCHAR *lpszText = malloc(len * sizeof(WCHAR));
	  if (!lpszText) return FALSE;
	  MultiByteToWideChar( CP_ACP, 0, lpText, -1, lpszText, len );
	  ret = FileMenu_AppendItemW(hMenu, lpszText, uID, icon, hMenuPopup, nItemHeight);
	  free(lpszText);
	}

	return ret;
}

/*************************************************************************
 * FileMenu_InsertUsingPidl			[SHELL32.110]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_InsertUsingPidl (
	HMENU hmenu,
	UINT uID,
	LPCITEMIDLIST pidl,
	UINT uFlags,
	UINT uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	TRACE("%p 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	pdump (pidl);

	bAbortInit = FALSE;

	FM_SetMenuParameter(hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	return FM_InitMenuPopup(hmenu, NULL);
}

/*************************************************************************
 * FileMenu_ReplaceUsingPidl			[SHELL32.113]
 *
 * FIXME: the static items are deleted but won't be refreshed
 */
int WINAPI FileMenu_ReplaceUsingPidl(
	HMENU	hmenu,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	TRACE("%p 0x%08x %p 0x%08x %p\n",
	hmenu, uID, pidl, uEnumFlags, lpfnCallback);

	FileMenu_DeleteAllItems (hmenu);

	FM_SetMenuParameter(hmenu, uID, pidl, 0, uEnumFlags, lpfnCallback);

	return FM_InitMenuPopup(hmenu, NULL);
}

/*************************************************************************
 * FileMenu_Invalidate			[SHELL32.111]
 */
void WINAPI FileMenu_Invalidate (HMENU hMenu)
{
	FIXME("%p\n",hMenu);
}

/*************************************************************************
 * FileMenu_FindSubMenuByPidl			[SHELL32.106]
 */
HMENU WINAPI FileMenu_FindSubMenuByPidl(
	HMENU	hMenu,
	LPCITEMIDLIST	pidl)
{
	FIXME("%p %p\n",hMenu, pidl);
	return 0;
}

/*************************************************************************
 * FileMenu_AppendFilesForPidl			[SHELL32.124]
 */
int WINAPI FileMenu_AppendFilesForPidl(
	HMENU	hmenu,
	LPCITEMIDLIST	pidl,
	BOOL	bAddSeparator)
{
	LPFMINFO	menudata;

	menudata = FM_GetMenuInfo(hmenu);

	menudata->bInitialized = FALSE;

	FM_InitMenuPopup(hmenu, pidl);

	if (bAddSeparator)
	  FileMenu_AppendItemW (hmenu, FM_SEPARATOR, 0, 0, 0, FM_DEFAULT_HEIGHT);

	TRACE("%p %p 0x%08x\n",hmenu, pidl,bAddSeparator);

	return 0;
}
/*************************************************************************
 * FileMenu_AddFilesForPidl			[SHELL32.125]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_AddFilesForPidl (
	HMENU	hmenu,
	UINT	uReserved,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uFlags,
	UINT	uEnumFlags,
	LPFNFMCALLBACK	lpfnCallback)
{
	TRACE("%p 0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uReserved, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	return FileMenu_InsertUsingPidl ( hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

}


/*************************************************************************
 * FileMenu_TrackPopupMenuEx			[SHELL32.116]
 */
BOOL WINAPI FileMenu_TrackPopupMenuEx (
	HMENU hMenu,
	UINT uFlags,
	int x,
	int y,
	HWND hWnd,
	LPTPMPARAMS lptpm)
{
	TRACE("%p 0x%08x 0x%x 0x%x %p %p\n",
	hMenu, uFlags, x, y, hWnd, lptpm);
	return TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, lptpm);
}

/*************************************************************************
 * FileMenu_GetLastSelectedItemPidls		[SHELL32.107]
 */
BOOL WINAPI FileMenu_GetLastSelectedItemPidls(
	UINT	uReserved,
	LPCITEMIDLIST	*ppidlFolder,
	LPCITEMIDLIST	*ppidlItem)
{
	FIXME("0x%08x %p %p\n",uReserved, ppidlFolder, ppidlItem);
	return FALSE;
}

#define FM_ICON_SIZE	16
#define FM_Y_SPACE	4
#define FM_SPACE1	4
#define FM_SPACE2	2
#define FM_LEFTBORDER	2
#define FM_RIGHTBORDER	8
/*************************************************************************
 * FileMenu_MeasureItem				[SHELL32.112]
 */
LRESULT WINAPI FileMenu_MeasureItem(
	HWND	hWnd,
	LPMEASUREITEMSTRUCT	lpmis)
{
	LPFMITEM pMyItem = (LPFMITEM)(lpmis->itemData);
	HDC hdc = GetDC(hWnd);
	SIZE size;
	LPFMINFO menuinfo;

	TRACE("%p %p %s\n", hWnd, lpmis, debugstr_w(pMyItem->szItemText));

	GetTextExtentPoint32W(hdc, pMyItem->szItemText, pMyItem->cchItemText, &size);

	lpmis->itemWidth = size.cx + FM_LEFTBORDER + FM_ICON_SIZE + FM_SPACE1 + FM_SPACE2 + FM_RIGHTBORDER;
	lpmis->itemHeight = (size.cy > (FM_ICON_SIZE + FM_Y_SPACE)) ? size.cy : (FM_ICON_SIZE + FM_Y_SPACE);

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->nBorderWidth)
	  lpmis->itemWidth += menuinfo->nBorderWidth;

	TRACE("-- 0x%04x 0x%04x\n", lpmis->itemWidth, lpmis->itemHeight);
	ReleaseDC (hWnd, hdc);
	return 0;
}
/*************************************************************************
 * FileMenu_DrawItem				[SHELL32.105]
 */
LRESULT WINAPI FileMenu_DrawItem(
	HWND			hWnd,
	LPDRAWITEMSTRUCT	lpdis)
{
	LPFMITEM pMyItem = (LPFMITEM)(lpdis->itemData);
	COLORREF clrPrevText, clrPrevBkgnd;
	int xi,yi,xt,yt;
	HIMAGELIST hImageList;
	RECT TextRect;
	LPFMINFO menuinfo;

	TRACE("%p %p %s\n", hWnd, lpdis, debugstr_w(pMyItem->szItemText));

	if (lpdis->itemState & ODS_SELECTED)
	{
	  clrPrevText = SetTextColor(lpdis->hDC, GetSysColor (COLOR_HIGHLIGHTTEXT));
	  clrPrevBkgnd = SetBkColor(lpdis->hDC, GetSysColor (COLOR_HIGHLIGHT));
	}
	else
	{
	  clrPrevText = SetTextColor(lpdis->hDC, GetSysColor (COLOR_MENUTEXT));
	  clrPrevBkgnd = SetBkColor(lpdis->hDC, GetSysColor (COLOR_MENU));
	}

        TextRect = lpdis->rcItem;

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->nBorderWidth)
	  TextRect.left += menuinfo->nBorderWidth;

	TextRect.left += FM_LEFTBORDER;
	xi = TextRect.left + FM_SPACE1;
	yi = TextRect.top + FM_Y_SPACE/2;
	TextRect.bottom -= FM_Y_SPACE/2;

	xt = xi + FM_ICON_SIZE + FM_SPACE2;
	yt = yi;

	ExtTextOutW (lpdis->hDC, xt , yt, ETO_OPAQUE, &TextRect, pMyItem->szItemText, pMyItem->cchItemText, NULL);

	Shell_GetImageLists(0, &hImageList);
	ImageList_Draw(hImageList, pMyItem->iIconIndex, lpdis->hDC, xi, yi, ILD_NORMAL);

        TRACE("-- %s\n", wine_dbgstr_rect(&TextRect));

	SetTextColor(lpdis->hDC, clrPrevText);
	SetBkColor(lpdis->hDC, clrPrevBkgnd);

	return TRUE;
}

/*************************************************************************
 * FileMenu_InitMenuPopup			[SHELL32.109]
 *
 * NOTES
 *  The filemenu is an ownerdrawn menu. Call this function responding to
 *  WM_INITPOPUPMENU
 *
 */
BOOL WINAPI FileMenu_InitMenuPopup (HMENU hmenu)
{
	FM_InitMenuPopup(hmenu, NULL);
	return TRUE;
}

/*************************************************************************
 * FileMenu_HandleMenuChar			[SHELL32.108]
 */
LRESULT WINAPI FileMenu_HandleMenuChar(
	HMENU	hMenu,
	WPARAM	wParam)
{
	FIXME("%p 0x%08Ix\n",hMenu,wParam);
	return 0;
}

/*************************************************************************
 * FileMenu_DeleteAllItems			[SHELL32.104]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI FileMenu_DeleteAllItems (HMENU hmenu)
{
	MENUITEMINFOW	mii;
	LPFMINFO	menudata;

	int i;

	TRACE("%p\n", hmenu);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU|MIIM_DATA;

	for (i = 0; i < GetMenuItemCount( hmenu ); i++)
	{ GetMenuItemInfoW(hmenu, i, TRUE, &mii );

	  SHFree((LPFMINFO)mii.dwItemData);

	  if (mii.hSubMenu)
	    FileMenu_Destroy(mii.hSubMenu);
	}

	while (DeleteMenu (hmenu, 0, MF_BYPOSITION)){};

	menudata = FM_GetMenuInfo(hmenu);

	menudata->bInitialized = FALSE;

	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByCmd 			[SHELL32.117]
 *
 */
BOOL WINAPI FileMenu_DeleteItemByCmd (HMENU hMenu, UINT uID)
{
	MENUITEMINFOW mii;

	TRACE("%p 0x%08x\n", hMenu, uID);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoW(hMenu, uID, FALSE, &mii );
	if ( mii.hSubMenu )
	{
	  /* FIXME: Do what? */
	}

	DeleteMenu(hMenu, MF_BYCOMMAND, uID);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByIndex			[SHELL32.140]
 */
BOOL WINAPI FileMenu_DeleteItemByIndex ( HMENU hMenu, UINT uPos)
{
	MENUITEMINFOW mii;

	TRACE("%p 0x%08x\n", hMenu, uPos);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOW));
	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoW(hMenu, uPos, TRUE, &mii );
	if ( mii.hSubMenu )
	{
	  /* FIXME: Do what? */
	}

	DeleteMenu(hMenu, MF_BYPOSITION, uPos);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByFirstID			[SHELL32.141]
 */
BOOL WINAPI FileMenu_DeleteItemByFirstID(
	HMENU	hMenu,
	UINT	uID)
{
	TRACE("%p 0x%08x\n", hMenu, uID);
	return FALSE;
}

/*************************************************************************
 * FileMenu_DeleteSeparator			[SHELL32.142]
 */
BOOL WINAPI FileMenu_DeleteSeparator(HMENU hMenu)
{
	TRACE("%p\n", hMenu);
	return FALSE;
}

/*************************************************************************
 * FileMenu_EnableItemByCmd			[SHELL32.143]
 */
BOOL WINAPI FileMenu_EnableItemByCmd(
	HMENU	hMenu,
	UINT	uID,
	BOOL	bEnable)
{
	TRACE("%p 0x%08x 0x%08x\n", hMenu, uID,bEnable);
	return FALSE;
}

/*************************************************************************
 * FileMenu_GetItemExtent			[SHELL32.144]
 *
 * NOTES
 *  if the menu is too big, entries are getting cut away!!
 */
DWORD WINAPI FileMenu_GetItemExtent (HMENU hMenu, UINT uPos)
{	RECT rect;

	FIXME("%p 0x%08x\n", hMenu, uPos);

	if (GetMenuItemRect(0, hMenu, uPos, &rect))
        {
          FIXME("%s\n", wine_dbgstr_rect(&rect));
	  return ((rect.right-rect.left)<<16) + (rect.top-rect.bottom);
	}
	return 0x00100010; /*FIXME*/
}

/*************************************************************************
 * FileMenu_AbortInitMenu 			[SHELL32.120]
 *
 */
void WINAPI FileMenu_AbortInitMenu (void)
{	TRACE("\n");
	bAbortInit = TRUE;
}

/*************************************************************************
 * SHFind_InitMenuPopup				[SHELL32.149]
 *
 * Get the IContextMenu instance for the submenu of options displayed
 * for the Search entry in the Classic style Start menu.
 *
 * PARAMETERS
 *  hMenu		[in] handle of menu previously created
 *  hWndParent	[in] parent window
 *  w			[in] no pointer (0x209 over here) perhaps menu IDs ???
 *  x			[in] no pointer (0x226 over here)
 *
 * RETURNS
 *  LPXXXXX			 pointer to struct containing a func addr at offset 8
 *					 or NULL at failure.
 */
LPVOID WINAPI SHFind_InitMenuPopup (HMENU hMenu, HWND hWndParent, DWORD w, DWORD x)
{
	FIXME("hmenu=%p hwnd=%p 0x%08lx 0x%08lx stub\n",
		hMenu,hWndParent,w,x);
	return NULL; /* this is supposed to be a pointer */
}

/*************************************************************************
 * _SHIsMenuSeparator   (internal)
 */
static BOOL _SHIsMenuSeparator(HMENU hm, int i)
{
	MENUITEMINFOW mii;

	mii.cbSize = sizeof(MENUITEMINFOW);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    /* WARNING: We MUST initialize it to 0*/
	if (!GetMenuItemInfoW(hm, i, TRUE, &mii))
	{
	  return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{
	  return(TRUE);
	}

	return(FALSE);
}

/*************************************************************************
 * Shell_MergeMenus				[SHELL32.67]
 */
UINT WINAPI Shell_MergeMenus (HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{	int		nItem;
	HMENU		hmSubMenu;
	BOOL		bAlreadySeparated;
	MENUITEMINFOW	miiSrc;
	WCHAR		szName[256];
	UINT		uTemp, uIDMax = uIDAdjust;

	TRACE("hmenu1=%p hmenu2=%p 0x%04x 0x%04x 0x%04x  0x%04lx\n",
		 hmDst, hmSrc, uInsert, uIDAdjust, uIDAdjustMax, uFlags);

	if (!hmDst || !hmSrc)
	  return uIDMax;

	nItem = GetMenuItemCount(hmDst);
        if (nItem == -1)
	  return uIDMax;

	if (uInsert >= (UINT)nItem)	/* insert position inside menu? */
	{
	  uInsert = (UINT)nItem;	/* append on the end */
	  bAlreadySeparated = TRUE;
	}
	else
	{
	  bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);
	}

	if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	{
	  /* Add a separator between the menus */
	  InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	  bAlreadySeparated = TRUE;
	}


	/* Go through the menu items and clone them*/
	for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
	{
	  miiSrc.cbSize = sizeof(MENUITEMINFOW);
	  miiSrc.fMask =  MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;

	  /* We need to reset this every time through the loop in case menus DON'T have IDs*/
	  miiSrc.fType = MFT_STRING;
	  miiSrc.dwTypeData = szName;
	  miiSrc.dwItemData = 0;
	  miiSrc.cch = ARRAY_SIZE(szName);

	  if (!GetMenuItemInfoW(hmSrc, nItem, TRUE, &miiSrc))
	  {
	    continue;
	  }

/*	  TRACE("found menu=0x%04x %s id=0x%04x mask=0x%08x smenu=0x%04x\n", hmSrc, debugstr_a(miiSrc.dwTypeData), miiSrc.wID, miiSrc.fMask,  miiSrc.hSubMenu);
*/
	  if (miiSrc.fType & MFT_SEPARATOR)
	  {
	    /* This is a separator; don't put two of them in a row */
	    if (bAlreadySeparated)
	      continue;

	    bAlreadySeparated = TRUE;
	  }
	  else if (miiSrc.hSubMenu)
	  {
	    if (uFlags & MM_SUBMENUSHAVEIDS)
	    {
	      miiSrc.wID += uIDAdjust;			/* add uIDAdjust to the ID */

	      if (miiSrc.wID > uIDAdjustMax)		/* skip IDs higher than uIDAdjustMax */
	        continue;

	      if (uIDMax <= miiSrc.wID)			/* remember the highest ID */
	        uIDMax = miiSrc.wID + 1;
	    }
	    else
	    {
	      miiSrc.fMask &= ~MIIM_ID;			/* Don't set IDs for submenus that didn't have them already */
	    }
	    hmSubMenu = miiSrc.hSubMenu;

	    miiSrc.hSubMenu = CreatePopupMenu();

	    if (!miiSrc.hSubMenu) return(uIDMax);

	    uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust, uIDAdjustMax, uFlags & MM_SUBMENUSHAVEIDS);

	    if (uIDMax <= uTemp)
	      uIDMax = uTemp;

	    bAlreadySeparated = FALSE;
	  }
	  else						/* normal menu item */
	  {
	    miiSrc.wID += uIDAdjust;			/* add uIDAdjust to the ID */

	    if (miiSrc.wID > uIDAdjustMax)		/* skip IDs higher than uIDAdjustMax */
	      continue;

	    if (uIDMax <= miiSrc.wID)			/* remember the highest ID */
	      uIDMax = miiSrc.wID + 1;

	    bAlreadySeparated = FALSE;
	  }

/*	  TRACE("inserting menu=0x%04x %s id=0x%04x mask=0x%08x smenu=0x%04x\n", hmDst, debugstr_a(miiSrc.dwTypeData), miiSrc.wID, miiSrc.fMask, miiSrc.hSubMenu);
*/
	  if (!InsertMenuItemW(hmDst, uInsert, TRUE, &miiSrc))
	  {
	    return(uIDMax);
	  }
	}

	/* Ensure the correct number of separators at the beginning of the
	inserted menu items*/
	if (uInsert == 0)
	{
	  if (bAlreadySeparated)
	  {
	    DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	  }
	}
	else
	{
	  if (_SHIsMenuSeparator(hmDst, uInsert-1))
	  {
	    if (bAlreadySeparated)
	    {
	      DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	    }
	  }
	  else
	  {
	    if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	    {
	      /* Add a separator between the menus*/
	      InsertMenuW(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	    }
	  }
	}
	return(uIDMax);
}

typedef struct
{
    IContextMenu3 IContextMenu3_iface;
    IContextMenu **menus;
    UINT *offsets;
    UINT menu_count;
    ULONG refCount;
}CompositeCMenu;

static const IContextMenu3Vtbl CompositeCMenuVtbl;

static CompositeCMenu* impl_from_IContextMenu3(IContextMenu3* iface)
{
    return CONTAINING_RECORD(iface, CompositeCMenu, IContextMenu3_iface);
}

static HRESULT CompositeCMenu_Constructor(IContextMenu **menus,UINT menu_count, REFIID riid, void **ppv)
{
    CompositeCMenu *ret;
    UINT i;

    TRACE("(%p,%u,%s,%p)\n",menus,menu_count,shdebugstr_guid(riid),ppv);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;
    ret->IContextMenu3_iface.lpVtbl = &CompositeCMenuVtbl;
    ret->menu_count = menu_count;
    ret->menus = malloc(menu_count * sizeof(IContextMenu*));
    if(!ret->menus)
    {
        free(ret);
        return E_OUTOFMEMORY;
    }
    ret->offsets = calloc(menu_count, sizeof(UINT));
    if(!ret->offsets)
    {
        free(ret->menus);
        free(ret);
        return E_OUTOFMEMORY;
    }
    ret->refCount=0;
    memcpy(ret->menus,menus,menu_count*sizeof(IContextMenu*));
    for(i=0;i<menu_count;i++)
        IContextMenu_AddRef(menus[i]);
    return IContextMenu3_QueryInterface(&(ret->IContextMenu3_iface),riid,ppv);
}

static void CompositeCMenu_Destroy(CompositeCMenu *This)
{
    UINT i;
    for(i=0;i<This->menu_count;i++)
        IContextMenu_Release(This->menus[i]);
    free(This->menus);
    free(This->offsets);
    free(This);
}

static HRESULT WINAPI CompositeCMenu_QueryInterface(IContextMenu3 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s,%p)\n",iface,shdebugstr_guid(riid),ppv);
    if(!ppv)
        return E_INVALIDARG;
    if(IsEqualIID(riid,&IID_IUnknown) || IsEqualIID(riid,&IID_IContextMenu) ||
       IsEqualIID(riid,&IID_IContextMenu2) || IsEqualIID(riid,&IID_IContextMenu3))
        *ppv=iface;
    else
        return E_NOINTERFACE;
    IContextMenu3_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI CompositeCMenu_AddRef(IContextMenu3 *iface)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    TRACE("(%p)->()\n",iface);
    return ++This->refCount;
}

static ULONG WINAPI CompositeCMenu_Release(IContextMenu3 *iface)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    TRACE("(%p)->()\n",iface);
    if(--This->refCount)
        return This->refCount;
    CompositeCMenu_Destroy(This);
    return 0;
}

static UINT CompositeCMenu_GetIndexForCommandId(CompositeCMenu *This,UINT id)
{
    UINT low=0;
    UINT high=This->menu_count;
    while(high-low!=1)
    {
        UINT i=(high+low)/2;
        if(This->offsets[i]<=id)
            low=i;
        else
            high=i;
    }
    return low;
}

static HRESULT WINAPI CompositeCMenu_GetCommandString(IContextMenu3* iface, UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    UINT index = CompositeCMenu_GetIndexForCommandId(This,idCmd);
    TRACE("(%p)->(%Ix,%x,%p,%s,%u)\n",iface,idCmd,uFlags,pwReserved,pszName,cchMax);
    return IContextMenu_GetCommandString(This->menus[index],idCmd,uFlags,pwReserved,pszName,cchMax);
}

static HRESULT WINAPI CompositeCMenu_InvokeCommand(IContextMenu3* iface,LPCMINVOKECOMMANDINFO pici)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);

    TRACE("(%p)->(%p)\n", iface, pici);

    if (IS_INTRESOURCE(pici->lpVerb))
    {
        UINT id = (UINT_PTR)pici->lpVerb;
        UINT index = CompositeCMenu_GetIndexForCommandId(This, id);
        return IContextMenu_InvokeCommand(This->menus[index], pici);
    }
    else
    {
        /*call each handler until one of them succeeds*/
        UINT i;

        for (i = 0; i < This->menu_count; i++)
        {
            HRESULT hres;
            if (SUCCEEDED(hres = IContextMenu_InvokeCommand(This->menus[i], pici)))
                return hres;
        }
        return E_FAIL;
    }
}

static HRESULT WINAPI CompositeCMenu_QueryContextMenu(IContextMenu3 *iface, HMENU hmenu,UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    UINT i=0;
    UINT id_offset=idCmdFirst;
    TRACE("(%p)->(%p,%u,%u,%u,%x)\n",iface,hmenu,indexMenu,idCmdFirst,idCmdLast,uFlags);
    for(;i<This->menu_count;i++)
    {
        HRESULT hres;
        This->offsets[i]=id_offset;
        hres = IContextMenu_QueryContextMenu(This->menus[i],hmenu,indexMenu,id_offset,idCmdLast,uFlags);
        if(SUCCEEDED(hres))
            id_offset+=hres;
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, id_offset-idCmdFirst);
}

static HRESULT WINAPI CompositeCMenu_HandleMenuMsg(IContextMenu3 *iface, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    HMENU menu;
    UINT id;
    UINT index;
    IContextMenu2 *handler;
    HRESULT hres;
    TRACE("(%p)->(%x,%Ix,%Ix)\n",iface,uMsg,wParam,lParam);
    switch(uMsg)
    {
    case WM_INITMENUPOPUP:
        menu = (HMENU)wParam;
        id = GetMenuItemID(menu,LOWORD(lParam));
        break;
    case WM_DRAWITEM:
        id = ((DRAWITEMSTRUCT*)lParam)->itemID;
        break;
    case WM_MEASUREITEM:
        id = ((MEASUREITEMSTRUCT*)lParam)->itemID;
        break;
    default:
        WARN("Unimplemented uMsg: 0x%x\n",uMsg);
        return E_NOTIMPL;
    }
    index = CompositeCMenu_GetIndexForCommandId(This,id);
    hres = IContextMenu_QueryInterface(This->menus[index],&IID_IContextMenu2,
                                       (void**)&handler);
    if(SUCCEEDED(hres))
        return IContextMenu2_HandleMenuMsg(handler,uMsg,wParam,lParam);
    return S_OK;
}

static HRESULT WINAPI CompositeCMenu_HandleMenuMsg2(IContextMenu3 *iface, UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *plResult)
{
    CompositeCMenu *This = impl_from_IContextMenu3(iface);
    HMENU menu;
    UINT id;
    UINT index;
    IContextMenu3 *handler;
    HRESULT hres;
    LRESULT lres;
    TRACE("(%p)->(%x,%Ix,%Ix,%p)\n",iface,uMsg,wParam,lParam,plResult);
    if(!plResult)
        plResult=&lres;
    switch(uMsg)
    {
    case WM_INITMENUPOPUP:
        menu = (HMENU)wParam;
        id = GetMenuItemID(menu,LOWORD(lParam));
        break;
    case WM_DRAWITEM:
        id = ((DRAWITEMSTRUCT*)lParam)->itemID;
        break;
    case WM_MEASUREITEM:
        id = ((MEASUREITEMSTRUCT*)lParam)->itemID;
        break;
    case WM_MENUCHAR:
        {
            UINT i=0;
            for(;i<This->menu_count;i++)
            {
                hres = IContextMenu_QueryInterface(This->menus[i],&IID_IContextMenu3,(void**)&handler);
                if(SUCCEEDED(hres))
                {
                    hres = IContextMenu3_HandleMenuMsg2(handler,uMsg,wParam,lParam,plResult);
                    if(SUCCEEDED(hres) && HIWORD(*plResult))
                        return hres;
                }
            }
        }
    default:
        WARN("Unimplemented uMsg: 0x%x\n",uMsg);
        return E_NOTIMPL;
    }
    index = CompositeCMenu_GetIndexForCommandId(This,id);
    hres = IContextMenu_QueryInterface(This->menus[index],&IID_IContextMenu3,(void**)&handler);
    if(SUCCEEDED(hres))
        return IContextMenu3_HandleMenuMsg2(handler,uMsg,wParam,lParam,plResult);
    return S_OK;
}

static const IContextMenu3Vtbl CompositeCMenuVtbl=
{
    CompositeCMenu_QueryInterface,
    CompositeCMenu_AddRef,
    CompositeCMenu_Release,
    CompositeCMenu_QueryContextMenu,
    CompositeCMenu_InvokeCommand,
    CompositeCMenu_GetCommandString,
    CompositeCMenu_HandleMenuMsg,
    CompositeCMenu_HandleMenuMsg2
};

static HRESULT SHELL_CreateContextMenu(HWND hwnd, IContextMenu* system_menu,
                                IShellFolder *folder, LPCITEMIDLIST folder_pidl,
                                LPCITEMIDLIST *apidl, UINT cidl, const HKEY *aKeys,
                                UINT cKeys,REFIID riid, void** ppv)
{
    HRESULT ret;
    TRACE("(%p,%p,%p,%p,%p,%u,%p,%u,%s,%p)\n",hwnd,system_menu,folder,folder_pidl,apidl,cidl,aKeys,cKeys,shdebugstr_guid(riid),ppv);
    ret = CompositeCMenu_Constructor(&system_menu,1,riid,ppv);
    return ret;
}

HRESULT WINAPI CDefFolderMenu_Create2(LPCITEMIDLIST pidlFolder, HWND hwnd, UINT cidl,
                                      LPCITEMIDLIST *apidl, IShellFolder *psf,
                                      LPFNDFMCALLBACK lpfn, UINT nKeys, const HKEY *ahkeys,
                                      IContextMenu **ppcm)
{
    IContextMenu *system_menu;
    HRESULT hres;
    LPITEMIDLIST folder_pidl;
    TRACE("(%p,%p,%u,%p,%p,%u,%p,%p)\n",pidlFolder,hwnd,cidl,apidl,psf,nKeys,ahkeys,ppcm);
    if(!pidlFolder)
    {
        IPersistFolder2 *persist;
        IShellFolder_QueryInterface(psf,&IID_IPersistFolder2,(void**)&persist);
        IPersistFolder2_GetCurFolder(persist,&folder_pidl);
        IPersistFolder2_Release(persist);
    }
    else
        folder_pidl=ILClone(pidlFolder);

    ItemMenu_Constructor(psf, folder_pidl, (const LPCITEMIDLIST*)apidl, cidl, &IID_IContextMenu, (void**)&system_menu);
    hres= SHELL_CreateContextMenu(hwnd,system_menu,psf,folder_pidl,apidl,cidl,ahkeys,nKeys,&IID_IContextMenu,(void**)ppcm);
    IContextMenu_Release(system_menu);
    ILFree(folder_pidl);
    return hres;
}

HRESULT WINAPI SHCreateDefaultContextMenu(const DEFCONTEXTMENU *pdcm, REFIID riid, void **ppv)
{
    IShellFolder *folder=pdcm->psf;
    LPITEMIDLIST folder_pidl;
    HRESULT ret;
    IContextMenu *system_menu;
    TRACE("(%p,%s,%p)\n",pdcm,shdebugstr_guid(riid),ppv);
    if(!pdcm->pidlFolder)
    {
        IPersistFolder2 *persist;
        IShellFolder_QueryInterface(folder,&IID_IPersistFolder2,(void**)&persist);
        IPersistFolder2_GetCurFolder(persist,&folder_pidl);
        IPersistFolder2_Release(persist);
    }
    else
        folder_pidl=ILClone(pdcm->pidlFolder);
    if(pdcm->cKeys==0)
        FIXME("Loading shell extensions using IQueryAssociations not yet supported\n");

    ItemMenu_Constructor(folder, folder_pidl, (const LPCITEMIDLIST*)pdcm->apidl, pdcm->cidl, &IID_IContextMenu, (void**)&system_menu);
    ret = SHELL_CreateContextMenu(pdcm->hwnd,system_menu,folder,folder_pidl,(LPCITEMIDLIST*)pdcm->apidl,pdcm->cidl,pdcm->aKeys,pdcm->cKeys,riid,ppv);
    IContextMenu_Release(system_menu);
    ILFree(folder_pidl);
    return ret;
}
