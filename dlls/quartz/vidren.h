/*
 * Implements CLSID_VideoRenderer.
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

#ifndef	WINE_DSHOW_VIDREN_H
#define	WINE_DSHOW_VIDREN_H

#include "iunk.h"
#include "basefilt.h"
#include "seekpass.h"

typedef struct CVideoRendererImpl CVideoRendererImpl;
typedef struct CVideoRendererPinImpl CVideoRendererPinImpl;


typedef struct VidRen_IBasicVideo
{
	ICOM_VFIELD(IBasicVideo2);
} VidRen_IBasicVideo;

typedef struct VidRen_IVideoWindow
{
	ICOM_VFIELD(IVideoWindow);
} VidRen_IVideoWindow;

struct CVideoRendererImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;
	VidRen_IBasicVideo	basvid;
	VidRen_IVideoWindow	vidwin;
	QUARTZ_IFDelegation	qiext;

	CSeekingPassThru*	pSeekPass;
	CVideoRendererPinImpl*	pPin;

	BOOL	m_fInFlush;

	/* for rendering */
	HANDLE	m_hEventInit;
	HANDLE	m_hThread;
	HWND	m_hwnd;
	CRITICAL_SECTION	m_csReceive;
	BOOL	m_bSampleIsValid;
	BYTE*	m_pSampleData;
	DWORD	m_cbSampleData;
};

struct CVideoRendererPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CMemInputPinBaseImpl	meminput;

	CVideoRendererImpl* pRender;
};



#define	CVideoRendererImpl_THIS(iface,member)	CVideoRendererImpl*	This = ((CVideoRendererImpl*)(((char*)iface)-offsetof(CVideoRendererImpl,member)))
#define	CVideoRendererPinImpl_THIS(iface,member)	CVideoRendererPinImpl*	This = ((CVideoRendererPinImpl*)(((char*)iface)-offsetof(CVideoRendererPinImpl,member)))

HRESULT CVideoRendererImpl_InitIBasicVideo2( CVideoRendererImpl* This );
void CVideoRendererImpl_UninitIBasicVideo2( CVideoRendererImpl* This );
HRESULT CVideoRendererImpl_InitIVideoWindow( CVideoRendererImpl* This );
void CVideoRendererImpl_UninitIVideoWindow( CVideoRendererImpl* This );

HRESULT QUARTZ_CreateVideoRenderer(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateVideoRendererPin(
        CVideoRendererImpl* pFilter,
        CRITICAL_SECTION* pcsPin,
	CRITICAL_SECTION* pcsPinReceive,
        CVideoRendererPinImpl** ppPin);


#endif	/* WINE_DSHOW_VIDREN_H */
