/* MPR.dll
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include "win.h"
#include "debug.h"

DWORD WINAPI WNetGetCachedPassword(
	LPSTR	pbResource,
	WORD	cbResource,
	LPSTR	pbPassword,
	LPWORD	pcbPassword,
	BYTE	nType
) {
	FIXME(mpr,"(%s,%d,%p,%d,%d): stub\n",
		pbResource,cbResource,pbPassword,*pcbPassword,nType);
	return 0;
}

DWORD WINAPI MultinetGetConnectionPerformance32A(
	LPNETRESOURCE32A lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
) {
	FIXME(mpr,"(%p,%p): stub\n",lpNetResource,lpNetConnectInfoStruct);
	return 1;
}
