/*
 * Defines the COM interfaces and APIs related to IShellFolder
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_SHELLFOLDER_H
#define __WINE_WINE_OBJ_SHELLFOLDER_H

#include "wine/obj_base.h"
#include "wine/obj_moniker.h"		/* for LPBC */
#include "wine/obj_enumidlist.h"
#include "winbase.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
*  STRRET (temporary, move it away)
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
 * Predeclare the interfaces
 */
DEFINE_SHLGUID(IID_IShellFolder,        0x000214E6L, 0, 0);
typedef struct IShellFolder IShellFolder, *LPSHELLFOLDER;

DEFINE_SHLGUID(IID_IPersistFolder,      0x000214EAL, 0, 0);
typedef struct IPersistFolder IPersistFolder, *LPPERSISTFOLDER;


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
#define ICOM_INTERFACE IShellFolder
#define IShellFolder_METHODS \
    ICOM_METHOD6( HRESULT, ParseDisplayName, HWND, hwndOwner,LPBC, pbcReserved, LPOLESTR, lpszDisplayName, ULONG *, pchEaten, LPITEMIDLIST *, ppidl, ULONG *, pdwAttributes) \
    ICOM_METHOD3( HRESULT, EnumObjects, HWND, hwndOwner, DWORD, grfFlags, LPENUMIDLIST *, ppenumIDList)\
    ICOM_METHOD4( HRESULT, BindToObject, LPCITEMIDLIST, pidl, LPBC, pbcReserved, REFIID, riid, LPVOID *, ppvOut)\
    ICOM_METHOD4( HRESULT, BindToStorage, LPCITEMIDLIST, pidl, LPBC, pbcReserved, REFIID, riid, LPVOID *, ppvObj)\
    ICOM_METHOD3( HRESULT, CompareIDs, LPARAM, lParam, LPCITEMIDLIST, pidl1, LPCITEMIDLIST, pidl2)\
    ICOM_METHOD3( HRESULT, CreateViewObject, HWND, hwndOwner, REFIID, riid, LPVOID *, ppvOut)\
    ICOM_METHOD3( HRESULT, GetAttributesOf, UINT, cidl, LPCITEMIDLIST *, apidl, ULONG *, rgfInOut)\
    ICOM_METHOD6( HRESULT, GetUIObjectOf, HWND, hwndOwner, UINT, cidl, LPCITEMIDLIST *, apidl, REFIID, riid, UINT *, prgfInOut, LPVOID *, ppvOut)\
    ICOM_METHOD3( HRESULT, GetDisplayNameOf, LPCITEMIDLIST, pidl, DWORD, uFlags, LPSTRRET, lpName)\
    ICOM_METHOD5( HRESULT, SetNameOf, HWND, hwndOwner, LPCITEMIDLIST, pidl,LPCOLESTR, lpszName, DWORD, uFlags,LPITEMIDLIST *, ppidlOut)\
    ICOM_METHOD2( HRESULT, GetFolderPath, LPSTR, lpszOut, DWORD, dwOutSize)
#define IShellFolder_IMETHODS \
    IUnknown_IMETHODS \
    IShellFolder_METHODS
ICOM_DEFINE(IShellFolder,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IShellFolder_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IShellFolder_AddRef(p)				ICOM_CALL (AddRef,p)
#define IShellFolder_Release(p)				ICOM_CALL (Release,p)
/*** IShellFolder methods ***/
#define IShellFolder_ParseDisplayName(p,a,b,c,d,e,f)	ICOM_CALL6(ParseDisplayName,p,a,b,c,d,e,f)
#define IShellFolder_EnumObjects(p,a,b,c)		ICOM_CALL3(EnumObjects,p,a,b,c)
#define IShellFolder_BindToObject(p,a,b,c,d)		ICOM_CALL4(BindToObject,p,a,b,c,d)
#define IShellFolder_BindToStorage(p,a,b,c,d)		ICOM_CALL4(BindToStorage,p,a,b,c,d)
#define IShellFolder_CompareIDs(p,a,b,c)		ICOM_CALL3(CompareIDs,p,a,b,c)
#define IShellFolder_CreateViewObject(p,a,b,c)		ICOM_CALL3(CreateViewObject,p,a,b,c)
#define IShellFolder_GetAttributesOf(p,a,b,c)		ICOM_CALL3(GetAttributesOf,p,a,b,c)
#define IShellFolder_GetUIObjectOf(p,a,b,c,d,e,f)	ICOM_CALL6(GetUIObjectOf,p,a,b,c,d,e,f)
#define IShellFolder_GetDisplayNameOf(p,a,b,c)		ICOM_CALL3(GetDisplayNameOf,p,a,b,c)
#define IShellFolder_SetNameOf(p,a,b,c,d,e)		ICOM_CALL5(SetNameOf,p,a,b,c,d,e)
#define IShellFolder_GetFolderPath(p,a,b)		ICOM_CALL2(GetFolderPath,p,a,b)

/*****************************************************************************
 * IPersistFolder interface
 */

DEFINE_GUID (CLSID_SFMyComp,0x20D04FE0,0x3AEA,0x1069,0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D); 
DEFINE_GUID (CLSID_SFINet,  0x871C5380,0x42A0,0x1069,0xA2,0xEA,0x08,0x00,0x2B,0x30,0x30,0x9D);
DEFINE_GUID (CLSID_SFFile,  0xF3364BA0,0x65B9,0x11CE,0xA9,0xBA,0x00,0xAA,0x00,0x4A,0xE8,0x37);

#define ICOM_INTERFACE IPersistFolder
#define IPersistFolder_METHODS \
    ICOM_METHOD1( HRESULT, Initialize, LPCITEMIDLIST, pidl)
#define IPersistFolder_IMETHODS \
    IPersist_IMETHODS \
    IPersistFolder_METHODS
ICOM_DEFINE(IPersistFolder, IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistFolder_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b) 
#define IPersistFolder_AddRef(p)		ICOM_CALL (AddRef,p)
#define IPersistFolder_Release(p)		ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistFolder_GetClassID(p,a)		ICOM_CALL1(GetClassID,p,a)
/*** IPersistFolder methods ***/
#define IPersistFolder_Initialize(p,a)		ICOM_CALL1(Initialize,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLLINK_H */
