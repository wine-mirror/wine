/*
 * Implements CLSID_FileWriter.
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
 *	FIXME - not tested
 */

#include "config.h"

#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
#include "uuids.h"
#include "evcode.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "filesink.h"
#include "seekpass.h"

static const WCHAR QUARTZ_FileWriter_Name[] =
{ 'F','i','l','e',' ','W','r','i','t','e','r',0 };
static const WCHAR QUARTZ_FileWriterPin_Name[] =
{ 'I','n',0 };


/* FIXME - add this flag to strmif.h */
#define AM_FILE_OVERWRITE	0x1

/***************************************************************************
 *
 *	CFileWriterImpl methods
 *
 */

static HRESULT CFileWriterImpl_OnActive( CBaseFilterImpl* pImpl )
{
	CFileWriterImpl_THIS(pImpl,basefilter);

	FIXME( "(%p)\n", This );

	return NOERROR;
}

static HRESULT CFileWriterImpl_OnInactive( CBaseFilterImpl* pImpl )
{
	CFileWriterImpl_THIS(pImpl,basefilter);

	FIXME( "(%p)\n", This );

	return NOERROR;
}

static const CBaseFilterHandlers filterhandlers =
{
	CFileWriterImpl_OnActive, /* pOnActive */
	CFileWriterImpl_OnInactive, /* pOnInactive */
	NULL, /* pOnStop */
};


/***************************************************************************
 *
 *	CFileWriterPinImpl methods
 *
 */

static HRESULT CFileWriterPinImpl_OnPreConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pPin);

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_OnPostConnect( CPinBaseImpl* pImpl, IPin* pPin )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pPin);

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_OnDisconnect( CPinBaseImpl* pImpl )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	TRACE("(%p)\n",This);

	if ( This->meminput.pAllocator != NULL )
	{
		IMemAllocator_Decommit(This->meminput.pAllocator);
		IMemAllocator_Release(This->meminput.pAllocator);
		This->meminput.pAllocator = NULL;
	}

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_CheckMediaType( CPinBaseImpl* pImpl, const AM_MEDIA_TYPE* pmt )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	TRACE("(%p,%p)\n",This,pmt);

	if ( !IsEqualGUID( &pmt->majortype, &MEDIATYPE_Stream ) )
		return E_FAIL;

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_Receive( CPinBaseImpl* pImpl, IMediaSample* pSample )
{
	CFileWriterPinImpl_THIS(pImpl,pin);
	BYTE*	pData = NULL;
	LONG	lLength;
	ULONG	cbWritten;
	HRESULT hr;
	REFERENCE_TIME	rtStart;
	REFERENCE_TIME	rtEnd;
	LARGE_INTEGER	dlibMove;

	TRACE( "(%p,%p)\n",This,pSample );

	if ( This->pRender->m_fInFlush )
		return S_FALSE;
	if ( pSample == NULL )
		return E_POINTER;

	hr = IMediaSample_GetPointer(pSample,&pData);
	if ( FAILED(hr) )
		return hr;
	lLength = (LONG)IMediaSample_GetActualDataLength(pSample);
	if ( lLength == 0 )
		return S_OK;

	if ( lLength < 0 )
	{
		ERR( "invalid length: %ld\n", lLength );
		return S_OK;
	}

	hr = IMediaSample_GetTime( pSample, &rtStart, &rtEnd );
	if ( FAILED(hr) )
		return hr;

	dlibMove.QuadPart = rtStart;
	hr = IStream_Seek(CFileWriterPinImpl_IStream(This),dlibMove,STREAM_SEEK_SET,NULL);
	if ( FAILED(hr) )
		return hr;

	hr = IStream_Write(CFileWriterPinImpl_IStream(This),pData,lLength,&cbWritten);

	return hr;
}

static HRESULT CFileWriterPinImpl_ReceiveCanBlock( CPinBaseImpl* pImpl )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	TRACE( "(%p)\n", This );

	return S_FALSE;
}

