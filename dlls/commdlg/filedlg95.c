/*
 * COMMDLG - File Open Dialogs Win95 look and feel
 *
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "winbase.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "dlgs.h"
#include "cdlg.h"
#include "debugtools.h"
#include "cderr.h"
#include "tweak.h"
#include "winnls.h"
#include "shellapi.h"
#include "tchar.h"
#include "filedlgbrowser.h"
#include "wine/obj_contextmenu.h"

DEFAULT_DEBUG_CHANNEL(commdlg);

/***********************************************************************
 * Data structure and global variables
 */
typedef struct SFolder
{
  int m_iImageIndex;    /* Index of picture in image list */
  HIMAGELIST hImgList;
  int m_iIndent;      /* Indentation index */
  LPITEMIDLIST pidlItem;  /* absolute pidl of the item */ 

} SFOLDER,*LPSFOLDER;

typedef struct tagLookInInfo
{
  int iMaxIndentation;
  UINT uSelectedItem;
} LookInInfos;


/***********************************************************************
 * Defines and global variables
 */

/* Draw item constant */
#define ICONWIDTH 18
#define YTEXTOFFSET 2
#define XTEXTOFFSET 3

/* AddItem flags*/
#define LISTEND -1

/* SearchItem methods */
#define SEARCH_PIDL 1
#define SEARCH_EXP  2
#define ITEM_NOTFOUND -1

/* Undefined windows message sent by CreateViewObject*/
#define WM_GETISHELLBROWSER  WM_USER+7

/* NOTE
 * Those macros exist in windowsx.h. However, you can't really use them since
 * they rely on the UNICODE defines and can't be use inside Wine itself.
 */

/* Combo box macros */
#define CBAddString(hwnd,str) \
  SendMessageA(hwnd,CB_ADDSTRING,0,(LPARAM)str);

#define CBInsertString(hwnd,str,pos) \
  SendMessageA(hwnd,CB_INSERTSTRING,(WPARAM)pos,(LPARAM)str);

#define CBDeleteString(hwnd,pos) \
  SendMessageA(hwnd,CB_DELETESTRING,(WPARAM)pos,0);

#define CBSetItemDataPtr(hwnd,iItemId,dataPtr) \
  SendMessageA(hwnd,CB_SETITEMDATA,(WPARAM)iItemId,(LPARAM)dataPtr);

#define CBGetItemDataPtr(hwnd,iItemId) \
  SendMessageA(hwnd,CB_GETITEMDATA,(WPARAM)iItemId,0)

#define CBGetLBText(hwnd,iItemId,str) \
  SendMessageA(hwnd,CB_GETLBTEXT,(WPARAM)iItemId,(LPARAM)str);

#define CBGetCurSel(hwnd) \
  SendMessageA(hwnd,CB_GETCURSEL,0,0);

#define CBSetCurSel(hwnd,pos) \
  SendMessageA(hwnd,CB_SETCURSEL,(WPARAM)pos,0);

#define CBGetCount(hwnd) \
    SendMessageA(hwnd,CB_GETCOUNT,0,0);
#define CBShowDropDown(hwnd,show) \
  SendMessageA(hwnd,CB_SHOWDROPDOWN,(WPARAM)show,0);
#define CBSetItemHeight(hwnd,index,height) \
  SendMessageA(hwnd,CB_SETITEMHEIGHT,(WPARAM)index,(LPARAM)height);


const char *FileOpenDlgInfosStr = "FileOpenDlgInfos"; /* windows property description string */
const char *LookInInfosStr = "LookInInfos"; /* LOOKIN combo box property */

static const char defaultFilter[] = "*.*";

/***********************************************************************
 * Prototypes
 */

/* Internal functions used by the dialog */
static LRESULT FILEDLG95_OnWMInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_OnWMGetIShellBrowser(HWND hwnd);
       BOOL    FILEDLG95_OnOpen(HWND hwnd);
static LRESULT FILEDLG95_InitUI(HWND hwnd);
static void    FILEDLG95_Clean(HWND hwnd);

/* Functions used by the shell object */
static LRESULT FILEDLG95_SHELL_Init(HWND hwnd);
static BOOL    FILEDLG95_SHELL_UpFolder(HWND hwnd);
static BOOL    FILEDLG95_SHELL_ExecuteCommand(HWND hwnd, LPCSTR lpVerb);
static void    FILEDLG95_SHELL_Clean(HWND hwnd);

/* Functions used by the filetype combo box */
static HRESULT FILEDLG95_FILETYPE_Init(HWND hwnd);
static BOOL    FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_FILETYPE_SearchExt(HWND hwnd,LPSTR lpstrExt);
static void    FILEDLG95_FILETYPE_Clean(HWND hwnd);

/* Functions used by the Look In combo box */
static HRESULT FILEDLG95_LOOKIN_Init(HWND hwndCombo);
static LRESULT FILEDLG95_LOOKIN_DrawItem(LPDRAWITEMSTRUCT pDIStruct);
static BOOL    FILEDLG95_LOOKIN_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_LOOKIN_AddItem(HWND hwnd,LPITEMIDLIST pidl, int iInsertId);
static int     FILEDLG95_LOOKIN_SearchItem(HWND hwnd,WPARAM searchArg,int iSearchMethod);
static int     FILEDLG95_LOOKIN_InsertItemAfterParent(HWND hwnd,LPITEMIDLIST pidl);
static int     FILEDLG95_LOOKIN_RemoveMostExpandedItem(HWND hwnd);
       int     FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl);
static void    FILEDLG95_LOOKIN_Clean(HWND hwnd);

/* Miscellaneous tool functions */
HRESULT       GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPSTR lpstrFileName);
HRESULT       GetFileName(HWND hwnd, LPITEMIDLIST pidl, LPSTR lpstrFileName);
IShellFolder* GetShellFolderFromPidl(LPITEMIDLIST pidlAbs);
LPITEMIDLIST  GetParentPidl(LPITEMIDLIST pidl);
LPITEMIDLIST  GetPidlFromName(IShellFolder *psf,LPCSTR lpcstrFileName);

/* Shell memory allocation */
void *MemAlloc(UINT size);
void MemFree(void *mem);

BOOL WINAPI GetFileName95(FileOpenDlgInfos *fodInfos);
HRESULT WINAPI FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode);
HRESULT FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL FILEDLG95_OnOpenMultipleFiles(HWND hwnd, LPSTR lpstrFileList, UINT nFileCount, UINT sizeUsed);

/***********************************************************************
 *      GetFileName95
 *
 * Creates an Open common dialog box that lets the user select 
 * the drive, directory, and the name of a file or set of files to open.
 *
 * IN  : The FileOpenDlgInfos structure associated with the dialog
 * OUT : TRUE on success
 *       FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 */
BOOL WINAPI GetFileName95(FileOpenDlgInfos *fodInfos)
{

    LRESULT lRes;
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;

    /* Create the dialog from a template */

    if(!(hRes = FindResourceA(COMMDLG_hInstance32,MAKEINTRESOURCEA(NEWFILEOPENORD),RT_DIALOGA)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMMDLG_hInstance32, hRes )) ||
        !(template = LockResource( hDlgTmpl )))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
    lRes = DialogBoxIndirectParamA(COMMDLG_hInstance32,
                                  (LPDLGTEMPLATEA) template,
                                  fodInfos->ofnInfos->hwndOwner,
                                  (DLGPROC) FileOpenDlgProc95,
                                  (LPARAM) fodInfos);

    /* Unable to create the dialog*/
    if( lRes == -1)
        return FALSE;
    
    return lRes;
}

/***********************************************************************
 *      GetFileDialog95A
 *
 * Copy the OPENFILENAMEA structure in a FileOpenDlgInfos structure.
 * Call GetFileName95 with this structure and clean the memory.
 *
 * IN  : The OPENFILENAMEA initialisation structure passed to
 *       GetOpenFileNameA win api function (see filedlg.c)
 */
BOOL  WINAPI GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType)
{

  BOOL ret;
  FileOpenDlgInfos *fodInfos;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter = NULL;
  LPSTR lpstrCustomFilter = NULL;
  LPCSTR lpstrInitialDir = NULL;
  DWORD dwFlags = 0;
  
  /* Initialise FileOpenDlgInfos structure*/  
  fodInfos = (FileOpenDlgInfos*)MemAlloc(sizeof(FileOpenDlgInfos));

  /* Pass in the original ofn */
  fodInfos->ofnInfos = ofn;
  
  /* Save original hInstance value */
  hInstance = ofn->hInstance;
  fodInfos->ofnInfos->hInstance = MapHModuleLS(ofn->hInstance);

  if (ofn->lpstrFilter)
  {
    LPSTR s,x;
    lpstrFilter = ofn->lpstrFilter;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPSTR)ofn->lpstrFilter;
    while (*s)
      s = s+strlen(s)+1;
    s++;
    x = (LPSTR)MemAlloc(s-ofn->lpstrFilter);
    memcpy(x,ofn->lpstrFilter,s-ofn->lpstrFilter);
    fodInfos->ofnInfos->lpstrFilter = (LPSTR)x;
  }
  if (ofn->lpstrCustomFilter)
  {
    LPSTR s,x;
    lpstrCustomFilter = ofn->lpstrCustomFilter;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPSTR)ofn->lpstrCustomFilter;
    while (*s)
      s = s+strlen(s)+1;
    s++;
    x = MemAlloc(s-ofn->lpstrCustomFilter);
    memcpy(x,ofn->lpstrCustomFilter,s-ofn->lpstrCustomFilter);
    fodInfos->ofnInfos->lpstrCustomFilter = (LPSTR)x;
  }

  dwFlags = ofn->Flags;
  fodInfos->ofnInfos->Flags = ofn->Flags|OFN_WINE;

  /* Replace the NULL lpstrInitialDir by the current folder */
  lpstrInitialDir = ofn->lpstrInitialDir;
  if(!lpstrInitialDir)
  {
    fodInfos->ofnInfos->lpstrInitialDir = MemAlloc(MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH,(LPSTR)fodInfos->ofnInfos->lpstrInitialDir);
  }

  /* Initialise the dialog property */
  fodInfos->DlgInfos.dwDlgProp = 0;
  fodInfos->DlgInfos.hwndCustomDlg = (HWND)NULL;
  
  switch(iDlgType)
  {
  case OPEN_DIALOG :
      ret = GetFileName95(fodInfos);
      break;
  case SAVE_DIALOG :
      fodInfos->DlgInfos.dwDlgProp |= FODPROP_SAVEDLG;
      ret = GetFileName95(fodInfos);
      break;
  default :
      ret = 0;
  }

  if (lpstrInitialDir)
  {
      MemFree((LPVOID)(fodInfos->ofnInfos->lpstrInitialDir));
    fodInfos->ofnInfos->lpstrInitialDir = lpstrInitialDir;
      }
  if (lpstrFilter)
  {
    MemFree((LPVOID)(fodInfos->ofnInfos->lpstrFilter));
    fodInfos->ofnInfos->lpstrFilter = lpstrFilter;
  }
  if (lpstrCustomFilter)
    {
    MemFree((LPVOID)(fodInfos->ofnInfos->lpstrCustomFilter));
    fodInfos->ofnInfos->lpstrCustomFilter = lpstrCustomFilter;
      }

  ofn->Flags = dwFlags;
  ofn->hInstance = hInstance;
  MemFree((LPVOID)(fodInfos));
  return ret;
}

