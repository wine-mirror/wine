/*
 * Implements CLSID_VideoRenderer.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	WINE_DSHOW_VIDREN_H
#define	WINE_DSHOW_VIDREN_H

#include "iunk.h"
#include "basefilt.h"

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

	CVideoRendererPinImpl*	pPin;

	BOOL	m_fInFlush;

	/* for rendering */
	HANDLE	m_hEventInit;
	HANDLE	m_hThread;
	HWND	m_hwnd;
	CRITICAL_SECTION	m_csSample;
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
        CVideoRendererPinImpl** ppPin);


#endif	/* WINE_DSHOW_VIDREN_H */
