/*
 * Copyright 1999 Marcus Meissner
 * Copyright 2002 Michael Günnewig
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

#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "windowsx.h"
#include "mmsystem.h"
#include "vfw.h"

#include "avifile_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************/

static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile* iface,REFIID refiid,LPVOID *obj);
static ULONG   WINAPI IAVIFile_fnAddRef(IAVIFile* iface);
static ULONG   WINAPI IAVIFile_fnRelease(IAVIFile* iface);
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
  IAVIFile_fnDeleteStream
};

static HRESULT WINAPI IPersistFile_fnQueryInterface(IPersistFile*iface,REFIID refiid,LPVOID*obj);
static ULONG   WINAPI IPersistFile_fnAddRef(IPersistFile*iface);
static ULONG   WINAPI IPersistFile_fnRelease(IPersistFile*iface);
static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile*iface,CLSID*pClassID);
static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile*iface);
static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile*iface,LPCOLESTR pszFileName,DWORD dwMode);
static HRESULT WINAPI IPersistFile_fnSave(IPersistFile*iface,LPCOLESTR pszFileName,BOOL fRemember);
static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile*iface,LPCOLESTR pszFileName);
static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile*iface,LPOLESTR*ppszFileName);

struct ICOM_VTABLE(IPersistFile) ipersistft = {
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IPersistFile_fnQueryInterface,
  IPersistFile_fnAddRef,
  IPersistFile_fnRelease,
  IPersistFile_fnGetClassID,
  IPersistFile_fnIsDirty,
  IPersistFile_fnLoad,
  IPersistFile_fnSave,
  IPersistFile_fnSaveCompleted,
  IPersistFile_fnGetCurFile
};

static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream*iface,REFIID refiid,LPVOID *obj);
static ULONG   WINAPI IAVIStream_fnAddRef(IAVIStream*iface);
static ULONG   WINAPI IAVIStream_fnRelease(IAVIStream* iface);
static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream*iface,LPARAM lParam1,LPARAM lParam2);
static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream*iface,AVISTREAMINFOW *psi,LONG size);
static LONG    WINAPI IAVIStream_fnFindSample(IAVIStream*iface,LONG pos,LONG flags);
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

typedef struct _IAVIFileImpl IAVIFileImpl;

typedef struct _IPersistFileImpl {
  /* IUnknown stuff */
  ICOM_VFIELD(IPersistFile);

  /* IPersistFile stuff */
  IAVIFileImpl     *paf;
} IPersistFileImpl;

typedef struct _IAVIStreamImpl {
  /* IUnknown stuff */
  ICOM_VFIELD(IAVIStream);
  DWORD		    ref;

  /* IAVIStream stuff */
  IAVIFileImpl     *paf;
  AVISTREAMINFOW    sInfo;
} IAVIStreamImpl;

struct _IAVIFileImpl {
  /* IUnknown stuff */
  ICOM_VFIELD(IAVIFile);
  DWORD		    ref;

  /* IAVIFile stuff... */
  IPersistFileImpl  iPersistFile;

  AVIFILEINFOW      fInfo;
  IAVIStreamImpl   *ppStreams[MAX_AVISTREAMS];

  /* IPersistFile stuff ... */
  HMMIO             hmmio;
  LPWSTR            szFileName;
  UINT              uMode;
  BOOL              fDirty;
};

/***********************************************************************/

HRESULT AVIFILE_CreateAVIFile(REFIID riid, LPVOID *ppv)
{
  IAVIFileImpl *pfile;
  HRESULT       hr;

  assert(riid != NULL && ppv != NULL);

  *ppv = NULL;

  pfile = (IAVIFileImpl*)LocalAlloc(LPTR, sizeof(IAVIFileImpl));
  if (pfile == NULL)
    return AVIERR_MEMORY;

  ICOM_VTBL(pfile)               = &iavift;
  ICOM_VTBL(&pfile->iPersistFile) = &ipersistft;
  pfile->ref = 0;
  pfile->iPersistFile.paf = pfile;

  hr = IUnknown_QueryInterface((IUnknown*)pfile, riid, ppv);
  if (FAILED(hr))
    LocalFree((HLOCAL)pfile);

  return hr;
}

