#ifndef _WINE_SHLOBJ_H
#define _WINE_SHLOBJ_H

#include "shell.h"
#include "ole.h"
#include "ole2.h"
#include "compobj.h"
#include "storage.h"
#include "commctrl.h"
#include "interfaces.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

#define __T(x)      x
#define _T(x)       __T(x)
#define TEXT        _T

/****************************************************************************
*  DllGetClassObject
*/
DWORD WINAPI SHELL32_DllGetClassObject(LPCLSID,REFIID,LPVOID*);


typedef LPVOID	LPBC; /* *IBindCtx really */

/* foreward declaration of the objects*/
typedef struct tagCONTEXTMENU	*LPCONTEXTMENU,	IContextMenu;
typedef struct tagSHELLEXTINIT	*LPSHELLEXTINIT,IShellExtInit;
typedef struct tagENUMIDLIST	*LPENUMIDLIST,	IEnumIDList;
typedef struct tagSHELLFOLDER	*LPSHELLFOLDER,	IShellFolder;
typedef struct tagSHELLVIEW	*LPSHELLVIEW,	IShellView;
typedef struct tagSHELLBROWSER	*LPSHELLBROWSER,IShellBrowser;
typedef struct tagDATAOBJECT	*LPDATAOBJECT,	IDataObject;
typedef struct tagSHELLICON	*LPSHELLICON,	IShellIcon;
typedef struct tagDOCKINGWINDOWFRAME	*LPDOCKINGWINDOWFRAME,	IDockingWindowFrame;
typedef struct tagSERVICEPROVIDER	*LPSERVICEPROVIDER,	IServiceProvider;
typedef struct tagCOMMDLGBROWSER	*LPCOMMDLGBROWSER,	ICommDlgBrowser;
typedef struct tagENUMFORMATETC	*LPENUMFORMATETC,	IEnumFORMATETC;
 
typedef struct IAdviseSink		IAdviseSink,	*LPIADVISESINK;
typedef struct IEnumSTATDATA		IEnumSTATDATA,	*LPENUMSTATDATA;
/****************************************************************************
*  SHELL ID
*/
/* strange Objects */
DEFINE_SHLGUID(IID_IEnumUnknown,	0x00000100L, 0, 0);
DEFINE_SHLGUID(IID_IEnumString,		0x00000101L, 0, 0);
DEFINE_SHLGUID(IID_IEnumMoniker,	0x00000102L, 0, 0);
DEFINE_SHLGUID(IID_IEnumFORMATETC,	0x00000103L, 0, 0);
DEFINE_SHLGUID(IID_IEnumOLEVERB,	0x00000104L, 0, 0);
DEFINE_SHLGUID(IID_IEnumSTATDATA,	0x00000105L, 0, 0);

DEFINE_SHLGUID(IID_IPersistStream,	0x00000109L, 0, 0);
DEFINE_SHLGUID(IID_IPersistStorage,	0x0000010AL, 0, 0);
DEFINE_SHLGUID(IID_IPersistFile,	0x0000010BL, 0, 0);
DEFINE_SHLGUID(IID_IPersist,		0x0000010CL, 0, 0);
DEFINE_SHLGUID(IID_IViewObject,		0x0000010DL, 0, 0);
DEFINE_SHLGUID(IID_IDataObject,		0x0000010EL, 0, 0);

DEFINE_GUID (IID_IServiceProvider,	0x6D5140C1L, 0x7436, 0x11CE, 0x80, 0x34, 0x00, 0xAA, 0x00, 0x60, 0x09, 0xFA);
DEFINE_GUID (IID_IDockingWindow,	0x012dd920L, 0x7B26, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);
DEFINE_GUID (IID_IDockingWindowSite,	0x2A342FC2L, 0x7B26, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);
DEFINE_GUID (IID_IDockingWindowFrame,	0x47D2657AL, 0x7B27, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);

DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);
/* shell32 formatids */
DEFINE_SHLGUID(FMTID_Intshcut,          0x000214A0L, 0, 0);
DEFINE_SHLGUID(FMTID_InternetSite,      0x000214A1L, 0, 0);
/* command group ids */
DEFINE_SHLGUID(CGID_Explorer,           0x000214D0L, 0, 0);
DEFINE_SHLGUID(CGID_ShellDocView,       0x000214D1L, 0, 0);

 /* shell32interface ids */
