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
	int dummy;
} CMPEGVideoDecoderImpl;


/***************************************************************************
 *
 *	CMPEGVideoDecoderImpl methods
 *
 */

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

	return S_OK;
}

static HRESULT CMPEGVideoDecoderImpl_Cleanup( CTransformBaseImpl* pImpl )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return S_OK;

	/* destruct */

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return S_OK;
}

static HRESULT CMPEGVideoDecoderImpl_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_ProcessReceive( CTransformBaseImpl* pImpl, IMediaSample* pSampIn )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT CMPEGVideoDecoderImpl_EndTransform( CTransformBaseImpl* pImpl )
{
	CMPEGVideoDecoderImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
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



