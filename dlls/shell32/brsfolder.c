/*
 * Copyright 1999 Juergen Schmied
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
 * FIXME:
 *  - view with root unequal desktop
 *  - many memory leaks
 *  - show only filesystem objects
 */

#include <stdlib.h>
#include <string.h>

#include "wine/debug.h"
#include "undocshell.h"
#include "shlguid.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellapi.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HWND		hwndTreeView;
static LPBROWSEINFOW	lpBrowseInfo;
static LPITEMIDLIST	pidlRet;

static void FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST  lpifq, HTREEITEM hParent);

static void InitializeTreeView(HWND hwndParent, LPCITEMIDLIST root)
{
	HIMAGELIST	hImageList;
	IShellFolder *	lpsf;
	HRESULT	hr;

	hwndTreeView = GetDlgItem (hwndParent, IDD_TREEVIEW);
	Shell_GetImageList(NULL, &hImageList);

	TRACE("dlg=%p tree=%p\n", hwndParent, hwndTreeView );

	if (hImageList && hwndTreeView)
	{ TreeView_SetImageList(hwndTreeView, hImageList, 0);
	}

	/* so far, this method doesn't work (still missing the upper level), keep the old way */
#if 0
	if (_ILIsDesktop (root)) {
	   hr = SHGetDesktopFolder(&lpsf);
	} else {
	   IShellFolder *	lpsfdesktop;

	   hr = SHGetDesktopFolder(&lpsfdesktop);
	   if (SUCCEEDED(hr)) {
	      hr = IShellFolder_BindToObject(lpsfdesktop, root, 0,(REFIID)&IID_IShellFolder,(LPVOID *)&lpsf);
	      IShellFolder_Release(lpsfdesktop);
	   }
	}
#else
	hr = SHGetDesktopFolder(&lpsf);
#endif

	if (SUCCEEDED(hr) && hwndTreeView)
	{ TreeView_DeleteAllItems(hwndTreeView);
       	  FillTreeView(lpsf, NULL, TVI_ROOT);
	}

	if (SUCCEEDED(hr))
	{ IShellFolder_Release(lpsf);
	}
	TRACE("done\n");
}

static int GetIcon(LPITEMIDLIST lpi, UINT uFlags)
{	SHFILEINFOW    sfi;
	SHGetFileInfoW((LPCWSTR)lpi, 0 ,&sfi, sizeof(SHFILEINFOW), uFlags);
	return sfi.iIcon;
}

static void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, LPTVITEMW lpTV_ITEM)
{	TRACE("%p %p\n",lpifq, lpTV_ITEM);

	lpTV_ITEM->iImage = GetIcon(lpifq, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	lpTV_ITEM->iSelectedImage = GetIcon(lpifq, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);

	return;
}

typedef struct tagID
{
   LPSHELLFOLDER lpsfParent;
   LPITEMIDLIST  lpi;
   LPITEMIDLIST  lpifq;
} TV_ITEMDATA, *LPTV_ITEMDATA;

static BOOL GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags, LPWSTR lpFriendlyName)
{
	BOOL   bSuccess=TRUE;
	STRRET str;

	TRACE("%p %p %lx %p\n", lpsf, lpi, dwFlags, lpFriendlyName);
	if (SUCCEEDED(IShellFolder_GetDisplayNameOf(lpsf, lpi, dwFlags, &str)))
	{
	  if (FAILED(StrRetToStrNW(lpFriendlyName, MAX_PATH, &str, lpi)))
	  {
	      bSuccess = FALSE;
	  }
	}
	else
	  bSuccess = FALSE;

	TRACE("-- %s\n", debugstr_w(lpFriendlyName));
	return bSuccess;
}

