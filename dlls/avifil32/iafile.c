/*
 * Copyright 1999 Marcus Meissner
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 *
 * FIXME - implements editing/writing.
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

#define	AVIFILE_STREAMS_MAX	4


static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile* iface,REFIID refiid,LPVOID *obj);
static ULONG WINAPI IAVIFile_fnAddRef(IAVIFile* iface);
static ULONG WINAPI IAVIFile_fnRelease(IAVIFile* iface);
static HRESULT WINAPI IAVIFile_fnInfo(IAVIFile*iface,AVIFILEINFOW*afi,LONG size);
static HRESULT WINAPI IAVIFile_fnGetStream(IAVIFile*iface,PAVISTREAM*avis,DWORD fccType,LONG lParam);
static HRESULT WINAPI IAVIFile_fnCreateStream(IAVIFile*iface,PAVISTREAM*avis,AVISTREAMINFOW*asi);
static HRESULT WINAPI IAVIFile_fnWriteData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG size);
static HRESULT WINAPI IAVIFile_fnReadData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG *size);
static HRESULT WINAPI IAVIFile_fnEndRecord(IAVIFile*iface);
static HRESULT WINAPI IAVIFile_fnDeleteStream(IAVIFile*iface,DWORD fccType,LONG lParam);

struct ICOM_VTABLE(IAVIFile) iavift = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IAVIFile_fnQueryInterface,
    IAVIFile_fnAddRef,
    IAVIFile_fnRelease,
    IAVIFile_fnInfo,
    IAVIFile_fnGetStream,
    IAVIFile_fnCreateStream,
    IAVIFile_fnWriteData,
    IAVIFile_fnReadData,
    IAVIFile_fnEndRecord,
    IAVIFile_fnDeleteStream,
    /* IAVIFILE_fnOpen */ /* FIXME? */
};


typedef struct IAVIFileImpl
{
	ICOM_VFIELD(IAVIFile);
	/* IUnknown stuff */
	DWORD			ref;
	/* IAVIFile stuff */
	HANDLE			hf;
	DWORD			dwAVIFileCaps;
	DWORD			dwAVIFileScale;
	DWORD			dwAVIFileRate;
	DWORD			dwAVIFileLength;
	DWORD			dwAVIFileEditCount;
	MainAVIHeader		hdr;
	IAVIStream*		pStreams[AVIFILE_STREAMS_MAX];
	AVIStreamHeader		strhdrs[AVIFILE_STREAMS_MAX];
	DWORD			dwMoviTop;
	DWORD			dwCountOfIndexEntry;
	AVIINDEXENTRY*		pIndexEntry;
	AVIINDEXENTRY*		pStreamIndexEntry[AVIFILE_STREAMS_MAX+1];
} IAVIFileImpl;


/****************************************************************************
 * AVI file parser.
 */

static HRESULT AVIFILE_IAVIFile_ReadNextChunkHeader(
		IAVIFileImpl* This, FOURCC* pfcc, DWORD* pdwSize )
{
	BYTE	buf[8];
	DWORD	dwRead;

	if ( ( !ReadFile( This->hf, buf, 8, &dwRead, NULL ) ) ||
	     ( 8 != dwRead ) )
		return AVIERR_FILEREAD;
	*pfcc = mmioFOURCC(buf[0],buf[1],buf[2],buf[3]);
	*pdwSize = ( ((DWORD)buf[4])       ) |
		   ( ((DWORD)buf[5]) <<  8 ) |
		   ( ((DWORD)buf[6]) << 16 ) |
		   ( ((DWORD)buf[7]) << 24 );

	return S_OK;
}

static HRESULT AVIFILE_IAVIFile_SkipChunkData(
		IAVIFileImpl* This, DWORD dwChunkSize )
{
	LONG	lHigh = 0;
	DWORD	dwRes;

	if ( dwChunkSize == 0 )
		return S_OK;

	SetLastError(NO_ERROR);
	dwRes = SetFilePointer( This->hf, (LONG)dwChunkSize,
				&lHigh, FILE_CURRENT );
	if ( dwRes == (DWORD)0xffffffff && GetLastError() != NO_ERROR )
		return AVIERR_FILEREAD;

	return S_OK;
}

