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
#include "handle32.h"
#include "pe_image.h"
#include "stddebug.h"
#define DEBUG_WIN32
#include "debug.h"
  
/* The global error value
 */
int WIN32_LastError;

/* Standard system handles for stdin, stdout, and stderr.
 */
FILE_OBJECT *hstdin, *hstdout, *hstderr;

static int CreateStdHandles(void);

/*********************************************************************
 *              CloseHandle             (KERNEL32.23)
 */
BOOL CloseHandle(HANDLE32 handle)
{
    int rc;

    if(ValidateKernelObject(handle) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    switch(handle->magic)
    {
        case KERNEL_OBJECT_UNUSED:
            SetLastError(ERROR_INVALID_HANDLE);
            return 0;

        case KERNEL_OBJECT_FILE:
            rc = CloseFileHandle((FILE_OBJECT *)handle);
            break;

        default:
            dprintf_win32(stddeb, "CloseHandle: type %ld not implemented yet.\n",
                   handle->magic);
            break;
    }

    ReleaseKernelObject(handle);
    return 0;
}

/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.235)
 */
DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
    strcpy(lpFilename, "c:\\dummy");
    return 8;
}

/***********************************************************************
 *              GetModuleHandle         (KERNEL32.237)
 */
HMODULE WIN32_GetModuleHandle(char *module)
{
    HMODULE hModule;

    dprintf_win32(stddeb, "GetModuleHandle: %s\n", module ? module : "NULL");
    if (module == NULL) hModule = GetExePtr( GetCurrentTask() );
    else hModule = GetModuleHandle(module);
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
    lpStartupInfo->hStdInput  = (HANDLE)0;
    lpStartupInfo->hStdOutput = (HANDLE)1;
    lpStartupInfo->hStdError  = (HANDLE)2;
}

/* Initialize whatever internal data structures we need.
 *
 * Returns 1 on success, 0 on failure.
 */
int KERN32_Init(void)
{
    /* Create the standard system handles
     */
    if(CreateStdHandles() != 0)
        return 0;

    return 1;
}

/* CreateStdHandles creates the standard input, output, and error handles.
 * These handles aren't likely to be used since they're generally used for
 * console output, but startup code still likes to mess with them.  They're
 * also useful for debugging since apps and runtime libraries might write
 * errors to stderr.
 *
 * Returns 0 on success, nonzero on failure.
 */
static int CreateStdHandles(void)
{
    /* Create the standard input handle.
     */
    hstdin = (FILE_OBJECT *)CreateKernelObject(sizeof(FILE_OBJECT));
    if(hstdin == NULL)
        return 1;
    hstdin->common.magic = KERNEL_OBJECT_FILE;
    hstdin->fd = 0;
    hstdin->type = FILE_TYPE_CHAR;
    hstdin->misc_flags = 0;

    /* Create the standard output handle
     */
    hstdout = (FILE_OBJECT *)CreateKernelObject(sizeof(FILE_OBJECT));
    if(hstdout == NULL)
        return 1;
    hstdout->common.magic = KERNEL_OBJECT_FILE;
    hstdout->fd = 1;
    hstdout->type = FILE_TYPE_CHAR;
    hstdout->misc_flags = 0;

    /* Create the standard error handle
     */
    hstderr = (FILE_OBJECT *)CreateKernelObject(sizeof(FILE_OBJECT));
    if(hstderr == NULL)
        return 1;
    hstderr->common.magic = KERNEL_OBJECT_FILE;
    hstderr->fd = 2;
    hstderr->type = FILE_TYPE_CHAR;
    hstderr->misc_flags = 0;

    return 0;
}
