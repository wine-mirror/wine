/* MPR.dll
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include "win.h"
#include "stddebug.h"
#include "debug.h"

DWORD WINAPI WNetGetCachedPassword(
	LPSTR	pbResource,
	WORD	cbResource,
	LPSTR	pbPassword,
	LPWORD	pcbPassword,
	BYTE	nType
) {
	fprintf(stdnimp,"WNetGetCachedPassword(%s,%d,%p,%d,%d)\n",
		pbResource,cbResource,pbPassword,*pcbPassword,nType
	);
	return 0;
}

DWORD WINAPI MultinetGetConnectionPerformance32A(
	LPNETRESOURCE32A lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
) {
	fprintf(stdnimp,"MultinetGetConnectionPerformance(%p,%p)\n",
			lpNetResource,
			lpNetConnectInfoStruct
	);
	return 1;
}
