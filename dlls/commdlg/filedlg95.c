/*
 * COMMDLG - File Open Dialogs Win95 look and feel
 *
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "ldt.h"
#include "heap.h"
#include "commdlg.h"
#include "dlgs.h"
#include "debugtools.h"
#include "cderr.h"
#include "tweak.h"
#include "winnls.h"
#include "shellapi.h"
#include "commctrl.h"
#include "tchar.h"
#include "filedlgbrowser.h"
#include "wine/undocshell.h"
#include "wine/obj_contextmenu.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

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
static BOOL    FILEDLG95_SHELL_NewFolder(HWND hwnd);
       BOOL    FILEDLG95_SHELL_FillIncludedItemList(HWND hwnd,
                                                        LPITEMIDLIST pidlCurrentFolder,
                                                        LPSTR lpstrMask);
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

BOOL WINAPI GetOpenFileName95(FileOpenDlgInfos *fodInfos);
BOOL WINAPI GetSaveFileName95(FileOpenDlgInfos *fodInfos);
HRESULT WINAPI FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/***********************************************************************
 *      GetOpenFileName95
 *
 * Creates an Open common dialog box that lets the user select 
 * the drive, directory, and the name of a file or set of files to open.
 *
 * IN  : The FileOpenDlgInfos structure associated with the dialog
 * OUT : TRUE on success
 *       FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 */
