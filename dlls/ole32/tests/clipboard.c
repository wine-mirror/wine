/*
 * Clipboard unit tests
 *
 * Copyright 2006 Kevin Koltzau
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

#include "wine/test.h"

#define InitFormatEtc(fe, cf, med) \
        {\
        (fe).cfFormat=cf;\
        (fe).dwAspect=DVASPECT_CONTENT;\
        (fe).ptd=NULL;\
        (fe).tymed=med;\
        (fe).lindex=-1;\
        };

typedef struct DataObjectImpl {
    const IDataObjectVtbl *lpVtbl;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    HANDLE text;
    IStream *stm;
    IStorage *stg;
} DataObjectImpl;

typedef struct EnumFormatImpl {
    const IEnumFORMATETCVtbl *lpVtbl;
    LONG ref;

    FORMATETC *fmtetc;
    UINT fmtetc_cnt;

    UINT cur;
} EnumFormatImpl;

static BOOL expect_DataObjectImpl_QueryGetData = TRUE;
static ULONG DataObjectImpl_GetData_calls = 0;

static UINT cf_stream, cf_storage, cf_another, cf_onemore;

static HRESULT EnumFormatImpl_Create(FORMATETC *fmtetc, UINT size, LPENUMFORMATETC *lplpformatetc);

static HRESULT WINAPI EnumFormatImpl_QueryInterface(IEnumFORMATETC *iface, REFIID riid, LPVOID *ppvObj)
{
    EnumFormatImpl *This = (EnumFormatImpl*)iface;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumFORMATETC)) {
        IEnumFORMATETC_AddRef(iface);
        *ppvObj = This;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumFormatImpl_AddRef(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = (EnumFormatImpl*)iface;
    LONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI EnumFormatImpl_Release(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = (EnumFormatImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        HeapFree(GetProcessHeap(), 0, This->fmtetc);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI EnumFormatImpl_Next(IEnumFORMATETC *iface, ULONG celt,
                                          FORMATETC *rgelt, ULONG *pceltFetched)
{
    EnumFormatImpl *This = (EnumFormatImpl*)iface;
    ULONG count, i;

    if(!rgelt)
        return E_INVALIDARG;

    count = min(celt, This->fmtetc_cnt - This->cur);
    for(i = 0; i < count; i++, This->cur++, rgelt++)
    {
        *rgelt = This->fmtetc[This->cur];
        if(rgelt->ptd)
        {
            DWORD size = This->fmtetc[This->cur].ptd->tdSize;
            rgelt->ptd = CoTaskMemAlloc(size);
            memcpy(rgelt->ptd, This->fmtetc[This->cur].ptd, size);
        }
    }
    if(pceltFetched)
        *pceltFetched = count;
    return count == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFormatImpl_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumFormatImpl_Reset(IEnumFORMATETC *iface)
{
    EnumFormatImpl *This = (EnumFormatImpl*)iface;

    This->cur = 0;
    return S_OK;
}

static HRESULT WINAPI EnumFormatImpl_Clone(IEnumFORMATETC *iface, IEnumFORMATETC **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumFORMATETCVtbl VT_EnumFormatImpl = {
    EnumFormatImpl_QueryInterface,
    EnumFormatImpl_AddRef,
    EnumFormatImpl_Release,
    EnumFormatImpl_Next,
    EnumFormatImpl_Skip,
    EnumFormatImpl_Reset,
    EnumFormatImpl_Clone
};

static HRESULT EnumFormatImpl_Create(FORMATETC *fmtetc, UINT fmtetc_cnt, IEnumFORMATETC **lplpformatetc)
{
    EnumFormatImpl *ret;

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumFormatImpl));
    ret->lpVtbl = &VT_EnumFormatImpl;
    ret->ref = 1;
    ret->cur = 0;
    ret->fmtetc_cnt = fmtetc_cnt;
    ret->fmtetc = HeapAlloc(GetProcessHeap(), 0, fmtetc_cnt*sizeof(FORMATETC));
    memcpy(ret->fmtetc, fmtetc, fmtetc_cnt*sizeof(FORMATETC));
    *lplpformatetc = (LPENUMFORMATETC)ret;
    return S_OK;
}

static HRESULT WINAPI DataObjectImpl_QueryInterface(IDataObject *iface, REFIID riid, LPVOID *ppvObj)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDataObject)) {
        IDataObject_AddRef(iface);
        *ppvObj = This;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DataObjectImpl_AddRef(IDataObject* iface)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI DataObjectImpl_Release(IDataObject* iface)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
    {
        int i;
        if(This->text) GlobalFree(This->text);
        for(i = 0; i < This->fmtetc_cnt; i++)
            HeapFree(GetProcessHeap(), 0, This->fmtetc[i].ptd);
        HeapFree(GetProcessHeap(), 0, This->fmtetc);
        if(This->stm) IStream_Release(This->stm);
        if(This->stg) IStorage_Release(This->stg);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DataObjectImpl_GetData(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;
    UINT i;
    BOOL foundFormat = FALSE;

    DataObjectImpl_GetData_calls++;

    if(pformatetc->lindex != -1)
        return DV_E_FORMATETC;

    for(i = 0; i < This->fmtetc_cnt; i++)
    {
        if(This->fmtetc[i].cfFormat == pformatetc->cfFormat)
        {
            foundFormat = TRUE;
            if(This->fmtetc[i].tymed & pformatetc->tymed)
            {
                pmedium->pUnkForRelease = (LPUNKNOWN)iface;
                IUnknown_AddRef(pmedium->pUnkForRelease);

                if(pformatetc->cfFormat == CF_TEXT)
                    U(*pmedium).hGlobal = This->text;
                else if(pformatetc->cfFormat == cf_stream)
                    U(*pmedium).pstm = This->stm;
                else if(pformatetc->cfFormat == cf_storage)
                    U(*pmedium).pstg = This->stg;
                return S_OK;
            }
        }
    }

    return foundFormat ? DV_E_TYMED : DV_E_FORMATETC;
}

static HRESULT WINAPI DataObjectImpl_GetDataHere(IDataObject* iface, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_QueryGetData(IDataObject* iface, FORMATETC *pformatetc)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;
    UINT i;
    BOOL foundFormat = FALSE;

    if (!expect_DataObjectImpl_QueryGetData)
        ok(0, "unexpected call to DataObjectImpl_QueryGetData\n");

    if(pformatetc->lindex != -1)
        return DV_E_LINDEX;

    for(i=0; i<This->fmtetc_cnt; i++) {
        if(This->fmtetc[i].cfFormat == pformatetc->cfFormat) {
            foundFormat = TRUE;
            if(This->fmtetc[i].tymed == pformatetc->tymed)
                return S_OK;
        }
    }
    return foundFormat?DV_E_FORMATETC:DV_E_TYMED;
}

static HRESULT WINAPI DataObjectImpl_GetCanonicalFormatEtc(IDataObject* iface, FORMATETC *pformatectIn,
                                                           FORMATETC *pformatetcOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_SetData(IDataObject* iface, FORMATETC *pformatetc,
                                             STGMEDIUM *pmedium, BOOL fRelease)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumFormatEtc(IDataObject* iface, DWORD dwDirection,
                                                   IEnumFORMATETC **ppenumFormatEtc)
{
    DataObjectImpl *This = (DataObjectImpl*)iface;

    if(dwDirection != DATADIR_GET) {
        ok(0, "unexpected direction %d\n", dwDirection);
        return E_NOTIMPL;
    }
    return EnumFormatImpl_Create(This->fmtetc, This->fmtetc_cnt, ppenumFormatEtc);
}

static HRESULT WINAPI DataObjectImpl_DAdvise(IDataObject* iface, FORMATETC *pformatetc, DWORD advf,
                                             IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_DUnadvise(IDataObject* iface, DWORD dwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObjectImpl_EnumDAdvise(IDataObject* iface, IEnumSTATDATA **ppenumAdvise)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDataObjectVtbl VT_DataObjectImpl =
{
    DataObjectImpl_QueryInterface,
    DataObjectImpl_AddRef,
    DataObjectImpl_Release,
    DataObjectImpl_GetData,
    DataObjectImpl_GetDataHere,
    DataObjectImpl_QueryGetData,
    DataObjectImpl_GetCanonicalFormatEtc,
    DataObjectImpl_SetData,
    DataObjectImpl_EnumFormatEtc,
    DataObjectImpl_DAdvise,
    DataObjectImpl_DUnadvise,
    DataObjectImpl_EnumDAdvise
};

static HRESULT DataObjectImpl_CreateText(LPCSTR text, LPDATAOBJECT *lplpdataobj)
{
    DataObjectImpl *obj;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(DataObjectImpl));
    obj->lpVtbl = &VT_DataObjectImpl;
    obj->ref = 1;
    obj->text = GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
    strcpy(GlobalLock(obj->text), text);
    GlobalUnlock(obj->text);
    obj->stm = NULL;
    obj->stg = NULL;

    obj->fmtetc_cnt = 1;
    obj->fmtetc = HeapAlloc(GetProcessHeap(), 0, obj->fmtetc_cnt*sizeof(FORMATETC));
    InitFormatEtc(obj->fmtetc[0], CF_TEXT, TYMED_HGLOBAL);

    *lplpdataobj = (LPDATAOBJECT)obj;
    return S_OK;
}

static HRESULT DataObjectImpl_CreateComplex(LPDATAOBJECT *lplpdataobj)
{
    DataObjectImpl *obj;
    const char *stm_data = "complex stream";
    const char *text_data = "complex text";
    ILockBytes *lbs;
    static const WCHAR devname[] = {'m','y','d','e','v',0};
    DEVMODEW dm;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(DataObjectImpl));
    obj->lpVtbl = &VT_DataObjectImpl;
    obj->ref = 1;
    obj->text = GlobalAlloc(GMEM_MOVEABLE, strlen(text_data) + 1);
    strcpy(GlobalLock(obj->text), text_data);
    GlobalUnlock(obj->text);
    CreateStreamOnHGlobal(NULL, TRUE, &obj->stm);
    IStream_Write(obj->stm, stm_data, strlen(stm_data), NULL);

    CreateILockBytesOnHGlobal(NULL, TRUE, &lbs);
    StgCreateDocfileOnILockBytes(lbs, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &obj->stg);
    ILockBytes_Release(lbs);

    obj->fmtetc_cnt = 5;
    /* zeroing here since FORMATETC has a hole in it, and it's confusing to have this uninitialised. */
    obj->fmtetc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, obj->fmtetc_cnt*sizeof(FORMATETC));
    InitFormatEtc(obj->fmtetc[0], CF_TEXT, TYMED_HGLOBAL);
    InitFormatEtc(obj->fmtetc[1], cf_stream, TYMED_ISTREAM);
    InitFormatEtc(obj->fmtetc[2], cf_storage, TYMED_ISTORAGE);
    InitFormatEtc(obj->fmtetc[3], cf_another, TYMED_ISTORAGE|TYMED_ISTREAM|TYMED_HGLOBAL);
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    lstrcpyW(dm.dmDeviceName, devname);
    obj->fmtetc[3].ptd = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(devname) + dm.dmSize + dm.dmDriverExtra);
    obj->fmtetc[3].ptd->tdSize = FIELD_OFFSET(DVTARGETDEVICE, tdData) + sizeof(devname) + dm.dmSize + dm.dmDriverExtra;
    obj->fmtetc[3].ptd->tdDriverNameOffset = FIELD_OFFSET(DVTARGETDEVICE, tdData);
    obj->fmtetc[3].ptd->tdDeviceNameOffset = 0;
    obj->fmtetc[3].ptd->tdPortNameOffset   = 0;
    obj->fmtetc[3].ptd->tdExtDevmodeOffset = obj->fmtetc[3].ptd->tdDriverNameOffset + sizeof(devname);
    lstrcpyW((WCHAR*)obj->fmtetc[3].ptd->tdData, devname);
    memcpy(obj->fmtetc[3].ptd->tdData + sizeof(devname), &dm, dm.dmSize + dm.dmDriverExtra);

    InitFormatEtc(obj->fmtetc[4], cf_stream, 0xfffff);
    obj->fmtetc[4].dwAspect = DVASPECT_ICON;

    *lplpdataobj = (LPDATAOBJECT)obj;
    return S_OK;
}

