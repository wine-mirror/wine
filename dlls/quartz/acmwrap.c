/*
 * Implements ACM Wrapper(CLSID_ACMWrapper).
 *
 *	FIXME - stub
 *
 * Copyright (C) 2002 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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
#include "msacm.h"
#include "strmif.h"
#include "control.h"
#include "amvideo.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"


static const WCHAR ACMWrapper_FilterName[] =
{'A','C','M',' ','W','r','a','p','p','e','r',0};


typedef struct CACMWrapperImpl
{
	HACMSTREAM	has;
} CACMWrapperImpl;

/***************************************************************************
 *
 *	CACMWrapperImpl methods
 *
 */

static void ACMWrapper_Close( CACMWrapperImpl* This )
{
	if ( This->has != (HACMSTREAM)NULL )
	{
		acmStreamReset( This->has, 0 );
		acmStreamClose( This->has, 0 );
		This->has = (HACMSTREAM)NULL;
	}
}

static HRESULT ACMWrapper_Init( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This != NULL )
		return NOERROR;

	This = (CACMWrapperImpl*)QUARTZ_AllocMem( sizeof(CACMWrapperImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CACMWrapperImpl) );
	pImpl->m_pUserData = This;

	/* construct */
	This->has = (HACMSTREAM)NULL;

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_Cleanup( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return NOERROR;

	/* destruct */
	ACMWrapper_Close( This );

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return S_OK;
}

static HRESULT ACMWrapper_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	FIXME("(%p)\n",This);
	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);
	hr = ACMWrapper_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = ACMWrapper_CheckMediaType( pImpl, pmtIn, pmtOut );
	if ( FAILED(hr) )
		return hr;

	*pbTransInPlace = FALSE;
	*pbTryToReuseSample = FALSE;

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	FIXME("(%p,%p,%p,%d)\n",This,pmtIn,pmtOut,bReuseSample);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_ProcessReceive( CTransformBaseImpl* pImpl, IMediaSample* pSampIn )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;
	BYTE*	pDataIn = NULL;
	LONG	lDataInLen;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	lDataInLen = IMediaSample_GetActualDataLength( pSampIn );

	return E_NOTIMPL;
}

static HRESULT ACMWrapper_EndTransform( CTransformBaseImpl* pImpl )
{
	CACMWrapperImpl*	This = pImpl->m_pUserData;

	TRACE("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	ACMWrapper_Close( This );

	return S_OK;
}

static const TransformBaseHandlers transhandlers =
{
	ACMWrapper_Init,
	ACMWrapper_Cleanup,
	ACMWrapper_CheckMediaType,
	ACMWrapper_GetOutputTypes,
	ACMWrapper_GetAllocProp,
	ACMWrapper_BeginTransform,
	ACMWrapper_ProcessReceive,
	NULL,
	ACMWrapper_EndTransform,
};

HRESULT QUARTZ_CreateACMWrapper(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_ACMWrapper,
		ACMWrapper_FilterName,
		NULL, NULL,
		&transhandlers );
}


