/*
 * Implementation of IMediaFilter for FilterGraph.
 *
 * FIXME - stub.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"
#include "vfwmsgs.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"


#define	WINE_QUARTZ_POLL_INTERVAL	10

/*****************************************************************************/

static
HRESULT CFilterGraph_PollGraphState(
	CFilterGraph* This,
	FILTER_STATE* pState)
{
	HRESULT	hr;
	QUARTZ_CompListItem*	pItem;
	IBaseFilter*	pFilter;

	hr = S_OK;

	EnterCriticalSection( &This->m_csGraphState );
	QUARTZ_CompList_Lock( This->m_pFilterList );

	pItem = QUARTZ_CompList_GetFirst( This->m_pFilterList );

	while ( pItem != NULL )
	{
		pFilter = (IBaseFilter*)QUARTZ_CompList_GetItemPtr( pItem );
		hr = IBaseFilter_GetState( pFilter, (DWORD)0, pState );
		if ( hr != S_OK )
			break;

		pItem = QUARTZ_CompList_GetNext( This->m_pFilterList, pItem );
	}

	QUARTZ_CompList_Unlock( This->m_pFilterList );
	LeaveCriticalSection( &This->m_csGraphState );

	TRACE( "returns %08lx, state %d\n",
		hr, *pState );

	return hr;
}

/*****************************************************************************/

static HRESULT WINAPI
IMediaFilter_fnQueryInterface(IMediaFilter* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaFilter_fnAddRef(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaFilter_fnRelease(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IMediaFilter_fnGetClassID(IMediaFilter* iface,CLSID* pclsid)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IPersist_GetClassID(
		CFilterGraph_IPersist(This),pclsid);
}

static HRESULT WINAPI
IMediaFilter_fnStop(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);
	HRESULT	hr;
	HRESULT	hrFilter;
	QUARTZ_CompListItem*	pItem;
	IBaseFilter*	pFilter;

	TRACE("(%p)->()\n",This);

	hr = S_OK;

	EnterCriticalSection( &This->m_csGraphState );

	if ( This->m_stateGraph != State_Stopped )
	{
		QUARTZ_CompList_Lock( This->m_pFilterList );

		pItem = QUARTZ_CompList_GetFirst( This->m_pFilterList );

		while ( pItem != NULL )
		{
			pFilter = (IBaseFilter*)QUARTZ_CompList_GetItemPtr( pItem );
			hrFilter = IBaseFilter_Stop( pFilter );
			if ( hrFilter != S_OK )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}

			pItem = QUARTZ_CompList_GetNext( This->m_pFilterList, pItem );
		}

		QUARTZ_CompList_Unlock( This->m_pFilterList );

		This->m_stateGraph = State_Stopped;
	}

	LeaveCriticalSection( &This->m_csGraphState );

	return hr;
}

static HRESULT WINAPI
IMediaFilter_fnPause(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);
	HRESULT	hr;
	HRESULT	hrFilter;
	QUARTZ_CompListItem*	pItem;
	IBaseFilter*	pFilter;

	TRACE("(%p)->()\n",This);

	hr = S_OK;

	EnterCriticalSection( &This->m_csGraphState );

	if ( This->m_stateGraph != State_Paused )
	{
		QUARTZ_CompList_Lock( This->m_pFilterList );

		pItem = QUARTZ_CompList_GetFirst( This->m_pFilterList );

		while ( pItem != NULL )
		{
			pFilter = (IBaseFilter*)QUARTZ_CompList_GetItemPtr( pItem );
			hrFilter = IBaseFilter_Pause( pFilter );
			if ( hrFilter != S_OK )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}

			pItem = QUARTZ_CompList_GetNext( This->m_pFilterList, pItem );
		}

		QUARTZ_CompList_Unlock( This->m_pFilterList );

		This->m_stateGraph = State_Paused;
	}

	LeaveCriticalSection( &This->m_csGraphState );

	return hr;
}

