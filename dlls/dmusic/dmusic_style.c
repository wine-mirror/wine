/* IDirectMusicStyle Implementation
 * IDirectMusicStyle8 Implementation
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

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicStyle IUnknown parts follow: */
HRESULT WINAPI IDirectMusicStyleImpl_QueryInterface (LPDIRECTMUSICSTYLE iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyleImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicStyle))
	{
		IDirectMusicStyleImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyleImpl_AddRef (LPDIRECTMUSICSTYLE iface)
{
	ICOM_THIS(IDirectMusicStyleImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyleImpl_Release (LPDIRECTMUSICSTYLE iface)
{
	ICOM_THIS(IDirectMusicStyleImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyle Interface follow: */
HRESULT WINAPI IDirectMusicStyleImpl_GetBand (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicBand** ppBand)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_EnumBand (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetDefaultBand (LPDIRECTMUSICSTYLE iface, IDirectMusicBand** ppBand)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_EnumMotif (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetMotif (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetDefaultChordMap (LPDIRECTMUSICSTYLE iface, IDirectMusicChordMap** ppChordMap)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_EnumChordMap (LPDIRECTMUSICSTYLE iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetChordMap (LPDIRECTMUSICSTYLE iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetTimeSignature (LPDIRECTMUSICSTYLE iface, DMUS_TIMESIGNATURE* pTimeSig)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyleImpl_GetTempo (LPDIRECTMUSICSTYLE iface, double* pTempo)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicStyle) DirectMusicStyle_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyleImpl_QueryInterface,
	IDirectMusicStyleImpl_AddRef,
	IDirectMusicStyleImpl_Release,
	IDirectMusicStyleImpl_GetBand,
	IDirectMusicStyleImpl_EnumBand,
	IDirectMusicStyleImpl_GetDefaultBand,
	IDirectMusicStyleImpl_EnumMotif,
	IDirectMusicStyleImpl_GetMotif,
	IDirectMusicStyleImpl_GetDefaultChordMap,
	IDirectMusicStyleImpl_EnumChordMap,
	IDirectMusicStyleImpl_GetChordMap,
	IDirectMusicStyleImpl_GetTimeSignature,
	IDirectMusicStyleImpl_GetEmbellishmentLength,
	IDirectMusicStyleImpl_GetTempo
};


/* IDirectMusicStyle8 IUnknown parts follow: */
HRESULT WINAPI IDirectMusicStyle8Impl_QueryInterface (LPDIRECTMUSICSTYLE8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectMusicStyle8))
	{
		IDirectMusicStyle8Impl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicStyle8Impl_AddRef (LPDIRECTMUSICSTYLE8 iface)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicStyle8Impl_Release (LPDIRECTMUSICSTYLE8 iface)
{
	ICOM_THIS(IDirectMusicStyle8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicStyle Interface part follow: */
HRESULT WINAPI IDirectMusicStyle8Impl_GetBand (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicBand** ppBand)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumBand (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultBand (LPDIRECTMUSICSTYLE8 iface, IDirectMusicBand** ppBand)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumMotif (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetMotif (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicSegment** ppSegment)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetDefaultChordMap (LPDIRECTMUSICSTYLE8 iface, IDirectMusicChordMap** ppChordMap)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_EnumChordMap (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetChordMap (LPDIRECTMUSICSTYLE8 iface, WCHAR* pwszName, IDirectMusicChordMap** ppChordMap)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetTimeSignature (LPDIRECTMUSICSTYLE8 iface, DMUS_TIMESIGNATURE* pTimeSig)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetEmbellishmentLength (LPDIRECTMUSICSTYLE8 iface, DWORD dwType, DWORD dwLevel, DWORD* pdwMin, DWORD* pdwMax)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IDirectMusicStyle8Impl_GetTempo (LPDIRECTMUSICSTYLE8 iface, double* pTempo)
{
	FIXME("stub\n");
	return DS_OK;
}

/* IDirectMusicStyle8 Interface part follow: */
HRESULT WINAPI IDirectMusicStyle8ImplEnumPattern (LPDIRECTMUSICSTYLE8 iface, DWORD dwIndex, DWORD dwPatternType, WCHAR* pwszName)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IDirectMusicStyle8) DirectMusicStyle8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicStyle8Impl_QueryInterface,
	IDirectMusicStyle8Impl_AddRef,
	IDirectMusicStyle8Impl_Release,
	IDirectMusicStyle8Impl_GetBand,
	IDirectMusicStyle8Impl_EnumBand,
	IDirectMusicStyle8Impl_GetDefaultBand,
	IDirectMusicStyle8Impl_EnumMotif,
	IDirectMusicStyle8Impl_GetMotif,
	IDirectMusicStyle8Impl_GetDefaultChordMap,
	IDirectMusicStyle8Impl_EnumChordMap,
	IDirectMusicStyle8Impl_GetChordMap,
	IDirectMusicStyle8Impl_GetTimeSignature,
	IDirectMusicStyle8Impl_GetEmbellishmentLength,
	IDirectMusicStyle8Impl_GetTempo,
	IDirectMusicStyle8ImplEnumPattern
};
