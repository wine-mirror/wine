/*
 *	OLEAUT32
 *
 */
#include <string.h>
#include "winuser.h"
#include "winerror.h"
#include "winversion.h"
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

/***********************************************************************
 *           OaBuildVersion           [OLEAUT32.170]
 *
 * known OLEAUT32.DLL versions:
 * OLE 2.1  NT				1993-95	10     3023
 * OLE 2.1					10     3027
 * OLE 2.20 W95/NT			1993-96	20     4112
 * OLE 2.20 W95/NT			1993-96	20     4118
 * OLE 2.20 W95/NT			1993-96	20     4122
 * OLE 2.30 W95/NT			1993-98	30     4265
 * OLE 2.40 NT??			1993-98	40     4267
 * OLE 2.40 W98 SE orig. file		1993-98	40     4275
 */
UINT WINAPI OaBuildVersion()
{
    FIXME("Please report to a.mohr@mailto.de if you get version error messages !\n");
    switch(VERSION_GetVersion())
    {
	case WIN31:
		return MAKELONG(4049, 20); /* from Win32s 1.1e */
	case WIN95:
		return MAKELONG(4265, 30);
	case WIN98:
		return MAKELONG(4275, 40); /* value of W98 SE; orig. W98 AFAIK has 4265, 30 just as W95 */
	case NT351:
		return MAKELONG(4265, 30); /* value borrowed from Win95 */
	case NT40:
		return MAKELONG(4122, 20); /* ouch ! Quite old, I guess */
	default:
		ERR("Version value not known yet. Please investigate it !\n");
		return 0x0;
    }
}
