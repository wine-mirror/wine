/*
 * Implements MIDI Parser.
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



static const WCHAR QUARTZ_MIDIParser_Name[] =
{ 'Q','u','i','c','k','T','i','m','e',' ','M','o','v','i','e',' ','P','a','r','s','e','r',0 };
static const WCHAR QUARTZ_MIDIParserInPin_Name[] =
{ 'I','n',0 };
static const WCHAR QUARTZ_MIDIParserOutPin_Name[] =
{ 'O','u','t',0 };


/****************************************************************************
 *
 *	CMIDIParseImpl
 */


typedef struct CMIDIParseImpl CMIDIParseImpl;

struct CMIDIParseImpl
{
};


static HRESULT CMIDIParseImpl_InitParser( CParserImpl* pImpl, ULONG* pcStreams )
{
	WARN("(%p,%p) stub\n",pImpl,pcStreams);

	return E_NOTIMPL;
}

static HRESULT CMIDIParseImpl_UninitParser( CParserImpl* pImpl )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static LPCWSTR CMIDIParseImpl_GetOutPinName( CParserImpl* pImpl, ULONG nStreamIndex )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	TRACE("(%p,%lu)\n",This,nStreamIndex);

	return QUARTZ_MIDIParserOutPin_Name;
}

static HRESULT CMIDIParseImpl_GetStreamType( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT CMIDIParseImpl_CheckStreamType( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	return E_NOTIMPL;
}

static HRESULT CMIDIParseImpl_GetAllocProp( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	FIXME("(%p,%p) stub\n",This,pReqProp);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMIDIParseImpl_GetNextRequest( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop, DWORD* pdwSampleFlags )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	FIXME("(%p) stub\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMIDIParseImpl_ProcessSample( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample )
{
	CMIDIParseImpl*	This = (CMIDIParseImpl*)pImpl->m_pUserData;

	FIXME("(%p,%lu,%ld,%ld,%p)\n",This,nStreamIndex,(long)llStart,lLength,pSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static const struct ParserHandlers CMIDIParseImpl_Handlers =
{
	CMIDIParseImpl_InitParser,
	CMIDIParseImpl_UninitParser,
	CMIDIParseImpl_GetOutPinName,
	CMIDIParseImpl_GetStreamType,
	CMIDIParseImpl_CheckStreamType,
	CMIDIParseImpl_GetAllocProp,
	CMIDIParseImpl_GetNextRequest,
	CMIDIParseImpl_ProcessSample,

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

HRESULT QUARTZ_CreateMIDIParser(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateParser(
		punkOuter,ppobj,
		&CLSID_quartzMIDIParser,
		QUARTZ_MIDIParser_Name,
		QUARTZ_MIDIParserInPin_Name,
		&CMIDIParseImpl_Handlers );
}