static HRESULT AVIFILE_IAVIFile_ReadChunkData(
		IAVIFileImpl* This, DWORD dwChunkSize,
		LPVOID lpvBuf, DWORD dwBufSize, LPDWORD lpdwRead )
{
	if ( dwBufSize > dwChunkSize )
		dwBufSize = dwChunkSize;
	if ( ( !ReadFile( This->hf, lpvBuf, dwBufSize, lpdwRead, NULL ) ) ||
	     ( dwBufSize != *lpdwRead ) )
		return AVIERR_FILEREAD;

	return AVIFILE_IAVIFile_SkipChunkData( This, dwChunkSize - dwBufSize );
}

static HRESULT AVIFILE_IAVIFile_SeekToSpecifiedChunk(
		IAVIFileImpl* This, FOURCC fccType, DWORD* pdwLen )
{
	HRESULT	hr;
	FOURCC	fcc;
	BYTE	buf[4];
	DWORD	dwRead;

	while ( 1 )
	{
		hr = AVIFILE_IAVIFile_ReadNextChunkHeader(
				This, &fcc, pdwLen );
		if ( hr != S_OK )
			return hr;
		if ( fcc == fccType )
			return S_OK;

		if ( fcc == FOURCC_LIST )
		{
		    if ( ( !ReadFile( This->hf, buf, 4, &dwRead, NULL ) ) ||
			 ( 4 != dwRead ) )
			return AVIERR_FILEREAD;
		}
		else
		{
		    hr = AVIFILE_IAVIFile_SkipChunkData(
					This, *pdwLen );
		    if ( hr != S_OK )
			return hr;
		}
	}
}


WINE_AVISTREAM_DATA* AVIFILE_Alloc_IAVIStreamData( DWORD dwFmtLen )
{
	WINE_AVISTREAM_DATA*	pData;

	pData = (WINE_AVISTREAM_DATA*)
		HeapAlloc( AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
			   sizeof(WINE_AVISTREAM_DATA) );
	if ( pData == NULL )
		return NULL;
	if ( dwFmtLen > 0 )
	{
		pData->pbFmt = (BYTE*)
			HeapAlloc( AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
				   sizeof(BYTE)*dwFmtLen );
		if ( pData->pbFmt == NULL )
		{
			AVIFILE_Free_IAVIStreamData( pData );
			return NULL;
		}
	}
	pData->dwFmtLen = dwFmtLen;

	return pData;
}

void AVIFILE_Free_IAVIStreamData( WINE_AVISTREAM_DATA* pData )
{
	if ( pData != NULL )
	{
		if ( pData->pbFmt != NULL )
			HeapFree( AVIFILE_data.hHeap,0,pData->pbFmt );
		HeapFree( AVIFILE_data.hHeap,0,pData );
	}
}

