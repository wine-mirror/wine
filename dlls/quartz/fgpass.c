/*
 * Implementation of IBasicAudio, IBasicVideo2, IVideoWindow for FilterGraph.
 *
 * FIXME - stub.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
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



static HRESULT CFilterGraph_QueryBasicAudio(
	CFilterGraph* This, IBasicAudio** ppAudio )
{
	FIXME("(%p,%p) stub!\n",This,ppAudio);
	return E_NOTIMPL;
}

static HRESULT CFilterGraph_QueryBasicVideo(
	CFilterGraph* This, IBasicVideo** ppVideo )
{
	FIXME("(%p,%p) stub!\n",This,ppVideo);
	return E_NOTIMPL;
}

static HRESULT CFilterGraph_QueryBasicVideo2(
	CFilterGraph* This, IBasicVideo2** ppVideo )
{
	FIXME("(%p,%p) stub!\n",This,ppVideo);
	return E_NOTIMPL;
}

static HRESULT CFilterGraph_QueryVideoWindow(
	CFilterGraph* This, IVideoWindow** ppVidWin )
{
	FIXME("(%p,%p) stub!\n",This,ppVidWin);
	return E_NOTIMPL;
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

	return IBasicAudio_put_Volume(pAudio,lVol);
}

static HRESULT WINAPI
IBasicAudio_fnget_Volume(IBasicAudio* iface,long* plVol)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	return IBasicAudio_get_Volume(pAudio,plVol);
}

static HRESULT WINAPI
IBasicAudio_fnput_Balance(IBasicAudio* iface,long lBalance)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	return IBasicAudio_put_Balance(pAudio,lBalance);
}

static HRESULT WINAPI
IBasicAudio_fnget_Balance(IBasicAudio* iface,long* plBalance)
{
	CFilterGraph_THIS(iface,basaud);
	QUERYBASICAUDIO

	TRACE("(%p)->()\n",This);

	return IBasicAudio_get_Balance(pAudio,plBalance);
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
 *	CFilterGraph::IBasicVideo2
 *
 */


#define QUERYBASICVIDEO	\
	IBasicVideo* pVideo = NULL; \
	HRESULT hr; \
	hr = CFilterGraph_QueryBasicVideo( This, &pVideo ); \
	if ( FAILED(hr) ) return hr;

#define QUERYBASICVIDEO2	\
	IBasicVideo2* pVideo = NULL; \
	HRESULT hr; \
	hr = CFilterGraph_QueryBasicVideo2( This, &pVideo ); \
	if ( FAILED(hr) ) return hr;


