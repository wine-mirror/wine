/*
 *	TYPELIB
 *
 *	Copyright 1997	Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "oleauto.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "wine/obj_base.h"
#include "debug.h"
#include "winversion.h"

/****************************************************************************
 *		QueryPathOfRegTypeLib16	[TYPELIB.14]
 *
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
		FIXME(ole,"(%s,%d,%d,0x%04lx,%p),can't handle non-string guids.\n",xguid,wMaj,wMin,(DWORD)lcid,path);
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
 *		QueryPathOfRegTypeLib32	[OLEAUT32.164]
 * RETURNS
 *	path of typelib
 */
HRESULT WINAPI
QueryPathOfRegTypeLib(	
	REFGUID guid,	/* [in] referenced guid */
	WORD wMaj,	/* [in] major version */
	WORD wMin,	/* [in] minor version */
	LCID lcid,	/* [in] locale id */
	LPBSTR path	/* [out] path of typelib */
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
		FIXME(ole,"(%s,%d,%d,0x%04lx,%p),stub!\n",xguid,wMaj,wMin,(DWORD)lcid,path);
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
 * NOTES
 *    Docs: OLECHAR32 FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLib16(
    OLECHAR *szFile, /* [in] Name of file to load from */
    void * *pptLib) /* [out] Pointer to pointer to loaded type library */
{
    FIXME(ole, "('%s',%p): stub\n",debugstr_w((LPWSTR)szFile),pptLib);

    if (pptLib!=0)
      *pptLib=0;

    return E_FAIL;
}

/******************************************************************************
 *		LoadTypeLib32	[OLEAUT32.161]
 * Loads and registers a type library
 * NOTES
 *    Docs: OLECHAR32 FAR* szFile
 *    Docs: iTypeLib FAR* FAR* pptLib
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI LoadTypeLib(
    OLECHAR *szFile,   /* [in] Name of file to load from */
    void * *pptLib) /* [out] Pointer to pointer to loaded type library */
{
    FIXME(ole, "('%s',%p): stub\n",debugstr_w(szFile),pptLib);

    if (pptLib!=0)
      *pptLib=0;

    return E_FAIL;
}

/******************************************************************************
 *		LoadRegTypeLib	[OLEAUT32.162]
 */
HRESULT WINAPI LoadRegTypeLib(
  REFGUID        rguid,
  unsigned short wVerMajor,
  unsigned short wVerMinor,
  LCID           lcid,
  void**         pptLib)
{
  FIXME(ole, "(): stub\n");

  if (pptLib!=0)
    *pptLib=0;
  
  return E_FAIL;
}

/******************************************************************************
 *		RegisterTypeLib32	[OLEAUT32.163]
 * Adds information about a type library to the System Registry           
 * NOTES
 *    Docs: ITypeLib FAR * ptlib
 *    Docs: OLECHAR32 FAR* szFullPath
 *    Docs: OLECHAR32 FAR* szHelpDir
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: Status
 */
HRESULT WINAPI RegisterTypeLib(
     ITypeLib * ptlib,      /*[in] Pointer to the library*/
     OLECHAR * szFullPath, /*[in] full Path of the library*/
     OLECHAR * szHelpDir)  /*[in] dir to the helpfile for the library, may be NULL*/
{   FIXME(ole, "(%p,%s,%s): stub\n",ptlib, debugstr_w(szFullPath),debugstr_w(szHelpDir));
    return S_OK;	/* FIXME: pretend everything is OK */
}

/****************************************************************************
 *	OABuildVersion				(TYPELIB.15)
 * RETURNS
 *	path of typelib
 */
DWORD WINAPI OABuildVersion16(void)
{
WINDOWS_VERSION ver = VERSION_GetVersion();

    switch (ver) {
      case WIN95:
	return MAKELONG(0xbd0, 0xa); /* Win95A */
      case WIN31:
	return MAKELONG(0xbd3, 0x3); /* WfW 3.11 */
      default:
	FIXME(ole, "Version value not known yet. Please investigate it !");
	return MAKELONG(0xbd0, 0xa); /* return Win95A for now */
    }
}