static HRESULT CFileWriterPinImpl_EndOfStream( CPinBaseImpl* pImpl )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	/* FIXME - don't notify twice until stopped or seeked. */
	return CBaseFilterImpl_MediaEventNotify(
		&This->pRender->basefilter, EC_COMPLETE,
		(LONG_PTR)S_OK, (LONG_PTR)(IBaseFilter*)(This->pRender) );
}

static HRESULT CFileWriterPinImpl_BeginFlush( CPinBaseImpl* pImpl )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = TRUE;

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_EndFlush( CPinBaseImpl* pImpl )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	return NOERROR;
}

static HRESULT CFileWriterPinImpl_NewSegment( CPinBaseImpl* pImpl, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double rate )
{
	CFileWriterPinImpl_THIS(pImpl,pin);

	FIXME( "(%p)\n", This );

	This->pRender->m_fInFlush = FALSE;

	return NOERROR;
}




static const CBasePinHandlers pinhandlers =
{
	CFileWriterPinImpl_OnPreConnect, /* pOnPreConnect */
	CFileWriterPinImpl_OnPostConnect, /* pOnPostConnect */
	CFileWriterPinImpl_OnDisconnect, /* pOnDisconnect */
	CFileWriterPinImpl_CheckMediaType, /* pCheckMediaType */
	NULL, /* pQualityNotify */
	CFileWriterPinImpl_Receive, /* pReceive */
	CFileWriterPinImpl_ReceiveCanBlock, /* pReceiveCanBlock */
	CFileWriterPinImpl_EndOfStream, /* pEndOfStream */
	CFileWriterPinImpl_BeginFlush, /* pBeginFlush */
	CFileWriterPinImpl_EndFlush, /* pEndFlush */
	CFileWriterPinImpl_NewSegment, /* pNewSegment */
};