static void test_get_clipboard(void)
{
    HRESULT hr;
    IDataObject *data_obj;
    FORMATETC fmtetc;
    STGMEDIUM stgmedium;

    hr = OleGetClipboard(NULL);
    ok(hr == E_INVALIDARG, "OleGetClipboard(NULL) should return E_INVALIDARG instead of 0x%08x\n", hr);

    hr = OleGetClipboard(&data_obj);
    ok(hr == S_OK, "OleGetClipboard failed with error 0x%08x\n", hr);

    /* test IDataObject_QueryGetData */

    /* clipboard's IDataObject_QueryGetData shouldn't defer to our IDataObject_QueryGetData */
    expect_DataObjectImpl_QueryGetData = FALSE;

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == S_OK, "IDataObject_QueryGetData failed with error 0x%08x\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = 0xdeadbeef;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC, "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08x\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC, "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08x\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.lindex = 256;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_FORMATETC || broken(hr == S_OK),
        "IDataObject_QueryGetData should have failed with DV_E_FORMATETC instead of 0x%08x\n", hr);
    if (hr == S_OK)
        ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.cfFormat = CF_RIFF;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == DV_E_CLIPFORMAT, "IDataObject_QueryGetData should have failed with DV_E_CLIPFORMAT instead of 0x%08x\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.tymed = TYMED_FILE;
    hr = IDataObject_QueryGetData(data_obj, &fmtetc);
    ok(hr == S_OK, "IDataObject_QueryGetData failed with error 0x%08x\n", hr);

    expect_DataObjectImpl_QueryGetData = TRUE;

    /* test IDataObject_GetData */

    DataObjectImpl_GetData_calls = 0;

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08x\n", hr);
    ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = 0xdeadbeef;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08x\n", hr);
    ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.dwAspect = DVASPECT_THUMBNAIL;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == S_OK, "IDataObject_GetData failed with error 0x%08x\n", hr);
    ReleaseStgMedium(&stgmedium);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.lindex = 256;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_FORMATETC || broken(hr == S_OK), "IDataObject_GetData should have failed with DV_E_FORMATETC instead of 0x%08x\n", hr);
    if (hr == S_OK)
    {
        /* undo the unexpected success */
        DataObjectImpl_GetData_calls--;
        ReleaseStgMedium(&stgmedium);
    }

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.cfFormat = CF_RIFF;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_FORMATETC, "IDataObject_GetData should have failed with DV_E_FORMATETC instead of 0x%08x\n", hr);

    InitFormatEtc(fmtetc, CF_TEXT, TYMED_HGLOBAL);
    fmtetc.tymed = TYMED_FILE;
    hr = IDataObject_GetData(data_obj, &fmtetc, &stgmedium);
    ok(hr == DV_E_TYMED, "IDataObject_GetData should have failed with DV_E_TYMED instead of 0x%08x\n", hr);

    ok(DataObjectImpl_GetData_calls == 6, "DataObjectImpl_GetData should have been called 6 times instead of %d times\n", DataObjectImpl_GetData_calls);

    IDataObject_Release(data_obj);
}

