/*
 *	Implementation of CLSID_SeekingPassThru
 *
 *	FIXME - not tested yet.
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
#include "control.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "seekpass.h"


/***************************************************************************
 *
 *	CSeekingPassThru::ISeekingPassThru
 *
 */

static HRESULT WINAPI
ISeekingPassThru_fnQueryInterface(ISeekingPassThru* iface,REFIID riid,void** ppobj)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
ISeekingPassThru_fnAddRef(ISeekingPassThru* iface)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
ISeekingPassThru_fnRelease(ISeekingPassThru* iface)
{
	CSeekingPassThru_THIS(iface,seekpass);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
ISeekingPassThru_fnInit(ISeekingPassThru* iface,BOOL bRendering,IPin* pPin)
{
	CSeekingPassThru_THIS(iface,seekpass);

	FIXME("(%p)->(%d,%p) not tested!\n",This,bRendering,pPin);

	if ( pPin == NULL )
		return E_POINTER;

	/* Why is 'bRendering' given as an argument?? */
	EnterCriticalSection( &This->cs );

	if ( This->passthru.pPin != NULL )
		IPin_Release( This->passthru.pPin );
	This->passthru.pPin = pPin; IPin_AddRef( pPin );

	LeaveCriticalSection( &This->cs );

	return NOERROR;
}


static ICOM_VTABLE(ISeekingPassThru) iseekingpassthru =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	ISeekingPassThru_fnQueryInterface,
	ISeekingPassThru_fnAddRef,
	ISeekingPassThru_fnRelease,
	/* ISeekingPassThru fields */
	ISeekingPassThru_fnInit,
};

static
HRESULT CSeekingPassThru_InitISeekingPassThru(CSeekingPassThru* This)
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->seekpass) = &iseekingpassthru;
	This->passthru.punk = This->unk.punkControl;
	This->passthru.pPin = NULL;
	InitializeCriticalSection( &This->cs );

	return NOERROR;
}

static
void CSeekingPassThru_UninitISeekingPassThru(CSeekingPassThru* This)
{
	TRACE("(%p)\n",This);
	if ( This->passthru.pPin != NULL )
	{
		IPin_Release( This->passthru.pPin );
		This->passthru.pPin = NULL;
	}
	DeleteCriticalSection( &This->cs );
}

/***************************************************************************
 *
 *	new/delete for CLSID_SeekingPassThru.
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_ISeekingPassThru, offsetof(CSeekingPassThru,seekpass)-offsetof(CSeekingPassThru,unk) },
  { &IID_IMediaPosition, offsetof(CSeekingPassThru,passthru.mpos)-offsetof(CSeekingPassThru,unk) },
  { &IID_IMediaSeeking, offsetof(CSeekingPassThru,passthru.mseek)-offsetof(CSeekingPassThru,unk) },
};


static void QUARTZ_DestroySeekingPassThru(IUnknown* punk)
{
	CSeekingPassThru_THIS(punk,unk);

	CSeekingPassThru_UninitISeekingPassThru(This);
}

HRESULT QUARTZ_CreateSeekingPassThru(IUnknown* punkOuter,void** ppobj)
{
	CSeekingPassThru*	This;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CSeekingPassThru*)QUARTZ_AllocObj( sizeof(CSeekingPassThru) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, punkOuter );
	hr = CSeekingPassThru_InitISeekingPassThru(This);
	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj( This );
		return hr;
	}

	This->unk.pEntries = IFEntries;
	This->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroySeekingPassThru;

	*ppobj = (void*)(&This->unk);

	return S_OK;
}




/***************************************************************************
 *
 *	CPassThruImpl Helper methods.
 *
 */

static
HRESULT CPassThruImpl_GetConnected( CPassThruImpl* pImpl, IPin** ppPin )
{
	return IPin_ConnectedTo( pImpl->pPin, ppPin );
}