/***********************************************************************
 *      GetFileDialog95W
 *
 * Copy the OPENFILENAMEW structure in a FileOpenDlgInfos structure.
 * Call GetFileName95 with this structure and clean the memory.
 *
 * IN  : The OPENFILENAMEW initialisation structure passed to
 *       GetOpenFileNameW win api function (see filedlg.c)
 *
 * FIXME:
 *   some more strings are needing to be convertet AtoW
 */
BOOL  WINAPI GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos *fodInfos;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter = NULL;
  LPSTR lpstrCustomFilter = NULL;
  DWORD dwFlags;

  /* Initialise FileOpenDlgInfos structure*/
  fodInfos = (FileOpenDlgInfos*)MemAlloc(sizeof(FileOpenDlgInfos));

  /*  Pass in the original ofn */
  fodInfos->ofnInfos = (LPOPENFILENAMEA) ofn;

  /* Save hInstance */
  hInstance = fodInfos->ofnInfos->hInstance;
  fodInfos->ofnInfos->hInstance = MapHModuleLS(ofn->hInstance);

  /* Save lpstrFilter */
  if (ofn->lpstrFilter)
  {
    LPWSTR  s;
    LPSTR x,y;
    int n;

    lpstrFilter = fodInfos->ofnInfos->lpstrFilter;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPWSTR)ofn->lpstrFilter;
    
    while (*s)
      s = s+lstrlenW(s)+1;
    s++;
    n = s - ofn->lpstrFilter; /* already divides by 2. ptr magic */
    x = y = (LPSTR)MemAlloc(n);
    s = (LPWSTR)ofn->lpstrFilter;
    while (*s) {
      lstrcpyWtoA(x,s);
      x+=lstrlenA(x)+1;
      s+=lstrlenW(s)+1;
    }
    *x=0;
    fodInfos->ofnInfos->lpstrFilter = (LPSTR)y;
  }
  /* Save lpstrCustomFilter */
  if (ofn->lpstrCustomFilter)
  {
    LPWSTR  s;
    LPSTR x,y;
    int n;

    lpstrCustomFilter = fodInfos->ofnInfos->lpstrCustomFilter;
    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPWSTR)ofn->lpstrCustomFilter;
    while (*s)
      s = s+lstrlenW(s)+1;
    s++;
    n = s - ofn->lpstrCustomFilter;
    x = y = (LPSTR)MemAlloc(n);
    s = (LPWSTR)ofn->lpstrCustomFilter;
    while (*s) {
      lstrcpyWtoA(x,s);
      x+=lstrlenA(x)+1;
      s+=lstrlenW(s)+1;
    }
    *x=0;
    fodInfos->ofnInfos->lpstrCustomFilter = (LPSTR)y;
  }

  /* Save Flags */
  dwFlags = fodInfos->ofnInfos->Flags;
  fodInfos->ofnInfos->Flags = ofn->Flags|OFN_WINE|OFN_UNICODE;

  /* Initialise the dialog property */
  fodInfos->DlgInfos.dwDlgProp = 0;
  
  switch(iDlgType)
  {
  case OPEN_DIALOG :
      ret = GetFileName95(fodInfos);
      break;
  case SAVE_DIALOG :
      fodInfos->DlgInfos.dwDlgProp |= FODPROP_SAVEDLG;
      ret = GetFileName95(fodInfos);
      break;
  default :
      ret = 0;
  }
      
  /* Cleaning */
  /* Restore Flags */
  fodInfos->ofnInfos->Flags = dwFlags;

  /* Restore lpstrFilter */
  if (fodInfos->ofnInfos->lpstrFilter)
  {
    MemFree((LPVOID)(fodInfos->ofnInfos->lpstrFilter));
    fodInfos->ofnInfos->lpstrFilter = lpstrFilter;
  }
  if (fodInfos->ofnInfos->lpstrCustomFilter)
  {
    MemFree((LPVOID)(fodInfos->ofnInfos->lpstrCustomFilter));
    fodInfos->ofnInfos->lpstrCustomFilter = lpstrCustomFilter;
  }

  /* Restore hInstance */
  fodInfos->ofnInfos->hInstance = hInstance;
  MemFree((LPVOID)(fodInfos));
  return ret;
}

void ArrangeCtrlPositions( HWND hwndChildDlg, HWND hwndParentDlg)
{

	HWND hwndChild,hwndStc32;
	RECT rectParent, rectChild, rectCtrl, rectStc32, rectTemp;
	POINT ptMoveCtl;
	POINT ptParentClient;

	ptMoveCtl.x = ptMoveCtl.y = 0;
	hwndStc32=GetDlgItem(hwndChildDlg,stc32);
	GetClientRect(hwndParentDlg,&rectParent);
	GetClientRect(hwndChildDlg,&rectChild);
	if(hwndStc32)
	{
		GetWindowRect(hwndStc32,&rectStc32);
		MapWindowPoints(0, hwndChildDlg,(LPPOINT)&rectStc32,2);
		CopyRect(&rectTemp,&rectStc32);

		SetRect(&rectStc32,rectStc32.left,rectStc32.top,rectStc32.left + (rectParent.right-rectParent.left),rectStc32.top+(rectParent.bottom-rectParent.top));
		SetWindowPos(hwndStc32,0,rectStc32.left,rectStc32.top,rectStc32.right-rectStc32.left,rectStc32.bottom-rectStc32.top,SWP_NOMOVE|SWP_NOZORDER | SWP_NOACTIVATE);

		if(rectStc32.right < rectTemp.right)
		{
			ptParentClient.x = max((rectParent.right-rectParent.left),(rectChild.right-rectChild.left));
			ptMoveCtl.x = 0;
		}
		else
		{
			ptMoveCtl.x = (rectStc32.right - rectTemp.right);
			ptParentClient.x = max((rectParent.right-rectParent.left),((rectChild.right-rectChild.left)+rectStc32.right-rectTemp.right));
		}
		if(rectStc32.bottom < rectTemp.bottom)
		{
			ptParentClient.y = max((rectParent.bottom-rectParent.top),(rectChild.bottom-rectChild.top));
			ptMoveCtl.y = 0;
		}
		else
		{
			ptMoveCtl.y = (rectStc32.bottom - rectTemp.bottom);
			ptParentClient.y = max((rectParent.bottom-rectParent.top),((rectChild.bottom-rectChild.top)+rectStc32.bottom-rectTemp.bottom));
		}
	}
	else
	{
		if( (GetWindow(hwndChildDlg,GW_CHILD)) == (HWND) NULL)
                   return;
		ptParentClient.x = rectParent.right-rectParent.left;
		ptParentClient.y = (rectParent.bottom-rectParent.top) + (rectChild.bottom-rectChild.top);
		ptMoveCtl.y = rectParent.bottom-rectParent.top;
		ptMoveCtl.x=0;
	}
	SetRect(&rectParent,rectParent.left,rectParent.top,rectParent.left+ptParentClient.x,rectParent.top+ptParentClient.y);
	AdjustWindowRectEx( &rectParent,GetWindowLongA(hwndParentDlg,GWL_STYLE),FALSE,GetWindowLongA(hwndParentDlg,GWL_EXSTYLE));

	SetWindowPos(hwndChildDlg, 0, 0,0, ptParentClient.x,ptParentClient.y,
		 SWP_NOZORDER );
	SetWindowPos(hwndParentDlg, 0, rectParent.left,rectParent.top, (rectParent.right- rectParent.left),
		(rectParent.bottom-rectParent.top),SWP_NOMOVE | SWP_NOZORDER);
	
	hwndChild = GetWindow(hwndChildDlg,GW_CHILD);
	if(hwndStc32)
	{
		GetWindowRect(hwndStc32,&rectStc32);
		MapWindowPoints( 0, hwndChildDlg,(LPPOINT)&rectStc32,2);
	}
	else
		SetRect(&rectStc32,0,0,0,0);

	if (hwndChild )
	{
		do
		{
			if(hwndChild != hwndStc32)
			{
			if (GetWindowLongA( hwndChild, GWL_STYLE ) & WS_MAXIMIZE)
				continue;
			GetWindowRect(hwndChild,&rectCtrl);
			MapWindowPoints( 0, hwndParentDlg,(LPPOINT)&rectCtrl,2);
                                  
				/*
				   Check the initial position of the controls relative to the initial
				   position and size of stc32 (before it is expanded).
				*/
                if (rectCtrl.left > rectTemp.right && rectCtrl.top > rectTemp.bottom)
				{
					rectCtrl.left += ptMoveCtl.x;
				rectCtrl.top  += ptMoveCtl.y;
				}
				else if (rectCtrl.left > rectTemp.right)
					rectCtrl.left += ptMoveCtl.x;
				else if (rectCtrl.top > rectTemp.bottom)
					rectCtrl.top  += ptMoveCtl.y;
					
				SetWindowPos( hwndChild, 0, rectCtrl.left, rectCtrl.top, 
				rectCtrl.right-rectCtrl.left,rectCtrl.bottom-rectCtrl.top,
				SWP_NOSIZE | SWP_NOZORDER );
				}
			}
		while ((hwndChild=GetWindow( hwndChild, GW_HWNDNEXT )) != (HWND)NULL);
	}		
	hwndChild = GetWindow(hwndParentDlg,GW_CHILD);
	
	if(hwndStc32)
	{
		GetWindowRect(hwndStc32,&rectStc32);
		MapWindowPoints( 0, hwndChildDlg,(LPPOINT)&rectStc32,2);
		ptMoveCtl.x = rectStc32.left - 0;
		ptMoveCtl.y = rectStc32.top - 0;
		if (hwndChild )
		{
			do
			{
				if(hwndChild != hwndChildDlg)
				{

					if (GetWindowLongA( hwndChild, GWL_STYLE ) & WS_MAXIMIZE)
						continue;
					GetWindowRect(hwndChild,&rectCtrl);
					MapWindowPoints( 0, hwndParentDlg,(LPPOINT)&rectCtrl,2);

					rectCtrl.left += ptMoveCtl.x;
					rectCtrl.top += ptMoveCtl.y;

					SetWindowPos( hwndChild, 0, rectCtrl.left, rectCtrl.top, 
					rectCtrl.right-rectCtrl.left,rectCtrl.bottom-rectCtrl.top,
					SWP_NOSIZE |SWP_NOZORDER );
				}
			}
			while ((hwndChild=GetWindow( hwndChild, GW_HWNDNEXT )) != (HWND)NULL);
		}		
	}

}


