/*
 *	Shell Folder stuff
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"

/******************************************************************************
 * IEnumIDList implementation
 */

static ULONG WINAPI IEnumIDList_AddRef(LPENUMIDLIST this) {
	fprintf(stderr,"IEnumIDList(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IEnumIDList_Release(LPENUMIDLIST this) {
	fprintf(stderr,"IEnumIDList(%p)->Release()\n",this);
	if (!--(this->ref)) {
		fprintf(stderr,"	-> freeing IEnumIDList(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IEnumIDList_Next(
	LPENUMIDLIST this,ULONG celt,LPITEMIDLIST *rgelt,ULONG *pceltFetched
) {
	fprintf(stderr,"IEnumIDList(%p)->Next(%ld,%p,%p),stub!\n",
		this,celt,rgelt,pceltFetched
	);
	*pceltFetched = 0; /* we don't have any ... */
	return 0;
}

static IEnumIDList_VTable eidlvt = {
	(void *)1,
	IEnumIDList_AddRef,
	IEnumIDList_Release,
	IEnumIDList_Next,
	(void *)5,
        (void *)6,
        (void *)7
};

LPENUMIDLIST IEnumIDList_Constructor() {
	LPENUMIDLIST	lpeidl;

	lpeidl= (LPENUMIDLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumIDList));
	lpeidl->ref = 1;
	lpeidl->lpvtbl = &eidlvt;
	return lpeidl;
}

/******************************************************************************
 * IShellFolder implementation
 */
static ULONG WINAPI IShellFolder_Release(LPSHELLFOLDER this) {
	fprintf(stderr,"IShellFolder(%p)->Release()\n",this);
	if (!--(this->ref)) {
		fprintf(stderr,"	-> freeing IShellFolder(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static ULONG WINAPI IShellFolder_AddRef(LPSHELLFOLDER this) {
	fprintf(stderr,"IShellFolder(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static HRESULT WINAPI IShellFolder_GetAttributesOf(
	LPSHELLFOLDER this,UINT32 cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut
) {
	fprintf(stderr,"IShellFolder(%p)->GetAttributesOf(%d,%p,%p),stub!\n",
		this,cidl,apidl,rgfInOut
	);
	return 0;
}

static HRESULT WINAPI IShellFolder_BindToObject(
	LPSHELLFOLDER this,LPCITEMIDLIST pidl,LPBC pbcReserved,REFIID riid,LPVOID * ppvOut
) {
	char	xclsid[50];

	WINE_StringFromCLSID(riid,xclsid);
	fprintf(stderr,"IShellFolder(%p)->BindToObject(%p,%p,%s,%p),stub!\n",
		this,pidl,pbcReserved,xclsid,ppvOut
	);
	*ppvOut = IShellFolder_Constructor();
	return 0;
}

static HRESULT WINAPI IShellFolder_ParseDisplayName(
	LPSHELLFOLDER this,HWND32 hwndOwner,LPBC pbcReserved,
	LPOLESTR32 lpszDisplayName,DWORD *pchEaten,LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes
) {
	fprintf(stderr,"IShellFolder(%p)->ParseDisplayName(%08x,%p,%p,%p,%p,%p),stub!\n",
		this,hwndOwner,pbcReserved,lpszDisplayName,pchEaten,ppidl,pdwAttributes
	);
	*(DWORD*)pbcReserved = 0;
	return 0;
}

static HRESULT WINAPI IShellFolder_EnumObjects(
	LPSHELLFOLDER this,HWND32 hwndOwner,DWORD grfFlags,
	LPENUMIDLIST* ppenumIDList
) {
	fprintf(stderr,"IShellFolder(%p)->EnumObjects(0x%04x,0x%08lx,%p),stub!\n",
		this,hwndOwner,grfFlags,ppenumIDList
	);
	*ppenumIDList = IEnumIDList_Constructor();
	return 0;
}

static HRESULT WINAPI IShellFolder_CreateViewObject(
	LPSHELLFOLDER this,HWND32 hwndOwner,REFIID riid,LPVOID *ppv
) {
	char	xclsid[50];

	WINE_StringFromCLSID(riid,xclsid);
	fprintf(stderr,"IShellFolder(%p)->CreateViewObject(0x%04x,%s,%p),stub!\n",
		this,hwndOwner,xclsid,ppv
	);
	*(DWORD*)ppv = 0;
	return 0;
}


static struct IShellFolder_VTable sfvt = {
        (void *)1,
	IShellFolder_AddRef,
	IShellFolder_Release,
	IShellFolder_ParseDisplayName,
	IShellFolder_EnumObjects,
	IShellFolder_BindToObject,
        (void *)7,
        (void *)8,
	IShellFolder_CreateViewObject,
	IShellFolder_GetAttributesOf,
        (void *)11,
        (void *)12,
        (void *)13
};

LPSHELLFOLDER IShellFolder_Constructor() {
	LPSHELLFOLDER	sf;

	sf = (LPSHELLFOLDER)HeapAlloc(GetProcessHeap(),0,sizeof(IShellFolder));
	sf->ref		= 1;
	sf->lpvtbl	= &sfvt;
	return sf;
}

static struct IShellLink_VTable slvt = {
    (void *)1,
    (void *)2,
    (void *)3,
    (void *)4,
    (void *)5,
    (void *)6,
    (void *)7,
    (void *)8,
    (void *)9,
    (void *)10,
    (void *)11,
    (void *)12,
    (void *)13,
    (void *)14,
    (void *)15,
    (void *)16,
    (void *)17,
    (void *)18,
    (void *)19,
    (void *)20,
    (void *)21
};

LPSHELLLINK IShellLink_Constructor() {
	LPSHELLLINK sl;

	sl = (LPSHELLLINK)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLink));
	sl->ref = 1;
	sl->lpvtbl = &slvt;
	return sl;
}
