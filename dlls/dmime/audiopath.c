/* IDirectMusicAudioPath Implementation
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicAudioPath IUnknown parts follow: */
HRESULT WINAPI IDirectMusicAudioPathImpl_QueryInterface (LPDIRECTMUSICAUDIOPATH iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicAudioPath))
	{
		IDirectMusicAudioPathImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicAudioPathImpl_AddRef (LPDIRECTMUSICAUDIOPATH iface)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicAudioPathImpl_Release (LPDIRECTMUSICAUDIOPATH iface)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicAudioPath Interface follow: */
HRESULT WINAPI IDirectMusicAudioPathImpl_GetObjectInPath (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannel, DWORD dwStage, DWORD dwBuffer, REFGUID guidObject, WORD dwIndex, REFGUID iidInterface, void** ppObject)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	FIXME("(%p, %ld, %ld, %ld, %s, %d, %s, %p): stub\n", This, dwPChannel, dwStage, dwBuffer, debugstr_guid(guidObject), dwIndex, debugstr_guid(iidInterface), ppObject);

	switch (dwStage) {
	case DMUS_PATH_BUFFER:
	  {
	    if (IsEqualGUID(iidInterface,&IID_IDirectSoundBuffer8)) {
	      IDirectSoundBuffer8_QueryInterface(This->pDSBuffer, &IID_IDirectSoundBuffer8, ppObject);
	      TRACE("returning %p\n",*ppObject);
	      return S_OK;
	    } else if (IsEqualGUID(iidInterface,&IID_IDirectSound3DBuffer)) {
	      IDirectSoundBuffer8_QueryInterface(This->pDSBuffer, &IID_IDirectSound3DBuffer, ppObject);
	      TRACE("returning %p\n",*ppObject);
	      return S_OK;
	    } else {
	      FIXME("Bad iid\n");
	    }
	  }
	  break;

	case DMUS_PATH_PRIMARY_BUFFER: {
	  if (IsEqualGUID(iidInterface,&IID_IDirectSound3DListener)) {
	    IDirectSoundBuffer8_QueryInterface(This->pPrimary, &IID_IDirectSound3DListener, ppObject);
	    return S_OK;
	  }else {
	    FIXME("bad iid...\n");
	  }
	}
	break;

	case DMUS_PATH_AUDIOPATH_GRAPH:
	  {
	    if (IsEqualGUID(iidInterface, &IID_IDirectMusicGraph)) {
	      if (NULL == This->pToolGraph) {
		IDirectMusicGraphImpl* pGraph;
		pGraph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicGraphImpl));		
		pGraph->lpVtbl = &DirectMusicGraph_Vtbl;
		pGraph->ref = 1;
		This->pToolGraph = (IDirectMusicGraph*) pGraph;
	      }
	      *ppObject = (LPDIRECTMUSICGRAPH) This->pToolGraph; 
	      IDirectMusicGraphImpl_AddRef((LPDIRECTMUSICGRAPH) *ppObject);
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
	    *ppObject = (LPDIRECTMUSICPERFORMANCE8) This->pPerf; 
	    IDirectMusicPerformance8Impl_AddRef((LPDIRECTMUSICPERFORMANCE8) *ppObject);
	    return S_OK;
	  }
	  break;

	case DMUS_PATH_PERFORMANCE_GRAPH:
	  {
	    IDirectMusicGraph* pPerfoGraph = NULL; 
	    IDirectMusicPerformance8Impl_GetGraph((LPDIRECTMUSICPERFORMANCE8) This->pPerf, &pPerfoGraph);
	    if (NULL == pPerfoGraph) {
	      IDirectMusicGraphImpl* pGraph = NULL; 
	      pGraph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicGraphImpl));		
	      pGraph->lpVtbl = &DirectMusicGraph_Vtbl;
	      pGraph->ref = 1;
	      IDirectMusicPerformance8Impl_SetGraph((LPDIRECTMUSICPERFORMANCE8) This->pPerf, (IDirectMusicGraph*) pGraph);
	      /* we need release as SetGraph do an AddRef */
	      IDirectMusicGraphImpl_Release((LPDIRECTMUSICGRAPH) pGraph);
	      pPerfoGraph = (LPDIRECTMUSICGRAPH) pGraph;
	    }
	    *ppObject = (LPDIRECTMUSICGRAPH) pPerfoGraph; 
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

HRESULT WINAPI IDirectMusicAudioPathImpl_Activate (LPDIRECTMUSICAUDIOPATH iface, BOOL fActivate)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	FIXME("(%p, %d): stub\n", This, fActivate);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAudioPathImpl_SetVolume (LPDIRECTMUSICAUDIOPATH iface, long lVolume, DWORD dwDuration)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	FIXME("(%p, %li, %ld): stub\n", This, lVolume, dwDuration);

	return S_OK;
}

HRESULT WINAPI IDirectMusicAudioPathImpl_ConvertPChannel (LPDIRECTMUSICAUDIOPATH iface, DWORD dwPChannelIn, DWORD* pdwPChannelOut)
{
	ICOM_THIS(IDirectMusicAudioPathImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwPChannelIn, pdwPChannelOut);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicAudioPath) DirectMusicAudioPath_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicAudioPathImpl_QueryInterface,
	IDirectMusicAudioPathImpl_AddRef,
	IDirectMusicAudioPathImpl_Release,
	IDirectMusicAudioPathImpl_GetObjectInPath,
	IDirectMusicAudioPathImpl_Activate,
	IDirectMusicAudioPathImpl_SetVolume,
	IDirectMusicAudioPathImpl_ConvertPChannel
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicAudioPath (LPCGUID lpcGUID, LPDIRECTMUSICAUDIOPATH* ppDMCAPath, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicAudioPath))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
