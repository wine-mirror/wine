/*
 *  Implementation of IShellBrowser for the File Open common dialog
 * 
 *
 */

#include <stdio.h>
#include "unknwn.h"
#include "filedlgbrowser.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "heap.h"
#include "shlguid.h"
#include "wine/obj_dataobject.h"
#include "debugtools.h"
#include "cdlg.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(commdlg)

/**************************************************************************
*   Structure
*/
static ICOM_VTABLE(IShellBrowser) IShellBrowserImpl_Vtbl =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
        /* IUnknown */
        IShellBrowserImpl_QueryInterface,
        IShellBrowserImpl_AddRef,
        IShellBrowserImpl_Release,
        /* IOleWindow */
        IShellBrowserImpl_GetWindow,
        IShellBrowserImpl_ContextSensitiveHelp,
        /*  IShellBrowser */
        IShellBrowserImpl_InsertMenusSB,
        IShellBrowserImpl_SetMenuSB,
        IShellBrowserImpl_RemoveMenusSB,
        IShellBrowserImpl_SetStatusTextSB,
        IShellBrowserImpl_EnableModelessSB,
        IShellBrowserImpl_TranslateAcceleratorSB,
        IShellBrowserImpl_BrowseObject,
        IShellBrowserImpl_GetViewStateStream,
        IShellBrowserImpl_GetControlWindow,
        IShellBrowserImpl_SendControlMsg,
        IShellBrowserImpl_QueryActiveShellView,
        IShellBrowserImpl_OnViewWindowActive,
        IShellBrowserImpl_SetToolbarItems
};

static ICOM_VTABLE(ICommDlgBrowser) IShellBrowserImpl_ICommDlgBrowser_Vtbl =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
        /* IUnknown */
        IShellBrowserImpl_ICommDlgBrowser_QueryInterface,
        IShellBrowserImpl_ICommDlgBrowser_AddRef,
        IShellBrowserImpl_ICommDlgBrowser_Release,
        /* ICommDlgBrowser */
        IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand,
        IShellBrowserImpl_ICommDlgBrowser_OnStateChange,
        IShellBrowserImpl_ICommDlgBrowser_IncludeObject
};


/**************************************************************************
*   Local Prototypes
*/

HRESULT IShellBrowserImpl_ICommDlgBrowser_OnSelChange(ICommDlgBrowser *iface, IShellView *ppshv);
#if 0
LPITEMIDLIST GetSelectedPidl(IShellView *ppshv);
#endif

/**************************************************************************
*   External Prototypes
*/
extern const char *FileOpenDlgInfosStr; 

extern HRESULT          GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPSTR lpstrFileName);
extern HRESULT          GetFileName(HWND hwnd, LPITEMIDLIST pidl, LPSTR lpstrFileName);
extern IShellFolder*    GetShellFolderFromPidl(LPITEMIDLIST pidlAbs);
extern LPITEMIDLIST     GetParentPidl(LPITEMIDLIST pidl);
extern LPITEMIDLIST     GetPidlFromName(IShellFolder *psf,LPCSTR lpcstrFileName);

extern BOOL    FILEDLG95_SHELL_FillIncludedItemList(HWND hwnd,
                                                        LPITEMIDLIST pidlCurrentFolder,
                                                        LPSTR lpstrMask);

extern int     FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl);
extern BOOL    FILEDLG95_OnOpen(HWND hwnd);
extern HRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode);


/**************************************************************************
*  IShellBrowserImpl_Construct
*/
IShellBrowser * IShellBrowserImpl_Construct(HWND hwndOwner)
{
    IShellBrowserImpl *sb;
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwndOwner,FileOpenDlgInfosStr);

    sb=(IShellBrowserImpl*)COMDLG32_SHAlloc(sizeof(IShellBrowserImpl));

    /* Initialisation of the member variables */
    sb->ref=1;
    sb->hwndOwner = hwndOwner;

    /* Initialisation of the vTables */
    sb->lpVtbl = &IShellBrowserImpl_Vtbl;
    sb->lpVtbl2 = &IShellBrowserImpl_ICommDlgBrowser_Vtbl;

    COMDLG32_SHGetSpecialFolderLocation(hwndOwner, CSIDL_DESKTOP,
                               &fodInfos->ShellInfos.pidlAbsCurrent);

    TRACE("%p\n", sb);

    return (IShellBrowser *) sb;
}