BOOL WINAPI GetOpenFileName95(FileOpenDlgInfos *fodInfos)
{

    LRESULT lRes;
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;

    /* Create the dialog from a template */

  if (fodInfos->ofnInfos.Flags & OFN_ENABLETEMPLATEHANDLE)
  {
    if (!(template = LockResource( MapHModuleSL(fodInfos->ofnInfos.hInstance ))))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
  }
  else if (fodInfos->ofnInfos.Flags & OFN_ENABLETEMPLATE)
  {
    if (!(hRes = FindResourceA(MapHModuleSL(fodInfos->ofnInfos.hInstance),
            (fodInfos->ofnInfos.lpTemplateName), RT_DIALOGA)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(hDlgTmpl = LoadResource( MapHModuleSL(fodInfos->ofnInfos.hInstance),
             hRes )) ||
        !(template = LockResource( hDlgTmpl )))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
  }
  else
  {
    if(!(hRes = FindResourceA(COMMDLG_hInstance32,MAKEINTRESOURCEA(IDD_OPENDIALOG),RT_DIALOGA)))
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
  }

    lRes = DialogBoxIndirectParamA(COMMDLG_hInstance32,
                                  (LPDLGTEMPLATEA) template,
                                  fodInfos->ofnInfos.hwndOwner,
                                  (DLGPROC) FileOpenDlgProc95,
                                  (LPARAM) fodInfos);

    /* Unable to create the dialog*/
    if( lRes == -1)
        return FALSE;
    
    return lRes;
}

/***********************************************************************
 *      GetSaveFileName95
 *
 * Creates an Save as common dialog box that lets the user select
 * the drive, directory, and the name of a file or set of files to open.
 *
 * IN  : The FileOpenDlgInfos structure associated with the dialog
 * OUT : TRUE on success
 *       FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 */
BOOL WINAPI GetSaveFileName95(FileOpenDlgInfos *fodInfos)
{

    LRESULT lRes;
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;

    /* Create the dialog from a template */

  if (fodInfos->ofnInfos.Flags & OFN_ENABLETEMPLATEHANDLE)
  {
    if (!(template = LockResource( MapHModuleSL(fodInfos->ofnInfos.hInstance ))))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
  }
  else if (fodInfos->ofnInfos.Flags & OFN_ENABLETEMPLATE)
  {
    if (!(hRes = FindResourceA(MapHModuleSL(fodInfos->ofnInfos.hInstance),
            (fodInfos->ofnInfos.lpTemplateName), RT_DIALOGA)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(hDlgTmpl = LoadResource( MapHModuleSL(fodInfos->ofnInfos.hInstance),
             hRes )) ||
        !(template = LockResource( hDlgTmpl )))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
  }
  else
  {
    if(!(hRes = FindResourceA(COMMDLG_hInstance32,MAKEINTRESOURCEA(IDD_SAVEDIALOG),RT_DIALOGA)))
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
  }
    lRes = DialogBoxIndirectParamA(COMMDLG_hInstance32,
                                  (LPDLGTEMPLATEA) template,
                                  fodInfos->ofnInfos.hwndOwner,
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
 * Call GetOpenFileName95 with this structure and clean the memory.
 *
 * IN  : The OPENFILENAMEA initialisation structure passed to
 *       GetOpenFileNameA win api function (see filedlg.c)
 */
BOOL  WINAPI GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType)
{

  BOOL ret;
  FileOpenDlgInfos *fodInfos;
  
  /* Initialise FileOpenDlgInfos structure*/  
  fodInfos = (FileOpenDlgInfos*)MemAlloc(sizeof(FileOpenDlgInfos));
  memset(&fodInfos->ofnInfos,'\0',sizeof(*ofn)); fodInfos->ofnInfos.lStructSize = sizeof(*ofn);
  fodInfos->ofnInfos.hwndOwner = ofn->hwndOwner;
  fodInfos->ofnInfos.hInstance = MapHModuleLS(ofn->hInstance);
  if (ofn->lpstrFilter)
  {
    LPSTR s,x;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPSTR)ofn->lpstrFilter;
    while (*s)
      s = s+strlen(s)+1;
    s++;
    x = (LPSTR)MemAlloc(s-ofn->lpstrFilter);
    memcpy(x,ofn->lpstrFilter,s-ofn->lpstrFilter);
    fodInfos->ofnInfos.lpstrFilter = (LPSTR)x;
  }
  if (ofn->lpstrCustomFilter)
  {
    LPSTR s,x;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = (LPSTR)ofn->lpstrCustomFilter;
    while (*s)
      s = s+strlen(s)+1;
    s++;
    x = MemAlloc(s-ofn->lpstrCustomFilter);
    memcpy(x,ofn->lpstrCustomFilter,s-ofn->lpstrCustomFilter);
    fodInfos->ofnInfos.lpstrCustomFilter = (LPSTR)x;
  }
  fodInfos->ofnInfos.nMaxCustFilter = ofn->nMaxCustFilter;
  if(ofn->nFilterIndex)
    fodInfos->ofnInfos.nFilterIndex = --ofn->nFilterIndex;
  if (ofn->nMaxFile)
  {
      fodInfos->ofnInfos.lpstrFile = (LPSTR)MemAlloc(ofn->nMaxFile);
      strcpy((LPSTR)fodInfos->ofnInfos.lpstrFile,ofn->lpstrFile);
  }
  fodInfos->ofnInfos.nMaxFile = ofn->nMaxFile;
  fodInfos->ofnInfos.nMaxFileTitle = ofn->nMaxFileTitle;
    if (fodInfos->ofnInfos.nMaxFileTitle)
      fodInfos->ofnInfos.lpstrFileTitle = (LPSTR)MemAlloc(ofn->nMaxFileTitle);
  if (ofn->lpstrInitialDir)
  {
      fodInfos->ofnInfos.lpstrInitialDir = (LPSTR)MemAlloc(strlen(ofn->lpstrInitialDir));
      strcpy((LPSTR)fodInfos->ofnInfos.lpstrInitialDir,ofn->lpstrInitialDir);
  }

  if (ofn->lpstrTitle)
  {
      fodInfos->ofnInfos.lpstrTitle = (LPSTR)MemAlloc(strlen(ofn->lpstrTitle));
      strcpy((LPSTR)fodInfos->ofnInfos.lpstrTitle,ofn->lpstrTitle);
  }

  fodInfos->ofnInfos.Flags = ofn->Flags|OFN_WINE;
  fodInfos->ofnInfos.nFileOffset = ofn->nFileOffset;
  fodInfos->ofnInfos.nFileExtension = ofn->nFileExtension;
  if (ofn->lpstrDefExt)
  {
      fodInfos->ofnInfos.lpstrDefExt = MemAlloc(strlen(ofn->lpstrDefExt));
      strcpy((LPSTR)fodInfos->ofnInfos.lpstrDefExt,ofn->lpstrDefExt);
  }
  fodInfos->ofnInfos.lCustData = ofn->lCustData;
  fodInfos->ofnInfos.lpfnHook = (LPOFNHOOKPROC)ofn->lpfnHook;

  if (ofn->lpTemplateName)
  {
      /* template don't work - using normal dialog */  
      /* fodInfos->ofnInfos.lpTemplateName = MemAlloc(strlen(ofn->lpTemplateName));
	strcpy((LPSTR)fodInfos->ofnInfos.lpTemplateName,ofn->lpTemplateName);*/
      fodInfos->ofnInfos.Flags &= ~OFN_ENABLETEMPLATEHANDLE;
      fodInfos->ofnInfos.Flags &= ~OFN_ENABLETEMPLATE;
      FIXME("File dialog 95 template not implemented\n");
      
  }

  /* Replace the NULL lpstrInitialDir by the current folder */
  if(!ofn->lpstrInitialDir)
  {
    fodInfos->ofnInfos.lpstrInitialDir = MemAlloc(MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH,(LPSTR)fodInfos->ofnInfos.lpstrInitialDir);
  }

  switch(iDlgType)
  {
  case OPEN_DIALOG :
      ret = GetOpenFileName95(fodInfos);
      break;
  case SAVE_DIALOG :
      ret = GetSaveFileName95(fodInfos);
      break;
  default :
      ret = 0;
  }

  ofn->nFileOffset = fodInfos->ofnInfos.nFileOffset;
  ofn->nFileExtension = fodInfos->ofnInfos.nFileExtension;
  if (fodInfos->ofnInfos.lpstrFilter)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrFilter));
  if (fodInfos->ofnInfos.lpTemplateName)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpTemplateName));
  if (fodInfos->ofnInfos.lpstrDefExt)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrDefExt));
  if (fodInfos->ofnInfos.lpstrTitle)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrTitle));
  if (fodInfos->ofnInfos.lpstrInitialDir)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrInitialDir));
  if (fodInfos->ofnInfos.lpstrCustomFilter)
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrCustomFilter));

  if (fodInfos->ofnInfos.lpstrFile) 
    {
      strcpy(ofn->lpstrFile,fodInfos->ofnInfos.lpstrFile);
      MemFree((LPVOID)fodInfos->ofnInfos.lpstrFile);
    }
  if (fodInfos->ofnInfos.lpstrFileTitle) 
    {
      if (ofn->lpstrFileTitle)
          strcpy(ofn->lpstrFileTitle,
                 fodInfos->ofnInfos.lpstrFileTitle);
      MemFree((LPVOID)fodInfos->ofnInfos.lpstrFileTitle);
      }

  MemFree((LPVOID)(fodInfos));
  return ret;
}