static void FillTreeView(IShellFolder * lpsf, LPITEMIDLIST  pidl, HTREEITEM hParent)
{
	TVITEMW 	tvi;
	TVINSERTSTRUCTW	tvins;
	HTREEITEM	hPrev = 0;
	LPENUMIDLIST	lpe=0;
	LPITEMIDLIST	pidlTemp=0;
	LPTV_ITEMDATA	lptvid=0;
	ULONG		ulFetched;
	HRESULT		hr;
	WCHAR		szBuff[MAX_PATH];
	HWND		hwnd=GetParent(hwndTreeView);

	TRACE("%p %p %x\n",lpsf, pidl, (INT)hParent);
	SetCapture(GetParent(hwndTreeView));
	SetCursor(LoadCursorA(0, IDC_WAITA));

	hr=IShellFolder_EnumObjects(lpsf, hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &lpe);

	if (SUCCEEDED(hr))
	{ while (NOERROR == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched))
	  { ULONG ulAttrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
	    IShellFolder_GetAttributesOf(lpsf, 1, &pidlTemp, &ulAttrs);
	    if (ulAttrs & (SFGAO_HASSUBFOLDER | SFGAO_FOLDER))
	    { if (ulAttrs & SFGAO_FOLDER)
	      { tvi.mask  = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

	        if (ulAttrs & SFGAO_HASSUBFOLDER)
	        {  tvi.cChildren=1;
	           tvi.mask |= TVIF_CHILDREN;
	        }

	        if (!(lptvid = (LPTV_ITEMDATA)SHAlloc(sizeof(TV_ITEMDATA))))
		    goto Done;

	        if (!GetName(lpsf, pidlTemp, SHGDN_NORMAL, szBuff))
		    goto Done;

	        tvi.pszText    = szBuff;
	        tvi.cchTextMax = MAX_PATH;
	        tvi.lParam = (LPARAM)lptvid;

	        IShellFolder_AddRef(lpsf);
	        lptvid->lpsfParent = lpsf;
	        lptvid->lpi	= ILClone(pidlTemp);
	        lptvid->lpifq	= ILCombine(pidl, pidlTemp);
	        GetNormalAndSelectedIcons(lptvid->lpifq, &tvi);

	        tvins.DUMMYUNIONNAME.item         = tvi;
	        tvins.hInsertAfter = hPrev;
	        tvins.hParent      = hParent;

	        hPrev = (HTREEITEM)TreeView_InsertItemW(hwndTreeView, &tvins);

	      }
	    }
	    SHFree(pidlTemp);  /* Finally, free the pidl that the shell gave us... */
	    pidlTemp=0;
	  }
	}

Done:
	ReleaseCapture();
	SetCursor(LoadCursorW(0, IDC_ARROWW));

	if (lpe)
	  IEnumIDList_Release(lpe);
	if (pidlTemp )
	  SHFree(pidlTemp);
}

static LRESULT MsgNotify(HWND hWnd,  UINT CtlID, LPNMHDR lpnmh)
{
	NMTREEVIEWW	*pnmtv   = (NMTREEVIEWW *)lpnmh;
	LPTV_ITEMDATA	lptvid;  /* Long pointer to TreeView item data */
	IShellFolder *	lpsf2=0;


	TRACE("%p %x %p msg=%x\n", hWnd,  CtlID, lpnmh, pnmtv->hdr.code);

	switch (pnmtv->hdr.idFrom)
	{ case IDD_TREEVIEW:
	    switch (pnmtv->hdr.code)
	    { case TVN_DELETEITEMA:
	      case TVN_DELETEITEMW:
	        { FIXME("TVN_DELETEITEMA/W\n");
	          lptvid=(LPTV_ITEMDATA)pnmtv->itemOld.lParam;
	          IShellFolder_Release(lptvid->lpsfParent);
	          SHFree(lptvid->lpi);
	          SHFree(lptvid->lpifq);
	          SHFree(lptvid);
		}
	        break;

	      case TVN_ITEMEXPANDINGA:
	      case TVN_ITEMEXPANDINGW:
		{ FIXME("TVN_ITEMEXPANDINGA/W\n");
		  if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
	            break;

	          lptvid=(LPTV_ITEMDATA)pnmtv->itemNew.lParam;
	          if (SUCCEEDED(IShellFolder_BindToObject(lptvid->lpsfParent, lptvid->lpi,0,(REFIID)&IID_IShellFolder,(LPVOID *)&lpsf2)))
	          { FillTreeView( lpsf2, lptvid->lpifq, pnmtv->itemNew.hItem );
	          }
	          TreeView_SortChildren(hwndTreeView, pnmtv->itemNew.hItem, FALSE);
		}
	        break;
	      case TVN_SELCHANGEDA:
	      case TVN_SELCHANGEDW:
	        lptvid=(LPTV_ITEMDATA)pnmtv->itemNew.lParam;
		pidlRet = lptvid->lpifq;
		if (lpBrowseInfo->lpfn)
		   (lpBrowseInfo->lpfn)(hWnd, BFFM_SELCHANGED, (LPARAM)pidlRet, lpBrowseInfo->lParam);
	        break;

	      default:
	        FIXME("unhandled (%d)\n", pnmtv->hdr.code);
		break;
	    }
	    break;

	  default:
	    break;
	}

	return 0;
}