static void AVIFILE_IAVIFile_InitIndexTable(
		IAVIFileImpl* This,
		AVIINDEXENTRY* pIndexBuf,
		AVIINDEXENTRY* pIndexData,
		DWORD dwCountOfIndexEntry )
{
	DWORD	dwStreamIndex;
	DWORD	dwIndex;
	FOURCC	ckid;

	dwStreamIndex = 0;
	for ( ; dwStreamIndex < (AVIFILE_STREAMS_MAX+1); dwStreamIndex ++ )
		This->pStreamIndexEntry[dwStreamIndex] = NULL;

	dwStreamIndex = 0;
	for ( ; dwStreamIndex < This->hdr.dwStreams; dwStreamIndex ++ )
	{
		ckid = mmioFOURCC('0','0'+dwStreamIndex,0,0);
		TRACE( "testing ckid %c%c%c%c\n",
			(int)(ckid>> 0)&0xff,
			(int)(ckid>> 8)&0xff,
			(int)(ckid>>16)&0xff,
			(int)(ckid>>24)&0xff );
		This->pStreamIndexEntry[dwStreamIndex] = pIndexBuf;
		FIXME( "pIndexBuf = %p\n", pIndexBuf );
		for ( dwIndex = 0; dwIndex < dwCountOfIndexEntry; dwIndex++ )
		{
		    TRACE( "ckid %c%c%c%c\n",
			    (int)(pIndexData[dwIndex].ckid>> 0)&0xff,
                            (int)(pIndexData[dwIndex].ckid>> 8)&0xff,
                            (int)(pIndexData[dwIndex].ckid>>16)&0xff,
                            (int)(pIndexData[dwIndex].ckid>>24)&0xff );
                            
		    if ( (pIndexData[dwIndex].ckid & mmioFOURCC(0xff,0xff,0,0))
								== ckid )
		    {
			memcpy( pIndexBuf, &pIndexData[dwIndex],
				sizeof(AVIINDEXENTRY) );
			pIndexBuf ++;
		    }
		}
                FIXME( "pIndexBuf = %p\n", pIndexBuf );
	}
	This->pStreamIndexEntry[This->hdr.dwStreams] = pIndexBuf;
}


/****************************************************************************
 * Create an IAVIFile object.
 */

static HRESULT AVIFILE_IAVIFile_Construct( IAVIFileImpl* This );
static void AVIFILE_IAVIFile_Destruct( IAVIFileImpl* This );

HRESULT AVIFILE_CreateIAVIFile(void** ppobj)
{
	IAVIFileImpl	*This;
	HRESULT		hr;

	TRACE("(%p)\n",ppobj);
	*ppobj = NULL;
	This = (IAVIFileImpl*)HeapAlloc(AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
					sizeof(IAVIFileImpl));
	if ( This == NULL )
		return AVIERR_MEMORY;
	This->ref = 1;
	ICOM_VTBL(This) = &iavift;
	hr = AVIFILE_IAVIFile_Construct( This );
	if ( hr != S_OK )
	{
		AVIFILE_IAVIFile_Destruct( This );
		return hr;
	}

	TRACE("new -> %p\n",This);
	*ppobj = (LPVOID)This;

	return S_OK;
}

/****************************************************************************
 * IUnknown interface
 */

static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile* iface,REFIID refiid,LPVOID *obj) {
	ICOM_THIS(IAVIFileImpl,iface);

	TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(refiid),obj);
	if ( IsEqualGUID(&IID_IUnknown,refiid) ||
	     IsEqualGUID(&IID_IAVIFile,refiid) )
	{
		*obj = iface;
		IAVIFile_AddRef(iface);
		return S_OK;
	}
	return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IAVIFile_fnAddRef(IAVIFile* iface) {
	ICOM_THIS(IAVIFileImpl,iface);

	TRACE("(%p)->AddRef()\n",iface);
	return ++(This->ref);
}

static ULONG WINAPI IAVIFile_fnRelease(IAVIFile* iface) {
	ICOM_THIS(IAVIFileImpl,iface);

	TRACE("(%p)->Release()\n",iface);
	if ( (--(This->ref)) > 0 )
		return This->ref;

	AVIFILE_IAVIFile_Destruct(This);
	HeapFree(AVIFILE_data.hHeap,0,iface);
	return 0;
}

/****************************************************************************
 * IAVIFile interface
 */

static HRESULT AVIFILE_IAVIFile_Construct( IAVIFileImpl* This )
{
	DWORD	dwIndex;

	This->hf = INVALID_HANDLE_VALUE;
	This->dwAVIFileCaps = 0;
	This->dwAVIFileScale = 0;
	This->dwAVIFileRate = 0;
	This->dwAVIFileLength = 0;
	This->dwAVIFileEditCount = 0;
	for ( dwIndex = 0; dwIndex < AVIFILE_STREAMS_MAX; dwIndex++ )
		This->pStreams[dwIndex] = NULL;
	This->dwCountOfIndexEntry = 0;
	This->pIndexEntry = NULL;

	AVIFILE_data.dwClassObjRef ++;

	return S_OK;
}

