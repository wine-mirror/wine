/*
 * Win32s Universal Thunk API
 *
 * Copyright 1999 Ulrich Weigand
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

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);

#pragma pack(push,1)

typedef struct
{
    BYTE    popl_eax;
    BYTE    pushl;
    DWORD   target;
    BYTE    pushl_eax;
    BYTE    ljmp;
    DWORD   utglue16;

} UT16THUNK;

typedef struct
{
    BYTE    popl_eax;
    BYTE    pushl;
    DWORD   target;
    BYTE    pushl_eax;
    BYTE    jmp;
    DWORD   utglue32;

} UT32THUNK;

#pragma pack(pop)

typedef struct _UTINFO
{
    struct _UTINFO    *next;
    HMODULE            hModule;
    HMODULE16          hModule16;

    UT16THUNK          ut16;
    UT32THUNK          ut32;

} UTINFO;

static UTINFO *UT_head; /* head of Universal Thunk list */

typedef DWORD (CALLBACK *UTGLUEPROC)( LPVOID lpBuff, DWORD dwUserDefined );

BOOL WINAPI UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff );

VOID WINAPI UTUnRegister( HMODULE hModule );


/****************************************************************************
 *		UTGlue16 (KERNEL.666) (KERNEL Wine-specific export)
 */
DWORD WINAPI UTGlue16( LPVOID lpBuff, DWORD dwUserDefined, SEGPTR *translationList,
		       UTGLUEPROC target )
{
    INT i;

    /* Convert arguments to flat pointers */

    if ( translationList )
        for ( i = 0; translationList[i]; i++ )
        {
            LPVOID flatPtr = MapSL( translationList[i] );
            *(LPVOID *)flatPtr = MapSL( *(SEGPTR *)flatPtr );
        }

    /* Call 32-bit routine */

    return target( lpBuff, dwUserDefined );
}

/****************************************************************************
 *		UTGlue32
 */
static DWORD WINAPI UTGlue32( FARPROC16 target, LPVOID lpBuff, DWORD dwUserDefined,
                              LPVOID translationList[] )
{
    SEGPTR segBuff, *segptrList = NULL;
    INT i, nList = 0;
    DWORD retv;
    WORD args[4];

    /* Convert arguments to SEGPTRs */

    if ( translationList )
        for ( nList = 0; translationList[nList]; nList++ )
            ;

    if ( nList )
    {
        segptrList = HeapAlloc( GetProcessHeap(), 0, sizeof(SEGPTR)*nList );
        if ( !segptrList )
        {
            FIXME("Unable to allocate segptrList!\n" );
            return 0;
        }

        for ( i = 0; i < nList; i++ )
            segptrList[i] = *(SEGPTR *)translationList[i]
                          = MapLS( *(LPVOID *)translationList[i] );
    }

    segBuff = MapLS( lpBuff );

    /* Call 16-bit routine */

    args[3] = SELECTOROF(segBuff);
    args[2] = OFFSETOF(segBuff);
    args[1] = HIWORD(dwUserDefined);
    args[0] = LOWORD(dwUserDefined);
    WOWCallback16Ex( (DWORD)target, WCB16_PASCAL, sizeof(args), args, &retv );

    /* Free temporary selectors */

    UnMapLS( segBuff );

    if ( nList )
    {
        for ( i = 0; i < nList; i++ )
            UnMapLS( segptrList[i] );

        HeapFree( GetProcessHeap(), 0, segptrList );
    }

    return retv;
}

/****************************************************************************
 *		UTAlloc
 */
