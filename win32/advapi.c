/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "shell.h"
#include "heap.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           StartServiceCtrlDispatcherA   [ADVAPI32.196]
 */
BOOL32 WINAPI StartServiceCtrlDispatcher32A(LPSERVICE_TABLE_ENTRY32A servent)
{
	fprintf(stderr,"StartServiceCtrlDispatcherA(%p (%s)), STUB.\n",
		servent,
		servent->lpServiceName
	);
	return TRUE;
}

/***********************************************************************
 *           StartServiceCtrlDispatcherW   [ADVAPI32.197]
 */
BOOL32 WINAPI StartServiceCtrlDispatcher32W(LPSERVICE_TABLE_ENTRY32W servent)
{
	char	buffer[200];

	lstrcpynWtoA(buffer,servent->lpServiceName,200);
	fprintf(stderr,"StartServiceCtrlDispatcherA(%p (%s)), STUB.\n",
		servent,
		buffer
	);
	return TRUE;
}


/***********************************************************************
 *           OpenProcessToken   [ADVAPI32.197]
 */
BOOL32 WINAPI OpenProcessToken(HANDLE32 process,DWORD desiredaccess,
                               HANDLE32 *thandle)
{
	fprintf(stdnimp,"OpenProcessToken(%08x,%08lx,%p),stub!\n",
		process,desiredaccess,thandle
	);
	return TRUE;
}

/***********************************************************************
 *           OpenThreadToken		[ADVAPI32.114]
 */
BOOL32 WINAPI OpenThreadToken( HANDLE32 thread,DWORD desiredaccess,
                               BOOL32 openasself,HANDLE32 *thandle )
{
	fprintf(stdnimp,"OpenThreadToken(%08x,%08lx,%d,%p),stub!\n",
		thread,desiredaccess,openasself,thandle
	);
	return TRUE;
}

/***********************************************************************
 *           LookupPrivilegeValueA   [ADVAPI32.90]
 */
BOOL32 WINAPI LookupPrivilegeValue32A(LPCSTR system,LPCSTR name,LPVOID bla)
{
	fprintf(stdnimp,"LookupPrivilegeValue32A(%s,%s,%p),stub\n",
		system,name,bla
	);
	return TRUE;
}
BOOL32 WINAPI AdjustTokenPrivileges(HANDLE32 TokenHandle,BOOL32 DisableAllPrivileges,
	LPVOID NewState,DWORD BufferLength,LPVOID PreviousState,
	LPDWORD ReturnLength )
{
	return TRUE;
}

/***********************************************************************
 *           GetTokenInformation	[ADVAPI32.66]
 */
BOOL32 WINAPI GetTokenInformation(
	HANDLE32 token,/*TOKEN_INFORMATION_CLASS*/ DWORD tokeninfoclass,LPVOID tokeninfo,
	DWORD tokeninfolength,LPDWORD retlen
) {
	fprintf(stderr,"GetTokenInformation(%08x,%ld,%p,%ld,%p)\n",
		token,tokeninfoclass,tokeninfo,tokeninfolength,retlen
	);
	return TRUE;
}

/*SC_HANDLE*/
DWORD WINAPI OpenSCManagerA(LPCSTR machine,LPCSTR dbname,DWORD desiredaccess)
{
	fprintf(stderr,"OpenSCManagerA(%s,%s,%08lx)\n",machine,dbname,desiredaccess);
	return 0;
}

DWORD WINAPI OpenSCManagerW(LPCWSTR machine,LPCWSTR dbname,DWORD desiredaccess)
{
	LPSTR	machineA = HEAP_strdupWtoA(GetProcessHeap(),0,machine);
	LPSTR	dbnameA = HEAP_strdupWtoA(GetProcessHeap(),0,dbname);
	fprintf(stderr,"OpenSCManagerW(%s,%s,%08lx)\n",machineA,dbnameA,desiredaccess);
	HeapFree(GetProcessHeap(),0,machineA);
	HeapFree(GetProcessHeap(),0,dbnameA);
	return 0;
}

BOOL32 WINAPI AllocateLocallyUniqueId(LPLUID lpluid) {
	lpluid->LowPart = time(NULL);
	lpluid->HighPart = 0;
	return TRUE;
}
