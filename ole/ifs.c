/*
 *	basic interfaces
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "ldt.h"
#include "heap.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "local.h"
#include "module.h"
#include "debug.h"

/*
 * IUnknown
 */

/******************************************************************************
 *		IUnknown_AddRef	[???]
 */
static ULONG WINAPI IUnknown_AddRef(LPUNKNOWN this) { 
	TRACE(relay,"(%p)->AddRef()\n",this);
	return ++(this->ref);
}
static ULONG WINAPI IUnknown_Release(LPUNKNOWN this) {
	TRACE(relay,"(%p)->Release()\n",this);
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IUnknown_QueryInterface(LPUNKNOWN this,REFIID refiid,LPVOID *obj) {
	char	xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);

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

	unk = (LPUNKNOWN)HeapAlloc(GetProcessHeap(),0,sizeof(IUnknown));
	unk->lpvtbl	= &uvt;
	unk->ref	= 1;
	return unk;
}

/*
 * IMalloc
 */

/******************************************************************************
 *		IMalloc16_AddRef	[COMPOBJ.501]
 */
ULONG WINAPI IMalloc16_AddRef(LPMALLOC16 this) {
	TRACE(relay,"(%p)->AddRef()\n",this);
	return 1; /* cannot be freed */
}

/******************************************************************************
 *		IMalloc16_Release	[COMPOBJ.502]
 */
ULONG WINAPI IMalloc16_Release(LPMALLOC16 this) {
	TRACE(relay,"(%p)->Release()\n",this);
	return 1; /* cannot be freed */
}

/******************************************************************************
 *		IMalloc16_QueryInterface	[COMPOBJ.500]
 */
HRESULT WINAPI IMalloc16_QueryInterface(LPMALLOC16 this,REFIID refiid,LPVOID *obj) {
	char	xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);
	if (	!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)) ||
		!memcmp(&IID_IMalloc,refiid,sizeof(IID_IMalloc))
	) {
		*obj = this;
		return 0;
	}
	return OLE_E_ENUM_NOMORE; 
}

LPVOID WINAPI IMalloc16_Alloc(LPMALLOC16 this,DWORD cb) {
	TRACE(relay,"(%p)->Alloc(%ld)\n",this,cb);
	return (LPVOID)PTR_SEG_OFF_TO_SEGPTR(this->heap,LOCAL_Alloc(this->heap,0,cb));
}

LPVOID WINAPI IMalloc16_Realloc(LPMALLOC16 this,LPVOID pv,DWORD cb) {
	TRACE(relay,"(%p)->Realloc(%p,%ld)\n",this,pv,cb);
	return (LPVOID)PTR_SEG_OFF_TO_SEGPTR(this->heap,LOCAL_ReAlloc(this->heap,0,LOWORD(pv),cb));
}
VOID WINAPI IMalloc16_Free(LPMALLOC16 this,LPVOID pv) {
	TRACE(relay,"(%p)->Free(%p)\n",this,pv);
	LOCAL_Free(this->heap,LOWORD(pv));
}

DWORD WINAPI IMalloc16_GetSize(LPMALLOC16 this,LPVOID pv) {
	TRACE(relay,"(%p)->GetSize(%p)\n",this,pv);
	return LOCAL_Size(this->heap,LOWORD(pv));
}

INT16 WINAPI IMalloc16_DidAlloc(LPMALLOC16 this,LPVOID pv) {
	TRACE(relay,"(%p)->DidAlloc(%p)\n",this,pv);
	return (INT16)-1;
}
LPVOID WINAPI IMalloc16_HeapMinimize(LPMALLOC16 this) {
	TRACE(relay,"(%p)->HeapMinimize()\n",this);
	return NULL;
}

#ifdef OLD_TABLE
/* FIXME: This is unused */
static IMalloc16_VTable mvt16 = {
	IMalloc16_QueryInterface,
	IMalloc16_AddRef,
	IMalloc16_Release,
	IMalloc16_Alloc,
	IMalloc16_Realloc,
	IMalloc16_Free,
	IMalloc16_GetSize,
	IMalloc16_DidAlloc,
	IMalloc16_HeapMinimize,
};
#endif
static IMalloc16_VTable *msegvt16 = NULL;

