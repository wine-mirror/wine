/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "compobj.h"
#include "ole.h"
#include "ole2.h"
#include "stddebug.h"
#include "debug.h"

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
 */
HRESULT WINAPI CoInitialize(LPVOID lpReserved)
{
	dprintf_ole(stdnimp,"CoInitialize\n");
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
 *           CoDisconnectObject
 */
OLESTATUS WINAPI CoDisconnectObject(
	LPUNKNOWN lpUnk,
	DWORD reserved)
{
    dprintf_ole(stdnimp,"CoDisconnectObject:%p %lx\n",lpUnk,reserved);
    return OLE_OK;
}
