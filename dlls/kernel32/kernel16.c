/*
 * 16-bit kernel initialization code
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wownt32.h"

#include "toolhelp.h"
#include "kernel_private.h"
#include "kernel16_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

/**************************************************************************
 *		DllEntryPoint   (KERNEL.669)
 */
BOOL WINAPI KERNEL_DllEntryPoint( DWORD reasion, HINSTANCE16 inst, WORD ds,
                                  WORD heap, DWORD reserved1, WORD reserved2 )
{
    static int done;

    /* the entry point can be called multiple times */
    if (done) return TRUE;
    done = 1;

    /* Initialize 16-bit thunking entry points */
    if (!WOWTHUNK_Init()) return FALSE;

    /* Initialize DOS memory */
    if (!DOSMEM_Init()) return FALSE;

    /* Initialize special KERNEL entry points */

    NE_SetEntryPoint( inst, 178, GetWinFlags16() );

    NE_SetEntryPoint( inst, 454, wine_get_cs() );
    NE_SetEntryPoint( inst, 455, wine_get_ds() );

    NE_SetEntryPoint( inst, 183, DOSMEM_0000H );       /* KERNEL.183: __0000H */
    NE_SetEntryPoint( inst, 173, DOSMEM_BiosSysSeg );  /* KERNEL.173: __ROMBIOS */
    NE_SetEntryPoint( inst, 193, DOSMEM_BiosDataSeg ); /* KERNEL.193: __0040H */
    NE_SetEntryPoint( inst, 194, DOSMEM_BiosSysSeg );  /* KERNEL.194: __F000H */

    /* Initialize KERNEL.THHOOK */
    TASK_InstallTHHook(MapSL((SEGPTR)GetProcAddress16( inst, (LPCSTR)332 )));

    /* Initialize the real-mode selector entry points */
#define SET_ENTRY_POINT( num, addr ) \
    NE_SetEntryPoint( inst, (num), GLOBAL_CreateBlock( GMEM_FIXED, \
                      DOSMEM_MapDosToLinear(addr), 0x10000, inst, \
                      WINE_LDT_FLAGS_DATA ))

    SET_ENTRY_POINT( 174, 0xa0000 );  /* KERNEL.174: __A000H */
    SET_ENTRY_POINT( 181, 0xb0000 );  /* KERNEL.181: __B000H */
    SET_ENTRY_POINT( 182, 0xb8000 );  /* KERNEL.182: __B800H */
    SET_ENTRY_POINT( 195, 0xc0000 );  /* KERNEL.195: __C000H */
    SET_ENTRY_POINT( 179, 0xd0000 );  /* KERNEL.179: __D000H */
    SET_ENTRY_POINT( 190, 0xe0000 );  /* KERNEL.190: __E000H */
#undef SET_ENTRY_POINT

    /* Force loading of some dlls */
    LoadLibrary16( "system.drv" );

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


/* thunk for 16-bit CreateThread */
struct thread_args
{
    FARPROC16 proc;
    DWORD     param;
};

static DWORD CALLBACK start_thread16( LPVOID threadArgs )
{
    struct thread_args args = *(struct thread_args *)threadArgs;
    HeapFree( GetProcessHeap(), 0, threadArgs );
    return K32WOWCallback16( (DWORD)args.proc, args.param );
}

/***********************************************************************
 *           CreateThread16   (KERNEL.441)
 */
HANDLE WINAPI CreateThread16( SECURITY_ATTRIBUTES *sa, DWORD stack,
                              FARPROC16 start, SEGPTR param,
                              DWORD flags, LPDWORD id )
{
    struct thread_args *args = HeapAlloc( GetProcessHeap(), 0, sizeof(*args) );
    if (!args) return INVALID_HANDLE_VALUE;
    args->proc = start;
    args->param = param;
    return CreateThread( sa, stack, start_thread16, args, flags, id );
}


/***********************************************************************
 *           wait_input_idle
 *
 * user32.WaitForInputIdle releases the win16 lock, so here is a replacement.
 */
static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    DWORD ret;
    HANDLE handles[2];

    handles[0] = process;
    SERVER_START_REQ( get_process_idle_event )
    {
        req->handle = process;
        if (!(ret = wine_server_call_err( req ))) handles[1] = reply->event;
    }
    SERVER_END_REQ;
    if (ret) return WAIT_FAILED;  /* error */
    if (!handles[1]) return 0;  /* no event to wait on */

    return WaitForMultipleObjects( 2, handles, FALSE, timeout );
}


/**************************************************************************
 *           WINOLDAP entry point
 */
void WINAPI WINOLDAP_EntryPoint( CONTEXT86 *context )
{
    PDB16 *psp;
    INT len;
    LPSTR cmdline;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD count, exit_code = 1;

    InitTask16( context );

    TRACE( "(ds=%x es=%x fs=%x gs=%x, bx=%04x cx=%04x di=%04x si=%x)\n",
            context->SegDs, context->SegEs, context->SegFs, context->SegGs,
            context->Ebx, context->Ecx, context->Edi, context->Esi );

    psp = GlobalLock16( context->SegEs );
    len = psp->cmdLine[0];
    cmdline = HeapAlloc( GetProcessHeap(), 0, len + 1 );
    memcpy( cmdline, psp->cmdLine + 1, len );
    cmdline[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 10 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 10000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ReleaseThunkLock( &count );

        WaitForSingleObject( info.hProcess, INFINITE );
        GetExitCodeProcess( info.hProcess, &exit_code );
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else
        ReleaseThunkLock( &count );

    HeapFree( GetProcessHeap(), 0, cmdline );
    ExitThread( exit_code );
}
