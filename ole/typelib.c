/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wintypes.h"
#include "heap.h"
#include "windows.h"
#include "winreg.h"
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "oleauto.h"
#include "compobj.h"
#include "debug.h"

/****************************************************************************
 *	QueryPathOfRegTypeLib			(TYPELIB.14)
 * the path is "Classes\Typelib\<guid>\<major>.<minor>\<lcid>\win16\"
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib16(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR16 path	/* [out] path of typelib */
) {
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	DWORD	plen;

	if (HIWORD(guid)) {
		WINE_StringFromCLSID(guid,xguid);
		sprintf(typelibkey,"SOFTWARE\\Classes\\Typelib\\%s\\%d.%d\\%ld\\win16",
			xguid,wMaj,wMin,lcid&0xff
		);
	} else {
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
		FIXME(ole,"(%s,%d,%d,0x%04x,%p),can't handle non-string guids.\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValue16(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		FIXME(ole,"key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = SysAllocString16(pathname);
	return S_OK;
}

/****************************************************************************
 *	QueryPathOfRegTypeLib			(OLEAUT32.164)
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib32(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR32 path	/* [out] path of typelib */
) {
	char	xguid[80];
	char	typelibkey[100],pathname[260];
	DWORD	plen;


	if (HIWORD(guid)) {
		WINE_StringFromCLSID(guid,xguid);
		sprintf(typelibkey,"SOFTWARE\\Classes\\Typelib\\%s\\%d.%d\\%ld\\win32",
			xguid,wMaj,wMin,lcid&0xff
		);
	} else {
		sprintf(xguid,"<guid 0x%08lx>",(DWORD)guid);
		FIXME(ole,"(%s,%d,%d,0x%04x,%p),stub!\n",xguid,wMaj,wMin,(DWORD)lcid,path);
		return E_FAIL;
	}
	plen = sizeof(pathname);
	if (RegQueryValue16(HKEY_LOCAL_MACHINE,typelibkey,pathname,&plen)) {
		FIXME(ole,"key %s not found\n",typelibkey);
		return E_FAIL;
	}
	*path = HEAP_strdupAtoW(GetProcessHeap(),0,pathname);
	return S_OK;
}

/******************************************************************************
 * LoadTypeLib [TYPELIB.3]  Loads and registers a type library
 *
 * NOTES
 *    Docs: OLECHAR FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLib(
    void *szFile,   /* [in] Name of file to load from */
    void * *pptLib) /* [out] Pointer to pointer to loaded type library */
{
    FIXME(ole, "(%s,%p): stub\n",debugstr_a(szFile),pptLib);
    return E_FAIL;
}


/****************************************************************************
 *	OABuildVersion				(TYPELIB.15)
 * RETURNS
 *	path of typelib
 */
DWORD WINAPI OABuildVersion()
{
	return MAKELONG(0xbd3, 0x3);
}
