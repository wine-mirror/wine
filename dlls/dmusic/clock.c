/* IReferenceClock Implementation
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
 * MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
HRESULT WINAPI IReferenceClockImpl_QueryInterface (IReferenceClock *iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IReferenceClock)) {
		IReferenceClockImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}

	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IReferenceClockImpl_AddRef (IReferenceClock *iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IReferenceClockImpl_Release (IReferenceClock *iface)
{
	ICOM_THIS(IReferenceClockImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IReferenceClock Interface follow: */
HRESULT WINAPI IReferenceClockImpl_GetTime (IReferenceClock *iface, REFERENCE_TIME* pTime)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	TRACE("(%p, %p)\n", This, pTime);
	*pTime = This->rtTime;
	
	return S_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdviseTime (IReferenceClock *iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	FIXME("(%p, %lli, %lli, %p, %p): stub\n", This, baseTime, streamTime, hEvent, pdwAdviseCookie);

	return S_OK;
}

HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (IReferenceClock *iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	FIXME("(%p, %lli, %lli, %p, %p): stub\n", This, startTime, periodTime, hSemaphore, pdwAdviseCookie);

	return S_OK;
}

HRESULT WINAPI IReferenceClockImpl_Unadvise (IReferenceClock *iface, DWORD dwAdviseCookie)
{
	ICOM_THIS(IReferenceClockImpl,iface);

	FIXME("(%p, %ld): stub\n", This, dwAdviseCookie);

	return S_OK;
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

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateReferenceClock (LPCGUID lpcGUID, IReferenceClock** ppRC, LPUNKNOWN pUnkOuter)
{
	IReferenceClockImpl* clock;
	
	if (IsEqualIID (lpcGUID, &IID_IReferenceClock))
	{
		clock = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IReferenceClockImpl));
		if (NULL == clock) {
			*ppRC = NULL;
			return E_OUTOFMEMORY;
		}
		clock->lpVtbl = &ReferenceClock_Vtbl;
		clock->ref = 1;
		clock->rtTime = 0;
		clock->pClockInfo.dwSize = sizeof (DMUS_CLOCKINFO);
		
		*ppRC = (IReferenceClock *) clock;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;	
}
