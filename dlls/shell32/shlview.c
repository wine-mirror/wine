/*
 *	ShellView
 *
 *	Copyright 1998,1999	<juergen.schmied@metronet.de>
 *
 *  FIXME: when the ShellView_WndProc gets a WM_NCDESTROY should we do a
 *  Release() ??? 
 *
 */

#include <stdlib.h>
#include <string.h>

#include "servprov.h"
#include "wine/obj_base.h"
#include "wine/obj_shellfolder.h"
#include "wine/obj_shellview.h"
#include "wine/obj_commdlgbrowser.h"
#include "wine/obj_shellbrowser.h"
#include "wine/obj_dockingwindowframe.h"
#include "wine/obj_extracticon.h"
#include "wine/obj_dragdrop.h"
#include "wine/undocshell.h"
#include "shresdef.h"
#include "spy.h"
#include "debugtools.h"
#include "winerror.h"

#include "docobj.h"
#include "pidl.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

typedef struct 
{	ICOM_VTABLE(IShellView)* lpvtbl;
	DWORD		ref;
	ICOM_VTABLE(IOleCommandTarget)*	lpvtblOleCommandTarget;
	ICOM_VTABLE(IDropTarget)*	lpvtblDropTarget;
	ICOM_VTABLE(IDropSource)*	lpvtblDropSource;
	ICOM_VTABLE(IViewObject)*	lpvtblViewObject;
	IShellFolder*	pSFParent;
	IShellBrowser*	pShellBrowser;
	ICommDlgBrowser*	pCommDlgBrowser;
	HWND		hWnd;
	HWND		hWndList;
	HWND		hWndParent;
	FOLDERSETTINGS	FolderSettings;
	HMENU		hMenu;
	UINT		uState;
	UINT		cidl;
	LPITEMIDLIST	*apidl;
} IShellViewImpl;

static struct ICOM_VTABLE(IShellView) svvt;

static struct ICOM_VTABLE(IOleCommandTarget) ctvt;
#define _IOleCommandTarget_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblOleCommandTarget))) 
#define _ICOM_THIS_From_IOleCommandTarget(class, name) class* This = (class*)(((char*)name)-_IOleCommandTarget_Offset); 

static struct ICOM_VTABLE(IDropTarget) dtvt;
#define _IDropTarget_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblDropTarget))) 
#define _ICOM_THIS_From_IDropTarget(class, name) class* This = (class*)(((char*)name)-_IDropTarget_Offset); 

static struct ICOM_VTABLE(IDropSource) dsvt;
#define _IDropSource_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblDropSource))) 
#define _ICOM_THIS_From_IDropSource(class, name) class* This = (class*)(((char*)name)-_IDropSource_Offset); 

static struct ICOM_VTABLE(IViewObject) vovt;
#define _IViewObject_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblViewObject))) 
#define _ICOM_THIS_From_IViewObject(class, name) class* This = (class*)(((char*)name)-_IViewObject_Offset); 

/*menu items */
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     2000

#define TOOLBAR_ID   (L"SHELLDLL_DefView")
/*windowsx.h */
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
/* winuser.h */
#define WM_SETTINGCHANGE                WM_WININICHANGE
extern void WINAPI _InsertMenuItem (HMENU hmenu, UINT indexMenu, BOOL fByPosition, 
			UINT wID, UINT fType, LPSTR dwTypeData, UINT fState);

/*
  Items merged into the toolbar and and the filemenu
*/
typedef struct
{  int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   BYTE  bState;
   BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

MYTOOLINFO Tools[] = 
{
{ FCIDM_SHVIEW_BIGICON,    0, 0, IDS_VIEW_LARGE,   TBSTATE_ENABLED, TBSTYLE_BUTTON },
{ FCIDM_SHVIEW_SMALLICON,  0, 0, IDS_VIEW_SMALL,   TBSTATE_ENABLED, TBSTYLE_BUTTON },
{ FCIDM_SHVIEW_LISTVIEW,   0, 0, IDS_VIEW_LIST,    TBSTATE_ENABLED, TBSTYLE_BUTTON },
{ FCIDM_SHVIEW_REPORTVIEW, 0, 0, IDS_VIEW_DETAILS, TBSTATE_ENABLED, TBSTYLE_BUTTON },
{ -1, 0, 0, 0, 0, 0}
};

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

/**********************************************************
 *	IShellView_Constructor
 */
IShellView * IShellView_Constructor( IShellFolder * pFolder)
{	IShellViewImpl * sv;
	sv=(IShellViewImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IShellViewImpl));
	sv->ref=1;
	sv->lpvtbl=&svvt;
	sv->lpvtblOleCommandTarget=&ctvt;
	sv->lpvtblDropTarget=&dtvt;
	sv->lpvtblDropSource=&dsvt;
	sv->lpvtblViewObject=&vovt;

	sv->pSFParent	= pFolder;

	if(pFolder)
	  IShellFolder_AddRef(pFolder);

	TRACE("(%p)->(%p)\n",sv, pFolder);
	shell32_ObjCount++;
	return (IShellView *) sv;
}

/**********************************************************
 *
 * ##### helperfunctions for communication with ICommDlgBrowser #####
 */
static BOOL IsInCommDlg(IShellViewImpl * This)
{	return(This->pCommDlgBrowser != NULL);
}

