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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);
WINE_DECLARE_DEBUG_CHANNEL(debug);


static BOOL	QUERYPERF_Initialized		= 0;
#if defined(__i386__) && defined(__GNUC__)
static BOOL	QUERYPERF_RDTSC_Use		= 0;
static LONGLONG	QUERYPERF_RDTSC_Frequency	= 0;
#endif

static void QUERYPERF_Init(void)
{
#if defined(__i386__) && defined(__GNUC__)
    /* We are running on i386 and compiling on GCC.
     * Do a runtime check to see if we have the rdtsc instruction available
     */
    FILE		*fp;
    char		line[256], *s, *value;
    double		cpuMHz;

    TRACE("()\n");

    if (IsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE ))
    {
	/* rdtsc is available.  However, in order to use it
	 * we also need to be able to get the processor's
	 * speed.  Currently we do this by reading /proc/cpuinfo
	 * which makes it Linux-specific.
	 */

	TRACE("rdtsc available\n");

	fp = fopen( "/proc/cpuinfo", "r" );
	if (fp)
	{
	    while(fgets( line, sizeof(line), fp ))
	    {
		/* NOTE: the ':' is the only character we can rely on */
		if (!(value = strchr( line, ':' )))
		    continue;

		/* terminate the valuename */
		*value++ = '\0';
		/* skip any leading spaces */
		while (*value == ' ') value++;
		if ((s = strchr( value, '\n' )))
		    *s = '\0';

		if (!strncasecmp( line, "cpu MHz", strlen( "cpu MHz" ) ))
		{
		    if (sscanf( value, "%lf", &cpuMHz ) == 1)
		    {
			QUERYPERF_RDTSC_Frequency = (LONGLONG)(cpuMHz * 1000000.0);
			QUERYPERF_RDTSC_Use = TRUE;
			TRACE("using frequency: %lldHz\n", QUERYPERF_RDTSC_Frequency);
			break;
		    }
		}
	    }
	    fclose(fp);
	}
    }
#endif
    QUERYPERF_Initialized = TRUE;
}


/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.@)
 */
BOOL WINAPI QueryPerformanceCounter(PLARGE_INTEGER counter)
{
    struct timeval tv;

    if (!QUERYPERF_Initialized)
	QUERYPERF_Init();

#if defined(__i386__) && defined(__GNUC__)
    if (QUERYPERF_RDTSC_Use)
    {
	/* i586 optimized version */
	__asm__ __volatile__ ( "rdtsc"
			       : "=a" (counter->s.LowPart), "=d" (counter->s.HighPart) );
	return TRUE;
    }
    /* fall back to generic routine (ie, for i386, i486) */
#endif

    /* generic routine */
    gettimeofday( &tv, NULL );
    counter->QuadPart = (LONGLONG)tv.tv_usec + (LONGLONG)tv.tv_sec * 1000000;
    return TRUE;
}

/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.@)
 */
BOOL WINAPI QueryPerformanceFrequency(PLARGE_INTEGER frequency)
{
    if (!QUERYPERF_Initialized)
	QUERYPERF_Init();

#if defined(__i386__) && defined(__GNUC__)
    if (QUERYPERF_RDTSC_Use)
    {
	frequency->QuadPart = QUERYPERF_RDTSC_Frequency;
	return TRUE;
    }
#endif

    frequency->s.LowPart	= 1000000;
    frequency->s.HighPart	= 0;
    return TRUE;
}

/****************************************************************************
 *		FlushInstructionCache (KERNEL32.@)
 */
BOOL WINAPI FlushInstructionCache(HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize)
{
    if (GetVersion() & 0x80000000) return TRUE; /* not NT, always TRUE */
    FIXME_(debug)("(0x%08lx,%p,0x%08lx): stub\n",(DWORD)hProcess, lpBaseAddress, dwSize);
    return TRUE;
}

/***********************************************************************
 *           GetSystemPowerStatus      (KERNEL32.@)
 */
BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS sps_ptr)
{
    return FALSE;   /* no power management support */
}


/***********************************************************************
 *           SetSystemPowerState      (KERNEL32.@)
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
 * CreateMailslotA [KERNEL32.@]
 */
HANDLE WINAPI CreateMailslotA( LPCSTR lpName, DWORD nMaxMessageSize,
                                   DWORD lReadTimeout, LPSECURITY_ATTRIBUTES sa)
{
    FIXME("(%s,%ld,%ld,%p): stub\n", debugstr_a(lpName),
          nMaxMessageSize, lReadTimeout, sa);
    return (HANDLE)1;
}


/******************************************************************************
 * CreateMailslotW [KERNEL32.@]  Creates a mailslot with specified name
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
    return (HANDLE)1;
}


/******************************************************************************
 * GetMailslotInfo [KERNEL32.@]  Retrieves info about specified mailslot
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
    FIXME("(%04x, %04x, %08lx, %08lx): stub.\n", hFileHandle, hExistingCompletionPort, dwCompletionKey, dwNumberOfConcurrentThreads);
    return (HANDLE)NULL;
}

/******************************************************************************
 *		GetQueuedCompletionStatus (KERNEL32.@)
 */
BOOL WINAPI GetQueuedCompletionStatus(
    HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred,
    LPDWORD lpCompletionKey, LPOVERLAPPED *lpOverlapped, DWORD dwMilliseconds
) {
    FIXME("(%x,%p,%p,%p,%ld), stub!\n",CompletionPort,lpNumberOfBytesTransferred,lpCompletionKey,lpOverlapped,dwMilliseconds);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
