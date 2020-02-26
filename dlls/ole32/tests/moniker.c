/*
 * Moniker Tests
 *
 * Copyright 2004 Robert Shearman
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

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "ocidl.h"
#include "comcat.h"
#include "olectl.h"
#include "initguid.h"

#include "wine/test.h"
#include "wine/heap.h"

#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %d\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %d\n", cLocks)
#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08x\n", hr)

#define CHECK_EXPECTED_METHOD(method_name) \
do { \
    ok(*expected_method_list != NULL, "Extra method %s called\n", method_name); \
        if (*expected_method_list) \
        { \
            ok(!strcmp(*expected_method_list, method_name), "Expected %s to be called instead of %s\n", \
                    *expected_method_list, method_name); \
            expected_method_list++; \
        } \
} while(0)

static char const * const *expected_method_list;
static const WCHAR wszFileName1[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','1','.','d','o','c',0};
static const WCHAR wszFileName2[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','2','.','d','o','c',0};

static const CLSID CLSID_TestMoniker =
{ /* b306bfbc-496e-4f53-b93e-2ff9c83223d7 */
    0xb306bfbc,
    0x496e,
    0x4f53,
    {0xb9, 0x3e, 0x2f, 0xf9, 0xc8, 0x32, 0x23, 0xd7}
};

DEFINE_OLEGUID(CLSID_FileMoniker,      0x303, 0, 0);
DEFINE_OLEGUID(CLSID_ItemMoniker,      0x304, 0, 0);
DEFINE_OLEGUID(CLSID_AntiMoniker,      0x305, 0, 0);
DEFINE_OLEGUID(CLSID_CompositeMoniker, 0x309, 0, 0);
DEFINE_OLEGUID(CLSID_ClassMoniker,     0x31a, 0, 0);
DEFINE_OLEGUID(CLSID_PointerMoniker,   0x306, 0, 0);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == ref, "Unexpected got %u.\n", refcount);
}

#define TEST_MONIKER_TYPE_TODO(m,t) _test_moniker_type(m, t, TRUE, __LINE__)
#define TEST_MONIKER_TYPE(m,t) _test_moniker_type(m, t, FALSE, __LINE__)
static void _test_moniker_type(IMoniker *moniker, DWORD type, BOOL todo, int line)
{
    DWORD type2;
    HRESULT hr;

    hr = IMoniker_IsSystemMoniker(moniker, &type2);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#x.\n", hr);
todo_wine_if(todo)
    ok_(__FILE__, line)(type2 == type, "Unexpected moniker type %d.\n", type2);
}

#define TEST_DISPLAY_NAME(m,name) _test_moniker_name(m, name, __LINE__)
static void _test_moniker_name(IMoniker *moniker, const WCHAR *name, int line)
{
    WCHAR *display_name;
    IBindCtx *pbc;
    HRESULT hr;

    hr = CreateBindCtx(0, &pbc);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create bind context, hr %#x.\n", hr);

    hr = IMoniker_GetDisplayName(moniker, pbc, NULL, &display_name);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get display name, hr %#x.\n", hr);
    ok_(__FILE__, line)(!lstrcmpW(display_name, name), "Unexpected display name %s.\n", wine_dbgstr_w(display_name));

    CoTaskMemFree(display_name);
    IBindCtx_Release(pbc);
}

static IMoniker *create_antimoniker(DWORD level)
{
    LARGE_INTEGER pos;
    IMoniker *moniker;
    IStream *stream;
    HRESULT hr;

    hr = CreateAntiMoniker(&moniker);
    ok(hr == S_OK, "Failed to create antimoniker, hr %#x.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#x.\n", hr);

    hr = IStream_Write(stream, &level, sizeof(level), NULL);
    ok(hr == S_OK, "Failed to write contents, hr %#x.\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to rewind, hr %#x.\n", hr);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Failed to load, hr %#x.\n", hr);

    IStream_Release(stream);

    return moniker;
}

static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
}

static SIZE_T round_global_size(SIZE_T size)
{
    static SIZE_T global_size_alignment = -1;
    if (global_size_alignment == -1)
    {
        void *p = GlobalAlloc(GMEM_FIXED, 1);
        global_size_alignment = GlobalSize(p);
        GlobalFree(p);
    }

    return ((size + global_size_alignment - 1) & ~(global_size_alignment - 1));
}

static DWORD external_connections;

static HRESULT WINAPI ExternalConnection_QueryInterface(IExternalConnection *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ExternalConnection_AddRef(IExternalConnection *iface)
{
    return 2;
}

static ULONG WINAPI ExternalConnection_Release(IExternalConnection *iface)
{
    return 1;
}

static DWORD WINAPI ExternalConnection_AddConnection(IExternalConnection *iface, DWORD extconn, DWORD reserved)
{
    ok(extconn == EXTCONN_STRONG, "extconn = %d\n", extconn);
    ok(!reserved, "reserved = %x\n", reserved);
    return ++external_connections;
}

static DWORD WINAPI ExternalConnection_ReleaseConnection(IExternalConnection *iface, DWORD extconn,
        DWORD reserved, BOOL fLastReleaseCloses)
{
    ok(extconn == EXTCONN_STRONG, "extconn = %d\n", extconn);
    ok(!reserved, "reserved = %x\n", reserved);

    return --external_connections;
}

static const IExternalConnectionVtbl ExternalConnectionVtbl = {
    ExternalConnection_QueryInterface,
    ExternalConnection_AddRef,
    ExternalConnection_Release,
    ExternalConnection_AddConnection,
    ExternalConnection_ReleaseConnection
};

static IExternalConnection ExternalConnection = { &ExternalConnectionVtbl };

static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IExternalConnection)) {
        *ppvObj = &ExternalConnection;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IClassFactory_Release(LPCLASSFACTORY iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    Test_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory Test_ClassFactory = { &TestClassFactory_Vtbl };

typedef struct
{
    IUnknown IUnknown_iface;
    ULONG refs;
} HeapUnknown;

static inline HeapUnknown *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HeapUnknown, IUnknown_iface);
}

static HRESULT WINAPI HeapUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI HeapUnknown_AddRef(IUnknown *iface)
{
    HeapUnknown *This = impl_from_IUnknown(iface);
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement((LONG*)&This->refs);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static const IUnknownVtbl HeapUnknown_Vtbl =
{
    HeapUnknown_QueryInterface,
    HeapUnknown_AddRef,
    HeapUnknown_Release
};

struct test_moniker
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    IOleItemContainer IOleItemContainer_iface;
    IParseDisplayName IParseDisplayName_iface;
    LONG refcount;

    BOOL no_IROTData;
};

static struct test_moniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, struct test_moniker, IMoniker_iface);
}

static struct test_moniker *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, struct test_moniker, IROTData_iface);
}

static struct test_moniker *impl_from_IOleItemContainer(IOleItemContainer *iface)
{
    return CONTAINING_RECORD(iface, struct test_moniker, IOleItemContainer_iface);
}

static struct test_moniker *impl_from_IParseDisplayName(IParseDisplayName *iface)
{
    return CONTAINING_RECORD(iface, struct test_moniker, IParseDisplayName_iface);
}

