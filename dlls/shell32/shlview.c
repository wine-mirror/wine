/*
 *	ShellView
 *
 *	Copyright 1998	<juergen.schmied@metronet.de>
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "winnls.h"
#include "winproc.h"
#include "commctrl.h"

#include "shell32_main.h"
#include "pidl.h"
#include "shresdef.h"

/***********************************************************************
*   IShellView implementation
*/
static HRESULT WINAPI IShellView_QueryInterface(LPSHELLVIEW,REFIID, LPVOID *);
static ULONG WINAPI IShellView_AddRef(LPSHELLVIEW) ;
static ULONG WINAPI IShellView_Release(LPSHELLVIEW);
    /* IOleWindow methods */
static HRESULT WINAPI IShellView_GetWindow(LPSHELLVIEW,HWND32 * lphwnd);
static HRESULT WINAPI IShellView_ContextSensitiveHelp(LPSHELLVIEW,BOOL32 fEnterMode);
    /* IShellView methods */
static HRESULT WINAPI IShellView_TranslateAccelerator(LPSHELLVIEW,LPMSG32 lpmsg);
static HRESULT WINAPI IShellView_EnableModeless(LPSHELLVIEW,BOOL32 fEnable);
static HRESULT WINAPI IShellView_UIActivate(LPSHELLVIEW,UINT32 uState);
static HRESULT WINAPI IShellView_Refresh(LPSHELLVIEW);
static HRESULT WINAPI IShellView_CreateViewWindow(LPSHELLVIEW, IShellView *lpPrevView,LPCFOLDERSETTINGS lpfs, IShellBrowser * psb,RECT32 * prcView, HWND32  *phWnd);
static HRESULT WINAPI IShellView_DestroyViewWindow(LPSHELLVIEW);
static HRESULT WINAPI IShellView_GetCurrentInfo(LPSHELLVIEW, LPFOLDERSETTINGS lpfs);
static HRESULT WINAPI IShellView_AddPropertySheetPages(LPSHELLVIEW, DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
static HRESULT WINAPI IShellView_SaveViewState(LPSHELLVIEW);
static HRESULT WINAPI IShellView_SelectItem(LPSHELLVIEW, LPCITEMIDLIST pidlItem, UINT32 uFlags);
static HRESULT WINAPI IShellView_GetItemObject(LPSHELLVIEW, UINT32 uItem, REFIID riid,LPVOID *ppv);

static BOOL32 ShellView_CanDoIDockingWindow(LPSHELLVIEW);

static struct IShellView_VTable svvt = 
{ IShellView_QueryInterface,
  IShellView_AddRef,
  IShellView_Release,
  IShellView_GetWindow,
  IShellView_ContextSensitiveHelp,
  IShellView_TranslateAccelerator,
  IShellView_EnableModeless,
  IShellView_UIActivate,
  IShellView_Refresh,
  IShellView_CreateViewWindow,
  IShellView_DestroyViewWindow,
  IShellView_GetCurrentInfo,
  IShellView_AddPropertySheetPages,
  IShellView_SaveViewState,
  IShellView_SelectItem,
  IShellView_GetItemObject
};

//menu items
#define IDM_VIEW_FILES  (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_VIEW_IDW    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_MYFILEITEM  (FCIDM_SHVIEWFIRST + 0x502)

#define ID_LISTVIEW     2000

#define MENU_OFFSET  1
#define MENU_MAX     100

#define TOOLBAR_ID   (L"SHELLDLL_DefView")
//windowsx.h
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND32)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
// winuser.h
#define WM_SETTINGCHANGE                WM_WININICHANGE

typedef struct
{  int   idCommand;
   int   iImage;
   int   idButtonString;
   int   idMenuString;
   int   nStringOffset;
   BYTE  bState;
   BYTE  bStyle;
} MYTOOLINFO, *LPMYTOOLINFO;

