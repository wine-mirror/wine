/*
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 *
 * FIXME - implements color space(depth) converter.
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winerror.h"
#include "vfw.h"
#include "debugtools.h"
#include "avifile_private.h"

DEFAULT_DEBUG_CHANNEL(avifile);

static HRESULT WINAPI IGetFrame_fnQueryInterface(IGetFrame* iface,REFIID refiid,LPVOID *obj);
static ULONG WINAPI IGetFrame_fnAddRef(IGetFrame* iface);
static ULONG WINAPI IGetFrame_fnRelease(IGetFrame* iface);
static LPVOID WINAPI IGetFrame_fnGetFrame(IGetFrame* iface,LONG lPos);
static HRESULT WINAPI IGetFrame_fnBegin(IGetFrame* iface,LONG lStart,LONG lEnd,LONG lRate);
static HRESULT WINAPI IGetFrame_fnEnd(IGetFrame* iface);
static HRESULT WINAPI IGetFrame_fnSetFormat(IGetFrame* iface,LPBITMAPINFOHEADER lpbi,LPVOID lpBits,INT x,INT y,INT dx,INT dy);

struct ICOM_VTABLE(IGetFrame) igetfrm = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IGetFrame_fnQueryInterface,
	IGetFrame_fnAddRef,
	IGetFrame_fnRelease,
	IGetFrame_fnGetFrame,
	IGetFrame_fnBegin,
	IGetFrame_fnEnd,
	IGetFrame_fnSetFormat,
};

typedef struct IGetFrameImpl
{
	ICOM_VFIELD(IGetFrame);
	/* IUnknown stuff */
	DWORD			ref;
	/* IGetFrame stuff */
	IAVIStream*		pas;
	HIC			hIC;
	LONG			lCachedFrame;
	BITMAPINFO*		pbiICIn;
	BITMAPINFO*		pbiICOut;
	LPVOID			pvICOutBits;
	LPVOID			pvICInFmtBuf;
	DWORD			dwICInDataBufSize;
	LPVOID			pvICInDataBuf;
	LPVOID			pvICOutBuf;
} IGetFrameImpl;

static HRESULT IGetFrame_Construct( IGetFrameImpl* This,
				    IAVIStream* pstr,
				    LPBITMAPINFOHEADER lpbi );
static void IGetFrame_Destruct( IGetFrameImpl* This );




static LPVOID AVIFILE_IGetFrame_DecodeFrame(IGetFrameImpl* This,LONG lPos)
{
	HRESULT	hr;
	DWORD	dwRes;
	LONG	lFrameLength;
	LONG	lSampleCount;
	ICDECOMPRESS	icd;

	if ( This->hIC == (HIC)NULL )
		return NULL;

	hr = IAVIStream_Read(This->pas,lPos,1,NULL,0,
			     &lFrameLength,&lSampleCount);
	if ( hr != S_OK || lSampleCount <= 0 )
	{
		FIXME( "IAVIStream_Read failed! res = %08lx\n", hr );
		return NULL;
	}
	TRACE( "frame length = %ld\n", lFrameLength );

	if ( This->dwICInDataBufSize < lFrameLength )
	{
		LPVOID	lpv;

		if ( This->pvICInDataBuf == NULL )
		{
			lpv = HeapAlloc(
				AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
				lFrameLength );
		}
		else
		{
			lpv = HeapReAlloc(
				AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
				This->pvICInDataBuf,lFrameLength );
		}
		if ( lpv == NULL )
		{
			ERR( "out of memory!\n" );
			return NULL;
		}
		This->pvICInDataBuf = lpv;
		This->dwICInDataBufSize = lFrameLength;
	}

	hr = IAVIStream_Read(This->pas,lPos,1,
			     This->pvICInDataBuf,This->dwICInDataBufSize,
			     &lFrameLength,&lSampleCount);
	if ( hr != S_OK || lSampleCount <= 0 )
	{
		FIXME( "IAVIStream_Read to buffer failed! res = %08lx\n", hr );
		return NULL;
	}

	This->pbiICIn->bmiHeader.biSizeImage = lFrameLength;

	TRACE( "call ICM_DECOMPRESS\n" );
	icd.dwFlags = (*(BYTE*)This->pvICInDataBuf) == 'c' ?
				      ICDECOMPRESS_NOTKEYFRAME : 0;
	icd.lpbiInput = &This->pbiICIn->bmiHeader;
	icd.lpInput = (BYTE*)This->pvICInDataBuf + 8;
	icd.lpbiOutput = &This->pbiICOut->bmiHeader;
	icd.lpOutput = This->pvICOutBits;
	icd.ckid = *((DWORD*)This->pvICInDataBuf);
	dwRes = ICSendMessage(This->hIC,ICM_DECOMPRESS,
			      (DWORD)(&icd),sizeof(ICDECOMPRESS) );
	TRACE( "returned from ICM_DECOMPRESS\n" );
	if ( dwRes != ICERR_OK )
	{
		ERR( "ICDecompress failed!\n" );
		return NULL;
	}

	This->lCachedFrame = lPos;

	return This->pvICOutBits;
}

