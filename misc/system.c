/*
 * SYSTEM DLL routines
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"


/***********************************************************************
 *           InquireSystem   (SYSTEM.1)
 */
DWORD InquireSystem( WORD code, WORD drive, BOOL enable )
{
    WORD drivetype;

    switch(code)
    {
    case 0:  /* Get timer resolution */
        return 54925;

    case 1:  /* Get drive type */
        drivetype = GetDriveType16( drive );
        return MAKELONG( drivetype, drivetype );

    case 2:  /* Enable one-drive logic */
        fprintf( stderr, "InquireSystem(2): set single-drive %d not supported\n", enable );
        return 0;
    }
    fprintf( stderr, "InquireSystem: unknown code %d\n", code );
    return 0;
}
