/*
 *	ShellView
 *
 *	Copyright 1998	<juergen.schmied@metronet.de>
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

#include "shresdef.h"
#include "spy.h"
#include "debug.h"
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
	UINT		uSelected;
	LPITEMIDLIST	*aSelectedItems;
} IShellViewImpl;

static struct ICOM_VTABLE(IShellView) svvt;

static struct ICOM_VTABLE(IOleCommandTarget) ctvt;
#define _IOleCommandTarget_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblOleCommandTarget))) 
#define _ICOM_THIS_From_IOleCommandTarget(class, name) class* This = (class*)(((void*)name)-_IOleCommandTarget_Offset); 

static struct ICOM_VTABLE(IDropTarget) dtvt;
#define _IDropTarget_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblDropTarget))) 
#define _ICOM_THIS_From_IDropTarget(class, name) class* This = (class*)(((void*)name)-_IDropTarget_Offset); 

static struct ICOM_VTABLE(IViewObject) vovt;
#define _IViewObject_Offset ((int)(&(((IShellViewImpl*)0)->lpvtblViewObject))) 
#define _ICOM_THIS_From_IViewObject(class, name) class* This = (class*)(((void*)name)-_IViewObject_Offset); 

/*menu items */
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     2000

#define MENU_OFFSET  1
#define MENU_MAX     100

#define TOOLBAR_ID   (L"SHELLDLL_DefView")
/*windowsx.h */
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
/* winuser.h */
#define WM_SETTINGCHANGE                WM_WININICHANGE
extern void WINAPI _InsertMenuItem (HMENU hmenu, UINT indexMenu, BOOL fByPosition, 
			UINT wID, UINT fType, LPSTR dwTypeData, UINT fState);

typedef struct
{  int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   int   nStringOffset;
   BYTE  bState;
   BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

extern 	LPCVOID _Resource_Men_MENU_001_0_data;
extern 	LPCVOID _Resource_Men_MENU_002_0_data;

MYTOOLINFO g_Tools[] = 
{ {IDM_VIEW_FILES, 0, IDS_TB_VIEW_FILES, IDS_MI_VIEW_FILES, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {-1, 0, 0, 0, 0, 0, 0}   
};
BOOL g_bViewKeys;
BOOL g_bShowIDW;

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

/**************************************************************************
*  IShellView_Constructor
*/
IShellView * IShellView_Constructor( IShellFolder * pFolder, LPCITEMIDLIST pidl)
{	IShellViewImpl * sv;
	sv=(IShellViewImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IShellViewImpl));
	sv->ref=1;
	sv->lpvtbl=&svvt;
	sv->lpvtblOleCommandTarget=&ctvt;
	sv->lpvtblDropTarget=&dtvt;
	sv->lpvtblViewObject=&vovt;

	sv->hMenu	= 0;
	sv->pSFParent	= pFolder;
	sv->uSelected = 0;
	sv->aSelectedItems = NULL;

	if(pFolder)
	  IShellFolder_AddRef(pFolder);

	TRACE(shell,"(%p)->(%p pidl=%p)\n",sv, pFolder, pidl);
	shell32_ObjCount++;
	return (IShellView *) sv;
}
/**************************************************************************
*  helperfunctions for communication with ICommDlgBrowser
*
*/
static BOOL IsInCommDlg(IShellViewImpl * This)
{	return(This->pCommDlgBrowser != NULL);
}
static HRESULT IncludeObject(IShellViewImpl * This, LPCITEMIDLIST pidl)
{
	if ( IsInCommDlg(This) )
	{ TRACE(shell,"ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
	  return (ICommDlgBrowser_IncludeObject(This->pCommDlgBrowser, (IShellView*)This, pidl));
	}
	return S_OK;
}

static HRESULT OnDefaultCommand(IShellViewImpl * This)
{
	if (IsInCommDlg(This))
	{ TRACE(shell,"ICommDlgBrowser::OnDefaultCommand\n");
	  return (ICommDlgBrowser_OnDefaultCommand(This->pCommDlgBrowser, (IShellView*)This));
	}
	return S_FALSE;
}

static HRESULT OnStateChange(IShellViewImpl * This, UINT uFlags)
{
	if (IsInCommDlg(This))
	{ TRACE(shell,"ICommDlgBrowser::OnStateChange flags=%x\n", uFlags);
	  return (ICommDlgBrowser_OnStateChange(This->pCommDlgBrowser, (IShellView*)This, uFlags));
	}
	return S_FALSE;
}
static void SetStyle(IShellViewImpl * This, DWORD dwAdd, DWORD dwRemove)
{	DWORD tmpstyle;

	TRACE(shell,"(%p)\n", This);

	tmpstyle = GetWindowLongA(This->hWndList, GWL_STYLE);
	SetWindowLongA(This->hWndList, GWL_STYLE, dwAdd | (tmpstyle & ~dwRemove));
}

static void CheckToolbar(IShellViewImpl * This)
{	LRESULT result;

	TRACE(shell,"\n");
	
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_SMALLICON, (This->FolderSettings.ViewMode==FVM_LIST)? TRUE : FALSE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_CHECKBUTTON,
		FCIDM_TB_REPORTVIEW, (This->FolderSettings.ViewMode==FVM_DETAILS)? TRUE : FALSE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_SMALLICON, TRUE, &result);
	IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON,
		FCIDM_TB_REPORTVIEW, TRUE, &result);
	TRACE(shell,"--\n");
}

