/*
 * Copyright (C) Hidenori TAKESHIMA
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

#ifndef WINE_DSHOW_SYSCLOCK_H
#define WINE_DSHOW_SYSCLOCK_H

/*
		implements CLSID_SystemClock.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IReferenceClock

*/

#include "iunk.h"

typedef struct SC_IReferenceClockImpl
{
	ICOM_VFIELD(IReferenceClock);
} SC_IReferenceClockImpl;


/* implementation limit */
#define	WINE_QUARTZ_SYSCLOCK_TIMER_MAX	64

typedef struct QUARTZ_TimerEntry
{
	DWORD			dwAdvCookie;
	BOOL			fPeriodic;
	HANDLE			hEvent;
	REFERENCE_TIME	rtStart;
	REFERENCE_TIME	rtInterval;
} QUARTZ_TimerEntry;

typedef struct CSystemClock
{
	QUARTZ_IUnkImpl	unk;
	SC_IReferenceClockImpl	refclk;

	/* IReferenceClock fields. */
	CRITICAL_SECTION	m_csClock;
	DWORD	m_dwTimeLast;
	REFERENCE_TIME	m_rtLast;
	HANDLE	m_hThreadTimer;
	HANDLE	m_hEventInit;
	DWORD	m_idThreadTimer;

	DWORD			m_dwAdvCookieNext;
	QUARTZ_TimerEntry	m_timerEntries[WINE_QUARTZ_SYSCLOCK_TIMER_MAX];
} CSystemClock;

#define	CSystemClock_THIS(iface,member)		CSystemClock*	This = ((CSystemClock*)(((char*)iface)-offsetof(CSystemClock,member)))

HRESULT QUARTZ_CreateSystemClock(IUnknown* punkOuter,void** ppobj);

HRESULT CSystemClock_InitIReferenceClock( CSystemClock* psc );
void CSystemClock_UninitIReferenceClock( CSystemClock* psc );


#endif  /* WINE_DSHOW_SYSCLOCK_H */
