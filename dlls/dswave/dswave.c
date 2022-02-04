/* IDirectMusicWave Implementation
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

#include "dswave_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dswave);

/* an interface that is, according to my tests, obtained by loader after loading object; is it acting
   as some sort of bridge between object and loader? */
static const GUID IID_IDirectMusicWavePRIVATE = {0x69e934e4,0x97f1,0x4f1d,{0x88,0xe8,0xf2,0xac,0x88,0x67,0x13,0x27}};

/*****************************************************************************
 * IDirectMusicWaveImpl implementation
 */
typedef struct IDirectMusicWaveImpl {
    IUnknown IUnknown_iface;
    struct dmobject dmobj;
    LONG ref;
} IDirectMusicWaveImpl;

/* IDirectMusicWaveImpl IUnknown part: */
static inline IDirectMusicWaveImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicWaveImpl, IUnknown_iface);
}

static HRESULT WINAPI IUnknownImpl_QueryInterface(IUnknown *iface, REFIID riid, void **ret_iface)
{
    IDirectMusicWaveImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicWavePRIVATE)) {
        FIXME("(%p, %s, %p): Unsupported private interface\n", This, debugstr_guid(riid), ret_iface);
        return E_NOINTERFACE;
    } else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IUnknownImpl_AddRef(IUnknown *iface)
{
    IDirectMusicWaveImpl *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IUnknownImpl_Release(IUnknown *iface)
{
    IDirectMusicWaveImpl *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
        DSWAVE_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl unknown_vtbl = {
    IUnknownImpl_QueryInterface,
    IUnknownImpl_AddRef,
    IUnknownImpl_Release
};

/* IDirectMusicWaveImpl IDirectMusicObject part: */
static HRESULT WINAPI wave_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != mmioFOURCC('W','A','V','E')) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_NAME_INFO | DMUS_OBJ_OBJECT | DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    wave_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicWaveImpl IPersistStream part: */
static inline IDirectMusicWaveImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicWaveImpl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI wave_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicWaveImpl *This = impl_from_IPersistStream(iface);
    struct chunk_entry riff = {0};

    /* Without the private interface the implementation should go to dmime/segment.c */
    FIXME("(%p, %p): loading not implemented (only descriptor is loaded)\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &riff) != S_OK || riff.id != FOURCC_RIFF ||
            riff.type != mmioFOURCC('W','A','V','E'))
        return DMUS_E_UNSUPPORTED_STREAM;
    stream_reset_chunk_start(stream, &riff);

    return IDirectMusicObject_ParseDescriptor(&This->dmobj.IDirectMusicObject_iface, stream,
            &This->dmobj.desc);
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    wave_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI create_dswave(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicWaveImpl *obj;
    HRESULT hr;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectMusicWaveImpl));
    if (!obj) {
        *ppobj = NULL;
        return E_OUTOFMEMORY;
    }
    obj->IUnknown_iface.lpVtbl = &unknown_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectSoundWave, &obj->IUnknown_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    DSWAVE_LockModule();
    hr = IUnknown_QueryInterface(&obj->IUnknown_iface, lpcGUID, ppobj);
    IUnknown_Release(&obj->IUnknown_iface);
    return hr;
}
