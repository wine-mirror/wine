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
#include "module.h"
#include "callback.h"
#include "debug.h"
#include "stddebug.h"

static CRITICAL_SECTION Win16Mutex;

/***********************************************
 *           GetPWinLock    (KERNEL32)
 * Return the infamous Win16Mutex.
 */
VOID WINAPI GetPWinLock(CRITICAL_SECTION **lock)
{
        fprintf(stderr,"GetPWinlock(%p)\n",lock);
        *lock = &Win16Mutex;
}

/**********************************************************************
 *           _KERNEL32_88
 */
BOOL32 WINAPI WOW32_1(SEGPTR segptr,LPLDT_ENTRY ldtent)
{
    return GetThreadSelectorEntry(GetCurrentThreadId(),segptr>>16,ldtent);
}


/***********************************************************************
 *           _KERNEL32_18    (KERNEL32.18)
 * 'Of course you cannot directly access Windows internal structures'
 */

DWORD WINAPI _KERNEL32_18(DWORD processid,DWORD action)
{
	PDB32	*process;
	TDB	*pTask;

	action+=56;
	fprintf(stderr,"KERNEL32_18(%ld,%ld+0x38)\n",processid,action);
	if (action>56)
		return 0;
	if (!processid) {
		process=pCurrentProcess;
		/* check if valid process */
	} else
		process=(PDB32*)pCurrentProcess; /* decrypt too, if needed */
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
		return (DWORD)pCurrentThread;
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
 *							(KERNEL32.33)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI _KERNEL32_34()
{
	fprintf(stderr,"KERNEL32_34(), STUB returning 0\n");
	return 0;
}

BOOL32 WINAPI _KERNEL32_99(HANDLE32 threadid,DWORD exitcode,DWORD x) {
	fprintf(stderr,"KERNEL32_99(%d,%ld,0x%08lx),stub\n",threadid,exitcode,x);
	return TRUE;
}

DWORD WINAPI _KERNEL32_98(DWORD x) {
	fprintf(stderr,"KERNEL32_98(0x%08lx),stub\n",x);
	return 1;
}
