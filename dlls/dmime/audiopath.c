/* IDirectMusicAudioPath Implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct IDirectMusicAudioPathImpl {
    IDirectMusicAudioPath IDirectMusicAudioPath_iface;
    LONG ref;
    IDirectMusicPerformance8* pPerf;
    IDirectMusicGraph*        pToolGraph;
    IDirectSoundBuffer*       pDSBuffer;
    IDirectSoundBuffer*       pPrimary;

    BOOL fActive;
};

struct audio_path_config
{
    IUnknown IUnknown_iface;
    struct dmobject dmobj;
    LONG ref;
};

static inline struct IDirectMusicAudioPathImpl *impl_from_IDirectMusicAudioPath(IDirectMusicAudioPath *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectMusicAudioPathImpl, IDirectMusicAudioPath_iface);
}

static inline struct audio_path_config *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_path_config, dmobj.IPersistStream_iface);
}

void set_audiopath_perf_pointer(IDirectMusicAudioPath *iface, IDirectMusicPerformance8 *performance)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPerf = performance;
}

void set_audiopath_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pDSBuffer = buffer;
}

void set_audiopath_primary_dsound_buffer(IDirectMusicAudioPath *iface, IDirectSoundBuffer *buffer)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    This->pPrimary = buffer;
}

/*****************************************************************************
 * IDirectMusicAudioPathImpl implementation
 */
static HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (IDirectMusicAudioPath *iface, REFIID riid, void **ppobj)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID (riid, &IID_IDirectMusicAudioPath) || IsEqualIID (riid, &IID_IUnknown))
        *ppobj = &This->IDirectMusicAudioPath_iface;

    if (*ppobj) {
        IUnknown_AddRef((IUnknown*)*ppobj);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_AddRef (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicAudioPathImpl_Release (IDirectMusicAudioPath *iface)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%ld\n", This, ref);

    if (ref == 0) {
        if (This->pPrimary)
            IDirectSoundBuffer_Release(This->pPrimary);
        if (This->pDSBuffer)
            IDirectSoundBuffer_Release(This->pDSBuffer);
        This->pPerf = NULL;
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (IDirectMusicAudioPath *iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer,
    REFGUID guidObject, WORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
	HRESULT hr;

	FIXME("(%p, %ld, %ld, %ld, %s, %d, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_dmguid(guidObject),
            dwIndex, debugstr_dmguid(iidInterface), ppObject);
	    
	switch (dwStage) {
        case DMUS_PATH_BUFFER:
          if (This->pDSBuffer)
          {
              if (IsEqualIID (iidInterface, &IID_IUnknown) ||
                  IsEqualIID (iidInterface, &IID_IDirectSoundBuffer)  ||
                  IsEqualIID (iidInterface, &IID_IDirectSoundBuffer8) ||
                  IsEqualIID (iidInterface, &IID_IDirectSound3DBuffer)) {
                  return IDirectSoundBuffer_QueryInterface(This->pDSBuffer, iidInterface, ppObject);
              }

              WARN("Unsupported interface %s\n", debugstr_dmguid(iidInterface));
              *ppObject = NULL;
              return E_NOINTERFACE;
          }
          break;

	case DMUS_PATH_PRIMARY_BUFFER: {
	  if (IsEqualIID (iidInterface, &IID_IDirectSound3DListener)) {
            IDirectSoundBuffer_QueryInterface(This->pPrimary, &IID_IDirectSound3DListener, ppObject);
	    return S_OK;
	  } else {
	    FIXME("bad iid...\n");
	  }
	}
	break;

	case DMUS_PATH_AUDIOPATH_GRAPH:
	  {
	    if (IsEqualIID (iidInterface, &IID_IDirectMusicGraph)) {
	      if (NULL == This->pToolGraph) {
		IDirectMusicGraph* pGraph;
		hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
		if (FAILED(hr))
		  return hr;
		This->pToolGraph = pGraph;
	      }
	      *ppObject = This->pToolGraph;
	      IDirectMusicGraph_AddRef((LPDIRECTMUSICGRAPH) *ppObject);
	      return S_OK;
	    }
	  }
	  break;

	case DMUS_PATH_AUDIOPATH_TOOL:
	  {
	    /* TODO */
	  }
	  break;

	case DMUS_PATH_PERFORMANCE:
	  {
	    /* TODO check wanted GUID */
	    *ppObject = This->pPerf;
	    IUnknown_AddRef((LPUNKNOWN) *ppObject);
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_GRAPH:
	  {
	    IDirectMusicGraph* pPerfoGraph = NULL; 
	    IDirectMusicPerformance8_GetGraph(This->pPerf, &pPerfoGraph);
	    if (NULL == pPerfoGraph) {
	      IDirectMusicGraph* pGraph = NULL;
	      hr = create_dmgraph(&IID_IDirectMusicGraph, (void**)&pGraph);
	      if (FAILED(hr))
		return hr;
	      IDirectMusicPerformance8_SetGraph(This->pPerf, pGraph);
	      pPerfoGraph = pGraph;
	    }
	    *ppObject = pPerfoGraph;
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_TOOL:
	default:
	  break;
	}

	*ppObject = NULL;
	return E_INVALIDARG;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_Activate(IDirectMusicAudioPath *iface, BOOL activate)
{
    struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);

    FIXME("(%p, %d): semi-stub\n", This, activate);

    if (!!activate == This->fActive)
        return S_FALSE;

    if (!activate && This->pDSBuffer) {
        /* Path is being deactivated */
        IDirectSoundBuffer_Stop(This->pDSBuffer);
    }

    This->fActive = !!activate;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (IDirectMusicAudioPath *iface, LONG lVolume, DWORD dwDuration)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %li, %ld): stub\n", This, lVolume, dwDuration);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (IDirectMusicAudioPath *iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut)
{
  struct IDirectMusicAudioPathImpl *This = impl_from_IDirectMusicAudioPath(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwPChannelIn, pdwPChannelOut);
  return S_OK;
}

static const IDirectMusicAudioPathVtbl DirectMusicAudioPathVtbl = {
  IDirectMusicAudioPathImpl_QueryInterface,
  IDirectMusicAudioPathImpl_AddRef,
  IDirectMusicAudioPathImpl_Release,
  IDirectMusicAudioPathImpl_GetObjectInPath,
  IDirectMusicAudioPathImpl_Activate,
  IDirectMusicAudioPathImpl_SetVolume,
  IDirectMusicAudioPathImpl_ConvertPChannel
};

/* IDirectMusicObject */
static HRESULT WINAPI path_config_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
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
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_AUDIOPATH_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicAudioPathConfig;
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
    path_config_IDirectMusicObject_ParseDescriptor
};

/* IPersistStream */
static HRESULT WINAPI path_config_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    struct audio_path_config *This = impl_from_IPersistStream(iface);

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
    path_config_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

static inline struct audio_path_config *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct audio_path_config, IUnknown_iface);
}

static HRESULT WINAPI path_config_IUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppobj)
{
    struct audio_path_config *This = impl_from_IUnknown(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown))
        *ppobj = &This->IUnknown_iface;
    else if (IsEqualIID (riid, &IID_IDirectMusicObject))
        *ppobj = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID (riid, &IID_IPersistStream))
        *ppobj = &This->dmobj.IPersistStream_iface;

    if (*ppobj)
    {
        IUnknown_AddRef((IUnknown*)*ppobj);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI path_config_IUnknown_AddRef(IUnknown *unk)
{
    struct audio_path_config *This = impl_from_IUnknown(unk);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI path_config_IUnknown_Release(IUnknown *unk)
{
    struct audio_path_config *This = impl_from_IUnknown(unk);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
        free(This);
    return ref;
}

static const IUnknownVtbl path_config_unk_vtbl =
{
    path_config_IUnknown_QueryInterface,
    path_config_IUnknown_AddRef,
    path_config_IUnknown_Release,
};

/* for ClassFactory */
HRESULT create_dmaudiopath(REFIID riid, void **ppobj)
{
    IDirectMusicAudioPathImpl* obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicAudioPath_iface.lpVtbl = &DirectMusicAudioPathVtbl;
    obj->ref = 1;

    hr = IDirectMusicAudioPath_QueryInterface(&obj->IDirectMusicAudioPath_iface, riid, ppobj);
    IDirectMusicAudioPath_Release(&obj->IDirectMusicAudioPath_iface);
    return hr;
}

HRESULT create_dmaudiopath_config(REFIID riid, void **ppobj)
{
    struct audio_path_config *obj;
    HRESULT hr;

    *ppobj = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicAudioPathConfig, &obj->IUnknown_iface);
    obj->IUnknown_iface.lpVtbl = &path_config_unk_vtbl;
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    obj->ref = 1;

    hr = IUnknown_QueryInterface(&obj->IUnknown_iface, riid, ppobj);
    IUnknown_Release(&obj->IUnknown_iface);
    return hr;
}
