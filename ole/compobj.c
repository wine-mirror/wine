/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 */

#define INITGUID

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ole.h"
#include "ole2.h"
#include "winerror.h"
#include "debug.h"
#include "file.h"
#include "compobj.h"
#include "heap.h"
#include "ldt.h"
#include "interfaces.h"
#include "shlobj.h"
#include "ddraw.h"
#include "dsound.h"
#include "dinput.h"
#include "d3d.h"
#include "dplay.h"


LPMALLOC16 currentMalloc16=NULL;
LPMALLOC32 currentMalloc32=NULL;

HTASK16 hETask = 0;
WORD Table_ETask[62];

/******************************************************************************
 *           CoBuildVersion [COMPOBJ.1]
 *
 * RETURNS
 *	Current built version, hiword is majornumber, loword is minornumber
 */
DWORD WINAPI CoBuildVersion()
{
    TRACE(ole,"(void)\n");
    return (rmm<<16)+rup;
}

/******************************************************************************
 *		CoInitialize16	[COMPOBJ.2]
 * Set the win16 IMalloc used for memory management
 */
HRESULT WINAPI CoInitialize16(
	LPMALLOC16 lpReserved	/* [in] pointer to win16 malloc interface */
) {
    currentMalloc16 = lpReserved;
    return S_OK;
}

/******************************************************************************
 *		CoInitialize32	[OLE32.26]
 * Set the win32 IMalloc used for memorymanagement
 */
HRESULT WINAPI CoInitialize32(
	LPMALLOC32 lpReserved	/* [in] pointer to win32 malloc interface */
) {
    currentMalloc32 = lpReserved;
    return S_OK;
}

/***********************************************************************
 *           CoUnitialize   [COMPOBJ.3]
 * Don't know what it does.
 */
void WINAPI CoUnitialize()
{
    TRACE(ole,"(void)\n");
}

/***********************************************************************
 *           CoGetMalloc16    [COMPOBJ.4]
 * RETURNS
 *	The current win16 IMalloc
 */
HRESULT WINAPI CoGetMalloc16(
	DWORD dwMemContext,	/* [in] unknown */
	LPMALLOC16 * lpMalloc	/* [out] current win16 malloc interface */
) {
    if(!currentMalloc16)
	currentMalloc16 = IMalloc16_Constructor();
    *lpMalloc = currentMalloc16;
    return S_OK;
}

/******************************************************************************
 *		CoGetMalloc32	[OLE32.20]
 *
 * RETURNS
 *	The current win32 IMalloc
 */
HRESULT WINAPI CoGetMalloc32(
	DWORD dwMemContext,	/* [in] unknown */
	LPMALLOC32 *lpMalloc	/* [out] current win32 malloc interface */
) {
    if(!currentMalloc32)
	currentMalloc32 = IMalloc32_Constructor();
    *lpMalloc = currentMalloc32;
    return S_OK;
}

/***********************************************************************
 *           CoCreateStandardMalloc16 [COMPOBJ.71]
 */
OLESTATUS WINAPI CoCreateStandardMalloc16(DWORD dwMemContext,
					  LPMALLOC16 *lpMalloc)
{
    /* FIXME: docu says we shouldn't return the same allocator as in
     * CoGetMalloc16 */
    *lpMalloc = IMalloc16_Constructor();
    return S_OK;
}

/******************************************************************************
 *		CoDisconnectObject	[COMPOBJ.15]
 */
OLESTATUS WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    TRACE(ole,"%p %lx\n",lpUnk,reserved);
    return S_OK;
}

/***********************************************************************
 *           IsEqualGUID [COMPOBJ.18]
 * Compares two Unique Identifiers
 * RETURNS
 *	TRUE if equal
 */
BOOL16 WINAPI IsEqualGUID(
	GUID* g1,	/* [in] unique id 1 */
	GUID* g2	/* [in] unique id 2 */
) {
    return !memcmp( g1, g2, sizeof(GUID) );
}

/******************************************************************************
 *		CLSIDFromString16	[COMPOBJ.20]
 * Converts a unique identifier from it's string representation into 
 * the GUID struct.
 *
 * Class id: DWORD-WORD-WORD-BYTES[2]-BYTES[6] 
 *
 * RETURNS
 *	the converted GUID
 */
