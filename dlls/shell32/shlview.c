/*
 *	ShellView
 *
 *	Copyright 1998,1999	<juergen.schmied@debitel.net>
 *
 * This is the view visualizing the data provided by the shellfolder.
 * No direct access to data from pidls should be done from here.
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
 * FIXME: The order by part of the background context menu should be
 * built according to the columns shown.
 *
 * FIXME: Load/Save the view state from/into the stream provided by
 * the ShellBrowser
 *
 * FIXME: CheckToolbar: handle the "new folder" and "folder up" button
 *
 * FIXME: ShellView_FillList: consider sort orders
 *
 * FIXME: implement the drag and drop in the old (msg-based) way
 *
 * FIXME: when the ShellView_WndProc gets a WM_NCDESTROY should we do a
 * Release() ???
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define CINTERFACE
#define COBJMACROS

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winnls.h"
#include "objbase.h"
#include "servprov.h"
#include "shlguid.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "shresdef.h"
#include "wine/debug.h"

#include "docobj.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellfolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Generic structure used by several messages */
typedef struct
{
  DWORD dwReserved;
  DWORD dwReserved2;
  LPCITEMIDLIST pidl;
  DWORD *lpdwUser;
} SFVCBINFO;

/* SFVCB_SELECTIONCHANGED structure */
typedef struct
{
  UINT uOldState;
  UINT uNewState;
  LPCITEMIDLIST pidl;
  DWORD *lpdwUser;
} SFVSELECTSTATE;

/* SFVCB_COPYHOOKCALLBACK structure */
typedef struct
{
  HWND hwnd;
  UINT wFunc;
  UINT wFlags;
  const char *pszSrcFile;
  DWORD dwSrcAttribs;
  const char *pszDestFile;
  DWORD dwDestAttribs;
} SFVCOPYHOOKINFO;

/* SFVCB_GETDETAILSOF structure */
typedef struct
{
    LPCITEMIDLIST pidl;
    int fmt;
    int cx;
    STRRET lpText;
} SFVCOLUMNINFO;

typedef struct
{   BOOL    bIsAscending;
    INT     nHeaderID;
    INT     nLastHeaderID;
}LISTVIEW_SORT_INFO, *LPLISTVIEW_SORT_INFO;

typedef struct
{
        IShellView3       IShellView3_iface;
        IOleCommandTarget IOleCommandTarget_iface;
        IDropTarget       IDropTarget_iface;
        IDropSource       IDropSource_iface;
        IViewObject       IViewObject_iface;
        IFolderView2      IFolderView2_iface;
        IShellFolderView  IShellFolderView_iface;
        IShellFolderViewDual3 IShellFolderViewDual3_iface;
        LONG              ref;
	IShellFolder*	pSFParent;
	IShellFolder2*	pSF2Parent;
	IShellBrowser*	pShellBrowser;
	ICommDlgBrowser*	pCommDlgBrowser;
	HWND		hWnd;		/* SHELLDLL_DefView */
	HWND		hWndList;	/* ListView control */
	HWND		hWndParent;
	FOLDERSETTINGS	FolderSettings;
	HMENU		hMenu;
	UINT		uState;
	UINT		cidl;
	LPITEMIDLIST	*apidl;
        LISTVIEW_SORT_INFO ListViewSortInfo;
	ULONG			hNotify;	/* change notification handle */
	HANDLE		hAccel;
	DWORD		dwAspects;
	DWORD		dwAdvf;
	IAdviseSink    *pAdvSink;
        IDropTarget*    pCurDropTarget; /* The sub-item, which is currently dragged over */
        IDataObject*    pCurDataObject; /* The dragged data-object */
        LONG            iDragOverItem;  /* Dragged over item's index, iff pCurDropTarget != NULL */
        UINT            cScrollDelay;   /* Send a WM_*SCROLL msg every 250 ms during drag-scroll */
        POINT           ptLastMousePos; /* Mouse position at last DragOver call */
        UINT            columns;        /* Number of shell folder columns */
} IShellViewImpl;

static inline IShellViewImpl *impl_from_IShellView3(IShellView3 *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IShellView3_iface);
}

static inline IShellViewImpl *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IOleCommandTarget_iface);
}

static inline IShellViewImpl *impl_from_IDropTarget(IDropTarget *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IDropTarget_iface);
}

static inline IShellViewImpl *impl_from_IDropSource(IDropSource *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IDropSource_iface);
}

static inline IShellViewImpl *impl_from_IViewObject(IViewObject *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IViewObject_iface);
}

static inline IShellViewImpl *impl_from_IFolderView2(IFolderView2 *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IFolderView2_iface);
}

static inline IShellViewImpl *impl_from_IShellFolderView(IShellFolderView *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IShellFolderView_iface);
}

static inline IShellViewImpl *impl_from_IShellFolderViewDual3(IShellFolderViewDual3 *iface)
{
    return CONTAINING_RECORD(iface, IShellViewImpl, IShellFolderViewDual3_iface);
}

/* ListView Header IDs */
#define LISTVIEW_COLUMN_NAME 0
#define LISTVIEW_COLUMN_SIZE 1
#define LISTVIEW_COLUMN_TYPE 2
#define LISTVIEW_COLUMN_TIME 3
#define LISTVIEW_COLUMN_ATTRIB 4

/*menu items */
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     1

#define SHV_CHANGE_NOTIFY WM_USER + 0x1111

/*windowsx.h */
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)

/*
  Items merged into the toolbar and the filemenu
*/
typedef struct
{  int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   BYTE  bState;
   BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

static const MYTOOLINFO Tools[] =
{
{ FCIDM_SHVIEW_BIGICON,    0, 0, IDS_VIEW_LARGE,   TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_SMALLICON,  0, 0, IDS_VIEW_SMALL,   TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_LISTVIEW,   0, 0, IDS_VIEW_LIST,    TBSTATE_ENABLED, BTNS_BUTTON },
{ FCIDM_SHVIEW_REPORTVIEW, 0, 0, IDS_VIEW_DETAILS, TBSTATE_ENABLED, BTNS_BUTTON },
{ -1, 0, 0, 0, 0, 0}
};

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

/**********************************************************
 *
 * ##### helperfunctions for communication with ICommDlgBrowser #####
 */
static BOOL IsInCommDlg(IShellViewImpl * This)
{
    return This->pCommDlgBrowser != NULL;
}

static HRESULT IncludeObject(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
    HRESULT ret = S_OK;

    if (IsInCommDlg(This))
    {
        TRACE("ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
        ret = ICommDlgBrowser_IncludeObject(This->pCommDlgBrowser, (IShellView *)&This->IShellView3_iface, pidl);
        TRACE("-- returns 0x%08lx\n", ret);
    }

    return ret;
}

static HRESULT OnDefaultCommand(IShellViewImpl * This)
{
    HRESULT ret = S_FALSE;

    if (IsInCommDlg(This))
    {
        TRACE("ICommDlgBrowser::OnDefaultCommand\n");
        ret = ICommDlgBrowser_OnDefaultCommand(This->pCommDlgBrowser, (IShellView *)&This->IShellView3_iface);
        TRACE("-- returns 0x%08lx\n", ret);
    }

    return ret;
}

static HRESULT OnStateChange(IShellViewImpl * This, UINT change)
{
    HRESULT ret = S_FALSE;

    if (IsInCommDlg(This))
    {
        TRACE("ICommDlgBrowser::OnStateChange change=%d\n", change);
        ret = ICommDlgBrowser_OnStateChange(This->pCommDlgBrowser, (IShellView *)&This->IShellView3_iface, change);
        TRACE("-- returns 0x%08lx\n", ret);
    }

    return ret;
}

/**********************************************************
 *	set the toolbar of the filedialog buttons
 *
 * - activates the buttons from the shellbrowser according to
 *   the view state
 */
static void CheckToolbar(IShellViewImpl * This)
{
	LRESULT result;

	TRACE("\n");

	if (IsInCommDlg(This))
	{
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
                FCIDM_TB_SMALLICON, This->FolderSettings.ViewMode == FVM_LIST, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
                FCIDM_TB_REPORTVIEW, This->FolderSettings.ViewMode == FVM_DETAILS, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_SMALLICON, TRUE, &result);
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_REPORTVIEW, TRUE, &result);
	}
}

/**********************************************************
 *
 * ##### helperfunctions for initializing the view #####
 */
/**********************************************************
 *	change the style of the listview control
 */
static void SetStyle(IShellViewImpl * This, DWORD dwAdd, DWORD dwRemove)
{
	DWORD tmpstyle;

	TRACE("(%p)\n", This);

	tmpstyle = GetWindowLongW(This->hWndList, GWL_STYLE);
	SetWindowLongW(This->hWndList, GWL_STYLE, dwAdd | (tmpstyle & ~dwRemove));
}

static DWORD ViewModeToListStyle(UINT ViewMode)
{
	DWORD dwStyle;

	TRACE("%d\n", ViewMode);

	switch (ViewMode)
	{
	  case FVM_ICON:	dwStyle = LVS_ICON;		break;
	  case FVM_DETAILS:	dwStyle = LVS_REPORT;		break;
	  case FVM_SMALLICON:	dwStyle = LVS_SMALLICON;	break;
	  case FVM_LIST:	dwStyle = LVS_LIST;		break;
	  default:
	  {
		FIXME("ViewMode %d not implemented\n", ViewMode);
		dwStyle = LVS_LIST;
		break;
	  }
	}

	return dwStyle;
}

/**********************************************************
* ShellView_CreateList()
*
* - creates the list view window
*/
static BOOL ShellView_CreateList (IShellViewImpl * This)
{	DWORD dwStyle, dwExStyle;

	TRACE("%p\n",This);

	dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		  LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS;
        dwExStyle = WS_EX_CLIENTEDGE;

        dwStyle |= ViewModeToListStyle(This->FolderSettings.ViewMode);

	if (This->FolderSettings.fFlags & FWF_AUTOARRANGE)	dwStyle |= LVS_AUTOARRANGE;
	if (This->FolderSettings.fFlags & FWF_DESKTOP)
	  This->FolderSettings.fFlags |= FWF_NOCLIENTEDGE | FWF_NOSCROLL;
	if (This->FolderSettings.fFlags & FWF_SINGLESEL)	dwStyle |= LVS_SINGLESEL;
	if (This->FolderSettings.fFlags & FWF_NOCLIENTEDGE)
	  dwExStyle &= ~WS_EX_CLIENTEDGE;

	This->hWndList=CreateWindowExW( dwExStyle,
					WC_LISTVIEWW,
					NULL,
					dwStyle,
					0,0,0,0,
					This->hWnd,
					(HMENU)ID_LISTVIEW,
					shell32_hInstance,
					NULL);

	if(!This->hWndList)
	  return FALSE;

        This->ListViewSortInfo.bIsAscending = TRUE;
        This->ListViewSortInfo.nHeaderID = -1;
        This->ListViewSortInfo.nLastHeaderID = -1;

        if (This->FolderSettings.fFlags & FWF_DESKTOP) {
          /*
           * FIXME: look at the registry value
           * HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced\ListviewShadow
           * and activate drop shadows if necessary
           */
           if (0)
             SendMessageW(This->hWndList, LVM_SETTEXTBKCOLOR, 0, CLR_NONE);
           else
             SendMessageW(This->hWndList, LVM_SETTEXTBKCOLOR, 0, GetSysColor(COLOR_DESKTOP));

           SendMessageW(This->hWndList, LVM_SETTEXTCOLOR, 0, RGB(255,255,255));
        }

        /*  UpdateShellSettings(); */
	return TRUE;
}

/**********************************************************
* ShellView_InitList()
*
* - adds all needed columns to the shellview
*/
static void ShellView_InitList(IShellViewImpl *This)
{
    IShellDetails *details = NULL;
    HIMAGELIST big_icons, small_icons;
    LVCOLUMNW lvColumn;
    SHELLDETAILS sd;
    WCHAR nameW[50];
    HRESULT hr;
    HFONT list_font, old_font;
    HDC list_dc;
    TEXTMETRICW tm;

    TRACE("(%p)\n", This);

    Shell_GetImageLists( &big_icons, &small_icons );
    SendMessageW(This->hWndList, LVM_DELETEALLITEMS, 0, 0);
    SendMessageW(This->hWndList, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)small_icons);
    SendMessageW(This->hWndList, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)big_icons);

    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvColumn.pszText = nameW;

    if (!This->pSF2Parent)
    {
        hr = IShellFolder_QueryInterface(This->pSFParent, &IID_IShellDetails, (void**)&details);
        if (hr != S_OK)
        {
            WARN("IShellFolder2/IShellDetails not supported\n");
            return;
        }
    }

    list_font = (HFONT)SendMessageW(This->hWndList, WM_GETFONT, 0, 0);
    list_dc = GetDC(This->hWndList);
    old_font = SelectObject(list_dc, list_font);
    GetTextMetricsW(list_dc, &tm);
    SelectObject(list_dc, old_font);
    ReleaseDC(This->hWndList, list_dc);

    for (This->columns = 0;; This->columns++)
    {
        if (This->pSF2Parent)
            hr = IShellFolder2_GetDetailsOf(This->pSF2Parent, NULL, This->columns, &sd);
        else
            hr = IShellDetails_GetDetailsOf(details, NULL, This->columns, &sd);
        if (FAILED(hr)) break;

        lvColumn.fmt = sd.fmt;
        lvColumn.cx = MulDiv(sd.cxChar, tm.tmAveCharWidth * 3, 2); /* chars->pixel */
        StrRetToStrNW(nameW, ARRAY_SIZE(nameW), &sd.str, NULL);
        SendMessageW(This->hWndList, LVM_INSERTCOLUMNW, This->columns, (LPARAM)&lvColumn);
    }

    if (details) IShellDetails_Release(details);
}

/* LVM_SORTITEMS callback used when initially inserting items */
static INT CALLBACK ShellView_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
    LPITEMIDLIST pidl1 = (LPITEMIDLIST)lParam1;
    LPITEMIDLIST pidl2 = (LPITEMIDLIST)lParam2;
    IShellFolder *folder = (IShellFolder *)lpData;
    int ret;

    TRACE("pidl1=%p, pidl2=%p, shellfolder=%p\n", pidl1, pidl2, folder);

    ret = (SHORT)HRESULT_CODE(IShellFolder_CompareIDs(folder, 0, pidl1, pidl2));
    TRACE("ret=%i\n", ret);
    return ret;
}

