/*
* Copyright 2026 Alistair Leslie-Hughes
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
#include <initguid.h>
#include "oleidl.h"
#include "rpcproxy.h"
#include "rtscom.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rtscom);


struct rtstylus
{
    IRealTimeStylus IRealTimeStylus_iface;
    LONG ref;
};

static struct rtstylus *impl_from_IRealTimeStylus( IRealTimeStylus *iface )
{
    return CONTAINING_RECORD( iface, struct rtstylus, IRealTimeStylus_iface );
}

static HRESULT WINAPI rtstylus_QueryInterface(IRealTimeStylus *iface, REFIID iid, void **obj)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IRealTimeStylus ))
    {
        IRealTimeStylus_AddRef( iface );
        *obj = &This->IRealTimeStylus_iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI rtstylus_AddRef(IRealTimeStylus *iface)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI rtstylus_Release(IRealTimeStylus *iface)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
    {
        free( This );
    }

    return ref;
}

static HRESULT WINAPI rtstylus_get_Enabled(IRealTimeStylus *iface, BOOL *pfEnable)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, pfEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_put_Enabled(IRealTimeStylus *iface, BOOL fEnable)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %d\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_get_HWND(IRealTimeStylus *iface, HANDLE_PTR *phwnd)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_put_HWND(IRealTimeStylus *iface, HANDLE_PTR hwnd)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %Ix\n", This, hwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_get_WindowInputRectangle(IRealTimeStylus *iface, RECT *prcWndInputRect)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, prcWndInputRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_put_WindowInputRectangle(IRealTimeStylus *iface, const RECT *prcWndInputRect)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, prcWndInputRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_AddStylusSyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusSyncPlugin *piPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, piPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_RemoveStylusSyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusSyncPlugin **ppiPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, ppiPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_RemoveAllStylusSyncPlugins(IRealTimeStylus *iface)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStylusSyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusSyncPlugin **ppiPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, ppiPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStylusSyncPluginCount(IRealTimeStylus *iface, ULONG *pcPlugins)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, pcPlugins);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_AddStylusAsyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusAsyncPlugin *piPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, piPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_RemoveStylusAsyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusAsyncPlugin **ppiPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, ppiPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_RemoveAllStylusAsyncPlugins(IRealTimeStylus *iface)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStylusAsyncPlugin(IRealTimeStylus *iface, ULONG iIndex, IStylusAsyncPlugin **ppiPlugin)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, iIndex, ppiPlugin);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStylusAsyncPluginCount(IRealTimeStylus *iface, ULONG *pcPlugins)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, pcPlugins);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_get_ChildRealTimeStylusPlugin(IRealTimeStylus *iface, IRealTimeStylus **ppiRTS)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, ppiRTS);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_putref_ChildRealTimeStylusPlugin(IRealTimeStylus *iface, IRealTimeStylus *piRTS)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, piRTS);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_AddCustomStylusDataToQueue(IRealTimeStylus *iface, StylusQueue sq, const GUID *pGuidId,
                ULONG cbData, BYTE *pbData)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %d, %p, %lu, %p\n", This, sq, pGuidId, cbData, pbData);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_ClearStylusQueues(IRealTimeStylus *iface)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_SetAllTabletsMode(IRealTimeStylus *iface, BOOL fUseMouseForInput)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %d\n", This, fUseMouseForInput);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_SetSingleTabletMode(IRealTimeStylus *iface, IInkTablet *piTablet)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, piTablet);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetTablet(IRealTimeStylus *iface, IInkTablet **ppiSingleTablet)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, ppiSingleTablet);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetTabletContextIdFromTablet(IRealTimeStylus *iface, IInkTablet *piTablet, TABLET_CONTEXT_ID *ptcid)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p, %p\n", This, piTablet, ptcid);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetTabletFromTabletContextId(IRealTimeStylus *iface, TABLET_CONTEXT_ID tcid, IInkTablet **ppiTablet)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %ld, %p\n", This, tcid, ppiTablet);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetAllTabletContextIds(IRealTimeStylus *iface, ULONG *pcTcidCount, TABLET_CONTEXT_ID **ppTcids)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p, %p\n", This, pcTcidCount, ppTcids);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStyluses(IRealTimeStylus *iface, IInkCursors **ppiInkCursors)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p\n", This, ppiInkCursors);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetStylusForId(IRealTimeStylus *iface, STYLUS_ID sid, IInkCursor **ppiInkCursor)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %ld, %p\n", This, sid, ppiInkCursor);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_SetDesiredPacketDescription(IRealTimeStylus *iface, ULONG cProperties, const GUID *pPropertyGuids)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p\n", This, cProperties, pPropertyGuids);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetDesiredPacketDescription(IRealTimeStylus *iface, ULONG *pcProperties, GUID **ppPropertyGuids)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %p, %p\n", This, pcProperties, ppPropertyGuids);
    return E_NOTIMPL;
}

static HRESULT WINAPI rtstylus_GetPacketDescriptionData(IRealTimeStylus *iface, TABLET_CONTEXT_ID tcid, FLOAT *pfInkToDeviceScaleX,
    FLOAT *pfInkToDeviceScaleY, ULONG *pcPacketProperties, PACKET_PROPERTY **ppPacketProperties)
{
    struct rtstylus *This = impl_from_IRealTimeStylus(iface);
    FIXME("%p, %lu, %p, %p, %p, %p\n", This, tcid, pfInkToDeviceScaleX, pfInkToDeviceScaleY, pcPacketProperties, ppPacketProperties);
    return E_NOTIMPL;
}

static const struct IRealTimeStylusVtbl realtime_stylus_vtbl =
{
    rtstylus_QueryInterface,
    rtstylus_AddRef,
    rtstylus_Release,
    rtstylus_get_Enabled,
    rtstylus_put_Enabled,
    rtstylus_get_HWND,
    rtstylus_put_HWND,
    rtstylus_get_WindowInputRectangle,
    rtstylus_put_WindowInputRectangle,
    rtstylus_AddStylusSyncPlugin,
    rtstylus_RemoveStylusSyncPlugin,
    rtstylus_RemoveAllStylusSyncPlugins,
    rtstylus_GetStylusSyncPlugin,
    rtstylus_GetStylusSyncPluginCount,
    rtstylus_AddStylusAsyncPlugin,
    rtstylus_RemoveStylusAsyncPlugin,
    rtstylus_RemoveAllStylusAsyncPlugins,
    rtstylus_GetStylusAsyncPlugin,
    rtstylus_GetStylusAsyncPluginCount,
    rtstylus_get_ChildRealTimeStylusPlugin,
    rtstylus_putref_ChildRealTimeStylusPlugin,
    rtstylus_AddCustomStylusDataToQueue,
    rtstylus_ClearStylusQueues,
    rtstylus_SetAllTabletsMode,
    rtstylus_SetSingleTabletMode,
    rtstylus_GetTablet,
    rtstylus_GetTabletContextIdFromTablet,
    rtstylus_GetTabletFromTabletContextId,
    rtstylus_GetAllTabletContextIds,
    rtstylus_GetStyluses,
    rtstylus_GetStylusForId,
    rtstylus_SetDesiredPacketDescription,
    rtstylus_GetDesiredPacketDescription,
    rtstylus_GetPacketDescriptionData
};

static HRESULT realtime_stylus_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct rtstylus *This = malloc( sizeof(*This) );
    HRESULT hr = E_FAIL;

    if (!This) return E_OUTOFMEMORY;
    This->IRealTimeStylus_iface.lpVtbl = &realtime_stylus_vtbl;
    This->ref = 1;

    hr = IRealTimeStylus_QueryInterface( &This->IRealTimeStylus_iface, iid, obj );
    IRealTimeStylus_Release( &This->IRealTimeStylus_iface );
    return hr;
}

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

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface, BOOL lock )
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

static struct class_factory runtime_styus_cf = { { &class_factory_vtbl }, realtime_stylus_create };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    IClassFactory *cf = NULL;

    TRACE( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (IsEqualCLSID( clsid, &CLSID_RealTimeStylus ))
        cf = &runtime_styus_cf.IClassFactory_iface;
    else
        FIXME( "(%s %s %p) unsupported interface\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}