HRESULT WINAPI FileOpenDlgProcUserTemplate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(GetParent(hwnd),FileOpenDlgInfosStr);
  switch(uMsg)
  {
  	case WM_INITDIALOG:
	{         
            fodInfos = (FileOpenDlgInfos *)lParam;
                lParam = (LPARAM) fodInfos->ofnInfos;
   		ArrangeCtrlPositions(hwnd,GetParent(hwnd));
            if(fodInfos && (fodInfos->ofnInfos->Flags & OFN_ENABLEHOOK) && fodInfos->ofnInfos->lpfnHook)
                 return CallWindowProcA((WNDPROC)fodInfos->ofnInfos->lpfnHook,hwnd,uMsg,wParam,lParam);
	        return 0;	
        } 
    }
    if(fodInfos && (fodInfos->ofnInfos->Flags & OFN_ENABLEHOOK) && fodInfos->ofnInfos->lpfnHook )
        return CallWindowProcA((WNDPROC)fodInfos->ofnInfos->lpfnHook,hwnd,uMsg,wParam,lParam); 
  return DefWindowProcA(hwnd,uMsg,wParam,lParam); 
}

HWND CreateTemplateDialog(FileOpenDlgInfos *fodInfos,HWND hwnd)
{
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;
    HWND hChildDlg = 0;
   if (fodInfos->ofnInfos->Flags & OFN_ENABLETEMPLATE || fodInfos->ofnInfos->Flags & OFN_ENABLETEMPLATEHANDLE)
   {
   	if (fodInfos->ofnInfos->Flags  & OFN_ENABLETEMPLATEHANDLE)
   	{
           if( !(template = LockResource( fodInfos->ofnInfos->hInstance)))
    		{
        	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        	return (HWND)NULL;
    		}
		
   	}
 	else
   	{
   	 if (!(hRes = FindResourceA(MapHModuleSL(fodInfos->ofnInfos->hInstance),
            (fodInfos->ofnInfos->lpTemplateName), RT_DIALOGA)))
    	{
        	COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
       		 return (HWND)NULL;
    	}
    	if (!(hDlgTmpl = LoadResource( MapHModuleSL(fodInfos->ofnInfos->hInstance),
             hRes )) ||
                 !(template = LockResource( hDlgTmpl )))
    	{
        	COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        	return (HWND)NULL;
    	}
  	}

	hChildDlg= CreateDialogIndirectParamA(fodInfos->ofnInfos->hInstance,template,hwnd,(DLGPROC)FileOpenDlgProcUserTemplate,(LPARAM)fodInfos);
   	if(hChildDlg)
   	{
   		ShowWindow(hChildDlg,SW_SHOW); 
        	return hChildDlg;
   	}
 }
 else if(fodInfos->ofnInfos->Flags & OFN_ENABLEHOOK && fodInfos->ofnInfos->lpfnHook)
 {
	RECT rectHwnd;
	DLGTEMPLATE tmplate;
	GetClientRect(hwnd,&rectHwnd);
	tmplate.style = WS_CHILD | WS_CLIPSIBLINGS;
	tmplate.dwExtendedStyle = 0;
	tmplate.cdit = 0;
	tmplate.x = 0;
	tmplate.y = 0;
	tmplate.cx = rectHwnd.right-rectHwnd.left;
	tmplate.cy = rectHwnd.bottom-rectHwnd.top;
       
	return CreateDialogIndirectParamA(fodInfos->ofnInfos->hInstance,&tmplate,hwnd,(DLGPROC)FileOpenDlgProcUserTemplate,(LPARAM)fodInfos);
 }
return (HWND)NULL;
}
 
/***********************************************************************
*          SendCustomDlgNotificationMessage
*
* Send CustomDialogNotification (CDN_FIRST -- CDN_LAST) message to the custom template dialog
*/

HRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode)
{
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwndParentDlg,FileOpenDlgInfosStr);
    if(!fodInfos)
        return 0;
    if(fodInfos->DlgInfos.hwndCustomDlg)
    {
        OFNOTIFYA ofnNotify;
        ofnNotify.hdr.hwndFrom=hwndParentDlg;
        ofnNotify.hdr.idFrom=0;
        ofnNotify.hdr.code = uCode;
        ofnNotify.lpOFN = fodInfos->ofnInfos;
        return SendMessageA(fodInfos->DlgInfos.hwndCustomDlg,WM_NOTIFY,0,(LPARAM)&ofnNotify);
    }
    return TRUE;
}

/***********************************************************************
*         FILEDLG95_HandleCustomDialogMessages
*
* Handle Custom Dialog Messages (CDM_FIRST -- CDM_LAST) messages
*/
HRESULT FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPSTR lpstrFileSpec;
    int reqSize;
    char lpstrPath[MAX_PATH];
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
    if(!fodInfos) return -1;

    switch(uMsg)
    {
        case CDM_GETFILEPATH:
            GetDlgItemTextA(hwnd,IDC_FILENAME,lpstrPath, sizeof(lpstrPath));
            lpstrFileSpec = (LPSTR)COMDLG32_PathFindFilenameA(lpstrPath);
            if (lpstrFileSpec==lpstrPath) 
	    {
                char lpstrCurrentDir[MAX_PATH];
                /* Prepend the current path */
                COMDLG32_SHGetPathFromIDListA(fodInfos->ShellInfos.pidlAbsCurrent,lpstrCurrentDir);
                if ((LPSTR)lParam!=NULL)
                    wsnprintfA((LPSTR)lParam,(int)wParam,"%s\\%s",lpstrCurrentDir,lpstrPath);
                reqSize=strlen(lpstrCurrentDir)+1+strlen(lpstrPath)+1;
            } 
	    else 
	    {
                lstrcpynA((LPSTR)lParam,(LPSTR)lpstrPath,(int)wParam);
                reqSize=strlen(lpstrPath);
            }
            /* return the required buffer size */
            return reqSize;

        case CDM_GETFOLDERPATH:
	    COMDLG32_SHGetPathFromIDListA(fodInfos->ShellInfos.pidlAbsCurrent,lpstrPath);
            if ((LPSTR)lParam!=NULL)
                lstrcpynA((LPSTR)lParam,lpstrPath,(int)wParam);
            return strlen(lpstrPath);

        case CDM_GETSPEC:
	    reqSize=GetDlgItemTextA(hwnd,IDC_FILENAME,lpstrPath, sizeof(lpstrPath));
            lpstrFileSpec = (LPSTR)COMDLG32_PathFindFilenameA(lpstrPath);
            if ((LPSTR)lParam!=NULL)
                lstrcpynA((LPSTR)lParam, lpstrFileSpec, (int)wParam);
            return strlen(lpstrFileSpec);

        case CDM_SETCONTROLTEXT:
	    if ( 0 != lParam )
	        SetDlgItemTextA( hwnd, (UINT) wParam, (LPSTR) lParam );
	    return TRUE;

        case CDM_HIDECONTROL:
        case CDM_SETDEFEXT:
            FIXME("CDM_HIDECONTROL,CDM_SETCONTROLTEXT,CDM_SETDEFEXT not implemented\n");
            return -1;
    }
    return TRUE;
}
 
/***********************************************************************
 *          FileOpenDlgProc95
 *
 * File open dialog procedure
 */
HRESULT WINAPI FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG :
         /* Adds the FileOpenDlgInfos in the property list of the dialog 
            so it will be easily accessible through a GetPropA(...) */
      	 SetPropA(hwnd, FileOpenDlgInfosStr, (HANDLE) lParam);

      	 FILEDLG95_OnWMInitDialog(hwnd, wParam, lParam);
      	 ((FileOpenDlgInfos *)lParam)->DlgInfos.hwndCustomDlg = 
     	CreateTemplateDialog((FileOpenDlgInfos *)lParam,hwnd);
         SendCustomDlgNotificationMessage(hwnd,CDN_INITDONE);
         return 0;
    case WM_COMMAND:
      return FILEDLG95_OnWMCommand(hwnd, wParam, lParam);
    case WM_DRAWITEM:
      {
        switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
        {
        case IDC_LOOKIN:
          FILEDLG95_LOOKIN_DrawItem((LPDRAWITEMSTRUCT) lParam);
          return TRUE;
        }
      }
      return FALSE;
          
    case WM_GETISHELLBROWSER:
      return FILEDLG95_OnWMGetIShellBrowser(hwnd);

  case WM_DESTROY:
      RemovePropA(hwnd, FileOpenDlgInfosStr);
        return FALSE;

    case WM_NOTIFY:
    {
	LPNMHDR lpnmh = (LPNMHDR)lParam;
	UINT stringId = -1;

	/* set up the button tooltips strings */
	if(TTN_GETDISPINFOA == lpnmh->code )
	{
	    LPNMTTDISPINFOA lpdi = (LPNMTTDISPINFOA)lParam; 
	    switch(lpnmh->idFrom )
	    {
		/* Up folder button */
		case FCIDM_TB_UPFOLDER:
		    stringId = IDS_UPFOLDER;
		    break;
		/* New folder button */
		case FCIDM_TB_NEWFOLDER:
		    stringId = IDS_NEWFOLDER;
		    break;
		/* List option button */
		case FCIDM_TB_SMALLICON:
		    stringId = IDS_LISTVIEW;
		    break;
		/* Details option button */
		case FCIDM_TB_REPORTVIEW:
		    stringId = IDS_REPORTVIEW;
		    break;
	    }
	    lpdi->hinst = COMMDLG_hInstance32; 
	    lpdi->lpszText =  (LPSTR) stringId;
	}
        return FALSE;
    }
    default :
      if(uMsg >= CDM_FIRST && uMsg <= CDM_LAST)
        return FILEDLG95_HandleCustomDialogMessages(hwnd, uMsg, wParam, lParam);
      return FALSE;
  }
}

/***********************************************************************
 *      FILEDLG95_OnWMInitDialog
 *
 * WM_INITDIALOG message handler
 */