static HRESULT IncludeObject(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
	HRESULT ret = S_OK;
	
	if ( IsInCommDlg(This) )
	{
	  TRACE("ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
	  ret = ICommDlgBrowser_IncludeObject(This->pCommDlgBrowser, (IShellView*)This, pidl);
	  TRACE("--0x%08lx\n", ret);
	}
	return ret;
}

static HRESULT OnDefaultCommand(IShellViewImpl * This)
{
	HRESULT ret = S_FALSE;
	
	if (IsInCommDlg(This))
	{
	  TRACE("ICommDlgBrowser::OnDefaultCommand\n");
	  ret = ICommDlgBrowser_OnDefaultCommand(This->pCommDlgBrowser, (IShellView*)This);
	  TRACE("--\n");
	}
	return ret;
}

static HRESULT OnStateChange(IShellViewImpl * This, UINT uFlags)
{
	HRESULT ret = S_FALSE;

	if (IsInCommDlg(This))
	{
	  TRACE("ICommDlgBrowser::OnStateChange flags=%x\n", uFlags);
	  ret = ICommDlgBrowser_OnStateChange(This->pCommDlgBrowser, (IShellView*)This, uFlags);
	  TRACE("--\n");
	}
	return ret;
}
/**********************************************************
 *
 * ##### helperfunctions for initializing the view #####
 */
/**********************************************************
 *	set the toolbar buttons
 */
static void CheckToolbar(IShellViewImpl * This)
{
	LRESULT result;

	TRACE("\n");
	
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_SMALLICON, (This->FolderSettings.ViewMode==FVM_LIST)? TRUE : FALSE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_REPORTVIEW, (This->FolderSettings.ViewMode==FVM_DETAILS)? TRUE : FALSE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_SMALLICON, TRUE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_REPORTVIEW, TRUE, &result);
}

/**********************************************************
 *	change the style of the listview control
 */

static void SetStyle(IShellViewImpl * This, DWORD dwAdd, DWORD dwRemove)
{
	DWORD tmpstyle;

	TRACE("(%p)\n", This);

	tmpstyle = GetWindowLongA(This->hWndList, GWL_STYLE);
	SetWindowLongA(This->hWndList, GWL_STYLE, dwAdd | (tmpstyle & ~dwRemove));
}

/**********************************************************
* ShellView_CreateList()
*
*/
static BOOL ShellView_CreateList (IShellViewImpl * This)
{	DWORD dwStyle;

	TRACE("%p\n",This);

	dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER |
		  LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_ALIGNLEFT | LVS_AUTOARRANGE;

	switch (This->FolderSettings.ViewMode)
	{
	  case FVM_ICON:	dwStyle |= LVS_ICON;		break;
	  case FVM_DETAILS: 	dwStyle |= LVS_REPORT;		break;
	  case FVM_SMALLICON: 	dwStyle |= LVS_SMALLICON;	break;
	  case FVM_LIST: 	dwStyle |= LVS_LIST;		break;
	  default:		dwStyle |= LVS_LIST;		break;
	}

	if (This->FolderSettings.fFlags && FWF_AUTOARRANGE)	dwStyle |= LVS_AUTOARRANGE;
	/*if (This->FolderSettings.fFlags && FWF_DESKTOP); used from explorer*/
	if (This->FolderSettings.fFlags && FWF_SINGLESEL)	dwStyle |= LVS_SINGLESEL;

	This->hWndList=CreateWindowExA( WS_EX_CLIENTEDGE,
					WC_LISTVIEWA,
					NULL,
					dwStyle,
					0,0,0,0,
					This->hWnd,
					(HMENU)ID_LISTVIEW,
					shell32_hInstance,
					NULL);

	if(!This->hWndList)
	  return FALSE;

	/*  UpdateShellSettings(); */
	return TRUE;
}
/**********************************************************
* ShellView_InitList()
*/
static BOOL ShellView_InitList(IShellViewImpl * This)
{
	LVCOLUMNA lvColumn;
	CHAR        szString[50];

	TRACE("%p\n",This);

	ListView_DeleteAllItems(This->hWndList);

	/*initialize the columns */
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	/*  |  LVCF_SUBITEM;*/
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = szString;

	lvColumn.cx = 120;
	LoadStringA(shell32_hInstance, IDS_SHV_COLUMN1, szString, sizeof(szString));
	ListView_InsertColumnA(This->hWndList, 0, &lvColumn);

	lvColumn.cx = 80;
	LoadStringA(shell32_hInstance, IDS_SHV_COLUMN2, szString, sizeof(szString));
	ListView_InsertColumnA(This->hWndList, 1, &lvColumn);

	lvColumn.cx = 170;
	LoadStringA(shell32_hInstance, IDS_SHV_COLUMN3, szString, sizeof(szString));
	ListView_InsertColumnA(This->hWndList, 2, &lvColumn);

	lvColumn.cx = 60;
	LoadStringA(shell32_hInstance, IDS_SHV_COLUMN4, szString, sizeof(szString));
	ListView_InsertColumnA(This->hWndList, 3, &lvColumn);

	ListView_SetImageList(This->hWndList, ShellSmallIconList, LVSIL_SMALL);
	ListView_SetImageList(This->hWndList, ShellBigIconList, LVSIL_NORMAL);

	return TRUE;
}
/**********************************************************
* ShellView_CompareItems() 
*
* NOTES
*  internal, CALLBACK for DSA_Sort
*/   
static INT CALLBACK ShellView_CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{
	int ret;
	TRACE("pidl1=%p pidl2=%p lpsf=%p\n", lParam1, lParam2, (LPVOID) lpData);

	if(!lpData) return 0;

	ret =  (SHORT) SCODE_CODE(IShellFolder_CompareIDs((LPSHELLFOLDER)lpData, 0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2));  
	TRACE("ret=%i\n",ret);
	return ret;
}

