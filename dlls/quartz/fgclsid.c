/*
 * Implementation of IPersist for FilterGraph.
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
IPersist_fnQueryInterface(IPersist* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,persist);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IPersist_fnAddRef(IPersist* iface)
{
	CFilterGraph_THIS(iface,persist);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IPersist_fnRelease(IPersist* iface)
{
	CFilterGraph_THIS(iface,persist);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IPersist_fnGetClassID(IPersist* iface,CLSID* pclsid)
{
	CFilterGraph_THIS(iface,persist);

	TRACE("(%p)->()\n",This);

	if ( pclsid == NULL )
		return E_POINTER;
	memcpy( pclsid, &CLSID_FilterGraph, sizeof(CLSID) );

	return E_NOTIMPL;
}


static ICOM_VTABLE(IPersist) ipersist =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IPersist_fnQueryInterface,
	IPersist_fnAddRef,
	IPersist_fnRelease,
	/* IPersist fields */
	IPersist_fnGetClassID,
};

HRESULT CFilterGraph_InitIPersist( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->persist) = &ipersist;

	return NOERROR;
}

void CFilterGraph_UninitIPersist( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}
