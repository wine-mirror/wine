/*
 * Implementation of IBasicAudio, IBasicVideo, IVideoWindow for FilterGraph.
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"


static HRESULT CFilterGraph_QIFilters(
	CFilterGraph* This, REFIID riid, void** ppvobj )
{
	HRESULT hr = E_NOINTERFACE;
	DWORD	n;

	TRACE( "(%p,%p,%p)\n",This,riid,ppvobj);

	EnterCriticalSection ( &This->m_csFilters );

	for ( n = 0; n < This->m_cActiveFilters; n++ )
	{
		if ( IUnknown_QueryInterface(This->m_pActiveFilters[n].pFilter,riid,ppvobj) == S_OK )
		{
			hr = S_OK;
			break;
		}
	}

	LeaveCriticalSection ( &This->m_csFilters );

	return hr;
}


static HRESULT CFilterGraph_QueryBasicAudio(
	CFilterGraph* This, IBasicAudio** ppAudio )
{
	return CFilterGraph_QIFilters(This,&IID_IBasicAudio,(void**)ppAudio);
}

static HRESULT CFilterGraph_QueryBasicVideo(
	CFilterGraph* This, IBasicVideo** ppVideo )
{
	return CFilterGraph_QIFilters(This,&IID_IBasicVideo,(void**)ppVideo);
}

static HRESULT CFilterGraph_QueryVideoWindow(
	CFilterGraph* This, IVideoWindow** ppVidWin )
{
	return CFilterGraph_QIFilters(This,&IID_IVideoWindow,(void**)ppVidWin);
}



/***************************************************************************
 *
 *	CFilterGraph::IBasicAudio
 *
 */

#define QUERYBASICAUDIO	\
	IBasicAudio* pAudio = NULL; \
	HRESULT hr; \
	hr = CFilterGraph_QueryBasicAudio( This, &pAudio ); \
	if ( FAILED(hr) ) return hr;


static HRESULT WINAPI
IBasicAudio_fnQueryInterface(IBasicAudio* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,basaud);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IBasicAudio_fnAddRef(IBasicAudio* iface)
{
	CFilterGraph_THIS(iface,basaud);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IBasicAudio_fnRelease(IBasicAudio* iface)
{
	CFilterGraph_THIS(iface,basaud);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IBasicAudio_fnGetTypeInfoCount(IBasicAudio* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnGetTypeInfo(IBasicAudio* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnGetIDsOfNames(IBasicAudio* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnInvoke(IBasicAudio* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IBasicAudio_fnput_Volume(IBasicAudio* iface,long lVol)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	hr = IBasicAudio_put_Volume(pAudio,lVol);
	IBasicAudio_Release(pAudio);
	return hr;
}

static HRESULT WINAPI
IBasicAudio_fnget_Volume(IBasicAudio* iface,long* plVol)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	hr = IBasicAudio_get_Volume(pAudio,plVol);
	IBasicAudio_Release(pAudio);
	return hr;
}

static HRESULT WINAPI
IBasicAudio_fnput_Balance(IBasicAudio* iface,long lBalance)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	hr = IBasicAudio_put_Balance(pAudio,lBalance);
	IBasicAudio_Release(pAudio);
	return hr;
}

static HRESULT WINAPI
IBasicAudio_fnget_Balance(IBasicAudio* iface,long* plBalance)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	hr = IBasicAudio_get_Balance(pAudio,plBalance);
	IBasicAudio_Release(pAudio);
	return hr;
}


static ICOM_VTABLE(IBasicAudio) ibasicaudio =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IBasicAudio_fnQueryInterface,
	IBasicAudio_fnAddRef,
	IBasicAudio_fnRelease,
	/* IDispatch fields */
	IBasicAudio_fnGetTypeInfoCount,
	IBasicAudio_fnGetTypeInfo,
	IBasicAudio_fnGetIDsOfNames,
	IBasicAudio_fnInvoke,
	/* IBasicAudio fields */
	IBasicAudio_fnput_Volume,
	IBasicAudio_fnget_Volume,
	IBasicAudio_fnput_Balance,
	IBasicAudio_fnget_Balance,
};


HRESULT CFilterGraph_InitIBasicAudio( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->basaud) = &ibasicaudio;

	return NOERROR;
}