LPMALLOC16
IMalloc16_Constructor() {
	LPMALLOC16	this;
        HMODULE16	hcomp = GetModuleHandle16("COMPOBJ");

	this = (LPMALLOC16)SEGPTR_NEW(IMalloc16);
        if (!msegvt16) {
            this->lpvtbl = msegvt16 = SEGPTR_NEW(IMalloc16_VTable);

#define FN(x) this->lpvtbl->fn##x = (void*)WIN32_GetProcAddress16(hcomp,"IMalloc16_"#x);assert(this->lpvtbl->fn##x)
            FN(QueryInterface);
            FN(AddRef);
            FN(Release);
            FN(Alloc);
            FN(Realloc);
            FN(Free);
            FN(GetSize);
            FN(DidAlloc);
            FN(HeapMinimize);
            msegvt16 = (LPMALLOC16_VTABLE)SEGPTR_GET(msegvt16);
#undef FN
            this->lpvtbl = msegvt16;
	}
	this->ref = 1;
	/* FIXME: implement multiple heaps */
	this->heap = GlobalAlloc16(GMEM_MOVEABLE,64000);
	LocalInit(this->heap,0,64000);
	return (LPMALLOC16)SEGPTR_GET(this);
}

/*
 * IMalloc32
 */

/******************************************************************************
 *		IMalloc32_AddRef	[???]
 */
static ULONG WINAPI IMalloc32_AddRef(LPMALLOC32 this) {
	TRACE(relay,"(%p)->AddRef()\n",this);
	return 1; /* cannot be freed */
}

/******************************************************************************
 *		IMalloc32_Release	[???]
 */
static ULONG WINAPI IMalloc32_Release(LPMALLOC32 this) {
	TRACE(relay,"(%p)->Release()\n",this);
	return 1; /* cannot be freed */
}

/******************************************************************************
 *		IMalloc32_QueryInterface	[???]
 */
static HRESULT WINAPI IMalloc32_QueryInterface(LPMALLOC32 this,REFIID refiid,LPVOID *obj) {
	char	xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(relay,"(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);
	if (	!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)) ||
		!memcmp(&IID_IMalloc,refiid,sizeof(IID_IMalloc))
	) {
		*obj = this;
		return 0;
	}
	return OLE_E_ENUM_NOMORE; 
}

static LPVOID WINAPI IMalloc32_Alloc(LPMALLOC32 this,DWORD cb) {
	TRACE(relay,"(%p)->Alloc(%ld)\n",this,cb);
	return HeapAlloc(GetProcessHeap(),0,cb);
}

static LPVOID WINAPI IMalloc32_Realloc(LPMALLOC32 this,LPVOID pv,DWORD cb) {
	TRACE(relay,"(%p)->Realloc(%p,%ld)\n",this,pv,cb);
	return HeapReAlloc(GetProcessHeap(),0,pv,cb);
}
static VOID WINAPI IMalloc32_Free(LPMALLOC32 this,LPVOID pv) {
	TRACE(relay,"(%p)->Free(%p)\n",this,pv);
	HeapFree(GetProcessHeap(),0,pv);
}

static DWORD WINAPI IMalloc32_GetSize(LPMALLOC32 this,LPVOID pv) {
	TRACE(relay,"(%p)->GetSize(%p)\n",this,pv);
	return HeapSize(GetProcessHeap(),0,pv);
}

static INT32 WINAPI IMalloc32_DidAlloc(LPMALLOC32 this,LPVOID pv) {
	TRACE(relay,"(%p)->DidAlloc(%p)\n",this,pv);
	return -1;
}
static LPVOID WINAPI IMalloc32_HeapMinimize(LPMALLOC32 this) {
	TRACE(relay,"(%p)->HeapMinimize()\n",this);
	return NULL;
}

static IMalloc32_VTable VT_IMalloc32 = {
	IMalloc32_QueryInterface,
	IMalloc32_AddRef,
	IMalloc32_Release,
	IMalloc32_Alloc,
	IMalloc32_Realloc,
	IMalloc32_Free,
	IMalloc32_GetSize,
	IMalloc32_DidAlloc,
	IMalloc32_HeapMinimize,
};

LPMALLOC32
IMalloc32_Constructor() {
	LPMALLOC32	this;

	this = (LPMALLOC32)HeapAlloc(GetProcessHeap(),0,sizeof(IMalloc32));
	this->lpvtbl = &VT_IMalloc32;
	this->ref = 1;
	return this;
}

/****************************************************************************
 * API Functions
 */

/******************************************************************************
 *		IsValidInterface32	[OLE32.78]
 *
 * RETURNS
 *  True, if the passed pointer is a valid interface
 */
BOOL32 WINAPI IsValidInterface32(
	LPUNKNOWN punk	/* [in] interface to be tested */
) {
	return !(
		IsBadReadPtr32(punk,4)					||
		IsBadReadPtr32(punk->lpvtbl,4)				||
		IsBadReadPtr32(punk->lpvtbl->fnQueryInterface,9)	||
		IsBadCodePtr32(punk->lpvtbl->fnQueryInterface)
	);
}