HRESULT CPassThruImpl_QueryPosPass(
	CPassThruImpl* pImpl, IMediaPosition** ppPosition )
{
	IPin*	pPin;
	HRESULT	hr;

	hr = CPassThruImpl_GetConnected( pImpl, &pPin );
	if ( FAILED(hr) )
		return hr;
	hr = IPin_QueryInterface(pPin,&IID_IMediaPosition,(void**)ppPosition);
	IPin_Release(pPin);

	return hr;
}

HRESULT CPassThruImpl_QuerySeekPass(
	CPassThruImpl* pImpl, IMediaSeeking** ppSeeking )
{
	IPin*	pPin;
	HRESULT	hr;

	hr = CPassThruImpl_GetConnected( pImpl, &pPin );
	if ( FAILED(hr) )
		return hr;
	hr = IPin_QueryInterface(pPin,&IID_IMediaSeeking,(void**)ppSeeking);
	IPin_Release(pPin);

	return hr;
}

/***************************************************************************
 *
 *	An implementation for CPassThruImpl::IMediaPosition.
 *
 */


#define QUERYPOSPASS \
	IMediaPosition* pPos = NULL; \
	HRESULT hr; \
	hr = CPassThruImpl_QueryPosPass( This, &pPos ); \
	if ( FAILED(hr) ) return hr;

static HRESULT WINAPI
IMediaPosition_fnQueryInterface(IMediaPosition* iface,REFIID riid,void** ppobj)
{
	CPassThruImpl_THIS(iface,mpos);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punk,riid,ppobj);
}

static ULONG WINAPI
IMediaPosition_fnAddRef(IMediaPosition* iface)
{
	CPassThruImpl_THIS(iface,mpos);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punk);
}

static ULONG WINAPI
IMediaPosition_fnRelease(IMediaPosition* iface)
{
	CPassThruImpl_THIS(iface,mpos);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punk);
}

static HRESULT WINAPI
IMediaPosition_fnGetTypeInfoCount(IMediaPosition* iface,UINT* pcTypeInfo)
{
	CPassThruImpl_THIS(iface,mpos);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnGetTypeInfo(IMediaPosition* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CPassThruImpl_THIS(iface,mpos);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnGetIDsOfNames(IMediaPosition* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CPassThruImpl_THIS(iface,mpos);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnInvoke(IMediaPosition* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CPassThruImpl_THIS(iface,mpos);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IMediaPosition_fnget_Duration(IMediaPosition* iface,REFTIME* prefTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_get_Duration(pPos,prefTime);
}

static HRESULT WINAPI
IMediaPosition_fnput_CurrentPosition(IMediaPosition* iface,REFTIME refTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_put_CurrentPosition(pPos,refTime);
}

static HRESULT WINAPI
IMediaPosition_fnget_CurrentPosition(IMediaPosition* iface,REFTIME* prefTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_get_CurrentPosition(pPos,prefTime);
}

static HRESULT WINAPI
IMediaPosition_fnget_StopTime(IMediaPosition* iface,REFTIME* prefTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_get_StopTime(pPos,prefTime);
}

static HRESULT WINAPI
IMediaPosition_fnput_StopTime(IMediaPosition* iface,REFTIME refTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_put_StopTime(pPos,refTime);
}

static HRESULT WINAPI
IMediaPosition_fnget_PrerollTime(IMediaPosition* iface,REFTIME* prefTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_get_PrerollTime(pPos,prefTime);
}

static HRESULT WINAPI
IMediaPosition_fnput_PrerollTime(IMediaPosition* iface,REFTIME refTime)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_put_PrerollTime(pPos,refTime);
}

static HRESULT WINAPI
IMediaPosition_fnput_Rate(IMediaPosition* iface,double dblRate)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_put_Rate(pPos,dblRate);
}

static HRESULT WINAPI
IMediaPosition_fnget_Rate(IMediaPosition* iface,double* pdblRate)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_get_Rate(pPos,pdblRate);
}

static HRESULT WINAPI
IMediaPosition_fnCanSeekForward(IMediaPosition* iface,LONG* pCanSeek)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_CanSeekForward(pPos,pCanSeek);
}

