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

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08lx\n", hr)

static IPersistStorage OleObjectPersistStg;
static IOleCache OleObjectCache;
static IRunnableObject OleObjectRunnable;

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
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleObject))
    {
        *ppv = (void *)iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IPersistStorage))
    {
        *ppv = &OleObjectPersistStg;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IOleCache))
    {
        *ppv = &OleObjectCache;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IRunnableObject))
    {
        *ppv = &OleObjectRunnable;
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    trace("OleObject_QueryInterface: returning E_NOINTERFACE\n");
    *ppv = NULL;
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

    trace("end\n");
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

    IStorage_Release(pStorage);

    hr = CoRevokeClassObject(dwRegister);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();
}