/***************************************************************************
 *
 *	new/delete CFileWriterImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry FilterIFEntries[] =
{
  { &IID_IPersist, offsetof(CFileWriterImpl,basefilter)-offsetof(CFileWriterImpl,unk) },
  { &IID_IMediaFilter, offsetof(CFileWriterImpl,basefilter)-offsetof(CFileWriterImpl,unk) },
  { &IID_IBaseFilter, offsetof(CFileWriterImpl,basefilter)-offsetof(CFileWriterImpl,unk) },
  { &IID_IFileSinkFilter, offsetof(CFileWriterImpl,filesink)-offsetof(CFileWriterImpl,unk) },
  { &IID_IFileSinkFilter2, offsetof(CFileWriterImpl,filesink)-offsetof(CFileWriterImpl,unk) },
};

static HRESULT CFileWriterImpl_OnQueryInterface(
	IUnknown* punk, const IID* piid, void** ppobj )
{
	CFileWriterImpl_THIS(punk,unk);

	if ( This->pSeekPass == NULL )
		return E_NOINTERFACE;

	if ( IsEqualGUID( &IID_IMediaPosition, piid ) ||
		 IsEqualGUID( &IID_IMediaSeeking, piid ) )
	{
		TRACE( "IMediaSeeking(or IMediaPosition) is queried\n" );
		return IUnknown_QueryInterface( (IUnknown*)(&This->pSeekPass->unk), piid, ppobj );
	}

	return E_NOINTERFACE;
}

static void QUARTZ_DestroyFileWriter(IUnknown* punk)
{
	CFileWriterImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );
	CFileWriterImpl_OnInactive(&This->basefilter);

	if ( This->pPin != NULL )
	{
		IUnknown_Release(This->pPin->unk.punkControl);
		This->pPin = NULL;
	}
	if ( This->pSeekPass != NULL )
	{
		IUnknown_Release((IUnknown*)&This->pSeekPass->unk);
		This->pSeekPass = NULL;
	}

	if ( This->m_hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( This->m_hFile );
		This->m_hFile = INVALID_HANDLE_VALUE;
	}
	if ( This->m_pszFileName != NULL )
	{
		QUARTZ_FreeMem( This->m_pszFileName );
		This->m_pszFileName = NULL;
	}
	QUARTZ_MediaType_Free( &This->m_mt );

	CFileWriterImpl_UninitIFileSinkFilter2(This);
	CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);

	DeleteCriticalSection( &This->m_csReceive );
}

HRESULT QUARTZ_CreateFileWriter(IUnknown* punkOuter,void** ppobj)
{
	CFileWriterImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p)\n",punkOuter,ppobj);

	This = (CFileWriterImpl*)
		QUARTZ_AllocObj( sizeof(CFileWriterImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	This->pSeekPass = NULL;
	This->pPin = NULL;
	This->m_fInFlush = FALSE;

	This->m_hFile = INVALID_HANDLE_VALUE;
	This->m_pszFileName = NULL;
	This->m_cbFileName = 0;
	This->m_dwMode = 0;
	ZeroMemory( &This->m_mt, sizeof(AM_MEDIA_TYPE) );

	QUARTZ_IUnkInit( &This->unk, punkOuter );
	This->qiext.pNext = NULL;
	This->qiext.pOnQueryInterface = &CFileWriterImpl_OnQueryInterface;
	QUARTZ_IUnkAddDelegation( &This->unk, &This->qiext );

	hr = CBaseFilterImpl_InitIBaseFilter(
		&This->basefilter,
		This->unk.punkControl,
		&CLSID_FileWriter,
		QUARTZ_FileWriter_Name,
		&filterhandlers );
	if ( SUCCEEDED(hr) )
	{
		hr = CFileWriterImpl_InitIFileSinkFilter2(This);
		if ( FAILED(hr) )
		{
			CBaseFilterImpl_UninitIBaseFilter(&This->basefilter);
		}
	}

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(This);
		return hr;
	}

	This->unk.pEntries = FilterIFEntries;
	This->unk.dwEntries = sizeof(FilterIFEntries)/sizeof(FilterIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyFileWriter;

	InitializeCriticalSection( &This->m_csReceive );

	hr = QUARTZ_CreateFileWriterPin(
		This,
		&This->basefilter.csFilter,
		&This->m_csReceive,
		&This->pPin );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CompList_AddComp(
			This->basefilter.pInPins,
			(IUnknown*)&This->pPin->pin,
			NULL, 0 );
	if ( SUCCEEDED(hr) )
		hr = QUARTZ_CreateSeekingPassThruInternal(
			(IUnknown*)&(This->unk), &This->pSeekPass,
			TRUE, (IPin*)&(This->pPin->pin) );

	if ( FAILED(hr) )
	{
		IUnknown_Release( This->unk.punkControl );
		return hr;
	}

	*ppobj = (void*)&(This->unk);

	return S_OK;
}

/***************************************************************************
 *
 *	new/delete CFileWriterPinImpl
 *
 */

/* can I use offsetof safely? - FIXME? */
static QUARTZ_IFEntry PinIFEntries[] =
{
  { &IID_IPin, offsetof(CFileWriterPinImpl,pin)-offsetof(CFileWriterPinImpl,unk) },
  { &IID_IMemInputPin, offsetof(CFileWriterPinImpl,meminput)-offsetof(CFileWriterPinImpl,unk) },
  { &IID_IStream, offsetof(CFileWriterPinImpl,stream)-offsetof(CFileWriterPinImpl,unk) },
};

static void QUARTZ_DestroyFileWriterPin(IUnknown* punk)
{
	CFileWriterPinImpl_THIS(punk,unk);

	TRACE( "(%p)\n", This );

	CPinBaseImpl_UninitIPin( &This->pin );
	CMemInputPinBaseImpl_UninitIMemInputPin( &This->meminput );
	CFileWriterPinImpl_UninitIStream(This);
}

