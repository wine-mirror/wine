/*
 * Regster/Unregister servers. (for internal use)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winreg.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

#include "regsvr.h"


HRESULT QUARTZ_RegisterDLLServer(
	const CLSID* pclsid,
	LPCSTR lpFriendlyName,
	LPCSTR lpNameOfDLL )
{
	FIXME( "stub!\n" );
	return E_FAIL;
}

HRESULT QUARTZ_UnregisterDLLServer(
	const CLSID* pclsid )
{
	FIXME( "stub!\n" );
	return E_FAIL;
}


HRESULT QUARTZ_RegisterFilter(
	const CLSID* pguidFilterCategory,
	const CLSID* pclsid,
	const BYTE* pbFilterData,
	DWORD cbFilterData,
	LPCSTR lpFriendlyName )
{
	FIXME( "stub!\n" );
	return E_FAIL;
}

HRESULT QUARTZ_UnregisterFilter(
	const CLSID* pguidFilterCategory,
	const CLSID* pclsid )
{
	FIXME( "stub!\n" );
	return E_FAIL;
}



