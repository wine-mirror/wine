/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "stddebug.h"
#include "debug.h"
#include "compobj.h"

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
 *           CLSIDFromString [COMPOBJ.19]
 */

OLESTATUS WINAPI StringFromCLSID(const CLSID *id, LPSTR idstr)
{
  static const char *hex = "0123456789ABCDEF";
  char *s;
  int	i;

  sprintf(idstr, "{%08lx-%04x-%04x-%2x%2x-",
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

