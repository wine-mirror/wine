#ifndef __WINE_SHLOBJ_H
#define __WINE_SHLOBJ_H

#include "wintypes.h"
#include "winbase.h"	/* WIN32_FIND_* */
#include "wine/obj_base.h"
#include "wine/obj_shelllink.h"
#include "ole2.h"
#include "shell.h"
#include "oleobj.h"
#include "commctrl.h"
#include "prsht.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

/****************************************************************************
*  DllGetClassObject
*/
DWORD WINAPI SHELL32_DllGetClassObject(REFCLSID,REFIID,LPVOID*);



/* foreward declaration of the objects*/
typedef struct IContextMenu IContextMenu, *LPCONTEXTMENU;
typedef struct tagSHELLEXTINIT	*LPSHELLEXTINIT,IShellExtInit;
typedef struct tagENUMIDLIST	*LPENUMIDLIST,	IEnumIDList;
typedef struct tagSHELLFOLDER	*LPSHELLFOLDER,	IShellFolder;
typedef struct tagSHELLVIEW	*LPSHELLVIEW,	IShellView;
typedef struct tagSHELLBROWSER	*LPSHELLBROWSER,IShellBrowser;
typedef struct tagSHELLICON	*LPSHELLICON,	IShellIcon;
typedef struct tagDOCKINGWINDOWFRAME	*LPDOCKINGWINDOWFRAME,	IDockingWindowFrame;
typedef struct tagCOMMDLGBROWSER	*LPCOMMDLGBROWSER,	ICommDlgBrowser;
 
/****************************************************************************
*  STRRET
*/
#define	STRRET_WSTR	0x0000
#define	STRRET_OFFSETA	0x0001
#define	STRRET_CSTRA	0x0002
#define STRRET_ASTR	0X0003
#define STRRET_OFFSETW	0X0004
#define STRRET_CSTRW	0X0005


typedef struct _STRRET
{ UINT uType;		/* STRRET_xxx */
  union
  { LPWSTR	pOleStr;	/* OLESTR that will be freed */
    LPSTR	pStr;
    UINT	uOffset;	/* OffsetINT32o SHITEMID (ANSI) */
    char	cStr[MAX_PATH];	/* Buffer to fill in */
    WCHAR	cStrW[MAX_PATH];
  }u;
} STRRET,*LPSTRRET;


/*****************************************************************************
 * IContextMenu interface
 */
#define THIS LPCONTEXTMENU this

/* default menu items*/
#define IDM_EXPLORE  0
#define IDM_OPEN     1
#define IDM_RENAME   2
#define IDM_LAST     IDM_RENAME

/* QueryContextMenu uFlags */
#define CMF_NORMAL              0x00000000
#define CMF_DEFAULTONLY         0x00000001
#define CMF_VERBSONLY           0x00000002
#define CMF_EXPLORE             0x00000004
#define CMF_NOVERBS             0x00000008
#define CMF_CANRENAME           0x00000010
#define CMF_NODEFAULT           0x00000020
#define CMF_INCLUDESTATIC       0x00000040
#define CMF_RESERVED            0xffff0000      /* View specific */

/* GetCommandString uFlags */
#define GCS_VERBA        0x00000000     /* canonical verb */
#define GCS_HELPTEXTA    0x00000001     /* help text (for status bar) */
#define GCS_VALIDATEA    0x00000002     /* validate command exists */
#define GCS_VERBW        0x00000004     /* canonical verb (unicode) */
#define GCS_HELPTEXTW    0x00000005     /* help text (unicode version) */
#define GCS_VALIDATEW    0x00000006     /* validate command exists (unicode) */
#define GCS_UNICODE      0x00000004     /* for bit testing - Unicode string */

#define GCS_VERB        GCS_VERBA
#define GCS_HELPTEXT    GCS_HELPTEXTA
#define GCS_VALIDATE    GCS_VALIDATEA

#define CMDSTR_NEWFOLDERA   "NewFolder"
#define CMDSTR_VIEWLISTA    "ViewList"
#define CMDSTR_VIEWDETAILSA "ViewDetails"
#define CMDSTR_NEWFOLDERW   L"NewFolder"
#define CMDSTR_VIEWLISTW    L"ViewList"
#define CMDSTR_VIEWDETAILSW L"ViewDetails"

#define CMDSTR_NEWFOLDER    CMDSTR_NEWFOLDERA
#define CMDSTR_VIEWLIST     CMDSTR_VIEWLISTA
#define CMDSTR_VIEWDETAILS  CMDSTR_VIEWDETAILSA

