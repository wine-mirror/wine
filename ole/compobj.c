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
#include "stddebug.h"
#include "debug.h"
#include "file.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "ddraw.h"
#include "dsound.h"
#include "dinput.h"
#include "d3d.h"

DWORD currentMalloc=0;

/***********************************************************************
 *           CoBuildVersion [COMPOBJ.1]
 */
DWORD WINAPI CoBuildVersion()
{
	dprintf_ole(stddeb,"CoBuildVersion()\n");
	return (rmm<<16)+rup;
}

/***********************************************************************
 *           CoInitialize	[COMPOBJ.2]
 * lpReserved is an IMalloc pointer in 16bit OLE. We just stored it as-is.
 */
HRESULT WINAPI CoInitialize(DWORD lpReserved)
{
	dprintf_ole(stdnimp,"CoInitialize\n");
	/* remember the LPMALLOC, maybe somebody wants to read it later on */
	currentMalloc = lpReserved;
	return S_OK;
}

/***********************************************************************
 *           CoUnitialize   [COMPOBJ.3]
 */
void WINAPI CoUnitialize()
{
	dprintf_ole(stdnimp,"CoUnitialize()\n");
}

/***********************************************************************
 *           CoGetMalloc    [COMPOBJ.4]
 */
HRESULT WINAPI CoGetMalloc(DWORD dwMemContext, DWORD * lpMalloc)
{
	if(currentMalloc)
	{
		*lpMalloc = currentMalloc;
		return S_OK;
	}
	*lpMalloc = 0;
	/* 16-bit E_NOTIMPL */
	return 0x80000001L;
}

/***********************************************************************
 *           CoDisconnectObject
 */
OLESTATUS WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    dprintf_ole(stdnimp,"CoDisconnectObject:%p %lx\n",lpUnk,reserved);
    return OLE_OK;
}

/***********************************************************************
 *           IsEqualGUID [COMPOBJ.18]
 */
BOOL16 WINAPI IsEqualGUID(GUID* g1, GUID* g2)
{
    return !memcmp( g1, g2, sizeof(GUID) );
}

/***********************************************************************
 *           CLSIDFromString [COMPOBJ.20]
 */

/* Class id: DWORD-WORD-WORD-BYTES[2]-BYTES[6] */

OLESTATUS WINAPI CLSIDFromString(const LPCSTR idstr, CLSID *id)
{
  BYTE *s = (BYTE *) idstr;
  BYTE *p;
  int	i;
  BYTE table[256];

  dprintf_ole(stddeb,"ClsIDFromString() %s -> %p\n", idstr, id);

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

  return OLE_OK;
}

/***********************************************************************
 *           StringFromCLSID [COMPOBJ.19]
 */
OLESTATUS WINAPI StringFromCLSID(const CLSID *id, LPSTR idstr)
{
  static const char *hex = "0123456789ABCDEF";
  char *s;
  int	i;

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

  dprintf_ole(stddeb,"StringFromClsID: %p->%s\n", id, idstr);

  return OLE_OK;
}

/***********************************************************************
 *           CLSIDFromProgID [COMPOBJ.61]
 */

OLESTATUS WINAPI CLSIDFromProgID(LPCSTR progid,LPCLSID riid)
{
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
	return CLSIDFromString(buf2,riid);
}

OLESTATUS WINAPI LookupETask(LPVOID p1,LPVOID p2) {
	fprintf(stderr,"LookupETask(%p,%p),stub!\n",p1,p2);
	return 0;
}

OLESTATUS WINAPI CallObjectInWOW(LPVOID p1,LPVOID p2) {
	fprintf(stderr,"CallObjectInWOW(%p,%p),stub!\n",p1,p2);
	return 0;
}

/***********************************************************************
 *		CoRegisterClassObject [COMPOBJ.5]
 */
OLESTATUS WINAPI CoRegisterClassObject(
	REFCLSID rclsid, LPUNKNOWN pUnk,DWORD dwClsContext,DWORD flags,
	LPDWORD lpdwRegister
) {
	char	buf[80];

	StringFromCLSID(rclsid,buf);

	fprintf(stderr,"CoRegisterClassObject(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		buf,pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}

/***********************************************************************
 *		CoRegisterClassObject [COMPOBJ.27]
 */
OLESTATUS WINAPI CoRegisterMessageFilter16(
	LPMESSAGEFILTER lpMessageFilter,LPMESSAGEFILTER *lplpMessageFilter
) {
	fprintf(stderr,"CoRegisterMessageFilter(%p,%p),stub!\n",
		lpMessageFilter,lplpMessageFilter
	);
	return 0;
}

/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13, OLE32.7]
 */
HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
				DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	fprintf(stderr, "CoCreateInstance(): stub !\n");
	*ppv = NULL;
	return S_OK;
}

/***********************************************************************
 *           CoFreeUnusedLibraries [COMPOBJ.17]
 */
void WINAPI CoFreeUnusedLibraries()
{
	fprintf(stderr, "CoFreeUnusedLibraries(): stub !\n");
}

/***********************************************************************
 *           CoFileTimeNow [COMPOBJ.82, OLE32.10]
 *
 *	stores the current system time in lpFileTime
 */
HRESULT WINAPI CoFileTimeNow(FILETIME *lpFileTime)
{
	DOSFS_UnixTimeToFileTime(time(NULL), lpFileTime, 0);
	return S_OK;
}
