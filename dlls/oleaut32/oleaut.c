/*
 *	OLEAUT32
 *
 * Copyright 1999, 2000 Marcus Meissner
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

#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"
#include "wine/obj_oleaut.h"
#include "wine/obj_olefont.h"

#include "tmarshal.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static WCHAR	_delimiter[2] = {'!',0}; /* default delimiter apparently */
static WCHAR	*pdelimiter = &_delimiter[0];

/***********************************************************************
 *		RegisterActiveObject (OLEAUT32.33)
 */
HRESULT WINAPI RegisterActiveObject(
	LPUNKNOWN punk,REFCLSID rcid,DWORD dwFlags,LPDWORD pdwRegister
) {
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
	ret = IRunningObjectTable_Register(runobtable,dwFlags,punk,moniker,pdwRegister);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}

/***********************************************************************
 *		RevokeActiveObject (OLEAUT32.34)
 */
HRESULT WINAPI RevokeActiveObject(DWORD xregister,LPVOID reserved)
{
	LPRUNNINGOBJECTTABLE	runobtable;
	HRESULT			ret;

	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) return ret;
	ret = IRunningObjectTable_Revoke(runobtable,xregister);
	if (SUCCEEDED(ret)) ret = S_OK;
	IRunningObjectTable_Release(runobtable);
	return ret;
}

/***********************************************************************
 *		GetActiveObject (OLEAUT32.35)
 */
HRESULT WINAPI GetActiveObject(REFCLSID rcid,LPVOID preserved,LPUNKNOWN *ppunk)
{
	WCHAR 			guidbuf[80];
	HRESULT			ret;
	LPRUNNINGOBJECTTABLE	runobtable;
	LPMONIKER		moniker;

	StringFromGUID2(rcid,guidbuf,39);
	ret = CreateItemMoniker(pdelimiter,guidbuf,&moniker);
	if (FAILED(ret))
		return ret;
	ret = GetRunningObjectTable(0,&runobtable);
	if (FAILED(ret)) {
		IMoniker_Release(moniker);
		return ret;
	}
	ret = IRunningObjectTable_GetObject(runobtable,moniker,ppunk);
	IRunningObjectTable_Release(runobtable);
	IMoniker_Release(moniker);
	return ret;
}


/***********************************************************************
 *           OaBuildVersion           [OLEAUT32.170]
 *
 * known OLEAUT32.DLL versions:
 * OLE 2.1  NT				1993-95	10     3023
 * OLE 2.1					10     3027
 * Win32s 1.1e					20     4049
 * OLE 2.20 W95/NT			1993-96	20     4112
 * OLE 2.20 W95/NT			1993-96	20     4118
 * OLE 2.20 W95/NT			1993-96	20     4122
 * OLE 2.30 W95/NT			1993-98	30     4265
 * OLE 2.40 NT??			1993-98	40     4267
 * OLE 2.40 W98 SE orig. file		1993-98	40     4275
 * OLE 2.40 W2K orig. file		1993-XX 40     4514
 *
 * I just decided to use version 2.20 for Win3.1, 2.30 for Win95 & NT 3.51,
 * and 2.40 for all newer OSs. The build number is maximum, i.e. 0xffff.
 */
UINT WINAPI OaBuildVersion()
{
    switch(GetVersion() & 0x8000ffff)  /* mask off build number */
    {
    case 0x80000a03:  /* WIN31 */
		return MAKELONG(0xffff, 20);
    case 0x00003303:  /* NT351 */
		return MAKELONG(0xffff, 30);
    case 0x80000004:  /* WIN95; I'd like to use the "standard" w95 minor
		         version here (30), but as we still use w95
		         as default winver (which is good IMHO), I better
		         play safe and use the latest value for w95 for now.
		         Change this as soon as default winver gets changed
		         to something more recent */
    case 0x80000a04:  /* WIN98 */
    case 0x00000004:  /* NT40 */
    case 0x00000005:  /* W2K */
		return MAKELONG(0xffff, 40);
    default:
		ERR("Version value not known yet. Please investigate it !\n");
		return 0x0;
    }
}

/***********************************************************************
 *		DllRegisterServer (OLEAUT32.320)
 */
HRESULT WINAPI OLEAUT32_DllRegisterServer() {
    FIXME("stub!\n");
    return S_OK;
}

/***********************************************************************
 *		DllUnregisterServer (OLEAUT32.321)
 */
HRESULT WINAPI OLEAUT32_DllUnregisterServer() {
    FIXME("stub!\n");
    return S_OK;
}

extern void _get_STDFONT_CF(LPVOID);

/***********************************************************************
 *		DllGetClassObject (OLEAUT32.1)
 */
HRESULT WINAPI OLEAUT32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualGUID(rclsid,&CLSID_StdFont)) {
	if (IsEqualGUID(iid,&IID_IClassFactory)) {
	    _get_STDFONT_CF(ppv);
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    }
    if (IsEqualGUID(rclsid,&CLSID_PSOAInterface)) {
	if (S_OK==TypeLibFac_DllGetClassObject(rclsid,iid,ppv))
	    return S_OK;
	/*FALLTHROUGH*/
    }
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (OLEAUT32.410)
 */
HRESULT WINAPI OLEAUT32_DllCanUnloadNow() {
    FIXME("(), stub!\n");
    return S_FALSE;
}
