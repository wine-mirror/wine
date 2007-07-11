/*
 * Kernel initialization code
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winternl.h"
#include "wownt32.h"

#include "wine/winbase16.h"
#include "wine/library.h"
#include "wincon.h"
#include "toolhelp.h"
#include "kernel_private.h"
#include "kernel16_private.h"
#include "console_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

extern  int __wine_set_signal_handler(unsigned, int (*)(unsigned));

static ULONGLONG server_start_time;

/***********************************************************************
 *           KERNEL thread initialisation routine
 */
static void thread_attach(void)
{
    /* allocate the 16-bit stack (FIXME: should be done lazily) */
    HGLOBAL16 hstack = WOWGlobalAlloc16( GMEM_FIXED, 0x10000 );
    kernel_get_thread_data()->stack_sel = GlobalHandleToSel16( hstack );
    NtCurrentTeb()->WOW32Reserved = (void *)MAKESEGPTR( kernel_get_thread_data()->stack_sel,
                                                        0x10000 - sizeof(STACK16FRAME) );
}


/***********************************************************************
 *           KERNEL thread finalisation routine
 */
static void thread_detach(void)
{
    /* free the 16-bit stack */
    WOWGlobalFree16( kernel_get_thread_data()->stack_sel );
    NtCurrentTeb()->WOW32Reserved = 0;
    if (NtCurrentTeb()->Tib.SubSystemTib) TASK_ExitTask();
}


/***********************************************************************
 *           set_entry_point
 */
static void set_entry_point( HMODULE module, const char *name, DWORD rva )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;

    if ((exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        DWORD *functions = (DWORD *)((char *)module + exports->AddressOfFunctions);
        const WORD *ordinals = (const WORD *)((const char *)module + exports->AddressOfNameOrdinals);
        const DWORD *names = (const DWORD *)((const char *)module +  exports->AddressOfNames);
        int min = 0, max = exports->NumberOfNames - 1;

        while (min <= max)
        {
            int res, pos = (min + max) / 2;
            const char *ename = (const char *)module + names[pos];
            if (!(res = strcmp( ename, name )))
            {
                WORD ordinal = ordinals[pos];
                assert( ordinal < exports->NumberOfFunctions );
                TRACE( "setting %s at %p to %08x\n", name, &functions[ordinal], rva );
                functions[ordinal] = rva;
                return;
            }
            if (res > 0) max = pos - 1;
            else min = pos + 1;
        }
    }
}


/***********************************************************************
 *           KERNEL process initialisation routine
 */
static BOOL process_attach( HMODULE module )
{
    SYSTEM_INFO si;
    SYSTEM_TIMEOFDAY_INFORMATION ti;
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;

    /* FIXME: should probably be done in ntdll */
    GetSystemInfo( &si );
    NtCurrentTeb()->Peb->NumberOfProcessors = si.dwNumberOfProcessors;

    NtQuerySystemInformation( SystemTimeOfDayInformation, &ti, sizeof(ti), NULL );
    server_start_time = ti.liKeBootTime.QuadPart;

    /* Setup registry locale information */
    LOCALE_InitRegistry();

    /* Setup computer name */
    COMPUTERNAME_Init();

    /* convert value from server:
     * + 0 => INVALID_HANDLE_VALUE
     * + console handle needs to be mapped
     */
    if (!params->hStdInput)
        params->hStdInput = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdInput)))
        params->hStdInput = console_handle_map(params->hStdInput);

    if (!params->hStdOutput)
        params->hStdOutput = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdOutput)))
        params->hStdOutput = console_handle_map(params->hStdOutput);

    if (!params->hStdError)
        params->hStdError = INVALID_HANDLE_VALUE;
    else if (VerifyConsoleIoHandle(console_handle_map(params->hStdError)))
        params->hStdError = console_handle_map(params->hStdError);

    /* copy process information from ntdll */
    ENV_CopyStartupInformation();

    if (!(GetVersion() & 0x80000000))
    {
        /* Securom checks for this one when version is NT */
        set_entry_point( module, "FT_Thunk", 0 );
    }
#ifdef __i386__
    else
    {
        /* create the shared heap for broken win95 native dlls */
        HeapCreate( HEAP_SHARED, 0, 0 );
        /* setup emulation of protected instructions from 32-bit code */
        RtlAddVectoredExceptionHandler( TRUE, INSTR_vectored_handler );
    }
#endif

    /* finish the process initialisation for console bits, if needed */
    __wine_set_signal_handler(SIGINT, CONSOLE_HandleCtrlC);

    if (params->ConsoleHandle == (HANDLE)1)  /* FIXME */
    {
        HMODULE mod = GetModuleHandleA(0);
        if (RtlImageNtHeader(mod)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            AllocConsole();
    }
    /* else TODO for DETACHED_PROCESS:
     * 1/ inherit console + handles
     * 2/ create std handles, if handles are not inherited
     * TBD when not using wineserver handles for console handles
     */

    /* Create 16-bit task */
    LoadLibrary16( "krnl386.exe" );
    thread_attach();
    TASK_CreateMainTask();
    return TRUE;
}

/***********************************************************************
 *           KERNEL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        return process_attach( hinst );
    case DLL_THREAD_ATTACH:
        thread_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        WriteOutProfiles16();
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           MulDiv   (KERNEL32.@)
 * RETURNS
 *	Result of multiplication and division
 *	-1: Overflow occurred or Divisor was 0
 */
INT WINAPI MulDiv( INT nMultiplicand, INT nMultiplier, INT nDivisor)
{
    LONGLONG ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
         ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((LONGLONG)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((LONGLONG)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
}


/******************************************************************************
 *           GetTickCount64       (KERNEL32.@)
 */
ULONGLONG WINAPI GetTickCount64(void)
{
    LARGE_INTEGER now;

    NtQuerySystemTime( &now );
    return (now.QuadPart - server_start_time) / 10000;
}


/***********************************************************************
 *           GetTickCount       (KERNEL32.@)
 *
 * Get the number of milliseconds the system has been running.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current tick count.
 *
 * NOTES
 *  -The value returned will wrap arounf every 2^32 milliseconds.
 *  -Under Windows, tick 0 is the moment at which the system is rebooted.
 *  Under Wine, tick 0 begins at the moment the wineserver process is started,
 */
DWORD WINAPI GetTickCount(void)
{
    return GetTickCount64();
}