static void test_cf_dataobject(IDataObject *data)
{
    UINT cf = 0;
    UINT cf_dataobject = RegisterClipboardFormatA("DataObject");
    UINT cf_ole_priv_data = RegisterClipboardFormatA("Ole Private Data");
    BOOL found_dataobject = FALSE, found_priv_data = FALSE;

    OpenClipboard(NULL);
    do
    {
        cf = EnumClipboardFormats(cf);
        if(cf == cf_dataobject)
        {
            HGLOBAL h = GetClipboardData(cf);
            HWND *ptr = GlobalLock(h);
            DWORD size = GlobalSize(h);
            HWND clip_owner = GetClipboardOwner();

            found_dataobject = TRUE;
            ok(size >= sizeof(*ptr), "size %d\n", size);
            if(data)
                ok(*ptr == clip_owner, "hwnd %p clip_owner %p\n", *ptr, clip_owner);
            else /* ole clipboard flushed */
                ok(*ptr == NULL, "hwnd %p\n", *ptr);
            GlobalUnlock(h);
        }
        else if(cf == cf_ole_priv_data)
        {
            found_priv_data = TRUE;
            if(data)
            {
                HGLOBAL h = GetClipboardData(cf);
                DWORD *ptr = GlobalLock(h);
                DWORD size = GlobalSize(h);

                if(size != ptr[1])
                    win_skip("Ole Private Data in win9x format\n");
                else
                {
                    HRESULT hr;
                    IEnumFORMATETC *enum_fmt;
                    DWORD count = 0;
                    FORMATETC fmt;
                    struct formatetcetc
                    {
                        FORMATETC fmt;
                        BOOL first_use_of_cf;
                        DWORD res[2];
                    } *fmt_ptr;
                    struct priv_data
                    {
                        DWORD res1;
                        DWORD size;
                        DWORD res2;
                        DWORD count;
                        DWORD res3[2];
                        struct formatetcetc fmts[1];
                    } *priv = (struct priv_data*)ptr;
                    CLIPFORMAT cfs_seen[10];

                    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);
                    ok(hr == S_OK, "got %08x\n", hr);
                    fmt_ptr = priv->fmts;

                    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
                    {
                        int i;
                        BOOL seen_cf = FALSE;

                        ok(fmt_ptr->fmt.cfFormat == fmt.cfFormat,
                           "got %08x expected %08x\n", fmt_ptr->fmt.cfFormat, fmt.cfFormat);
                        ok(fmt_ptr->fmt.dwAspect == fmt.dwAspect, "got %08x expected %08x\n",
                           fmt_ptr->fmt.dwAspect, fmt.dwAspect);
                        ok(fmt_ptr->fmt.lindex == fmt.lindex, "got %08x expected %08x\n",
                           fmt_ptr->fmt.lindex, fmt.lindex);
                        ok(fmt_ptr->fmt.tymed == fmt.tymed, "got %08x expected %08x\n",
                           fmt_ptr->fmt.tymed, fmt.tymed);
                        for(i = 0; i < count; i++)
                            if(fmt_ptr->fmt.cfFormat == cfs_seen[i])
                            {
                                seen_cf = TRUE;
                                break;
                            }
                        cfs_seen[count] = fmt.cfFormat;
                        ok(fmt_ptr->first_use_of_cf == seen_cf ? FALSE : TRUE, "got %08x expected %08x\n",
                           fmt_ptr->first_use_of_cf, !seen_cf);
                        ok(fmt_ptr->res[0] == 0, "got %08x\n", fmt_ptr->res[1]);
                        ok(fmt_ptr->res[1] == 0, "got %08x\n", fmt_ptr->res[2]);
                        if(fmt.ptd)
                        {
                            DVTARGETDEVICE *target;

                            ok(fmt_ptr->fmt.ptd != NULL, "target device offset zero\n");
                            target = (DVTARGETDEVICE*)((char*)priv + (DWORD)fmt_ptr->fmt.ptd);
                            ok(!memcmp(target, fmt.ptd, fmt.ptd->tdSize), "target devices differ\n");
                            CoTaskMemFree(fmt.ptd);
                        }
                        fmt_ptr++;
                        count++;
                    }
                    ok(priv->res1 == 0, "got %08x\n", priv->res1);
                    ok(priv->res2 == 1, "got %08x\n", priv->res2);
                    ok(priv->count == count, "got %08x expected %08x\n", priv->count, count);
                    ok(priv->res3[0] == 0, "got %08x\n", priv->res3[0]);
                    ok(priv->res3[1] == 0, "got %08x\n", priv->res3[1]);

                    GlobalUnlock(h);
                    IEnumFORMATETC_Release(enum_fmt);
                }
            }
        }
    } while(cf);
    CloseClipboard();
    ok(found_dataobject, "didn't find cf_dataobject\n");
    ok(found_priv_data, "didn't find cf_ole_priv_data\n");
}

