/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
 *
 * Written by Weather
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

const SIZE_T XSystemAppSpecificDeviceIdBytes = 45;
const SIZE_T XSystemConsoleIdBytes = 39;
const SIZE_T XSystemXboxLiveSandboxIdMaxBytes = 16;

struct x_system
{
    IXSystemImpl5 IXSystemImpl5_iface;
    LONG ref;
};

static inline struct x_system *impl_from_IXSystemImpl5( IXSystemImpl5 *iface )
{
    return CONTAINING_RECORD( iface, struct x_system, IXSystemImpl5_iface );
}

static HRESULT WINAPI x_system_QueryInterface( IXSystemImpl5 *iface, REFIID iid, void **out )
{
    struct x_system *impl = impl_from_IXSystemImpl5( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown      ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl  ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl2 ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl3 ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl4 ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl5 ))
    {
        IXSystemImpl5_AddRef( *out = &impl->IXSystemImpl5_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_system_AddRef( IXSystemImpl5 *iface )
{
    struct x_system *impl = impl_from_IXSystemImpl5( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_system_Release( IXSystemImpl5 *iface )
{
    struct x_system *impl = impl_from_IXSystemImpl5( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_system_XSystemGetConsoleId( IXSystemImpl5 *iface, INT32 consoleIdSize, char *consoleId, SIZE_T *consoleIdUsed )
{
    /* For Windows, Console ID is always `00000000.00000000.00000000.00000000.00 */
    const char *Id = "00000000.00000000.00000000.00000000.00";

    TRACE( "iface %p, consoleIdSize %d, consoleId %p, consoleIdUsed %p\n", iface, consoleIdSize, consoleId, consoleIdUsed );

    if (!consoleId || !consoleIdUsed)
        return E_POINTER;

    if (consoleIdSize < XSystemConsoleIdBytes)
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

    strcpy_s( consoleId, consoleIdSize, Id );
    *consoleIdUsed = strlen( Id ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_system_XSystemGetXboxLiveSandboxId( IXSystemImpl5 *iface, INT32 sandboxIdSize, char *sandboxId, SIZE_T *sandboxIdUsed )
{
    /* Always assume RETAIL environment for Wine */
    const char *Id = "RETAIL";

    TRACE( "iface %p, sandboxIdSize %d, sandboxId %p, sandboxIdUsed %p\n", iface, sandboxIdSize, sandboxId, sandboxIdUsed );

    if (!sandboxId || !sandboxIdUsed)
        return E_POINTER;

    if (sandboxIdSize < XSystemXboxLiveSandboxIdMaxBytes)
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

    strcpy_s( sandboxId, sandboxIdSize, Id );
    *sandboxIdUsed = strlen( Id ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_system_XSystemGetAppSpecificDeviceId( IXSystemImpl5 *iface, INT32 appSpecificDeviceIdSize, char *appSpecificDeviceId, SIZE_T *appSpecificDeviceIdUsed )
{
    /* Fake device ID — 44 chars + NUL = XSystemAppSpecificDeviceIdBytes */
    const char *id = "WINEEX000000000000000000000000000000000000000";
    TRACE( "iface %p\n", iface );
    if (!appSpecificDeviceId || !appSpecificDeviceIdUsed) return E_POINTER;
    if (appSpecificDeviceIdSize < (INT32)XSystemAppSpecificDeviceIdBytes)
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    strcpy_s( appSpecificDeviceId, appSpecificDeviceIdSize, id );
    *appSpecificDeviceIdUsed = strlen( id ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_system_XSystemHandleTrack( IXSystemImpl5 *iface, XSystemHandleCallback callback, void *context )
{
    TRACE( "iface %p, callback %p, context %p\n", iface, callback, context );
    return S_OK;
}

static BOOLEAN WINAPI x_system_XSystemIsHandleValid( IXSystemImpl5 *iface, XSystemHandle handle )
{
    TRACE( "iface %p, handle %p\n", iface, handle );
    return handle != NULL;
}

static void WINAPI x_system_XSystemAllowFullDownloadBandwidth( IXSystemImpl5 *iface, BOOLEAN enable )
{
    TRACE( "iface %p, enable %d\n", iface, enable );
}

static const struct IXSystemImpl5Vtbl x_system_vtbl =
{
    x_system_QueryInterface,
    x_system_AddRef,
    x_system_Release,
    /* IXSystemImpl/IXSystemImpl2 methods */
    x_system_XSystemGetConsoleId,
    x_system_XSystemGetXboxLiveSandboxId,
    x_system_XSystemGetAppSpecificDeviceId,
    /* IXSystemImpl3 methods */
    x_system_XSystemHandleTrack,
    x_system_XSystemIsHandleValid,
    /* IXSystemImpl4 methods */
    x_system_XSystemAllowFullDownloadBandwidth,
};

static struct x_system x_system =
{
    {&x_system_vtbl},
    0,
};

IXSystemImpl *x_system_impl = (IXSystemImpl *)&x_system.IXSystemImpl5_iface;
