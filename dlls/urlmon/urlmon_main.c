/*
 * UrlMon
 *
 * Copyright (c) 2000 Patrik Stridvall
 *
 */

#include "windef.h"
#include "winerror.h"
#include "wtypes.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/***********************************************************************
 *		URLMON_DllInstall (URLMON.@)
 */
HRESULT WINAPI URLMON_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
  FIXME("(%s, %s): stub\n", bInstall?"TRUE":"FALSE", 
	debugstr_w(cmdline));

  return S_OK;
}

/***********************************************************************
 *		DllCanUnloadNow (URLMON.@)
 */
HRESULT WINAPI URLMON_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (URLMON.@)
 */
HRESULT WINAPI URLMON_DllGetClassObject(REFCLSID rclsid, REFIID riid,
					LPVOID *ppv)
{
    FIXME("(%p, %p, %p): stub\n", debugstr_guid(rclsid), 
	  debugstr_guid(riid), ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
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

