/*
 * Win32 miscellaneous functions
 *
 * Copyright 1995 Thomas Sandford (tdgsandf@prds-grn.demon.co.uk)
 */

/* Misc. new functions - they should be moved into appropriate files
at a later date. */

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "windows.h"
#include "winnt.h"
#include "winerror.h"
#include "debug.h"

/****************************************************************************
 *		UTRegister (KERNEL32.697)
 */
BOOL32 WINAPI UTRegister(HMODULE32 hModule,
                      LPSTR lpsz16BITDLL,
                      LPSTR lpszInitName,
                      LPSTR lpszProcName,
                      /*UT32PROC*/ LPVOID *ppfn32Thunk,
                      /*FARPROC*/ LPVOID pfnUT32CallBack,
                      LPVOID lpBuff)
{
    fprintf(stdnimp, "UTRegister Stub called!\n");
    return TRUE;
}

/****************************************************************************
 *		UTUnRegister (KERNEL32.698)
 */
BOOL32 WINAPI UTUnRegister(HMODULE32 hModule)
{
    fprintf(stdnimp, "UTUnRegister Stub called!\n");
    return TRUE;
}


/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.564)
 */
BOOL32 WINAPI QueryPerformanceCounter(LPLARGE_INTEGER counter)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);
    counter->LowPart = tv.tv_usec+tv.tv_sec*1000000;
    counter->HighPart = 0;
    return TRUE;
}

HANDLE32 WINAPI FindFirstChangeNotification32A(LPCSTR lpPathName,BOOL32 bWatchSubtree,DWORD dwNotifyFilter) {
	fprintf(stderr,"FindFirstChangeNotification(%s,%d,%08lx),stub\n",
		lpPathName,bWatchSubtree,dwNotifyFilter
	);
	return 0xcafebabe;
}

BOOL32 WINAPI FindNextChangeNotification(HANDLE32 fcnhandle) {
	fprintf(stderr,"FindNextChangeNotification(%08x),stub!\n",fcnhandle);
	return FALSE;
}

/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.565)
 */
BOOL32 WINAPI QueryPerformanceFrequency(LPLARGE_INTEGER frequency)
{
	frequency->LowPart	= 1000000;
	frequency->HighPart	= 0;
	return TRUE;
}

/****************************************************************************
 *		DeviceIoControl (KERNEL32.188)
 */
BOOL32 WINAPI DeviceIoControl(HANDLE32 hDevice, DWORD dwIoControlCode, 
			      LPVOID lpvlnBuffer, DWORD cblnBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpoPverlapped)
{

        fprintf(stdnimp, "DeviceIoControl Stub called!\n");
	/* FIXME: Set appropriate error */
	return FALSE;

}

/****************************************************************************
 *		FlushInstructionCache (KERNEL32.261)
 */
BOOL32 WINAPI FlushInstructionCache(DWORD x,DWORD y,DWORD z) {
	fprintf(stderr,"FlushInstructionCache(0x%08lx,0x%08lx,0x%08lx)\n",x,y,z);
	return TRUE;
}