DEFINE_SHLGUID(IID_INewShortcutHookA,   0x000214E1L, 0, 0);
DEFINE_SHLGUID(IID_IShellBrowser,       0x000214E2L, 0, 0);
#define SID_SShellBrowser IID_IShellBrowser
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
    UINT32	uOffset;	/* OffsetINT32o SHITEMID (ANSI) */
    char	cStr[MAX_PATH];	/* Buffer to fill in */
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
#define CMF_RESERVED            0xffff0000      // View specific

/* GetCommandString uFlags */
#define GCS_VERBA        0x00000000     // canonical verb
#define GCS_HELPTEXTA    0x00000001     // help text (for status bar)
#define GCS_VALIDATEA    0x00000002     // validate command exists
#define GCS_VERBW        0x00000004     // canonical verb (unicode)
#define GCS_HELPTEXTW    0x00000005     // help text (unicode version)
#define GCS_VALIDATEW    0x00000006     // validate command exists (unicode)
#define GCS_UNICODE      0x00000004     // for bit testing - Unicode string

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
{   DWORD cbSize;        // sizeof(CMINVOKECOMMANDINFO)
    DWORD fMask;         // any combination of CMIC_MASK_*
    HWND32 hwnd;         // might be NULL (indicating no owner window)
    LPCSTR lpVerb;       // either a string or MAKEINTRESOURCE(idOffset)
    LPCSTR lpParameters; // might be NULL (indicating no parameter)
    LPCSTR lpDirectory;  // might be NULL (indicating no specific directory)
   INT32 nShow;           // one of SW_ values for ShowWindow() API

    DWORD dwHotKey;
    HANDLE32 hIcon;
} CMINVOKECOMMANDINFO32,  *LPCMINVOKECOMMANDINFO32;

typedef struct tagCMInvokeCommandInfoEx 
{   DWORD cbSize;        // must be sizeof(CMINVOKECOMMANDINFOEX)
    DWORD fMask;         // any combination of CMIC_MASK_*
    HWND32 hwnd;         // might be NULL (indicating no owner window)
    LPCSTR lpVerb;       // either a string or MAKEINTRESOURCE(idOffset)
    LPCSTR lpParameters; // might be NULL (indicating no parameter)
    LPCSTR lpDirectory;  // might be NULL (indicating no specific directory)
	INT32 nShow;           // one of SW_ values for ShowWindow() API

    DWORD dwHotKey;
    
    HANDLE32 hIcon;
    LPCSTR lpTitle;        // For CreateProcess-StartupInfo.lpTitle
    LPCWSTR lpVerbW;       // Unicode verb (for those who can use it)
    LPCWSTR lpParametersW; // Unicode parameters (for those who can use it)
    LPCWSTR lpDirectoryW;  // Unicode directory (for those who can use it)
    LPCWSTR lpTitleW;      // Unicode title (for those who can use it)
    POINT32 ptInvoke;      // Point where it's invoked

} CMINVOKECOMMANDINFOEX32,  *LPCMINVOKECOMMANDINFOEX32;


typedef struct IContextMenu_VTable
{   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(QueryContextMenu)(THIS_ HMENU32 hmenu,UINT32 indexMenu,UINT32 idCmdFirst, UINT32 idCmdLast,UINT32 uFlags) PURE;
    STDMETHOD(InvokeCommand)(THIS_ LPCMINVOKECOMMANDINFO32 lpici) PURE;
    STDMETHOD(GetCommandString)(THIS_ UINT32 idCmd,UINT32 uType,UINT32 * pwReserved,LPSTR pszName,UINT32 cchMax) PURE;

    /* undocumented not only in ContextMenu2 */
    STDMETHOD(HandleMenuMsg)(THIS_  UINT32 uMsg,WPARAM32 wParam,LPARAM lParam) PURE;

    /* possibly another nasty entry from ContextMenu3 ?*/    
    void * guard;
} IContextMenu_VTable,*LPCONTEXTMENU_VTABLE;

struct tagCONTEXTMENU
{ LPCONTEXTMENU_VTABLE	lpvtbl;
  DWORD			ref;
  LPSHELLFOLDER	pSFParent;
  LPITEMIDLIST	*aPidls;
  BOOL32		bAllValues;
};

#undef THIS
/*****************************************************************************
 * structures for shell clipboard formats
 */
typedef enum tagDVASPECT
{	DVASPECT_CONTENT	= 1,
        DVASPECT_THUMBNAIL	= 2,
        DVASPECT_ICON	= 4,
        DVASPECT_DOCPRINT	= 8
} DVASPECT;

