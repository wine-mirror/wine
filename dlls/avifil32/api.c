/*
 * Copyright 1999 Marcus Meissner
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winerror.h"
#include "ole2.h"
#include "vfw.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(avifile);

#include "avifile_private.h"


/***********************************************************************
 *		AVIFileInit (AVIFILE.100)
 *		AVIFileInit (AVIFIL32.@)
 */
void WINAPI AVIFileInit(void)
{
	TRACE("()\n");
	if ( AVIFILE_data.dwAVIFileRef == 0 )
	{
		if ( FAILED(CoInitialize(NULL)) )
			AVIFILE_data.fInitCOM = FALSE;
		else
			AVIFILE_data.fInitCOM = TRUE;
	}
	AVIFILE_data.dwAVIFileRef ++;
}

/***********************************************************************
 *		AVIFileExit (AVIFILE.101)
 *		AVIFileExit (AVIFIL32.@)
 */
void WINAPI AVIFileExit(void)
{
	TRACE("()\n");
	if ( AVIFILE_data.dwAVIFileRef == 0 )
	{
		ERR( "unexpected AVIFileExit()\n" );
		return;
	}

	AVIFILE_data.dwAVIFileRef --;
	if ( AVIFILE_data.dwAVIFileRef == 0 )
	{
		if ( AVIFILE_data.fInitCOM )
		{
			CoUninitialize();
			AVIFILE_data.fInitCOM = FALSE;
		}
	}
}

/***********************************************************************
 *		AVIFileAddRef (AVIFIL32.@)
 */
ULONG WINAPI AVIFileAddRef(PAVIFILE pfile)
{
	return IAVIFile_AddRef( pfile );
}

/***********************************************************************
 *		AVIFileRelease (AVIFILE.141)
 *		AVIFileRelease (AVIFIL32.@)
 */
ULONG WINAPI AVIFileRelease(PAVIFILE pfile)
{
	return IAVIFile_Release( pfile );
}

/***********************************************************************
 *		AVIFileOpen  (AVIFILE.102)
 *		AVIFileOpenA (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileOpenA(
	PAVIFILE* ppfile,LPCSTR szFile,UINT uMode,LPCLSID lpHandler )
{
	WCHAR*	pwsz;
	HRESULT	hr;

	TRACE("(%p,%p,%u,%p)\n",ppfile,szFile,uMode,lpHandler);
	pwsz = AVIFILE_strdupAtoW( szFile );
	if ( pwsz == NULL )
		return AVIERR_MEMORY;
	hr = AVIFileOpenW(ppfile,pwsz,uMode,lpHandler);
	HeapFree( AVIFILE_data.hHeap, 0, pwsz );
	return hr;
}

/***********************************************************************
 *		AVIFileOpenW (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileOpenW(
	PAVIFILE* ppfile,LPCWSTR szFile,UINT uMode,LPCLSID lpHandler )
{
	HRESULT	hr;
	IClassFactory*	pcf;
	CLSID	clsRIFF;

	TRACE("(%p,%p,%u,%p)\n",ppfile,szFile,uMode,lpHandler);
	*ppfile = (PAVIFILE)NULL;

	if ( lpHandler == NULL )
	{
		/* FIXME - check RIFF type and get a handler from registry
		 *         if IAVIFile::Open is worked...
		 */
		memcpy( &clsRIFF, &CLSID_AVIFile, sizeof(CLSID) );
		lpHandler = &clsRIFF;
	}

	/*
	 * FIXME - MS says IAVIFile::Open will be called,
	 *         but no such method in vfw.h... why????
	 */
	if ( !IsEqualGUID( lpHandler, &CLSID_AVIFile ) )
		return REGDB_E_CLASSNOTREG;

	hr = AVIFILE_DllGetClassObject(&CLSID_AVIFile,
				       &IID_IClassFactory,(void**)&pcf);
	if ( hr != S_OK )
		return hr;

	hr = IClassFactory_CreateInstance( pcf, NULL, &IID_IAVIFile,
					   (void**)ppfile );
	IClassFactory_Release( pcf );

	if ( hr == S_OK )
	{
		/* FIXME??? */
		hr = AVIFILE_IAVIFile_Open( *ppfile, szFile, uMode );
		if ( hr != S_OK )
		{
			IAVIFile_Release( (*ppfile) );
			*ppfile = NULL;
		}
	}

	return hr;
}

