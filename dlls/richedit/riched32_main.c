/*
 * Win32 Richedit control
 *
 * Copyright (C) 2000 Hidenori Takeshima <hidenori@a2.ctktv.ne.jp>
 */

#include "winbase.h"
#include "debugtools.h"
/*#include "richedit.h"*/


DEFAULT_DEBUG_CHANNEL(richedit);

/******************************************************************************
 *
 * RICHED32_Register [Internal]
 *
 */
static BOOL
RICHED32_Register( void )
{
    FIXME( "stub\n" );

    return FALSE;
}

/******************************************************************************
 *
 * RICHED32_Unregister
 *
 */
static void
RICHED32_Unregister( void )
{
}

/******************************************************************************
 *
 * RICHED32_Init [Internal]
 *
 */

BOOL WINAPI
RICHED32_Init( HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
    switch ( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
        if ( !RICHED32_Register() )
            return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        RICHED32_Unregister();
        break;
    }

    return TRUE;
}

