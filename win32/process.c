/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include "winbase.h"
#include "winerror.h"
#include "heap.h"
#include "thread.h"
#include "process.h"
#include "pe_image.h"
#include "file.h"
#include "task.h"
#include "toolhelp.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32)


/*********************************************************************
 *      Process_ClockTimeToFileTime
 *      (olorin@fandra.org, 20-Sep-1998)
 *      Converts clock_t into FILETIME.
 *      Used by GetProcessTime.
 *      Differences to UnixTimeToFileTime:
 *          1) Divided by CLK_TCK
 *          2) Time is relative. There is no 'starting date', so there is 
 *             no need in offset correction, like in UnixTimeToFileTime
 *      FIXME: This function should be moved to a more appropriate .c file
 *      FIXME: On floating point operations, it is assumed that
 *             floating values are truncated on convertion to integer.
 */
void Process_ClockTimeToFileTime( clock_t unix_time, LPFILETIME filetime )
{
    double td = (unix_time*10000000.0)/CLK_TCK;
    /* Yes, double, because long int might overflow here. */
#if SIZEOF_LONG_LONG >= 8
    unsigned long long t = td;
    filetime->dwLowDateTime  = (UINT) t;
    filetime->dwHighDateTime = (UINT) (t >> 32);
#else
    double divider = 1. * (1 << 16) * (1 << 16);
    filetime->dwHighDateTime = (UINT) (td / divider);
    filetime->dwLowDateTime  = (UINT) (td - filetime->dwHighDateTime*divider);
    /* using floor() produces wierd results, better leave this as it is 
     * ( with (UINT32) convertion )
     */
#endif
}

/*********************************************************************
 *	GetProcessTimes				[KERNEL32.262]
 *
 * FIXME: lpCreationTime, lpExitTime are NOT INITIALIZED.
 * olorin@fandra.org: Would be nice to substract the cpu time,
 *                    used by Wine at startup.
 *                    Also, there is a need to separate times
 *                    used by different applications.
 */
BOOL WINAPI GetProcessTimes(
	HANDLE hprocess,LPFILETIME lpCreationTime,LPFILETIME lpExitTime,
	LPFILETIME lpKernelTime, LPFILETIME lpUserTime
) {
	struct tms tms;

	times(&tms);
        Process_ClockTimeToFileTime(tms.tms_utime,lpUserTime);
        Process_ClockTimeToFileTime(tms.tms_stime,lpKernelTime);
	return TRUE;
}