HRESULT QUARTZ_CreateFileWriterPin(
        CFileWriterImpl* pFilter,
        CRITICAL_SECTION* pcsPin,
        CRITICAL_SECTION* pcsPinReceive,
        CFileWriterPinImpl** ppPin)
{
	CFileWriterPinImpl*	This = NULL;
	HRESULT hr;

	TRACE("(%p,%p,%p,%p)\n",pFilter,pcsPin,pcsPinReceive,ppPin);

	This = (CFileWriterPinImpl*)
		QUARTZ_AllocObj( sizeof(CFileWriterPinImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;

	QUARTZ_IUnkInit( &This->unk, NULL );
	This->pRender = pFilter;

	hr = CPinBaseImpl_InitIPin(
		&This->pin,
		This->unk.punkControl,
		pcsPin, pcsPinReceive,
		&pFilter->basefilter,
		QUARTZ_FileWriterPin_Name,
		FALSE,
		&pinhandlers );

	if ( SUCCEEDED(hr) )
	{
		hr = CMemInputPinBaseImpl_InitIMemInputPin(
			&This->meminput,
			This->unk.punkControl,
			&This->pin );
		if ( SUCCEEDED(hr) )
		{
			hr = CFileWriterPinImpl_InitIStream(This);
			if ( FAILED(hr) )
			{
				CMemInputPinBaseImpl_UninitIMemInputPin(&This->meminput);
			}
		}

		if ( FAILED(hr) )
		{
			CPinBaseImpl_UninitIPin( &This->pin );
		}
	}

	if ( FAILED(hr) )
	{
		QUARTZ_FreeObj(This);
		return hr;
	}

	This->unk.pEntries = PinIFEntries;
	This->unk.dwEntries = sizeof(PinIFEntries)/sizeof(PinIFEntries[0]);
	This->unk.pOnFinalRelease = QUARTZ_DestroyFileWriterPin;

	*ppPin = This;

	TRACE("returned successfully.\n");

	return S_OK;
}

/***************************************************************************
 *
 *	CFileWriterPinImpl::IStream
 *
 */

static HRESULT WINAPI
IStream_fnQueryInterface(IStream* iface,REFIID riid,void** ppobj)
{
	CFileWriterPinImpl_THIS(iface,stream);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IStream_fnAddRef(IStream* iface)
{
	CFileWriterPinImpl_THIS(iface,stream);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IStream_fnRelease(IStream* iface)
{
	CFileWriterPinImpl_THIS(iface,stream);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IStream_fnRead(IStream* iface,void* pv,ULONG cb,ULONG* pcbRead)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->()\n",This);

	return E_FAIL;
}

static HRESULT WINAPI
IStream_fnWrite(IStream* iface,const void* pv,ULONG cb,ULONG* pcbWritten)
{
	CFileWriterPinImpl_THIS(iface,stream);
	HRESULT hr;

	FIXME("(%p)->(%p,%lu,%p)\n",This,pv,cb,pcbWritten);

	EnterCriticalSection( &This->pRender->m_csReceive );
	if ( This->pRender->m_hFile == INVALID_HANDLE_VALUE )
	{
		hr = E_UNEXPECTED;
		goto err;
	}

	if ( ! WriteFile( This->pRender->m_hFile, pv, cb, pcbWritten, NULL ) )
	{
		hr = E_FAIL;
		goto err;
	}

	hr = S_OK;
err:
	LeaveCriticalSection( &This->pRender->m_csReceive );
	return hr;
}

static HRESULT WINAPI
IStream_fnSeek(IStream* iface,LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER* plibNewPosition)
{
	CFileWriterPinImpl_THIS(iface,stream);
	HRESULT hr;
	DWORD	dwDistLow;
	LONG	lDistHigh;

	FIXME("(%p)->() stub!\n",This);

	EnterCriticalSection( &This->pRender->m_csReceive );
	if ( This->pRender->m_hFile == INVALID_HANDLE_VALUE )
	{
		hr = E_UNEXPECTED;
		goto err;
	}

	dwDistLow = dlibMove.s.LowPart;
	lDistHigh = dlibMove.s.HighPart;

	SetLastError(0);
	dwDistLow = SetFilePointer( This->pRender->m_hFile, (LONG)dwDistLow, &lDistHigh, dwOrigin );
	if ( dwDistLow == 0xffffffff && GetLastError() != 0 )
	{
		hr = E_FAIL;
		goto err;
	}

	if ( plibNewPosition != NULL )
	{
		plibNewPosition->s.LowPart = dwDistLow;
		plibNewPosition->s.HighPart = lDistHigh;
	}

	hr = S_OK;
err:
	LeaveCriticalSection( &This->pRender->m_csReceive );
	return hr;
}

static HRESULT WINAPI
IStream_fnSetSize(IStream* iface,ULARGE_INTEGER libNewSize)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	if ( This->pRender->m_hFile == INVALID_HANDLE_VALUE )
		return E_UNEXPECTED;


	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnCopyTo(IStream* iface,IStream* pstrm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->()\n",This);

	return E_FAIL;
}

static HRESULT WINAPI
IStream_fnCommit(IStream* iface,DWORD grfCommitFlags)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnRevert(IStream* iface)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnLockRegion(IStream* iface,ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnUnlockRegion(IStream* iface,ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnStat(IStream* iface,STATSTG* pstatstg,DWORD grfStatFlag)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI
IStream_fnClone(IStream* iface,IStream** ppstrm)
{
	CFileWriterPinImpl_THIS(iface,stream);

	FIXME("(%p)->() stub!\n",This);

	return E_NOTIMPL;
}

static ICOM_VTABLE(IStream) istream =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IStream_fnQueryInterface,
	IStream_fnAddRef,
	IStream_fnRelease,
	/* IStream fields */
	IStream_fnRead,
	IStream_fnWrite,
	IStream_fnSeek,
	IStream_fnSetSize,
	IStream_fnCopyTo,
	IStream_fnCommit,
	IStream_fnRevert,
	IStream_fnLockRegion,
	IStream_fnUnlockRegion,
	IStream_fnStat,
	IStream_fnClone,
};

HRESULT CFileWriterPinImpl_InitIStream( CFileWriterPinImpl* This )
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->stream) = &istream;

	return NOERROR;
}

HRESULT CFileWriterPinImpl_UninitIStream( CFileWriterPinImpl* This )
{
	TRACE("(%p)\n",This);

	return S_OK;
}


/***************************************************************************
 *
 *	CFileWriterImpl::IFileSinkFilter2
 *
 */

static HRESULT WINAPI
IFileSinkFilter2_fnQueryInterface(IFileSinkFilter2* iface,REFIID riid,void** ppobj)
{
	CFileWriterImpl_THIS(iface,filesink);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->unk.punkControl,riid,ppobj);
}

static ULONG WINAPI
IFileSinkFilter2_fnAddRef(IFileSinkFilter2* iface)
{
	CFileWriterImpl_THIS(iface,filesink);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->unk.punkControl);
}

static ULONG WINAPI
IFileSinkFilter2_fnRelease(IFileSinkFilter2* iface)
{
	CFileWriterImpl_THIS(iface,filesink);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->unk.punkControl);
}

