/*
 * Implements Asynchronous File/URL Source.
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

#ifndef	WINE_DSHOW_ASYNCSRC_H
#define	WINE_DSHOW_ASYNCSRC_H

#include "iunk.h"
#include "basefilt.h"

typedef struct CAsyncSourceImpl	CAsyncSourceImpl;
typedef struct CAsyncSourcePinImpl	CAsyncSourcePinImpl;
typedef struct AsyncSourceRequest	AsyncSourceRequest;
typedef struct AsyncSourceHandlers	AsyncSourceHandlers;

typedef struct CAsyncReaderImpl
{
	ICOM_VFIELD(IAsyncReader);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IAsyncReader fields */
	CAsyncSourceImpl*	pSource;

	CRITICAL_SECTION	m_csReader;
	BOOL	m_bInFlushing;
	CRITICAL_SECTION	m_csRequest;
	CRITICAL_SECTION	m_csReply;
	AsyncSourceRequest*	m_pReplyFirst;
	CRITICAL_SECTION	m_csFree;
	AsyncSourceRequest*	m_pFreeFirst;
} CAsyncReaderImpl;

typedef struct CFileSourceFilterImpl
{
	ICOM_VFIELD(IFileSourceFilter);

	/* IUnknown fields */
	IUnknown*	punkControl;
	/* IFileSourceFilter fields */
	CAsyncSourceImpl*	pSource;

	CRITICAL_SECTION*	pcsFileSource;
	WCHAR*	m_pwszFileName;
	DWORD	m_cbFileName;
	AM_MEDIA_TYPE	m_mt;
} CFileSourceFilterImpl;

struct CAsyncSourceImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;
	CFileSourceFilterImpl	filesrc;

	CRITICAL_SECTION	csFilter;
	CAsyncSourcePinImpl*	pPin;
	const AsyncSourceHandlers*	m_pHandler;
	void*	m_pUserData;
};

struct CAsyncSourcePinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CAsyncReaderImpl	async;
	QUARTZ_IFDelegation	qiext;

	BOOL	bAsyncReaderQueried;
	CAsyncSourceImpl*	pSource;
};

struct AsyncSourceRequest
{
	AsyncSourceRequest*	pNext;

	IMediaSample*	pSample; /* for async req. */
	DWORD_PTR	dwContext; /* for async req. */
	HRESULT	hr;
};

struct AsyncSourceHandlers
{
	/* all handlers MUST be implemented. */
	HRESULT (*pLoad)( CAsyncSourceImpl* pImpl, LPCWSTR lpwszSourceName );
	HRESULT (*pCleanup)( CAsyncSourceImpl* pImpl );
	HRESULT (*pGetLength)( CAsyncSourceImpl* pImpl, LONGLONG* pllTotal, LONGLONG* pllAvailable );
	/* S_OK = OK / S_FALSE = Canceled / other = error */
	/* hEventCancel may be NULL */
	HRESULT (*pRead)( CAsyncSourceImpl* pImpl, LONGLONG llOfsStart, LONG lLength, BYTE* pBuf, LONG* plReturned, HANDLE hEventCancel );
};

#define	CAsyncSourceImpl_THIS(iface,member)		CAsyncSourceImpl*	This = ((CAsyncSourceImpl*)(((char*)iface)-offsetof(CAsyncSourceImpl,member)))
#define	CAsyncSourcePinImpl_THIS(iface,member)		CAsyncSourcePinImpl*	This = ((CAsyncSourcePinImpl*)(((char*)iface)-offsetof(CAsyncSourcePinImpl,member)))


HRESULT CAsyncReaderImpl_InitIAsyncReader(
	CAsyncReaderImpl* This, IUnknown* punkControl,
	CAsyncSourceImpl* pSource );
void CAsyncReaderImpl_UninitIAsyncReader(
	CAsyncReaderImpl* This );
HRESULT CFileSourceFilterImpl_InitIFileSourceFilter(
	CFileSourceFilterImpl* This, IUnknown* punkControl,
	CAsyncSourceImpl* pSource,
	CRITICAL_SECTION* pcsFileSource );
void CFileSourceFilterImpl_UninitIFileSourceFilter(
	CFileSourceFilterImpl* This );


HRESULT QUARTZ_CreateAsyncSource(
	IUnknown* punkOuter,void** ppobj,
	const CLSID* pclsidAsyncSource,
	LPCWSTR pwszAsyncSourceName,
	LPCWSTR pwszOutPinName,
	const AsyncSourceHandlers* pHandler );
HRESULT QUARTZ_CreateAsyncSourcePin(
	CAsyncSourceImpl* pFilter,
	CRITICAL_SECTION* pcsPin,
	CAsyncSourcePinImpl** ppPin,
	LPCWSTR pwszPinName );


HRESULT QUARTZ_CreateAsyncReader(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateURLReader(IUnknown* punkOuter,void** ppobj);

#define ASYNCSRC_FILE_BLOCKSIZE	16384


#endif	/* WINE_DSHOW_ASYNCSRC_H */