static HRESULT WINAPI test_moniker_parse_QueryInterface(IParseDisplayName *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IParseDisplayName))
    {
        *obj = iface;
        IParseDisplayName_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_moniker_parse_AddRef(IParseDisplayName *iface)
{
    struct test_moniker *moniker = impl_from_IParseDisplayName(iface);
    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI test_moniker_parse_Release(IParseDisplayName *iface)
{
    struct test_moniker *moniker = impl_from_IParseDisplayName(iface);
    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI test_moniker_parse_ParseDisplayName(IParseDisplayName *iface,
        IBindCtx *pbc, LPOLESTR displayname, ULONG *eaten, IMoniker **out)
{
    return E_NOTIMPL;
}

static const IParseDisplayNameVtbl test_moniker_parse_vtbl =
{
    test_moniker_parse_QueryInterface,
    test_moniker_parse_AddRef,
    test_moniker_parse_Release,
    test_moniker_parse_ParseDisplayName,
};

static HRESULT WINAPI test_item_container_QueryInterface(IOleItemContainer *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IOleItemContainer) ||
            IsEqualIID(riid, &IID_IOleContainer) ||
            IsEqualIID(riid, &IID_IParseDisplayName) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IOleItemContainer_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_item_container_AddRef(IOleItemContainer *iface)
{
    struct test_moniker *moniker = impl_from_IOleItemContainer(iface);
    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI test_item_container_Release(IOleItemContainer *iface)
{
    struct test_moniker *moniker = impl_from_IOleItemContainer(iface);
    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI test_item_container_ParseDisplayName(IOleItemContainer *iface,
        IBindCtx *pbc, LPOLESTR displayname, ULONG *eaten, IMoniker **out)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_item_container_EnumObjects(IOleItemContainer *iface, DWORD flags,
        IEnumUnknown **ppenum)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI test_item_container_LockContainer(IOleItemContainer *iface, BOOL lock)
{
    return S_OK;
}

static HRESULT WINAPI test_item_container_GetObject(IOleItemContainer *iface, LPOLESTR item,
        DWORD bind_speed, IBindCtx *pbc, REFIID riid, void **obj)
{
    struct test_moniker *moniker = impl_from_IOleItemContainer(iface);

    if (IsEqualIID(riid, &IID_IParseDisplayName))
    {
        *obj = &moniker->IParseDisplayName_iface;
        IOleItemContainer_AddRef(iface);
    }

    return 0x8bee0000 | bind_speed;
}

static HRESULT WINAPI test_item_container_GetObjectStorage(IOleItemContainer *iface, LPOLESTR item,
        IBindCtx *pbc, REFIID riid, void **obj)
{
    return 0x8bee0001;
}

static HRESULT WINAPI test_item_container_IsRunning(IOleItemContainer *iface, LPOLESTR item)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IOleItemContainerVtbl test_item_container_vtbl =
{
    test_item_container_QueryInterface,
    test_item_container_AddRef,
    test_item_container_Release,
    test_item_container_ParseDisplayName,
    test_item_container_EnumObjects,
    test_item_container_LockContainer,
    test_item_container_GetObject,
    test_item_container_GetObjectStorage,
    test_item_container_IsRunning,
};

static HRESULT WINAPI
Moniker_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    struct test_moniker *moniker = impl_from_IMoniker(iface);

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid)      ||
        IsEqualIID(&IID_IPersist, riid)      ||
        IsEqualIID(&IID_IPersistStream,riid) ||
        IsEqualIID(&IID_IMoniker, riid))
        *ppvObject = iface;
    if (IsEqualIID(&IID_IROTData, riid))
    {
        CHECK_EXPECTED_METHOD("Moniker_QueryInterface(IID_IROTData)");
        if (!moniker->no_IROTData)
            *ppvObject = &moniker->IROTData_iface;
    }

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI
Moniker_AddRef(IMoniker* iface)
{
    struct test_moniker *moniker = impl_from_IMoniker(iface);
    return InterlockedIncrement(&moniker->refcount);
}

static ULONG WINAPI
Moniker_Release(IMoniker* iface)
{
    struct test_moniker *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    if (!refcount)
        heap_free(moniker);

    return refcount;
}

static HRESULT WINAPI
Moniker_GetClassID(IMoniker* iface, CLSID *pClassID)
{
    CHECK_EXPECTED_METHOD("Moniker_GetClassID");

    *pClassID = CLSID_TestMoniker;

    return S_OK;
}

static HRESULT WINAPI
Moniker_IsDirty(IMoniker* iface)
{
    CHECK_EXPECTED_METHOD("Moniker_IsDirty");

    return S_FALSE;
}

static HRESULT WINAPI
Moniker_Load(IMoniker* iface, IStream* pStm)
{
    CHECK_EXPECTED_METHOD("Moniker_Load");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    CHECK_EXPECTED_METHOD("Moniker_Save");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    CHECK_EXPECTED_METHOD("Moniker_GetSizeMax");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToObject(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid,
        void **obj)
{
    struct test_moniker *moniker = impl_from_IMoniker(iface);

    if (IsEqualIID(riid, &IID_IOleItemContainer))
    {
        *obj = &moniker->IOleItemContainer_iface;
        IMoniker_AddRef(iface);
        return S_OK;
    }

    CHECK_EXPECTED_METHOD("Moniker_BindToObject");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvObject)
{
    CHECK_EXPECTED_METHOD("Moniker_BindToStorage");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    CHECK_EXPECTED_METHOD("Moniker_Reduce");

    if (ppmkReduced==NULL)
        return E_POINTER;

    IMoniker_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI
Moniker_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
    CHECK_EXPECTED_METHOD("Moniker_ComposeWith");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    CHECK_EXPECTED_METHOD("Moniker_Enum");

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI
Moniker_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    CHECK_EXPECTED_METHOD("Moniker_IsEqual");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Hash(IMoniker* iface,DWORD* pdwHash)
{
    CHECK_EXPECTED_METHOD("Moniker_Hash");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    CHECK_EXPECTED_METHOD("Moniker_IsRunning");
    return 0x8beef000;
}

static HRESULT WINAPI
Moniker_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc,
                                    IMoniker* pmkToLeft, FILETIME* pFileTime)
{
    CHECK_EXPECTED_METHOD("Moniker_GetTimeOfLastChange");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    CHECK_EXPECTED_METHOD("Moniker_Inverse");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    CHECK_EXPECTED_METHOD("Moniker_CommonPrefixWith");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    CHECK_EXPECTED_METHOD("Moniker_RelativePathTo");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    static const WCHAR wszDisplayName[] = {'*','*','G','e','m','m','a',0};
    CHECK_EXPECTED_METHOD("Moniker_GetDisplayName");
    *ppszDisplayName = CoTaskMemAlloc(sizeof(wszDisplayName));
    memcpy(*ppszDisplayName, wszDisplayName, sizeof(wszDisplayName));
    return S_OK;
}

static HRESULT WINAPI
Moniker_ParseDisplayName(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                     LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    CHECK_EXPECTED_METHOD("Moniker_ParseDisplayName");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    CHECK_EXPECTED_METHOD("Moniker_IsSystemMoniker");

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_NONE;

    return S_FALSE;
}

static HRESULT WINAPI
ROTData_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{
    struct test_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI
ROTData_AddRef(IROTData *iface)
{
    struct test_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI
ROTData_Release(IROTData* iface)
{
    struct test_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI
ROTData_GetComparisonData(IROTData* iface, BYTE* pbData,
                                          ULONG cbMax, ULONG* pcbData)
{
    CHECK_EXPECTED_METHOD("ROTData_GetComparisonData");

    *pcbData = 1;
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    *pbData = 0xde;

    return S_OK;
}

static IROTDataVtbl ROTDataVtbl =
{
    ROTData_QueryInterface,
    ROTData_AddRef,
    ROTData_Release,
    ROTData_GetComparisonData
};

static const IMonikerVtbl MonikerVtbl =
{
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static struct test_moniker *create_test_moniker(void)
{
    struct test_moniker *obj;

    obj = heap_alloc_zero(sizeof(*obj));
    obj->IMoniker_iface.lpVtbl = &MonikerVtbl;
    obj->IROTData_iface.lpVtbl = &ROTDataVtbl;
    obj->IOleItemContainer_iface.lpVtbl = &test_item_container_vtbl;
    obj->IParseDisplayName_iface.lpVtbl = &test_moniker_parse_vtbl;
    obj->refcount = 1;

    return obj;
}

static void test_ROT(void)
{
    static const WCHAR wszFileName[] = {'B','E','2','0','E','2','F','5','-',
        '1','9','0','3','-','4','A','A','E','-','B','1','A','F','-',
        '2','0','4','6','E','5','8','6','C','9','2','5',0};
    HRESULT hr;
    IMoniker *pMoniker = NULL;
    struct test_moniker *test_moniker;
    IRunningObjectTable *pROT = NULL;
    DWORD dwCookie;
    static const char *methods_register_no_ROTData[] =
    {
        "Moniker_Reduce",
        "Moniker_GetTimeOfLastChange",
        "Moniker_QueryInterface(IID_IROTData)",
        "Moniker_GetDisplayName",
        "Moniker_GetClassID",
        NULL
    };
    static const char *methods_register[] =
    {
        "Moniker_Reduce",
        "Moniker_GetTimeOfLastChange",
        "Moniker_QueryInterface(IID_IROTData)",
        "ROTData_GetComparisonData",
        NULL
    };
    static const char *methods_isrunning_no_ROTData[] =
    {
        "Moniker_Reduce",
        "Moniker_QueryInterface(IID_IROTData)",
        "Moniker_GetDisplayName",
        "Moniker_GetClassID",
        NULL
    };
    static const char *methods_isrunning[] =
    {
        "Moniker_Reduce",
        "Moniker_QueryInterface(IID_IROTData)",
        "ROTData_GetComparisonData",
        NULL
    };

    cLocks = 0;

    hr = GetRunningObjectTable(0, &pROT);
    ok_ole_success(hr, GetRunningObjectTable);

    test_moniker = create_test_moniker();
    test_moniker->no_IROTData = TRUE;

    expected_method_list = methods_register_no_ROTData;
    external_connections = 0;
    /* try with our own moniker that doesn't support IROTData */
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE, (IUnknown *)&Test_ClassFactory,
            &test_moniker->IMoniker_iface, &dwCookie);
    ok(hr == S_OK, "Failed to register interface, hr %#x.\n", hr);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);
    ok(external_connections == 1, "external_connections = %d\n", external_connections);

    ok_more_than_one_lock();

    expected_method_list = methods_isrunning_no_ROTData;
    hr = IRunningObjectTable_IsRunning(pROT, &test_moniker->IMoniker_iface);
    ok_ole_success(hr, IRunningObjectTable_IsRunning);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);
    ok(external_connections == 0, "external_connections = %d\n", external_connections);

    ok_no_locks();

    expected_method_list = methods_register;
    /* try with our own moniker */
    test_moniker->no_IROTData = FALSE;
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE,
        (IUnknown *)&Test_ClassFactory, &test_moniker->IMoniker_iface, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    ok_more_than_one_lock();

    expected_method_list = methods_isrunning;
    hr = IRunningObjectTable_IsRunning(pROT, &test_moniker->IMoniker_iface);
    ok_ole_success(hr, IRunningObjectTable_IsRunning);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    hr = CreateFileMoniker(wszFileName, &pMoniker);
    ok_ole_success(hr, CreateClassMoniker);

    /* test flags: 0 */
    external_connections = 0;
    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown*)&Test_ClassFactory,
                                      pMoniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);
    ok(external_connections == 0, "external_connections = %d\n", external_connections);

    ok_more_than_one_lock();

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    /* test flags: ROTFLAGS_REGISTRATIONKEEPSALIVE */
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);

    ok_more_than_one_lock();

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    /* test flags: ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT */
    /* only succeeds when process is started by SCM and has LocalService
     * or RunAs AppId values */
    hr = IRunningObjectTable_Register(pROT,
        ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    todo_wine {
    ok(hr == CO_E_WRONG_SERVER_IDENTITY, "Unexpected hr %#x.\n", hr);
    }

    hr = IRunningObjectTable_Register(pROT, 0xdeadbeef,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    ok(hr == E_INVALIDARG, "IRunningObjectTable_Register should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    IMoniker_Release(pMoniker);
    IMoniker_Release(&test_moniker->IMoniker_iface);

    IRunningObjectTable_Release(pROT);
}

static void test_ROT_multiple_entries(void)
{
    HRESULT hr;
    IMoniker *pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;
    DWORD dwCookie1, dwCookie2;
    IUnknown *pObject = NULL;
    static const WCHAR moniker_path[] =
        {'\\', 'w','i','n','d','o','w','s','\\','s','y','s','t','e','m','\\','t','e','s','t','1','.','d','o','c',0};

    hr = GetRunningObjectTable(0, &pROT);
    ok_ole_success(hr, GetRunningObjectTable);

    hr = CreateFileMoniker(moniker_path, &pMoniker);
    ok_ole_success(hr, CreateFileMoniker);

    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown *)&Test_ClassFactory, pMoniker, &dwCookie1);
    ok_ole_success(hr, IRunningObjectTable_Register);

    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown *)&Test_ClassFactory, pMoniker, &dwCookie2);
    ok(hr == MK_S_MONIKERALREADYREGISTERED, "IRunningObjectTable_Register should have returned MK_S_MONIKERALREADYREGISTERED instead of 0x%08x\n", hr);

    ok(dwCookie1 != dwCookie2, "cookie returned for registering duplicate object shouldn't match cookie of original object (0x%x)\n", dwCookie1);

    hr = IRunningObjectTable_GetObject(pROT, pMoniker, &pObject);
    ok_ole_success(hr, IRunningObjectTable_GetObject);
    IUnknown_Release(pObject);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    hr = IRunningObjectTable_GetObject(pROT, pMoniker, &pObject);
    ok_ole_success(hr, IRunningObjectTable_GetObject);
    IUnknown_Release(pObject);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie2);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    IMoniker_Release(pMoniker);

    IRunningObjectTable_Release(pROT);
}