static HRESULT WINAPI
IFileSinkFilter2_fnSetFileName(IFileSinkFilter2* iface,LPCOLESTR pszFileName,const AM_MEDIA_TYPE* pmt)
{
	CFileWriterImpl_THIS(iface,filesink);
	HRESULT hr;

	TRACE("(%p)->(%s,%p)\n",This,debugstr_w(pszFileName),pmt);

	if ( pszFileName == NULL )
		return E_POINTER;

	if ( This->m_pszFileName != NULL )
		return E_UNEXPECTED;

	This->m_cbFileName = sizeof(WCHAR)*(lstrlenW(pszFileName)+1);
	This->m_pszFileName = (WCHAR*)QUARTZ_AllocMem( This->m_cbFileName );
	if ( This->m_pszFileName == NULL )
		return E_OUTOFMEMORY;
	memcpy( This->m_pszFileName, pszFileName, This->m_cbFileName );

	if ( pmt != NULL )
	{
		hr = QUARTZ_MediaType_Copy( &This->m_mt, pmt );
		if ( FAILED(hr) )
			goto err;
	}
	else
	{
		ZeroMemory( &This->m_mt, sizeof(AM_MEDIA_TYPE) );
		memcpy( &This->m_mt.majortype, &MEDIATYPE_Stream, sizeof(GUID) );
		memcpy( &This->m_mt.subtype, &MEDIASUBTYPE_NULL, sizeof(GUID) );
		This->m_mt.lSampleSize = 1;
		memcpy( &This->m_mt.formattype, &FORMAT_None, sizeof(GUID) );
	}

	This->m_hFile = CreateFileW(
		This->m_pszFileName,
		GENERIC_WRITE,
		0,
		NULL,
		( This->m_dwMode == AM_FILE_OVERWRITE ) ? CREATE_ALWAYS : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL );
	if ( This->m_hFile == INVALID_HANDLE_VALUE )
	{
		hr = E_FAIL;
		goto err;
	}

	This->pPin->pin.pmtAcceptTypes = &This->m_mt;
	This->pPin->pin.cAcceptTypes = 1;

	return NOERROR;
err:;
	return hr;
}

