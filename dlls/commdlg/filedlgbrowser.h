/*
 *  Implementation of IShellBrowser for the File Open common dialog
 * 
 *
 */

#ifndef SHBROWSER_H
#define SHBROWSER_H TRUE

#include "shlobj.h"
#include "winbase.h"
#include "objbase.h"
#include "commdlg.h"

/***********************************************************************
 * Defines and global variables
 */
#define _ICOM_THIS_FromICommDlgBrowser(Class,name) Class* This = (Class*) (((char*)name)-sizeof(void *))

/* dialog internal property */

#define FODPROP_SAVEDLG 0x0001  /* File dialog is a Save file dialog */
#define FODPROP_USEVIEW 0x0002  /* Indicates the user selection must be taken 
				   from the IShellView */

/***********************************************************************
 * Data structure
 */


typedef struct
{

    ICOM_VTABLE(IShellBrowser)* lpVtbl;         /* IShellBrowser VTable */
    ICOM_VTABLE(ICommDlgBrowser)* lpVtbl2;      /* ICommDlgBrowser VTable */
    DWORD ref;                                  /* Reference counter */
    HWND hwndOwner;                             /* Owner dialog of the interface */

} IShellBrowserImpl;

typedef struct
{
    
    OPENFILENAMEA ofnInfos;
    struct {
        IShellBrowser *FOIShellBrowser; 
        IShellFolder *FOIShellFolder;
        IShellView *FOIShellView;
    } Shell;

    struct {
        HWND hwndOwner;
        HWND hwndView;
        RECT rectView;
        FOLDERSETTINGS folderSettings;
        LPITEMIDLIST pidlAbsCurrent;
        LPWSTR lpstrCurrentFilter;
    } ShellInfos;

    struct {
        HWND hwndFileTypeCB;
        HWND hwndLookInCB;
        HWND hwndFileName;
	DWORD dwDlgProp;
    } DlgInfos;

} FileOpenDlgInfos;

/***********************************************************************
 * Prototypes for the methods of the IShellBrowserImpl class
 */
/* Constructor */
IShellBrowser * IShellBrowserImpl_Construct(HWND hwndOwner);

/* IUnknown */
HRESULT WINAPI IShellBrowserImpl_QueryInterface(IShellBrowser *iface,
                                            REFIID riid, 
                                            LPVOID *ppvObj);

ULONG WINAPI IShellBrowserImpl_AddRef(IShellBrowser * iface);

ULONG WINAPI IShellBrowserImpl_Release(IShellBrowser * iface);

/* IOleWindow */
HRESULT WINAPI IShellBrowserImpl_GetWindow(IShellBrowser * iface,  
                                           HWND * phwnd);

HRESULT WINAPI IShellBrowserImpl_ContextSensitiveHelp(IShellBrowser * iface,
                                                      BOOL fEnterMode);

/* IShellBrowser */

HRESULT WINAPI IShellBrowserImpl_BrowseObject(IShellBrowser *iface, 
                                          LPCITEMIDLIST pidl,   
                                          UINT wFlags);

HRESULT WINAPI IShellBrowserImpl_EnableModelessSB(IShellBrowser *iface,    
                                              BOOL fEnable);
                                              
HRESULT WINAPI IShellBrowserImpl_GetControlWindow(IShellBrowser *iface,    
                                              UINT id,    
                                              HWND *lphwnd);

HRESULT WINAPI IShellBrowserImpl_GetViewStateStream(IShellBrowser *iface,
                                                DWORD grfMode,    
                                                LPSTREAM *ppStrm);

HRESULT WINAPI IShellBrowserImpl_InsertMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared,
                                           LPOLEMENUGROUPWIDTHS lpMenuWidths);

HRESULT WINAPI IShellBrowserImpl_OnViewWindowActive(IShellBrowser *iface,
                                                IShellView *ppshv);
                                              

HRESULT WINAPI IShellBrowserImpl_QueryActiveShellView(IShellBrowser *iface,
                                                  IShellView **ppshv);

HRESULT WINAPI IShellBrowserImpl_RemoveMenusSB(IShellBrowser *iface,
                                           HMENU hmenuShared);

HRESULT WINAPI IShellBrowserImpl_SendControlMsg(IShellBrowser *iface,    
                                            UINT id,    
                                            UINT uMsg,    
                                            WPARAM wParam,    
                                            LPARAM lParam,
                                            LRESULT *pret);

HRESULT WINAPI IShellBrowserImpl_SetMenuSB(IShellBrowser *iface,
                                       HMENU hmenuShared,    
                                       HOLEMENU holemenuReserved,
                                       HWND hwndActiveObject);

HRESULT WINAPI IShellBrowserImpl_SetStatusTextSB(IShellBrowser *iface,
                                             LPCOLESTR lpszStatusText);
                                                

HRESULT WINAPI IShellBrowserImpl_SetToolbarItems(IShellBrowser *iface,
                                             LPTBBUTTON lpButtons,    
                                             UINT nButtons,    
                                             UINT uFlags);
                                              
HRESULT WINAPI IShellBrowserImpl_TranslateAcceleratorSB(IShellBrowser *iface,
                                                    LPMSG lpmsg,    
                                                    WORD wID);


/* ICommDlgBrowser */

HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_QueryInterface(ICommDlgBrowser *iface,
                                            REFIID riid, 
                                            LPVOID *ppvObj);

ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_AddRef(ICommDlgBrowser * iface);

ULONG WINAPI IShellBrowserImpl_ICommDlgBrowser_Release(ICommDlgBrowser * iface);

HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand(ICommDlgBrowser *iface,
                                                                  IShellView *ppshv);

HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_OnStateChange(ICommDlgBrowser *iface,
                                                               IShellView *ppshv,
                                                               ULONG uChange);

HRESULT WINAPI IShellBrowserImpl_ICommDlgBrowser_IncludeObject(ICommDlgBrowser *iface, 
                                                               IShellView * ppshv,
                                                               LPCITEMIDLIST pidl);



LPITEMIDLIST GetSelectedPidl(IShellView *ppshv);

#endif /*SHBROWSER_H*/