/*************************************************************************
 * ShellView_ListViewCompareItems
 *
 * Compare Function for the Listview (FileOpen Dialog)
 *
 * PARAMS
 *     lParam1       [I] the first ItemIdList to compare with
 *     lParam2       [I] the second ItemIdList to compare with
 *     lpData        [I] The column ID for the header Ctrl to process
 *
 * RETURNS
 *     A negative value if the first item should precede the second,
 *     a positive value if the first item should follow the second,
 *     or zero if the two items are equivalent
 *
 * NOTES
 *	FIXME: function does what ShellView_CompareItems is supposed to do.
 *	unify it and figure out how to use the undocumented first parameter
 *	of IShellFolder_CompareIDs to do the job this function does and
 *	move this code to IShellFolder.
 *	make LISTVIEW_SORT_INFO obsolete
 *	the way this function works is only usable if we had only
 *	filesystemfolders  (25/10/99 jsch)
 */
static INT CALLBACK ShellView_ListViewCompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
    INT nDiff=0;
    FILETIME fd1, fd2;
    char strName1[MAX_PATH], strName2[MAX_PATH];
    BOOL bIsFolder1, bIsFolder2,bIsBothFolder;
    LPITEMIDLIST pItemIdList1 = lParam1;
    LPITEMIDLIST pItemIdList2 = lParam2;
    LISTVIEW_SORT_INFO *pSortInfo = (LPLISTVIEW_SORT_INFO) lpData;


    bIsFolder1 = _ILIsFolder(pItemIdList1);
    bIsFolder2 = _ILIsFolder(pItemIdList2);
    bIsBothFolder = bIsFolder1 && bIsFolder2;

    /* When sorting between a File and a Folder, the Folder gets sorted first */
    if( (bIsFolder1 || bIsFolder2) && !bIsBothFolder)
    {
        nDiff = bIsFolder1 ? -1 : 1;
    }
    else
    {
        /* Sort by Time: Folders or Files can be sorted */

        if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_TIME)
        {
            _ILGetFileDateTime(pItemIdList1, &fd1);
            _ILGetFileDateTime(pItemIdList2, &fd2);
            nDiff = CompareFileTime(&fd2, &fd1);
        }
        /* Sort by Attribute: Folder or Files can be sorted */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_ATTRIB)
        {
            _ILGetFileAttributes(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileAttributes(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by FileName: Folder or Files can be sorted */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_NAME || bIsBothFolder)
        {
            /* Sort by Text */
            _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
            _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
        /* Sort by File Size, Only valid for Files */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_SIZE)
        {
            nDiff = (INT)(_ILGetFileSize(pItemIdList1, NULL, 0) - _ILGetFileSize(pItemIdList2, NULL, 0));
        }
        /* Sort by File Type, Only valid for Files */
        else if(pSortInfo->nHeaderID == LISTVIEW_COLUMN_TYPE)
        {
            /* Sort by Type */
            _ILGetFileType(pItemIdList1, strName1, MAX_PATH);
            _ILGetFileType(pItemIdList2, strName2, MAX_PATH);
            nDiff = lstrcmpiA(strName1, strName2);
        }
    }
    /*  If the Date, FileSize, FileType, Attrib was the same, sort by FileName */

    if(nDiff == 0)
    {
        _ILSimpleGetText(pItemIdList1, strName1, MAX_PATH);
        _ILSimpleGetText(pItemIdList2, strName2, MAX_PATH);
        nDiff = lstrcmpiA(strName1, strName2);
    }

    if(!pSortInfo->bIsAscending)
    {
        nDiff = -nDiff;
    }

    return nDiff;

}

/**********************************************************
*  LV_FindItemByPidl()
*/
static int LV_FindItemByPidl(
	IShellViewImpl * This,
	LPCITEMIDLIST pidl)
{
	LVITEMW lvItem;
	lvItem.iSubItem = 0;
	lvItem.mask = LVIF_PARAM;
	for(lvItem.iItem = 0;
		SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);
		lvItem.iItem++)
	{
	  LPITEMIDLIST currentpidl = (LPITEMIDLIST) lvItem.lParam;
	  HRESULT hr = IShellFolder_CompareIDs(This->pSFParent, 0, pidl, currentpidl);
	  if(SUCCEEDED(hr) && !HRESULT_CODE(hr))
	  {
	    return lvItem.iItem;
	  }
  	}
	return -1;
}

static void shellview_add_item(IShellViewImpl *shellview, LPCITEMIDLIST pidl)
{
    LVITEMW item;
    UINT i;

    TRACE("(%p)(pidl=%p)\n", shellview, pidl);

    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = (LPARAM)pidl;
    item.pszText = LPSTR_TEXTCALLBACKW;
    item.iImage = I_IMAGECALLBACK;
    SendMessageW(shellview->hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);

    for (i = 1; i < shellview->columns; i++)
    {
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 1;
        item.pszText = LPSTR_TEXTCALLBACKW;
        SendMessageW(shellview->hWndList, LVM_SETITEMW, 0, (LPARAM)&item);
    }
}