#define CMIC_MASK_HOTKEY        SEE_MASK_HOTKEY
#define CMIC_MASK_ICON          SEE_MASK_ICON
#define CMIC_MASK_FLAG_NO_UI    SEE_MASK_FLAG_NO_UI
#define CMIC_MASK_UNICODE       SEE_MASK_UNICODE
#define CMIC_MASK_NO_CONSOLE    SEE_MASK_NO_CONSOLE
#define CMIC_MASK_HASLINKNAME   SEE_MASK_HASLINKNAME
#define CMIC_MASK_FLAG_SEP_VDM  SEE_MASK_FLAG_SEPVDM
#define CMIC_MASK_HASTITLE      SEE_MASK_HASTITLE
#define CMIC_MASK_ASYNCOK       SEE_MASK_ASYNCOK

#define CMIC_MASK_PTINVOKE      0x20000000

/*NOTE: When SEE_MASK_HMONITOR is set, hIcon is treated as hMonitor */
typedef struct tagCMINVOKECOMMANDINFO 
{   DWORD cbSize;        /* sizeof(CMINVOKECOMMANDINFO) */
    DWORD fMask;         /* any combination of CMIC_MASK_* */
    HWND hwnd;         /* might be NULL (indicating no owner window) */
    LPCSTR lpVerb;       /* either a string or MAKEINTRESOURCE(idOffset) */
    LPCSTR lpParameters; /* might be NULL (indicating no parameter) */
    LPCSTR lpDirectory;  /* might be NULL (indicating no specific directory) */
   INT nShow;           /* one of SW_ values for ShowWindow() API */

    DWORD dwHotKey;
    HANDLE hIcon;
} CMINVOKECOMMANDINFO,  *LPCMINVOKECOMMANDINFO;

typedef struct tagCMInvokeCommandInfoEx 
{   DWORD cbSize;        /* must be sizeof(CMINVOKECOMMANDINFOEX) */
    DWORD fMask;         /* any combination of CMIC_MASK_* */
    HWND hwnd;         /* might be NULL (indicating no owner window) */
    LPCSTR lpVerb;       /* either a string or MAKEINTRESOURCE(idOffset) */
    LPCSTR lpParameters; /* might be NULL (indicating no parameter) */
    LPCSTR lpDirectory;  /* might be NULL (indicating no specific directory) */
	INT nShow;           /* one of SW_ values for ShowWindow() API */

    DWORD dwHotKey;
    
    HANDLE hIcon;
    LPCSTR lpTitle;        /* For CreateProcess-StartupInfo.lpTitle */
    LPCWSTR lpVerbW;       /* Unicode verb (for those who can use it) */
    LPCWSTR lpParametersW; /* Unicode parameters (for those who can use it) */
    LPCWSTR lpDirectoryW;  /* Unicode directory (for those who can use it) */
    LPCWSTR lpTitleW;      /* Unicode title (for those who can use it) */
    POINT ptInvoke;      /* Point where it's invoked */

} CMINVOKECOMMANDINFOEX,  *LPCMINVOKECOMMANDINFOEX;
#undef THIS


#define ICOM_INTERFACE IContextMenu
#define IContextMenu_METHODS \
    ICOM_METHOD5(HRESULT,QueryContextMenu,       HMENU,hmenu, UINT,indexMenu, UINT,idCmdFirst, UINT,idCmdLast, UINT,uFlags) \
    ICOM_METHOD1(HRESULT,InvokeCommand,       LPCMINVOKECOMMANDINFO,lpici) \
    ICOM_METHOD5(HRESULT,GetCommandString,       UINT,idCmd, UINT,uType, UINT*,pwReserved, LPSTR,pszName, UINT,cchMax) \
    ICOM_METHOD3(HRESULT,HandleMenuMsg,        UINT,uMsg,WPARAM,wParam,LPARAM,lParam) \
    void * guard;   /*possibly another nasty entry from ContextMenu3 ?*/
#define IContextMenu_IMETHODS \
		IUnknown_IMETHODS \
    IContextMenu_METHODS
