#include "wine/debug.h"
#include "winerror.h"


WINE_DEFAULT_DEBUG_CHANNEL(quartz);

DWORD dll_ref = 0;

/***********************************************************************
 *      DllRegisterServer (QUARTZ.@)
 */
HRESULT WINAPI QUARTZ_DllRegisterServer()
{
	FIXME("(): stub\n");
	return 0;
}

/***********************************************************************
 *		DllGetClassObject (QUARTZ.@)
 */
HRESULT WINAPI QUARTZ_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (QUARTZ.@)
 */
HRESULT WINAPI QUARTZ_DllCanUnloadNow()
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}
