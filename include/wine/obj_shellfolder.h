/*
 * Defines the COM interfaces and APIs related to IShellFolder
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 1999 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_OBJ_SHELLFOLDER_H
#define __WINE_WINE_OBJ_SHELLFOLDER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
*  STRRET
*/
#define	STRRET_WSTR	0x0000
#define	STRRET_OFFSET	0x0001
#define	STRRET_CSTR	0x0002

typedef struct _STRRET {
  UINT uType;			/* STRRET_xxx */
  union {
    LPWSTR	pOleStr;	/* OLESTR that will be freed */
    UINT	uOffset;	/* Offset into SHITEMID (ANSI) */
    char	cStr[MAX_PATH];	/* ANSI Buffer */
  } DUMMYUNIONNAME;
} STRRET,*LPSTRRET;

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IShellFolder IShellFolder, *LPSHELLFOLDER;

typedef struct IPersistFolder IPersistFolder, *LPPERSISTFOLDER;

DEFINE_GUID(IID_IPersistFolder2, 0x1ac3d9f0L, 0x175C, 0x11D1, 0x95, 0xBE, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x4F);
typedef struct IPersistFolder2 IPersistFolder2, *LPPERSISTFOLDER2;

DEFINE_GUID(IID_IPersistFolder3, 0xcef04fdf, 0xfe72, 0x11d2, 0x87, 0xa5, 0x0, 0xc0, 0x4f, 0x68, 0x37, 0xcf);
typedef struct IPersistFolder3 IPersistFolder3, *LPPERSISTFOLDER3;

DEFINE_GUID(IID_IShellFolder2,  0xB82C5AA8, 0xA41B, 0x11D2, 0xBE, 0x32, 0x0, 0xc0, 0x4F, 0xB9, 0x36, 0x61);
typedef struct IShellFolder2 IShellFolder2, *LPSHELLFOLDER2;

DEFINE_GUID(IID_IEnumExtraSearch,  0xE700BE1, 0x9DB6, 0x11D1, 0xA1, 0xCE, 0x0, 0xc0, 0x4F, 0xD7, 0x5D, 0x13);
typedef struct IEnumExtraSearch IEnumExtraSearch, *LPENUMEXTRASEARCH;

/*****************************************************************************
 * IEnumExtraSearch interface
 */

typedef struct
{
  GUID	guidSearch;
  WCHAR wszFriendlyName[80];
  WCHAR	wszMenuText[80];
  WCHAR wszHelpText[MAX_PATH];
  WCHAR wszUrl[2084];
  WCHAR wszIcon[MAX_PATH+10];
  WCHAR wszGreyIcon[MAX_PATH+10];
  WCHAR wszClrIcon[MAX_PATH+10];
} EXTRASEARCH,* LPEXTRASEARCH;

#define INTERFACE IEnumExtraSearch
#define IEnumExtraSearch_METHODS \
    STDMETHOD(Next)(THIS_ ULONG  celt, LPEXTRASEARCH * rgelt, ULONG * pceltFetched) PURE; \
    STDMETHOD(Skip)(THIS_ ULONG  celt) PURE; \
    STDMETHOD(Reset)(THIS) PURE; \
    STDMETHOD(Clone)(THIS_ IEnumExtraSearch ** ppenum) PURE;
#define IEnumExtraSearch_IMETHODS \
    IUnknown_IMETHODS \
    IEnumExtraSearch_METHODS
