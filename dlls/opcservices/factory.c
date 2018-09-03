/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include "initguid.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "msopc.h"

#include "wine/debug.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

static HRESULT WINAPI opc_factory_QueryInterface(IOpcFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcFactory) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_factory_AddRef(IOpcFactory *iface)
{
    return 2;
}

static ULONG WINAPI opc_factory_Release(IOpcFactory *iface)
{
    return 1;
}

static HRESULT WINAPI opc_factory_CreatePackageRootUri(IOpcFactory *iface, IOpcUri **uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_factory_CreatePartUri(IOpcFactory *iface, LPCWSTR uri, IOpcPartUri **part_uri)
{
    TRACE("iface %p, uri %s, part_uri %p.\n", iface, debugstr_w(uri), part_uri);

    return opc_part_uri_create(uri, part_uri);
}

static HRESULT WINAPI opc_factory_CreateStreamOnFile(IOpcFactory *iface, LPCWSTR filename,
        OPC_STREAM_IO_MODE io_mode, SECURITY_ATTRIBUTES *sa, DWORD flags, IStream **stream)
{
    FIXME("iface %p, filename %s, io_mode %d, sa %p, flags %#x, stream %p stub!\n", iface, debugstr_w(filename),
            io_mode, sa, flags, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_factory_CreatePackage(IOpcFactory *iface, IOpcPackage **package)
{
    TRACE("iface %p, package %p.\n", iface, package);

    return opc_package_create(package);
}

static HRESULT WINAPI opc_factory_ReadPackageFromStream(IOpcFactory *iface, IStream *stream,
        OPC_READ_FLAGS flags, IOpcPackage **package)
{
    FIXME("iface %p, stream %p, flags %#x, package %p stub!\n", iface, stream, flags, package);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_factory_WritePackageToStream(IOpcFactory *iface, IOpcPackage *package, OPC_WRITE_FLAGS flags,
        IStream *stream)
{
    FIXME("iface %p, package %p, flags %#x, stream %p stub!\n", iface, package, flags, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_factory_CreateDigitalSignatureManager(IOpcFactory *iface, IOpcPackage *package,
        IOpcDigitalSignatureManager **signature_manager)
{
    FIXME("iface %p, package %p, signature_manager %p stub!\n", iface, package, signature_manager);

    return E_NOTIMPL;
}

static const IOpcFactoryVtbl opc_factory_vtbl =
{
    opc_factory_QueryInterface,
    opc_factory_AddRef,
    opc_factory_Release,
    opc_factory_CreatePackageRootUri,
    opc_factory_CreatePartUri,
    opc_factory_CreateStreamOnFile,
    opc_factory_CreatePackage,
    opc_factory_ReadPackageFromStream,
    opc_factory_WritePackageToStream,
    opc_factory_CreateDigitalSignatureManager,
};

static IOpcFactory opc_factory = { &opc_factory_vtbl };

static HRESULT WINAPI opc_class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IClassFactory) ||
            IsEqualGUID(iid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI opc_class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI opc_class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IOpcFactory_QueryInterface(&opc_factory, iid, out);
}

static HRESULT WINAPI opc_class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("iface %p, dolock %d stub!\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl opc_class_factory_vtbl =
{
    opc_class_factory_QueryInterface,
    opc_class_factory_AddRef,
    opc_class_factory_Release,
    opc_class_factory_CreateInstance,
    opc_class_factory_LockServer
};

static IClassFactory opc_class_factory = { &opc_class_factory_vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    TRACE("clsid %s, iid %s, out %p\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualCLSID(clsid, &CLSID_OpcFactory))
        return IClassFactory_QueryInterface(&opc_class_factory, iid, out);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    return E_NOTIMPL;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

static HINSTANCE OPC_hInstance;

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD reason, void *reserved)
{
    OPC_hInstance = hInstDLL;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return __wine_register_resources( OPC_hInstance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( OPC_hInstance );
}
