/*
 * Implementation of CLSID_FilterGraph.
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
#include "strmif.h"
#include "uuids.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_IFilterGraph, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
  { &IID_IGraphBuilder, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
  { &IID_IFilterGraph2, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
};

HRESULT QUARTZ_CreateFilterGraph(IUnknown* punkOuter,void** ppobj)
{
	CFilterGraph*	pfg;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfg = (CFilterGraph*)QUARTZ_AllocObj( sizeof(CFilterGraph) );
	if ( pfg == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfg->unk );
	CFilterGraph_InitIFilterGraph2( pfg );

	pfg->unk.pEntries = IFEntries;
	pfg->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);

	*ppobj = (void*)pfg;

	return S_OK;
}