/**************************************************************************
*
*
*  The INTERFACE of the IShellBrowser object
*
*/

/*
 * IUnknown
 */

/***************************************************************************
*  IShellBrowserImpl_QueryInterface
*/
HRESULT WINAPI IShellBrowserImpl_QueryInterface(IShellBrowser *iface,
                                            REFIID riid, 
                                            LPVOID *ppvObj)
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
    { *ppvObj = This; 
    }
    else if(IsEqualIID(riid, &IID_IOleWindow))  /*IOleWindow*/
    { *ppvObj = (IOleWindow*)This;
    }

    else if(IsEqualIID(riid, &IID_IShellBrowser))  /*IShellBrowser*/
    { *ppvObj = (IShellBrowser*)This;
    }

    else if(IsEqualIID(riid, &IID_ICommDlgBrowser))  /*ICommDlgBrowser*/
    { *ppvObj = (ICommDlgBrowser*) &(This->lpVtbl2);
    }

    if(*ppvObj)
    { IUnknown_AddRef( (IShellBrowser*) *ppvObj);
      return S_OK;
    }
    return E_NOINTERFACE;
}

/**************************************************************************
*  IShellBrowser::AddRef
*/
ULONG WINAPI IShellBrowserImpl_AddRef(IShellBrowser * iface)
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    return ++(This->ref);
}

/**************************************************************************
*  IShellBrowserImpl_Release
*/
ULONG WINAPI IShellBrowserImpl_Release(IShellBrowser * iface)
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    if (!--(This->ref)) 
    { 
      HeapFree(GetProcessHeap(),0, This);
      return 0;
    }
    return This->ref;
}

/*
 * IOleWindow
 */

/**************************************************************************
*  IShellBrowserImpl_GetWindow  (IOleWindow)
*
*  Inherited from IOleWindow::GetWindow
*
*  See Windows documentation for more details
*
*  Note : We will never be window less in the File Open dialog
*  
*/
HRESULT WINAPI IShellBrowserImpl_GetWindow(IShellBrowser * iface,  
                                           HWND * phwnd)
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    if(!This->hwndOwner)
        return E_FAIL;

    *phwnd = This->hwndOwner;

    return (*phwnd) ? S_OK : E_UNEXPECTED; 

}

/**************************************************************************
*  IShellBrowserImpl_ContextSensitiveHelp
*/
HRESULT WINAPI IShellBrowserImpl_ContextSensitiveHelp(IShellBrowser * iface,
                                                      BOOL fEnterMode)
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/*
 * IShellBrowser
 */