static HRESULT WINAPI
IMediaPosition_fnCanSeekBackward(IMediaPosition* iface,LONG* pCanSeek)
{
	CPassThruImpl_THIS(iface,mpos);
	QUERYPOSPASS

	TRACE("(%p)->()\n",This);

	return IMediaPosition_CanSeekBackward(pPos,pCanSeek);
}


static ICOM_VTABLE(IMediaPosition) impos =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaPosition_fnQueryInterface,
	IMediaPosition_fnAddRef,
	IMediaPosition_fnRelease,
	/* IDispatch fields */
	IMediaPosition_fnGetTypeInfoCount,
	IMediaPosition_fnGetTypeInfo,
	IMediaPosition_fnGetIDsOfNames,
	IMediaPosition_fnInvoke,
	/* IMediaPosition fields */
	IMediaPosition_fnget_Duration,
	IMediaPosition_fnput_CurrentPosition,
	IMediaPosition_fnget_CurrentPosition,
	IMediaPosition_fnget_StopTime,
	IMediaPosition_fnput_StopTime,
	IMediaPosition_fnget_PrerollTime,
	IMediaPosition_fnput_PrerollTime,
	IMediaPosition_fnput_Rate,
	IMediaPosition_fnget_Rate,
	IMediaPosition_fnCanSeekForward,
	IMediaPosition_fnCanSeekBackward,
};


HRESULT CPassThruImpl_InitIMediaPosition( CPassThruImpl* pImpl )
{
	TRACE("(%p)\n",pImpl);
	ICOM_VTBL(&pImpl->mpos) = &impos;

	return NOERROR;
}

void CPassThruImpl_UninitIMediaPosition( CPassThruImpl* pImpl )
{
	TRACE("(%p)\n",pImpl);
}

#undef QUERYPOSPASS


/***************************************************************************
 *
 *	An implementation for CPassThruImpl::IMediaSeeking.
 *
 */

#define QUERYSEEKPASS \
	IMediaSeeking* pSeek = NULL; \
	HRESULT hr; \
	hr = CPassThruImpl_QuerySeekPass( This, &pSeek ); \
	if ( FAILED(hr) ) return hr;


static HRESULT WINAPI
IMediaSeeking_fnQueryInterface(IMediaSeeking* iface,REFIID riid,void** ppobj)
{
	CPassThruImpl_THIS(iface,mseek);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->punk,riid,ppobj);
}

static ULONG WINAPI
IMediaSeeking_fnAddRef(IMediaSeeking* iface)
{
	CPassThruImpl_THIS(iface,mseek);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->punk);
}

static ULONG WINAPI
IMediaSeeking_fnRelease(IMediaSeeking* iface)
{
	CPassThruImpl_THIS(iface,mseek);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->punk);
}


static HRESULT WINAPI
IMediaSeeking_fnGetCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetCapabilities(pSeek,pdwCaps);
}

static HRESULT WINAPI
IMediaSeeking_fnCheckCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_CheckCapabilities(pSeek,pdwCaps);
}

static HRESULT WINAPI
IMediaSeeking_fnIsFormatSupported(IMediaSeeking* iface,const GUID* pidFormat)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_IsFormatSupported(pSeek,pidFormat);
}

static HRESULT WINAPI
IMediaSeeking_fnQueryPreferredFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_QueryPreferredFormat(pSeek,pidFormat);
}

static HRESULT WINAPI
IMediaSeeking_fnGetTimeFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetTimeFormat(pSeek,pidFormat);
}

static HRESULT WINAPI
IMediaSeeking_fnIsUsingTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_IsUsingTimeFormat(pSeek,pidFormat);
}

static HRESULT WINAPI
IMediaSeeking_fnSetTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_SetTimeFormat(pSeek,pidFormat);
}

