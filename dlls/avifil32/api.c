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
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "shellapi.h"
#include "vfw.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************
 * copied from dlls/shell32/undocshell.h
 */
HRESULT WINAPI SHCoCreateInstance(LPCSTR lpszClsid,REFCLSID rClsid,
				  LPUNKNOWN pUnkOuter,REFIID riid,LPVOID *ppv);

/***********************************************************************
 * copied from dlls/ole32/compobj.c
 */
static HRESULT AVIFILE_CLSIDFromString(LPCSTR idstr, LPCLSID id)
{
  BYTE *s = (BYTE*)idstr;
  BYTE *p;
  INT   i;
  BYTE table[256];

  if (!s) {
    memset(s, 0, sizeof(CLSID));
    return S_OK;
  } else {  /* validate the CLSID string */
    if (lstrlenA(s) != 38)
      return CO_E_CLASSSTRING;

    if ((s[0]!='{') || (s[9]!='-') || (s[14]!='-') || (s[19]!='-') ||
	(s[24]!='-') || (s[37]!='}'))
      return CO_E_CLASSSTRING;

    for (i = 1; i < 37; i++) {
      if ((i == 9) || (i == 14) || (i == 19) || (i == 24))
	continue;
      if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
	    ((s[i] >= 'a') && (s[i] <= 'f'))  ||
	    ((s[i] >= 'A') && (s[i] <= 'F')))
	  )
	return CO_E_CLASSSTRING;
    }
  }

  TRACE("%s -> %p\n", s, id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++)
    table['0' + i] = i;

  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */
  p = (BYTE *) id;

  s++;	/* skip leading brace  */
  for (i = 0; i < 4; i++) {
    p[3 - i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 4;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  /* these are just sequential bytes */
  for (i = 0; i < 2; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  s++;	/* skip - */

  for (i = 0; i < 6; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }

  return S_OK;
}

/***********************************************************************
 *		AVIFileInit		(AVIFIL32.@)
 *		AVIFileInit		(AVIFILE.100)
 */
void WINAPI AVIFileInit(void) {
  /* need to load ole32.dll if not already done and get some functions */
  FIXME("(): stub!\n");
}

/***********************************************************************
 *		AVIFileExit		(AVIFIL32.@)
 *		AVIFileExit		(AVIFILE.101)
 */
void WINAPI AVIFileExit(void) {
  /* need to free ole32.dll if we are the last exit call */
  FIXME("(): stub!\n");
}

/***********************************************************************
 *		AVIFileOpenA		(AVIFIL32.@)
 *		AVIFileOpenA		(AVIFILE.102)
 */
HRESULT WINAPI AVIFileOpenA(PAVIFILE *ppfile, LPCSTR szFile, UINT uMode,
			    LPCLSID lpHandler)
{
  LPWSTR  wszFile = NULL;
  HRESULT hr;
  int     len;

  TRACE("(%p,%s,0x%08X,%s)\n", ppfile, debugstr_a(szFile), uMode,
	debugstr_guid(lpHandler));

  /* check parameters */
  if (ppfile == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  /* convert ASCII string to Unicode and call unicode function */
  len = lstrlenA(szFile);
  if (len <= 0)
    return AVIERR_BADPARAM;

  wszFile = (LPWSTR)LocalAlloc(LPTR, (len + 1) * sizeof(WCHAR));
  if (wszFile == NULL)
    return AVIERR_MEMORY;

  MultiByteToWideChar(CP_ACP, 0, szFile, -1, wszFile, len + 1);
  wszFile[len + 1] = 0;

  hr = AVIFileOpenW(ppfile, wszFile, uMode, lpHandler);

  LocalFree((HLOCAL)wszFile);

  return hr;
}

/***********************************************************************
 *		AVIFileOpenW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileOpenW(PAVIFILE *ppfile, LPCWSTR szFile, UINT uMode,
			    LPCLSID lpHandler)
{
  IPersistFile *ppersist = NULL;
  CLSID         clsidHandler;
  HRESULT       hr;

  FIXME("(%p,%s,0x%X,%s): stub!\n", ppfile, debugstr_w(szFile), uMode,
	debugstr_guid(lpHandler));

  /* check parameters */
  if (ppfile == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppfile = NULL;

  /* if no handler then try guessing it by extension */
  if (lpHandler == NULL) {
    FIXME(": must read HKEY_CLASSES_ROOT\\AVIFile\\Extensions\\%s\n", debugstr_w(strrchrW(szFile, L'.')));
  } else
    memcpy(&clsidHandler, lpHandler, sizeof(clsidHandler));

  /* crete instance of handler */
  hr = SHCoCreateInstance(NULL, &clsidHandler, NULL,
			  &IID_IAVIFile, (LPVOID*)ppfile);
  if (FAILED(hr) || *ppfile == NULL)
    return hr;

  /* ask for IPersistFile interface for loading/creating the file */
  hr = IAVIFile_QueryInterface(*ppfile, &IID_IPersistFile, (LPVOID*)&ppersist);
  if (FAILED(hr) || ppersist == NULL) {
    IAVIFile_Release(*ppfile);
    *ppfile = NULL;
    return hr;
  }

  hr = IPersistFile_Load(ppersist, szFile, uMode);
  IPersistFile_Release(ppersist);
  if (FAILED(hr)) {
    IAVIFile_Release(*ppfile);
    *ppfile = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIFileAddRef		(AVIFIL32.@)
 *		AVIFileAddRef		(AVIFILE.140)
 */
ULONG WINAPI AVIFileAddRef(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIFile_AddRef(pfile);
}

/***********************************************************************
 *		AVIFileRelease		(AVIFIL32.@)
 *		AVIFileRelease		(AVIFILE.141)
 */
ULONG WINAPI AVIFileRelease(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIFile_Release(pfile);
}

/***********************************************************************
 *		AVIFileInfoA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoA(PAVIFILE pfile, LPAVIFILEINFOA afi, LONG size)
{
  AVIFILEINFOW afiw;
  HRESULT      hres;

  TRACE("(%p,%p,%ld)\n", pfile, afi, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;
  if (size < sizeof(AVIFILEINFOA))
    return AVIERR_BADSIZE;

  hres = IAVIFile_Info(pfile, &afiw, sizeof(afiw));

  memcpy(afi, &afiw, sizeof(*afi) - sizeof(afi->szFileType));
  WideCharToMultiByte(CP_ACP, 0, afiw.szFileType, -1, afi->szFileType,
		      sizeof(afi->szFileType), NULL, NULL);
  afi->szFileType[sizeof(afi->szFileType) - 1] = 0;

  return hres;
}

/***********************************************************************
 *		AVIFileInfoW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileInfoW(PAVIFILE pfile, LPAVIFILEINFOW afiw, LONG size)
{
  TRACE("(%p,%p,%ld)\n", pfile, afiw, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_Info(pfile, afiw, size);
}

/***********************************************************************
 *		AVIFileGetStream	(AVIFIL32.@)
 *		AVIFileGetStream	(AVIFILE.143)
 */
HRESULT WINAPI AVIFileGetStream(PAVIFILE pfile, PAVISTREAM *avis,
				DWORD fccType, LONG lParam)
{
  TRACE("(%p,%p,'%4.4s',%ld)\n", pfile, avis, (char*)&fccType, lParam);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_GetStream(pfile, avis, fccType, lParam);
}

/***********************************************************************
 *		AVIFileCreateStreamA	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamA(PAVIFILE pfile, PAVISTREAM *ppavi,
				    LPAVISTREAMINFOA psi)
{
  AVISTREAMINFOW	psiw;

  TRACE("(%p,%p,%p)\n", pfile, ppavi, psi);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  /* Only the szName at the end is different */
  memcpy(&psiw, psi, sizeof(*psi) - sizeof(psi->szName));
  MultiByteToWideChar(CP_ACP, 0, psi->szName, -1, psiw.szName,
		      sizeof(psiw.szName) / sizeof(psiw.szName[0]));

  return IAVIFile_CreateStream(pfile, ppavi, &psiw);
}

/***********************************************************************
 *		AVIFileCreateStreamW	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileCreateStreamW(PAVIFILE pfile, PAVISTREAM *avis,
				    LPAVISTREAMINFOW asi)
{
  TRACE("(%p,%p,%p)\n", pfile, avis, asi);

  return IAVIFile_CreateStream(pfile, avis, asi);
}

/***********************************************************************
 *		AVIFileWriteData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileWriteData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LONG size)
{
  TRACE("(%p,'%4.4s',%p,%ld)\n", pfile, (char*)&fcc, lp, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_WriteData(pfile, fcc, lp, size);
}

/***********************************************************************
 *		AVIFileReadData		(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileReadData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LPLONG size)
{
  TRACE("(%p,'%4.4s',%p,%p)\n", pfile, (char*)&fcc, lp, size);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_ReadData(pfile, fcc, lp, size);
}

/***********************************************************************
 *		AVIFileEndRecord	(AVIFIL32.@)
 */
HRESULT WINAPI AVIFileEndRecord(PAVIFILE pfile)
{
  TRACE("(%p)\n", pfile);

  if (pfile == NULL)
    return AVIERR_BADHANDLE;

  return IAVIFile_EndRecord(pfile);
}

/***********************************************************************
 *		AVIStreamAddRef		(AVIFIL32.@)
 */
ULONG WINAPI AVIStreamAddRef(PAVISTREAM pstream)
{
  TRACE("(%p)\n", pstream);

  if (pstream == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIStream_AddRef(pstream);
}

/***********************************************************************
 *		AVIStreamRelease	(AVIFIL32.@)
 */
ULONG WINAPI AVIStreamRelease(PAVISTREAM pstream)
{
  TRACE("(%p)\n", pstream);

  if (pstream == NULL) {
    ERR(": bad handle passed!\n");
    return 0;
  }

  return IAVIStream_Release(pstream);
}

HRESULT WINAPI AVIStreamCreate(PAVISTREAM *ppavi, LONG lParam1, LONG lParam2,
			       LPCLSID pclsidHandler)
{
  HRESULT hr;

  TRACE("(%p,0x%08lX,0x%08lX,%s)\n", ppavi, lParam1, lParam2,
	debugstr_guid(pclsidHandler));

  if (ppavi == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;
  if (pclsidHandler == NULL)
    return AVIERR_UNSUPPORTED;

  hr = SHCoCreateInstance(NULL, pclsidHandler, NULL,
			  &IID_IAVIStream, (LPVOID*)ppavi);
  if (FAILED(hr) || *ppavi == NULL)
    return hr;

  hr = IAVIStream_Create(*ppavi, lParam1, lParam2);
  if (FAILED(hr)) {
    IAVIStream_Release(*ppavi);
    *ppavi = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIStreamInfoA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoA(PAVISTREAM pstream, LPAVISTREAMINFOA asi,
			      LONG size)
{
  AVISTREAMINFOW asiw;
  HRESULT	 hres;

  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;
  if (size < sizeof(AVISTREAMINFOA))
    return AVIERR_BADSIZE;

  hres = IAVIStream_Info(pstream, &asiw, sizeof(asiw));

  memcpy(asi, &asiw, sizeof(asiw) - sizeof(asiw.szName));
  WideCharToMultiByte(CP_ACP, 0, asiw.szName, -1, asi->szName,
		      sizeof(asi->szName), NULL, NULL);
  asi->szName[sizeof(asi->szName) - 1] = 0;

  return hres;
}

/***********************************************************************
 *		AVIStreamInfoW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamInfoW(PAVISTREAM pstream, LPAVISTREAMINFOW asi,
			      LONG size)
{
  TRACE("(%p,%p,%ld)\n", pstream, asi, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Info(pstream, asi, size);
}

/***********************************************************************
 *		AVIStreamFindSample	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamFindSample(PAVISTREAM pstream, LONG pos, DWORD flags)
{
  TRACE("(%p,%ld,0x%lX)\n", pstream, pos, flags);

  if (pstream == NULL)
    return -1;

  return IAVIStream_FindSample(pstream, pos, flags);
}

/***********************************************************************
 *		AVIStreamReadFormat	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadFormat(PAVISTREAM pstream, LONG pos,
				   LPVOID format, LPLONG formatsize)
{
  TRACE("(%p,%ld,%p,%p)\n", pstream, pos, format, formatsize);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_ReadFormat(pstream, pos, format, formatsize);
}

/***********************************************************************
 *		AVIStreamSetFormat	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamSetFormat(PAVISTREAM pstream, LONG pos,
				  LPVOID format, LONG formatsize)
{
  TRACE("(%p,%ld,%p,%ld)\n", pstream, pos, format, formatsize);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_SetFormat(pstream, pos, format, formatsize);
}

/***********************************************************************
 *		AVIStreamRead		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamRead(PAVISTREAM pstream, LONG start, LONG samples,
			     LPVOID buffer, LONG buffersize,
			     LPLONG bytesread, LPLONG samplesread)
{
  TRACE("(%p,%ld,%ld,%p,%ld,%p,%p)\n", pstream, start, samples, buffer,
	buffersize, bytesread, samplesread);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Read(pstream, start, samples, buffer, buffersize,
			 bytesread, samplesread);
}

/***********************************************************************
 *		AVIStreamWrite		(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWrite(PAVISTREAM pstream, LONG start, LONG samples,
			      LPVOID buffer, LONG buffersize, DWORD flags,
			      LPLONG sampwritten, LPLONG byteswritten)
{
  TRACE("(%p,%ld,%ld,%p,%ld,0x%lX,%p,%p)\n", pstream, start, samples, buffer,
	buffersize, flags, sampwritten, byteswritten);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_Write(pstream, start, samples, buffer, buffersize,
			  flags, sampwritten, byteswritten);
}

/***********************************************************************
 *		AVIStreamReadData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamReadData(PAVISTREAM pstream, DWORD fcc, LPVOID lp,
				 LPLONG lpread)
{
  TRACE("(%p,'%4.4s',%p,%p)\n", pstream, (char*)&fcc, lp, lpread);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_ReadData(pstream, fcc, lp, lpread);
}

/***********************************************************************
 *		AVIStreamWriteData	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamWriteData(PAVISTREAM pstream, DWORD fcc, LPVOID lp,
				  LONG size)
{
  TRACE("(%p,'%4.4s',%p,%ld)\n", pstream, (char*)&fcc, lp, size);

  if (pstream == NULL)
    return AVIERR_BADHANDLE;

  return IAVIStream_WriteData(pstream, fcc, lp, size);
}

/***********************************************************************
 *		AVIStreamGetFrameOpen	(AVIFIL32.@)
 */
PGETFRAME WINAPI AVIStreamGetFrameOpen(PAVISTREAM pstream,
				       LPBITMAPINFOHEADER lpbiWanted)
{
  PGETFRAME pg = NULL;

  TRACE("(%p,%p)\n", pstream, lpbiWanted);

  if (FAILED(IAVIStream_QueryInterface(pstream, &IID_IGetFrame, (LPVOID*)&pg)) ||
      pg == NULL) {
    FIXME(": need internal class for IGetFrame!\n");
    return NULL;
  }

  if (FAILED(IGetFrame_SetFormat(pg, lpbiWanted, NULL, 0, 0, -1, -1))) {
    IGetFrame_Release(pg);
    return NULL;
  }

  return pg;
}

/***********************************************************************
 *		AVIStreamGetFrame	(AVIFIL32.@)
 */
LPVOID WINAPI AVIStreamGetFrame(PGETFRAME pg, LONG pos)
{
  TRACE("(%p,%ld)\n", pg, pos);

  if (pg == NULL)
    return NULL;

  return IGetFrame_GetFrame(pg, pos);
}

/***********************************************************************
 *		AVIStreamGetFrameClose (AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamGetFrameClose(PGETFRAME pg)
{
  TRACE("(%p)\n", pg);

  if (pg != NULL)
    return IGetFrame_Release(pg);
  return 0;
}

/***********************************************************************
 *		AVIMakeCompressedStream	(AVIFIL32.@)
 */
HRESULT WINAPI AVIMakeCompressedStream(PAVISTREAM *ppsCompressed,
				       PAVISTREAM psSource,
				       LPAVICOMPRESSOPTIONS aco,
				       LPCLSID pclsidHandler)
{
  AVISTREAMINFOW asiw;
  CHAR           szRegKey[25];
  CHAR           szValue[100];
  CLSID          clsidHandler;
  HRESULT        hr;
  LONG           size = sizeof(szValue);

  TRACE("(%p,%p,%p,%s)\n", ppsCompressed, psSource, aco,
	debugstr_guid(pclsidHandler));

  if (ppsCompressed == NULL)
    return AVIERR_BADPARAM;
  if (psSource == NULL)
    return AVIERR_BADHANDLE;

  *ppsCompressed = NULL;

  /* if no handler given get default ones based on streamtype */
  if (pclsidHandler == NULL) {
    hr = IAVIStream_Info(psSource, &asiw, sizeof(asiw));
    if (FAILED(hr))
      return hr;

    wsprintfA(szRegKey, "AVIFile\\Compressors\\%4.4s", (char*)&asiw.fccHandler);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, szRegKey, szValue, &size) != ERROR_SUCCESS)
      return AVIERR_UNSUPPORTED;
    if (AVIFILE_CLSIDFromString(szValue, &clsidHandler) != S_OK)
      return AVIERR_UNSUPPORTED;
  } else
    memcpy(&clsidHandler, pclsidHandler, sizeof(clsidHandler));

  hr = SHCoCreateInstance(NULL, &clsidHandler, NULL,
			  &IID_IAVIStream, (LPVOID*)ppsCompressed);
  if (FAILED(hr) || *ppsCompressed == NULL)
    return hr;

  hr = IAVIStream_Create(*ppsCompressed, (LPARAM)psSource, (LPARAM)aco);
  if (FAILED(hr)) {
    IAVIStream_Release(*ppsCompressed);
    *ppsCompressed = NULL;
  }

  return hr;
}

/***********************************************************************
 *		AVIStreamOpenFromFileA	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileA(PAVISTREAM *ppavi, LPCSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, LPCLSID pclsidHandler)
{
  PAVIFILE pfile = NULL;
  HRESULT  hr;

  TRACE("(%p,%s,'%4.4s',%ld,0x%X,%s)\n", ppavi, debugstr_a(szFile),
	(char*)&fccType, lParam, mode, debugstr_guid(pclsidHandler));

  if (ppavi == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;

  hr = AVIFileOpenA(&pfile, szFile, mode, pclsidHandler);
  if (FAILED(hr) || pfile == NULL)
    return hr;

  hr = IAVIFile_GetStream(pfile, ppavi, fccType, lParam);
  IAVIFile_Release(pfile);

  return hr;
}

/***********************************************************************
 *		AVIStreamOpenFromFileW	(AVIFIL32.@)
 */
HRESULT WINAPI AVIStreamOpenFromFileW(PAVISTREAM *ppavi, LPCWSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, LPCLSID pclsidHandler)
{
  PAVIFILE pfile = NULL;
  HRESULT  hr;

  TRACE("(%p,%s,'%4.4s',%ld,0x%X,%s)\n", ppavi, debugstr_w(szFile),
	(char*)&fccType, lParam, mode, debugstr_guid(pclsidHandler));

  if (ppavi == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppavi = NULL;

  hr = AVIFileOpenW(&pfile, szFile, mode, pclsidHandler);
  if (FAILED(hr) || pfile == NULL)
    return hr;

  hr = IAVIFile_GetStream(pfile, ppavi, fccType, lParam);
  IAVIFile_Release(pfile);

  return hr;
}

/***********************************************************************
 *		AVIStreamStart		(AVIFIL32.@)
 */
LONG WINAPI AVIStreamStart(PAVISTREAM pstream)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p)\n", pstream);

  if (pstream == NULL)
    return 0;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return 0;

  return asiw.dwLength;
}

/***********************************************************************
 *		AVIStreamLength		(AVIFIL32.@)
 */
LONG WINAPI AVIStreamLength(PAVISTREAM pstream)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p)\n", pstream);

  if (pstream == NULL)
    return 0;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return 0;

  return asiw.dwLength;
}

/***********************************************************************
 *		AVIStreamSampleToTime	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamSampleToTime(PAVISTREAM pstream, LONG lSample)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p,%ld)\n", pstream, lSample);

  if (pstream == NULL)
    return -1;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return -1;
  if (asiw.dwRate == 0)
    return -1;

  return (LONG)(((float)lSample * asiw.dwScale * 1000.0) / asiw.dwRate);
}

/***********************************************************************
 *		AVIStreamTimeToSample	(AVIFIL32.@)
 */
LONG WINAPI AVIStreamTimeToSample(PAVISTREAM pstream, LONG lTime)
{
  AVISTREAMINFOW asiw;

  TRACE("(%p,%ld)\n", pstream, lTime);

  if (pstream == NULL)
    return -1;

  if (FAILED(IAVIStream_Info(pstream, &asiw, sizeof(asiw))))
    return -1;
  if (asiw.dwScale == 0)
    return -1;

  return (LONG)(((float)lTime * asiw.dwRate) / asiw.dwScale / 1000.0);
}
