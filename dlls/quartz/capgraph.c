/*
 * Implementation of CLSID_CaptureGraphBuilder[2].
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
 *
 *	FIXME - stub
 *	FIXME - not tested
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
#include "capgraph.h"


/***************************************************************************
 *
 *	CCaptureGraph::ICaptureGraphBuilder
 *
 */

static HRESULT WINAPI
ICaptureGraphBuilder_fnQueryInterface(ICaptureGraphBuilder* iface,REFIID riid,void** ppobj)
{
	CCaptureGraph_THIS(iface,capgraph1);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
ICaptureGraphBuilder_fnAddRef(ICaptureGraphBuilder* iface)
{
	CCaptureGraph_THIS(iface,capgraph1);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
ICaptureGraphBuilder_fnRelease(ICaptureGraphBuilder* iface)
{
	CCaptureGraph_THIS(iface,capgraph1);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}


static HRESULT WINAPI
ICaptureGraphBuilder_fnSetFiltergraph(ICaptureGraphBuilder* iface,IGraphBuilder* pgb)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnGetFiltergraph(ICaptureGraphBuilder* iface,IGraphBuilder** ppgb)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnSetOutputFileName(ICaptureGraphBuilder* iface,const GUID* pguidType,LPCOLESTR pName,IBaseFilter** ppFilter,IFileSinkFilter** ppSink)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnFindInterface(ICaptureGraphBuilder* iface,const GUID* pguidCat,IBaseFilter* pFilter,REFIID riid,void** ppvobj)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnRenderStream(ICaptureGraphBuilder* iface,const GUID* pguidCat,IUnknown* pSource,IBaseFilter* pCompressor,IBaseFilter* pRenderer)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnControlStream(ICaptureGraphBuilder* iface,const GUID* pguidCat,IBaseFilter* pFilter,REFERENCE_TIME* prtStart,REFERENCE_TIME* prtStop,WORD wStartCookie,WORD wStopCookie)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnAllocCapFile(ICaptureGraphBuilder* iface,LPCOLESTR pName,DWORDLONG llSize)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
ICaptureGraphBuilder_fnCopyCaptureFile(ICaptureGraphBuilder* iface,LPOLESTR pOrgName,LPOLESTR pNewName,int fAllowEscAbort,IAMCopyCaptureFileProgress* pCallback)
{
	CCaptureGraph_THIS(iface,capgraph1);

	FIXME("(%p)->()\n",This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(ICaptureGraphBuilder) icapgraph1 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	ICaptureGraphBuilder_fnQueryInterface,
	ICaptureGraphBuilder_fnAddRef,
	ICaptureGraphBuilder_fnRelease,
	/* ICaptureGraphBuilder fields */
	ICaptureGraphBuilder_fnSetFiltergraph,
	ICaptureGraphBuilder_fnGetFiltergraph,
	ICaptureGraphBuilder_fnSetOutputFileName,
	ICaptureGraphBuilder_fnFindInterface,
	ICaptureGraphBuilder_fnRenderStream,
	ICaptureGraphBuilder_fnControlStream,
	ICaptureGraphBuilder_fnAllocCapFile,
	ICaptureGraphBuilder_fnCopyCaptureFile,
};


static HRESULT CCaptureGraph_InitICaptureGraphBuilder( CCaptureGraph* This )
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->capgraph1) = &icapgraph1;

	return NOERROR;
}

static void CCaptureGraph_UninitICaptureGraphBuilder( CCaptureGraph* This )
{
	TRACE("(%p)\n",This);
}


/***************************************************************************
 *
 *	new/delete for CCaptureGraph
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry IFEntries[] =
{
  { &IID_ICaptureGraphBuilder, offsetof(CCaptureGraph,capgraph1)-offsetof(CCaptureGraph,unk) },
};

static void QUARTZ_DestroyCaptureGraph(IUnknown* punk)
{
	CCaptureGraph_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CCaptureGraph_UninitICaptureGraphBuilder(This);
}

HRESULT QUARTZ_CreateCaptureGraph(IUnknown* punkOuter,void** ppobj)
{
	CCaptureGraph*	pcg;
	HRESULT	hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	pcg = (CCaptureGraph*)QUARTZ_AllocObj( sizeof(CCaptureGraph) );
	if ( pcg == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &pcg->unk, punkOuter );
	pcg->m_pfg = NULL;

	hr = CCaptureGraph_InitICaptureGraphBuilder(pcg);

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(pcg);
		return hr;
	}

	pcg->unk.pEntries = IFEntries;
	pcg->unk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	pcg->unk.pOnFinalRelease = QUARTZ_DestroyCaptureGraph;

	*ppobj = (void*)(&pcg->unk);

	return S_OK;
}