static LRESULT FILEDLG95_OnWMInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LPITEMIDLIST pidlItemId;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) lParam;

  TRACE("\n");

  /* Make sure the common control DLL is loaded */      
  InitCommonControls();

  /* Initialise shell objects */
  FILEDLG95_SHELL_Init(hwnd);

  /* Initialise dialog UI */
  FILEDLG95_InitUI(hwnd);

  /* Initialize the Look In combo box*/
  FILEDLG95_LOOKIN_Init(fodInfos->DlgInfos.hwndLookInCB);

  /* Initialize the filter combo box */
  FILEDLG95_FILETYPE_Init(hwnd);

  /* Get the initial directory pidl */

  if(!(pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder,fodInfos->ofnInfos->lpstrInitialDir)))
  {
    char path[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH,path);
    pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder,
                     path);

  }

  /* Browse to the initial directory */
  IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,pidlItemId,SBSP_RELATIVE);

  /* Free pidlItem memory */
  COMDLG32_SHFree(pidlItemId);

  return TRUE;
}
/***********************************************************************
 *      FILEDLG95_Clean
 *
 * Regroups all the cleaning functions of the filedlg
 */
void FILEDLG95_Clean(HWND hwnd)
{
      FILEDLG95_FILETYPE_Clean(hwnd);
      FILEDLG95_LOOKIN_Clean(hwnd);
      FILEDLG95_SHELL_Clean(hwnd);
}
/***********************************************************************
 *      FILEDLG95_OnWMCommand
 *
 * WM_COMMAND message handler
 */
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  WORD wNotifyCode = HIWORD(wParam); /* notification code */
  WORD wID = LOWORD(wParam);         /* item, control, or accelerator identifier */
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  switch(wID)
  {
    /* OK button */
  case IDOK:
      if(FILEDLG95_OnOpen(hwnd))
        SendCustomDlgNotificationMessage(hwnd,CDN_FILEOK);
    break;
    /* Cancel button */
  case IDCANCEL:
      FILEDLG95_Clean(hwnd);
      EndDialog(hwnd, FALSE);
    break;
    /* Filetype combo box */
  case IDC_FILETYPE:
    FILEDLG95_FILETYPE_OnCommand(hwnd,wNotifyCode);
    break;
    /* LookIn combo box */
  case IDC_LOOKIN:
    FILEDLG95_LOOKIN_OnCommand(hwnd,wNotifyCode);
    break;

  /* --- toolbar --- */
    /* Up folder button */
  case FCIDM_TB_UPFOLDER:
    FILEDLG95_SHELL_UpFolder(hwnd);
    break;
    /* New folder button */
  case FCIDM_TB_NEWFOLDER:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_NEWFOLDER);
    break;
    /* List option button */
  case FCIDM_TB_SMALLICON:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWLIST);
    break;
    /* Details option button */
  case FCIDM_TB_REPORTVIEW:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWDETAILS);
    break;

  case IDC_FILENAME:
    break;

  }
  /* Do not use the listview selection anymore */
  fodInfos->DlgInfos.dwDlgProp &= ~FODPROP_USEVIEW;
  return 0; 
}

/***********************************************************************
 *      FILEDLG95_OnWMGetIShellBrowser
 *
 * WM_GETISHELLBROWSER message handler
 */
static LRESULT FILEDLG95_OnWMGetIShellBrowser(HWND hwnd)
{

  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  SetWindowLongA(hwnd,DWL_MSGRESULT,(LONG)fodInfos->Shell.FOIShellBrowser);

  return TRUE; 
}


/***********************************************************************
 *      FILEDLG95_InitUI
 *
 */
static LRESULT FILEDLG95_InitUI(HWND hwnd)
{
  TBBUTTON tbb[] =
  {{VIEW_PARENTFOLDER, FCIDM_TB_UPFOLDER,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0 },
   {0,                 0,                   TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0 },
   {VIEW_NEWFOLDER,    FCIDM_TB_NEWFOLDER,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0 },
   {0,                 0,                   TBSTATE_ENABLED, TBSTYLE_SEP, {0, 0}, 0, 0 },
   {VIEW_LIST,         FCIDM_TB_SMALLICON,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0 },
   {VIEW_DETAILS,      FCIDM_TB_REPORTVIEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0 },
  };
  TBADDBITMAP tba = { HINST_COMMCTRL, IDB_VIEW_SMALL_COLOR };
  RECT rectTB;
  
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  /* Get the hwnd of the controls */
  fodInfos->DlgInfos.hwndFileName = GetDlgItem(hwnd,IDC_FILENAME);
  fodInfos->DlgInfos.hwndFileTypeCB = GetDlgItem(hwnd,IDC_FILETYPE);
  fodInfos->DlgInfos.hwndLookInCB = GetDlgItem(hwnd,IDC_LOOKIN);

  /* construct the toolbar */
  GetWindowRect(GetDlgItem(hwnd,IDC_TOOLBARSTATIC),&rectTB);
  MapWindowPoints( 0, hwnd,(LPPOINT)&rectTB,2);

  fodInfos->DlgInfos.hwndTB = CreateWindowExA(0, TOOLBARCLASSNAMEA, (LPSTR) NULL, 
        WS_CHILD | WS_GROUP | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
        0, 0, 150, 26,
	hwnd, (HMENU) IDC_TOOLBAR, COMMDLG_hInstance32, NULL); 
 
  SetWindowPos(fodInfos->DlgInfos.hwndTB, 0, 
  	rectTB.left,rectTB.top, rectTB.right-rectTB.left, rectTB.bottom-rectTB.top,
	SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );

  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

/* fixme: use TB_LOADIMAGES when implemented */
/*  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_LOADIMAGES, (WPARAM) IDB_VIEW_SMALL_COLOR, HINST_COMMCTRL);*/
  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_ADDBITMAP, (WPARAM) 12, (LPARAM) &tba);

  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_ADDBUTTONSA, (WPARAM) 6,(LPARAM) &tbb);
  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_AUTOSIZE, 0, 0); 

  /* Set the window text with the text specified in the OPENFILENAME structure */
  if(fodInfos->ofnInfos->lpstrTitle)
  {
      SetWindowTextA(hwnd,fodInfos->ofnInfos->lpstrTitle);
  }
  else if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      SetWindowTextA(hwnd,"Save");
  }

  /* Initialise the file name edit control */
  if(fodInfos->ofnInfos->lpstrFile)
  {
      SetDlgItemTextA(hwnd,IDC_FILENAME,fodInfos->ofnInfos->lpstrFile);
  }
  /* Must the open as read only check box be checked ?*/
  if(fodInfos->ofnInfos->Flags & OFN_READONLY)
  {
    SendDlgItemMessageA(hwnd,IDC_OPENREADONLY,BM_SETCHECK,(WPARAM)TRUE,0);
  }
  /* Must the open as read only check box be hid ?*/
  if(fodInfos->ofnInfos->Flags & OFN_HIDEREADONLY)
  {
    ShowWindow(GetDlgItem(hwnd,IDC_OPENREADONLY),SW_HIDE);
  }
  /* Must the help button be hid ?*/
  if (!(fodInfos->ofnInfos->Flags & OFN_SHOWHELP))
  {
    ShowWindow(GetDlgItem(hwnd, pshHelp), SW_HIDE);
  }
  /* Resize the height, if open as read only checkbox ad help button
     are hidden */
  if ( (fodInfos->ofnInfos->Flags & OFN_HIDEREADONLY) &&
       (!(fodInfos->ofnInfos->Flags & OFN_SHOWHELP)) )
  {
    RECT rectDlg, rectHelp, rectCancel;
    GetWindowRect(hwnd, &rectDlg);
    GetWindowRect(GetDlgItem(hwnd, pshHelp), &rectHelp);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rectCancel);
    /* subtract the height of the help button plus the space between
       the help button and the cancel button to the height of the dialog */
    SetWindowPos(hwnd, 0, 0, 0, rectDlg.right-rectDlg.left, 
                 (rectDlg.bottom-rectDlg.top) - (rectHelp.bottom - rectCancel.bottom), 
                 SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
  }
  /* change Open to Save */
  if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      SetDlgItemTextA(hwnd,IDOK,"Save");
  }
  return 0;
}

/***********************************************************************
 *      FILEDLG95_OnOpenMultipleFiles
 *      
 * Handles the opening of multiple files.
 * 
*/
BOOL FILEDLG95_OnOpenMultipleFiles(HWND hwnd, LPSTR lpstrFileList, UINT nFileCount, UINT sizeUsed)
{
  CHAR   lpstrPathSpec[MAX_PATH] = "";
  CHAR   lpstrTempFileList[MAX_PATH] = "";
  LPSTR  lpstrFile;
  UINT   sizePath;
  UINT   nCount;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  lpstrFile = fodInfos->ofnInfos->lpstrFile;

  COMDLG32_SHGetPathFromIDListA( fodInfos->ShellInfos.pidlAbsCurrent, lpstrPathSpec );
  sizePath = lstrlenA( lpstrPathSpec );

  memset( lpstrFile, 0x0, fodInfos->ofnInfos->nMaxFile * sizeof(CHAR) );

  if ( fodInfos->ofnInfos->Flags & OFN_FILEMUSTEXIST || 
	  !(fodInfos->ofnInfos->Flags & OFN_EXPLORER ))
          {
      LPSTR lpstrTemp = lpstrFileList; 

      for ( nCount = 0; nCount < nFileCount; nCount++ )
      {
          WIN32_FIND_DATAA findData;
	  CHAR lpstrFindFile[MAX_PATH];

          memset( lpstrFindFile, 0x0, MAX_PATH * sizeof(CHAR) );

          lstrcpyA( lpstrFindFile, lpstrPathSpec );
	  lstrcatA( lpstrFindFile, "\\" );
          lstrcatA( lpstrFindFile, lpstrTemp );

	  if ( FindFirstFileA( lpstrFindFile, &findData ) == INVALID_HANDLE_VALUE )
          {
	      CHAR lpstrNotFound[100];
	      CHAR lpstrMsg[100];
	      CHAR tmp[400];

	      LoadStringA(COMMDLG_hInstance32, IDS_FILENOTFOUND, lpstrNotFound, 100);
	      LoadStringA(COMMDLG_hInstance32, IDS_VERIFYFILE, lpstrMsg, 100);

	      strcpy(tmp, lpstrFindFile);
	      strcat(tmp, "\n");
	      strcat(tmp, lpstrNotFound);
	      strcat(tmp, "\n");
	      strcat(tmp, lpstrMsg);

	      MessageBoxA(hwnd,
		      tmp,
		      fodInfos->ofnInfos->lpstrTitle,
		      MB_OK | MB_ICONEXCLAMATION);
	      return FALSE;
  }
  
	  if (!(fodInfos->ofnInfos->Flags & OFN_EXPLORER ))
  {
	      lstrcatA( lpstrTempFileList, findData.cAlternateFileName);
	      if ( nCount + 1 < nFileCount)
		  lstrcatA( lpstrTempFileList, " ");
  }
	  lpstrTemp += strlen(lpstrFileList) + 1;
      }
  }

      if ( fodInfos->ofnInfos->Flags & OFN_EXPLORER )
      {
      lstrcpyA( lpstrFile, lpstrPathSpec);
      memcpy( lpstrFile + sizePath + 1, lpstrFileList, sizeof(CHAR) * sizeUsed );
      }
      else
      {
      memcpy( lpstrFile, lpstrTempFileList, sizeof(CHAR) * strlen(lpstrTempFileList));
      }

      fodInfos->ofnInfos->nFileOffset = sizePath + 1;
      fodInfos->ofnInfos->nFileExtension = 0;

  /* clean and exit */
  FILEDLG95_Clean(hwnd);
  return EndDialog(hwnd,TRUE);
}