/****************************************************************************/

HRESULT AVIFILE_CreateIGetFrame(void** ppobj,
				IAVIStream* pstr,LPBITMAPINFOHEADER lpbi)
{
	IGetFrameImpl	*This;
	HRESULT		hr;

	*ppobj = NULL;
	This = (IGetFrameImpl*)HeapAlloc(AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
					  sizeof(IGetFrameImpl));
	This->ref = 1;
	ICOM_VTBL(This) = &igetfrm;
	hr = IGetFrame_Construct( This, pstr, lpbi );
	if ( hr != S_OK )
	{
		IGetFrame_Destruct( This );
		return hr;
	}

	*ppobj = (LPVOID)This;

	return S_OK;
}

/****************************************************************************
 * IUnknown interface
 */

static HRESULT WINAPI IGetFrame_fnQueryInterface(IGetFrame* iface,REFIID refiid,LPVOID *obj)
{
	ICOM_THIS(IGetFrameImpl,iface);

	TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if ( IsEqualGUID(&IID_IUnknown,refiid) ||
	     IsEqualGUID(&IID_IGetFrame,refiid) )
	{
		IGetFrame_AddRef(iface);
		*obj = iface;
		return S_OK;
	}

	return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IGetFrame_fnAddRef(IGetFrame* iface)
{
	ICOM_THIS(IGetFrameImpl,iface);

	TRACE("(%p)->AddRef()\n",iface);
	return ++(This->ref);
}

static ULONG WINAPI IGetFrame_fnRelease(IGetFrame* iface)
{
	ICOM_THIS(IGetFrameImpl,iface);

	TRACE("(%p)->Release()\n",iface);
	if ((--(This->ref)) > 0 )
		return This->ref;
	IGetFrame_Destruct(This);
	if ( This->pas != NULL )
		IAVIStream_Release( This->pas );

	HeapFree(AVIFILE_data.hHeap,0,iface);
	return 0;
}

/****************************************************************************
 * IGetFrrame interface
 */

static LPVOID WINAPI IGetFrame_fnGetFrame(IGetFrame* iface,LONG lPos)
{
	ICOM_THIS(IGetFrameImpl,iface);
	LPVOID	lpv;
	LONG	lKeyFrame;

	TRACE( "(%p)->(%ld)\n", This, lPos );

	if ( lPos < 0 )
		return NULL;

	if ( This->lCachedFrame == lPos )
		return This->pvICOutBits;
	if ( (This->lCachedFrame+1) != lPos )
	{
		lKeyFrame = IAVIStream_FindSample( This->pas, lPos,
						   FIND_KEY | FIND_PREV );
		if ( lKeyFrame < 0 || lKeyFrame > lPos )
			return NULL;
		while ( ++lKeyFrame < lPos )
		{
			lpv = AVIFILE_IGetFrame_DecodeFrame(This, lKeyFrame);
			if ( lpv == NULL )
				return NULL;
		}
	}

	lpv = AVIFILE_IGetFrame_DecodeFrame(This, lPos);
	TRACE( "lpv = %p\n",lpv );
	if ( lpv == NULL )
		return NULL;

	return lpv;
}

static HRESULT WINAPI IGetFrame_fnBegin(IGetFrame* iface,LONG lStart,LONG lEnd,LONG lRate)
{
	ICOM_THIS(IGetFrameImpl,iface);

	TRACE( "(%p)->(%ld,%ld,%ld)\n", This, lStart, lEnd, lRate );

	if ( This->hIC == (HIC)NULL )
		return E_UNEXPECTED;

	if ( ICDecompressBegin( This->hIC,
				This->pbiICIn,
				This->pbiICOut ) != ICERR_OK )
		return E_FAIL;

	return S_OK;
}

static HRESULT WINAPI IGetFrame_fnEnd(IGetFrame* iface)
{
	ICOM_THIS(IGetFrameImpl,iface);

	TRACE( "(%p)->()\n", This );

	if ( This->hIC == (HIC)NULL )
		return E_UNEXPECTED;

	if ( ICDecompressEnd( This->hIC ) != ICERR_OK )
		return E_FAIL;

	return S_OK;
}

static HRESULT WINAPI IGetFrame_fnSetFormat(IGetFrame* iface,LPBITMAPINFOHEADER lpbi,LPVOID lpBits,INT x,INT y,INT dx,INT dy)
{
	ICOM_THIS(IGetFrameImpl,iface);
	HRESULT	hr;
	LONG	fmtlen;
	BITMAPINFOHEADER	biTemp;
	DWORD	dwSizeImage;

	FIXME( "(%p)->(%p,%p,%d,%d,%d,%d)\n",This,lpbi,lpBits,x,y,dx,dy );

	IGetFrame_Destruct(This);

	hr = IAVIStream_ReadFormat(This->pas,0,NULL,&fmtlen);
	if ( hr != S_OK )
		return hr;
	This->pvICInFmtBuf = HeapAlloc(
		AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,fmtlen);
	if ( This->pvICInFmtBuf == NULL )
		return AVIERR_MEMORY;
	hr = IAVIStream_ReadFormat(This->pas,0,This->pvICInFmtBuf,&fmtlen);
	if ( hr != S_OK )
		return hr;
	This->pbiICIn = (LPBITMAPINFO)This->pvICInFmtBuf;

	This->hIC = (HIC)ICOpen( ICTYPE_VIDEO,
				 This->pbiICIn->bmiHeader.biCompression,
				 ICMODE_DECOMPRESS );
	if ( This->hIC == (HIC)NULL )
	{
		ERR( "no AVI decompressor for %c%c%c%c.\n",
		     (int)(This->pbiICIn->bmiHeader.biCompression>> 0)&0xff,
		     (int)(This->pbiICIn->bmiHeader.biCompression>> 8)&0xff,
		     (int)(This->pbiICIn->bmiHeader.biCompression>>16)&0xff,
		     (int)(This->pbiICIn->bmiHeader.biCompression>>24)&0xff );
		return E_FAIL;
	}

	if ( lpbi == NULL || lpbi == ((LPBITMAPINFOHEADER)1) )
	{
		memset( &biTemp, 0, sizeof(biTemp) );
		biTemp.biSize = sizeof(BITMAPINFOHEADER);
		biTemp.biWidth = This->pbiICIn->bmiHeader.biWidth;
		biTemp.biHeight = This->pbiICIn->bmiHeader.biHeight;
		biTemp.biPlanes = 1;
		biTemp.biBitCount = 24;
		biTemp.biCompression = 0;
		lpbi = &biTemp;
	}

	if ( lpbi->biPlanes != 1 || lpbi->biCompression != 0 )
		return E_FAIL;

	dwSizeImage =
		((This->pbiICIn->bmiHeader.biWidth*lpbi->biBitCount+7)/8)*
					This->pbiICIn->bmiHeader.biHeight;
	This->pvICOutBuf = HeapAlloc(
		AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
		(sizeof(BITMAPINFO)+sizeof(RGBQUAD)*256)*2+
		dwSizeImage );
	if ( This->pvICOutBuf == NULL )
		return AVIERR_MEMORY;

	This->pbiICOut = (BITMAPINFO*)This->pvICOutBuf;
	This->pvICOutBits = (LPVOID)( (BYTE*)This->pvICOutBuf +
				sizeof(BITMAPINFO) + sizeof(RGBQUAD)*256 );

	This->pbiICOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	This->pbiICOut->bmiHeader.biWidth = This->pbiICIn->bmiHeader.biWidth;
	This->pbiICOut->bmiHeader.biHeight = This->pbiICIn->bmiHeader.biHeight;
	This->pbiICOut->bmiHeader.biPlanes = 1;
	This->pbiICOut->bmiHeader.biBitCount = lpbi->biBitCount;
	This->pbiICOut->bmiHeader.biSizeImage = dwSizeImage;
	memcpy( This->pvICOutBits, This->pbiICOut, sizeof(BITMAPINFOHEADER) );

	return S_OK;
}

static HRESULT IGetFrame_Construct( IGetFrameImpl* This,
				    IAVIStream* pstr,
				    LPBITMAPINFOHEADER lpbi )
{
	HRESULT	hr;

	TRACE( "(%p)->(%p,%p)\n",This,pstr,lpbi );

	IAVIStream_AddRef( pstr );
	This->pas = pstr;
	This->hIC = (HIC)NULL;
	This->lCachedFrame = -1L;
	This->pbiICIn = NULL;
	This->pbiICOut = NULL;
	This->pvICInFmtBuf = NULL;
	This->pvICInDataBuf = NULL;
	This->dwICInDataBufSize = 0;
	This->pvICOutBuf = NULL;

	hr = IGetFrame_SetFormat((IGetFrame*)This,lpbi,NULL,0,0,0,0);
	if ( hr != S_OK )
		return hr;

	return S_OK;
}

static void IGetFrame_Destruct( IGetFrameImpl* This )
{
	if ( This->hIC != (HIC)NULL )
	{
		ICClose( This->hIC );
		This->hIC = (HIC)NULL;
	}
	if ( This->pvICInFmtBuf != NULL )
	{
		HeapFree( AVIFILE_data.hHeap, 0, This->pvICInFmtBuf );
		This->pvICInFmtBuf = NULL;
	}
	if ( This->pvICInDataBuf != NULL )
	{
		HeapFree( AVIFILE_data.hHeap, 0, This->pvICInDataBuf );
		This->pvICInDataBuf = NULL;
	}
	if ( This->pvICOutBuf != NULL )
	{
		HeapFree( AVIFILE_data.hHeap, 0, This->pvICOutBuf );
		This->pvICOutBuf = NULL;
	}

	This->lCachedFrame = -1L;
	This->pbiICIn = NULL;
	This->pbiICOut = NULL;
	This->dwICInDataBufSize = 0;
}
