#ifndef _WINE_SHLOBJ_H
#define _WINE_SHLOBJ_H

#include "shell.h"
#include "ole.h"
#include "ole2.h"
#include "compobj.h"
#include "storage.h"
#include "commctrl.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,



/****************************************************************************
*  DllGetClassObject
*/
DWORD WINAPI SHELL32_DllGetClassObject(LPCLSID,REFIID,LPVOID*);


typedef LPVOID	LPBC; /* *IBindCtx really */

/* foreward declaration of the objects*/
typedef struct IEnumIDList IEnumIDList,*LPENUMIDLIST;
typedef struct tagSHELLFOLDER *LPSHELLFOLDER,IShellFolder;
typedef struct tagSHELLVIEW *LPSHELLVIEW,IShellView;
typedef struct tagSHELLBROWSER *LPSHELLBROWSER,IShellBrowser;

/****************************************************************************
*  SHELL ID
*/
/* shell32 classids */
DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);
/* shell32 formatids */
DEFINE_SHLGUID(FMTID_Intshcut,          0x000214A0L, 0, 0);
DEFINE_SHLGUID(FMTID_InternetSite,      0x000214A1L, 0, 0);
/* command group ids */
DEFINE_SHLGUID(CGID_Explorer,           0x000214D0L, 0, 0);
DEFINE_SHLGUID(CGID_ShellDocView,       0x000214D1L, 0, 0);

 /* shell32 Interface ids */
DEFINE_SHLGUID(IID_INewShortcutHookA,   0x000214E1L, 0, 0);
DEFINE_SHLGUID(IID_IShellBrowser,       0x000214E2L, 0, 0);
DEFINE_SHLGUID(IID_IShellView,          0x000214E3L, 0, 0);
DEFINE_SHLGUID(IID_IContextMenu,        0x000214E4L, 0, 0);
DEFINE_SHLGUID(IID_IShellIcon,          0x000214E5L, 0, 0);
DEFINE_SHLGUID(IID_IShellFolder,        0x000214E6L, 0, 0);
DEFINE_SHLGUID(IID_IShellExtInit,       0x000214E8L, 0, 0);
DEFINE_SHLGUID(IID_IShellPropSheetExt,  0x000214E9L, 0, 0);
DEFINE_SHLGUID(IID_IExtractIcon,        0x000214EBL, 0, 0);
DEFINE_SHLGUID(IID_IShellLink,          0x000214EEL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHook,      0x000214EFL, 0, 0);
DEFINE_SHLGUID(IID_IFileViewer,         0x000214F0L, 0, 0);
DEFINE_SHLGUID(IID_ICommDlgBrowser,     0x000214F1L, 0, 0);
DEFINE_SHLGUID(IID_IEnumIDList,         0x000214F2L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerSite,     0x000214F3L, 0, 0);
DEFINE_SHLGUID(IID_IContextMenu2,       0x000214F4L, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookA,  0x000214F5L, 0, 0);
DEFINE_SHLGUID(IID_IPropSheetPage,      0x000214F6L, 0, 0);
DEFINE_SHLGUID(IID_INewShortcutHookW,   0x000214F7L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerW,        0x000214F8L, 0, 0);
DEFINE_SHLGUID(IID_IShellLinkW,         0x000214F9L, 0, 0);
DEFINE_SHLGUID(IID_IExtractIconW,       0x000214FAL, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookW,  0x000214FBL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHookW,     0x000214FCL, 0, 0);

/****************************************************************************
*  STRRET
*/
#define	STRRET_WSTR	    0x0000
#define	STRRET_OFFSET	0x0001
#define	STRRET_CSTR	    0x0002

typedef struct _STRRET
{ UINT32 uType;		/* STRRET_xxx */
  union
  { LPWSTR	pOleStr;	/* OLESTR that will be freed */
    UINT32	uOffset;	/* Offset into SHITEMID (ANSI) */
    char	cStr[MAX_PATH];	/* Buffer to fill in */
  }u;
} STRRET,*LPSTRRET;

/****************************************************************************
 * INTERNAL CLASS: PIDL-Manager
 * Source: HOWTO extend the explorer namespace
 * ftp.microsoft.com/ ... softlib ... regview.exe
 */
