/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"

int WIN32_LastError;

/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.235)
 */
DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
    strcpy(lpFilename, "c:\\dummy");
    return 8;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.273)
 */
VOID GetStartupInfoA(LPSTARTUPINFO lpStartupInfo)
{
    lpStartupInfo->cb = sizeof(STARTUPINFO);
    lpStartupInfo->lpReserved = NULL;
    lpStartupInfo->lpDesktop = "Desktop";
    lpStartupInfo->lpTitle = "Title";

    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->hStdInput  = (HANDLE)0;
    lpStartupInfo->hStdOutput = (HANDLE)1;
    lpStartupInfo->hStdError  = (HANDLE)2;
}

int KERN32_Init(void)
{
    return 1;
}