static HRESULT WINAPI IAVIFile_fnQueryInterface(IAVIFile *iface, REFIID refiid,
						LPVOID *obj)
{
  ICOM_THIS(IAVIFileImpl,iface);

  TRACE("(%p,%s,%p)\n", This, debugstr_guid(refiid), obj);

  if (IsEqualGUID(&IID_IUnknown, refiid) ||
      IsEqualGUID(&IID_IAVIFile, refiid)) {
    *obj = iface;
    return S_OK;
  } else if (IsEqualGUID(&IID_IPersistFile, refiid)) {
    *obj = &This->iPersistFile;
    return S_OK;
  }

  return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IAVIFile_fnAddRef(IAVIFile *iface)
{
  ICOM_THIS(IAVIFileImpl,iface);

  TRACE("(%p)\n",iface);
  return ++(This->ref);
}

static ULONG WINAPI IAVIFile_fnRelease(IAVIFile *iface)
{
  ICOM_THIS(IAVIFileImpl,iface);

  FIXME("(%p): partial stub!\n",iface);
  if (!--(This->ref)) {
    if (This->fDirty) {
      /* FIXME: write headers to disk */
    }

    HeapFree(GetProcessHeap(),0,iface);
    return 0;
  }
  return This->ref;
}

static HRESULT WINAPI IAVIFile_fnInfo(IAVIFile *iface, LPAVIFILEINFOW afi,
				      LONG size)
{
  ICOM_THIS(IAVIFileImpl,iface);

  TRACE("(%p,%p,%ld)\n",iface,afi,size);

  if (afi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  memcpy(afi, &This->fInfo, min(size, sizeof(This->fInfo)));

  if (size < sizeof(This->fInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static HRESULT WINAPI IAVIFile_fnGetStream(IAVIFile *iface, PAVISTREAM *avis,
					   DWORD fccType, LONG lParam)
{
  FIXME("(%p,%p,0x%08lX,%ld): stub\n", iface, avis, fccType, lParam);
  /* FIXME: create interface etc. */
  return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnCreateStream(IAVIFile *iface,PAVISTREAM *avis,
					      LPAVISTREAMINFOW asi)
{
/*  ICOM_THIS(IAVIStreamImpl,iface); */

  FIXME("(%p,%p,%p): stub\n", iface, avis, asi);

  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IAVIFile_fnWriteData(IAVIFile *iface, DWORD ckid,
					   LPVOID lpData, LONG size)
{
  FIXME("(%p,0x%08lX,%p,%ld): stub\n", iface, ckid, lpData, size);
  /* FIXME: write data to file */
  return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnReadData(IAVIFile *iface, DWORD ckid,
					  LPVOID lpData, LONG *size)
{
  FIXME("(%p,0x%08lX,%p,%p): stub\n", iface, ckid, lpData, size);
  /* FIXME: read at most size bytes from file */
  return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnEndRecord(IAVIFile *iface)
{
  FIXME("(%p): stub\n",iface);
  /* FIXME: end record -- for interleaved files */
  return E_FAIL;
}

static HRESULT WINAPI IAVIFile_fnDeleteStream(IAVIFile *iface, DWORD fccType,
					      LONG lParam)
{
  FIXME("(%p,0x%08lX,%ld): stub\n", iface, fccType, lParam);
  /* FIXME: delete stream */
  return E_FAIL;
}

/***********************************************************************/

static HRESULT WINAPI IPersistFile_fnQueryInterface(IPersistFile *iface,
						    REFIID refiid, LPVOID *obj)
{
  ICOM_THIS(IPersistFileImpl,iface);

  assert(This->paf != NULL);

  return IAVIFile_QueryInterface((PAVIFILE)This->paf, refiid, obj);
}

static ULONG   WINAPI IPersistFile_fnAddRef(IPersistFile *iface)
{
  ICOM_THIS(IPersistFileImpl,iface);

  assert(This->paf != NULL);

  return IAVIFile_AddRef((PAVIFILE)This->paf);
}

static ULONG   WINAPI IPersistFile_fnRelease(IPersistFile *iface)
{
  ICOM_THIS(IPersistFileImpl,iface);

  assert(This->paf != NULL);

  return IAVIFile_Release((PAVIFILE)This->paf);
}

static HRESULT WINAPI IPersistFile_fnGetClassID(IPersistFile *iface,
						LPCLSID pClassID)
{
  TRACE("(%p,%p)\n", iface, pClassID);

  if (pClassID == NULL)
    return AVIERR_BADPARAM;

  memcpy(pClassID, &CLSID_AVIFile, sizeof(CLSID_AVIFile));

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnIsDirty(IPersistFile *iface)
{
  ICOM_THIS(IPersistFileImpl,iface);

  TRACE("(%p)\n", iface);

  assert(This->paf != NULL);

  return (This->paf->fDirty ? S_OK : S_FALSE);
}

static HRESULT WINAPI IPersistFile_fnLoad(IPersistFile *iface,
					  LPCOLESTR pszFileName, DWORD dwMode)
{
  ICOM_THIS(IPersistFileImpl,iface);

  FIXME("(%p,%s,0x%08lX): stub\n", iface, debugstr_w(pszFileName), dwMode);

  assert(This->paf != NULL);

  return AVIERR_ERROR;
}

static HRESULT WINAPI IPersistFile_fnSave(IPersistFile *iface,
					  LPCOLESTR pszFileName,BOOL fRemember)
{
  TRACE("(%p,%s,%d)\n", iface, debugstr_w(pszFileName), fRemember);

  /* We write directly to disk, so nothing to do. */

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnSaveCompleted(IPersistFile *iface,
						   LPCOLESTR pszFileName)
{
  TRACE("(%p,%s)\n", iface, debugstr_w(pszFileName));

  /* We write directly to disk, so nothing to do. */

  return AVIERR_OK;
}

static HRESULT WINAPI IPersistFile_fnGetCurFile(IPersistFile *iface,
						LPOLESTR *ppszFileName)
{
  ICOM_THIS(IPersistFileImpl,iface);

  TRACE("(%p,%p)\n", iface, ppszFileName);

  if (ppszFileName == NULL)
    return AVIERR_BADPARAM;

  *ppszFileName = NULL;

  assert(This->paf != NULL);

  if (This->paf->szFileName != NULL) {
    int len = lstrlenW(This->paf->szFileName);

    *ppszFileName = (LPOLESTR)GlobalAllocPtr(GHND, len * sizeof(WCHAR));
    if (*ppszFileName == NULL)
      return AVIERR_MEMORY;

    memcpy(*ppszFileName, This->paf->szFileName, len * sizeof(WCHAR));
  }

  return AVIERR_OK;
}

/***********************************************************************/

static HRESULT WINAPI IAVIStream_fnQueryInterface(IAVIStream *iface,
						  REFIID refiid, LPVOID *obj)
{
  ICOM_THIS(IAVIStreamImpl,iface);

  TRACE("(%p,%s,%p)\n", iface, debugstr_guid(refiid), obj);

  if (IsEqualGUID(&IID_IUnknown, refiid) ||
      IsEqualGUID(&IID_IAVIStream, refiid)) {
    *obj = This;
    return S_OK;
  }
  /* FIXME: IAVIStreaming interface */

  return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IAVIStream_fnAddRef(IAVIStream *iface)
{
  ICOM_THIS(IAVIStreamImpl,iface);

  TRACE("(%p)\n",iface);
  return ++(This->ref);
}

static ULONG WINAPI IAVIStream_fnRelease(IAVIStream* iface)
{
  ICOM_THIS(IAVIStreamImpl,iface);

  TRACE("(%p)\n",iface);

  /* we belong to the AVIFile, which must free us! */
  if (This->ref == 0) {
    ERR(": already released!\n");
    return 0;
  }

  return --This->ref;
}

static HRESULT WINAPI IAVIStream_fnCreate(IAVIStream *iface, LPARAM lParam1,
					  LPARAM lParam2)
{
  TRACE("(%p,0x%08lX,0x%08lX)\n", iface, lParam1, lParam2);

  /* This IAVIStream interface needs an AVIFile */
  return AVIERR_UNSUPPORTED;
}

static HRESULT WINAPI IAVIStream_fnInfo(IAVIStream *iface,LPAVISTREAMINFOW psi,
					LONG size)
{
  ICOM_THIS(IAVIStreamImpl,iface);

  TRACE("(%p,%p,%ld)\n", iface, psi, size);

  if (psi == NULL)
    return AVIERR_BADPARAM;
  if (size < 0)
    return AVIERR_BADSIZE;

  memcpy(psi, &This->sInfo, min(size, sizeof(This->sInfo)));

  if (size < sizeof(This->sInfo))
    return AVIERR_BUFFERTOOSMALL;
  return AVIERR_OK;
}

static LONG WINAPI IAVIStream_fnFindSample(IAVIStream *iface, LONG pos,
					   LONG flags)
{
  FIXME("(%p,%ld,0x%08lX): stub\n",iface,pos,flags);

  return -1;
}

static HRESULT WINAPI IAVIStream_fnReadFormat(IAVIStream *iface, LONG pos,
					      LPVOID format, LONG *formatsize)
{
  FIXME("(%p,%ld,%p,%p): stub\n", iface, pos, format, formatsize);

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnSetFormat(IAVIStream *iface, LONG pos,
					     LPVOID format, LONG formatsize)
{
/*  ICOM_THIS(IAVIStreamImpl,iface); */

  FIXME("(%p,%ld,%p,%ld): stub\n", iface, pos, format, formatsize);

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnRead(IAVIStream *iface, LONG start,
					LONG samples, LPVOID buffer,
					LONG buffersize, LPLONG bytesread,
					LPLONG samplesread)
{
/*  ICOM_THIS(IAVIStreamImpl,iface); */

  FIXME("(%p,%ld,%ld,%p,%ld,%p,%p): stub\n", iface, start, samples, buffer,
	buffersize, bytesread, samplesread);

  if (bytesread != NULL)
    *bytesread = 0;
  if (samplesread != NULL)
    *samplesread = 0;

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnWrite(IAVIStream *iface, LONG start,
					 LONG samples, LPVOID buffer,
					 LONG buffersize, DWORD flags,
					 LPLONG sampwritten,
					 LPLONG byteswritten)
{
/*  ICOM_THIS(IAVIStreamImpl,iface); */

  FIXME("(%p,%ld,%ld,%p,%ld,0x%08lX,%p,%p): stub\n", iface, start, samples,
	buffer, buffersize, flags, sampwritten, byteswritten);

  if (sampwritten != NULL)
    *sampwritten = 0;
  if (byteswritten != NULL)
    *byteswritten = 0;

  if (buffer == NULL && buffersize > 0)
    return AVIERR_BADPARAM;

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnDelete(IAVIStream *iface, LONG start,
					  LONG samples)
{
  FIXME("(%p,%ld,%ld): stub\n", iface, start, samples);

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnReadData(IAVIStream *iface, DWORD fcc,
					    LPVOID lp, LPLONG lpread)
{
  FIXME("(%p,0x%08lX,%p,%p): stub\n", iface, fcc, lp, lpread);

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnWriteData(IAVIStream *iface, DWORD fcc,
					     LPVOID lp, LONG size)
{
  FIXME("(%p,0x%08lx,%p,%ld): stub\n", iface, fcc, lp, size);

  return E_FAIL;
}

static HRESULT WINAPI IAVIStream_fnSetInfo(IAVIStream *iface,
					   LPAVISTREAMINFOW info, LONG infolen)
{
  FIXME("(%p,%p,%ld): stub\n", iface, info, infolen);

  return E_FAIL;
}