/***********************************************************************
 *		AVIFileInfoW (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoW(PAVIFILE pfile,AVIFILEINFOW* pfi,LONG lSize)
{
	return IAVIFile_Info( pfile, pfi, lSize );
}

/***********************************************************************
 *		AVIFileInfo  (AVIFIL32.@)
 *		AVIFileInfoA (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoA(PAVIFILE pfile,AVIFILEINFOA* pfi,LONG lSize)
{
	AVIFILEINFOW	fiw;
	HRESULT		hr;

	if ( lSize < sizeof(AVIFILEINFOA) )
		return AVIERR_BADSIZE;
	hr = AVIFileInfoW( pfile, &fiw, sizeof(AVIFILEINFOW) );
	if ( hr != S_OK )
		return hr;
	memcpy( pfi,&fiw,sizeof(AVIFILEINFOA) );
	AVIFILE_strncpyWtoA( pfi->szFileType, fiw.szFileType,
			     sizeof(pfi->szFileType) );
	pfi->szFileType[sizeof(pfi->szFileType)-1] = 0;

	return S_OK;
}

/***********************************************************************
 *		AVIFileGetStream (AVIFILE.143)
 *		AVIFileGetStream (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileGetStream(PAVIFILE pfile,PAVISTREAM* pas,DWORD fccType,LONG lParam)
{
	return IAVIFile_GetStream(pfile,pas,fccType,lParam);
}

/***********************************************************************
 *		AVIFileCreateStreamW (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamW(PAVIFILE pfile,PAVISTREAM* ppas,AVISTREAMINFOW* pasi)
{
	return IAVIFile_CreateStream(pfile,ppas,pasi);
}

/***********************************************************************
 *		AVIFileCreateStreamA (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamA(PAVIFILE pfile,PAVISTREAM* ppas,AVISTREAMINFOA* pasi)
{
	AVISTREAMINFOW	siw;
	HRESULT		hr;

	memcpy( &siw,pasi,sizeof(AVISTREAMINFOA) );
	AVIFILE_strncpyAtoW( siw.szName, pasi->szName,
		sizeof(siw.szName)/sizeof(siw.szName[0]) );
	siw.szName[sizeof(siw.szName)/sizeof(siw.szName[0])-1] = 0;

	hr = AVIFileCreateStreamW(pfile,ppas,&siw);

	return hr;
}

/***********************************************************************
 *		AVIFileWriteData (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileWriteData(
	PAVIFILE pfile,DWORD dwChunkId,LPVOID lpvData,LONG cbData )
{
	return IAVIFile_WriteData( pfile,dwChunkId,lpvData,cbData );
}

/***********************************************************************
 *		AVIFileReadData (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileReadData(
	PAVIFILE pfile,DWORD dwChunkId,LPVOID lpvData,LPLONG pcbData )
{
	return IAVIFile_ReadData( pfile,dwChunkId,lpvData,pcbData );
}

/***********************************************************************
 *		AVIFileEndRecord (AVIFIL32.@)
 */
HRESULT WINAPI AVIFileEndRecord( PAVIFILE pfile )
{
	return IAVIFile_EndRecord( pfile );
}

/***********************************************************************
 *		AVIStreamAddRef (AVIFIL32.@)
 */
ULONG WINAPI AVIStreamAddRef(PAVISTREAM pas)
{
	return IAVIStream_Release(pas);
}

/***********************************************************************
 *		AVIStreamRelease (AVIFIL32.@)
 */
ULONG WINAPI AVIStreamRelease(PAVISTREAM pas)
{
	return IAVIStream_Release(pas);
}

/***********************************************************************
 *		AVIStreamInfoW (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoW(PAVISTREAM pas,AVISTREAMINFOW* psi,LONG lSize)
{
 	return IAVIStream_Info(pas,psi,lSize);
}

/***********************************************************************
 *		AVIStreamInfo  (AVIFIL32.@)
 *		AVIStreamInfoA (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoA(PAVISTREAM pas,AVISTREAMINFOA* psi,LONG lSize)
{
 	AVISTREAMINFOW	siw;
	HRESULT		hr;

	if (lSize < sizeof(AVISTREAMINFOA))
		return AVIERR_BADSIZE;
 	hr = AVIStreamInfoW(pas,&siw,sizeof(AVISTREAMINFOW));
	if ( hr != S_OK )
		return hr;
	memcpy( psi,&siw,sizeof(AVIFILEINFOA) );
	AVIFILE_strncpyWtoA( psi->szName, siw.szName, sizeof(psi->szName) );
	psi->szName[sizeof(psi->szName)-1] = 0;

	return hr;
}

/***********************************************************************
 *		AVIStreamFindSample (AVIFIL32.@)
 */
