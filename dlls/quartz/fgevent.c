/*
 * CLSID_FilterGraph event handling.
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
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
#include "evcode.h"
#include "uuids.h"
#include "vfwmsgs.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"

#define EVENTQUEUE_BLOCKSIZE	16
#define EVENTQUEUE_MAX			1024

struct FilterGraph_MEDIAEVENT
{
	long		lEventCode;
	LONG_PTR	lParam1;
	LONG_PTR	lParam2;
};


/***************************************************************************
 *
 *	CLSID_FilterGraph::IMediaEvent[Ex]
 *
 */

static HRESULT WINAPI
IMediaEventEx_fnQueryInterface(IMediaEventEx* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaevent);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaEventEx_fnAddRef(IMediaEventEx* iface)
{
	CFilterGraph_THIS(iface,mediaevent);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaEventEx_fnRelease(IMediaEventEx* iface)
{
	CFilterGraph_THIS(iface,mediaevent);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMediaEventEx_fnGetTypeInfoCount(IMediaEventEx* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnGetTypeInfo(IMediaEventEx* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnGetIDsOfNames(IMediaEventEx* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnInvoke(IMediaEventEx* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IMediaEventEx_fnGetEventHandle(IMediaEventEx* iface,OAEVENT* hEvent)
{
	CFilterGraph_THIS(iface,mediaevent);

	TRACE("(%p)->()\n",This);

	*hEvent = (OAEVENT)This->m_hMediaEvent;

	return NOERROR;
}

static HRESULT WINAPI
IMediaEventEx_fnGetEvent(IMediaEventEx* iface,long* plEventCode,LONG_PTR* plParam1,LONG_PTR* plParam2,long lTimeOut)
{
	CFilterGraph_THIS(iface,mediaevent);
	ULONG cbQueued;
	DWORD dw;
	DWORD dwStart;
	HRESULT	hr;
	FilterGraph_MEDIAEVENT*	pEvent;

	TRACE("(%p)->(%p,%p,%p,%ld)\n",This,plEventCode,
		plParam1,plParam2,lTimeOut);

	if ( plEventCode == NULL || plParam1 == NULL || plParam2 == NULL )
		return E_POINTER;

	while ( 1 )
	{
		dwStart = GetTickCount();
		dw = WaitForSingleObject( This->m_hMediaEvent, lTimeOut );
		if ( dw == WAIT_TIMEOUT )
			return VFW_E_TIMEOUT;
		if ( dw != WAIT_OBJECT_0 )
			return E_FAIL;

		EnterCriticalSection( &This->m_csMediaEvents );
		hr = S_FALSE;
		if ( This->m_cbMediaEventsMax > 0 )
		{
			cbQueued =
				(This->m_cbMediaEventsMax +
				 This->m_cbMediaEventsPut - This->m_cbMediaEventsGet) %
					This->m_cbMediaEventsMax;
			if ( cbQueued > 0 )
			{
				pEvent = &This->m_pMediaEvents[This->m_cbMediaEventsGet];
				*plEventCode = pEvent->lEventCode;
				*plParam1 = pEvent->lParam1;
				*plParam2 = pEvent->lParam2;
				This->m_cbMediaEventsGet = (This->m_cbMediaEventsGet + 1) %
						This->m_cbMediaEventsMax;

				hr = NOERROR;
			}
		}
		LeaveCriticalSection( &This->m_csMediaEvents );

		if ( hr != S_FALSE )
			return hr;
		if ( lTimeOut != INFINITE )
		{
			lTimeOut -= GetTickCount() - dwStart;
			if ( lTimeOut < 0 )
				return VFW_E_TIMEOUT;
		}
	}
}

static HRESULT WINAPI
IMediaEventEx_fnWaitForCompletion(IMediaEventEx* iface,long lTimeOut,long* plEventCode)
{
	CFilterGraph_THIS(iface,mediaevent);
	HRESULT hr;
	long lEventCode;
	LONG_PTR lParam1;
	LONG_PTR lParam2;
	DWORD dwTimePrev;
	DWORD dwTimeCur;

	TRACE("(%p)->(%ld,%p)\n",This,lTimeOut,plEventCode);

	if ( plEventCode == NULL )
		return E_POINTER;
	*plEventCode = 0;

	dwTimePrev = GetTickCount();

	while ( 1 )
	{
		hr = IMediaEventEx_GetEvent(
				CFilterGraph_IMediaEventEx(This),
				&lEventCode,&lParam1,&lParam2,lTimeOut);
		if ( hr == VFW_E_TIMEOUT )
			hr = E_ABORT;
		if ( hr != NOERROR )
			return hr;
		IMediaEventEx_FreeEventParams(
				CFilterGraph_IMediaEventEx(This),
				lEventCode,lParam1,lParam2);

		if ( lEventCode == EC_COMPLETE ||
			 lEventCode == EC_ERRORABORT ||
			 lEventCode == EC_USERABORT )
		{
			*plEventCode = lEventCode;
			return NOERROR;
		}

		if ( lTimeOut != INFINITE )
		{
			dwTimeCur = GetTickCount();
			lTimeOut -= dwTimeCur - dwTimePrev;
			dwTimePrev = dwTimeCur;
			if ( lTimeOut < 0 )
				return E_ABORT;
		}
	}
}

static HRESULT WINAPI
IMediaEventEx_fnCancelDefaultHandling(IMediaEventEx* iface,long lEventCode)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnRestoreDefaultHandling(IMediaEventEx* iface,long lEventCode)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnFreeEventParams(IMediaEventEx* iface,long lEventCode,LONG_PTR lParam1,LONG_PTR lParam2)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnSetNotifyWindow(IMediaEventEx* iface,OAHWND hwnd,long message,LONG_PTR lParam)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnSetNotifyFlags(IMediaEventEx* iface,long lNotifyFlags)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnGetNotifyFlags(IMediaEventEx* iface,long* plNotifyFlags)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IMediaEventEx) imediaevent =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaEventEx_fnQueryInterface,
	IMediaEventEx_fnAddRef,
	IMediaEventEx_fnRelease,
	/* IDispatch fields */
	IMediaEventEx_fnGetTypeInfoCount,
	IMediaEventEx_fnGetTypeInfo,
	IMediaEventEx_fnGetIDsOfNames,
	IMediaEventEx_fnInvoke,
	/* IMediaEvent fields */
	IMediaEventEx_fnGetEventHandle,
	IMediaEventEx_fnGetEvent,
	IMediaEventEx_fnWaitForCompletion,
	IMediaEventEx_fnCancelDefaultHandling,
	IMediaEventEx_fnRestoreDefaultHandling,
	IMediaEventEx_fnFreeEventParams,
	/* IMediaEventEx fields */
	IMediaEventEx_fnSetNotifyWindow,
	IMediaEventEx_fnSetNotifyFlags,
	IMediaEventEx_fnGetNotifyFlags,
};


HRESULT CFilterGraph_InitIMediaEventEx( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaevent) = &imediaevent;

	pfg->m_hMediaEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
	if ( pfg->m_hMediaEvent == (HANDLE)NULL )
		return E_OUTOFMEMORY;

	InitializeCriticalSection( &pfg->m_csMediaEvents );
	pfg->m_pMediaEvents = NULL;
	pfg->m_cbMediaEventsPut = 0;
	pfg->m_cbMediaEventsGet = 0;
	pfg->m_cbMediaEventsMax = 0;

	return NOERROR;
}

void CFilterGraph_UninitIMediaEventEx( CFilterGraph* pfg )
{
	HRESULT hr;
	long lEventCode;
	LONG_PTR lParam1;
	LONG_PTR lParam2;

	TRACE("(%p)\n",pfg);

	while ( 1 )
	{
		hr = IMediaEventEx_GetEvent(
				CFilterGraph_IMediaEventEx(pfg),
				&lEventCode,&lParam1,&lParam2,0);
		if ( hr != NOERROR )
			break;
		IMediaEventEx_FreeEventParams(
				CFilterGraph_IMediaEventEx(pfg),
				lEventCode,lParam1,lParam2);
	}

	if ( pfg->m_pMediaEvents != NULL )
	{
		QUARTZ_FreeMem( pfg->m_pMediaEvents );
		pfg->m_pMediaEvents = NULL;
	}

	DeleteCriticalSection( &pfg->m_csMediaEvents );
	CloseHandle( pfg->m_hMediaEvent );
}

/***************************************************************************
 *
 *	CLSID_FilterGraph::IMediaEventSink
 *
 */

static HRESULT WINAPI
IMediaEventSink_fnQueryInterface(IMediaEventSink* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaEventSink_fnAddRef(IMediaEventSink* iface)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaEventSink_fnRelease(IMediaEventSink* iface)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMediaEventSink_fnNotify(IMediaEventSink* iface,long lEventCode,LONG_PTR lParam1,LONG_PTR lParam2)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	FIXME("(%p)->(%ld,%08x,%08x) stub!\n",This,lEventCode,lParam1,lParam2);

	return E_NOTIMPL;
}


static ICOM_VTABLE(IMediaEventSink) imediaeventsink =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaEventSink_fnQueryInterface,
	IMediaEventSink_fnAddRef,
	IMediaEventSink_fnRelease,
	/* IMediaEventSink fields */
	IMediaEventSink_fnNotify,
};



HRESULT CFilterGraph_InitIMediaEventSink( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaeventsink) = &imediaeventsink;

	return NOERROR;
}

void CFilterGraph_UninitIMediaEventSink( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

