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
#include "windows.h"
#include "winnt.h"
#include "winerror.h"
#include "heap.h"
#include "debug.h"
#include "debugstr.h"

/****************************************************************************
 *		UTRegister (KERNEL32.697)
 */
BOOL32 WINAPI UTRegister(HMODULE32 hModule,
                      LPSTR lpsz16BITDLL,
                      LPSTR lpszInitName,
                      LPSTR lpszProcName,
                      /*UT32PROC*/ LPVOID *ppfn32Thunk,
                      /*FARPROC*/ LPVOID pfnUT32CallBack,
                      LPVOID lpBuff)
{
    FIXME(updown, "(%#x,...): stub\n",hModule);
    return TRUE;
}

/****************************************************************************
 *		UTUnRegister (KERNEL32.698)
 */
BOOL32 WINAPI UTUnRegister(HMODULE32 hModule)
{
    FIXME(updown, "(%#x...): stub\n", hModule);
    return TRUE;
}


/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.564)
 */
BOOL32 WINAPI QueryPerformanceCounter(LPLARGE_INTEGER counter)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);
    counter->LowPart = tv.tv_usec+tv.tv_sec*1000000;
    counter->HighPart = 0;
    return TRUE;
}

HANDLE32 WINAPI FindFirstChangeNotification32A(LPCSTR lpPathName,BOOL32 bWatchSubtree,DWORD dwNotifyFilter) {
	FIXME(file,"(%s,%d,%08lx): stub\n",
	      lpPathName,bWatchSubtree,dwNotifyFilter);
	return 0xcafebabe;
}

HANDLE32 WINAPI FindFirstChangeNotification32W(LPCWSTR lpPathName,BOOL32 bWatchSubtree,DWORD dwNotifyFilter) {
	FIXME(file,"(%s,%d,%08lx): stub\n",
	      debugstr_w(lpPathName),bWatchSubtree,dwNotifyFilter);
	return 0xcafebabe;
}

BOOL32 WINAPI FindNextChangeNotification(HANDLE32 fcnhandle) {
	FIXME(file,"(%08x): stub!\n",fcnhandle);
	return FALSE;
}

BOOL32 WINAPI FindCloseChangeNotification(HANDLE32 fcnhandle) {
	FIXME(file,"(%08x): stub!\n",fcnhandle);
	return FALSE;
}

/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.565)
 */
BOOL32 WINAPI QueryPerformanceFrequency(LPLARGE_INTEGER frequency)
{
	frequency->LowPart	= 1000000;
	frequency->HighPart	= 0;
	return TRUE;
}

/****************************************************************************
 *		FlushInstructionCache (KERNEL32.261)
 */
BOOL32 WINAPI FlushInstructionCache(DWORD x,DWORD y,DWORD z) {
	FIXME(debug,"(0x%08lx,0x%08lx,0x%08lx): stub\n",x,y,z);
	return TRUE;
}

/***********************************************************************
 *           CreateNamedPipeA   (KERNEL32.168)
 */
HANDLE32 WINAPI CreateNamedPipeA (LPCSTR lpName, DWORD dwOpenMode,
				  DWORD dwPipeMode, DWORD nMaxInstances,
				  DWORD nOutBufferSize, DWORD nInBufferSize,
				  DWORD nDefaultTimeOut,
				  LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  FIXME (win32, "(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
	 debugstr_a(lpName), dwOpenMode, dwPipeMode, nMaxInstances,
	 nOutBufferSize, nInBufferSize, nDefaultTimeOut, 
	 lpSecurityAttributes);
  /* if (nMaxInstances > PIPE_UNLIMITED_INSTANCES) {
    SetLastError (ERROR_INVALID_PARAMETER);
    return INVALID_HANDLE_VALUE;
  } */

  SetLastError (ERROR_UNKNOWN);
  return INVALID_HANDLE_VALUE32;
}

/***********************************************************************
 *           CreateNamedPipeW   (KERNEL32.169)
 */
HANDLE32 WINAPI CreateNamedPipeW (LPCWSTR lpName, DWORD dwOpenMode,
				  DWORD dwPipeMode, DWORD nMaxInstances,
				  DWORD nOutBufferSize, DWORD nInBufferSize,
				  DWORD nDefaultTimeOut,
				  LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  FIXME (win32, "(Name=%s, OpenMode=%#08lx, dwPipeMode=%#08lx, MaxInst=%ld, OutBSize=%ld, InBuffSize=%ld, DefTimeOut=%ld, SecAttr=%p): stub\n",
	 debugstr_w(lpName), dwOpenMode, dwPipeMode, nMaxInstances,
	 nOutBufferSize, nInBufferSize, nDefaultTimeOut, 
	 lpSecurityAttributes);

  SetLastError (ERROR_UNKNOWN);
  return INVALID_HANDLE_VALUE32;
}

/***********************************************************************
 *	CreatePipe (KERNEL32.170)
 */

BOOL32 WINAPI CreatePipe(PHANDLE hReadPipe,
                         PHANDLE hWritePipe,
                         LPSECURITY_ATTRIBUTES lpPipeAttributes,
                         DWORD nSize)
{
  FIXME (win32,"ReadPipe=%p WritePipe=%p SecAttr=%p Size=%ld",
               hReadPipe,hWritePipe,lpPipeAttributes,nSize);
  SetLastError(ERROR_UNKNOWN);
  return FALSE;
}