#define THIS LPPIDLMGR this
typedef enum tagPIDLTYPE
{ PT_DESKTOP = 0x00000000,
  PT_MYCOMP =  0x00000001,
  PT_CONTROL = 0x00000002,
  PT_RECYCLER =0x00000004,
  PT_DRIVE =   0x00000008,
  PT_FOLDER =  0x00000010,
  PT_VALUE =   0x00000020,
  PT_TEXT = PT_FOLDER | PT_VALUE
} PIDLTYPE;

typedef struct tagPIDLDATA
{ PIDLTYPE type;
  CHAR    szText[1];
} PIDLDATA, FAR *LPPIDLDATA;

typedef struct pidlmgr pidlmgr,*LPPIDLMGR;
typedef struct PidlMgr_VTable
{  STDMETHOD_(LPITEMIDLIST, CreateDesktop) (THIS);
   STDMETHOD_(LPITEMIDLIST, CreateMyComputer) (THIS);
   STDMETHOD_(LPITEMIDLIST, CreateDrive) (THIS_ LPCSTR);
   STDMETHOD_(LPITEMIDLIST, CreateFolder) (THIS_ LPCSTR);
   STDMETHOD_(LPITEMIDLIST, CreateValue) (THIS_ LPCSTR);
/*   STDMETHOD_(void, Delete) (THIS_ LPITEMIDLIST);*/
   STDMETHOD_(LPITEMIDLIST, GetNextItem) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, GetDesktop) (THIS_ LPCITEMIDLIST, LPSTR);
   STDMETHOD_(BOOL32, GetDrive) (THIS_ LPCITEMIDLIST, LPSTR, UINT16);
   STDMETHOD_(LPITEMIDLIST, GetLastItem) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(DWORD, GetItemText) (THIS_ LPCITEMIDLIST, LPSTR, UINT16);
   STDMETHOD_(BOOL32, IsDesktop) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, IsMyComputer) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, IsDrive) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, IsFolder) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, IsValue) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, HasFolders) (THIS_ LPSTR, LPCITEMIDLIST);
   STDMETHOD_(DWORD, GetFolderText) (THIS_ LPCITEMIDLIST, LPSTR, DWORD);
   STDMETHOD_(DWORD, GetValueText) (THIS_ LPCITEMIDLIST, LPSTR, DWORD);
   STDMETHOD_(BOOL32, GetValueType) (THIS_ LPCITEMIDLIST, LPCITEMIDLIST, LPDWORD);
   STDMETHOD_(DWORD, GetDataText) (THIS_ LPCITEMIDLIST, LPCITEMIDLIST, LPSTR, DWORD);
   STDMETHOD_(DWORD, GetPidlPath) (THIS_ LPCITEMIDLIST, LPSTR, DWORD);
   STDMETHOD_(LPITEMIDLIST, Create) (THIS_ PIDLTYPE, LPVOID, UINT16);
   STDMETHOD_(DWORD, GetData) (THIS_ PIDLTYPE, LPCITEMIDLIST, LPVOID, UINT16);
   STDMETHOD_(LPPIDLDATA, GetDataPointer) (THIS_ LPCITEMIDLIST);
   STDMETHOD_(BOOL32, SeparatePathAndValue) (THIS_ LPCITEMIDLIST, LPITEMIDLIST*, LPITEMIDLIST*);

} *LPPIDLMGR_VTABLE,PidlMgr_VTable;

struct pidlmgr 
{ LPPIDLMGR_VTABLE	lpvtbl;
};

#ifdef __WINE__
extern LPPIDLMGR PidlMgr_Constructor();
extern void PidlMgr_Destructor(THIS);
#endif

#undef THIS

/*****************************************************************************
 * IEnumIDList interface
 */
#define THIS LPENUMIDLIST this

typedef struct tagENUMLIST
{ struct tagENUMLIST	*pNext;
  LPITEMIDLIST pidl;
} ENUMLIST, *LPENUMLIST;

typedef struct IEnumIDList_VTable 
{    /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IEnumIDList methods *** */
    STDMETHOD(Next)  (THIS_ ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)  (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumIDList **ppenum) PURE;
		/* *** private methods *** */
    STDMETHOD_(BOOL32,CreateEnumList)(THIS_ LPCSTR, DWORD) PURE;
    STDMETHOD_(BOOL32,AddToEnumList)(THIS_ LPITEMIDLIST) PURE;
    STDMETHOD_(BOOL32,DeleteList)(THIS) PURE;

		
} IEnumIDList_VTable,*LPENUMIDLIST_VTABLE;

