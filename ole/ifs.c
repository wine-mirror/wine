/*
 *	basic interfaces
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "stddebug.h"
#include "debug.h"

static ULONG WINAPI IUnknown_AddRef(LPUNKNOWN this) { 
	dprintf_relay(stddeb,"IUnknown(%p)->AddRef()\n",this);
	return ++(this->ref);
}
static ULONG WINAPI IUnknown_Release(LPUNKNOWN this) {
	dprintf_relay(stddeb,"IUnknown(%p)->Release()\n",this);
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IUnknown_QueryInterface(LPUNKNOWN this,REFIID refiid,LPVOID *obj) {
	char	xrefiid[50];

	StringFromCLSID((LPCLSID)refiid,xrefiid);
	dprintf_relay(stddeb,"IUnknown(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);

	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = this;
		return 0; 
	}
	return OLE_E_ENUM_NOMORE; 
}

static IUnknown_VTable uvt = {
	IUnknown_QueryInterface,
	IUnknown_AddRef,
	IUnknown_Release
};


LPUNKNOWN
IUnknown_Constructor() {
	LPUNKNOWN	unk;

	fprintf(stderr,"cloning IUnknown.\n");
	unk = (LPUNKNOWN)HeapAlloc(GetProcessHeap(),0,sizeof(IUnknown));
	unk->lpvtbl	= &uvt;
	unk->ref	= 1;
	return unk;
}

static ULONG WINAPI IMalloc_AddRef(LPMALLOC this) {
	dprintf_relay(stddeb,"IMalloc(%p)->AddRef()\n",this);
	return 1; /* cannot be freed */
}

static ULONG WINAPI IMalloc_Release(LPMALLOC this) {
	dprintf_relay(stddeb,"IMalloc(%p)->Release()\n",this);
	return 1; /* cannot be freed */
}

static HRESULT WINAPI IMalloc_QueryInterface(LPMALLOC this,REFIID refiid,LPVOID *obj) {
	char	xrefiid[50];

	StringFromCLSID((LPCLSID)refiid,xrefiid);
	dprintf_relay(stddeb,"IMalloc(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);
	if (	!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)) ||
		!memcmp(&IID_IMalloc,refiid,sizeof(IID_IMalloc))
	) {
		*obj = this;
		return 0;
	}
	return OLE_E_ENUM_NOMORE; 
}

static LPVOID WINAPI IMalloc_Alloc(LPMALLOC this,DWORD cb) {
	dprintf_relay(stddeb,"IMalloc(%p)->Alloc(%ld)\n",this,cb);
	return HeapAlloc(GetProcessHeap(),0,cb);
}

static LPVOID WINAPI IMalloc_Realloc(LPMALLOC this,LPVOID pv,DWORD cb) {
	dprintf_relay(stddeb,"IMalloc(%p)->Realloc(%p,%ld)\n",this,pv,cb);
	return HeapReAlloc(GetProcessHeap(),0,pv,cb);
}
static VOID WINAPI IMalloc_Free(LPMALLOC this,LPVOID pv) {
	dprintf_relay(stddeb,"IMalloc(%p)->Free(%p)\n",this,pv);
	HeapFree(GetProcessHeap(),0,pv);
}

static DWORD WINAPI IMalloc_GetSize(LPMALLOC this,LPVOID pv) {
	dprintf_relay(stddeb,"IMalloc(%p)->GetSize(%p)\n",this,pv);
	return HeapSize(GetProcessHeap(),0,pv);
}

static LPINT32 WINAPI IMalloc_DidAlloc(LPMALLOC this,LPVOID pv) {
	dprintf_relay(stddeb,"IMalloc(%p)->DidAlloc(%p)\n",this,pv);
	return (LPINT32)0xffffffff;
}
static LPVOID WINAPI IMalloc_HeapMinimize(LPMALLOC this) {
	dprintf_relay(stddeb,"IMalloc(%p)->HeapMinimize()\n",this);
	return NULL;
}

static IMalloc_VTable VT_IMalloc = {
	IMalloc_QueryInterface,
	IMalloc_AddRef,
	IMalloc_Release,
	IMalloc_Alloc,
	IMalloc_Realloc,
	IMalloc_Free,
	IMalloc_GetSize,
	IMalloc_DidAlloc,
	IMalloc_HeapMinimize,
};

LPMALLOC
IMalloc_Constructor() {
	LPMALLOC	this;

	fprintf(stderr,"cloning IMalloc\n");
	this = (LPMALLOC)HeapAlloc(GetProcessHeap(),0,sizeof(IMalloc));
	this->lpvtbl = &VT_IMalloc;
	this->ref = 1;
	return this;
}