/***********************************************************************
 *      GetFileDialog95W
 *
 * Copy the OPENFILENAMEW structure in a FileOpenDlgInfos structure.
 * Call GetOpenFileName95 with this structure and clean the memory.
 *
 * IN  : The OPENFILENAMEW initialisation structure passed to
 *       GetOpenFileNameW win api function (see filedlg.c)
 */
BOOL  WINAPI GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos *fodInfos;

  /* Initialise FileOpenDlgInfos structure*/
  fodInfos = (FileOpenDlgInfos*)MemAlloc(sizeof(FileOpenDlgInfos));
  memset(&fodInfos->ofnInfos,'\0',sizeof(*ofn));
  fodInfos->ofnInfos.lStructSize = sizeof(*ofn);
  fodInfos->ofnInfos.hwndOwner = ofn->hwndOwner;
  fodInfos->ofnInfos.hInstance = MapHModuleLS(ofn->hInstance);
  if (ofn->lpstrFilter)
  {
    LPWSTR  s;
    LPSTR x,y;
    int n;

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
    fodInfos->ofnInfos.lpstrFilter = (LPSTR)y;
  }
  if (ofn->lpstrCustomFilter) {
    LPWSTR  s;
    LPSTR x,y;
    int n;

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
    fodInfos->ofnInfos.lpstrCustomFilter = (LPSTR)y;
  }
  fodInfos->ofnInfos.nMaxCustFilter = ofn->nMaxCustFilter;
  fodInfos->ofnInfos.nFilterIndex = ofn->nFilterIndex;
  if (ofn->nMaxFile)
     fodInfos->ofnInfos.lpstrFile = (LPSTR)MemAlloc(ofn->nMaxFile);
  fodInfos->ofnInfos.nMaxFile = ofn->nMaxFile;
  fodInfos->ofnInfos.nMaxFileTitle = ofn->nMaxFileTitle;
  if (ofn->nMaxFileTitle)
    fodInfos->ofnInfos.lpstrFileTitle = (LPSTR)MemAlloc(ofn->nMaxFileTitle);
  if (ofn->lpstrInitialDir)
    fodInfos->ofnInfos.lpstrInitialDir = (LPSTR)MemAlloc(lstrlenW(ofn->lpstrInitialDir));
  if (ofn->lpstrTitle)
    fodInfos->ofnInfos.lpstrTitle = (LPSTR)MemAlloc(lstrlenW(ofn->lpstrTitle));
  fodInfos->ofnInfos.Flags = ofn->Flags|OFN_WINE|OFN_UNICODE;
  fodInfos->ofnInfos.nFileOffset = ofn->nFileOffset;
  fodInfos->ofnInfos.nFileExtension = ofn->nFileExtension;
  if (ofn->lpstrDefExt)
    fodInfos->ofnInfos.lpstrDefExt = (LPSTR)MemAlloc(lstrlenW(ofn->lpstrDefExt));
  fodInfos->ofnInfos.lCustData = ofn->lCustData;
  fodInfos->ofnInfos.lpfnHook = (LPOFNHOOKPROC)ofn->lpfnHook;
  if (ofn->lpTemplateName) 
    fodInfos->ofnInfos.lpTemplateName = (LPSTR)MemAlloc(lstrlenW(ofn->lpTemplateName));
  switch(iDlgType)
  {
  case OPEN_DIALOG :
      ret = GetOpenFileName95(fodInfos);
      break;
  case SAVE_DIALOG :
      ret = GetSaveFileName95(fodInfos);
      break;
  default :
      ret = 0;
  }
      

  /* Cleaning */
  ofn->nFileOffset = fodInfos->ofnInfos.nFileOffset;
  ofn->nFileExtension = fodInfos->ofnInfos.nFileExtension;
  if (fodInfos->ofnInfos.lpstrFilter)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpstrFilter));
  if (fodInfos->ofnInfos.lpTemplateName)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpTemplateName));
  if (fodInfos->ofnInfos.lpstrDefExt)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpstrDefExt));
  if (fodInfos->ofnInfos.lpstrTitle)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpstrTitle));
  if (fodInfos->ofnInfos.lpstrInitialDir)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpstrInitialDir));
  if (fodInfos->ofnInfos.lpstrCustomFilter)
    MemFree((LPVOID)(fodInfos->ofnInfos.lpstrCustomFilter));

  if (fodInfos->ofnInfos.lpstrFile) {
  lstrcpyAtoW(ofn->lpstrFile,(fodInfos->ofnInfos.lpstrFile));
  MemFree((LPVOID)(fodInfos->ofnInfos.lpstrFile));
  }

  if (fodInfos->ofnInfos.lpstrFileTitle) {
      if (ofn->lpstrFileTitle)
                lstrcpyAtoW(ofn->lpstrFileTitle,
      (fodInfos->ofnInfos.lpstrFileTitle));
      MemFree((LPVOID)(fodInfos->ofnInfos.lpstrFileTitle));
  }
  MemFree((LPVOID)(fodInfos));
  return ret;

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
      return FILEDLG95_OnWMInitDialog(hwnd, wParam, lParam);
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
    default :
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

  /* Adds the FileOpenDlgInfos in the property list of the dialog 
     so it will be easily accessible through a GetPropA(...) */
  SetPropA(hwnd, FileOpenDlgInfosStr, (HANDLE) fodInfos);

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

  if(!(pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder,fodInfos->ofnInfos.lpstrInitialDir)))
  {
    char path[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH,path);
    pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder,
                     path);

  }

  /* Browse to the initial directory */
  IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,pidlItemId,SBSP_RELATIVE);

  /* Free pidlItem memory */
  SHFree(pidlItemId);

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
  WORD wNotifyCode = HIWORD(wParam); // notification code 
  WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier

  switch(wID)
  {
    /* OK button */
  case IDOK:
      FILEDLG95_OnOpen(hwnd);
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
    /* Up folder button */
  case IDC_UPFOLDER:
    FILEDLG95_SHELL_UpFolder(hwnd);
    break;
    /* List option button */
  case IDC_LIST:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWLIST);
    break;
    /* Details option button */
  case IDC_DETAILS:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWDETAILS);
    break;
    /* New folder button */
  case IDC_NEWFOLDER:
    FILEDLG95_SHELL_NewFolder(hwnd);
    break;

  case IDC_FILENAME:
    break;

  }

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
  HIMAGELIST himlToolbar;
  HICON hicon;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  /* Get the hwnd of the controls */
  fodInfos->DlgInfos.hwndFileName = GetDlgItem(hwnd,IDC_FILENAME);
  fodInfos->DlgInfos.hwndFileTypeCB = GetDlgItem(hwnd,IDC_FILETYPE);
  fodInfos->DlgInfos.hwndLookInCB = GetDlgItem(hwnd,IDC_LOOKIN);

    ShowWindow(GetDlgItem(hwnd,IDC_SHELLSTATIC),SW_HIDE);
  /* Load the icons bitmaps */

  if((himlToolbar = COMDLG32_ImageList_LoadImageA(COMMDLG_hInstance32,
                                     MAKEINTRESOURCEA(IDB_TOOLBAR),
                                     0,
                                     1,
                                     CLR_DEFAULT,
                                     IMAGE_BITMAP,
                                     0)))
  {
    /* Up folder icon */
    if((hicon = COMDLG32_ImageList_GetIcon(himlToolbar,0,ILD_NORMAL)))
      SendDlgItemMessageA(hwnd,IDC_UPFOLDER,BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
    /* New folder icon */
    if((hicon = COMDLG32_ImageList_GetIcon(himlToolbar,1,ILD_NORMAL)))
      SendDlgItemMessageA(hwnd,IDC_NEWFOLDER,BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
    /* List view icon */
    if((hicon = COMDLG32_ImageList_GetIcon(himlToolbar,2,ILD_NORMAL)))
      SendDlgItemMessageA(hwnd,IDC_LIST,BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
    /* Detail view icon */
    if((hicon = COMDLG32_ImageList_GetIcon(himlToolbar,3,ILD_NORMAL)))
      SendDlgItemMessageA(hwnd,IDC_DETAILS,BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
  }



  /* Set the window text with the text specified in the OPENFILENAME structure */
  if(fodInfos->ofnInfos.lpstrTitle)
    SetWindowTextA(hwnd,fodInfos->ofnInfos.lpstrTitle);

  /* Initialise the file name edit control */
  if(strlen(fodInfos->ofnInfos.lpstrFile))
  {
      SetDlgItemTextA(hwnd,IDC_FILENAME,fodInfos->ofnInfos.lpstrFile);
  }
  /* Must the open as read only check box be checked ?*/
  if(fodInfos->ofnInfos.Flags & OFN_READONLY)
  {
    SendDlgItemMessageA(hwnd,IDC_OPENREADONLY,BM_SETCHECK,(WPARAM)TRUE,0);
  }
  /* Must the open as read only check box be hid ?*/
  if(fodInfos->ofnInfos.Flags & OFN_HIDEREADONLY)
  {
    ShowWindow(GetDlgItem(hwnd,IDC_OPENREADONLY),SW_HIDE);
  }

  return 0;
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
  char lpstrFileName[MAX_PATH];
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(GetDlgItemTextA(hwnd,IDC_FILENAME,lpstrFileName,MAX_PATH))
  {
    char *tmp;
    char lpstrFile[MAX_PATH];

    /* Get the selected file name and path */
    SHGetPathFromIDListA(fodInfos->ShellInfos.pidlAbsCurrent,
                         lpstrFile);
    if(strcmp(&lpstrFile[strlen(lpstrFile)-1],"\\"))
        strcat(lpstrFile,"\\");
    strcat(lpstrFile,lpstrFileName);

    /* Check if this is a search */
    if(strchr(lpstrFileName,'*') || strchr(lpstrFileName,'?'))
    {
      int iPos;

       /* Set the current filter with the current selection */
      if(fodInfos->ShellInfos.lpstrCurrentFilter)
         MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

      fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc((strlen(lpstrFileName)+1)*2);
      lstrcpyAtoW(fodInfos->ShellInfos.lpstrCurrentFilter,(LPSTR)strlwr((LPSTR)lpstrFileName));

      
      IShellView_Refresh(fodInfos->Shell.FOIShellView);

      if(-1 < (iPos = FILEDLG95_FILETYPE_SearchExt(fodInfos->DlgInfos.hwndFileTypeCB,lpstrFileName)))
        CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB,iPos);

      return FALSE;
    }

    /* Check file extension */

    if(!strrchr(lpstrFile,'.'))
    {
        /* if the file has no extension, append the selected
           extension of the filetype combo box */
        int iExt; 
        LPSTR lpstrExt; 
        LPSTR lpstrTmp;
        iExt = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);
        lpstrTmp = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iExt);

        if((lpstrExt = strchr(lpstrTmp,';')))
        {
            int i = lpstrExt - lpstrTmp;
            lpstrExt = MemAlloc(i);
            strncpy(lpstrExt,&lpstrTmp[1],i-1);
        }
        else
        {
            lpstrExt = MemAlloc(strlen(lpstrTmp)-1);
            strcpy(lpstrExt,&lpstrTmp[1]);
        }

        if(!strcmp(&lpstrExt[1],"*") && fodInfos->ofnInfos.lpstrDefExt)
        {
            lpstrExt = MemAlloc(strlen(fodInfos->ofnInfos.lpstrDefExt)+1);
            strcat(lpstrExt,".");
            strcat(lpstrExt,(LPSTR) fodInfos->ofnInfos.lpstrDefExt);
        }

        strcat(lpstrFile,lpstrExt);
    }
    /* Check if the selected file exist */

    if(strlen(lpstrFile) > fodInfos->ofnInfos.nMaxFile)
    {
        /* set error FNERR_BUFFERTOSMALL */
        FILEDLG95_Clean(hwnd);
        return EndDialog(hwnd,FALSE);
    }
    strcpy(fodInfos->ofnInfos.lpstrFile,lpstrFile);

    /* Set the lpstrFileTitle of the OPENFILENAME structure */
    if(fodInfos->ofnInfos.lpstrFileTitle)
      strncpy(fodInfos->ofnInfos.lpstrFileTitle,
              lpstrFileName,
              fodInfos->ofnInfos.nMaxFileTitle);

    /* Check if the file is to be opened as read only */
    if(BST_CHECKED == SendDlgItemMessageA(hwnd,IDC_OPENREADONLY,BM_GETSTATE,0,0))
      SetFileAttributesA(fodInfos->ofnInfos.lpstrFile,FILE_ATTRIBUTE_READONLY);

    /*  nFileExtension and nFileOffset of OPENFILENAME structure */
    tmp = strrchr(fodInfos->ofnInfos.lpstrFile,'\\');
    fodInfos->ofnInfos.nFileOffset = tmp - fodInfos->ofnInfos.lpstrFile + 1;
    tmp = strrchr(fodInfos->ofnInfos.lpstrFile,'.');
    fodInfos->ofnInfos.nFileExtension = tmp - fodInfos->ofnInfos.lpstrFile + 1;

    /* Check if selected file exists */
    if(!GetPidlFromName(fodInfos->Shell.FOIShellFolder, lpstrFileName))
    {
      /* Tell the user the selected does not exist */
      if(fodInfos->ofnInfos.Flags & OFN_FILEMUSTEXIST)
      {
        char lpstrNotFound[100];
        char lpstrMsg[100];
        char tmp[400];

        LoadStringA(COMMDLG_hInstance32,IDS_FILENOTFOUND,lpstrNotFound,100);
        LoadStringA(COMMDLG_hInstance32,IDS_VERIFYFILE,lpstrMsg,100);

        strcpy(tmp,fodInfos->ofnInfos.lpstrFile);
        strcat(tmp,"\n");
        strcat(tmp,lpstrNotFound);
        strcat(tmp,"\n");
        strcat(tmp,lpstrMsg);

        MessageBoxA(hwnd,tmp,fodInfos->ofnInfos.lpstrTitle,MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
      }
      /* Ask the user if he wants to create the file*/
      if(fodInfos->ofnInfos.Flags & OFN_CREATEPROMPT)
      {
        char tmp[100];

        LoadStringA(COMMDLG_hInstance32,IDS_CREATEFILE,tmp,100);

        if(IDYES == MessageBoxA(hwnd,tmp,fodInfos->ofnInfos.lpstrTitle,MB_YESNO | MB_ICONQUESTION))
        {
            /* Create the file, clean and exit */
            FILEDLG95_Clean(hwnd);
            return EndDialog(hwnd,TRUE);
        }
	return FALSE;        
      }
    }
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
  if(FAILED(SHGetDesktopFolder(&fodInfos->Shell.FOIShellFolder)))
    return E_FAIL;

  /*ShellInfos */
  fodInfos->ShellInfos.hwndOwner = hwnd;

  fodInfos->ShellInfos.folderSettings.fFlags = FWF_AUTOARRANGE | FWF_ALIGNLEFT;
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
  CMINVOKECOMMANDINFO ci;
  TRACE("\n");

  if(SUCCEEDED(IShellView_GetItemObject(fodInfos->Shell.FOIShellView,
					SVGIO_BACKGROUND,
					&IID_IContextMenu,
					(LPVOID*)&pcm)))
  {
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
 *      FILEDLG95_SHELL_NewFolder
 *
 * Creates a new directory with New folder as name 
 * If the function succeeds, the return value is nonzero.
 * FIXME: let the contextmenu (CMDSTR_NEWFOLDER) do this thing
 */
static BOOL FILEDLG95_SHELL_NewFolder(HWND hwnd)
{
  char lpstrDirName[MAX_PATH] = "New folder";
  BOOL bRes;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if((bRes = CreateDirectoryA(lpstrDirName,NULL)))
  {
    LPITEMIDLIST pidl = GetPidlFromName(fodInfos->Shell.FOIShellFolder,lpstrDirName);
    IShellView_Refresh(fodInfos->Shell.FOIShellView);
    IShellView_SelectItem(fodInfos->Shell.FOIShellView,
                pidl,
                        (SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE
                |SVSI_FOCUSED|SVSI_SELECT));
  }



  return bRes;
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

  if(fodInfos->ofnInfos.lpstrFilter)
  {
    int iStrIndex = 0;
    int iPos = 0;
    LPSTR lpstrFilter;
    LPSTR lpstrTmp;

    for(;;)
    {
      /* filter is a list...  title\0ext\0......\0\0 */
      /* Set the combo item text to the title and the item data
         to the ext */
      char *lpstrExt = NULL;
      LPSTR lpstrExtTmp = NULL;
      /* Get the title */
      lpstrTmp = (&((LPBYTE)fodInfos->ofnInfos.lpstrFilter)[iStrIndex]);
      if(!strlen(lpstrTmp))
        break;
      iStrIndex += strlen(lpstrTmp) +1;
      /* Get the extension */
      lpstrExtTmp = (&((LPBYTE)fodInfos->ofnInfos.lpstrFilter)[iStrIndex]);
      if(!lpstrExtTmp)
          break;

      lpstrExt = (LPSTR) MemAlloc(strlen(lpstrExtTmp));
      if(!lpstrExt)
          break;
      
      strcpy(lpstrExt,lpstrExtTmp);

      iStrIndex += strlen(lpstrExt) +1;
            
      /* Add the item at the end of the combo */
      CBAddString(fodInfos->DlgInfos.hwndFileTypeCB,lpstrTmp);
      CBSetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iPos++,lpstrExt);
    }
    /* Set the current filter to the one specified
       in the initialisation structure */
    CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB,
                fodInfos->ofnInfos.nFilterIndex);

    lpstrFilter = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                           fodInfos->ofnInfos.nFilterIndex);
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
    case CBN_CLOSEUP:
    {
      LPSTR lpstrFilter;

      /* Get the current item of the filetype combo box */
      int iItem = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);

      /* Set the current filter with the current selection */
      if(fodInfos->ShellInfos.lpstrCurrentFilter)
         MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

      lpstrFilter = (LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             iItem);
      if(lpstrFilter)
      {
        fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc((strlen(lpstrFilter)+1)*2);
        lstrcpyAtoW(fodInfos->ShellInfos.lpstrCurrentFilter,(LPSTR)strlwr((LPSTR)lpstrFilter));
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

    if(!_stricmp(lpstrExt,ext))
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

  /* Initialise data of Desktop folder */
  SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&pidlTmp);
  FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);
  SHFree(pidlTmp);

  SHGetSpecialFolderLocation(0,CSIDL_DRIVES,&pidlDrives);

  SHGetDesktopFolder(&psfRoot);

  if (psfRoot)
  {
    /* enumerate the contents of the desktop */
    if(SUCCEEDED(IShellFolder_EnumObjects(psfRoot, hwndCombo, SHCONTF_FOLDERS, &lpeRoot)))
    {
      while (S_OK == IEnumIDList_Next(lpeRoot, 1, &pidlTmp, NULL))
      {
	FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);

	/* special handling for CSIDL_DRIVES */
	if (ILIsEqual(pidlTmp, pidlDrives))
	{
	  if(SUCCEEDED(IShellFolder_BindToObject(psfRoot, pidlTmp, NULL, &IID_IShellFolder, (LPVOID*)&psfDrives)))
	  {
	    /* enumerate the drives */
	    if(SUCCEEDED(IShellFolder_EnumObjects(psfDrives, hwndCombo,SHCONTF_FOLDERS, &lpeDrives)))
	    {
	      while (S_OK == IEnumIDList_Next(lpeDrives, 1, &pidlTmp1, NULL))
	      {
	        pidlAbsTmp = ILCombine(pidlTmp, pidlTmp1);
	        FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlAbsTmp,LISTEND);
	        SHFree(pidlAbsTmp);
	        SHFree(pidlTmp1);
	      }
	      IEnumIDList_Release(lpeDrives);
	    }
	    IShellFolder_Release(psfDrives);
	  }
	}
        SHFree(pidlTmp);
      }
      IEnumIDList_Release(lpeRoot);
    }
  }

  IShellFolder_Release(psfRoot);
  SHFree(pidlDrives);

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
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                               0,    
                                               &sfi,    
                                               sizeof (SHFILEINFOA),   
                                               SHGFI_PIDL | SHGFI_SMALLICON |    
                                               SHGFI_OPENICON | SHGFI_SYSICONINDEX    | 
                                               SHGFI_DISPLAYNAME );   
  }
  else
  {
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
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
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
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

  TRACE("\n");

  switch(wNotifyCode)
  {
  case CBN_CLOSEUP:
    {
      LPSFOLDER tmpFolder;
      int iItem; 

      iItem = CBGetCurSel(fodInfos->DlgInfos.hwndLookInCB);

      tmpFolder = (LPSFOLDER) CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,
                                               iItem);


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
  SFOLDER *tmpFolder = MemAlloc(sizeof(SFOLDER));
  LookInInfos *liInfos;

  TRACE("\n");

  if(!(liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr)))
    return -1;
    
  tmpFolder->m_iIndent = 0;

  if(!pidl)
    return -1;

  /* Calculate the indentation of the item in the lookin*/
  pidlNext = pidl;
  while( (pidlNext=ILGetNext(pidlNext)) )
  {
    tmpFolder->m_iIndent++;
  }

  tmpFolder->pidlItem = ILClone(pidl);

  if(tmpFolder->m_iIndent > liInfos->iMaxIndentation)
    liInfos->iMaxIndentation = tmpFolder->m_iIndent;
  
  SHGetFileInfoA((LPSTR)pidl,
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
  SHFree((LPVOID)pidlParent);

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

    if(iSearchMethod == SEARCH_PIDL && ILIsEqual((LPITEMIDLIST)searchArg,tmpFolder->pidlItem))
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
    SHGetDesktopFolder(&lpsf);
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
    StrRetToStrN(lpstrFileName, MAX_PATH, &str, pidl);
    return NOERROR;
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

  if(SUCCEEDED(SHGetDesktopFolder(&psfParent)))
  {
    psf = psfParent;
    if(pidlAbs && pidlAbs->mkid.cb)
    {
      if(FAILED(IShellFolder_BindToObject(psfParent, pidlAbs, NULL, &IID_IShellFolder, (LPVOID*)&psf)))
      {
        psf = NULL;
      }
    }
    IShellFolder_Release(psfParent);
  }

  return psf;

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

  pidlParent = ILClone(pidl);
  ILRemoveLastID(pidlParent);

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
  

  if(!lpcstrFileName)
    return NULL;
    
  MultiByteToWideChar(CP_ACP,
                      MB_PRECOMPOSED,   
                      lpcstrFileName,  
                      -1,  
                      (LPWSTR)lpwstrDirName,  
                      MAX_PATH);  

    

  if(SUCCEEDED(IShellFolder_ParseDisplayName(psf,
                                             0,
                                             NULL,
                                             (LPWSTR)lpwstrDirName,
                                             &ulEaten,
                                             &pidl,
                                             NULL)))
  {
    return pidl;
  }
  return NULL;
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
