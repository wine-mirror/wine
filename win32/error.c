/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"

static int WIN32_LastError;

/**********************************************************************
 *              GetLastError            (KERNEL32.227)
 */
DWORD GetLastError(void)
{
    return WIN32_LastError;
}

/**********************************************************************
 *              SetLastError            (KERNEL32.497)
 *
 * This is probably not used by apps too much, but it's useful for
 * our own internal use.
 */
void SetLastError(DWORD error)
{
    WIN32_LastError = error;
}

DWORD ErrnoToLastError(int errno_num)
{
    return errno_num;   /* Obviously not finished yet. :-) */
}
