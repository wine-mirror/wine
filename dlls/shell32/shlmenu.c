/*
 * see www.geocities.com/SiliconValley/4942/filemenu.html
 */
#include <assert.h>
#include <string.h>

#include "wine/obj_base.h"
#include "wine/obj_enumidlist.h"
#include "wine/obj_shellfolder.h"

#include "heap.h"
#include "debug.h"
#include "winversion.h"
#include "shell32_main.h"

#include "pidl.h"

BOOL WINAPI FileMenu_DeleteAllItems (HMENU hMenu);
BOOL WINAPI FileMenu_AppendItemA(HMENU hMenu, LPCSTR lpText, UINT uID, int icon, HMENU hMenuPopup, int nItemHeight);

typedef struct
{	BOOL		bInitialized;
	BOOL		bIsMagic;

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
	char	szItemText[1];
} FMITEM, * LPFMITEM;

static BOOL bAbortInit;

#define	CCH_MAXITEMTEXT 256

DEFAULT_DEBUG_CHANNEL(shell)

LPFMINFO FM_GetMenuInfo(HMENU hmenu)
{	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return NULL;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;

	assert ((menudata != 0) && (MenuInfo.cbSize == sizeof(MENUINFO)));
	
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

	TRACE(shell,"\n");
	
	menudata = FM_GetMenuInfo(hmenu);
	
	if ( menudata->pidl)
	{ SHFree(menudata->pidl);
	}
	
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
static int FM_InitMenuPopup(HMENU hmenu, LPITEMIDLIST pAlternatePidl)
{	IShellFolder	*lpsf, *lpsf2;
	ULONG		ulItemAttr;
	UINT		uID, uFlags, uEnumFlags;
	LPFNFMCALLBACK	lpfnCallback;
	LPITEMIDLIST	pidl;
	char		sTemp[MAX_PATH];
	int		NumberOfItems = 0, iIcon;
	MENUINFO	MenuInfo;
	LPFMINFO	menudata;

	TRACE(shell,"\n");

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;

	if (! GetMenuInfo(hmenu, &MenuInfo))
	  return FALSE;

	menudata = (LPFMINFO)MenuInfo.dwMenuData;
	
	assert ((menudata != 0) && (MenuInfo.cbSize == sizeof(MENUINFO)));
	
	if (menudata->bInitialized)
	  return 0;
	
	uID = menudata->uID;
	pidl = ((pAlternatePidl) ? pAlternatePidl : menudata->pidl);
	uFlags = menudata->uFlags;
	uEnumFlags = menudata->uEnumFlags;
	lpfnCallback = menudata->lpfnCallback;

	menudata->bInitialized = FALSE;
	SetMenuInfo(hmenu, &MenuInfo);
	
	if (SUCCEEDED (SHGetDesktopFolder(&lpsf)))
	{
	  if (SUCCEEDED(IShellFolder_BindToObject(lpsf, pidl,0,(REFIID)&IID_IShellFolder,(LPVOID *)&lpsf2)))
	  {
	    IEnumIDList	*lpe = NULL;

	    if (SUCCEEDED (IShellFolder_EnumObjects(lpsf2, 0, uEnumFlags, &lpe )))
	    {

	      LPITEMIDLIST pidlTemp = NULL;
	      ULONG ulFetched;

	      while ((!bAbortInit) && (NOERROR == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched)))
	      {
		if (SUCCEEDED (IShellFolder_GetAttributesOf(lpsf, 1, &pidlTemp, &ulItemAttr)))
		{
		  ILGetDisplayName( pidlTemp, sTemp);
		  if (! (PidlToSicIndex(lpsf, pidlTemp, FALSE, &iIcon)))
		    iIcon = FM_BLANK_ICON;
		  if ( SFGAO_FOLDER & ulItemAttr)
		  {
		    LPFMINFO lpFmMi;
		    MENUINFO MenuInfo;
		    HMENU hMenuPopup = CreatePopupMenu();
	
		    lpFmMi = (LPFMINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FMINFO));

		    lpFmMi->pidl = ILCombine(pidl, pidlTemp);
		    lpFmMi->uEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

		    MenuInfo.cbSize = sizeof(MENUINFO);
		    MenuInfo.fMask = MIM_MENUDATA;
		    MenuInfo.dwMenuData = (DWORD) lpFmMi;
		    SetMenuInfo (hMenuPopup, &MenuInfo);

		    FileMenu_AppendItemA (hmenu, sTemp, uID, iIcon, hMenuPopup, FM_DEFAULT_HEIGHT);
		  }
		  else
		  {
		    ((LPSTR)PathFindExtensionA(sTemp))[0] = 0x00;
		    FileMenu_AppendItemA (hmenu, sTemp, uID, iIcon, 0, FM_DEFAULT_HEIGHT);
		  }
		}

		if (lpfnCallback)
		{
		  TRACE(shell,"enter callback\n");
		  lpfnCallback ( pidl, pidlTemp);
		  TRACE(shell,"leave callback\n");
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
	  FileMenu_AppendItemA (hmenu, "(empty)", uID, FM_BLANK_ICON, 0, FM_DEFAULT_HEIGHT);

	menudata->bInitialized = TRUE;
	SetMenuInfo(hmenu, &MenuInfo);

	return NumberOfItems;
}
/*************************************************************************
 * FileMenu_Create				[SHELL32.114]
 *
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

	TRACE(shell,"0x%08lx 0x%08x 0x%08x 0x%08x 0x%08x  hMenu=0x%08x\n",
	crBorderColor, nBorderWidth, hBorderBmp, nSelHeight, uFlags, hMenu);

	menudata = (LPFMINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FMINFO));
	menudata->bIsMagic = TRUE;
	menudata->crBorderColor = crBorderColor;
	menudata->nBorderWidth = nBorderWidth;
	menudata->hBorderBmp = hBorderBmp;

	MenuInfo.cbSize = sizeof(MENUINFO);
	MenuInfo.fMask = MIM_MENUDATA;
	MenuInfo.dwMenuData = (DWORD) menudata;
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

	TRACE(shell,"0x%08x\n", hmenu);

	FileMenu_DeleteAllItems (hmenu);
	
	menudata = FM_GetMenuInfo(hmenu);

	if ( menudata->pidl)
	{ SHFree( menudata->pidl);
	}
	HeapFree(GetProcessHeap(), 0, menudata);

	DestroyMenu (hmenu);
}

/*************************************************************************
 * FileMenu_AppendItemAW			[SHELL32.115]
 *
 */
BOOL WINAPI FileMenu_AppendItemA(
	HMENU hMenu,
	LPCSTR lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	LPSTR lpszText = (LPSTR)lpText;
	MENUITEMINFOA	mii;
	LPFMITEM myItem;

	TRACE(shell,"0x%08x %s 0x%08x 0x%08x 0x%08x 0x%08x\n",
	hMenu, (lpszText!=FM_SEPARATOR) ? lpText: NULL,
	uID, icon, hMenuPopup, nItemHeight);
	
	ZeroMemory (&mii, sizeof(MENUITEMINFOA));
	
	mii.cbSize = sizeof(MENUITEMINFOA);

	if (lpText != FM_SEPARATOR)
	{ int len = strlen (lpText);
	  myItem = (LPFMITEM) SHAlloc( sizeof(FMITEM) + len);
	  strcpy (myItem->szItemText, lpText);
	  myItem->cchItemText = len;
	  myItem->iIconIndex = icon;
	  myItem->hMenu = hMenu;
	  mii.fMask = MIIM_DATA;
	  mii.dwItemData = (DWORD) myItem;
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

	InsertMenuItemA (hMenu, (UINT)-1, TRUE, &mii);

	return TRUE;

}
BOOL WINAPI FileMenu_AppendItemAW(
	HMENU hMenu,
	LPCVOID lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	BOOL ret;
	LPSTR lpszText=NULL;

	if (VERSION_OsIsUnicode() && (lpText!=FM_SEPARATOR))
	  lpszText = HEAP_strdupWtoA ( GetProcessHeap(),0, lpText);

	ret = FileMenu_AppendItemA(hMenu, (lpszText) ? lpszText : lpText, uID, icon, hMenuPopup, nItemHeight);

	if (lpszText)
	  HeapFree( GetProcessHeap(), 0, lpszText );

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
	TRACE(shell,"0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	pdump (pidl);

	bAbortInit = FALSE;

	FM_SetMenuParameter(hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);	

	return FM_InitMenuPopup(hmenu, NULL);
}

/*************************************************************************
 * FileMenu_ReplaceUsingPidl			[SHELL32.113]
 *
 */
int WINAPI FileMenu_ReplaceUsingPidl(
	HMENU	hmenu,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	TRACE(shell,"0x%08x 0x%08x %p 0x%08x %p\n",
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
	FIXME(shell,"0x%08x\n",hMenu);	
}

/*************************************************************************
 * FileMenu_FindSubMenuByPidl			[SHELL32.106]
 */
HMENU WINAPI FileMenu_FindSubMenuByPidl(
	HMENU	hMenu,
	LPCITEMIDLIST	pidl)
{
	FIXME(shell,"0x%08x %p\n",hMenu, pidl);	
	return 0;
}

/*************************************************************************
 * FileMenu_AppendFilesForPidl			[SHELL32.124]
 */
HMENU WINAPI FileMenu_AppendFilesForPidl(
	HMENU	hmenu,
	LPCITEMIDLIST	pidl,
	BOOL	bAddSeperator)
{
	LPFMINFO	menudata;

	menudata = FM_GetMenuInfo(hmenu);
	
	menudata->bInitialized = FALSE;
	
	FM_InitMenuPopup(hmenu, pidl);

	if (bAddSeperator)
	  FileMenu_AppendItemA (hmenu, FM_SEPARATOR, 0, 0, 0, FM_DEFAULT_HEIGHT);

	TRACE (shell,"0x%08x %p 0x%08x\n",hmenu, pidl,bAddSeperator);	

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
	TRACE(shell,"0x%08x 0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hmenu, uReserved, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

	return FileMenu_InsertUsingPidl ( hmenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);

}


/*************************************************************************
 * FileMenu_TrackPopupMenuEx			[SHELL32.116]
 */
HRESULT WINAPI FileMenu_TrackPopupMenuEx (
	HMENU hMenu,
	UINT uFlags,
	int x,
	int y,
	HWND hWnd,
	LPTPMPARAMS lptpm)
{
	TRACE(shell,"0x%08x 0x%08x 0x%x 0x%x 0x%08x %p\n",
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
	FIXME(shell,"0x%08x %p %p\n",uReserved, ppidlFolder, ppidlItem);
	return 0;
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
		
	TRACE(shell,"0x%08x %p %s\n", hWnd, lpmis, pMyItem->szItemText);
	
	GetTextExtentPoint32A(hdc, pMyItem->szItemText, pMyItem->cchItemText, &size);
	
	lpmis->itemWidth = size.cx + FM_LEFTBORDER + FM_ICON_SIZE + FM_SPACE1 + FM_SPACE2 + FM_RIGHTBORDER;
	lpmis->itemHeight = (size.cy > (FM_ICON_SIZE + FM_Y_SPACE)) ? size.cy : (FM_ICON_SIZE + FM_Y_SPACE);

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->bIsMagic)
	  lpmis->itemWidth += menuinfo->nBorderWidth;
	
	TRACE(shell,"-- 0x%04x 0x%04x\n", lpmis->itemWidth, lpmis->itemHeight);
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
	RECT TextRect, BorderRect;
	LPFMINFO menuinfo;
	
	TRACE(shell,"0x%08x %p %s\n", hWnd, lpdis, pMyItem->szItemText);
	
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
	
	CopyRect(&TextRect, &(lpdis->rcItem));

	/* add the menubitmap */
	menuinfo = FM_GetMenuInfo(pMyItem->hMenu);
	if (menuinfo->bIsMagic)
	  TextRect.left += menuinfo->nBorderWidth;
	
	BorderRect.right = menuinfo->nBorderWidth;
/*	FillRect(lpdis->hDC, &BorderRect, CreateSolidBrush( menuinfo->crBorderColor));
*/
	TextRect.left += FM_LEFTBORDER;
	xi = TextRect.left + FM_SPACE1;
	yi = TextRect.top + FM_Y_SPACE/2;
	TextRect.bottom -= FM_Y_SPACE/2;

	xt = xi + FM_ICON_SIZE + FM_SPACE2;
	yt = yi;

	ExtTextOutA (lpdis->hDC, xt , yt, ETO_OPAQUE, &TextRect, pMyItem->szItemText, pMyItem->cchItemText, NULL);
	
	Shell_GetImageList(0, &hImageList);
	pImageList_Draw(hImageList, pMyItem->iIconIndex, lpdis->hDC, xi, yi, ILD_NORMAL);

	TRACE(shell,"-- 0x%04x 0x%04x 0x%04x 0x%04x\n", TextRect.left, TextRect.top, TextRect.right, TextRect.bottom);
	
	SetTextColor(lpdis->hDC, clrPrevText);
	SetBkColor(lpdis->hDC, clrPrevBkgnd);

	return TRUE;
}

/*************************************************************************
 * FileMenu_InitMenuPopup			[SHELL32.109]
 *
 * NOTES
 *  The filemenu is a ownerdrawn menu. Call this function responding to 
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
	FIXME(shell,"0x%08x 0x%08x\n",hMenu,wParam);
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
	MENUITEMINFOA	mii;
	LPFMINFO	menudata;

	int i;
	
	TRACE(shell,"0x%08x\n", hmenu);
	
	ZeroMemory ( &mii, sizeof(MENUITEMINFOA));
	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_SUBMENU|MIIM_DATA;

	for (i = 0; i < GetMenuItemCount( hmenu ); i++)
	{ GetMenuItemInfoA(hmenu, i, TRUE, &mii );

	  if (mii.dwItemData)
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
 * FileMenu_DeleteItemByCmd 			[SHELL32.]
 *
 */
BOOL WINAPI FileMenu_DeleteItemByCmd (HMENU hMenu, UINT uID)
{
	MENUITEMINFOA mii;

	TRACE(shell,"0x%08x 0x%08x\n", hMenu, uID);
	
	ZeroMemory ( &mii, sizeof(MENUITEMINFOA));
	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoA(hMenu, uID, FALSE, &mii );
	if ( mii.hSubMenu );

	DeleteMenu(hMenu, MF_BYCOMMAND, uID);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByIndex			[SHELL32.140]
 */
BOOL WINAPI FileMenu_DeleteItemByIndex ( HMENU hMenu, UINT uPos)
{
	MENUITEMINFOA mii;

	TRACE(shell,"0x%08x 0x%08x\n", hMenu, uPos);

	ZeroMemory ( &mii, sizeof(MENUITEMINFOA));
	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_SUBMENU;

	GetMenuItemInfoA(hMenu, uPos, TRUE, &mii );
	if ( mii.hSubMenu );

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
	TRACE(shell,"0x%08x 0x%08x\n", hMenu, uID);
	return 0;
}

/*************************************************************************
 * FileMenu_DeleteSeparator			[SHELL32.142]
 */
BOOL WINAPI FileMenu_DeleteSeparator(HMENU hMenu)
{
	TRACE(shell,"0x%08x\n", hMenu);
	return 0;
}

/*************************************************************************
 * FileMenu_EnableItemByCmd			[SHELL32.143]
 */
BOOL WINAPI FileMenu_EnableItemByCmd(
	HMENU	hMenu,
	UINT	uID,
	BOOL	bEnable)
{
	TRACE(shell,"0x%08x 0x%08x 0x%08x\n", hMenu, uID,bEnable);
	return 0;
}

/*************************************************************************
 * FileMenu_GetItemExtent			[SHELL32.144]
 * 
 * NOTES
 *  if the menu is to big, entrys are getting cut away!!
 */
DWORD WINAPI FileMenu_GetItemExtent (HMENU hMenu, UINT uPos)
{	RECT rect;
	
	FIXME (shell,"0x%08x 0x%08x\n", hMenu, uPos);

	if (GetMenuItemRect(0, hMenu, uPos, &rect))
	{ FIXME (shell,"0x%04x 0x%04x 0x%04x 0x%04x\n",
	  rect.right, rect.left, rect.top, rect.bottom);
	  return ((rect.right-rect.left)<<16) + (rect.top-rect.bottom);
	}
	return 0x00100010; /*fixme*/
}

/*************************************************************************
 * FileMenu_AbortInitMenu 			[SHELL32.120]
 *
 */
void WINAPI FileMenu_AbortInitMenu (void)
{	TRACE(shell,"\n");
	bAbortInit = TRUE;
}

/*************************************************************************
 * SHFind_InitMenuPopup				[SHELL32.149]
 *
 *
 * PARAMETERS
 *  hMenu		[in] handel of menu previously created
 *  hWndParent	[in] parent window
 *  w			[in] no pointer
 *  x			[in] no pointer
 */
HRESULT WINAPI SHFind_InitMenuPopup (HMENU hMenu, HWND hWndParent, DWORD w, DWORD x)
{	FIXME(shell,"hmenu=0x%08x hwnd=0x%08x 0x%08lx 0x%08lx stub\n",
		hMenu,hWndParent,w,x);
	return TRUE;
}

/*************************************************************************
 * Shell_MergeMenus				[SHELL32.67]
 *
 */
BOOL _SHIsMenuSeparator(HMENU hm, int i)
{
	MENUITEMINFOA mii;

	mii.cbSize = sizeof(MENUITEMINFOA);
	mii.fMask = MIIM_TYPE;
	mii.cch = 0;    /* WARNING: We MUST initialize it to 0*/
	if (!GetMenuItemInfoA(hm, i, TRUE, &mii))
	{ return(FALSE);
	}

	if (mii.fType & MFT_SEPARATOR)
	{ return(TRUE);
	}

        return(FALSE);
}
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
HRESULT WINAPI Shell_MergeMenus (HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{	int		nItem;
	HMENU		hmSubMenu;
	BOOL		bAlreadySeparated;
	MENUITEMINFOA miiSrc;
	char		szName[256];
	UINT		uTemp, uIDMax = uIDAdjust;

	FIXME(shell,"hmenu1=0x%04x hmenu2=0x%04x 0x%04x 0x%04x 0x%04x  0x%04lx stub\n",
		 hmDst, hmSrc, uInsert, uIDAdjust, uIDAdjustMax, uFlags);

	if (!hmDst || !hmSrc)
	{ return uIDMax;
	}

	nItem = GetMenuItemCount(hmDst);
	if (uInsert >= (UINT)nItem)
	{ uInsert = (UINT)nItem;
	  bAlreadySeparated = TRUE;
	}
	else
	{ bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);;
	}
	if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	{ /* Add a separator between the menus */
	  InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	  bAlreadySeparated = TRUE;
	}


	/* Go through the menu items and clone them*/
	for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
	{ miiSrc.cbSize = sizeof(MENUITEMINFOA);
	  miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
	  /* We need to reset this every time through the loop in case
	  menus DON'T have IDs*/
	  miiSrc.fType = MFT_STRING;
	  miiSrc.dwTypeData = szName;
	  miiSrc.dwItemData = 0;
	  miiSrc.cch = sizeof(szName);

	  if (!GetMenuItemInfoA(hmSrc, nItem, TRUE, &miiSrc))
	  { continue;
	  }
	  if (miiSrc.fType & MFT_SEPARATOR)
	  { /* This is a separator; don't put two of them in a row*/
	    if (bAlreadySeparated)
	    { continue;
	    }
	    bAlreadySeparated = TRUE;
	  }
	  else if (miiSrc.hSubMenu)
	  { if (uFlags & MM_SUBMENUSHAVEIDS)
	    { /* Adjust the ID and check it*/
	      miiSrc.wID += uIDAdjust;
	      if (miiSrc.wID > uIDAdjustMax)
	      { continue;
	      }
	      if (uIDMax <= miiSrc.wID)
	      { uIDMax = miiSrc.wID + 1;
	      }
	    }
	    else
	    { /* Don't set IDs for submenus that didn't have them already */
	      miiSrc.fMask &= ~MIIM_ID;
	    }
	    hmSubMenu = miiSrc.hSubMenu;
	    miiSrc.hSubMenu = CreatePopupMenu();
	    if (!miiSrc.hSubMenu)
	    { return(uIDMax);
	    }
	    uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust, uIDAdjustMax, uFlags&MM_SUBMENUSHAVEIDS);
	    if (uIDMax <= uTemp)
	    { uIDMax = uTemp;
	    }
	    bAlreadySeparated = FALSE;
	  }
	  else
	  { /* Adjust the ID and check it*/
	    miiSrc.wID += uIDAdjust;
	    if (miiSrc.wID > uIDAdjustMax)
	    { continue;
	    }
	    if (uIDMax <= miiSrc.wID)
	    { uIDMax = miiSrc.wID + 1;
	    }
	    bAlreadySeparated = FALSE;
	  }
	  if (!InsertMenuItemA(hmDst, uInsert, TRUE, &miiSrc))
	  { return(uIDMax);
	  }
	}

	/* Ensure the correct number of separators at the beginning of the
	inserted menu items*/
	if (uInsert == 0)
	{ if (bAlreadySeparated)
	  { DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	  }
	}
	else
	{ if (_SHIsMenuSeparator(hmDst, uInsert-1))
	  { if (bAlreadySeparated)
	    { DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
	    }
	  }
	  else
	  { if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
	    { /* Add a separator between the menus*/
	      InsertMenuA(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	    }
	  }
	}
	return(uIDMax);
}
