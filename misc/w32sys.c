/*
 * W32SYS
 * helper DLL for Win32s
 *
 * Copyright (c) 1996 Anand Kumria
 */

#include "windows.h"
#include "w32sys.h"


/***********************************************************************
 *           GetWin32sInfo   (W32SYS.12)
 */
WORD WINAPI GetWin32sInfo( LPWIN32SINFO lpInfo)
{
    lpInfo->bMajor = 1;
    lpInfo->bMinor = 3;
    lpInfo->wBuildNumber = 0;
    lpInfo->fDebug = FALSE;

    return 0;
}