static HRESULT WINAPI
IMediaSeeking_fnGetDuration(IMediaSeeking* iface,LONGLONG* pllDuration)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetDuration(pSeek,pllDuration);
}

static HRESULT WINAPI
IMediaSeeking_fnGetStopPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetStopPosition(pSeek,pllPos);
}

static HRESULT WINAPI
IMediaSeeking_fnGetCurrentPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetCurrentPosition(pSeek,pllPos);
}

static HRESULT WINAPI
IMediaSeeking_fnConvertTimeFormat(IMediaSeeking* iface,LONGLONG* pllOut,const GUID* pidFmtOut,LONGLONG llIn,const GUID* pidFmtIn)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_ConvertTimeFormat(pSeek,pllOut,pidFmtOut,llIn,pidFmtIn);
}

static HRESULT WINAPI
IMediaSeeking_fnSetPositions(IMediaSeeking* iface,LONGLONG* pllCur,DWORD dwCurFlags,LONGLONG* pllStop,DWORD dwStopFlags)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_SetPositions(pSeek,pllCur,dwCurFlags,pllStop,dwStopFlags);
}

static HRESULT WINAPI
IMediaSeeking_fnGetPositions(IMediaSeeking* iface,LONGLONG* pllCur,LONGLONG* pllStop)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetPositions(pSeek,pllCur,pllStop);
}

static HRESULT WINAPI
IMediaSeeking_fnGetAvailable(IMediaSeeking* iface,LONGLONG* pllFirst,LONGLONG* pllLast)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetAvailable(pSeek,pllFirst,pllLast);
}

static HRESULT WINAPI
IMediaSeeking_fnSetRate(IMediaSeeking* iface,double dblRate)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_SetRate(pSeek,dblRate);
}

static HRESULT WINAPI
IMediaSeeking_fnGetRate(IMediaSeeking* iface,double* pdblRate)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetRate(pSeek,pdblRate);
}

static HRESULT WINAPI
IMediaSeeking_fnGetPreroll(IMediaSeeking* iface,LONGLONG* pllPreroll)
{
	CPassThruImpl_THIS(iface,mseek);
	QUERYSEEKPASS

	TRACE("(%p)->()\n",This);

	return IMediaSeeking_GetPreroll(pSeek,pllPreroll);
}




static ICOM_VTABLE(IMediaSeeking) imseek =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaSeeking_fnQueryInterface,
	IMediaSeeking_fnAddRef,
	IMediaSeeking_fnRelease,
	/* IMediaSeeking fields */
	IMediaSeeking_fnGetCapabilities,
	IMediaSeeking_fnCheckCapabilities,
	IMediaSeeking_fnIsFormatSupported,
	IMediaSeeking_fnQueryPreferredFormat,
	IMediaSeeking_fnGetTimeFormat,
	IMediaSeeking_fnIsUsingTimeFormat,
	IMediaSeeking_fnSetTimeFormat,
	IMediaSeeking_fnGetDuration,
	IMediaSeeking_fnGetStopPosition,
	IMediaSeeking_fnGetCurrentPosition,
	IMediaSeeking_fnConvertTimeFormat,
	IMediaSeeking_fnSetPositions,
	IMediaSeeking_fnGetPositions,
	IMediaSeeking_fnGetAvailable,
	IMediaSeeking_fnSetRate,
	IMediaSeeking_fnGetRate,
	IMediaSeeking_fnGetPreroll,
};



HRESULT CPassThruImpl_InitIMediaSeeking( CPassThruImpl* pImpl )
{
	TRACE("(%p)\n",pImpl);
	ICOM_VTBL(&pImpl->mseek) = &imseek;

	return NOERROR;
}

void CPassThruImpl_UninitIMediaSeeking( CPassThruImpl* pImpl )
{
	TRACE("(%p)\n",pImpl);
}

#undef QUERYSEEKPASS
