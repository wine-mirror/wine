/*
 * Implementation of IBasicVideo2 for FilterGraph.
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

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitRate(IBasicVideo2* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_BitErrorRate(IBasicVideo2* iface,long* plRate)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_VideoHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceLeft(IBasicVideo2* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceLeft(IBasicVideo2* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceWidth(IBasicVideo2* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceTop(IBasicVideo2* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceTop(IBasicVideo2* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_SourceHeight(IBasicVideo2* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_SourceHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationLeft(IBasicVideo2* iface,long lLeft)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationLeft(IBasicVideo2* iface,long* plLeft)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationWidth(IBasicVideo2* iface,long lWidth)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationWidth(IBasicVideo2* iface,long* plWidth)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationTop(IBasicVideo2* iface,long lTop)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationTop(IBasicVideo2* iface,long* plTop)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnput_DestinationHeight(IBasicVideo2* iface,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnget_DestinationHeight(IBasicVideo2* iface,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetSourcePosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetSourcePosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultSourcePosition(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDestinationPosition(IBasicVideo2* iface,long lLeft,long lTop,long lWidth,long lHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetDestinationPosition(IBasicVideo2* iface,long* plLeft,long* plTop,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnSetDefaultDestinationPosition(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoSize(IBasicVideo2* iface,long* plWidth,long* plHeight)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetVideoPaletteEntries(IBasicVideo2* iface,long lStart,long lCount,long* plRet,long* plPaletteEntry)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetCurrentImage(IBasicVideo2* iface,long* plBufferSize,long* plDIBBuffer)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultSource(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnIsUsingDefaultDestination(IBasicVideo2* iface)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicVideo2_fnGetPreferredAspectRatio(IBasicVideo2* iface,long* plRateX,long* plRateY)
{
	CFilterGraph_THIS(iface,basvid);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
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


void CFilterGraph_InitIBasicVideo2( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->basvid) = &ibasicvideo;
}

