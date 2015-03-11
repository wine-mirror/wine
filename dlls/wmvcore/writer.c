/*
 * Copyright 2015 Jacek Caban for CodeWeavers
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

#include "wmvcore.h"
#include "wmsdkidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

typedef struct {
    IWMWriter IWMWriter_iface;
    LONG ref;
} WMWriter;

static inline WMWriter *impl_from_IWMWriter(IWMWriter *iface)
{
    return CONTAINING_RECORD(iface, WMWriter, IWMWriter_iface);
}

static HRESULT WINAPI WMWriter_QueryInterface(IWMWriter *iface, REFIID riid, void **ppv)
{
    WMWriter *This = impl_from_IWMWriter(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMWriter_iface;
    }else if(IsEqualGUID(&IID_IWMWriter, riid)) {
        TRACE("(%p)->(IID_IWMWriter %p)\n", This, ppv);
        *ppv = &This->IWMWriter_iface;
    }else {
        FIXME("Unsupported iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMWriter_AddRef(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMWriter_Release(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMWriter_SetProfileByID(IWMWriter *iface, REFGUID guidProfile)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_guid(guidProfile));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetProfile(IWMWriter *iface, IWMProfile *profile)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%p)\n", This, profile);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetOutputFilename(IWMWriter *iface, const WCHAR *filename)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(filename));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputCount(IWMWriter *iface, DWORD *pcInputs)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%p)\n", This, pcInputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputProps(IWMWriter *iface, DWORD dwInputNum, IWMInputMediaProps **input)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNum, input);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_SetInputProps(IWMWriter *iface, DWORD dwInputNum, IWMInputMediaProps *input)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNum, input);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputFormatCount(IWMWriter *iface, DWORD dwInputNumber, DWORD *pcFormat)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, dwInputNumber, pcFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_GetInputFormat(IWMWriter *iface, DWORD dwInputNumber, DWORD dwFormatNumber,
        IWMInputMediaProps **props)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwInputNumber, dwFormatNumber, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_BeginWriting(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_EndWriting(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_AllocateSample(IWMWriter *iface, DWORD size, INSSBuffer **sample)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %p)\n", This, size, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_WriteSample(IWMWriter *iface, DWORD dwInputNum, QWORD cnsSampleTime,
        DWORD flags, INSSBuffer *sample)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)->(%d %s %x %p)\n", This, dwInputNum, wine_dbgstr_longlong(cnsSampleTime), flags, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMWriter_Flush(IWMWriter *iface)
{
    WMWriter *This = impl_from_IWMWriter(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IWMWriterVtbl WMWriterVtbl = {
    WMWriter_QueryInterface,
    WMWriter_AddRef,
    WMWriter_Release,
    WMWriter_SetProfileByID,
    WMWriter_SetProfile,
    WMWriter_SetOutputFilename,
    WMWriter_GetInputCount,
    WMWriter_GetInputProps,
    WMWriter_SetInputProps,
    WMWriter_GetInputFormatCount,
    WMWriter_GetInputFormat,
    WMWriter_BeginWriting,
    WMWriter_EndWriting,
    WMWriter_AllocateSample,
    WMWriter_WriteSample,
    WMWriter_Flush
};

HRESULT WINAPI WMCreateWriter(IUnknown *reserved, IWMWriter **writer)
{
    WMWriter *ret;

    TRACE("(%p %p)\n", reserved, writer);

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IWMWriter_iface.lpVtbl = &WMWriterVtbl;
    ret->ref = 1;

    *writer = &ret->IWMWriter_iface;
    return S_OK;
}