/*************************************************************************
 *             BrsFolderDlgProc32  (not an exported API function)
 */
static INT_PTR CALLBACK BrsFolderDlgProc(HWND hWnd, UINT msg, WPARAM wParam,
				     LPARAM lParam )
{
	TRACE("hwnd=%p msg=%04x 0x%08x 0x%08lx\n", hWnd,  msg, wParam, lParam );

	switch(msg)
	{ case WM_INITDIALOG:
	    pidlRet = NULL;
	    lpBrowseInfo = (LPBROWSEINFOW) lParam;
	    if (lpBrowseInfo->ulFlags & ~(BIF_STATUSTEXT))
	      FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~(BIF_STATUSTEXT));
	    if (lpBrowseInfo->lpszTitle) {
	       SetWindowTextW(GetDlgItem(hWnd, IDD_TITLE), lpBrowseInfo->lpszTitle);
	    } else {
	       ShowWindow(GetDlgItem(hWnd, IDD_TITLE), SW_HIDE);
	    }
	    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT))
	       ShowWindow(GetDlgItem(hWnd, IDD_STATUS), SW_HIDE);

	    if (lpBrowseInfo->pidlRoot )
	      FIXME("root is desktop\n");

	    InitializeTreeView(hWnd, lpBrowseInfo->pidlRoot);

	    if (lpBrowseInfo->lpfn) {
	       (lpBrowseInfo->lpfn)(hWnd, BFFM_INITIALIZED, 0, lpBrowseInfo->lParam);
	       (lpBrowseInfo->lpfn)(hWnd, BFFM_SELCHANGED, 0/*FIXME*/, lpBrowseInfo->lParam);
	    }

	    return TRUE;

	  case WM_NOTIFY:
	    MsgNotify( hWnd, (UINT)wParam, (LPNMHDR)lParam);
	    break;

	  case WM_COMMAND:
	    switch (wParam)
	    { case IDOK:
	        pdump ( pidlRet );
	        SHGetPathFromIDListW(pidlRet, lpBrowseInfo->pszDisplayName);
	        EndDialog(hWnd, (DWORD) ILClone(pidlRet));
	        return TRUE;

	      case IDCANCEL:
	        EndDialog(hWnd, 0);
	        return TRUE;
	    }
	    break;
	case BFFM_SETSTATUSTEXTA:
	   TRACE("Set status %s\n", debugstr_a((LPSTR)lParam));
	   SetWindowTextA(GetDlgItem(hWnd, IDD_STATUS), (LPSTR)lParam);
	   break;
	case BFFM_SETSTATUSTEXTW:
	   TRACE("Set status %s\n", debugstr_w((LPWSTR)lParam));
	   SetWindowTextW(GetDlgItem(hWnd, IDD_STATUS), (LPWSTR)lParam);
	   break;
	case BFFM_ENABLEOK:
	   TRACE("Enable %ld\n", lParam);
	   EnableWindow(GetDlgItem(hWnd, 1), (lParam)?TRUE:FALSE);
	   break;
	case BFFM_SETSELECTIONA:
	   if (wParam)
	      TRACE("Set selection %s\n", debugstr_a((LPSTR)lParam));
	   else
	      TRACE("Set selection %p\n", (void*)lParam);
	   break;
	case BFFM_SETSELECTIONW:
	   if (wParam)
	      TRACE("Set selection %s\n", debugstr_w((LPWSTR)lParam));
	   else
	      TRACE("Set selection %p\n", (void*)lParam);
	   break;
	}
	return FALSE;
}

