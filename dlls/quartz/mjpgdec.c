/*
 * Implements AVI MJPG Decompressor.
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
#include "vfw.h"
#include "strmif.h"
#include "control.h"
#include "amvideo.h"
#include "vfwmsgs.h"
#include "uuids.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "quartz_private.h"
#include "xform.h"

static const WCHAR MJPGDec_FilterName[] =
{'M','J','P','G',' ','D','e','c','o','m','p','r','e','s','s','o','r',0};

typedef struct CMJPGDecImpl
{
	AM_MEDIA_TYPE*	m_pmtConv;
	DWORD	m_cConv;
} CMJPGDecImpl;

/***************************************************************************
 *
 *	CMJPGDecImpl methods
 *
 */

static void MJPGDec_FreeOutTypes(CMJPGDecImpl* This)
{
	DWORD	i;

	if ( This->m_pmtConv == NULL )
		return;

	TRACE("cConv = %lu\n",This->m_cConv);
	for ( i = 0; i < This->m_cConv; i++ )
	{
		QUARTZ_MediaType_Free(&This->m_pmtConv[i]);
	}
	QUARTZ_FreeMem(This->m_pmtConv);
	This->m_pmtConv = NULL;
	This->m_cConv = 0;
}


static HRESULT MJPGDec_Init( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	if ( This != NULL )
		return NOERROR;

	This = (CMJPGDecImpl*)QUARTZ_AllocMem( sizeof(CMJPGDecImpl) );
	if ( This == NULL )
		return E_OUTOFMEMORY;
	ZeroMemory( This, sizeof(CMJPGDecImpl) );
	pImpl->m_pUserData = This;
	/* construct */
	This->m_pmtConv = NULL;
	This->m_cConv = 0;

	return NOERROR;
}

static HRESULT MJPGDec_Cleanup( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	if ( This == NULL )
		return NOERROR;
	/* destruct */
	MJPGDec_FreeOutTypes(This);

	QUARTZ_FreeMem( This );
	pImpl->m_pUserData = NULL;

	return NOERROR;
}

static HRESULT MJPGDec_CheckMediaType( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	WARN("(%p) stub\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}

static HRESULT MJPGDec_GetOutputTypes( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE** ppmtAcceptTypes, ULONG* pcAcceptTypes )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	MJPGDec_FreeOutTypes(This);

	*ppmtAcceptTypes = This->m_pmtConv;
	*pcAcceptTypes = This->m_cConv;

	return E_NOTIMPL;
}

static HRESULT MJPGDec_GetAllocProp( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, ALLOCATOR_PROPERTIES* pProp, BOOL* pbTransInPlace, BOOL* pbTryToReuseSample )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	return E_NOTIMPL;
}

static HRESULT MJPGDec_BeginTransform( CTransformBaseImpl* pImpl, const AM_MEDIA_TYPE* pmtIn, const AM_MEDIA_TYPE* pmtOut, BOOL bReuseSample )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = MJPGDec_CheckMediaType( pImpl, pmtIn, NULL );
	if ( FAILED(hr) )
		return hr;

	return E_NOTIMPL;
}

static HRESULT MJPGDec_Transform( CTransformBaseImpl* pImpl, IMediaSample* pSampIn, IMediaSample* pSampOut )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;
	BYTE*	pDataIn = NULL;
	BYTE*	pDataOut = NULL;
	HRESULT hr;

	FIXME("(%p)\n",This);

	if ( This == NULL )
		return E_UNEXPECTED;

	hr = IMediaSample_GetPointer( pSampIn, &pDataIn );
	if ( FAILED(hr) )
		return hr;
	hr = IMediaSample_GetPointer( pSampOut, &pDataOut );
	if ( FAILED(hr) )
		return hr;

	return E_NOTIMPL;
}

static HRESULT MJPGDec_EndTransform( CTransformBaseImpl* pImpl )
{
	CMJPGDecImpl*	This = pImpl->m_pUserData;

	if ( This == NULL )
		return E_UNEXPECTED;

	return E_NOTIMPL;
}


static const TransformBaseHandlers transhandlers =
{
	MJPGDec_Init,
	MJPGDec_Cleanup,
	MJPGDec_CheckMediaType,
	MJPGDec_GetOutputTypes,
	MJPGDec_GetAllocProp,
	MJPGDec_BeginTransform,
	NULL,
	MJPGDec_Transform,
	MJPGDec_EndTransform,
};

HRESULT QUARTZ_CreateMJPGDecompressor(IUnknown* punkOuter,void** ppobj)
{
	return QUARTZ_CreateTransformBase(
		punkOuter,ppobj,
		&CLSID_quartzMJPGDecompressor,
		MJPGDec_FilterName,
		NULL, NULL,
		&transhandlers );
}