static void AVIFILE_IAVIFile_Destruct( IAVIFileImpl* This )
{
	DWORD	dwIndex;

	if ( This->pIndexEntry != NULL )
	{
		HeapFree(AVIFILE_data.hHeap,0,This->pIndexEntry);
		This->pIndexEntry = NULL;
	}

	for ( dwIndex = 0; dwIndex < AVIFILE_STREAMS_MAX; dwIndex++ )
	{
		if ( This->pStreams[dwIndex] != NULL )
		{
			IAVIStream_Release( This->pStreams[dwIndex] );
			This->pStreams[dwIndex] = NULL;
		}
	}

	if ( This->hf != INVALID_HANDLE_VALUE )
		CloseHandle( This->hf );

	AVIFILE_data.dwClassObjRef --;
}

static HRESULT WINAPI IAVIFile_fnInfo(IAVIFile*iface,AVIFILEINFOW*afi,LONG size)
{
	ICOM_THIS(IAVIFileImpl,iface);
	AVIFILEINFOW	fiw;

	FIXME("(%p)->Info(%p,%ld)\n",iface,afi,size);

	memset( &fiw, 0, sizeof(fiw) );
	fiw.dwMaxBytesPerSec = This->hdr.dwMaxBytesPerSec;
	fiw.dwFlags = This->hdr.dwFlags;
	fiw.dwCaps = This->dwAVIFileCaps;
	fiw.dwStreams = This->hdr.dwStreams;
	fiw.dwSuggestedBufferSize = This->hdr.dwSuggestedBufferSize;
	fiw.dwWidth = This->hdr.dwWidth;
	fiw.dwHeight = This->hdr.dwHeight;
	fiw.dwScale = This->dwAVIFileScale; /* FIXME */
	fiw.dwRate = This->dwAVIFileRate; /* FIXME */
	fiw.dwLength = This->dwAVIFileLength; /* FIXME */
	fiw.dwEditCount = This->dwAVIFileEditCount; /* FIXME */
	/* fiw.szFileType[64]; */

	if ( size > sizeof(AVIFILEINFOW) )
		size = sizeof(AVIFILEINFOW);
	memcpy( afi, &fiw, size );

	return S_OK;
}

static HRESULT WINAPI IAVIFile_fnGetStream(IAVIFile*iface,PAVISTREAM*avis,DWORD fccType,LONG lParam)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p)->GetStream(%p,0x%08lx,%ld)\n",iface,avis,fccType,lParam);
	if ( fccType != 0 )
		return E_FAIL;
	if ( lParam < 0 || lParam >= This->hdr.dwStreams )
		return E_FAIL;
	*avis = This->pStreams[lParam];
	IAVIStream_AddRef( *avis );

	return S_OK;
}

static HRESULT WINAPI IAVIFile_fnCreateStream(IAVIFile*iface,PAVISTREAM*avis,AVISTREAMINFOW*asi)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p,%p,%p)\n",This,avis,asi);
	return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnWriteData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG size)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p)->WriteData(0x%08lx,%p,%ld)\n",This,ckid,lpData,size);
	/* FIXME: write data to file */
	return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnReadData(IAVIFile*iface,DWORD ckid,LPVOID lpData,LONG *size)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p)->ReadData(0x%08lx,%p,%p)\n",This,ckid,lpData,size);
	/* FIXME: read at most size bytes from file */

	return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnEndRecord(IAVIFile*iface)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p)->EndRecord()\n",This);
	/* FIXME: end record? */
	return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnDeleteStream(IAVIFile*iface,DWORD fccType,LONG lParam)
{
	ICOM_THIS(IAVIFileImpl,iface);

	FIXME("(%p)->DeleteStream(0x%08lx,%ld)\n",This,fccType,lParam);
	/* FIXME: delete stream? */
	return E_FAIL;
}

