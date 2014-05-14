/*
 * Copyright 2012 Austin English
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "wmsdkidl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("(): stub\n");

    return S_OK;
}

HRESULT WINAPI WMCreateEditor(IWMMetadataEditor **editor)
{
    FIXME("(%p): stub\n", editor);

    *editor = NULL;

    return E_NOTIMPL;
}

typedef struct {
    IWMReader IWMReader_iface;
    LONG ref;
} WMReader;

static inline WMReader *impl_from_IWMReader(IWMReader *iface)
{
    return CONTAINING_RECORD(iface, WMReader, IWMReader_iface);
}

static HRESULT WINAPI WMReader_QueryInterface(IWMReader *iface, REFIID riid, void **ppv)
{
    WMReader *This = impl_from_IWMReader(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMReader_iface;
    }else if(IsEqualGUID(riid, &IID_IWMReader)) {
        TRACE("(%p)->(IID_IWMReader %p)\n", This, ppv);
        *ppv = &This->IWMReader_iface;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMReader_AddRef(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMReader_Release(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMReader_Open(IWMReader *iface, const WCHAR *url, IWMReaderCallback *callback, void *context)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(url), callback, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Close(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputCount(IWMReader *iface, DWORD *outputs)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%p)\n", This, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputProps(IWMReader *iface, DWORD output_num, IWMOutputMediaProps **output)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_SetOutputProps(IWMReader *iface, DWORD output_num, IWMOutputMediaProps *output)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputFormatCount(IWMReader *iface, DWORD output_num, DWORD *formats)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %p)\n", This, output_num, formats);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_GetOutputFormat(IWMReader *iface, DWORD output_num, DWORD format_num, IWMOutputMediaProps **props)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%u %u %p)\n", This, output_num, format_num, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Start(IWMReader *iface, QWORD start, QWORD duration, float rate, void *context)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)->(%s %s %f %p)\n", This, wine_dbgstr_longlong(start), wine_dbgstr_longlong(duration), rate, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Stop(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Pause(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMReader_Resume(IWMReader *iface)
{
    WMReader *This = impl_from_IWMReader(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMReaderVtbl WMReaderVtbl = {
    WMReader_QueryInterface,
    WMReader_AddRef,
    WMReader_Release,
    WMReader_Open,
    WMReader_Close,
    WMReader_GetOutputCount,
    WMReader_GetOutputProps,
    WMReader_SetOutputProps,
    WMReader_GetOutputFormatCount,
    WMReader_GetOutputFormat,
    WMReader_Start,
    WMReader_Stop,
    WMReader_Pause,
    WMReader_Resume
};

HRESULT WINAPI WMCreateReader(IUnknown *reserved, DWORD rights, IWMReader **ret_reader)
{
    WMReader *reader;

    TRACE("(%p, %x, %p)\n", reserved, rights, ret_reader);

    reader = heap_alloc(sizeof(*reader));
    if(!reader)
        return E_OUTOFMEMORY;

    reader->IWMReader_iface.lpVtbl = &WMReaderVtbl;
    reader->ref = 1;

    *ret_reader = &reader->IWMReader_iface;
    return E_NOTIMPL;
}

HRESULT WINAPI WMCreateSyncReader(IUnknown *pcert, DWORD rights, IWMSyncReader **syncreader)
{
    FIXME("(%p, %x, %p): stub\n", pcert, rights, syncreader);

    *syncreader = NULL;

    return E_NOTIMPL;
}

typedef struct {
    IWMProfileManager IWMProfileManager_iface;
    LONG ref;
} WMProfileManager;

static inline WMProfileManager *impl_from_IWMProfileManager(IWMProfileManager *iface)
{
    return CONTAINING_RECORD(iface, WMProfileManager, IWMProfileManager_iface);
}

static HRESULT WINAPI WMProfileManager_QueryInterface(IWMProfileManager *iface, REFIID riid, void **ppv)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMProfileManager_iface;
    }else if(IsEqualGUID(&IID_IWMProfileManager, riid)) {
        TRACE("(%p)->(IID_IWMProfileManager %p)\n", This, ppv);
        *ppv = &This->IWMProfileManager_iface;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMProfileManager_AddRef(IWMProfileManager *iface)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMProfileManager_Release(IWMProfileManager *iface)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMProfileManager_CreateEmptyProfile(IWMProfileManager *iface, WMT_VERSION version, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%x %p)\n", This, version, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadProfileByID(IWMProfileManager *iface, REFGUID guid, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(guid), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadProfileByData(IWMProfileManager *iface, const WCHAR *profile, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(profile), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_SaveProfile(IWMProfileManager *iface, IWMProfile *profile, WCHAR *profile_str, DWORD *len)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%p %p %p)\n", This, profile, profile_str, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_GetSystemProfileCount(IWMProfileManager *iface, DWORD *ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%p)\n", This, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadSystemProfile(IWMProfileManager *iface, DWORD index, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager(iface);
    FIXME("(%p)->(%d %p)\n", This, index, ret);
    return E_NOTIMPL;
}

static const IWMProfileManagerVtbl WMProfileManagerVtbl = {
    WMProfileManager_QueryInterface,
    WMProfileManager_AddRef,
    WMProfileManager_Release,
    WMProfileManager_CreateEmptyProfile,
    WMProfileManager_LoadProfileByID,
    WMProfileManager_LoadProfileByData,
    WMProfileManager_SaveProfile,
    WMProfileManager_GetSystemProfileCount,
    WMProfileManager_LoadSystemProfile
};

HRESULT WINAPI WMCreateProfileManager(IWMProfileManager **ret)
{
    WMProfileManager *profile_mgr;

    TRACE("(%p)\n", ret);

    profile_mgr = heap_alloc(sizeof(*profile_mgr));
    if(!profile_mgr)
        return E_OUTOFMEMORY;

    profile_mgr->IWMProfileManager_iface.lpVtbl = &WMProfileManagerVtbl;
    profile_mgr->ref = 1;

    *ret = &profile_mgr->IWMProfileManager_iface;
    return S_OK;
}