enum tagTYMED
{	TYMED_HGLOBAL   = 1,
	TYMED_FILE      = 2,
	TYMED_ISTREAM   = 4,
	TYMED_ISTORAGE  = 8,
	TYMED_GDI       = 16,
	TYMED_MFPICT    = 32,
	TYMED_ENHMF     = 64,
	TYMED_NULL      = 0
} TYMED;
  
typedef struct
{	DWORD tdSize;
	WORD tdDriverNameOffset;
	WORD tdDeviceNameOffset;
	WORD tdPortNameOffset;
	WORD tdExtDevmodeOffset;
	BYTE tdData[ 1 ];
} DVTARGETDEVICE32;

typedef WORD CLIPFORMAT32, *LPCLIPFORMAT32;

/* dataobject as answer to a request */
typedef struct 
{	DWORD tymed;
	union 
	{ HBITMAP32 hBitmap;
	  /*HMETAFILEPICT32 hMetaFilePict;*/
	  /*HENHMETAFILE32 hEnhMetaFile;*/
	  HGLOBAL32 hGlobal;
	  LPOLESTR32 lpszFileName;
	  IStream32 *pstm;
	  IStorage32 *pstg;
        } u;
	IUnknown *pUnkForRelease;
} STGMEDIUM32;   

/* wished data format */
typedef struct 
{	CLIPFORMAT32 cfFormat;
	DVTARGETDEVICE32 *ptd;
	DWORD dwAspect;
	LONG lindex;
	DWORD tymed;
} FORMATETC32, *LPFORMATETC32;

/* shell specific clipboard formats */

/* DATAOBJECT_InitShellIDList*/
#define CFSTR_SHELLIDLIST       TEXT("Shell IDList Array")      /* CF_IDLIST */

extern UINT32 cfShellIDList;

typedef struct
{	UINT32 cidl;
	UINT32 aoffset[1];
} CIDA, *LPCIDA;

#define CFSTR_SHELLIDLISTOFFSET TEXT("Shell Object Offsets")    /* CF_OBJECTPOSITIONS */
#define CFSTR_NETRESOURCES      TEXT("Net Resource")            /* CF_NETRESOURCE */

/* DATAOBJECT_InitFileGroupDesc */
#define CFSTR_FILEDESCRIPTORA   TEXT("FileGroupDescriptor")     /* CF_FILEGROUPDESCRIPTORA */
extern UINT32 cfFileGroupDesc;

#define CFSTR_FILEDESCRIPTORW   TEXT("FileGroupDescriptorW")    /* CF_FILEGROUPDESCRIPTORW */

/* DATAOBJECT_InitFileContents*/
#define CFSTR_FILECONTENTS      TEXT("FileContents")            /* CF_FILECONTENTS */
extern UINT32 cfFileContents;

#define CFSTR_FILENAMEA         TEXT("FileName")                /* CF_FILENAMEA */
#define CFSTR_FILENAMEW         TEXT("FileNameW")               /* CF_FILENAMEW */
#define CFSTR_PRINTERGROUP      TEXT("PrinterFriendlyName")     /* CF_PRINTERS */
#define CFSTR_FILENAMEMAPA      TEXT("FileNameMap")             /* CF_FILENAMEMAPA */
#define CFSTR_FILENAMEMAPW      TEXT("FileNameMapW")            /* CF_FILENAMEMAPW */
#define CFSTR_SHELLURL          TEXT("UniformResourceLocator")
#define CFSTR_PREFERREDDROPEFFECT TEXT("Preferred DropEffect")
#define CFSTR_PERFORMEDDROPEFFECT TEXT("Performed DropEffect")
#define CFSTR_PASTESUCCEEDED    TEXT("Paste Succeeded")
#define CFSTR_INDRAGLOOP        TEXT("InShellDragLoop")

/**************************************************************************
 *  IDLList "Item ID List List"
 *
 *  NOTES
 *   interal data holder for IDataObject
 */
typedef struct tagLPIDLLIST	*LPIDLLIST,	IDLList;

#define THIS LPIDLLIST this

enum
{	State_UnInit=1,
	State_Init=2,
	State_OutOfMem=3
} IDLListState;
 
