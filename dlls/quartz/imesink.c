/*
 * Implementation of IMediaEventSink for FilterGraph.
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
IMediaEventSink_fnQueryInterface(IMediaEventSink* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IMediaEventSink_fnAddRef(IMediaEventSink* iface)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IMediaEventSink_fnRelease(IMediaEventSink* iface)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IMediaEventSink_fnNotify(IMediaEventSink* iface,long lEventCode,LONG_PTR lParam1,LONG_PTR lParam2)
{
	CFilterGraph_THIS(iface,mediaeventsink);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}


static ICOM_VTABLE(IMediaEventSink) imediaeventsink =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IMediaEventSink_fnQueryInterface,
	IMediaEventSink_fnAddRef,
	IMediaEventSink_fnRelease,
	/* IMediaEventSink fields */
	IMediaEventSink_fnNotify,
};



HRESULT CFilterGraph_InitIMediaEventSink( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->mediaeventsink) = &imediaeventsink;

	return NOERROR;
}

void CFilterGraph_UninitIMediaEventSink( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

