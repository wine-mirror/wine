/*
 *	OLE2 library
 *
 *	Copyright 1995	Martin von Loewis
 */

/*	At the moment, these are only empty stubs.
 */

#include "windows.h"
#include "ole2.h"
#include "process.h"
#include "debug.h"

/***********************************************************************
 *           OleBuildVersion     [OLE.1]
 */
DWORD WINAPI OleBuildVersion()
{
    dprintf_info(ole,"OleBuildVersion()\n");
    return (rmm<<16)+rup;
}

/***********************************************************************
 *           OleInitialize       (OLE2.2) (OLE32.108)
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
{
    dprintf_fixme(ole,"OleInitialize - stub\n");
    return S_OK;
}

DWORD WINAPI CoGetCurrentProcess() {
	return PROCESS_Current();
}

/***********************************************************************
 *           OleUnitialize       (OLE2.3) (OLE32.131)
 */
void WINAPI OleUninitialize(void)
{
    dprintf_fixme(ole,"OleUninitialize() - stub\n");
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
HRESULT WINAPI CoRegisterMessageFilter32(
    LPMESSAGEFILTER lpMessageFilter,	/* Pointer to interface */
    LPMESSAGEFILTER *lplpMessageFilter	/* Indirect pointer to prior instance if non-NULL */
) {
    dprintf_fixme(ole,"CoRegisterMessageFilter() - stub\n");
    if (lplpMessageFilter) {
	*lplpMessageFilter = NULL;
    }
    return S_OK;
}

/***********************************************************************
 *           OleInitializeWOW (OLE32.27)
 */
HRESULT WINAPI OleInitializeWOW(DWORD x) {
        fprintf(stderr,"OleInitializeWOW(0x%08lx),stub!\n",x);
        return 0;
}
