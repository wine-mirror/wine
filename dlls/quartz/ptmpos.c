/*
 * A pass-through class for IMediaPosition.
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


#define QUERYPOSPASS \
	IMediaPosition* pPos; \
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
