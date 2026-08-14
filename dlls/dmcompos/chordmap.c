/* IDirectMusicChordMap Implementation
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

#include "dmcompos_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmcompos);

/*****************************************************************************
 * IDirectMusicChordMapImpl implementation
 */
typedef struct IDirectMusicChordMapImpl {
    IDirectMusicChordMap IDirectMusicChordMap_iface;
    struct dmobject dmobj;
    LONG  ref;
} IDirectMusicChordMapImpl;

/* IDirectMusicChordMapImpl IDirectMusicChordMap part: */
static inline IDirectMusicChordMapImpl *impl_from_IDirectMusicChordMap(IDirectMusicChordMap *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicChordMapImpl, IDirectMusicChordMap_iface);
}

static HRESULT WINAPI IDirectMusicChordMapImpl_QueryInterface(IDirectMusicChordMap *iface,
        REFIID riid, void **ret_iface)
{
    IDirectMusicChordMapImpl *This = impl_from_IDirectMusicChordMap(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicChordMap))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicChordMapImpl_AddRef(IDirectMusicChordMap *iface)
{
    IDirectMusicChordMapImpl *This = impl_from_IDirectMusicChordMap(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicChordMapImpl_Release(IDirectMusicChordMap *iface)
{
    IDirectMusicChordMapImpl *This = impl_from_IDirectMusicChordMap(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI IDirectMusicChordMapImpl_GetScale(IDirectMusicChordMap *iface,
        DWORD *pdwScale)
{
    IDirectMusicChordMapImpl *This = impl_from_IDirectMusicChordMap(iface);
    FIXME("(%p, %p): stub\n", This, pdwScale);
    return S_OK;
}

static const IDirectMusicChordMapVtbl dmchordmap_vtbl = {
    IDirectMusicChordMapImpl_QueryInterface,
    IDirectMusicChordMapImpl_AddRef,
    IDirectMusicChordMapImpl_Release,
    IDirectMusicChordMapImpl_GetScale
};

/* IDirectMusicChordMapImpl IDirectMusicObject part: */
static HRESULT WINAPI chord_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_CHORDMAP_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc, DMUS_OBJ_OBJECT);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicChordMap;
    desc->dwValidData |= DMUS_OBJ_CLASS;

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
    chord_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicChordMapImpl IPersistStream part: */
static inline IDirectMusicChordMapImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicChordMapImpl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI chord_persist_stream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicChordMapImpl *This = impl_from_IPersistStream(iface);
    struct chunk_entry chordmap = {0};
    struct chunk_entry chunk = {.parent = &chordmap};
    HRESULT hr;

    FIXME("(%p, %p): Loading not implemented yet\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &chordmap) != S_OK || chordmap.id != FOURCC_RIFF ||
            chordmap.type != DMUS_FOURCC_CHORDMAP_FORM)
    {
        WARN("Invalid chunk %s\n", debugstr_chunk(&chordmap));
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    if (FAILED(hr = dmobj_parsedescriptor(stream, &chordmap, &This->dmobj.desc, DMUS_OBJ_OBJECT))
                || FAILED(hr = stream_reset_chunk_data(stream, &chordmap)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
            case DMUS_FOURCC_GUID_CHUNK:
            case DMUS_FOURCC_VERSION_CHUNK:
            case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
                /* already parsed/ignored by dmobj_parsedescriptor */
                break;

            default:
                FIXME("Ignoring chunk %s\n", debugstr_chunk(&chunk));
                break;
        }

        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    chord_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmchordmap(REFIID lpcGUID, void **ppobj)
{
    IDirectMusicChordMapImpl* obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicChordMap_iface.lpVtbl = &dmchordmap_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicChordMap,
            (IUnknown *)&obj->IDirectMusicChordMap_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

    hr = IDirectMusicChordMap_QueryInterface(&obj->IDirectMusicChordMap_iface, lpcGUID, ppobj);
    IDirectMusicChordMap_Release(&obj->IDirectMusicChordMap_iface);

    return hr;
}