static WCHAR swBrowseTempName[] = {'S','H','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.@]
 * SHBrowseForFolder  [SHELL32.@]
 */
LPITEMIDLIST WINAPI SHBrowseForFolderA (LPBROWSEINFOA lpbi)
{
	BROWSEINFOW bi;
	LPITEMIDLIST lpid;
	INT len;
	
	TRACE("(%p{lpszTitle=%s,owner=%p})\n", lpbi,
	    lpbi ? debugstr_a(lpbi->lpszTitle) : NULL, lpbi ? lpbi->hwndOwner : NULL);

	if (!lpbi)
	  return NULL;

	bi.hwndOwner = lpbi->hwndOwner;
	bi.pidlRoot = lpbi->pidlRoot;
	if (lpbi->pszDisplayName)
	{
	  len = MultiByteToWideChar(CP_ACP, 0, lpbi->pszDisplayName, -1, NULL, 0);
	  bi.pszDisplayName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	  MultiByteToWideChar(CP_ACP, 0, lpbi->pszDisplayName, -1, bi.pszDisplayName, len);
	}
	else
	  bi.pszDisplayName = NULL;

	if (lpbi->lpszTitle)
	{
	  len = MultiByteToWideChar(CP_ACP, 0, lpbi->lpszTitle, -1, NULL, 0);
	  bi.lpszTitle = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	  MultiByteToWideChar(CP_ACP, 0, lpbi->lpszTitle, -1, (LPWSTR)bi.lpszTitle, len);
	}
	else
	  bi.lpszTitle = NULL;

	bi.ulFlags = lpbi->ulFlags;
	bi.lpfn = lpbi->lpfn;
	bi.lParam = lpbi->lParam;
	bi.iImage = lpbi->iImage;
	lpid = (LPITEMIDLIST) DialogBoxParamW(shell32_hInstance,
	                                      swBrowseTempName, lpbi->hwndOwner,
	                                      BrsFolderDlgProc, (INT)&bi);
	if (bi.pszDisplayName)
	{
	  WideCharToMultiByte(CP_ACP, 0, bi.pszDisplayName, -1, lpbi->pszDisplayName, MAX_PATH, 0, NULL);
	  HeapFree(GetProcessHeap(), 0, bi.pszDisplayName);
	}
	if (bi.lpszTitle)
	{
	  HeapFree(GetProcessHeap(), 0, (LPVOID)bi.lpszTitle);
	}
	lpbi->iImage = bi.iImage;
	return lpid;
}


/*************************************************************************
 * SHBrowseForFolderW [SHELL32.@]
 */
LPITEMIDLIST WINAPI SHBrowseForFolderW (LPBROWSEINFOW lpbi)
{
	TRACE("((%p->{lpszTitle=%s,owner=%p})\n", lpbi,
	    lpbi ? debugstr_w(lpbi->lpszTitle) : NULL, lpbi ? lpbi->hwndOwner : 0);

	if (!lpbi)
	  return NULL;

	return (LPITEMIDLIST) DialogBoxParamW(shell32_hInstance,
	                                      swBrowseTempName, lpbi->hwndOwner,
	                                      BrsFolderDlgProc, (INT)lpbi);
}
