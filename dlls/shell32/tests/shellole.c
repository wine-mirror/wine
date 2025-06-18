/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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
#define CONST_VTABLE

#include <stdio.h>
#include <wine/test.h>

#include "winbase.h"
#include "shlobj.h"
#include "shellapi.h"
#include "initguid.h"

DEFINE_GUID(FMTID_Test,0x12345678,0x1234,0x1234,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12);
DEFINE_GUID(FMTID_NotExisting, 0x12345678,0x1234,0x1234,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x13);
DEFINE_GUID(CLSID_ClassMoniker, 0x0000031a,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(Create);
DEFINE_EXPECT(Delete);
DEFINE_EXPECT(Open);
DEFINE_EXPECT(ReadMultiple);
DEFINE_EXPECT(ReadMultipleCodePage);
DEFINE_EXPECT(Release);
DEFINE_EXPECT(Stat);
DEFINE_EXPECT(WriteMultiple);

DEFINE_EXPECT(autoplay_BindToObject);
DEFINE_EXPECT(autoplay_GetClassObject);

static HRESULT (WINAPI *pSHPropStgCreate)(IPropertySetStorage*, REFFMTID, const CLSID*,
        DWORD, DWORD, DWORD, IPropertyStorage**, UINT*);
static HRESULT (WINAPI *pSHPropStgReadMultiple)(IPropertyStorage*, UINT,
        ULONG, const PROPSPEC*, PROPVARIANT*);
static HRESULT (WINAPI *pSHPropStgWriteMultiple)(IPropertyStorage*, UINT*,
        ULONG, const PROPSPEC*, PROPVARIANT*, PROPID);
static HRESULT (WINAPI *pSHCreateQueryCancelAutoPlayMoniker)(IMoniker**);
static HRESULT (WINAPI *pSHCreateSessionKey)(REGSAM, HKEY*);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("shell32.dll");

    pSHPropStgCreate = (void*)GetProcAddress(hmod, "SHPropStgCreate");
    pSHPropStgReadMultiple = (void*)GetProcAddress(hmod, "SHPropStgReadMultiple");
    pSHPropStgWriteMultiple = (void*)GetProcAddress(hmod, "SHPropStgWriteMultiple");
    pSHCreateQueryCancelAutoPlayMoniker = (void*)GetProcAddress(hmod, "SHCreateQueryCancelAutoPlayMoniker");
    pSHCreateSessionKey = (void*)GetProcAddress(hmod, (char*)723);
}

static HRESULT WINAPI PropertyStorage_QueryInterface(IPropertyStorage *This,
        REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI PropertyStorage_AddRef(IPropertyStorage *This)
{
    ok(0, "unexpected call\n");
    return 2;
}

static ULONG WINAPI PropertyStorage_Release(IPropertyStorage *This)
{
    CHECK_EXPECT(Release);
    return 1;
}

static HRESULT WINAPI PropertyStorage_ReadMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec, PROPVARIANT *rgpropvar)
{
    if(cpspec == 1) {
        CHECK_EXPECT(ReadMultipleCodePage);

        ok(rgpspec != NULL, "rgpspec = NULL\n");
        ok(rgpropvar != NULL, "rgpropvar = NULL\n");

        ok(rgpspec[0].ulKind == PRSPEC_PROPID, "rgpspec[0].ulKind = %ld\n", rgpspec[0].ulKind);
        ok(rgpspec[0].propid == PID_CODEPAGE, "rgpspec[0].propid = %ld\n", rgpspec[0].propid);

        rgpropvar[0].vt = VT_I2;
        rgpropvar[0].iVal = 1234;
    } else {
        CHECK_EXPECT(ReadMultiple);

        ok(cpspec == 10, "cpspec = %lu\n", cpspec);
        ok(rgpspec == (void*)0xdeadbeef, "rgpspec = %p\n", rgpspec);
        ok(rgpropvar != NULL, "rgpropvar = NULL\n");

        ok(rgpropvar[0].vt==0 || broken(rgpropvar[0].vt==VT_BSTR), "rgpropvar[0].vt = %d\n", rgpropvar[0].vt);

        rgpropvar[0].vt = VT_BSTR;
        rgpropvar[0].bstrVal = (void*)0xdeadbeef;
        rgpropvar[1].vt = VT_LPSTR;
        rgpropvar[1].pszVal = (void*)0xdeadbeef;
        rgpropvar[2].vt = VT_BYREF|VT_I1;
        rgpropvar[2].pcVal = (void*)0xdeadbeef;
        rgpropvar[3].vt = VT_BYREF|VT_VARIANT;
        rgpropvar[3].pvarVal = (void*)0xdeadbeef;
    }

    return S_OK;
}

