/*
 * Implementation of IGraphVersion for FilterGraph.
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
IGraphVersion_fnQueryInterface(IGraphVersion* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,graphversion);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IGraphVersion_fnAddRef(IGraphVersion* iface)
{
	CFilterGraph_THIS(iface,graphversion);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IGraphVersion_fnRelease(IGraphVersion* iface)
{
	CFilterGraph_THIS(iface,graphversion);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
IGraphVersion_fnQueryVersion(IGraphVersion* iface,LONG* plVersion)
{
	CFilterGraph_THIS(iface,graphversion);

	TRACE("(%p)->(%p)\n",This,plVersion);

	if ( plVersion == NULL )
		return E_POINTER;

	EnterCriticalSection( &This->m_csGraphVersion );
	*plVersion = This->m_lGraphVersion;
	LeaveCriticalSection( &This->m_csGraphVersion );

	return NOERROR;
}


static ICOM_VTABLE(IGraphVersion) igraphversion =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IGraphVersion_fnQueryInterface,
	IGraphVersion_fnAddRef,
	IGraphVersion_fnRelease,
	/* IGraphVersion fields */
	IGraphVersion_fnQueryVersion,
};



HRESULT CFilterGraph_InitIGraphVersion( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->graphversion) = &igraphversion;

	InitializeCriticalSection( &pfg->m_csGraphVersion );
	pfg->m_lGraphVersion = 1;

	return NOERROR;
}

void CFilterGraph_UninitIGraphVersion( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);

	DeleteCriticalSection( &pfg->m_csGraphVersion );
}