/***********************************************************************
 *      FILEDLG95_OnOpen
 *
 * Ok button WM_COMMAND message handler
 * 
 * If the function succeeds, the return value is nonzero.
 */
BOOL FILEDLG95_OnOpen(HWND hwnd)
{
  CHAR lpstrSpecifiedByUser[MAX_PATH];
  CHAR lpstrFileList[MAX_PATH];
  LPSTR lpstrFile;
  UINT nStrCharCount = 0;
  UINT nFileCount = 0;
  UINT nFileIndex = 0;
  UINT sizeUsed = 0;
  UINT nStrLen = 0;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  lpstrSpecifiedByUser[0]=0;

  TRACE("\n");


  lpstrFile = fodInfos->ofnInfos->lpstrFile;

  GetDlgItemTextA(hwnd, IDC_FILENAME, lpstrSpecifiedByUser, MAX_PATH);
  nStrLen = strlen(lpstrSpecifiedByUser);

  while ( nStrCharCount <= nStrLen )
  {
      if ( lpstrSpecifiedByUser[nStrCharCount]=='"' )
      {
          nStrCharCount++;
          while ((lpstrSpecifiedByUser[nStrCharCount]!='"') &&
             (nStrCharCount <= nStrLen))
          {
              lpstrFileList[nFileIndex++] = lpstrSpecifiedByUser[nStrCharCount];
              nStrCharCount++;
              sizeUsed++;
          }
          lpstrFileList[nFileIndex++] = '\0';
          sizeUsed++;
          nFileCount++;
      } 
      nStrCharCount++;
  }

  if(nFileCount > 0)
      return FILEDLG95_OnOpenMultipleFiles(hwnd, lpstrFileList, nFileCount, sizeUsed);

  if (nStrLen)
  {
      LPSHELLFOLDER psfDesktop;
      LPITEMIDLIST browsePidl;
      LPSTR lpstrFileSpec;
      LPSTR lpstrTemp;
      char lpstrPathSpec[MAX_PATH];
      char lpstrCurrentDir[MAX_PATH];
      char lpstrPathAndFile[MAX_PATH];

      lpstrPathSpec[0]	  = '\0';
      lpstrCurrentDir[0]  = '\0';
      lpstrPathAndFile[0] = '\0';

      /* Separate the file spec from the path spec 
	 e.g.: 
	      lpstrSpecifiedByUser  lpstrPathSpec  lpstrFileSpec
	      C:\TEXT1\TEXT2        C:\TEXT1          TEXT2
      */      
      if (nFileCount == 0)
      {	  
          lpstrFileSpec = (LPSTR)COMDLG32_PathFindFilenameA(lpstrSpecifiedByUser);
          strcpy(lpstrPathSpec,lpstrSpecifiedByUser);
          COMDLG32_PathRemoveFileSpecA(lpstrPathSpec);
      }
      
      /* Get the index of the selected item in the filetype combo box */
      fodInfos->ofnInfos->nFilterIndex = (DWORD) CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);
      /* nFilterIndex is 1 based while combo GetCurSel return zero based index */
      fodInfos->ofnInfos->nFilterIndex++;

      /* Get the current directory name */
      COMDLG32_SHGetPathFromIDListA(fodInfos->ShellInfos.pidlAbsCurrent,
				    lpstrCurrentDir);

      /* Create an absolute path name */ 
      if(lpstrSpecifiedByUser[1] != ':')
      {
          switch(lpstrSpecifiedByUser[0])
          {
      	      /* Add drive spec  \TEXT => C:\TEXT */
       	      case '\\':
       	      {
                  int lenPathSpec=strlen(lpstrPathSpec);
                  int iCopy = (lenPathSpec!=0?2:3);
                  memmove(lpstrPathSpec+iCopy,lpstrPathSpec,lenPathSpec);
       	          strncpy(lpstrPathSpec,lpstrCurrentDir,iCopy);
       	      }
       	      break;           
              /* Go to parent ..\TEXT */ 
	      case '.':
	      {
       		  int iSize;
       		  char lpstrTmp2[MAX_PATH];
       		  LPSTR lpstrTmp = strrchr(lpstrCurrentDir,'\\');

       		  iSize = lpstrTmp - lpstrCurrentDir;
       		  strncpy(lpstrTmp2,lpstrCurrentDir,iSize + 1);
		  if(strlen(lpstrSpecifiedByUser) <= 3)
                      *lpstrFileSpec='\0';
       		  if(strcmp(lpstrPathSpec,".."))
       		      strcat(lpstrTmp2,&lpstrPathSpec[3]);
		  strcpy(lpstrPathSpec,lpstrTmp2);
       	      }
       	      break;
       	      default:
              {
	  	  char lpstrTmp[MAX_PATH];

       		  if(strcmp(&lpstrCurrentDir[strlen(lpstrCurrentDir)-1],"\\"))
       		      strcat(lpstrCurrentDir,"\\");
       		  strcpy(lpstrTmp,lpstrCurrentDir);
       		  strcat(lpstrTmp,lpstrPathSpec);
       		  strcpy(lpstrPathSpec,lpstrTmp);
       	      }
          } /* end switch */
      }

      if(strlen(lpstrPathSpec))
      {
	  /* Browse to the right directory */
	  COMDLG32_SHGetDesktopFolder(&psfDesktop); 
	  if((browsePidl = GetPidlFromName(psfDesktop,lpstrPathSpec)))
	  {
	      /* Browse to directory */
	      IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
					 browsePidl,
					 SBSP_ABSOLUTE);
	      COMDLG32_SHFree(browsePidl);
	  }
	  else
	  {
	      /* Path does not exist */
	      if(fodInfos->ofnInfos->Flags & OFN_PATHMUSTEXIST)
	      {
		  MessageBoxA(hwnd,
			      "Path does not exist",
			      fodInfos->ofnInfos->lpstrTitle,
			      MB_OK | MB_ICONEXCLAMATION);
		  return FALSE;
	      }
	  }
	  
	  strcat(lpstrPathAndFile,lpstrPathSpec);
	  IShellFolder_Release(psfDesktop);
      }
      else
      {
	  strcat(lpstrPathAndFile,lpstrCurrentDir);
      }

      /* Create the path and file string */
      COMDLG32_PathAddBackslashA(lpstrPathAndFile);
      strcat(lpstrPathAndFile,lpstrFileSpec);
      
      /* Update the edit field */
      SetDlgItemTextA(hwnd,IDC_FILENAME,lpstrFileSpec);
      SendDlgItemMessageA(hwnd,IDC_FILENAME,EM_SETSEL,0,-1);
      
      /* Don't go further if we dont have a file spec */
      if(!strlen(lpstrFileSpec) || !strcmp(lpstrFileSpec,lpstrPathSpec))
	  return FALSE;

      /* Time to check lpstrFileSpec         */ 
      /* search => contains * or ?           */
      /* browse => contains a directory name */
      /* file   => contains a file name      */

    /* Check if this is a search */
      if(strchr(lpstrFileSpec,'*') || strchr(lpstrFileSpec,'?'))
    {
      int iPos;

       /* Set the current filter with the current selection */
      if(fodInfos->ShellInfos.lpstrCurrentFilter)
         MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

	  fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc((strlen(lpstrFileSpec)+1)*2);
	  lstrcpyAtoW(fodInfos->ShellInfos.lpstrCurrentFilter,
		      (LPSTR)strlwr((LPSTR)lpstrFileSpec));
      
      IShellView_Refresh(fodInfos->Shell.FOIShellView);

	  if(-1 < (iPos = FILEDLG95_FILETYPE_SearchExt(fodInfos->DlgInfos.hwndFileTypeCB,
						       lpstrFileSpec)))
        CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB,iPos);

      return FALSE;
    }

      /* browse if the user specified a directory */
      browsePidl = GetPidlFromName(fodInfos->Shell.FOIShellFolder,
	      lpstrFileSpec);
      if (!browsePidl) /* not a directory check the specified file exists */
      {
	  int iExt;
	  char lpstrFileSpecTemp[MAX_PATH];
	  LPSTR lpstrExt;
	  LPSTR lpOrg;
	  LPSTR lpBuf;
	  iExt = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);
	  lpOrg = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB, iExt);
	  if ((int)lpOrg == -1)
	      lpOrg = NULL;	// we get -1 if the filetype LB is empty 

	  lpstrExt = lpOrg;

	  /*
	     add user specified extentions to the file one by one and
	     check if the file exists
	 */
	 while(lpOrg)
	 {
	     int i;
	     if ((lpstrExt = strchr(lpOrg, ';')))
	     {
		 i = lpstrExt - lpOrg;
	     }
	     else
		 i = strlen(lpOrg);
	     lpBuf = MemAlloc(i+1);
	     strncpy(lpBuf, lpOrg, i);
	     lpBuf[i] = 0;
	     strcpy(lpstrFileSpecTemp, lpstrFileSpec);
	     if (lpstrFileSpecTemp[strlen(lpstrFileSpecTemp)-1] == '.')
	     {
		 if (strchr(lpBuf, '.'))
		     strcat(lpstrFileSpecTemp, (strchr(lpBuf, '.')) + 1);
	     }
	     else
		 strcat(lpstrFileSpecTemp, strchr(lpBuf, '.'));
	     browsePidl = GetPidlFromName(fodInfos->Shell.FOIShellFolder,
		     lpstrFileSpecTemp);
	     MemFree((void *)lpBuf);
	     if (browsePidl)
	     {
		 strcpy(lpstrFileSpec,lpstrFileSpecTemp);
		 break;
	     }
	     if (lpstrExt)
		 lpOrg = lpstrExt+1;
	     else
		 lpOrg = NULL;
	 }
      }

      if (browsePidl)
      {
          ULONG  ulAttr = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
	  int    nMsgBoxRet;
	  char   lpstrFileExist[MAX_PATH + 50];

          IShellFolder_GetAttributesOf(fodInfos->Shell.FOIShellFolder, 
				       1, 
				       &browsePidl, 
				       &ulAttr);

	  /* Browse to directory if it is a folder */
	  if (ulAttr & SFGAO_FOLDER)
	  {
	      if(FAILED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
						   browsePidl,
						   SBSP_RELATIVE)))
        {
		  if(fodInfos->ofnInfos->Flags & OFN_PATHMUSTEXIST)
        {
		      MessageBoxA(hwnd,
				  "Path does not exist",
				  fodInfos->ofnInfos->lpstrTitle,
				  MB_OK | MB_ICONEXCLAMATION);
		      COMDLG32_SHFree(browsePidl);
		      return FALSE;
        }
    }
	      COMDLG32_SHFree(browsePidl);
	      return FALSE;
    }

	  /* The file does exist, so ask the user if we should overwrite it */
	  if((fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG) && 
	     (fodInfos->ofnInfos->Flags & OFN_OVERWRITEPROMPT))
	  {
		strcpy(lpstrFileExist, lpstrFileSpec);
		strcat(lpstrFileExist, " already exists.\nDo you want to replace it?");

		nMsgBoxRet = MessageBoxA(hwnd,
			    		lpstrFileExist,
			    		fodInfos->ofnInfos->lpstrTitle,
			    		MB_YESNO | MB_ICONEXCLAMATION);
		if (nMsgBoxRet == IDNO)
		{
		    COMDLG32_SHFree(browsePidl);
		    return FALSE;
		}
	  }
	  COMDLG32_SHFree(browsePidl);
      }
      else
    {
	  /* File does not exist in current directory */

	  /* The selected file does not exist */
      /* Tell the user the selected does not exist */
      if(fodInfos->ofnInfos->Flags & OFN_FILEMUSTEXIST)
      {
        char lpstrNotFound[100];
        char lpstrMsg[100];
        char tmp[400];

	      LoadStringA(COMMDLG_hInstance32,
			  IDS_FILENOTFOUND,
			  lpstrNotFound,
			  100);
	      LoadStringA(COMMDLG_hInstance32,
			  IDS_VERIFYFILE,
			  lpstrMsg,
			  100);

        strcpy(tmp,fodInfos->ofnInfos->lpstrFile);
        strcat(tmp,"\n");
        strcat(tmp,lpstrNotFound);
        strcat(tmp,"\n");
        strcat(tmp,lpstrMsg);

	      MessageBoxA(hwnd,
			  tmp,
			  fodInfos->ofnInfos->lpstrTitle,
			  MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
      }
      /* Ask the user if he wants to create the file*/
      if(fodInfos->ofnInfos->Flags & OFN_CREATEPROMPT)
      {
        char tmp[100];

        LoadStringA(COMMDLG_hInstance32,IDS_CREATEFILE,tmp,100);

	      if(IDYES == MessageBoxA(hwnd,tmp,fodInfos->ofnInfos->lpstrTitle,
				      MB_YESNO | MB_ICONQUESTION))
        {
            /* Create the file, clean and exit */
            FILEDLG95_Clean(hwnd);
            return EndDialog(hwnd,TRUE);
        }
	return FALSE;        
      }
    }

      /* Open the selected file */

      /* Check file extension */
      if(!strrchr(lpstrPathAndFile,'.'))
      {
      	  /* if the file has no extension, append the selected
       	     extension of the filetype combo box */
       	  int iExt; 
       	  LPSTR lpstrExt;
       	  iExt = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);
	  lpstrTemp = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iExt);

	  if((LPSTR)-1 != lpstrTemp)
	  { 
	      if((lpstrExt = strchr(lpstrTemp,';')))
	      {
      	          int i = lpstrExt - lpstrTemp;
       	          lpstrExt = MemAlloc(i);
       	          strncpy(lpstrExt,&lpstrTemp[1],i-1);
      	      }
       	      else
       	      {
       	          lpstrExt = MemAlloc(strlen(lpstrTemp));
       	          strcpy(lpstrExt,&lpstrTemp[1]);
       	      }
		  
	      if(!strcmp(&lpstrExt[1],"*") && fodInfos->ofnInfos->lpstrDefExt)
	      {
                  MemFree(lpstrExt);
       	          lpstrExt = MemAlloc(strlen(fodInfos->ofnInfos->lpstrDefExt)+2);
       	          strcat(lpstrExt,".");
       	          strcat(lpstrExt,(LPSTR) fodInfos->ofnInfos->lpstrDefExt);
       	      }
	      strcat(lpstrPathAndFile,lpstrExt);
	      MemFree( lpstrExt );
	  }
      }
      /* Check that size size of the file does not exceed buffer size */
      if(strlen(lpstrPathAndFile) > fodInfos->ofnInfos->nMaxFile)
      {
	  /* set error FNERR_BUFFERTOSMALL */
	  FILEDLG95_Clean(hwnd);
      	  return EndDialog(hwnd,FALSE);
      }
      strcpy(fodInfos->ofnInfos->lpstrFile,lpstrPathAndFile);

      /* Set the lpstrFileTitle of the OPENFILENAME structure */
      if(fodInfos->ofnInfos->lpstrFileTitle)
      	  strncpy(fodInfos->ofnInfos->lpstrFileTitle,
       		  lpstrFileSpec,
       		  fodInfos->ofnInfos->nMaxFileTitle);

      /* Check if the file is to be opened as read only */	      
      if(BST_CHECKED == SendDlgItemMessageA(hwnd,
					    IDC_OPENREADONLY,
					    BM_GETSTATE,0,0))
	  SetFileAttributesA(fodInfos->ofnInfos->lpstrFile,
			     FILE_ATTRIBUTE_READONLY);

      /*  nFileExtension and nFileOffset of OPENFILENAME structure */
      lpstrTemp = strrchr(fodInfos->ofnInfos->lpstrFile,'\\');
      fodInfos->ofnInfos->nFileOffset = lpstrTemp - fodInfos->ofnInfos->lpstrFile + 1;
      lpstrTemp = strrchr(fodInfos->ofnInfos->lpstrFile,'.');
      fodInfos->ofnInfos->nFileExtension = lpstrTemp - fodInfos->ofnInfos->lpstrFile + 1;

    
    /* clean and exit */
    FILEDLG95_Clean(hwnd);
    return EndDialog(hwnd,TRUE);
  }

  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_SHELL_Init
 *
 * Initialisation of the shell objects
 */
