/*
 *	OLEAUT32
 *
 */
#include <string.h>
#include "winuser.h"
#include "winerror.h"
#include "oleauto.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole)

HRESULT WINAPI RegisterActiveObject(
	LPUNKNOWN punk,REFCLSID rcid,DWORD dwFlags,LPDWORD pdwRegister
) {
	char buf[80];

	if (rcid)
		WINE_StringFromCLSID(rcid,buf);
	else
		sprintf(buf,"<clsid-%p>",rcid);
	FIXME("(%p,%s,0x%08lx,%p), stub!\n",punk,buf,dwFlags,pdwRegister);
	return E_FAIL;
}

HRESULT WINAPI RevokeActiveObject(DWORD xregister,LPVOID reserved)
{
	FIXME("(0x%08lx,%p),stub!\n",xregister,reserved);
	return E_FAIL;
}

HRESULT WINAPI GetActiveObject(REFCLSID rcid,LPVOID preserved,LPUNKNOWN *ppunk)
{
	char buf[80];

	if (rcid)
		WINE_StringFromCLSID(rcid,buf);
	else
		sprintf(buf,"<clsid-%p>",rcid);
	FIXME("(%s,%p,%p),stub!\n",buf,preserved,ppunk);
	return E_FAIL;
}