/*****************************************************************************
 *	AVIFILE_IAVIFile_Open (internal)
 */
HRESULT AVIFILE_IAVIFile_Open( PAVIFILE paf, LPCWSTR szFile, UINT uMode )
{
	ICOM_THIS(IAVIFileImpl,paf);
	HRESULT		hr;
	DWORD		dwAcc;
	DWORD		dwShared;
	DWORD		dwCreate;
	BYTE		buf[12];
	DWORD		dwRead;
	FOURCC		fccFileType;
	DWORD		dwLen;
	DWORD		dwIndex;

	FIXME("(%p)->Open(%p,%u)\n",This,szFile,uMode);

	if ( This->hf != INVALID_HANDLE_VALUE )
	{
		CloseHandle( This->hf );
		This->hf = INVALID_HANDLE_VALUE;
	}

	switch ( uMode & 0x3 )
	{
	case OF_READ: /* 0x0 */
		dwAcc = GENERIC_READ;
		dwCreate = OPEN_EXISTING;
		This->dwAVIFileCaps = AVIFILECAPS_CANREAD;
		break;
	case OF_WRITE: /* 0x1 */
		dwAcc = GENERIC_WRITE;
		dwCreate = OPEN_ALWAYS;
		This->dwAVIFileCaps = AVIFILECAPS_CANWRITE;
		break;
	case OF_READWRITE: /* 0x2 */
		dwAcc = GENERIC_READ|GENERIC_WRITE;
		dwCreate = OPEN_ALWAYS;
		This->dwAVIFileCaps = AVIFILECAPS_CANREAD|AVIFILECAPS_CANWRITE;
		break;
	default:
		return E_FAIL;
	}

	if ( This->dwAVIFileCaps & AVIFILECAPS_CANWRITE )
	{
		FIXME( "editing AVI is currently not supported!\n" );
		return E_FAIL;
	}

	switch ( uMode & 0x70 )
	{
	case OF_SHARE_COMPAT: /* 0x00 */
		dwShared = FILE_SHARE_READ|FILE_SHARE_WRITE;
		break;
	case OF_SHARE_EXCLUSIVE: /* 0x10 */
		dwShared = 0;
		break;
	case OF_SHARE_DENY_WRITE: /* 0x20 */
		dwShared = FILE_SHARE_READ;
		break;
	case OF_SHARE_DENY_READ: /* 0x30 */
		dwShared = FILE_SHARE_WRITE;
		break;
	case OF_SHARE_DENY_NONE: /* 0x40 */
		dwShared = FILE_SHARE_READ|FILE_SHARE_WRITE;
		break;
	default:
		return E_FAIL;
	}
	if ( uMode & OF_CREATE )
		dwCreate = CREATE_ALWAYS;

	This->hf = CreateFileW( szFile, dwAcc, dwShared, NULL,
				dwCreate, FILE_ATTRIBUTE_NORMAL,
				(HANDLE)NULL );
	if ( This->hf == INVALID_HANDLE_VALUE )
		return AVIERR_FILEOPEN;

	if ( dwAcc & GENERIC_READ )
	{
	    if ( !ReadFile( This->hf, buf, 12, &dwRead, NULL ) )
		return AVIERR_FILEREAD;
	    if ( dwRead == 12 )
	    {
		if ( mmioFOURCC(buf[0],buf[1],buf[2],buf[3]) != FOURCC_RIFF )
			return AVIERR_BADFORMAT;

		fccFileType = mmioFOURCC(buf[8],buf[9],buf[10],buf[11]);
		if ( fccFileType != formtypeAVI )
			return AVIERR_BADFORMAT;

		/* get AVI main header. */
		hr = AVIFILE_IAVIFile_SeekToSpecifiedChunk(
				This, ckidAVIMAINHDR, &dwLen );
		if ( hr != S_OK )
			return hr;
		if ( dwLen < (sizeof(DWORD)*10) )
			return AVIERR_BADFORMAT;
		hr = AVIFILE_IAVIFile_ReadChunkData(
				This, dwLen,
				&(This->hdr), sizeof(MainAVIHeader), &dwLen );
		if ( This->hdr.dwStreams == 0 ||
		     This->hdr.dwStreams > AVIFILE_STREAMS_MAX )
			return AVIERR_BADFORMAT;

		/* get stream headers. */
		dwIndex = 0;
		while ( dwIndex < This->hdr.dwStreams )
		{
			WINE_AVISTREAM_DATA*	pData;

			hr = AVIFILE_IAVIFile_SeekToSpecifiedChunk(
					This, ckidSTREAMHEADER, &dwLen );
			if ( hr != S_OK )
				return hr;
			if ( dwLen < (sizeof(DWORD)*12) )
				return AVIERR_BADFORMAT;
			hr = AVIFILE_IAVIFile_ReadChunkData(
				This, dwLen,
				&This->strhdrs[dwIndex],
				sizeof(AVIStreamHeader), &dwLen );

			hr = AVIFILE_IAVIFile_SeekToSpecifiedChunk(
					This, ckidSTREAMFORMAT, &dwLen );
			if ( hr != S_OK )
				return hr;
			pData = AVIFILE_Alloc_IAVIStreamData( dwLen );
			if ( pData == NULL )
				return AVIERR_MEMORY;
			hr = AVIFILE_IAVIFile_ReadChunkData(
				This, dwLen,
				pData->pbFmt, dwLen, &dwLen );
			if ( hr != S_OK )
			{
				AVIFILE_Free_IAVIStreamData( pData );
				return hr;
			}
			pData->dwStreamIndex = dwIndex;
			pData->pstrhdr = &This->strhdrs[dwIndex];

			hr = AVIStreamCreate(&This->pStreams[dwIndex],
					     (LONG)paf, (LONG)(pData), NULL );
			if ( hr != S_OK )
			{
				AVIFILE_Free_IAVIStreamData( pData );
				return hr;
			}

			if ( (This->strhdrs[dwIndex].fccType
					== mmioFOURCC('v','i','d','s')) ||
			     (This->strhdrs[dwIndex].fccType
					== mmioFOURCC('V','I','D','S')) )
			{
				This->dwAVIFileScale =
					This->strhdrs[dwIndex].dwScale;
				This->dwAVIFileRate =
					This->strhdrs[dwIndex].dwRate;
				This->dwAVIFileLength =
					This->strhdrs[dwIndex].dwLength;
			}
			else
			if ( This->dwAVIFileScale == 0 )
			{
				This->dwAVIFileScale =
					This->strhdrs[dwIndex].dwScale;
				This->dwAVIFileRate =
					This->strhdrs[dwIndex].dwRate;
				This->dwAVIFileLength =
					This->strhdrs[dwIndex].dwLength;
			}

			dwIndex ++;
		}

		/* skip movi. */
		while ( 1 )
		{
			hr = AVIFILE_IAVIFile_SeekToSpecifiedChunk(
					This, FOURCC_LIST, &dwLen );
			if ( hr != S_OK )
				return hr;
			if ( dwLen < 4 )
				return AVIERR_BADFORMAT;

			This->dwMoviTop = SetFilePointer( This->hf,0,NULL,FILE_CURRENT );
			if ( This->dwMoviTop == 0xffffffff )
				return AVIERR_BADFORMAT;

			if ( ( !ReadFile(This->hf, buf, 4, &dwRead, NULL) ) ||
			     ( dwRead != 4 ) )
				return AVIERR_FILEREAD;

			hr = AVIFILE_IAVIFile_SkipChunkData(
					This, dwLen - 4 );
			if ( hr != S_OK )
				return hr;
			if ( mmioFOURCC(buf[0],buf[1],buf[2],buf[3])
				== mmioFOURCC('m', 'o', 'v', 'i') )
				break;
		}

		/* get idx1. */
		hr = AVIFILE_IAVIFile_SeekToSpecifiedChunk(
				This, ckidAVINEWINDEX, &dwLen );
		if ( hr != S_OK )
			return hr;

		This->dwCountOfIndexEntry = dwLen / sizeof(AVIINDEXENTRY);
		This->pIndexEntry = (AVIINDEXENTRY*)
			HeapAlloc(AVIFILE_data.hHeap,HEAP_ZERO_MEMORY,
				  sizeof(AVIINDEXENTRY) *
				  This->dwCountOfIndexEntry * 2 );
		if ( This->pIndexEntry == NULL )
			return AVIERR_MEMORY;
		hr = AVIFILE_IAVIFile_ReadChunkData(
				This, dwLen,
				This->pIndexEntry + This->dwCountOfIndexEntry,
				sizeof(AVIINDEXENTRY) *
				This->dwCountOfIndexEntry, &dwLen );
		if ( hr != S_OK )
			return hr;
		AVIFILE_IAVIFile_InitIndexTable(
				This, This->pIndexEntry,
				This->pIndexEntry + This->dwCountOfIndexEntry,
				This->dwCountOfIndexEntry );
	    }
	    else
	    {
		/* FIXME - create the handle has GENERIC_WRITE access. */
		return AVIERR_FILEREAD;
	    }
	}
	else
	{
	    return AVIERR_FILEOPEN; /* FIXME */
	}

	return S_OK;
}