static HRESULT FILEDLG95_SHELL_Init(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  /*
   * Initialisation of the FileOpenDialogInfos structure 
   */

  /* Shell */

  fodInfos->Shell.FOIShellView = NULL;
  if(FAILED(COMDLG32_SHGetDesktopFolder(&fodInfos->Shell.FOIShellFolder)))
    return E_FAIL;

  /*ShellInfos */
  fodInfos->ShellInfos.hwndOwner = hwnd;

  fodInfos->ShellInfos.folderSettings.fFlags = 0; 
  /* Disable multi-select if flag not set */
  if (!(fodInfos->ofnInfos->Flags & OFN_ALLOWMULTISELECT))
  {
     fodInfos->ShellInfos.folderSettings.fFlags |= FWF_SINGLESEL; 
  }
  fodInfos->ShellInfos.folderSettings.fFlags |= FWF_AUTOARRANGE | FWF_ALIGNLEFT;
  fodInfos->ShellInfos.folderSettings.ViewMode = FVM_LIST;

  GetWindowRect(GetDlgItem(hwnd,IDC_SHELLSTATIC),&fodInfos->ShellInfos.rectView);
  ScreenToClient(hwnd,(LPPOINT)&fodInfos->ShellInfos.rectView.left);
  ScreenToClient(hwnd,(LPPOINT)&fodInfos->ShellInfos.rectView.right);

  /* Construct the IShellBrowser interface */
  fodInfos->Shell.FOIShellBrowser = IShellBrowserImpl_Construct(hwnd);  
    
  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_SHELL_ExecuteCommand
 *
 * Change the folder option and refresh the view
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_SHELL_ExecuteCommand(HWND hwnd, LPCSTR lpVerb)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  IContextMenu * pcm;
  TRACE("(0x%08x,%p)\n", hwnd, lpVerb);

  if(SUCCEEDED(IShellView_GetItemObject(fodInfos->Shell.FOIShellView,
					SVGIO_BACKGROUND,
					&IID_IContextMenu,
					(LPVOID*)&pcm)))
  {
    CMINVOKECOMMANDINFO ci;
    ZeroMemory(&ci, sizeof(CMINVOKECOMMANDINFO));
    ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
    ci.lpVerb = lpVerb;
    ci.hwnd = hwnd;

    IContextMenu_InvokeCommand(pcm, &ci);
    IContextMenu_Release(pcm);
  }

  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_SHELL_UpFolder
 *
 * Browse to the specified object
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_SHELL_UpFolder(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                          NULL,
                                          SBSP_PARENT)))
  {
    return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_SHELL_Clean
 *
 * Cleans the memory used by shell objects
 */
static void FILEDLG95_SHELL_Clean(HWND hwnd)
{
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

    TRACE("\n");

    /* clean Shell interfaces */
    IShellView_DestroyViewWindow(fodInfos->Shell.FOIShellView);
    IShellView_Release(fodInfos->Shell.FOIShellView);
    IShellFolder_Release(fodInfos->Shell.FOIShellFolder);
    IShellBrowser_Release(fodInfos->Shell.FOIShellBrowser);
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_Init
 *
 * Initialisation of the file type combo box 
 */
static HRESULT FILEDLG95_FILETYPE_Init(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(fodInfos->ofnInfos->lpstrFilter)
  {
    int iStrIndex = 0;
    int iPos = 0;
    LPSTR lpstrFilter;
    LPSTR lpstrTmp;

    for(;;)
    {
      /* filter is a list...  title\0ext\0......\0\0 
       * Set the combo item text to the title and the item data
       *  to the ext
       */
      char *lpstrExt = NULL;
      LPSTR lpstrExtTmp = NULL;

      /* Get the title */
      lpstrTmp = (&((LPBYTE)fodInfos->ofnInfos->lpstrFilter)[iStrIndex]);
      if(!strlen(lpstrTmp))
        break;
      iStrIndex += strlen(lpstrTmp) +1;
      /* Get the extension */
      lpstrExtTmp = (&((LPBYTE)fodInfos->ofnInfos->lpstrFilter)[iStrIndex]);
      if(!lpstrExtTmp)
          break;

      lpstrExt = (LPSTR) MemAlloc(strlen(lpstrExtTmp)+1);
      if(!lpstrExt)
          break;
      
      strcpy(lpstrExt,lpstrExtTmp);

      iStrIndex += strlen(lpstrExt) +1;
            
      /* Add the item at the end of the combo */
      CBAddString(fodInfos->DlgInfos.hwndFileTypeCB,lpstrTmp);
      CBSetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iPos++,lpstrExt);
    }
      /* Set the current filter to the one specified
       * in the initialisation structure 
      */
  
      /* set default filter index */
      if(fodInfos->ofnInfos->nFilterIndex == 0 && fodInfos->ofnInfos->lpstrCustomFilter == NULL)
        fodInfos->ofnInfos->nFilterIndex = 1;
      /* First, check to make sure our index isn't out of bounds. */
      if ( fodInfos->ofnInfos->nFilterIndex > iPos )
  	fodInfos->ofnInfos->nFilterIndex = iPos;
 
 
      /* Get the current index selection. */
      CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB,
                  fodInfos->ofnInfos->nFilterIndex-1);
  
      /* Get the corresponding text string from the combo box. */
      lpstrFilter = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             fodInfos->ofnInfos->nFilterIndex-1);

    if(lpstrFilter)
    {
        fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc((strlen(lpstrFilter)+1)*2);
        lstrcpyAtoW(fodInfos->ShellInfos.lpstrCurrentFilter,strlwr(lpstrFilter));
    }
  }
  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_OnCommand
 *
 * WM_COMMAND of the file type combo box
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  switch(wNotifyCode)
  {
    case CBN_SELENDOK:
    {
      LPSTR lpstrFilter;

      /* Get the current item of the filetype combo box */
      int iItem = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);

      /* set the current filter index - indexed from 1 */
      fodInfos->ofnInfos->nFilterIndex = iItem + 1;

      /* Set the current filter with the current selection */
      if(fodInfos->ShellInfos.lpstrCurrentFilter)
         MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

      lpstrFilter = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             iItem);
      if((int)lpstrFilter != CB_ERR)
      {
        fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc((strlen(lpstrFilter)+1)*2);
        lstrcpyAtoW(fodInfos->ShellInfos.lpstrCurrentFilter,(LPSTR)strlwr((LPSTR)lpstrFilter));
        SendCustomDlgNotificationMessage(hwnd,CDN_TYPECHANGE);
      }

      /* Refresh the actual view to display the included items*/
      IShellView_Refresh(fodInfos->Shell.FOIShellView);

    }
  }
  return FALSE;
}
/***********************************************************************
 *      FILEDLG95_FILETYPE_SearchExt
 *
 * Search for pidl in the lookin combo box
 * returns the index of the found item
 */
