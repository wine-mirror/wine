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
#include "windowsx.h"

#include "ole2.h"
#include "shellapi.h"
#include "vfw.h"
#include "msacm.h"

#include "avifile_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(avifile);

/***********************************************************************
 * copied from dlls/shell32/undocshell.h
 */
HRESULT WINAPI SHCoCreateInstance(LPCSTR lpszClsid,REFCLSID rClsid,
				  LPUNKNOWN pUnkOuter,REFIID riid,LPVOID *ppv);

/***********************************************************************
 * for AVIBuildFilterW -- uses fixed size table
 */
#define MAX_FILTERS 30 /* 30 => 7kB */

typedef struct _AVIFilter {
  WCHAR szClsid[40];
  WCHAR szExtensions[MAX_FILTERS * 7];
} AVIFilter;

/***********************************************************************
 * for AVISaveOptions
 */
static struct {
  UINT                  uFlags;
  INT                   nStreams;
  PAVISTREAM           *ppavis;
  LPAVICOMPRESSOPTIONS *ppOptions;
  INT                   nCurrent;
} SaveOpts;

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

static BOOL AVIFILE_GetFileHandlerByExtension(LPCWSTR szFile, LPCLSID lpclsid)
{
  CHAR   szRegKey[25];
  CHAR   szValue[100];
  LPWSTR szExt = strrchrW(szFile, '.');
  LONG   len = sizeof(szValue) / sizeof(szValue[0]);

  if (szExt == NULL)
    return FALSE;

  szExt++;

  wsprintfA(szRegKey, "AVIFile\\Extensions\\%.3ls", szExt);
  if (RegQueryValueA(HKEY_CLASSES_ROOT, szRegKey, szValue, &len) != ERROR_SUCCESS)
    return FALSE;

  return (AVIFILE_CLSIDFromString(szValue, lpclsid) == S_OK);
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
 *		AVIFileOpen		(AVIFILE.102)
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

  TRACE("(%p,%s,0x%X,%s)\n", ppfile, debugstr_w(szFile), uMode,
	debugstr_guid(lpHandler));

  /* check parameters */
  if (ppfile == NULL || szFile == NULL)
    return AVIERR_BADPARAM;

  *ppfile = NULL;

  /* if no handler then try guessing it by extension */
  if (lpHandler == NULL) {
    if (! AVIFILE_GetFileHandlerByExtension(szFile, &clsidHandler))
      return AVIERR_UNSUPPORTED;
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
 *		AVIFileInfo		(AVIFIL32.@)
 *		AVIFileInfoA		(AVIFIL32.@)
 *		AVIFileInfo		(AVIFILE.142)
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
 *		AVIFileEndRecord	(AVIFILE.148)
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
 *		AVIStreamAddRef		(AVIFILE.160)
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
 *		AVIStreamRelease	(AVIFILE.161)
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

/***********************************************************************
 *		AVIStreamCreate		(AVIFIL32.@)
 *		AVIStreamCreate		(AVIFILE.104)
 */
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
    pg = AVIFILE_CreateGetFrame(pstream);
    if (pg == NULL)
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

    wsprintfA(szRegKey, "AVIFile\\Compressors\\%4.4s", (char*)&asiw.fccType);
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
 *		AVIStreamOpenFromFile   (AVIFILE.103)
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
 *		AVIStreamStart		(AVIFILE.130)
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

  return asiw.dwStart;
}

/***********************************************************************
 *		AVIStreamLength		(AVIFILE.131)
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
 *		AVIStreamSampleToTime	(AVIFILE.133)
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
 *		AVIStreamTimeToSample	(AVIFILE.132)
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

/***********************************************************************
 *		AVIBuildFilterA		(AVIFIL32.@)
 */
HRESULT WINAPI AVIBuildFilterA(LPSTR szFilter, LONG cbFilter, BOOL fSaving)
{
  LPWSTR  wszFilter;
  HRESULT hr;

  TRACE("(%p,%ld,%d)\n", szFilter, cbFilter, fSaving);

  /* check parameters */
  if (szFilter == NULL)
    return AVIERR_BADPARAM;
  if (cbFilter < 2)
    return AVIERR_BADSIZE;

  szFilter[0] = 0;
  szFilter[1] = 0;

  wszFilter = (LPWSTR)GlobalAllocPtr(GHND, cbFilter);
  if (wszFilter == NULL)
    return AVIERR_MEMORY;

  hr = AVIBuildFilterW(wszFilter, cbFilter, fSaving);
  if (SUCCEEDED(hr)) {
    WideCharToMultiByte(CP_ACP, 0, wszFilter, cbFilter,
			szFilter, cbFilter, NULL, NULL);
  }

  GlobalFreePtr(wszFilter);

  return hr;
}

/***********************************************************************
 *		AVIBuildFilterW		(AVIFIL32.@)
 */
HRESULT WINAPI AVIBuildFilterW(LPWSTR szFilter, LONG cbFilter, BOOL fSaving)
{
  static const WCHAR szClsid[] = {'C','L','S','I','D',0};
  static const WCHAR szExtensionFmt[] = {';','*','.','%','s',0};
  static const WCHAR szAVIFileExtensions[] =
    {'A','V','I','F','i','l','e','\\','E','x','t','e','n','s','i','o','n','s',0};

  AVIFilter *lp;
  WCHAR      szAllFiles[40];
  WCHAR      szFileExt[10];
  WCHAR      szValue[128];
  HKEY       hKey;
  LONG       n, i;
  LONG       size;
  LONG       count = 0;

  TRACE("(%p,%ld,%d)\n", szFilter, cbFilter, fSaving);

  /* check parameters */
  if (szFilter == NULL)
    return AVIERR_BADPARAM;
  if (cbFilter < 2)
    return AVIERR_BADSIZE;

  lp = (AVIFilter*)GlobalAllocPtr(GHND, MAX_FILTERS * sizeof(AVIFilter));
  if (lp == NULL)
    return AVIERR_MEMORY;

  /*
   * 1. iterate over HKEY_CLASSES_ROOT\\AVIFile\\Extensions and collect
   *    extensions and CLSID's
   * 2. iterate over collected CLSID's and copy it's description and it's
   *    extensions to szFilter if it fits
   *
   * First filter is named "All multimedia files" and it's filter is a
   * collection of all possible extensions except "*.*".
   */
  if (RegOpenKeyW(HKEY_CLASSES_ROOT, szAVIFileExtensions, &hKey) != S_OK) {
    GlobalFreePtr(lp);
    return AVIERR_ERROR;
  }
  for (n = 0;RegEnumKeyW(hKey, n, szFileExt, sizeof(szFileExt)) == S_OK;n++) {
    /* get CLSID to extension */
    size = sizeof(szValue)/sizeof(szValue[0]);
    if (RegQueryValueW(hKey, szFileExt, szValue, &size) != S_OK)
      break;

    /* search if the CLSID is already known */
    for (i = 1; i <= count; i++) {
      if (lstrcmpW(lp[i].szClsid, szValue) == 0)
	break; /* a new one */
    }

    if (count - i == -1) {
      /* it's a new CLSID */

      /* FIXME: How do we get info's about read/write capabilities? */

      if (count >= MAX_FILTERS) {
	/* try to inform user of our full fixed size table */
	ERR(": More than %d filters found! Adjust MAX_FILTERS in dlls/avifil32/api.c\n", MAX_FILTERS);
	break;
      }

      lstrcpyW(lp[i].szClsid, szValue);

      count++;
    }

    /* append extension to the filter */
    wsprintfW(szValue, szExtensionFmt, szFileExt);
    if (lp[i].szExtensions[0] == 0)
      lstrcatW(lp[i].szExtensions, szValue + 1);
    else
      lstrcatW(lp[i].szExtensions, szValue);

    /* also append to the "all multimedia"-filter */
    if (lp[0].szExtensions[0] == 0)
      lstrcatW(lp[0].szExtensions, szValue + 1);
    else
      lstrcatW(lp[0].szExtensions, szValue);
  }
  RegCloseKey(hKey);

  /* 2. get descriptions for the CLSIDs and fill out szFilter */
  if (RegOpenKeyW(HKEY_CLASSES_ROOT, szClsid, &hKey) != S_OK) {
    GlobalFreePtr(lp);
    return AVIERR_ERROR;
  }
  for (n = 0; n <= count; n++) {
    /* first the description */
    if (n != 0) {
      size = sizeof(szValue)/sizeof(szValue[0]);
      if (RegQueryValueW(hKey, lp[n].szClsid, szValue, &size) == S_OK) {
	size = lstrlenW(szValue);
	lstrcpynW(szFilter, szValue, cbFilter);
      }
    } else
      size = LoadStringW(AVIFILE_hModule,IDS_ALLMULTIMEDIA,szFilter,cbFilter);

    /* check for enough space */
    size++;
    if (cbFilter < size + lstrlenW(lp[n].szExtensions) + 2) {
      szFilter[0] = 0;
      szFilter[1] = 0;
      GlobalFreePtr(lp);
      RegCloseKey(hKey);
      return AVIERR_BUFFERTOOSMALL;
    }
    cbFilter -= size;
    szFilter += size;

    /* and then the filter */
    lstrcpynW(szFilter, lp[n].szExtensions, cbFilter);
    size = lstrlenW(lp[n].szExtensions) + 1;
    cbFilter -= size;
    szFilter += size;
  }

  RegCloseKey(hKey);
  GlobalFreePtr(lp);

  /* add "All files" "*.*" filter if enough space left */
  size = LoadStringW(AVIFILE_hModule, IDS_ALLFILES,
		     szAllFiles, sizeof(szAllFiles)) + 1;
  if (cbFilter > size) {
    int i;

    /* replace '@' with \000 to seperate description of filter */
    for (i = 0; i < size && szAllFiles[i] != 0; i++) {
      if (szAllFiles[i] == '@') {
	szAllFiles[i] = 0;
	break;
      }
    }
      
    memcpy(szFilter, szAllFiles, size * sizeof(szAllFiles[0]));
    szFilter += size;
    szFilter[0] = 0;

    return AVIERR_OK;
  } else {
    szFilter[0] = 0;
    return AVIERR_BUFFERTOOSMALL;
  }
}

static BOOL AVISaveOptionsFmtChoose(HWND hWnd)
{
  LPAVICOMPRESSOPTIONS pOptions = SaveOpts.ppOptions[SaveOpts.nCurrent];
  AVISTREAMINFOW       sInfo;

  TRACE("(%p)\n", hWnd);

  if (pOptions == NULL || SaveOpts.ppavis[SaveOpts.nCurrent] == NULL) {
    ERR(": bad state!\n");
    return FALSE;
  }

  if (FAILED(AVIStreamInfoW(SaveOpts.ppavis[SaveOpts.nCurrent],
			    &sInfo, sizeof(sInfo)))) {
    ERR(": AVIStreamInfoW failed!\n");
    return FALSE;
  }

  if (sInfo.fccType == streamtypeVIDEO) {
    COMPVARS cv;
    BOOL     ret;

    memset(&cv, 0, sizeof(cv));

    if ((pOptions->dwFlags & AVICOMPRESSF_VALID) == 0) {
      memset(pOptions, 0, sizeof(AVICOMPRESSOPTIONS));
      pOptions->fccType    = streamtypeVIDEO;
      pOptions->fccHandler = comptypeDIB;
      pOptions->dwQuality  = ICQUALITY_DEFAULT;
    }

    cv.cbSize     = sizeof(cv);
    cv.dwFlags    = ICMF_COMPVARS_VALID;
    /*cv.fccType    = pOptions->fccType; */
    cv.fccHandler = pOptions->fccHandler;
    cv.lQ         = pOptions->dwQuality;
    cv.lpState    = pOptions->lpParms;
    cv.cbState    = pOptions->cbParms;
    if (pOptions->dwFlags & AVICOMPRESSF_KEYFRAMES)
      cv.lKey = pOptions->dwKeyFrameEvery;
    else
      cv.lKey = 0;
    if (pOptions->dwFlags & AVICOMPRESSF_DATARATE)
      cv.lDataRate = pOptions->dwBytesPerSecond / 1024; /* need kBytes */
    else
      cv.lDataRate = 0;

    ret = ICCompressorChoose(hWnd, SaveOpts.uFlags, NULL,
			     SaveOpts.ppavis[SaveOpts.nCurrent], &cv, NULL);

    if (ret) {
      pOptions->lpParms   = cv.lpState;
      pOptions->cbParms   = cv.cbState;
      pOptions->dwQuality = cv.lQ;
      if (cv.lKey != 0) {
	pOptions->dwKeyFrameEvery = cv.lKey;
	pOptions->dwFlags |= AVICOMPRESSF_KEYFRAMES;
      } else
	pOptions->dwFlags &= ~AVICOMPRESSF_KEYFRAMES;
      if (cv.lDataRate != 0) {
	pOptions->dwBytesPerSecond = cv.lDataRate * 1024; /* need bytes */
	pOptions->dwFlags |= AVICOMPRESSF_DATARATE;
      } else
	pOptions->dwFlags &= ~AVICOMPRESSF_DATARATE;
      pOptions->dwFlags  |= AVICOMPRESSF_VALID;
    }
    ICCompressorFree(&cv);

    return ret;
  } else if (sInfo.fccType == streamtypeAUDIO) {
    ACMFORMATCHOOSEW afmtc;
    MMRESULT         ret;
    LONG             size;

    /* FIXME: check ACM version -- Which version is needed? */

    memset(&afmtc, 0, sizeof(afmtc));
    afmtc.cbStruct  = sizeof(afmtc);
    afmtc.fdwStyle  = 0;
    afmtc.hwndOwner = hWnd;

    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &size);
    if ((pOptions->cbFormat == 0 || pOptions->lpFormat == NULL) && size != 0) {
      pOptions->lpFormat = GlobalAllocPtr(GMEM_MOVEABLE, size);
      pOptions->cbFormat = size;
    } else if (pOptions->cbFormat < size) {
      pOptions->lpFormat = GlobalReAllocPtr(pOptions->lpFormat, size, GMEM_MOVEABLE);
      pOptions->cbFormat = size;
    }
    if (pOptions->lpFormat == NULL)
      return FALSE;
    afmtc.pwfx  = pOptions->lpFormat;
    afmtc.cbwfx = pOptions->cbFormat;

    size = 0;
    AVIStreamFormatSize(SaveOpts.ppavis[SaveOpts.nCurrent],
			sInfo.dwStart, &size);
    if (size < sizeof(PCMWAVEFORMAT))
      size = sizeof(PCMWAVEFORMAT);
    afmtc.pwfxEnum = GlobalAllocPtr(GHND, size);
    if (afmtc.pwfxEnum != NULL) {
      AVIStreamReadFormat(SaveOpts.ppavis[SaveOpts.nCurrent],
			  sInfo.dwStart, afmtc.pwfxEnum, &size);
      afmtc.fdwEnum = ACM_FORMATENUMF_CONVERT;
    }

    ret = acmFormatChooseW(&afmtc);
    if (ret == S_OK)
      pOptions->dwFlags |= AVICOMPRESSF_VALID;

    if (afmtc.pwfxEnum != NULL)
      GlobalFreePtr(afmtc.pwfxEnum);

    return (ret == S_OK ? TRUE : FALSE);
  } else {
    ERR(": unknown streamtype 0x%08lX\n", sInfo.fccType);
    return FALSE;
  }
}