/**************************************************************************
*  IShellBrowserImpl_BrowseObject
*
*  See Windows documentation on IShellBrowser::BrowseObject for more details
*
*  This function will override user specified flags and will always 
*  use SBSP_DEFBROWSER and SBSP_DEFMODE.  
*/
HRESULT WINAPI IShellBrowserImpl_BrowseObject(IShellBrowser *iface, 
                                              LPCITEMIDLIST pidl, 
                                              UINT wFlags)
{
    HRESULT hRes;
    IShellFolder *psfTmp;
    IShellView *psvTmp;
    FileOpenDlgInfos *fodInfos;
    LPITEMIDLIST pidlTmp;
    HWND hwndView;
    HWND hDlgWnd;
    BOOL bViewHasFocus;

    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)(%p,0x%08x)\n", This, pidl, wFlags);

    fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);

    /* Format the pidl according to its parameter's category */
    if(wFlags & SBSP_RELATIVE)
    {
        
        /* SBSP_RELATIVE  A relative pidl (relative from the current folder) */
        if(FAILED(hRes = IShellFolder_BindToObject(fodInfos->Shell.FOIShellFolder,
             pidl, NULL, &IID_IShellFolder, (LPVOID *)&psfTmp)))
        {
            ERR("bind to object failed\n");
	    return hRes;
        }
        /* create an absolute pidl */
        pidlTmp = COMDLG32_PIDL_ILCombine(fodInfos->ShellInfos.pidlAbsCurrent,
                                                        (LPITEMIDLIST)pidl);
    }
    else if(wFlags & SBSP_PARENT)
    {
        /* Browse the parent folder (ignores the pidl) */
        pidlTmp = GetParentPidl(fodInfos->ShellInfos.pidlAbsCurrent);
        psfTmp = GetShellFolderFromPidl(pidlTmp);

    }
    else /* SBSP_ABSOLUTE is 0x0000 */
    {
        /* An absolute pidl (relative from the desktop) */
        pidlTmp =  COMDLG32_PIDL_ILClone((LPITEMIDLIST)pidl);
        psfTmp = GetShellFolderFromPidl(pidlTmp);
    }
    
    if(!psfTmp) return E_FAIL;

    /* If the pidl to browse to is equal to the actual pidl ... 
       do nothing and pretend you did it*/
    if(COMDLG32_PIDL_ILIsEqual(pidlTmp,fodInfos->ShellInfos.pidlAbsCurrent))
    {
        IShellFolder_Release(psfTmp);
	COMDLG32_SHFree(pidlTmp);
        return NOERROR;
    }

    /* Release the current DataObject */
    if (fodInfos->Shell.FOIDataObject)
    {
      IDataObject_Release(fodInfos->Shell.FOIDataObject);
      fodInfos->Shell.FOIDataObject = NULL;
    }

    /* Create the associated view */
    if(FAILED(hRes = IShellFolder_CreateViewObject(psfTmp, fodInfos->ShellInfos.hwndOwner,
           &IID_IShellView, (LPVOID *)&psvTmp))) return hRes;

    /* Check if listview has focus */
    bViewHasFocus = IsChild(fodInfos->ShellInfos.hwndView,GetFocus());

    /* Get the foldersettings from the old view */
    if(fodInfos->Shell.FOIShellView)
      IShellView_GetCurrentInfo(fodInfos->Shell.FOIShellView, &fodInfos->ShellInfos.folderSettings);

    /* Release the old fodInfos->Shell.FOIShellView and update its value.
    We have to update this early since ShellView_CreateViewWindow of native
    shell32 calls OnStateChange and needs the correct view here.*/
    if(fodInfos->Shell.FOIShellView)
    {
      IShellView_DestroyViewWindow(fodInfos->Shell.FOIShellView);
      IShellView_Release(fodInfos->Shell.FOIShellView);
    }
    fodInfos->Shell.FOIShellView = psvTmp;

    /* Release old FOIShellFolder and update its value */
    if (fodInfos->Shell.FOIShellFolder)
      IShellFolder_Release(fodInfos->Shell.FOIShellFolder);
    fodInfos->Shell.FOIShellFolder = psfTmp;

    /* Release old pidlAbsCurrent and update its value */
    COMDLG32_SHFree((LPVOID)fodInfos->ShellInfos.pidlAbsCurrent);
    fodInfos->ShellInfos.pidlAbsCurrent = pidlTmp;

    /* Create the window */
    TRACE("create view window\n");
    if(FAILED(hRes = IShellView_CreateViewWindow(psvTmp, NULL,
         &fodInfos->ShellInfos.folderSettings, fodInfos->Shell.FOIShellBrowser,
         &fodInfos->ShellInfos.rectView, &hwndView))) return hRes;

    fodInfos->ShellInfos.hwndView = hwndView;

    /* Select the new folder in the Look In combo box of the Open file dialog */
    FILEDLG95_LOOKIN_SelectItem(fodInfos->DlgInfos.hwndLookInCB,fodInfos->ShellInfos.pidlAbsCurrent);

    /* changes the tab order of the ListView to reflect the window's File Dialog */
    hDlgWnd = GetDlgItem(GetParent(hwndView), IDC_LOOKIN); 
    SetWindowPos(hwndView, hDlgWnd, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

    /* Since we destroyed the old view if it had focus set focus to the newly created view */
    if (bViewHasFocus)
      SetFocus(fodInfos->ShellInfos.hwndView);

    return hRes; 
}

