/*
 * UrlMon
 *
 * Copyright (c) 2000 Patrik Stridvall
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

#include "windef.h"
#include "winerror.h"
#include "wtypes.h"
#include "ole2.h"
#include "urlmon.h"

#include "comimpl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);


const COMIMPL_CLASSENTRY COMIMPL_ClassList[] =
{
  /* list of exported classes */
  { NULL, NULL }
};


/***********************************************************************
 *		DllInstall (URLMON.@)
 */
HRESULT WINAPI URLMON_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
  FIXME("(%s, %s): stub\n", bInstall?"TRUE":"FALSE", 
	debugstr_w(cmdline));

  return S_OK;
}

/***********************************************************************
 *		DllRegisterServer (URLMON.@)
 */
HRESULT WINAPI URLMON_DllRegisterServer(void)
{
    FIXME("(void): stub\n");

    return S_OK;
}

/***********************************************************************
 *		DllRegisterServerEx (URLMON.@)
 */
HRESULT WINAPI URLMON_DllRegisterServerEx(void)
{
    FIXME("(void): stub\n");

    return E_FAIL;
}

/***********************************************************************
 *		DllUnregisterServer (URLMON.@)
 */
HRESULT WINAPI URLMON_DllUnregisterServer(void)
{
    FIXME("(void): stub\n");

    return S_OK;
}

/**************************************************************************
 *                 UrlMkSetSessionOption (URLMON.@)
 */
 HRESULT WINAPI UrlMkSetSessionOption(long lost, LPVOID *splat, long time,
 					long nosee)
{
    FIXME("(%#lx, %p, %#lx, %#lx): stub\n", lost, splat, time, nosee);
    
    return S_OK;
}

/**************************************************************************
 *                 CoInternetGetSession (URLMON.@)
 */
HRESULT WINAPI CoInternetGetSession(DWORD dwSessionMode,
                                    LPVOID /* IInternetSession ** */ ppIInternetSession,
                                    DWORD dwReserved)
{
    FIXME("(%ld, %p, %ld): stub\n", dwSessionMode, ppIInternetSession,
	  dwReserved);

    if(dwSessionMode) {
      ERR("dwSessionMode: %ld, must be zero\n", dwSessionMode);
    }

    if(dwReserved) {
      ERR("dwReserved: %ld, must be zero\n", dwReserved);
    }

    return E_NOTIMPL;
}


/**************************************************************************
 *                 ObtainUserAgentString (URLMON.@)
 */
HRESULT WINAPI ObtainUserAgentString(DWORD dwOption, LPCSTR pcszUAOut, DWORD *cbSize)
{
    FIXME("(%ld, %p, %p): stub\n", dwOption, pcszUAOut, cbSize);

    if(dwOption) {
      ERR("dwOption: %ld, must be zero\n", dwOption);
    }

    return E_NOTIMPL;
}

/*****************************************************************************
 * stubs
 */

typedef struct
{
	DWORD		dwStructSize;
	LPSTR		lpszLoggedUrlName;
	SYSTEMTIME	StartTime;
	SYSTEMTIME	EndTime;
	LPSTR		lpszExtendedInfo;
} HIT_LOGGING_INFO;


typedef struct
{
	ULONG	cbSize;
	DWORD	dwFlags;
	DWORD	dwAdState;
	LPWSTR	szTitle;
	LPWSTR	szAbstract;
	LPWSTR	szHREF;
	DWORD	dwInstalledVersionMS;
	DWORD	dwInstalledVersionLS;
	DWORD	dwUpdateVersionMS;
	DWORD	dwUpdateVersionLS;
	DWORD	dwAdvertisedVersionMS;
	DWORD	dwAdvertisedVersionLS;
	DWORD	dwReserved;
} SOFTDISTINFO;