/***********************************************************************
 *           GetSystemPowerStatus      (KERNEL32.621)
 */
BOOL32 WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS sps_ptr)
{
    return FALSE;   /* no power management support */
}


/***********************************************************************
 *           SetSystemPowerState      (KERNEL32.630)
 */
BOOL32 WINAPI SetSystemPowerState(BOOL32 suspend_or_hibernate,
                                  BOOL32 force_flag)
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
HANDLE32 WINAPI CreateMailslot32A( LPCSTR lpName, DWORD nMaxMessageSize,
                                   DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa)
{
    FIXME(win32, "(%s,%ld,%ld,%p): stub\n", debugstr_a(lpName),
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
HANDLE32 WINAPI CreateMailslot32W( LPCWSTR lpName, DWORD nMaxMessageSize,
                                   DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa )
{
    FIXME(win32, "(%s,%ld,%ld,%p): stub\n", debugstr_w(lpName), 
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
BOOL32 WINAPI GetMailslotInfo( HANDLE32 hMailslot, LPDWORD lpMaxMessageSize,
                               LPDWORD lpNextSize, LPDWORD lpMessageCount,
                               LPDWORD lpReadTimeout )
{
    FIXME(win32, "(%d): stub\n",hMailslot);
    *lpMaxMessageSize = NULL;
    *lpNextSize = NULL;
    *lpMessageCount = NULL;
    *lpReadTimeout = NULL;
    return TRUE;
}


/******************************************************************************
 * GetCompressedFileSize32A [KERNEL32.291]
 *
 * NOTES
 *    This should call the W function below
 */
DWORD WINAPI GetCompressedFileSize32A(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh)
{
    FIXME(win32, "(...): stub\n");
    return 0xffffffff;
}


/******************************************************************************
 * GetCompressedFileSize32W [KERNEL32.292]  
 * 
 * RETURNS
 *    Success: Low-order doubleword of number of bytes
 *    Failure: 0xffffffff
 */
DWORD WINAPI GetCompressedFileSize32W(
    LPCWSTR lpFileName,     /* [in]  Pointer to name of file */
    LPDWORD lpFileSizeHigh) /* [out] Receives high-order doubleword of size */
{
    FIXME(win32, "(%s,%p): stub\n",debugstr_w(lpFileName),lpFileSizeHigh);
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
    FIXME(win32, "(void): stub\n");
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
    FIXME(win32, "(%lx): stub\n",dwThreadId);
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
    FIXME(win32, "(%ld): stub\n", dwLevel);
}


/******************************************************************************
 * WaitForDebugEvent [KERNEL32.720]
 * Waits for a debugging event to occur in a process being debugged
 *
 * PARAMS
 *    lpDebugEvent   [I] Address of structure for event information
 *    dwMilliseconds [I] Number of milliseconds to wait for event
 *
 * RETURNS STD
 */
BOOL32 WINAPI WaitForDebugEvent( LPDEBUG_EVENT lpDebugEvent, 
                                 DWORD dwMilliseconds )
{
    FIXME(win32, "(%p,%ld): stub\n", lpDebugEvent, dwMilliseconds);
    return TRUE;
}


/******************************************************************************
 * SetComputerName32A [KERNEL32.621]  
 */
BOOL32 WINAPI SetComputerName32A( LPCSTR lpComputerName )
{
    LPWSTR lpComputerNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpComputerName);
    BOOL32 ret = SetComputerName32W(lpComputerNameW);
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
BOOL32 WINAPI SetComputerName32W( LPCWSTR lpComputerName )
{
    FIXME(win32, "(%s): stub\n", debugstr_w(lpComputerName));
    return TRUE;
}


BOOL32 WINAPI EnumPorts32A(LPSTR name,DWORD level,LPBYTE ports,DWORD bufsize,LPDWORD bufneeded,LPDWORD bufreturned) {
	FIXME(win32,"(%s,%ld,%p,%ld,%p,%p), stub!\n",name,level,ports,bufsize,bufneeded,bufreturned);
	return FALSE;
}

/******************************************************************************
 * IsDebuggerPresent [KERNEL32.827]
 *
 */
BOOL32 WINAPI IsDebuggerPresent() {
	FIXME(win32," ... no debuggers yet, returning FALSE.\n");
	return FALSE; 
}


/******************************************************************************
 * OpenDesktop32A [USER32.408]
 *
 * NOTES
 *    Return type should be HDESK
 */
HANDLE32 WINAPI OpenDesktop32A( LPCSTR lpszDesktop, DWORD dwFlags, 
                                BOOL32 fInherit, DWORD dwDesiredAccess )
{
    FIXME(win32,"(%s,%lx,%i,%lx): stub\n",debugstr_a(lpszDesktop),dwFlags,
          fInherit,dwDesiredAccess);
    return 1;
}


BOOL32 WINAPI SetUserObjectInformation32A( HANDLE32 hObj, int nIndex, 
                                           LPVOID pvInfo, DWORD nLength )
{
    FIXME(win32,"(%x,%d,%p,%lx): stub\n",hObj,nIndex,pvInfo,nLength);
    return TRUE;
}


BOOL32 WINAPI SetThreadDesktop( HANDLE32 hDesktop )
{
    FIXME(win32,"(%x): stub\n",hDesktop);
    return TRUE;
}

