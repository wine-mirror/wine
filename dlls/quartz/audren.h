/*
 * Audio Renderer (CLSID_AudioRender)
 *
 * FIXME
 *  - implements IRefereneceClock.
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

#ifndef	WINE_DSHOW_AUDREN_H
#define	WINE_DSHOW_AUDREN_H

#include "iunk.h"
#include "basefilt.h"
#include "seekpass.h"

#define WINE_QUARTZ_WAVEOUT_COUNT	4

typedef struct CAudioRendererImpl	CAudioRendererImpl;
typedef struct CAudioRendererPinImpl	CAudioRendererPinImpl;


typedef struct AudRen_IBasicAudioImpl
{
	ICOM_VFIELD(IBasicAudio);
} AudRen_IBasicAudioImpl;

struct CAudioRendererImpl
{
	QUARTZ_IUnkImpl	unk;
	CBaseFilterImpl	basefilter;
	AudRen_IBasicAudioImpl	basaud;
	QUARTZ_IFDelegation	qiext;

	CSeekingPassThru*	pSeekPass;
	CAudioRendererPinImpl* pPin;

	CRITICAL_SECTION	m_csReceive;
	BOOL	m_fInFlush;

	/* for waveOut */
	long		m_lAudioVolume;
	long		m_lAudioBalance;
	BOOL		m_fWaveOutInit;
	HANDLE		m_hEventRender;
	HWAVEOUT	m_hWaveOut;
	DWORD		m_dwBlockSize;
	WAVEHDR*	m_phdrCur;
	WAVEHDR		m_hdr[WINE_QUARTZ_WAVEOUT_COUNT];
};

struct CAudioRendererPinImpl
{
	QUARTZ_IUnkImpl	unk;
	CPinBaseImpl	pin;
	CMemInputPinBaseImpl	meminput;

	CAudioRendererImpl* pRender;
};

#define	CAudioRendererImpl_THIS(iface,member)	CAudioRendererImpl*	This = ((CAudioRendererImpl*)(((char*)iface)-offsetof(CAudioRendererImpl,member)))
#define	CAudioRendererPinImpl_THIS(iface,member)	CAudioRendererPinImpl*	This = ((CAudioRendererPinImpl*)(((char*)iface)-offsetof(CAudioRendererPinImpl,member)))


HRESULT CAudioRendererImpl_InitIBasicAudio( CAudioRendererImpl* This );
void CAudioRendererImpl_UninitIBasicAudio( CAudioRendererImpl* This );

HRESULT QUARTZ_CreateAudioRenderer(IUnknown* punkOuter,void** ppobj);
HRESULT QUARTZ_CreateAudioRendererPin(
        CAudioRendererImpl* pFilter,
        CRITICAL_SECTION* pcsPin,
	CRITICAL_SECTION* pcsPinReceive,
        CAudioRendererPinImpl** ppPin);



#endif	/* WINE_DSHOW_AUDREN_H */
