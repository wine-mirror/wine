/*
 * Speech API (SAPI) main file.
 *
 * Copyright (C) 2017 Huw Davies
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
#include "objbase.h"
#include "rpcproxy.h"
#include "oaidl.h"
#include "ocidl.h"

#include "initguid.h"
#include "sapiddk.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *, REFIID, void **);
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI class_factory_QueryInterface( IClassFactory *iface,
                                                    REFIID iid, void **obj )
{
    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI class_factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance( IClassFactory *iface,
                                                    IUnknown *outer, REFIID iid,
                                                    void **obj )
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE( "%p %s %p\n", outer, debugstr_guid( iid ), obj );

    *obj = NULL;
    return This->create_instance( outer, iid, obj );
}

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface,
                                                BOOL lock )
{
    FIXME( "%d: stub!\n", lock );
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer
};

static struct class_factory data_key_cf       = { { &class_factory_vtbl }, data_key_create };
static struct class_factory file_stream_cf    = { { &class_factory_vtbl }, file_stream_create };
static struct class_factory mmaudio_out_cf    = { { &class_factory_vtbl }, mmaudio_out_create };
static struct class_factory resource_mgr_cf   = { { &class_factory_vtbl }, resource_manager_create };
static struct class_factory speech_stream_cf  = { { &class_factory_vtbl }, speech_stream_create };
static struct class_factory speech_voice_cf   = { { &class_factory_vtbl }, speech_voice_create };
static struct class_factory token_category_cf = { { &class_factory_vtbl }, token_category_create };
static struct class_factory token_enum_cf     = { { &class_factory_vtbl }, token_enum_create };
static struct class_factory token_cf          = { { &class_factory_vtbl }, token_create };

/******************************************************************
 *             DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **obj )
{
    IClassFactory *cf = NULL;

    TRACE( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (IsEqualCLSID( clsid, &CLSID_SpDataKey ))
        cf = &data_key_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpFileStream ))
        cf = &file_stream_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpMMAudioOut ))
        cf = &mmaudio_out_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpObjectTokenCategory ))
        cf = &token_category_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpObjectTokenEnum ))
        cf = &token_enum_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpObjectToken ))
        cf = &token_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpResourceManager ))
        cf = &resource_mgr_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpStream ))
        cf = &speech_stream_cf.IClassFactory_iface;
    else if (IsEqualCLSID( clsid, &CLSID_SpVoice ))
        cf = &speech_voice_cf.IClassFactory_iface;

    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}