ICOM_DEFINE(IContextMenu, IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
// *** IUnknown methods *** //
#define IContextMenu_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IContextMenu_AddRef(p)             ICOM_CALL (AddRef,p)
#define IContextMenu_Release(p)            ICOM_CALL (Release,p)
// *** IContextMenu methods *** //
#define IContextMenu_QueryContextMenu(p,a,b,c,d,e)     ICOM_CALL5(QueryContextMenu,p,a,b,c,d,e)
#define IContextMenu_InvokeCommand(p,a)                ICOM_CALL1(InvokeCommand,p,a)
#define IContextMenu_GetCommandString(p,a,b,c,d,e)     ICOM_CALL5(GetCommandString,p,a,b,c,d,e)
#define IContextMenu_HandleMenuMsg(p,a,b,c)            ICOM_CALL3(HandleMenuMsg,p,a,b,c)
#endif

/*****************************************************************************
 * structures for shell clipboard formats
 */
typedef enum tagDVASPECT
{	DVASPECT_CONTENT	= 1,
        DVASPECT_THUMBNAIL	= 2,
        DVASPECT_ICON	= 4,
        DVASPECT_DOCPRINT	= 8
} DVASPECT;

/* shell specific clipboard formats */

/* DATAOBJECT_InitShellIDList*/
#define CFSTR_SHELLIDLIST       "Shell IDList Array"      /* CF_IDLIST */

extern UINT cfShellIDList;

typedef struct
{	UINT cidl;
	UINT aoffset[1];
} CIDA, *LPCIDA;

#define CFSTR_SHELLIDLISTOFFSET "Shell Object Offsets"    /* CF_OBJECTPOSITIONS */
#define CFSTR_NETRESOURCES      "Net Resource"            /* CF_NETRESOURCE */

/* DATAOBJECT_InitFileGroupDesc */
#define CFSTR_FILEDESCRIPTORA   "FileGroupDescriptor"     /* CF_FILEGROUPDESCRIPTORA */
extern UINT cfFileGroupDesc;

#define CFSTR_FILEDESCRIPTORW   "FileGroupDescriptorW"    /* CF_FILEGROUPDESCRIPTORW */

/* DATAOBJECT_InitFileContents*/
#define CFSTR_FILECONTENTS      "FileContents"            /* CF_FILECONTENTS */
extern UINT cfFileContents;

#define CFSTR_FILENAMEA         "FileName"                /* CF_FILENAMEA */
#define CFSTR_FILENAMEW         "FileNameW"               /* CF_FILENAMEW */
#define CFSTR_PRINTERGROUP      "PrinterFriendlyName"     /* CF_PRINTERS */
#define CFSTR_FILENAMEMAPA      "FileNameMap"             /* CF_FILENAMEMAPA */
#define CFSTR_FILENAMEMAPW      "FileNameMapW"            /* CF_FILENAMEMAPW */
#define CFSTR_SHELLURL          "UniformResourceLocator"
#define CFSTR_PREFERREDDROPEFFECT "Preferred DropEffect"
#define CFSTR_PERFORMEDDROPEFFECT "Performed DropEffect"
#define CFSTR_PASTESUCCEEDED    "Paste Succeeded"
#define CFSTR_INDRAGLOOP        "InShellDragLoop"

/**************************************************************************
 *  IDLList "Item ID List List"
 *
 *  NOTES
 *   interal data holder for IDataObject
 */
typedef struct tagLPIDLLIST	*LPIDLLIST,	IDLList;

#define THIS LPIDLLIST this

typedef enum
{	State_UnInit=1,
	State_Init=2,
	State_OutOfMem=3
} IDLListState;
 
typedef struct IDLList_VTable
{	STDMETHOD_(UINT, GetState)(THIS);
	STDMETHOD_(LPITEMIDLIST, GetElement)(THIS_ UINT nIndex);
	STDMETHOD_(UINT, GetCount)(THIS);
	STDMETHOD_(BOOL, StoreItem)(THIS_ LPITEMIDLIST pidl);
	STDMETHOD_(BOOL, AddItems)(THIS_ LPITEMIDLIST *apidl, UINT cidl);
	STDMETHOD_(BOOL, InitList)(THIS);
	STDMETHOD_(void, CleanList)(THIS);
} IDLList_VTable,*LPIDLLIST_VTABLE;

struct tagLPIDLLIST
{	LPIDLLIST_VTABLE	lpvtbl;
	HDPA	dpa;
	UINT	uStep;
};

extern LPIDLLIST IDLList_Constructor (UINT uStep);
extern void IDLList_Destructor(LPIDLLIST this);
#undef THIS


/*****************************************************************************
 * IShellExtInit interface
 */
#define THIS LPSHELLEXTINIT this

typedef struct IShellExtInit_VTable 
{   /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IShellExtInit methods *** */
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID) PURE;
} IShellExtInit_VTable,*LPSHELLEXTINIT_VTABLE;

struct tagSHELLEXTINIT
{ LPSHELLEXTINIT_VTABLE	lpvtbl;
  DWORD			 ref;
};

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
    STDMETHOD_(BOOL,CreateEnumList)(THIS_ LPCSTR, DWORD) PURE;
    STDMETHOD_(BOOL,AddToEnumList)(THIS_ LPITEMIDLIST) PURE;
    STDMETHOD_(BOOL,DeleteList)(THIS) PURE;

		
} IEnumIDList_VTable,*LPENUMIDLIST_VTABLE;

