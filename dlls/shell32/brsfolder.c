#include <stdlib.h>
#include <string.h>

#include "debugtools.h"
#include "wine/undocshell.h"
#include "shlguid.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellapi.h"
#include "shresdef.h"

DEFAULT_DEBUG_CHANNEL(shell);

static HWND		hwndTreeView;
static LPBROWSEINFOA	lpBrowseInfo;
static LPITEMIDLIST	pidlRet;

static void FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST  lpifq, HTREEITEM hParent);

static void InitializeTreeView(HWND hwndParent, LPCITEMIDLIST root)
{
	HIMAGELIST	hImageList;
	IShellFolder *	lpsf;
	HRESULT	hr;

	hwndTreeView = GetDlgItem (hwndParent, IDD_TREEVIEW);
	Shell_GetImageList(NULL, &hImageList);
	
	TRACE("dlg=%x tree=%x\n", hwndParent, hwndTreeView );

	if (hImageList && hwndTreeView)
	{ TreeView_SetImageList(hwndTreeView, hImageList, 0);
	}

	/* so far, this method doesn't work (still missing the upper level), keep the old way */
#if 0
	if (root == NULL) {
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
{	SHFILEINFOA    sfi;
	SHGetFileInfoA((LPCSTR)lpi,0,&sfi, sizeof(SHFILEINFOA), uFlags);
	return sfi.iIcon;
}

static void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq,LPTVITEMA lpTV_ITEM)
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

static BOOL GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags, LPSTR lpFriendlyName)
{
	BOOL   bSuccess=TRUE;
	STRRET str;

	TRACE("%p %p %lx %p\n", lpsf, lpi, dwFlags, lpFriendlyName);
	if (SUCCEEDED(IShellFolder_GetDisplayNameOf(lpsf, lpi, dwFlags, &str)))
	{
	  if(FAILED(StrRetToStrNA (lpFriendlyName, MAX_PATH, &str, lpi)))
	  {
	      bSuccess = FALSE;
	  }
	}
	else
	  bSuccess = FALSE;

	TRACE("-- %s\n",lpFriendlyName);
	return bSuccess;
}

static void FillTreeView(IShellFolder * lpsf, LPITEMIDLIST  pidl, HTREEITEM hParent)
{
	TVITEMA 	tvi;
	TVINSERTSTRUCTA	tvins;
	HTREEITEM	hPrev = 0;
	LPENUMIDLIST	lpe=0;
	LPITEMIDLIST	pidlTemp=0;
	LPTV_ITEMDATA	lptvid=0;
	ULONG		ulFetched;
	HRESULT		hr;
	char		szBuff[256];
	HWND		hwnd=GetParent(hwndTreeView);

	TRACE("%p %p %x\n",lpsf, pidl, (INT)hParent);
	SetCapture(GetParent(hwndTreeView));
	SetCursor(LoadCursorA(0, IDC_WAITA));

	hr=IShellFolder_EnumObjects(lpsf,hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,&lpe);

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

	        if (!( lptvid = (LPTV_ITEMDATA)SHAlloc(sizeof(TV_ITEMDATA))))
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

	        hPrev = (HTREEITEM)TreeView_InsertItemA (hwndTreeView, &tvins);

	      }
	    }
	    SHFree(pidlTemp);  /* Finally, free the pidl that the shell gave us... */
	    pidlTemp=0;
	  }
	}

Done:
	ReleaseCapture();
	SetCursor(LoadCursorA(0, IDC_ARROWA));

	if (lpe)
	  IEnumIDList_Release(lpe);
	if (pidlTemp )
	  SHFree(pidlTemp);
}

static LRESULT MsgNotify(HWND hWnd,  UINT CtlID, LPNMHDR lpnmh)
{	
	NMTREEVIEWA	*pnmtv   = (NMTREEVIEWA *)lpnmh;
	LPTV_ITEMDATA	lptvid;  /* Long pointer to TreeView item data */
	IShellFolder *	lpsf2=0;
	

	TRACE("%x %x %p msg=%x\n", hWnd,  CtlID, lpnmh, pnmtv->hdr.code);

	switch (pnmtv->hdr.idFrom)
	{ case IDD_TREEVIEW:
	    switch (pnmtv->hdr.code)   
	    { case TVN_DELETEITEMA:
	        { FIXME("TVN_DELETEITEMA\n");
		  lptvid=(LPTV_ITEMDATA)pnmtv->itemOld.lParam;
	          IShellFolder_Release(lptvid->lpsfParent);
	          SHFree(lptvid->lpi);  
	          SHFree(lptvid->lpifq);  
	          SHFree(lptvid);  
		}
	        break;
			
	      case TVN_ITEMEXPANDINGA:
		{ FIXME("TVN_ITEMEXPANDINGA\n");
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
static BOOL WINAPI BrsFolderDlgProc( HWND hWnd, UINT msg, WPARAM wParam,
				     LPARAM lParam )
{       TRACE("hwnd=%i msg=%i 0x%08x 0x%08lx\n", hWnd,  msg, wParam, lParam );

	switch(msg)
	{ case WM_INITDIALOG:
	    pidlRet = NULL;
	    lpBrowseInfo = (LPBROWSEINFOA) lParam;
	    if (lpBrowseInfo->ulFlags & ~(BIF_STATUSTEXT))
	      FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~(BIF_STATUSTEXT));
	    if (lpBrowseInfo->lpszTitle) {
	       SetWindowTextA(GetDlgItem(hWnd, IDD_TITLE), lpBrowseInfo->lpszTitle);
	    } else {
	       ShowWindow(GetDlgItem(hWnd, IDD_TITLE), SW_HIDE);
	    }
	    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT))
	       ShowWindow(GetDlgItem(hWnd, IDD_STATUS), SW_HIDE);

	    if ( lpBrowseInfo->pidlRoot )
	      FIXME("root is desktop\n");

	    InitializeTreeView( hWnd, lpBrowseInfo->pidlRoot );

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
		SHGetPathFromIDListA(pidlRet, lpBrowseInfo->pszDisplayName);
	        EndDialog(hWnd, (DWORD) ILClone(pidlRet));
	        return TRUE;

	      case IDCANCEL:
	        EndDialog(hWnd, 0);
	        return TRUE;
		break;
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

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.209]
 *
 */
LPITEMIDLIST WINAPI SHBrowseForFolderA (LPBROWSEINFOA lpbi)
{
	TRACE("(%p{lpszTitle=%s,owner=%i})\n", 
	      lpbi, debugstr_a(lpbi->lpszTitle), lpbi->hwndOwner);

	return (LPITEMIDLIST) DialogBoxParamA( shell32_hInstance,
					       "SHBRSFORFOLDER_MSGBOX",  
					       lpbi->hwndOwner,
					       BrsFolderDlgProc, (INT)lpbi );
}
