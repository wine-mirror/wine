/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wintypes.h"
#include "windows.h"
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "compobj.h"
#include "debug.h"

/****************************************************************************
 *	QueryPathOfRegTypeLib			(TYPELIB.14)
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPSTR path	/* [out] path of typelib */
) {
	char	xguid[80];

	if (HIWORD(guid))
		WINE_StringFromCLSID(guid,xguid);
	else
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
	FIXME(ole,"(%s,%d,%d,0x%04lx,%p),stub!\n",xguid,wMaj,wMin,lcid,path);
	return E_FAIL;
}

DWORD WINAPI OABuildVersion()
{
	return MAKELONG(0xbd3, 0x3);
}
