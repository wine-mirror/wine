/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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
#include "initguid.h"
#include "objbase.h"
#include "cor.h"
#include "rometadata.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rometadata);

struct metadata_dispenser
{
    IMetaDataDispenserEx IMetaDataDispenserEx_iface;
    LONG refcount;
};

static inline struct metadata_dispenser *impl_from_IMetaDataDispenserEx(IMetaDataDispenserEx *iface)
{
    return CONTAINING_RECORD(iface, struct metadata_dispenser, IMetaDataDispenserEx_iface);
}

static HRESULT WINAPI MetaDataDispenser_QueryInterface(IMetaDataDispenserEx *iface, REFIID riid, void **obj)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IMetaDataDispenserEx)
        || IsEqualGUID(riid, &IID_IMetaDataDispenser)
        || IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMetaDataDispenserEx_AddRef(iface);
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MetaDataDispenser_AddRef(IMetaDataDispenserEx *iface)
{
    struct metadata_dispenser *this = impl_from_IMetaDataDispenserEx(iface);
    ULONG ref = InterlockedIncrement(&this->refcount);

    TRACE("%p ref=%lu\n", this, ref);

    return ref;
}

static ULONG WINAPI MetaDataDispenser_Release(IMetaDataDispenserEx *iface)
{
    struct metadata_dispenser *this = impl_from_IMetaDataDispenserEx(iface);
    ULONG ref = InterlockedDecrement(&this->refcount);

    TRACE("%p ref=%lu\n", this, ref);

    if (ref == 0)
        free(this);

    return ref;
}

static HRESULT WINAPI MetaDataDispenser_DefineScope(IMetaDataDispenserEx *iface, REFCLSID rclsid,
                                                    DWORD create_flags, REFIID riid, IUnknown **obj)
{
    FIXME("%p %s %lx %s %p\n", iface, debugstr_guid(rclsid), create_flags, debugstr_guid(riid),
            obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScope(IMetaDataDispenserEx *iface, const WCHAR *scope,
                                                  DWORD open_flags, REFIID riid, IUnknown **obj)
{
    FIXME("%p %s %lx %s %p\n", iface, debugstr_w(scope), open_flags, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScopeOnMemory(IMetaDataDispenserEx *iface, const void *data,
                                                          ULONG data_size, DWORD open_flags, REFIID riid,
                                                          IUnknown **obj)
{
    FIXME("%p %p %lu %lx %s %p\n", iface, data, data_size, open_flags, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_SetOption(IMetaDataDispenserEx *iface, REFGUID option_id, const VARIANT *value)
{
    FIXME("%p %s %p\n", iface, debugstr_guid(option_id), debugstr_variant(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_GetOption(IMetaDataDispenserEx *iface, REFGUID optionid, VARIANT *value)
{
    FIXME("%p %s %s\n", iface, debugstr_guid(optionid), debugstr_variant(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScopeOnITypeInfo(IMetaDataDispenserEx *iface, ITypeInfo *type_info,
                                                             DWORD open_flags, REFIID riid, IUnknown **obj)
{
    FIXME("%p %p %lu %s %p\n", iface, type_info, open_flags, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_GetCORSystemDirectory(IMetaDataDispenserEx *iface, WCHAR *buffer,
                                                              DWORD buffer_size, DWORD *return_length)
{
    FIXME("%p %p %lu %p\n", iface, buffer, buffer_size, return_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_FindAssembly(IMetaDataDispenserEx *iface, const WCHAR *app_base,
                                                     const WCHAR *private_bin, const WCHAR *global_bin,
                                                     const WCHAR *assembly_name, WCHAR *name, ULONG name_size,
                                                     ULONG *return_length)
{
    FIXME("%p %s %s %s %s %p %lu %p\n", iface, debugstr_w(app_base), debugstr_w(private_bin),
          debugstr_w(global_bin), debugstr_w(assembly_name), name, name_size, return_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_FindAssemblyModule(IMetaDataDispenserEx *iface, const WCHAR *app_base,
                                                           const WCHAR *private_bin, const WCHAR *global_bin,
                                                           const WCHAR *assembly_name, const WCHAR *module_name,
                                                           WCHAR *name, ULONG name_size, ULONG *return_length)
{
    FIXME("%p %s %s %s %s %s %p %lu %p\n", iface, debugstr_w(app_base), debugstr_w(private_bin),
          debugstr_w(global_bin), debugstr_w(assembly_name), debugstr_w(module_name), name, name_size, return_length);
    return E_NOTIMPL;
}

static const struct IMetaDataDispenserExVtbl MetaDataDispenserExVtbl =
{
    MetaDataDispenser_QueryInterface,
    MetaDataDispenser_AddRef,
    MetaDataDispenser_Release,
    MetaDataDispenser_DefineScope,
    MetaDataDispenser_OpenScope,
    MetaDataDispenser_OpenScopeOnMemory,
    MetaDataDispenser_SetOption,
    MetaDataDispenser_GetOption,
    MetaDataDispenser_OpenScopeOnITypeInfo,
    MetaDataDispenser_GetCORSystemDirectory,
    MetaDataDispenser_FindAssembly,
    MetaDataDispenser_FindAssemblyModule
};

STDAPI MetaDataGetDispenser(REFCLSID rclsid, REFIID riid, void **obj)
{
    struct metadata_dispenser *dispenser;
    HRESULT hr;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), obj);

    if (!IsEqualGUID(rclsid, &CLSID_CorMetaDataDispenser))
        return CLASS_E_CLASSNOTAVAILABLE;

    dispenser = malloc(sizeof(*dispenser));
    if (!dispenser)
        return E_OUTOFMEMORY;

    dispenser->IMetaDataDispenserEx_iface.lpVtbl = &MetaDataDispenserExVtbl;
    dispenser->refcount = 1;

    hr = IMetaDataDispenserEx_QueryInterface(&dispenser->IMetaDataDispenserEx_iface, riid, obj);
    IMetaDataDispenserEx_Release(&dispenser->IMetaDataDispenserEx_iface);
    return hr;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    TRACE("inst %p, reason %lu, reserved %p.\n", inst, reason, reserved);

    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(inst);

    return TRUE;
}
