/*
 * Win32 miscellaneous functions
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 */

/* Misc. new functions - they should be moved into appropriate files
at a later date. */

#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "windef.h"
#include "winerror.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);
DECLARE_DEBUG_CHANNEL(debug);


/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.564)
 */
BOOL WINAPI QueryPerformanceCounter(PLARGE_INTEGER counter)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);
    counter->s.LowPart = tv.tv_usec+tv.tv_sec*1000000;
    counter->s.HighPart = 0;
    return TRUE;
}

/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.565)
 */
BOOL WINAPI QueryPerformanceFrequency(PLARGE_INTEGER frequency)
{
	frequency->s.LowPart	= 1000000;
	frequency->s.HighPart	= 0;
	return TRUE;
}

/****************************************************************************
 *		FlushInstructionCache (KERNEL32.261)
 */
BOOL WINAPI FlushInstructionCache(DWORD x,DWORD y,DWORD z) {
	FIXME_(debug)("(0x%08lx,0x%08lx,0x%08lx): stub\n",x,y,z);
	return TRUE;
}

/***********************************************************************
 *           CreateNamedPipeA   (KERNEL32.168)
 */
HANDLE WINAPI CreateNamedPipeA (LPCSTR lpName, DWORD dwOpenMode,
				  DWORD dwPipeMode, DWORD nMaxInstances,
				  DWORD nOutBufferSize, DWORD nInBufferSize,
				  DWORD nDefaultTimeOut,
				  LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  FIXME("(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
        debugstr_a(lpName), dwOpenMode, dwPipeMode, nMaxInstances,
        nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
  /* if (nMaxInstances > PIPE_UNLIMITED_INSTANCES) {
    SetLastError (ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
  } */

  SetLastError (ERROR_UNKNOWN);
  return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           CreateNamedPipeW   (KERNEL32.169)
 */
HANDLE WINAPI CreateNamedPipeW (LPCWSTR lpName, DWORD dwOpenMode,
				  DWORD dwPipeMode, DWORD nMaxInstances,
				  DWORD nOutBufferSize, DWORD nInBufferSize,
				  DWORD nDefaultTimeOut,
				  LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  FIXME("(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
        debugstr_w(lpName), dwOpenMode, dwPipeMode, nMaxInstances,
        nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);

  SetLastError (ERROR_UNKNOWN);
  return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *           GetSystemPowerStatus      (KERNEL32.621)
 */
BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS sps_ptr)
{
    return FALSE;   /* no power management support */
}


/***********************************************************************
 *           SetSystemPowerState      (KERNEL32.630)
 */
BOOL WINAPI SetSystemPowerState(BOOL suspend_or_hibernate,
                                  BOOL force_flag)
{
    /* suspend_or_hibernate flag: w95 does not support
       this feature anyway */

    for ( ;0; )
    {
        if ( force_flag )
        {
        }
        else
        {
        }
    }
    return TRUE;
}


/******************************************************************************
 * CreateMailslot32A [KERNEL32.164]
 */
HANDLE WINAPI CreateMailslotA( LPCSTR lpName, DWORD nMaxMessageSize,
                                   DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa)
{
    FIXME("(%s,%ld,%ld,%p): stub\n", debugstr_a(lpName),
          nMaxMessageSize, lReadTimeout, sa);
    return 1;
}


/******************************************************************************
 * CreateMailslot32W [KERNEL32.165]  Creates a mailslot with specified name
 * 
 * PARAMS
 *    lpName          [I] Pointer to string for mailslot name
 *    nMaxMessageSize [I] Maximum message size
 *    lReadTimeout    [I] Milliseconds before read time-out
 *    sa              [I] Pointer to security structure
 *
 * RETURNS
 *    Success: Handle to mailslot
 *    Failure: INVALID_HANDLE_VALUE
 */
HANDLE WINAPI CreateMailslotW( LPCWSTR lpName, DWORD nMaxMessageSize,
                                   DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa )
{
    FIXME("(%s,%ld,%ld,%p): stub\n", debugstr_w(lpName), 
          nMaxMessageSize, lReadTimeout, sa);
    return 1;
}


/******************************************************************************
 * GetMailslotInfo [KERNEL32.347]  Retrieves info about specified mailslot
 *
 * PARAMS
 *    hMailslot        [I] Mailslot handle
 *    lpMaxMessageSize [O] Address of maximum message size
 *    lpNextSize       [O] Address of size of next message
 *    lpMessageCount   [O] Address of number of messages
 *    lpReadTimeout    [O] Address of read time-out
 * 
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetMailslotInfo( HANDLE hMailslot, LPDWORD lpMaxMessageSize,
                               LPDWORD lpNextSize, LPDWORD lpMessageCount,
                               LPDWORD lpReadTimeout )
{
    FIXME("(%04x): stub\n",hMailslot);
    if (lpMaxMessageSize) *lpMaxMessageSize = (DWORD)NULL;
    if (lpNextSize) *lpNextSize = (DWORD)NULL;
    if (lpMessageCount) *lpMessageCount = (DWORD)NULL;
    if (lpReadTimeout) *lpReadTimeout = (DWORD)NULL;
    return TRUE;
}


/******************************************************************************
 * GetCompressedFileSize32A [KERNEL32.291]
 *
 * NOTES
 *    This should call the W function below
 */
DWORD WINAPI GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh)
{
    FIXME("(...): stub\n");
    return 0xffffffff;
}


/******************************************************************************
 * GetCompressedFileSize32W [KERNEL32.292]  
 * 
 * RETURNS
 *    Success: Low-order doubleword of number of bytes
 *    Failure: 0xffffffff
 */
DWORD WINAPI GetCompressedFileSizeW(
    LPCWSTR lpFileName,     /* [in]  Pointer to name of file */
    LPDWORD lpFileSizeHigh) /* [out] Receives high-order doubleword of size */
{
    FIXME("(%s,%p): stub\n",debugstr_w(lpFileName),lpFileSizeHigh);
    return 0xffffffff;
}


/******************************************************************************
 * GetProcessWindowStation [USER32.280]  Returns handle of window station
 *
 * NOTES
 *    Docs say the return value is HWINSTA
 *
 * RETURNS
 *    Success: Handle to window station associated with calling process
 *    Failure: NULL
 */
DWORD WINAPI GetProcessWindowStation(void)
{
    FIXME("(void): stub\n");
    return 1;
}


/******************************************************************************
 * GetThreadDesktop [USER32.295]  Returns handle to desktop
 *
 * NOTES
 *    Docs say the return value is HDESK
 *
 * PARAMS
 *    dwThreadId [I] Thread identifier
 *
 * RETURNS
 *    Success: Handle to desktop associated with specified thread
 *    Failure: NULL
 */
DWORD WINAPI GetThreadDesktop( DWORD dwThreadId )
{
    FIXME("(%lx): stub\n",dwThreadId);
    return 1;
}


/******************************************************************************
 * SetDebugErrorLevel [USER32.475]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 */
VOID WINAPI SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%ld): stub\n", dwLevel);
}