static void test_set_clipboard(void)
{
    HRESULT hr;
    ULONG ref;
    LPDATAOBJECT data1, data2, data_cmpl;
    HGLOBAL hblob, h;

    cf_stream = RegisterClipboardFormatA("stream format");
    cf_storage = RegisterClipboardFormatA("storage format");
    cf_another = RegisterClipboardFormatA("another format");
    cf_onemore = RegisterClipboardFormatA("one more format");

    hr = DataObjectImpl_CreateText("data1", &data1);
    ok(SUCCEEDED(hr), "Failed to create data1 object: 0x%08x\n", hr);
    if(FAILED(hr))
        return;
    hr = DataObjectImpl_CreateText("data2", &data2);
    ok(SUCCEEDED(hr), "Failed to create data2 object: 0x%08x\n", hr);
    if(FAILED(hr))
        return;
    hr = DataObjectImpl_CreateComplex(&data_cmpl);
    ok(SUCCEEDED(hr), "Failed to create complex data object: 0x%08x\n", hr);
    if(FAILED(hr))
        return;

    hr = OleSetClipboard(data1);
    ok(hr == CO_E_NOTINITIALIZED, "OleSetClipboard should have failed with CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    CoInitialize(NULL);
    hr = OleSetClipboard(data1);
    ok(hr == CO_E_NOTINITIALIZED ||
       hr == CLIPBRD_E_CANT_SET, /* win9x */
       "OleSetClipboard should have failed with "
       "CO_E_NOTINITIALIZED or CLIPBRD_E_CANT_SET instead of 0x%08x\n", hr);
    CoUninitialize();

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize failed with error 0x%08x\n", hr);

    hr = OleSetClipboard(data1);
    ok(hr == S_OK, "failed to set clipboard to data1, hr = 0x%08x\n", hr);

    test_cf_dataobject(data1);

    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_OK, "expected current clipboard to be data1, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_FALSE, "did not expect current clipboard to be data2, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08x\n", hr);

    test_get_clipboard();

    hr = OleSetClipboard(data2);
    ok(hr == S_OK, "failed to set clipboard to data2, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_FALSE, "did not expect current clipboard to be data1, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_OK, "expected current clipboard to be data2, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08x\n", hr);

    /* put a format directly onto the clipboard to show
       OleFlushClipboard doesn't empty the clipboard */
    hblob = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE|GMEM_ZEROINIT, 10);
    OpenClipboard(NULL);
    h = SetClipboardData(cf_onemore, hblob);
    ok(h == hblob, "got %p\n", h);
    h = GetClipboardData(cf_onemore);
    ok(h == hblob, "got %p\n", h);
    CloseClipboard();

    hr = OleFlushClipboard();
    ok(hr == S_OK, "failed to flush clipboard, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(data1);
    ok(hr == S_FALSE, "did not expect current clipboard to be data1, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(data2);
    ok(hr == S_FALSE, "did not expect current clipboard to be data2, hr = 0x%08x\n", hr);
    hr = OleIsCurrentClipboard(NULL);
    ok(hr == S_FALSE, "expect S_FALSE, hr = 0x%08x\n", hr);

    /* format should survive the flush */
    OpenClipboard(NULL);
    h = GetClipboardData(cf_onemore);
    ok(h == hblob, "got %p\n", h);
    CloseClipboard();

    test_cf_dataobject(NULL);

    ok(OleSetClipboard(NULL) == S_OK, "failed to clear clipboard, hr = 0x%08x\n", hr);

    OpenClipboard(NULL);
    h = GetClipboardData(cf_onemore);
    ok(h == NULL, "got %p\n", h);
    CloseClipboard();

    hr = OleSetClipboard(data_cmpl);
    ok(hr == S_OK, "failed to set clipboard to complex data, hr = 0x%08x\n", hr);
    test_cf_dataobject(data_cmpl);

    ok(OleSetClipboard(NULL) == S_OK, "failed to clear clipboard, hr = 0x%08x\n", hr);

    ref = IDataObject_Release(data1);
    ok(ref == 0, "expected data1 ref=0, got %d\n", ref);
    ref = IDataObject_Release(data2);
    ok(ref == 0, "expected data2 ref=0, got %d\n", ref);
    ref = IDataObject_Release(data_cmpl);
    ok(ref == 0, "expected data_cmpl ref=0, got %d\n", ref);

    OleUninitialize();
}


START_TEST(clipboard)
{
    test_set_clipboard();
}
