/*
 * SHDOCVW - Internet Explorer Web Control
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"

#include "initguid.h"
#include "ole2.h"
#include "shlwapi.h"

#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/***********************************************************************
 *              DllCanUnloadNow (SHDOCVW.109) */
HRESULT WINAPI SHDOCVW_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.312)
 */
HRESULT WINAPI SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("\n");

    if (IsEqualGUID(&IID_IClassFactory, riid))
    {
        /* Pass back our shdocvw class factory */
        *ppv = (LPVOID)&SHDOCVW_ClassFactory;
        IClassFactory_AddRef((IClassFactory*)&SHDOCVW_ClassFactory);

        return S_OK;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllGetVersion (SHDOCVW.113)
 */
HRESULT WINAPI SHDOCVW_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}

/*************************************************************************
 *              DllInstall (SHDOCVW.114)
 */
HRESULT WINAPI SHDOCVW_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;
}

/***********************************************************************
 *		DllRegisterServer (SHDOCVW.124)
 */
HRESULT WINAPI SHDOCVW_DllRegisterServer()
{
    FIXME("(), stub!\n");
    return S_OK;
}

/***********************************************************************
 *		DllUnregisterServer (SHDOCVW.127)
 */
HRESULT WINAPI SHDOCVW_DllUnregisterServer()
{
    FIXME("(), stub!\n");
    return S_OK;
}

