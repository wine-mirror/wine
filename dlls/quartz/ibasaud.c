/*
 * Implementation of IBasicAudio for FilterGraph.
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

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnget_Volume(IBasicAudio* iface,long* plVol)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnput_Balance(IBasicAudio* iface,long lBalance)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IBasicAudio_fnget_Balance(IBasicAudio* iface,long* plBalance)
{
	CFilterGraph_THIS(iface,basaud);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
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

