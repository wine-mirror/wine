/*
 * IDirectMusicInstrument Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

static const GUID IID_IDirectMusicInstrumentPRIVATE = { 0xbcb20080, 0xa40c, 0x11d1, { 0x86, 0xbc, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef } };

/* IDirectMusicInstrument IUnknown part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_QueryInterface(LPDIRECTMUSICINSTRUMENT iface, REFIID riid, LPVOID *ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicInstrument))
    {
        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicInstrumentPRIVATE))
    {
        /* it seems to me that this interface is only basic IUnknown, without any
         * other inherited functions... *sigh* this is the worst scenario, since it means
         * that whoever calls it knows the layout of original implementation table and therefore
         * tries to get data by direct access... expect crashes
         */
        FIXME("*sigh*... requested private/unspecified interface\n");

        *ret_iface = iface;
        IDirectMusicInstrument_AddRef(iface);
        return S_OK;
    }

    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);

    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_AddRef(LPDIRECTMUSICINSTRUMENT iface)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    DMUSIC_LockModule();

    return ref;
}

static ULONG WINAPI IDirectMusicInstrumentImpl_Release(LPDIRECTMUSICINSTRUMENT iface)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    DMUSIC_UnlockModule();

    return ref;
}

/* IDirectMusicInstrumentImpl IDirectMusicInstrument part: */
static HRESULT WINAPI IDirectMusicInstrumentImpl_GetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD* pdwPatch)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%p)\n", This, pdwPatch);

    *pdwPatch = MIDILOCALE2Patch(&This->pHeader->Locale);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicInstrumentImpl_SetPatch(LPDIRECTMUSICINSTRUMENT iface, DWORD dwPatch)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);

    TRACE("(%p)->(%d): stub\n", This, dwPatch);

    Patch2MIDILOCALE(dwPatch, &This->pHeader->Locale);

    return S_OK;
}

static const IDirectMusicInstrumentVtbl DirectMusicInstrument_Vtbl =
{
    IDirectMusicInstrumentImpl_QueryInterface,
    IDirectMusicInstrumentImpl_AddRef,
    IDirectMusicInstrumentImpl_Release,
    IDirectMusicInstrumentImpl_GetPatch,
    IDirectMusicInstrumentImpl_SetPatch
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicInstrumentImpl* dminst;
	
	dminst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicInstrumentImpl));
	if (NULL == dminst) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	dminst->IDirectMusicInstrument_iface.lpVtbl = &DirectMusicInstrument_Vtbl;
	dminst->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicInstrument_QueryInterface(&dminst->IDirectMusicInstrument_iface, lpcGUID, ppobj);
}

static HRESULT read_from_stream(IStream *stream, void *data, ULONG size)
{
    ULONG bytes_read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &bytes_read);
    if(FAILED(hr)){
        TRACE("IStream_Read failed: %08x\n", hr);
        return hr;
    }
    if (bytes_read < size) {
        TRACE("Didn't read full chunk: %u < %u\n", bytes_read, size);
        return E_FAIL;
    }

    return S_OK;
}

static inline DWORD subtract_bytes(DWORD len, DWORD bytes)
{
    if(bytes > len){
        TRACE("Apparent mismatch in chunk lengths? %u bytes remaining, %u bytes read\n", len, bytes);
        return 0;
    }
    return len - bytes;
}

static HRESULT load_instrument(IDirectMusicInstrumentImpl *This, IStream *stream, DWORD length)
{
    HRESULT hr;
    FOURCC fourcc;
    DWORD bytes;
    LARGE_INTEGER move;

    while(length){
        hr = read_from_stream(stream, &fourcc, sizeof(fourcc));
        if(FAILED(hr))
            return hr;

        hr = read_from_stream(stream, &bytes, sizeof(bytes));
        if(FAILED(hr))
            return hr;

        length = subtract_bytes(length, sizeof(fourcc) + sizeof(bytes));

        switch(fourcc){
        case FOURCC_INSH:
            TRACE("INSH chunk: %u bytes\n", bytes);
            hr = read_from_stream(stream, This->pHeader, sizeof(*This->pHeader));
            if(FAILED(hr))
                return hr;

            move.QuadPart = bytes - sizeof(*This->pHeader);
            hr = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
            if(FAILED(hr)){
                WARN("IStream_Seek failed: %08x\n", hr);
                return hr;
            }

            length = subtract_bytes(length, bytes);
            break;

        case FOURCC_DLID:
            TRACE("DLID chunk: %u bytes\n", bytes);
            hr = read_from_stream(stream, This->pInstrumentID, sizeof(*This->pInstrumentID));
            if(FAILED(hr))
                return hr;

            move.QuadPart = bytes - sizeof(*This->pInstrumentID);
            hr = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
            if(FAILED(hr)){
                WARN("IStream_Seek failed: %08x\n", hr);
                return hr;
            }

            length = subtract_bytes(length, bytes);
            break;

        default:
            TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(fourcc), bytes);

            move.QuadPart = bytes;
            hr = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
            if(FAILED(hr)){
                WARN("IStream_Seek failed: %08x\n", hr);
                return hr;
            }

            length = subtract_bytes(length, bytes);
            break;
        }
    }

    return S_OK;
}

/* aux. function that completely loads instrument; my tests indicate that it's 
   called somewhere around IDirectMusicCollection_GetInstrument */
HRESULT IDirectMusicInstrumentImpl_Custom_Load(LPDIRECTMUSICINSTRUMENT iface, LPSTREAM stream)
{
    IDirectMusicInstrumentImpl *This = impl_from_IDirectMusicInstrument(iface);
    LARGE_INTEGER move;
    FOURCC fourcc;
    DWORD bytes;
    HRESULT hr;

    TRACE("(%p, %p, offset = %s)\n", This, stream, wine_dbgstr_longlong(This->liInstrumentPosition.QuadPart));

    hr = IStream_Seek(stream, This->liInstrumentPosition, STREAM_SEEK_SET, NULL);
    if(FAILED(hr)){
        WARN("IStream_Seek failed: %08x\n", hr);
        goto load_failure;
    }

    hr = read_from_stream(stream, &fourcc, sizeof(fourcc));
    if(FAILED(hr))
        goto load_failure;

    if(fourcc != FOURCC_LIST){
        WARN("Loading failed: Expected LIST chunk, got: %s\n", debugstr_fourcc(fourcc));
        goto load_failure;
    }

    hr = read_from_stream(stream, &bytes, sizeof(bytes));
    if(FAILED(hr))
        goto load_failure;

    TRACE("LIST chunk: %u bytes\n", bytes);
    while(1){
        hr = read_from_stream(stream, &fourcc, sizeof(fourcc));
        if(FAILED(hr))
            goto load_failure;

        switch(fourcc){
        case FOURCC_INS:
            TRACE("INS  chunk: (no byte count)\n");
            hr = load_instrument(This, stream, bytes - sizeof(FOURCC));
            if(FAILED(hr))
                goto load_failure;
            break;

        default:
            hr = read_from_stream(stream, &bytes, sizeof(bytes));
            if(FAILED(hr))
                goto load_failure;

            TRACE("Unknown chunk %s: %u bytes\n", debugstr_fourcc(fourcc), bytes);

            move.QuadPart = bytes;
            hr = IStream_Seek(stream, move, STREAM_SEEK_CUR, NULL);
            if(FAILED(hr)){
                WARN("IStream_Seek failed: %08x\n", hr);
                return hr;
            }

            break;
        }
    }

    return S_OK;

load_failure:
    return DMUS_E_UNSUPPORTED_STREAM;
}
