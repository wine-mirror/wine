/*
 * Implements QuickTime Parser(Splitter).
 *
 *	FIXME - stub
 *	FIXME - no seeking
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
#include "mmsystem.h"
#include "vfw.h"
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "uuids.h"


#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "parser.h"
#include "mtype.h"



static const WCHAR QUARTZ_QTParser_Name[] =
{ 'Q','u','i','c','k','T','i','m','e',' ','M','o','v','i','e',' ','P','a','r','s','e','r',0 };
static const WCHAR QUARTZ_QTParserInPin_Name[] =
{ 'I','n',0 };
static const WCHAR QUARTZ_QTParserOutPin_Basename[] =
{ 'S','t','r','e','a','m',0 };

#define WINE_QUARTZ_QTPINNAME_MAX	64

/****************************************************************************
 *
 *	CQTParseImpl
 */


typedef struct CQTParseImpl CQTParseImpl;
typedef struct CQTParseStream CQTParseStream;

struct CQTParseImpl
{
	CQTParseStream*	pStreamsBuf;
	DWORD	cIndexEntries;
	WCHAR	wchWork[ WINE_QUARTZ_QTPINNAME_MAX ];
};

struct CQTParseStream
{
	int	dummy;
};


static HRESULT CQTParseImpl_InitParser( CParserImpl* pImpl, ULONG* pcStreams )
{
	WARN("(%p,%p) stub\n",pImpl,pcStreams);

	return E_NOTIMPL;
}

static HRESULT CQTParseImpl_UninitParser( CParserImpl* pImpl )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static LPCWSTR CQTParseImpl_GetOutPinName( CParserImpl* pImpl, ULONG nStreamIndex )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;
	int wlen;

	TRACE("(%p,%lu)\n",This,nStreamIndex);

	if ( This == NULL /*|| nStreamIndex >= This->avih.dwStreams*/ )
		return NULL;

	wlen = lstrlenW(QUARTZ_QTParserOutPin_Basename);
	memcpy( This->wchWork, QUARTZ_QTParserOutPin_Basename, sizeof(WCHAR)*wlen );
	This->wchWork[ wlen ] = (nStreamIndex/10) + '0';
	This->wchWork[ wlen+1 ] = (nStreamIndex%10) + '0';
	This->wchWork[ wlen+2 ] = 0;

	return This->wchWork;
}

static HRESULT CQTParseImpl_GetStreamType( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT CQTParseImpl_CheckStreamType( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT CQTParseImpl_GetAllocProp( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	FIXME("(%p,%p) stub\n",This,pReqProp);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CQTParseImpl_GetNextRequest( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop, DWORD* pdwSampleFlags )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CQTParseImpl_ProcessSample( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample )
{
	CQTParseImpl*	This = (CQTParseImpl*)pImpl->m_pUserData;

	FIXME("(%p,%lu,%ld,%ld,%p)\n",This,nStreamIndex,(long)llStart,lLength,pSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static const struct ParserHandlers CQTParseImpl_Handlers =
{
	CQTParseImpl_InitParser,
	CQTParseImpl_UninitParser,
	CQTParseImpl_GetOutPinName,
	CQTParseImpl_GetStreamType,
	CQTParseImpl_CheckStreamType,
	CQTParseImpl_GetAllocProp,
	CQTParseImpl_GetNextRequest,
	CQTParseImpl_ProcessSample,

	/* for IQualityControl */
	NULL, /* pQualityNotify */

	/* for seeking */
	NULL, /* pGetSeekingCaps */
	NULL, /* pIsTimeFormatSupported */
	NULL, /* pGetCurPos */
	NULL, /* pSetCurPos */
	NULL, /* pGetDuration */
	NULL, /* pGetStopPos */
	NULL, /* pSetStopPos */
	NULL, /* pGetPreroll */
};

HRESULT QUARTZ_CreateQuickTimeMovieParser(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_quartzQuickTimeMovieParser,
		QUARTZ_QTParser_Name,
		QUARTZ_QTParserInPin_Name,
		&CQTParseImpl_Handlers );
}