struct IEnumIDList
{ LPENUMIDLIST_VTABLE	lpvtbl;
  DWORD			 ref;
  LPPIDLMGR  mpPidlMgr;
  LPENUMLIST mpFirst;
  LPENUMLIST mpLast;
  LPENUMLIST mpCurrent;
};

#undef THIS
//--------------------------------------------------------------------------
//
// FOLDERSETTINGS
//
//  FOLDERSETTINGS is a data structure that explorer passes from one folder
// view to another, when the user is browsing. It calls ISV::GetCurrentInfo
// member to get the current settings and pass it to ISV::CreateViewWindow
// to allow the next folder view "inherit" it. These settings assumes a
// particular UI (which the shell's folder view has), and shell extensions
// may or may not use those settings.
//
//--------------------------------------------------------------------------

typedef LPBYTE LPVIEWSETTINGS;

// NB Bitfields.
// FWF_DESKTOP implies FWF_TRANSPARENT/NOCLIENTEDGE/NOSCROLL
typedef enum
{ FWF_AUTOARRANGE =       0x0001,
  FWF_ABBREVIATEDNAMES =  0x0002,
  FWF_SNAPTOGRID =        0x0004,
  FWF_OWNERDATA =         0x0008,
  FWF_BESTFITWINDOW =     0x0010,
  FWF_DESKTOP =           0x0020,
  FWF_SINGLESEL =         0x0040,
  FWF_NOSUBFOLDERS =      0x0080,
  FWF_TRANSPARENT  =      0x0100,
  FWF_NOCLIENTEDGE =      0x0200,
  FWF_NOSCROLL     =      0x0400,
  FWF_ALIGNLEFT    =      0x0800,
  FWF_SINGLECLICKACTIVATE=0x8000  // TEMPORARY -- NO UI FOR THIS
} FOLDERFLAGS;

typedef enum
{ FVM_ICON =              1,
  FVM_SMALLICON =         2,
  FVM_LIST =              3,
  FVM_DETAILS =           4,
} FOLDERVIEWMODE;

typedef struct
{ UINT32 ViewMode;       // View mode (FOLDERVIEWMODE values)
  UINT32 fFlags;         // View options (FOLDERFLAGS bits)
} FOLDERSETTINGS, *LPFOLDERSETTINGS;

typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;

/* FIXME; the next two lines are propersheet related, move to prsht.h when created */
struct _PSP;
typedef struct _PSP FAR* HPROPSHEETPAGE;

typedef BOOL32 (CALLBACK FAR * LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);
typedef BOOL32 (CALLBACK FAR * LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE,LPARAM);

/************************************************************************
 * IShellFolder interface
 */

#define THIS LPSHELLFOLDER this

/* IShellFolder::GetDisplayNameOf/SetNameOf uFlags */
typedef enum
{ SHGDN_NORMAL            = 0,        /* default (display purpose) */
  SHGDN_INFOLDER          = 1,        /* displayed under a folder (relative)*/
  SHGDN_FORPARSING        = 0x8000    /* for ParseDisplayName or path */
} SHGNO;

/* IShellFolder::EnumObjects */
typedef enum tagSHCONTF
{ SHCONTF_FOLDERS         = 32,       /* for shell browser */
  SHCONTF_NONFOLDERS      = 64,       /* for default view */
  SHCONTF_INCLUDEHIDDEN   = 128       /* for hidden/system objects */
} SHCONTF;

/* from oleidl.h */
#define	DROPEFFECT_NONE		0
#define	DROPEFFECT_COPY		1
#define	DROPEFFECT_MOVE		2
#define	DROPEFFECT_LINK		4
#define	DROPEFFECT_SCROLL       0x80000000

