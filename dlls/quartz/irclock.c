/*
 * Implementation of CLSID_SystemClock.
 *
 * FIXME - not tested yet.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "strmif.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "sysclock.h"


#define	QUARTZ_MSG_ADDTIMER			(WM_APP+0)
#define	QUARTZ_MSG_REMOVETIMER		(WM_APP+1)
#define	QUARTZ_MSG_EXITTHREAD		(WM_APP+2)


/****************************************************************************/

static QUARTZ_TimerEntry* IReferenceClock_AllocTimerEntry(CSystemClock* This)
{
	QUARTZ_TimerEntry*	pEntry;
	DWORD	dw;

	pEntry = &This->m_timerEntries[0];
	for ( dw = 0; dw < WINE_QUARTZ_SYSCLOCK_TIMER_MAX; dw++ )
	{
		if ( pEntry->hEvent == (HANDLE)NULL )
			return pEntry;
		pEntry ++;
	}

	return NULL;
}

static QUARTZ_TimerEntry* IReferenceClock_SearchTimer(CSystemClock* This, DWORD dwAdvCookie)
{
	QUARTZ_TimerEntry*	pEntry;
	DWORD	dw;

	pEntry = &This->m_timerEntries[0];
	for ( dw = 0; dw < WINE_QUARTZ_SYSCLOCK_TIMER_MAX; dw++ )
	{
		if ( pEntry->hEvent != (HANDLE)NULL &&
			 pEntry->dwAdvCookie == dwAdvCookie )
			return pEntry;
		pEntry ++;
	}

	return NULL;
}

static void IReferenceClock_OnTimerUpdated(CSystemClock* This)
{
	QUARTZ_TimerEntry*	pEntry;
	REFERENCE_TIME	rtCur;
	REFERENCE_TIME	rtSignal;
	REFERENCE_TIME	rtCount;
	HRESULT	hr;
	LONG	lCount;
	DWORD	dw;

	hr = IReferenceClock_GetTime((IReferenceClock*)(&This->refclk),&rtCur);
	if ( hr != NOERROR )
		return;

	pEntry = &This->m_timerEntries[0];
	for ( dw = 0; dw < WINE_QUARTZ_SYSCLOCK_TIMER_MAX; dw++ )
	{
		if ( pEntry->hEvent != (HANDLE)NULL )
		{
			rtSignal = pEntry->rtStart + pEntry->rtInterval;
			if ( rtCur >= rtSignal )
			{
				if ( pEntry->fPeriodic )
				{
					rtCount = ((rtCur - pEntry->rtStart) / pEntry->rtInterval);
					lCount = ( rtCount > (REFERENCE_TIME)0x7fffffff ) ?
						(LONG)0x7fffffff : (LONG)rtCount;
					if ( !ReleaseSemaphore( pEntry->hEvent, lCount, NULL ) )
					{
						while ( lCount > 0 )
						{
							if ( !ReleaseSemaphore( pEntry->hEvent, 1, NULL ) )
								break;
						}
					}
				}
				else
				{
					SetEvent( pEntry->hEvent );
					pEntry->hEvent = (HANDLE)NULL;
				}
			}
		}
		pEntry ++;
	}
}