/**********************************************************
* LV_RenameItem()
*/
static BOOLEAN LV_RenameItem(IShellViewImpl * This, LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew )
{
	int nItem;
	LVITEMW lvItem;

	TRACE("(%p)(pidlold=%p pidlnew=%p)\n", This, pidlOld, pidlNew);

	nItem = LV_FindItemByPidl(This, ILFindLastID(pidlOld));
	if ( -1 != nItem )
	{
	  lvItem.mask = LVIF_PARAM;		/* only the pidl */
	  lvItem.iItem = nItem;
	  SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

	  SHFree((LPITEMIDLIST)lvItem.lParam);
	  lvItem.mask = LVIF_PARAM;
	  lvItem.iItem = nItem;
	  lvItem.lParam = (LPARAM) ILClone(ILFindLastID(pidlNew));	/* set the item's data */
	  SendMessageW(This->hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
	  SendMessageW(This->hWndList, LVM_UPDATE, nItem, 0);
	  return TRUE;					/* FIXME: better handling */
	}
	return FALSE;
}
/**********************************************************
* ShellView_FillList()
*
* - gets the objectlist from the shellfolder
* - sorts the list
* - fills the list into the view
*/

static HRESULT ShellView_FillList(IShellViewImpl *This)
{
    IFolderView2 *folderview = &This->IFolderView2_iface;
    LPENUMIDLIST pEnumIDList;
    LPITEMIDLIST pidl;
    DWORD fetched;
    HRESULT hr;

    TRACE("(%p)\n", This);

    /* get the itemlist from the shfolder*/
    hr = IShellFolder_EnumObjects(This->pSFParent, This->hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
    if (hr != S_OK)
        return hr;

    IFolderView2_SetRedraw(folderview, FALSE);

    /* copy the items into the array */
    while ((S_OK == IEnumIDList_Next(pEnumIDList, 1, &pidl, &fetched)) && fetched)
    {
        if (IncludeObject(This, pidl) == S_OK)
            shellview_add_item(This, pidl);
        else
            ILFree(pidl);
    }

    SendMessageW(This->hWndList, LVM_SORTITEMS, (WPARAM)This->pSFParent, (LPARAM)ShellView_CompareItems);

    IFolderView2_SetRedraw(folderview, TRUE);

    IEnumIDList_Release(pEnumIDList);
    return S_OK;
}

/**********************************************************
*  ShellView_OnCreate()
*/
static LRESULT ShellView_OnCreate(IShellViewImpl *This)
{
    IShellView3 *iface = &This->IShellView3_iface;
    IPersistFolder2 *ppf2;
    IDropTarget* pdt;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (ShellView_CreateList(This))
    {
        ShellView_InitList(This);
        ShellView_FillList(This);
    }

    hr = IShellView3_QueryInterface(iface, &IID_IDropTarget, (void**)&pdt);
    if (hr == S_OK)
    {
        RegisterDragDrop(This->hWnd, pdt);
        IDropTarget_Release(pdt);
    }

    /* register for receiving notifications */
    hr = IShellFolder_QueryInterface(This->pSFParent, &IID_IPersistFolder2, (LPVOID*)&ppf2);
    if (hr == S_OK)
    {
        LPITEMIDLIST raw_pidl;
        SHChangeNotifyEntry ntreg;

        hr = IPersistFolder2_GetCurFolder(ppf2, &raw_pidl);
        if(SUCCEEDED(hr))
        {
            LPITEMIDLIST computer_pidl;
            SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&computer_pidl);
            if(ILIsParent(computer_pidl,raw_pidl,FALSE))
            {
                /* Normalize the pidl to unixfs to workaround an issue with
                 * sending notifications on dos paths
                 */
                WCHAR path[MAX_PATH];
                SHGetPathFromIDListW(raw_pidl,path);
                SHParseDisplayName(path,NULL,(LPITEMIDLIST*)&ntreg.pidl,0,NULL);
                SHFree(raw_pidl);
            }
            else
                ntreg.pidl = raw_pidl;
            ntreg.fRecursive = TRUE;
            This->hNotify = SHChangeNotifyRegister(This->hWnd, SHCNRF_InterruptLevel, SHCNE_ALLEVENTS,
                                                   SHV_CHANGE_NOTIFY, 1, &ntreg);
            SHFree((LPITEMIDLIST)ntreg.pidl);
            SHFree(computer_pidl);
        }
        IPersistFolder2_Release(ppf2);
    }

    This->hAccel = LoadAcceleratorsW(shell32_hInstance, L"shv_accel");

    return S_OK;
}

/**********************************************************
 *	#### Handling of the menus ####
 */

/**********************************************************
* ShellView_BuildFileMenu()
*/
static HMENU ShellView_BuildFileMenu(IShellViewImpl * This)
{	WCHAR	szText[MAX_PATH];
	MENUITEMINFOW	mii;
	int	nTools,i;
	HMENU	hSubMenu;

	TRACE("(%p)\n",This);

	hSubMenu = CreatePopupMenu();
	if(hSubMenu)
	{ /*get the number of items in our global array*/
	  for(nTools = 0; Tools[nTools].idCommand != -1; nTools++){}

	  /*add the menu items*/
	  for(i = 0; i < nTools; i++)
	  {
	    LoadStringW(shell32_hInstance, Tools[i].idMenuString, szText, MAX_PATH);

	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	    if(BTNS_SEP != Tools[i].bStyle) /* no separator*/
	    {
	      mii.fType = MFT_STRING;
	      mii.fState = MFS_ENABLED;
	      mii.dwTypeData = szText;
	      mii.wID = Tools[i].idCommand;
	    }
	    else
	    {
	      mii.fType = MFT_SEPARATOR;
	    }
	    /* tack This item onto the end of the menu */
	    InsertMenuItemW(hSubMenu, (UINT)-1, TRUE, &mii);
	  }
	}
	TRACE("-- return (menu=%p)\n",hSubMenu);
	return hSubMenu;
}

static void ShellView_MergeFileMenu(IShellViewImpl *This, HMENU hSubMenu)
{
     if (hSubMenu)
     {
         MENUITEMINFOW mii;

         /* insert This item at the beginning of the menu */

         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE;
         mii.wID = 0;
         mii.fType = MFT_SEPARATOR;
         InsertMenuItemW(hSubMenu, 0, TRUE, &mii);

         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
         mii.dwTypeData = (LPWSTR)L"dummy45";
         mii.fState = MFS_ENABLED;
         mii.wID = IDM_MYFILEITEM;
         mii.fType = MFT_STRING;
         InsertMenuItemW(hSubMenu, 0, TRUE, &mii);
    }
}

/**********************************************************
* ShellView_MergeViewMenu()
*/

static void ShellView_MergeViewMenu(IShellViewImpl *This, HMENU hSubMenu)
{
    TRACE("(%p)->(submenu=%p)\n",This,hSubMenu);

    /* add a separator at the correct position in the menu */
    if (hSubMenu)
    {
        MENUITEMINFOW mii;

        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.wID = 0;
        mii.fType = MFT_SEPARATOR;
        InsertMenuItemW(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
        mii.fType = MFT_STRING;
        mii.dwTypeData = (LPWSTR)L"View";
        mii.hSubMenu = LoadMenuW(shell32_hInstance, L"MENU_001");
        InsertMenuItemW(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
    }
}

/**********************************************************
*   ShellView_GetSelections()
*
* - fills the this->apidl list with the selected objects
*
* RETURNS
*  number of selected items
*/
static UINT ShellView_GetSelections(IShellViewImpl * This)
{
	LVITEMW	lvItem;
	UINT	i = 0;

	SHFree(This->apidl);

	This->cidl = SendMessageW(This->hWndList, LVM_GETSELECTEDCOUNT, 0, 0);
	This->apidl = SHAlloc(This->cidl * sizeof(LPITEMIDLIST));

	TRACE("selected=%i\n", This->cidl);

	if(This->apidl)
	{
	  TRACE("-- Items selected =%u\n", This->cidl);

	  lvItem.mask = LVIF_STATE | LVIF_PARAM;
	  lvItem.stateMask = LVIS_SELECTED;
	  lvItem.iItem = 0;
	  lvItem.iSubItem = 0;

	  while(ListView_GetItemW(This->hWndList, &lvItem) && (i < This->cidl))
	  {
	    if(lvItem.state & LVIS_SELECTED)
	    {
	      This->apidl[i] = (LPITEMIDLIST)lvItem.lParam;
	      i++;
	      TRACE("-- selected Item found\n");
	    }
	    lvItem.iItem++;
	  }
	}
	return This->cidl;

}

/**********************************************************
 *	ShellView_OpenSelectedItems()
 */
static HRESULT ShellView_OpenSelectedItems(IShellViewImpl * This)
{
	static UINT CF_IDLIST = 0;
	HRESULT hr;
	IDataObject* selection;
	FORMATETC fetc;
	STGMEDIUM stgm;
	LPIDA pIDList;
	LPCITEMIDLIST parent_pidl;
	WCHAR parent_path[MAX_PATH];
	LPCWSTR parent_dir = NULL;
	SFGAOF attribs;
	int i;

	if (0 == ShellView_GetSelections(This))
	{
	  return S_OK;
	}
	hr = IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl,
	                                (LPCITEMIDLIST*)This->apidl, &IID_IDataObject,
	                                0, (LPVOID *)&selection);
	if (FAILED(hr))
	  return hr;

	if (0 == CF_IDLIST)
	{
	  CF_IDLIST = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
	}
	fetc.cfFormat = CF_IDLIST;
	fetc.ptd = NULL;
	fetc.dwAspect = DVASPECT_CONTENT;
	fetc.lindex = -1;
	fetc.tymed = TYMED_HGLOBAL;

	hr = IDataObject_QueryGetData(selection, &fetc);
	if (FAILED(hr))
	  return hr;

	hr = IDataObject_GetData(selection, &fetc, &stgm);
	if (FAILED(hr))
	  return hr;

	pIDList = GlobalLock(stgm.hGlobal);

	parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pIDList+pIDList->aoffset[0]);
	hr = IShellFolder_GetAttributesOf(This->pSFParent, 1, &parent_pidl, &attribs);
	if (SUCCEEDED(hr) && (attribs & SFGAO_FILESYSTEM) &&
	    SHGetPathFromIDListW(parent_pidl, parent_path))
	{
	  parent_dir = parent_path;
	}

	for (i = pIDList->cidl; i > 0; --i)
	{
	  LPCITEMIDLIST pidl;

	  pidl = (LPCITEMIDLIST)((LPBYTE)pIDList+pIDList->aoffset[i]);

	  attribs = SFGAO_FOLDER;
	  hr = IShellFolder_GetAttributesOf(This->pSFParent, 1, &pidl, &attribs);

	  if (SUCCEEDED(hr) && ! (attribs & SFGAO_FOLDER))
	  {
	    SHELLEXECUTEINFOW shexinfo;

	    shexinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
	    shexinfo.fMask = SEE_MASK_INVOKEIDLIST;	/* SEE_MASK_IDLIST is also possible. */
	    shexinfo.hwnd = NULL;
	    shexinfo.lpVerb = NULL;
	    shexinfo.lpFile = NULL;
	    shexinfo.lpParameters = NULL;
	    shexinfo.lpDirectory = parent_dir;
	    shexinfo.nShow = SW_NORMAL;
	    shexinfo.lpIDList = ILCombine(parent_pidl, pidl);

	    ShellExecuteExW(&shexinfo);    /* Discard error/success info */

            ILFree(shexinfo.lpIDList);
	  }
	}

	GlobalUnlock(stgm.hGlobal);
	ReleaseStgMedium(&stgm);

	IDataObject_Release(selection);

	return S_OK;
}

/**********************************************************
 *	ShellView_DoContextMenu()
 */
static void ShellView_DoContextMenu(IShellViewImpl * This, WORD x, WORD y, BOOL bDefault)
{	UINT	uCommand;
	DWORD	wFlags;
	HMENU	hMenu;
	BOOL	fExplore = FALSE;
	HWND	hwndTree = 0;
	LPCONTEXTMENU	pContextMenu = NULL;
	CMINVOKECOMMANDINFO	cmi;

	TRACE("%p, %d, %d, %d.\n", This, x, y, bDefault);

	/* look, what's selected and create a context menu object of it*/
	if( ShellView_GetSelections(This) )
	{
	  IShellFolder_GetUIObjectOf( This->pSFParent, This->hWndParent, This->cidl, (LPCITEMIDLIST*)This->apidl,
                                      &IID_IContextMenu, NULL, (LPVOID *)&pContextMenu);

	  if(pContextMenu)
	  {
	    TRACE("-- pContextMenu\n");
	    hMenu = CreatePopupMenu();

	    if( hMenu )
	    {
	      /* See if we are in Explore or Open mode. If the browser's tree is present, we are in Explore mode.*/
	      if(SUCCEEDED(IShellBrowser_GetControlWindow(This->pShellBrowser,FCW_TREE, &hwndTree)) && hwndTree)
	      {
	        TRACE("-- explore mode\n");
	        fExplore = TRUE;
	      }

	      /* build the flags depending on what we can do with the selected item */
	      wFlags = CMF_NORMAL | (This->cidl != 1 ? 0 : CMF_CANRENAME) | (fExplore ? CMF_EXPLORE : 0);

	      /* let the ContextMenu merge its items in */
	      if (SUCCEEDED(IContextMenu_QueryContextMenu( pContextMenu, hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, wFlags )))
	      {
	        if (This->FolderSettings.fFlags & FWF_DESKTOP)
		  SetMenuDefaultItem(hMenu, FCIDM_SHVIEW_OPEN, MF_BYCOMMAND);

		if( bDefault )
		{
		  TRACE("-- get menu default command\n");
		  uCommand = GetMenuDefaultItem(hMenu, FALSE, GMDI_GOINTOPOPUPS);
		}
		else
		{
		  TRACE("-- track popup\n");
		  uCommand = TrackPopupMenu( hMenu,TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
		}

		if(uCommand > 0)
		{
		  TRACE("-- uCommand=%u\n", uCommand);
		  if (uCommand==FCIDM_SHVIEW_OPEN && IsInCommDlg(This))
		  {
		    TRACE("-- dlg: OnDefaultCommand\n");
		    if (OnDefaultCommand(This) != S_OK)
		    {
		      ShellView_OpenSelectedItems(This);
		    }
		  }
		  else
		  {
		    TRACE("-- explore -- invoke command\n");
		    ZeroMemory(&cmi, sizeof(cmi));
		    cmi.cbSize = sizeof(cmi);
		    cmi.hwnd = This->hWndParent; /* this window has to answer CWM_GETISHELLBROWSER */
                    cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
		    IContextMenu_InvokeCommand(pContextMenu, &cmi);
		  }
		}
		DestroyMenu(hMenu);
	      }
	    }
	    if (pContextMenu)
	      IContextMenu_Release(pContextMenu);
	  }
	}
	else	/* background context menu */
	{
	  IContextMenu2 *pCM;

	  hMenu = CreatePopupMenu();

	  BackgroundMenu_Constructor(This->pSFParent, FALSE, &IID_IContextMenu2, (void**)&pCM);
	  IContextMenu2_QueryContextMenu(pCM, hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, 0);

	  uCommand = TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
	  DestroyMenu(hMenu);

	  TRACE("-- (%p)->(uCommand=0x%08x )\n",This, uCommand);

	  ZeroMemory(&cmi, sizeof(cmi));
	  cmi.cbSize = sizeof(cmi);
          cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
	  cmi.hwnd = This->hWndParent;
	  IContextMenu2_InvokeCommand(pCM, &cmi);

	  IContextMenu2_Release(pCM);
	}
}

/**********************************************************
 *	##### message handling #####
 */

/**********************************************************
*  ShellView_OnSize()
*/
static LRESULT ShellView_OnSize(IShellViewImpl * This, WORD wWidth, WORD wHeight)
{
	TRACE("%p width=%u height=%u\n",This, wWidth,wHeight);

	/*resize the ListView to fit our window*/
	if(This->hWndList)
	{
	  MoveWindow(This->hWndList, 0, 0, wWidth, wHeight, TRUE);
	}

	return S_OK;
}
/**********************************************************
* ShellView_OnDeactivate()
*
* NOTES
*  internal
*/
static void ShellView_OnDeactivate(IShellViewImpl * This)
{
	TRACE("%p\n",This);

	if(This->uState != SVUIA_DEACTIVATE)
	{
	  if(This->hMenu)
	  {
	    IShellBrowser_SetMenuSB(This->pShellBrowser,0, 0, 0);
	    IShellBrowser_RemoveMenusSB(This->pShellBrowser,This->hMenu);
	    DestroyMenu(This->hMenu);
	    This->hMenu = 0;
	  }

	  This->uState = SVUIA_DEACTIVATE;
	}
}

/**********************************************************
* ShellView_OnActivate()
*/
static LRESULT ShellView_OnActivate(IShellViewImpl *This, UINT uState)
{
    OLEMENUGROUPWIDTHS   omw = { {0, 0, 0, 0, 0, 0} };
    MENUITEMINFOW mii;

    TRACE("(%p) uState=%x\n",This,uState);

    /* don't do anything if the state isn't really changing */
    if (This->uState == uState) return S_OK;

    ShellView_OnDeactivate(This);

    /* only do This if we are active */
    if (uState != SVUIA_DEACTIVATE)
    {
        /* merge the menus */
        This->hMenu = CreateMenu();

        if (This->hMenu)
	{
            IShellBrowser_InsertMenusSB(This->pShellBrowser, This->hMenu, &omw);
	    TRACE("-- after fnInsertMenusSB\n");

            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.wID = 0;
            mii.hSubMenu = ShellView_BuildFileMenu(This);
            mii.hbmpChecked = NULL;
            mii.hbmpUnchecked = NULL;
            mii.dwItemData = 0;
            /* build the top level menu get the menu item's text */
            mii.dwTypeData = (LPWSTR)L"dummy 31";
            mii.cch = 0;
            mii.hbmpItem = NULL;

            /* insert our menu into the menu bar */
            if (mii.hSubMenu)
                InsertMenuItemW(This->hMenu, FCIDM_MENU_HELP, FALSE, &mii);

            /* get the view menu so we can merge with it */
            memset(&mii, 0, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;

            if (GetMenuItemInfoW(This->hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
                ShellView_MergeViewMenu(This, mii.hSubMenu);

            /* add the items that should only be added if we have the focus */
	    if (SVUIA_ACTIVATE_FOCUS == uState)
	    {
                /* get the file menu so we can merge with it */
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_SUBMENU;

                if (GetMenuItemInfoW(This->hMenu, FCIDM_MENU_FILE, FALSE, &mii))
                    ShellView_MergeFileMenu(This, mii.hSubMenu);
	    }

            TRACE("-- before fnSetMenuSB\n");
            IShellBrowser_SetMenuSB(This->pShellBrowser, This->hMenu, 0, This->hWnd);
	}
    }
    This->uState = uState;
    TRACE("--\n");
    return S_OK;
}

/**********************************************************
*  ShellView_OnSetFocus()
*
*/
static LRESULT ShellView_OnSetFocus(IShellViewImpl * This)
{
	TRACE("%p\n",This);

	/* Tell the browser one of our windows has received the focus. This
	should always be done before merging menus (OnActivate merges the
	menus) if one of our windows has the focus.*/

	IShellBrowser_OnViewWindowActive(This->pShellBrowser, (IShellView *)&This->IShellView3_iface);
	ShellView_OnActivate(This, SVUIA_ACTIVATE_FOCUS);

	/* Set the focus to the listview */
	SetFocus(This->hWndList);

	/* Notify the ICommDlgBrowser interface */
	OnStateChange(This,CDBOSC_SETFOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnKillFocus()
*/
static LRESULT ShellView_OnKillFocus(IShellViewImpl * This)
{
	TRACE("%p.\n", This);

	ShellView_OnActivate(This, SVUIA_ACTIVATE_NOFOCUS);
	/* Notify the ICommDlgBrowser */
	OnStateChange(This,CDBOSC_KILLFOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnCommand()
*
* NOTES
*	the CmdIDs are the ones from the context menu
*/
static LRESULT ShellView_OnCommand(IShellViewImpl * This,DWORD dwCmdID, DWORD dwCmd, HWND hwndCmd)
{
	TRACE("(%p)->(0x%08lx 0x%08lx %p) stub\n",This, dwCmdID, dwCmd, hwndCmd);

	switch(dwCmdID)
	{
	  case FCIDM_SHVIEW_SMALLICON:
	    This->FolderSettings.ViewMode = FVM_SMALLICON;
	    SetStyle (This, LVS_SMALLICON, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_BIGICON:
	    This->FolderSettings.ViewMode = FVM_ICON;
	    SetStyle (This, LVS_ICON, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_LISTVIEW:
	    This->FolderSettings.ViewMode = FVM_LIST;
	    SetStyle (This, LVS_LIST, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  case FCIDM_SHVIEW_REPORTVIEW:
	    This->FolderSettings.ViewMode = FVM_DETAILS;
	    SetStyle (This, LVS_REPORT, LVS_TYPEMASK);
	    CheckToolbar(This);
	    break;

	  /* the menu IDs for sorting are 0x30... see shell32.rc */
	  case 0x30:
	  case 0x31:
	  case 0x32:
	  case 0x33:
            This->ListViewSortInfo.nHeaderID = dwCmdID - 0x30;
	    This->ListViewSortInfo.bIsAscending = TRUE;
	    This->ListViewSortInfo.nLastHeaderID = This->ListViewSortInfo.nHeaderID;
	    SendMessageW(This->hWndList, LVM_SORTITEMS, (WPARAM) &This->ListViewSortInfo, (LPARAM)ShellView_ListViewCompareItems);
	    break;

	  default:
	    TRACE("-- COMMAND 0x%04lx unhandled\n", dwCmdID);
	}
	return 0;
}

/**********************************************************
* ShellView_OnNotify()
*/

static LRESULT ShellView_OnNotify(IShellViewImpl * This, UINT CtlID, LPNMHDR lpnmh)
{	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lpnmh;
	NMLVDISPINFOW *lpdi = (NMLVDISPINFOW *)lpnmh;
	LPITEMIDLIST pidl;

	TRACE("%p CtlID=%u lpnmh->code=%x\n",This,CtlID,lpnmh->code);

	switch(lpnmh->code)
	{
	  case NM_SETFOCUS:
	    TRACE("-- NM_SETFOCUS %p\n",This);
	    ShellView_OnSetFocus(This);
	    break;

	  case NM_KILLFOCUS:
	    TRACE("-- NM_KILLFOCUS %p\n",This);
	    ShellView_OnDeactivate(This);
	    /* Notify the ICommDlgBrowser interface */
	    OnStateChange(This,CDBOSC_KILLFOCUS);
	    break;

	  case NM_CUSTOMDRAW:
	    TRACE("-- NM_CUSTOMDRAW %p\n",This);
	    return CDRF_DODEFAULT;

	  case NM_RELEASEDCAPTURE:
	    TRACE("-- NM_RELEASEDCAPTURE %p\n",This);
	    break;

	  case NM_CLICK:
	    TRACE("-- NM_CLICK %p\n",This);
	    break;

	  case NM_RCLICK:
	    TRACE("-- NM_RCLICK %p\n",This);
	    break;	    

          case NM_DBLCLK:
            TRACE("-- NM_DBLCLK %p\n",This);
            if (OnDefaultCommand(This) != S_OK) ShellView_OpenSelectedItems(This);
            break;

          case NM_RETURN:
            TRACE("-- NM_RETURN %p\n",This);
            if (OnDefaultCommand(This) != S_OK) ShellView_OpenSelectedItems(This);
            break;

	  case HDN_ENDTRACKW:
	    TRACE("-- HDN_ENDTRACKW %p\n",This);
	    /*nColumn1 = ListView_GetColumnWidth(This->hWndList, 0);
	    nColumn2 = ListView_GetColumnWidth(This->hWndList, 1);*/
	    break;

	  case LVN_DELETEITEM:
	    TRACE("-- LVN_DELETEITEM %p\n",This);
	    SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
	    break;

	  case LVN_DELETEALLITEMS:
	    TRACE("-- LVN_DELETEALLITEMS %p\n",This);
	    return FALSE;

	  case LVN_INSERTITEM:
	    TRACE("-- LVN_INSERTITEM (STUB)%p\n",This);
	    break;

	  case LVN_ITEMACTIVATE:
	    TRACE("-- LVN_ITEMACTIVATE %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  case LVN_COLUMNCLICK:
	    This->ListViewSortInfo.nHeaderID = lpnmlv->iSubItem;
	    if(This->ListViewSortInfo.nLastHeaderID == This->ListViewSortInfo.nHeaderID)
	    {
	      This->ListViewSortInfo.bIsAscending = !This->ListViewSortInfo.bIsAscending;
	    }
	    else
	    {
	      This->ListViewSortInfo.bIsAscending = TRUE;
	    }
	    This->ListViewSortInfo.nLastHeaderID = This->ListViewSortInfo.nHeaderID;

	    SendMessageW(lpnmlv->hdr.hwndFrom, LVM_SORTITEMS, (WPARAM) &This->ListViewSortInfo, (LPARAM)ShellView_ListViewCompareItems);
	    break;

	  case LVN_GETDISPINFOA:
          case LVN_GETDISPINFOW:
	    TRACE("-- LVN_GETDISPINFO %p\n",This);
	    pidl = (LPITEMIDLIST)lpdi->item.lParam;

	    if(lpdi->item.mask & LVIF_TEXT)	/* text requested */
	    {
	      static WCHAR emptyW[] = { 0 };
	      SHELLDETAILS sd;
	      HRESULT hr;

	      if (This->pSF2Parent)
	      {
	        hr = IShellFolder2_GetDetailsOf(This->pSF2Parent, pidl, lpdi->item.iSubItem, &sd);
	      }
	      else
	      {
	        IShellDetails *details;

	        hr = IShellFolder_QueryInterface(This->pSFParent, &IID_IShellDetails, (void**)&details);
	        if (hr == S_OK)
	        {
	          hr = IShellDetails_GetDetailsOf(details, pidl, lpdi->item.iSubItem, &sd);
	          IShellDetails_Release(details);
	        }
	        else
	          WARN("IShellFolder2/IShellDetails not supported\n");
	      }

	      if (hr != S_OK)
	      {
	          /* set to empty on failure */
	          sd.str.uType = STRRET_WSTR;
	          sd.str.pOleStr = emptyW;
	      }

              if (lpnmh->code == LVN_GETDISPINFOW)
              {
                  StrRetToStrNW( lpdi->item.pszText, lpdi->item.cchTextMax, &sd.str, NULL);
                  TRACE("-- text=%s\n", debugstr_w(lpdi->item.pszText));
              }
              else
              {
                  /* LVN_GETDISPINFOA - shouldn't happen */
                  NMLVDISPINFOA *lpdiA = (NMLVDISPINFOA *)lpnmh;
                  StrRetToStrNA( lpdiA->item.pszText, lpdiA->item.cchTextMax, &sd.str, NULL);
                  TRACE("-- text=%s\n", lpdiA->item.pszText);
              }
	    }

	    if(lpdi->item.mask & LVIF_IMAGE)	/* image requested */
	    {
	      lpdi->item.iImage = SHMapPIDLToSystemImageListIndex(This->pSFParent, pidl, 0);
	    }
	    break;

	  case LVN_ITEMCHANGED:
	    TRACE("-- LVN_ITEMCHANGED %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  case LVN_BEGINDRAG:
	  case LVN_BEGINRDRAG:
	    TRACE("-- LVN_BEGINDRAG\n");

	    if (ShellView_GetSelections(This))
	    {
	      IDataObject * pda;
	      DWORD dwAttributes = SFGAO_CANLINK;
	      DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;

	      if (SUCCEEDED(IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, (LPCITEMIDLIST*)This->apidl, &IID_IDataObject,0,(LPVOID *)&pda)))
	      {
                  IDropSource *pds = &This->IDropSource_iface; /* own DropSource interface */

	  	  if (SUCCEEDED(IShellFolder_GetAttributesOf(This->pSFParent, This->cidl, (LPCITEMIDLIST*)This->apidl, &dwAttributes)))
		  {
		    if (dwAttributes & SFGAO_CANLINK)
		    {
		      dwEffect |= DROPEFFECT_LINK;
		    }
		  }

	          if (pds)
	          {
	            DWORD dwEffect2;
		    DoDragDrop(pda, pds, dwEffect, &dwEffect2);
		  }
	          IDataObject_Release(pda);
	      }
	    }
	    break;

	  case LVN_BEGINLABELEDITW:
	    {
	      DWORD dwAttr = SFGAO_CANRENAME;
	      pidl = (LPITEMIDLIST)lpdi->item.lParam;

	      TRACE("-- LVN_BEGINLABELEDITW %p\n",This);

	      IShellFolder_GetAttributesOf(This->pSFParent, 1, (LPCITEMIDLIST*)&pidl, &dwAttr);
	      if (SFGAO_CANRENAME & dwAttr)
	      {
	        return FALSE;
	      }
	      return TRUE;
	    }

	  case LVN_ENDLABELEDITW:
	    {
	      TRACE("-- LVN_ENDLABELEDITA %p\n",This);
	      if (lpdi->item.pszText)
	      {
	        HRESULT hr;
		LVITEMW lvItem;

		lvItem.iItem = lpdi->item.iItem;
		lvItem.iSubItem = 0;
		lvItem.mask = LVIF_PARAM;
		SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

		pidl = (LPITEMIDLIST)lpdi->item.lParam;
	        hr = IShellFolder_SetNameOf(This->pSFParent, 0, pidl, lpdi->item.pszText, SHGDN_INFOLDER, &pidl);

		if(SUCCEEDED(hr) && pidl)
		{
	          lvItem.mask = LVIF_PARAM;
		  lvItem.lParam = (LPARAM)pidl;
		  SendMessageW(This->hWndList, LVM_SETITEMW, 0, (LPARAM) &lvItem);
		  return TRUE;
		}
	      }
	      return FALSE;
	    }

	  case LVN_KEYDOWN:
	    {
	      LPNMLVKEYDOWN plvKeyDown = (LPNMLVKEYDOWN) lpnmh;

              /* initiate a rename of the selected file or directory */
              switch (plvKeyDown->wVKey)
              {
              case VK_F2:
                {
                  INT i = SendMessageW(This->hWndList, LVM_GETSELECTEDCOUNT, 0, 0);

                  if (i == 1)
                  {
                    /* get selected item */
                    i = SendMessageW(This->hWndList, LVM_GETNEXTITEM, -1, MAKELPARAM (LVNI_SELECTED, 0));

                    SendMessageW(This->hWndList, LVM_ENSUREVISIBLE, i, 0);
                    SendMessageW(This->hWndList, LVM_EDITLABELW, i, 0);
                  }
                }
                break;
              case VK_DELETE:
                {
		  UINT i, count;
		  int item_index;
		  LVITEMW item;
		  LPITEMIDLIST* pItems;
		  ISFHelper *psfhlp;
		  HRESULT hr;

		  hr = IShellFolder_QueryInterface(This->pSFParent, &IID_ISFHelper, (void**)&psfhlp);
		  if (hr != S_OK) return 0;

		  if(!(count = SendMessageW(This->hWndList, LVM_GETSELECTEDCOUNT, 0, 0)))
		  {
		    ISFHelper_Release(psfhlp);
		    return 0;
		  }

		  /* allocate memory for the pidl array */
		  pItems = malloc(sizeof(ITEMIDLIST*) * count);

		  /* retrieve all selected items */
		  i = 0;
		  item_index = -1;

		  while (count > i)
		  {
		    /* get selected item */
		    item_index = SendMessageW(This->hWndList, LVM_GETNEXTITEM, item_index,
                                              MAKELPARAM (LVNI_SELECTED, 0));
		    item.iItem = item_index;
		    item.mask = LVIF_PARAM;
		    SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM)&item);

		    /* get item pidl */
		    pItems[i] = (LPITEMIDLIST)item.lParam;

		    i++;
		  }

		  /* perform the item deletion */
		  ISFHelper_DeleteItems(psfhlp, i, (LPCITEMIDLIST*)pItems);
		  ISFHelper_Release(psfhlp);

		  /* free pidl array memory */
		  free(pItems);
                }
		break;

	      case VK_F5:
                /* Initiate a refresh */
		IShellView3_Refresh(&This->IShellView3_iface);
		break;

	      case VK_BACK:
		{
		  LPSHELLBROWSER lpSb;
		  if((lpSb = (LPSHELLBROWSER)SendMessageW(This->hWndParent, CWM_GETISHELLBROWSER, 0, 0)))
		  {
		    IShellBrowser_BrowseObject(lpSb, NULL, SBSP_PARENT);
		  }
	        }
		break;

	      default:
		FIXME("LVN_KEYDOWN key=0x%08x\n", plvKeyDown->wVKey);
	      }
	    }
	    break;

	  default:
	    TRACE("-- %p WM_COMMAND %x unhandled\n", This, lpnmh->code);
	    break;
	}
	return 0;
}

/**********************************************************
* ShellView_OnChange()
*/

static LRESULT ShellView_OnChange(IShellViewImpl * This, const LPCITEMIDLIST *pidls, LONG event)
{
    LPCITEMIDLIST pidl;
    BOOL ret = TRUE;

    TRACE("(%p)->(%p, %p, 0x%08lx)\n", This, pidls[0], pidls[1], event);

    switch (event)
    {
        case SHCNE_MKDIR:
        case SHCNE_CREATE:
            pidl = ILClone(ILFindLastID(pidls[0]));
            shellview_add_item(This, pidl);
            break;
        case SHCNE_RMDIR:
        case SHCNE_DELETE:
        {
            INT i = LV_FindItemByPidl(This, ILFindLastID(pidls[0]));
            ret = SendMessageW(This->hWndList, LVM_DELETEITEM, i, 0);
            break;
        }
        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
            LV_RenameItem(This, pidls[0], pidls[1]);
            break;
        case SHCNE_UPDATEITEM:
	    break;
    }
    return ret;
}
/**********************************************************
*  ShellView_WndProc
*/

static LRESULT CALLBACK ShellView_WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	IShellViewImpl * pThis = (IShellViewImpl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	LPCREATESTRUCTW lpcs;

	TRACE("(hwnd=%p msg=%x wparm=%Ix lparm=%Ix)\n",hWnd, uMessage, wParam, lParam);

	switch (uMessage)
	{
	  case WM_NCCREATE:
	    lpcs = (LPCREATESTRUCTW)lParam;
            pThis = lpcs->lpCreateParams;
	    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (ULONG_PTR)pThis);
	    pThis->hWnd = hWnd;        /*set the window handle*/
	    break;

	  case WM_SIZE:		return ShellView_OnSize(pThis,LOWORD(lParam), HIWORD(lParam));
	  case WM_SETFOCUS:	return ShellView_OnSetFocus(pThis);
	  case WM_KILLFOCUS:	return ShellView_OnKillFocus(pThis);
	  case WM_CREATE:	return ShellView_OnCreate(pThis);
	  case WM_ACTIVATE:	return ShellView_OnActivate(pThis, SVUIA_ACTIVATE_FOCUS);
	  case WM_NOTIFY:	return ShellView_OnNotify(pThis,(UINT)wParam, (LPNMHDR)lParam);
	  case WM_COMMAND:      return ShellView_OnCommand(pThis,
					GET_WM_COMMAND_ID(wParam, lParam),
					GET_WM_COMMAND_CMD(wParam, lParam),
					GET_WM_COMMAND_HWND(wParam, lParam));
	  case SHV_CHANGE_NOTIFY: return ShellView_OnChange(pThis, (const LPCITEMIDLIST*)wParam, (LONG)lParam);

	  case WM_CONTEXTMENU:  ShellView_DoContextMenu(pThis, LOWORD(lParam), HIWORD(lParam), FALSE);
	                        return 0;

	  case WM_SHOWWINDOW:	UpdateWindow(pThis->hWndList);
				break;

	  case WM_GETDLGCODE:   return SendMessageW(pThis->hWndList, uMessage, 0, 0);
          case WM_SETFONT:      return SendMessageW(pThis->hWndList, WM_SETFONT, wParam, lParam);
          case WM_GETFONT:      return SendMessageW(pThis->hWndList, WM_GETFONT, wParam, lParam);

	  case WM_DESTROY:	
	  			RevokeDragDrop(pThis->hWnd);
				SHChangeNotifyDeregister(pThis->hNotify);
	                        break;

	  case WM_ERASEBKGND:
	    if ((pThis->FolderSettings.fFlags & FWF_DESKTOP) ||
	        (pThis->FolderSettings.fFlags & FWF_TRANSPARENT))
	      return 1;
	    break;
	}

	return DefWindowProcW(hWnd, uMessage, wParam, lParam);
}
/**********************************************************
*
*
*  The INTERFACE of the IShellView object
*
*
**********************************************************
*  IShellView_QueryInterface
*/
static HRESULT WINAPI IShellView_fnQueryInterface(IShellView3 *iface, REFIID riid, void **ppvObj)
{
	IShellViewImpl *This = impl_from_IShellView3(iface);

	TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) ||
	   IsEqualIID(riid, &IID_IShellView) ||
	   IsEqualIID(riid, &IID_IShellView2) ||
	   IsEqualIID(riid, &IID_IShellView3) ||
	   IsEqualIID(riid, &IID_CDefView))
	{
	  *ppvObj = &This->IShellView3_iface;
	}
	else if(IsEqualIID(riid, &IID_IShellFolderView))
	{
          *ppvObj = &This->IShellFolderView_iface;
	}
	else if(IsEqualIID(riid, &IID_IFolderView) ||
		IsEqualIID(riid, &IID_IFolderView2))
	{
          *ppvObj = &This->IFolderView2_iface;
	}
	else if(IsEqualIID(riid, &IID_IOleCommandTarget))
	{
          *ppvObj = &This->IOleCommandTarget_iface;
	}
	else if(IsEqualIID(riid, &IID_IDropTarget))
	{
          *ppvObj = &This->IDropTarget_iface;
	}
	else if(IsEqualIID(riid, &IID_IDropSource))
	{
          *ppvObj = &This->IDropSource_iface;
	}
	else if(IsEqualIID(riid, &IID_IViewObject))
	{
          *ppvObj = &This->IViewObject_iface;
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef( (IUnknown*)*ppvObj );
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**********************************************************
*  IShellView_AddRef
*/
static ULONG WINAPI IShellView_fnAddRef(IShellView3 *iface)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}
/**********************************************************
*  IShellView_Release
*/
static ULONG WINAPI IShellView_fnRelease(IShellView3 *iface)
{
	IShellViewImpl *This = impl_from_IShellView3(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%li)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IShellView(%p)\n",This);

	  DestroyWindow(This->hWndList);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  if(This->pSF2Parent)
	    IShellFolder2_Release(This->pSF2Parent);

	  SHFree(This->apidl);

	  if(This->pAdvSink)
	    IAdviseSink_Release(This->pAdvSink);

	  free(This);
	}
	return refCount;
}

/**********************************************************
*  ShellView_GetWindow
*/
static HRESULT WINAPI IShellView_fnGetWindow(IShellView3 *iface, HWND *phWnd)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);

    TRACE("(%p)\n", This);

    *phWnd = This->hWnd;
    return S_OK;
}

static HRESULT WINAPI IShellView_fnContextSensitiveHelp(IShellView3 *iface, BOOL mode)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    TRACE("(%p)->(%d)\n", This, mode);
    return E_NOTIMPL;
}

/**********************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
static HRESULT WINAPI IShellView_fnTranslateAccelerator(IShellView3 *iface, MSG *lpmsg)
{
#if 0
	IShellViewImpl *This = (IShellViewImpl *)iface;

	FIXME("(%p)->(%p: hwnd=%x msg=%x lp=%x wp=%x) stub\n",This,lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);
#endif

	if ((lpmsg->message>=WM_KEYFIRST) && (lpmsg->message<=WM_KEYLAST))
	{
	  TRACE("-- key=0x04%Ix\n",lpmsg->wParam) ;
	}
	return S_FALSE; /* not handled */
}

static HRESULT WINAPI IShellView_fnEnableModeless(IShellView3 *iface, BOOL fEnable)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);

    FIXME("(%p) stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnUIActivate(IShellView3 *iface, UINT uState)
{
	IShellViewImpl *This = impl_from_IShellView3(iface);

/*
	CHAR	szName[MAX_PATH];
*/
	LRESULT	lResult;
	int	nPartArray[1] = {-1};

	TRACE("%p, %d.\n", This, uState);

	/*don't do anything if the state isn't really changing*/
	if(This->uState == uState)
	{
	  return S_OK;
	}

	/*OnActivate handles the menu merging and internal state*/
	ShellView_OnActivate(This, uState);

	/*only do This if we are active*/
	if(uState != SVUIA_DEACTIVATE)
	{

/*
	  GetFolderPath is not a method of IShellFolder
	  IShellFolder_GetFolderPath( This->pSFParent, szName, sizeof(szName) );
*/
	  /* set the number of parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETPARTS, 1,
	  						(LPARAM)nPartArray, &lResult);

	  /* set the text for the parts */
/*
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETTEXTA,
							0, (LPARAM)szName, &lResult);
*/
	}

	return S_OK;
}

static HRESULT WINAPI IShellView_fnRefresh(IShellView3 *iface)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);

    TRACE("(%p)\n", This);

    SendMessageW(This->hWndList, LVM_DELETEALLITEMS, 0, 0);
    ShellView_FillList(This);

    return S_OK;
}

static HRESULT WINAPI IShellView_fnCreateViewWindow(IShellView3 *iface, IShellView *prev_view,
        const FOLDERSETTINGS *settings, IShellBrowser *owner, RECT *rect, HWND *hWnd)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    TRACE("(%p)->(%p %p %p %p %p)\n", This, prev_view, settings, owner, rect, hWnd);
    return IShellView3_CreateViewWindow3(iface, owner, prev_view, SV3CVW3_DEFAULT,
        settings->fFlags, settings->fFlags, settings->ViewMode, NULL, rect, hWnd);
}

static HRESULT WINAPI IShellView_fnDestroyViewWindow(IShellView3 *iface)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);

    TRACE("(%p)\n", This);

    if (!This->hWnd)
        return S_OK;

    /* Make absolutely sure all our UI is cleaned up. */
    IShellView3_UIActivate(iface, SVUIA_DEACTIVATE);

    if (This->hMenu)
        DestroyMenu(This->hMenu);

    DestroyWindow(This->hWnd);
    if (This->pShellBrowser) IShellBrowser_Release(This->pShellBrowser);
    if (This->pCommDlgBrowser) ICommDlgBrowser_Release(This->pCommDlgBrowser);

    This->hMenu = NULL;
    This->hWnd = NULL;
    This->pShellBrowser = NULL;
    This->pCommDlgBrowser = NULL;

    return S_OK;
}

