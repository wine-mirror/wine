/*
 * Implementation of CLSID_FilterGraph.
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
  { &IID_IPersist, offsetof(CFilterGraph,persist)-offsetof(CFilterGraph,unk) },
  { &IID_IDispatch, offsetof(CFilterGraph,disp)-offsetof(CFilterGraph,unk) },
  { &IID_IFilterGraph, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
  { &IID_IGraphBuilder, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
  { &IID_IFilterGraph2, offsetof(CFilterGraph,fgraph)-offsetof(CFilterGraph,unk) },
  { &IID_IGraphVersion, offsetof(CFilterGraph,graphversion)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaControl, offsetof(CFilterGraph,mediacontrol)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaFilter, offsetof(CFilterGraph,mediafilter)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaEvent, offsetof(CFilterGraph,mediaevent)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaEventEx, offsetof(CFilterGraph,mediaevent)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaEventSink, offsetof(CFilterGraph,mediaeventsink)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaPosition, offsetof(CFilterGraph,mediaposition)-offsetof(CFilterGraph,unk) },
  { &IID_IMediaSeeking, offsetof(CFilterGraph,mediaseeking)-offsetof(CFilterGraph,unk) },
  { &IID_IBasicVideo, offsetof(CFilterGraph,basvid)-offsetof(CFilterGraph,unk) },
  { &IID_IBasicVideo2, offsetof(CFilterGraph,basvid)-offsetof(CFilterGraph,unk) },
  { &IID_IBasicAudio, offsetof(CFilterGraph,basaud)-offsetof(CFilterGraph,unk) },
  { &IID_IVideoWindow, offsetof(CFilterGraph,vidwin)-offsetof(CFilterGraph,unk) },
};


struct FGInitEntry
{
	HRESULT (*pInit)(CFilterGraph*);
	void (*pUninit)(CFilterGraph*);
};

static const struct FGInitEntry FGRAPH_Init[] =
{
	#define	FGENT(a)	{&CFilterGraph_Init##a,&CFilterGraph_Uninit##a},

	FGENT(IPersist)
	FGENT(IDispatch)
	FGENT(IFilterGraph2)
	FGENT(IGraphVersion)
	FGENT(IMediaControl)
	FGENT(IMediaFilter)
	FGENT(IMediaEventEx)
	FGENT(IMediaEventSink)
	FGENT(IMediaPosition)
	FGENT(IMediaSeeking)
	FGENT(IBasicVideo2)
	FGENT(IBasicAudio)
	FGENT(IVideoWindow)

	#undef	FGENT
	{ NULL, NULL },
};


static void QUARTZ_DestroyFilterGraph(IUnknown* punk)
{
	CFilterGraph_THIS(punk,unk);
	int	i;

	/* At first, call Stop. */
	IMediaControl_Stop( CFilterGraph_IMediaControl(This) );
	IMediaFilter_Stop( CFilterGraph_IMediaFilter(This) );

	i = 0;
	while ( FGRAPH_Init[i].pInit != NULL )
	{
		FGRAPH_Init[i].pUninit( This );
		i++;
	}

	TRACE( "succeeded.\n" );
}

HRESULT QUARTZ_CreateFilterGraph(IUnknown* punkOuter,void** ppobj)
{
	CFilterGraph*	pfg;
	HRESULT	hr;
	int	i;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pfg = (CFilterGraph*)QUARTZ_AllocObj( sizeof(CFilterGraph) );
	if ( pfg == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pfg->unk, punkOuter );

	i = 0;
	hr = NOERROR;
	while ( FGRAPH_Init[i].pInit != NULL )
	{
		hr = FGRAPH_Init[i].pInit( pfg );
		if ( FAILED(hr) )
			break;
		i++;
	}

	if ( FAILED(hr) )
	{
		while ( --i >= 0 )
			FGRAPH_Init[i].pUninit( pfg );
		QUARTZ_FreeObj( pfg );
		return hr;
	}

	pfg->unk.pEntries = IFEntries;
	pfg->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	pfg->unk.pOnFinalRelease = QUARTZ_DestroyFilterGraph;

	*ppobj = (void*)(&pfg->unk);

	return S_OK;
}


