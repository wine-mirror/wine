/*
 * Object Linking and Embedding Tests
 *
 * Copyright 2005 Robert Shearman
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlguid.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

static IPersistStorage OleObjectPersistStg;
static IOleCache *cache;
static IRunnableObject *runnable;

static char const * const *expected_method_list;

#define CHECK_EXPECTED_METHOD(method_name) \
    do { \
        trace("%s\n", method_name); \
        ok(*expected_method_list != NULL, "Extra method %s called\n", method_name); \
        if (*expected_method_list) \
        { \
            ok(!strcmp(*expected_method_list, method_name), "Expected %s to be called instead of %s\n", \
                *expected_method_list, method_name); \
            expected_method_list++; \
        } \
    } while(0)

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    CHECK_EXPECTED_METHOD("OleObject_QueryInterface");

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleObject))
        *ppv = (void *)iface;
    else if (IsEqualIID(riid, &IID_IPersistStorage))
        *ppv = &OleObjectPersistStg;
    else if (IsEqualIID(riid, &IID_IOleCache))
        *ppv = cache;
    else if (IsEqualIID(riid, &IID_IRunnableObject))
        *ppv = runnable;

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    trace("OleObject_QueryInterface: returning E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObject_AddRef");
    return 2;
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObject_Release");
    return 1;
}

static HRESULT WINAPI OleObject_SetClientSite
    (
        IOleObject *iface,
        IOleClientSite *pClientSite
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetClientSite");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite
    (
        IOleObject *iface,
        IOleClientSite **ppClientSite
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetClientSite");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetHostNames
    (
        IOleObject *iface,
        LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetHostNames");
    return S_OK;
}

static HRESULT WINAPI OleObject_Close
    (
        IOleObject *iface,
        DWORD dwSaveOption
    )
{
    CHECK_EXPECTED_METHOD("OleObject_Close");
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker
    (
        IOleObject *iface,
        DWORD dwWhichMoniker,
        IMoniker *pmk
    )
{
    CHECK_EXPECTED_METHOD("OleObject_SetMoniker");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetMoniker
    (
        IOleObject *iface,
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        IMoniker **ppmk
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetMoniker");
    return S_OK;
}

static HRESULT WINAPI OleObject_InitFromData
    (
        IOleObject *iface,
        IDataObject *pDataObject,
        BOOL fCreation,
        DWORD dwReserved
    )
{
    CHECK_EXPECTED_METHOD("OleObject_InitFromData");
    return S_OK;
}

static HRESULT WINAPI OleObject_GetClipboardData
    (
        IOleObject *iface,
        DWORD dwReserved,
        IDataObject **ppDataObject
    )
{
    CHECK_EXPECTED_METHOD("OleObject_GetClipboardData");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb
    (
        IOleObject *iface,
        LONG iVerb,
        LPMSG lpmsg,
        IOleClientSite *pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect
    )
{
    CHECK_EXPECTED_METHOD("OleObject_DoVerb");
    return S_OK;
}

static HRESULT WINAPI OleObject_EnumVerbs
    (
        IOleObject *iface,
        IEnumOLEVERB **ppEnumOleVerb
    )
{
    CHECK_EXPECTED_METHOD("OleObject_EnumVerbs");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update
    (
        IOleObject *iface
    )
{
    CHECK_EXPECTED_METHOD("OleObject_Update");
    return S_OK;
}

static HRESULT WINAPI OleObject_IsUpToDate
    (
        IOleObject *iface
    )
{
    CHECK_EXPECTED_METHOD("OleObject_IsUpToDate");
    return S_OK;
}

HRESULT WINAPI OleObject_GetUserClassID
(
    IOleObject *iface,
    CLSID *pClsid
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetUserClassID");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObject_GetUserType
(
    IOleObject *iface,
    DWORD dwFormOfType,
    LPOLESTR *pszUserType
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetUserType");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObject_SetExtent
(
    IOleObject *iface,
    DWORD dwDrawAspect,
    SIZEL *psizel
)
{
    CHECK_EXPECTED_METHOD("OleObject_SetExtent");
    return S_OK;
}

HRESULT WINAPI OleObject_GetExtent
(
    IOleObject *iface,
    DWORD dwDrawAspect,
    SIZEL *psizel
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetExtent");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObject_Advise
(
    IOleObject *iface,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObject_Advise");
    return S_OK;
}

HRESULT WINAPI OleObject_Unadvise
(
    IOleObject *iface,
    DWORD dwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObject_Unadvise");
    return S_OK;
}

HRESULT WINAPI OleObject_EnumAdvise
(
    IOleObject *iface,
    IEnumSTATDATA **ppenumAdvise
)
{
    CHECK_EXPECTED_METHOD("OleObject_EnumAdvise");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObject_GetMiscStatus
(
    IOleObject *iface,
    DWORD dwAspect,
    DWORD *pdwStatus
)
{
    CHECK_EXPECTED_METHOD("OleObject_GetMiscStatus");
    *pdwStatus = DVASPECT_CONTENT;
    return S_OK;
}

HRESULT WINAPI OleObject_SetColorScheme
(
    IOleObject *iface,
    LOGPALETTE *pLogpal
)
{
    CHECK_EXPECTED_METHOD("OleObject_SetColorScheme");
    return E_NOTIMPL;
}

static IOleObjectVtbl OleObjectVtbl =
{
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static IOleObject OleObject = { &OleObjectVtbl };

static HRESULT WINAPI OleObjectPersistStg_QueryInterface(IPersistStorage *iface, REFIID riid, void **ppv)
{
    trace("OleObjectPersistStg_QueryInterface\n");
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectPersistStg_AddRef(IPersistStorage *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectPersistStg_Release(IPersistStorage *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Release");
    return 1;
}

static HRESULT WINAPI OleObjectPersistStg_GetClassId(IPersistStorage *iface, CLSID *clsid)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_GetClassId");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObjectPersistStg_IsDirty
(
    IPersistStorage *iface
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_IsDirty");
    return S_OK;
}

HRESULT WINAPI OleObjectPersistStg_InitNew
(
    IPersistStorage *iface,
    IStorage *pStg
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_InitNew");
    return S_OK;
}

HRESULT WINAPI OleObjectPersistStg_Load
(
    IPersistStorage *iface,
    IStorage *pStg
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Load");
    return S_OK;
}

HRESULT WINAPI OleObjectPersistStg_Save
(
    IPersistStorage *iface,
    IStorage *pStgSave,
    BOOL fSameAsLoad
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_Save");
    return S_OK;
}

HRESULT WINAPI OleObjectPersistStg_SaveCompleted
(
    IPersistStorage *iface,
    IStorage *pStgNew
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_SaveCompleted");
    return S_OK;
}

HRESULT WINAPI OleObjectPersistStg_HandsOffStorage
(
    IPersistStorage *iface
)
{
    CHECK_EXPECTED_METHOD("OleObjectPersistStg_HandsOffStorage");
    return S_OK;
}

static IPersistStorageVtbl OleObjectPersistStgVtbl = 
{
    OleObjectPersistStg_QueryInterface,
    OleObjectPersistStg_AddRef,
    OleObjectPersistStg_Release,
    OleObjectPersistStg_GetClassId,
    OleObjectPersistStg_IsDirty,
    OleObjectPersistStg_InitNew,
    OleObjectPersistStg_Load,
    OleObjectPersistStg_Save,
    OleObjectPersistStg_SaveCompleted,
    OleObjectPersistStg_HandsOffStorage
};

static IPersistStorage OleObjectPersistStg = { &OleObjectPersistStgVtbl };

static HRESULT WINAPI OleObjectCache_QueryInterface(IOleCache *iface, REFIID riid, void **ppv)
{
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectCache_AddRef(IOleCache *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectCache_Release(IOleCache *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Release");
    return 1;
}

HRESULT WINAPI OleObjectCache_Cache
(
    IOleCache *iface,
    FORMATETC *pformatetc,
    DWORD advf,
    DWORD *pdwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Cache");
    return S_OK;
}

HRESULT WINAPI OleObjectCache_Uncache
(
    IOleCache *iface,
    DWORD dwConnection
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_Uncache");
    return S_OK;
}

HRESULT WINAPI OleObjectCache_EnumCache
(
    IOleCache *iface,
    IEnumSTATDATA **ppenumSTATDATA
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_EnumCache");
    return S_OK;
}


HRESULT WINAPI OleObjectCache_InitCache
(
    IOleCache *iface,
    IDataObject *pDataObject
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_InitCache");
    return S_OK;
}


HRESULT WINAPI OleObjectCache_SetData
(
    IOleCache *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease
)
{
    CHECK_EXPECTED_METHOD("OleObjectCache_SetData");
    return S_OK;
}


static IOleCacheVtbl OleObjectCacheVtbl =
{
    OleObjectCache_QueryInterface,
    OleObjectCache_AddRef,
    OleObjectCache_Release,
    OleObjectCache_Cache,
    OleObjectCache_Uncache,
    OleObjectCache_EnumCache,
    OleObjectCache_InitCache,
    OleObjectCache_SetData
};

static IOleCache OleObjectCache = { &OleObjectCacheVtbl };

static HRESULT WINAPI OleObjectCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI OleObjectCF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI OleObjectCF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI OleObjectCF_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static HRESULT WINAPI OleObjectCF_LockServer(IClassFactory *iface, BOOL lock)
{
    return S_OK;
}

static IClassFactoryVtbl OleObjectCFVtbl =
{
    OleObjectCF_QueryInterface,
    OleObjectCF_AddRef,
    OleObjectCF_Release,
    OleObjectCF_CreateInstance,
    OleObjectCF_LockServer
};

static IClassFactory OleObjectCF = { &OleObjectCFVtbl };

static HRESULT WINAPI OleObjectRunnable_QueryInterface(IRunnableObject *iface, REFIID riid, void **ppv)
{
    return IUnknown_QueryInterface((IUnknown *)&OleObject, riid, ppv);
}

static ULONG WINAPI OleObjectRunnable_AddRef(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_AddRef");
    return 2;
}

static ULONG WINAPI OleObjectRunnable_Release(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_Release");
    return 1;
}

HRESULT WINAPI OleObjectRunnable_GetRunningClass(
    IRunnableObject *iface,
    LPCLSID lpClsid)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_GetRunningClass");
    return E_NOTIMPL;
}

HRESULT WINAPI OleObjectRunnable_Run(
    IRunnableObject *iface,
    LPBINDCTX pbc)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_Run");
    return S_OK;
}

BOOL WINAPI OleObjectRunnable_IsRunning(IRunnableObject *iface)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_IsRunning");
    return TRUE;
}

HRESULT WINAPI OleObjectRunnable_LockRunning(
    IRunnableObject *iface,
    BOOL fLock,
    BOOL fLastUnlockCloses)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_LockRunning");
    return S_OK;
}

HRESULT WINAPI OleObjectRunnable_SetContainedObject(
    IRunnableObject *iface,
    BOOL fContained)
{
    CHECK_EXPECTED_METHOD("OleObjectRunnable_SetContainedObject");
    return S_OK;
}

static IRunnableObjectVtbl OleObjectRunnableVtbl =
{
    OleObjectRunnable_QueryInterface,
    OleObjectRunnable_AddRef,
    OleObjectRunnable_Release,
    OleObjectRunnable_GetRunningClass,
    OleObjectRunnable_Run,
    OleObjectRunnable_IsRunning,
    OleObjectRunnable_LockRunning,
    OleObjectRunnable_SetContainedObject
};

static IRunnableObject OleObjectRunnable = { &OleObjectRunnableVtbl };

static const CLSID CLSID_Equation3 = {0x0002CE02, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static void test_OleCreate(IStorage *pStorage)
{
    HRESULT hr;
    IOleObject *pObject;
    FORMATETC formatetc;
    static const char *methods_olerender_none[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_Release",
        NULL
    };
    static const char *methods_olerender_draw[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_QueryInterface",
        "OleObjectRunnable_AddRef",
        "OleObjectRunnable_Run",
        "OleObjectRunnable_Release",
        "OleObject_QueryInterface",
        "OleObjectCache_AddRef",
        "OleObjectCache_Cache",
        "OleObjectCache_Release",
        "OleObject_Release",
        NULL
    };
    static const char *methods_olerender_format[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_GetMiscStatus",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_SetClientSite",
        "OleObject_Release",
        "OleObject_QueryInterface",
        "OleObjectRunnable_AddRef",
        "OleObjectRunnable_Run",
        "OleObjectRunnable_Release",
        "OleObject_QueryInterface",
        "OleObjectCache_AddRef",
        "OleObjectCache_Cache",
        "OleObjectCache_Release",
        "OleObject_Release",
        NULL
    };
    static const char *methods_olerender_asis[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_Release",
        NULL
    };
    static const char *methods_olerender_draw_no_runnable[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_QueryInterface",
        "OleObject_QueryInterface",
        "OleObjectCache_AddRef",
        "OleObjectCache_Cache",
        "OleObjectCache_Release",
        "OleObject_Release",
        NULL
    };
    static const char *methods_olerender_draw_no_cache[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_InitNew",
        "OleObjectPersistStg_Release",
        "OleObject_QueryInterface",
        "OleObjectRunnable_AddRef",
        "OleObjectRunnable_Run",
        "OleObjectRunnable_Release",
        "OleObject_QueryInterface",
        "OleObject_Release",
        NULL
    };

    runnable = &OleObjectRunnable;
    cache = &OleObjectCache;
    expected_method_list = methods_olerender_none;
    trace("OleCreate with OLERENDER_NONE:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_NONE, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    expected_method_list = methods_olerender_draw;
    trace("OleCreate with OLERENDER_DRAW:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    formatetc.cfFormat = CF_TEXT;
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;
    expected_method_list = methods_olerender_format;
    trace("OleCreate with OLERENDER_FORMAT:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_FORMAT, &formatetc, (IOleClientSite *)0xdeadbeef, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    expected_method_list = methods_olerender_asis;
    trace("OleCreate with OLERENDER_ASIS:\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_ASIS, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    runnable = NULL;
    expected_method_list = methods_olerender_draw_no_runnable;
    trace("OleCreate with OLERENDER_DRAW (no IOlObjectRunnable):\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    runnable = &OleObjectRunnable;
    cache = NULL;
    expected_method_list = methods_olerender_draw_no_cache;
    trace("OleCreate with OLERENDER_DRAW (no IOlObjectRunnable):\n");
    hr = OleCreate(&CLSID_Equation3, &IID_IOleObject, OLERENDER_DRAW, NULL, NULL, pStorage, (void **)&pObject);
    ok_ole_success(hr, "OleCreate");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);
    trace("end\n");
}

static void test_OleLoad(IStorage *pStorage)
{
    HRESULT hr;
    IOleObject *pObject;

    static const char *methods_oleload[] =
    {
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_QueryInterface",
        "OleObject_AddRef",
        "OleObject_GetMiscStatus",
        "OleObject_QueryInterface",
        "OleObjectPersistStg_AddRef",
        "OleObjectPersistStg_Load",
        "OleObjectPersistStg_Release",
        "OleObject_SetClientSite",
        "OleObject_Release",
        "OleObject_QueryInterface",
        "OleObject_Release",
        NULL
    };

    expected_method_list = methods_oleload;
    trace("OleLoad:\n");
    hr = OleLoad(pStorage, &IID_IOleObject, (IOleClientSite *)0xdeadbeef, (void **)&pObject);
    ok_ole_success(hr, "OleLoad");
    IOleObject_Release(pObject);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);
}

static BOOL STDMETHODCALLTYPE draw_continue(ULONG_PTR param)
{
    CHECK_EXPECTED_METHOD("draw_continue");
    return TRUE;
}

static HRESULT WINAPI AdviseSink_QueryInterface(IAdviseSink *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IAdviseSink) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI AdviseSink_AddRef(IAdviseSink *iface)
{
    return 2;
}

static ULONG WINAPI AdviseSink_Release(IAdviseSink *iface)
{
    return 1;
}


static void WINAPI AdviseSink_OnDataChange(
    IAdviseSink *iface,
    FORMATETC *pFormatetc,
    STGMEDIUM *pStgmed)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnDataChange");
}

static void WINAPI AdviseSink_OnViewChange(
    IAdviseSink *iface,
    DWORD dwAspect,
    LONG lindex)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnViewChange");
}

static void WINAPI AdviseSink_OnRename(
    IAdviseSink *iface,
    IMoniker *pmk)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnRename");
}

static void WINAPI AdviseSink_OnSave(IAdviseSink *iface)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnSave");
}

static void WINAPI AdviseSink_OnClose(IAdviseSink *iface)
{
    CHECK_EXPECTED_METHOD("AdviseSink_OnClose");
}

static IAdviseSinkVtbl AdviseSinkVtbl =
{
    AdviseSink_QueryInterface,
    AdviseSink_AddRef,
    AdviseSink_Release,
    AdviseSink_OnDataChange,
    AdviseSink_OnViewChange,
    AdviseSink_OnRename,
    AdviseSink_OnSave,
    AdviseSink_OnClose
};

static IAdviseSink AdviseSink = { &AdviseSinkVtbl };

static HRESULT WINAPI DataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
        return S_OK;
    }
    *ppvObject = NULL;
    return S_OK;
}

static ULONG WINAPI DataObject_AddRef(
            IDataObject*     iface)
{
    return 2;
}

static ULONG WINAPI DataObject_Release(
            IDataObject*     iface)
{
    return 1;
}

static HRESULT WINAPI DataObject_GetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetcIn,
        STGMEDIUM*       pmedium)
{
    CHECK_EXPECTED_METHOD("DataObject_GetData");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(
        IDataObject*     iface,
        LPFORMATETC      pformatetc,
        STGMEDIUM*       pmedium)
{
    CHECK_EXPECTED_METHOD("DataObject_GetDataHere");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetc)
{
    CHECK_EXPECTED_METHOD("DataObject_QueryGetData");
    return S_OK;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(
        IDataObject*     iface,
        LPFORMATETC      pformatectIn,
        LPFORMATETC      pformatetcOut)
{
    CHECK_EXPECTED_METHOD("DataObject_GetCanonicalFormatEtc");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(
        IDataObject*     iface,
        LPFORMATETC      pformatetc,
        STGMEDIUM*       pmedium,
        BOOL             fRelease)
{
    CHECK_EXPECTED_METHOD("DataObject_SetData");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(
        IDataObject*     iface,
        DWORD            dwDirection,
        IEnumFORMATETC** ppenumFormatEtc)
{
    CHECK_EXPECTED_METHOD("DataObject_EnumFormatEtc");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DAdvise(
        IDataObject*     iface,
        FORMATETC*       pformatetc,
        DWORD            advf,
        IAdviseSink*     pAdvSink,
        DWORD*           pdwConnection)
{
    CHECK_EXPECTED_METHOD("DataObject_DAdvise");
    *pdwConnection = 1;
    return S_OK;
}

static HRESULT WINAPI DataObject_DUnadvise(
        IDataObject*     iface,
        DWORD            dwConnection)
{
    CHECK_EXPECTED_METHOD("DataObject_DUnadvise");
    return S_OK;
}

static HRESULT WINAPI DataObject_EnumDAdvise(
        IDataObject*     iface,
        IEnumSTATDATA**  ppenumAdvise)
{
    CHECK_EXPECTED_METHOD("DataObject_EnumDAdvise");
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl DataObjectVtbl =
{
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise
};

static IDataObject DataObject = { &DataObjectVtbl };

static void test_data_cache(void)
{
    HRESULT hr;
    IOleCache2 *pOleCache;
    IStorage *pStorage;
    IPersistStorage *pPS;
    IViewObject *pViewObject;
    IOleCacheControl *pOleCacheControl;
    FORMATETC fmtetc;
    STGMEDIUM stgmedium;
    DWORD dwConnection;
    DWORD dwFreeze;
    RECTL rcBounds;
    HDC hdcMem;
    CLSID clsid;
    char szSystemDir[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    static const WCHAR wszShell32[] = {'\\','s','h','e','l','l','3','2','.','d','l','l',0};

    static const char *methods_cacheinitnew[] =
    {
        "AdviseSink_OnViewChange",
        "AdviseSink_OnViewChange",
        "draw_continue",
        "DataObject_DAdvise",
        "DataObject_DAdvise",
        "DataObject_DUnadvise",
        "DataObject_DUnadvise",
        NULL
    };
    static const char *methods_cacheload[] =
    {
        "AdviseSink_OnViewChange",
        "draw_continue",
        "draw_continue",
        "draw_continue",
        "DataObject_GetData",
        "DataObject_GetData",
        "DataObject_GetData",
        NULL
    };

    GetSystemDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(szSystemDir[0]));

    expected_method_list = methods_cacheinitnew;

    fmtetc.cfFormat = CF_METAFILEPICT;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;

    hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &pStorage);
    ok_ole_success(hr, "StgCreateDocfile");

    /* Test with new data */

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IOleCache2, (LPVOID *)&pOleCache);
    ok_ole_success(hr, "CreateDataCache");

    hr = IOleCache_QueryInterface(pOleCache, &IID_IPersistStorage, (LPVOID *)&pPS);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IPersistStorage)");
    hr = IOleCache_QueryInterface(pOleCache, &IID_IViewObject, (LPVOID *)&pViewObject);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IViewObject)");
    hr = IOleCache_QueryInterface(pOleCache, &IID_IOleCacheControl, (LPVOID *)&pOleCacheControl);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IOleCacheControl)");

    hr = IViewObject_SetAdvise(pViewObject, DVASPECT_ICON, ADVF_PRIMEFIRST, &AdviseSink);
    ok_ole_success(hr, "IViewObject_SetAdvise");

    hr = IPersistStorage_InitNew(pPS, pStorage);
    ok_ole_success(hr, "IPersistStorage_InitNew");

    hr = IPersistStorage_IsDirty(pPS);
    ok_ole_success(hr, "IPersistStorage_IsDirty");

    hr = IPersistStorage_GetClassID(pPS, &clsid);
    ok_ole_success(hr, "IPersistStorage_GetClassID");
    ok(IsEqualCLSID(&clsid, &IID_NULL), "clsid should be blank\n");

    hr = IOleCache_Uncache(pOleCache, 0xdeadbeef);
    ok(hr == OLE_E_NOCONNECTION, "IOleCache_Uncache with invalid value should return OLE_E_NOCONNECTION instead of 0x%x\n", hr);

    for (fmtetc.cfFormat = CF_TEXT; fmtetc.cfFormat < CF_MAX; fmtetc.cfFormat++)
    {
        int i;
        fmtetc.dwAspect = DVASPECT_THUMBNAIL;
        for (i = 0; i < 7; i++)
        {
            fmtetc.tymed = 1 << i;
            hr = IOleCache_Cache(pOleCache, &fmtetc, 0, &dwConnection);
            if ((fmtetc.cfFormat == CF_METAFILEPICT && fmtetc.tymed == TYMED_MFPICT) ||
                (fmtetc.cfFormat == CF_BITMAP && fmtetc.tymed == TYMED_GDI) ||
                (fmtetc.cfFormat == CF_DIB && fmtetc.tymed == TYMED_HGLOBAL) ||
                (fmtetc.cfFormat == CF_ENHMETAFILE && fmtetc.tymed == TYMED_ENHMF))
                ok(hr == S_OK, "IOleCache_Cache cfFormat = %d, tymed = %d should have returned S_OK instead of 0x%08x\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else if (fmtetc.tymed == TYMED_HGLOBAL)
                ok(hr == CACHE_S_FORMATETC_NOTSUPPORTED,
                    "IOleCache_Cache cfFormat = %d, tymed = %d should have returned CACHE_S_FORMATETC_NOTSUPPORTED instead of 0x%08x\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            else
                ok(hr == DV_E_TYMED, "IOleCache_Cache cfFormat = %d, tymed = %d should have returned DV_E_TYMED instead of 0x%08x\n",
                    fmtetc.cfFormat, fmtetc.tymed, hr);
            if (SUCCEEDED(hr))
            {
                hr = IOleCache_Uncache(pOleCache, dwConnection);
                ok_ole_success(hr, "IOleCache_Uncache");
            }
        }
    }

    fmtetc.cfFormat = CF_BITMAP;
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    fmtetc.tymed = TYMED_GDI;
    hr = IOleCache_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok_ole_success(hr, "IOleCache_Cache");

    fmtetc.cfFormat = 0;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.tymed = TYMED_MFPICT;
    hr = IOleCache_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok_ole_success(hr, "IOleCache_Cache");

    MultiByteToWideChar(CP_ACP, 0, szSystemDir, -1, wszPath, sizeof(wszPath)/sizeof(wszPath[0]));
    memcpy(wszPath+lstrlenW(wszPath), wszShell32, sizeof(wszShell32));

    fmtetc.cfFormat = CF_METAFILEPICT;
    stgmedium.tymed = TYMED_MFPICT;
    stgmedium.hMetaFilePict = OleMetafilePictFromIconAndLabel(
        LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION)), wszPath, wszPath, 0);
    stgmedium.pUnkForRelease = NULL;

    fmtetc.dwAspect = DVASPECT_CONTENT;
    hr = IOleCache_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok(hr == OLE_E_BLANK, "IOleCache_SetData for aspect not in cache should have return OLE_E_BLANK instead of 0x%08x\n", hr);

    fmtetc.dwAspect = DVASPECT_ICON;
    hr = IOleCache_SetData(pOleCache, &fmtetc, &stgmedium, FALSE);
    ok_ole_success(hr, "IOleCache_SetData");

    hr = IViewObject_Freeze(pViewObject, DVASPECT_ICON, -1, NULL, &dwFreeze);
    todo_wine {
    ok_ole_success(hr, "IViewObject_Freeze");
    hr = IViewObject_Freeze(pViewObject, DVASPECT_CONTENT, -1, NULL, &dwFreeze);
    ok(hr == OLE_E_BLANK, "IViewObject_Freeze with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);
    }

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

    DeleteDC(hdcMem);

    hr = IOleCacheControl_OnRun(pOleCacheControl, &DataObject);
    todo_wine {
    ok_ole_success(hr, "IOleCacheControl_OnRun");
    }

    hr = IPersistStorage_Save(pPS, pStorage, TRUE);
    ok_ole_success(hr, "IPersistStorage_Save");

    hr = IPersistStorage_SaveCompleted(pPS, NULL);
    ok_ole_success(hr, "IPersistStorage_SaveCompleted");

    hr = IPersistStorage_IsDirty(pPS);
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%x\n", hr);

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache_Release(pOleCache);
    IOleCacheControl_Release(pOleCacheControl);

    todo_wine {
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);
    }

    /* Test with loaded data */
    trace("Testing loaded data with CreateDataCache:\n");
    expected_method_list = methods_cacheload;

    hr = CreateDataCache(NULL, &CLSID_NULL, &IID_IOleCache2, (LPVOID *)&pOleCache);
    ok_ole_success(hr, "CreateDataCache");

    hr = IOleCache_QueryInterface(pOleCache, &IID_IPersistStorage, (LPVOID *)&pPS);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IPersistStorage)");
    hr = IOleCache_QueryInterface(pOleCache, &IID_IViewObject, (LPVOID *)&pViewObject);
    ok_ole_success(hr, "IOleCache_QueryInterface(IID_IViewObject)");

    hr = IViewObject_SetAdvise(pViewObject, DVASPECT_ICON, ADVF_PRIMEFIRST, &AdviseSink);
    ok_ole_success(hr, "IViewObject_SetAdvise");

    hr = IPersistStorage_Load(pPS, pStorage);
    ok_ole_success(hr, "IPersistStorage_Load");

    hr = IPersistStorage_IsDirty(pPS);
    ok(hr == S_FALSE, "IPersistStorage_IsDirty should have returned S_FALSE instead of 0x%x\n", hr);

    fmtetc.cfFormat = 0;
    fmtetc.dwAspect = DVASPECT_ICON;
    fmtetc.lindex = -1;
    fmtetc.ptd = NULL;
    fmtetc.tymed = TYMED_MFPICT;
    hr = IOleCache_Cache(pOleCache, &fmtetc, 0, &dwConnection);
    ok(hr == CACHE_S_SAMECACHE, "IOleCache_Cache with already loaded data format type should return CACHE_S_SAMECACHE instead of 0x%x\n", hr);

    rcBounds.left = 0;
    rcBounds.top = 0;
    rcBounds.right = 100;
    rcBounds.bottom = 100;
    hdcMem = CreateCompatibleDC(NULL);

    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    hr = IViewObject_Draw(pViewObject, DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

    /* unload the cached storage object, causing it to be reloaded */
    hr = IOleCache2_DiscardCache(pOleCache, DISCARDCACHE_NOSAVE);
    ok_ole_success(hr, "IOleCache2_DiscardCache");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");

    /* unload the cached storage object, but don't allow it to be reloaded */
    hr = IPersistStorage_HandsOffStorage(pPS);
    ok_ole_success(hr, "IPersistStorage_HandsOffStorage");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok_ole_success(hr, "IViewObject_Draw");
    hr = IOleCache2_DiscardCache(pOleCache, DISCARDCACHE_NOSAVE);
    ok_ole_success(hr, "IOleCache2_DiscardCache");
    hr = IViewObject_Draw(pViewObject, DVASPECT_ICON, -1, NULL, NULL, NULL, hdcMem, &rcBounds, NULL, draw_continue, 0xdeadbeef);
    ok(hr == OLE_E_BLANK, "IViewObject_Draw with uncached aspect should have returned OLE_E_BLANK instead of 0x%08x\n", hr);

    DeleteDC(hdcMem);

    todo_wine {
    hr = IOleCache_InitCache(pOleCache, &DataObject);
    ok(hr == CACHE_E_NOCACHE_UPDATED, "IOleCache_InitCache should have returned CACHE_E_NOCACHE_UPDATED instead of 0x%08x\n", hr);
    }

    IPersistStorage_Release(pPS);
    IViewObject_Release(pViewObject);
    IOleCache_Release(pOleCache);

    todo_wine {
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);
    }

    IStorage_Release(pStorage);
    ReleaseStgMedium(&stgmedium);
}

START_TEST(ole2)
{
    DWORD dwRegister;
    IStorage *pStorage;
    STATSTG statstg;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoRegisterClassObject(&CLSID_Equation3, (IUnknown *)&OleObjectCF, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegister);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = StgCreateDocfile(NULL, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 0, &pStorage);
    ok_ole_success(hr, "StgCreateDocfile");

    test_OleCreate(pStorage);

    hr = IStorage_Stat(pStorage, &statstg, STATFLAG_NONAME);
    ok_ole_success(hr, "IStorage_Stat");
    ok(IsEqualCLSID(&CLSID_Equation3, &statstg.clsid), "Wrong CLSID in storage\n");

    test_OleLoad(pStorage);

    IStorage_Release(pStorage);

    hr = CoRevokeClassObject(dwRegister);
    ok_ole_success(hr, "CoRevokeClassObject");

    test_data_cache();

    CoUninitialize();
}
