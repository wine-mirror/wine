/*
 * Implementation of IGraphConfig for FilterGraph.
 *
 * FIXME - stub.
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
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"



static HRESULT WINAPI
IGraphConfig_fnQueryInterface(IGraphConfig* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,grphconf);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IGraphConfig_fnAddRef(IGraphConfig* iface)
{
	CFilterGraph_THIS(iface,grphconf);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IGraphConfig_fnRelease(IGraphConfig* iface)
{
	CFilterGraph_THIS(iface,grphconf);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}



static HRESULT WINAPI
IGraphConfig_fnReconnect(IGraphConfig* iface,IPin* pOut,IPin* pIn,const AM_MEDIA_TYPE* pmt,IBaseFilter* pFilter,HANDLE hAbort,DWORD dwFlags)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnReconfigure(IGraphConfig* iface,IGraphConfigCallback* pCallback,PVOID pvParam,DWORD dwFlags,HANDLE hAbort)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnAddFilterToCache(IGraphConfig* iface,IBaseFilter* pFilter)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnEnumCacheFilter(IGraphConfig* iface,IEnumFilters** ppenum)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnRemoveFilterFromCache(IGraphConfig* iface,IBaseFilter* pFilter)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnGetStartTime(IGraphConfig* iface,REFERENCE_TIME* prt)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnPushThroughData(IGraphConfig* iface,IPin* pOut,IPinConnection* pConn,HANDLE hAbort)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnSetFilterFlags(IGraphConfig* iface,IBaseFilter* pFilter,DWORD dwFlags)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnGetFilterFlags(IGraphConfig* iface,IBaseFilter* pFilter,DWORD* pdwFlags)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IGraphConfig_fnRemoveFilterEx(IGraphConfig* iface,IBaseFilter* pFilter,DWORD dwFlags)
{
	CFilterGraph_THIS(iface,grphconf);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}





static ICOM_VTABLE(IGraphConfig) igraphconfig =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IGraphConfig_fnQueryInterface,
	IGraphConfig_fnAddRef,
	IGraphConfig_fnRelease,
	/* IGraphConfig fields */
	IGraphConfig_fnReconnect,
	IGraphConfig_fnReconfigure,
	IGraphConfig_fnAddFilterToCache,
	IGraphConfig_fnEnumCacheFilter,
	IGraphConfig_fnRemoveFilterFromCache,
	IGraphConfig_fnGetStartTime,
	IGraphConfig_fnPushThroughData,
	IGraphConfig_fnSetFilterFlags,
	IGraphConfig_fnGetFilterFlags,
	IGraphConfig_fnRemoveFilterEx,
};



HRESULT CFilterGraph_InitIGraphConfig( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->grphconf) = &igraphconfig;

	return NOERROR;
}

void CFilterGraph_UninitIGraphConfig( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