static HRESULT WINAPI
IBasicVideo2_fnQueryInterface(IBasicVideo2* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IBasicVideo2_fnAddRef(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IBasicVideo2_fnRelease(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IBasicVideo2_fnGetTypeInfoCount(IBasicVideo2* iface,UINT* pcTypeInfo)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetTypeInfo(IBasicVideo2* iface,UINT iTypeInfo, LCID lcid, ITypeInfo** ppobj)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetIDsOfNames(IBasicVideo2* iface,REFIID riid, LPOLESTR* ppwszName, UINT cNames, LCID lcid, DISPID* pDispId)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnInvoke(IBasicVideo2* iface,DISPID DispId, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarRes, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}


static HRESULT WINAPI
IBasicVideo2_fnget_AvgTimePerFrame(IBasicVideo2* iface,REFTIME* prefTime)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_AvgTimePerFrame(pVideo,prefTime);
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitRate(IBasicVideo2* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_BitRate(pVideo,plRate);
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitErrorRate(IBasicVideo2* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_BitErrorRate(pVideo,plRate);
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_VideoWidth(pVideo,plWidth);
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_VideoHeight(pVideo,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceLeft(IBasicVideo2* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_SourceLeft(pVideo,lLeft);
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceLeft(IBasicVideo2* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_SourceLeft(pVideo,plLeft);
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceWidth(IBasicVideo2* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_SourceWidth(pVideo,lWidth);
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_SourceWidth(pVideo,plWidth);
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceTop(IBasicVideo2* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_SourceTop(pVideo,lTop);
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceTop(IBasicVideo2* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_SourceTop(pVideo,plTop);
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceHeight(IBasicVideo2* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_SourceHeight(pVideo,lHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_SourceHeight(pVideo,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationLeft(IBasicVideo2* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_DestinationLeft(pVideo,lLeft);
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationLeft(IBasicVideo2* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_DestinationLeft(pVideo,plLeft);
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationWidth(IBasicVideo2* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_DestinationWidth(pVideo,lWidth);
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_DestinationWidth(pVideo,plWidth);
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationTop(IBasicVideo2* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_DestinationTop(pVideo,lTop);
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationTop(IBasicVideo2* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_DestinationTop(pVideo,plTop);
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationHeight(IBasicVideo2* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_put_DestinationHeight(pVideo,lHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_get_DestinationHeight(pVideo,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnSetSourcePosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_SetSourcePosition(pVideo,lLeft,lTop,lWidth,lHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnGetSourcePosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_GetSourcePosition(pVideo,plLeft,plTop,plWidth,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultSourcePosition(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_SetDefaultSourcePosition(pVideo);
}

static HRESULT WINAPI
IBasicVideo2_fnSetDestinationPosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_SetDestinationPosition(pVideo,lLeft,lTop,lWidth,lHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnGetDestinationPosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_GetDestinationPosition(pVideo,plLeft,plTop,plWidth,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultDestinationPosition(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_SetDefaultDestinationPosition(pVideo);
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoSize(IBasicVideo2* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_GetVideoSize(pVideo,plWidth,plHeight);
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoPaletteEntries(IBasicVideo2* iface,long lStart,long lCount,long* plRet,long* plPaletteEntry)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_GetVideoPaletteEntries(pVideo,lStart,lCount,plRet,plPaletteEntry);
}

static HRESULT WINAPI
IBasicVideo2_fnGetCurrentImage(IBasicVideo2* iface,long* plBufferSize,long* plDIBBuffer)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_GetCurrentImage(pVideo,plBufferSize,plDIBBuffer);
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultSource(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_IsUsingDefaultSource(pVideo);
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultDestination(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO

	TRACE("(%p)->()\n",This);

	return IBasicVideo_IsUsingDefaultDestination(pVideo);
}

static HRESULT WINAPI
IBasicVideo2_fnGetPreferredAspectRatio(IBasicVideo2* iface,long* plRateX,long* plRateY)
{
	CFilterGraph_THIS(iface,basvid);
	QUERYBASICVIDEO2

	TRACE("(%p)->()\n",This);

	return IBasicVideo2_GetPreferredAspectRatio(pVideo,plRateX,plRateY);
}




static ICOM_VTABLE(IBasicVideo2) ibasicvideo =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IBasicVideo2_fnQueryInterface,
	IBasicVideo2_fnAddRef,
	IBasicVideo2_fnRelease,
	/* IDispatch fields */
	IBasicVideo2_fnGetTypeInfoCount,
	IBasicVideo2_fnGetTypeInfo,
	IBasicVideo2_fnGetIDsOfNames,
	IBasicVideo2_fnInvoke,
	/* IBasicVideo fields */
	IBasicVideo2_fnget_AvgTimePerFrame,
	IBasicVideo2_fnget_BitRate,
	IBasicVideo2_fnget_BitErrorRate,
	IBasicVideo2_fnget_VideoWidth,
	IBasicVideo2_fnget_VideoHeight,
	IBasicVideo2_fnput_SourceLeft,
	IBasicVideo2_fnget_SourceLeft,
	IBasicVideo2_fnput_SourceWidth,
	IBasicVideo2_fnget_SourceWidth,
	IBasicVideo2_fnput_SourceTop,
	IBasicVideo2_fnget_SourceTop,
	IBasicVideo2_fnput_SourceHeight,
	IBasicVideo2_fnget_SourceHeight,
	IBasicVideo2_fnput_DestinationLeft,
	IBasicVideo2_fnget_DestinationLeft,
	IBasicVideo2_fnput_DestinationWidth,
	IBasicVideo2_fnget_DestinationWidth,
	IBasicVideo2_fnput_DestinationTop,
	IBasicVideo2_fnget_DestinationTop,
	IBasicVideo2_fnput_DestinationHeight,
	IBasicVideo2_fnget_DestinationHeight,
	IBasicVideo2_fnSetSourcePosition,
	IBasicVideo2_fnGetSourcePosition,
	IBasicVideo2_fnSetDefaultSourcePosition,
	IBasicVideo2_fnSetDestinationPosition,
	IBasicVideo2_fnGetDestinationPosition,
	IBasicVideo2_fnSetDefaultDestinationPosition,
	IBasicVideo2_fnGetVideoSize,
	IBasicVideo2_fnGetVideoPaletteEntries,
	IBasicVideo2_fnGetCurrentImage,
	IBasicVideo2_fnIsUsingDefaultSource,
	IBasicVideo2_fnIsUsingDefaultDestination,
	/* IBasicVideo2 fields */
	IBasicVideo2_fnGetPreferredAspectRatio,
};


HRESULT CFilterGraph_InitIBasicVideo2( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->basvid) = &ibasicvideo;

	return NOERROR;
}

void CFilterGraph_UninitIBasicVideo2( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
}

#undef QUERYBASICVIDEO2
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

	return IVideoWindow_put_Caption(pVidWin,strCaption);
}

static HRESULT WINAPI
IVideoWindow_fnget_Caption(IVideoWindow* iface,BSTR* pstrCaption)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Caption(pVidWin,pstrCaption);
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyle(IVideoWindow* iface,long lStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_WindowStyle(pVidWin,lStyle);
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyle(IVideoWindow* iface,long* plStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_WindowStyle(pVidWin,plStyle);
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowStyleEx(IVideoWindow* iface,long lExStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_WindowStyleEx(pVidWin,lExStyle);
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowStyleEx(IVideoWindow* iface,long* plExStyle)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_WindowStyleEx(pVidWin,plExStyle);
}

static HRESULT WINAPI
IVideoWindow_fnput_AutoShow(IVideoWindow* iface,long lAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_AutoShow(pVidWin,lAutoShow);
}

static HRESULT WINAPI
IVideoWindow_fnget_AutoShow(IVideoWindow* iface,long* plAutoShow)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_AutoShow(pVidWin,plAutoShow);
}

static HRESULT WINAPI
IVideoWindow_fnput_WindowState(IVideoWindow* iface,long lState)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_WindowState(pVidWin,lState);
}

static HRESULT WINAPI
IVideoWindow_fnget_WindowState(IVideoWindow* iface,long* plState)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_WindowState(pVidWin,plState);
}

static HRESULT WINAPI
IVideoWindow_fnput_BackgroundPalette(IVideoWindow* iface,long lBackPal)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_BackgroundPalette(pVidWin,lBackPal);
}

static HRESULT WINAPI
IVideoWindow_fnget_BackgroundPalette(IVideoWindow* iface,long* plBackPal)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_BackgroundPalette(pVidWin,plBackPal);
}

static HRESULT WINAPI
IVideoWindow_fnput_Visible(IVideoWindow* iface,long lVisible)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Visible(pVidWin,lVisible);
}

static HRESULT WINAPI
IVideoWindow_fnget_Visible(IVideoWindow* iface,long* plVisible)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Visible(pVidWin,plVisible);
}

static HRESULT WINAPI
IVideoWindow_fnput_Left(IVideoWindow* iface,long lLeft)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Left(pVidWin,lLeft);
}

static HRESULT WINAPI
IVideoWindow_fnget_Left(IVideoWindow* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Left(pVidWin,plLeft);
}

static HRESULT WINAPI
IVideoWindow_fnput_Width(IVideoWindow* iface,long lWidth)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Width(pVidWin,lWidth);
}

static HRESULT WINAPI
IVideoWindow_fnget_Width(IVideoWindow* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Width(pVidWin,plWidth);
}

static HRESULT WINAPI
IVideoWindow_fnput_Top(IVideoWindow* iface,long lTop)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Top(pVidWin,lTop);
}

static HRESULT WINAPI
IVideoWindow_fnget_Top(IVideoWindow* iface,long* plTop)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Top(pVidWin,plTop);
}

static HRESULT WINAPI
IVideoWindow_fnput_Height(IVideoWindow* iface,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Height(pVidWin,lHeight);
}

static HRESULT WINAPI
IVideoWindow_fnget_Height(IVideoWindow* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Height(pVidWin,plHeight);
}

static HRESULT WINAPI
IVideoWindow_fnput_Owner(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_Owner(pVidWin,hwnd);
}

static HRESULT WINAPI
IVideoWindow_fnget_Owner(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_Owner(pVidWin,phwnd);
}

static HRESULT WINAPI
IVideoWindow_fnput_MessageDrain(IVideoWindow* iface,OAHWND hwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_MessageDrain(pVidWin,hwnd);
}

static HRESULT WINAPI
IVideoWindow_fnget_MessageDrain(IVideoWindow* iface,OAHWND* phwnd)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_MessageDrain(pVidWin,phwnd);
}

static HRESULT WINAPI
IVideoWindow_fnget_BorderColor(IVideoWindow* iface,long* plColor)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_BorderColor(pVidWin,plColor);
}

static HRESULT WINAPI
IVideoWindow_fnput_BorderColor(IVideoWindow* iface,long lColor)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_BorderColor(pVidWin,lColor);
}

static HRESULT WINAPI
IVideoWindow_fnget_FullScreenMode(IVideoWindow* iface,long* plMode)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_get_FullScreenMode(pVidWin,plMode);
}

static HRESULT WINAPI
IVideoWindow_fnput_FullScreenMode(IVideoWindow* iface,long lMode)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_put_FullScreenMode(pVidWin,lMode);
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowForeground(IVideoWindow* iface,long lFocus)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_SetWindowForeground(pVidWin,lFocus);
}

static HRESULT WINAPI
IVideoWindow_fnNotifyOwnerMessage(IVideoWindow* iface,OAHWND hwnd,long message,LONG_PTR wParam,LONG_PTR lParam)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_NotifyOwnerMessage(pVidWin,hwnd,message,wParam,lParam);
}

static HRESULT WINAPI
IVideoWindow_fnSetWindowPosition(IVideoWindow* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_SetWindowPosition(pVidWin,lLeft,lTop,lWidth,lHeight);
}

static HRESULT WINAPI
IVideoWindow_fnGetWindowPosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_GetWindowPosition(pVidWin,plLeft,plTop,plWidth,plHeight);
}

static HRESULT WINAPI
IVideoWindow_fnGetMinIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_GetMinIdealImageSize(pVidWin,plWidth,plHeight);
}

static HRESULT WINAPI
IVideoWindow_fnGetMaxIdealImageSize(IVideoWindow* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_GetMaxIdealImageSize(pVidWin,plWidth,plHeight);
}

static HRESULT WINAPI
IVideoWindow_fnGetRestorePosition(IVideoWindow* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_GetRestorePosition(pVidWin,plLeft,plTop,plWidth,plHeight);
}

static HRESULT WINAPI
IVideoWindow_fnHideCursor(IVideoWindow* iface,long lHide)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_HideCursor(pVidWin,lHide);
}

static HRESULT WINAPI
IVideoWindow_fnIsCursorHidden(IVideoWindow* iface,long* plHide)
{
	CFilterGraph_THIS(iface,vidwin);
	QUERYVIDEOWINDOW

	TRACE("(%p)->()\n",This);

	return IVideoWindow_IsCursorHidden(pVidWin,plHide);
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
