/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 *      Copyright 1999  Noomen Hamza
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "debug.h"

/******************************************************************************
 *		GetRunningObjectTable16	[OLE2.30]
 */
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPVOID *pprot) {
	FIXME(ole,"(%ld,%p),stub!\n",reserved,pprot);
	return E_FAIL;
}


/***********************************************************************
 *           GetRunningObjectTable32 (OLE2.73)
 */
HRESULT WINAPI GetRunningObjectTable(DWORD reserved, LPVOID *pprot) {
	FIXME(ole,"(%ld,%p),stub!\n",reserved,pprot);
    return E_FAIL;
}