ICOM_DEFINE(IEnumExtraSearch,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IEnumIDList_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumIDList_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IEnumIDList_Release(p)                  (p)->lpVtbl->Release(p)
/*** IEnumIDList methods ***/
#define IEnumIDList_Next(p,a,b,c)               (p)->lpVtbl->Next(p,a,b,c)
#define IEnumIDList_Skip(p,a)                   (p)->lpVtbl->Skip(p,a)
#define IEnumIDList_Reset(p)                    (p)->lpVtbl->Reset(p)
#define IEnumIDList_Clone(p,a)                  (p)->lpVtbl->Clone(p,a)
#endif

/*****************************************************************************
 * IShellFolder::GetDisplayNameOf/SetNameOf uFlags
 */
typedef enum
{	SHGDN_NORMAL		= 0,		/* default (display purpose) */
	SHGDN_INFOLDER		= 1,		/* displayed under a folder (relative)*/
	SHGDN_FORPARSING	= 0x8000	/* for ParseDisplayName or path */
} SHGNO;

/*****************************************************************************
 * IShellFolder::EnumObjects
 */
typedef enum tagSHCONTF
{	SHCONTF_FOLDERS		= 32,	/* for shell browser */
	SHCONTF_NONFOLDERS	= 64,	/* for default view */
	SHCONTF_INCLUDEHIDDEN	= 128	/* for hidden/system objects */
} SHCONTF;

/*****************************************************************************
 * IShellFolder::GetAttributesOf flags
 */
#define SFGAO_CANCOPY		DROPEFFECT_COPY /* Objects can be copied */
#define SFGAO_CANMOVE		DROPEFFECT_MOVE /* Objects can be moved */
#define SFGAO_CANLINK		DROPEFFECT_LINK /* Objects can be linked */
#define SFGAO_CANRENAME		0x00000010L	/* Objects can be renamed */
#define SFGAO_CANDELETE		0x00000020L	/* Objects can be deleted */
#define SFGAO_HASPROPSHEET	0x00000040L	/* Objects have property sheets */
#define SFGAO_DROPTARGET	0x00000100L	/* Objects are drop target */
#define SFGAO_CAPABILITYMASK	0x00000177L
#define SFGAO_LINK		0x00010000L	/* Shortcut (link) */
#define SFGAO_SHARE		0x00020000L	/* shared */
#define SFGAO_READONLY		0x00040000L	/* read-only */
#define SFGAO_GHOSTED		0x00080000L	/* ghosted icon */
#define SFGAO_HIDDEN            0x00080000L	/* hidden object */
#define SFGAO_DISPLAYATTRMASK	0x000F0000L
#define SFGAO_FILESYSANCESTOR	0x10000000L	/* It contains file system folder */
#define SFGAO_FOLDER		0x20000000L	/* It's a folder. */
#define SFGAO_FILESYSTEM	0x40000000L	/* is a file system thing (file/folder/root) */
#define SFGAO_HASSUBFOLDER	0x80000000L	/* Expandable in the map pane */
#define SFGAO_CONTENTSMASK	0x80000000L
#define SFGAO_VALIDATE		0x01000000L	/* invalidate cached information */
#define SFGAO_REMOVABLE		0x02000000L	/* is this removeable media? */
#define SFGAO_BROWSABLE		0x08000000L	/* is in-place browsable */
#define SFGAO_NONENUMERATED	0x00100000L	/* is a non-enumerated object */
#define SFGAO_NEWCONTENT	0x00200000L	/* should show bold in explorer tree */

/************************************************************************
 *
 * FOLDERSETTINGS
*/

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
 * Desktopfolder
 */

extern IShellFolder * pdesktopfolder;

DWORD WINAPI SHGetDesktopFolder(IShellFolder * *);

/*****************************************************************************
 * IShellFolder interface
 */
#define INTERFACE IShellFolder
#define IShellFolder_METHODS \
    STDMETHOD(ParseDisplayName)(THIS_ HWND  hwndOwner,LPBC  pbcReserved, LPOLESTR  lpszDisplayName, ULONG  * pchEaten, LPITEMIDLIST  * ppidl, ULONG  * pdwAttributes) PURE; \
    STDMETHOD(EnumObjects)(THIS_ HWND  hwndOwner, DWORD  grfFlags, LPENUMIDLIST  * ppenumIDList) PURE;\
    STDMETHOD(BindToObject)(THIS_ LPCITEMIDLIST  pidl, LPBC  pbcReserved, REFIID  riid, LPVOID  * ppvOut) PURE;\
    STDMETHOD(BindToStorage)(THIS_ LPCITEMIDLIST  pidl, LPBC  pbcReserved, REFIID  riid, LPVOID  * ppvObj) PURE;\
    STDMETHOD(CompareIDs)(THIS_ LPARAM  lParam, LPCITEMIDLIST  pidl1, LPCITEMIDLIST  pidl2) PURE;\
    STDMETHOD(CreateViewObject)(THIS_ HWND  hwndOwner, REFIID  riid, LPVOID  * ppvOut) PURE;\
    STDMETHOD(GetAttributesOf)(THIS_ UINT  cidl, LPCITEMIDLIST  * apidl, ULONG  * rgfInOut) PURE;\
    STDMETHOD(GetUIObjectOf)(THIS_ HWND  hwndOwner, UINT  cidl, LPCITEMIDLIST  * apidl, REFIID  riid, UINT  * prgfInOut, LPVOID  * ppvOut) PURE;\
    STDMETHOD(GetDisplayNameOf)(THIS_ LPCITEMIDLIST  pidl, DWORD  uFlags, LPSTRRET  lpName) PURE;\
    STDMETHOD(SetNameOf)(THIS_ HWND  hwndOwner, LPCITEMIDLIST  pidl,LPCOLESTR  lpszName, DWORD  uFlags,LPITEMIDLIST  * ppidlOut) PURE;
#define IShellFolder_IMETHODS \
    IUnknown_IMETHODS \
    IShellFolder_METHODS
ICOM_DEFINE(IShellFolder,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellFolder_QueryInterface(p,a,b)              (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellFolder_AddRef(p)                          (p)->lpVtbl->AddRef(p)
#define IShellFolder_Release(p)                         (p)->lpVtbl->Release(p)
/*** IShellFolder methods ***/
#define IShellFolder_ParseDisplayName(p,a,b,c,d,e,f)    (p)->lpVtbl->ParseDisplayName(p,a,b,c,d,e,f)
#define IShellFolder_EnumObjects(p,a,b,c)               (p)->lpVtbl->EnumObjects(p,a,b,c)
#define IShellFolder_BindToObject(p,a,b,c,d)            (p)->lpVtbl->BindToObject(p,a,b,c,d)
#define IShellFolder_BindToStorage(p,a,b,c,d)           (p)->lpVtbl->BindToStorage(p,a,b,c,d)
#define IShellFolder_CompareIDs(p,a,b,c)                (p)->lpVtbl->CompareIDs(p,a,b,c)
#define IShellFolder_CreateViewObject(p,a,b,c)          (p)->lpVtbl->CreateViewObject(p,a,b,c)
#define IShellFolder_GetAttributesOf(p,a,b,c)           (p)->lpVtbl->GetAttributesOf(p,a,b,c)
#define IShellFolder_GetUIObjectOf(p,a,b,c,d,e,f)       (p)->lpVtbl->GetUIObjectOf(p,a,b,c,d,e,f)
#define IShellFolder_GetDisplayNameOf(p,a,b,c)          (p)->lpVtbl->GetDisplayNameOf(p,a,b,c)
#define IShellFolder_SetNameOf(p,a,b,c,d,e)             (p)->lpVtbl->SetNameOf(p,a,b,c,d,e)
#endif

/*****************************************************************************
 * IShellFolder2 interface
 */
/* IShellFolder2 */

/* GetDefaultColumnState */
typedef enum
{
	SHCOLSTATE_TYPE_STR	= 0x00000001,
	SHCOLSTATE_TYPE_INT	= 0x00000002,
	SHCOLSTATE_TYPE_DATE	= 0x00000003,
	SHCOLSTATE_TYPEMASK	= 0x0000000F,
	SHCOLSTATE_ONBYDEFAULT	= 0x00000010,
	SHCOLSTATE_SLOW		= 0x00000020,
	SHCOLSTATE_EXTENDED	= 0x00000040,
	SHCOLSTATE_SECONDARYUI	= 0x00000080,
	SHCOLSTATE_HIDDEN	= 0x00000100
} SHCOLSTATE;

typedef struct
{
	GUID	fmtid;
	DWORD	pid;
} SHCOLUMNID, *LPSHCOLUMNID;
typedef const SHCOLUMNID* LPCSHCOLUMNID;

/* GetDetailsEx */
#define PID_FINDDATA		0
#define PID_NETRESOURCE		1
#define PID_DESCRIPTIONID	2

typedef struct
{
	int	fmt;
	int	cxChar;
	STRRET	str;
} SHELLDETAILS, *LPSHELLDETAILS;

#define INTERFACE IShellFolder2
#define IShellFolder2_METHODS \
    STDMETHOD(GetDefaultSearchGUID)(THIS_ LPGUID  lpguid) PURE;\
    STDMETHOD(EnumSearches)(THIS_ LPENUMEXTRASEARCH  * ppEnum) PURE; \
    STDMETHOD(GetDefaultColumn)(THIS_ DWORD  dwReserved, ULONG  * pSort, ULONG  * pDisplay) PURE;\
    STDMETHOD(GetDefaultColumnState)(THIS_ UINT  iColumn, DWORD  * pcsFlags) PURE;\
    STDMETHOD(GetDetailsEx)(THIS_ LPCITEMIDLIST  pidl, const SHCOLUMNID  * pscid, VARIANT  * pv) PURE;\
    STDMETHOD(GetDetailsOf)(THIS_ LPCITEMIDLIST  pidl, UINT  iColumn, LPSHELLDETAILS  pDetails) PURE;\
    STDMETHOD(MapNameToSCID)(THIS_ LPCWSTR  pwszName, SHCOLUMNID  * pscid) PURE;
#define IShellFolder2_IMETHODS \
    IShellFolder_METHODS \
    IShellFolder2_METHODS
ICOM_DEFINE(IShellFolder2, IShellFolder)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellFolder2_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellFolder2_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define IShellFolder2_Release(p)                        (p)->lpVtbl->Release(p)
/*** IShellFolder methods ***/
#define IShellFolder2_ParseDisplayName(p,a,b,c,d,e,f)   (p)->lpVtbl->ParseDisplayName(p,a,b,c,d,e,f)
#define IShellFolder2_EnumObjects(p,a,b,c)              (p)->lpVtbl->EnumObjects(p,a,b,c)
#define IShellFolder2_BindToObject(p,a,b,c,d)           (p)->lpVtbl->BindToObject(p,a,b,c,d)
#define IShellFolder2_BindToStorage(p,a,b,c,d)          (p)->lpVtbl->BindToStorage(p,a,b,c,d)
#define IShellFolder2_CompareIDs(p,a,b,c)               (p)->lpVtbl->CompareIDs(p,a,b,c)
#define IShellFolder2_CreateViewObject(p,a,b,c)         (p)->lpVtbl->CreateViewObject(p,a,b,c)
#define IShellFolder2_GetAttributesOf(p,a,b,c)          (p)->lpVtbl->GetAttributesOf(p,a,b,c)
#define IShellFolder2_GetUIObjectOf(p,a,b,c,d,e,f)      (p)->lpVtbl->GetUIObjectOf(p,a,b,c,d,e,f)
#define IShellFolder2_GetDisplayNameOf(p,a,b,c)         (p)->lpVtbl->GetDisplayNameOf(p,a,b,c)
#define IShellFolder2_SetNameOf(p,a,b,c,d,e)            (p)->lpVtbl->SetNameOf(p,a,b,c,d,e)
/*** IShellFolder2 methods ***/
#define IShellFolder2_GetDefaultSearchGUID(p,a)         (p)->lpVtbl->GetDefaultSearchGUID(p,a)
#define IShellFolder2_EnumSearches(p,a)                 (p)->lpVtbl->EnumSearches(p,a)
#define IShellFolder2_GetDefaultColumn(p,a,b,c)         (p)->lpVtbl->GetDefaultColumn(p,a,b,c)
#define IShellFolder2_GetDefaultColumnState(p,a,b)      (p)->lpVtbl->GetDefaultColumnState(p,a,b)
#define IShellFolder2_GetDetailsEx(p,a,b,c)             (p)->lpVtbl->GetDetailsEx(p,a,b,c)
#define IShellFolder2_GetDetailsOf(p,a,b,c)             (p)->lpVtbl->GetDetailsOf(p,a,b,c)
#define IShellFolder2_MapNameToSCID(p,a,b)              (p)->lpVtbl->MapNameToSCID(p,a,b)
#endif

/*****************************************************************************
 * IPersistFolder interface
 */

#define INTERFACE IPersistFolder
#define IPersistFolder_METHODS \
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST  pidl) PURE;
#define IPersistFolder_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS
ICOM_DEFINE(IPersistFolder, IPersist)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPersistFolder_QueryInterface(p,a,b)    (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistFolder_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IPersistFolder_Release(p)               (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IPersistFolder_GetClassID(p,a)          (p)->lpVtbl->GetClassID(p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder_Initialize(p,a)          (p)->lpVtbl->Initialize(p,a)
#endif

/*****************************************************************************
 * IPersistFolder2 interface
 */

#define INTERFACE IPersistFolder2
#define IPersistFolder2_METHODS \
    STDMETHOD(GetCurFolder)(THIS_ LPITEMIDLIST * pidl) PURE;
#define IPersistFolder2_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS \
    IPersistFolder2_METHODS
ICOM_DEFINE(IPersistFolder2, IPersistFolder)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPersistFolder2_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistFolder2_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IPersistFolder2_Release(p)              (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IPersistFolder2_GetClassID(p,a)         (p)->lpVtbl->GetClassID(p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder2_Initialize(p,a)         (p)->lpVtbl->Initialize(p,a)
/*** IPersistFolder2 methods ***/
#define IPersistFolder2_GetCurFolder(p,a)       (p)->lpVtbl->GetCurFolder(p,a)
#endif


/*****************************************************************************
 * IPersistFolder3 interface
 */

typedef struct {
	LPITEMIDLIST	pidlTargetFolder;
	WCHAR		szTargetParsingName[MAX_PATH];
	WCHAR		szNetworkProvider[MAX_PATH];
	DWORD		dwAttributes;
	int		csidl;
} PERSIST_FOLDER_TARGET_INFO;

#define INTERFACE IPersistFolder3
#define IPersistFolder3_METHODS \
    STDMETHOD(InitializeEx)(THIS_ IBindCtx * pbc, LPCITEMIDLIST  pidlRoot, const PERSIST_FOLDER_TARGET_INFO * ppfti) PURE;\
    STDMETHOD(GetFolderTargetInfo)(THIS_ PERSIST_FOLDER_TARGET_INFO * ppfti) PURE;
#define IPersistFolder3_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS \
    IPersistFolder2_METHODS \
    IPersistFolder3_METHODS
ICOM_DEFINE(IPersistFolder3, IPersistFolder2)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPersistFolder3_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IPersistFolder3_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IPersistFolder3_Release(p)              (p)->lpVtbl->Release(p)
/*** IPersist methods ***/
#define IPersistFolder3_GetClassID(p,a)         (p)->lpVtbl->GetClassID(p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder3_Initialize(p,a)         (p)->lpVtbl->Initialize(p,a)
/*** IPersistFolder2 methods ***/
#define IPersistFolder3_GetCurFolder(p,a)       (p)->lpVtbl->GetCurFolder(p,a)
/*** IPersistFolder3 methods ***/
#define IPersistFolder3_InitializeEx(p,a,b,c)   (p)->lpVtbl->InitializeEx(p,a,b,c)
#define IPersistFolder3_GetFolderTargetInfo(p,a) (p)->lpVtbl->InitializeEx(p,a)
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLFOLDER_H */
