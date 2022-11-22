/* IDirectMusicGraph
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

#include "dmime_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct IDirectMusicGraphImpl {
  IDirectMusicGraph IDirectMusicGraph_iface;
  struct dmobject dmobj;
  LONG ref;
  WORD              num_tools;
  struct list       Tools;
};

static inline IDirectMusicGraphImpl *impl_from_IDirectMusicGraph(IDirectMusicGraph *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicGraphImpl, IDirectMusicGraph_iface);
}

static inline IDirectMusicGraphImpl *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicGraphImpl, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI DirectMusicGraph_QueryInterface(IDirectMusicGraph *iface, REFIID riid, void **ret_iface)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicGraph))
    {
        *ret_iface = &This->IDirectMusicGraph_iface;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;

    if (*ret_iface)
    {
        IDirectMusicGraph_AddRef(iface);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectMusicGraph_AddRef(IDirectMusicGraph *iface)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    DMIME_LockModule();
    return ref;
}

static ULONG WINAPI DirectMusicGraph_Release(IDirectMusicGraph *iface)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    DMIME_UnlockModule();
    return ref;
}

static HRESULT WINAPI DirectMusicGraph_StampPMsg(IDirectMusicGraph *iface, DMUS_PMSG *msg)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    FIXME("(%p, %p): stub\n", This, msg);
    return S_OK;
}

static HRESULT WINAPI DirectMusicGraph_InsertTool(IDirectMusicGraph *iface, IDirectMusicTool* pTool, DWORD* pdwPChannels, DWORD cPChannels, LONG lIndex)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    struct list* pEntry = NULL;
    struct list* pPrevEntry = NULL;
    LPDMUS_PRIVATE_GRAPH_TOOL pIt = NULL;
    LPDMUS_PRIVATE_GRAPH_TOOL pNewTool = NULL;

    FIXME("(%p, %p, %p, %ld, %li): use of pdwPChannels\n", This, pTool, pdwPChannels, cPChannels, lIndex);
  
    if (!pTool)
        return E_POINTER;

    if (lIndex < 0)
        lIndex = This->num_tools + lIndex;

    pPrevEntry = &This->Tools;
    LIST_FOR_EACH(pEntry, &This->Tools)
    {
        pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_GRAPH_TOOL, entry);
        if (pIt->dwIndex == lIndex)
            return DMUS_E_ALREADY_EXISTS;

        if (pIt->dwIndex > lIndex)
          break ;
        pPrevEntry = pEntry;
    }

    ++This->num_tools;
    pNewTool = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_GRAPH_TOOL));
    pNewTool->pTool = pTool;
    pNewTool->dwIndex = lIndex;
    IDirectMusicTool8_AddRef(pTool);
    IDirectMusicTool8_Init(pTool, iface);
    list_add_tail (pPrevEntry->next, &pNewTool->entry);

#if 0
  DWORD dwNum = 0;
  IDirectMusicTool8_GetMediaTypes(pTool, &dwNum);
#endif

    return DS_OK;
}

static HRESULT WINAPI DirectMusicGraph_GetTool(IDirectMusicGraph *iface, DWORD dwIndex, IDirectMusicTool** ppTool)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    struct list* pEntry = NULL;
    LPDMUS_PRIVATE_GRAPH_TOOL pIt = NULL;

    TRACE("(%p, %ld, %p)\n", This, dwIndex, ppTool);

    LIST_FOR_EACH (pEntry, &This->Tools)
    {
        pIt = LIST_ENTRY(pEntry, DMUS_PRIVATE_GRAPH_TOOL, entry);
        if (pIt->dwIndex == dwIndex)
        {
            *ppTool = pIt->pTool;
            if (*ppTool)
                IDirectMusicTool_AddRef(*ppTool);
            return S_OK;
        }
        if (pIt->dwIndex > dwIndex)
            break ;
    }

    return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI DirectMusicGraph_RemoveTool(IDirectMusicGraph *iface, IDirectMusicTool* pTool)
{
    IDirectMusicGraphImpl *This = impl_from_IDirectMusicGraph(iface);
    FIXME("(%p, %p): stub\n", This, pTool);
    return S_OK;
}

static const IDirectMusicGraphVtbl DirectMusicGraphVtbl =
{
    DirectMusicGraph_QueryInterface,
    DirectMusicGraph_AddRef,
    DirectMusicGraph_Release,
    DirectMusicGraph_StampPMsg,
    DirectMusicGraph_InsertTool,
    DirectMusicGraph_GetTool,
    DirectMusicGraph_RemoveTool
};

static HRESULT WINAPI graph_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream)
        return E_POINTER;
    if (!desc || desc->dwSize != sizeof(*desc))
        return E_INVALIDARG;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_TOOLGRAPH_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicGraph;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    graph_IDirectMusicObject_ParseDescriptor
};

static HRESULT WINAPI graph_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    IDirectMusicGraphImpl *This = impl_from_IPersistStream(iface);

    FIXME("(%p, %p): Loading not implemented yet\n", This, stream);

    return IDirectMusicObject_ParseDescriptor(&This->dmobj.IDirectMusicObject_iface, stream,
            &This->dmobj.desc);
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    graph_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmgraph(REFIID riid, void **ret_iface)
{
    IDirectMusicGraphImpl* obj;
    HRESULT hr;

    *ret_iface = NULL;
  
    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicGraphImpl));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IDirectMusicGraph_iface.lpVtbl = &DirectMusicGraphVtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicGraph, (IUnknown *)&obj->IDirectMusicGraph_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&obj->Tools);

    hr = IDirectMusicGraph_QueryInterface(&obj->IDirectMusicGraph_iface, riid, ret_iface);
    IDirectMusicGraph_Release(&obj->IDirectMusicGraph_iface);

    return hr;
}
