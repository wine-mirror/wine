/*
 * Implementation of IMediaFilter for FilterGraph.
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
IMediaFilter_fnQueryInterface(IMediaFilter* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaFilter_fnAddRef(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaFilter_fnRelease(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IMediaFilter_fnGetClassID(IMediaFilter* iface,CLSID* pclsid)
{
	CFilterGraph_THIS(iface,mediafilter);

	TRACE("(%p)->()\n",This);

	return IPersist_GetClassID(
		CFilterGraph_IPersist(This),pclsid);
}

static HRESULT WINAPI
IMediaFilter_fnStop(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnPause(IMediaFilter* iface)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnRun(IMediaFilter* iface,REFERENCE_TIME rtStart)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnGetState(IMediaFilter* iface,DWORD dw,FILTER_STATE* pState)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnSetSyncSource(IMediaFilter* iface,IReferenceClock* pobjClock)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IMediaFilter_fnGetSyncSource(IMediaFilter* iface,IReferenceClock** ppobjClock)
{
	CFilterGraph_THIS(iface,mediafilter);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}



static ICOM_VTABLE(IMediaFilter) imediafilter =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaFilter_fnQueryInterface,
	IMediaFilter_fnAddRef,
	IMediaFilter_fnRelease,
	/* IPersist fields */
	IMediaFilter_fnGetClassID,
	/* IMediaFilter fields */
	IMediaFilter_fnStop,
	IMediaFilter_fnPause,
	IMediaFilter_fnRun,
	IMediaFilter_fnGetState,
	IMediaFilter_fnSetSyncSource,
	IMediaFilter_fnGetSyncSource,
};

HRESULT CFilterGraph_InitIMediaFilter( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediafilter) = &imediafilter;

	return NOERROR;
}

void CFilterGraph_UninitIMediaFilter( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}