static HRESULT WINAPI IShellView_fnGetCurrentInfo(IShellView3 *iface, LPFOLDERSETTINGS lpfs)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);

    TRACE("(%p)->(%p) vmode=%x flags=%x\n", This, lpfs,
        This->FolderSettings.ViewMode, This->FolderSettings.fFlags);

    if (!lpfs) return E_INVALIDARG;
    *lpfs = This->FolderSettings;
    return S_OK;
}

static HRESULT WINAPI IShellView_fnAddPropertySheetPages(IShellView3 *iface, DWORD dwReserved,
		LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnSaveViewState(IShellView3 *iface)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    FIXME("(%p) stub\n", This);
    return S_OK;
}

static HRESULT WINAPI IShellView_fnSelectItem(IShellView3 *iface, LPCITEMIDLIST pidl, UINT flags)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    int i;

    TRACE("(%p)->(pidl=%p, 0x%08x)\n",This, pidl, flags);

    i = LV_FindItemByPidl(This, pidl);
    if (i == -1) return S_OK;

    return IFolderView2_SelectItem(&This->IFolderView2_iface, i, flags);
}

static HRESULT WINAPI IShellView_fnGetItemObject(IShellView3 *iface, UINT uItem, REFIID riid,
        void **ppvOut)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    HRESULT hr = E_NOINTERFACE;

    TRACE("(%p)->(0x%08x, %s, %p)\n",This, uItem, debugstr_guid(riid), ppvOut);

    *ppvOut = NULL;

    switch(uItem)
    {
    case SVGIO_BACKGROUND:

        if (IsEqualIID(&IID_IContextMenu, riid))
            return BackgroundMenu_Constructor(This->pSFParent, FALSE, riid, ppvOut);
        else if (IsEqualIID(&IID_IDispatch, riid)) {
            *ppvOut = &This->IShellFolderViewDual3_iface;
            IShellFolderViewDual3_AddRef(&This->IShellFolderViewDual3_iface);
            return S_OK;
        }
        else
            FIXME("unsupported interface requested %s\n", debugstr_guid(riid));

        break;

    case SVGIO_SELECTION:
	ShellView_GetSelections(This);
	hr = IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, (LPCITEMIDLIST*)This->apidl, riid, 0, ppvOut);
	break;

    default:
        FIXME("unimplemented for uItem = 0x%08x\n", uItem);
    }
    TRACE("-- (%p)->(interface=%p)\n",This, *ppvOut);

    return hr;
}

