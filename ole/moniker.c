/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "ldt.h"
#include "heap.h"
#include "compobj.h"
#include "interfaces.h"
#include "storage.h"
#include "shlobj.h"
#include "local.h"
#include "module.h"
#include "debug.h"

/******************************************************************************
 *		CreateFileMoniker16	[OLE2.28]
 */
HRESULT WINAPI
CreateFileMoniker16(
	LPCOLESTR16 lpszPathName,	/* [in] pathname */
	LPMONIKER * ppmk		/* [out] new moniker object */
) {
	FIXME(ole,"(%s,%p),stub!\n",lpszPathName,ppmk);
	*ppmk = NULL;
	return E_FAIL;
}

/******************************************************************************
 *		CreateFileMoniker32	[OLE32.55]
 */
HRESULT WINAPI
CreateFileMoniker32(
	LPCOLESTR32 lpszPathName,	/* [in] pathname */
	LPMONIKER * ppmk		/* [out] new moniker object */
) {
	FIXME(ole,"(%s,%p),stub!\n",debugstr_w(lpszPathName),ppmk);
	*ppmk = NULL;
	return E_FAIL;
}
