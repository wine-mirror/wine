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
 */

#ifndef	WINE_DSHOW_FILESINK_H
#define	WINE_DSHOW_FILESINK_H

#include "iunk.h"
#include "basefilt.h"
#include "seekpass.h"

typedef struct CFileWriterImpl	CFileWriterImpl;
typedef struct CFileWriterPinImpl	CFileWriterPinImpl;


typedef struct FileWriterPin_IStreamImpl
{
	ICOM_VFIELD(IStream);
} FileWriterPin_IStreamImpl;

typedef struct FileWriter_IFileSinkFilter2Impl
{
	ICOM_VFIELD(IFileSinkFilter2);
} FileWriter_IFileSinkFilter2Impl;

struct CFileWriterImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;
	FileWriter_IFileSinkFilter2Impl	filesink;
	QUARTZ_IFDelegation	qiext;

	CSeekingPassThru*	pSeekPass;
	CFileWriterPinImpl* pPin;

	CRITICAL_SECTION	m_csReceive;
	BOOL	m_fInFlush;

	/* for writing */
	HANDLE	m_hFile;
	WCHAR*	m_pszFileName;
	DWORD	m_cbFileName;
	DWORD	m_dwMode;
	AM_MEDIA_TYPE	m_mt;
};

struct CFileWriterPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CMemInputPinBaseImpl	meminput;
	FileWriterPin_IStreamImpl	stream;

	CFileWriterImpl* pRender;
};

#define	CFileWriterImpl_THIS(iface,member)	CFileWriterImpl*	This = ((CFileWriterImpl*)(((char*)iface)-offsetof(CFileWriterImpl,member)))
#define	CFileWriterPinImpl_THIS(iface,member)	CFileWriterPinImpl*	This = ((CFileWriterPinImpl*)(((char*)iface)-offsetof(CFileWriterPinImpl,member)))


HRESULT CFileWriterPinImpl_InitIStream( CFileWriterPinImpl* This );
HRESULT CFileWriterPinImpl_UninitIStream( CFileWriterPinImpl* This );
HRESULT CFileWriterImpl_InitIFileSinkFilter2( CFileWriterImpl* This );
HRESULT CFileWriterImpl_UninitIFileSinkFilter2( CFileWriterImpl* This );

HRESULT QUARTZ_CreateFileWriter(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateFileWriterPin(
        CFileWriterImpl* pFilter,
        CRITICAL_SECTION* pcsPin,
        CRITICAL_SECTION* pcsPinReceive,
        CFileWriterPinImpl** ppPin);


#endif	/* WINE_DSHOW_FILESINK_H */
