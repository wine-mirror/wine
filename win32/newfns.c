/*
 * Win32 miscellaneous functions
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Misc. new functions - they should be moved into appropriate files
at a later date. */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);


/****************************************************************************
 *		FlushInstructionCache (KERNEL32.@)
 */
BOOL WINAPI FlushInstructionCache(HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize)
{
    if (GetVersion() & 0x80000000) return TRUE; /* not NT, always TRUE */
    FIXME("(%p,%p,0x%08lx): stub\n",hProcess, lpBaseAddress, dwSize);
    return TRUE;
}


/******************************************************************************
 * GetCompressedFileSizeA [KERNEL32.@]
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
 * GetCompressedFileSizeW [KERNEL32.@]
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
 *		CreateIoCompletionPort (KERNEL32.@)
 */
HANDLE WINAPI CreateIoCompletionPort(HANDLE hFileHandle,
HANDLE hExistingCompletionPort, DWORD dwCompletionKey,
DWORD dwNumberOfConcurrentThreads)
{
    FIXME("(%p, %p, %08lx, %08lx): stub.\n", hFileHandle, hExistingCompletionPort, dwCompletionKey, dwNumberOfConcurrentThreads);
    return NULL;
}

/******************************************************************************
 *		GetQueuedCompletionStatus (KERNEL32.@)
 */
BOOL WINAPI GetQueuedCompletionStatus(
    HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred,
    LPDWORD lpCompletionKey, LPOVERLAPPED *lpOverlapped, DWORD dwMilliseconds
) {
    FIXME("(%p,%p,%p,%p,%ld), stub!\n",CompletionPort,lpNumberOfBytesTransferred,lpCompletionKey,lpOverlapped,dwMilliseconds);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           Beep   (KERNEL32.@)
 */
BOOL WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    static const char beep = '\a';
    /* dwFreq and dwDur are ignored by Win95 */
    if (isatty(2)) write( 2, &beep, 1 );
    return TRUE;
}
