/*
 * Implementation of IFilterGraph.
 *
 * FIXME - stub.
 * FIXME - implement IGraphBuilder / IFilterGraph2.
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
#include "vfwmsgs.h"
#include "wine/unicode.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "fgraph.h"
#include "enumunk.h"


static HRESULT CFilterGraph_DisconnectAllPins( IBaseFilter* pFilter )
{
	IEnumPins*	pEnum = NULL;
	IPin*	pPin;
	IPin*	pConnTo;
	ULONG	cFetched;
	HRESULT	hr;

	hr = IBaseFilter_EnumPins( pFilter, &pEnum );
	if ( FAILED(hr) )
		return hr;
	if ( pEnum == NULL )
		return E_FAIL;

	while ( 1 )
	{
		pPin = NULL;
		cFetched = 0;
		hr = IEnumPins_Next( pEnum, 1, &pPin, &cFetched );
		if ( FAILED(hr) )
			break;
		if ( hr != NOERROR || pPin == NULL || cFetched != 1 )
		{
			hr = NOERROR;
			break;
		}

		pConnTo = NULL;
		hr = IPin_ConnectedTo(pPin,&pConnTo);
		if ( hr == NOERROR && pConnTo != NULL )
		{
			IPin_Disconnect(pPin);
			IPin_Disconnect(pConnTo);
			IPin_Release(pConnTo);
		}

		IPin_Release( pPin );
	}

	IEnumPins_Release( pEnum );

	return hr;
}


/****************************************************************************/

