#include "wine/debug.h"
#include "winerror.h"


WINE_DEFAULT_DEBUG_CHANNEL(qcap);

/***********************************************************************
 *      DllRegisterServer (QCAP.@)
 */
HRESULT WINAPI QCAP_DllRegisterServer()
{
	FIXME("(): stub\n");
	return 0;
}

/***********************************************************************
 *		DllGetClassObject (QCAP.@)
 */
HRESULT WINAPI QCAP_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
