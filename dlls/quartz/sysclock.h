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


typedef struct CSystemClock
{
	QUARTZ_IUnkImpl	unk;
	SC_IReferenceClockImpl	refclk;

	/* IReferenceClock fields. */
} CSystemClock;

#define	CSystemClock_THIS(iface,member)		CSystemClock*	This = ((CSystemClock*)(((char*)iface)-offsetof(CSystemClock,member)))

HRESULT QUARTZ_CreateSystemClock(IUnknown* punkOuter,void** ppobj);

void CSystemClock_InitIReferenceClock( CSystemClock* psc );


#endif  /* WINE_DSHOW_SYSCLOCK_H */
