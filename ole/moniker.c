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
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
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

/******************************************************************************
 *		CreateItemMoniker32	[OLE32.58]
 */
HRESULT WINAPI
CreateItemMoniker32(
	LPCOLESTR32 lpszDelim,	/* [in] */
	LPCOLESTR32 lpszItem,	/* [in] */
	LPMONIKER * ppmk	/* [out] new moniker object */
)
{
    FIXME (ole,"(%s %s %p),stub!\n",
	   debugstr_w(lpszDelim), debugstr_w(lpszItem), ppmk);
    return E_FAIL;
}

