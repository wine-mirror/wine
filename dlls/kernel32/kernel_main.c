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

#include "wine/library.h"
#include "kernel_private.h"
#include "console_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

extern int CDECL __wine_set_signal_handler(unsigned, int (*)(unsigned));

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
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;

    NtQuerySystemInformation( SystemBasicInformation, &system_info, sizeof(system_info), NULL );

    /* Setup registry locale information */
    LOCALE_InitRegistry();

    /* Setup registry timezone information */
    TIMEZONE_InitRegistry();

    /* Setup computer name */
    COMPUTERNAME_Init();

    CONSOLE_Init(params);

    /* copy process information from ntdll */
    ENV_CopyStartupInformation();

    if (!(GetVersion() & 0x80000000))
    {
        /* Securom checks for this one when version is NT */
        set_entry_point( module, "FT_Thunk", 0 );
    }
    else
    {
        LDR_MODULE *ldr;

        if (LdrFindEntryForAddress( GetModuleHandleW( 0 ), &ldr ) || !(ldr->Flags & LDR_WINE_INTERNAL))
            LoadLibraryA( "krnl386.exe16" );
    }

    /* finish the process initialisation for console bits, if needed */
    __wine_set_signal_handler(SIGINT, CONSOLE_HandleCtrlC);

    if (params->ConsoleHandle == KERNEL32_CONSOLE_ALLOC)
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
        DisableThreadLibraryCalls( hinst );
        return process_attach( hinst );
    case DLL_PROCESS_DETACH:
        WritePrivateProfileSectionW( NULL, NULL, NULL );
        CONSOLE_Exit();
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
ULONGLONG WINAPI DECLSPEC_HOTPATCH GetTickCount64(void)
{
    LARGE_INTEGER counter, frequency;

    NtQueryPerformanceCounter( &counter, &frequency );
    return counter.QuadPart * 1000 / frequency.QuadPart;
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
 *  The value returned will wrap around every 2^32 milliseconds.
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTickCount(void)
{
    return GetTickCount64();
}

/******************************************************************************
 *           GetSystemRegistryQuota       (KERNEL32.@)
 */
BOOL WINAPI GetSystemRegistryQuota(PDWORD pdwQuotaAllowed, PDWORD pdwQuotaUsed)
{
    FIXME("(%p, %p) faking reported quota values\n", pdwQuotaAllowed, pdwQuotaUsed);

    if (pdwQuotaAllowed)
        *pdwQuotaAllowed = 2 * 1000 * 1000 * 1000; /* 2 GB */

    if (pdwQuotaUsed)
        *pdwQuotaUsed = 100 * 1000 * 1000; /* 100 MB */

    return TRUE;
}