static HRESULT WINAPI
IFilterGraph2_fnQueryInterface(IFilterGraph2* iface,REFIID riid,void** ppobj)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IFilterGraph2_fnAddRef(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IFilterGraph2_fnRelease(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IFilterGraph2_fnAddFilter(IFilterGraph2* iface,IBaseFilter* pFilter, LPCWSTR pName)
{
	CFilterGraph_THIS(iface,fgraph);
	FILTER_INFO	info;
	HRESULT	hr;
	HRESULT	hrSucceeded = S_OK;
	QUARTZ_CompListItem*	pItem;
	int i,iLen;

	TRACE( "(%p)->(%p,%s)\n",This,pFilter,debugstr_w(pName) );

	QUARTZ_CompList_Lock( This->m_pFilterList );

	if ( pName != NULL )
	{
		pItem = QUARTZ_CompList_SearchData(
			This->m_pFilterList,
			pName, sizeof(WCHAR)*(strlenW(pName)+1) );
		if ( pItem == NULL )
			goto name_ok;

		hrSucceeded = VFW_S_DUPLICATE_NAME;

		iLen = strlenW(pName);
		if ( iLen > 32 )
			iLen = 32;
		memcpy( info.achName, pName, sizeof(WCHAR)*iLen );
		info.achName[iLen] = 0;
	}
	else
	{
		ZeroMemory( &info, sizeof(info) );
		hr = IBaseFilter_QueryFilterInfo( pFilter, &info );
		if ( FAILED(hr) )
			goto end;

		iLen = strlenW(info.achName);
		pItem = QUARTZ_CompList_SearchData(
			This->m_pFilterList,
			info.achName, sizeof(WCHAR)*(iLen+1) );
		if ( pItem == NULL )
		{
			pName = info.achName;
			goto name_ok;
		}
	}

	/* generate modified names for this filter.. */
	iLen = strlenW(info.achName);
	if ( iLen > 32 )
		iLen = 32;
	info.achName[iLen++] = ' ';

	for ( i = 0; i <= 99; i++ )
	{
		info.achName[iLen+0] = (i%10) + '0';
		info.achName[iLen+1] = ((i/10)%10) + '0';
		info.achName[iLen+2] = 0;
		pItem = QUARTZ_CompList_SearchData(
			This->m_pFilterList,
			info.achName, sizeof(WCHAR)*(iLen+3) );
		if ( pItem == NULL )
		{
			pName = info.achName;
			goto name_ok;
		}
	}

	hr = ( pName == NULL ) ? E_FAIL : VFW_E_DUPLICATE_NAME;
	goto end;

name_ok:
	/* register this filter. */
	hr = QUARTZ_CompList_AddComp(
		This->m_pFilterList, (IUnknown*)pFilter,
		pName, sizeof(WCHAR)*(strlenW(pName)+1) );
	if ( FAILED(hr) )
		goto end;

	hr = IBaseFilter_JoinFilterGraph(pFilter,(IFilterGraph*)iface,pName);
	if ( FAILED(hr) )
	{
		QUARTZ_CompList_RemoveComp(
			This->m_pFilterList,(IUnknown*)pFilter);
		goto end;
	}

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

	hr = hrSucceeded;
end:
	QUARTZ_CompList_Unlock( This->m_pFilterList );

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnRemoveFilter(IFilterGraph2* iface,IBaseFilter* pFilter)
{
	CFilterGraph_THIS(iface,fgraph);
	QUARTZ_CompListItem*	pItem;
	HRESULT	hr = NOERROR;

	TRACE( "(%p)->(%p)\n",This,pFilter );

	QUARTZ_CompList_Lock( This->m_pFilterList );

	pItem = QUARTZ_CompList_SearchComp(
		This->m_pFilterList, (IUnknown*)pFilter );
	if ( pItem != NULL )
	{
		CFilterGraph_DisconnectAllPins(pFilter);
		hr = IBaseFilter_JoinFilterGraph(
			pFilter, NULL, QUARTZ_CompList_GetDataPtr(pItem) );
		QUARTZ_CompList_RemoveComp(
			This->m_pFilterList, (IUnknown*)pFilter );
	}

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

	QUARTZ_CompList_Unlock( This->m_pFilterList );

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnEnumFilters(IFilterGraph2* iface,IEnumFilters** ppEnum)
{
	CFilterGraph_THIS(iface,fgraph);
	HRESULT	hr;

	TRACE( "(%p)->(%p)\n",This,ppEnum );

	QUARTZ_CompList_Lock( This->m_pFilterList );

	hr = QUARTZ_CreateEnumUnknown(
		&IID_IEnumFilters, (void**)ppEnum, This->m_pFilterList );

	QUARTZ_CompList_Unlock( This->m_pFilterList );

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnFindFilterByName(IFilterGraph2* iface,LPCWSTR pName,IBaseFilter** ppFilter)
{
	CFilterGraph_THIS(iface,fgraph);
	QUARTZ_CompListItem*	pItem;
	HRESULT	hr = E_FAIL;

	TRACE( "(%p)->(%s,%p)\n",This,debugstr_w(pName),ppFilter );

	if ( ppFilter == NULL )
		return E_POINTER;
	*ppFilter = NULL;

	QUARTZ_CompList_Lock( This->m_pFilterList );

	pItem = QUARTZ_CompList_SearchData(
		This->m_pFilterList,
		pName, sizeof(WCHAR)*(strlenW(pName)+1) );
	if ( pItem != NULL )
	{
		*ppFilter = (IBaseFilter*)QUARTZ_CompList_GetItemPtr(pItem);
		hr = NOERROR;
	}

	QUARTZ_CompList_Unlock( This->m_pFilterList );

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnConnectDirect(IFilterGraph2* iface,IPin* pOut,IPin* pIn,const AM_MEDIA_TYPE* pmt)
{
	CFilterGraph_THIS(iface,fgraph);
	IPin*	pConnTo;
	PIN_INFO	infoIn;
	PIN_INFO	infoOut;
	FILTER_INFO	finfoIn;
	FILTER_INFO	finfoOut;
	HRESULT	hr;

	TRACE( "(%p)->(%p,%p,%p)\n",This,pOut,pIn,pmt );

	infoIn.pFilter = NULL;
	infoOut.pFilter = NULL;
	finfoIn.pGraph = NULL;
	finfoOut.pGraph = NULL;

	QUARTZ_CompList_Lock( This->m_pFilterList );

	hr = IPin_QueryPinInfo(pIn,&infoIn);
	if ( FAILED(hr) )
		goto end;
	hr = IPin_QueryPinInfo(pOut,&infoOut);
	if ( FAILED(hr) )
		goto end;
	if ( infoIn.pFilter == NULL || infoOut.pFilter == NULL ||
		 infoIn.dir != PINDIR_INPUT || infoOut.dir != PINDIR_OUTPUT )
	{
		hr = E_FAIL;
		goto end;
	}

	hr = IBaseFilter_QueryFilterInfo(infoIn.pFilter,&finfoIn);
	if ( FAILED(hr) )
		goto end;
	hr = IBaseFilter_QueryFilterInfo(infoOut.pFilter,&finfoOut);
	if ( FAILED(hr) )
		goto end;
	if ( finfoIn.pGraph != ((IFilterGraph*)iface) ||
		 finfoOut.pGraph != ((IFilterGraph*)iface) )
	{
		hr = E_FAIL;
		goto end;
	}

	pConnTo = NULL;
	hr = IPin_ConnectedTo(pIn,&pConnTo);
	if ( FAILED(hr) )
		goto end;
	if ( pConnTo != NULL )
	{
		IPin_Release(pConnTo);
		goto end;
	}

	pConnTo = NULL;
	hr = IPin_ConnectedTo(pOut,&pConnTo);
	if ( FAILED(hr) )
		goto end;
	if ( pConnTo != NULL )
	{
		IPin_Release(pConnTo);
		goto end;
	}

	hr = IPin_Connect(pIn,pOut,pmt);
	if ( FAILED(hr) )
		goto end;
	hr = IPin_Connect(pOut,pIn,pmt);
	if ( FAILED(hr) )
	{
		IPin_Disconnect(pIn);
		goto end;
	}

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

end:
	QUARTZ_CompList_Unlock( This->m_pFilterList );

	if ( infoIn.pFilter != NULL )
		IBaseFilter_Release(infoIn.pFilter);
	if ( infoOut.pFilter != NULL )
		IBaseFilter_Release(infoOut.pFilter);
	if ( finfoIn.pGraph != NULL )
		IFilterGraph_Release(finfoIn.pGraph);
	if ( finfoOut.pGraph != NULL )
		IFilterGraph_Release(finfoOut.pGraph);

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnReconnect(IFilterGraph2* iface,IPin* pPin)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->(%p) stub!\n",This,pPin );

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnDisconnect(IFilterGraph2* iface,IPin* pPin)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->(%p) stub!\n",This,pPin );

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnSetDefaultSyncSource(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnConnect(IFilterGraph2* iface,IPin* pOut,IPin* pIn)
{
	CFilterGraph_THIS(iface,fgraph);
	HRESULT	hr;

	TRACE( "(%p)->(%p,%p)\n",This,pOut,pIn );

	/* At first, try to connect directly. */
	hr = IFilterGraph_ConnectDirect(iface,pOut,pIn,NULL);
	if ( hr == NOERROR )
		return NOERROR;

	/* FIXME - try to connect indirectly. */
	FIXME( "(%p)->(%p,%p) stub!\n",This,pOut,pIn );


	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRender(IFilterGraph2* iface,IPin* pOut)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->(%p) stub!\n",This,pOut );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRenderFile(IFilterGraph2* iface,LPCWSTR lpFileName,LPCWSTR lpPlayList)
{
	CFilterGraph_THIS(iface,fgraph);
	HRESULT	hr;
	IBaseFilter*	pFilter = NULL;
	IEnumPins*	pEnum = NULL;
	IPin*	pPin;
	ULONG	cFetched;
	PIN_DIRECTION	dir;
	ULONG	cTryToRender;
	ULONG	cActRender;

	TRACE( "(%p)->(%s,%s)\n",This,
		debugstr_w(lpFileName),debugstr_w(lpPlayList) );

	if ( lpPlayList != NULL )
		return E_INVALIDARG;

	pFilter = NULL;
	hr = IFilterGraph2_AddSourceFilter(iface,lpFileName,NULL,&pFilter);
	if ( FAILED(hr) )
		goto end;
	if ( pFilter == NULL )
	{
		hr = E_FAIL;
		goto end;
	}
	pEnum = NULL;
	hr = IBaseFilter_EnumPins( pFilter, &pEnum );
	if ( FAILED(hr) )
		goto end;
	if ( pEnum == NULL )
	{
		hr = E_FAIL;
		goto end;
	}

	cTryToRender = 0;
	cActRender = 0;

	while ( 1 )
	{
		pPin = NULL;
		cFetched = 0;
		hr = IEnumPins_Next( pEnum, 1, &pPin, &cFetched );
		if ( FAILED(hr) )
			goto end;
		if ( hr != NOERROR || pPin == NULL || cFetched != 1 )
		{
			hr = NOERROR;
			break;
		}
		hr = IPin_QueryDirection( pPin, &dir );
		if ( hr == NOERROR && dir == PINDIR_OUTPUT )
		{
			cTryToRender ++;
			hr = IFilterGraph2_Render( iface, pPin );
			if ( hr == NOERROR )
				cActRender ++;
		}
		IPin_Release( pPin );
	}

	if ( hr == NOERROR )
	{
		if ( cTryToRender > cActRender )
			hr = VFW_S_PARTIAL_RENDER;
		if ( cActRender == 0 )
			hr = E_FAIL;
	}

end:
	if ( pEnum != NULL )
		IEnumPins_Release( pEnum );
	if ( pFilter != NULL )
		IBaseFilter_Release( pFilter );

	return hr;
}

static HRESULT WINAPI
IFilterGraph2_fnAddSourceFilter(IFilterGraph2* iface,LPCWSTR lpFileName,LPCWSTR lpFilterName,IBaseFilter** ppBaseFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->(%s,%s,%p) stub!\n",This,
		debugstr_w(lpFileName),debugstr_w(lpFilterName),ppBaseFilter );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnSetLogFile(IFilterGraph2* iface,DWORD_PTR hFile)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnAbort(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnShouldOperationContinue(IFilterGraph2* iface)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnAddSourceFilterForMoniker(IFilterGraph2* iface,IMoniker* pMon,IBindCtx* pCtx,LPCWSTR pFilterName,IBaseFilter** ppFilter)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->() stub!\n", This );
	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnReconnectEx(IFilterGraph2* iface,IPin* pPin,const AM_MEDIA_TYPE* pmt)
{
	CFilterGraph_THIS(iface,fgraph);

	FIXME( "(%p)->(%p,%p) stub!\n",This,pPin,pmt );

	EnterCriticalSection( &This->m_csGraphVersion );
	This->m_lGraphVersion ++;
	LeaveCriticalSection( &This->m_csGraphVersion );

	return E_NOTIMPL;
}

static HRESULT WINAPI
IFilterGraph2_fnRenderEx(IFilterGraph2* iface,IPin* pPin,DWORD dwParam1,DWORD* pdwParam2)
{
	CFilterGraph_THIS(iface,fgraph);

	/* undoc. */
	FIXME( "(%p)->(%p,%08lx,%p) stub!\n",This,pPin,dwParam1,pdwParam2);
	return E_NOTIMPL;
}




static ICOM_VTABLE(IFilterGraph2) ifgraph =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IFilterGraph2_fnQueryInterface,
	IFilterGraph2_fnAddRef,
	IFilterGraph2_fnRelease,
	/* IFilterGraph fields */
	IFilterGraph2_fnAddFilter,
	IFilterGraph2_fnRemoveFilter,
	IFilterGraph2_fnEnumFilters,
	IFilterGraph2_fnFindFilterByName,
	IFilterGraph2_fnConnectDirect,
	IFilterGraph2_fnReconnect,
	IFilterGraph2_fnDisconnect,
	IFilterGraph2_fnSetDefaultSyncSource,
	/* IGraphBuilder fields */
	IFilterGraph2_fnConnect,
	IFilterGraph2_fnRender,
	IFilterGraph2_fnRenderFile,
	IFilterGraph2_fnAddSourceFilter,
	IFilterGraph2_fnSetLogFile,
	IFilterGraph2_fnAbort,
	IFilterGraph2_fnShouldOperationContinue,
	/* IFilterGraph2 fields */
	IFilterGraph2_fnAddSourceFilterForMoniker,
	IFilterGraph2_fnReconnectEx,
	IFilterGraph2_fnRenderEx,
};

HRESULT CFilterGraph_InitIFilterGraph2( CFilterGraph* pfg )
{
	TRACE("(%p)\n",pfg);
	ICOM_VTBL(&pfg->fgraph) = &ifgraph;

	pfg->m_pFilterList = QUARTZ_CompList_Alloc();
	if ( pfg->m_pFilterList == NULL )
		return E_OUTOFMEMORY;

	return NOERROR;
}

void CFilterGraph_UninitIFilterGraph2( CFilterGraph* pfg )
{
	QUARTZ_CompListItem*	pItem;

	TRACE("(%p)\n",pfg);

	/* remove all filters... */
	while ( 1 )
	{
		pItem = QUARTZ_CompList_GetFirst( pfg->m_pFilterList );
		if ( pItem == NULL )
			break;
		IFilterGraph2_fnRemoveFilter(
			(IFilterGraph2*)(&pfg->fgraph),
			(IBaseFilter*)QUARTZ_CompList_GetItemPtr(pItem) );
	}

	QUARTZ_CompList_Free( pfg->m_pFilterList );
}