static void AVISaveOptionsUpdate(HWND hWnd)
{
  static const WCHAR szVideoFmt[]={'%','l','d','x','%','l','d','x','%','d',0};
  static const WCHAR szAudioFmt[]={'%','s',' ','%','s',0};

  WCHAR          szFormat[128];
  AVISTREAMINFOW sInfo;
  LPVOID         lpFormat;
  LONG           size;

  TRACE("(%p)\n", hWnd);

  SaveOpts.nCurrent = SendDlgItemMessageW(hWnd,IDC_STREAM,CB_GETCURSEL,0,0);
  if (SaveOpts.nCurrent < 0)
    return;

  if (FAILED(AVIStreamInfoW(SaveOpts.ppavis[SaveOpts.nCurrent], &sInfo, sizeof(sInfo))))
    return;

  AVIStreamFormatSize(SaveOpts.ppavis[SaveOpts.nCurrent],sInfo.dwStart,&size);
  if (size > 0) {
    szFormat[0] = 0;

    /* read format to build format descriotion string */
    lpFormat = GlobalAllocPtr(GHND, size);
    if (lpFormat != NULL) {
      if (SUCCEEDED(AVIStreamReadFormat(SaveOpts.ppavis[SaveOpts.nCurrent],sInfo.dwStart,lpFormat, &size))) {
	if (sInfo.fccType == streamtypeVIDEO) {
	  LPBITMAPINFOHEADER lpbi = lpFormat;
	  ICINFO icinfo;

	  wsprintfW(szFormat, szVideoFmt, lpbi->biWidth,
		    lpbi->biHeight, lpbi->biBitCount);

	  if (lpbi->biCompression != BI_RGB) {
	    HIC    hic;

	    hic = ICLocate(ICTYPE_VIDEO, sInfo.fccHandler, lpFormat,
			   NULL, ICMODE_DECOMPRESS);
	    if (hic != (HIC)NULL) {
	      if (ICGetInfo(hic, &icinfo, sizeof(icinfo)) == S_OK)
		lstrcatW(szFormat, icinfo.szDescription);
	      ICClose(hic);
	    }
	  } else {
	    LoadStringW(AVIFILE_hModule, IDS_UNCOMPRESSED,
			icinfo.szDescription, sizeof(icinfo.szDescription));
	    lstrcatW(szFormat, icinfo.szDescription);
	  }
	} else if (sInfo.fccType == streamtypeAUDIO) {
	  ACMFORMATTAGDETAILSW aftd;
	  ACMFORMATDETAILSW    afd;

	  memset(&aftd, 0, sizeof(aftd));
	  memset(&afd, 0, sizeof(afd));

	  aftd.cbStruct     = sizeof(aftd);
	  aftd.dwFormatTag  = afd.dwFormatTag =
	    ((PWAVEFORMATEX)lpFormat)->wFormatTag;
	  aftd.cbFormatSize = afd.cbwfx = size;

	  afd.cbStruct      = sizeof(afd);
	  afd.pwfx          = lpFormat;

	  if (acmFormatTagDetailsW((HACMDRIVER)NULL, &aftd,
				   ACM_FORMATTAGDETAILSF_FORMATTAG) == S_OK) {
	    if (acmFormatDetailsW(NULL,&afd,ACM_FORMATDETAILSF_FORMAT) == S_OK)
	      wsprintfW(szFormat, szAudioFmt, afd.szFormat, aftd.szFormatTag);
	  }
	}
      }
      GlobalFreePtr(lpFormat);
    }

    /* set text for format description */
    SetDlgItemTextW(hWnd, IDC_FORMATTEXT, szFormat);

    /* Disable option button for unsupported streamtypes */
    if (sInfo.fccType == streamtypeVIDEO ||
	sInfo.fccType == streamtypeAUDIO)
      EnableWindow(GetDlgItem(hWnd, IDC_OPTIONS), TRUE);
    else
      EnableWindow(GetDlgItem(hWnd, IDC_OPTIONS), FALSE);
  }

}