/******************************************************************************
 * SetComputerName32A [KERNEL32.621]  
 */
BOOL WINAPI SetComputerNameA( LPCSTR lpComputerName )
{
    LPWSTR lpComputerNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpComputerName);
    BOOL ret = SetComputerNameW(lpComputerNameW);
    HeapFree(GetProcessHeap(),0,lpComputerNameW);
    return ret;
}


/******************************************************************************
 * SetComputerName32W [KERNEL32.622]
 *
 * PARAMS
 *    lpComputerName [I] Address of new computer name
 * 
 * RETURNS STD
 */
BOOL WINAPI SetComputerNameW( LPCWSTR lpComputerName )
{
    FIXME("(%s): stub\n", debugstr_w(lpComputerName));
    return TRUE;
}


BOOL WINAPI EnumPortsA(LPSTR name,DWORD level,LPBYTE ports,DWORD bufsize,LPDWORD bufneeded,LPDWORD bufreturned) {
	FIXME("(%s,%ld,%p,%ld,%p,%p), stub!\n",name,level,ports,bufsize,bufneeded,bufreturned);
	return FALSE;
}

/******************************************************************************
 * IsDebuggerPresent [KERNEL32.827]
 *
 */
BOOL WINAPI IsDebuggerPresent() {
	FIXME(" ... no debuggers yet, returning FALSE.\n");
	return FALSE; 
}

/******************************************************************************
 * OpenDesktop32A [USER32.408]
 *
 * NOTES
 *    Return type should be HDESK
 *
 *    Not supported on Win9x - returns NULL and calls SetLastError.
 */
HANDLE WINAPI OpenDesktopA( LPCSTR lpszDesktop, DWORD dwFlags, 
                                BOOL fInherit, DWORD dwDesiredAccess )
{
    FIXME("(%s,%lx,%i,%lx): stub\n",debugstr_a(lpszDesktop),dwFlags,
          fInherit,dwDesiredAccess);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


BOOL WINAPI SetUserObjectInformationA( HANDLE hObj, int nIndex, 
                                           LPVOID pvInfo, DWORD nLength )
{
    FIXME("(%x,%d,%p,%lx): stub\n",hObj,nIndex,pvInfo,nLength);
    return TRUE;
}


BOOL WINAPI SetThreadDesktop( HANDLE hDesktop )
{
    FIXME("(%x): stub\n",hDesktop);
    return TRUE;
}

HANDLE WINAPI CreateIoCompletionPort(HANDLE hFileHandle,
HANDLE hExistingCompletionPort, DWORD dwCompletionKey,
DWORD dwNumberOfConcurrentThreads)
{
    FIXME("(%04x, %04x, %08lx, %08lx): stub.\n", hFileHandle, hExistingCompletionPort, dwCompletionKey, dwNumberOfConcurrentThreads);
    return (HANDLE)NULL;
}


/******************************************************************************
 *                    GetProcessDefaultLayout [USER32.802]
 *
 * Gets the default layout for parentless windows.
 * Right now, just returns 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI GetProcessDefaultLayout( DWORD *pdwDefaultLayout )
{
    if ( !pdwDefaultLayout ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
     }
    FIXME( "( %p ): No BiDi\n", pdwDefaultLayout );
    *pdwDefaultLayout = 0;
    return TRUE;
}


/******************************************************************************
 *                    SetProcessDefaultLayout [USER32.803]
 *
 * Sets the default layout for parentless windows.
 * Right now, only accepts 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI SetProcessDefaultLayout( DWORD dwDefaultLayout )
{
    if ( dwDefaultLayout == 0 )
        return TRUE;
    FIXME( "( %08lx ): No BiDi\n", dwDefaultLayout );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