typedef struct IDLList_VTable
{	STDMETHOD_(UINT32, GetState)(THIS);
	STDMETHOD_(LPITEMIDLIST, GetElement)(THIS_ UINT32 nIndex);
	STDMETHOD_(UINT32, GetCount)(THIS);
	STDMETHOD_(BOOL32, StoreItem)(THIS_ LPITEMIDLIST pidl);
	STDMETHOD_(BOOL32, AddItems)(THIS_ LPITEMIDLIST *apidl, UINT32 cidl);
	STDMETHOD_(BOOL32, InitList)(THIS);
	STDMETHOD_(void, CleanList)(THIS);
} IDLList_VTable,*LPIDLLIST_VTABLE;

struct tagLPIDLLIST
{	LPIDLLIST_VTABLE	lpvtbl;
	HDPA	dpa;
	UINT32	uStep;
};

extern LPIDLLIST IDLList_Constructor (UINT32 uStep);
extern void IDLList_Destructor(LPIDLLIST this);
#undef THIS
/*****************************************************************************
 * IEnumFORMATETC interface
 */
#define THIS LPENUMFORMATETC this

typedef struct IEnumFORMATETC_VTable 
{    /* IUnknown methods */
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	/* IEnumFORMATETC methods */
	STDMETHOD (Next)(THIS_ ULONG celt, FORMATETC32 *rgelt, ULONG *pceltFethed) PURE;
	STDMETHOD (Skip)(THIS_ ULONG celt) PURE;
	STDMETHOD (Reset)(THIS) PURE;
	STDMETHOD (Clone)(THIS_ IEnumFORMATETC ** ppenum) PURE;
} IEnumFORMATETC_VTable,*LPENUMFORMATETC_VTABLE;

struct tagENUMFORMATETC
{	LPENUMFORMATETC_VTABLE	lpvtbl;
	DWORD			 ref;
        UINT32	posFmt;
        UINT32	countFmt;
        LPFORMATETC32 pFmt;
};

#undef THIS

/*****************************************************************************
 * IDataObject interface
 */
#define THIS LPDATAOBJECT this

typedef struct IDataObject_VTable 
{	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD (GetData )(THIS_ LPFORMATETC32 pformatetcIn, STGMEDIUM32 *pmedium) PURE;
	STDMETHOD (GetDataHere)(THIS_ LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium) PURE;
        STDMETHOD (QueryGetData)(THIS_ LPFORMATETC32 pformatetc) PURE;
        STDMETHOD (GetCanonicalFormatEtc)(THIS_ LPFORMATETC32 pformatectIn, LPFORMATETC32 pformatetcOut) PURE;
        STDMETHOD (SetData)(THIS_ LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium, BOOL32 fRelease) PURE;
        STDMETHOD (EnumFormatEtc)(THIS_ DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) PURE;
        STDMETHOD (DAdvise )(THIS_ LPFORMATETC32 *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) PURE;
        STDMETHOD (DUnadvise)(THIS_ DWORD dwConnection) PURE;
        STDMETHOD (EnumDAdvise)(THIS_ IEnumSTATDATA **ppenumAdvise) PURE;
} IDataObject_VTable,*LPDATAOBJECT_VTABLE;

struct tagDATAOBJECT
{	LPDATAOBJECT_VTABLE	lpvtbl;
	DWORD	ref;
	LPSHELLFOLDER	psf;
	LPIDLLIST  lpill;	/* the data of the dataobject */
	LPITEMIDLIST  pidl;	
};

#undef THIS


/*****************************************************************************
 * IShellExtInit interface
 */
#define THIS LPSHELLEXTINIT this

typedef struct IShellExtInit_VTable 
{   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellExtInit methods ***
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
    STDMETHOD_(BOOL32,CreateEnumList)(THIS_ LPCSTR, DWORD) PURE;
    STDMETHOD_(BOOL32,AddToEnumList)(THIS_ LPITEMIDLIST) PURE;
    STDMETHOD_(BOOL32,DeleteList)(THIS) PURE;

		
} IEnumIDList_VTable,*LPENUMIDLIST_VTABLE;

struct tagENUMIDLIST
{ LPENUMIDLIST_VTABLE	lpvtbl;
  DWORD		 ref;
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

	/* utility functions */
   STDMETHOD_(BOOL32,GetFolderPath)(THIS_ LPSTR, DWORD);
   
} *LPSHELLFOLDER_VTABLE,IShellFolder_VTable;