static UTINFO *UTAlloc( HMODULE hModule, HMODULE16 hModule16,
                        FARPROC16 target16, FARPROC target32 )
{
    static FARPROC16 UTGlue16_Segptr = NULL;
    UTINFO *ut;

    if ( !UTGlue16_Segptr )
    {
        HMODULE16 hMod = GetModuleHandle16( "KERNEL" );
        UTGlue16_Segptr = GetProcAddress16( hMod, "UTGlue16" );
        if ( !UTGlue16_Segptr ) return NULL;
    }

    ut = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UTINFO) );
    if ( !ut ) return NULL;

    ut->hModule   = hModule;
    ut->hModule16 = hModule16;

    ut->ut16.popl_eax  = 0x58;
    ut->ut16.pushl     = 0x68;
    ut->ut16.target    = (DWORD)target32;
    ut->ut16.pushl_eax = 0x50;
    ut->ut16.ljmp      = 0xea;
    ut->ut16.utglue16  = (DWORD)UTGlue16_Segptr;

    ut->ut32.popl_eax  = 0x58;
    ut->ut32.pushl     = 0x68;
    ut->ut32.target    = (DWORD)target16;
    ut->ut32.pushl_eax = 0x50;
    ut->ut32.jmp       = 0xe9;
    ut->ut32.utglue32  = (DWORD)UTGlue32 - ((DWORD)&ut->ut32.utglue32 + sizeof(DWORD));

    ut->next = UT_head;
    UT_head = ut;

    return ut;
}

/****************************************************************************
 *		UTFree
 */
static void UTFree( UTINFO *ut )
{
    UTINFO **ptr;

    for ( ptr = &UT_head; *ptr; ptr = &(*ptr)->next )
        if ( *ptr == ut )
        {
            *ptr = ut->next;
            break;
        }

    HeapFree( GetProcessHeap(), 0, ut );
}

/****************************************************************************
 *		UTFind
 */
static UTINFO *UTFind( HMODULE hModule )
{
    UTINFO *ut;

    for ( ut = UT_head; ut; ut =ut->next )
        if ( ut->hModule == hModule )
            break;

    return ut;
}


/****************************************************************************
 *		UTRegister (KERNEL32.@)
 */
BOOL WINAPI UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff )
{
    UTINFO *ut;
    HMODULE16 hModule16;
    FARPROC16 target16, init16;
    static BOOL done;

    if (!done)
    {
        LoadLibrary16( "gdi.exe" );
        LoadLibrary16( "user.exe" );
        done = TRUE;
    }

    /* Load 16-bit DLL and get UTProc16 entry point */

    if (   (hModule16 = LoadLibrary16( lpsz16BITDLL )) <= 32
        || (target16  = GetProcAddress16( hModule16, lpszProcName )) == 0 )
        return FALSE;

    /* Allocate UTINFO struct */

    RtlAcquirePebLock();
    if ( (ut = UTFind( hModule )) != NULL )
        ut = NULL;
    else
        ut = UTAlloc( hModule, hModule16, target16, pfnUT32CallBack );
    RtlReleasePebLock();

    if ( !ut )
    {
        FreeLibrary16( hModule16 );
        return FALSE;
    }

    /* Call UTInit16 if present */

    if (     lpszInitName
         && (init16 = GetProcAddress16( hModule16, lpszInitName )) != 0 )
    {
        SEGPTR callback = MapLS( &ut->ut16 );
        SEGPTR segBuff  = MapLS( lpBuff );
        WORD args[4];
        DWORD ret;

        args[3] = SELECTOROF(callback);
        args[2] = OFFSETOF(callback);
        args[1] = SELECTOROF(segBuff);
        args[0] = OFFSETOF(segBuff);
        WOWCallback16Ex( (DWORD)init16, WCB16_PASCAL, sizeof(args), args, &ret );
        UnMapLS( segBuff );
        UnMapLS( callback );
        if (!ret)
        {
            UTUnRegister( hModule );
            return FALSE;
        }
    }

    /* Return 32-bit thunk */

    *ppfn32Thunk = (FARPROC) &ut->ut32;

    return TRUE;
}

/****************************************************************************
 *		UTUnRegister (KERNEL32.@)
 */
VOID WINAPI UTUnRegister( HMODULE hModule )
{
    UTINFO *ut;
    HMODULE16 hModule16 = 0;

    RtlAcquirePebLock();
    ut = UTFind( hModule );
    if ( ut )
    {
        hModule16 = ut->hModule16;
        UTFree( ut );
    }
    RtlReleasePebLock();

    if ( hModule16 )
        FreeLibrary16( hModule16 );
}

/****************************************************************************
 *		UTInit     (KERNEL.493)
 */
WORD WINAPI UTInit16( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME("(%08lx, %08lx, %08lx, %08lx): stub\n", x1, x2, x3, x4 );
    return 0;
}