/*****************************************************************************
 *	AVIFILE_IAVIFile_GetIndexTable (internal)
 */
HRESULT AVIFILE_IAVIFile_GetIndexTable( PAVIFILE paf, DWORD dwStreamIndex,
					AVIINDEXENTRY** ppIndexEntry,
					DWORD* pdwCountOfIndexEntry )
{
	ICOM_THIS(IAVIFileImpl,paf);

	if ( dwStreamIndex < 0 || dwStreamIndex >= This->hdr.dwStreams )
	{
		FIXME( "invalid stream index %lu\n", dwStreamIndex );
		return E_FAIL;
	}
	FIXME( "cur %p, next %p\n",
		This->pStreamIndexEntry[dwStreamIndex],
		This->pStreamIndexEntry[dwStreamIndex+1] );
	*ppIndexEntry = This->pStreamIndexEntry[dwStreamIndex];
	*pdwCountOfIndexEntry =
		This->pStreamIndexEntry[dwStreamIndex+1] -
			This->pStreamIndexEntry[dwStreamIndex];

	return S_OK;
}

/*****************************************************************************
 *	AVIFILE_IAVIFile_ReadMovieData (internal)
 */
HRESULT AVIFILE_IAVIFile_ReadMovieData( PAVIFILE paf, DWORD dwOffset,
					DWORD dwLength, LPVOID lpvBuf )
{
	ICOM_THIS(IAVIFileImpl,paf);
	LONG	lHigh = 0;
	DWORD	dwRes;

	if ( dwLength == 0 )
		return S_OK;
	SetLastError(NO_ERROR);
	dwRes = SetFilePointer( This->hf, (LONG)(dwOffset+This->dwMoviTop),
				&lHigh, FILE_BEGIN );
	if ( dwRes == (DWORD)0xffffffff && GetLastError() != NO_ERROR )
		return AVIERR_FILEREAD;

	if ( ( !ReadFile(This->hf, lpvBuf, dwLength, &dwRes, NULL) ) ||
	     ( dwLength != dwRes ) )
	{
		FIXME( "error in ReadFile()\n" );
		return AVIERR_FILEREAD;
	}

	return S_OK;
}