static HRESULT WINAPI PropertyStorage_WriteMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec, const PROPVARIANT *rgpropvar,
        PROPID propidNameFirst)
{
    CHECK_EXPECT(WriteMultiple);

    ok(cpspec == 20, "cpspec = %ld\n", cpspec);
    ok(rgpspec == (void*)0xdeadbeef, "rgpspec = %p\n", rgpspec);
    ok(rgpropvar == (void*)0xdeadbeef, "rgpropvar = %p\n", rgpspec);
    ok(propidNameFirst == PID_FIRST_USABLE, "propidNameFirst = %ld\n", propidNameFirst);
    return S_OK;
}

static HRESULT WINAPI PropertyStorage_DeleteMultiple(IPropertyStorage *This, ULONG cpspec,
        const PROPSPEC *rgpspec)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_ReadPropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid, LPOLESTR *rglpwstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_WritePropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid, const LPOLESTR *rglpwstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_DeletePropertyNames(IPropertyStorage *This, ULONG cpropid,
        const PROPID *rgpropid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Commit(IPropertyStorage *This, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Revert(IPropertyStorage *This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Enum(IPropertyStorage *This, IEnumSTATPROPSTG **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_SetTimes(IPropertyStorage *This, const FILETIME *pctime,
        const FILETIME *patime, const FILETIME *pmtime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_SetClass(IPropertyStorage *This, REFCLSID clsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStorage_Stat(IPropertyStorage *This, STATPROPSETSTG *statpsstg)
{
    CHECK_EXPECT(Stat);

    memset(statpsstg, 0, sizeof(STATPROPSETSTG));
    memcpy(&statpsstg->fmtid, &FMTID_Test, sizeof(FMTID));
    statpsstg->grfFlags = PROPSETFLAG_ANSI;
    return S_OK;
}

static IPropertyStorageVtbl PropertyStorageVtbl = {
    PropertyStorage_QueryInterface,
    PropertyStorage_AddRef,
    PropertyStorage_Release,
    PropertyStorage_ReadMultiple,
    PropertyStorage_WriteMultiple,
    PropertyStorage_DeleteMultiple,
    PropertyStorage_ReadPropertyNames,
    PropertyStorage_WritePropertyNames,
    PropertyStorage_DeletePropertyNames,
    PropertyStorage_Commit,
    PropertyStorage_Revert,
    PropertyStorage_Enum,
    PropertyStorage_SetTimes,
    PropertyStorage_SetClass,
    PropertyStorage_Stat
};

static IPropertyStorage PropertyStorage = { &PropertyStorageVtbl };

static HRESULT WINAPI PropertySetStorage_QueryInterface(IPropertySetStorage *This,
        REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI PropertySetStorage_AddRef(IPropertySetStorage *This)
{
    ok(0, "unexpected call\n");
    return 2;
}

static ULONG WINAPI PropertySetStorage_Release(IPropertySetStorage *This)
{
    ok(0, "unexpected call\n");
    return 1;
}

static HRESULT WINAPI PropertySetStorage_Create(IPropertySetStorage *This,
        REFFMTID rfmtid, const CLSID *pclsid, DWORD grfFlags,
        DWORD grfMode, IPropertyStorage **ppprstg)
{
    CHECK_EXPECT(Create);
    ok(IsEqualGUID(rfmtid, &FMTID_Test) || IsEqualGUID(rfmtid, &FMTID_NotExisting),
            "Incorrect rfmtid value\n");
    ok(pclsid == NULL, "pclsid != NULL\n");
    ok(grfFlags == PROPSETFLAG_ANSI, "grfFlags = %lx\n", grfFlags);
    ok(grfMode == STGM_READ, "grfMode = %lx\n", grfMode);

    *ppprstg = &PropertyStorage;
    return S_OK;
}

static HRESULT WINAPI PropertySetStorage_Open(IPropertySetStorage *This,
        REFFMTID rfmtid, DWORD grfMode, IPropertyStorage **ppprstg)
{
    CHECK_EXPECT(Open);

    if(IsEqualGUID(rfmtid, &FMTID_Test)) {
        ok(grfMode == STGM_READ, "grfMode = %lx\n", grfMode);

        *ppprstg = &PropertyStorage;
        return S_OK;
    }

    return STG_E_FILENOTFOUND;
}

static HRESULT WINAPI PropertySetStorage_Delete(IPropertySetStorage *This,
        REFFMTID rfmtid)
{
    CHECK_EXPECT(Delete);
    ok(IsEqualGUID(rfmtid, &FMTID_Test), "wrong rfmtid value\n");
    return S_OK;
}

static HRESULT WINAPI PropertySetStorage_Enum(IPropertySetStorage *This,
        IEnumSTATPROPSETSTG **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertySetStorageVtbl PropertySetStorageVtbl = {
    PropertySetStorage_QueryInterface,
    PropertySetStorage_AddRef,
    PropertySetStorage_Release,
    PropertySetStorage_Create,
    PropertySetStorage_Open,
    PropertySetStorage_Delete,
    PropertySetStorage_Enum
};

static IPropertySetStorage PropertySetStorage = { &PropertySetStorageVtbl };

static void test_SHPropStg_functions(void)
{
    IPropertyStorage *property_storage;
    UINT codepage;
    PROPVARIANT read[10];
    HRESULT hres;

    if(!pSHPropStgCreate || !pSHPropStgReadMultiple || !pSHPropStgWriteMultiple) {
        win_skip("SHPropStg* functions are missing\n");
        return;
    }

    if(0) {
        /* Crashes on Windows */
        pSHPropStgCreate(NULL, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
        pSHPropStgCreate(&PropertySetStorage, NULL, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
        pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
                STGM_READ, OPEN_EXISTING, NULL, &codepage);
    }

    SET_EXPECT(Open);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_DEFAULT,
            STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_NotExisting, NULL,
            PROPSETFLAG_DEFAULT, STGM_READ, OPEN_EXISTING, &property_storage, &codepage);
    ok(hres == STG_E_FILENOTFOUND, "hres = %lx\n", hres);
    CHECK_CALLED(Open);

    SET_EXPECT(Open);
    SET_EXPECT(Release);
    SET_EXPECT(Delete);
    SET_EXPECT(Create);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, NULL, PROPSETFLAG_ANSI,
            STGM_READ, CREATE_ALWAYS, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(Release);
    CHECK_CALLED(Delete);
    CHECK_CALLED(Create);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    SET_EXPECT(Create);
    SET_EXPECT(ReadMultipleCodePage);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_NotExisting, NULL, PROPSETFLAG_ANSI,
            STGM_READ, CREATE_ALWAYS, &property_storage, &codepage);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(Open);
    CHECK_CALLED(Create);
    CHECK_CALLED(ReadMultipleCodePage);

    SET_EXPECT(Open);
    hres = pSHPropStgCreate(&PropertySetStorage, &FMTID_Test, &FMTID_NotExisting,
            PROPSETFLAG_DEFAULT, STGM_READ, OPEN_EXISTING, &property_storage, NULL);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(Open);

    SET_EXPECT(Stat);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(WriteMultiple);
    codepage = 0;
    hres = pSHPropStgWriteMultiple(property_storage, &codepage, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(codepage == 1234, "codepage = %d\n", codepage);
    CHECK_CALLED(Stat);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(WriteMultiple);

    SET_EXPECT(Stat);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(WriteMultiple);
    hres = pSHPropStgWriteMultiple(property_storage, NULL, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(Stat);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(WriteMultiple);

    SET_EXPECT(Stat);
    SET_EXPECT(WriteMultiple);
    codepage = 1000;
    hres = pSHPropStgWriteMultiple(property_storage, &codepage, 20, (void*)0xdeadbeef, (void*)0xdeadbeef, PID_FIRST_USABLE);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(codepage == 1000, "codepage = %d\n", codepage);
    CHECK_CALLED(Stat);
    CHECK_CALLED(WriteMultiple);

    read[0].vt = VT_BSTR;
    read[0].bstrVal = (void*)0xdeadbeef;
    SET_EXPECT(ReadMultiple);
    SET_EXPECT(ReadMultipleCodePage);
    SET_EXPECT(Stat);
    hres = pSHPropStgReadMultiple(property_storage, 0, 10, (void*)0xdeadbeef, read);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(ReadMultiple);
    CHECK_CALLED(ReadMultipleCodePage);
    CHECK_CALLED(Stat);

    SET_EXPECT(ReadMultiple);
    SET_EXPECT(Stat);
    hres = pSHPropStgReadMultiple(property_storage, 1251, 10, (void*)0xdeadbeef, read);
    ok(hres == S_OK, "hres = %lx\n", hres);
    CHECK_CALLED(ReadMultiple);
    CHECK_CALLED(Stat);
}

static HRESULT WINAPI test_activator_QI(IClassActivator *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassActivator))
    {
        *ppv = iface;
    }

    if (!*ppv) return E_NOINTERFACE;

    IClassActivator_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI test_activator_AddRef(IClassActivator *iface)
{
    return 2;
}

static ULONG WINAPI test_activator_Release(IClassActivator *iface)
{
    return 1;
}

static HRESULT WINAPI test_activator_GetClassObject(IClassActivator *iface, REFCLSID clsid,
    DWORD context, LCID locale, REFIID riid, void **ppv)
{
    CHECK_EXPECT(autoplay_GetClassObject);
    ok(IsEqualGUID(clsid, &CLSID_QueryCancelAutoPlay), "clsid %s\n", wine_dbgstr_guid(clsid));
    ok(IsEqualIID(riid, &IID_IQueryCancelAutoPlay), "riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static const IClassActivatorVtbl test_activator_vtbl = {
    test_activator_QI,
    test_activator_AddRef,
    test_activator_Release,
    test_activator_GetClassObject
};

static IClassActivator test_activator = { &test_activator_vtbl };

static HRESULT WINAPI test_moniker_QueryInterface(IMoniker* iface, REFIID riid, void **ppvObject)
{
    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid))
    {
        *ppvObject = iface;
    }

    if (!*ppvObject)
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI test_moniker_AddRef(IMoniker* iface)
{
    return 2;
}

static ULONG WINAPI test_moniker_Release(IMoniker* iface)
{
    return 1;
}

static HRESULT WINAPI test_moniker_GetClassID(IMoniker* iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsDirty(IMoniker* iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Load(IMoniker* iface, IStream* pStm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_BindToObject(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* moniker_to_left,
                                                REFIID riid,
                                                void** ppv)
{
    CHECK_EXPECT(autoplay_BindToObject);
    ok(pbc != NULL, "got %p\n", pbc);
    ok(moniker_to_left == NULL, "got %p\n", moniker_to_left);
    ok(IsEqualIID(riid, &IID_IClassActivator), "got riid %s\n", wine_dbgstr_guid(riid));

    if (IsEqualIID(riid, &IID_IClassActivator))
    {
        *ppv = &test_activator;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Reduce(IMoniker* iface,
                                      IBindCtx* pbc,
                                      DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft,
                                      IMoniker** ppmkReduced)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_ComposeWith(IMoniker* iface,
                                           IMoniker* pmkRight,
                                           BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsEqual(IMoniker* iface, IMoniker* pmkOtherMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Hash(IMoniker* iface, DWORD* pdwHash)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pItemTime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_Inverse(IMoniker* iface, IMoniker** ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_GetDisplayName(IMoniker* iface,
                                              IBindCtx* pbc,
                                              IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_ParseDisplayName(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName,
                                                ULONG* pchEaten,
                                                IMoniker** ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_moniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMonikerVtbl test_moniker_vtbl =
{
    test_moniker_QueryInterface,
    test_moniker_AddRef,
    test_moniker_Release,
    test_moniker_GetClassID,
    test_moniker_IsDirty,
    test_moniker_Load,
    test_moniker_Save,
    test_moniker_GetSizeMax,
    test_moniker_BindToObject,
    test_moniker_BindToStorage,
    test_moniker_Reduce,
    test_moniker_ComposeWith,
    test_moniker_Enum,
    test_moniker_IsEqual,
    test_moniker_Hash,
    test_moniker_IsRunning,
    test_moniker_GetTimeOfLastChange,
    test_moniker_Inverse,
    test_moniker_CommonPrefixWith,
    test_moniker_RelativePathTo,
    test_moniker_GetDisplayName,
    test_moniker_ParseDisplayName,
    test_moniker_IsSystemMoniker
};

static IMoniker test_moniker = { &test_moniker_vtbl };

static void test_SHCreateQueryCancelAutoPlayMoniker(void)
{
    IBindCtx *ctxt;
    IMoniker *mon;
    IUnknown *unk;
    CLSID clsid;
    HRESULT hr;
    DWORD sys;

    if (!pSHCreateQueryCancelAutoPlayMoniker)
    {
        win_skip("SHCreateQueryCancelAutoPlayMoniker is not available, skipping tests.\n");
        return;
    }

    hr = pSHCreateQueryCancelAutoPlayMoniker(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = pSHCreateQueryCancelAutoPlayMoniker(&mon);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    sys = -1;
    hr = IMoniker_IsSystemMoniker(mon, &sys);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(sys == MKSYS_CLASSMONIKER, "got %ld\n", sys);

    memset(&clsid, 0, sizeof(clsid));
    hr = IMoniker_GetClassID(mon, &clsid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_ClassMoniker), "got %s\n", wine_dbgstr_guid(&clsid));

    /* extract used CLSID that implements this hook */
    SET_EXPECT(autoplay_BindToObject);
    SET_EXPECT(autoplay_GetClassObject);

    CreateBindCtx(0, &ctxt);
    hr = IMoniker_BindToObject(mon, ctxt, &test_moniker, &IID_IQueryCancelAutoPlay, (void**)&unk);
    ok(hr == E_NOTIMPL, "got 0x%08lx\n", hr);
    IBindCtx_Release(ctxt);

    CHECK_CALLED(autoplay_BindToObject);
    CHECK_CALLED(autoplay_GetClassObject);

    IMoniker_Release(mon);
}

#define WM_EXPECTED_VALUE WM_APP
#define DROP_NC_AREA       1
#define DROP_WIDE_FILENAME 2
struct DragParam {
    HWND hwnd;
    HANDLE ready;
};

static LRESULT WINAPI drop_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static DWORD flags;
    static char expected_filename[MAX_PATH];

    switch (msg) {
    case WM_EXPECTED_VALUE:
    {
        lstrcpynA(expected_filename, (const char*)wparam, sizeof(expected_filename));
        flags = lparam;
        break;
    }
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wparam;
        char filename[MAX_PATH] = "dummy";
        WCHAR filenameW[MAX_PATH] = L"dummy", expected_filenameW[MAX_PATH];
        POINT pt;
        BOOL r;
        UINT num, len;
        winetest_push_context("%s(%lu)", wine_dbgstr_a(expected_filename), flags);

        num = DragQueryFileA(hDrop, 0xffffffff, NULL, 0);
        ok(num == 1, "expected 1, got %u\n", num);

        num = DragQueryFileA(hDrop, 0xffffffff, (char*)0xdeadbeef, 0xffffffff);
        ok(num == 1, "expected 1, got %u\n", num);

        len = strlen(expected_filename);
        num = DragQueryFileA(hDrop, 0, NULL, 0);
        ok(num == len, "expected %u, got %u\n", len, num);

        num = DragQueryFileA(hDrop, 0, filename, 0);
        ok(num == len, "expected %u, got %u\n", len, num);
        ok(!strcmp(filename, "dummy"), "got %s\n", filename);

        num = DragQueryFileA(hDrop, 0, filename, sizeof(filename));
        ok(num == len, "expected %u, got %u\n", len, num);
        ok(!strcmp(filename, expected_filename), "expected %s, got %s\n",
           expected_filename, filename);

        memset(filename, 0xaa, sizeof(filename));
        num = DragQueryFileA(hDrop, 0, filename, 2);
        ok(num == 1, "expected 1, got %u\n", num);
        ok(filename[0] == expected_filename[0], "expected '%c', got '%c'\n",
           expected_filename[0], filename[0]);
        ok(filename[1] == '\0', "expected nul, got %#x\n", (BYTE)filename[1]);

        MultiByteToWideChar(CP_ACP, 0, expected_filename, -1,
                            expected_filenameW, ARRAY_SIZE(expected_filenameW));

        len = wcslen(expected_filenameW);
        num = DragQueryFileW(hDrop, 0, NULL, 0);
        ok(num == len, "expected %u, got %u\n", len, num);

        num = DragQueryFileW(hDrop, 0, filenameW, 0);
        ok(num == len, "expected %u, got %u\n", len, num);
        ok(!wcscmp(filenameW, L"dummy"), "got %s\n", wine_dbgstr_w(filenameW));

        num = DragQueryFileW(hDrop, 0, filenameW, ARRAY_SIZE(filename));
        ok(num == len, "expected %u, got %u\n", len, num);
        ok(!wcscmp(filenameW, expected_filenameW), "expected %s, got %s\n",
           wine_dbgstr_w(expected_filenameW), wine_dbgstr_w(filenameW));

        memset(filenameW, 0xaa, sizeof(filenameW));
        num = DragQueryFileW(hDrop, 0, filenameW, 2);
        ok(num == 1, "expected 1, got %u\n", num);
        ok(filenameW[0] == expected_filenameW[0], "expected '%lc', got '%lc'\n",
           expected_filenameW[0], filenameW[0]);
        ok(filenameW[1] == L'\0', "expected nul, got %#x\n", (WCHAR)filenameW[1]);

        r = DragQueryPoint(hDrop, &pt);
        ok(r == !(flags & DROP_NC_AREA), "expected %d, got %d\n", !(flags & DROP_NC_AREA), r);
        ok(pt.x == 10, "expected 10, got %ld\n", pt.x);
        ok(pt.y == 20, "expected 20, got %ld\n", pt.y);
        DragFinish(hDrop);
        winetest_pop_context();
        return 0;
    }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI drop_window_therad(void *arg)
{
    struct DragParam *param = arg;
    WNDCLASSA cls;
    WINDOWINFO info;
    BOOL r;
    MSG msg;

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = drop_window_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "drop test";
    RegisterClassA(&cls);

    param->hwnd = CreateWindowA("drop test", NULL, 0, 0, 0, 0, 0,
                                NULL, 0, NULL, 0);
    ok(param->hwnd != NULL, "CreateWindow failed: %ld\n", GetLastError());

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    r = GetWindowInfo(param->hwnd, &info);
    ok(r, "got %d\n", r);
    ok(!(info.dwExStyle & WS_EX_ACCEPTFILES), "got %08lx\n", info.dwExStyle);

    DragAcceptFiles(param->hwnd, TRUE);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    r = GetWindowInfo(param->hwnd, &info);
    ok(r, "got %d\n", r);
    ok((info.dwExStyle & WS_EX_ACCEPTFILES), "got %08lx\n", info.dwExStyle);

    SetEvent(param->ready);

    while ((r = GetMessageA(&msg, NULL, 0, 0)) != 0) {
        if (r == (BOOL)-1) {
            ok(0, "unexpected return value, got %d\n", r);
            break;
        }
        DispatchMessageA(&msg);
    }

    DestroyWindow(param->hwnd);
    UnregisterClassA("drop test", GetModuleHandleA(NULL));
    return 0;
}

static void test_DragQueryFile(BOOL non_client_flag)
{
    struct DragParam param;
    HANDLE hThread;
    DWORD rc;
    HGLOBAL hDrop;
    DROPFILES *pDrop;
    int ret, i;
    BOOL r;
    static const struct {
        UINT codepage;
        const char* filename;
    } testcase[] = {
        { 0, "c:\\wintest.bin" },
        { 932, "d:\\\x89\xb9\x8a\x79_02.CHY" },
    };

    param.ready = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(param.ready != NULL, "can't create event\n");
    hThread = CreateThread(NULL, 0, drop_window_therad, &param, 0, NULL);

    rc = WaitForSingleObject(param.ready, 5000);
    ok(rc == WAIT_OBJECT_0, "got %lu\n", rc);

    for (i = 0; i < ARRAY_SIZE(testcase); i++)
    {
        winetest_push_context("%d", i);
        if (testcase[i].codepage && testcase[i].codepage != GetACP())
        {
            skip("need codepage %u for this test\n", testcase[i].codepage);
            winetest_pop_context();
            continue;
        }

        ret = MultiByteToWideChar(CP_ACP, 0, testcase[i].filename, -1, NULL, 0);
        ok(ret > 0, "got %d\n", ret);
        hDrop = GlobalAlloc(GHND, sizeof(DROPFILES) + (ret + 1) * sizeof(WCHAR));
        pDrop = GlobalLock(hDrop);
        pDrop->pt.x = 10;
        pDrop->pt.y = 20;
        pDrop->fNC = non_client_flag;
        pDrop->pFiles = sizeof(DROPFILES);
        ret = MultiByteToWideChar(CP_ACP, 0, testcase[i].filename, -1,
                                  (LPWSTR)(pDrop + 1), ret);
        ok(ret > 0, "got %d\n", ret);
        pDrop->fWide = TRUE;
        GlobalUnlock(hDrop);

        r = PostMessageA(param.hwnd, WM_EXPECTED_VALUE,
                         (WPARAM)testcase[i].filename, DROP_WIDE_FILENAME | (non_client_flag ? DROP_NC_AREA : 0));
        ok(r, "got %d\n", r);

        r = PostMessageA(param.hwnd, WM_DROPFILES, (WPARAM)hDrop, 0);
        ok(r, "got %d\n", r);

        hDrop = GlobalAlloc(GHND, sizeof(DROPFILES) + strlen(testcase[i].filename) + 2);
        pDrop = GlobalLock(hDrop);
        pDrop->pt.x = 10;
        pDrop->pt.y = 20;
        pDrop->fNC = non_client_flag;
        pDrop->pFiles = sizeof(DROPFILES);
        strcpy((char *)(pDrop + 1), testcase[i].filename);
        pDrop->fWide = FALSE;
        GlobalUnlock(hDrop);

        r = PostMessageA(param.hwnd, WM_EXPECTED_VALUE,
                         (WPARAM)testcase[i].filename, non_client_flag ? DROP_NC_AREA : 0);
        ok(r, "got %d\n", r);

        r = PostMessageA(param.hwnd, WM_DROPFILES, (WPARAM)hDrop, 0);
        ok(r, "got %d\n", r);

        winetest_pop_context();
    }

    r = PostMessageA(param.hwnd, WM_QUIT, 0, 0);
    ok(r, "got %d\n", r);

    rc = WaitForSingleObject(hThread, 5000);
    ok(rc == WAIT_OBJECT_0, "got %ld\n", rc);

    CloseHandle(param.ready);
    CloseHandle(hThread);
}
#undef WM_EXPECTED_VALUE
#undef DROP_NC_AREA
#undef DROP_WIDE_FILENAME

static void test_SHCreateSessionKey(void)
{
    static const WCHAR session_format[] =
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo\\%u";
    HKEY hkey, hkey2;
    HRESULT hr;
    DWORD session;
    WCHAR sessionW[ARRAY_SIZE(session_format) + 16];
    LONG ret;

    if (!pSHCreateSessionKey)
    {
        win_skip("SHCreateSessionKey is not implemented\n");
        return;
    }

    if (0) /* crashes on native */
        hr = pSHCreateSessionKey(KEY_READ, NULL);

    hkey = (HKEY)0xdeadbeef;
    hr = pSHCreateSessionKey(0, &hkey);
    ok(hr == E_ACCESSDENIED, "got 0x%08lx\n", hr);
    ok(hkey == NULL, "got %p\n", hkey);

    hr = pSHCreateSessionKey(KEY_READ, &hkey);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = pSHCreateSessionKey(KEY_READ, &hkey2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(hkey != hkey2, "got %p, %p\n", hkey, hkey2);

    RegCloseKey(hkey);
    RegCloseKey(hkey2);

    /* check the registry */
    ProcessIdToSessionId( GetCurrentProcessId(), &session);
    if (session)
    {
        wsprintfW(sessionW, session_format, session);
        ret = RegOpenKeyW(HKEY_CURRENT_USER, sessionW, &hkey);
        ok(!ret, "key not found\n");
        RegCloseKey(hkey);
    }
}

static void test_dragdrophelper(void)
{
    IDragSourceHelper *dragsource;
    IDropTargetHelper *target;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, &IID_IDropTargetHelper, (void **)&target);
    ok(hr == S_OK, "Failed to create IDropTargetHelper, %#lx\n", hr);

    hr = IDropTargetHelper_QueryInterface(target, &IID_IDragSourceHelper, (void **)&dragsource);
    ok(hr == S_OK, "QI failed, %#lx\n", hr);
    IDragSourceHelper_Release(dragsource);

    IDropTargetHelper_Release(target);
}

START_TEST(shellole)
{
    HRESULT hr;

    init();

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize failed (0x%08lx)\n", hr);
    if (hr != S_OK)
        return;

    test_SHPropStg_functions();
    test_SHCreateQueryCancelAutoPlayMoniker();
    test_DragQueryFile(TRUE);
    test_DragQueryFile(FALSE);
    test_SHCreateSessionKey();
    test_dragdrophelper();

    CoUninitialize();
}