MYTOOLINFO g_Tools[] = 
{ {IDM_VIEW_FILES, 0, IDS_TB_VIEW_FILES, IDS_MI_VIEW_FILES, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {-1, 0, 0, 0, 0, 0, 0}   
};
BOOL32 g_bViewKeys;
BOOL32 g_bShowIDW;

typedef void (CALLBACK *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

/**************************************************************************
*  IShellView_Constructor
*/
LPSHELLVIEW IShellView_Constructor( LPSHELLFOLDER pFolder, LPCITEMIDLIST pidl)
{	LPSHELLVIEW sv;
	sv=(LPSHELLVIEW)HeapAlloc(GetProcessHeap(),0,sizeof(IShellView));
	sv->ref=1;
	sv->lpvtbl=&svvt;
  
	sv->mpidl	= ILClone(pidl);
	sv->hMenu	=0;
	sv->pSFParent	= pFolder;
	sv->uSelected = 0;
	sv->aSelectedItems = NULL;

	if(sv->pSFParent)
	  sv->pSFParent->lpvtbl->fnAddRef(sv->pSFParent);

	TRACE(shell,"(%p)->(%p pidl=%p)\n",sv, pFolder, pidl);
	return sv;
}
/**************************************************************************
*  helperfunctions for communication with ICommDlgBrowser
*
*/
static BOOL32 IsInCommDlg(LPSHELLVIEW this)
{	return(this->pCommDlgBrowser != NULL);
}
static HRESULT IncludeObject(LPSHELLVIEW this, LPCITEMIDLIST pidl)
{	if ( IsInCommDlg(this) )
	{ TRACE(shell,"ICommDlgBrowser::IncludeObject pidl=%p\n", pidl);
	  return (this->pCommDlgBrowser->lpvtbl->fnIncludeObject(this->pCommDlgBrowser, this, pidl));
	}
	return S_OK;
}
static HRESULT OnDefaultCommand(LPSHELLVIEW this)
{	if (IsInCommDlg(this))
	{ TRACE(shell,"ICommDlgBrowser::OnDefaultCommand\n");
	  return (this->pCommDlgBrowser->lpvtbl->fnOnDefaultCommand(this->pCommDlgBrowser, this));
	}
	return S_FALSE;
}
static HRESULT OnStateChange(LPSHELLVIEW this, UINT32 uFlags)
{	if (IsInCommDlg(this))
	{ TRACE(shell,"ICommDlgBrowser::OnStateChange flags=%x\n", uFlags);
	  return (this->pCommDlgBrowser->lpvtbl->fnOnStateChange(this->pCommDlgBrowser, this, uFlags));
	}
	return S_FALSE;
}

/**************************************************************************
* ShellView_CreateList()
*
*/

BOOL32 ShellView_CreateList (LPSHELLVIEW this)
{	DWORD dwStyle;

	TRACE(shell,"%p\n",this);

	dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_SHAREIMAGELISTS | LVS_EDITLABELS;
	switch (this->FolderSettings.ViewMode)
	{ case FVM_ICON:	dwStyle |= LVS_ICON;		break;
	  case FVM_SMALLICON: 	dwStyle |= LVS_SMALLICON;	break;
	  case FVM_LIST: 	dwStyle |= LVS_LIST;		break;
	  case FVM_DETAILS: 	dwStyle |= LVS_REPORT;		break;
	}
	if (this->FolderSettings.fFlags && FWF_AUTOARRANGE)	dwStyle |= LVS_AUTOARRANGE;
	/*if (this->FolderSettings.fFlags && FWF_DESKTOP); used from explorer*/
	if (this->FolderSettings.fFlags && FWF_SINGLESEL)	dwStyle |= LVS_SINGLESEL;

	this->hWndList=CreateWindowEx32A( WS_EX_CLIENTEDGE,
					  WC_LISTVIEW32A,
					  NULL,
					  dwStyle,
					  0,0,0,0,
  					  this->hWnd,
					  (HMENU32)ID_LISTVIEW,
					  shell32_hInstance,
					  NULL);

	if(!this->hWndList)
	  return FALSE;

	//  UpdateShellSettings();
	return TRUE;
}
/**************************************************************************
* ShellView_InitList()
*
* NOTES
*  internal
*/
int  nColumn1=100; /* width of column */
int  nColumn2=100;
int  nColumn3=100;
int  nColumn4=100;

BOOL32 ShellView_InitList(LPSHELLVIEW this)
{ LVCOLUMN32A lvColumn;
  CHAR        szString[50];

  TRACE(shell,"%p\n",this);


  ListView_DeleteAllItems(this->hWndList);		/*empty the list*/

  //initialize the columns
  lvColumn.mask = LVCF_FMT | LVCF_WIDTH  |  LVCF_SUBITEM;

  lvColumn.fmt = LVCFMT_LEFT;
  lvColumn.pszText = szString;

  lvColumn.cx = nColumn1;
  strcpy(szString,"File");
  /*LoadString32A(shell32_hInstance, IDS_COLUMN1, szString, sizeof(szString));*/
  ListView_InsertColumn32A(this->hWndList, 0, &lvColumn);

  lvColumn.cx = nColumn2;
  strcpy(szString,"COLUMN2");
  ListView_InsertColumn32A(this->hWndList, 1, &lvColumn);

  lvColumn.cx = nColumn3;
  strcpy(szString,"COLUMN3");
  ListView_InsertColumn32A(this->hWndList, 2, &lvColumn);

  lvColumn.cx = nColumn4;
  strcpy(szString,"COLUMN4");
  ListView_InsertColumn32A(this->hWndList, 3, &lvColumn);

  ListView_SetImageList(this->hWndList, ShellSmallIconList, LVSIL_SMALL);
  ListView_SetImageList(this->hWndList, ShellBigIconList, LVSIL_NORMAL);
  
  return TRUE;
}
/**************************************************************************
* ShellView_CompareItems() 
*
* NOTES
*  internal, CALLBACK for DSA_Sort
*/   
int CALLBACK ShellView_CompareItems(LPVOID lParam1, LPVOID lParam2, LPARAM lpData)
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

static HRESULT ShellView_FillList(LPSHELLVIEW this)
{	LPENUMIDLIST	pEnumIDList;
	LPITEMIDLIST	pidl;
	DWORD		dwFetched;
	UINT32		i;
	LVITEM32A	lvItem;
	HRESULT		hRes;
	HDPA		hdpa;

	TRACE(shell,"%p\n",this);

	/* get the itemlist from the shfolder*/  
	hRes = this->pSFParent->lpvtbl->fnEnumObjects(this->pSFParent,this->hWnd, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &pEnumIDList);
        if (hRes != S_OK)
        { if (hRes==S_FALSE)
	    return(NOERROR);
	  return(hRes);
        }

	/* create a pointer array */	
  	hdpa = DPA_Create(16);
	if (!hdpa)
	{ return(E_OUTOFMEMORY);
	}

	/* copy the items into the array*/
	while((S_OK == pEnumIDList->lpvtbl->fnNext(pEnumIDList,1, &pidl, &dwFetched)) && dwFetched)
	{ if (DPA_InsertPtr(hdpa, 0x7fff, pidl) == -1)
	  { SHFree(pidl);
          } 
	}
	
	/*sort the array*/
	DPA_Sort(hdpa, ShellView_CompareItems, (LPARAM)this->pSFParent);

	/*turn the listview's redrawing off*/
 	SendMessage32A(this->hWndList, WM_SETREDRAW, FALSE, 0); 

        for (i=0; i < DPA_GetPtrCount(hdpa); ++i)
        { pidl = (LPITEMIDLIST)DPA_GetPtr(hdpa, i);
	  if (IncludeObject(this, pidl) == S_OK)	/* in a commdlg this works as a filemask*/
	  { ZeroMemory(&lvItem, sizeof(lvItem));	/* create the listviewitem*/
	    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;		/*set the mask*/
	    lvItem.iItem = ListView_GetItemCount(this->hWndList);	/*add the item to the end of the list*/
	    lvItem.lParam = (LPARAM)ILClone(pidl);			/*set the item's data*/
	    lvItem.pszText = LPSTR_TEXTCALLBACK32A;			/*get text on a callback basis*/
	    lvItem.iImage = I_IMAGECALLBACK;				/*get the image on a callback basis*/
	    ListView_InsertItem32A(this->hWndList, &lvItem);
	  }
	  else
	    SHFree(pidl);	/* the listview has a COPY*/
        }

	/*turn the listview's redrawing back on and force it to draw*/
	SendMessage32A(this->hWndList, WM_SETREDRAW, TRUE, 0);
	InvalidateRect32(this->hWndList, NULL, TRUE);
	UpdateWindow32(this->hWndList);

	pEnumIDList->lpvtbl->fnRelease(pEnumIDList); /* destroy the list*/
	DPA_Destroy(hdpa);
	
	return S_OK;
}

/**************************************************************************
*  ShellView_OnCreate()
*
* NOTES
*  internal
*/   
LRESULT ShellView_OnCreate(LPSHELLVIEW this)
{ TRACE(shell,"%p\n",this);

  if(ShellView_CreateList(this))
  {  if(ShellView_InitList(this))
     { ShellView_FillList(this);
     }
  }

  return S_OK;
}
/**************************************************************************
*  ShellView_OnSize()
*/   
LRESULT ShellView_OnSize(LPSHELLVIEW this, WORD wWidth, WORD wHeight)
{	TRACE(shell,"%p width=%u height=%u\n",this, wWidth,wHeight);

	/*resize the ListView to fit our window*/
	if(this->hWndList)
	{ MoveWindow32(this->hWndList, 0, 0, wWidth, wHeight, TRUE);
	}

	return S_OK;
}
/**************************************************************************
* ShellView_BuildFileMenu()
*/   
HMENU32 ShellView_BuildFileMenu(LPSHELLVIEW this)
{	CHAR			szText[MAX_PATH];
	MENUITEMINFO32A	mii;
	int				nTools,i;
	HMENU32 		hSubMenu;

	TRACE(shell,"(%p) semi-stub\n",this);

	hSubMenu = CreatePopupMenu32();
	if(hSubMenu)
	{ /*get the number of items in our global array*/
	  for(nTools = 0; g_Tools[nTools].idCommand != -1; nTools++){}

	  /*add the menu items*/
	  for(i = 0; i < nTools; i++)
	  { strcpy(szText, "dummy BuildFileMenu");
      
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	    if(TBSTYLE_SEP != g_Tools[i].bStyle)
	    { mii.fType = MFT_STRING;
	      mii.fState = MFS_ENABLED;
	      mii.dwTypeData = szText;
	      mii.wID = g_Tools[i].idCommand;
	    }
	    else
	    { mii.fType = MFT_SEPARATOR;
	    }
	    /* tack this item onto the end of the menu */
	    InsertMenuItem32A(hSubMenu, (UINT32)-1, TRUE, &mii);
	  }
	}
	return hSubMenu;
}
/**************************************************************************
* ShellView_MergeFileMenu()
*/   
void ShellView_MergeFileMenu(LPSHELLVIEW this, HMENU32 hSubMenu)
{   MENUITEMINFO32A   mii;
	CHAR          szText[MAX_PATH];

	TRACE(shell,"(%p)->(submenu=0x%08x) stub\n",this,hSubMenu);
	if(hSubMenu)
	{ ZeroMemory(&mii, sizeof(mii));

	  /* add a separator */
	  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	  mii.fType = MFT_SEPARATOR;
	  mii.fState = MFS_ENABLED;

	  /*insert this item at the beginning of the menu */
	  InsertMenuItem32A(hSubMenu, 0, TRUE, &mii);

	  /*add the file menu items */
      strcpy(szText,"dummy 45");
      
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	  mii.fType = MFT_STRING;
	  mii.fState = MFS_ENABLED;
	  mii.dwTypeData = szText;
	  mii.wID = IDM_MYFILEITEM;

	  /*insert this item at the beginning of the menu */
	  InsertMenuItem32A(hSubMenu, 0, TRUE, &mii);
	}
}

/**************************************************************************
* ShellView_MergeViewMenu()
*/   
void ShellView_MergeViewMenu(LPSHELLVIEW this, HMENU32 hSubMenu)
{   MENUITEMINFO32A   mii;
    CHAR          szText[MAX_PATH];

	TRACE(shell,"(%p)->(submenu=0x%08x) stub\n",this,hSubMenu);
	if(hSubMenu)
	{ ZeroMemory(&mii, sizeof(mii));

	  /*add a separator at the correct position in the menu*/
	  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	  mii.fType = MFT_SEPARATOR;
	  mii.fState = MFS_ENABLED;
	  InsertMenuItem32A(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);

	  /*add the view menu items at the correct position in the menu*/
      strcpy(szText,"Dummy 46");
      
	  mii.cbSize = sizeof(mii);
	  mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	  mii.fType = MFT_STRING;
	  mii.fState = MFS_ENABLED;
	  mii.dwTypeData = szText;
	  mii.wID = IDM_VIEW_FILES;
	  InsertMenuItem32A(hSubMenu, FCIDM_MENU_VIEW_SEP_OPTIONS, FALSE, &mii);
	}
}
/**************************************************************************
* ShellView_UpdateMenu()
*/
LRESULT ShellView_UpdateMenu(LPSHELLVIEW this, HMENU32 hMenu)
{	TRACE(shell,"(%p)->(menu=0x%08x\n",this,hMenu);
	CheckMenuItem32(hMenu, IDM_VIEW_FILES, MF_BYCOMMAND | (g_bViewKeys ? MF_CHECKED: MF_UNCHECKED));

	if(ShellView_CanDoIDockingWindow(this))
	{ EnableMenuItem32(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_ENABLED);
	  CheckMenuItem32(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | (g_bShowIDW ? MF_CHECKED: MF_UNCHECKED));
	}
	else
	{ EnableMenuItem32(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	  CheckMenuItem32(hMenu, IDM_VIEW_IDW, MF_BYCOMMAND | MF_UNCHECKED);
	}
	return 0;
}

/**************************************************************************
* ShellView_OnDeactivate()
*
* NOTES
*  internal
*/   
void ShellView_OnDeactivate(LPSHELLVIEW this)
{ TRACE(shell,"%p\n",this);
  if(this->uState != SVUIA_DEACTIVATE)
  { if(this->hMenu)
    { this->pShellBrowser->lpvtbl->fnSetMenuSB(this->pShellBrowser,0, 0, 0);
      this->pShellBrowser->lpvtbl->fnRemoveMenusSB(this->pShellBrowser,this->hMenu);
      DestroyMenu32(this->hMenu);
      this->hMenu = 0;
      }

   this->uState = SVUIA_DEACTIVATE;
   }
}

/**************************************************************************
* CShellView_OnActivate()
*/   
LRESULT ShellView_OnActivate(LPSHELLVIEW this, UINT32 uState)
{	OLEMENUGROUPWIDTHS32   omw = { {0, 0, 0, 0, 0, 0} };
	MENUITEMINFO32A         mii;
	CHAR                szText[MAX_PATH];

	TRACE(shell,"%p uState=%x\n",this,uState);    

	//don't do anything if the state isn't really changing
	if(this->uState == uState)
	{ return S_OK;
	}

	ShellView_OnDeactivate(this);

	//only do this if we are active
	if(uState != SVUIA_DEACTIVATE)
	{ //merge the menus
	  this->hMenu = CreateMenu32();
   
	  if(this->hMenu)
	  { this->pShellBrowser->lpvtbl->fnInsertMenusSB(this->pShellBrowser, this->hMenu, &omw);

	    /*build the top level menu get the menu item's text*/
	    strcpy(szText,"dummy 31");
      
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	    mii.fType = MFT_STRING;
	    mii.fState = MFS_ENABLED;
	    mii.dwTypeData = szText;
	    mii.hSubMenu = ShellView_BuildFileMenu(this);

	    /*insert our menu into the menu bar*/
	    if(mii.hSubMenu)
	    { InsertMenuItem32A(this->hMenu, FCIDM_MENU_HELP, FALSE, &mii);
	    }

	    /*get the view menu so we can merge with it*/
	    ZeroMemory(&mii, sizeof(mii));
	    mii.cbSize = sizeof(mii);
	    mii.fMask = MIIM_SUBMENU;
      
	    if(GetMenuItemInfo32A(this->hMenu, FCIDM_MENU_VIEW, FALSE, &mii))
	    { ShellView_MergeViewMenu(this, mii.hSubMenu);
	    }

	    /*add the items that should only be added if we have the focus*/
	    if(SVUIA_ACTIVATE_FOCUS == uState)
	    { //get the file menu so we can merge with it
	      ZeroMemory(&mii, sizeof(mii));
	      mii.cbSize = sizeof(mii);
	      mii.fMask = MIIM_SUBMENU;
      
	      if(GetMenuItemInfo32A(this->hMenu, FCIDM_MENU_FILE, FALSE, &mii))
	      { ShellView_MergeFileMenu(this, mii.hSubMenu);
	      }
	    }
	  this->pShellBrowser->lpvtbl->fnSetMenuSB(this->pShellBrowser, this->hMenu, 0, this->hWnd);
	  }
	}
	this->uState = uState;
	return 0;
}

/**************************************************************************
*  ShellView_OnSetFocus()
*
* NOTES
*  internal
*/
LRESULT ShellView_OnSetFocus(LPSHELLVIEW this)
{	TRACE(shell,"%p\n",this);
	/* Tell the browser one of our windows has received the focus. This should always 
	be done before merging menus (OnActivate merges the menus) if one of our 
	windows has the focus.*/
	this->pShellBrowser->lpvtbl->fnOnViewWindowActive(this->pShellBrowser,this);
	ShellView_OnActivate(this, SVUIA_ACTIVATE_FOCUS);

	return 0;
}

/**************************************************************************
* ShellView_OnKillFocus()
*/   
LRESULT ShellView_OnKillFocus(LPSHELLVIEW this)
{	TRACE(shell,"(%p) stub\n",this);
	ShellView_OnActivate(this, SVUIA_ACTIVATE_NOFOCUS);
	return 0;
}

/**************************************************************************
* ShellView_AddRemoveDockingWindow()
*/   
BOOL32 ShellView_AddRemoveDockingWindow(LPSHELLVIEW this, BOOL32 bAdd)
{	BOOL32	bReturn = FALSE;
	HRESULT	hr;
	LPSERVICEPROVIDER	pSP;
	LPDOCKINGWINDOWFRAME	pFrame;
	
	FIXME(shell,"(%p)->(badd=0x%08x) stub\n",this,bAdd);

	/* get the browser's IServiceProvider */
	hr = this->pShellBrowser->lpvtbl->fnQueryInterface(this->pShellBrowser, (REFIID)&IID_IServiceProvider, (LPVOID*)&pSP);
	if(SUCCEEDED(hr))
	{ /*get the IDockingWindowFrame pointer*/
	  hr = pSP->lpvtbl->fnQueryService(pSP, (REFGUID)&SID_SShellBrowser, (REFIID)&IID_IDockingWindowFrame, (LPVOID*)&pFrame);
	  if(SUCCEEDED(hr))
	  { if(bAdd)
	    { hr = S_OK;
	      /*if(!this->pDockingWindow)
	      { //create the toolbar object
	        this->pDockingWindow = DockingWindow_Constructor(this, this->hWnd);
	      }

	      if(this->pDockingWindow)
	      { //add the toolbar object
	        hr = pFrame->lpvtbl->fnAddToolbar(pFrame, (IDockingWindow*)this->pDockingWindow, TOOLBAR_ID, 0);

	        if(SUCCEEDED(hr))
	        { bReturn = TRUE;
	        }
	      }*/
	    }
	    else
	    { /*if(this->pDockingWindow)
	      { hr = pFrame->->lpvtbl->fnRemoveToolbar(pFrame, (IDockingWindow*)this->pDockingWindow, DWFRF_NORMAL);

	        if(SUCCEEDED(hr))
	        { // RemoveToolbar should release the toolbar object which will cause 
	          //it to destroy itself. Our toolbar object is no longer valid at 
	          //this point.
            
	          this->pDockingWindow = NULL;
	          bReturn = TRUE;
	        }
	      }*/
	    }
	    pFrame->lpvtbl->fnRelease(pFrame);
	  }
	  pSP->lpvtbl->fnRelease(pSP);
	}
	return bReturn;
}

/**************************************************************************
* ShellView_CanDoIDockingWindow()
*/   
BOOL32 ShellView_CanDoIDockingWindow(LPSHELLVIEW this)
{	BOOL32 bReturn = FALSE;
	HRESULT hr;
	LPSERVICEPROVIDER pSP;
	LPDOCKINGWINDOWFRAME pFrame;
	
	FIXME(shell,"(%p) stub\n",this);
	
	/*get the browser's IServiceProvider*/
	hr = this->pShellBrowser->lpvtbl->fnQueryInterface(this->pShellBrowser, (REFIID)&IID_IServiceProvider, (LPVOID*)&pSP);
	if(hr==S_OK)
	{ hr = pSP->lpvtbl->fnQueryService(pSP, (REFGUID)&SID_SShellBrowser, (REFIID)&IID_IDockingWindowFrame, (LPVOID*)&pFrame);
   	  if(SUCCEEDED(hr))
	  { bReturn = TRUE;
	    pFrame->lpvtbl->fnRelease(pFrame);
	  }
	  pSP->lpvtbl->fnRelease(pSP);
	}
	return bReturn;
}

/**************************************************************************
*  ShellView_UpdateShellSettings()
*/
void ShellView_UpdateShellSettings(LPSHELLVIEW this)
{	FIXME(shell,"(%p) stub\n",this);
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

	hinstShell32 = LoadLibrary(TEXT("shell32.dll"));
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

	ListView_SetExtendedListViewStyle(this->hWndList, dwExStyles);
*/
}

/**************************************************************************
*   ShellView_OnSettingChange()
*/   
LRESULT ShellView_OnSettingChange(LPSHELLVIEW this, LPCSTR lpszSection)
{	TRACE(shell,"(%p) stub\n",this);
	//if(0 == lstrcmpi(lpszSection, TEXT("ShellState")))
	{ ShellView_UpdateShellSettings(this);
	  return 0;
	}
	return 0;
}

/**************************************************************************
*   ShellView_DoContextMenu()
*/   
void ShellView_DoContextMenu(LPSHELLVIEW this, WORD x, WORD y, BOOL32 fDefault)
{	UINT32	uCommand, i;
	DWORD	wFlags;
	HMENU32 hMenu;
	BOOL32  fExplore = FALSE;
	HWND32  hwndTree = 0;
	INT32          	nMenuIndex;
	LVITEM32A	lvItem;
	MENUITEMINFO32A	mii;
	LPCONTEXTMENU	pContextMenu = NULL;
	CMINVOKECOMMANDINFO32  cmi;
	
	TRACE(shell,"(%p)->(0x%08x 0x%08x 0x%08x) stub\n",this, x, y, fDefault);
	this->uSelected = ListView_GetSelectedCount(this->hWndList);
	this->aSelectedItems = (LPITEMIDLIST*)SHAlloc(this->uSelected * sizeof(LPITEMIDLIST));

	if(this->aSelectedItems)
	{ TRACE(shell,"-- Items selected =%u\n", this->uSelected);
	  ZeroMemory(&lvItem, sizeof(lvItem));
	  lvItem.mask = LVIF_STATE | LVIF_PARAM;
	  lvItem.stateMask = LVIS_SELECTED;
	  lvItem.iItem = 0;

	  i = 0;
   
	  while(ListView_GetItem32A(this->hWndList, &lvItem) && (i < this->uSelected))
	  { if(lvItem.state & LVIS_SELECTED)
	    { this->aSelectedItems[i] = (LPITEMIDLIST)lvItem.lParam;
	      i++;
	      TRACE(shell,"-- selected Item found\n");
	    }
	    lvItem.iItem++;
	  }

	  this->pSFParent->lpvtbl->fnGetUIObjectOf(	this->pSFParent,
							this->hWndParent,
							this->uSelected,
							this->aSelectedItems,
							(REFIID)&IID_IContextMenu,
							NULL,
							(LPVOID *)&pContextMenu);
   
	  if(pContextMenu)
	  { TRACE(shell,"-- pContextMenu\n");
	    hMenu = CreatePopupMenu32();

            /* See if we are in Explore or Open mode. If the browser's tree is present, 
            then we are in Explore mode.*/
        
	    fExplore = FALSE;
	    hwndTree = 0;
	    if(SUCCEEDED(this->pShellBrowser->lpvtbl->fnGetControlWindow(this->pShellBrowser,FCW_TREE, &hwndTree)) && hwndTree)
	    { TRACE(shell,"-- fExplore\n");
	      fExplore = TRUE;
	    }

	    if(hMenu && SUCCEEDED(pContextMenu->lpvtbl->fnQueryContextMenu( pContextMenu,
									    hMenu,
									    0,
									    MENU_OFFSET,
									    MENU_MAX,
									    CMF_NORMAL | (this->uSelected != 1 ? 0 : CMF_CANRENAME) | (fExplore ? CMF_EXPLORE : 0))))
	    { if(fDefault)
	      { TRACE(shell,"-- fDefault\n");
	        uCommand = 0;
            
	        ZeroMemory(&mii, sizeof(mii));
	        mii.cbSize = sizeof(mii);
	        mii.fMask = MIIM_STATE | MIIM_ID;

	        nMenuIndex = 0;

	        /*find the default item in the menu*/
	        while(GetMenuItemInfo32A(hMenu, nMenuIndex, TRUE, &mii))
	        { if(mii.fState & MFS_DEFAULT)
	          { uCommand = mii.wID;
	            break;
	          }
	          nMenuIndex++;
	        }
	      }
	      else
	      { TRACE(shell,"-- ! fDefault\n");
	        uCommand = TrackPopupMenu32( hMenu,TPM_LEFTALIGN | TPM_RETURNCMD,x,y,0,this->hWnd,NULL);
	      }
         
	      if(uCommand > 0)
	      { TRACE(shell,"-- uCommand=%u\n", uCommand);
	        if (((uCommand-MENU_OFFSET) == IDM_EXPLORE) || ((uCommand-MENU_OFFSET)  == IDM_OPEN))
		{ if (IsInCommDlg(this))			/* are we part of a commctrl? */
		  { TRACE(shell,"-- fnOnDefaultCommand\n");
		    OnDefaultCommand(this);
		  }
		  else					/* we are acting with a full featured IShellBrowser */
		  { TRACE(shell,"-- fnBrowseObject pidl =%p\n", this->aSelectedItems[0]);
		    wFlags = SBSP_DEFBROWSER | SBSP_DEFMODE | SBSP_RELATIVE;
		    this->pShellBrowser->lpvtbl->fnBrowseObject(this->pShellBrowser, 
								this->aSelectedItems[0], 
								wFlags);
		  }
		}
		else
		{ ZeroMemory(&cmi, sizeof(cmi));
	          cmi.cbSize = sizeof(cmi);
	          cmi.hwnd = this->hWndParent;
	          cmi.lpVerb = (LPCSTR)MAKEINTRESOURCE32A(uCommand - MENU_OFFSET);
			pContextMenu->lpvtbl->fnInvokeCommand(pContextMenu, &cmi);
		}
	      }
	      DestroyMenu32(hMenu);
	    }
	    pContextMenu->lpvtbl->fnRelease(pContextMenu);
	  }
	  SHFree(this->aSelectedItems);
	  this->aSelectedItems=NULL;
	  this->uSelected=0;
	}
}

/**************************************************************************
* ShellView_OnCommand()
*/   
LRESULT ShellView_OnCommand(LPSHELLVIEW this,DWORD dwCmdID, DWORD dwCmd, HWND32 hwndCmd)
{	TRACE(shell,"(%p)->(0x%08lx 0x%08lx 0x%08x) stub\n",this, dwCmdID, dwCmd, hwndCmd);
	switch(dwCmdID)
	{ case IDM_VIEW_FILES:
	    g_bViewKeys = ! g_bViewKeys;
	    IShellView_Refresh(this);
	    break;

	  case IDM_VIEW_IDW:
	    g_bShowIDW = ! g_bShowIDW;
	    ShellView_AddRemoveDockingWindow(this, g_bShowIDW);
	    break;
   
	  case IDM_MYFILEITEM:
	    MessageBeep32(MB_OK);
	    break;

	  default:
	    FIXME(shell,"-- COMMAND unhandled\n");
	}
	return 0;
}

/**************************************************************************
* ShellView_OnNotify()
*/
   
LRESULT ShellView_OnNotify(LPSHELLVIEW this, UINT32 CtlID, LPNMHDR lpnmh)
{	NM_LISTVIEW *lpnmlv = (NM_LISTVIEW*)lpnmh;
	NMLVDISPINFO32A *lpdi = (NMLVDISPINFO32A *)lpnmh;
	LPITEMIDLIST pidl;
	DWORD dwCursor; 
	STRRET   str;  
	UINT32  uFlags;
	IExtractIcon *pei;
  
	TRACE(shell,"%p CtlID=%u lpnmh->code=%x\n",this,CtlID,lpnmh->code);
  
	switch(lpnmh->code)
	{ case NM_SETFOCUS:
	    TRACE(shell,"-- NM_SETFOCUS %p\n",this);
	    ShellView_OnSetFocus(this);
	    break;

	  case NM_KILLFOCUS:
	    TRACE(shell,"-- NM_KILLFOCUS %p\n",this);
	    ShellView_OnDeactivate(this);
	    break;

	  case HDN_ENDTRACK32A:
	    TRACE(shell,"-- HDN_ENDTRACK32A %p\n",this);
	    /*nColumn1 = ListView_GetColumnWidth(this->hWndList, 0);
	    nColumn2 = ListView_GetColumnWidth(this->hWndList, 1);*/
	    break;
   
	  case LVN_DELETEITEM:
	    TRACE(shell,"-- LVN_DELETEITEM %p\n",this);
	    SHFree((LPITEMIDLIST)lpnmlv->lParam);     /*delete the pidl because we made a copy of it*/
	    break;
   
	  case NM_DBLCLK:
	  case NM_RETURN:
	    TRACE(shell,"-- NM_RETURN|NM_DBLCLK ignored, waiting for LVN_ITEMACTIVATE\n");
	    break;

	  case LVN_ITEMACTIVATE:
	    TRACE(shell,"-- LVN_ITEMACTIVATE %p\n",this);
	    ShellView_DoContextMenu(this, 0, 0, TRUE);
	    break;
   
	  case NM_RCLICK:
	    TRACE(shell,"-- NM_RCLICK %p\n",this);
	    dwCursor = GetMessagePos();
	    ShellView_DoContextMenu(this, LOWORD(dwCursor), HIWORD(dwCursor), FALSE);
	    break;

	  case LVN_GETDISPINFO32A:
	    TRACE(shell,"-- LVN_GETDISPINFO32A %p\n",this);
	    pidl = (LPITEMIDLIST)lpdi->item.lParam;


	    if(lpdi->item.iSubItem)		  /*is the sub-item information being requested?*/
	    { if(lpdi->item.mask & LVIF_TEXT)	 /*is the text being requested?*/
	      { if(_ILIsValue(pidl))	/*is this a value or a folder?*/
	        {  _ILGetDataText(this->mpidl, pidl, lpdi->item.pszText, lpdi->item.cchTextMax);
	           if(!*lpdi->item.pszText)
	           sprintf(lpdi->item.pszText, "file attrib %u", lpdi->item.iSubItem );
	        }
	        else  /*its a folder*/
	        { sprintf(lpdi->item.pszText, "folder attrib %u", lpdi->item.iSubItem );
	        }
	      }
	    }
	    else	   /*the item text is being requested*/
	    { if(lpdi->item.mask & LVIF_TEXT)	   /*is the text being requested?*/
	      { if(SUCCEEDED(this->pSFParent->lpvtbl->fnGetDisplayNameOf(this->pSFParent,pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &str)))
	        { if(STRRET_WSTR == str.uType)
	          { WideCharToLocal32(lpdi->item.pszText, str.u.pOleStr, lpdi->item.cchTextMax);
	            SHFree(str.u.pOleStr);
	          }
	          if(STRRET_CSTR == str.uType)
	          { strncpy(lpdi->item.pszText, str.u.cStr, lpdi->item.cchTextMax);
	          }
	        }
	      }

	      if(lpdi->item.mask & LVIF_IMAGE) 		/*is the image being requested?*/
	      { if(SUCCEEDED(this->pSFParent->lpvtbl->fnGetUIObjectOf(this->pSFParent,this->hWnd,1,
				(LPCITEMIDLIST*)&pidl, (REFIID)&IID_IExtractIcon, NULL, (LPVOID*)&pei)))
	        { //GetIconLoaction will give us the index into our image list
	          pei->lpvtbl->fnGetIconLocation(pei, GIL_FORSHELL, NULL, 0, &lpdi->item.iImage, &uFlags);
	          pei->lpvtbl->fnRelease(pei);
	        }
	      }
	    }
	    TRACE(shell,"-- text=%s image=%x\n",lpdi->item.pszText, lpdi->item.iImage);
	    break;

	  case NM_CLICK:
	    WARN(shell,"-- NM_CLICK %p\n",this);
	    break;

	  case LVN_ITEMCHANGING:
	    WARN(shell,"-- LVN_ITEMCHANGING %p\n",this);
	    break;

	  case LVN_ITEMCHANGED:
	    WARN(shell,"-- LVN_ITEMCHANGED %p\n",this);
	    OnStateChange(this, CDBOSC_SELCHANGE);  
	    break;

	  case LVN_DELETEALLITEMS:
	    WARN(shell,"-- LVN_DELETEALLITEMS %p\n",this);
	    break;

	  case LVN_INSERTITEM:
	    WARN(shell,"-- LVN_INSERTITEM %p\n",this);
	    break;

	  case LVN_BEGINDRAG:
	    WARN(shell,"-- LVN_BEGINDRAG %p\n",this);
	    break;

	  case NM_CUSTOMDRAW:
	    WARN(shell,"NM_CUSTOMDRAW %p\n",this);
	    break;

	  default:
	    FIXME (shell,"-- WM_NOTIFY unhandled\n");
	    break;;
	}
	return 0;
}

/**************************************************************************
*  ShellView_WndProc
*/

LRESULT CALLBACK ShellView_WndProc(HWND32 hWnd, UINT32 uMessage, WPARAM32 wParam, LPARAM lParam)
{ LPSHELLVIEW pThis = (LPSHELLVIEW)GetWindowLong32A(hWnd, GWL_USERDATA);
  LPCREATESTRUCT32A lpcs;
  DWORD dwCursor;
  
  TRACE(shell,"(hwnd=%x msg=%x wparm=%x lparm=%lx)\n",hWnd, uMessage, wParam, lParam);
    
  switch (uMessage)
  { case WM_NCCREATE:
      { TRACE(shell,"-- WM_NCCREATE\n");
        lpcs = (LPCREATESTRUCT32A)lParam;
        pThis = (LPSHELLVIEW)(lpcs->lpCreateParams);
        SetWindowLong32A(hWnd, GWL_USERDATA, (LONG)pThis);
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
      UpdateWindow32(pThis->hWndList);
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
      return ShellView_UpdateMenu(pThis, (HMENU32)wParam);
   
   case WM_NOTIFY:
      TRACE(shell,"-- WM_NOTIFY\n");
      return ShellView_OnNotify(pThis,(UINT32)wParam, (LPNMHDR)lParam);

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

/* -------------*/
   case WM_MOVE:
      WARN(shell,"-- WM_MOVE\n");   
      break;
   
   case WM_ACTIVATEAPP:
      WARN(shell,"-- WM_ACTIVATEAPP\n");
      break;

   case WM_NOTIFYFORMAT:
      WARN(shell,"-- WM_NOTIFYFORMAT\n");
      break;

   case WM_NCPAINT:
      WARN(shell,"-- WM_NCPAINT\n");
      break;

   case WM_ERASEBKGND:
      WARN(shell,"-- WM_ERASEBKGND\n");
      break;

   case WM_PAINT:
      WARN(shell,"-- WM_PAINT\n");
      break;

   case WM_NCCALCSIZE:
      WARN(shell,"-- WM_NCCALCSIZE\n");
      break;

   case WM_WINDOWPOSCHANGING:
      WARN(shell,"-- WM_WINDOWPOSCHANGING\n");
      break;

   case WM_WINDOWPOSCHANGED:
      WARN(shell,"-- WM_WINDOWPOSCHANGED\n");
      break;

   case WM_MOUSEACTIVATE:
      WARN(shell,"-- WM_MOUSEACTIVATE\n");
      break;

   case WM_SETCURSOR:
      WARN(shell,"-- WM_SETCURSOR\n");
      break;

   case WM_DESTROY:
      WARN(shell,"-- WM_DESTROY\n");
      break;

   case WM_NCDESTROY:
      WARN(shell,"-- WM_NCDESTROY\n");
      break;

   case WM_CONTEXTMENU:
      WARN(shell,"-- WM_CONTEXTMENU\n");
      break;

   case WM_MENUSELECT:
      WARN(shell,"-- WM_MENUSELECT\n");
      break;

   case WM_CAPTURECHANGED:
      WARN(shell,"-- WM_CAPTURECHANGED\n");
      break;

   case WM_CHILDACTIVATE:
      WARN(shell,"-- WM_CHILDACTIVATE\n");
      break;

   case WM_ENTERIDLE:
      WARN(shell,"-- WM_ENTERIDLE\n");
      break;

   default:
      FIXME(shell,"-- MESSAGE unhandled\n");
      break;
  }
  return DefWindowProc32A (hWnd, uMessage, wParam, lParam);
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
static HRESULT WINAPI IShellView_QueryInterface(LPSHELLVIEW this,REFIID riid, LPVOID *ppvObj)
{ char    xriid[50];
  WINE_StringFromCLSID((LPCLSID)riid,xriid);
  TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IShellView))  /*IShellView*/
  { *ppvObj = (IShellView*)this;
  }   

  if(*ppvObj)
  { (*(LPSHELLVIEW*)ppvObj)->lpvtbl->fnAddRef(this);      
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
  TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}   
/**************************************************************************
*  IShellView::AddRef
*/
static ULONG WINAPI IShellView_AddRef(LPSHELLVIEW this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
  return ++(this->ref);
}
/**************************************************************************
*  IShellView_Release
*/
static ULONG WINAPI IShellView_Release(LPSHELLVIEW this)
{ TRACE(shell,"(%p)->()\n",this);
  if (!--(this->ref)) 
  { TRACE(shell," destroying IShellView(%p)\n",this);

    if(this->pSFParent)
       this->pSFParent->lpvtbl->fnRelease(this->pSFParent);

    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  return this->ref;
}
/**************************************************************************
*  ShellView_GetWindow
*/
static HRESULT WINAPI IShellView_GetWindow(LPSHELLVIEW this,HWND32 * phWnd)
{	TRACE(shell,"(%p) stub\n",this);
	*phWnd = this->hWnd;

	return S_OK;
}
static HRESULT WINAPI IShellView_ContextSensitiveHelp(LPSHELLVIEW this,BOOL32 fEnterMode)
{ FIXME(shell,"(%p) stub\n",this);
  return E_NOTIMPL;
}
/**************************************************************************
* IShellView_TranslateAccelerator
*
* FIXME:
*  use the accel functions
*/
static HRESULT WINAPI IShellView_TranslateAccelerator(LPSHELLVIEW this,LPMSG32 lpmsg)
{	FIXME(shell,"(%p)->(%p: hwnd=%x msg=%x lp=%lx wp=%x) stub\n",this,lpmsg, lpmsg->hwnd, lpmsg->message, lpmsg->lParam, lpmsg->wParam);
/*	switch (lpmsg->message)
	{ case WM_RBUTTONDOWN:		
		return SendMessage32A ( lpmsg->hwnd, WM_NOTIFY, );
	}*/
	return S_FALSE;
}
static HRESULT WINAPI IShellView_EnableModeless(LPSHELLVIEW this,BOOL32 fEnable)
{ FIXME(shell,"(%p) stub\n",this);
  return E_NOTIMPL;
}
static HRESULT WINAPI IShellView_UIActivate(LPSHELLVIEW this,UINT32 uState)
{	CHAR	szName[MAX_PATH];
	LRESULT	lResult;
	int	nPartArray[1] = {-1};

	TRACE(shell,"(%p)->(state=%x) stub\n",this, uState);
	/*don't do anything if the state isn't really changing*/
	if(this->uState == uState)
	{ return S_OK;
	}

	/*OnActivate handles the menu merging and internal state*/
	ShellView_OnActivate(this, uState);

	/*remove the docking window*/
	if(g_bShowIDW)
	{ ShellView_AddRemoveDockingWindow(this, FALSE);
	}

	/*only do this if we are active*/
	if(uState != SVUIA_DEACTIVATE)
	{ //update the status bar
	   strcpy(szName, "dummy32");
   
	  this->pSFParent->lpvtbl->fnGetFolderPath( this->pSFParent,
	  					    szName + strlen(szName),
						    sizeof(szName) - strlen(szName));

	  /* set the number of parts */
	  this->pShellBrowser->lpvtbl->fnSendControlMsg(this->pShellBrowser,
	  						FCW_STATUS,
							SB_SETPARTS,
							1,
							(LPARAM)nPartArray,
							&lResult);

	  /* set the text for the parts */
	  this->pShellBrowser->lpvtbl->fnSendControlMsg(this->pShellBrowser,
							FCW_STATUS,
							SB_SETTEXT32A,
							0,
							(LPARAM)szName,
							&lResult);

	  //add the docking window if necessary
	  if(g_bShowIDW)
	  { ShellView_AddRemoveDockingWindow(this, TRUE);
	  }
	}
	return S_OK;
}
static HRESULT WINAPI IShellView_Refresh(LPSHELLVIEW this)
{	TRACE(shell,"(%p)\n",this);

	ListView_DeleteAllItems(this->hWndList);
	ShellView_FillList(this);

	return S_OK;
}
static HRESULT WINAPI IShellView_CreateViewWindow(LPSHELLVIEW this, IShellView *lpPrevView,
                     LPCFOLDERSETTINGS lpfs, IShellBrowser * psb,RECT32 * prcView, HWND32  *phWnd)
{	WNDCLASS32A wc;
/*	LRESULT dwResult;*/
	*phWnd = 0;

	
	TRACE(shell,"(%p)->(shlview=%p set=%p shlbrs=%p rec=%p hwnd=%p) incomplete\n",this, lpPrevView,lpfs, psb, prcView, phWnd);
	TRACE(shell,"-- vmode=%x flags=%x left=%i top=%i right=%i bottom=%i\n",lpfs->ViewMode, lpfs->fFlags ,prcView->left,prcView->top, prcView->right, prcView->bottom);

	/*set up the member variables*/
	this->pShellBrowser = psb;
	this->FolderSettings = *lpfs;

	/*get our parent window*/
	this->pShellBrowser->lpvtbl->fnAddRef(this->pShellBrowser);
	this->pShellBrowser->lpvtbl->fnGetWindow(this->pShellBrowser, &(this->hWndParent));

/*	this->pShellBrowser->lpvtbl->fnSendControlMsg(this->pShellBrowser, FCW_TOOLBAR, TB_ENABLEBUTTON, 0xa004, TRUE, &dwResult);
*/
	/* try to get the ICommDlgBrowserInterface */
	this->pCommDlgBrowser=NULL;
	if ( SUCCEEDED (this->pShellBrowser->lpvtbl->fnQueryInterface( this->pShellBrowser,
								        (REFIID)&IID_ICommDlgBrowser,
									(LPVOID*) &this->pCommDlgBrowser)))
	{ TRACE(shell,"-- CommDlgBrowser\n");
	}
	   
	/*if our window class has not been registered, then do so*/
	if(!GetClassInfo32A(shell32_hInstance, SV_CLASS_NAME, &wc))
	{ ZeroMemory(&wc, sizeof(wc));
	  wc.style          = CS_HREDRAW | CS_VREDRAW;
	  wc.lpfnWndProc    = (WNDPROC32) ShellView_WndProc;
	  wc.cbClsExtra     = 0;
	  wc.cbWndExtra     = 0;
	  wc.hInstance      = shell32_hInstance;
	  wc.hIcon          = 0;
	  wc.hCursor        = LoadCursor32A (0, IDC_ARROW32A);
	  wc.hbrBackground  = (HBRUSH32) (COLOR_WINDOW + 1);
	  wc.lpszMenuName   = NULL;
	  wc.lpszClassName  = SV_CLASS_NAME;
   
	  if(!RegisterClass32A(&wc))
	    return E_FAIL;
	}

	*phWnd = CreateWindowEx32A(0, SV_CLASS_NAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                           prcView->left, prcView->top, prcView->right - prcView->left, prcView->bottom - prcView->top,
                           this->hWndParent, 0, shell32_hInstance, (LPVOID)this);
                           
	if(!*phWnd)
	  return E_FAIL;

	return S_OK;
}

static HRESULT WINAPI IShellView_DestroyViewWindow(LPSHELLVIEW this)
{	TRACE(shell,"(%p)\n",this);

	/*Make absolutely sure all our UI is cleaned up.*/
	IShellView_UIActivate(this, SVUIA_DEACTIVATE);
	if(this->hMenu)
   	{ DestroyMenu32(this->hMenu);
	}
	DestroyWindow32(this->hWnd);
	this->pShellBrowser->lpvtbl->fnRelease(this->pShellBrowser);
	return S_OK;
}
static HRESULT WINAPI IShellView_GetCurrentInfo(LPSHELLVIEW this, LPFOLDERSETTINGS lpfs)
{	TRACE(shell,"(%p)->(%p) vmode=%x flags=%x\n",this, lpfs, 
		this->FolderSettings.ViewMode, this->FolderSettings.fFlags);
  
	if (lpfs)
	{ *lpfs = this->FolderSettings;
	  return NOERROR;
	}
	else
	  return E_INVALIDARG;
}
static HRESULT WINAPI IShellView_AddPropertySheetPages(LPSHELLVIEW this, DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{ FIXME(shell,"(%p) stub\n",this);
  return E_NOTIMPL;
}
static HRESULT WINAPI IShellView_SaveViewState(LPSHELLVIEW this)
{ FIXME(shell,"(%p) stub\n",this);
  return S_OK;
}
static HRESULT WINAPI IShellView_SelectItem(LPSHELLVIEW this, LPCITEMIDLIST pidlItem, UINT32 uFlags)
{ FIXME(shell,"(%p)->(pidl=%p, 0x%08x) stub\n",this, pidlItem, uFlags);
  return E_NOTIMPL;
}
static HRESULT WINAPI IShellView_GetItemObject(LPSHELLVIEW this, UINT32 uItem, REFIID riid, LPVOID *ppvOut)
{ 	LPDATAOBJECT pDataObject;
	char    xriid[50];
	HRESULT       hr;
	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(uItem=0x%08x,\n\tIID=%s, ppv=%p)\n",this, uItem, xriid, ppvOut);

	*ppvOut = NULL;
	pDataObject = IDataObject_Constructor(this->hWndParent, this->pSFParent,this->aSelectedItems,this->uSelected);
	if(!pDataObject)
	  return E_OUTOFMEMORY;
	hr = pDataObject->lpvtbl->fnQueryInterface(pDataObject, riid, ppvOut);
	pDataObject->lpvtbl->fnRelease(pDataObject);

	TRACE(shell,"-- (%p)->(interface=%p)\n",this, ppvOut);

	return hr;
}