void CFilterGraph_UninitIBasicAudio( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

#undef QUERYBASICAUDIO


/***************************************************************************
 *
 *	CFilterGraph::IBasicVideo
 *
 */


#define QUERYBASICVIDEO	\
	IBasicVideo* pVideo = NULL; \
	HRESULT hr; \
	hr = CFilterGraph_QueryBasicVideo( This, &pVideo ); \
	if ( FAILED(hr) ) return hr;


static HRESULT WINAPI
IBasicVideo_fnQueryInterface(IBasicVideo* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IBasicVideo_fnAddRef(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IBasicVideo_fnRelease(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IBasicVideo_fnGetTypeInfoCount(IBasicVideo* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo_fnGetTypeInfo(IBasicVideo* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo_fnGetIDsOfNames(IBasicVideo* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo_fnInvoke(IBasicVideo* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IBasicVideo_fnget_AvgTimePerFrame(IBasicVideo* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_AvgTimePerFrame(pVideo,prefTime);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_BitRate(IBasicVideo* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_BitRate(pVideo,plRate);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_BitErrorRate(IBasicVideo* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_BitErrorRate(pVideo,plRate);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_VideoWidth(IBasicVideo* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_VideoWidth(pVideo,plWidth);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_VideoHeight(IBasicVideo* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_VideoHeight(pVideo,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_SourceLeft(IBasicVideo* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_SourceLeft(pVideo,lLeft);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_SourceLeft(IBasicVideo* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_SourceLeft(pVideo,plLeft);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_SourceWidth(IBasicVideo* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_SourceWidth(pVideo,lWidth);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_SourceWidth(IBasicVideo* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_SourceWidth(pVideo,plWidth);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_SourceTop(IBasicVideo* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_SourceTop(pVideo,lTop);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_SourceTop(IBasicVideo* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_SourceTop(pVideo,plTop);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_SourceHeight(IBasicVideo* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_SourceHeight(pVideo,lHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_SourceHeight(IBasicVideo* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_SourceHeight(pVideo,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_DestinationLeft(IBasicVideo* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_DestinationLeft(pVideo,lLeft);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_DestinationLeft(IBasicVideo* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_DestinationLeft(pVideo,plLeft);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_DestinationWidth(IBasicVideo* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_DestinationWidth(pVideo,lWidth);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_DestinationWidth(IBasicVideo* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_DestinationWidth(pVideo,plWidth);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_DestinationTop(IBasicVideo* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_DestinationTop(pVideo,lTop);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_DestinationTop(IBasicVideo* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_DestinationTop(pVideo,plTop);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnput_DestinationHeight(IBasicVideo* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_put_DestinationHeight(pVideo,lHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnget_DestinationHeight(IBasicVideo* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_get_DestinationHeight(pVideo,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnSetSourcePosition(IBasicVideo* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_SetSourcePosition(pVideo,lLeft,lTop,lWidth,lHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnGetSourcePosition(IBasicVideo* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_GetSourcePosition(pVideo,plLeft,plTop,plWidth,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnSetDefaultSourcePosition(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_SetDefaultSourcePosition(pVideo);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnSetDestinationPosition(IBasicVideo* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_SetDestinationPosition(pVideo,lLeft,lTop,lWidth,lHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnGetDestinationPosition(IBasicVideo* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_GetDestinationPosition(pVideo,plLeft,plTop,plWidth,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnSetDefaultDestinationPosition(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_SetDefaultDestinationPosition(pVideo);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnGetVideoSize(IBasicVideo* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_GetVideoSize(pVideo,plWidth,plHeight);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnGetVideoPaletteEntries(IBasicVideo* iface,long lStart,long lCount,long* plRet,long* plPaletteEntry)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_GetVideoPaletteEntries(pVideo,lStart,lCount,plRet,plPaletteEntry);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnGetCurrentImage(IBasicVideo* iface,long* plBufferSize,long* plDIBBuffer)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_GetCurrentImage(pVideo,plBufferSize,plDIBBuffer);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnIsUsingDefaultSource(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_IsUsingDefaultSource(pVideo);
	IBasicVideo_Release(pVideo);
	return hr;
}

static HRESULT WINAPI
IBasicVideo_fnIsUsingDefaultDestination(IBasicVideo* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	hr = IBasicVideo_IsUsingDefaultDestination(pVideo);
	IBasicVideo_Release(pVideo);
	return hr;
}



static ICOM_VTABLE(IBasicVideo) ibasicvideo =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IBasicVideo_fnQueryInterface,
	IBasicVideo_fnAddRef,
	IBasicVideo_fnRelease,
	/* IDispatch fields */
	IBasicVideo_fnGetTypeInfoCount,
	IBasicVideo_fnGetTypeInfo,
	IBasicVideo_fnGetIDsOfNames,
	IBasicVideo_fnInvoke,
	/* IBasicVideo fields */
	IBasicVideo_fnget_AvgTimePerFrame,
	IBasicVideo_fnget_BitRate,
	IBasicVideo_fnget_BitErrorRate,
	IBasicVideo_fnget_VideoWidth,
	IBasicVideo_fnget_VideoHeight,
	IBasicVideo_fnput_SourceLeft,
	IBasicVideo_fnget_SourceLeft,
	IBasicVideo_fnput_SourceWidth,
	IBasicVideo_fnget_SourceWidth,
	IBasicVideo_fnput_SourceTop,
	IBasicVideo_fnget_SourceTop,
	IBasicVideo_fnput_SourceHeight,
	IBasicVideo_fnget_SourceHeight,
	IBasicVideo_fnput_DestinationLeft,
	IBasicVideo_fnget_DestinationLeft,
	IBasicVideo_fnput_DestinationWidth,
	IBasicVideo_fnget_DestinationWidth,
	IBasicVideo_fnput_DestinationTop,
	IBasicVideo_fnget_DestinationTop,
	IBasicVideo_fnput_DestinationHeight,
	IBasicVideo_fnget_DestinationHeight,
	IBasicVideo_fnSetSourcePosition,
	IBasicVideo_fnGetSourcePosition,
	IBasicVideo_fnSetDefaultSourcePosition,
	IBasicVideo_fnSetDestinationPosition,
	IBasicVideo_fnGetDestinationPosition,
	IBasicVideo_fnSetDefaultDestinationPosition,
	IBasicVideo_fnGetVideoSize,
	IBasicVideo_fnGetVideoPaletteEntries,
	IBasicVideo_fnGetCurrentImage,
	IBasicVideo_fnIsUsingDefaultSource,
	IBasicVideo_fnIsUsingDefaultDestination,
};


HRESULT CFilterGraph_InitIBasicVideo( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->basvid) = &ibasicvideo;

	return NOERROR;
}

void CFilterGraph_UninitIBasicVideo( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

#undef QUERYBASICVIDEO

/***************************************************************************
 *
 *	CFilterGraph::IVideoWindow
 *
 */

#define QUERYVIDEOWINDOW \
	IVideoWindow* pVidWin = NULL; \
	HRESULT hr; \
	hr = CFilterGraph_QueryVideoWindow( This, &pVidWin ); \
	if ( FAILED(hr) ) return hr;


static HRESULT WINAPI
IVideoWindow_fnQueryInterface(IVideoWindow* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IVideoWindow_fnAddRef(IVideoWindow* iface)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IVideoWindow_fnRelease(IVideoWindow* iface)
{
	CFilterGraph_THIS(iface,vidwin);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfoCount(IVideoWindow* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetTypeInfo(IVideoWindow* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnGetIDsOfNames(IVideoWindow* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IVideoWindow_fnInvoke(IVideoWindow* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,vidwin);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}



static HRESULT WINAPI
IVideoWindow_fnput_Caption(IVideoWindow* iface,BSTR strCaption)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Caption(pVidWin,strCaption);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Caption(IVideoWindow* iface,BSTR* pstrCaption)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Caption(pVidWin,pstrCaption);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyle(IVideoWindow* iface,long lStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_WindowStyle(pVidWin,lStyle);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyle(IVideoWindow* iface,long* plStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_WindowStyle(pVidWin,plStyle);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyleEx(IVideoWindow* iface,long lExStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_WindowStyleEx(pVidWin,lExStyle);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyleEx(IVideoWindow* iface,long* plExStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_WindowStyleEx(pVidWin,plExStyle);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_AutoShow(IVideoWindow* iface,long lAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_AutoShow(pVidWin,lAutoShow);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_AutoShow(IVideoWindow* iface,long* plAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_AutoShow(pVidWin,plAutoShow);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowState(IVideoWindow* iface,long lState)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_WindowState(pVidWin,lState);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowState(IVideoWindow* iface,long* plState)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_WindowState(pVidWin,plState);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_BackgroundPalette(IVideoWindow* iface,long lBackPal)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_BackgroundPalette(pVidWin,lBackPal);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_BackgroundPalette(IVideoWindow* iface,long* plBackPal)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_BackgroundPalette(pVidWin,plBackPal);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Visible(IVideoWindow* iface,long lVisible)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Visible(pVidWin,lVisible);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Visible(IVideoWindow* iface,long* plVisible)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Visible(pVidWin,plVisible);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Left(IVideoWindow* iface,long lLeft)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Left(pVidWin,lLeft);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Left(IVideoWindow* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Left(pVidWin,plLeft);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Width(IVideoWindow* iface,long lWidth)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Width(pVidWin,lWidth);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Width(IVideoWindow* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr =IVideoWindow_get_Width(pVidWin,plWidth);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Top(IVideoWindow* iface,long lTop)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Top(pVidWin,lTop);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Top(IVideoWindow* iface,long* plTop)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Top(pVidWin,plTop);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Height(IVideoWindow* iface,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Height(pVidWin,lHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Height(IVideoWindow* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Height(pVidWin,plHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_Owner(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_Owner(pVidWin,hwnd);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_Owner(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_Owner(pVidWin,phwnd);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_MessageDrain(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_MessageDrain(pVidWin,hwnd);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_MessageDrain(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_MessageDrain(pVidWin,phwnd);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_BorderColor(IVideoWindow* iface,long* plColor)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_BorderColor(pVidWin,plColor);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_BorderColor(IVideoWindow* iface,long lColor)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_BorderColor(pVidWin,lColor);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnget_FullScreenMode(IVideoWindow* iface,long* plMode)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_get_FullScreenMode(pVidWin,plMode);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnput_FullScreenMode(IVideoWindow* iface,long lMode)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_put_FullScreenMode(pVidWin,lMode);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowForeground(IVideoWindow* iface,long lFocus)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_SetWindowForeground(pVidWin,lFocus);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnNotifyOwnerMessage(IVideoWindow* iface,OAHWND hwnd,long message,LONG_PTR wParam,LONG_PTR lParam)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_NotifyOwnerMessage(pVidWin,hwnd,message,wParam,lParam);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowPosition(IVideoWindow* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_SetWindowPosition(pVidWin,lLeft,lTop,lWidth,lHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnGetWindowPosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_GetWindowPosition(pVidWin,plLeft,plTop,plWidth,plHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnGetMinIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_GetMinIdealImageSize(pVidWin,plWidth,plHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnGetMaxIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_GetMaxIdealImageSize(pVidWin,plWidth,plHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnGetRestorePosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_GetRestorePosition(pVidWin,plLeft,plTop,plWidth,plHeight);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnHideCursor(IVideoWindow* iface,long lHide)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_HideCursor(pVidWin,lHide);
	IVideoWindow_Release(pVidWin);
	return hr;
}

static HRESULT WINAPI
IVideoWindow_fnIsCursorHidden(IVideoWindow* iface,long* plHide)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	hr = IVideoWindow_IsCursorHidden(pVidWin,plHide);
	IVideoWindow_Release(pVidWin);
	return hr;
}




static ICOM_VTABLE(IVideoWindow) ivideowindow =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IVideoWindow_fnQueryInterface,
	IVideoWindow_fnAddRef,
	IVideoWindow_fnRelease,
	/* IDispatch fields */
	IVideoWindow_fnGetTypeInfoCount,
	IVideoWindow_fnGetTypeInfo,
	IVideoWindow_fnGetIDsOfNames,
	IVideoWindow_fnInvoke,
	/* IVideoWindow fields */
	IVideoWindow_fnput_Caption,
	IVideoWindow_fnget_Caption,
	IVideoWindow_fnput_WindowStyle,
	IVideoWindow_fnget_WindowStyle,
	IVideoWindow_fnput_WindowStyleEx,
	IVideoWindow_fnget_WindowStyleEx,
	IVideoWindow_fnput_AutoShow,
	IVideoWindow_fnget_AutoShow,
	IVideoWindow_fnput_WindowState,
	IVideoWindow_fnget_WindowState,
	IVideoWindow_fnput_BackgroundPalette,
	IVideoWindow_fnget_BackgroundPalette,
	IVideoWindow_fnput_Visible,
	IVideoWindow_fnget_Visible,
	IVideoWindow_fnput_Left,
	IVideoWindow_fnget_Left,
	IVideoWindow_fnput_Width,
	IVideoWindow_fnget_Width,
	IVideoWindow_fnput_Top,
	IVideoWindow_fnget_Top,
	IVideoWindow_fnput_Height,
	IVideoWindow_fnget_Height,
	IVideoWindow_fnput_Owner,
	IVideoWindow_fnget_Owner,
	IVideoWindow_fnput_MessageDrain,
	IVideoWindow_fnget_MessageDrain,
	IVideoWindow_fnget_BorderColor,
	IVideoWindow_fnput_BorderColor,
	IVideoWindow_fnget_FullScreenMode,
	IVideoWindow_fnput_FullScreenMode,
	IVideoWindow_fnSetWindowForeground,
	IVideoWindow_fnNotifyOwnerMessage,
	IVideoWindow_fnSetWindowPosition,
	IVideoWindow_fnGetWindowPosition,
	IVideoWindow_fnGetMinIdealImageSize,
	IVideoWindow_fnGetMaxIdealImageSize,
	IVideoWindow_fnGetRestorePosition,
	IVideoWindow_fnHideCursor,
	IVideoWindow_fnIsCursorHidden,

};


HRESULT CFilterGraph_InitIVideoWindow( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->vidwin) = &ivideowindow;

	return NOERROR;
}

void CFilterGraph_UninitIVideoWindow( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

#undef QUERYVIDEOWINDOW
