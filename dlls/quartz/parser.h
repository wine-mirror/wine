/*
 * Implements Parser.
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

#ifndef	WINE_DSHOW_PARSER_H
#define	WINE_DSHOW_PARSER_H

#include "iunk.h"
#include "basefilt.h"

typedef struct CParserImpl CParserImpl;
typedef struct CParserInPinImpl CParserInPinImpl;
typedef struct CParserOutPinImpl CParserOutPinImpl;
typedef struct ParserHandlers ParserHandlers;

/* {D51BD5A1-7548-11CF-A520-0080C77EF58A} */
DEFINE_GUID(CLSID_quartzWaveParser,
0xD51BD5A1,0x7548,0x11CF,0xA5,0x20,0x00,0x80,0xC7,0x7E,0xF5,0x8A);

struct CParserImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;

	CParserInPinImpl* m_pInPin;
	ULONG	m_cOutStreams;
	CParserOutPinImpl**	m_ppOutPins;
	GUID	m_guidTimeFormat;

	CRITICAL_SECTION	m_csParser;
	IAsyncReader*	m_pReader;
	IMemAllocator*	m_pAllocator;
	ALLOCATOR_PROPERTIES	m_propAlloc;
	HANDLE	m_hEventInit;
	DWORD	m_dwThreadId;
	HANDLE	m_hThread;
	BOOL	m_bSendEOS;

	const ParserHandlers*	m_pHandler;
	void*	m_pUserData;
};

struct CParserInPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CMemInputPinBaseImpl	meminput;

	CParserImpl*	pParser;
};

struct CParserOutPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CQualityControlPassThruImpl	qcontrol;
	struct { ICOM_VFIELD(IMediaSeeking); } mediaseeking;
	struct { ICOM_VFIELD(IMediaPosition); } mediaposition;

	CParserImpl*	pParser;
	ULONG	nStreamIndex;

	AM_MEDIA_TYPE	m_mtOut;
	IMemAllocator*	m_pOutPinAllocator;
	void*	m_pUserData;

	/* for parser */
	BOOL	m_bReqUsed;
	IMediaSample*	m_pReqSample;
	LONGLONG	m_llReqStart;
	LONG	m_lReqLength;
	REFERENCE_TIME	m_rtReqStart;
	REFERENCE_TIME	m_rtReqStop;
	DWORD	m_dwSampleFlags;
};



struct ParserHandlers
{
	HRESULT (*pInitParser)( CParserImpl* pImpl, ULONG* pcStreams );
	HRESULT (*pUninitParser)( CParserImpl* pImpl );
	LPCWSTR (*pGetOutPinName)( CParserImpl* pImpl, ULONG nStreamIndex );
	HRESULT (*pGetStreamType)( CParserImpl* pImpl, ULONG nStreamIndex, AM_MEDIA_TYPE* pmt );
	HRESULT (*pCheckStreamType)( CParserImpl* pImpl, ULONG nStreamIndex, const AM_MEDIA_TYPE* pmt );
	HRESULT (*pGetAllocProp)( CParserImpl* pImpl, ALLOCATOR_PROPERTIES* pReqProp );
	/* S_OK - ok, S_FALSE - end of stream */
	HRESULT (*pGetNextRequest)( CParserImpl* pImpl, ULONG* pnStreamIndex, LONGLONG* pllStart, LONG* plLength, REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop, DWORD* pdwSampleFlags );
	HRESULT (*pProcessSample)( CParserImpl* pImpl, ULONG nStreamIndex, LONGLONG llStart, LONG lLength, IMediaSample* pSample );

	/* for IQualityControl */
	HRESULT (*pQualityNotify)( CParserImpl* pImpl, ULONG nStreamIndex, Quality q );

	/* for seeking */
	HRESULT (*pGetSeekingCaps)( CParserImpl* pImpl, DWORD* pdwCaps );
	HRESULT (*pIsTimeFormatSupported)( CParserImpl* pImpl, const GUID* pTimeFormat );
	HRESULT (*pGetCurPos)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG* pllPos );
	HRESULT (*pSetCurPos)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG llPos );
	HRESULT (*pGetDuration)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG* pllDuration );
	HRESULT (*pGetStopPos)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG* pllPos );
	HRESULT (*pSetStopPos)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG llPos );
	HRESULT (*pGetPreroll)( CParserImpl* pImpl, const GUID* pTimeFormat, DWORD nStreamIndex, LONGLONG* pllPreroll );
};

