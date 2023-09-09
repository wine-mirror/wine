/*
 * IDirectMusicCollection Implementation
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
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

struct instrument_entry
{
    struct list entry;
    IDirectMusicInstrument *instrument;
};

struct collection
{
    IDirectMusicCollection IDirectMusicCollection_iface;
    struct dmobject dmobj;
    LONG ref;

    IStream *pStm; /* stream from which we load collection and later instruments */
    CHAR *szCopyright; /* FIXME: should probably be placed somewhere else */
    DLSHEADER *pHeader;
    /* pool table */
    POOLTABLE *pPoolTable;
    POOLCUE *pPoolCues;

    struct list instruments;
};

static inline struct collection *impl_from_IDirectMusicCollection(IDirectMusicCollection *iface)
{
    return CONTAINING_RECORD(iface, struct collection, IDirectMusicCollection_iface);
}

static inline struct collection *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct collection, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI collection_QueryInterface(IDirectMusicCollection *iface,
        REFIID riid, void **ret_iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicCollection))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else
    {
        WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI collection_AddRef(IDirectMusicCollection *iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI collection_Release(IDirectMusicCollection *iface)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", iface, ref);

    if (!ref)
    {
        struct instrument_entry *instrument_entry;
        void *next;

        LIST_FOR_EACH_ENTRY_SAFE(instrument_entry, next, &This->instruments, struct instrument_entry, entry)
        {
            list_remove(&instrument_entry->entry);
            IDirectMusicInstrument_Release(instrument_entry->instrument);
            free(instrument_entry);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI collection_GetInstrument(IDirectMusicCollection *iface,
        DWORD patch, IDirectMusicInstrument **instrument)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    struct instrument_entry *entry;
    DWORD inst_patch;
    HRESULT hr;

    TRACE("(%p, %lu, %p)\n", iface, patch, instrument);

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
    {
        if (FAILED(hr = IDirectMusicInstrument_GetPatch(entry->instrument, &inst_patch))) return hr;
        if (patch == inst_patch)
        {
            *instrument = entry->instrument;
            IDirectMusicInstrument_AddRef(entry->instrument);
            TRACE(": returning instrument %p\n", entry->instrument);
            return S_OK;
        }
    }

    TRACE(": instrument not found\n");
    return DMUS_E_INVALIDPATCH;
}

static HRESULT WINAPI collection_EnumInstrument(IDirectMusicCollection *iface,
        DWORD index, DWORD *patch, LPWSTR name, DWORD name_length)
{
    struct collection *This = impl_from_IDirectMusicCollection(iface);
    struct instrument_entry *entry;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %ld)\n", iface, index, patch, name, name_length);

    LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
    {
        if (index--) continue;
        if (FAILED(hr = IDirectMusicInstrument_GetPatch(entry->instrument, patch))) return hr;
        if (name)
        {
            struct instrument *instrument = impl_from_IDirectMusicInstrument(entry->instrument);
            DWORD length = min(lstrlenW(instrument->wszName), name_length - 1);
            memcpy(name, instrument->wszName, length * sizeof(WCHAR));
            name[length] = '\0';
        }
        return S_OK;
    }

    return S_FALSE;
}

static const IDirectMusicCollectionVtbl collection_vtbl =
{
    collection_QueryInterface,
    collection_AddRef,
    collection_Release,
    collection_GetInstrument,
    collection_EnumInstrument,
};

static HRESULT WINAPI collection_object_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != FOURCC_DLS) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_NOTADLSCOL;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc, DMUS_OBJ_NAME_INFO|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicCollection;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl collection_object_vtbl =
{
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    collection_object_ParseDescriptor,
};

static HRESULT WINAPI collection_stream_Load(IPersistStream *iface,
        IStream *stream)
{
    struct collection *This = impl_from_IPersistStream(iface);
    DMUS_PRIVATE_CHUNK chunk;
    DWORD StreamSize, StreamCount, ListSize[2], ListCount[2];
    LARGE_INTEGER liMove; /* used when skipping chunks */

    IStream_AddRef(stream); /* add count for later references */
    This->pStm = stream;

    IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
    TRACE_(dmfile)(": %s chunk (size = %#04lx)", debugstr_fourcc(chunk.fccID), chunk.dwSize);

    if (chunk.fccID != FOURCC_RIFF) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = chunk.dwSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return E_FAIL;
    }

    IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
    TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunk.fccID));
    StreamSize = chunk.dwSize - sizeof(FOURCC);
    StreamCount = 0;

    if (chunk.fccID != FOURCC_DLS) {
        TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
        liMove.QuadPart = StreamSize;
        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
        return E_FAIL;
    }

    TRACE_(dmfile)(": collection form\n");
    do {
        IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
        StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
        TRACE_(dmfile)(": %s chunk (size = %#04lx)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
        switch (chunk.fccID) {
            case FOURCC_COLH: {
                TRACE_(dmfile)(": collection header chunk\n");
                This->pHeader = calloc(1, chunk.dwSize);
                IStream_Read(stream, This->pHeader, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_DLID: {
                TRACE_(dmfile)(": DLID (GUID) chunk\n");
                This->dmobj.desc.dwValidData |= DMUS_OBJ_OBJECT;
                IStream_Read(stream, &This->dmobj.desc.guidObject, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_VERS: {
                TRACE_(dmfile)(": version chunk\n");
                This->dmobj.desc.dwValidData |= DMUS_OBJ_VERSION;
                IStream_Read(stream, &This->dmobj.desc.vVersion, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_PTBL: {
                TRACE_(dmfile)(": pool table chunk\n");
                This->pPoolTable = calloc(1, sizeof(POOLTABLE));
                IStream_Read(stream, This->pPoolTable, sizeof(POOLTABLE), NULL);
                chunk.dwSize -= sizeof(POOLTABLE);
                This->pPoolCues = calloc(This->pPoolTable->cCues, sizeof(POOLCUE));
                IStream_Read(stream, This->pPoolCues, chunk.dwSize, NULL);
                break;
            }
            case FOURCC_LIST: {
                IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
                TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                ListSize[0] = chunk.dwSize - sizeof(FOURCC);
                ListCount[0] = 0;
                switch (chunk.fccID) {
                    case DMUS_FOURCC_INFO_LIST: {
                        TRACE_(dmfile)(": INFO list\n");
                        do {
                            IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
                            ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                            TRACE_(dmfile)(": %s chunk (size = %#04lx)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                            switch (chunk.fccID) {
                                case mmioFOURCC('I','N','A','M'): {
                                    CHAR szName[DMUS_MAX_NAME];
                                    TRACE_(dmfile)(": name chunk\n");
                                    This->dmobj.desc.dwValidData |= DMUS_OBJ_NAME;
                                    IStream_Read(stream, szName, chunk.dwSize, NULL);
                                    MultiByteToWideChar(CP_ACP, 0, szName, -1, This->dmobj.desc.wszName, DMUS_MAX_NAME);
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        liMove.QuadPart = 1;
                                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    }
                                    break;
                                }
                                case mmioFOURCC('I','A','R','T'): {
                                    TRACE_(dmfile)(": artist chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                case mmioFOURCC('I','C','O','P'): {
                                    TRACE_(dmfile)(": copyright chunk\n");
                                    This->szCopyright = calloc(1, chunk.dwSize);
                                    IStream_Read(stream, This->szCopyright, chunk.dwSize, NULL);
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        liMove.QuadPart = 1;
                                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    }
                                    break;
                                }
                                case mmioFOURCC('I','S','B','J'): {
                                    TRACE_(dmfile)(": subject chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                case mmioFOURCC('I','C','M','T'): {
                                    TRACE_(dmfile)(": comment chunk (ignored)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                                default: {
                                    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                    if (even_or_odd(chunk.dwSize)) {
                                        ListCount[0]++;
                                        chunk.dwSize++;
                                    }
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                            }
                            TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
                        } while (ListCount[0] < ListSize[0]);
                        break;
                    }
                    case FOURCC_WVPL: {
                        TRACE_(dmfile)(": wave pool list (mark & skip)\n");
                        liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                        break;
                    }
                    case FOURCC_LINS: {
                        TRACE_(dmfile)(": instruments list\n");
                        do {
                            IStream_Read(stream, &chunk, sizeof(FOURCC) + sizeof(DWORD), NULL);
                            ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunk.dwSize;
                            TRACE_(dmfile)(": %s chunk (size = %#04lx)", debugstr_fourcc(chunk.fccID), chunk.dwSize);
                            switch (chunk.fccID) {
                                case FOURCC_LIST: {
                                    IStream_Read(stream, &chunk.fccID, sizeof(FOURCC), NULL);
                                    TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunk.fccID));
                                    ListSize[1] = chunk.dwSize - sizeof(FOURCC);
                                    ListCount[1] = 0;
                                    switch (chunk.fccID) {
                                        case FOURCC_INS: {
                                            struct instrument_entry *entry = calloc(1, sizeof(*entry));
                                            TRACE_(dmfile)(": instrument list\n");
                                            liMove.QuadPart = -12;
                                            IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                            if (FAILED(instrument_create_from_stream(stream, &entry->instrument))) free(entry);
                                            else list_add_tail(&This->instruments, &entry->entry);
                                            break;
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                                    liMove.QuadPart = chunk.dwSize;
                                    IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                                    break;
                                }
                            }
                            TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
                        } while (ListCount[0] < ListSize[0]);
                        break;
                    }
                    default: {
                        TRACE_(dmfile)(": unknown (skipping)\n");
                        liMove.QuadPart = chunk.dwSize - sizeof(FOURCC);
                        IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                        break;
                    }
                }
                break;
            }
            default: {
                TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
                liMove.QuadPart = chunk.dwSize;
                IStream_Seek(stream, liMove, STREAM_SEEK_CUR, NULL);
                break;
            }
        }
        TRACE_(dmfile)(": StreamCount = %ld < StreamSize = %ld\n", StreamCount, StreamSize);
    } while (StreamCount < StreamSize);

    TRACE_(dmfile)(": reading finished\n");


    /* DEBUG: dumps whole collection object tree: */
    if (TRACE_ON(dmusic))
    {
        struct instrument_entry *entry;
        int i = 0;

        TRACE("*** IDirectMusicCollection (%p) ***\n", &This->IDirectMusicCollection_iface);
        dump_DMUS_OBJECTDESC(&This->dmobj.desc);

        TRACE(" - Collection header:\n");
        TRACE("    - cInstruments: %ld\n", This->pHeader->cInstruments);
        TRACE(" - Instruments:\n");

        LIST_FOR_EACH_ENTRY(entry, &This->instruments, struct instrument_entry, entry)
            TRACE("    - Instrument[%i]: %p\n", i++, entry->instrument);
    }

    return S_OK;
}

static const IPersistStreamVtbl collection_stream_vtbl =
{
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    unimpl_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    collection_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax,
};

HRESULT collection_create(IUnknown **ret_iface)
{
    struct collection *collection;

    *ret_iface = NULL;
    if (!(collection = calloc(1, sizeof(*collection)))) return E_OUTOFMEMORY;
    collection->IDirectMusicCollection_iface.lpVtbl = &collection_vtbl;
    collection->ref = 1;
    dmobject_init(&collection->dmobj, &CLSID_DirectMusicCollection,
            (IUnknown *)&collection->IDirectMusicCollection_iface);
    collection->dmobj.IDirectMusicObject_iface.lpVtbl = &collection_object_vtbl;
    collection->dmobj.IPersistStream_iface.lpVtbl = &collection_stream_vtbl;
    list_init(&collection->instruments);

    TRACE("Created DirectMusicCollection %p\n", collection);
    *ret_iface = (IUnknown *)&collection->IDirectMusicCollection_iface;
    return S_OK;
}
