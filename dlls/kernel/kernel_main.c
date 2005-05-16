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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <signal.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "thread.h"
#include "winreg.h"
#include "winternl.h"

#include "wine/winbase16.h"
#include "wine/library.h"
#include "module.h"
#include "wincon.h"
#include "toolhelp.h"
#include "kernel_private.h"
#include "console_private.h"

extern void LOCALE_InitRegistry(void);
extern void COMPUTERNAME_Init(void);

extern  int __wine_set_signal_handler(unsigned, int (*)(unsigned));
/* memory/environ.c */
extern void ENV_CopyStartupInformation(void);

extern int main_create_flags;

static CRITICAL_SECTION ldt_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &ldt_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": ldt_section") }
};
static CRITICAL_SECTION ldt_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 *           locking for LDT routines
 */
static void ldt_lock(void)
{
    RtlEnterCriticalSection( &ldt_section );
}

static void ldt_unlock(void)
{
    RtlLeaveCriticalSection( &ldt_section );
}


/***********************************************************************
 *           KERNEL thread initialisation routine
 */
static void thread_attach(void)
{
    /* allocate the 16-bit stack (FIXME: should be done lazily) */
    HGLOBAL16 hstack = K32WOWGlobalAlloc16( GMEM_FIXED, 0x10000 );
    NtCurrentTeb()->stack_sel = GlobalHandleToSel16( hstack );
    NtCurrentTeb()->WOW32Reserved = (void *)MAKESEGPTR( NtCurrentTeb()->stack_sel,
                                                        0x10000 - sizeof(STACK16FRAME) );
}


/***********************************************************************
 *           KERNEL thread finalisation routine
 */
static void thread_detach(void)
{
    /* free the 16-bit stack */
    K32WOWGlobalFree16( NtCurrentTeb()->stack_sel );
    NtCurrentTeb()->WOW32Reserved = 0;
    if (NtCurrentTeb()->Tib.SubSystemTib) TASK_ExitTask();
}


/***********************************************************************
 *           KERNEL process initialisation routine
 */
static BOOL process_attach(void)
{
    HMODULE16 hModule;
    SYSTEM_INFO si;

    /* FIXME: should probably be done in ntdll */
    GetSystemInfo( &si );
    NtCurrentTeb()->Peb->NumberOfProcessors = si.dwNumberOfProcessors;

    /* Setup registry locale information */
    LOCALE_InitRegistry();

    /* Initialize 16-bit thunking entry points */
    if (!WOWTHUNK_Init()) return FALSE;

    /* Initialize DOS memory */
    if (!DOSMEM_Init()) return FALSE;

    /* Setup computer name */
    COMPUTERNAME_Init();

    /* copy process information from ntdll */
    ENV_CopyStartupInformation();

    if ((hModule = LoadLibrary16( "krnl386.exe" )) >= 32)
    {
        /* Initialize special KERNEL entry points */

        /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */
        NE_SetEntryPoint( hModule, 178, GetWinFlags16() );

        /* Initialize KERNEL.454/455 (__FLATCS/__FLATDS) */
        NE_SetEntryPoint( hModule, 454, wine_get_cs() );
        NE_SetEntryPoint( hModule, 455, wine_get_ds() );

        /* Initialize KERNEL.THHOOK */
        TASK_InstallTHHook(MapSL((SEGPTR)GetProcAddress16( hModule, (LPCSTR)332 )));

        /* Initialize the real-mode selector entry points */
#define SET_ENTRY_POINT( num, addr ) \
    NE_SetEntryPoint( hModule, (num), GLOBAL_CreateBlock( GMEM_FIXED, \
                      DOSMEM_MapDosToLinear(addr), 0x10000, hModule, \
                      WINE_LDT_FLAGS_DATA ))

        SET_ENTRY_POINT( 174, 0xa0000 );  /* KERNEL.174: __A000H */
        SET_ENTRY_POINT( 181, 0xb0000 );  /* KERNEL.181: __B000H */
        SET_ENTRY_POINT( 182, 0xb8000 );  /* KERNEL.182: __B800H */
        SET_ENTRY_POINT( 195, 0xc0000 );  /* KERNEL.195: __C000H */
        SET_ENTRY_POINT( 179, 0xd0000 );  /* KERNEL.179: __D000H */
        SET_ENTRY_POINT( 190, 0xe0000 );  /* KERNEL.190: __E000H */
        NE_SetEntryPoint( hModule, 183, DOSMEM_0000H );       /* KERNEL.183: __0000H */
        NE_SetEntryPoint( hModule, 173, DOSMEM_BiosSysSeg );  /* KERNEL.173: __ROMBIOS */
        NE_SetEntryPoint( hModule, 193, DOSMEM_BiosDataSeg ); /* KERNEL.193: __0040H */
        NE_SetEntryPoint( hModule, 194, DOSMEM_BiosSysSeg );  /* KERNEL.194: __F000H */
#undef SET_ENTRY_POINT

        /* Force loading of some dlls */
        LoadLibrary16( "system.drv" );
    }

#ifdef __i386__
    /* Create the shared heap for broken win95 native dlls */
    if (GetVersion() & 0x80000000) HeapCreate( HEAP_SHARED, 0, 0 );
#endif

    /* initialize LDT locking */
    wine_ldt_init_locking( ldt_lock, ldt_unlock );

    /* finish the process initialisation for console bits, if needed */
    __wine_set_signal_handler(SIGINT, CONSOLE_HandleCtrlC);

    if (main_create_flags & CREATE_NEW_CONSOLE)
    {
        HMODULE mod = GetModuleHandleA(0);
        if (RtlImageNtHeader(mod)->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            AllocConsole();
    }
    else if (!(main_create_flags & DETACHED_PROCESS))
    {
        /* 1/ shall inherit console + handles
         * 2/ shall create std handles, if handles are not inherited
         * TBD when not using wineserver handles for console handles
         */
    }

    if (main_create_flags & CREATE_NEW_PROCESS_GROUP)
        SetConsoleCtrlHandler(NULL, TRUE);

    /* Create 16-bit task */
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
        return process_attach();
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
 *		EnableDos (KERNEL.41)
 *		DisableDos (KERNEL.42)
 *		GetLastDiskChange (KERNEL.98)
 *		ValidateCodeSegments (KERNEL.100)
 *		KbdRst (KERNEL.123)
 *		EnableKernel (KERNEL.124)
 *		DisableKernel (KERNEL.125)
 *		ValidateFreeSpaces (KERNEL.200)
 *		K237 (KERNEL.237)
 *		BUNNY_351 (KERNEL.351)
 *		PIGLET_361 (KERNEL.361)
 *
 * Entry point for kernel functions that do nothing.
 */
LONG WINAPI KERNEL_nop(void)
{
    return 0;
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