struct tagENUMIDLIST
{ LPENUMIDLIST_VTABLE	lpvtbl;
  DWORD		 ref;
  LPENUMLIST mpFirst;
  LPENUMLIST mpLast;
  LPENUMLIST mpCurrent;
};

#undef THIS
/*-------------------------------------------------------------------------- */
/* */
/* FOLDERSETTINGS */
/* */
/*  FOLDERSETTINGS is a data structure that explorer passes from one folder */
/* view to another, when the user is browsing. It calls ISV::GetCurrentInfo */
/* member to get the current settings and pass it to ISV::CreateViewWindow */
/* to allow the next folder view "inherit" it. These settings assumes a */
/* particular UI (which the shell's folder view has), and shell extensions */
/* may or may not use those settings. */
/* */
/*-------------------------------------------------------------------------- */

typedef LPBYTE LPVIEWSETTINGS;

/* NB Bitfields. */
/* FWF_DESKTOP implies FWF_TRANSPARENT/NOCLIENTEDGE/NOSCROLL */
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
  FWF_SINGLECLICKACTIVATE=0x8000  /* TEMPORARY -- NO UI FOR THIS */
} FOLDERFLAGS;

typedef enum
{ FVM_ICON =              1,
  FVM_SMALLICON =         2,
  FVM_LIST =              3,
  FVM_DETAILS =           4
} FOLDERVIEWMODE;

typedef struct
{ UINT ViewMode;       /* View mode (FOLDERVIEWMODE values) */
  UINT fFlags;         /* View options (FOLDERFLAGS bits) */
} FOLDERSETTINGS, *LPFOLDERSETTINGS;

typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;

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
    STDMETHOD(ParseDisplayName) (THIS_ HWND hwndOwner,LPBC pbcReserved, LPOLESTR lpszDisplayName,ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;
    STDMETHOD(EnumObjects)( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList) PURE;
    STDMETHOD(BindToObject)(THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(BindToStorage)(THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD(CompareIDs)(THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject)(THIS_ HWND hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(GetAttributesOf)(THIS_ UINT cidl, LPCITEMIDLIST * apidl,ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)(THIS_ HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf)(THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)(THIS_ HWND hwndOwner, LPCITEMIDLIST pidl,LPCOLESTR lpszName, DWORD uFlags,LPITEMIDLIST * ppidlOut) PURE;

	/* utility functions */
   STDMETHOD_(BOOL,GetFolderPath)(THIS_ LPSTR, DWORD);
   
} *LPSHELLFOLDER_VTABLE,IShellFolder_VTable;

struct tagSHELLFOLDER {
	LPSHELLFOLDER_VTABLE	lpvtbl;
	DWORD			ref;
	LPSTR			sMyPath;
	LPITEMIDLIST		pMyPidl;
	LPITEMIDLIST		mpidl;
};

extern LPSHELLFOLDER pdesktopfolder;

/************************
* Shellfolder API
*/
DWORD WINAPI SHGetDesktopFolder(LPSHELLFOLDER *);
#undef THIS

/************************************************************************
* IShellBrowser interface
*/
#define THIS LPSHELLBROWSER this
/* targets for GetWindow/SendControlMsg */
#define FCW_STATUS		0x0001
#define FCW_TOOLBAR		0x0002
#define FCW_TREE		0x0003
#define FCW_INTERNETBAR		0x0006
#define FCW_PROGRESS		0x0008

/* wFlags for BrowseObject*/
#define SBSP_DEFBROWSER		0x0000
#define SBSP_SAMEBROWSER	0x0001
#define SBSP_NEWBROWSER		0x0002

#define SBSP_DEFMODE		0x0000
#define SBSP_OPENMODE		0x0010
#define SBSP_EXPLOREMODE	0x0020

#define SBSP_ABSOLUTE		0x0000
#define SBSP_RELATIVE		0x1000
#define SBSP_PARENT		0x2000
#define SBSP_NAVIGATEBACK	0x4000
#define SBSP_NAVIGATEFORWARD	0x8000

#define SBSP_ALLOW_AUTONAVIGATE		0x10000

#define SBSP_INITIATEDBYHLINKFRAME	0x80000000
#define SBSP_REDIRECT			0x40000000
#define SBSP_WRITENOHISTORY		0x08000000

/* uFlage for SetToolbarItems */
#define FCT_MERGE       0x0001
#define FCT_CONFIGABLE  0x0002
#define FCT_ADDTOEND    0x0004

/* undocumented, found in the web posted by Chris Becke */ 
#define CWM_SETPATH	(WM_USER+2)
#define CWM_WANTIDLE	(WM_USER+3)
#define CWM_GETSETCURRENTINFO	(WM_USER+4)
#define CWM_SELECTITEM	(WM_USER+5)
#define CWM_STOPWAITING	(WM_USER+6)
#define CWM_GETISHELLBROWSER (WM_USER+7)

typedef struct IShellBrowser_VTable 
{    /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IOleWindow methods *** */
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    /* *** IShellBrowser methods *** (same as IOleInPlaceFrame) */
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) PURE;
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenuReserved, HWND hwndActiveObject) PURE;
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText) PURE;
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID) PURE;

    /* *** IShellBrowser methods *** */
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags) PURE;
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode, LPSTREAM  *ppStrm) PURE;
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND * lphwnd) PURE;
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * pret) PURE;
    STDMETHOD(QueryActiveShellView)(THIS_ IShellView ** ppshv) PURE;
    STDMETHOD(OnViewWindowActive)(THIS_ IShellView * ppshv) PURE;
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) PURE;
} *LPSHELLBROWSER_VTABLE,IShellBrowser_VTable;