/* IShellFolder::GetAttributesOf flags */
#define SFGAO_CANCOPY           DROPEFFECT_COPY /* Objects can be copied */
#define SFGAO_CANMOVE           DROPEFFECT_MOVE /* Objects can be moved */
#define SFGAO_CANLINK           DROPEFFECT_LINK /* Objects can be linked */
#define SFGAO_CANRENAME         0x00000010L     /* Objects can be renamed */
#define SFGAO_CANDELETE         0x00000020L     /* Objects can be deleted */
#define SFGAO_HASPROPSHEET      0x00000040L     /* Objects have property sheets */
#define SFGAO_DROPTARGET        0x00000100L     /* Objects are drop target */
#define SFGAO_CAPABILITYMASK    0x00000177L
#define SFGAO_LINK              0x00010000L     /* Shortcut (link) */
#define SFGAO_SHARE             0x00020000L     /* shared */
#define SFGAO_READONLY          0x00040000L     /* read-only */
#define SFGAO_GHOSTED           0x00080000L     /* ghosted icon */
#define SFGAO_DISPLAYATTRMASK   0x000F0000L
#define SFGAO_FILESYSANCESTOR   0x10000000L     /* It contains file system folder */
#define SFGAO_FOLDER            0x20000000L     /* It's a folder. */
#define SFGAO_FILESYSTEM        0x40000000L     /* is a file system thing (file/folder/root) */
#define SFGAO_HASSUBFOLDER      0x80000000L     /* Expandable in the map pane */
#define SFGAO_CONTENTSMASK      0x80000000L
#define SFGAO_VALIDATE          0x01000000L     /* invalidate cached information */
#define SFGAO_REMOVABLE        0x02000000L      /* is this removeable media? */

typedef struct IShellFolder_VTable {
    /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IShellFolder methods *** */
    STDMETHOD(ParseDisplayName) (THIS_ HWND32 hwndOwner,LPBC pbcReserved, LPOLESTR32 lpszDisplayName,ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;
    STDMETHOD(EnumObjects)( THIS_ HWND32 hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList) PURE;
    STDMETHOD(BindToObject)(THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(BindToStorage)(THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD(CompareIDs)(THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject)(THIS_ HWND32 hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(GetAttributesOf)(THIS_ UINT32 cidl, LPCITEMIDLIST * apidl,ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)(THIS_ HWND32 hwndOwner, UINT32 cidl, LPCITEMIDLIST * apidl,REFIID riid, UINT32 * prgfInOut, LPVOID * ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf)(THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)(THIS_ HWND32 hwndOwner, LPCITEMIDLIST pidl,LPCOLESTR32 lpszName, DWORD uFlags,LPITEMIDLIST * ppidlOut) PURE;
} *LPSHELLFOLDER_VTABLE,IShellFolder_VTable;

struct tagSHELLFOLDER {
	LPSHELLFOLDER_VTABLE	lpvtbl;
	DWORD			   ref;
	LPSTR        mlpszFolder;
    LPPIDLMGR	   pPidlMgr;
	LPITEMIDLIST mpidl;
	LPITEMIDLIST mpidlNSRoot;
	LPSHELLFOLDER mpSFParent;
};

extern LPSHELLFOLDER pdesktopfolder;
#undef THIS

/************************************************************************
* IShellBrowser Inteface
*/
#define THIS LPSHELLBROWSER this

typedef struct IShellBrowser_VTable 
{    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND32 * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL32 fEnterMode) PURE;

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU32 hmenuShared, LPOLEMENUGROUPWIDTHS32 lpMenuWidths) PURE;
    STDMETHOD(SetMenuSB) (THIS_ HMENU32 hmenuShared, HOLEMENU32 holemenuReserved, HWND32 hwndActiveObject) PURE;
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU32 hmenuShared) PURE;
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR32 lpszStatusText) PURE;
    STDMETHOD(EnableModelessSB) (THIS_ BOOL32 fEnable) PURE;
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG32 lpmsg, WORD wID) PURE;

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT32 wFlags) PURE;
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode, LPSTREAM32  *ppStrm) PURE;
    STDMETHOD(GetControlWindow)(THIS_ UINT32 id, HWND32 * lphwnd) PURE;
    STDMETHOD(SendControlMsg)(THIS_ UINT32 id, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam, LRESULT * pret) PURE;
    STDMETHOD(QueryActiveShellView)(THIS_ IShellView ** ppshv) PURE;
    STDMETHOD(OnViewWindowActive)(THIS_ IShellView * ppshv) PURE;
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT32 nButtons, UINT32 uFlags) PURE;
} *LPSHELLBROWSER_VTABLE,IShellBrowser_VTable;;