struct tagSHELLFOLDER {
	LPSHELLFOLDER_VTABLE	lpvtbl;
	DWORD			ref;
	LPSTR			mlpszFolder;
	LPITEMIDLIST	mpidl;
	LPITEMIDLIST	mpidlNSRoot;
	LPSHELLFOLDER	mpSFParent;
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
* IShellView interface
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
#define FCIDM_SHVIEWFIRST           0x0000
#define FCIDM_SHVIEWLAST            0x7fff
#define FCIDM_BROWSERFIRST          0xa000
#define FCIDM_BROWSERLAST           0xbf00
#define FCIDM_GLOBALFIRST           0x8000
#define FCIDM_GLOBALLAST            0x9fff

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
} IShellView_VTable,*LPSHELLVIEW_VTABLE;

struct tagSHELLVIEW 
{ LPSHELLVIEW_VTABLE lpvtbl;
  DWORD		ref;
  LPITEMIDLIST	mpidl;
  LPSHELLFOLDER	pSFParent;
  LPSHELLBROWSER	pShellBrowser;
  LPCOMMDLGBROWSER	pCommDlgBrowser;
  HWND32	hWnd;
  HWND32	hWndList;
  HWND32	hWndParent;
  FOLDERSETTINGS	FolderSettings;
  HMENU32	hMenu;
  UINT32	uState;
  UINT32	uSelected;
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

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile,INT32 cchMaxPath, WIN32_FIND_DATA32A *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName,INT32 cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir,INT32 cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs,INT32 cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ INT32 *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ INT32 iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath,INT32 cchIconPath,INT32 *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath,INT32 iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND32 hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
} IShellLink_VTable,*LPSHELLLINK_VTABLE;

struct IShellLink {
	LPSHELLLINK_VTABLE	lpvtbl;
	DWORD			ref;
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
#define GIL_OPENICON     0x0001      // allows containers to specify an "open" look
#define GIL_FORSHELL     0x0002      // icon is to be displayed in a ShellFolder
#define GIL_ASYNC        0x0020      // this is an async extract, return E_ASYNC

/* GetIconLocation() return flags */
#define GIL_SIMULATEDOC  0x0001      // simulate this document icon for this
#define GIL_PERINSTANCE  0x0002      // icons from this class are per instance (each file has its own)
#define GIL_PERCLASS     0x0004      // icons from this class per class (shared for all files of this type)
#define GIL_NOTFILENAME  0x0008      // location is not a filename, must call ::ExtractIcon
#define GIL_DONTCACHE    0x0010      // this icon should not be cached

typedef struct IExtractIcon IExtractIcon,*LPEXTRACTICON;
typedef struct IExtractIcon_VTable
{ /*** IUnknown methods ***/
  STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
  STDMETHOD_(ULONG,Release) (THIS) PURE;

  /*** IExtractIcon methods ***/
  STDMETHOD(GetIconLocation)(THIS_ UINT32 uFlags, LPSTR szIconFile, UINT32 cchMax,INT32 * piIndex, UINT32 * pwFlags) PURE;
  STDMETHOD(Extract)(THIS_ LPCSTR pszFile, UINT32 nIconIndex, HICON32 *phiconLarge, HICON32 *phiconSmall, UINT32 nIconSize) PURE;
}IExtractIccon_VTable,*LPEXTRACTICON_VTABLE;

struct IExtractIcon 
{ LPEXTRACTICON_VTABLE lpvtbl;
  DWORD ref;
  LPITEMIDLIST pidl;
};

#undef THIS
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
  STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT32 flags, LPINT32 lpIconIndex) PURE;
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
    STDMETHOD(GetWindow) (THIS_ HWND32 * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL32 fEnterMode) PURE;

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
 * IServiceProvider interface
 */
#define THIS LPSERVICEPROVIDER this

typedef struct IServiceProvider_VTable
{	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(QueryService)(THIS_ REFGUID guidService, REFIID riid, void **ppvObject);
} IServiceProvider_VTable, *LPSERVICEPROVIDER_VTABLE;

struct tagSERVICEPROVIDER
{	LPSERVICEPROVIDER_VTABLE lpvtbl;
	DWORD ref;
};           
/****************************************************************************
 * Class constructors
 */
#ifdef __WINE__
extern LPDATAOBJECT	IDataObject_Constructor();
extern LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT32, const FORMATETC32 []);

extern LPCLASSFACTORY	IClassFactory_Constructor();
extern LPCONTEXTMENU	IContextMenu_Constructor(LPSHELLFOLDER, LPCITEMIDLIST *, UINT32);
extern LPSHELLFOLDER	IShellFolder_Constructor(LPSHELLFOLDER,LPITEMIDLIST);
extern LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER, LPCITEMIDLIST);
extern LPSHELLLINK	IShellLink_Constructor();
extern LPENUMIDLIST	IEnumIDList_Constructor(LPCSTR,DWORD);
extern LPEXTRACTICON	IExtractIcon_Constructor(LPITEMIDLIST);
#endif
/****************************************************************************
 * Shell Execute API
 */