static int FILEDLG95_FILETYPE_SearchExt(HWND hwnd,LPSTR lpstrExt)
{
  int i = 0;
  int iCount = CBGetCount(hwnd);

  TRACE("\n");

  for(;i<iCount;i++)
  {
      LPSTR ext = (LPSTR) CBGetItemDataPtr(hwnd,i);
      if(!strcasecmp(lpstrExt,ext))
          return i;
  }

  return -1;
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_Clean
 *
 * Clean the memory used by the filetype combo box
 */
static void FILEDLG95_FILETYPE_Clean(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
  int iPos;
  int iCount = CBGetCount(fodInfos->DlgInfos.hwndFileTypeCB);

  TRACE("\n");

  /* Delete each string of the combo and their associated data */
  for(iPos = iCount-1;iPos>=0;iPos--)
  {
    MemFree((LPVOID)(CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iPos)));
    CBDeleteString(fodInfos->DlgInfos.hwndFileTypeCB,iPos);
  }
  /* Current filter */
  if(fodInfos->ShellInfos.lpstrCurrentFilter)
     MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

}
    
/***********************************************************************
 *      FILEDLG95_LOOKIN_Init
 *
 * Initialisation of the look in combo box 
 */
static HRESULT FILEDLG95_LOOKIN_Init(HWND hwndCombo)
{
  IShellFolder	*psfRoot, *psfDrives;
  IEnumIDList	*lpeRoot, *lpeDrives;
  LPITEMIDLIST	pidlDrives, pidlTmp, pidlTmp1, pidlAbsTmp;

  LookInInfos *liInfos = MemAlloc(sizeof(LookInInfos));

  TRACE("\n");

  liInfos->iMaxIndentation = 0;

  SetPropA(hwndCombo, LookInInfosStr, (HANDLE) liInfos);
  CBSetItemHeight(hwndCombo,0,GetSystemMetrics(SM_CYSMICON));

  /* Initialise data of Desktop folder */
  COMDLG32_SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&pidlTmp);
  FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);
  COMDLG32_SHFree(pidlTmp);

  COMDLG32_SHGetSpecialFolderLocation(0,CSIDL_DRIVES,&pidlDrives);

  COMDLG32_SHGetDesktopFolder(&psfRoot);

  if (psfRoot)
  {
    /* enumerate the contents of the desktop */
    if(SUCCEEDED(IShellFolder_EnumObjects(psfRoot, hwndCombo, SHCONTF_FOLDERS, &lpeRoot)))
    {
      while (S_OK == IEnumIDList_Next(lpeRoot, 1, &pidlTmp, NULL))
      {
	FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);

	/* special handling for CSIDL_DRIVES */
	if (COMDLG32_PIDL_ILIsEqual(pidlTmp, pidlDrives))
	{
	  if(SUCCEEDED(IShellFolder_BindToObject(psfRoot, pidlTmp, NULL, &IID_IShellFolder, (LPVOID*)&psfDrives)))
	  {
	    /* enumerate the drives */
	    if(SUCCEEDED(IShellFolder_EnumObjects(psfDrives, hwndCombo,SHCONTF_FOLDERS, &lpeDrives)))
	    {
	      while (S_OK == IEnumIDList_Next(lpeDrives, 1, &pidlTmp1, NULL))
	      {
	        pidlAbsTmp = COMDLG32_PIDL_ILCombine(pidlTmp, pidlTmp1);
	        FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlAbsTmp,LISTEND);
	        COMDLG32_SHFree(pidlAbsTmp);
	        COMDLG32_SHFree(pidlTmp1);
	      }
	      IEnumIDList_Release(lpeDrives);
	    }
	    IShellFolder_Release(psfDrives);
	  }
	}
        COMDLG32_SHFree(pidlTmp);
      }
      IEnumIDList_Release(lpeRoot);
    }
  }

  IShellFolder_Release(psfRoot);
  COMDLG32_SHFree(pidlDrives);
  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_DrawItem
 *
 * WM_DRAWITEM message handler
 */
static LRESULT FILEDLG95_LOOKIN_DrawItem(LPDRAWITEMSTRUCT pDIStruct)
{
  COLORREF crWin = GetSysColor(COLOR_WINDOW);
  COLORREF crHighLight = GetSysColor(COLOR_HIGHLIGHT);
  COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
  RECT rectText;
  RECT rectIcon;
  SHFILEINFOA sfi;
  HIMAGELIST ilItemImage;
  int iIndentation;
  LPSFOLDER tmpFolder;


  LookInInfos *liInfos = (LookInInfos *)GetPropA(pDIStruct->hwndItem,LookInInfosStr);

  TRACE("\n");

  if(pDIStruct->itemID == -1)
    return 0;

  if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(pDIStruct->hwndItem,
                            pDIStruct->itemID)))
    return 0;


  if(pDIStruct->itemID == liInfos->uSelectedItem)
  {
    ilItemImage = (HIMAGELIST) COMDLG32_SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                               0,    
                                               &sfi,    
                                               sizeof (SHFILEINFOA),   
                                               SHGFI_PIDL | SHGFI_SMALLICON |    
                                               SHGFI_OPENICON | SHGFI_SYSICONINDEX    | 
                                               SHGFI_DISPLAYNAME );   
  }
  else
  {
    ilItemImage = (HIMAGELIST) COMDLG32_SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                                  0, 
                                                  &sfi, 
                                                  sizeof (SHFILEINFOA),
                                                  SHGFI_PIDL | SHGFI_SMALLICON | 
                                                  SHGFI_SYSICONINDEX | 
                                                  SHGFI_DISPLAYNAME);
  }

  /* Is this item selected ?*/
  if(pDIStruct->itemState & ODS_SELECTED)
  {
    SetTextColor(pDIStruct->hDC,(0x00FFFFFF & ~(crText)));
    SetBkColor(pDIStruct->hDC,crHighLight);
    FillRect(pDIStruct->hDC,&pDIStruct->rcItem,(HBRUSH)crHighLight);
  }
  else
  {
    SetTextColor(pDIStruct->hDC,crText);
    SetBkColor(pDIStruct->hDC,crWin);
    FillRect(pDIStruct->hDC,&pDIStruct->rcItem,(HBRUSH)crWin);
  }

  /* Do not indent item  if drawing in the edit of the combo*/
  if(pDIStruct->itemState & ODS_COMBOBOXEDIT)
  {
    iIndentation = 0;
    ilItemImage = (HIMAGELIST) COMDLG32_SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                                0, 
                                                &sfi, 
                                                sizeof (SHFILEINFOA), 
                                                SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_OPENICON 
                                                | SHGFI_SYSICONINDEX | SHGFI_DISPLAYNAME  );

  }
  else
  {
    iIndentation = tmpFolder->m_iIndent;
  }
  /* Draw text and icon */

  /* Initialise the icon display area */
  rectIcon.left = pDIStruct->rcItem.left + ICONWIDTH/2 * iIndentation;
  rectIcon.top = pDIStruct->rcItem.top;
  rectIcon.right = rectIcon.left + ICONWIDTH;
  rectIcon.bottom = pDIStruct->rcItem.bottom;

  /* Initialise the text display area */
  rectText.left = rectIcon.right;
  rectText.top = pDIStruct->rcItem.top + YTEXTOFFSET;
  rectText.right = pDIStruct->rcItem.right + XTEXTOFFSET;
  rectText.bottom = pDIStruct->rcItem.bottom;

 
  /* Draw the icon from the image list */
  COMDLG32_ImageList_Draw(ilItemImage,
                 sfi.iIcon,
                 pDIStruct->hDC,  
                 rectIcon.left,  
                 rectIcon.top,  
                 ILD_TRANSPARENT );  

  /* Draw the associated text */
  if(sfi.szDisplayName)
    TextOutA(pDIStruct->hDC,rectText.left,rectText.top,sfi.szDisplayName,strlen(sfi.szDisplayName));


  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_OnCommand
 *
 * LookIn combo box WM_COMMAND message handler
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_LOOKIN_OnCommand(HWND hwnd, WORD wNotifyCode)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  switch(wNotifyCode)
  {
  case CBN_SELENDOK:
    {
      LPSFOLDER tmpFolder;
      int iItem; 

      iItem = CBGetCurSel(fodInfos->DlgInfos.hwndLookInCB);

      if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,
                                               iItem)))
	return FALSE;


      if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                              tmpFolder->pidlItem,
                                              SBSP_ABSOLUTE)))
      {
        return TRUE;
      }
      break;
    }
      
  }
  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_AddItem
 *
 * Adds an absolute pidl item to the lookin combo box
 * returns the index of the inserted item
 */
