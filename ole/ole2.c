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
 *           OleInitialize       (OLE2.2) (OLE32.108)
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
{
    dprintf_ole(stdnimp,"OleInitialize\n");
	return S_OK;
}

/***********************************************************************
 *           OleUnitialize       (OLE2.3) (OLE32.131)
 */
void WINAPI OleUninitialize(void)
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

/***********************************************************************
 *           CoRegisterMessageFilter   [OLE32.38]
 */
HRESULT CoRegisterMessageFilter(
    LPMESSAGEFILTER lpMessageFilter,	/* Pointer to interface */
    LPMESSAGEFILTER *lplpMessageFilter	/* Indirect pointer to prior instance if non-NULL */
) {
    dprintf_ole(stdnimp,"CoRegisterMessageFilter()\n");
    if (lplpMessageFilter) {
	*lplpMessageFilter = NULL;
    }
    return S_OK;
}