static HRESULT WINAPI
IMediaFilter_fnRun(IMediaFilter* iface,REFERENCE_TIME rtStart)
{
	CFilterGraph_THIS(iface,mediafilter);
	HRESULT	hr;
	HRESULT	hrFilter;
	QUARTZ_CompListItem*	pItem;
	IBaseFilter*	pFilter;
	IReferenceClock*	pClock;

	TRACE("(%p)->()\n",This);

	/* handle the special time. */
	if ( rtStart == (REFERENCE_TIME)0 )
	{
		hr = IMediaFilter_GetSyncSource(iface,&pClock);
		if ( hr == S_OK && pClock != NULL )
		{
			IReferenceClock_GetTime(pClock,&rtStart);
			IReferenceClock_Release(pClock);
		}
	}

	hr = S_OK;

	EnterCriticalSection( &This->m_csGraphState );

	if ( This->m_stateGraph != State_Running )
	{
		QUARTZ_CompList_Lock( This->m_pFilterList );

		pItem = QUARTZ_CompList_GetFirst( This->m_pFilterList );

		while ( pItem != NULL )
		{
			pFilter = (IBaseFilter*)QUARTZ_CompList_GetItemPtr( pItem );
			hrFilter = IBaseFilter_Run( pFilter, rtStart );
			if ( hrFilter != S_OK )
			{
				if ( SUCCEEDED(hr) )
					hr = hrFilter;
			}

			pItem = QUARTZ_CompList_GetNext( This->m_pFilterList, pItem );
		}

		QUARTZ_CompList_Unlock( This->m_pFilterList );

		This->m_stateGraph = State_Running;
	}

	LeaveCriticalSection( &This->m_csGraphState );

	return hr;
}

static HRESULT WINAPI
IMediaFilter_fnGetState(IMediaFilter* iface,DWORD dwTimeOut,FILTER_STATE* pState)
{
	CFilterGraph_THIS(iface,mediafilter);
	HRESULT	hr;
	DWORD	dwTickStart;
	DWORD	dwTickUsed;

	TRACE("(%p)->(%p)\n",This,pState);
	if ( pState == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->m_csGraphState );
	*pState = This->m_stateGraph;
	LeaveCriticalSection( &This->m_csGraphState );

	dwTickStart = GetTickCount();

	while ( 1 )
	{
		hr = CFilterGraph_PollGraphState( This, pState );
		if ( hr != VFW_S_STATE_INTERMEDIATE )
			break;
		if ( dwTimeOut == 0 )
			break;

		Sleep( (dwTimeOut >= WINE_QUARTZ_POLL_INTERVAL) ?
			WINE_QUARTZ_POLL_INTERVAL : dwTimeOut );

		dwTickUsed = GetTickCount() - dwTickStart;

		dwTickStart += dwTickUsed;
		if ( dwTimeOut <= dwTickUsed )
			dwTimeOut = 0;
		else
			dwTimeOut -= dwTickUsed;
	}

	return hr;
}

static HRESULT WINAPI
IMediaFilter_fnSetSyncSource(IMediaFilter* iface,IReferenceClock* pobjClock)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnGetSyncSource(IMediaFilter* iface,IReferenceClock** ppobjClock)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IMediaFilter) imediafilter =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaFilter_fnQueryInterface,
	IMediaFilter_fnAddRef,
	IMediaFilter_fnRelease,
	/* IPersist fields */
	IMediaFilter_fnGetClassID,
	/* IMediaFilter fields */
	IMediaFilter_fnStop,
	IMediaFilter_fnPause,
	IMediaFilter_fnRun,
	IMediaFilter_fnGetState,
	IMediaFilter_fnSetSyncSource,
	IMediaFilter_fnGetSyncSource,
};

HRESULT CFilterGraph_InitIMediaFilter( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);

	ICOM_VTBL(&pfg->mediafilter) = &imediafilter;

	InitializeCriticalSection( &pfg->m_csGraphState );
	pfg->m_stateGraph = State_Stopped;

	return NOERROR;
}

void CFilterGraph_UninitIMediaFilter( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);

	DeleteCriticalSection( &pfg->m_csGraphState );
}