static HRESULT WINAPI
IFileSinkFilter2_fnGetCurFile(IFileSinkFilter2* iface,LPOLESTR* ppszFileName,AM_MEDIA_TYPE* pmt)
{
	CFileWriterImpl_THIS(iface,filesink);
	HRESULT hr = E_NOTIMPL;

	TRACE("(%p)->(%p,%p)\n",This,ppszFileName,pmt);

	if ( ppszFileName == NULL || pmt == NULL )
		return E_POINTER;

	if ( This->m_pszFileName == NULL )
		return E_FAIL;

	hr = QUARTZ_MediaType_Copy( pmt, &This->m_mt );
	if ( FAILED(hr) )
		return hr;

	*ppszFileName = (WCHAR*)CoTaskMemAlloc( This->m_cbFileName );
	if ( *ppszFileName == NULL )
	{
		QUARTZ_MediaType_Free(pmt);
		ZeroMemory( pmt, sizeof(AM_MEDIA_TYPE) );
		return E_OUTOFMEMORY;
	}

	memcpy( *ppszFileName, This->m_pszFileName, This->m_cbFileName );

	return NOERROR;
}

static HRESULT WINAPI
IFileSinkFilter2_fnSetMode(IFileSinkFilter2* iface,DWORD dwFlags)
{
	CFileWriterImpl_THIS(iface,filesink);

	TRACE("(%p)->(%08lx)\n",This,dwFlags);

	if ( dwFlags != 0 && dwFlags != AM_FILE_OVERWRITE )
		return E_INVALIDARG;
	This->m_dwMode = dwFlags;

	return S_OK;
}

static HRESULT WINAPI
IFileSinkFilter2_fnGetMode(IFileSinkFilter2* iface,DWORD* pdwFlags)
{
	CFileWriterImpl_THIS(iface,filesink);

	TRACE("(%p)->(%p)\n",This,pdwFlags);

	if ( pdwFlags == NULL )
		return E_POINTER;

	*pdwFlags = This->m_dwMode;

	return S_OK;
}

static ICOM_VTABLE(IFileSinkFilter2) ifilesink2 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	/* IUnknown fields */
	IFileSinkFilter2_fnQueryInterface,
	IFileSinkFilter2_fnAddRef,
	IFileSinkFilter2_fnRelease,
	/* IFileSinkFilter2 fields */
	IFileSinkFilter2_fnSetFileName,
	IFileSinkFilter2_fnGetCurFile,
	IFileSinkFilter2_fnSetMode,
	IFileSinkFilter2_fnGetMode,
};

HRESULT CFileWriterImpl_InitIFileSinkFilter2( CFileWriterImpl* This )
{
	TRACE("(%p)\n",This);
	ICOM_VTBL(&This->filesink) = &ifilesink2;

	return NOERROR;
}

HRESULT CFileWriterImpl_UninitIFileSinkFilter2( CFileWriterImpl* This )
{
	TRACE("(%p)\n",This);

	return S_OK;
}


