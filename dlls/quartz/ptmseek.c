/*
 * A pass-through class for IMediaSeeking.
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
#include "ptimpl.h"


#define QUERYSEEKPASS \
	IMediaSeeking* pSeek; \
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
