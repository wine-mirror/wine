/*
 * Audio Renderer (CLSID_AudioRender)
 *
 * FIXME
 *  - implements IRefereneceClock.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	WINE_DSHOW_AUDREN_H
#define	WINE_DSHOW_AUDREN_H

#include "iunk.h"
#include "basefilt.h"

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

	CAudioRendererPinImpl* pPin;

	BOOL	m_fInFlush;

	/* for waveOut */
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
        CAudioRendererPinImpl** ppPin);



#endif	/* WINE_DSHOW_AUDREN_H */