/**********************************************************
* ShellView_FillList()
*
* NOTES
*  internal
*/   

static HRESULT ShellView_FillList(IShellViewImpl * This)
{
	LPENUMIDLIST	pEnumIDList;
	LPITEMIDLIST	pidl;
	DWORD		dwFetched;
	UINT		i;
	LVITEMA	lvItem;
	HRESULT		hRes;
	HDPA		hdpa;

	TRACE("%p\n",This);

	/* get the itemlist from the shfolder*/  
	hRes = IShellFolder_EnumObjects(This->pSFParent,This->hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
	if (hRes != S_OK)
	{
	  if (hRes==S_FALSE)
	    return(NOERROR);
	  return(hRes);
	}

	/* create a pointer array */	
	hdpa = pDPA_Create(16);
	if (!hdpa)
	{
	  return(E_OUTOFMEMORY);
	}

	/* copy the items into the array*/
	while((S_OK == IEnumIDList_Next(pEnumIDList,1, &pidl, &dwFetched)) && dwFetched)
	{
	  if (pDPA_InsertPtr(hdpa, 0x7fff, pidl) == -1)
	  {
	    SHFree(pidl);
	  } 
	}

	/*sort the array*/
	pDPA_Sort(hdpa, ShellView_CompareItems, (LPARAM)This->pSFParent);

	/*turn the listview's redrawing off*/
	SendMessageA(This->hWndList, WM_SETREDRAW, FALSE, 0); 

	for (i=0; i < DPA_GetPtrCount(hdpa); ++i) 	/* DPA_GetPtrCount is a macro*/
	{
	  pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
	  if (IncludeObject(This, pidl) == S_OK)	/* in a commdlg This works as a filemask*/
	  {
	    ZeroMemory(&lvItem, sizeof(lvItem));	/* create the listviewitem*/
	    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;		/*set the mask*/
	    lvItem.iItem = ListView_GetItemCount(This->hWndList);	/*add the item to the end of the list*/
	    lvItem.lParam = (LPARAM) pidl;				/*set the item's data*/
	    lvItem.pszText = LPSTR_TEXTCALLBACKA;			/*get text on a callback basis*/
	    lvItem.iImage = I_IMAGECALLBACK;				/*get the image on a callback basis*/
	    ListView_InsertItemA(This->hWndList, &lvItem);
	  }
	  else
	    SHFree(pidl);	/* the listview has the COPY*/
	}

	/*turn the listview's redrawing back on and force it to draw*/
	SendMessageA(This->hWndList, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(This->hWndList, NULL, TRUE);
	UpdateWindow(This->hWndList);

	IEnumIDList_Release(pEnumIDList); /* destroy the list*/
	pDPA_Destroy(hdpa);
	
	return S_OK;
}

/**********************************************************
*  ShellView_OnCreate()
*/   
static LRESULT ShellView_OnCreate(IShellViewImpl * This)
{
#if 0
	IDropTarget* pdt;
#endif
	TRACE("%p\n",This);

	if(ShellView_CreateList(This))
	{
	  if(ShellView_InitList(This))
	  {
	    ShellView_FillList(This);
	  }
	}
	
#if 0
	/* This makes a chrash since we havn't called OleInititialize. But if we
	do this call in DllMain it breaks some apps. The native shell32 does not 
	call OleInitialize and not even depend on ole32.dll. 
	But for some apps it works...*/
	if (SUCCEEDED(IShellFolder_CreateViewObject(This->pSFParent, This->hWnd, &IID_IDropTarget, (LPVOID*)&pdt)))
	{
	  pRegisterDragDrop(This->hWnd, pdt);
	  IDropTarget_Release(pdt);
	}
#endif
	return S_OK;
}

/**********************************************************
 *	#### Handling of the menus ####
 */

/**********************************************************
* ShellView_BuildFileMenu()
*/
static HMENU ShellView_BuildFileMenu(IShellViewImpl * This)
{	CHAR	szText[MAX_PATH];
	MENUITEMINFOA	mii;
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
	    LoadStringA(shell32_hInstance, Tools[i].idMenuString, szText, MAX_PATH);

	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	    if(TBSTYLE_SEP != Tools[i].bStyle) /* no seperator*/
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
	    InsertMenuItemA(hSubMenu, (UINT)-1, TRUE, &mii);
	  }
	}
	TRACE("-- return (menu=0x%x)\n",hSubMenu);
	return hSubMenu;
}
/**********************************************************
* ShellView_MergeFileMenu()
*/
static void ShellView_MergeFileMenu(IShellViewImpl * This, HMENU hSubMenu)
{	TRACE("(%p)->(submenu=0x%08x) stub\n",This,hSubMenu);

	if(hSubMenu)
	{ /*insert This item at the beginning of the menu */
	  _InsertMenuItem(hSubMenu, 0, TRUE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);
	  _InsertMenuItem(hSubMenu, 0, TRUE, IDM_MYFILEITEM, MFT_STRING, "dummy45", MFS_ENABLED);

	}
	TRACE("--\n");	
}