static
DWORD WINAPI IReferenceClock_TimerEntry( LPVOID lpvParam )
{
	CSystemClock*	This = (CSystemClock*)lpvParam;
	MSG	msg;
	DWORD	dwRes;

	/* initialize the message queue. */
	PeekMessageA( &msg, (HWND)NULL, 0, 0, PM_NOREMOVE );
	/* resume the owner thread. */
	SetEvent( This->m_hEventInit );

	/* message loop. */
	while ( 1 )
	{
		dwRes = MsgWaitForMultipleObjects(
			0, NULL, FALSE,
			INFINITE, /* FIXME */
			QS_ALLEVENTS );

		EnterCriticalSection( &This->m_csClock );
		IReferenceClock_OnTimerUpdated(This);
		LeaveCriticalSection( &This->m_csClock );

		while ( PeekMessageA( &msg, (HWND)NULL, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
				goto quitthread;

			if ( msg.hwnd != (HWND)NULL )
			{
				TranslateMessage( &msg );
				DispatchMessageA( &msg );
			}
			else
			{
				switch ( msg.message )
				{
				case QUARTZ_MSG_ADDTIMER:
				case QUARTZ_MSG_REMOVETIMER:
					break;
				case QUARTZ_MSG_EXITTHREAD:
					PostQuitMessage(0);
					break;
				default:
					FIXME( "invalid message %04u\n", (unsigned)msg.message );
					break;
				}
			}
		}
	}

quitthread:
	return 0;
}

/****************************************************************************/

static HRESULT WINAPI
IReferenceClock_fnQueryInterface(IReferenceClock* iface,REFIID riid,void** ppobj)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IReferenceClock_fnAddRef(IReferenceClock* iface)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IReferenceClock_fnRelease(IReferenceClock* iface)
{
	CSystemClock_THIS(iface,refclk);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IReferenceClock_fnGetTime(IReferenceClock* iface,REFERENCE_TIME* prtTime)
{
	CSystemClock_THIS(iface,refclk);
	DWORD	dwTimeCur;

	TRACE( "(%p)->(%p)\n", This, prtTime );

	if ( prtTime == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->m_csClock );

	dwTimeCur = GetTickCount();
	This->m_rtLast += (REFERENCE_TIME)(DWORD)(dwTimeCur - This->m_dwTimeLast);

	This->m_dwTimeLast = dwTimeCur;

	*prtTime = This->m_dwTimeLast;

	LeaveCriticalSection( &This->m_csClock );

	return NOERROR;
}

static HRESULT WINAPI
IReferenceClock_fnAdviseTime(IReferenceClock* iface,REFERENCE_TIME rtBase,REFERENCE_TIME rtStream,HEVENT hEvent,DWORD_PTR* pdwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);
	QUARTZ_TimerEntry*	pEntry;
	HRESULT	hr;
	REFERENCE_TIME	rtCur;

	TRACE( "(%p)->()\n", This );

	if ( pdwAdvCookie == NULL )
		return E_POINTER;
	if ( hEvent == (HANDLE)NULL )
		return E_INVALIDARG;

	EnterCriticalSection( &This->m_csClock );

	*pdwAdvCookie = (DWORD_PTR)(This->m_dwAdvCookieNext ++);

	hr = IReferenceClock_GetTime(iface,&rtCur);
	if ( hr != NOERROR )
		goto err;
	if ( rtCur >= (rtBase+rtStream) )
	{
		SetEvent(hEvent);
		hr = NOERROR;
		goto err;
	}

	pEntry = IReferenceClock_AllocTimerEntry(This);
	if ( pEntry == NULL )
	{
		hr = E_FAIL;
		goto err;
	}

	if ( !PostThreadMessageA(
			This->m_idThreadTimer,
			QUARTZ_MSG_ADDTIMER,
			0, 0 ) )
	{
		hr = E_FAIL;
		goto err;
	}

	pEntry->dwAdvCookie = *pdwAdvCookie;
	pEntry->fPeriodic = FALSE;
	pEntry->hEvent = hEvent;
	pEntry->rtStart = rtBase;
	pEntry->rtInterval = rtStream;

	hr = NOERROR;
err:
	LeaveCriticalSection( &This->m_csClock );

	return hr;
}

static HRESULT WINAPI
IReferenceClock_fnAdvisePeriodic(IReferenceClock* iface,REFERENCE_TIME rtStart,REFERENCE_TIME rtPeriod,HSEMAPHORE hSemaphore,DWORD_PTR* pdwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);
	QUARTZ_TimerEntry*	pEntry;
	HRESULT	hr;

	TRACE( "(%p)->()\n", This );

	if ( pdwAdvCookie == NULL )
		return E_POINTER;
	if ( hSemaphore == (HSEMAPHORE)NULL )
		return E_INVALIDARG;

	EnterCriticalSection( &This->m_csClock );

	*pdwAdvCookie = (DWORD_PTR)(This->m_dwAdvCookieNext ++);

	pEntry = IReferenceClock_AllocTimerEntry(This);
	if ( pEntry == NULL )
	{
		hr = E_FAIL;
		goto err;
	}

	if ( !PostThreadMessageA(
			This->m_idThreadTimer,
			QUARTZ_MSG_ADDTIMER,
			0, 0 ) )
	{
		hr = E_FAIL;
		goto err;
	}

	pEntry->dwAdvCookie = *pdwAdvCookie;
	pEntry->fPeriodic = TRUE;
	pEntry->hEvent = (HANDLE)hSemaphore;
	pEntry->rtStart = rtStart;
	pEntry->rtInterval = rtPeriod;

	hr = NOERROR;
err:
	LeaveCriticalSection( &This->m_csClock );

	return hr;
}

