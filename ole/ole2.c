/*
 *	OLE2 library
 *
 *	Copyright 1995	Martin von Loewis
 */

#include "windows.h"
#include "winerror.h"
#include "ole2.h"
#include "process.h"
#include "debug.h"
#include "wine/obj_base.h"
#include "wine/obj_clientserver.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"

/******************************************************************************
 *		OleBuildVersion	[OLE2.1]
 */
DWORD WINAPI OleBuildVersion(void)
{
    TRACE(ole,"(void)\n");
    return (rmm<<16)+rup;
}

/***********************************************************************
 *           OleInitialize       (OLE2.2) (OLE32.108)
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
{
    FIXME(ole,"OleInitialize - stub\n");
    return S_OK;
}

/******************************************************************************
 *		CoGetCurrentProcess	[COMPOBJ.34] [OLE2.2][OLE32.108]
 *
 * NOTES
 *   Is DWORD really the correct return type for this function?
 */
DWORD WINAPI CoGetCurrentProcess(void) {
	return (DWORD)PROCESS_Current();
}

/******************************************************************************
 *		OleUninitialize	[OLE2.3] [OLE32.131]
 */
void WINAPI OleUninitialize(void)
{
    FIXME(ole,"stub\n");
}

/***********************************************************************
 *           OleFlushClipboard   [OLE2.76]
 */
HRESULT WINAPI OleFlushClipboard(void)
{
    return S_OK;
}

/***********************************************************************
 *           OleSetClipboard     [OLE32.127]
 */
HRESULT WINAPI OleSetClipboard(LPVOID pDataObj)
{
    FIXME(ole,"(%p), stub!\n", pDataObj);
    return S_OK;
}

/******************************************************************************
 *		CoRegisterMessageFilter32	[OLE32.38]
 */
HRESULT WINAPI CoRegisterMessageFilter32(
    LPMESSAGEFILTER lpMessageFilter,	/* Pointer to interface */
    LPMESSAGEFILTER *lplpMessageFilter	/* Indirect pointer to prior instance if non-NULL */
) {
    FIXME(ole,"stub\n");
    if (lplpMessageFilter) {
	*lplpMessageFilter = NULL;
    }
    return S_OK;
}

/******************************************************************************
 *		OleInitializeWOW	[OLE32.109]
 */
HRESULT WINAPI OleInitializeWOW(DWORD x) {
        FIXME(ole,"(0x%08lx),stub!\n",x);
        return 0;
}

/******************************************************************************
 *		GetRunningObjectTable16	[OLE2.30]
 */
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPVOID *pprot) {
	FIXME(ole,"(%ld,%p),stub!\n",reserved,pprot);
	return E_FAIL;
}


/***********************************************************************
 *           GetRunningObjectTable32 (OLE2.73)
 */
HRESULT WINAPI GetRunningObjectTable32(DWORD reserved, LPVOID *pprot) {
	FIXME(ole,"(%ld,%p),stub!\n",reserved,pprot);
	return E_FAIL;
}

/***********************************************************************
 *           RegisterDragDrop16 (OLE2.35)
 */
HRESULT WINAPI RegisterDragDrop16(
	HWND16 hwnd,
	LPDROPTARGET pDropTarget
) {
	FIXME(ole,"(0x%04x,%p),stub!\n",hwnd,pDropTarget);
	return S_OK;
}

/***********************************************************************
 *           RegisterDragDrop32 (OLE32.139)
 */
HRESULT WINAPI RegisterDragDrop32(
	HWND32 hwnd,
	LPDROPTARGET pDropTarget
) {
	FIXME(ole,"(0x%04x,%p),stub!\n",hwnd,pDropTarget);
	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop16 (OLE2.36)
 */
HRESULT WINAPI RevokeDragDrop16(
	HWND16 hwnd
) {
	FIXME(ole,"(0x%04x),stub!\n",hwnd);
	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop32 (OLE32.141)
 */
HRESULT WINAPI RevokeDragDrop32(
	HWND32 hwnd
) {
	FIXME(ole,"(0x%04x),stub!\n",hwnd);
	return S_OK;
}

/***********************************************************************
 *           OleRegGetUserType (OLE32.122)
 */
HRESULT WINAPI OleRegGetUserType32( 
	REFCLSID clsid, 
	DWORD dwFormOfType,
	LPOLESTR32* pszUserType)
{
	FIXME(ole,",stub!\n");
	return S_OK;
}

/***********************************************************************
 * CreateBindCtx32 [OLE32.52]
 */
HRESULT WINAPI CreateBindCtx32 (DWORD reserved,	LPVOID *ppbc)
{
    FIXME(ole,"(0x%08lx %p): stub!\n", reserved, ppbc);
    *ppbc = 0;
/*    return S_OK; */
    return E_OUTOFMEMORY;
}
