/*
 * Implementation of IDispatch for FilterGraph.
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
IDispatch_fnQueryInterface(IDispatch* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,disp);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IDispatch_fnAddRef(IDispatch* iface)
{
	CFilterGraph_THIS(iface,disp);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IDispatch_fnRelease(IDispatch* iface)
{
	CFilterGraph_THIS(iface,disp);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IDispatch_fnGetTypeInfoCount(IDispatch* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,disp);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IDispatch_fnGetTypeInfo(IDispatch* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,disp);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IDispatch_fnGetIDsOfNames(IDispatch* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,disp);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IDispatch_fnInvoke(IDispatch* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,disp);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IDispatch) idispatch =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IDispatch_fnQueryInterface,
	IDispatch_fnAddRef,
	IDispatch_fnRelease,
	/* IDispatch fields */
	IDispatch_fnGetTypeInfoCount,
	IDispatch_fnGetTypeInfo,
	IDispatch_fnGetIDsOfNames,
	IDispatch_fnInvoke,
};


HRESULT CFilterGraph_InitIDispatch( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->disp) = &idispatch;

	return NOERROR;
}

void CFilterGraph_UninitIDispatch( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