static HRESULT WINAPI ParseDisplayName_QueryInterface(IParseDisplayName *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IParseDisplayName))
    {
        *ppv = iface;
        IParseDisplayName_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ParseDisplayName_AddRef(IParseDisplayName *iface)
{
    return 2;
}

static ULONG WINAPI ParseDisplayName_Release(IParseDisplayName *iface)
{
    return 1;
}

static LPCWSTR expected_display_name;

static HRESULT WINAPI ParseDisplayName_ParseDisplayName(IParseDisplayName *iface,
                                                        IBindCtx *pbc,
                                                        LPOLESTR pszDisplayName,
                                                        ULONG *pchEaten,
                                                        IMoniker **ppmkOut)
{
    char display_nameA[256];
    WideCharToMultiByte(CP_ACP, 0, pszDisplayName, -1, display_nameA, sizeof(display_nameA), NULL, NULL);
    ok(!lstrcmpW(pszDisplayName, expected_display_name), "unexpected display name \"%s\"\n", display_nameA);
    ok(pszDisplayName == expected_display_name, "pszDisplayName should be the same pointer as passed into MkParseDisplayName\n");
    *pchEaten = lstrlenW(pszDisplayName);
    return CreateAntiMoniker(ppmkOut);
}

static const IParseDisplayNameVtbl ParseDisplayName_Vtbl =
{
    ParseDisplayName_QueryInterface,
    ParseDisplayName_AddRef,
    ParseDisplayName_Release,
    ParseDisplayName_ParseDisplayName
};

static IParseDisplayName ParseDisplayName = { &ParseDisplayName_Vtbl };

static int count_moniker_matches(IBindCtx * pbc, IEnumMoniker * spEM)
{
    IMoniker * spMoniker;
    int monCnt=0, matchCnt=0;

    while ((IEnumMoniker_Next(spEM, 1, &spMoniker, NULL)==S_OK))
    {
        HRESULT hr;
        WCHAR * szDisplayn;
        monCnt++;
        hr=IMoniker_GetDisplayName(spMoniker, pbc, NULL, &szDisplayn);
        if (SUCCEEDED(hr))
        {
            if (!lstrcmpiW(szDisplayn, wszFileName1) || !lstrcmpiW(szDisplayn, wszFileName2))
                matchCnt++;
            CoTaskMemFree(szDisplayn);
        }
    }
    return matchCnt;
}

static void test_MkParseDisplayName(void)
{
    IBindCtx * pbc = NULL;
    HRESULT hr;
    IMoniker * pmk  = NULL;
    IMoniker * pmk1 = NULL;
    IMoniker * pmk2 = NULL;
    ULONG eaten;
    int matchCnt;
    IUnknown * object = NULL;

    IUnknown *lpEM1;

    IEnumMoniker *spEM1  = NULL;
    IEnumMoniker *spEM2  = NULL;
    IEnumMoniker *spEM3  = NULL;

    DWORD pdwReg1=0;
    DWORD grflags=0;
    DWORD pdwReg2=0;
    IRunningObjectTable * pprot=NULL;

    /* CLSID of My Computer */
    static const WCHAR wszDisplayName[] = {'c','l','s','i','d',':',
        '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};
    static const WCHAR wszDisplayNameClsid[] = {'c','l','s','i','d',':',0};
    static const WCHAR wszNonExistentProgId[] = {'N','o','n','E','x','i','s','t','e','n','t','P','r','o','g','I','d',':',0};
    static const WCHAR wszDisplayNameRunning[] = {'W','i','n','e','T','e','s','t','R','u','n','n','i','n','g',0};
    static const WCHAR wszDisplayNameProgId1[] = {'S','t','d','F','o','n','t',':',0};
    static const WCHAR wszDisplayNameProgId2[] = {'@','S','t','d','F','o','n','t',0};
    static const WCHAR wszDisplayNameProgIdFail[] = {'S','t','d','F','o','n','t',0};
    static const WCHAR wszEmpty[] = {0};
    char szDisplayNameFile[256];
    WCHAR wszDisplayNameFile[256];
    int i, len;

    const struct
    {
        LPBC *ppbc;
        LPCOLESTR szDisplayName;
        LPDWORD pchEaten;
        LPMONIKER *ppmk;
    } invalid_parameters[] =
    {
        {NULL,  NULL,     NULL,   NULL},
        {NULL,  NULL,     NULL,   &pmk},
        {NULL,  NULL,     &eaten, NULL},
        {NULL,  NULL,     &eaten, &pmk},
        {NULL,  wszEmpty, NULL,   NULL},
        {NULL,  wszEmpty, NULL,   &pmk},
        {NULL,  wszEmpty, &eaten, NULL},
        {NULL,  wszEmpty, &eaten, &pmk},
        {&pbc,  NULL,     NULL,   NULL},
        {&pbc,  NULL,     NULL,   &pmk},
        {&pbc,  NULL,     &eaten, NULL},
        {&pbc,  NULL,     &eaten, &pmk},
        {&pbc,  wszEmpty, NULL,   NULL},
        {&pbc,  wszEmpty, NULL,   &pmk},
        {&pbc,  wszEmpty, &eaten, NULL},
        {&pbc,  wszEmpty, &eaten, &pmk},
    };

    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        eaten = 0xdeadbeef;
        pmk = (IMoniker *)0xdeadbeef;
        hr = MkParseDisplayName(invalid_parameters[i].ppbc ? *invalid_parameters[i].ppbc : NULL,
                                invalid_parameters[i].szDisplayName,
                                invalid_parameters[i].pchEaten,
                                invalid_parameters[i].ppmk);
        ok(hr == E_INVALIDARG, "[%d] MkParseDisplayName should have failed with E_INVALIDARG instead of 0x%08x\n", i, hr);
        ok(eaten == 0xdeadbeef, "[%d] Processed character count should have been 0xdeadbeef instead of %u\n", i, eaten);
        ok(pmk == (IMoniker *)0xdeadbeef, "[%d] Output moniker pointer should have been 0xdeadbeef instead of %p\n", i, pmk);
    }

    eaten = 0xdeadbeef;
    pmk = (IMoniker *)0xdeadbeef;
    hr = MkParseDisplayName(pbc, wszNonExistentProgId, &eaten, &pmk);
todo_wine
    ok(hr == MK_E_SYNTAX, "Unexpected hr %#x.\n", hr);
    ok(eaten == 0, "Processed character count should have been 0 instead of %u\n", eaten);
    ok(pmk == NULL, "Output moniker pointer should have been NULL instead of %p\n", pmk);

    /* no special handling of "clsid:" without the string form of the clsid
     * following */
    eaten = 0xdeadbeef;
    pmk = (IMoniker *)0xdeadbeef;
    hr = MkParseDisplayName(pbc, wszDisplayNameClsid, &eaten, &pmk);
todo_wine
    ok(hr == MK_E_SYNTAX, "Unexpected hr %#x.\n", hr);
    ok(eaten == 0, "Processed character count should have been 0 instead of %u\n", eaten);
    ok(pmk == NULL, "Output moniker pointer should have been NULL instead of %p\n", pmk);

    /* shows clsid has higher precedence than a running object */
    hr = CreateFileMoniker(wszDisplayName, &pmk);
    ok_ole_success(hr, CreateFileMoniker);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok_ole_success(hr, IBindCtx_GetRunningObjectTable);
    hr = IRunningObjectTable_Register(pprot, 0, (IUnknown *)&Test_ClassFactory, pmk, &pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Register);
    IMoniker_Release(pmk);
    pmk = NULL;
    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == ARRAY_SIZE(wszDisplayName) - 1,
        "Processed character count should have been 43 instead of %u\n", eaten);
    if (pmk)
    {
        TEST_MONIKER_TYPE(pmk, MKSYS_CLASSMONIKER);
        IMoniker_Release(pmk);
    }
    hr = IRunningObjectTable_Revoke(pprot, pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);
    IRunningObjectTable_Release(pprot);

    hr = CreateFileMoniker(wszDisplayNameRunning, &pmk);
    ok_ole_success(hr, CreateFileMoniker);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok_ole_success(hr, IBindCtx_GetRunningObjectTable);
    hr = IRunningObjectTable_Register(pprot, 0, (IUnknown *)&Test_ClassFactory, pmk, &pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Register);
    IMoniker_Release(pmk);
    pmk = NULL;
    hr = MkParseDisplayName(pbc, wszDisplayNameRunning, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == ARRAY_SIZE(wszDisplayNameRunning) - 1,
        "Processed character count should have been 15 instead of %u\n", eaten);
    if (pmk)
    {
        TEST_MONIKER_TYPE(pmk, MKSYS_FILEMONIKER);
        IMoniker_Release(pmk);
    }
    hr = IRunningObjectTable_Revoke(pprot, pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);
    IRunningObjectTable_Release(pprot);

    hr = CoRegisterClassObject(&CLSID_StdFont, (IUnknown *)&ParseDisplayName, CLSCTX_INPROC_SERVER, REGCLS_MULTI_SEPARATE, &pdwReg1);
    ok_ole_success(hr, CoRegisterClassObject);

    expected_display_name = wszDisplayNameProgId1;
    hr = MkParseDisplayName(pbc, wszDisplayNameProgId1, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == ARRAY_SIZE(wszDisplayNameProgId1) - 1,
        "Processed character count should have been 8 instead of %u\n", eaten);
    if (pmk)
    {
        TEST_MONIKER_TYPE(pmk, MKSYS_ANTIMONIKER);
        IMoniker_Release(pmk);
    }

    expected_display_name = wszDisplayNameProgId2;
    hr = MkParseDisplayName(pbc, wszDisplayNameProgId2, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == ARRAY_SIZE(wszDisplayNameProgId2) - 1,
        "Processed character count should have been 8 instead of %u\n", eaten);
    if (pmk)
    {
        TEST_MONIKER_TYPE(pmk, MKSYS_ANTIMONIKER);
        IMoniker_Release(pmk);
    }

    eaten = 0xdeadbeef;
    pmk = (IMoniker *)0xdeadbeef;
    hr = MkParseDisplayName(pbc, wszDisplayNameProgIdFail, &eaten, &pmk);
todo_wine
    ok(hr == MK_E_SYNTAX, "Unexpected hr %#x.\n", hr);
    ok(eaten == 0, "Processed character count should have been 0 instead of %u\n", eaten);
    ok(pmk == NULL, "Output moniker pointer should have been NULL instead of %p\n", pmk);

    hr = CoRevokeClassObject(pdwReg1);
    ok_ole_success(hr, CoRevokeClassObject);

    GetSystemDirectoryA(szDisplayNameFile, sizeof(szDisplayNameFile));
    strcat(szDisplayNameFile, "\\kernel32.dll");
    len = MultiByteToWideChar(CP_ACP, 0, szDisplayNameFile, -1, wszDisplayNameFile,
        ARRAY_SIZE(wszDisplayNameFile));
    hr = MkParseDisplayName(pbc, wszDisplayNameFile, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == len - 1, "Processed character count should have been %d instead of %u\n", len - 1, eaten);
    if (pmk)
    {
        TEST_MONIKER_TYPE(pmk, MKSYS_FILEMONIKER);
        IMoniker_Release(pmk);
    }

    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    ok(eaten == ARRAY_SIZE(wszDisplayName) - 1,
        "Processed character count should have been 43 instead of %u\n", eaten);

    if (pmk)
    {
        hr = IMoniker_BindToObject(pmk, pbc, NULL, &IID_IUnknown, (LPVOID*)&object);
        ok_ole_success(hr, IMoniker_BindToObject);

        if (SUCCEEDED(hr))
            IUnknown_Release(object);
        IMoniker_Release(pmk);
    }
    IBindCtx_Release(pbc);

    /* Test the EnumMoniker interface */
    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = CreateFileMoniker(wszFileName1, &pmk1);
    ok(hr == S_OK, "Failed to create file moniker, hr %#x.\n", hr);
    hr = CreateFileMoniker(wszFileName2, &pmk2);
    ok(hr == S_OK, "Failed to create file moniker, hr %#x.\n", hr);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok(hr == S_OK, "Failed to get ROT, hr %#x.\n", hr);

    /* Check EnumMoniker before registering */
    hr = IRunningObjectTable_EnumRunning(pprot, &spEM1);
    ok(hr == S_OK, "Failed to get enum object, hr %#x.\n", hr);
    hr = IEnumMoniker_QueryInterface(spEM1, &IID_IUnknown, (void *)&lpEM1);
    /* Register a couple of Monikers and check is ok */
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    matchCnt = count_moniker_matches(pbc, spEM1);

    grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk1, &pdwReg1);
    ok(hr == S_OK, "Failed to register object, hr %#x.\n", hr);

    grflags=0;
    grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk2, &pdwReg2);
    ok(hr == S_OK, "Failed to register object, hr %#x.\n", hr);

    hr = IRunningObjectTable_EnumRunning(pprot, &spEM2);
    ok(hr == S_OK, "Failed to get enum object, hr %#x.\n", hr);

    matchCnt = count_moniker_matches(pbc, spEM2);
    ok(matchCnt==2, "Number of matches should be equal to 2 not %i\n", matchCnt);

    IEnumMoniker_Clone(spEM2, &spEM3);

    matchCnt = count_moniker_matches(pbc, spEM3);
    ok(matchCnt==0, "Number of matches should be equal to 0 not %i\n", matchCnt);
    IEnumMoniker_Reset(spEM3);

    matchCnt = count_moniker_matches(pbc, spEM3);
    ok(matchCnt==2, "Number of matches should be equal to 2 not %i\n", matchCnt);

    hr = IRunningObjectTable_Revoke(pprot,pdwReg1);
    ok(hr == S_OK, "Failed to revoke, hr %#x.\n", hr);
    hr = IRunningObjectTable_Revoke(pprot,pdwReg2);
    ok(hr == S_OK, "Failed to revoke, hr %#x.\n", hr);
    IUnknown_Release(lpEM1);
    IEnumMoniker_Release(spEM1);
    IEnumMoniker_Release(spEM2);
    IEnumMoniker_Release(spEM3);
    IMoniker_Release(pmk1);
    IMoniker_Release(pmk2);
    IRunningObjectTable_Release(pprot);

    IBindCtx_Release(pbc);
}

static const LARGE_INTEGER llZero;

static const BYTE expected_class_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
    0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,
};

static const BYTE expected_class_moniker_saved_data[] =
{
     0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,
};

static const BYTE expected_class_moniker_comparison_data[] =
{
     0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
};