struct tagSHELLBROWSER 
{ LPSHELLBROWSER_VTABLE	lpvtbl;
  DWORD	ref;
};

#undef THIS

/************************************************************************
* IShellView interface
*/
#define THIS LPSHELLVIEW this

/* shellview select item flags*/
#define SVSI_DESELECT   0x0000
#define SVSI_SELECT     0x0001
#define SVSI_EDIT       0x0003  /* includes select */
#define SVSI_DESELECTOTHERS 0x0004
#define SVSI_ENSUREVISIBLE  0x0008
#define SVSI_FOCUSED        0x0010

/* shellview get item object flags */
#define SVGIO_BACKGROUND    0x00000000
#define SVGIO_SELECTION     0x00000001
#define SVGIO_ALLVIEW       0x00000002

/* The explorer dispatches WM_COMMAND messages based on the range of
 command/menuitem IDs. All the IDs of menuitems that the view (right
 pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer
 won't dispatch them). The view should not deal with any menuitems
 in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future
 version of the shell).

  FCIDM_SHVIEWFIRST/LAST      for the right pane (IShellView)
  FCIDM_BROWSERFIRST/LAST     for the explorer frame (IShellBrowser)
  FCIDM_GLOBAL/LAST           for the explorer's submenu IDs
*/
#define FCIDM_SHVIEWFIRST	0x0000
/* undocumented */
#define FCIDM_SHVIEW_ARRANGE	0x7001
#define FCIDM_SHVIEW_DELETE	0x7011
#define FCIDM_SHVIEW_PROPERTIES	0x7013
#define FCIDM_SHVIEW_CUT	0x7018
#define FCIDM_SHVIEW_COPY	0x7019
#define FCIDM_SHVIEW_INSERT	0x701A
#define FCIDM_SHVIEW_UNDO	0x701B
#define FCIDM_SHVIEW_INSERTLINK	0x701C
#define FCIDM_SHVIEW_SELECTALL	0x7021
#define FCIDM_SHVIEW_INVERTSELECTION	0x7022
#define FCIDM_SHVIEW_BIGICON	0x7029
#define FCIDM_SHVIEW_SMALLICON	0x702A
#define FCIDM_SHVIEW_LISTVIEW	0x702B	
#define FCIDM_SHVIEW_REPORTVIEW	0x702C
#define FCIDM_SHVIEW_AUTOARRANGE	0x7031  
#define FCIDM_SHVIEW_SNAPTOGRID	0x7032
#define FCIDM_SHVIEW_HELP	0x7041

#define FCIDM_SHVIEWLAST	0x7fff
#define FCIDM_BROWSERFIRST	0xA000
/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_SMALLICON	0xA003
#define FCIDM_TB_REPORTVIEW	0xA004

#define FCIDM_BROWSERLAST	0xbf00
#define FCIDM_GLOBALFIRST	0x8000
#define FCIDM_GLOBALLAST	0x9fff

/*
* Global submenu IDs and separator IDs
*/
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

/* control IDs known to the view */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)

/* uState values for IShellView::UIActivate */
typedef enum 
{ SVUIA_DEACTIVATE       = 0,
  SVUIA_ACTIVATE_NOFOCUS = 1,
  SVUIA_ACTIVATE_FOCUS   = 2,
  SVUIA_INPLACEACTIVATE  = 3          /* new flag for IShellView2 */
} SVUIA_STATUS;