LONG WINAPI AVIStreamFindSample(PAVISTREAM pas,LONG lPos,LONG lFlags)
{
	return IAVIStream_FindSample(pas,lPos,lFlags);
}

/***********************************************************************
 *		AVIStreamReadFormat (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadFormat(PAVISTREAM pas,LONG pos,LPVOID format,LONG *formatsize) {
	return IAVIStream_ReadFormat(pas,pos,format,formatsize);
}

/***********************************************************************
 *		AVIStreamSetFormat (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamSetFormat(PAVISTREAM pas,LONG pos,LPVOID format,LONG formatsize) {
	return IAVIStream_SetFormat(pas,pos,format,formatsize);
}

/***********************************************************************
 *		AVIStreamReadData (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadData(PAVISTREAM pas,DWORD fcc,LPVOID lp,LONG *lpread) {
	return IAVIStream_ReadData(pas,fcc,lp,lpread);
}

/***********************************************************************
 *		AVIStreamWriteData (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWriteData(PAVISTREAM pas,DWORD fcc,LPVOID lp,LONG size) {
	return IAVIStream_WriteData(pas,fcc,lp,size);
}

/***********************************************************************
 *		AVIStreamRead (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamRead(PAVISTREAM pas,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread)
{
	return IAVIStream_Read(pas,start,samples,buffer,buffersize,bytesread,samplesread);
}

/***********************************************************************
 *		AVIStreamWrite (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWrite(PAVISTREAM pas,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten) {
	return IAVIStream_Write(pas,start,samples,buffer,buffersize,flags,sampwritten,byteswritten);
}


/***********************************************************************
 *		AVIStreamStart (AVIFIL32.@)
 */
LONG WINAPI AVIStreamStart(PAVISTREAM pas)
{
	AVISTREAMINFOW	si;
	HRESULT			hr;

	hr = IAVIStream_Info(pas,&si,sizeof(si));
	if (hr != S_OK)
		return -1;
	return (LONG)si.dwStart;
}

/***********************************************************************
 *		AVIStreamLength (AVIFIL32.@)
 */
LONG WINAPI AVIStreamLength(PAVISTREAM pas)
{
	AVISTREAMINFOW	si;
	HRESULT			hr;

	hr = IAVIStream_Info(pas,&si,sizeof(si));
	if (hr != S_OK)
		return -1;
	return (LONG)si.dwLength;
}

/***********************************************************************
 *		AVIStreamTimeToSample (AVIFIL32.@)
 */
LONG WINAPI AVIStreamTimeToSample(PAVISTREAM pas,LONG lTime)
{
	AVISTREAMINFOW	si;
	HRESULT			hr;

	hr = IAVIStream_Info(pas,&si,sizeof(si));
	if (hr != S_OK)
		return -1;

	/* I am too lazy... */
	FIXME("(%p,%ld)",pas,lTime);
	return (LONG)-1L;
}

/***********************************************************************
 *		AVIStreamSampleToTime (AVIFIL32.@)
 */
LONG WINAPI AVIStreamSampleToTime(PAVISTREAM pas,LONG lSample)
{
	AVISTREAMINFOW	si;
	HRESULT			hr;

	hr = IAVIStream_Info(pas,&si,sizeof(si));
	if (hr != S_OK)
		return -1;

	/* I am too lazy... */
	FIXME("(%p,%ld)",pas,lSample);
	return (LONG)-1L;
}

/***********************************************************************
 *		AVIStreamBeginStreaming (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamBeginStreaming(PAVISTREAM pas,LONG lStart,LONG lEnd,LONG lRate)
{
	FIXME("(%p)->(%ld,%ld,%ld),stub!\n",pas,lStart,lEnd,lRate);
	return E_FAIL;
}

/***********************************************************************
 *		AVIStreamEndStreaming (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamEndStreaming(PAVISTREAM pas)
{
	FIXME("(%p)->(),stub!\n",pas);
	return E_FAIL;
}

/***********************************************************************
 *		AVIStreamGetFrameOpen (AVIFIL32.@)
 */
