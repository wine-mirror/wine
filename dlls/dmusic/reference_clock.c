/*
 * IReferenceClock Implementation
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

/* IReferenceClock IUnknown parts follow: */
HRESULT WINAPI IReferenceClockImpl_QueryInterface (LPREFERENCECLOCK iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IReferenceClock))
	{
		IReferenceClockImpl_AddRef(iface);
		*ppobj = This;
		return DS_OK;
	}
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IReferenceClockImpl_AddRef (LPREFERENCECLOCK iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IReferenceClockImpl_Release (LPREFERENCECLOCK iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IReferenceClock Interface follow: */
HRESULT WINAPI IReferenceClockImpl_GetTime (LPREFERENCECLOCK iface, REFERENCE_TIME* pTime)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdviseTime (LPREFERENCECLOCK iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (LPREFERENCECLOCK iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

HRESULT WINAPI IReferenceClockImpl_Unadvise (LPREFERENCECLOCK iface, DWORD dwAdviseCookie)
{
	FIXME("stub\n");
	return DS_OK;
}

ICOM_VTABLE(IReferenceClock) ReferenceClock_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IReferenceClockImpl_QueryInterface,
	IReferenceClockImpl_AddRef,
	IReferenceClockImpl_Release,
	IReferenceClockImpl_GetTime,
	IReferenceClockImpl_AdviseTime,
	IReferenceClockImpl_AdvisePeriodic,
	IReferenceClockImpl_Unadvise
};
