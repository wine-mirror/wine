/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "module.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"
#include "process.h"  /* for pCurrentProcess */


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.161)
 */
LPCSTR WINAPI GetCommandLine32A(void)
{
    return pCurrentProcess->env_db->cmd_line;
}

/***********************************************************************
 *           GetCommandLineW      (KERNEL32.162)
 */
LPCWSTR WINAPI GetCommandLine32W(void)
{
    static WCHAR buffer[256];

    lstrcpynAtoW(buffer,GetCommandLine32A(),256);
    return buffer;
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