typedef struct IShellView_VTable
{   /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IOleWindow methods *** */
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    /* *** IShellView methods *** */
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(UIActivate) (THIS_ UINT uState) PURE;
    STDMETHOD(Refresh) (THIS) PURE;
    STDMETHOD(CreateViewWindow)(THIS_ IShellView *lpPrevView,LPCFOLDERSETTINGS lpfs, IShellBrowser * psb,RECT * prcView, HWND  *phWnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT uFlags) PURE;
    STDMETHOD(GetItemObject)(THIS_ UINT uItem, REFIID riid,LPVOID *ppv) PURE;
} IShellView_VTable,*LPSHELLVIEW_VTABLE;

struct tagSHELLVIEW 
{ LPSHELLVIEW_VTABLE lpvtbl;
  DWORD		ref;
  LPITEMIDLIST	mpidl;
  LPSHELLFOLDER	pSFParent;
  LPSHELLBROWSER	pShellBrowser;
  LPCOMMDLGBROWSER	pCommDlgBrowser;
  HWND	hWnd;
  HWND	hWndList;
  HWND	hWndParent;
  FOLDERSETTINGS	FolderSettings;
  HMENU	hMenu;
  UINT	uState;
  UINT	uSelected;
  LPITEMIDLIST	*aSelectedItems;
};

typedef GUID SHELLVIEWID;
#define SV_CLASS_NAME   ("SHELLDLL_DefView")

#undef THIS
/****************************************************************************
 * ICommDlgBrowser interface
 */
#define THIS LPCOMMDLGBROWSER this

/* for OnStateChange*/
#define CDBOSC_SETFOCUS     0x00000000
#define CDBOSC_KILLFOCUS    0x00000001
#define CDBOSC_SELCHANGE    0x00000002
#define CDBOSC_RENAME       0x00000003

typedef struct ICommDlgBrowser_VTable
{   /* IUnknown methods */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* ICommDlgBrowser methods */
    STDMETHOD(OnDefaultCommand) (THIS_  LPSHELLVIEW ppshv) PURE;
    STDMETHOD(OnStateChange) (THIS_ LPSHELLVIEW ppshv, ULONG uChange) PURE;
    STDMETHOD(IncludeObject) (THIS_ LPSHELLVIEW ppshv, LPCITEMIDLIST pidl) PURE;
} ICommDlgBrowser_VTable,*LPCOMMDLGBROWSER_VTABLE;

struct tagCOMMDLGBROWSER
{ LPCOMMDLGBROWSER_VTABLE lpvtbl;
  DWORD			     ref;
};
#undef THIS

/****************************************************************************
 * IExtractIconinterface
 *
 * FIXME
 *  Is the ExtractIconA interface
 */
#define THIS LPEXTRACTICON this

/* GetIconLocation() input flags*/
#define GIL_OPENICON     0x0001      /* allows containers to specify an "open" look */
#define GIL_FORSHELL     0x0002      /* icon is to be displayed in a ShellFolder */
#define GIL_ASYNC        0x0020      /* this is an async extract, return E_ASYNC */

/* GetIconLocation() return flags */
#define GIL_SIMULATEDOC  0x0001      /* simulate this document icon for this */
#define GIL_PERINSTANCE  0x0002      /* icons from this class are per instance (each file has its own) */
#define GIL_PERCLASS     0x0004      /* icons from this class per class (shared for all files of this type) */
#define GIL_NOTFILENAME  0x0008      /* location is not a filename, must call ::ExtractIcon */
#define GIL_DONTCACHE    0x0010      /* this icon should not be cached */

typedef struct IExtractIcon IExtractIcon,*LPEXTRACTICON;
typedef struct IExtractIcon_VTable
{ /*** IUnknown methods ***/
  STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
  STDMETHOD_(ULONG,Release) (THIS) PURE;

  /*** IExtractIcon methods ***/
  STDMETHOD(GetIconLocation)(THIS_ UINT uFlags, LPSTR szIconFile, UINT cchMax,INT * piIndex, UINT * pwFlags) PURE;
  STDMETHOD(Extract)(THIS_ LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) PURE;
}IExtractIcon_VTable,*LPEXTRACTICON_VTABLE;

struct IExtractIcon 
{ LPEXTRACTICON_VTABLE lpvtbl;
  DWORD ref;
  LPITEMIDLIST pidl;
};

#undef THIS

DWORD WINAPI SHMapPIDLToSystemImageListIndex(LPSHELLFOLDER sh,LPITEMIDLIST pidl,DWORD z);

/****************************************************************************
 * IShellIcon interface
 */

#define THIS LPSHELLICON this

typedef struct IShellIcon_VTable
{ /*** IUnknown methods ***/
  STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
  STDMETHOD_(ULONG,Release) (THIS) PURE;

  /*** IShellIcon methods ***/
  STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex) PURE;
} IShellIcon_VTable,*LPSHELLICON_VTABLE;