#define SE_ERR_FNF              2       // file not found
#define SE_ERR_PNF              3       // path not found
#define SE_ERR_ACCESSDENIED     5       // access denied
#define SE_ERR_OOM              8       // out of memory
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
        HWND32 hwnd;
        LPCSTR   lpVerb;
        LPCSTR   lpFile;
        LPCSTR   lpParameters;
        LPCSTR   lpDirectory;
       INT32 nShow;
        HINSTANCE32 hInstApp;
        /* Optional fields */
        LPVOID lpIDList;
        LPCSTR   lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union 
        { HANDLE32 hIcon;
          HANDLE32 hMonitor;
        } u;
        HANDLE32 hProcess;
} SHELLEXECUTEINFO32A, *LPSHELLEXECUTEINFO32A;

typedef struct _SHELLEXECUTEINFOW
{       DWORD cbSize;
        ULONG fMask;
        HWND32 hwnd;
        LPCWSTR  lpVerb;
        LPCWSTR  lpFile;
        LPCWSTR  lpParameters;
        LPCWSTR  lpDirectory;
       INT32 nShow;
        HINSTANCE32 hInstApp;
        /* Optional fields*/
        LPVOID lpIDList;
        LPCWSTR  lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union
        { HANDLE32 hIcon;
          HANDLE32 hMonitor;
        } u;
        HANDLE32 hProcess;
} SHELLEXECUTEINFO32W, *LPSHELLEXECUTEINFO32W;

#define SHELLEXECUTEINFO   WINELIB_NAME_AW(SHELLEXECUTEINFO)
#define LPSHELLEXECUTEINFO WINELIB_NAME_AW(LPSHELLEXECUTEINFO)

BOOL32 WINAPI ShellExecuteEx32A(LPSHELLEXECUTEINFO32A lpExecInfo);
BOOL32 WINAPI ShellExecuteEx32W(LPSHELLEXECUTEINFO32W lpExecInfo);
#define ShellExecuteEx  WINELIB_NAME_AW(ShellExecuteEx)

void WINAPI WinExecError32A(HWND32 hwnd,INT32 error, LPCSTR lpstrFileName, LPCSTR lpstrTitle);
void WINAPI WinExecError32W(HWND32 hwnd,INT32 error, LPCWSTR lpstrFileName, LPCWSTR lpstrTitle);
#define WinExecError  WINELIB_NAME_AW(WinExecError)



/****************************************************************************
 * SHBrowseForFolder API
 */
typedef INT32 (CALLBACK* BFFCALLBACK)(HWND32 hwnd, UINT32 uMsg, LPARAM lParam, LPARAM lpData);

typedef struct tagBROWSEINFO32A {
    HWND32        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR         pszDisplayName;
    LPCSTR        lpszTitle;
    UINT32        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
	INT32         iImage;
} BROWSEINFO32A, *PBROWSEINFO32A, *LPBROWSEINFO32A;

typedef struct tagBROWSEINFO32W {
    HWND32        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT32        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
	INT32         iImage;
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

/*
 The shell keeps track of some per-user state to handle display
 options that is of majorinterest to ISVs.
 The key one requested right now is "DoubleClickInWebView".
*/
typedef struct 
{   BOOL32 fShowAllObjects : 1;
    BOOL32 fShowExtensions : 1;
    BOOL32 fNoConfirmRecycle : 1;
    BOOL32 fShowSysFiles : 1;
    BOOL32 fShowCompColor : 1;
    BOOL32 fDoubleClickInWebView : 1;
    BOOL32 fDesktopHTML : 1;
    BOOL32 fWin95Classic : 1;
    BOOL32 fDontPrettyPath : 1;
    BOOL32 fShowAttribCol : 1;
    BOOL32 fMapNetDrvBtn : 1;
    BOOL32 fShowInfoTip : 1;
    BOOL32 fHideIcons : 1;
    UINT32 fRestFlags : 3;
} SHELLFLAGSTATE, * LPSHELLFLAGSTATE;


#undef PURE
#undef FAR
#undef THIS
#undef THIS_
#undef STDMETHOD
#undef STDMETHOD_
#endif /*_WINE_SHLOBJ_H*/
