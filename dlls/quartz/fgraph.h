#ifndef	WINE_DSHOW_FGRAPH_H
#define	WINE_DSHOW_FGRAPH_H

/*
		implements CLSID_FilterGraph.

	- At least, the following interfaces should be implemented:

	IUnknown
		+ IFilterGraph - IGraphBuilder - IFilterGraph2
		+ IDispatch - IMediaControl
		+ IDispatch - IMediaEvent - IMediaEventEx
		+ IDispatch - IMediaPosition
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

typedef struct FG_IMediaControlImpl
{
	ICOM_VFIELD(IMediaControl);
} FG_IMediaControlImpl;

typedef struct FG_IMediaEventImpl
{
	ICOM_VFIELD(IMediaEventEx);
} FG_IMediaEventImpl;

typedef struct FG_IMediaPositionImpl
{
	ICOM_VFIELD(IMediaPosition);
} FG_IMediaPositionImpl;

typedef struct FG_IMediaSeekingImpl
{
	ICOM_VFIELD(IMediaSeeking);
} FG_IMediaSeekingImpl;

typedef struct FG_IBasicVideoImpl
{
	ICOM_VFIELD(IBasicVideo2);
} FG_IBasicVideoImpl;

typedef struct FG_IBasicAudioImpl
{
	ICOM_VFIELD(IBasicAudio);
} FG_IBasicAudioImpl;

typedef struct FG_IVideoWindowImpl
{
	ICOM_VFIELD(IVideoWindow);
} FG_IVideoWindowImpl;


typedef struct CFilterGraph
{
	QUARTZ_IUnkImpl	unk;
	FG_IFilterGraph2Impl	fgraph;
	FG_IMediaControlImpl	mediacontrol;
	FG_IMediaEventImpl	mediaevent;
	FG_IMediaPositionImpl	mediaposition;
	FG_IMediaSeekingImpl	mediaseeking;
	FG_IBasicVideoImpl	basvid;
	FG_IBasicAudioImpl	basaud;
	FG_IVideoWindowImpl	vidwin;

	/* IFilterGraph2 fields. */
	/* IMediaControl fields. */
	/* IMediaEvent fields. */
	/* IMediaPosition fields. */
	/* IMediaSeeking fields. */
	/* IBasicVideo fields. */
	/* IBasicAudio fields. */
	/* IVideoWindow fields. */
} CFilterGraph;

#define	CFilterGraph_THIS(iface,member)		CFilterGraph*	This = ((CFilterGraph*)(((char*)iface)-offsetof(CFilterGraph,member)))

HRESULT QUARTZ_CreateFilterGraph(IUnknown* punkOuter,void** ppobj);

void CFilterGraph_InitIFilterGraph2( CFilterGraph* pfg );
void CFilterGraph_InitIMediaControl( CFilterGraph* pfg );
void CFilterGraph_InitIMediaEventEx( CFilterGraph* pfg );
void CFilterGraph_InitIMediaPosition( CFilterGraph* pfg );
void CFilterGraph_InitIMediaSeeking( CFilterGraph* pfg );
void CFilterGraph_InitIBasicVideo2( CFilterGraph* pfg );
void CFilterGraph_InitIBasicAudio( CFilterGraph* pfg );
void CFilterGraph_InitIVideoWindow( CFilterGraph* pfg );


#endif	/* WINE_DSHOW_FGRAPH_H */
