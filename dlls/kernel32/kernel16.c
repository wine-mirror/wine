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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wownt32.h"

#include "kernel_private.h"
#include "kernel16_private.h"
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
 *		Reserved1 (KERNEL.77)
 */
SEGPTR WINAPI KERNEL_AnsiNext16(SEGPTR current)
{
    return (*(char *)MapSL(current)) ? current + 1 : current;
}

/***********************************************************************
 *		Reserved2(KERNEL.78)
 */
SEGPTR WINAPI KERNEL_AnsiPrev16( SEGPTR start, SEGPTR current )
{
    return (current==start)?start:current-1;
}

/***********************************************************************
 *		Reserved3 (KERNEL.79)
 */
SEGPTR WINAPI KERNEL_AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = toupper(*s);
            s++;
        }
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}

/***********************************************************************
 *		Reserved4 (KERNEL.80)
 */
SEGPTR WINAPI KERNEL_AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = tolower(*s);
            s++;
        }
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}

/***********************************************************************
 *		Reserved5 (KERNEL.87)
 */
INT16 WINAPI KERNEL_lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)strcmp( str1, str2 );
}

/***********************************************************************
 *           lstrcpy   (KERNEL.88)
 */
SEGPTR WINAPI lstrcpy16( SEGPTR dst, LPCSTR src )
{
    if (!lstrcpyA( MapSL(dst), src )) dst = 0;
    return dst;
}

/***********************************************************************
 *           lstrcat   (KERNEL.89)
 */
SEGPTR WINAPI lstrcat16( SEGPTR dst, LPCSTR src )
{
    /* Windows does not check for NULL pointers here, so we don't either */
    strcat( MapSL(dst), src );
    return dst;
}

/***********************************************************************
 *           lstrlen   (KERNEL.90)
 */
INT16 WINAPI lstrlen16( LPCSTR str )
{
    return (INT16)lstrlenA( str );
}

/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void WINAPI hmemcpy16( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}

/***********************************************************************
 *           lstrcpyn   (KERNEL.353)
 */
SEGPTR WINAPI lstrcpyn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    lstrcpynA( MapSL(dst), src, n );
    return dst;
}

/***********************************************************************
 *           lstrcatn   (KERNEL.352)
 */
SEGPTR WINAPI lstrcatn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    LPSTR p = MapSL(dst);
    LPSTR start = p;

    while (*p) p++;
    if ((n -= (p - start)) <= 0) return dst;
    lstrcpynA( p, src, n );
    return dst;
}

/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi16( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage == -1 ) codepage = CP_ACP;
    return WideCharToMultiByte( codepage, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
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
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook16(FARPROC16 func)
{
    static FARPROC16 hook;

    FIXME("(%p), stub.\n", func);
    return InterlockedExchangePointer( (void **)&hook, func );
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
 *           _DebugOutput                    (KERNEL.328)
 */
void WINAPIV _DebugOutput( WORD flags, LPCSTR spec, VA_LIST16 valist )
{
    char caller[101];

    /* Decode caller address */
    if (!GetModuleName16( GetExePtr(CURRENT_STACK16->cs), caller, sizeof(caller) ))
        sprintf( caller, "%04X:%04X", CURRENT_STACK16->cs, CURRENT_STACK16->ip );

    /* FIXME: cannot use wvsnprintf16 from kernel */
    /* wvsnprintf16( temp, sizeof(temp), spec, valist ); */

    /* Output */
    FIXME("%s %04x %s\n", caller, flags, debugstr_a(spec) );
}
