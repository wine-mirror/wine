/*
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream*iface,REFIID refiid,LPVOID *obj);
static ULONG WINAPI IAVIStream_fnAddRef(IAVIStream*iface);
static ULONG WINAPI IAVIStream_fnRelease(IAVIStream* iface);
static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream*iface,LPARAM lParam1,LPARAM lParam2);
static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream*iface,AVISTREAMINFOW *psi,LONG size);
static LONG WINAPI IAVIStream_fnFindSample(IAVIStream*iface,LONG pos,LONG flags);
static HRESULT WINAPI IAVIStream_fnReadFormat(IAVIStream*iface,LONG pos,LPVOID format,LONG *formatsize);
static HRESULT WINAPI IAVIStream_fnSetFormat(IAVIStream*iface,LONG pos,LPVOID format,LONG formatsize);
static HRESULT WINAPI IAVIStream_fnRead(IAVIStream*iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread);
static HRESULT WINAPI IAVIStream_fnWrite(IAVIStream*iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten);
static HRESULT WINAPI IAVIStream_fnDelete(IAVIStream*iface,LONG start,LONG samples);
static HRESULT WINAPI IAVIStream_fnReadData(IAVIStream*iface,DWORD fcc,LPVOID lp,LONG *lpread);
static HRESULT WINAPI IAVIStream_fnWriteData(IAVIStream*iface,DWORD fcc,LPVOID lp,LONG size);
static HRESULT WINAPI IAVIStream_fnSetInfo(IAVIStream*iface,AVISTREAMINFOW*info,LONG infolen);


struct ICOM_VTABLE(IAVIStream) iavist = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IAVIStream_fnQueryInterface,
    IAVIStream_fnAddRef,
    IAVIStream_fnRelease,
    IAVIStream_fnCreate,
    IAVIStream_fnInfo,
    IAVIStream_fnFindSample,
    IAVIStream_fnReadFormat,
    IAVIStream_fnSetFormat,
    IAVIStream_fnRead,
    IAVIStream_fnWrite,
    IAVIStream_fnDelete,
    IAVIStream_fnReadData,
    IAVIStream_fnWriteData,
    IAVIStream_fnSetInfo
};



typedef struct IAVIStreamImpl
{
	ICOM_VFIELD(IAVIStream);
	/* IUnknown stuff */
	DWORD		ref;
	/* IAVIStream stuff */
	IAVIFile*		paf;
	WINE_AVISTREAM_DATA*	pData;
} IAVIStreamImpl;

static HRESULT IAVIStream_Construct( IAVIStreamImpl* This );
static void IAVIStream_Destruct( IAVIStreamImpl* This );

HRESULT AVIFILE_CreateIAVIStream(void** ppobj)
{
	IAVIStreamImpl	*This;
	HRESULT		hr;

	*ppobj = NULL;
	This = (IAVIStreamImpl*)HeapAlloc(AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
					  sizeof(IAVIStreamImpl));
	This->ref = 1;
	ICOM_VTBL(This) = &iavist;
	hr = IAVIStream_Construct( This );
	if ( hr != S_OK )
	{
		IAVIStream_Destruct( This );
		return hr;
	}

	*ppobj = (LPVOID)This;

	return S_OK;
}


/****************************************************************************
 * IUnknown interface
 */

static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream*iface,REFIID refiid,LPVOID *obj) {
	ICOM_THIS(IAVIStreamImpl,iface);

	TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if ( IsEqualGUID(&IID_IUnknown,refiid) ||
	     IsEqualGUID(&IID_IAVIStream,refiid) )
	{
		IAVIStream_AddRef(iface);
		*obj = iface;
		return S_OK;
	}
	/* can return IGetFrame interface too */

	return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IAVIStream_fnAddRef(IAVIStream*iface) {
	ICOM_THIS(IAVIStreamImpl,iface);

	TRACE("(%p)->AddRef()\n",iface);
	return ++(This->ref);
}

static ULONG WINAPI IAVIStream_fnRelease(IAVIStream* iface) {
	ICOM_THIS(IAVIStreamImpl,iface);

	TRACE("(%p)->Release()\n",iface);
	if ((--(This->ref)) > 0 )
		return This->ref;
	IAVIStream_Destruct(This);

	HeapFree(AVIFILE_data.hHeap,0,iface);
	return 0;
}

/****************************************************************************
 * IAVIStream interface
 */

static HRESULT IAVIStream_Construct( IAVIStreamImpl* This )
{
	This->paf = NULL;
	This->pData = NULL;

	AVIFILE_data.dwClassObjRef ++;

	return S_OK;
}

static void IAVIStream_Destruct( IAVIStreamImpl* This )
{
	AVIFILE_data.dwClassObjRef --;
}

static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream*iface,LPARAM lParam1,LPARAM lParam2)
{
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->Create(%ld,%ld)\n",iface,lParam1,lParam2);

	This->paf = (IAVIFile*)lParam1;
	This->pData = (WINE_AVISTREAM_DATA*)lParam2;

	return S_OK;
}

