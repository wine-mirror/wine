/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.161)
 */
LPSTR GetCommandLineA(void)
{
    static char buffer[256];
    PDB *pdb = (PDB *)GlobalLock( GetCurrentPDB() );
    memcpy( buffer, &pdb->cmdLine[1], pdb->cmdLine[0] );
    dprintf_win32(stddeb,"CommandLine = %s\n", buffer );
    return buffer;
}

