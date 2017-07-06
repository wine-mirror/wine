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
#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "mfreadwrite.h"

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


HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource *source, IMFAttributes *attributes,
                                                   IMFSourceReader **reader)
{
    FIXME("%p %p %p stub.\n", source, attributes, reader);

    return E_NOTIMPL;
}

typedef struct _srcreader
{
    IMFSourceReader IMFSourceReader_iface;
    LONG ref;
} srcreader;

static inline srcreader *impl_from_IMFSourceReader(IMFSourceReader *iface)
{
    return CONTAINING_RECORD(iface, srcreader, IMFSourceReader_iface);
}

static HRESULT WINAPI src_reader_QueryInterface(IMFSourceReader *iface, REFIID riid, void **out)
{
    srcreader *This = impl_from_IMFSourceReader(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IMFSourceReader))
    {
        *out = &This->IMFSourceReader_iface;
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

static ULONG WINAPI src_reader_AddRef(IMFSourceReader *iface)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI src_reader_Release(IMFSourceReader *iface)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI src_reader_GetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL *selected)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p\n", This, index, selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetStreamSelection(IMFSourceReader *iface, DWORD index, BOOL selected)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %d\n", This, index, selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetNativeMediaType(IMFSourceReader *iface, DWORD index, DWORD typeindex,
            IMFMediaType **type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %d, %p\n", This, index, typeindex, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetCurrentMediaType(IMFSourceReader *iface, DWORD index, IMFMediaType **type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p\n", This, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentMediaType(IMFSourceReader *iface, DWORD index, DWORD *reserved,
        IMFMediaType *type)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %p, %p\n", This, index, reserved, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_SetCurrentPosition(IMFSourceReader *iface, REFGUID format, REFPROPVARIANT position)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(format), position);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_ReadSample(IMFSourceReader *iface, DWORD index,
        DWORD flags, DWORD *actualindex, DWORD *sampleflags, LONGLONG *timestamp,
        IMFSample **sample)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, 0x%08x, %p, %p, %p, %p\n", This, index, flags, actualindex,
          sampleflags, timestamp, sample);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_Flush(IMFSourceReader *iface, DWORD index)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetServiceForStream(IMFSourceReader *iface, DWORD index, REFGUID service,
        REFIID riid, void **object)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %s, %s, %p\n", This, index, debugstr_guid(service), debugstr_guid(riid), object);
    return E_NOTIMPL;
}

static HRESULT WINAPI src_reader_GetPresentationAttribute(IMFSourceReader *iface, DWORD index,
        REFGUID guid, PROPVARIANT *attr)
{
    srcreader *This = impl_from_IMFSourceReader(iface);
    FIXME("%p, 0x%08x, %s, %p\n", This, index, debugstr_guid(guid), attr);
    return E_NOTIMPL;
}

struct IMFSourceReaderVtbl srcreader_vtbl =
{
    src_reader_QueryInterface,
    src_reader_AddRef,
    src_reader_Release,
    src_reader_GetStreamSelection,
    src_reader_SetStreamSelection,
    src_reader_GetNativeMediaType,
    src_reader_GetCurrentMediaType,
    src_reader_SetCurrentMediaType,
    src_reader_SetCurrentPosition,
    src_reader_ReadSample,
    src_reader_Flush,
    src_reader_GetServiceForStream,
    src_reader_GetPresentationAttribute
};

HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream *stream, IMFAttributes *attributes, IMFSourceReader **reader)
{
    srcreader *object;

    TRACE("%p, %p, %p\n", stream, attributes, reader);

    object = HeapAlloc( GetProcessHeap(), 0, sizeof(*object) );
    if(!object)
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IMFSourceReader_iface.lpVtbl = &srcreader_vtbl;

    *reader = &object->IMFSourceReader_iface;
    return S_OK;
}
