/*
 *	OLE2 library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole2.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           OleBuildVersion     [OLE.1]
 */
DWORD WINAPI OleBuildVersion()
{
    dprintf_ole(stddeb,"OleBuildVersion()\n");
    return (rmm<<16)+rup;
}

/***********************************************************************
 *           OleInitialize       [OLE2.2]
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
{
    dprintf_ole(stdnimp,"OleInitialize\n");
	return S_OK;
}

/***********************************************************************
 *           OleUnitialize       [OLE2.3]
 */
void WINAPI OleUninitialize()
{
    dprintf_ole(stdnimp,"OleUninitialize()\n");
}

/***********************************************************************
 *           OleFlushClipboard   [OLE2.76]
 */
HRESULT WINAPI OleFlushClipboard()
{
    return S_OK;
}

