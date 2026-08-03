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

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wincon.h"
#include "winternl.h"

#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static STARTUPINFOA startup_infoA;

/***********************************************************************
 *           set_entry_point
 */
#ifdef __i386__
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
                DWORD oldprot;

                TRACE( "setting %s at %p to %08lx\n", name, &functions[ordinal], rva );
                VirtualProtect( functions + ordinal, sizeof(*functions), PAGE_READWRITE, &oldprot );
                functions[ordinal] = rva;
                VirtualProtect( functions + ordinal, sizeof(*functions), oldprot, &oldprot );
                return;
            }
            if (res > 0) max = pos - 1;
            else min = pos + 1;
        }
    }
}
#endif


/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.@)
 */
VOID WINAPI GetStartupInfoA( LPSTARTUPINFOA info )
{
    *info = startup_infoA;
}

static void copy_startup_info(void)
{
    RTL_USER_PROCESS_PARAMETERS* rupp;
    ANSI_STRING         ansi;

    RtlAcquirePebLock();

    rupp = NtCurrentTeb()->Peb->ProcessParameters;

    startup_infoA.cb                   = sizeof(startup_infoA);
    startup_infoA.lpReserved           = NULL;
    startup_infoA.lpDesktop = !RtlUnicodeStringToAnsiString( &ansi, &rupp->Desktop, TRUE ) ? ansi.Buffer : NULL;
    startup_infoA.lpTitle = !RtlUnicodeStringToAnsiString( &ansi, &rupp->WindowTitle, TRUE ) ? ansi.Buffer : NULL;
    startup_infoA.dwX                  = rupp->dwX;
    startup_infoA.dwY                  = rupp->dwY;
    startup_infoA.dwXSize              = rupp->dwXSize;
    startup_infoA.dwYSize              = rupp->dwYSize;
    startup_infoA.dwXCountChars        = rupp->dwXCountChars;
    startup_infoA.dwYCountChars        = rupp->dwYCountChars;
    startup_infoA.dwFillAttribute      = rupp->dwFillAttribute;
    startup_infoA.dwFlags              = rupp->dwFlags;
    startup_infoA.wShowWindow          = rupp->wShowWindow;
    startup_infoA.cbReserved2          = rupp->RuntimeInfo.MaximumLength;
    startup_infoA.lpReserved2          = rupp->RuntimeInfo.MaximumLength ? (void*)rupp->RuntimeInfo.Buffer : NULL;
    if (rupp->dwFlags & STARTF_USESTDHANDLES)
    {
        startup_infoA.hStdInput        = rupp->hStdInput;
        startup_infoA.hStdOutput       = rupp->hStdOutput;
        startup_infoA.hStdError        = rupp->hStdError;
    }
    else
    {
        startup_infoA.hStdInput        = INVALID_HANDLE_VALUE;
        startup_infoA.hStdOutput       = INVALID_HANDLE_VALUE;
        startup_infoA.hStdError        = INVALID_HANDLE_VALUE;
    }
    RtlReleasePebLock();
}

/***********************************************************************
 *           KERNEL process initialisation routine
 */
static BOOL process_attach( HMODULE module )
{
    RtlSetUnhandledExceptionFilter( UnhandledExceptionFilter );

    NtQuerySystemInformation( SystemBasicInformation, &system_info, sizeof(system_info), NULL );
    kernelbase_global_data = KernelBaseGetGlobalData();

    copy_startup_info();

#ifdef __i386__
    if (!(GetVersion() & 0x80000000))
    {
        /* Securom checks for this one when version is NT */
        set_entry_point( module, "FT_Thunk", 0 );
    }
    else
    {
        LDR_DATA_TABLE_ENTRY *ldr;

        if (LdrFindEntryForAddress( GetModuleHandleW( 0 ), &ldr ) || !(ldr->Flags & LDR_WINE_INTERNAL))
            LoadLibraryA( "krnl386.exe16" );
    }
#endif
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