static void MergeToolBar(IShellViewImpl * This)
{	LRESULT iStdBMOffset;
	LRESULT iViewBMOffset;
	TBADDBITMAP ab;
	TBBUTTON tbActual[6];
	int i;
	enum
	{ IN_STD_BMP = 0x4000,
	  IN_VIEW_BMP = 0x8000,
	} ;
	static const TBBUTTON c_tbDefault[] =
	{ { STD_COPY | IN_STD_BMP, FCIDM_SHVIEW_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, -1},
	  { 0,	0,	TBSTATE_ENABLED, TBSTYLE_SEP, {0,0}, 0, -1 },
	  { VIEW_LARGEICONS | IN_VIEW_BMP, FCIDM_SHVIEW_BIGICON,	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
	  { VIEW_SMALLICONS | IN_VIEW_BMP, FCIDM_SHVIEW_SMALLICON,	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
	  { VIEW_LIST       | IN_VIEW_BMP, FCIDM_SHVIEW_LISTVIEW,	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
	  { VIEW_DETAILS    | IN_VIEW_BMP, FCIDM_SHVIEW_REPORTVIEW,	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
	} ;

	TRACE(shell,"\n");

	ab.hInst = HINST_COMMCTRL;		/* hinstCommctrl */
	ab.nID   = IDB_STD_SMALL_COLOR;	/* std bitmaps */
	IShellBrowser_SendControlMsg(This->pShellBrowser,FCW_TOOLBAR,
				 TB_ADDBITMAP, 8, (LPARAM)&ab, &iStdBMOffset);

	TRACE(shell,"TB_ADDBITMAP returns %lx\n", iStdBMOffset);

	ab.nID   = IDB_VIEW_SMALL_COLOR;	/* std view bitmaps */
	IShellBrowser_SendControlMsg(This->pShellBrowser,FCW_TOOLBAR,
				 TB_ADDBITMAP, 8, (LPARAM)&ab, &iViewBMOffset);

	TRACE(shell,"TB_ADDBITMAP returns %lx\n", iViewBMOffset);

	for (i=0; i<6; ++i)
	{ tbActual[i] = c_tbDefault[i];
	  if (!(tbActual[i].fsStyle & TBSTYLE_SEP))
	  { if (tbActual[i].iBitmap & IN_VIEW_BMP)
	    { tbActual[i].iBitmap = (tbActual[i].iBitmap & ~IN_VIEW_BMP) + iViewBMOffset;
	    }
	    else if (tbActual[i].iBitmap & IN_STD_BMP)
	    { tbActual[i].iBitmap = (tbActual[i].iBitmap & ~IN_STD_BMP) + iStdBMOffset;
	    }
	  }
	}

	IShellBrowser_SetToolbarItems(This->pShellBrowser,tbActual, 6, FCT_MERGE);

	CheckToolbar(This);
	TRACE(shell,"--\n");

}

/**************************************************************************
* ShellView_CreateList()
*
*/

BOOL ShellView_CreateList (IShellViewImpl * This)
{	DWORD dwStyle;

	TRACE(shell,"%p\n",This);

	dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER |
		  LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_ALIGNLEFT;
	switch (This->FolderSettings.ViewMode)
	{ case FVM_ICON:	dwStyle |= LVS_ICON;		break;
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
/**************************************************************************
* ShellView_InitList()
*
* NOTES
*  internal
*/
int  nColumn1=120; /* width of column */
int  nColumn2=80;
int  nColumn3=170;
int  nColumn4=60;

BOOL ShellView_InitList(IShellViewImpl * This)
{	LVCOLUMNA lvColumn;
	CHAR        szString[50];

	TRACE(shell,"%p\n",This);

	ListView_DeleteAllItems(This->hWndList);

	/*initialize the columns */
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	/*  |  LVCF_SUBITEM;*/
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = szString;

	lvColumn.cx = nColumn1;
	strcpy(szString,"File");
	/*LoadStringA(shell32_hInstance, IDS_COLUMN1, szString, sizeof(szString));*/
	ListView_InsertColumnA(This->hWndList, 0, &lvColumn);

	lvColumn.cx = nColumn2;
	strcpy(szString,"Size");
	ListView_InsertColumnA(This->hWndList, 1, &lvColumn);

	lvColumn.cx = nColumn3;
	strcpy(szString,"Type");
	ListView_InsertColumnA(This->hWndList, 2, &lvColumn);

	lvColumn.cx = nColumn4;
	strcpy(szString,"Modified");
	ListView_InsertColumnA(This->hWndList, 3, &lvColumn);

	ListView_SetImageList(This->hWndList, ShellSmallIconList, LVSIL_SMALL);
	ListView_SetImageList(This->hWndList, ShellBigIconList, LVSIL_NORMAL);
  
	return TRUE;
}
/**************************************************************************
* ShellView_CompareItems() 
*
* NOTES
*  internal, CALLBACK for DSA_Sort
*/   
INT CALLBACK ShellView_CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
{	int ret;
	TRACE(shell,"pidl1=%p pidl2=%p lpsf=%p\n", lParam1, lParam2, (LPVOID) lpData);

	if(!lpData)
	  return 0;
	  
	ret =  (int)((LPSHELLFOLDER)lpData)->lpvtbl->fnCompareIDs((LPSHELLFOLDER)lpData, 0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2);  
	TRACE(shell,"ret=%i\n",ret);
	return ret;
}

/**************************************************************************
* ShellView_FillList()
*
* NOTES
*  internal
*/   

static HRESULT ShellView_FillList(IShellViewImpl * This)
{	LPENUMIDLIST	pEnumIDList;
	LPITEMIDLIST	pidl;
	DWORD		dwFetched;
	UINT		i;
	LVITEMA	lvItem;
	HRESULT		hRes;
	HDPA		hdpa;

	TRACE(shell,"%p\n",This);

	/* get the itemlist from the shfolder*/  
	hRes = IShellFolder_EnumObjects(This->pSFParent,This->hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
	if (hRes != S_OK)
	{ if (hRes==S_FALSE)
	    return(NOERROR);
	  return(hRes);
	}

	/* create a pointer array */	
	hdpa = pDPA_Create(16);
	if (!hdpa)
	{ return(E_OUTOFMEMORY);
	}

	/* copy the items into the array*/
	while((S_OK == IEnumIDList_Next(pEnumIDList,1, &pidl, &dwFetched)) && dwFetched)
	{ if (pDPA_InsertPtr(hdpa, 0x7fff, pidl) == -1)
	  { SHFree(pidl);
	  } 
	}

	/*sort the array*/
	pDPA_Sort(hdpa, ShellView_CompareItems, (LPARAM)This->pSFParent);

	/*turn the listview's redrawing off*/
	SendMessageA(This->hWndList, WM_SETREDRAW, FALSE, 0); 

	for (i=0; i < DPA_GetPtrCount(hdpa); ++i) 	/* DPA_GetPtrCount is a macro*/
	{ pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
	  if (IncludeObject(This, pidl) == S_OK)	/* in a commdlg This works as a filemask*/
	  { ZeroMemory(&lvItem, sizeof(lvItem));	/* create the listviewitem*/
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

/**************************************************************************
*  ShellView_OnCreate()
*
* NOTES
*  internal
*/   
LRESULT ShellView_OnCreate(IShellViewImpl * This)
{ TRACE(shell,"%p\n",This);

  if(ShellView_CreateList(This))
  {  if(ShellView_InitList(This))
     { ShellView_FillList(This);
     }
  }

  return S_OK;
}
/**************************************************************************
*  ShellView_OnSize()
*/   
LRESULT ShellView_OnSize(IShellViewImpl * This, WORD wWidth, WORD wHeight)
{	TRACE(shell,"%p width=%u height=%u\n",This, wWidth,wHeight);

	/*resize the ListView to fit our window*/
	if(This->hWndList)
	{ MoveWindow(This->hWndList, 0, 0, wWidth, wHeight, TRUE);
	}

	return S_OK;
}
/**************************************************************************
* ShellView_BuildFileMenu()
*/   
HMENU ShellView_BuildFileMenu(IShellViewImpl * This)
{	CHAR	szText[MAX_PATH];
	MENUITEMINFOA	mii;
	int	nTools,i;
	HMENU	hSubMenu;

	TRACE(shell,"(%p) semi-stub\n",This);

	hSubMenu = CreatePopupMenu();
	if(hSubMenu)
	{ /*get the number of items in our global array*/
	  for(nTools = 0; g_Tools[nTools].idCommand != -1; nTools++){}

	  /*add the menu items*/
	  for(i = 0; i < nTools; i++)
	  { strcpy(szText, "dummy BuildFileMenu");
      
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	    if(TBSTYLE_SEP != g_Tools[i].bStyle) /* no seperator*/
	    { mii.fType = MFT_STRING;
	      mii.fState = MFS_ENABLED;
	      mii.dwTypeData = szText;
	      mii.wID = g_Tools[i].idCommand;
	    }
	    else
	    { mii.fType = MFT_SEPARATOR;
	    }
	    /* tack This item onto the end of the menu */
	    InsertMenuItemA(hSubMenu, (UINT)-1, TRUE, &mii);
	  }
	}
	TRACE(shell,"-- return (menu=0x%x)\n",hSubMenu);
	return hSubMenu;
}
/**************************************************************************
* ShellView_MergeFileMenu()
*/   
void ShellView_MergeFileMenu(IShellViewImpl * This, HMENU hSubMenu)
{	TRACE(shell,"(%p)->(submenu=0x%08x) stub\n",This,hSubMenu);

	if(hSubMenu)
	{ /*insert This item at the beginning of the menu */
	  _InsertMenuItem(hSubMenu, 0, TRUE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);
	  _InsertMenuItem(hSubMenu, 0, TRUE, IDM_MYFILEITEM, MFT_STRING, "dummy45", MFS_ENABLED);

	}
	TRACE(shell,"--\n");	
}

/**************************************************************************
* ShellView_MergeViewMenu()
*/   

void ShellView_MergeViewMenu(IShellViewImpl * This, HMENU hSubMenu)
{	MENUITEMINFOA	mii;

	TRACE(shell,"(%p)->(submenu=0x%08x)\n",This,hSubMenu);

	if(hSubMenu)
	{ /*add a separator at the correct position in the menu*/
	  _InsertMenuItem(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, 0, MFT_SEPARATOR, NULL, MFS_ENABLED);

	  ZeroMemory(&mii, sizeof(mii));
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;;
	  mii.fType = MFT_STRING;
	  mii.dwTypeData = "View";
	  mii.hSubMenu = LoadMenuIndirectA(&_Resource_Men_MENU_001_0_data);
	  InsertMenuItemA(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
	}
}

/**************************************************************************
* ShellView_CanDoIDockingWindow()
*/   
BOOL ShellView_CanDoIDockingWindow(IShellViewImpl * This)
{	BOOL bReturn = FALSE;
	HRESULT hr;
	LPSERVICEPROVIDER pSP;
	LPDOCKINGWINDOWFRAME pFrame;
	
	FIXME(shell,"(%p) stub\n",This);
	
	/*get the browser's IServiceProvider*/
	hr = IShellBrowser_QueryInterface(This->pShellBrowser, (REFIID)&IID_IServiceProvider, (LPVOID*)&pSP);
	if(hr==S_OK)
	{ hr = IServiceProvider_QueryService(pSP, (REFGUID)&SID_SShellBrowser, (REFIID)&IID_IDockingWindowFrame, (LPVOID*)&pFrame);
	  if(SUCCEEDED(hr))
	  { bReturn = TRUE;
	    pFrame->lpvtbl->fnRelease(pFrame);
	  }
	  IServiceProvider_Release(pSP);
	}
	return bReturn;
}

/**************************************************************************
* ShellView_UpdateMenu()
*/
LRESULT ShellView_UpdateMenu(IShellViewImpl * This, HMENU hMenu)
{	TRACE(shell,"(%p)->(menu=0x%08x)\n",This,hMenu);
	CheckMenuItem(hMenu, IDM_VIEW_FILES, MF_BYCOMMAND | (g_bViewKeys ? MF_CHECKED: MF_UNCHECKED));

	if(ShellView_CanDoIDockingWindow(This))
	{ EnableMenuItem(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_ENABLED);
	  CheckMenuItem(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | (g_bShowIDW ? MF_CHECKED: MF_UNCHECKED));
	}
	else
	{ EnableMenuItem(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	  CheckMenuItem(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_UNCHECKED);
	}
	return S_OK;
}

/**************************************************************************
* ShellView_OnDeactivate()
*
* NOTES
*  internal
*/   
void ShellView_OnDeactivate(IShellViewImpl * This)
{ TRACE(shell,"%p\n",This);
  if(This->uState != SVUIA_DEACTIVATE)
  { if(This->hMenu)
    { IShellBrowser_SetMenuSB(This->pShellBrowser,0, 0, 0);
      IShellBrowser_RemoveMenusSB(This->pShellBrowser,This->hMenu);
      DestroyMenu(This->hMenu);
      This->hMenu = 0;
      }

   This->uState = SVUIA_DEACTIVATE;
   }
}

/**************************************************************************
* ShellView_OnActivate()
*/   
LRESULT ShellView_OnActivate(IShellViewImpl * This, UINT uState)
{	OLEMENUGROUPWIDTHS   omw = { {0, 0, 0, 0, 0, 0} };
	MENUITEMINFOA         mii;
	CHAR                szText[MAX_PATH];

	TRACE(shell,"%p uState=%x\n",This,uState);    

	/*don't do anything if the state isn't really changing */
	if(This->uState == uState)
	{ return S_OK;
	}

	ShellView_OnDeactivate(This);

	/*only do This if we are active */
	if(uState != SVUIA_DEACTIVATE)
	{ /*merge the menus */
	  This->hMenu = CreateMenu();
   
	  if(This->hMenu)
	  { IShellBrowser_InsertMenusSB(This->pShellBrowser, This->hMenu, &omw);
	    TRACE(shell,"-- after fnInsertMenusSB\n");    
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
	    { InsertMenuItemA(This->hMenu, FCIDM_MENU_HELP, FALSE, &mii);
	    }

	    /*get the view menu so we can merge with it*/
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU;
      
	    if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
	    { ShellView_MergeViewMenu(This, mii.hSubMenu);
	    }

	    /*add the items that should only be added if we have the focus*/
	    if(SVUIA_ACTIVATE_FOCUS == uState)
	    { /*get the file menu so we can merge with it */
	      ZeroMemory(&mii, sizeof(mii));
	      mii.cbSize = sizeof(mii);
	      mii.fMask = MIIM_SUBMENU;
      
	      if(GetMenuItemInfoA(This->hMenu, FCIDM_MENU_FILE, FALSE, &mii))
	      { ShellView_MergeFileMenu(This, mii.hSubMenu);
	      }
	    }
	  TRACE(shell,"-- before fnSetMenuSB\n");      
	  IShellBrowser_SetMenuSB(This->pShellBrowser, This->hMenu, 0, This->hWnd);
	  }
	}
	This->uState = uState;
	TRACE(shell,"--\n");    
	return S_OK;
}

/**************************************************************************
*  ShellView_OnSetFocus()
*
* NOTES
*  internal
*/
LRESULT ShellView_OnSetFocus(IShellViewImpl * This)
{	TRACE(shell,"%p\n",This);
	/* Tell the browser one of our windows has received the focus. This should always 
	be done before merging menus (OnActivate merges the menus) if one of our 
	windows has the focus.*/
	IShellBrowser_OnViewWindowActive(This->pShellBrowser,(IShellView*) This);
	ShellView_OnActivate(This, SVUIA_ACTIVATE_FOCUS);

	return 0;
}

/**************************************************************************
* ShellView_OnKillFocus()
*/   
LRESULT ShellView_OnKillFocus(IShellViewImpl * This)
{	TRACE(shell,"(%p) stub\n",This);
	ShellView_OnActivate(This, SVUIA_ACTIVATE_NOFOCUS);
	return 0;
}

/**************************************************************************
* ShellView_AddRemoveDockingWindow()
*/   
BOOL ShellView_AddRemoveDockingWindow(IShellViewImpl * This, BOOL bAdd)
{	BOOL	bReturn = FALSE;
	HRESULT	hr;
	LPSERVICEPROVIDER	pSP;
	LPDOCKINGWINDOWFRAME	pFrame;
	
	WARN(shell,"(%p)->(badd=0x%08x) semi-stub\n",This,bAdd);

	/* get the browser's IServiceProvider */
	hr = IShellBrowser_QueryInterface(This->pShellBrowser, (REFIID)&IID_IServiceProvider, (LPVOID*)&pSP);
	if(SUCCEEDED(hr))
	{ /*get the IDockingWindowFrame pointer*/
	  hr = IServiceProvider_QueryService(pSP, (REFGUID)&SID_SShellBrowser, (REFIID)&IID_IDockingWindowFrame, (LPVOID*)&pFrame);
	  if(SUCCEEDED(hr))
	  { if(bAdd)
	    { hr = S_OK;
	      FIXME(shell,"no docking implemented\n");
#if 0
	      if(!This->pDockingWindow)
	      { /* create the toolbar object */
	        This->pDockingWindow = DockingWindow_Constructor(This, This->hWnd);
	      }

	      if(This->pDockingWindow)
	      { /*add the toolbar object */
	        hr = pFrame->lpvtbl->fnAddToolbar(pFrame, (IDockingWindow*)This->pDockingWindow, TOOLBAR_ID, 0);

	        if(SUCCEEDED(hr))
	        { bReturn = TRUE;
	        }
	      }
#endif
	    }
	    else
	    { FIXME(shell,"no docking implemented\n");
#if 0
	      if(This->pDockingWindow)
	      { hr = pFrame->lpvtbl->fnRemoveToolbar(pFrame, (IDockingWindow*)This->pDockingWindow, DWFRF_NORMAL);

	        if(SUCCEEDED(hr))
	        { /* RemoveToolbar should release the toolbar object which will cause  */
	          /*it to destroy itself. Our toolbar object is no longer valid at  */
	          /*This point. */
            
	          This->pDockingWindow = NULL;
	          bReturn = TRUE;
	        }
	      }
#endif
	    }
	    pFrame->lpvtbl->fnRelease(pFrame);
	  }
	  IServiceProvider_Release(pSP);
	}
	return bReturn;
}

/**************************************************************************
*  ShellView_UpdateShellSettings()
*/
void ShellView_UpdateShellSettings(IShellViewImpl * This)
{	FIXME(shell,"(%p) stub\n",This);
	return ;
/*
	SHELLFLAGSTATE       sfs;
	HINSTANCE            hinstShell32;
*/
	/* Since SHGetSettings is not implemented in all versions of the shell, get the 
	function address manually at run time. This allows the code to run on all 
	platforms.*/
/*
	ZeroMemory(&sfs, sizeof(sfs));
*/
	/* The default, in case any of the following steps fails, is classic Windows 95 
	style.*/
/*
	sfs.fWin95Classic = TRUE;

	hinstShell32 = LoadLibrary("shell32.dll");
	if(hinstShell32)
	{ PFNSHGETSETTINGSPROC pfnSHGetSettings;

      pfnSHGetSettings = (PFNSHGETSETTINGSPROC)GetProcAddress(hinstShell32, "SHGetSettings");
	  if(pfnSHGetSettings)
      { (*pfnSHGetSettings)(&sfs, SSF_DOUBLECLICKINWEBVIEW | SSF_WIN95CLASSIC);
      }
	  FreeLibrary(hinstShell32);
	}

	DWORD dwExStyles = 0;

	if(!sfs.fWin95Classic && !sfs.fDoubleClickInWebView)
	  dwExStyles |= LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT;

	ListView_SetExtendedListViewStyle(This->hWndList, dwExStyles);
*/
}

/**************************************************************************
*   ShellView_OnSettingChange()
*/   
LRESULT ShellView_OnSettingChange(IShellViewImpl * This, LPCSTR lpszSection)
{	TRACE(shell,"(%p) stub\n",This);
/*if(0 == lstrcmpi(lpszSection, "ShellState"))*/
	{ ShellView_UpdateShellSettings(This);
	  return 0;
	}
	return 0;
}
/**************************************************************************
* ShellView_OnCommand()
*/   
LRESULT ShellView_OnCommand(IShellViewImpl * This,DWORD dwCmdID, DWORD dwCmd, HWND hwndCmd)
{	TRACE(shell,"(%p)->(0x%08lx 0x%08lx 0x%08x) stub\n",This, dwCmdID, dwCmd, hwndCmd);
	switch(dwCmdID)
	{ case IDM_VIEW_FILES:
	    g_bViewKeys = ! g_bViewKeys;
	    IShellView_Refresh((IShellView*) This);
	    break;

	  case IDM_VIEW_IDW:
	    g_bShowIDW = ! g_bShowIDW;
	    ShellView_AddRemoveDockingWindow(This, g_bShowIDW);
	    break;
   
	  case IDM_MYFILEITEM:
	    MessageBeep(MB_OK);
	    break;

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
	    TRACE(shell,"-- COMMAND 0x%04lx unhandled\n", dwCmdID);
	}
	return 0;
}

/**************************************************************************
*   ShellView_GetSelections()
*
* RETURNS
*  number of selected items
*/   
UINT ShellView_GetSelections(IShellViewImpl * This)
{	LVITEMA	lvItem;
	UINT	i;


	if (This->aSelectedItems)
	{ SHFree(This->aSelectedItems);
	}

	This->uSelected = ListView_GetSelectedCount(This->hWndList);
	This->aSelectedItems = (LPITEMIDLIST*)SHAlloc(This->uSelected * sizeof(LPITEMIDLIST));

	TRACE(shell,"selected=%i\n", This->uSelected);
	
	if(This->aSelectedItems)
	{ TRACE(shell,"-- Items selected =%u\n", This->uSelected);
	  ZeroMemory(&lvItem, sizeof(lvItem));
	  lvItem.mask = LVIF_STATE | LVIF_PARAM;
	  lvItem.stateMask = LVIS_SELECTED;
	  lvItem.iItem = 0;

	  i = 0;
   
	  while(ListView_GetItemA(This->hWndList, &lvItem) && (i < This->uSelected))
	  { if(lvItem.state & LVIS_SELECTED)
	    { This->aSelectedItems[i] = (LPITEMIDLIST)lvItem.lParam;
	      i++;
	      TRACE(shell,"-- selected Item found\n");
	    }
	    lvItem.iItem++;
	  }
	}
	return This->uSelected;

}
/**************************************************************************
*   ShellView_DoContextMenu()
*/   
void ShellView_DoContextMenu(IShellViewImpl * This, WORD x, WORD y, BOOL fDefault)
{	UINT	uCommand;
	DWORD	wFlags;
	HMENU hMenu;
	BOOL  fExplore = FALSE;
	HWND  hwndTree = 0;
	INT          	nMenuIndex;
	MENUITEMINFOA	mii;
	LPCONTEXTMENU	pContextMenu = NULL;
	CMINVOKECOMMANDINFO  cmi;
	
	TRACE(shell,"(%p)->(0x%08x 0x%08x 0x%08x) stub\n",This, x, y, fDefault);

	/* look, what's selected and create a context menu object of it*/
	if(ShellView_GetSelections(This))
	{ This->pSFParent->lpvtbl->fnGetUIObjectOf( This->pSFParent, This->hWndParent, This->uSelected,
						    This->aSelectedItems, (REFIID)&IID_IContextMenu,
						    NULL, (LPVOID *)&pContextMenu);
   
	  if(pContextMenu)
	  { TRACE(shell,"-- pContextMenu\n");
	    hMenu = CreatePopupMenu();

	    if( hMenu )
	    { /* See if we are in Explore or Open mode. If the browser's tree
	         is present, then we are in Explore mode.*/
        
	      if(SUCCEEDED(IShellBrowser_GetControlWindow(This->pShellBrowser,FCW_TREE, &hwndTree)) && hwndTree)
	      { TRACE(shell,"-- explore mode\n");
	        fExplore = TRUE;
	      }

	      wFlags = CMF_NORMAL | (This->uSelected != 1 ? 0 : CMF_CANRENAME) | (fExplore ? CMF_EXPLORE : 0);

	      if (SUCCEEDED(pContextMenu->lpvtbl->fnQueryContextMenu( pContextMenu, hMenu, 0, MENU_OFFSET, MENU_MAX, wFlags )))
	      { if( fDefault )
	        { TRACE(shell,"-- get menu default command\n");

	          uCommand = nMenuIndex = 0;
       	          ZeroMemory(&mii, sizeof(mii));
	          mii.cbSize = sizeof(mii);
	          mii.fMask = MIIM_STATE | MIIM_ID;

	          while(GetMenuItemInfoA(hMenu, nMenuIndex, TRUE, &mii))	/*find the default item in the menu*/
	          { if(mii.fState & MFS_DEFAULT)
	            { uCommand = mii.wID;
	              break;
	            }
	            nMenuIndex++;
	          }
	        }
	        else
	        { TRACE(shell,"-- track popup\n");
	          uCommand = TrackPopupMenu( hMenu,TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
	        }		
         
	        if(uCommand > 0)
	        { TRACE(shell,"-- uCommand=%u\n", uCommand);
	          if (IsInCommDlg(This) && (((uCommand-MENU_OFFSET)==IDM_EXPLORE) || ((uCommand-MENU_OFFSET)==IDM_OPEN)))
		  { TRACE(shell,"-- dlg: OnDefaultCommand\n");
		    OnDefaultCommand(This);
		  }
		  else
		  { TRACE(shell,"-- explore -- invoke command\n");
		      ZeroMemory(&cmi, sizeof(cmi));
	              cmi.cbSize = sizeof(cmi);
	              cmi.hwnd = This->hWndParent;
	              cmi.lpVerb = (LPCSTR)MAKEINTRESOURCEA(uCommand - MENU_OFFSET);
		      pContextMenu->lpvtbl->fnInvokeCommand(pContextMenu, &cmi);
		  }
	        }
	        DestroyMenu(hMenu);
	      }
	    }
	    if (pContextMenu)
	      pContextMenu->lpvtbl->fnRelease(pContextMenu);
	  }
	}
	else	/* background context menu */
	{ hMenu = LoadMenuIndirectA(&_Resource_Men_MENU_002_0_data);
	  uCommand = TrackPopupMenu( GetSubMenu(hMenu,0),TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,This->hWnd,NULL);
	  ShellView_OnCommand(This, uCommand, 0,0);
	  DestroyMenu(hMenu);
	}
}

/**************************************************************************
* ShellView_OnNotify()
*/
   
LRESULT ShellView_OnNotify(IShellViewImpl * This, UINT CtlID, LPNMHDR lpnmh)
{	NM_LISTVIEW *lpnmlv = (NM_LISTVIEW*)lpnmh;
	NMLVDISPINFOA *lpdi = (NMLVDISPINFOA *)lpnmh;
	LPITEMIDLIST pidl;
	DWORD dwCursor; 
	STRRET   str;  

	TRACE(shell,"%p CtlID=%u lpnmh->code=%x\n",This,CtlID,lpnmh->code);

	switch(lpnmh->code)
	{ case NM_SETFOCUS:
	    TRACE(shell,"-- NM_SETFOCUS %p\n",This);
	    ShellView_OnSetFocus(This);
	    break;

	  case NM_KILLFOCUS:
	    TRACE(shell,"-- NM_KILLFOCUS %p\n",This);
	    ShellView_OnDeactivate(This);
	    break;

	  case HDN_ENDTRACKA:
	    TRACE(shell,"-- HDN_ENDTRACKA %p\n",This);
	    /*nColumn1 = ListView_GetColumnWidth(This->hWndList, 0);
	    nColumn2 = ListView_GetColumnWidth(This->hWndList, 1);*/
	    break;

	  case LVN_DELETEITEM:
	    TRACE(shell,"-- LVN_DELETEITEM %p\n",This);
	    SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
	    break;

	  case LVN_ITEMACTIVATE:
	    TRACE(shell,"-- LVN_ITEMACTIVATE %p\n",This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    ShellView_DoContextMenu(This, 0, 0, TRUE);
	    break;

	  case NM_RCLICK:
	    TRACE(shell,"-- NM_RCLICK %p\n",This);
	    dwCursor = GetMessagePos();
	    ShellView_DoContextMenu(This, LOWORD(dwCursor), HIWORD(dwCursor), FALSE);
	    break;

	  case LVN_GETDISPINFOA:
	    TRACE(shell,"-- LVN_GETDISPINFOA %p\n",This);
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
			{ if (!( HCR_MapTypeToValue(sTemp, sTemp, 64)
			         && HCR_MapTypeToValue(sTemp, lpdi->item.pszText, lpdi->item.cchTextMax )))
			  { strncpy (lpdi->item.pszText, sTemp, lpdi->item.cchTextMax);
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
		      strncpy (lpdi->item.pszText, "Folder", lpdi->item.cchTextMax);
		      break;
		    case 3:  
		      _ILGetFileDate (pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
		      break;
		  }
	        }
	        TRACE(shell,"-- text=%s\n",lpdi->item.pszText);		
	      }
	    }
	    else	   /*the item text is being requested*/
	    { if(lpdi->item.mask & LVIF_TEXT)	   /*is the text being requested?*/
	      { if(SUCCEEDED(IShellFolder_GetDisplayNameOf(This->pSFParent,pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str)))
	        { if(STRRET_WSTR == str.uType)
	          { WideCharToLocal(lpdi->item.pszText, str.u.pOleStr, lpdi->item.cchTextMax);
	            SHFree(str.u.pOleStr);
	          }
	          else if(STRRET_CSTRA == str.uType)
	          { strncpy(lpdi->item.pszText, str.u.cStr, lpdi->item.cchTextMax);
	          }
		  else
		  { FIXME(shell,"type wrong\n");
		  }
	        }
	        TRACE(shell,"-- text=%s\n",lpdi->item.pszText);
	      }

	      if(lpdi->item.mask & LVIF_IMAGE) 		/*is the image being requested?*/
	      { lpdi->item.iImage = SHMapPIDLToSystemImageListIndex(This->pSFParent, pidl, 0);
	      }
	    }
	    break;

	  case LVN_ITEMCHANGED:
	    TRACE(shell,"-- LVN_ITEMCHANGED %p\n",This);
	    ShellView_GetSelections(This);
	    OnStateChange(This, CDBOSC_SELCHANGE);  /* the browser will get the IDataObject now */
	    break;

	  default:
	    TRACE (shell,"-- %p WM_COMMAND %s unhandled\n", This, SPY_GetMsgName(lpnmh->code));
	    break;;
	}
	return 0;
}

/**************************************************************************
*  ShellView_WndProc
*/

LRESULT CALLBACK ShellView_WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{ IShellViewImpl * pThis = (IShellViewImpl*)GetWindowLongA(hWnd, GWL_USERDATA);
  LPCREATESTRUCTA lpcs;
  DWORD dwCursor;
  
  TRACE(shell,"(hwnd=%x msg=%x wparm=%x lparm=%lx)\n",hWnd, uMessage, wParam, lParam);
    
  switch (uMessage)
  { case WM_NCCREATE:
      { TRACE(shell,"-- WM_NCCREATE\n");
        lpcs = (LPCREATESTRUCTA)lParam;
        pThis = (IShellViewImpl*)(lpcs->lpCreateParams);
        SetWindowLongA(hWnd, GWL_USERDATA, (LONG)pThis);
        pThis->hWnd = hWnd;        /*set the window handle*/
      }
      break;
   
   case WM_SIZE:
      TRACE(shell,"-- WM_SIZE\n");
      return ShellView_OnSize(pThis,LOWORD(lParam), HIWORD(lParam));
   
   case WM_SETFOCUS:
      TRACE(shell,"-- WM_SETFOCUS\n");   
      return ShellView_OnSetFocus(pThis);
 
   case WM_KILLFOCUS:
      TRACE(shell,"-- WM_KILLFOCUS\n");
  	  return ShellView_OnKillFocus(pThis);

   case WM_CREATE:
      TRACE(shell,"-- WM_CREATE\n");
      return ShellView_OnCreate(pThis);

   case WM_SHOWWINDOW:
      TRACE(shell,"-- WM_SHOWWINDOW\n");
      UpdateWindow(pThis->hWndList);
      break;

   case WM_ACTIVATE:
      TRACE(shell,"-- WM_ACTIVATE\n");
      return ShellView_OnActivate(pThis, SVUIA_ACTIVATE_FOCUS);
   
   case WM_COMMAND:
      TRACE(shell,"-- WM_COMMAND\n");
      return ShellView_OnCommand(pThis, GET_WM_COMMAND_ID(wParam, lParam), 
                                  GET_WM_COMMAND_CMD(wParam, lParam), 
                                  GET_WM_COMMAND_HWND(wParam, lParam));
   
   case WM_INITMENUPOPUP:
      TRACE(shell,"-- WM_INITMENUPOPUP\n");
      return ShellView_UpdateMenu(pThis, (HMENU)wParam);
   
   case WM_NOTIFY:
      TRACE(shell,"-- WM_NOTIFY\n");
      return ShellView_OnNotify(pThis,(UINT)wParam, (LPNMHDR)lParam);

   case WM_SETTINGCHANGE:
      TRACE(shell,"-- WM_SETTINGCHANGE\n");
      return ShellView_OnSettingChange(pThis,(LPCSTR)lParam);

   case WM_PARENTNOTIFY:
      TRACE(shell,"-- WM_PARENTNOTIFY\n");
      if ( LOWORD(wParam) == WM_RBUTTONDOWN ) /* fixme: should not be handled here*/
      { dwCursor = GetMessagePos();
	ShellView_DoContextMenu(pThis, LOWORD(dwCursor), HIWORD(dwCursor), FALSE);
	return TRUE;
      }
      break;

   default:
      TRACE(shell,"-- message %s unhandled\n", SPY_GetMsgName(uMessage));
      break;
  }
  return DefWindowProcA (hWnd, uMessage, wParam, lParam);
}
/**************************************************************************
*
*
*  The INTERFACE of the IShellView object
*
*
***************************************************************************
*  IShellView_QueryInterface
*/
static HRESULT WINAPI IShellView_fnQueryInterface(IShellView * iface,REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IShellViewImpl, iface);

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellView))  /*IShellView*/
	{ *ppvObj = (IShellView*)This;
	}
	else if(IsEqualIID(riid, &IID_IOleCommandTarget))  /*IOleCommandTarget*/
	{    *ppvObj = (IOleCommandTarget*)&(This->lpvtblOleCommandTarget);
	}   
	else if(IsEqualIID(riid, &IID_IDropTarget))  /*IDropTarget*/
	{    *ppvObj = (IDropTarget*)&(This->lpvtblDropTarget);
	}   
	else if(IsEqualIID(riid, &IID_IViewObject))  /*ViewObject*/
	{    *ppvObj = (IViewObject*)&(This->lpvtblViewObject);
	}   

	if(*ppvObj)
	{ IShellView_AddRef( (IShellView*) *ppvObj);
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IShellView::AddRef
*/
static ULONG WINAPI IShellView_fnAddRef(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/**************************************************************************
*  IShellView_Release
*/
static ULONG WINAPI IShellView_fnRelease(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)->()\n",This);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell," destroying IShellView(%p)\n",This);

	  if(This->pSFParent)
	    IShellFolder_Release(This->pSFParent);

	  if (This->aSelectedItems)
	    SHFree(This->aSelectedItems);

	  if (This->pCommDlgBrowser)
	    ICommDlgBrowser_Release(This->pCommDlgBrowser);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/**************************************************************************
*  ShellView_GetWindow
*/
static HRESULT WINAPI IShellView_fnGetWindow(IShellView * iface,HWND * phWnd)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)\n",This);

	*phWnd = This->hWnd;

	return S_OK;
}

static HRESULT WINAPI IShellView_fnContextSensitiveHelp(IShellView * iface,BOOL fEnterMode)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME(shell,"(%p) stub\n",This);

	return E_NOTIMPL;
}
/**************************************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
static HRESULT WINAPI IShellView_fnTranslateAccelerator(IShellView * iface,LPMSG lpmsg)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME(shell,"(%p)->(%p: hwnd=%x msg=%x lp=%lx wp=%x) stub\n",This,lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);

	
	switch (lpmsg->message)
	{ case WM_KEYDOWN: 	TRACE(shell,"-- key=0x04%x",lpmsg->wParam) ;
	}
	return S_FALSE;
}

static HRESULT WINAPI IShellView_fnEnableModeless(IShellView * iface,BOOL fEnable)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME(shell,"(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnUIActivate(IShellView * iface,UINT uState)
{
	ICOM_THIS(IShellViewImpl, iface);

	CHAR	szName[MAX_PATH];
	LRESULT	lResult;
	int	nPartArray[1] = {-1};

	TRACE(shell,"(%p)->(state=%x) stub\n",This, uState);
	/*don't do anything if the state isn't really changing*/
	if(This->uState == uState)
	{ return S_OK;
	}

	/*OnActivate handles the menu merging and internal state*/
	ShellView_OnActivate(This, uState);

	/*remove the docking window*/
	if(g_bShowIDW)
	{ ShellView_AddRemoveDockingWindow(This, FALSE);
	}

	/*only do This if we are active*/
	if(uState != SVUIA_DEACTIVATE)
	{ /*update the status bar */
	   strcpy(szName, "dummy32");

	  IShellFolder_GetFolderPath( This->pSFParent, szName + strlen(szName), sizeof(szName) - strlen(szName));

	  /* set the number of parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETPARTS, 1,
	  						(LPARAM)nPartArray, &lResult);

	  /* set the text for the parts */
	  IShellBrowser_SendControlMsg(This->pShellBrowser, FCW_STATUS, SB_SETTEXTA,
							0, (LPARAM)szName, &lResult);

	  /*add the docking window if necessary */
	  if(g_bShowIDW)
	  { ShellView_AddRemoveDockingWindow(This, TRUE);
	  }
	}
	return S_OK;
}
static HRESULT WINAPI IShellView_fnRefresh(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)\n",This);

	ListView_DeleteAllItems(This->hWndList);
	ShellView_FillList(This);

	return S_OK;
}
static HRESULT WINAPI IShellView_fnCreateViewWindow(IShellView * iface, IShellView *lpPrevView,
                     LPCFOLDERSETTINGS lpfs, IShellBrowser * psb, RECT * prcView, HWND  *phWnd)
{
	ICOM_THIS(IShellViewImpl, iface);

	WNDCLASSA wc;
	*phWnd = 0;

	
	TRACE(shell,"(%p)->(shlview=%p set=%p shlbrs=%p rec=%p hwnd=%p) incomplete\n",This, lpPrevView,lpfs, psb, prcView, phWnd);
	TRACE(shell,"-- vmode=%x flags=%x left=%i top=%i right=%i bottom=%i\n",lpfs->ViewMode, lpfs->fFlags ,prcView->left,prcView->top, prcView->right, prcView->bottom);

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
	{ TRACE(shell,"-- CommDlgBrowser\n");
	}
	   
	/*if our window class has not been registered, then do so*/
	if(!GetClassInfoA(shell32_hInstance, SV_CLASS_NAME, &wc))
	{ ZeroMemory(&wc, sizeof(wc));
	  wc.style          = CS_HREDRAW | CS_VREDRAW;
	  wc.lpfnWndProc    = (WNDPROC) ShellView_WndProc;
	  wc.cbClsExtra     = 0;
	  wc.cbWndExtra     = 0;
	  wc.hInstance      = shell32_hInstance;
	  wc.hIcon          = 0;
	  wc.hCursor        = LoadCursorA (0, IDC_ARROWA);
	  wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
	  wc.lpszMenuName   = NULL;
	  wc.lpszClassName  = SV_CLASS_NAME;

	  if(!RegisterClassA(&wc))
	    return E_FAIL;
	}

	*phWnd = CreateWindowExA(0, SV_CLASS_NAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
	                   prcView->left, prcView->top, prcView->right - prcView->left, prcView->bottom - prcView->top,
	                   This->hWndParent, 0, shell32_hInstance, (LPVOID)This);
	                   
	MergeToolBar(This);
	
	if(!*phWnd)
	  return E_FAIL;

	return S_OK;
}

static HRESULT WINAPI IShellView_fnDestroyViewWindow(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)\n",This);

	/*Make absolutely sure all our UI is cleaned up.*/
	IShellView_UIActivate((IShellView*)This, SVUIA_DEACTIVATE);
	if(This->hMenu)
	{ DestroyMenu(This->hMenu);
	}
	DestroyWindow(This->hWnd);
	IShellBrowser_Release(This->pShellBrowser);
	return S_OK;
}

static HRESULT WINAPI IShellView_fnGetCurrentInfo(IShellView * iface, LPFOLDERSETTINGS lpfs)
{
	ICOM_THIS(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(%p) vmode=%x flags=%x\n",This, lpfs, 
		This->FolderSettings.ViewMode, This->FolderSettings.fFlags);

	if (lpfs)
	{ *lpfs = This->FolderSettings;
	  return NOERROR;
	}
	else
	  return E_INVALIDARG;
}

static HRESULT WINAPI IShellView_fnAddPropertySheetPages(IShellView * iface, DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME(shell,"(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnSaveViewState(IShellView * iface)
{
	ICOM_THIS(IShellViewImpl, iface);

	FIXME(shell,"(%p) stub\n",This);

	return S_OK;
}

static HRESULT WINAPI IShellView_fnSelectItem(IShellView * iface, LPCITEMIDLIST pidlItem, UINT uFlags)
{	ICOM_THIS(IShellViewImpl, iface);
	
	FIXME(shell,"(%p)->(pidl=%p, 0x%08x) stub\n",This, pidlItem, uFlags);

	return E_NOTIMPL;
}

static HRESULT WINAPI IShellView_fnGetItemObject(IShellView * iface, UINT uItem, REFIID riid, LPVOID *ppvOut)
{
	ICOM_THIS(IShellViewImpl, iface);

	LPUNKNOWN	pObj = NULL; 
	char    xriid[50];
	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(uItem=0x%08x,\n\tIID=%s, ppv=%p)\n",This, uItem, xriid, ppvOut);

	*ppvOut = NULL;
	if(IsEqualIID(riid, &IID_IContextMenu))
	{ ShellView_GetSelections(This);
	  pObj =(LPUNKNOWN)IContextMenu_Constructor(This->pSFParent,This->aSelectedItems,This->uSelected);	
	}
	else if (IsEqualIID(riid, &IID_IDataObject))
	{ ShellView_GetSelections(This);
	  pObj =(LPUNKNOWN)IDataObject_Constructor(This->hWndParent, This->pSFParent,This->aSelectedItems,This->uSelected);
	}

	TRACE(shell,"-- (%p)->(interface=%p)\n",This, ppvOut);

	if(!pObj)
	  return E_OUTOFMEMORY;
	*ppvOut = pObj;
	return S_OK;
}

static struct ICOM_VTABLE(IShellView) svvt = 
{	IShellView_fnQueryInterface,
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


/************************************************************************
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

/************************************************************************
 * ISVOleCmdTarget_AddRef (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_AddRef(
	IOleCommandTarget *	iface)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellFolder, iface);

	return IShellFolder_AddRef((IShellFolder*)This);
}

/************************************************************************
 * ISVOleCmdTarget_Release (IUnknown)
 */
static ULONG WINAPI ISVOleCmdTarget_Release(
	IOleCommandTarget *	iface)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	return IShellFolder_Release((IShellFolder*)This);
}

/************************************************************************
 * ISVOleCmdTarget_Exec (IOleCommandTarget)
 */
static HRESULT WINAPI ISVOleCmdTarget_QueryStatus(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	ULONG cCmds, 
	OLECMD * prgCmds,
	OLECMDTEXT* pCmdText)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	FIXME(shell, "(%p)->(%p 0x%08lx %p %p\n", This, pguidCmdGroup, cCmds, prgCmds, pCmdText);
	return E_NOTIMPL;
}

/************************************************************************
 * ISVOleCmdTarget_Exec (IOleCommandTarget)
 */
static HRESULT WINAPI ISVOleCmdTarget_Exec(
	IOleCommandTarget *iface,
	const GUID* pguidCmdGroup,
	DWORD nCmdID,
	DWORD nCmdexecopt,
	VARIANT* pvaIn,
	VARIANT* pvaOut)
{
	_ICOM_THIS_From_IOleCommandTarget(IShellViewImpl, iface);

	FIXME(shell, "(%p)->(%p 0x%08lx 0x%08lx %p %p)\n", This, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IOleCommandTarget) ctvt = 
{
	ISVOleCmdTarget_QueryInterface,
	ISVOleCmdTarget_AddRef,
	ISVOleCmdTarget_Release,
	ISVOleCmdTarget_QueryStatus,
	ISVOleCmdTarget_Exec
};

/****************************************************************************
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

	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVDropTarget_AddRef( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVDropTarget_Release( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

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

	FIXME(shell, "Stub: This=%p, DataObject=%p\n",This,pDataObject);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISVDropTarget_DragOver(
	IDropTarget	*iface,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISVDropTarget_DragLeave(
	IDropTarget	*iface)
{
	_ICOM_THIS_From_IDropTarget(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

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

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IDropTarget) dtvt = 
{
	ISVDropTarget_QueryInterface,
	ISVDropTarget_AddRef,
	ISVDropTarget_Release,
	ISVDropTarget_DragEnter,
	ISVDropTarget_DragOver,
	ISVDropTarget_DragLeave,
	ISVDropTarget_Drop
};

/****************************************************************************
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

	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	return IShellFolder_QueryInterface((IShellFolder*)This, riid, ppvObj);
}

static ULONG WINAPI ISVViewObject_AddRef( IViewObject *iface)
{
	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	return IShellFolder_AddRef((IShellFolder*)This);
}

static ULONG WINAPI ISVViewObject_Release( IViewObject *iface)
{
	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

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
	pfnContinue pfnContinue,
	DWORD dwContinue)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

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

	FIXME(shell, "Stub: This=%p\n",This);

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

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_Unfreeze(
	IViewObject 	*iface,
	DWORD dwFreeze)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_SetAdvise(
	IViewObject 	*iface,
	DWORD aspects,
	DWORD advf,
	IAdviseSink* pAdvSink)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISVViewObject_GetAdvise(
	IViewObject 	*iface,
	DWORD* pAspects,
	DWORD* pAdvf,
	IAdviseSink** ppAdvSink)
{	

	_ICOM_THIS_From_IViewObject(IShellViewImpl, iface);

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}


static struct ICOM_VTABLE(IViewObject) vovt = 
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