static HRESULT WINAPI
IReferenceClock_fnUnadvise(IReferenceClock* iface,DWORD_PTR dwAdvCookie)
{
	CSystemClock_THIS(iface,refclk);
	QUARTZ_TimerEntry*	pEntry;

	TRACE( "(%p)->(%lu)\n", This, (DWORD)dwAdvCookie );

	EnterCriticalSection( &This->m_csClock );

	pEntry = IReferenceClock_SearchTimer(This,(DWORD)dwAdvCookie);
	if ( pEntry != NULL )
	{
		pEntry->hEvent = (HANDLE)NULL;
	}

	LeaveCriticalSection( &This->m_csClock );

	return NOERROR;
}

static ICOM_VTABLE(IReferenceClock) irefclk =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IReferenceClock_fnQueryInterface,
	IReferenceClock_fnAddRef,
	IReferenceClock_fnRelease,
	/* IReferenceClock fields */
	IReferenceClock_fnGetTime,
	IReferenceClock_fnAdviseTime,
	IReferenceClock_fnAdvisePeriodic,
	IReferenceClock_fnUnadvise,
};


HRESULT CSystemClock_InitIReferenceClock( CSystemClock* psc )
{
	HANDLE	hEvents[2];

	TRACE("(%p)\n",psc);
	ICOM_VTBL(&psc->refclk) = &irefclk;

	InitializeCriticalSection( &psc->m_csClock );
	psc->m_dwTimeLast = GetTickCount();
	psc->m_rtLast = (REFERENCE_TIME)0;
	psc->m_hThreadTimer = (HANDLE)NULL;
	psc->m_hEventInit = (HANDLE)NULL;
	psc->m_idThreadTimer = 0;
	psc->m_dwAdvCookieNext = 1;
	ZeroMemory( psc->m_timerEntries, sizeof(psc->m_timerEntries) );

	psc->m_hEventInit = CreateEventA( NULL, TRUE, FALSE, NULL );
	if ( psc->m_hEventInit == (HANDLE)NULL )
		goto err;

	psc->m_hThreadTimer = CreateThread(
		NULL, 0,
		IReferenceClock_TimerEntry,
		psc, 0, &psc->m_idThreadTimer );

	if ( psc->m_hThreadTimer == (HANDLE)NULL )
	{
		CloseHandle( psc->m_hEventInit );
		psc->m_hEventInit = (HANDLE)NULL;
		goto err;
	}

	hEvents[0] = psc->m_hEventInit;
	hEvents[1] = psc->m_hThreadTimer;
	if ( WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE )
			!= WAIT_OBJECT_0 )
	{
		CloseHandle( psc->m_hEventInit );
		psc->m_hEventInit = (HANDLE)NULL;
		CloseHandle( psc->m_hThreadTimer );
		psc->m_hThreadTimer = (HANDLE)NULL;
		goto err;
	}

	return NOERROR;

err:
	DeleteCriticalSection( &psc->m_csClock );
	return E_FAIL;
}

void CSystemClock_UninitIReferenceClock( CSystemClock* psc )
{
	TRACE("(%p)\n",psc);

	if ( psc->m_hThreadTimer != (HANDLE)NULL )
	{
		if ( PostThreadMessageA(
			psc->m_idThreadTimer,
			QUARTZ_MSG_EXITTHREAD,
			0, 0 ) )
		{
			WaitForSingleObject( psc->m_hThreadTimer, INFINITE );
		}
		CloseHandle( psc->m_hThreadTimer );
		psc->m_hThreadTimer = (HANDLE)NULL;
	}

	DeleteCriticalSection( &psc->m_csClock );
}
