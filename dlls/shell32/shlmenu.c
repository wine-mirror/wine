/*
 *
 */
#include <string.h>

#include "wine/obj_base.h"
#include "wine/obj_enumidlist.h"
#include "wine/obj_shellfolder.h"

#include "heap.h"
#include "debug.h"
#include "winversion.h"
#include "shell32_main.h"

#include "pidl.h"

DEFAULT_DEBUG_CHANNEL(shell)

BOOL WINAPI FileMenu_DeleteAllItems (HMENU hMenu);

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
	HMENU ret = CreatePopupMenu();

	FIXME(shell,"0x%08lx 0x%08x 0x%08x 0x%08x 0x%08x  ret=0x%08x\n",
	crBorderColor, nBorderWidth, hBorderBmp, nSelHeight, uFlags, ret);

	return ret;
}

/*************************************************************************
 * FileMenu_Destroy				[SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
void WINAPI FileMenu_Destroy (HMENU hMenu)
{
	TRACE(shell,"0x%08x\n", hMenu);
	FileMenu_DeleteAllItems (hMenu);
	DestroyMenu (hMenu);
}

/*************************************************************************
 * FileMenu_AppendItemAW			[SHELL32.115]
 *
 */
BOOL WINAPI FileMenu_AppendItemAW(
	HMENU hMenu,
	LPCVOID lpText,
	UINT uID,
	int icon,
	HMENU hMenuPopup,
	int nItemHeight)
{
	LPSTR lpszText = (LPSTR)lpText;
	MENUITEMINFOA	mii;
	ZeroMemory (&mii, sizeof(MENUITEMINFOA));
	
	if (VERSION_OsIsUnicode() && (lpszText!=FM_SEPARATOR))
	  lpszText = HEAP_strdupWtoA ( GetProcessHeap(),0, lpText);

	FIXME(shell,"0x%08x %s 0x%08x 0x%08x 0x%08x 0x%08x\n",
	hMenu, (lpszText!=FM_SEPARATOR) ? lpszText: NULL,
	uID, icon, hMenuPopup, nItemHeight);

	mii.cbSize = sizeof(mii);
	
	if ( hMenuPopup )
	{ /* sub menu */
	  mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;;
	  mii.hSubMenu = hMenuPopup;
	  mii.fType = MFT_STRING;
	  mii.dwTypeData = lpszText;
	}
	else if (lpText == FM_SEPARATOR )
	{ mii.fMask = MIIM_ID | MIIM_TYPE;
	  mii.fType = MFT_SEPARATOR;
	}
	else
	{ /* normal item */
	  mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	  mii.dwTypeData = lpszText;
	  mii.fState = MFS_ENABLED | MFS_DEFAULT;
	  mii.fType = MFT_STRING;
	}
	mii.wID = uID;

	InsertMenuItemA (hMenu, (UINT)-1, TRUE, &mii);

	if (VERSION_OsIsUnicode())
	  HeapFree( GetProcessHeap(), 0, lpszText );
	
	return TRUE;

}
 
/*************************************************************************
 * FileMenu_InsertUsingPidl			[SHELL32.110]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_InsertUsingPidl (
	HMENU hMenu,
	UINT uID,
	LPCITEMIDLIST pidl,
	UINT uFlags,
	UINT uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{	
	IShellFolder	*lpsf, *lpsf2;
	IEnumIDList	*lpe=0;
	ULONG		ulFetched;
	LPITEMIDLIST	pidlTemp=0;
	ULONG		ulItemAttr;
	char		sTemp[MAX_PATH];
	int		NumberOfItems = 0;

	FIXME(shell,"0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hMenu, uID, pidl, uFlags, uEnumFlags, lpfnCallback);
	pdump (pidl);

	if (SUCCEEDED (SHGetDesktopFolder(&lpsf)))
	{ if (SUCCEEDED(IShellFolder_BindToObject(lpsf, pidl,0,(REFIID)&IID_IShellFolder,(LPVOID *)&lpsf2)))
	  { if (SUCCEEDED (IShellFolder_EnumObjects(lpsf2, 0, uEnumFlags, &lpe )))
	    { while (NOERROR == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched))
	      { if (SUCCEEDED (IShellFolder_GetAttributesOf(lpsf, 1, &pidlTemp, &ulItemAttr)))
		{ ILGetDisplayName( pidlTemp, sTemp);
		  if ( SFGAO_FOLDER & ulItemAttr)
		  { FileMenu_AppendItemAW (hMenu, sTemp, uID, FM_BLANK_ICON, CreatePopupMenu(), FM_DEFAULT_HEIGHT);
		  }
		  else
		  { FileMenu_AppendItemAW (hMenu, sTemp, uID, FM_BLANK_ICON, 0, FM_DEFAULT_HEIGHT);
		  }
		}
		TRACE(shell,"enter callback\n");
		lpfnCallback ( pidl, pidlTemp);
		TRACE(shell,"leave callback\n");
	        NumberOfItems++;
	      }
	      IEnumIDList_Release (lpe);
	    }
	    IShellFolder_Release(lpsf2);
	  }
	  IShellFolder_Release(lpsf);
	}

	return NumberOfItems;
}

/*************************************************************************
 * FileMenu_ReplaceUsingPidl			[SHELL32.113]
 *
 */
