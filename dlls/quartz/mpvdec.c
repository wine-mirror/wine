/*
 * Implements MPEG Video Decoder(CLSID_CMpegVideoCodec)
 *
 *	FIXME - what library can we use? SMPEG??
 *
 *	FIXME - stub
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
#include "winerror.h"
#include "strmif.h"
#include "control.h"
#include "amvideo.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"
#include "mtype.h"

static const WCHAR CMPEGVideoDecoderImpl_FilterName[] =
{'M','P','E','G',' ','V','i','d','e','o',' ','D','e','c','o','d','e','r',0};


typedef struct CMPEGVideoDecoderImpl
{
	AM_MEDIA_TYPE*	pmt;
	DWORD		cmt;

} CMPEGVideoDecoderImpl;


/*****************************************************************************
 *
 *	codec-dependent stuffs	- no codec
 *
 */

#define	NO_CODEC_IMPL

static void Codec_OnConstruct(CMPEGVideoDecoderImpl* This)
{
}

static void Codec_OnCleanup(CMPEGVideoDecoderImpl* This)
{
}

static HRESULT Codec_BeginTransform(CTransformBaseImpl* pImpl,CMPEGVideoDecoderImpl* This)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}

static HRESULT Codec_ProcessReceive(CTransformBaseImpl* pImpl,CMPEGVideoDecoderImpl* This,IMediaSample* pSampIn)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}

static HRESULT Codec_EndTransform(CTransformBaseImpl* pImpl,CMPEGVideoDecoderImpl* This)
{
	FIXME("no codec\n");
	return E_NOTIMPL;
}



/***************************************************************************
 *
 *	CMPEGVideoDecoderImpl methods
 *
 */

static void CMPEGVideoDecoderImpl_CleanupOutTypes(CMPEGVideoDecoderImpl* This)
{
	DWORD	i;

	if ( This->pmt != NULL )
	{
		for ( i = 0; i < This->cmt; i++ )
		{
			QUARTZ_MediaType_Free(&This->pmt[i]);
		}
		QUARTZ_FreeMem(This->pmt);
		This->pmt = NULL;
	}
	This->cmt = 0;
}

static HRESULT CMPEGVideoDecoderImpl_Init( CTransformBaseImpl* pImpl )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This != NULL )
		return NOERROR;

	This = (CMPEGVideoDecoderImpl*)QUARTZ_AllocMem( sizeof(CMPEGVideoDecoderImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CMPEGVideoDecoderImpl) );
	pImpl->m_pUserData = This;

	/* construct */
	This->pmt = NULL;
	This->cmt = 0;
	Codec_OnConstruct(This);

	return S_OK;
}

static HRESULT CMPEGVideoDecoderImpl_Cleanup( CTransformBaseImpl* pImpl )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return S_OK;

	/* destruct */
	Codec_OnCleanup(This);
	CMPEGVideoDecoderImpl_CleanupOutTypes(This);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return S_OK;
}

static HRESULT CMPEGVideoDecoderImpl_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;



#ifdef	NO_CODEC_IMPL
	WARN("no codec implementation\n");
	return E_NOTIMPL;
#else
	return S_OK;
#endif
}

static HRESULT CMPEGVideoDecoderImpl_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGVideoDecoderImpl_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	CMPEGVideoDecoderImpl_CleanupOutTypes(This);

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGVideoDecoderImpl_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = CMPEGVideoDecoderImpl_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	return Codec_BeginTransform(pImpl,This);
}

static HRESULT CMPEGVideoDecoderImpl_ProcessReceive( CTransformBaseImpl* pImpl, IMediaSample* pSampIn )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return Codec_ProcessReceive(pImpl,This,pSampIn);
}

static HRESULT CMPEGVideoDecoderImpl_EndTransform( CTransformBaseImpl* pImpl )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	hr = Codec_EndTransform(pImpl,This);
	if ( FAILED(hr) )
		return hr;

	return S_OK;
}

static const TransformBaseHandlers transhandlers =
{
	CMPEGVideoDecoderImpl_Init,
	CMPEGVideoDecoderImpl_Cleanup,
	CMPEGVideoDecoderImpl_CheckMediaType,
	CMPEGVideoDecoderImpl_GetOutputTypes,
	CMPEGVideoDecoderImpl_GetAllocProp,
	CMPEGVideoDecoderImpl_BeginTransform,
	CMPEGVideoDecoderImpl_ProcessReceive,
	NULL,
	CMPEGVideoDecoderImpl_EndTransform,
};

HRESULT QUARTZ_CreateCMpegVideoCodec(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_CMpegVideoCodec,
		CMPEGVideoDecoderImpl_FilterName,
		NULL, NULL,
		&transhandlers );
}



