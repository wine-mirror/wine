/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "ole.h"
#include "ole2.h"
#include "stddebug.h"
#include "debug.h"

DWORD currentMalloc=0;

/***********************************************************************
 *           CoBuildVersion [COMPOBJ.1]
 */
DWORD CoBuildVersion()
{
	dprintf_ole(stddeb,"CoBuildVersion()\n");
	return (rmm<<16)+rup;
}

/***********************************************************************
 *           CoInitialize	[COMPOBJ.2]
 * lpReserved is an IMalloc pointer in 16bit OLE. We just stored it as-is.
 */
HRESULT CoInitialize(DWORD lpReserved)
{
	dprintf_ole(stdnimp,"CoInitialize\n");
	/* remember the LPMALLOC, maybe somebody wants to read it later on */
	currentMalloc = lpReserved;
	return S_OK;
}

/***********************************************************************
 *           CoUnitialize   [COMPOBJ.3]
 */
void CoUnitialize()
{
	dprintf_ole(stdnimp,"CoUnitialize()\n");
}

/***********************************************************************
 *           CoGetMalloc    [COMPOBJ.4]
 */
HRESULT CoGetMalloc(DWORD dwMemContext, DWORD * lpMalloc)
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
OLESTATUS CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    dprintf_ole(stdnimp,"CoDisconnectObject:%p %lx\n",lpUnk,reserved);
    return OLE_OK;
}
