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
#include "wine/obj_oleaut.h"
#include "strmif.h"
#include "control.h"
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
  { &IID_IMediaControl, offsetof(CFilterGraph,mediacontrol)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaEvent, offsetof(CFilterGraph,mediaevent)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaEventEx, offsetof(CFilterGraph,mediaevent)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaPosition, offsetof(CFilterGraph,mediaposition)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaSeeking, offsetof(CFilterGraph,mediaseeking)-offsetof(CFilterGraph,unk) },
  { &IID_IBasicVideo, offsetof(CFilterGraph,basvid)-offsetof(CFilterGraph,unk) },
  { &IID_IBasicAudio, offsetof(CFilterGraph,basaud)-offsetof(CFilterGraph,unk) },
  { &IID_IVideoWindow, offsetof(CFilterGraph,vidwin)-offsetof(CFilterGraph,unk) },
};

HRESULT QUARTZ_CreateFilterGraph(IUnknown* punkOuter,void** ppobj)
{
	CFilterGraph*	pfg;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfg = (CFilterGraph*)QUARTZ_AllocObj( sizeof(CFilterGraph) );
	if ( pfg == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfg->unk, punkOuter );
	CFilterGraph_InitIFilterGraph2( pfg );
	CFilterGraph_InitIMediaControl( pfg );
	CFilterGraph_InitIMediaEventEx( pfg );
	CFilterGraph_InitIMediaPosition( pfg );
	CFilterGraph_InitIMediaSeeking( pfg );
	CFilterGraph_InitIBasicVideo2( pfg );
	CFilterGraph_InitIBasicAudio( pfg );
	CFilterGraph_InitIVideoWindow( pfg );

	pfg->unk.pEntries = IFEntries;
	pfg->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);

	*ppobj = (void*)(&pfg->unk);

	return S_OK;
}
