/*
 * Win32s Universal Thunk API
 *
 * Copyright 1999 Ulrich Weigand 
 */

#include "windef.h"
#include "heap.h"
#include "module.h"
#include "selectors.h"
#include "callback.h"
#include "debug.h"
#include "debugstr.h"

#pragma pack(1)

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

#pragma pack(4)

typedef struct tagUTINFO
{
    struct tagUTINFO  *next;
    HMODULE            hModule;
    HMODULE16          hModule16;
    
    UT16THUNK          ut16;
    UT32THUNK          ut32;

} UTINFO;

static UTINFO *utAnchor;

BOOL WINAPI UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL, 
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack, 
                        LPVOID lpBuff );

VOID WINAPI UTUnRegister( HMODULE hModule );


/****************************************************************************
 *		UTGlue16     (WPROCS.*)
 */
DWORD WINAPI UTGlue16( LPVOID lpBuff, DWORD dwUserDefined, SEGPTR translationList[], 
                       DWORD (CALLBACK *target)( LPVOID lpBuff, DWORD dwUserDefined ) )
{
    INT i;

    /* Convert arguments to flat pointers */

    if ( translationList )
        for ( i = 0; translationList[i]; i++ )
        {
            LPVOID flatPtr = PTR_SEG_TO_LIN( translationList[i] );
            *(LPVOID *)flatPtr = PTR_SEG_TO_LIN( *(SEGPTR *)flatPtr );
        }

    /* Call 32-bit routine */

    return target( lpBuff, dwUserDefined );
}

/****************************************************************************
 *		UTGlue32
 */
DWORD WINAPI UTGlue32( FARPROC16 target, LPVOID lpBuff, DWORD dwUserDefined, 
                       LPVOID translationList[] )
{
    SEGPTR segBuff, *segptrList = NULL;
    INT i, nList = 0;
    DWORD retv;

    /* Convert arguments to SEGPTRs */

    if ( translationList )
        for ( nList = 0; translationList[nList]; nList++ )
            ;

    if ( nList )
    {
        segptrList = HeapAlloc( GetProcessHeap(), 0, sizeof(SEGPTR)*nList );
        if ( !segptrList )
        {
            FIXME( thunk, "Unable to allocate segptrList!" );
            return 0;
        }

        for ( i = 0; i < nList; i++ )
            segptrList[i] = *(SEGPTR *)translationList[i] 
                          = MapLS( *(LPVOID *)translationList[i] );
    }

    segBuff = MapLS( lpBuff );

    /* Call 16-bit routine */

    retv = Callbacks->CallUTProc( target, segBuff, dwUserDefined );

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
    UTINFO *ut = HeapAlloc( SegptrHeap, HEAP_ZERO_MEMORY, sizeof(UTINFO) );
    if ( !ut ) return NULL;

    ut->hModule   = hModule;
    ut->hModule16 = hModule16;

    ut->ut16.popl_eax  = 0x58;
    ut->ut16.pushl     = 0x68;
    ut->ut16.target    = (DWORD)target32;
    ut->ut16.pushl_eax = 0x50;
    ut->ut16.ljmp      = 0xea;
    ut->ut16.utglue16  = (DWORD)MODULE_GetWndProcEntry16( "UTGlue16" );

    ut->ut32.popl_eax  = 0x58;
    ut->ut32.pushl     = 0x68;
    ut->ut32.target    = (DWORD)target16;
    ut->ut32.pushl_eax = 0x50;
    ut->ut32.jmp       = 0xe9;
    ut->ut32.utglue32  = (DWORD)UTGlue32 - ((DWORD)&ut->ut32.utglue32 + sizeof(DWORD));

    ut->next = utAnchor;
    utAnchor = ut;

    return ut;
}

/****************************************************************************
 *		UTFree
 */
static void UTFree( UTINFO *ut )
{
    UTINFO **ptr;

    for ( ptr = &utAnchor; *ptr; ptr = &(*ptr)->next )
        if ( *ptr == ut )
        {
            *ptr = ut->next;
            break;
        }

    HeapFree( SegptrHeap, 0, ut );
}

/****************************************************************************
 *		UTFind
 */
static UTINFO *UTFind( HMODULE hModule )
{
    UTINFO *ut;

    for ( ut = utAnchor; ut; ut =ut->next )
        if ( ut->hModule == hModule )
            break;

    return ut;
}


/****************************************************************************
 *		UTRegister (KERNEL32.697)
 */
BOOL WINAPI UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL, 
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack, 
                        LPVOID lpBuff )
{
    UTINFO *ut;
    HMODULE16 hModule16; 
    FARPROC16 target16, init16; 

    /* Load 16-bit DLL and get UTProc16 entry point */

    if (   (hModule16 = LoadLibrary16( lpsz16BITDLL )) <= 32
        || (target16  = WIN32_GetProcAddress16( hModule16, lpszProcName )) == 0 )
        return FALSE;

    /* Allocate UTINFO struct */

    SYSTEM_LOCK();
    if ( (ut = UTFind( hModule )) != NULL )
        ut = NULL;
    else
        ut = UTAlloc( hModule, hModule16, target16, pfnUT32CallBack );
    SYSTEM_UNLOCK();

    if ( !ut )
    {
        FreeLibrary16( hModule16 );
        return FALSE;
    }

    /* Call UTInit16 if present */

    if (     lpszInitName
         && (init16 = WIN32_GetProcAddress16( hModule16, lpszInitName )) != 0 )
    {
        SEGPTR callback = SEGPTR_GET( &ut->ut16 );
        SEGPTR segBuff  = MapLS( lpBuff );

        if ( !Callbacks->CallUTProc( init16, callback, segBuff ) )
        {
            UnMapLS( segBuff );
            UTUnRegister( hModule );
            return FALSE;
        }
        UnMapLS( segBuff );
    }

    /* Return 32-bit thunk */

    *ppfn32Thunk = (FARPROC) &ut->ut32;
    
    return TRUE;
}

/****************************************************************************
 *		UTUnRegister (KERNEL32.698)
 */
VOID WINAPI UTUnRegister( HMODULE hModule )
{
    UTINFO *ut;
    HMODULE16 hModule16 = 0;

    SYSTEM_LOCK();
    ut = UTFind( hModule );
    if ( !ut )
    {
        hModule16 = ut->hModule16;
        UTFree( ut );
    }
    SYSTEM_UNLOCK();

    if ( hModule16 ) 
        FreeLibrary16( hModule16 );
}

/****************************************************************************
 *		UTInit16     (KERNEL.494)
 */
WORD WINAPI UTInit16( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME( thunk, "(%08lx, %08lx, %08lx, %08lx): stub\n", x1, x2, x3, x4 );
    return 0;
}

