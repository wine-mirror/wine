#ifndef	WINE_DSHOW_FGRAPH_H
#define	WINE_DSHOW_FGRAPH_H

/*
		implements CLSID_FilterGraph.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IFilterGraph - IGraphBuilder - IFilterGraph2
		+ IDispatch - IMediaControl
		+ IDispatch - IMediaEvent - IMediaEventEx
		+ IMediaSeeking
		+ IDispatch - IBasicVideo (pass to a renderer)
		+ IDispatch - IBasicAudio (pass to a renderer)
		+ IDispatch - IVideoWindow  (pass to a renderer)
 */

#include "iunk.h"

typedef struct FG_IFilterGraph2Impl
{
	ICOM_VFIELD(IFilterGraph2);
} FG_IFilterGraph2Impl;

typedef struct CFilterGraph
{
	QUARTZ_IUnkImpl	unk;
	FG_IFilterGraph2Impl	fgraph;

	/* IFilterGraph2 fields. */
} CFilterGraph;

#define	CFilterGraph_THIS(iface,member)		CFilterGraph*	This = ((CFilterGraph*)(((char*)iface)-offsetof(CFilterGraph,member)))

HRESULT QUARTZ_CreateFilterGraph(IUnknown* punkOuter,void** ppobj);

void CFilterGraph_InitIFilterGraph2( CFilterGraph* pfg );


#endif	/* WINE_DSHOW_FGRAPH_H */
