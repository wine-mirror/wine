/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1999 Bertho A. Stultiens
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "commdlg.h"
#include "cderr.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"


HINSTANCE COMDLG32_hInstance = 0;
HANDLE	COMDLG32_hActCtx = INVALID_HANDLE_VALUE;

static DWORD COMDLG32_TlsIndex = TLS_OUT_OF_INDEXES;

/***********************************************************************
 *	DllMain  (COMDLG32.init)
 *
 *    Initialization code for the COMDLG32 DLL
 *
 * RETURNS:
 *	FALSE if sibling could not be loaded or instantiated twice, TRUE
 *	otherwise.
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	TRACE("(%p, %ld, %p)\n", hInstance, Reason, Reserved);

	switch(Reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		ACTCTXW actctx = {0};

		COMDLG32_hInstance = hInstance;
		DisableThreadLibraryCalls(hInstance);

		actctx.cbSize = sizeof(actctx);
		actctx.hModule = COMDLG32_hInstance;
		actctx.lpResourceName = MAKEINTRESOURCEW(123);
		actctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
		COMDLG32_hActCtx = CreateActCtxW(&actctx);
		if (COMDLG32_hActCtx == INVALID_HANDLE_VALUE)
			ERR("failed to create activation context, last error %lu\n", GetLastError());

		break;
	}
	case DLL_PROCESS_DETACH:
            if (Reserved) break;
            if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES) TlsFree(COMDLG32_TlsIndex);
            if (COMDLG32_hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(COMDLG32_hActCtx);
            break;
	}
	return TRUE;
}
#undef GPA

/***********************************************************************
 *	COMDLG32_AllocMem 			(internal)
 * Get memory for internal datastructure plus stringspace etc.
 *	RETURNS
 *		Success: Pointer to a heap block
 *		Failure: null
 */
void *COMDLG32_AllocMem(int size)
{
    void *ptr = heap_alloc_zero(size);

    if (!ptr)
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
        return NULL;
    }

    return ptr;
}


/***********************************************************************
 *	COMDLG32_SetCommDlgExtendedError	(internal)
 *
 * Used to set the thread's local error value if a comdlg32 function fails.
 */
void COMDLG32_SetCommDlgExtendedError(DWORD err)
{
	TRACE("(%08lx)\n", err);
        if (COMDLG32_TlsIndex == TLS_OUT_OF_INDEXES)
	  COMDLG32_TlsIndex = TlsAlloc();
	if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES)
	  TlsSetValue(COMDLG32_TlsIndex, (LPVOID)(DWORD_PTR)err);
	else
	  FIXME("No Tls Space\n");
}


/***********************************************************************
 *	CommDlgExtendedError			(COMDLG32.@)
 *
 * Get the thread's local error value if a comdlg32 function fails.
 *	RETURNS
 *		Current error value which might not be valid
 *		if a previous call succeeded.
 */
DWORD WINAPI CommDlgExtendedError(void)
{
        if (COMDLG32_TlsIndex != TLS_OUT_OF_INDEXES)
	  return (DWORD_PTR)TlsGetValue(COMDLG32_TlsIndex);
	else
	  return 0; /* we never set an error, so there isn't one */
}

/*************************************************************************
 * Implement the CommDlg32 class factory
 *
 * (Taken from shdocvw/factory.c; based on implementation in
 *  ddraw/main.c)
 */
typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*cf)(IUnknown*, REFIID, void**);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

/*************************************************************************
 * CDLGCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI CDLGCF_QueryInterface(IClassFactory* iface,
                                            REFIID riid, void **ppobj)
{
    TRACE("%p (%s %p)\n", iface, debugstr_guid(riid), ppobj);

    if(!ppobj)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid))
    {
        *ppobj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Interface not supported.\n");

    *ppobj = NULL;
    return E_NOINTERFACE;
}

/*************************************************************************
 * CDLGCF_AddRef (IUnknown)
 */
static ULONG WINAPI CDLGCF_AddRef(IClassFactory *iface)
{
    return 2; /* non-heap based object */
}

/*************************************************************************
 * CDLGCF_Release (IUnknown)
 */
static ULONG WINAPI CDLGCF_Release(IClassFactory *iface)
{
    return 1; /* non-heap based object */
}

/*************************************************************************
 * CDLGCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI CDLGCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                            REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return This->cf(pOuter, riid, ppobj);
}

/*************************************************************************
 * CDLGCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI CDLGCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("%p (%d)\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl CDLGCF_Vtbl =
{
    CDLGCF_QueryInterface,
    CDLGCF_AddRef,
    CDLGCF_Release,
    CDLGCF_CreateInstance,
    CDLGCF_LockServer
};

/*************************************************************************
 *              DllGetClassObject (COMMDLG32.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    static IClassFactoryImpl FileOpenDlgClassFactory = {{&CDLGCF_Vtbl}, FileOpenDialog_Constructor};
    static IClassFactoryImpl FileSaveDlgClassFactory = {{&CDLGCF_Vtbl}, FileSaveDialog_Constructor};

    TRACE("%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualGUID(&CLSID_FileOpenDialog, rclsid))
        return IClassFactory_QueryInterface(&FileOpenDlgClassFactory.IClassFactory_iface, riid, ppv);

    if(IsEqualGUID(&CLSID_FileSaveDialog, rclsid))
        return IClassFactory_QueryInterface(&FileSaveDlgClassFactory.IClassFactory_iface, riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}