OLESTATUS WINAPI CLSIDFromString16(
	LPCOLESTR16 idstr,	/* [in] string representation of guid */
	CLSID *id		/* [out] GUID converted from string */
) {
  BYTE *s = (BYTE *) idstr;
  BYTE *p;
  int	i;
  BYTE table[256];

  TRACE(ole,"%s -> %p\n", idstr, id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++) {
    table['0' + i] = i;
  }
  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

  if (strlen(idstr) != 38)
    return OLE_ERROR_OBJECT;

  p = (BYTE *) id;

  s++;	/* skip leading brace  */
  for (i = 0; i < 4; i++) {
    p[3 - i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 4;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  /* these are just sequential bytes */
  for (i = 0; i < 2; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  s++;	/* skip - */

  for (i = 0; i < 6; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }

  return S_OK;
}

/******************************************************************************
 *		CLSIDFromString32	[OLE32.3]
 * Converts a unique identifier from it's string representation into 
 * the GUID struct.
 * RETURNS
 *	the converted GUID
 */
OLESTATUS WINAPI CLSIDFromString32(
	LPCOLESTR32 idstr,	/* [in] string representation of GUID */
	CLSID *id		/* [out] GUID represented by above string */
) {
    LPOLESTR16      xid = HEAP_strdupWtoA(GetProcessHeap(),0,idstr);
    OLESTATUS       ret = CLSIDFromString16(xid,id);

    HeapFree(GetProcessHeap(),0,xid);
    return ret;
}

/******************************************************************************
 *		WINE_StringFromCLSID	[???]
 * Converts a GUID into the respective string representation.
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI WINE_StringFromCLSID(
	const CLSID *id,	/* [in] GUID to be converted */
	LPSTR idstr		/* [out] pointer to buffer to contain converted guid */
) {
  static const char *hex = "0123456789ABCDEF";
  char *s;
  int	i;

  if (!id)
	{ ERR(ole,"called with id=Null\n");
	  *idstr = 0x00;
	  return E_FAIL;
	}
	
  sprintf(idstr, "{%08lx-%04x-%04x-%02x%02x-",
	  id->Data1, id->Data2, id->Data3,
	  id->Data4[0], id->Data4[1]);
  s = &idstr[25];

  /* 6 hex bytes */
  for (i = 2; i < 8; i++) {
    *s++ = hex[id->Data4[i]>>4];
    *s++ = hex[id->Data4[i] & 0xf];
  }

  *s++ = '}';
  *s++ = '\0';

  for (i = strlen(idstr)-1; i >= 0; i--) {
    idstr[i] = toupper(idstr[i]);
  }

  TRACE(ole,"%p->%s\n", id, idstr);

  return OLE_OK;
}

/******************************************************************************
 *		StringFromCLSID16	[COMPOBJ.19]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI StringFromCLSID16(
	const CLSID *id,	/* [in] the GUID to be converted */
	LPOLESTR16 *idstr	/* [out] a pointer to a to-be-allocated segmented pointer pointing to the resulting string */

) {
    LPMALLOC16	mllc;
    OLESTATUS	ret;
    DWORD	args[2];

    ret = CoGetMalloc16(0,&mllc);
    if (ret) return ret;

    args[0] = (DWORD)mllc;
    args[1] = 40;

    /* No need for a Callback entry, we have WOWCallback16Ex which does
     * everything we need.
     */
    if (!WOWCallback16Ex(
    	(FARPROC16)((LPMALLOC16_VTABLE)PTR_SEG_TO_LIN(
		((LPMALLOC16)PTR_SEG_TO_LIN(mllc))->lpvtbl)
	)->fnAlloc,
	WCB16_CDECL,
	2,
	(LPVOID)args,
	(LPDWORD)idstr
    )) {
    	WARN(ole,"CallTo16 IMalloc16 failed\n");
    	return E_FAIL;
    }
    return WINE_StringFromCLSID(id,PTR_SEG_TO_LIN(*idstr));
}

/******************************************************************************
 *		StringFromCLSID32	[OLE32.151]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI StringFromCLSID32(
	const CLSID *id,	/* [in] the GUID to be converted */
	LPOLESTR32 *idstr	/* [out] a pointer to a to-be-allocated pointer pointing to the resulting string */
) {
	char            buf[80];
	OLESTATUS       ret;
	LPMALLOC32	mllc;

	if ((ret=CoGetMalloc32(0,&mllc)))
		return ret;

	ret=WINE_StringFromCLSID(id,buf);
	if (!ret) {
		*idstr = mllc->lpvtbl->fnAlloc(mllc,strlen(buf)*2+2);
		lstrcpyAtoW(*idstr,buf);
	}
	return ret;
}

/******************************************************************************
 *		StringFromGUID2	[COMPOBJ.76] [OLE32.152]
 *
 * Converts a global unique identifier into a string of an API-
 * specified fixed format. (The usual {.....} stuff.)
 *
 * RETURNS
 *	The (UNICODE) string representation of the GUID in 'str'
 *	The length of the resulting string, 0 if there was any problem.
 */
