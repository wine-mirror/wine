/*
 * Implementation of IFilterGraph.
 *
 * FIXME - stub.
 * FIXME - implement IGraphBuilder / IFilterGraph2.
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
IFilterGraph2_fnQueryInterface(IFilterGraph2* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IFilterGraph2_fnAddRef(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IFilterGraph2_fnRelease(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IFilterGraph2_fnAddFilter(IFilterGraph2* iface,IBaseFilter* pFilter, LPCWSTR pName)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRemoveFilter(IFilterGraph2* iface,IBaseFilter* pFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnEnumFilters(IFilterGraph2* iface,IEnumFilters** ppEnum)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnFindFilterByName(IFilterGraph2* iface,LPCWSTR pName,IBaseFilter** ppFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnConnectDirect(IFilterGraph2* iface,IPin* pOut,IPin* pIn,const AM_MEDIA_TYPE* pmt)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnReconnect(IFilterGraph2* iface,IPin* pPin)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnDisconnect(IFilterGraph2* iface,IPin* pPin)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnSetDefaultSyncSource(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnConnect(IFilterGraph2* iface,IPin* pOut,IPin* pIn)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRender(IFilterGraph2* iface,IPin* pOut)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRenderFile(IFilterGraph2* iface,LPCWSTR lpFileName,LPCWSTR lpPlayList)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnAddSourceFilter(IFilterGraph2* iface,LPCWSTR lpFileName,LPCWSTR lpFilterName,IBaseFilter** ppBaseFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnSetLogFile(IFilterGraph2* iface,DWORD_PTR hFile)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnAbort(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnShouldOperationContinue(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnAddSourceFilterForMoniker(IFilterGraph2* iface,IMoniker* pMon,IBindCtx* pCtx,LPCWSTR pFilterName,IBaseFilter** ppFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnReconnectEx(IFilterGraph2* iface,IPin* pPin,const AM_MEDIA_TYPE* pmt)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRenderEx(IFilterGraph2* iface,IPin* pPin,DWORD dwParam1,DWORD* pdwParam2)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */
	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}




static ICOM_VTABLE(IFilterGraph2) ifgraph =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IFilterGraph2_fnQueryInterface,
	IFilterGraph2_fnAddRef,
	IFilterGraph2_fnRelease,
	/* IFilterGraph fields */
	IFilterGraph2_fnAddFilter,
	IFilterGraph2_fnRemoveFilter,
	IFilterGraph2_fnEnumFilters,
	IFilterGraph2_fnFindFilterByName,
	IFilterGraph2_fnConnectDirect,
	IFilterGraph2_fnReconnect,
	IFilterGraph2_fnDisconnect,
	IFilterGraph2_fnSetDefaultSyncSource,
	/* IGraphBuilder fields */
	IFilterGraph2_fnConnect,
	IFilterGraph2_fnRender,
	IFilterGraph2_fnRenderFile,
	IFilterGraph2_fnAddSourceFilter,
	IFilterGraph2_fnSetLogFile,
	IFilterGraph2_fnAbort,
	IFilterGraph2_fnShouldOperationContinue,
	/* IFilterGraph2 fields */
	IFilterGraph2_fnAddSourceFilterForMoniker,
	IFilterGraph2_fnReconnectEx,
	IFilterGraph2_fnRenderEx,
};

void CFilterGraph_InitIFilterGraph2( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->fgraph) = &ifgraph;
}