static HRESULT WINAPI IShellView2_fnGetView(IShellView3 *iface, SHELLVIEWID *view_guid, ULONG view_type)
{
    FIXME("(%p)->(%s, %#lx) stub!\n", iface, debugstr_guid(view_guid), view_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellView2_fnCreateViewWindow2(IShellView3 *iface, SV2CVW2_PARAMS *view_params)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    TRACE("(%p)->(%p)\n", This, view_params);
    return IShellView3_CreateViewWindow3(iface, view_params->psbOwner, view_params->psvPrev,
        SV3CVW3_DEFAULT, view_params->pfs->fFlags, view_params->pfs->fFlags,
        view_params->pfs->ViewMode, view_params->pvid, view_params->prcView, &view_params->hwndView);
}

static HRESULT WINAPI IShellView2_fnHandleRename(IShellView3 *iface, LPCITEMIDLIST new_pidl)
{
    FIXME("(%p)->(new_pidl %p) stub!\n", iface, new_pidl);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellView2_fnSelectAndPositionItem(IShellView3 *iface, LPCITEMIDLIST item,
        UINT flags, POINT *point)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    TRACE("(%p)->(item %p, flags %#x, point %p)\n", This, item, flags, point);
    return IFolderView2_SelectAndPositionItems(&This->IFolderView2_iface, 1, &item, point, flags);
}

static HRESULT WINAPI IShellView3_fnCreateViewWindow3(IShellView3 *iface, IShellBrowser *owner,
    IShellView *prev_view, SV3CVW3_FLAGS view_flags, FOLDERFLAGS mask, FOLDERFLAGS flags,
    FOLDERVIEWMODE mode, const SHELLVIEWID *view_id, const RECT *rect, HWND *hwnd)
{
    IShellViewImpl *This = impl_from_IShellView3(iface);
    INITCOMMONCONTROLSEX icex;
    WNDCLASSW wc;
    HRESULT hr;
    HWND wnd;

    TRACE("(%p)->(%p %p 0x%08lx 0x%08x 0x%08x %d %s %s %p)\n", This, owner, prev_view, view_flags,
        mask, flags, mode, debugstr_guid(view_id), wine_dbgstr_rect(rect), hwnd);

    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    *hwnd = NULL;

    if (!owner || This->hWnd)
        return E_UNEXPECTED;

    if (view_flags != SV3CVW3_DEFAULT)
        FIXME("unsupported view flags 0x%08lx\n", view_flags);

    /* Set up the member variables */
    This->pShellBrowser = owner;
    This->FolderSettings.ViewMode = mode;
    This->FolderSettings.fFlags = mask & flags;

    if (view_id)
    {
        if (IsEqualGUID(view_id, &VID_LargeIcons))
            This->FolderSettings.ViewMode = FVM_ICON;
        else if (IsEqualGUID(view_id, &VID_SmallIcons))
            This->FolderSettings.ViewMode = FVM_SMALLICON;
        else if (IsEqualGUID(view_id, &VID_List))
            This->FolderSettings.ViewMode = FVM_LIST;
        else if (IsEqualGUID(view_id, &VID_Details))
            This->FolderSettings.ViewMode = FVM_DETAILS;
        else if (IsEqualGUID(view_id, &VID_Thumbnails))
            This->FolderSettings.ViewMode = FVM_THUMBNAIL;
        else if (IsEqualGUID(view_id, &VID_Tile))
            This->FolderSettings.ViewMode = FVM_TILE;
        else if (IsEqualGUID(view_id, &VID_ThumbStrip))
            This->FolderSettings.ViewMode = FVM_THUMBSTRIP;
        else
            FIXME("Ignoring unrecognized VID %s\n", debugstr_guid(view_id));
    }

    /* Get our parent window */
    IShellBrowser_AddRef(This->pShellBrowser);
    IShellBrowser_GetWindow(This->pShellBrowser, &This->hWndParent);

    /* Try to get the ICommDlgBrowserInterface, adds a reference !!! */
    This->pCommDlgBrowser = NULL;
    hr = IShellBrowser_QueryInterface(This->pShellBrowser, &IID_ICommDlgBrowser, (void **)&This->pCommDlgBrowser);
    if (hr == S_OK)
        TRACE("-- CommDlgBrowser %p\n", This->pCommDlgBrowser);

    /* If our window class has not been registered, then do so */
    if (!GetClassInfoW(shell32_hInstance, L"SHELLDLL_DefView", &wc))
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = ShellView_WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = shell32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = L"SHELLDLL_DefView";

        if (!RegisterClassW(&wc)) return E_FAIL;
    }

    wnd = CreateWindowExW(0, L"SHELLDLL_DefView", NULL, WS_CHILD | WS_TABSTOP,
            rect->left, rect->top,
            rect->right - rect->left,
            rect->bottom - rect->top,
            This->hWndParent, 0, shell32_hInstance, This);

    CheckToolbar(This);

    if (!wnd)
    {
        IShellBrowser_Release(This->pShellBrowser);
        return E_FAIL;
    }

    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow(wnd);

    *hwnd = wnd;

    return S_OK;
}

static const IShellView3Vtbl shellviewvtbl =
{
	IShellView_fnQueryInterface,
	IShellView_fnAddRef,
	IShellView_fnRelease,
	IShellView_fnGetWindow,
	IShellView_fnContextSensitiveHelp,
	IShellView_fnTranslateAccelerator,
	IShellView_fnEnableModeless,
	IShellView_fnUIActivate,
	IShellView_fnRefresh,
	IShellView_fnCreateViewWindow,
	IShellView_fnDestroyViewWindow,
	IShellView_fnGetCurrentInfo,
	IShellView_fnAddPropertySheetPages,
	IShellView_fnSaveViewState,
	IShellView_fnSelectItem,
	IShellView_fnGetItemObject,
	IShellView2_fnGetView,
	IShellView2_fnCreateViewWindow2,
	IShellView2_fnHandleRename,
	IShellView2_fnSelectAndPositionItem,
	IShellView3_fnCreateViewWindow3
};


