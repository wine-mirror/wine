/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
#include "wine/port.h"

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_QueryInterface(IDXGIOutput *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDXGIOutput)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_output_AddRef(IDXGIOutput *iface)
{
    struct dxgi_output *This = (struct dxgi_output *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_output_Release(IDXGIOutput *iface)
{
    struct dxgi_output *This = (struct dxgi_output *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u.\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateData(IDXGIOutput *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateDataInterface(IDXGIOutput *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetPrivateData(IDXGIOutput *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetParent(IDXGIOutput *iface,
        REFIID riid, void **parent)
{
    struct dxgi_output *This = (struct dxgi_output *)iface;

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IDXGIAdapter_QueryInterface((IDXGIAdapter *)This->adapter, riid, parent);
}

/* IDXGIOutput methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDesc(IDXGIOutput *iface, DXGI_OUTPUT_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList(IDXGIOutput *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC *desc)
{
    struct dxgi_output *This = (struct dxgi_output *)iface;
    enum wined3d_format_id wined3d_format;
    IWineD3D *wined3d;
    UINT i;

    TRACE("iface %p, format %s, flags %#x, mode_count %p, desc %p.\n",
            iface, debug_dxgi_format(format), flags, mode_count, desc);

    wined3d = IWineDXGIFactory_get_wined3d(This->adapter->parent);
    wined3d_format = wined3dformat_from_dxgi_format(format);

    if (!desc)
    {
        EnterCriticalSection(&dxgi_cs);
        *mode_count = IWineD3D_GetAdapterModeCount(wined3d, This->adapter->ordinal, wined3d_format);
        IWineD3D_Release(wined3d);
        LeaveCriticalSection(&dxgi_cs);

        return S_OK;
    }

    EnterCriticalSection(&dxgi_cs);
    for (i = 0; i < *mode_count; ++i)
    {
        WINED3DDISPLAYMODE mode;
        HRESULT hr;

        hr = IWineD3D_EnumAdapterModes(wined3d, This->adapter->ordinal, wined3d_format, i, &mode);
        if (FAILED(hr))
        {
            WARN("EnumAdapterModes failed, hr %#x.\n", hr);
            IWineD3D_Release(wined3d);
            LeaveCriticalSection(&dxgi_cs);
            return hr;
        }

        desc[i].Width = mode.Width;
        desc[i].Height = mode.Height;
        desc[i].RefreshRate.Numerator = mode.RefreshRate;
        desc[i].RefreshRate.Denominator = 1;
        desc[i].Format = format;
        desc[i].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; /* FIXME */
        desc[i].Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
    }
    IWineD3D_Release(wined3d);
    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode(IDXGIOutput *iface,
        const DXGI_MODE_DESC *mode, DXGI_MODE_DESC *closest_match, IUnknown *device)
{
    FIXME("iface %p, mode %p, closest_match %p, device %p stub!\n", iface, mode, closest_match, device);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_WaitForVBlank(IDXGIOutput *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_TakeOwnership(IDXGIOutput *iface, IUnknown *device, BOOL exclusive)
{
    FIXME("iface %p, device %p, exclusive %d stub!\n", iface, device, exclusive);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_output_ReleaseOwnership(IDXGIOutput *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControlCapabilities(IDXGIOutput *iface,
        DXGI_GAMMA_CONTROL_CAPABILITIES *gamma_caps)
{
    FIXME("iface %p, gamma_caps %p stub!\n", iface, gamma_caps);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetGammaControl(IDXGIOutput *iface,
        const DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControl(IDXGIOutput *iface, DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetDisplaySurface(IDXGIOutput *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData(IDXGIOutput *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetFrameStatistics(IDXGIOutput *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static const struct IDXGIOutputVtbl dxgi_output_vtbl =
{
    dxgi_output_QueryInterface,
    dxgi_output_AddRef,
    dxgi_output_Release,
    /* IDXGIObject methods */
    dxgi_output_SetPrivateData,
    dxgi_output_SetPrivateDataInterface,
    dxgi_output_GetPrivateData,
    dxgi_output_GetParent,
    /* IDXGIOutput methods */
    dxgi_output_GetDesc,
    dxgi_output_GetDisplayModeList,
    dxgi_output_FindClosestMatchingMode,
    dxgi_output_WaitForVBlank,
    dxgi_output_TakeOwnership,
    dxgi_output_ReleaseOwnership,
    dxgi_output_GetGammaControlCapabilities,
    dxgi_output_SetGammaControl,
    dxgi_output_GetGammaControl,
    dxgi_output_SetDisplaySurface,
    dxgi_output_GetDisplaySurfaceData,
    dxgi_output_GetFrameStatistics,
};

void dxgi_output_init(struct dxgi_output *output, struct dxgi_adapter *adapter)
{
    output->vtbl = &dxgi_output_vtbl;
    output->refcount = 1;
    output->adapter = adapter;
}