struct tagSHELLICON
{ LPSHELLICON_VTABLE lpvtbl;
  DWORD ref;
};
#undef THIS
/****************************************************************************
 * IDockingWindowFrame interface
 */
#define THIS LPDOCKINGWINDOWFRAME this

#define DWFRF_NORMAL		0x0000  /* femove toolbar flags*/
#define DWFRF_DELETECONFIGDATA	0x0001
#define DWFAF_HIDDEN		0x0001   /* add tolbar*/

typedef struct IDockingWindowFrame_VTable
{   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /*** IOleWindow methods ***/
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    /*** IDockingWindowFrame methods ***/
    STDMETHOD(AddToolbar) (THIS_ IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwAddFlags) PURE;
    STDMETHOD(RemoveToolbar) (THIS_ IUnknown* punkSrc, DWORD dwRemoveFlags) PURE;
    STDMETHOD(FindToolbar) (THIS_ LPCWSTR pwszItem, REFIID riid, LPVOID* ppvObj) PURE;
} IDockingWindowFrame_VTable, *LPDOCKINGWINDOWFRAME_VTABLE;

struct tagDOCKINGWINDOWFRAME
{ LPDOCKINGWINDOWFRAME_VTABLE lpvtbl;
  DWORD ref;
};

#undef THIS
/****************************************************************************
 * Shell Execute API
 */
#define SE_ERR_FNF              2       /* file not found */
#define SE_ERR_PNF              3       /* path not found */
#define SE_ERR_ACCESSDENIED     5       /* access denied */
#define SE_ERR_OOM              8       /* out of memory */
#define SE_ERR_DLLNOTFOUND      32
#define SE_ERR_SHARE                    26
#define SE_ERR_ASSOCINCOMPLETE          27
#define SE_ERR_DDETIMEOUT               28
#define SE_ERR_DDEFAIL                  29
#define SE_ERR_DDEBUSY                  30
#define SE_ERR_NOASSOC                  31

#define SEE_MASK_CLASSNAME        0x00000001
#define SEE_MASK_CLASSKEY         0x00000003
#define SEE_MASK_IDLIST           0x00000004
#define SEE_MASK_INVOKEIDLIST     0x0000000c
#define SEE_MASK_ICON             0x00000010
#define SEE_MASK_HOTKEY           0x00000020
#define SEE_MASK_NOCLOSEPROCESS   0x00000040
#define SEE_MASK_CONNECTNETDRV    0x00000080
#define SEE_MASK_FLAG_DDEWAIT     0x00000100
#define SEE_MASK_DOENVSUBST       0x00000200
#define SEE_MASK_FLAG_NO_UI       0x00000400
#define SEE_MASK_UNICODE          0x00004000
#define SEE_MASK_NO_CONSOLE       0x00008000
#define SEE_MASK_ASYNCOK          0x00100000
#define SEE_MASK_HMONITOR         0x00200000

typedef struct _SHELLEXECUTEINFOA
{       DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCSTR   lpVerb;
        LPCSTR   lpFile;
        LPCSTR   lpParameters;
        LPCSTR   lpDirectory;
       INT nShow;
        HINSTANCE hInstApp;
        /* Optional fields */
        LPVOID lpIDList;
        LPCSTR   lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union 
        { HANDLE hIcon;
          HANDLE hMonitor;
        } u;
        HANDLE hProcess;
} SHELLEXECUTEINFOA, *LPSHELLEXECUTEINFOA;

typedef struct _SHELLEXECUTEINFOW
{       DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCWSTR  lpVerb;
        LPCWSTR  lpFile;
        LPCWSTR  lpParameters;
        LPCWSTR  lpDirectory;
       INT nShow;
        HINSTANCE hInstApp;
        /* Optional fields*/
        LPVOID lpIDList;
        LPCWSTR  lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union
        { HANDLE hIcon;
          HANDLE hMonitor;
        } u;
        HANDLE hProcess;
} SHELLEXECUTEINFOW, *LPSHELLEXECUTEINFOW;

#define SHELLEXECUTEINFO   WINELIB_NAME_AW(SHELLEXECUTEINFO)
#define LPSHELLEXECUTEINFO WINELIB_NAME_AW(LPSHELLEXECUTEINFO)

BOOL WINAPI ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
#define ShellExecuteEx  WINELIB_NAME_AW(ShellExecuteEx)

void WINAPI WinExecErrorA(HWND hwnd,INT error, LPCSTR lpstrFileName, LPCSTR lpstrTitle);
void WINAPI WinExecErrorW(HWND hwnd,INT error, LPCWSTR lpstrFileName, LPCWSTR lpstrTitle);
#define WinExecError  WINELIB_NAME_AW(WinExecError)



