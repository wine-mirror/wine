/*
 * Implementation of IMediaPosition for FilterGraph.
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
IMediaPosition_fnQueryInterface(IMediaPosition* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaposition);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaPosition_fnAddRef(IMediaPosition* iface)
{
	CFilterGraph_THIS(iface,mediaposition);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaPosition_fnRelease(IMediaPosition* iface)
{
	CFilterGraph_THIS(iface,mediaposition);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMediaPosition_fnGetTypeInfoCount(IMediaPosition* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnGetTypeInfo(IMediaPosition* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnGetIDsOfNames(IMediaPosition* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnInvoke(IMediaPosition* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IMediaPosition_fnget_Duration(IMediaPosition* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnput_CurrentPosition(IMediaPosition* iface,REFTIME refTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnget_CurrentPosition(IMediaPosition* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnget_StopTime(IMediaPosition* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnput_StopTime(IMediaPosition* iface,REFTIME refTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnget_PrerollTime(IMediaPosition* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnput_PrerollTime(IMediaPosition* iface,REFTIME refTime)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnput_Rate(IMediaPosition* iface,double dblRate)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnget_Rate(IMediaPosition* iface,double* pdblRate)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnCanSeekForward(IMediaPosition* iface,LONG* pCanSeek)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaPosition_fnCanSeekBackward(IMediaPosition* iface,LONG* pCanSeek)
{
	CFilterGraph_THIS(iface,mediaposition);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}


static ICOM_VTABLE(IMediaPosition) imediaposition =
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


void CFilterGraph_InitIMediaPosition( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaposition) = &imediaposition;
}