#define	CParserImpl_THIS(iface,member)	CParserImpl*	This = ((CParserImpl*)(((char*)iface)-offsetof(CParserImpl,member)))
#define	CParserInPinImpl_THIS(iface,member)	CParserInPinImpl*	This = ((CParserInPinImpl*)(((char*)iface)-offsetof(CParserInPinImpl,member)))
#define	CParserOutPinImpl_THIS(iface,member)	CParserOutPinImpl*	This = ((CParserOutPinImpl*)(((char*)iface)-offsetof(CParserOutPinImpl,member)))


#define CParserOutPinImpl_IMediaSeeking(th)	((IMediaSeeking*)&((th)->mediaseeking))
#define CParserOutPinImpl_IMediaPosition(th)	((IMediaPosition*)&((th)->mediaposition))

HRESULT QUARTZ_CreateParser(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidParser,
	LPCWSTR pwszParserName,
	LPCWSTR pwszInPinName,
	const ParserHandlers* pHandler );
HRESULT QUARTZ_CreateParserInPin(
	CParserImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CParserInPinImpl** ppPin,
	LPCWSTR pwszPinName );
HRESULT QUARTZ_CreateParserOutPin(
	CParserImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CParserOutPinImpl** ppPin,
	ULONG nStreamIndex,
	LPCWSTR pwszPinName );


#define PARSER_POLL_INTERVAL	100

#define PARSER_RIFF_OfsFirst 12
#define PARSER_WAVE mmioFOURCC('W','A','V','E')
#define PARSER_AVI  mmioFOURCC('A','V','I',' ')
#define PARSER_AVIX mmioFOURCC('A','V','I','X')

#define PARSER_fmt  mmioFOURCC('f','m','t',' ')
#define PARSER_fact mmioFOURCC('f','a','c','t')
#define PARSER_data mmioFOURCC('d','a','t','a')

#define PARSER_LIST mmioFOURCC('L','I','S','T')

#define PARSER_hdrl mmioFOURCC('h','d','r','l')
#define PARSER_avih mmioFOURCC('a','v','i','h')
#define PARSER_strl mmioFOURCC('s','t','r','l')
#define PARSER_strh mmioFOURCC('s','t','r','h')
#define PARSER_strf mmioFOURCC('s','t','r','f')
#define PARSER_idx1 mmioFOURCC('i','d','x','1')
#define PARSER_indx mmioFOURCC('i','n','d','x')
#define PARSER_movi mmioFOURCC('m','o','v','i')
#define PARSER_JUNK mmioFOURCC('J','U','N','K')

#define PARSER_vids mmioFOURCC('v','i','d','s')
#define PARSER_auds mmioFOURCC('a','u','d','s')
#define PARSER_mids mmioFOURCC('m','i','d','s')
#define PARSER_txts mmioFOURCC('t','x','t','s')

#define PARSER_LE_UINT16(ptr)	(((DWORD)(ptr)[0])|((DWORD)(ptr)[1]<<8))
#define PARSER_LE_UINT32(ptr)	(((DWORD)(ptr)[0])|((DWORD)(ptr)[1]<<8)|((DWORD)(ptr)[2]<<16)|((DWORD)(ptr)[3]<<24))
#define PARSER_BE_UINT16(ptr)	(((DWORD)(ptr)[0]<<8)|((DWORD)(ptr)[1]))
#define PARSER_BE_UINT32(ptr)	(((DWORD)(ptr)[0]<<24)|((DWORD)(ptr)[1]<<16)|((DWORD)(ptr)[2]<<8)|((DWORD)(ptr)[3]))

HRESULT QUARTZ_CreateWaveParser(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateAVISplitter(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateMPEG1Splitter(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateMPEG2Splitter(IUnknown* punkOuter,void** ppobj);


HRESULT RIFF_GetNext(
	CParserImpl* pImpl, LONGLONG llOfs,
	DWORD* pdwCode, DWORD* pdwLength );
HRESULT RIFF_SearchChunk(
	CParserImpl* pImpl,
	DWORD dwSearchLengthMax,
	LONGLONG llOfs, DWORD dwChunk,
	LONGLONG* pllOfs, DWORD* pdwChunkLength );
HRESULT RIFF_SearchList(
	CParserImpl* pImpl,
	DWORD dwSearchLengthMax,
	LONGLONG llOfs, DWORD dwListChunk,
	LONGLONG* pllOfs, DWORD* pdwChunkLength );


#endif	/* WINE_DSHOW_PARSER_H */
