#ifndef _WINE_SHLOBJ_H
#define _WINE_SHLOBJ_H

#include "shell.h"
#include "ole.h"
#include "ole2.h"
#include "compobj.h"

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,


typedef LPVOID	LPBC; /* *IBindCtx really */

/*
 * shell32 classids
 */
DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);

/*
 * shell32 Interface ids
 */
DEFINE_SHLGUID(IID_IContextMenu,        0x000214E4L, 0, 0);
DEFINE_SHLGUID(IID_IShellFolder,        0x000214E6L, 0, 0);
DEFINE_SHLGUID(IID_IShellExtInit,       0x000214E8L, 0, 0);
DEFINE_SHLGUID(IID_IShellPropSheetExt,  0x000214E9L, 0, 0);
DEFINE_SHLGUID(IID_IExtractIcon,        0x000214EBL, 0, 0);
DEFINE_SHLGUID(IID_IShellLink,          0x000214EEL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHook,      0x000214EFL, 0, 0);
DEFINE_SHLGUID(IID_IFileViewer,         0x000214F0L, 0, 0);
DEFINE_SHLGUID(IID_IEnumIDList,         0x000214F2L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerSite,     0x000214F3L, 0, 0);

#define	STRRET_WSTR	0x0000
#define	STRRET_OFFSET	0x0001
#define	STRRET_CSTR	0x0002

typedef struct _STRRET
{
    UINT32 uType;		/* STRRET_xxx */
    union
    {
        LPWSTR	pOleStr;	/* OLESTR that will be freed */
        UINT32	uOffset;	/* Offset into SHITEMID (ANSI) */
        char	cStr[MAX_PATH];	/* Buffer to fill in */
    } DUMMYUNIONNAME;
} STRRET,*LPSTRRET;

typedef struct {
	WORD		cb;	/* nr of bytes in this item */
	BYTE		abID[1];/* first byte in this item */
} SHITEMID,*LPSHITEMID;

typedef struct {
	SHITEMID	mkid; /* first itemid in list */
} ITEMIDLIST,*LPITEMIDLIST,*LPCITEMIDLIST;

/*****************************************************************************
 * IEnumIDList interface
 */
#define THIS LPENUMIDLIST this

typedef struct IEnumIDList IEnumIDList,*LPENUMIDLIST;
typedef struct IEnumIDList_VTable {
    /* *** IUnknown methods *** */
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
} IEnumIDList_VTable,*LPENUMIDLIST_VTABLE;

struct IEnumIDList {
	LPENUMIDLIST_VTABLE	lpvtbl;
	DWORD			ref;
};
#undef THIS
/************************************************************************
 * The IShellFolder interface ... the basic interface for a lot of stuff
 */

#define THIS LPSHELLFOLDER this

/* IShellFolder::GetDisplayNameOf/SetNameOf uFlags */
typedef enum
{
    SHGDN_NORMAL            = 0,        /* default (display purpose) */
    SHGDN_INFOLDER          = 1,        /* displayed under a folder (relative)*/
    SHGDN_FORPARSING        = 0x8000,   /* for ParseDisplayName or path */
} SHGNO;

/* IShellFolder::EnumObjects */
typedef enum tagSHCONTF
{
    SHCONTF_FOLDERS         = 32,       /* for shell browser */
    SHCONTF_NONFOLDERS      = 64,       /* for default view */
    SHCONTF_INCLUDEHIDDEN   = 128,      /* for hidden/system objects */
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


typedef struct tagSHELLFOLDER *LPSHELLFOLDER,IShellFolder;
typedef struct IShellFolder_VTable {
    /* *** IUnknown methods *** */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    /* *** IShellFolder methods *** */
    STDMETHOD(ParseDisplayName) (THIS_ HWND32 hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;
    STDMETHOD(EnumObjects) ( THIS_ HWND32 hwndOwner, DWORD grfFlags, LPENUMIDLIST
* ppenumIDList) PURE;
    STDMETHOD(BindToObject)     (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(BindToStorage)    (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD(CompareIDs)       (THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject) (THIS_ HWND32 hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(GetAttributesOf)  (THIS_ UINT32 cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)    (THIS_ HWND32 hwndOwner, UINT32 cidl, LPCITEMIDLIST
* apidl,
                                 REFIID riid, UINT32 * prgfInOut, LPVOID * ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf) (THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)        (THIS_ HWND32 hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut) PURE;
} *LPSHELLFOLDER_VTABLE,IShellFolder_VTable;

struct tagSHELLFOLDER {
	LPSHELLFOLDER_VTABLE	lpvtbl;
	DWORD			ref;
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
    SLR_UPDATE          = 0x0004,
} SLR_FLAGS;

/* IShellLink::GetPath fFlags */
typedef enum {
    SLGP_SHORTPATH      = 0x0001,
    SLGP_UNCPRIORITY    = 0x0002,
} SLGP_FLAGS;



typedef struct IShellLink IShellLink,*LPSHELLLINK;
typedef struct IShellLink_VTable
{
    // *** IUnknown methods ***
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
extern LPSHELLFOLDER IShellFolder_Constructor();
extern LPSHELLLINK IShellLink_Constructor();
extern LPENUMIDLIST IEnumIDList_Constructor();
#endif

DWORD WINAPI SHELL32_DllGetClassObject(LPCLSID,REFIID,LPVOID*);

#undef PURE
#undef FAR
#undef THIS
#undef THIS_
#undef STDMETHOD
#undef STDMETHOD_
#endif /*_WINE_SHLOBJ_H*/
