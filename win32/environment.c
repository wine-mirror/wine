/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "module.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           GetCommandLineA      (KERNEL32.161)
 */
LPSTR GetCommandLineA(void)
{
    static char buffer[256];
    char *cp;
    PDB *pdb = (PDB *)GlobalLock16( GetCurrentPDB() );

#ifndef WINELIB
    strcpy(buffer, MODULE_GetModuleName( GetExePtr(GetCurrentTask()) ) );
    cp = buffer+strlen(buffer);
    *cp++ = ' ';
#else
    cp = buffer;
#endif;
    memcpy( cp, &pdb->cmdLine[1], pdb->cmdLine[0] );
    dprintf_win32(stddeb,"CommandLine = %s\n", buffer );
    return buffer;
}

