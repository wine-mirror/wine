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