/**********************************************************
 * ISVOleCmdTarget_QueryInterface (IUnknown)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryInterface(IOleCommandTarget *iface, REFIID iid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IOleCommandTarget(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, iid, ppvObj);
}

/**********************************************************
 * ISVOleCmdTarget_AddRef (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_AddRef(IOleCommandTarget *iface)
{
    IShellViewImpl *This = impl_from_IOleCommandTarget(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

/**********************************************************
 * ISVOleCmdTarget_Release (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_Release(IOleCommandTarget *iface)
{
    IShellViewImpl *This = impl_from_IOleCommandTarget(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

/**********************************************************
 * ISVOleCmdTarget_QueryStatus (IOleCommandTarget)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryStatus(
	IOleCommandTarget *iface,
	const GUID *pguidCmdGroup,
	ULONG cCmds,
	OLECMD *prgCmds,
	OLECMDTEXT *pCmdText)
{
    IShellViewImpl *This = impl_from_IOleCommandTarget(iface);
    UINT i;

    FIXME("(%p)->(%s %ld %p %p)\n",
              This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if (!prgCmds)
        return E_INVALIDARG;
    for (i = 0; i < cCmds; i++)
    {
        FIXME("\tprgCmds[%d].cmdID = %ld\n", i, prgCmds[i].cmdID);
        prgCmds[i].cmdf = 0;
    }
    return OLECMDERR_E_UNKNOWNGROUP;
}

/**********************************************************
 * ISVOleCmdTarget_Exec (IOleCommandTarget)
 *
 * nCmdID is the OLECMDID_* enumeration
 */
static HRESULT WINAPI ISVOleCmdTarget_Exec(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	DWORD nCmdID,
	DWORD nCmdexecopt,
	VARIANT* pvaIn,
	VARIANT* pvaOut)
{
	IShellViewImpl *This = impl_from_IOleCommandTarget(iface);

	FIXME("(%p)->(%s %ld 0x%08lx %s %p)\n",
              This, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, debugstr_variant(pvaIn), pvaOut);

	if (!pguidCmdGroup)
	    return OLECMDERR_E_UNKNOWNGROUP;

	if (IsEqualIID(pguidCmdGroup, &CGID_Explorer) &&
	   (nCmdID == OLECMDID_SHOWMESSAGE) &&
	   (nCmdexecopt == 4) && pvaOut)
	   return S_OK;
	if (IsEqualIID(pguidCmdGroup, &CGID_ShellDocView) &&
	   (nCmdID == OLECMDID_SPELL) &&
	   (nCmdexecopt == OLECMDEXECOPT_DODEFAULT))
	   return S_FALSE;

	return OLECMDERR_E_UNKNOWNGROUP;
}

static const IOleCommandTargetVtbl olecommandtargetvtbl =
{
	ISVOleCmdTarget_QueryInterface,
	ISVOleCmdTarget_AddRef,
	ISVOleCmdTarget_Release,
	ISVOleCmdTarget_QueryStatus,
	ISVOleCmdTarget_Exec
};

/**********************************************************
 * ISVDropTarget implementation
 */

static HRESULT WINAPI ISVDropTarget_QueryInterface(IDropTarget *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, riid, ppvObj);
}

static ULONG WINAPI ISVDropTarget_AddRef(IDropTarget *iface)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI ISVDropTarget_Release(IDropTarget *iface)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

/******************************************************************************
 * drag_notify_subitem [Internal]
 *
 * Figure out the shellfolder object, which is currently under the mouse cursor
 * and notify it via the IDropTarget interface.
 */

#define SCROLLAREAWIDTH 20

static HRESULT drag_notify_subitem(IShellViewImpl *This, DWORD grfKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    LVHITTESTINFO htinfo;
    LVITEMW lvItem;
    LONG lResult;
    HRESULT hr;
    RECT clientRect;

    /* Map from global to client coordinates and query the index of the listview-item, which is 
     * currently under the mouse cursor. */
    htinfo.pt.x = pt.x;
    htinfo.pt.y = pt.y;
    htinfo.flags = LVHT_ONITEM;
    ScreenToClient(This->hWndList, &htinfo.pt);
    lResult = SendMessageW(This->hWndList, LVM_HITTEST, 0, (LPARAM)&htinfo);

    /* Send WM_*SCROLL messages every 250 ms during drag-scrolling */
    GetClientRect(This->hWndList, &clientRect);
    if (htinfo.pt.x == This->ptLastMousePos.x && htinfo.pt.y == This->ptLastMousePos.y &&
        (htinfo.pt.x < SCROLLAREAWIDTH || htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH ||
         htinfo.pt.y < SCROLLAREAWIDTH || htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH ))
    {
        This->cScrollDelay = (This->cScrollDelay + 1) % 5; /* DragOver is called every 50 ms */
        if (This->cScrollDelay == 0) { /* Mouse did hover another 250 ms over the scroll-area */
            if (htinfo.pt.x < SCROLLAREAWIDTH) 
                SendMessageW(This->hWndList, WM_HSCROLL, SB_LINEUP, 0);
            if (htinfo.pt.x > clientRect.right - SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_HSCROLL, SB_LINEDOWN, 0);
            if (htinfo.pt.y < SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_VSCROLL, SB_LINEUP, 0);
            if (htinfo.pt.y > clientRect.bottom - SCROLLAREAWIDTH)
                SendMessageW(This->hWndList, WM_VSCROLL, SB_LINEDOWN, 0);
        }
    } else {
        This->cScrollDelay = 0; /* Reset, if the cursor is not over the listview's scroll-area */
    }
    This->ptLastMousePos = htinfo.pt;
 
    /* If we are still over the previous sub-item, notify it via DragOver and return. */
    if (This->pCurDropTarget && lResult == This->iDragOverItem)
    return IDropTarget_DragOver(This->pCurDropTarget, grfKeyState, pt, pdwEffect);
  
    /* We've left the previous sub-item, notify it via DragLeave and Release it. */
    if (This->pCurDropTarget) {
        IDropTarget_DragLeave(This->pCurDropTarget);
        IDropTarget_Release(This->pCurDropTarget);
        This->pCurDropTarget = NULL;
    }

    This->iDragOverItem = lResult;
    if (lResult == -1) {
        /* We are not above one of the listview's subitems. Bind to the parent folder's
         * DropTarget interface. */
        hr = IShellFolder_QueryInterface(This->pSFParent, &IID_IDropTarget, 
                                         (LPVOID*)&This->pCurDropTarget);
    } else {
        /* Query the relative PIDL of the shellfolder object represented by the currently
         * dragged over listview-item ... */
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = lResult;
        lvItem.iSubItem = 0;
        SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM) &lvItem);

        /* ... and bind pCurDropTarget to the IDropTarget interface of an UIObject of this object */
        hr = IShellFolder_GetUIObjectOf(This->pSFParent, This->hWndList, 1,
            (LPCITEMIDLIST*)&lvItem.lParam, &IID_IDropTarget, NULL, (LPVOID*)&This->pCurDropTarget);
    }

    /* If anything failed, pCurDropTarget should be NULL now, which ought to be a save state. */
    if (FAILED(hr)) 
        return hr;

    /* Notify the item just entered via DragEnter. */
    return IDropTarget_DragEnter(This->pCurDropTarget, This->pCurDataObject, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragEnter(IDropTarget *iface, IDataObject *pDataObject,
    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    /* Get a hold on the data object for later calls to DragEnter on the sub-folders */
    This->pCurDataObject = pDataObject;
    IDataObject_AddRef(pDataObject);

    return drag_notify_subitem(This, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragOver(IDropTarget *iface, DWORD grfKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);
    return drag_notify_subitem(This, grfKeyState, pt, pdwEffect);
}

static HRESULT WINAPI ISVDropTarget_DragLeave(IDropTarget *iface)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    if (This->pCurDropTarget)
    {
        IDropTarget_DragLeave(This->pCurDropTarget);
        IDropTarget_Release(This->pCurDropTarget);
        This->pCurDropTarget = NULL;
    }

    if (This->pCurDataObject)
    {
        IDataObject_Release(This->pCurDataObject);
        This->pCurDataObject = NULL;
    }

    This->iDragOverItem = 0;

    return S_OK;
}

static HRESULT WINAPI ISVDropTarget_Drop(IDropTarget *iface, IDataObject* pDataObject, 
    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IShellViewImpl *This = impl_from_IDropTarget(iface);

    if (!This->pCurDropTarget) return DRAGDROP_E_INVALIDHWND;

    IDropTarget_Drop(This->pCurDropTarget, pDataObject, grfKeyState, pt, pdwEffect);

    IDropTarget_Release(This->pCurDropTarget);
    IDataObject_Release(This->pCurDataObject);
    This->pCurDataObject = NULL;
    This->pCurDropTarget = NULL;
    This->iDragOverItem = 0;

    return S_OK;
}

static const IDropTargetVtbl droptargetvtbl =
{
	ISVDropTarget_QueryInterface,
	ISVDropTarget_AddRef,
	ISVDropTarget_Release,
	ISVDropTarget_DragEnter,
	ISVDropTarget_DragOver,
	ISVDropTarget_DragLeave,
	ISVDropTarget_Drop
};

/**********************************************************
 * ISVDropSource implementation
 */

static HRESULT WINAPI ISVDropSource_QueryInterface(IDropSource *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IDropSource(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, riid, ppvObj);
}

static ULONG WINAPI ISVDropSource_AddRef(IDropSource *iface)
{
    IShellViewImpl *This = impl_from_IDropSource(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI ISVDropSource_Release(IDropSource *iface)
{
    IShellViewImpl *This = impl_from_IDropSource(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

static HRESULT WINAPI ISVDropSource_QueryContinueDrag(
	IDropSource *iface,
	BOOL fEscapePressed,
	DWORD grfKeyState)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);
	TRACE("(%p)\n",This);

	if (fEscapePressed)
	  return DRAGDROP_S_CANCEL;
	else if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON))
	  return DRAGDROP_S_DROP;
	else
	  return S_OK;
}

static HRESULT WINAPI ISVDropSource_GiveFeedback(
	IDropSource *iface,
	DWORD dwEffect)
{
	IShellViewImpl *This = impl_from_IDropSource(iface);
	TRACE("(%p)\n",This);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static const IDropSourceVtbl dropsourcevtbl =
{
	ISVDropSource_QueryInterface,
	ISVDropSource_AddRef,
	ISVDropSource_Release,
	ISVDropSource_QueryContinueDrag,
	ISVDropSource_GiveFeedback
};
/**********************************************************
 * ISVViewObject implementation
 */

static HRESULT WINAPI ISVViewObject_QueryInterface(IViewObject *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IViewObject(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, riid, ppvObj);
}

static ULONG WINAPI ISVViewObject_AddRef(IViewObject *iface)
{
    IShellViewImpl *This = impl_from_IViewObject(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI ISVViewObject_Release(IViewObject *iface)
{
    IShellViewImpl *This = impl_from_IViewObject(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

static HRESULT WINAPI ISVViewObject_Draw(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void* pvAspect,
	DVTARGETDEVICE* ptd,
	HDC hdcTargetDev,
	HDC hdcDraw,
	LPCRECTL lprcBounds,
	LPCRECTL lprcWBounds,
	BOOL (CALLBACK *pfnContinue)(ULONG_PTR dwContinue),
	ULONG_PTR dwContinue)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_GetColorSet(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void *pvAspect,
	DVTARGETDEVICE* ptd,
	HDC hicTargetDevice,
	LOGPALETTE** ppColorSet)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Freeze(
	IViewObject 	*iface,
	DWORD dwDrawAspect,
	LONG lindex,
	void* pvAspect,
	DWORD* pdwFreeze)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Unfreeze(
	IViewObject 	*iface,
	DWORD dwFreeze)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_SetAdvise(
	IViewObject 	*iface,
	DWORD aspects,
	DWORD advf,
	IAdviseSink* pAdvSink)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	FIXME("partial stub: %p %08lx %08lx %p\n",
              This, aspects, advf, pAdvSink);

	/* FIXME: we set the AdviseSink, but never use it to send any advice */
	This->pAdvSink = pAdvSink;
	This->dwAspects = aspects;
	This->dwAdvf = advf;

	return S_OK;
}

static HRESULT WINAPI ISVViewObject_GetAdvise(
	IViewObject 	*iface,
	DWORD* pAspects,
	DWORD* pAdvf,
	IAdviseSink** ppAdvSink)
{

	IShellViewImpl *This = impl_from_IViewObject(iface);

	TRACE("This=%p pAspects=%p pAdvf=%p ppAdvSink=%p\n",
              This, pAspects, pAdvf, ppAdvSink);

	if( ppAdvSink )
	{
		IAdviseSink_AddRef( This->pAdvSink );
		*ppAdvSink = This->pAdvSink;
	}
	if( pAspects )
		*pAspects = This->dwAspects;
	if( pAdvf )
		*pAdvf = This->dwAdvf;

	return S_OK;
}


static const IViewObjectVtbl viewobjectvtbl =
{
	ISVViewObject_QueryInterface,
	ISVViewObject_AddRef,
	ISVViewObject_Release,
	ISVViewObject_Draw,
	ISVViewObject_GetColorSet,
	ISVViewObject_Freeze,
	ISVViewObject_Unfreeze,
	ISVViewObject_SetAdvise,
	ISVViewObject_GetAdvise
};

/* IFolderView2 */
static HRESULT WINAPI FolderView_QueryInterface(IFolderView2 *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, riid, ppvObj);
}

static ULONG WINAPI FolderView_AddRef(IFolderView2 *iface)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI FolderView_Release(IFolderView2 *iface)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

static HRESULT WINAPI FolderView_GetCurrentViewMode(IFolderView2 *iface, UINT *mode)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    TRACE("%p, %p.\n", This, mode);

    if(!mode)
        return E_INVALIDARG;

    *mode = This->FolderSettings.ViewMode;
    return S_OK;
}

static HRESULT WINAPI FolderView_SetCurrentViewMode(IFolderView2 *iface, UINT mode)
{
    IShellViewImpl *shellview = impl_from_IFolderView2(iface);
    DWORD style;

    TRACE("folder view %p, mode %u.\n", iface, mode);

    if (mode == FVM_AUTO)
        mode = FVM_ICON;

    if (mode >= FVM_FIRST && mode <= FVM_LAST)
    {
        style = ViewModeToListStyle(mode);
        SetStyle(shellview, style, LVS_TYPEMASK);
        shellview->FolderSettings.ViewMode = mode;
    }

    return S_OK;
}

static HRESULT WINAPI FolderView_GetFolder(IFolderView2 *iface, REFIID riid, void **ppv)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    return IShellFolder_QueryInterface(This->pSFParent, riid, ppv);
}

static HRESULT WINAPI FolderView_Item(IFolderView2 *iface, int index, PITEMID_CHILD *ppidl)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    LVITEMW item;

    TRACE("(%p)->(%d %p)\n", This, index, ppidl);

    item.mask = LVIF_PARAM;
    item.iItem = index;

    if (SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM)&item))
    {
        *ppidl = ILClone((PITEMID_CHILD)item.lParam);
        return S_OK;
    }
    else
    {
        *ppidl = 0;
        return E_INVALIDARG;
    }
}

