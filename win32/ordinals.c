/*
 * Win32 ordinal only exported functions that can't be stuffed somehwere else.
 *
 * Copyright 1997 Marcus Meissner
 */

#include <stdio.h>
#include "thread.h"
#include "winerror.h"
#include "heap.h"
#include "selectors.h"
#include "miscemu.h"
#include "winnt.h"
#include "process.h"
#include "module.h"
#include "task.h"
#include "callback.h"
#include "debug.h"

static CRITICAL_SECTION Win16Mutex;
static SEGPTR segWin16Mutex = NULL;


/***********************************************
 *           GetpWin16Lock    (KERNEL32.93)
 * Return the infamous Win16Mutex.
 */
VOID WINAPI GetpWin16Lock(CRITICAL_SECTION **lock)
{
    FIXME(win32, "(%p)\n",lock);
    *lock = &Win16Mutex;
}

/***********************************************
 *           GetPK16SysVar    (KERNEL32.92)
 * Return the infamous Win16Mutex.
 */
LPVOID WINAPI GetPK16SysVar(void)
{
    static BYTE PK16SysVar[128];

    FIXME(win32, "()\n");
    return PK16SysVar;
}


/**********************************************************************
 *           WOWGetDescriptor        (KERNEL32.88) (WOW32.1)
 */
BOOL32 WINAPI WOWGetDescriptor(SEGPTR segptr,LPLDT_ENTRY ldtent)
{
    return GetThreadSelectorEntry(GetCurrentThreadId(),segptr>>16,ldtent);
}


/***********************************************************************
 *           GetProcessDword    (KERNEL32.18)
 * 'Of course you cannot directly access Windows internal structures'
 */

DWORD WINAPI GetProcessDword(DWORD processid,DWORD action)
{
	PDB32	*process = processid? PROCESS_IdToPDB( processid )
                                    : PROCESS_Current();
	TDB	*pTask;

	action+=56;
	fprintf(stderr,"KERNEL32_18(%ld,%ld+0x38)\n",processid,action);
	if (!process || action>56)
		return 0;
	switch (action) {
	case 0:	/* return app compat flags */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->compat_flags;
	case 4:	/* returns offset 0xb8 of process struct... dunno what it is */
		return 0;
	case 8:	/* return hinstance16 */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->hInstance;
	case 12:/* return expected windows version */
		pTask = (TDB*)GlobalLock16(process->task);
		if (!pTask)
			return 0;
		return pTask->version;
	case 16:/* return uncrypted pointer to current thread */
		return (DWORD)THREAD_Current();
	case 20:/* return uncrypted pointer to process */
		return (DWORD)process;
	case 24:/* return stdoutput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdOutput);
	case 28:/* return stdinput handle from startupinfo */
		return (DWORD)(process->env_db->startup_info->hStdInput);
	case 32:/* get showwindow flag from startupinfo */
		return (DWORD)(process->env_db->startup_info->wShowWindow);
	case 36:{/* return startup x and y sizes */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwXSize;if (x==0x80000000) x=0x8000;
		y=si->dwYSize;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 40:{/* return startup x and y */
		LPSTARTUPINFO32A si = process->env_db->startup_info;
		DWORD x,y;

		x=si->dwX;if (x==0x80000000) x=0x8000;
		y=si->dwY;if (y==0x80000000) y=0x8000;
		return (y<<16)+x;
	}
	case 44:/* return startup flags */
		return process->env_db->startup_info->dwFlags;
	case 48:/* return uncrypted pointer to parent process (if any) */
		return (DWORD)process->parent;
	case 52:/* return process flags */
		return process->flags;
	case 56:/* unexplored */
		return 0;
	default:
		fprintf(stderr,"_KERNEL32_18:unknown offset (%ld)\n",action);
		return 0;
	}
	/* shouldn't come here */
}


/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv()
{
	fprintf(stderr,"GetWin16DOSEnv(), STUB returning 0\n");
	return 0;
}

BOOL32 WINAPI _KERNEL32_100(HANDLE32 threadid,DWORD exitcode,DWORD x) {
	fprintf(stderr,"KERNEL32_100(%d,%ld,0x%08lx),stub\n",threadid,exitcode,x);
	return TRUE;
}

DWORD WINAPI _KERNEL32_99(DWORD x) {
	fprintf(stderr,"KERNEL32_99(0x%08lx),stub\n",x);
	return 1;
}

/************************************************************************
 *		KERNEL_449		(KERNEL.449)
 * This returns a segmented static pointer to a linear pointer to a critical
 * section in kernel32 address space. This is most likely the Win16 Lock,
 * but I am not sure.
 */
SEGPTR WINAPI KERNEL_449(void) { 
	if (!segWin16Mutex) {
		LPDWORD	w16m = SEGPTR_ALLOC(4);
		
		*w16m = (DWORD)&Win16Mutex;
		segWin16Mutex = SEGPTR_GET(w16m);
	}
	return segWin16Mutex;
}
