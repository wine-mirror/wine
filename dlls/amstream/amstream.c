/*
 * Implementation of IAMMultiMediaStream Interface
 *
 * Copyright 2004 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/debug.h"

#include "winbase.h"
#include "wingdi.h"

#include "amstream_private.h"
#include "amstream.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

typedef struct {
    IAMMultiMediaStream lpVtbl;
    int ref;
} IAMMultiMediaStreamImpl;

static struct ICOM_VTABLE(IAMMultiMediaStream) AM_Vtbl;

HRESULT AM_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IAMMultiMediaStreamImpl* object; 

    FIXME("(%p,%p)\n", pUnkOuter, ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAMMultiMediaStreamImpl));
    
    object->lpVtbl.lpVtbl = &AM_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_QueryInterface(IAMMultiMediaStream* iface, REFIID riid, void** ppvObject)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IAMMultiMediaStream))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IAMMultiMediaStreamImpl_AddRef(IAMMultiMediaStream* iface)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)\n", iface, This); 

  This->ref++;
  return S_OK;
}

static ULONG WINAPI IAMMultiMediaStreamImpl_Release(IAMMultiMediaStream* iface)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)\n", iface, This); 

  if (!--This->ref)
    HeapFree(GetProcessHeap(), 0, This);

  return S_OK;
}

/*** IMultiMediaStream methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_GetInformation(IAMMultiMediaStream* iface, char* pdwFlags, STREAM_TYPE* pStreamType)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pdwFlags, pStreamType); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetMediaStream(IAMMultiMediaStream* iface, REFMSPID idPurpose, IMediaStream** ppMediaStream)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, idPurpose, ppMediaStream); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_EnumMediaStreams(IAMMultiMediaStream* iface, long Index, IMediaStream** ppMediaStream)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%ld,%p) stub!\n", This, iface, Index, ppMediaStream); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetState(IAMMultiMediaStream* iface, STREAM_STATE* pCurrentState)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentState); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_SetState(IAMMultiMediaStream* iface, STREAM_STATE NewState)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);
  
  FIXME("(%p/%p)->() stub!\n", This, iface); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetTime(IAMMultiMediaStream* iface, STREAM_TIME* pCurrentTime)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentTime); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetDuration(IAMMultiMediaStream* iface, STREAM_TIME* pDuration)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pDuration); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_Seek(IAMMultiMediaStream* iface, STREAM_TIME SeekTime)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->() stub!\n", This, iface); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetEndOfStream(IAMMultiMediaStream* iface, HANDLE* phEOS)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, phEOS); 

  return S_FALSE;
}

/*** IAMMultiMediaStream methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_Initialize(IAMMultiMediaStream* iface, STREAM_TYPE StreamType, DWORD dwFlags, IGraphBuilder* pFilterGraph)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%lx,%lx,%p) stub!\n", This, iface, (DWORD)StreamType, dwFlags, pFilterGraph); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetFilterGraph(IAMMultiMediaStream* iface, IGraphBuilder** ppGraphBuilder)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppGraphBuilder); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetFilter(IAMMultiMediaStream* iface, IMediaStreamFilter** ppFilter)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppFilter); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_AddMediaStream(IAMMultiMediaStream* iface, IUnknown* pStreamObject, const MSPID* PurposeId,
                                          DWORD dwFlags, IMediaStream** ppNewStream)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p,%p,%lx,%p) stub!\n", This, iface, pStreamObject, PurposeId, dwFlags, ppNewStream); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_OpenFile(IAMMultiMediaStream* iface, LPCWSTR pszFileName, DWORD dwFlags)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p,%lx) stub!\n", This, iface, pszFileName, dwFlags); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_OpenMoniker(IAMMultiMediaStream* iface, IBindCtx* pCtx, IMoniker* pMoniker, DWORD dwFlags)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%p,%p,%lx) stub!\n", This, iface, pCtx, pMoniker, dwFlags); 

  return S_FALSE;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_Render(IAMMultiMediaStream* iface, DWORD dwFlags)
{
  ICOM_THIS(IAMMultiMediaStreamImpl, iface);

  FIXME("(%p/%p)->(%lx) stub!\n", This, iface, dwFlags); 

  return S_FALSE;
}

static ICOM_VTABLE(IAMMultiMediaStream) AM_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IAMMultiMediaStreamImpl_QueryInterface,
    IAMMultiMediaStreamImpl_AddRef,
    IAMMultiMediaStreamImpl_Release,
    IAMMultiMediaStreamImpl_GetInformation,
    IAMMultiMediaStreamImpl_GetMediaStream,
    IAMMultiMediaStreamImpl_EnumMediaStreams,
    IAMMultiMediaStreamImpl_GetState,
    IAMMultiMediaStreamImpl_SetState,
    IAMMultiMediaStreamImpl_GetTime,
    IAMMultiMediaStreamImpl_GetDuration,
    IAMMultiMediaStreamImpl_Seek,
    IAMMultiMediaStreamImpl_GetEndOfStream,
    IAMMultiMediaStreamImpl_Initialize,
    IAMMultiMediaStreamImpl_GetFilterGraph,
    IAMMultiMediaStreamImpl_GetFilter,
    IAMMultiMediaStreamImpl_AddMediaStream,
    IAMMultiMediaStreamImpl_OpenFile,
    IAMMultiMediaStreamImpl_OpenMoniker,
    IAMMultiMediaStreamImpl_Render
};