HRESULT WINAPI CreateFormatEnumerator(UINT cFormatEtcs,FORMATETC* pFormatEtcs,IEnumFORMATETC** ppenum)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI FindMediaType(LPCSTR pszTypes,CLIPFORMAT* pcfTypes)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI FindMediaTypeClass(IBindCtx* pbc,LPCSTR pszType,CLSID* pclsid,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI FindMimeFromData(IBindCtx* pbc,LPCWSTR pwszURL,void* pbuf,DWORD cb,LPCWSTR pwszMimeProposed,DWORD dwMimeFlags,LPWSTR* ppwszMimeOut,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI GetClassFileOrMime(IBindCtx* pbc,LPCWSTR pwszFilename,void* pbuf,DWORD cb,LPCWSTR pwszMime,DWORD dwReserved,CLSID* pclsid)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI GetClassURL(LPCWSTR pwszURL,CLSID* pclsid)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI GetSoftwareUpdateInfo(LPCWSTR pwszDistUnit,SOFTDISTINFO* psdi)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkGoBack(IUnknown* punk)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkGoForward(IUnknown* punk)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkNavigateMoniker(IUnknown* punk,IMoniker* pmonTarget)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkNavigateString(IUnknown* punk,LPCWSTR szTarget)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkSimpleNavigateToMoniker(
	IMoniker* pmonTarget,LPCWSTR pwszLocation,LPCWSTR pwszTargetFrame,
	IUnknown* punk,IBindCtx* pbc,IBindStatusCallback* pbscb,
	DWORD dwHLNF,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI HlinkSimpleNavigateToString(
	LPCWSTR pwszTarget,LPCWSTR pwszLocation,LPCWSTR pwszTargetFrame,
	IUnknown* punk,IBindCtx* pbc,IBindStatusCallback* pbscb,
	DWORD dwHLNF,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI IsAsyncMoniker(IMoniker* pmon)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

BOOL WINAPI IsLoggingEnabledA(LPCSTR pszURL)
{
	FIXME("() stub\n");
	return FALSE;
}

BOOL WINAPI IsLoggingEnabledW(LPCWSTR pwszURL)
{
	FIXME("() stub\n");
	return FALSE;
}

HRESULT WINAPI IsValidURL(IBindCtx* pbc,LPCWSTR pwszURL,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI MkParseDisplayNameEx(IBindCtx* pbc,LPCWSTR pwszDisplayName,ULONG* pulCharEaten,IMoniker** ppmon)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI RegisterFormatEnumerator(IBindCtx* pbc,IEnumFORMATETC* penum,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI RegisterMediaTypeClass(IBindCtx* pbc,UINT cTypes,const LPCSTR* pszTypes,CLSID* pclsid,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI RegisterMediaTypes(UINT cTypes,const LPCSTR* pszTypes,CLIPFORMAT* pcfTypes)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

void WINAPI ReleaseBindInfo(BINDINFO* pbi)
{
	FIXME("() stub\n");
}

HRESULT WINAPI RevokeFormatEnumerator(IBindCtx* pbc,IEnumFORMATETC* penum)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI SetSoftwareUpdateAdvertisementState(LPCWSTR pwszDistUnit,DWORD dwAdvState,DWORD dwAdvVerMS,DWORD dwAdvVerLS)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLDownloadToCacheFileA(IUnknown* punk,LPCSTR pszURL,LPSTR pszNameBuf,DWORD dwNameBufLen,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLDownloadToCacheFileW(IUnknown* punk,LPCWSTR pwszURL,LPWSTR pwszNameBuf,DWORD dwNameBufLen,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLDownloadToFileA(IUnknown* punk,LPCSTR pszURL,LPCSTR pszFileName,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLDownloadToFileW(IUnknown* punk,LPCWSTR pwszURL,LPCWSTR pwszFileName,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenBlockingStreamA(IUnknown* punk,LPCSTR pszURL,IStream** ppstream,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenBlockingStreamW(IUnknown* punk,LPCWSTR pwszURL,IStream** ppstream,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenPullStreamA(IUnknown* punk,LPCSTR pszURL,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenPullStreamW(IUnknown* punk,LPCWSTR pwszURL,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenStreamA(IUnknown* punk,LPCSTR pszURL,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI URLOpenStreamW(IUnknown* punk,LPCWSTR pwszURL,DWORD dwReserved,IBindStatusCallback* pbscb)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

HRESULT WINAPI UrlMkGetSessionOption(DWORD dwOpt,void* pvBuf,DWORD dwBufLen,DWORD* pdwLen,DWORD dwReserved)
{
	FIXME("() stub\n");
	return E_NOTIMPL;
}

BOOL WINAPI WriteHitLogging(HIT_LOGGING_INFO* pli)
{
	FIXME("() stub\n");
	return FALSE;
}


