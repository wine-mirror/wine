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
#include "heap.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
  
/* The global error value
 */
int WIN32_LastError;

/*********************************************************************
 *              CloseHandle             (KERNEL32.23)
 */
BOOL CloseHandle(KERNEL_OBJECT *handle)
{
    if ((int)handle<0x1000) /* FIXME: hack */
    	return CloseFileHandle((int)handle);
    if ((int)handle==0xFFFFFFFF)
    	return FALSE;
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
HMODULE32 WIN32_GetModuleHandleA(char *module)
{
    HMODULE32 hModule;

    dprintf_win32(stddeb, "GetModuleHandleA: %s\n", module ? module : "NULL");
/* Freecell uses the result of GetModuleHandleA(0) as the hInstance in
all calls to e.g. CreateWindowEx. */
    if (module == NULL) {
	TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
	hModule = pTask->hInstance;
    } else
	hModule = GetModuleHandle(module);
    dprintf_win32(stddeb, "GetModuleHandleA: returning %d\n", hModule );
    return hModule;
}

HMODULE32 WIN32_GetModuleHandleW(LPCWSTR module)
{
    HMODULE32 hModule;
    LPSTR modulea = HEAP_strdupWtoA( GetProcessHeap(), 0, module );
    hModule = WIN32_GetModuleHandleA( modulea );
    HeapFree( GetProcessHeap(), 0, modulea );
    return hModule;
}


/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.273)
 */
VOID GetStartupInfo32A(LPSTARTUPINFO32A lpStartupInfo)
{
    lpStartupInfo->cb = sizeof(STARTUPINFO32A);
    lpStartupInfo->lpReserved = "<Reserved>";
    lpStartupInfo->lpDesktop = "Desktop";
    lpStartupInfo->lpTitle = "Title";

    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->hStdInput  = (HANDLE32)0;
    lpStartupInfo->hStdOutput = (HANDLE32)1;
    lpStartupInfo->hStdError  = (HANDLE32)2;
}

/***********************************************************************
 *              GetStartupInfoW         (KERNEL32.274)
 */
VOID GetStartupInfo32W(LPSTARTUPINFO32W lpStartupInfo)
{
    lpStartupInfo->cb = sizeof(STARTUPINFO32W);
    lpStartupInfo->lpReserved = HEAP_strdupAtoW(GetProcessHeap(),0,"<Reserved>");
    lpStartupInfo->lpDesktop = HEAP_strdupAtoW(GetProcessHeap(), 0, "Desktop");
    lpStartupInfo->lpTitle = HEAP_strdupAtoW(GetProcessHeap(), 0, "Title");

    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL; /* must be NULL for VC runtime */
    lpStartupInfo->hStdInput  = (HANDLE32)0;
    lpStartupInfo->hStdOutput = (HANDLE32)1;
    lpStartupInfo->hStdError  = (HANDLE32)2;
}

/***********************************************************************
 *              GetStartupInfoA         (KERNEL32.284)
 * FIXME: perhaps supply better values. 
 *        add other architectures for WINELIB.
 */
VOID
GetSystemInfo(LPSYSTEM_INFO si) {
    WORD cpu;
    
    si->u.x.wProcessorArchitecture	= PROCESSOR_ARCHITECTURE_INTEL;

    si->dwPageSize			= 4096; /* 4K */
    si->lpMinimumApplicationAddress	= (void *)0x40000000;
    si->lpMaximumApplicationAddress	= (void *)0x80000000;
    si->dwActiveProcessorMask		= 1;
    si->dwNumberOfProcessors		= 1;
#ifdef WINELIB
    /* FIXME: perhaps check compilation defines ... */
    si->dwProcessorType			= PROCESSOR_INTEL_386;
    cpu = 3;
#else
    cpu = runtime_cpu();
    switch (cpu) {
    case 4: si->dwProcessorType		= PROCESSOR_INTEL_486;
	    break;
    case 5: si->dwProcessorType		= PROCESSOR_INTEL_PENTIUM;
	    break;
    case 3:
    default: si->dwProcessorType	= PROCESSOR_INTEL_386;
	    break;
    }
#endif
    si->dwAllocationGranularity		= 8; /* hmm? */
    si->wProcessorLevel			= cpu;
    si->wProcessorRevision		= 0; /* FIXME, see SDK */
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

/***********************************************************************
 *              GetComputerNameA         (KERNEL32.165)
 */
BOOL32
GetComputerName32A(LPSTR name,LPDWORD size) {
	if (-1==gethostname(name,*size))
		return FALSE;
	*size = lstrlen32A(name);
	return TRUE;
}

/***********************************************************************
 *              GetComputerNameW         (KERNEL32.166)
 */
BOOL32
GetComputerName32W(LPWSTR name,LPDWORD size) {
	LPSTR	nameA = (LPSTR)xmalloc(*size);

	if (!GetComputerName32A(nameA,size)) {
		free(nameA);
		return FALSE;
	}
	lstrcpynAtoW(name,nameA,*size);
	free(nameA);
	/* FIXME : size correct? */
	return TRUE;
}

/***********************************************************************
 *           GetUserNameA   [ADVAPI32.67]
 */
BOOL32 GetUserName32A(LPSTR lpszName, LPDWORD lpSize)
{
  size_t len;
  char *name;

  name=getlogin();
  len = name ? strlen(name) : 0;
  if (!len || !lpSize || len > *lpSize) {
    if (lpszName) *lpszName = 0;
    return 0;
  }
  *lpSize=len;
  strcpy(lpszName, name);
  return 1;
}

/***********************************************************************
 *           GetUserNameW   [ADVAPI32.68]
 */
BOOL32 GetUserName32W(LPWSTR lpszName, LPDWORD lpSize)
{
	LPSTR name = (LPSTR)xmalloc(*lpSize);
	DWORD	size = *lpSize;
	BOOL32 res = GetUserName32A(name,lpSize);

	lstrcpynAtoW(lpszName,name,size);
	return res;
}