INT32 WINAPI
StringFromGUID2(REFGUID id, LPOLESTR32 str, INT32 cmax)
{
  char		xguid[80];

  if (WINE_StringFromCLSID(id,xguid))
  	return 0;
  if (strlen(xguid)>=cmax)
  	return 0;
  lstrcpyAtoW(str,xguid);
  return strlen(xguid);
}

/******************************************************************************
 *		CLSIDFromProgID16	[COMPOBJ.61]
 * Converts a program id into the respective GUID. (By using a registry lookup)
 * RETURNS
 *	riid associated with the progid
 */
OLESTATUS WINAPI CLSIDFromProgID16(
	LPCOLESTR16 progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	char	*buf,buf2[80];
	DWORD	buf2len;
	HRESULT	err;
	HKEY	xhkey;

	buf = HeapAlloc(GetProcessHeap(),0,strlen(progid)+8);
	sprintf(buf,"%s\\CLSID",progid);
	if ((err=RegOpenKey32A(HKEY_CLASSES_ROOT,buf,&xhkey))) {
		HeapFree(GetProcessHeap(),0,buf);
		return OLE_ERROR_GENERIC;
	}
	HeapFree(GetProcessHeap(),0,buf);
	buf2len = sizeof(buf2);
	if ((err=RegQueryValue32A(xhkey,NULL,buf2,&buf2len))) {
		RegCloseKey(xhkey);
		return OLE_ERROR_GENERIC;
	}
	RegCloseKey(xhkey);
	return CLSIDFromString16(buf2,riid);
}

/******************************************************************************
 *		CLSIDFromProgID32	[OLE32.2]
 * Converts a program id into the respective GUID. (By using a registry lookup)
 * RETURNS
 *	riid associated with the progid
 */
OLESTATUS WINAPI CLSIDFromProgID32(
	LPCOLESTR32 progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	LPOLESTR16 pid = HEAP_strdupWtoA(GetProcessHeap(),0,progid);
	OLESTATUS       ret = CLSIDFromProgID16(pid,riid);

	HeapFree(GetProcessHeap(),0,pid);
	return ret;
}

/***********************************************************************
 *           LookupETask (COMPOBJ.94)
 */
OLESTATUS WINAPI LookupETask(HTASK16 *hTask,LPVOID p) {
	FIXME(ole,"(%p,%p),stub!\n",hTask,p);
	if ((*hTask = GetCurrentTask()) == hETask) {
		memcpy(p, Table_ETask, sizeof(Table_ETask));
	}
	return 0;
}

/***********************************************************************
 *           SetETask (COMPOBJ.95)
 */
OLESTATUS WINAPI SetETask(HTASK16 hTask, LPVOID p) {
        FIXME(ole,"(%04x,%p),stub!\n",hTask,p);
	hETask = hTask;
	return 0;
}

/***********************************************************************
 *           CallObjectInWOW (COMPOBJ.201)
 */