INT_PTR CALLBACK AVISaveOptionsDlgProc(HWND hWnd, UINT uMsg,
				    WPARAM wParam, LPARAM lParam)
{
  DWORD dwInterleave;
  BOOL  bIsInterleaved;
  INT   n;

  /*TRACE("(%p,%u,0x%04X,0x%08lX)\n", hWnd, uMsg, wParam, lParam);*/

  switch (uMsg) {
  case WM_INITDIALOG:
    SaveOpts.nCurrent = 0;
    if (SaveOpts.nStreams == 1) {
      EndDialog(hWnd, AVISaveOptionsFmtChoose(hWnd));
      return FALSE;
    }

    /* add streams */
    for (n = 0; n < SaveOpts.nStreams; n++) {
      AVISTREAMINFOW sInfo;

      AVIStreamInfoW(SaveOpts.ppavis[n], &sInfo, sizeof(sInfo));
      SendDlgItemMessageW(hWnd, IDC_STREAM, CB_ADDSTRING,
			  0L, (LPARAM)sInfo.szName);
    }

    /* select first stream */
    SendDlgItemMessageW(hWnd, IDC_STREAM, CB_SETCURSEL, 0, 0);
    SendMessageW(hWnd, WM_COMMAND,
		 GET_WM_COMMAND_MPS(IDC_STREAM, hWnd, CBN_SELCHANGE));

    /* initialize interleave */
    if (SaveOpts.ppOptions[0] != NULL &&
	(SaveOpts.ppOptions[0]->dwFlags & AVICOMPRESSF_VALID)) {
      bIsInterleaved = (SaveOpts.ppOptions[0]->dwFlags & AVICOMPRESSF_INTERLEAVE);
      dwInterleave = SaveOpts.ppOptions[0]->dwInterleaveEvery;
    } else {
      bIsInterleaved = TRUE;
      dwInterleave   = 0;
    }
    CheckDlgButton(hWnd, IDC_INTERLEAVE, bIsInterleaved);
    SetDlgItemInt(hWnd, IDC_INTERLEAVEEVERY, dwInterleave, FALSE);
    EnableWindow(GetDlgItem(hWnd, IDC_INTERLEAVEEVERY), bIsInterleaved);
    break;
  case WM_COMMAND:
    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
    case IDOK:
      /* get data from controls and save them */
      dwInterleave   = GetDlgItemInt(hWnd, IDC_INTERLEAVEEVERY, NULL, 0);
      bIsInterleaved = IsDlgButtonChecked(hWnd, IDC_INTERLEAVE);
      for (n = 0; n < SaveOpts.nStreams; n++) {
	if (SaveOpts.ppOptions[n] != NULL) {
	  if (bIsInterleaved) {
	    SaveOpts.ppOptions[n]->dwFlags |= AVICOMPRESSF_INTERLEAVE;
	    SaveOpts.ppOptions[n]->dwInterleaveEvery = dwInterleave;
	  } else
	    SaveOpts.ppOptions[n]->dwFlags &= ~AVICOMPRESSF_INTERLEAVE;
	}
      }
      /* fall through */
    case IDCANCEL:
      EndDialog(hWnd, GET_WM_COMMAND_CMD(wParam, lParam) == IDOK);
      break;
    case IDC_INTERLEAVE:
      EnableWindow(GetDlgItem(hWnd, IDC_INTERLEAVEEVERY),
		   IsDlgButtonChecked(hWnd, IDC_INTERLEAVE));
      break;
    case IDC_STREAM:
      if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
	/* update control elements */
	AVISaveOptionsUpdate(hWnd);
      }
      break;
    case IDC_OPTIONS:
      AVISaveOptionsFmtChoose(hWnd);
      break;
    };
    return FALSE;
  };

  return TRUE;
}