static HRESULT WINAPI FolderView_ItemCount(IFolderView2 *iface, UINT flags, int *items)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    TRACE("(%p)->(%u %p)\n", This, flags, items);

    if (flags != SVGIO_ALLVIEW)
        FIXME("some flags unsupported, %x\n", flags & ~SVGIO_ALLVIEW);

    *items = SendMessageW(This->hWndList, LVM_GETITEMCOUNT, 0, 0);

    return S_OK;
}

static HRESULT WINAPI FolderView_Items(IFolderView2 *iface, UINT flags, REFIID riid, void **ppv)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    int count, i;
    ITEMIDLIST **pidl;
    LVITEMW item;
    HRESULT hr;

    if (!IsEqualIID(riid, &IID_IShellItemArray))
    {
        FIXME("%s is not supported\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    if (flags != SVGIO_ALLVIEW)
        FIXME("some flags unsupported, %x\n", flags & ~SVGIO_ALLVIEW);

    count = SendMessageW(This->hWndList, LVM_GETITEMCOUNT, 0, 0);
    if (!count)
    {
        FIXME("Folder is empty\n");
        return E_FAIL;
    }

    pidl = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*pidl));
    if (!pidl) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        if (!SendMessageW(This->hWndList, LVM_GETITEMW, 0, (LPARAM)&item))
        {
            FIXME("LVM_GETITEMW(%d) failed\n" ,i);
            HeapFree(GetProcessHeap(), 0, pidl);
            return E_FAIL;
        }

        pidl[i] = (ITEMIDLIST *)item.lParam;
    }

    hr = SHCreateShellItemArray(NULL, This->pSFParent, count, (LPCITEMIDLIST *)pidl, (IShellItemArray **)ppv);

    HeapFree(GetProcessHeap(), 0, pidl);

    return hr;
}

static HRESULT WINAPI FolderView_GetSelectionMarkedItem(IFolderView2 *iface, int *item)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    TRACE("(%p)->(%p)\n", This, item);

    *item = SendMessageW(This->hWndList, LVM_GETSELECTIONMARK, 0, 0);

    return S_OK;
}

static HRESULT WINAPI FolderView_GetFocusedItem(IFolderView2 *iface, int *item)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    TRACE("(%p)->(%p)\n", This, item);

    *item = SendMessageW(This->hWndList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

    return S_OK;
}

static HRESULT WINAPI FolderView_GetItemPosition(IFolderView2 *iface, PCUITEMID_CHILD pidl, POINT *ppt)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %p), stub\n", This, pidl, ppt);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView_GetSpacing(IFolderView2 *iface, POINT *pt)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    TRACE("(%p)->(%p)\n", This, pt);

    if (!This->hWndList) return S_FALSE;

    if (pt)
    {
        DWORD ret;
        ret = SendMessageW(This->hWndList, LVM_GETITEMSPACING, 0, 0);

        pt->x = LOWORD(ret);
        pt->y = HIWORD(ret);
    }

    return S_OK;
}

static HRESULT WINAPI FolderView_GetDefaultSpacing(IFolderView2 *iface, POINT *pt)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p), stub\n", This, pt);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView_GetAutoArrange(IFolderView2 *iface)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p), stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView_SelectItem(IFolderView2 *iface, int item, DWORD flags)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    LVITEMW lvItem;

    TRACE("(%p)->(%d, %lx)\n", This, item, flags);

    lvItem.state = 0;
    lvItem.stateMask = LVIS_SELECTED;

    if (flags & SVSI_ENSUREVISIBLE)
        SendMessageW(This->hWndList, LVM_ENSUREVISIBLE, item, 0);

    /* all items */
    if (flags & SVSI_DESELECTOTHERS)
        SendMessageW(This->hWndList, LVM_SETITEMSTATE, -1, (LPARAM)&lvItem);

    /* this item */
    if (flags & SVSI_SELECT)
        lvItem.state |= LVIS_SELECTED;

    if (flags & SVSI_FOCUSED)
    {
        lvItem.stateMask |= LVIS_FOCUSED;
        lvItem.state |= LVIS_FOCUSED;
    }

    SendMessageW(This->hWndList, LVM_SETITEMSTATE, item, (LPARAM)&lvItem);

    if ((flags & SVSI_EDIT) == SVSI_EDIT)
        SendMessageW(This->hWndList, LVM_EDITLABELW, item, 0);

    return S_OK;
}

static HRESULT WINAPI FolderView_SelectAndPositionItems(IFolderView2 *iface, UINT cidl,
                                     PCUITEMID_CHILD_ARRAY apidl, POINT *apt, DWORD flags)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%u %p %p %lx), stub\n", This, cidl, apidl, apt, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetGroupBy(IFolderView2 *iface, REFPROPERTYKEY key, BOOL ascending)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %d), stub\n", This, key, ascending);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetGroupBy(IFolderView2 *iface, PROPERTYKEY *pkey, BOOL *ascending)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %p), stub\n", This, pkey, ascending);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetViewProperty(IFolderView2 *iface, PCUITEMID_CHILD pidl,
    REFPROPERTYKEY propkey, REFPROPVARIANT propvar)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %p %p), stub\n", This, pidl, propkey, propvar);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetViewProperty(IFolderView2 *iface, PCUITEMID_CHILD pidl,
    REFPROPERTYKEY propkey, PROPVARIANT *propvar)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %p %p), stub\n", This, pidl, propkey, propvar);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetTileViewProperties(IFolderView2 *iface, PCUITEMID_CHILD pidl,
    LPCWSTR prop_list)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %s), stub\n", This, pidl, debugstr_w(prop_list));
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetExtendedTileViewProperties(IFolderView2 *iface, PCUITEMID_CHILD pidl,
    LPCWSTR prop_list)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %s), stub\n", This, pidl, debugstr_w(prop_list));
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetText(IFolderView2 *iface, FVTEXTTYPE type, LPCWSTR text)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %s), stub\n", This, type, debugstr_w(text));
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetCurrentFolderFlags(IFolderView2 *iface, DWORD mask, DWORD flags)
{
    IShellViewImpl *shellview = impl_from_IFolderView2(iface);

    TRACE("folder view %p, mask %#lx, flags %#lx.\n", iface, mask, flags);

    shellview->FolderSettings.fFlags = (shellview->FolderSettings.fFlags & ~mask) | (flags & mask);
    return S_OK;
}

static HRESULT WINAPI FolderView2_GetCurrentFolderFlags(IFolderView2 *iface, DWORD *flags)
{
    IShellViewImpl *shellview = impl_from_IFolderView2(iface);

    TRACE("folder view %p, flags %p.\n", iface, flags);

    if (flags)
        *flags = shellview->FolderSettings.fFlags;
    return S_OK;
}

static HRESULT WINAPI FolderView2_GetSortColumnCount(IFolderView2 *iface, int *columns)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p), stub\n", This, columns);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetSortColumns(IFolderView2 *iface, const SORTCOLUMN *columns,
    int count)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %d), stub\n", This, columns, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetSortColumns(IFolderView2 *iface, SORTCOLUMN *columns,
    int count)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %d), stub\n", This, columns, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetItem(IFolderView2 *iface, int item, REFIID riid, void **ppv)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %s %p), stub\n", This, item, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetVisibleItem(IFolderView2 *iface, int start, BOOL previous,
    int *item)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %d %p), stub\n", This, start, previous, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetSelectedItem(IFolderView2 *iface, int start, int *item)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %p), stub\n", This, start, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetSelection(IFolderView2 *iface, BOOL none_implies_folder,
    IShellItemArray **array)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %p), stub\n", This, none_implies_folder, array);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetSelectionState(IFolderView2 *iface, PCUITEMID_CHILD pidl,
    DWORD *flags)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p %p), stub\n", This, pidl, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_InvokeVerbOnSelection(IFolderView2 *iface, LPCSTR verb)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%s), stub\n", This, debugstr_a(verb));
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetViewModeAndIconSize(IFolderView2 *iface, FOLDERVIEWMODE mode,
    int size)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%d %d), stub\n", This, mode, size);
    return S_OK;
}

static HRESULT WINAPI FolderView2_GetViewModeAndIconSize(IFolderView2 *iface, FOLDERVIEWMODE *mode,
    int *size)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);

    FIXME("(%p)->(%p %p), stub\n", This, mode, size);

    *size = 16; /* FIXME */
    *mode = This->FolderSettings.ViewMode;

    return S_OK;
}

static HRESULT WINAPI FolderView2_SetGroupSubsetCount(IFolderView2 *iface, UINT visible_rows)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%u), stub\n", This, visible_rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_GetGroupSubsetCount(IFolderView2 *iface, UINT *visible_rows)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p)->(%p), stub\n", This, visible_rows);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_SetRedraw(IFolderView2 *iface, BOOL redraw)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    TRACE("(%p)->(%d)\n", This, redraw);
    SendMessageW(This->hWndList, WM_SETREDRAW, redraw, 0);
    return S_OK;
}

static HRESULT WINAPI FolderView2_IsMoveInSameFolder(IFolderView2 *iface)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p), stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FolderView2_DoRename(IFolderView2 *iface)
{
    IShellViewImpl *This = impl_from_IFolderView2(iface);
    FIXME("(%p), stub\n", This);
    return E_NOTIMPL;
}

static const IFolderView2Vtbl folderviewvtbl =
{
    FolderView_QueryInterface,
    FolderView_AddRef,
    FolderView_Release,
    FolderView_GetCurrentViewMode,
    FolderView_SetCurrentViewMode,
    FolderView_GetFolder,
    FolderView_Item,
    FolderView_ItemCount,
    FolderView_Items,
    FolderView_GetSelectionMarkedItem,
    FolderView_GetFocusedItem,
    FolderView_GetItemPosition,
    FolderView_GetSpacing,
    FolderView_GetDefaultSpacing,
    FolderView_GetAutoArrange,
    FolderView_SelectItem,
    FolderView_SelectAndPositionItems,
    FolderView2_SetGroupBy,
    FolderView2_GetGroupBy,
    FolderView2_SetViewProperty,
    FolderView2_GetViewProperty,
    FolderView2_SetTileViewProperties,
    FolderView2_SetExtendedTileViewProperties,
    FolderView2_SetText,
    FolderView2_SetCurrentFolderFlags,
    FolderView2_GetCurrentFolderFlags,
    FolderView2_GetSortColumnCount,
    FolderView2_SetSortColumns,
    FolderView2_GetSortColumns,
    FolderView2_GetItem,
    FolderView2_GetVisibleItem,
    FolderView2_GetSelectedItem,
    FolderView2_GetSelection,
    FolderView2_GetSelectionState,
    FolderView2_InvokeVerbOnSelection,
    FolderView2_SetViewModeAndIconSize,
    FolderView2_GetViewModeAndIconSize,
    FolderView2_SetGroupSubsetCount,
    FolderView2_GetGroupSubsetCount,
    FolderView2_SetRedraw,
    FolderView2_IsMoveInSameFolder,
    FolderView2_DoRename
};

/* IShellFolderView */
static HRESULT WINAPI IShellFolderView_fnQueryInterface(IShellFolderView *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    return IShellView3_QueryInterface(&This->IShellView3_iface, riid, ppvObj);
}

static ULONG WINAPI IShellFolderView_fnAddRef(IShellFolderView *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI IShellFolderView_fnRelease(IShellFolderView *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

static HRESULT WINAPI IShellFolderView_fnRearrange(IShellFolderView *iface, LPARAM sort)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%Id) stub\n", This, sort);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnGetArrangeParam(IShellFolderView *iface, LPARAM *sort)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, sort);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnArrangeGrid(IShellFolderView *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnAutoArrange(IShellFolderView *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)\n", This);
    return IFolderView2_SetCurrentFolderFlags(&This->IFolderView2_iface, FWF_AUTOARRANGE, FWF_AUTOARRANGE);
}

static HRESULT WINAPI IShellFolderView_fnGetAutoArrange(IShellFolderView *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)\n", This);
    return IFolderView2_GetAutoArrange(&This->IFolderView2_iface);
}

static HRESULT WINAPI IShellFolderView_fnAddObject(
    IShellFolderView *iface,
    PITEMID_CHILD pidl,
    UINT *item)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p %p) stub\n", This, pidl, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnGetObject(
    IShellFolderView *iface,
    PITEMID_CHILD *pidl,
    UINT item)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)->(%p %d)\n", This, pidl, item);
    return IFolderView2_Item(&This->IFolderView2_iface, item, pidl);
}

static HRESULT WINAPI IShellFolderView_fnRemoveObject(
    IShellFolderView *iface,
    PITEMID_CHILD pidl,
    UINT *item)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);

    TRACE("(%p)->(%p %p)\n", This, pidl, item);

    if (pidl)
    {
        *item = LV_FindItemByPidl(This, ILFindLastID(pidl));
        SendMessageW(This->hWndList, LVM_DELETEITEM, *item, 0);
    }
    else
    {
        *item = 0;
        SendMessageW(This->hWndList, LVM_DELETEALLITEMS, 0, 0);
    }

    return S_OK;
}

static HRESULT WINAPI IShellFolderView_fnGetObjectCount(
    IShellFolderView *iface,
    UINT *count)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)->(%p)\n", This, count);
    return IFolderView2_ItemCount(&This->IFolderView2_iface, SVGIO_ALLVIEW, (INT*)count);
}

static HRESULT WINAPI IShellFolderView_fnSetObjectCount(
    IShellFolderView *iface,
    UINT count,
    UINT flags)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%d %x) stub\n", This, count, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnUpdateObject(
    IShellFolderView *iface,
    PITEMID_CHILD pidl_old,
    PITEMID_CHILD pidl_new,
    UINT *item)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p %p %p) stub\n", This, pidl_old, pidl_new, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnRefreshObject(
    IShellFolderView *iface,
    PITEMID_CHILD pidl,
    UINT *item)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p %p) stub\n", This, pidl, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSetRedraw(
    IShellFolderView *iface,
    BOOL redraw)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)->(%d)\n", This, redraw);
    return IFolderView2_SetRedraw(&This->IFolderView2_iface, redraw);
}