/****************************************************************************
 * SHBrowseForFolder API
 */
typedef INT (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct tagBROWSEINFOA {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR         pszDisplayName;
    LPCSTR        lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
	INT         iImage;
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

typedef struct tagBROWSEINFOW {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
	INT         iImage;
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW; 

#define BROWSEINFO   WINELIB_NAME_AW(BROWSEINFO)
#define PBROWSEINFO  WINELIB_NAME_AW(PBROWSEINFO)
#define LPBROWSEINFO WINELIB_NAME_AW(LPBROWSEINFO)

/* Browsing for directory. */
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
 
#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000

/* message from browser */
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_VALIDATEFAILEDA    3   /* lParam:szPath ret:1(cont),0(EndDialog) */
#define BFFM_VALIDATEFAILEDW    4   /* lParam:wzPath ret:1(cont),0(EndDialog) */

/* messages to browser */
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

LPITEMIDLIST WINAPI SHBrowseForFolderA(LPBROWSEINFOA lpbi);
/*LPITEMIDLIST WINAPI SHBrowseForFolder32W(LPBROWSEINFO32W lpbi);*/
#define  SHBrowseForFolder WINELIB_NAME_AW(SHBrowseForFolder)

/****************************************************************************
*	SHGetDataFromIDList API
*/
#define SHGDFIL_FINDDATA        1
#define SHGDFIL_NETRESOURCE     2
#define SHGDFIL_DESCRIPTIONID   3

#define SHDID_ROOT_REGITEM          1
#define SHDID_FS_FILE               2
#define SHDID_FS_DIRECTORY          3
#define SHDID_FS_OTHER              4
#define SHDID_COMPUTER_DRIVE35      5
#define SHDID_COMPUTER_DRIVE525     6
#define SHDID_COMPUTER_REMOVABLE    7
#define SHDID_COMPUTER_FIXED        8
#define SHDID_COMPUTER_NETDRIVE     9
#define SHDID_COMPUTER_CDROM        10
#define SHDID_COMPUTER_RAMDISK      11
#define SHDID_COMPUTER_OTHER        12
#define SHDID_NET_DOMAIN            13
#define SHDID_NET_SERVER            14
#define SHDID_NET_SHARE             15
#define SHDID_NET_RESTOFNET         16
#define SHDID_NET_OTHER             17

typedef struct _SHDESCRIPTIONID 
{   DWORD   dwDescriptionId;
    CLSID   clsid;
} SHDESCRIPTIONID, *LPSHDESCRIPTIONID;

HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
#define  SHGetDataFromIDList WINELIB_NAME_AW(SHGetDataFromIDList)


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
                         HWND hwnd,UINT uMsg,UINT wParam,LPARAM lParam);

/* NF valid values for the "viewmode" item of the SHELLTEMPLATE*/
#define NF_INHERITVIEW    0x0000
#define NF_LOCALVIEW        0x0001

typedef struct _SHELLVIEWDATA   /* idl */
{ DWORD           dwSize;
  LPSHELLFOLDER   pShellFolder;
  DWORD           dwUserParam;
  LPCITEMIDLIST   pidl;
  DWORD           v3;        /* always 0 */
  SHELLVIEWPROC   pCallBack;
  DWORD           viewmode;  /* NF_* enum */
} SHELLVIEWDATA, * LPSHELLVIEWDATA;

/*
 The shell keeps track of some per-user state to handle display
 options that is of majorinterest to ISVs.
 The key one requested right now is "DoubleClickInWebView".
*/
typedef struct 
{   BOOL fShowAllObjects : 1;
    BOOL fShowExtensions : 1;
    BOOL fNoConfirmRecycle : 1;
    BOOL fShowSysFiles : 1;
    BOOL fShowCompColor : 1;
    BOOL fDoubleClickInWebView : 1;
    BOOL fDesktopHTML : 1;
    BOOL fWin95Classic : 1;
    BOOL fDontPrettyPath : 1;
    BOOL fShowAttribCol : 1;
    BOOL fMapNetDrvBtn : 1;
    BOOL fShowInfoTip : 1;
    BOOL fHideIcons : 1;
    UINT fRestFlags : 3;
} SHELLFLAGSTATE, * LPSHELLFLAGSTATE;


DWORD WINAPI SHGetMalloc(LPMALLOC *lpmal) ;

#undef PURE
#undef FAR
#undef THIS
#undef THIS_
#undef STDMETHOD
#undef STDMETHOD_

#endif /* __WINE_SHLOBJ_H */