PGETFRAME WINAPI AVIStreamGetFrameOpen(PAVISTREAM pas,LPBITMAPINFOHEADER pbi)
{
	IGetFrame*	pgf;
	HRESULT		hr;
	AVISTREAMINFOW	si;

	FIXME("(%p,%p)\n",pas,pbi);

	hr = IAVIStream_Info(pas,&si,sizeof(si));
	if (hr != S_OK)
		return NULL;

	hr = AVIFILE_CreateIGetFrame((void**)&pgf,pas,pbi);
	if ( hr != S_OK )
		return NULL;
	hr = IGetFrame_Begin( pgf, si.dwStart, si.dwLength, 1000 );
	if ( hr != S_OK )
	{
		IGetFrame_Release( pgf );
		return NULL;
	}

	return pgf;
}

/***********************************************************************
 *		AVIStreamGetFrame (AVIFIL32.@)
 */
LPVOID WINAPI AVIStreamGetFrame(PGETFRAME pgf, LONG lPos)
{
	return IGetFrame_GetFrame(pgf,lPos);
}

/***********************************************************************
 *		AVIStreamGetFrameClose (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamGetFrameClose(PGETFRAME pgf)
{
	return IGetFrame_End(pgf);
}

/***********************************************************************
 *		AVIStreamOpenFromFileA (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileA(PAVISTREAM* ppas, LPCSTR szFile, DWORD fccType, LONG lParam, UINT uMode, CLSID* lpHandler)
{
	WCHAR*	pwsz;
	HRESULT	hr;

	pwsz = AVIFILE_strdupAtoW( szFile );
	if ( pwsz == NULL )
		return AVIERR_MEMORY;
	hr = AVIStreamOpenFromFileW(ppas,pwsz,fccType,lParam,uMode,lpHandler);
	HeapFree( AVIFILE_data.hHeap, 0, pwsz );
	return hr;
}

/***********************************************************************
 *		AVIStreamOpenFromFileW (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileW(PAVISTREAM* ppas, LPCWSTR szFile, DWORD fccType, LONG lParam, UINT uMode, CLSID* lpHandler)
{
	HRESULT	hr;
	PAVIFILE	paf;
	AVIFILEINFOW	fi;

	*ppas = NULL;
	hr = AVIFileOpenW(&paf,szFile,uMode,lpHandler);
	if ( hr != S_OK )
		return hr;
	hr = AVIFileInfoW(paf,&fi,sizeof(AVIFILEINFOW));
	if ( hr == S_OK )
		hr = AVIFileGetStream(paf,ppas,fccType,lParam);

	IAVIFile_Release(paf);

	return hr;
}

/***********************************************************************
 *		AVIStreamCreate (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamCreate(PAVISTREAM* ppas, LONG lParam1, LONG lParam2, CLSID* lpHandler)
{
	HRESULT	hr;
	IClassFactory*	pcf;

	*ppas = NULL;

	if ( lpHandler == NULL )
	{
		hr = AVIFILE_DllGetClassObject(&CLSID_AVIFile,
					       &IID_IClassFactory,(void**)&pcf);
	}
	else
	{
		if ( !AVIFILE_data.fInitCOM )
			return E_UNEXPECTED;
		hr = CoGetClassObject(lpHandler,CLSCTX_INPROC_SERVER,
				      NULL,&IID_IClassFactory,(void**)&pcf);
	}
	if ( hr != S_OK )
		return hr;

	hr = IClassFactory_CreateInstance( pcf, NULL, &IID_IAVIStream,
					   (void**)ppas );
	IClassFactory_Release( pcf );

	if ( hr == S_OK )
	{
		hr = IAVIStream_Create((*ppas),lParam1,lParam2);
		if ( hr != S_OK )
		{
			IAVIStream_Release((*ppas));
			*ppas = NULL;
		}
	}

	return hr;
}

/***********************************************************************
 *		AVIMakeCompressedStream (AVIFIL32.@)
 */
HRESULT WINAPI AVIMakeCompressedStream(PAVISTREAM *ppsCompressed,PAVISTREAM ppsSource,AVICOMPRESSOPTIONS *aco,CLSID *pclsidHandler)
{
	FIXME("(%p,%p,%p,%p)\n",ppsCompressed,ppsSource,aco,pclsidHandler);
	return E_FAIL;
}