/**************************************************************************
*  IShellBrowserImpl_EnableModelessSB
*/
HRESULT WINAPI IShellBrowserImpl_EnableModelessSB(IShellBrowser *iface,    
                                              BOOL fEnable)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellBrowserImpl_GetControlWindow
*/
HRESULT WINAPI IShellBrowserImpl_GetControlWindow(IShellBrowser *iface,    
                                              UINT id,    
                                              HWND *lphwnd)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}
/**************************************************************************
*  IShellBrowserImpl_GetViewStateStream
*/
HRESULT WINAPI IShellBrowserImpl_GetViewStateStream(IShellBrowser *iface,
                                                DWORD grfMode,    
                                                LPSTREAM *ppStrm)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}       
/**************************************************************************
*  IShellBrowserImpl_InsertMenusSB
*/
HRESULT WINAPI IShellBrowserImpl_InsertMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared,
                                           LPOLEMENUGROUPWIDTHS lpMenuWidths)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}
/**************************************************************************
*  IShellBrowserImpl_OnViewWindowActive
*/
HRESULT WINAPI IShellBrowserImpl_OnViewWindowActive(IShellBrowser *iface,
                                                IShellView *ppshv)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}   
/**************************************************************************
*  IShellBrowserImpl_QueryActiveShellView
*/
HRESULT WINAPI IShellBrowserImpl_QueryActiveShellView(IShellBrowser *iface,
                                                  IShellView **ppshv)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    FileOpenDlgInfos *fodInfos;

    TRACE("(%p)\n", This);

    fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);

    if(!(*ppshv = fodInfos->Shell.FOIShellView))
    {
        return E_FAIL;
    }
    IShellView_AddRef(fodInfos->Shell.FOIShellView);
    return NOERROR;
}   
/**************************************************************************
*  IShellBrowserImpl_RemoveMenusSB
*/
HRESULT WINAPI IShellBrowserImpl_RemoveMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}   
/**************************************************************************
*  IShellBrowserImpl_SendControlMsg
*/
HRESULT WINAPI IShellBrowserImpl_SendControlMsg(IShellBrowser *iface,    
                                            UINT id,    
                                            UINT uMsg,    
                                            WPARAM wParam,    
                                            LPARAM lParam,
                                            LRESULT *pret)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);
    LRESULT lres;
    
    TRACE("(%p)->(0x%08x 0x%08x 0x%08x 0x%08lx %p)\n", This, id, uMsg, wParam, lParam, pret);

    switch (id)
    {
      case FCW_TOOLBAR:
        lres = SendDlgItemMessageA( This->hwndOwner, IDC_TOOLBAR, uMsg, wParam, lParam);
	break;
      default:
        FIXME("ctrl id: %x\n", id);
        return E_NOTIMPL;
    }
    if (pret) *pret = lres;
    return S_OK;
}
/**************************************************************************
*  IShellBrowserImpl_SetMenuSB
*/
HRESULT WINAPI IShellBrowserImpl_SetMenuSB(IShellBrowser *iface,
                                       HMENU hmenuShared,    
                                       HOLEMENU holemenuReserved,
                                       HWND hwndActiveObject)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}   
