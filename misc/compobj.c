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
#include "stddebug.h"
#include "debug.h"

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