int WINAPI FileMenu_ReplaceUsingPidl(
	HMENU	hMenu,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uEnumFlags,
	LPFNFMCALLBACK lpfnCallback)
{
	FIXME(shell,"0x%08x 0x%08x %p 0x%08x %p\n",
	hMenu, uID, pidl, uEnumFlags, lpfnCallback);
	return 0;
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
	HMENU	hMenu,
	LPCITEMIDLIST	pidl,
	BOOL	bAddSeperator)
{
	FIXME(shell,"0x%08x %p 0x%08x\n",hMenu, pidl,bAddSeperator);	
	return 0;
}
/*************************************************************************
 * FileMenu_AddFilesForPidl			[SHELL32.125]
 *
 * NOTES
 *	uEnumFlags	any SHCONTF flag
 */
int WINAPI FileMenu_AddFilesForPidl (
	HMENU	hMenu,
	UINT	uReserved,
	UINT	uID,
	LPCITEMIDLIST	pidl,
	UINT	uFlags,
	UINT	uEnumFlags,
	LPFNFMCALLBACK	lpfnCallback)
{
	FIXME(shell,"0x%08x 0x%08x 0x%08x %p 0x%08x 0x%08x %p\n",
	hMenu, uReserved, uID, pidl, uFlags, uEnumFlags, lpfnCallback);
	pdump (pidl);
	return 0;

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
	FIXME(shell,"0x%08x 0x%08x 0x%x 0x%x 0x%08x %p stub\n",
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

/*************************************************************************
 * FileMenu_MeasureItem				[SHELL32.112]
 */
LRESULT WINAPI FileMenu_MeasureItem(
	HWND	hWnd,
	LPMEASUREITEMSTRUCT	lpmis)
{
	FIXME(shell,"0x%08x %p\n", hWnd, lpmis);
	return 0;
}

/*************************************************************************
 * FileMenu_DrawItem				[SHELL32.105]
 */
LRESULT WINAPI FileMenu_DrawItem(
	HWND			hWnd,
	LPDRAWITEMSTRUCT	lpdis)
{
	FIXME(shell,"0x%08x %p\n", hWnd, lpdis);
	return 0;
}

/*************************************************************************
 * FileMenu_InitMenuPopup			[SHELL32.109]
 *
 * NOTES
 *  The filemenu is a ownerdrawn menu. Call this function responding to 
 *  WM_INITPOPUPMENU
 *
 */
HRESULT WINAPI FileMenu_InitMenuPopup (DWORD hmenu)
{	FIXME(shell,"hmenu=0x%lx stub\n",hmenu);
	return 0;
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
BOOL WINAPI FileMenu_DeleteAllItems (HMENU hMenu)
{
	FIXME(shell,"0x%08x stub\n", hMenu);
	DestroyMenu (hMenu);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByCmd 			[SHELL32.]
 *
 */
BOOL WINAPI FileMenu_DeleteItemByCmd (HMENU hMenu, UINT uID)
{
	TRACE(shell,"0x%08x 0x%08x\n", hMenu, uID);

	DeleteMenu(hMenu, MF_BYCOMMAND, uID);
	return TRUE;
}

/*************************************************************************
 * FileMenu_DeleteItemByIndex			[SHELL32.140]
 */
BOOL WINAPI FileMenu_DeleteItemByIndex ( HMENU hMenu, UINT uPos)
{
	TRACE(shell,"0x%08x 0x%08x\n", hMenu, uPos);

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
 */
DWORD WINAPI FileMenu_GetItemExtent (HMENU hMenu, UINT uPos)
{	RECT rect;
	
	FIXME (shell,"0x%08x 0x%08x\n", hMenu, uPos);

	if (GetMenuItemRect(0, hMenu, uPos, &rect))
	{ FIXME (shell,"0x%04x 0x%04x 0x%04x 0x%04x\n",
	  rect.right, rect.left, rect.top, rect.bottom);
	  return ((rect.right-rect.left)<<16) + (rect.top-rect.bottom);
	}
	return 0x00200020; /*fixme*/
}

/*************************************************************************
 * FileMenu_AbortInitMenu 			[SHELL32.120]
 *
 */
void WINAPI FileMenu_AbortInitMenu (void)
{	TRACE(shell,"\n");
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
