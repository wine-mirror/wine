/* IDirectMusicGraph
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

/* IDirectMusicGraph IUnknown parts follow: */
HRESULT WINAPI IDirectMusicGraphImpl_QueryInterface (LPDIRECTMUSICGRAPH iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicGraph))
	{
		IDirectMusicGraphImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicGraphImpl_AddRef (LPDIRECTMUSICGRAPH iface)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicGraphImpl_Release (LPDIRECTMUSICGRAPH iface)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicGraph Interface follow: */
HRESULT WINAPI IDirectMusicGraphImpl_StampPMsg (LPDIRECTMUSICGRAPH iface, DMUS_PMSG* pPMSG)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);

	FIXME("(%p, %p): stub\n", This, pPMSG);

	return S_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_InsertTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool, DWORD* pdwPChannels, DWORD cPChannels, LONG lIndex)
{
    int i;
	IDirectMusicTool8Impl* p;
	IDirectMusicTool8Impl* toAdd = (IDirectMusicTool8Impl*) pTool;
    ICOM_THIS(IDirectMusicGraphImpl,iface);

	FIXME("(%p, %p, %p, %ld, %li): use of pdwPChannels\n", This, pTool, pdwPChannels, cPChannels, lIndex);

	if (0 == This->num_tools) {
	  This->pFirst = This->pLast = toAdd;
	  toAdd->pPrev = toAdd->pNext = NULL;
	} else if (lIndex == 0 || lIndex <= -This->num_tools) {
	  This->pFirst->pPrev = toAdd;
	  toAdd->pNext = This->pFirst;
	  toAdd->pPrev = NULL;
	  This->pFirst = toAdd;
	} else if (lIndex < 0) {
	  p = This->pLast;
	  for (i = 0; i < -lIndex; ++i) {
	    p = p->pPrev;
	  }
	  toAdd->pNext = p->pNext;
	  if (p->pNext) p->pNext->pPrev = toAdd;
	  p->pNext = toAdd;
	  toAdd->pPrev = p;
	} else if (lIndex >= This->num_tools) {
	  This->pLast->pNext = toAdd;
	  toAdd->pPrev = This->pLast;
	  toAdd->pNext = NULL;
	  This->pLast = toAdd;
	} else if (lIndex > 0) {
	  p = This->pFirst;
	  for (i = 0; i < lIndex; ++i) {
	    p = p->pNext;
	  }
	  toAdd->pPrev = p->pPrev;
	  if (p->pPrev) p->pPrev->pNext = toAdd;
	  p->pPrev = toAdd;
	  toAdd->pNext = p;
	}
	++This->num_tools;
	return DS_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_GetTool (LPDIRECTMUSICGRAPH iface, DWORD dwIndex, IDirectMusicTool** ppTool)
{
	int i;
	IDirectMusicTool8Impl* p = NULL;
	ICOM_THIS(IDirectMusicGraphImpl,iface);
	
	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, ppTool);

	p = This->pFirst;
	for (i = 0; i < dwIndex && i < This->num_tools; ++i) {
	  p = p->pNext;
	}
	*ppTool = (IDirectMusicTool*) p;
	if (NULL != *ppTool) {
	  IDirectMusicTool8Impl_AddRef((LPDIRECTMUSICTOOL8) *ppTool);
	}
	return DS_OK;
}

HRESULT WINAPI IDirectMusicGraphImpl_RemoveTool (LPDIRECTMUSICGRAPH iface, IDirectMusicTool* pTool)
{
	ICOM_THIS(IDirectMusicGraphImpl,iface);

	FIXME("(%p, %p): stub\n", This, pTool);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicGraph) DirectMusicGraph_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicGraphImpl_QueryInterface,
	IDirectMusicGraphImpl_AddRef,
	IDirectMusicGraphImpl_Release,
	IDirectMusicGraphImpl_StampPMsg,
	IDirectMusicGraphImpl_InsertTool,
	IDirectMusicGraphImpl_GetTool,
	IDirectMusicGraphImpl_RemoveTool
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicGraph (LPCGUID lpcGUID, LPDIRECTMUSICGRAPH *ppDMGrph, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicGraph))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