struct tagSHELLBROWSER 
{ LPSHELLBROWSER_VTABLE	lpvtbl;
  DWORD	ref;
};

#undef THIS

/************************************************************************
* IShellView Inteface
*/
#define THIS LPSHELLVIEW this

/* shellview select item flags*/
#define SVSI_DESELECT   0x0000
#define SVSI_SELECT     0x0001
#define SVSI_EDIT       0x0003  // includes select
#define SVSI_DESELECTOTHERS 0x0004
#define SVSI_ENSUREVISIBLE  0x0008
#define SVSI_FOCUSED        0x0010

/* shellview get item object flags */
#define SVGIO_BACKGROUND    0x00000000
#define SVGIO_SELECTION     0x00000001
#define SVGIO_ALLVIEW       0x00000002

/* uState values for IShellView::UIActivate */
typedef enum 
{ SVUIA_DEACTIVATE       = 0,
  SVUIA_ACTIVATE_NOFOCUS = 1,
  SVUIA_ACTIVATE_FOCUS   = 2,
  SVUIA_INPLACEACTIVATE  = 3          // new flag for IShellView2
} SVUIA_STATUS;



typedef struct IShellView_VTable
{   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND32 * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL32 fEnterMode) PURE;

    // *** IShellView methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG32 lpmsg) PURE;
    STDMETHOD(EnableModeless) (THIS_ BOOL32 fEnable) PURE;
    STDMETHOD(UIActivate) (THIS_ UINT32 uState) PURE;
    STDMETHOD(Refresh) (THIS) PURE;
    STDMETHOD(CreateViewWindow)(THIS_ IShellView *lpPrevView,LPCFOLDERSETTINGS lpfs, IShellBrowser * psb,RECT32 * prcView, HWND32  *phWnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT32 uFlags) PURE;
    STDMETHOD(GetItemObject)(THIS_ UINT32 uItem, REFIID riid,LPVOID *ppv) PURE;
} IShellView_VTable,*LPSHELLVIEW_VTABLE;;

struct tagSHELLVIEW 
{ LPSHELLVIEW_VTABLE lpvtbl;
  DWORD			     ref;
  LPITEMIDLIST       mpidl;
  LPSHELLFOLDER      pSFParent;
  LPSHELLBROWSER     pShellBrowser;
  HWND32             hWnd;
  FOLDERSETTINGS     FolderSettings;
  HWND32             hWndParent;
};

typedef GUID SHELLVIEWID;
#define SV_CLASS_NAME   ("ShellViewClass")

#undef THIS

/****************************************************************************
 * IShellLink interface
 */

#define THIS LPSHELLLINK this
/* IShellLink::Resolve fFlags */
typedef enum {
    SLR_NO_UI           = 0x0001,
    SLR_ANY_MATCH       = 0x0002,
    SLR_UPDATE          = 0x0004
} SLR_FLAGS;

/* IShellLink::GetPath fFlags */
typedef enum {
    SLGP_SHORTPATH      = 0x0001,
    SLGP_UNCPRIORITY    = 0x0002
} SLGP_FLAGS;



typedef struct IShellLink IShellLink,*LPSHELLLINK;
typedef struct IShellLink_VTable
{
    /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile, INT32 cchMaxPath, WIN32_FIND_DATA32A *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ INT32 *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ INT32 iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, INT32 cchIconPath, INT32 *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, INT32 iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND32 hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
} IShellLink_VTable,*LPSHELLLINK_VTABLE;

struct IShellLink {
	LPSHELLLINK_VTABLE	lpvtbl;
	DWORD			ref;
};

#undef THIS

#ifdef __WINE__

extern LPCLASSFACTORY IClassFactory_Constructor();
extern LPSHELLFOLDER IShellFolder_Constructor(LPSHELLFOLDER,LPITEMIDLIST);
extern LPSHELLVIEW IShellView_Constructor();
extern LPSHELLLINK IShellLink_Constructor();
extern LPENUMIDLIST IEnumIDList_Constructor(LPCSTR,DWORD,HRESULT*);
#endif

/****************************************************************************
 * SHBrowseForFolder API
 */