/**********************************************************
* ShellView_MergeViewMenu()
*/

static void ShellView_MergeViewMenu(IShellViewImpl * This, HMENU hSubMenu)
{	MENUITEMINFOA	mii;

	TRACE("(%p)->(submenu=0x%08x)\n",This,hSubMenu);

	if(hSubMenu)
	{ /*add a separator at the correct position in the menu*/
	  _InsertMenuItem(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);

	  ZeroMemory(&mii, sizeof(mii));
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;;
	  mii.fType = MFT_STRING;
	  mii.dwTypeData = "View";
	  mii.hSubMenu = LoadMenuA(shell32_hInstance, "MENU_001");
	  InsertMenuItemA(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
	}
}

/**********************************************************
*   ShellView_GetSelections()
*
* RETURNS
*  number of selected items
*/   
static UINT ShellView_GetSelections(IShellViewImpl * This)
{
	LVITEMA	lvItem;
	UINT	i = 0;

	if (This->apidl)
	{
	  SHFree(This->apidl);
	}

	This->cidl = ListView_GetSelectedCount(This->hWndList);
	This->apidl = (LPITEMIDLIST*)SHAlloc(This->cidl * sizeof(LPITEMIDLIST));

	TRACE("selected=%i\n", This->cidl);
	
	if(This->apidl)
	{
	  TRACE("-- Items selected =%u\n", This->cidl);

	  ZeroMemory(&lvItem, sizeof(lvItem));
	  lvItem.mask = LVIF_STATE | LVIF_PARAM;
	  lvItem.stateMask = LVIS_SELECTED;

	  while(ListView_GetItemA(This->hWndList, &lvItem) && (i < This->cidl))
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
 *	ShellView_DoContextMenu()
 */
static void ShellView_DoContextMenu(IShellViewImpl * This, WORD x, WORD y, BOOL bDefault)
{	UINT	uCommand;
	DWORD	wFlags;
	HMENU	hMenu;
	BOOL	fExplore = FALSE;
	HWND	hwndTree = 0;
	LPCONTEXTMENU	pContextMenu = NULL;
	IContextMenu *	pCM = NULL;
	CMINVOKECOMMANDINFO	cmi;
	
	TRACE("(%p)->(0x%08x 0x%08x 0x%08x) stub\n",This, x, y, bDefault);

	/* look, what's selected and create a context menu object of it*/
	if( ShellView_GetSelections(This) )
	{
	  IShellFolder_GetUIObjectOf( This->pSFParent, This->hWndParent, This->cidl, This->apidl, 
					(REFIID)&IID_IContextMenu, NULL, (LPVOID *)&pContextMenu);

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
		  if (IsInCommDlg(This) && ((uCommand==IDM_EXPLORE) || (uCommand==IDM_OPEN)))
		  {
		    TRACE("-- dlg: OnDefaultCommand\n");
		    OnDefaultCommand(This);
		  }
		  else
		  {
		    TRACE("-- explore -- invoke command\n");
		    ZeroMemory(&cmi, sizeof(cmi));
		    cmi.cbSize = sizeof(cmi);
		    cmi.hwnd = This->hWndParent;
		    cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
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
	  hMenu = CreatePopupMenu();

	  pCM = ISvBgCm_Constructor();
	  IContextMenu_QueryContextMenu(pCM, hMenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST, 0);

	  uCommand = TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
	  DestroyMenu(hMenu);

	  TRACE("-- (%p)->(uCommand=0x%08x )\n",This, uCommand);

	  ZeroMemory(&cmi, sizeof(cmi));
	  cmi.cbSize = sizeof(cmi);
	  cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand);
	  cmi.hwnd = This->hWndParent;
	  IContextMenu_InvokeCommand(pCM, &cmi);

	  IContextMenu_Release(pCM);
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
static LRESULT ShellView_OnActivate(IShellViewImpl * This, UINT uState)
{	OLEMENUGROUPWIDTHS   omw = { {0, 0, 0, 0, 0, 0} };
	MENUITEMINFOA         mii;
	CHAR                szText[MAX_PATH];

	TRACE("%p uState=%x\n",This,uState);    

	/*don't do anything if the state isn't really changing */
	if(This->uState == uState)
	{
	  return S_OK;
	}

	ShellView_OnDeactivate(This);

	/*only do This if we are active */
	if(uState != SVUIA_DEACTIVATE)
	{
	  /*merge the menus */
	  This->hMenu = CreateMenu();

	  if(This->hMenu)
	  {
	    IShellBrowser_InsertMenusSB(This->pShellBrowser, This->hMenu, &omw);
	    TRACE("-- after fnInsertMenusSB\n");    

	    /*build the top level menu get the menu item's text*/
	    strcpy(szText,"dummy 31");

	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	    mii.fType = MFT_STRING;
	    mii.fState = MFS_ENABLED;
	    mii.dwTypeData = szText;
	    mii.hSubMenu = ShellView_BuildFileMenu(This);

	    /*insert our menu into the menu bar*/
	    if(mii.hSubMenu)
	    {
	      InsertMenuItemA(This->hMenu, FCIDM_MENU_HELP, FALSE, &mii);
	    }

	    /*get the view menu so we can merge with it*/
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU;

	    if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
	    {
	      ShellView_MergeViewMenu(This, mii.hSubMenu);
	    }

	    /*add the items that should only be added if we have the focus*/
	    if(SVUIA_ACTIVATE_FOCUS == uState)
	    {
	      /*get the file menu so we can merge with it */
	      ZeroMemory(&mii, sizeof(mii));
	      mii.cbSize = sizeof(mii);
	      mii.fMask = MIIM_SUBMENU;

	      if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_FILE, FALSE, &mii))
	      {
	        ShellView_MergeFileMenu(This, mii.hSubMenu);
	      }
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

	IShellBrowser_OnViewWindowActive(This->pShellBrowser,(IShellView*) This);
	ShellView_OnActivate(This, SVUIA_ACTIVATE_FOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnKillFocus()
*/   
static LRESULT ShellView_OnKillFocus(IShellViewImpl * This)
{
	TRACE("(%p) stub\n",This);

	ShellView_OnActivate(This, SVUIA_ACTIVATE_NOFOCUS);

	return 0;
}

/**********************************************************
* ShellView_OnCommand()
*/   
static LRESULT ShellView_OnCommand(IShellViewImpl * This,DWORD dwCmdID, DWORD dwCmd, HWND hwndCmd)
{
	TRACE("(%p)->(0x%08lx 0x%08lx 0x%08x) stub\n",This, dwCmdID, dwCmd, hwndCmd);

	switch(dwCmdID)
	{
	  case FCIDM_SHVIEW_SMALLICON:
	    This->FolderSettings.ViewMode = FVM_SMALLICON;
	    SetStyle (This, LVS_SMALLICON, LVS_TYPEMASK);
	    break;

	  case FCIDM_SHVIEW_BIGICON:
	    This->FolderSettings.ViewMode = FVM_ICON;
	    SetStyle (This, LVS_ICON, LVS_TYPEMASK);
	    break;

	  case FCIDM_SHVIEW_LISTVIEW:
	    This->FolderSettings.ViewMode = FVM_LIST;
	    SetStyle (This, LVS_LIST, LVS_TYPEMASK);
	    break;

	  case FCIDM_SHVIEW_REPORTVIEW:
	    This->FolderSettings.ViewMode = FVM_DETAILS;
	    SetStyle (This, LVS_REPORT, LVS_TYPEMASK);
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
{	NM_LISTVIEW *lpnmlv = (NM_LISTVIEW*)lpnmh;
	NMLVDISPINFOA *lpdi = (NMLVDISPINFOA *)lpnmh;
	LPITEMIDLIST pidl;
	STRRET   str;  

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
	    break;

	  case HDN_ENDTRACKA:
	    TRACE("-- HDN_ENDTRACKA %p\n",This);
	    /*nColumn1 = ListView_GetColumnWidth(This->hWndList, 0);
	    nColumn2 = ListView_GetColumnWidth(This->hWndList, 1);*/
	    break;

	  case LVN_DELETEITEM:
	    TRACE("-- LVN_DELETEITEM %p\n",This);
	    SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
	    break;

	  case LVN_ITEMACTIVATE:
	    TRACE("-- LVN_ITEMACTIVATE %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    ShellView_DoContextMenu(This, 0, 0, TRUE);
	    break;

	  case LVN_GETDISPINFOA:
	    TRACE("-- LVN_GETDISPINFOA %p\n",This);
	    pidl = (LPITEMIDLIST)lpdi->item.lParam;


	    if(lpdi->item.iSubItem)		  /*is the sub-item information being requested?*/
	    { if(lpdi->item.mask & LVIF_TEXT)	 /*is the text being requested?*/
	      { if(_ILIsValue(pidl))	/*is This a value or a folder?*/
	        { switch (lpdi->item.iSubItem)
		  { case 1:	/* size */
		      _ILGetFileSize (pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
		      break;
		    case 2:	/* extension */
		      {	char sTemp[64];
		        if (_ILGetExtension (pidl, sTemp, 64))
			{ if (!( HCR_MapTypeToValue(sTemp, sTemp, 64, TRUE)
			         && HCR_MapTypeToValue(sTemp, lpdi->item.pszText, lpdi->item.cchTextMax, FALSE )))
			  { lstrcpynA (lpdi->item.pszText, sTemp, lpdi->item.cchTextMax);
			    strncat (lpdi->item.pszText, "-file", lpdi->item.cchTextMax);
			  }
			}
			else	/* no extension found */
			{ lpdi->item.pszText[0]=0x00;
			}    
		      }
		      break;
		    case 3:	/* date */
		      _ILGetFileDate (pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
		      break;
		  }
	        }
	        else  /*its a folder*/
	        { switch (lpdi->item.iSubItem)
		  { case 1:
		      strcpy(lpdi->item.pszText, "");
		      break;
	            case 2:
		      lstrcpynA (lpdi->item.pszText, "Folder", lpdi->item.cchTextMax);
		      break;
		    case 3:  
		      _ILGetFileDate (pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
		      break;
		  }
	        }
	        TRACE("-- text=%s\n",lpdi->item.pszText);		
	      }
	    }
	    else	   /*the item text is being requested*/
	    { 
	      if(lpdi->item.mask & LVIF_TEXT)	   /*is the text being requested?*/
	      {
	        if(SUCCEEDED(IShellFolder_GetDisplayNameOf(This->pSFParent,pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str)))
	        {
		  StrRetToStrNA(lpdi->item.pszText, lpdi->item.cchTextMax, &str, pidl); 
	        }
	        TRACE("-- text=%s\n",lpdi->item.pszText);
	      }

	      if(lpdi->item.mask & LVIF_IMAGE) 		/*is the image being requested?*/
	      {
	        lpdi->item.iImage = SHMapPIDLToSystemImageListIndex(This->pSFParent, pidl, 0);
	      }
	    }
	    break;

	  case LVN_ITEMCHANGED:
	    TRACE("-- LVN_ITEMCHANGED %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  case LVN_BEGINDRAG:
	  case LVN_BEGINRDRAG:

	    if (ShellView_GetSelections(This))
	    {  
	      IDataObject * pda;
	      DWORD dwAttributes;
	      DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
	      
	      if (SUCCEEDED(IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, This->apidl, &IID_IDataObject,0,(LPVOID *)&pda)))
	      {
	        IDropSource * pds = (IDropSource*)&(This->lpvtblDropSource);	/* own DropSource interface */

		if (SUCCEEDED(IShellFolder_GetAttributesOf(This->pSFParent, This->cidl, This->apidl, &dwAttributes)))
		{
		  if (dwAttributes & SFGAO_CANLINK)
		  {
		    dwEffect |= DROPEFFECT_LINK;
		  }
		}
		
	        if (pds)
	        {
	          DWORD dwEffect;
		  pDoDragDrop(pda, pds, dwEffect, &dwEffect);
		}

	        IDataObject_Release(pda);
	      }
	    }
	    break;

	  case LVN_BEGINLABELEDITA:
	  case LVN_ENDLABELEDITA:
	     FIXME("labeledit\n");
	     break;
	  
	  default:
	    TRACE("-- %p WM_COMMAND %s unhandled\n", This, SPY_GetMsgName(lpnmh->code));
	    break;;
	}
	return 0;
}

/**********************************************************
*  ShellView_WndProc
*/

static LRESULT CALLBACK ShellView_WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	IShellViewImpl * pThis = (IShellViewImpl*)GetWindowLongA(hWnd, GWL_USERDATA);
	LPCREATESTRUCTA lpcs;

	TRACE("(hwnd=%x msg=%x wparm=%x lparm=%lx)\n",hWnd, uMessage, wParam, lParam);

	switch (uMessage)
	{
	  case WM_NCCREATE:
	    lpcs = (LPCREATESTRUCTA)lParam;
	    pThis = (IShellViewImpl*)(lpcs->lpCreateParams);
	    SetWindowLongA(hWnd, GWL_USERDATA, (LONG)pThis);
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

	  case WM_CONTEXTMENU:  ShellView_DoContextMenu(pThis, LOWORD(lParam), HIWORD(lParam), FALSE);
	                        return 0;

	  case WM_SHOWWINDOW:	UpdateWindow(pThis->hWndList);
				break;

	  case WM_DESTROY:	pRevokeDragDrop(pThis->hWnd);
	                        break;
	}

	return DefWindowProcA (hWnd, uMessage, wParam, lParam);
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
static HRESULT WINAPI IShellView_fnQueryInterface(IShellView * iface,REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IShellViewImpl, iface);

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{
	  *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellView))
	{
	  *ppvObj = (IShellView*)This;
	}
	else if(IsEqualIID(riid, &IID_IOleCommandTarget))
	{
	  *ppvObj = (IOleCommandTarget*)&(This->lpvtblOleCommandTarget);
	}
	else if(IsEqualIID(riid, &IID_IDropTarget))
	{
	  *ppvObj = (IDropTarget*)&(This->lpvtblDropTarget);
	}
	else if(IsEqualIID(riid, &IID_IDropSource))
	{
	  *ppvObj = (IDropSource*)&(This->lpvtblDropSource);
	}
	else if(IsEqualIID(riid, &IID_IViewObject))
	{
	  *ppvObj = (IViewObject*)&(This->lpvtblViewObject);
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
static ULONG WINAPI IShellView_fnAddRef(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/**********************************************************
*  IShellView_Release
*/
static ULONG WINAPI IShellView_fnRelease(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{
	  TRACE(" destroying IShellView(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  if (This->apidl)
	    SHFree(This->apidl);

	  if (This->pCommDlgBrowser)
	    ICommDlgBrowser_Release(This->pCommDlgBrowser);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**********************************************************
*  ShellView_GetWindow
*/
static HRESULT WINAPI IShellView_fnGetWindow(IShellView * iface,HWND * phWnd)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)\n",This);

	*phWnd = This->hWnd;

	return S_OK;
}

static HRESULT WINAPI IShellView_fnContextSensitiveHelp(IShellView * iface,BOOL fEnterMode)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

/**********************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
static HRESULT WINAPI IShellView_fnTranslateAccelerator(IShellView * iface,LPMSG lpmsg)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME("(%p)->(%p: hwnd=%x msg=%x lp=%lx wp=%x) stub\n",This,lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);

	
	switch (lpmsg->message)
	{ case WM_KEYDOWN: 	TRACE("-- key=0x04%x",lpmsg->wParam) ;
	}
	return NOERROR;
}

static HRESULT WINAPI IShellView_fnEnableModeless(IShellView * iface,BOOL fEnable)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnUIActivate(IShellView * iface,UINT uState)
{
	ICOM_THIS(IShellViewImpl, iface);

	CHAR	szName[MAX_PATH];
	LRESULT	lResult;
	int	nPartArray[1] = {-1};

	TRACE("(%p)->(state=%x) stub\n",This, uState);

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

	  IShellFolder_GetFolderPath( This->pSFParent, szName, sizeof(szName) );

	  /* set the number of parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETPARTS, 1,
	  						(LPARAM)nPartArray, &lResult);

	  /* set the text for the parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETTEXTA,
							0, (LPARAM)szName, &lResult);
	}

	return S_OK;
}

static HRESULT WINAPI IShellView_fnRefresh(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)\n",This);

	ListView_DeleteAllItems(This->hWndList);
	ShellView_FillList(This);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnCreateViewWindow(
	IShellView * iface,
	IShellView *lpPrevView,
	LPCFOLDERSETTINGS lpfs,
	IShellBrowser * psb,
	RECT * prcView,
	HWND  *phWnd)
{
	ICOM_THIS(IShellViewImpl, iface);

	WNDCLASSA wc;
	*phWnd = 0;

	
	TRACE("(%p)->(shlview=%p set=%p shlbrs=%p rec=%p hwnd=%p) incomplete\n",This, lpPrevView,lpfs, psb, prcView, phWnd);
	TRACE("-- vmode=%x flags=%x left=%i top=%i right=%i bottom=%i\n",lpfs->ViewMode, lpfs->fFlags ,prcView->left,prcView->top, prcView->right, prcView->bottom);

	/*set up the member variables*/
	This->pShellBrowser = psb;
	This->FolderSettings = *lpfs;

	/*get our parent window*/
	IShellBrowser_AddRef(This->pShellBrowser);
	IShellBrowser_GetWindow(This->pShellBrowser, &(This->hWndParent));

	/* try to get the ICommDlgBrowserInterface, adds a reference !!! */
	This->pCommDlgBrowser=NULL;
	if ( SUCCEEDED (IShellBrowser_QueryInterface( This->pShellBrowser, 
			(REFIID)&IID_ICommDlgBrowser, (LPVOID*) &This->pCommDlgBrowser)))
	{
	  TRACE("-- CommDlgBrowser\n");
	}

	/*if our window class has not been registered, then do so*/
	if(!GetClassInfoA(shell32_hInstance, SV_CLASS_NAME, &wc))
	{
	  ZeroMemory(&wc, sizeof(wc));
	  wc.style		= CS_HREDRAW | CS_VREDRAW;
	  wc.lpfnWndProc	= (WNDPROC) ShellView_WndProc;
	  wc.cbClsExtra		= 0;
	  wc.cbWndExtra		= 0;
	  wc.hInstance		= shell32_hInstance;
	  wc.hIcon		= 0;
	  wc.hCursor		= LoadCursorA (0, IDC_ARROWA);
	  wc.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
	  wc.lpszMenuName	= NULL;
	  wc.lpszClassName	= SV_CLASS_NAME;

	  if(!RegisterClassA(&wc))
	    return E_FAIL;
	}

	*phWnd = CreateWindowExA(0,
				SV_CLASS_NAME,
				NULL,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
				prcView->left,
				prcView->top,
				prcView->right - prcView->left,
				prcView->bottom - prcView->top,
				This->hWndParent,
				0,
				shell32_hInstance,
				(LPVOID)This);

	CheckToolbar(This);
	
	if(!*phWnd)
	  return E_FAIL;

	return S_OK;
}

static HRESULT WINAPI IShellView_fnDestroyViewWindow(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)\n",This);

	/*Make absolutely sure all our UI is cleaned up.*/
	IShellView_UIActivate((IShellView*)This, SVUIA_DEACTIVATE);

	if(This->hMenu)
	{
	  DestroyMenu(This->hMenu);
	}

	DestroyWindow(This->hWnd);
	IShellBrowser_Release(This->pShellBrowser);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnGetCurrentInfo(IShellView * iface, LPFOLDERSETTINGS lpfs)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE("(%p)->(%p) vmode=%x flags=%x\n",This, lpfs, 
		This->FolderSettings.ViewMode, This->FolderSettings.fFlags);

	if (!lpfs) return E_INVALIDARG;

	*lpfs = This->FolderSettings;
	return NOERROR;
}

static HRESULT WINAPI IShellView_fnAddPropertySheetPages(IShellView * iface, DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnSaveViewState(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME("(%p) stub\n",This);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnSelectItem(IShellView * iface, LPCITEMIDLIST pidlItem, UINT uFlags)
{	ICOM_THIS(IShellViewImpl, iface);
	
	FIXME("(%p)->(pidl=%p, 0x%08x) stub\n",This, pidlItem, uFlags);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnGetItemObject(IShellView * iface, UINT uItem, REFIID riid, LPVOID *ppvOut)
{
	ICOM_THIS(IShellViewImpl, iface);

	char    xriid[50];
	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(uItem=0x%08x,\n\tIID=%s, ppv=%p)\n",This, uItem, xriid, ppvOut);

	*ppvOut = NULL;

	switch(uItem)
	{
	  case SVGIO_BACKGROUND:
	    *ppvOut = ISvBgCm_Constructor();
	    break;

	  case SVGIO_SELECTION:
	    ShellView_GetSelections(This);
	    IShellFolder_GetUIObjectOf(This->pSFParent, This->hWnd, This->cidl, This->apidl, riid, 0, ppvOut);
	    break;
	}
	TRACE("-- (%p)->(interface=%p)\n",This, *ppvOut);

	if(!*ppvOut) return E_OUTOFMEMORY;

	return S_OK;
}

static struct ICOM_VTABLE(IShellView) svvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	IShellView_fnGetItemObject
};


/**********************************************************
 * ISVOleCmdTarget_QueryInterface (IUnknown)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryInterface(
	IOleCommandTarget *	iface,
	REFIID			iid,
	LPVOID*			ppvObj)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	return IShellFolder_QueryInterface((IShellFolder*)This, iid, ppvObj);
}

/**********************************************************
 * ISVOleCmdTarget_AddRef (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_AddRef(
	IOleCommandTarget *	iface)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellFolder, iface);

	return IShellFolder_AddRef((IShellFolder*)This);
}

/**********************************************************
 * ISVOleCmdTarget_Release (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_Release(
	IOleCommandTarget *	iface)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	return IShellFolder_Release((IShellFolder*)This);
}

/**********************************************************
 * ISVOleCmdTarget_QueryStatus (IOleCommandTarget)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryStatus(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	ULONG cCmds, 
	OLECMD * prgCmds,
	OLECMDTEXT* pCmdText)
{
	char    xguid[50];

	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	WINE_StringFromCLSID((LPCLSID)pguidCmdGroup,xguid);

	FIXME("(%p)->(%p(%s) 0x%08lx %p %p\n", This, pguidCmdGroup, xguid, cCmds, prgCmds, pCmdText);
	return E_NOTIMPL;
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
	char    xguid[50];

	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	WINE_StringFromCLSID((LPCLSID)pguidCmdGroup,xguid);

	FIXME("(%p)->(\n\tTarget GUID:%s Command:0x%08lx Opt:0x%08lx %p %p)\n", This, xguid, nCmdID, nCmdexecopt, pvaIn, pvaOut);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IOleCommandTarget) ctvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISVOleCmdTarget_QueryInterface,
	ISVOleCmdTarget_AddRef,
	ISVOleCmdTarget_Release,
	ISVOleCmdTarget_QueryStatus,
	ISVOleCmdTarget_Exec
};

/**********************************************************
 * ISVDropTarget implementation
 */

static HRESULT WINAPI ISVDropTarget_QueryInterface(
	IDropTarget *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	char	xriid[50];

	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVDropTarget_AddRef( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVDropTarget_Release( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
}

static HRESULT WINAPI ISVDropTarget_DragEnter(
	IDropTarget 	*iface,
	IDataObject	*pDataObject,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{	

	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME("Stub: This=%p, DataObject=%p\n",This,pDataObject);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISVDropTarget_DragOver(
	IDropTarget	*iface,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISVDropTarget_DragLeave(
	IDropTarget	*iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISVDropTarget_Drop(
	IDropTarget	*iface,
	IDataObject*	pDataObject,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IDropTarget) dtvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static HRESULT WINAPI ISVDropSource_QueryInterface(
	IDropSource *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	char	xriid[50];

	_ICOM_THIS_From_IDropSource(IShellViewImpl, iface);

	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVDropSource_AddRef( IDropSource *iface)
{
	_ICOM_THIS_From_IDropSource(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVDropSource_Release( IDropSource *iface)
{
	_ICOM_THIS_From_IDropSource(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
}
static HRESULT WINAPI ISVDropSource_QueryContinueDrag(
	IDropSource *iface,
	BOOL fEscapePressed,
	DWORD grfKeyState)
{
	_ICOM_THIS_From_IDropSource(IShellViewImpl, iface);
	TRACE("(%p)\n",This);

	if (fEscapePressed)
	  return DRAGDROP_S_CANCEL;
	else if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON))
	  return DRAGDROP_S_DROP;
	else
	  return NOERROR;
}

static HRESULT WINAPI ISVDropSource_GiveFeedback(
	IDropSource *iface,
	DWORD dwEffect)
{
	_ICOM_THIS_From_IDropSource(IShellViewImpl, iface);
	TRACE("(%p)\n",This);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static struct ICOM_VTABLE(IDropSource) dsvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISVDropSource_QueryInterface,
	ISVDropSource_AddRef,
	ISVDropSource_Release,
	ISVDropSource_QueryContinueDrag,
	ISVDropSource_GiveFeedback
};
/**********************************************************
 * ISVViewObject implementation
 */

static HRESULT WINAPI ISVViewObject_QueryInterface(
	IViewObject *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	char	xriid[50];

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVViewObject_AddRef( IViewObject *iface)
{
	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVViewObject_Release( IViewObject *iface)
{
	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_Release((IShellFolder*)This);
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
	IVO_ContCallback pfnContinue,
	DWORD dwContinue)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

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

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

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

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Unfreeze(
	IViewObject 	*iface,
	DWORD dwFreeze)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_SetAdvise(
	IViewObject 	*iface,
	DWORD aspects,
	DWORD advf,
	IAdviseSink* pAdvSink)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_GetAdvise(
	IViewObject 	*iface,
	DWORD* pAspects,
	DWORD* pAdvf,
	IAdviseSink** ppAdvSink)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}


static struct ICOM_VTABLE(IViewObject) vovt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