OLESTATUS WINAPI CallObjectInWOW(LPVOID p1,LPVOID p2) {
	FIXME(ole,"(%p,%p),stub!\n",p1,p2);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject16	[COMPOBJ.5]
 *
 * Don't know where it registers it ...
 */
OLESTATUS WINAPI CoRegisterClassObject16(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext,
	DWORD flags,
	LPDWORD lpdwRegister
) {
	char	buf[80];

	WINE_StringFromCLSID(rclsid,buf);

	FIXME(ole,"(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		buf,pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject32	[OLE32.36]
 *
 * Don't know where it registers it ...
 */
OLESTATUS WINAPI CoRegisterClassObject32(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext,
	DWORD flags,
	LPDWORD lpdwRegister
) {
    char buf[80];

    WINE_StringFromCLSID(rclsid,buf);

    FIXME(ole,"(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
	    buf,pUnk,dwClsContext,flags,lpdwRegister
    );
    return 0;
}

/***********************************************************************
 *           CoGetClassObject [COMPOBJ.7]
 */
HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext,
                        LPVOID pvReserved, REFIID iid, LPVOID *ppv)
{
    char xclsid[50],xiid[50];
    LPCLASSFACTORY lpclf;
    HRESULT hres = E_UNEXPECTED;

    WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
    WINE_StringFromCLSID((LPCLSID)iid,xiid);
    TRACE(ole,"\n\tCLSID:\t%s,\n\tIID:\t%s\n",xclsid,xiid);

    *ppv = NULL;
    lpclf = IClassFactory_Constructor();
    if (lpclf)
    {
        hres = lpclf->lpvtbl->fnQueryInterface(lpclf,iid, ppv);
        lpclf->lpvtbl->fnRelease(lpclf);
    }
    return hres;
}

/******************************************************************************
 *		CoRegisterMessageFilter16	[COMPOBJ.27]
 */
OLESTATUS WINAPI CoRegisterMessageFilter16(
	LPMESSAGEFILTER lpMessageFilter,
	LPMESSAGEFILTER *lplpMessageFilter
) {
	FIXME(ole,"(%p,%p),stub!\n",lpMessageFilter,lplpMessageFilter);
	return 0;
}

/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13, OLE32.7]
 */
HRESULT WINAPI CoCreateInstance(
	REFCLSID rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD dwClsContext,
	REFIID iid,
	LPVOID *ppv
) {
#if 0
	char buf[80],xbuf[80];

	if (rclsid)
		WINE_StringFromCLSID(rclsid,buf);
	else
		sprintf(buf,"<rclsid-0x%08lx>",(DWORD)rclsid);
	if (iid)
		WINE_StringFromCLSID(iid,xbuf);
	else
		sprintf(xbuf,"<iid-0x%08lx>",(DWORD)iid);

	FIXME(ole,"(%s,%p,0x%08lx,%s,%p): stub !\n",buf,pUnkOuter,dwClsContext,xbuf,ppv);
	*ppv = NULL;
#else	
	HRESULT hres;
	LPCLASSFACTORY lpclf = 0;

	CoGetClassObject(rclsid, dwClsContext, NULL, &IID_IClassFactory, (LPVOID)&lpclf);
	hres = lpclf->lpvtbl->fnCreateInstance(lpclf, pUnkOuter, iid, ppv);
	lpclf->lpvtbl->fnRelease(lpclf);
	return hres;
#endif
}

/***********************************************************************
 *           CoFreeUnusedLibraries [COMPOBJ.17]
 */
void WINAPI CoFreeUnusedLibraries()
{
	FIXME(ole,"(), stub !\n");
}

/***********************************************************************
 *           CoFileTimeNow [COMPOBJ.82, OLE32.10]
 * RETURNS
 *	the current system time in lpFileTime
 */
HRESULT WINAPI CoFileTimeNow(
	FILETIME *lpFileTime	/* [out] the current time */
) {
	DOSFS_UnixTimeToFileTime(time(NULL), lpFileTime, 0);
	return S_OK;
}

/***********************************************************************
 *           CoTaskMemAlloc (OLE32.43)
 * RETURNS
 * 	pointer to newly allocated block
 */
LPVOID WINAPI CoTaskMemAlloc(
	ULONG size	/* [in] size of memoryblock to be allocated */
) {
    LPMALLOC32	lpmalloc;
    HRESULT	ret = CoGetMalloc32(0,&lpmalloc);

    if (ret) 
	return NULL;
    return lpmalloc->lpvtbl->fnAlloc(lpmalloc,size);
}

/***********************************************************************
 *           CoTaskMemFree (OLE32.44)
 */
VOID WINAPI CoTaskMemFree(
	LPVOID ptr	/* [in] pointer to be freed */
) {
    LPMALLOC32	lpmalloc;
    HRESULT	ret = CoGetMalloc32(0,&lpmalloc);

    if (ret) return;
    return lpmalloc->lpvtbl->fnFree(lpmalloc,ptr);
}

/***********************************************************************
 *           CoInitializeWOW (OLE32.27)
 */
HRESULT WINAPI CoInitializeWOW(DWORD x,DWORD y) {
    FIXME(ole,"(0x%08lx,0x%08lx),stub!\n",x,y);
    return 0;
}

/******************************************************************************
 *		CoLockObjectExternal16	[COMPOBJ.63]
 */
HRESULT WINAPI CoLockObjectExternal16(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL16 fLock,		/* [in] do lock */
    BOOL16 fLastUnlockReleases	/* [in] ? */
) {
    FIXME(ole,"(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/******************************************************************************
 *		CoLockObjectExternal32	[OLE32.31]
 */
HRESULT WINAPI CoLockObjectExternal32(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL32 fLock,		/* [in] do lock */
    BOOL32 fLastUnlockReleases	/* [in] ? */
) {
    FIXME(ole,"(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/***********************************************************************
 *           CoGetState16 [COMPOBJ.115]
 */
HRESULT WINAPI CoGetState16(LPDWORD state)
{
    FIXME(ole, "(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}
/***********************************************************************
 *           CoSetState32 [COM32.42]
 */
HRESULT WINAPI CoSetState32(LPDWORD state)
{
    FIXME(ole, "(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}
