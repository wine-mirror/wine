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
#include "handle32.h"
#include "except.h"
#include "task.h"
#include "stddebug.h"
#define DEBUG_WIN32
#include "debug.h"
  
/* The global error value
 */
int WIN32_LastError;

/*********************************************************************
 *              CloseHandle             (KERNEL32.23)
 */
BOOL CloseHandle(KERNEL_OBJECT *handle)
{
    if(ValidateKernelObject(handle) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (handle<0x1000) /* FIXME: hack */
    	return CloseFileHandle(handle);
    switch(handle->magic)
    {
        case KERNEL_OBJECT_UNUSED:
            SetLastError(ERROR_INVALID_HANDLE);
            return 0;
/* FIXME
        case KERNEL_OBJECT_FILE:
            rc = CloseFileHandle((FILE_OBJECT *)handle);
            break;
 */

        default:
            dprintf_win32(stddeb, "CloseHandle: type %ld not implemented yet.\n",
                   handle->magic);
            break;
    }

    ReleaseKernelObject(handle);
    return 0;
}

/***********************************************************************
 *              GetModuleHandle         (KERNEL32.237)
 */
HMODULE32 WIN32_GetModuleHandle(char *module)
{
    HMODULE32 hModule;

    dprintf_win32(stddeb, "GetModuleHandle: %s\n", module ? module : "NULL");
/* Freecell uses the result of GetModuleHandleA(0) as the hInstance in
all calls to e.g. CreateWindowEx. */
    if (module == NULL) {
	TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
	hModule = pTask->hInstance;
    } else
	hModule = GetModuleHandle(module);
    dprintf_win32(stddeb, "GetModuleHandle: returning %d\n", hModule );
    return hModule;
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

    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->hStdInput  = (HANDLE32)0;
    lpStartupInfo->hStdOutput = (HANDLE32)1;
    lpStartupInfo->hStdError  = (HANDLE32)2;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.284)
 * FIXME: perhaps supply better values.
 */
VOID
GetSystemInfo(LPSYSTEM_INFO si) {
    si->dwOemId		= 0x12345678;
    si->dwPageSize	= 4096; /* 4K */
    si->lpMinimumApplicationAddress = 0x40000000;
    si->lpMaximumApplicationAddress = 0x80000000;
    si->dwActiveProcessorMask       = 1;
    si->dwNumberOfProcessors        = 1;
#ifdef WINELIB
    si->dwProcessorType             = 3;
#else
    si->dwProcessorType             = runtime_cpu();
#endif
    si->dwAllocationGranularity     = 8; /* hmm? */
}

/* Initialize whatever internal data structures we need.
 *
 * Returns 1 on success, 0 on failure.
 */
int KERN32_Init(void)
{
#ifndef WINELIB
    /* Initialize exception handling */
    EXC_Init();
#endif
    return 1;
}
