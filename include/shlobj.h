#ifndef _WINE_SHLOBJ_H
#define _WINE_SHLOBJ_H

#include "ole.h"
#include "ole2.h"
#include "compobj.h"

typedef LPVOID	LPBC; /* *IBindCtx really */
typedef LPVOID	LPSTRRET,LPENUMIDLIST;

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

typedef struct {
	WORD		cb;	/* nr of bytes in this item */
	BYTE		abID[1];/* first byte in this item */
} SHITEMID,*LPSHITEMID;

typedef struct {
	SHITEMID	mkid; /* first itemid in list */
} ITEMIDLIST,*LPITEMIDLIST,*LPCITEMIDLIST;

/* The IShellFolder interface ... the basic interface for a lot of stuff */

typedef struct tagSHELLFOLDER *LPSHELLFOLDER,IShellFolder;
typedef struct {
	HRESULT	(CALLBACK *fnQueryInterface)(LPSHELLFOLDER this,REFIID refiid,LPVOID *obj);
	HRESULT	(CALLBACK *fnAddRef)(LPSHELLFOLDER this);
	HRESULT	(CALLBACK *fnRelease)(LPSHELLFOLDER this);
	/* IShellFolder methods */

	HRESULT (CALLBACK *fnParseDisplayName) (LPSHELLFOLDER this,HWND32 hwndOwner,LPBC pbcReserved,LPOLESTR lpszDisplayName,DWORD * pchEaten,LPITEMIDLIST * ppidl, DWORD *pdwAttributes) ;

	HRESULT (CALLBACK *fnEnumObjects)( LPSHELLFOLDER this,HWND32 hwndOwner, DWORD grfFlags, LPENUMIDLIST* ppenumIDList);
	HRESULT (CALLBACK *fnBindToObject)(LPSHELLFOLDER this, LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut);
	HRESULT (CALLBACK *fnBindToStorage)(LPSHELLFOLDER this, LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvObj);
    	HRESULT (CALLBACK *fnCompareIDs) (LPSHELLFOLDER this, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
	HRESULT (CALLBACK *fnCreateViewObject) (LPSHELLFOLDER this, HWND32 hwndOwner, REFIID riid, LPVOID * ppvOut);
	HRESULT (CALLBACK *fnGetAttributesOf)  (LPSHELLFOLDER this, UINT32 cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut);
	HRESULT (CALLBACK *fnGetUIObjectOf)    (LPSHELLFOLDER this, HWND32 hwndOwner, UINT32 cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT32 * prgfInOut, LPVOID * ppvOut);
	HRESULT (CALLBACK *fnGetDisplayNameOf) (LPSHELLFOLDER this, LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
	HRESULT (CALLBACK *fnSetNameOf)        (LPSHELLFOLDER this, HWND32 hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);
} *LPSHELLFOLDER_VTABLE;

struct tagSHELLFOLDER {
	LPSHELLFOLDER_VTABLE	*lpvtbl;
};
#endif /*_WINE_SHLOBJ_H*/