/**************************************************************************
*  IShellBrowserImpl_SetStatusTextSB
*/
HRESULT WINAPI IShellBrowserImpl_SetStatusTextSB(IShellBrowser *iface,
                                             LPCOLESTR lpszStatusText)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}   
/**************************************************************************
*  IShellBrowserImpl_SetToolbarItems
*/
HRESULT WINAPI IShellBrowserImpl_SetToolbarItems(IShellBrowser *iface,
                                             LPTBBUTTON lpButtons,    
                                             UINT nButtons,    
                                             UINT uFlags)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}   
/**************************************************************************
*  IShellBrowserImpl_TranslateAcceleratorSB
*/
HRESULT WINAPI IShellBrowserImpl_TranslateAcceleratorSB(IShellBrowser *iface,
                                                    LPMSG lpmsg,    
                                                    WORD wID)
                                              
{
    ICOM_THIS(IShellBrowserImpl, iface);

    TRACE("(%p)\n", This);

    /* Feature not implemented */
    return E_NOTIMPL;
}

/*
 * ICommDlgBrowser
 */

/***************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_QueryInterface
*/
HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_QueryInterface(ICommDlgBrowser *iface,
                                            REFIID riid, 
                                            LPVOID *ppvObj)
{
    _ICOM_THIS_FromICommDlgBrowser(IShellBrowser,iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_QueryInterface(This,riid,ppvObj);
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_AddRef
*/
ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_AddRef(ICommDlgBrowser * iface)
{
    _ICOM_THIS_FromICommDlgBrowser(IShellBrowser,iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_AddRef(This);
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_Release
*/
ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_Release(ICommDlgBrowser * iface)
{
    _ICOM_THIS_FromICommDlgBrowser(IShellBrowser,iface);

    TRACE("(%p)\n", This);

    return IShellBrowserImpl_Release(This);
}
/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand
*
*   Called when a user double-clicks in the view or presses the ENTER key
*/
HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand(ICommDlgBrowser *iface,
                                                                  IShellView *ppshv)
{
    LPITEMIDLIST pidl;
    FileOpenDlgInfos *fodInfos;

    _ICOM_THIS_FromICommDlgBrowser(IShellBrowserImpl,iface);

    TRACE("(%p)\n", This);

    fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);  
    
    /* If the selected object is not a folder, send a IDOK command to parent window */
    if((pidl = GetPidlFromDataObject(fodInfos->Shell.FOIDataObject, 1)))
    {
        HRESULT hRes;

        ULONG  ulAttr = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
        IShellFolder_GetAttributesOf(fodInfos->Shell.FOIShellFolder, 1, &pidl, &ulAttr);
	if (ulAttr & (SFGAO_FOLDER | SFGAO_HASSUBFOLDER) )
	{
          hRes = IShellBrowser_BrowseObject((IShellBrowser *)This,pidl,SBSP_RELATIVE);
	}
        else
	{
          /* Tell the dialog that the user selected a file */
	  hRes = PostMessageA(This->hwndOwner, WM_COMMAND, IDOK, 0L);
	}

        /* Free memory used by pidl */
        COMDLG32_SHFree((LPVOID)pidl);

        return hRes;
    }

    return E_FAIL;
}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_OnStateChange
*/
HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnStateChange(ICommDlgBrowser *iface,
                                                               IShellView *ppshv,
                                                               ULONG uChange)
{

    _ICOM_THIS_FromICommDlgBrowser(IShellBrowserImpl,iface);

    TRACE("(%p shv=%p)\n", This, ppshv);

    switch (uChange)
    {
        case CDBOSC_SETFOCUS:
             /* FIXME: Reset the default button.
	        This should be taken care of by defdlg. If control
	        other than button receives focus the default button
	        should be restored. */
             SendMessageA(This->hwndOwner, DM_SETDEFID, IDOK, 0);

            break;
        case CDBOSC_KILLFOCUS: 
	    {
		FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);
		if(fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
		    SetDlgItemTextA(fodInfos->ShellInfos.hwndOwner,IDOK,"&Save");
            }
            break;
        case CDBOSC_SELCHANGE:
            return IShellBrowserImpl_ICommDlgBrowser_OnSelChange(iface,ppshv);
        case CDBOSC_RENAME:
	    /* nothing to do */
            break;
    }

    return NOERROR;     
}

/* copied from shell32 to avoid linking to it */
static HRESULT COMDLG32_StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	TRACE("dest=0x%p len=0x%lx strret=0x%p pidl=%p stub\n",dest,len,src,pidl);

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
	    COMDLG32_SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTRA:
	    lstrcpynAtoW((LPWSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSETA:
	    if (pidl)
	    {
	      lstrcpynAtoW((LPWSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    }
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    { *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}
/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_IncludeObject
*/
HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_IncludeObject(ICommDlgBrowser *iface, 
                                                               IShellView * ppshv,
                                                               LPCITEMIDLIST pidl)
{
    FileOpenDlgInfos *fodInfos;
    ULONG ulAttr;
    STRRET str;
    WCHAR szPathW[MAX_PATH];

    _ICOM_THIS_FromICommDlgBrowser(IShellBrowserImpl,iface);

    TRACE("(%p)\n", This);

    fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);

    ulAttr = SFGAO_HIDDEN | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_LINK;
    IShellFolder_GetAttributesOf(fodInfos->Shell.FOIShellFolder, 1, &pidl, &ulAttr);
    
    if( (ulAttr & SFGAO_HIDDEN)                                         /* hidden */
      | !(ulAttr & (SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR))) /* special folder */
        return S_FALSE;

    /* always include directorys and links */
    if(ulAttr & (SFGAO_FOLDER | SFGAO_LINK)) 
        return S_OK;

    /* Check if there is a mask to apply if not */
    if(!fodInfos->ShellInfos.lpstrCurrentFilter || !lstrlenW(fodInfos->ShellInfos.lpstrCurrentFilter))
        return S_OK;

    if (SUCCEEDED(IShellFolder_GetDisplayNameOf(fodInfos->Shell.FOIShellFolder, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, &str)))
    {
      if (SUCCEEDED(COMDLG32_StrRetToStrNW(szPathW, MAX_PATH, &str, pidl)))
      {
	  if (COMDLG32_PathMatchSpecW(szPathW, fodInfos->ShellInfos.lpstrCurrentFilter))
          return S_OK;
      }
    }
    return S_FALSE;

}

/**************************************************************************
*  IShellBrowserImpl_ICommDlgBrowser_OnSelChange
*/  
HRESULT IShellBrowserImpl_ICommDlgBrowser_OnSelChange(ICommDlgBrowser *iface, IShellView *ppshv)
{
    FileOpenDlgInfos *fodInfos;

    _ICOM_THIS_FromICommDlgBrowser(IShellBrowserImpl,iface);

    fodInfos = (FileOpenDlgInfos *) GetPropA(This->hwndOwner,FileOpenDlgInfosStr);
    TRACE("(%p do=%p view=%p)\n", This, fodInfos->Shell.FOIDataObject, fodInfos->Shell.FOIShellView);
    
    /* release old selections */
    if (fodInfos->Shell.FOIDataObject)
      IDataObject_Release(fodInfos->Shell.FOIDataObject);
    
    /* get a new DataObject from the ShellView */
    if(FAILED(IShellView_GetItemObject(fodInfos->Shell.FOIShellView, SVGIO_SELECTION,
                              &IID_IDataObject, (LPVOID*)&fodInfos->Shell.FOIDataObject)))
      return E_FAIL;
					  
    FILEDLG95_FILENAME_FillFromSelection(This->hwndOwner);

    SendCustomDlgNotificationMessage(This->hwndOwner, CDN_SELCHANGE);
    return S_OK;
}