static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream*iface,AVISTREAMINFOW *psi,LONG size)
{
	ICOM_THIS(IAVIStreamImpl,iface);
	AVISTREAMINFOW	siw;

	FIXME("(%p)->Info(%p,%ld)\n",iface,psi,size);
	if ( This->pData == NULL )
		return E_UNEXPECTED;

	memset( &siw, 0, sizeof(AVISTREAMINFOW) );
	siw.fccType = This->pData->pstrhdr->fccType;
	siw.fccHandler = This->pData->pstrhdr->fccHandler;
	siw.dwFlags = This->pData->pstrhdr->dwFlags;
	siw.dwCaps = 0; /* FIXME */
	siw.wPriority = This->pData->pstrhdr->wPriority;
	siw.wLanguage = This->pData->pstrhdr->wLanguage;
	siw.dwScale = This->pData->pstrhdr->dwScale;
	siw.dwRate = This->pData->pstrhdr->dwRate;
	siw.dwStart = This->pData->pstrhdr->dwStart;
	siw.dwLength = This->pData->pstrhdr->dwLength;
	siw.dwInitialFrames = This->pData->pstrhdr->dwInitialFrames;
	siw.dwSuggestedBufferSize = This->pData->pstrhdr->dwSuggestedBufferSize;
	siw.dwQuality = This->pData->pstrhdr->dwQuality;
	siw.dwSampleSize = This->pData->pstrhdr->dwSampleSize;
	siw.rcFrame.left = This->pData->pstrhdr->rcFrame.left;
	siw.rcFrame.top = This->pData->pstrhdr->rcFrame.top;
	siw.rcFrame.right = This->pData->pstrhdr->rcFrame.right;
	siw.rcFrame.bottom = This->pData->pstrhdr->rcFrame.bottom;
	siw.dwEditCount = 0; /* FIXME */
	siw.dwFormatChangeCount = 0; /* FIXME */
	/* siw.szName[64] */

	if ( size > sizeof(AVISTREAMINFOW) )
		size = sizeof(AVISTREAMINFOW);
	memcpy( psi, &siw, size );

	return S_OK;
}

static LONG WINAPI IAVIStream_fnFindSample(IAVIStream*iface,LONG pos,LONG flags)
{
	ICOM_THIS(IAVIStreamImpl,iface);
	HRESULT	hr;
	AVIINDEXENTRY*	pIndexEntry;
	DWORD		dwCountOfIndexEntry;
	LONG		lCur, lAdd, lEnd;

	FIXME("(%p)->FindSample(%ld,0x%08lx)\n",This,pos,flags);

	hr = AVIFILE_IAVIFile_GetIndexTable(
		This->paf, This->pData->dwStreamIndex,
		&pIndexEntry, &dwCountOfIndexEntry );
	if ( hr != S_OK )
		return -1L;

	if ( flags & (~(FIND_DIR|FIND_TYPE|FIND_RET)) )
	{
		FIXME( "unknown flag %08lx\n", flags );
		return -1L;
	}

	switch ( flags & FIND_DIR )
	{
	case FIND_NEXT:
		lCur = pos;
		lAdd = 1;
		lEnd = dwCountOfIndexEntry;
		if ( lCur > dwCountOfIndexEntry )
			return -1L;
		break;
	case FIND_PREV:
		lCur = pos;
		if ( lCur > dwCountOfIndexEntry )
			lCur = dwCountOfIndexEntry;
		lAdd = -1;
		lEnd = 0;
		break;
	case FIND_FROM_START:
		lCur = 0;
		lAdd = 1;
		lEnd = dwCountOfIndexEntry;
		break;
	default:
		FIXME( "unknown direction flag %08lx\n", (flags & FIND_DIR) );
		return -1L;
	}

	switch ( flags & FIND_TYPE )
	{
	case FIND_KEY:
		while ( 1 )
		{
			if ( pIndexEntry[lCur].dwFlags & AVIIF_KEYFRAME )
				break;
			if ( lCur == lEnd )
				return -1L;
			lCur += lAdd;
		}
		break;
	case FIND_ANY:
		while ( 1 )
		{
			if ( !(pIndexEntry[lCur].dwFlags & AVIIF_NOTIME) )
				break;
			if ( lCur == lEnd )
				return -1L;
			lCur += lAdd;
		}
		break;
	case FIND_FORMAT:
		FIXME( "FIND_FORMAT is not implemented.\n" );
		return -1L;
	default:
		FIXME( "unknown type flag %08lx\n", (flags & FIND_TYPE) );
		return -1L;
	}

	switch ( flags & FIND_RET )
	{
	case FIND_POS:
		return lCur;
	case FIND_LENGTH:
		FIXME( "FIND_LENGTH is not implemented.\n" );
		return -1L;
	case FIND_OFFSET:
		return pIndexEntry[lCur].dwChunkOffset;
	case FIND_SIZE:
		return pIndexEntry[lCur].dwChunkLength;
	case FIND_INDEX:
		FIXME( "FIND_INDEX is not implemented.\n" );
		return -1L;
	default:
		FIXME( "unknown return type flag %08lx\n", (flags & FIND_RET) );
		break;
	}

	return -1L;
}

