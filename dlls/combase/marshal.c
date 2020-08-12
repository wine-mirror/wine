/*
 *     Copyright 2002 Marcus Meissner
 *     Copyright 2004 Mike Hearn, for CodeWeavers
 *     Copyright 2004 Rob Shearman, for CodeWeavers
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
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *            CoMarshalHresult        (combase.@)
 */
HRESULT WINAPI CoMarshalHresult(IStream *stream, HRESULT hresult)
{
    return IStream_Write(stream, &hresult, sizeof(hresult), NULL);
}

/***********************************************************************
 *            CoUnmarshalHresult      (combase.@)
 */
HRESULT WINAPI CoUnmarshalHresult(IStream *stream, HRESULT *phresult)
{
    return IStream_Read(stream, phresult, sizeof(*phresult), NULL);
}

/***********************************************************************
 *            CoGetInterfaceAndReleaseStream    (combase.@)
 */
HRESULT WINAPI CoGetInterfaceAndReleaseStream(IStream *stream, REFIID riid, void **obj)
{
    HRESULT hr;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), obj);

    if (!stream) return E_INVALIDARG;
    hr = CoUnmarshalInterface(stream, riid, obj);
    IStream_Release(stream);
    return hr;
}

/***********************************************************************
 *            CoMarshalInterThreadInterfaceInStream    (combase.@)
 */
HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, IUnknown *unk, IStream **stream)
{
    ULARGE_INTEGER xpos;
    LARGE_INTEGER seekto;
    HRESULT hr;

    TRACE("%s, %p, %p\n", debugstr_guid(riid), unk, stream);

    hr = CreateStreamOnHGlobal(NULL, TRUE, stream);
    if (FAILED(hr)) return hr;
    hr = CoMarshalInterface(*stream, riid, unk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);

    if (SUCCEEDED(hr))
    {
        memset(&seekto, 0, sizeof(seekto));
        IStream_Seek(*stream, seekto, STREAM_SEEK_SET, &xpos);
    }
    else
    {
        IStream_Release(*stream);
        *stream = NULL;
    }

    return hr;
}
