/*
 * Win32 ordinal only exported functions that can't be stuffed somehwere else.
 *
 * Copyright 1997 Marcus Meissner
 */

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
#include "stackframe.h"
#include "debug.h"

/**********************************************************************
 *           WOWGetDescriptor        (KERNEL32.88) (WOW32.1)
 */
BOOL32 WINAPI WOWGetDescriptor(SEGPTR segptr,LPLDT_ENTRY ldtent)
{
    return GetThreadSelectorEntry(GetCurrentThreadId(),segptr>>16,ldtent);
}


/***********************************************************************
 *           GetProcessDword    (KERNEL32.18) (KERNEL.485)
 * 'Of course you cannot directly access Windows internal structures'
 */
DWORD WINAPI GetProcessDword(DWORD processid,DWORD action)
{
	PDB32	*process = processid? PROCESS_IdToPDB( processid )
                                    : PROCESS_Current();
	TDB	*pTask;

	action+=56;
	TRACE(win32,"(%ld,%ld+0x38)\n",processid,action);
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
		return process->process_dword;
	default:
		WARN(win32,"Unknown offset (%ld)\n",action);
		return 0;
	}
	/* shouldn't come here */
}

/***********************************************************************
 *           SetProcessDword    (KERNEL.484)
 * 'Of course you cannot directly access Windows internal structures'
 */
VOID WINAPI SetProcessDword(DWORD processid,DWORD action,DWORD value)
{
	PDB32	*process = processid? PROCESS_IdToPDB( processid )
                                    : PROCESS_Current();

	action+=56;
	TRACE(win32,"(%ld,%ld+0x38)\n",processid,action);
	if (!process || action>56) return;

	switch (action) {
        case 56: process->process_dword = value; break;
	default:
		FIXME(win32,"Unknown offset (%ld)\n",action);
                break;
	}
}

/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv()
{
	FIXME(dosmem,"stub, returning 0\n");
	return 0;
}

/**********************************************************************
 *           GetPK16SysVar    (KERNEL32.92)
 */
LPVOID WINAPI GetPK16SysVar(void)
{
    static BYTE PK16SysVar[128];

    FIXME(win32, "()\n");
    return PK16SysVar;
}

/**********************************************************************
 *           CommonUnimpStub    (KERNEL32.17)
 */
REGS_ENTRYPOINT(CommonUnimpStub)
{
    if (EAX_reg(context))
        MSG( "*** Unimplemented Win32 API: %s\n", (LPSTR)EAX_reg(context) );

    switch ((ECX_reg(context) >> 4) & 0x0f)
    {
    case 15:  EAX_reg(context) = -1;   break;
    case 14:  EAX_reg(context) = 0x78; break;
    case 13:  EAX_reg(context) = 0x32; break;
    case 1:   EAX_reg(context) = 1;    break;
    default:  EAX_reg(context) = 0;    break;
    }

    ESP_reg(context) += (ECX_reg(context) & 0x0f) * 4;
}

/**********************************************************************
 *           HouseCleanLogicallyDeadHandles    (KERNEL32.33)
 */
void WINAPI HouseCleanLogicallyDeadHandles(void)
{
    /* Whatever this is supposed to do, our handles probably
       don't need it :-) */
}


BOOL32 WINAPI _KERNEL32_100(HANDLE32 threadid,DWORD exitcode,DWORD x) {
	FIXME(thread,"(%d,%ld,0x%08lx): stub\n",threadid,exitcode,x);
	return TRUE;
}

DWORD WINAPI _KERNEL32_99(DWORD x) {
	FIXME(win32,"(0x%08lx): stub\n",x);
	return 1;
}
/***********************************************************************
 *           PrivateExtractIconExA			[USER32.442]
 */
HRESULT WINAPI PrivateExtractIconExA ( DWORD u, DWORD v, DWORD w, DWORD x ,DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",u,v,w,x,y);
	return 0;
	
}
/***********************************************************************
 *           PrivateExtractIconExW			[USER32.443]
 */
HRESULT WINAPI PrivateExtractIconExW ( DWORD u, DWORD v, DWORD w, DWORD x ,DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",u,v,w,x,y);
	return 0;
	
}
/***********************************************************************
 *           PrivateExtractIconsW			[USER32.445]
 */
HRESULT WINAPI PrivateExtractIconsW ( DWORD r, DWORD s, DWORD t, DWORD u, DWORD v, DWORD w, DWORD x, DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",r,s,t,u,v,w,x,y );
	return 0;
	
}
/***********************************************************************
 *           RegisterShellHookWindow			[USER32.459]
 */
HRESULT WINAPI RegisterShellHookWindow ( DWORD u )
{	FIXME(win,"0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.132]
 */
HRESULT WINAPI DeregisterShellHookWindow ( DWORD u )
{	FIXME(win,"0x%08lx stub\n",u);
	return 0;
	
}
/***********************************************************************
 *           RegisterTaskList32   			[USER23.436]
 */
DWORD WINAPI RegisterTaskList32 (DWORD x)
{	FIXME(win,"0x%08lx\n",x);
	return TRUE;
}