static int FILEDLG95_LOOKIN_AddItem(HWND hwnd,LPITEMIDLIST pidl, int iInsertId)
{
  LPITEMIDLIST pidlNext;
  SHFILEINFOA sfi;
  SFOLDER *tmpFolder;
  LookInInfos *liInfos;

  TRACE("\n");

  if(!pidl)
    return -1;

  if(!(liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr)))
    return -1;
    
  tmpFolder = MemAlloc(sizeof(SFOLDER));
  tmpFolder->m_iIndent = 0;

  /* Calculate the indentation of the item in the lookin*/
  pidlNext = pidl;
  while( (pidlNext=COMDLG32_PIDL_ILGetNext(pidlNext)) )
  {
    tmpFolder->m_iIndent++;
  }

  tmpFolder->pidlItem = COMDLG32_PIDL_ILClone(pidl);

  if(tmpFolder->m_iIndent > liInfos->iMaxIndentation)
    liInfos->iMaxIndentation = tmpFolder->m_iIndent;
  
  COMDLG32_SHGetFileInfoA((LPSTR)pidl,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX 
                  | SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_ATTRIBUTES);


  if((sfi.dwAttributes & SFGAO_FILESYSANCESTOR) || (sfi.dwAttributes & SFGAO_FILESYSTEM))
    {
        int iItemID;
    /* Add the item at the end of the list */
    if(iInsertId < 0)
    {
      iItemID = CBAddString(hwnd,sfi.szDisplayName);
    }
    /* Insert the item at the iInsertId position*/
    else
    {
      iItemID = CBInsertString(hwnd,sfi.szDisplayName,iInsertId);
    }

        CBSetItemDataPtr(hwnd,iItemID,tmpFolder);
        return iItemID;
  }

  MemFree( tmpFolder );
  return -1;

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_InsertItemAfterParent
 *
 * Insert an item below its parent 
 */
static int FILEDLG95_LOOKIN_InsertItemAfterParent(HWND hwnd,LPITEMIDLIST pidl)
{
  
  LPITEMIDLIST pidlParent = GetParentPidl(pidl);
  int iParentPos;

  TRACE("\n");

  iParentPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)pidlParent,SEARCH_PIDL);

  if(iParentPos < 0)
  {
    iParentPos = FILEDLG95_LOOKIN_InsertItemAfterParent(hwnd,pidlParent);
  }

  /* Free pidlParent memory */
  COMDLG32_SHFree((LPVOID)pidlParent);

  return FILEDLG95_LOOKIN_AddItem(hwnd,pidl,iParentPos + 1);
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_SelectItem
 *
 * Adds an absolute pidl item to the lookin combo box
 * returns the index of the inserted item
 */
int FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl)
{
  int iItemPos;
  LookInInfos *liInfos;

  TRACE("\n");

  iItemPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)pidl,SEARCH_PIDL);

  liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr);

  if(iItemPos < 0)
  {
    while(FILEDLG95_LOOKIN_RemoveMostExpandedItem(hwnd) > -1);
    iItemPos = FILEDLG95_LOOKIN_InsertItemAfterParent(hwnd,pidl);
  }

  else
  {
    SFOLDER *tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,iItemPos);
    while(liInfos->iMaxIndentation > tmpFolder->m_iIndent)
    {
      int iRemovedItem;

      if(-1 == (iRemovedItem = FILEDLG95_LOOKIN_RemoveMostExpandedItem(hwnd)))
        break;
      if(iRemovedItem < iItemPos)
        iItemPos--;
    }
  }
  
  CBSetCurSel(hwnd,iItemPos);
  liInfos->uSelectedItem = iItemPos;

  return 0;

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_RemoveMostExpandedItem
 *
 * Remove the item with an expansion level over iExpansionLevel
 */
static int FILEDLG95_LOOKIN_RemoveMostExpandedItem(HWND hwnd)
{
  int iItemPos;

  LookInInfos *liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr);

  TRACE("\n");

  if(liInfos->iMaxIndentation <= 2)
    return -1;

  if((iItemPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)liInfos->iMaxIndentation,SEARCH_EXP)) >=0)
  {
    SFOLDER *tmpFolder;
    tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,iItemPos);
    CBDeleteString(hwnd,iItemPos);
    liInfos->iMaxIndentation--;

    return iItemPos;
  }

  return -1;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_SearchItem
 *
 * Search for pidl in the lookin combo box
 * returns the index of the found item
 */
static int FILEDLG95_LOOKIN_SearchItem(HWND hwnd,WPARAM searchArg,int iSearchMethod)
{
  int i = 0;
  int iCount = CBGetCount(hwnd);

  TRACE("\n");

  for(;i<iCount;i++)
  {
    LPSFOLDER tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,i);

    if(iSearchMethod == SEARCH_PIDL && COMDLG32_PIDL_ILIsEqual((LPITEMIDLIST)searchArg,tmpFolder->pidlItem))
      return i;
    if(iSearchMethod == SEARCH_EXP && tmpFolder->m_iIndent == (int)searchArg)
      return i;

  }

  return -1;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_Clean
 *
 * Clean the memory used by the lookin combo box
 */
static void FILEDLG95_LOOKIN_Clean(HWND hwnd)
{
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
    int iPos;
    int iCount = CBGetCount(fodInfos->DlgInfos.hwndLookInCB);

    TRACE("\n");

    /* Delete each string of the combo and their associated data */
    for(iPos = iCount-1;iPos>=0;iPos--)
    {
        MemFree((LPVOID)(CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,iPos)));
        CBDeleteString(fodInfos->DlgInfos.hwndLookInCB,iPos);
    }
    /* LookInInfos structure */
    RemovePropA(fodInfos->DlgInfos.hwndLookInCB,LookInInfosStr);

}
/*
 * TOOLS
 */

/***********************************************************************
 *      GetName
 *
 * Get the pidl's display name (relative to folder) and 
 * put it in lpstrFileName.
 * 
 * Return NOERROR on success,
 * E_FAIL otherwise
 */

HRESULT GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPSTR lpstrFileName)
{
  STRRET str;
  HRESULT hRes;

  TRACE("%p %p\n", lpsf, pidl);

  if(!lpsf)
  {
    HRESULT hRes;
    COMDLG32_SHGetDesktopFolder(&lpsf);
    hRes = GetName(lpsf,pidl,dwFlags,lpstrFileName);
    IShellFolder_Release(lpsf);
    return hRes;
  }

  /* Get the display name of the pidl relative to the folder */
  if (SUCCEEDED(hRes = IShellFolder_GetDisplayNameOf(lpsf,
                                                     pidl, 
                                                     dwFlags, 
                                                     &str)))
  {
      return StrRetToBufA(&str, pidl,lpstrFileName, MAX_PATH);
  }
  return E_FAIL;
}

/***********************************************************************
 *      GetShellFolderFromPidl
 *
 * pidlRel is the item pidl relative 
 * Return the IShellFolder of the absolute pidl
 */
IShellFolder *GetShellFolderFromPidl(LPITEMIDLIST pidlAbs)
{
  IShellFolder *psf = NULL,*psfParent;

  TRACE("%p\n", pidlAbs);

  if(SUCCEEDED(COMDLG32_SHGetDesktopFolder(&psfParent)))
  {
    psf = psfParent;
    if(pidlAbs && pidlAbs->mkid.cb)
    {
      if(SUCCEEDED(IShellFolder_BindToObject(psfParent, pidlAbs, NULL, &IID_IShellFolder, (LPVOID*)&psf)))
      {
	IShellFolder_Release(psfParent);
        return psf;
      }
    }
    /* return the desktop */
    return psfParent;
  }
  return NULL;
}

/***********************************************************************
 *      GetParentPidl
 *
 * Return the LPITEMIDLIST to the parent of the pidl in the list
 */
LPITEMIDLIST GetParentPidl(LPITEMIDLIST pidl)
{
  LPITEMIDLIST pidlParent;

  TRACE("%p\n", pidl);

  pidlParent = COMDLG32_PIDL_ILClone(pidl);
  COMDLG32_PIDL_ILRemoveLastID(pidlParent);
     
  return pidlParent;
}

/***********************************************************************
 *      GetPidlFromName
 *
 * returns the pidl of the file name relative to folder 
 * NULL if an error occured
 */
LPITEMIDLIST GetPidlFromName(IShellFolder *psf,LPCSTR lpcstrFileName)
{
  LPITEMIDLIST pidl;
  ULONG ulEaten;
  wchar_t lpwstrDirName[MAX_PATH];


  TRACE("sf=%p file=%s\n", psf, lpcstrFileName);

  if(!lpcstrFileName)
    return NULL;
    
  MultiByteToWideChar(CP_ACP,
                      MB_PRECOMPOSED,   
                      lpcstrFileName,  
                      -1,  
                      (LPWSTR)lpwstrDirName,  
                      MAX_PATH);  

  IShellFolder_ParseDisplayName(psf,                                0,
                                             NULL,
                                             (LPWSTR)lpwstrDirName,
                                             &ulEaten,
                                             &pidl,
                                NULL);   

    return pidl;
}

/***********************************************************************
 *      GetFileExtension
 *
 */
BOOL GetFileExtension(IShellFolder *psf,LPITEMIDLIST pidl,LPSTR lpstrFileExtension)
{
  char FileName[MAX_PATH];
  int result;
  char *pdest;
  int ch = '.';

  if(SUCCEEDED(GetName(psf,pidl,SHGDN_NORMAL,FileName)))
  {
    if(!(pdest = strrchr( FileName, ch )))
      return FALSE;

    result = pdest - FileName + 1;
    strcpy(lpstrFileExtension,&FileName[result]);
    return TRUE;
  }
  return FALSE;
}

/*
 * Memory allocation methods */
void *MemAlloc(UINT size)
{
    return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
}

void MemFree(void *mem)
{
    if(mem)
    {
        HeapFree(GetProcessHeap(),0,mem);
    }
}