static HRESULT WINAPI IAVIStream_fnReadFormat(IAVIStream*iface,LONG pos,LPVOID format,LONG *formatsize) {
	ICOM_THIS(IAVIStreamImpl,iface);

	TRACE("(%p)->ReadFormat(%ld,%p,%p)\n",This,pos,format,formatsize);
	if ( This->pData == NULL )
		return E_UNEXPECTED;

	/* FIXME - check pos. */
	if ( format == NULL )
	{
		*formatsize = This->pData->dwFmtLen;
		return S_OK;
	}
	if ( (*formatsize) < This->pData->dwFmtLen )
		return AVIERR_BUFFERTOOSMALL;

	memcpy( format, This->pData->pbFmt, This->pData->dwFmtLen );
	*formatsize = This->pData->dwFmtLen;

	return S_OK;
}

static HRESULT WINAPI IAVIStream_fnSetFormat(IAVIStream*iface,LONG pos,LPVOID format,LONG formatsize) {
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->SetFormat(%ld,%p,%ld)\n",This,pos,format,formatsize);
	return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnRead(IAVIStream*iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread) {
	ICOM_THIS(IAVIStreamImpl,iface);
	HRESULT	hr;
	AVIINDEXENTRY*	pIndexEntry;
	DWORD		dwCountOfIndexEntry;
	DWORD		dwFrameLength;

	FIXME("(%p)->Read(%ld,%ld,%p,%ld,%p,%p)\n",This,start,samples,buffer,buffersize,bytesread,samplesread);

	*bytesread = 0;
	*samplesread = 0;

	hr = AVIFILE_IAVIFile_GetIndexTable(
		This->paf, This->pData->dwStreamIndex,
		&pIndexEntry, &dwCountOfIndexEntry );
	if ( hr != S_OK )
		return hr;
	if ( start < 0 )
		return E_FAIL;
	if ( start >= dwCountOfIndexEntry || samples <= 0 )
	{
		FIXME("start %ld,samples %ld,total %ld\n",start,samples,dwCountOfIndexEntry);
		return S_OK;
	}

	/* FIXME - is this data valid??? */
	dwFrameLength = pIndexEntry[start].dwChunkLength + sizeof(DWORD)*2;

	if ( buffer == NULL )
	{
		*bytesread = dwFrameLength;
		*samplesread = 1;
		return S_OK;
	}
	if ( buffersize < dwFrameLength )
	{
		FIXME( "buffer is too small!\n" );
		return AVIERR_BUFFERTOOSMALL;
	}

	hr = AVIFILE_IAVIFile_ReadMovieData(
			This->paf,
			pIndexEntry[start].dwChunkOffset,
			dwFrameLength, buffer );
	if ( hr != S_OK )
	{
		FIXME( "ReadMovieData failed!\n");
		return hr;
	}
	*bytesread = dwFrameLength;
	*samplesread = 1;

	return S_OK;
}

static HRESULT WINAPI IAVIStream_fnWrite(IAVIStream*iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten) {
	ICOM_THIS(IAVIStreamImpl,iface);


	FIXME("(%p)->Write(%ld,%ld,%p,%ld,0x%08lx,%p,%p)\n",This,start,samples,buffer,buffersize,flags,sampwritten,byteswritten);
	return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnDelete(IAVIStream*iface,LONG start,LONG samples) {
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->Delete(%ld,%ld)\n",This,start,samples);
	return E_FAIL;
}
static HRESULT WINAPI IAVIStream_fnReadData(IAVIStream*iface,DWORD fcc,LPVOID lp,LONG *lpread) {
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->ReadData(0x%08lx,%p,%p)\n",This,fcc,lp,lpread);
	return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnWriteData(IAVIStream*iface,DWORD fcc,LPVOID lp,LONG size) {
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->WriteData(0x%08lx,%p,%ld)\n",This,fcc,lp,size);
	return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnSetInfo(IAVIStream*iface,AVISTREAMINFOW*info,LONG infolen) {
	ICOM_THIS(IAVIStreamImpl,iface);

	FIXME("(%p)->SetInfo(%p,%ld)\n",This,info,infolen);

	return E_FAIL;
}

