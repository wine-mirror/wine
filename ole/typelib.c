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

HRESULT WINAPI
QueryPathOfRegTypeLib(REFGUID guid,WORD wMaj, WORD wMin, LCID lcid,LPSTR path) 
{
	char	xguid[80];

	if (HIWORD(guid))
		WINE_StringFromCLSID(guid,xguid);
	else
		sprintf(xguid,"<guid 0x%08lx>",guid);
	fprintf(stderr,"QueryPathOfRegTypeLib(%s,%d,%d,0x%04x,%p),stub!\n",
		xguid,wMaj,wMin,lcid,path
	);
	return E_FAIL;
}