typedef int (CALLBACK* BFFCALLBACK)(HWND32 hwnd, UINT32 uMsg, LPARAM lParam, LPARAM lpData);

typedef struct tagBROWSEINFO32A {
    HWND32        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR         pszDisplayName;
    LPCSTR        lpszTitle;
    UINT32        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    int           iImage;
} BROWSEINFO32A, *PBROWSEINFO32A, *LPBROWSEINFO32A;

typedef struct tagBROWSEINFO32W {
    HWND32        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT32        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    int           iImage;
} BROWSEINFO32W, *PBROWSEINFO32W, *LPBROWSEINFO32W; 

#define BROWSEINFO   WINELIB_NAME_AW(BROWSEINFO)
#define PBROWSEINFO  WINELIB_NAME_AW(PBROWSEINFO)
#define LPBROWSEINFO WINELIB_NAME_AW(LPBROWSEINFO)

// Browsing for directory.
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
 
#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000

// message from browser
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_VALIDATEFAILEDA    3   // lParam:szPath ret:1(cont),0(EndDialog)
#define BFFM_VALIDATEFAILEDW    4   // lParam:wzPath ret:1(cont),0(EndDialog)

// messages to browser
#define BFFM_SETSTATUSTEXTA     (WM_USER+100)
#define BFFM_ENABLEOK           (WM_USER+101)
#define BFFM_SETSELECTIONA      (WM_USER+102)
#define BFFM_SETSELECTIONW      (WM_USER+103)
#define BFFM_SETSTATUSTEXTW     (WM_USER+104)

/*
#ifdef UNICODE
#define SHBrowseForFolder   SHBrowseForFolderW
#define BFFM_SETSTATUSTEXT  BFFM_SETSTATUSTEXTW
#define BFFM_SETSELECTION   BFFM_SETSELECTIONW

#define BFFM_VALIDATEFAILED BFFM_VALIDATEFAILEDW
#else
#define SHBrowseForFolder   SHBrowseForFolderA
#define BFFM_SETSTATUSTEXT  BFFM_SETSTATUSTEXTA
#define BFFM_SETSELECTION   BFFM_SETSELECTIONA

#define BFFM_VALIDATEFAILED BFFM_VALIDATEFAILEDA 
#endif 
*/

LPITEMIDLIST WINAPI SHBrowseForFolder32A(LPBROWSEINFO32A lpbi);
/*LPITEMIDLIST WINAPI SHBrowseForFolder32W(LPBROWSEINFO32W lpbi);*/
#define  SHBrowseForFolder WINELIB_NAME_AW(SHBrowseForFolder)

/****************************************************************************
 * shlview structures
 */
/*
* IShellFolderViewCallback Callback
*  This "callback" is called by the shells default IShellView implementation (that
*  we got using SHCreateShellViewEx()), to notify us of the various things that
*  are happening to the shellview (and ask for things too).
*
*  You don't have to support anything here - anything you don't want to 
*  handle, the shell will do itself if you just return E_NOTIMPL. This parameters
*  that the shell passes to this function are entirely undocumented.
*
*  HOWEVER, as the cabview sample as originally written used this callback, the
*  writers implemented the callback mechanism on top of their own IShellView.
*  Look there for some clues on what to do here.
*/

typedef HRESULT(CALLBACK *SHELLVIEWPROC)(DWORD dwUserParam,LPSHELLFOLDER psf,
                         HWND32 hwnd,UINT32 uMsg,UINT32 wParam,LPARAM lParam);

/* NF valid values for the "viewmode" item of the SHELLTEMPLATE*/
#define NF_INHERITVIEW    0x0000
#define NF_LOCALVIEW        0x0001

typedef struct _SHELLVIEWDATA   // idl
{ DWORD           dwSize;
  LPSHELLFOLDER   pShellFolder;
  DWORD           dwUserParam;
  LPCITEMIDLIST   pidl;
  DWORD           v3;        // always 0
  SHELLVIEWPROC   pCallBack;
  DWORD           viewmode;  // NF_* enum
} SHELLVIEWDATA, * LPSHELLVIEWDATA;

#undef PURE
#undef FAR
#undef THIS
#undef THIS_
#undef STDMETHOD
#undef STDMETHOD_
#endif /*_WINE_SHLOBJ_H*/
