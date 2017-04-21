/*
 *
 * Copyright 2014 Austin English
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

#include "initguid.h"
#include "mfapi.h"
#include "mferror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

/***********************************************************************
 *      MFStartup (mfplat.@)
 */
HRESULT WINAPI MFStartup(ULONG version, DWORD flags)
{
    FIXME("(%u, %u): stub\n", version, flags);
    return MF_E_BAD_STARTUP_VERSION;
}

/***********************************************************************
 *      MFShutdown (mfplat.@)
 */
HRESULT WINAPI MFShutdown(void)
{
    FIXME("(): stub\n");
    return S_OK;
}

static HRESULT WINAPI MFPluginControl_QueryInterface(IMFPluginControl *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IMFPluginControl)) {
        TRACE("(IID_IMFPluginControl %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("(%s %p)\n", debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MFPluginControl_AddRef(IMFPluginControl *iface)
{
    TRACE("\n");
    return 2;
}

static ULONG WINAPI MFPluginControl_Release(IMFPluginControl *iface)
{
    TRACE("\n");
    return 1;
}

static HRESULT WINAPI MFPluginControl_GetPreferredClsid(IMFPluginControl *iface, DWORD plugin_type,
        const WCHAR *selector, CLSID *clsid)
{
    FIXME("(%d %s %p)\n", plugin_type, debugstr_w(selector), clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_GetPreferredClsidByIndex(IMFPluginControl *iface, DWORD plugin_type,
        DWORD index, WCHAR **selector, CLSID *clsid)
{
    FIXME("(%d %d %p %p)\n", plugin_type, index, selector, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_SetPreferredClsid(IMFPluginControl *iface, DWORD plugin_type,
        const WCHAR *selector, const CLSID *clsid)
{
    FIXME("(%d %s %s)\n", plugin_type, debugstr_w(selector), debugstr_guid(clsid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_IsDisabled(IMFPluginControl *iface, DWORD plugin_type, REFCLSID clsid)
{
    FIXME("(%d %s)\n", plugin_type, debugstr_guid(clsid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_GetDisabledByIndex(IMFPluginControl *iface, DWORD plugin_type, DWORD index, CLSID *clsid)
{
    FIXME("(%d %d %p)\n", plugin_type, index, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI MFPluginControl_SetDisabled(IMFPluginControl *iface, DWORD plugin_type, REFCLSID clsid, BOOL disabled)
{
    FIXME("(%d %s %x)\n", plugin_type, debugstr_guid(clsid), disabled);
    return E_NOTIMPL;
}

static const IMFPluginControlVtbl MFPluginControlVtbl = {
    MFPluginControl_QueryInterface,
    MFPluginControl_AddRef,
    MFPluginControl_Release,
    MFPluginControl_GetPreferredClsid,
    MFPluginControl_GetPreferredClsidByIndex,
    MFPluginControl_SetPreferredClsid,
    MFPluginControl_IsDisabled,
    MFPluginControl_GetDisabledByIndex,
    MFPluginControl_SetDisabled
};

static IMFPluginControl plugin_control = { &MFPluginControlVtbl };

/***********************************************************************
 *      MFGetPluginControl (mfplat.@)
 */
HRESULT WINAPI MFGetPluginControl(IMFPluginControl **ret)
{
    TRACE("(%p)\n", ret);

    *ret = &plugin_control;
    return S_OK;
}

typedef struct _mfattributes
{
    IMFAttributes IMFAttributes_iface;
    LONG ref;
} mfattributes;

static inline mfattributes *impl_from_IMFAttributes(IMFAttributes *iface)
{
    return CONTAINING_RECORD(iface, mfattributes, IMFAttributes_iface);
}

static HRESULT WINAPI mfattributes_QueryInterface(IMFAttributes *iface, REFIID riid, void **out)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFAttributes))
    {
        *out = &This->IMFAttributes_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI mfattributes_AddRef(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI mfattributes_Release(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI mfattributes_GetItem(IMFAttributes *iface, REFGUID key, PROPVARIANT *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetItemType(IMFAttributes *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), type);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_CompareItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_Compare(IMFAttributes *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
                BOOL *result)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p, %d, %p\n", This, theirs, type, result);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUINT32(IMFAttributes *iface, REFGUID key, UINT32 *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUINT64(IMFAttributes *iface, REFGUID key, UINT64 *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetDouble(IMFAttributes *iface, REFGUID key, double *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetGUID(IMFAttributes *iface, REFGUID key, GUID *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetStringLength(IMFAttributes *iface, REFGUID key, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetString(IMFAttributes *iface, REFGUID key, WCHAR *value,
                UINT32 size, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), value, size, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetAllocatedString(IMFAttributes *iface, REFGUID key,
                                      WCHAR **value, UINT32 *length)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), value, length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetBlobSize(IMFAttributes *iface, REFGUID key, UINT32 *size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetBlob(IMFAttributes *iface, REFGUID key, UINT8 *buf,
                UINT32 bufsize, UINT32 *blobsize)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d, %p\n", This, debugstr_guid(key), buf, bufsize, blobsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetAllocatedBlob(IMFAttributes *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetUnknown(IMFAttributes *iface, REFGUID key, REFIID riid, void **ppv)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s, %p\n", This, debugstr_guid(key), debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetItem(IMFAttributes *iface, REFGUID key, REFPROPVARIANT Value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), Value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_DeleteItem(IMFAttributes *iface, REFGUID key)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s\n", This, debugstr_guid(key));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_DeleteAllItems(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUINT32(IMFAttributes *iface, REFGUID key, UINT32 value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %d\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUINT64(IMFAttributes *iface, REFGUID key, UINT64 value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetDouble(IMFAttributes *iface, REFGUID key, double value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %f\n", This, debugstr_guid(key), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetGUID(IMFAttributes *iface, REFGUID key, REFGUID value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_guid(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetString(IMFAttributes *iface, REFGUID key, const WCHAR *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %s\n", This, debugstr_guid(key), debugstr_w(value));

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetBlob(IMFAttributes *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p, %d\n", This, debugstr_guid(key), buf, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_SetUnknown(IMFAttributes *iface, REFGUID key, IUnknown *unknown)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %s, %p\n", This, debugstr_guid(key), unknown);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_LockStore(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_UnlockStore(IMFAttributes *iface)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetCount(IMFAttributes *iface, UINT32 *items)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p\n", This, items);

    if(items)
        *items = 0;

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_GetItemByIndex(IMFAttributes *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %d, %p, %p\n", This, index, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mfattributes_CopyAllItems(IMFAttributes *iface, IMFAttributes *dest)
{
    mfattributes *This = impl_from_IMFAttributes(iface);

    FIXME("%p, %p\n", This, dest);

    return E_NOTIMPL;
}

static const IMFAttributesVtbl mfattributes_vtbl =
{
    mfattributes_QueryInterface,
    mfattributes_AddRef,
    mfattributes_Release,
    mfattributes_GetItem,
    mfattributes_GetItemType,
    mfattributes_CompareItem,
    mfattributes_Compare,
    mfattributes_GetUINT32,
    mfattributes_GetUINT64,
    mfattributes_GetDouble,
    mfattributes_GetGUID,
    mfattributes_GetStringLength,
    mfattributes_GetString,
    mfattributes_GetAllocatedString,
    mfattributes_GetBlobSize,
    mfattributes_GetBlob,
    mfattributes_GetAllocatedBlob,
    mfattributes_GetUnknown,
    mfattributes_SetItem,
    mfattributes_DeleteItem,
    mfattributes_DeleteAllItems,
    mfattributes_SetUINT32,
    mfattributes_SetUINT64,
    mfattributes_SetDouble,
    mfattributes_SetGUID,
    mfattributes_SetString,
    mfattributes_SetBlob,
    mfattributes_SetUnknown,
    mfattributes_LockStore,
    mfattributes_UnlockStore,
    mfattributes_GetCount,
    mfattributes_GetItemByIndex,
    mfattributes_CopyAllItems
};

/***********************************************************************
 *      MFMFCreateAttributes (mfplat.@)
 */
HRESULT WINAPI MFCreateAttributes(IMFAttributes **attributes, UINT32 size)
{
    mfattributes *object;

    TRACE("%p, %d\n", attributes, size);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFAttributes_iface.lpVtbl = &mfattributes_vtbl;

    *attributes = &object->IMFAttributes_iface;
    return S_OK;
}
