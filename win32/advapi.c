/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "advapi32.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

/***********************************************************************
 *           StartServiceCtrlDispatcherA   [ADVAPI32.196]
 */
BOOL32
StartServiceCtrlDispatcher32A(LPSERVICE_TABLE_ENTRY32A servent)
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
BOOL32
StartServiceCtrlDispatcher32W(LPSERVICE_TABLE_ENTRY32W servent)
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
BOOL32
OpenProcessToken(HANDLE32 process,DWORD desiredaccess,HANDLE32 *thandle)
{
	fprintf(stdnimp,"OpenProcessToken(%08x,%08lx,%p),stub!\n",
		process,desiredaccess,thandle
	);
	return TRUE;
}

/***********************************************************************
 *           LookupPrivilegeValueA   [ADVAPI32.90]
 */
BOOL32
LookupPrivilegeValue32A(LPCSTR system,LPCSTR name,LPVOID bla)
{
	fprintf(stdnimp,"LookupPrivilegeValue32A(%s,%s,%p),stub\n",
		system,name,bla
	);
	return TRUE;
}
BOOL32
AdjustTokenPrivileges(HANDLE32 TokenHandle,BOOL32 DisableAllPrivileges,
	LPVOID NewState,DWORD BufferLength,LPVOID PreviousState,
	LPDWORD ReturnLength )
{
	return TRUE;
}