static const WCHAR expected_class_moniker_display_name[] =
{
    'c','l','s','i','d',':','0','0','0','2','E','0','0','5','-','0','0','0',
    '0','-','0','0','0','0','-','C','0','0','0','-','0','0','0','0','0','0',
    '0','0','0','0','4','6',':',0
};

static const BYTE expected_item_moniker_comparison_data[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
      '!',0x00, 'T',0x00, 'E',0x00, 'S',0x00,
      'T',0x00,0x00,0x00,
};

static const BYTE expected_item_moniker_comparison_data2[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
      'T',0x00, 'E',0x00, 'S',0x00, 'T',0x00,
     0x00,0x00,
};

static const BYTE expected_item_moniker_comparison_data4[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
      '&',0x00, '&',0x00, 'T',0x00, 'E',0x00,
      'S',0x00, 'T',0x00,0x00,0x00,
};

static const BYTE expected_item_moniker_comparison_data5[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
      'A',0x00, 'B',0x00, 'T',0x00, 'E',0x00,
      'S',0x00, 'T',0x00,0x00,0x00,
};

static const BYTE expected_item_moniker_comparison_data6[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,
};

static const BYTE expected_item_moniker_saved_data[] =
{
     0x02,0x00,0x00,0x00, '!',0x00,0x05,0x00,
     0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_saved_data2[] =
{
     0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,
      'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_saved_data3[] =
{
     0x01,0x00,0x00,0x00,0x00,0x05,0x00,0x00,
     0x00,'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_saved_data4[] =
{
     0x03,0x00,0x00,0x00, '&', '&',0x00,0x05,
     0x00,0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_saved_data5[] =
{
     0x03,0x00,0x00,0x00, 'a', 'b',0x00,0x05,
     0x00,0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_saved_data6[] =
{
     0x01,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
     0x00,0x00,
};

static const BYTE expected_item_moniker_marshal_data[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,
     0x02,0x00,0x00,0x00, '!',0x00,0x05,0x00,
     0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_marshal_data2[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x2e,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,
      'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_marshal_data3[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x32,0x00,0x00,0x00,
     0x01,0x00,0x00,0x00,0x00,0x05,0x00,0x00,
     0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_marshal_data4[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x3a,0x00,0x00,0x00,
     0x03,0x00,0x00,0x00, '&', '&',0x00,0x05,
     0x00,0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_marshal_data5[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x3a,0x00,0x00,0x00,
     0x03,0x00,0x00,0x00, 'a', 'b',0x00,0x05,
     0x00,0x00,0x00, 'T', 'e', 's', 't',0x00,
};

static const BYTE expected_item_moniker_marshal_data6[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x22,0x00,0x00,0x00,
     0x01,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
     0x00,0x00,
};

static const BYTE expected_anti_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_marshal_data2[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_saved_data[] =
{
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_saved_data2[] =
{
    0x02,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_comparison_data[] =
{
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_comparison_data2[] =
{
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x02,0x00,0x00,0x00,
};

static const BYTE expected_gc_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x09,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x2c,0x01,0x00,0x00,
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x21,0x00,0x05,0x00,
    0x00,0x00,0x54,0x65,0x73,0x74,0x00,0x4d,
    0x45,0x4f,0x57,0x04,0x00,0x00,0x00,0x0f,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x04,
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x00,
    0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x02,
    0x00,0x00,0x00,0x23,0x00,0x05,0x00,0x00,
    0x00,0x57,0x69,0x6e,0x65,0x00,
};

static const BYTE expected_gc_moniker_saved_data[] =
{
    0x02,0x00,0x00,0x00,0x04,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x46,0x02,0x00,0x00,0x00,
    0x21,0x00,0x05,0x00,0x00,0x00,0x54,0x65,
    0x73,0x74,0x00,0x04,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x46,0x02,0x00,0x00,0x00,0x23,
    0x00,0x05,0x00,0x00,0x00,0x57,0x69,0x6e,
    0x65,0x00,
};

static const BYTE expected_gc_moniker_comparison_data[] =
{
    0x09,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     '!',0x00, 'T',0x00, 'E',0x00, 'S',0x00,
     'T',0x00,0x00,0x00,0x04,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x46, '#',0x00, 'W',0x00,
     'I',0x00, 'N',0x00, 'E',0x00,0x00,0x00,
};

static void test_moniker(
    const char *testname, IMoniker *moniker,
    const BYTE *expected_moniker_marshal_data, unsigned int sizeof_expected_moniker_marshal_data,
    const BYTE *expected_moniker_saved_data, unsigned int sizeof_expected_moniker_saved_data,
    const BYTE *expected_moniker_comparison_data, unsigned int sizeof_expected_moniker_comparison_data,
    int expected_max_size, LPCWSTR expected_display_name)
{
    ULARGE_INTEGER max_size;
    IStream * stream;
    IROTData * rotdata;
    HRESULT hr;
    HGLOBAL hglobal;
    LPBYTE moniker_data;
    DWORD moniker_size;
    DWORD i, moniker_type;
    BOOL same;
    BYTE buffer[128];
    IMoniker * moniker_proxy;

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "%s: IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", testname, hr);

    /* Display Name */
    TEST_DISPLAY_NAME(moniker, expected_display_name);

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "%s: IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", testname, hr);

    /* IROTData::GetComparisonData test */

    hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rotdata);
    ok_ole_success(hr, IMoniker_QueryInterface_IID_IROTData);

    hr = IROTData_GetComparisonData(rotdata, buffer, sizeof(buffer), &moniker_size);
    ok_ole_success(hr, IROTData_GetComparisonData);

    if (hr != S_OK) moniker_size = 0;

    /* first check we have the right amount of data */
    ok(moniker_size == sizeof_expected_moniker_comparison_data,
        "%s: Size of comparison data differs (expected %d, actual %d)\n",
        testname, sizeof_expected_moniker_comparison_data, moniker_size);

    /* then do a byte-by-byte comparison */
    same = TRUE;
    for (i = 0; i < min(moniker_size, sizeof_expected_moniker_comparison_data); i++)
    {
        if (expected_moniker_comparison_data[i] != buffer[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "%s: Comparison data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", buffer[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    IROTData_Release(rotdata);
  
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
  
    /* Saving */
    moniker_type = 0;
    IMoniker_IsSystemMoniker(moniker, &moniker_type);

    hr = IMoniker_GetSizeMax(moniker, &max_size);
    ok(hr == S_OK, "Failed to get max size, hr %#x.\n", hr);
todo_wine_if(moniker_type == MKSYS_GENERICCOMPOSITE)
    ok(expected_max_size == max_size.u.LowPart, "%s: unexpected max size %u.\n", testname, max_size.u.LowPart);

    hr = IMoniker_Save(moniker, stream, TRUE);
    ok_ole_success(hr, IMoniker_Save);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok(moniker_size == round_global_size(sizeof_expected_moniker_saved_data),
        "%s: Size of saved data differs (expected %d, actual %d)\n",
        testname, (DWORD)round_global_size(sizeof_expected_moniker_saved_data), moniker_size);

    /* then do a byte-by-byte comparison */
    same = TRUE;
    for (i = 0; i < min(moniker_size, round_global_size(sizeof_expected_moniker_saved_data)); i++)
    {
        if (expected_moniker_saved_data[i] != moniker_data[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "%s: Saved data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Release(stream);

    /* Marshaling tests */

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok(moniker_size == round_global_size(sizeof_expected_moniker_marshal_data),
        "%s: Size of marshaled data differs (expected %d, actual %d)\n",
        testname, (DWORD)round_global_size(sizeof_expected_moniker_marshal_data), moniker_size);

    /* then do a byte-by-byte comparison */
    same = TRUE;
    if (expected_moniker_marshal_data)
    {
        for (i = 0; i < min(moniker_size, round_global_size(sizeof_expected_moniker_marshal_data)); i++)
        {
            if (expected_moniker_marshal_data[i] != moniker_data[i])
            {
                same = FALSE;
                break;
            }
        }
    }

    ok(same, "%s: Marshaled data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker_proxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    IStream_Release(stream);
    IMoniker_Release(moniker_proxy);
}

static void test_class_moniker(void)
{
    IMoniker *moniker, *moniker2, *inverse, *reduced, *anti;
    IEnumMoniker *enummoniker;
    HRESULT hr;
    DWORD hash;
    IBindCtx *bindctx;
    IUnknown *unknown;
    FILETIME filetime;

    hr = CreateClassMoniker(&CLSID_StdComponentCategoriesMgr, &moniker);
    ok_ole_success(hr, CreateClassMoniker);

    hr = IMoniker_QueryInterface(moniker, &CLSID_ClassMoniker, (void **)&unknown);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unknown == (IUnknown *)moniker, "Unexpected interface.\n");
    IUnknown_Release(unknown);

    test_moniker("class moniker", moniker, 
        expected_class_moniker_marshal_data, sizeof(expected_class_moniker_marshal_data),
        expected_class_moniker_saved_data, sizeof(expected_class_moniker_saved_data),
        expected_class_moniker_comparison_data, sizeof(expected_class_moniker_comparison_data),
        sizeof(expected_class_moniker_saved_data), expected_class_moniker_display_name);

    /* Hashing */

    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);

    ok(hash == CLSID_StdComponentCategoriesMgr.Data1,
        "Hash value != Data1 field of clsid, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */
    TEST_MONIKER_TYPE(moniker, MKSYS_CLASSMONIKER);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    ok(hr == E_NOTIMPL, "IMoniker_IsRunning should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == E_NOTIMPL, "IMoniker_IsRunning should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == MK_E_UNAVAILABLE, "IMoniker_GetTimeOfLastChange should return MK_E_UNAVAILABLE, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToObject);
    IUnknown_Release(unknown);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToStorage);
    IUnknown_Release(unknown);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == S_OK, "Failed to get inverse, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(inverse, MKSYS_ANTIMONIKER);
    IMoniker_Release(inverse);

    /* Reduce() */
    hr = IMoniker_Reduce(moniker, NULL, MKRREDUCE_ALL, NULL, &reduced);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#x.\n", hr);
    ok(reduced == moniker, "Unexpected moniker.\n");
    IMoniker_Release(reduced);

    /* Enum() */
    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, TRUE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, FALSE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    hr = IMoniker_Enum(moniker, FALSE, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IBindCtx_Release(bindctx);

    /* ComposeWith() */

    /* C + A -> () */
    anti = create_antimoniker(1);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    ok(!moniker2, "Unexpected pointer.\n");
    IMoniker_Release(anti);

    /* C + A2 -> A */
    anti = create_antimoniker(2);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    ok(!moniker2, "Unexpected pointer.\n");
    IMoniker_Release(anti);

    IMoniker_Release(moniker);
}

static void test_file_moniker(WCHAR* path)
{
    IMoniker *moniker1 = NULL, *moniker2 = NULL, *inverse, *reduced, *anti;
    IEnumMoniker *enummoniker;
    IBindCtx *bind_ctx;
    IStream *stream;
    IUnknown *unk;
    DWORD hash;
    HRESULT hr;

    hr = CreateFileMoniker(path, &moniker1);
    ok_ole_success(hr, CreateFileMoniker); 

    hr = IMoniker_QueryInterface(moniker1, &CLSID_FileMoniker, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unk == (IUnknown *)moniker1, "Unexpected interface.\n");
    IUnknown_Release(unk);

    hr = IMoniker_Inverse(moniker1, &inverse);
    ok(hr == S_OK, "Failed to get inverse, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(inverse, MKSYS_ANTIMONIKER);
    IMoniker_Release(inverse);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    /* Marshal */
    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker1, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);
    
    /* Rewind */
    hr = IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, IStream_Seek);

    /* Unmarshal */
    hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void**)&moniker2);
    ok_ole_success(hr, CoUnmarshalInterface);

    hr = IMoniker_IsEqual(moniker1, moniker2);
    ok_ole_success(hr, IsEqual);

    /* Reduce() */
    hr = CreateBindCtx(0, &bind_ctx);
    ok(hr == S_OK, "Failed to create bind context, hr %#x.\n", hr);

    hr = IMoniker_Reduce(moniker1, NULL, MKRREDUCE_ALL, NULL, &reduced);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Reduce(moniker1, bind_ctx, MKRREDUCE_ALL, NULL, &reduced);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#x.\n", hr);
    ok(reduced == moniker1, "Unexpected moniker.\n");
    IMoniker_Release(reduced);

    /* Enum() */
    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker1, TRUE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker1, FALSE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    hr = IMoniker_Enum(moniker1, FALSE, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IBindCtx_Release(bind_ctx);

    IStream_Release(stream);
    IMoniker_Release(moniker2);

    /* ComposeWith() */

    /* F + A -> () */
    anti = create_antimoniker(1);
    hr = IMoniker_ComposeWith(moniker1, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    ok(!moniker2, "Unexpected pointer.\n");
    IMoniker_Release(anti);

    /* I + A2 -> (A) */
    anti = create_antimoniker(2);
    hr = IMoniker_ComposeWith(moniker1, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker2, MKSYS_ANTIMONIKER);
    hr = IMoniker_Hash(moniker2, &hash);
    ok(hr == S_OK, "Failed to get hash, hr %#x.\n", hr);
    ok(hash == 0x80000001, "Unexpected hash.\n");
    IMoniker_Release(moniker2);

    IMoniker_Release(anti);

    IMoniker_Release(moniker1);
}

static void test_file_monikers(void)
{
    static WCHAR wszFile[][30] = {
        {'\\', 'w','i','n','d','o','w','s','\\','s','y','s','t','e','m','\\','t','e','s','t','1','.','d','o','c',0},
        {'\\', 'a','b','c','d','e','f','g','\\','h','i','j','k','l','\\','m','n','o','p','q','r','s','t','u','.','m','n','o',0},
        /* These map to themselves in Windows-1252 & 932 (Shift-JIS) */
        {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0},
        /* U+2020 = DAGGER     = 0x86 (1252) = 0x813f (932)
         * U+20AC = EURO SIGN  = 0x80 (1252) =  undef (932)
         * U+0100 .. = Latin extended-A
         */ 
        {0x20ac, 0x2020, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, 0x109, 0x10a, 0x10b, 0x10c,  0},
        };

    int i; 

    trace("ACP is %u\n", GetACP());

    for (i = 0; i < ARRAY_SIZE(wszFile); ++i)
    {
        int j ;
        if (i == 2)
        {
            BOOL used;
            WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, wszFile[i], -1, NULL, 0, NULL, &used );
            if (used)
            {
                skip("string 2 doesn't round trip in codepage %u\n", GetACP() );
                continue;
            }
        }
        for (j = lstrlenW(wszFile[i]); j > 0; --j)
        {
            wszFile[i][j] = 0;
            test_file_moniker(wszFile[i]);
        }
    }
}

static void test_item_moniker(void)
{
    static const char item_moniker_unicode_delim_stream[] =
    {
        0x05, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, '!',
        0x00, 0x02, 0x00, 0x00, 0x00,  'A', 0x00,
    };
    static const char item_moniker_unicode_item_stream[] =
    {
        0x02, 0x00, 0x00, 0x00,  '!', 0x00, 0x05, 0x00,
        0x00, 0x00, 0xff, 0xff, 0x00,  'B', 0x00,
    };
    static const char item_moniker_unicode_delim_item_stream[] =
    {
        0x05, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00,  '!',
        0x00, 0x06, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x00,  'C', 0x00,
    };
    static struct
    {
        const char *data;
        int data_len;
        const WCHAR *display_name;
    }
    item_moniker_data[] =
    {
        { item_moniker_unicode_delim_stream, sizeof(item_moniker_unicode_delim_stream), L"!A" },
        { item_moniker_unicode_item_stream, sizeof(item_moniker_unicode_item_stream), L"!B" },
        { item_moniker_unicode_delim_item_stream, sizeof(item_moniker_unicode_delim_item_stream), L"!C" },
    };
    static const struct
    {
        const WCHAR *delim1;
        const WCHAR *item1;
        const WCHAR *delim2;
        const WCHAR *item2;
        HRESULT hr;
    } isequal_tests[] =
    {
        { L"!", L"Item1", L"!", L"ITEM1", S_OK },
        { NULL, L"Item1", L"!", L"ITEM1", S_OK },
        { L"", L"Item1", L"!", L"ITEM1", S_OK },
        { L"&", L"Item1", L"!", L"ITEM1", S_OK },
        {L"&&", L"Item1", L"&", L"&Item1", S_FALSE },
        { NULL, L"Item1", NULL, L"Item2", S_FALSE },
        { NULL, L"Item1", NULL, L"ITEM1", S_OK },
    };
    static const struct
    {
        const WCHAR *delim;
        const WCHAR *item;
        DWORD hash;
    } hash_tests[] =
    {
        { L"!", L"Test", 0x73c },
        { L"%", L"Test", 0x73c },
        { L"%", L"TEST", 0x73c },
        { L"%", L"T", 0x54 },
        { L"%", L"A", 0x41 },
        { L"%", L"a", 0x41 },
    };
    static const char *methods_isrunning[] =
    {
        "Moniker_IsRunning",
        NULL
    };
    IMoniker *moniker, *moniker2, *reduced, *anti, *inverse;
    DWORD i, hash, eaten, cookie;
    HRESULT hr;
    IBindCtx *bindctx;
    IUnknown *unknown;
    static const WCHAR wszDelimiter[] = {'!',0};
    static const WCHAR wszObjectName[] = {'T','e','s','t',0};
    static const WCHAR expected_display_name[] = { '!','T','e','s','t',0 };
    struct test_moniker *container_moniker;
    WCHAR displayname[16] = L"display name";
    IEnumMoniker *enummoniker;
    IRunningObjectTable *rot;
    BIND_OPTS bind_opts;
    LARGE_INTEGER pos;
    IStream *stream;

    hr = CreateItemMoniker(NULL, wszObjectName, &moniker);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    hr = IMoniker_QueryInterface(moniker, &CLSID_ItemMoniker, (void **)&unknown);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unknown == (IUnknown *)moniker, "Unexpected interface.\n");
    IUnknown_Release(unknown);

    test_moniker("item moniker 2", moniker,
        expected_item_moniker_marshal_data2, sizeof(expected_item_moniker_marshal_data2),
        expected_item_moniker_saved_data2, sizeof(expected_item_moniker_saved_data2),
        expected_item_moniker_comparison_data2, sizeof(expected_item_moniker_comparison_data2),
        46, L"Test");

    IMoniker_Release(moniker);

    hr = CreateItemMoniker(L"", wszObjectName, &moniker);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    test_moniker("item moniker 3", moniker,
        expected_item_moniker_marshal_data3, sizeof(expected_item_moniker_marshal_data3),
        expected_item_moniker_saved_data3, sizeof(expected_item_moniker_saved_data3),
        expected_item_moniker_comparison_data2, sizeof(expected_item_moniker_comparison_data2),
        50, L"Test");

    IMoniker_Release(moniker);

    hr = CreateItemMoniker(L"&&", wszObjectName, &moniker);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    test_moniker("item moniker 4", moniker,
        expected_item_moniker_marshal_data4, sizeof(expected_item_moniker_marshal_data4),
        expected_item_moniker_saved_data4, sizeof(expected_item_moniker_saved_data4),
        expected_item_moniker_comparison_data4, sizeof(expected_item_moniker_comparison_data4),
        58, L"&&Test");

    IMoniker_Release(moniker);

    hr = CreateItemMoniker(L"ab", wszObjectName, &moniker);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    test_moniker("item moniker 5", moniker,
        expected_item_moniker_marshal_data5, sizeof(expected_item_moniker_marshal_data5),
        expected_item_moniker_saved_data5, sizeof(expected_item_moniker_saved_data5),
        expected_item_moniker_comparison_data5, sizeof(expected_item_moniker_comparison_data5),
        58, L"abTest");

    /* Serialize and load back. */
    hr = CreateItemMoniker(NULL, L"object", &moniker2);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#x.\n", hr);

    hr = CreateBindCtx(0, &bindctx);
    ok(hr == S_OK, "Failed to create bind context, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(item_moniker_data); ++i)
    {
        pos.QuadPart = 0;
        hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "Failed to seek stream, hr %#x.\n", hr);

        hr = IStream_Write(stream, item_moniker_data[i].data, item_moniker_data[i].data_len, NULL);
        ok(hr == S_OK, "Failed to write stream contents, hr %#x.\n", hr);

        pos.QuadPart = 0;
        hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "Failed to seek stream, hr %#x.\n", hr);

        hr = IMoniker_Load(moniker2, stream);
        ok(hr == S_OK, "Failed to load moniker, hr %#x.\n", hr);

        TEST_DISPLAY_NAME(moniker2, item_moniker_data[i].display_name);
    }

    IStream_Release(stream);

    IMoniker_Release(moniker2);
    IMoniker_Release(moniker);

    /* Hashing */

    for (i = 0; i < ARRAY_SIZE(hash_tests); ++i)
    {
        hr = CreateItemMoniker(hash_tests[i].delim, hash_tests[i].item, &moniker);
        ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

        hr = IMoniker_Hash(moniker, &hash);
        ok(hr == S_OK, "Failed to get hash value, hr %#x.\n", hr);
        ok(hash == hash_tests[i].hash, "%d: unexpected hash value %#x.\n", i, hash);

        IMoniker_Release(moniker);
    }

    hr = CreateItemMoniker(wszDelimiter, wszObjectName, &moniker);
    ok_ole_success(hr, CreateItemMoniker);

    test_moniker("item moniker 1", moniker,
        expected_item_moniker_marshal_data, sizeof(expected_item_moniker_marshal_data),
        expected_item_moniker_saved_data, sizeof(expected_item_moniker_saved_data),
        expected_item_moniker_comparison_data, sizeof(expected_item_moniker_comparison_data),
        54, expected_display_name);

    /* IsSystemMoniker test */
    TEST_MONIKER_TYPE(moniker, MKSYS_ITEMMONIKER);

    container_moniker = create_test_moniker();

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "IMoniker_IsRunning should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, NULL, &container_moniker->IMoniker_iface, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_FALSE, "IMoniker_IsRunning should return S_FALSE, not 0x%08x\n", hr);

    hr = CreateItemMoniker(wszDelimiter, wszObjectName, &moniker2);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);
    hr = IMoniker_IsRunning(moniker, bindctx, NULL, moniker2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    IMoniker_Release(moniker2);

    /* Different moniker as newly running. */
    hr = CreateItemMoniker(wszDelimiter, L"Item123", &moniker2);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, moniker2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IBindCtx_GetRunningObjectTable(bindctx, &rot);
    ok(hr == S_OK, "Failed to get ROT, hr %#x.\n", hr);

    hr = IRunningObjectTable_Register(rot, ROTFLAGS_REGISTRATIONKEEPSALIVE, (IUnknown *)moniker, moniker, &cookie);
    ok(hr == S_OK, "Failed to register, hr %#x.\n", hr);

    hr = IRunningObjectTable_IsRunning(rot, moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, moniker2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IRunningObjectTable_Revoke(rot, cookie);
    ok(hr == S_OK, "Failed to revoke registration, hr %#x.\n", hr);

    IRunningObjectTable_Release(rot);

    expected_method_list = methods_isrunning;
    hr = IMoniker_IsRunning(moniker, bindctx, &container_moniker->IMoniker_iface, NULL);
    ok(hr == 0x8beef000, "Unexpected hr %#x.\n", hr);

    expected_method_list = methods_isrunning;
    hr = IMoniker_IsRunning(moniker, bindctx, &container_moniker->IMoniker_iface, moniker2);
    ok(hr == 0x8beef000, "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker2);

    /* BindToObject() */
    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "IMoniker_BindToStorage should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, &container_moniker->IMoniker_iface, &IID_IUnknown, (void **)&unknown);
    ok(hr == (0x8bee0000 | BINDSPEED_INDEFINITE), "Unexpected hr %#x.\n", hr);

    bind_opts.cbStruct = sizeof(bind_opts);
    hr = IBindCtx_GetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to get bind options, hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 1;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_BindToObject(moniker, bindctx, &container_moniker->IMoniker_iface, &IID_IUnknown, (void **)&unknown);
    ok(hr == (0x8bee0000 | BINDSPEED_IMMEDIATE), "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 2499;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_BindToObject(moniker, bindctx, &container_moniker->IMoniker_iface, &IID_IUnknown, (void **)&unknown);
    ok(hr == (0x8bee0000 | BINDSPEED_IMMEDIATE), "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 2500;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_BindToObject(moniker, bindctx, &container_moniker->IMoniker_iface, &IID_IUnknown, (void **)&unknown);
    ok(hr == (0x8bee0000 | BINDSPEED_MODERATE), "Unexpected hr %#x.\n", hr);

    /* BindToStorage() */
    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, &container_moniker->IMoniker_iface, &IID_IUnknown, (void **)&unknown);
    ok(hr == 0x8bee0001, "Unexpected hr %#x.\n", hr);

    /* ParseDisplayName() */
    hr = IMoniker_ParseDisplayName(moniker, bindctx, NULL, displayname, &eaten, &moniker2);
    ok(hr == MK_E_SYNTAX, "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 0;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_ParseDisplayName(moniker, bindctx, &container_moniker->IMoniker_iface, displayname, &eaten, &moniker2);
    ok(hr == (0x8bee0000 | BINDSPEED_INDEFINITE), "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 1;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_ParseDisplayName(moniker, bindctx, &container_moniker->IMoniker_iface, displayname, &eaten, &moniker2);
    ok(hr == (0x8bee0000 | BINDSPEED_IMMEDIATE), "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 2499;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_ParseDisplayName(moniker, bindctx, &container_moniker->IMoniker_iface, displayname, &eaten, &moniker2);
    ok(hr == (0x8bee0000 | BINDSPEED_IMMEDIATE), "Unexpected hr %#x.\n", hr);

    bind_opts.dwTickCountDeadline = 2500;
    hr = IBindCtx_SetBindOptions(bindctx, &bind_opts);
    ok(hr == S_OK, "Failed to set bind options, hr %#x.\n", hr);
    hr = IMoniker_ParseDisplayName(moniker, bindctx, &container_moniker->IMoniker_iface, displayname, &eaten, &moniker2);
    ok(hr == (0x8bee0000 | BINDSPEED_MODERATE), "Unexpected hr %#x.\n", hr);

    IMoniker_Release(&container_moniker->IMoniker_iface);

    IBindCtx_Release(bindctx);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == S_OK, "Failed to get inverse, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(inverse, MKSYS_ANTIMONIKER);
    IMoniker_Release(inverse);

    /* Reduce() */
    hr = IMoniker_Reduce(moniker, NULL, MKRREDUCE_ALL, NULL, &reduced);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#x.\n", hr);
    ok(reduced == moniker, "Unexpected moniker.\n");
    IMoniker_Release(reduced);

    /* Enum() */
    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, TRUE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, FALSE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    hr = IMoniker_Enum(moniker, FALSE, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker);

    /* IsEqual */
    for (i = 0; i < ARRAY_SIZE(isequal_tests); ++i)
    {
        hr = CreateItemMoniker(isequal_tests[i].delim1, isequal_tests[i].item1, &moniker);
        ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

        hr = CreateItemMoniker(isequal_tests[i].delim2, isequal_tests[i].item2, &moniker2);
        ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

        hr = IMoniker_IsEqual(moniker, moniker2);
        ok(hr == isequal_tests[i].hr, "%d: unexpected result %#x.\n", i, hr);

        hr = IMoniker_IsEqual(moniker2, moniker);
        ok(hr == isequal_tests[i].hr, "%d: unexpected result %#x.\n", i, hr);

        IMoniker_Release(moniker);
        IMoniker_Release(moniker2);
    }

    /* Default instance. */
    hr = CoCreateInstance(&CLSID_ItemMoniker, NULL, CLSCTX_SERVER, &IID_IMoniker, (void **)&moniker);
    ok(hr == S_OK, "Failed to create item moniker, hr %#x.\n", hr);

    test_moniker("item moniker 6", moniker,
        expected_item_moniker_marshal_data6, sizeof(expected_item_moniker_marshal_data6),
        expected_item_moniker_saved_data6, sizeof(expected_item_moniker_saved_data6),
        expected_item_moniker_comparison_data6, sizeof(expected_item_moniker_comparison_data6),
        34, L"");

    hr = CoCreateInstance(&CLSID_ItemMoniker, (IUnknown *)moniker, CLSCTX_SERVER, &IID_IMoniker, (void **)&moniker2);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker);

    /* ComposeWith() */
    hr = CreateItemMoniker(L"!", L"Item", &moniker);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    /* I + A -> () */
    anti = create_antimoniker(1);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    ok(!moniker2, "Unexpected pointer.\n");
    IMoniker_Release(anti);

    /* I + A2 -> (A) */
    anti = create_antimoniker(2);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker2, MKSYS_ANTIMONIKER);
    hr = IMoniker_Hash(moniker2, &hash);
    ok(hr == S_OK, "Failed to get hash, hr %#x.\n", hr);
    ok(hash == 0x80000001, "Unexpected hash.\n");
    IMoniker_Release(moniker2);

    IMoniker_Release(anti);

    IMoniker_Release(moniker);
}

static void stream_write_dword(IStream *stream, DWORD value)
{
    LARGE_INTEGER pos;
    HRESULT hr;

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#x.\n", hr);

    hr = IStream_Write(stream, &value, sizeof(value), NULL);
    ok(hr == S_OK, "Stream write failed, hr %#x.\n", hr);

    hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#x.\n", hr);
}

static void test_anti_moniker(void)
{
    IMoniker *moniker, *moniker2, *moniker3, *inverse, *reduced;
    HRESULT hr;
    DWORD hash;
    IBindCtx *bindctx;
    FILETIME filetime;
    IUnknown *unknown;
    static const WCHAR expected_display_name[] = { '\\','.','.',0 };
    IEnumMoniker *enummoniker;
    IStream *stream;

    hr = CreateAntiMoniker(&moniker);
    ok_ole_success(hr, CreateAntiMoniker);

    hr = IMoniker_QueryInterface(moniker, &CLSID_AntiMoniker, (void **)&unknown);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(unknown == (IUnknown *)moniker, "Unexpected interface.\n");
    IUnknown_Release(unknown);

    test_moniker("anti moniker", moniker, 
        expected_anti_moniker_marshal_data, sizeof(expected_anti_moniker_marshal_data),
        expected_anti_moniker_saved_data, sizeof(expected_anti_moniker_saved_data),
        expected_anti_moniker_comparison_data, sizeof(expected_anti_moniker_comparison_data),
        20, expected_display_name);

    /* Hashing */
    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);
    ok(hash == 0x80000001,
        "Hash value != 0x80000001, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */
    TEST_MONIKER_TYPE(moniker, MKSYS_ANTIMONIKER);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == MK_E_NOINVERSE, "IMoniker_Inverse should have returned MK_E_NOINVERSE instead of 0x%08x\n", hr);
    ok(inverse == NULL, "inverse should have been set to NULL instead of %p\n", inverse);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_FALSE, "IMoniker_IsRunning should return S_FALSE, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == E_NOTIMPL, "IMoniker_GetTimeOfLastChange should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_NOTIMPL, "IMoniker_BindToObject should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_NOTIMPL, "IMoniker_BindToStorage should return E_NOTIMPL, not 0x%08x\n", hr);

    /* ComposeWith */
    hr = CreateAntiMoniker(&moniker2);
    ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

    moniker3 = moniker;
    hr = IMoniker_ComposeWith(moniker, moniker2, TRUE, &moniker3);
    ok(hr == MK_E_NEEDGENERIC, "Unexpected hr %#x.\n", hr);
    ok(!moniker3, "Unexpected interface.\n");

    hr = IMoniker_ComposeWith(moniker, moniker2, FALSE, &moniker3);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker3, MKSYS_GENERICCOMPOSITE);
    IMoniker_Release(moniker3);

    IMoniker_Release(moniker2);

    /* Load with composed number > 1. */
    hr = CreateAntiMoniker(&moniker2);
    ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#x.\n", hr);

    stream_write_dword(stream, 2);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    test_moniker("anti moniker 2", moniker,
        expected_anti_moniker_marshal_data2, sizeof(expected_anti_moniker_marshal_data2),
        expected_anti_moniker_saved_data2, sizeof(expected_anti_moniker_saved_data2),
        expected_anti_moniker_comparison_data2, sizeof(expected_anti_moniker_comparison_data2),
        20, L"\\..\\..");

    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker2, moniker);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Hash(moniker, &hash);
    ok(hr == S_OK, "Failed to get hash value, hr %#x.\n", hr);
    ok(hash == 0x80000002, "Unexpected hash value %#x.\n", hash);

    /* Display name reflects anti combination. */
    TEST_DISPLAY_NAME(moniker, L"\\..\\..");

    /* Limit is at 0xfffff. */
    stream_write_dword(stream, 0xfffff);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Hash(moniker, &hash);
    ok(hr == S_OK, "Failed to get hash value, hr %#x.\n", hr);
    ok(hash == 0x800fffff, "Unexpected hash value %#x.\n", hash);

    stream_write_dword(stream, 0xfffff + 1);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Hash(moniker, &hash);
    ok(hr == S_OK, "Failed to get hash value, hr %#x.\n", hr);
    ok(hash == 0x800fffff, "Unexpected hash value %#x.\n", hash);

    /* Zero combining counter is also valid. */
    stream_write_dword(stream, 0);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker2, moniker);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Hash(moniker, &hash);
    ok(hr == S_OK, "Failed to get hash value, hr %#x.\n", hr);
    ok(hash == 0x80000000, "Unexpected hash value %#x.\n", hash);

    TEST_DISPLAY_NAME(moniker, L"");

    /* Back to initial value. */
    stream_write_dword(stream, 1);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker2, moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Reduce() */
    hr = IMoniker_Reduce(moniker, NULL, MKRREDUCE_ALL, NULL, &reduced);
    ok(hr == MK_S_REDUCED_TO_SELF, "Unexpected hr %#x.\n", hr);
    ok(reduced == moniker, "Unexpected moniker.\n");
    IMoniker_Release(reduced);

    /* Enum() */
    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, TRUE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    enummoniker = (void *)0xdeadbeef;
    hr = IMoniker_Enum(moniker, FALSE, &enummoniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!enummoniker, "Unexpected pointer.\n");

    hr = IMoniker_Enum(moniker, FALSE, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* CommonPrefixWith() */
    stream_write_dword(stream, 0);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &moniker3);
    ok(hr == MK_S_ME, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    hr = IMoniker_CommonPrefixWith(moniker2, moniker, &moniker3);
    ok(hr == MK_S_HIM, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    stream_write_dword(stream, 10);

    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    stream_write_dword(stream, 5);

    hr = IMoniker_Load(moniker2, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &moniker3);
    ok(hr == MK_S_HIM, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker2, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    hr = IMoniker_CommonPrefixWith(moniker2, moniker, &moniker3);
    ok(hr == MK_S_ME, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker2, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    /* Now same length, 0 or 2 */
    stream_write_dword(stream, 0);
    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    stream_write_dword(stream, 0);
    hr = IMoniker_Load(moniker2, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &moniker3);
    ok(hr == MK_S_US, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    stream_write_dword(stream, 2);
    hr = IMoniker_Load(moniker, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    stream_write_dword(stream, 2);
    hr = IMoniker_Load(moniker2, stream);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &moniker3);
    ok(hr == MK_S_US, "Unexpected hr %#x.\n", hr);
    ok(moniker3 == moniker, "Unexpected prefix moniker.\n");
    IMoniker_Release(moniker3);

    IStream_Release(stream);
    IBindCtx_Release(bindctx);
    IMoniker_Release(moniker);
    IMoniker_Release(moniker2);
}

static void test_generic_composite_moniker(void)
{
    IMoniker *moniker, *inverse, *inverse2, *moniker1, *moniker2, *moniker3, *moniker4;
    IEnumMoniker *enummoniker;
    HRESULT hr;
    DWORD hash;
    IBindCtx *bindctx;
    FILETIME filetime;
    IUnknown *unknown;

    hr = CreateBindCtx(0, &bindctx);
    ok(hr == S_OK, "Failed to create bind context, hr %#x.\n", hr);

    hr = CreateItemMoniker(L"!", L"Test", &moniker1);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);
    hr = CreateItemMoniker(L"#", L"Wine", &moniker2);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);
    hr = CreateGenericComposite(moniker1, moniker2, &moniker);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    /* Compose with itself. */
    EXPECT_REF(moniker1, 2);
    hr = CreateGenericComposite(moniker1, moniker1, &moniker);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    EXPECT_REF(moniker1, 4);
    TEST_MONIKER_TYPE(moniker, MKSYS_GENERICCOMPOSITE);
    IMoniker_Release(moniker);

    /* (I) + (A) -> () */
    hr = IMoniker_Inverse(moniker1, &inverse);
    ok(hr == S_OK, "Failed to invert, hr %#x.\n", hr);
    hr = CreateGenericComposite(moniker1, inverse, &moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
todo_wine
    ok(!moniker, "Unexpected pointer.\n");
    if (moniker)
        IMoniker_Release(moniker);

    /* (I1,I2) + (A,A) -> (I1,I2+A,A) -> (I1,A) -> () */
    hr = CreateGenericComposite(moniker1, moniker2, &moniker);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    hr = CreateGenericComposite(inverse, inverse, &moniker3);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker3, MKSYS_GENERICCOMPOSITE);

    hr = CreateGenericComposite(moniker, moniker3, &moniker4);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
todo_wine
    ok(!moniker4, "Unexpected pointer.\n");
    if (moniker4)
        IMoniker_Release(moniker4);

    IMoniker_Release(moniker);
    IMoniker_Release(moniker3);

    /* (I1,I2) + (A2,A) -> (I1,I2+A2,A) -> (I1,A,A) -> (I1+A,A) -> (A) */
    hr = CreateGenericComposite(moniker1, moniker2, &moniker);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    ok(!!moniker, "Unexpected pointer.\n");
    inverse2 = create_antimoniker(2);
    hr = CreateGenericComposite(inverse2, inverse, &moniker3);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    ok(!!moniker3, "Unexpected pointer.\n");
    IMoniker_Release(inverse2);

    hr = CreateGenericComposite(moniker, moniker3, &moniker4);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    TEST_MONIKER_TYPE_TODO(moniker4, MKSYS_ANTIMONIKER);
    IMoniker_Release(moniker4);
    IMoniker_Release(moniker);
    IMoniker_Release(moniker3);

    /* (I1,I2) + (A,I2) -> (I1,I2+A,I2) -> (I1,I2) */
    hr = CreateGenericComposite(moniker1, moniker2, &moniker);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    ok(!!moniker, "Unexpected pointer.\n");

    hr = CreateGenericComposite(inverse, moniker2, &moniker3);
    ok(hr == S_OK, "Failed to create composite, hr %#x.\n", hr);
    ok(!!moniker3, "Unexpected pointer.\n");

    hr = CreateGenericComposite(moniker, moniker3, &moniker4);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker4, MKSYS_GENERICCOMPOSITE);

    TEST_DISPLAY_NAME(moniker4, L"!Test#Wine");

    IMoniker_Release(moniker4);
    IMoniker_Release(moniker3);

    IMoniker_Release(inverse);

    /* Generic composite is special, as it does not addref in this case. */
    hr = IMoniker_QueryInterface(moniker, &CLSID_CompositeMoniker, (void **)&unknown);
todo_wine
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ok(unknown == (IUnknown *)moniker, "Unexpected interface.\n");

    test_moniker("generic composite moniker", moniker, 
        expected_gc_moniker_marshal_data, sizeof(expected_gc_moniker_marshal_data),
        expected_gc_moniker_saved_data, sizeof(expected_gc_moniker_saved_data),
        expected_gc_moniker_comparison_data, sizeof(expected_gc_moniker_comparison_data),
        160, L"!Test#Wine");

    /* Hashing */

    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);

    ok(hash == 0xd87,
        "Hash value != 0xd87, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */
    TEST_MONIKER_TYPE(moniker, MKSYS_GENERICCOMPOSITE);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "IMoniker_IsRunning should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == MK_E_NOTBINDABLE, "IMoniker_GetTimeOfLastChange should return MK_E_NOTBINDABLE, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    todo_wine
    ok(hr == E_INVALIDARG, "IMoniker_BindToObject should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "IMoniker_BindToStorage should return E_INVALIDARG, not 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == S_OK, "Failed to get inverse, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(inverse, MKSYS_GENERICCOMPOSITE);
    IMoniker_Release(inverse);

    /* Enum() */
    hr = IMoniker_Enum(moniker, TRUE, &enummoniker);
    ok(hr == S_OK, "Failed to get enumerator, hr %#x.\n", hr);
    IEnumMoniker_Release(enummoniker);

    hr = IMoniker_Enum(moniker, FALSE, &enummoniker);
    ok(hr == S_OK, "Failed to get enumerator, hr %#x.\n", hr);
    IEnumMoniker_Release(enummoniker);

    hr = IMoniker_Enum(moniker, FALSE, NULL);
todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker);
}

static void test_pointer_moniker(void)
{
    IMoniker *moniker, *moniker2, *prefix, *inverse, *anti;
    IEnumMoniker *enummoniker;
    DWORD hash, size;
    HRESULT hr;
    IBindCtx *bindctx;
    FILETIME filetime;
    IUnknown *unknown;
    IStream *stream;
    IROTData *rotdata;
    LPOLESTR display_name;
    IMarshal *marshal;
    LARGE_INTEGER pos;
    CLSID clsid;

    cLocks = 0;

    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, NULL);
    ok(hr == E_INVALIDARG, "CreatePointerMoniker(x, NULL) should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, &moniker);
    ok_ole_success(hr, CreatePointerMoniker);

    hr = IMoniker_QueryInterface(moniker, &IID_IMoniker, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_QueryInterface(moniker, &CLSID_PointerMoniker, (void **)&unknown);
    ok(unknown == (IUnknown *)moniker, "Unexpected interface.\n");
    IUnknown_Release(unknown);

    hr = IMoniker_QueryInterface(moniker, &IID_IMarshal, (void **)&marshal);
    ok(hr == S_OK, "Failed to get interface, hr %#x.\n", hr);

    hr = IMarshal_GetUnmarshalClass(marshal, NULL, NULL, 0, NULL, 0, &clsid);
    ok(hr == S_OK, "Failed to get class, hr %#x.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_PointerMoniker), "Unexpected class.\n");

    hr = IMarshal_GetMarshalSizeMax(marshal, &IID_IMoniker, NULL, CLSCTX_INPROC, NULL, 0, &size);
    ok(hr == S_OK, "Failed to get marshal size, hr %#x.\n", hr);
    ok(size > 0, "Unexpected size %d.\n", size);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#x.\n", hr);

    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "Failed to marshal moniker, hr %#x.\n", hr);

    pos.QuadPart = 0;
    IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker2);
    ok(hr == S_OK, "Failed to unmarshal, hr %#x.\n", hr);
    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_OK, "Expected equal moniker, hr %#x.\n", hr);
    IMoniker_Release(moniker2);

    IStream_Release(stream);

    IMarshal_Release(marshal);

    ok_more_than_one_lock();

    /* Display Name */

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    hr = IMoniker_GetDisplayName(moniker, bindctx, NULL, &display_name);
    ok(hr == E_NOTIMPL, "IMoniker_GetDisplayName should have returned E_NOTIMPL instead of 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", hr);

    /* IROTData::GetComparisonData test */

    hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rotdata);
    ok(hr == E_NOINTERFACE, "IMoniker_QueryInterface(IID_IROTData) should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    /* Saving */

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = IMoniker_Save(moniker, stream, TRUE);
    ok(hr == E_NOTIMPL, "IMoniker_Save should have returned E_NOTIMPL instead of 0x%08x\n", hr);

    IStream_Release(stream);

    /* Hashing */
    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);
    ok(hash == PtrToUlong(&Test_ClassFactory),
        "Hash value should have been 0x%08x, instead of 0x%08x\n",
        PtrToUlong(&Test_ClassFactory), hash);

    /* IsSystemMoniker test */
    TEST_MONIKER_TYPE(moniker, MKSYS_POINTERMONIKER);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == S_OK, "Failed to get inverse, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(inverse, MKSYS_ANTIMONIKER);
    IMoniker_Release(inverse);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_OK, "IMoniker_IsRunning should return S_OK, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == E_NOTIMPL, "IMoniker_GetTimeOfLastChange should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToObject);
    IUnknown_Release(unknown);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToStorage);
    IUnknown_Release(unknown);

    IMoniker_Release(moniker);

todo_wine
    ok(cLocks == 0, "Number of locks should be 0, but actually is %d.\n", cLocks);

    hr = CreatePointerMoniker(NULL, &moniker);
    ok_ole_success(hr, CreatePointerMoniker);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_UNEXPECTED, "IMoniker_BindToObject should have returned E_UNEXPECTED instead of 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_UNEXPECTED, "IMoniker_BindToStorage should have returned E_UNEXPECTED instead of 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    /* Enum() */
    hr = IMoniker_Enum(moniker, TRUE, &enummoniker);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_Enum(moniker, FALSE, &enummoniker);
    ok(hr == E_NOTIMPL, "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker);

    /* CommonPrefixWith() */
    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, &moniker);
    ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, &moniker2);
    ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, NULL, &prefix);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &prefix);
    ok(hr == MK_S_US, "Unexpected hr %#x.\n", hr);
    ok(prefix == moniker, "Unexpected pointer.\n");
    IMoniker_Release(prefix);

    IMoniker_Release(moniker2);

    hr = CreatePointerMoniker((IUnknown *)moniker, &moniker2);
    ok(hr == S_OK, "Failed to create moniker, hr %#x.\n", hr);

    hr = IMoniker_IsEqual(moniker, moniker2);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IMoniker_CommonPrefixWith(moniker, moniker2, &prefix);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    IMoniker_Release(moniker2);

    /* ComposeWith() */

    /* P + A -> () */
    anti = create_antimoniker(1);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    ok(!moniker2, "Unexpected pointer.\n");
    IMoniker_Release(anti);

    /* P + A2 -> A */
    anti = create_antimoniker(2);
    hr = IMoniker_ComposeWith(moniker, anti, TRUE, &moniker2);
    ok(hr == S_OK, "Failed to compose, hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker2, MKSYS_ANTIMONIKER);
    IMoniker_Release(moniker2);

    IMoniker_Release(anti);

    IMoniker_Release(moniker);
}

static void test_bind_context(void)
{
    HRESULT hr;
    IBindCtx *pBindCtx;
    IEnumString *pEnumString;
    BIND_OPTS3 bind_opts;
    HeapUnknown *unknown;
    HeapUnknown *unknown2;
    IUnknown *param_obj;
    ULONG refs;
    static const WCHAR wszParamName[] = {'G','e','m','m','a',0};
    static const WCHAR wszNonExistent[] = {'N','o','n','E','x','i','s','t','e','n','t',0};

    hr = CreateBindCtx(0, NULL);
    ok(hr == E_INVALIDARG, "CreateBindCtx with NULL ppbc should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateBindCtx(0xdeadbeef, &pBindCtx);
    ok(hr == E_INVALIDARG, "CreateBindCtx with reserved value non-zero should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateBindCtx(0, &pBindCtx);
    ok_ole_success(hr, "CreateBindCtx");

    bind_opts.cbStruct = -1;
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(BIND_OPTS3) || broken(bind_opts.cbStruct == sizeof(BIND_OPTS2)) /* XP */,
        "Unexpected bind_opts.cbStruct %d.\n", bind_opts.cbStruct);

    bind_opts.cbStruct = sizeof(BIND_OPTS);
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(BIND_OPTS), "bind_opts.cbStruct was %d\n", bind_opts.cbStruct);

    memset(&bind_opts, 0xfe, sizeof(bind_opts));
    bind_opts.cbStruct = sizeof(bind_opts);
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(bind_opts) || bind_opts.cbStruct == sizeof(BIND_OPTS2) /* XP */,
        "Unexpected bind_opts.cbStruct %d.\n", bind_opts.cbStruct);
    ok(bind_opts.grfFlags == 0, "bind_opts.grfFlags was 0x%x instead of 0\n", bind_opts.grfFlags);
    ok(bind_opts.grfMode == STGM_READWRITE, "bind_opts.grfMode was 0x%x instead of STGM_READWRITE\n", bind_opts.grfMode);
    ok(bind_opts.dwTickCountDeadline == 0, "bind_opts.dwTickCountDeadline was %d instead of 0\n", bind_opts.dwTickCountDeadline);
    ok(bind_opts.dwTrackFlags == 0, "bind_opts.dwTrackFlags was 0x%x instead of 0\n", bind_opts.dwTrackFlags);
    ok(bind_opts.dwClassContext == (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER),
        "bind_opts.dwClassContext should have been 0x15 instead of 0x%x\n", bind_opts.dwClassContext);
    ok(bind_opts.locale == GetThreadLocale(), "bind_opts.locale should have been 0x%x instead of 0x%x\n", GetThreadLocale(), bind_opts.locale);
    ok(bind_opts.pServerInfo == NULL, "bind_opts.pServerInfo should have been NULL instead of %p\n", bind_opts.pServerInfo);
    if (bind_opts.cbStruct >= sizeof(BIND_OPTS3))
        ok(bind_opts.hwnd == NULL, "Unexpected bind_opts.hwnd %p.\n", bind_opts.hwnd);

    bind_opts.cbStruct = -1;
    hr = IBindCtx_SetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok(hr == E_INVALIDARG, "IBindCtx_SetBindOptions with bad cbStruct should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = IBindCtx_RegisterObjectParam(pBindCtx, (WCHAR *)wszParamName, NULL);
    ok(hr == E_INVALIDARG, "IBindCtx_RegisterObjectParam should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*unknown));
    unknown->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
    unknown->refs = 1;
    hr = IBindCtx_RegisterObjectParam(pBindCtx, (WCHAR *)wszParamName, &unknown->IUnknown_iface);
    ok_ole_success(hr, "IBindCtx_RegisterObjectParam");

    hr = IBindCtx_GetObjectParam(pBindCtx, (WCHAR *)wszParamName, &param_obj);
    ok_ole_success(hr, "IBindCtx_GetObjectParam");
    IUnknown_Release(param_obj);

    hr = IBindCtx_GetObjectParam(pBindCtx, (WCHAR *)wszNonExistent, &param_obj);
    ok(hr == E_FAIL, "IBindCtx_GetObjectParam with nonexistent key should have failed with E_FAIL instead of 0x%08x\n", hr);
    ok(param_obj == NULL, "IBindCtx_GetObjectParam with nonexistent key should have set output parameter to NULL instead of %p\n", param_obj);

    hr = IBindCtx_RevokeObjectParam(pBindCtx, (WCHAR *)wszNonExistent);
    ok(hr == E_FAIL, "IBindCtx_RevokeObjectParam with nonexistent key should have failed with E_FAIL instead of 0x%08x\n", hr);

    hr = IBindCtx_EnumObjectParam(pBindCtx, &pEnumString);
    ok(hr == E_NOTIMPL, "IBindCtx_EnumObjectParam should have returned E_NOTIMPL instead of 0x%08x\n", hr);
    ok(!pEnumString, "pEnumString should be NULL\n");

    hr = IBindCtx_RegisterObjectBound(pBindCtx, NULL);
    ok_ole_success(hr, "IBindCtx_RegisterObjectBound(NULL)");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, NULL);
    ok(hr == E_INVALIDARG, "IBindCtx_RevokeObjectBound(NULL) should have return E_INVALIDARG instead of 0x%08x\n", hr);

    unknown2 = HeapAlloc(GetProcessHeap(), 0, sizeof(*unknown));
    unknown2->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
    unknown2->refs = 1;
    hr = IBindCtx_RegisterObjectBound(pBindCtx, &unknown2->IUnknown_iface);
    ok_ole_success(hr, "IBindCtx_RegisterObjectBound");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, &unknown2->IUnknown_iface);
    ok_ole_success(hr, "IBindCtx_RevokeObjectBound");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, &unknown2->IUnknown_iface);
    ok(hr == MK_E_NOTBOUND, "IBindCtx_RevokeObjectBound with not bound object should have returned MK_E_NOTBOUND instead of 0x%08x\n", hr);

    IBindCtx_Release(pBindCtx);

    refs = IUnknown_Release(&unknown->IUnknown_iface);
    ok(!refs, "object param should have been destroyed, instead of having %d refs\n", refs);

    refs = IUnknown_Release(&unknown2->IUnknown_iface);
    ok(!refs, "bound object should have been destroyed, instead of having %d refs\n", refs);
}

static void test_save_load_filemoniker(void)
{
    IMoniker* pMk;
    IStream* pStm;
    HRESULT hr;
    ULARGE_INTEGER size;
    LARGE_INTEGER zero_pos, dead_pos, nulls_pos;
    DWORD some_val = 0xFEDCBA98;
    int i;

    /* see FileMonikerImpl_Save docs */
    zero_pos.QuadPart = 0;
    dead_pos.QuadPart = sizeof(WORD) + sizeof(DWORD) + (lstrlenW(wszFileName1) + 1) + sizeof(WORD);
    nulls_pos.QuadPart = dead_pos.QuadPart + sizeof(WORD);

    /* create the stream we're going to write to */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    size.u.LowPart = 128;
    hr = IStream_SetSize(pStm, size);
    ok_ole_success(hr, "IStream_SetSize");

    /* create and save a moniker */
    hr = CreateFileMoniker(wszFileName1, &pMk);
    ok_ole_success(hr, "CreateFileMoniker");

    hr = IMoniker_Save(pMk, pStm, TRUE);
    ok_ole_success(hr, "IMoniker_Save");
    IMoniker_Release(pMk);

    /* overwrite the constants with various values */
    hr = IStream_Seek(pStm, zero_pos, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");
    hr = IStream_Write(pStm, &some_val, sizeof(WORD), NULL);
    ok_ole_success(hr, "IStream_Write");

    hr = IStream_Seek(pStm, dead_pos, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");
    hr = IStream_Write(pStm, &some_val, sizeof(WORD), NULL);
    ok_ole_success(hr, "IStream_Write");

    hr = IStream_Seek(pStm, nulls_pos, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");
    for(i = 0; i < 5; ++i){
        hr = IStream_Write(pStm, &some_val, sizeof(DWORD), NULL);
        ok_ole_success(hr, "IStream_Write");
    }

    /* go back to the start of the stream */
    hr = IStream_Seek(pStm, zero_pos, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    /* create a new moniker and load into it */
    hr = CreateFileMoniker(wszFileName1, &pMk);
    ok_ole_success(hr, "CreateFileMoniker");

    hr = IMoniker_Load(pMk, pStm);
    ok_ole_success(hr, "IMoniker_Load");

    IMoniker_Release(pMk);
    IStream_Release(pStm);
}

static void test_MonikerCommonPrefixWith(void)
{
    IMoniker *moniker, *item, *file1, *file2, *composite, *composite2;
    HRESULT hr;

    moniker = (void *)0xdeadbeef;
    hr = MonikerCommonPrefixWith(NULL, NULL, &moniker);
todo_wine {
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);
    ok(!moniker, "Unexpected pointer.\n");
}
    if (hr == E_NOTIMPL)
        return;

    hr = CreateItemMoniker(L"!", L"Item", &item);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    hr = MonikerCommonPrefixWith(item, NULL, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = MonikerCommonPrefixWith(NULL, item, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = MonikerCommonPrefixWith(item, item, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = CreateFileMoniker(L"C:\\test.txt", &file1);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    hr = MonikerCommonPrefixWith(file1, NULL, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = MonikerCommonPrefixWith(NULL, file1, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    /* F x F */
    hr = MonikerCommonPrefixWith(file1, file1, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = CreateFileMoniker(L"C:\\a\\test.txt", &file2);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    /* F1 x F2 */
    hr = MonikerCommonPrefixWith(file1, file2, &moniker);
    ok(hr == MK_E_NOPREFIX, "Unexpected hr %#x.\n", hr);

    hr = CreateGenericComposite(file1, item, &composite);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    hr = CreateGenericComposite(file2, item, &composite2);
    ok(hr == S_OK, "Failed to create a moniker, hr %#x.\n", hr);

    /* F x (F,I) -> F */
    hr = MonikerCommonPrefixWith(file1, composite, &moniker);
    ok(hr == MK_S_ME, "Unexpected hr %#x.\n", hr);
    ok(moniker == file1, "Unexpected pointer.\n");
    IMoniker_Release(moniker);

    /* F1 x (F2,I) -> F */
    hr = MonikerCommonPrefixWith(file1, composite2, &moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker, MKSYS_FILEMONIKER);
    TEST_DISPLAY_NAME(moniker, L"C:\\");
    IMoniker_Release(moniker);

    /* (F2,I) x F1 -> F */
    hr = MonikerCommonPrefixWith(composite2, file1, &moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker, MKSYS_FILEMONIKER);
    TEST_DISPLAY_NAME(moniker, L"C:\\");
    IMoniker_Release(moniker);

    /* (F,I) x (F) -> F */
    hr = MonikerCommonPrefixWith(composite, file1, &moniker);
    ok(hr == MK_S_HIM, "Unexpected hr %#x.\n", hr);
    ok(moniker == file1, "Unexpected pointer.\n");
    IMoniker_Release(moniker);

    /* (F,I) x (F,I) -> (F,I) */
    hr = MonikerCommonPrefixWith(composite, composite, &moniker);
    ok(hr == MK_S_US, "Unexpected hr %#x.\n", hr);
    TEST_MONIKER_TYPE(moniker, MKSYS_GENERICCOMPOSITE);
    TEST_DISPLAY_NAME(moniker, L"C:\\test.txt!Item");
    ok(moniker != composite, "Unexpected pointer.\n");
    IMoniker_Release(moniker);

    /* (F1,I) x (F2,I) -> () */
    hr = MonikerCommonPrefixWith(composite, composite2, &moniker);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(!moniker, "Unexpected pointer %p.\n", moniker);

    IMoniker_Release(composite2);
    IMoniker_Release(composite);
    IMoniker_Release(file2);
    IMoniker_Release(file1);
    IMoniker_Release(item);
}

START_TEST(moniker)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_ROT();
    test_ROT_multiple_entries();
    test_MkParseDisplayName();
    test_class_moniker();
    test_file_monikers();
    test_item_moniker();
    test_anti_moniker();
    test_generic_composite_moniker();
    test_pointer_moniker();
    test_save_load_filemoniker();
    test_MonikerCommonPrefixWith();

    /* FIXME: test moniker creation funcs and parsing other moniker formats */

    test_bind_context();

    CoUninitialize();
}
