/*
 * Implementation of IMediaEvent[Ex] for FilterGraph.
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

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnGetEvent(IMediaEventEx* iface,long* lEventCode,LONG_PTR* plParam1,LONG_PTR* plParam2,long lTimeOut)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaEventEx_fnWaitForCompletion(IMediaEventEx* iface,long lTimeOut,long* plEventCode)
{
	CFilterGraph_THIS(iface,mediaevent);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
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


void CFilterGraph_InitIMediaEventEx( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaevent) = &imediaevent;
}