/***********************************************************************
 *		AVISaveOptions		(AVIFIL32.@)
 */
BOOL WINAPI AVISaveOptions(HWND hWnd, UINT uFlags, INT nStreams,
			   PAVISTREAM *ppavi, LPAVICOMPRESSOPTIONS *ppOptions)
{
  LPAVICOMPRESSOPTIONS pSavedOptions = NULL;
  INT ret, n;

  TRACE("(%p,0x%X,%d,%p,%p)\n", hWnd, uFlags, nStreams,
	ppavi, ppOptions);

  /* check parameters */
  if (nStreams <= 0 || ppavi == NULL || ppOptions == NULL)
    return AVIERR_BADPARAM;

  /* save options for case user press cancel */
  if (ppOptions != NULL && nStreams > 1) {
    pSavedOptions = GlobalAllocPtr(GHND,nStreams * sizeof(AVICOMPRESSOPTIONS));
    if (pSavedOptions == NULL)
      return FALSE;

    for (n = 0; n < nStreams; n++) {
      if (ppOptions[n] != NULL)
	memcpy(pSavedOptions + n, ppOptions[n], sizeof(AVICOMPRESSOPTIONS));
    }
  }

  SaveOpts.uFlags    = uFlags;
  SaveOpts.nStreams  = nStreams;
  SaveOpts.ppavis    = ppavi;
  SaveOpts.ppOptions = ppOptions;

  ret = DialogBoxW(AVIFILE_hModule, MAKEINTRESOURCEW(IDD_SAVEOPTIONS),
		   hWnd, AVISaveOptionsDlgProc);

  if (ret == -1)
    ret = FALSE;

  /* restore options when user pressed cancel */
  if (pSavedOptions != NULL && ret == FALSE) {
    for (n = 0; n < nStreams; n++) {
      if (ppOptions[n] != NULL)
	memcpy(ppOptions[n], pSavedOptions + n, sizeof(AVICOMPRESSOPTIONS));
    }
    GlobalFreePtr(pSavedOptions);
  }

  return (BOOL)ret;
}

/***********************************************************************
 *		AVISaveOptionsFree	(AVIFIL32.@)
 */
HRESULT WINAPI AVISaveOptionsFree(INT nStreams,LPAVICOMPRESSOPTIONS*ppOptions)
{
  TRACE("(%d,%p)\n", nStreams, ppOptions);

  if (nStreams < 0 || ppOptions == NULL)
    return AVIERR_BADPARAM;

  for (; nStreams > 0; nStreams--) {
    if (ppOptions[nStreams] != NULL) {
      ppOptions[nStreams]->dwFlags &= ~AVICOMPRESSF_VALID;

      if (ppOptions[nStreams]->lpParms != NULL) {
	GlobalFreePtr(ppOptions[nStreams]->lpParms);
	ppOptions[nStreams]->lpParms = NULL;
	ppOptions[nStreams]->cbParms = 0;
      }
      if (ppOptions[nStreams]->lpFormat != NULL) {
	GlobalFreePtr(ppOptions[nStreams]->lpFormat);
	ppOptions[nStreams]->lpFormat = NULL;
	ppOptions[nStreams]->cbFormat = 0;
      }
    }
  }

  return AVIERR_OK;
}