static HRESULT WINAPI IShellFolderView_fnGetSelectedCount(
    IShellFolderView *iface,
    UINT *count)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    IShellItemArray *selection;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, count);

    *count = 0;
    hr = IFolderView2_GetSelection(&This->IFolderView2_iface, FALSE, &selection);
    if (FAILED(hr))
        return hr;

    hr = IShellItemArray_GetCount(selection, (DWORD *)count);
    IShellItemArray_Release(selection);
    return hr;
}

static HRESULT WINAPI IShellFolderView_fnGetSelectedObjects(
    IShellFolderView *iface,
    PCITEMID_CHILD **pidl,
    UINT *items)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);

    TRACE("(%p)->(%p %p)\n", This, pidl, items);

    *items = ShellView_GetSelections( This );

    if (*items)
    {
        *pidl = LocalAlloc(0, *items*sizeof(LPITEMIDLIST));
        if (!*pidl) return E_OUTOFMEMORY;

        /* it's documented that caller shouldn't free PIDLs, only array itself */
        memcpy((PITEMID_CHILD*)*pidl, This->apidl, *items*sizeof(LPITEMIDLIST));
    }

    return S_OK;
}

static HRESULT WINAPI IShellFolderView_fnIsDropOnSource(
    IShellFolderView *iface,
    IDropTarget *drop_target)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, drop_target);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnGetDragPoint(
    IShellFolderView *iface,
    POINT *pt)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, pt);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnGetDropPoint(
    IShellFolderView *iface,
    POINT *pt)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, pt);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnMoveIcons(
    IShellFolderView *iface,
    IDataObject *obj)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)->(%p)\n", This, obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSetItemPos(
    IShellFolderView *iface,
    PCUITEMID_CHILD pidl,
    POINT *pt)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p %p) stub\n", This, pidl, pt);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnIsBkDropTarget(
    IShellFolderView *iface,
    IDropTarget *drop_target)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, drop_target);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSetClipboard(
    IShellFolderView *iface,
    BOOL move)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%d) stub\n", This, move);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSetPoints(
    IShellFolderView *iface,
    IDataObject *obj)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnGetItemSpacing(
    IShellFolderView *iface,
    ITEMSPACING *spacing)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, spacing);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSetCallback(
    IShellFolderView    *iface,
    IShellFolderViewCB  *new_cb,
    IShellFolderViewCB **old_cb)

{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p %p) stub\n", This, new_cb, old_cb);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnSelect(
    IShellFolderView *iface,
    UINT flags)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%d) stub\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IShellFolderView_fnQuerySupport(
    IShellFolderView *iface,
    UINT *support)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    TRACE("(%p)->(%p)\n", This, support);
    return S_OK;
}

static HRESULT WINAPI IShellFolderView_fnSetAutomationObject(
    IShellFolderView *iface,
    IDispatch *disp)
{
    IShellViewImpl *This = impl_from_IShellFolderView(iface);
    FIXME("(%p)->(%p) stub\n", This, disp);
    return E_NOTIMPL;
}

static const IShellFolderViewVtbl shellfolderviewvtbl =
{
    IShellFolderView_fnQueryInterface,
    IShellFolderView_fnAddRef,
    IShellFolderView_fnRelease,
    IShellFolderView_fnRearrange,
    IShellFolderView_fnGetArrangeParam,
    IShellFolderView_fnArrangeGrid,
    IShellFolderView_fnAutoArrange,
    IShellFolderView_fnGetAutoArrange,
    IShellFolderView_fnAddObject,
    IShellFolderView_fnGetObject,
    IShellFolderView_fnRemoveObject,
    IShellFolderView_fnGetObjectCount,
    IShellFolderView_fnSetObjectCount,
    IShellFolderView_fnUpdateObject,
    IShellFolderView_fnRefreshObject,
    IShellFolderView_fnSetRedraw,
    IShellFolderView_fnGetSelectedCount,
    IShellFolderView_fnGetSelectedObjects,
    IShellFolderView_fnIsDropOnSource,
    IShellFolderView_fnGetDragPoint,
    IShellFolderView_fnGetDropPoint,
    IShellFolderView_fnMoveIcons,
    IShellFolderView_fnSetItemPos,
    IShellFolderView_fnIsBkDropTarget,
    IShellFolderView_fnSetClipboard,
    IShellFolderView_fnSetPoints,
    IShellFolderView_fnGetItemSpacing,
    IShellFolderView_fnSetCallback,
    IShellFolderView_fnSelect,
    IShellFolderView_fnQuerySupport,
    IShellFolderView_fnSetAutomationObject
};

static HRESULT WINAPI shellfolderviewdual_QueryInterface(IShellFolderViewDual3 *iface, REFIID riid, void **ppvObj)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);

    TRACE("(%p)->(IID:%s,%p)\n", This, debugstr_guid(riid), ppvObj);

    if (IsEqualIID(riid, &IID_IShellFolderViewDual3) ||
        IsEqualIID(riid, &IID_IShellFolderViewDual2) ||
        IsEqualIID(riid, &IID_IShellFolderViewDual) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IShellFolderViewDual3_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI shellfolderviewdual_AddRef(IShellFolderViewDual3 *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    return IShellView3_AddRef(&This->IShellView3_iface);
}

static ULONG WINAPI shellfolderviewdual_Release(IShellFolderViewDual3 *iface)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    return IShellView3_Release(&This->IShellView3_iface);
}

static HRESULT WINAPI shellfolderviewdual_GetTypeInfoCount(IShellFolderViewDual3 *iface, UINT *pctinfo)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI shellfolderviewdual_GetTypeInfo(IShellFolderViewDual3 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    HRESULT hr;

    TRACE("(%p,%u,%ld,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IShellFolderViewDual3_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI shellfolderviewdual_GetIDsOfNames(
        IShellFolderViewDual3 *iface, REFIID riid, LPOLESTR *rgszNames, UINT
        cNames, LCID lcid, DISPID *rgDispId)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p, %s, %p, %u, %ld, %p)\n", This, debugstr_guid(riid), rgszNames,
            cNames, lcid, rgDispId);

    hr = get_typeinfo(IShellFolderViewDual3_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI shellfolderviewdual_Invoke(IShellFolderViewDual3 *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p, %ld, %s, %ld, %u, %p, %p, %p, %p)\n", This, dispIdMember,
            debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult,
            pExcepInfo, puArgErr);

    hr = get_typeinfo(IShellFolderViewDual3_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, &This->IShellFolderViewDual3_iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
    return hr;

}

static HRESULT WINAPI shellfolderviewdual_get_Application(IShellFolderViewDual3 *iface,
    IDispatch **disp)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);

    TRACE("%p %p\n", This, disp);

    if (!disp)
        return E_INVALIDARG;

    return IShellDispatch_Constructor(NULL, &IID_IDispatch, (void**)disp);
}

static HRESULT WINAPI shellfolderviewdual_get_Parent(IShellFolderViewDual3 *iface, IDispatch **disp)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, disp);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_Folder(IShellFolderViewDual3 *iface, Folder **folder)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, folder);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_SelectedItems(IShellFolderViewDual3 *iface, FolderItems **items)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, items);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_FocusedItem(IShellFolderViewDual3 *iface,
    FolderItem **item)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, item);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_SelectItem(IShellFolderViewDual3 *iface,
    VARIANT *v, int flags)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %s %x\n", This, debugstr_variant(v), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_PopupItemMenu(IShellFolderViewDual3 *iface,
    FolderItem *item, VARIANT vx, VARIANT vy, BSTR *command)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p %s %s %p\n", This, item, debugstr_variant(&vx), debugstr_variant(&vy), command);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_Script(IShellFolderViewDual3 *iface, IDispatch **disp)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, disp);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_ViewOptions(IShellFolderViewDual3 *iface, LONG *options)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, options);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_CurrentViewMode(IShellFolderViewDual3 *iface, UINT *mode)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_put_CurrentViewMode(IShellFolderViewDual3 *iface, UINT mode)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %u\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_SelectItemRelative(IShellFolderViewDual3 *iface, int relative)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %d\n", This, relative);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_GroupBy(IShellFolderViewDual3 *iface, BSTR *groupby)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, groupby);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_put_GroupBy(IShellFolderViewDual3 *iface, BSTR groupby)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %s\n", This, debugstr_w(groupby));
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_FolderFlags(IShellFolderViewDual3 *iface, DWORD *flags)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_put_FolderFlags(IShellFolderViewDual3 *iface, DWORD flags)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p 0x%08lx\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_SortColumns(IShellFolderViewDual3 *iface, BSTR *sortcolumns)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, sortcolumns);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_put_SortColumns(IShellFolderViewDual3 *iface, BSTR sortcolumns)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %s\n", This, debugstr_w(sortcolumns));
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_put_IconSize(IShellFolderViewDual3 *iface, int icon_size)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %d\n", This, icon_size);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_get_IconSize(IShellFolderViewDual3 *iface, int *icon_size)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %p\n", This, icon_size);
    return E_NOTIMPL;
}

static HRESULT WINAPI shellfolderviewdual_FilterView(IShellFolderViewDual3 *iface, BSTR filter_text)
{
    IShellViewImpl *This = impl_from_IShellFolderViewDual3(iface);
    FIXME("%p %s\n", This, debugstr_w(filter_text));
    return E_NOTIMPL;
}

static const IShellFolderViewDual3Vtbl shellfolderviewdualvtbl =
{
    shellfolderviewdual_QueryInterface,
    shellfolderviewdual_AddRef,
    shellfolderviewdual_Release,
    shellfolderviewdual_GetTypeInfoCount,
    shellfolderviewdual_GetTypeInfo,
    shellfolderviewdual_GetIDsOfNames,
    shellfolderviewdual_Invoke,
    shellfolderviewdual_get_Application,
    shellfolderviewdual_get_Parent,
    shellfolderviewdual_get_Folder,
    shellfolderviewdual_SelectedItems,
    shellfolderviewdual_get_FocusedItem,
    shellfolderviewdual_SelectItem,
    shellfolderviewdual_PopupItemMenu,
    shellfolderviewdual_get_Script,
    shellfolderviewdual_get_ViewOptions,
    shellfolderviewdual_get_CurrentViewMode,
    shellfolderviewdual_put_CurrentViewMode,
    shellfolderviewdual_SelectItemRelative,
    shellfolderviewdual_get_GroupBy,
    shellfolderviewdual_put_GroupBy,
    shellfolderviewdual_get_FolderFlags,
    shellfolderviewdual_put_FolderFlags,
    shellfolderviewdual_get_SortColumns,
    shellfolderviewdual_put_SortColumns,
    shellfolderviewdual_put_IconSize,
    shellfolderviewdual_get_IconSize,
    shellfolderviewdual_FilterView
};

/**********************************************************
 *	IShellView_Constructor
 */
IShellView *IShellView_Constructor(IShellFolder *folder)
{
    IShellViewImpl *sv;

    sv = calloc(1, sizeof(*sv));
    if (!sv)
        return NULL;

    sv->ref = 1;
    sv->IShellView3_iface.lpVtbl = &shellviewvtbl;
    sv->IOleCommandTarget_iface.lpVtbl = &olecommandtargetvtbl;
    sv->IDropTarget_iface.lpVtbl = &droptargetvtbl;
    sv->IDropSource_iface.lpVtbl = &dropsourcevtbl;
    sv->IViewObject_iface.lpVtbl = &viewobjectvtbl;
    sv->IFolderView2_iface.lpVtbl = &folderviewvtbl;
    sv->IShellFolderView_iface.lpVtbl = &shellfolderviewvtbl;
    sv->IShellFolderViewDual3_iface.lpVtbl = &shellfolderviewdualvtbl;

    sv->pSFParent = folder;
    if (folder) IShellFolder_AddRef(folder);
    IShellFolder_QueryInterface(sv->pSFParent, &IID_IShellFolder2, (void**)&sv->pSF2Parent);

    sv->pCurDropTarget = NULL;
    sv->pCurDataObject = NULL;
    sv->iDragOverItem = 0;
    sv->cScrollDelay = 0;
    sv->ptLastMousePos.x = 0;
    sv->ptLastMousePos.y = 0;
    sv->FolderSettings.ViewMode = FVM_TILE;
    sv->FolderSettings.fFlags = FWF_USESEARCHFOLDER;

    TRACE("(%p)->(%p)\n", sv, folder);
    return (IShellView*)&sv->IShellView3_iface;
}

/*************************************************************************
 * SHCreateShellFolderView			[SHELL32.256]
 *
 * Create a new instance of the default Shell folder view object.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: error value
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderView(const SFV_CREATE *desc, IShellView **shellview)
{
    TRACE("(%p, %p)\n", desc, shellview);

    *shellview = NULL;

    if (!desc || desc->cbSize != sizeof(*desc))
        return E_INVALIDARG;

    TRACE("sf=%p outer=%p callback=%p\n", desc->pshf, desc->psvOuter, desc->psfvcb);

    if (!desc->pshf)
        return E_UNEXPECTED;

    *shellview = IShellView_Constructor(desc->pshf);
    if (!*shellview)
        return E_OUTOFMEMORY;

    if (desc->psfvcb)
    {
        IShellFolderView *view;
        IShellView_QueryInterface(*shellview, &IID_IShellFolderView, (void **)&view);
        IShellFolderView_SetCallback(view, desc->psfvcb, NULL);
        IShellFolderView_Release(view);
    }

    return S_OK;
}

/*************************************************************************
 * SHCreateShellFolderViewEx			[SHELL32.174]
 *
 * Create a new instance of the default Shell folder view object.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: error value
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx(CSFV *desc, IShellView **shellview)
{
    TRACE("(%p, %p)\n", desc, shellview);

    TRACE("sf=%p pidl=%p cb=%p mode=0x%08x parm=%p\n", desc->pshf, desc->pidl, desc->pfnCallback,
        desc->fvm, desc->psvOuter);

    if (!desc->pshf)
        return E_UNEXPECTED;

    *shellview = IShellView_Constructor(desc->pshf);
    if (!*shellview)
        return E_OUTOFMEMORY;

    return S_OK;
}
