/*
 * Implementation of IMediaSeeking for FilterGraph.
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

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"



static HRESULT WINAPI
IMediaSeeking_fnQueryInterface(IMediaSeeking* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaSeeking_fnAddRef(IMediaSeeking* iface)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaSeeking_fnRelease(IMediaSeeking* iface)
{
	CFilterGraph_THIS(iface,mediaseeking);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IMediaSeeking_fnGetCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnCheckCapabilities(IMediaSeeking* iface,DWORD* pdwCaps)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnIsFormatSupported(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnQueryPreferredFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetTimeFormat(IMediaSeeking* iface,GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnIsUsingTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnSetTimeFormat(IMediaSeeking* iface,const GUID* pidFormat)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetDuration(IMediaSeeking* iface,LONGLONG* pllDuration)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetStopPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetCurrentPosition(IMediaSeeking* iface,LONGLONG* pllPos)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnConvertTimeFormat(IMediaSeeking* iface,LONGLONG* pllOut,const GUID* pidFmtOut,LONGLONG llIn,const GUID* pidFmtIn)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnSetPositions(IMediaSeeking* iface,LONGLONG* pllCur,DWORD dwCurFlags,LONGLONG* pllStop,DWORD dwStopFlags)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetPositions(IMediaSeeking* iface,LONGLONG* pllCur,LONGLONG* pllStop)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetAvailable(IMediaSeeking* iface,LONGLONG* pllFirst,LONGLONG* pllLast)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnSetRate(IMediaSeeking* iface,double dblRate)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetRate(IMediaSeeking* iface,double* pdblRate)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaSeeking_fnGetPreroll(IMediaSeeking* iface,LONGLONG* pllPreroll)
{
	CFilterGraph_THIS(iface,mediaseeking);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}




static ICOM_VTABLE(IMediaSeeking) imediaseeking =
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

void CFilterGraph_InitIMediaSeeking( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaseeking) = &imediaseeking;
}
