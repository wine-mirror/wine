/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "process.h"
#include "tlhelp32.h"
#include "toolhelp.h"
#include "heap.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(toolhelp)


/* FIXME: to make this working, we have to callback all these registered 
 * functions from all over the WINE code. Someone with more knowledge than
 * me please do that. -Marcus
 */
static struct notify
{
    HTASK16   htask;
    FARPROC16 lpfnCallback;
    WORD     wFlags;
} *notifys = NULL;

static int nrofnotifys = 0;

static FARPROC16 HookNotify = NULL;

BOOL16 WINAPI NotifyRegister16( HTASK16 htask, FARPROC16 lpfnCallback,
                              WORD wFlags )
{
    int	i;

    TRACE("(%x,%lx,%x) called.\n",
                      htask, (DWORD)lpfnCallback, wFlags );
    if (!htask) htask = GetCurrentTask();
    for (i=0;i<nrofnotifys;i++)
        if (notifys[i].htask==htask)
            break;
    if (i==nrofnotifys) {
        if (notifys==NULL)
            notifys=(struct notify*)HeapAlloc( SystemHeap, 0,
                                               sizeof(struct notify) );
        else
            notifys=(struct notify*)HeapReAlloc( SystemHeap, 0, notifys,
                                        sizeof(struct notify)*(nrofnotifys+1));
        if (!notifys) return FALSE;
        nrofnotifys++;
    }
    notifys[i].htask=htask;
    notifys[i].lpfnCallback=lpfnCallback;
    notifys[i].wFlags=wFlags;
    return TRUE;
}

BOOL16 WINAPI NotifyUnregister16( HTASK16 htask )
{
    int	i;
    
    TRACE("(%x) called.\n", htask );
    if (!htask) htask = GetCurrentTask();
    for (i=nrofnotifys;i--;)
        if (notifys[i].htask==htask)
            break;
    if (i==-1)
        return FALSE;
    memcpy(notifys+i,notifys+(i+1),sizeof(struct notify)*(nrofnotifys-i-1));
    notifys=(struct notify*)HeapReAlloc( SystemHeap, 0, notifys,
                                        (nrofnotifys-1)*sizeof(struct notify));
    nrofnotifys--;
    return TRUE;
}

BOOL16 WINAPI StackTraceCSIPFirst16(STACKTRACEENTRY *ste, WORD wSS, WORD wCS, WORD wIP, WORD wBP)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceFirst16(STACKTRACEENTRY *ste, HTASK16 Task)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceNext16(STACKTRACEENTRY *ste)
{
    return TRUE;
}

BOOL16 WINAPI InterruptRegister16( HTASK16 task, FARPROC callback )
{
    return TRUE;
}

BOOL16 WINAPI InterruptUnRegister16( HTASK16 task )
{
    return TRUE;
}

/***********************************************************************
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook16(FARPROC16 lpfnNotifyHandler)
{
FARPROC16 tmp;
	tmp = HookNotify;
	HookNotify = lpfnNotifyHandler;
	/* just return previously installed notification function */
	return tmp;
}


/***********************************************************************
 *           CreateToolHelp32Snapshot			(KERNEL32.179)
 */
HANDLE WINAPI CreateToolhelp32Snapshot( DWORD flags, DWORD process ) 
{
    struct create_snapshot_request *req = get_req_buffer();

    TRACE("%lx,%lx\n", flags, process );
    if (flags & (TH32CS_SNAPHEAPLIST|TH32CS_SNAPMODULE|TH32CS_SNAPTHREAD))
        FIXME("flags %lx not implemented\n", flags );
    if (!(flags & TH32CS_SNAPPROCESS))
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return INVALID_HANDLE_VALUE;
    }
    
    /* Now do the snapshot */
    req->flags   = flags & ~TH32CS_INHERIT;
    req->inherit = (flags & TH32CS_INHERIT) != 0;
    server_call( REQ_CREATE_SNAPSHOT );
    return req->handle;
}


/***********************************************************************
 *		TOOLHELP_Process32Next
 *
 * Implementation of Process32First/Next
 */
static BOOL TOOLHELP_Process32Next( HANDLE handle, LPPROCESSENTRY lppe, BOOL first )
{
    struct next_process_request *req = get_req_buffer();

    if (lppe->dwSize < sizeof (PROCESSENTRY))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR("Result buffer too small\n");
        return FALSE;
    }
    req->handle = handle;
    req->reset = first;
    if (server_call( REQ_NEXT_PROCESS )) return FALSE;
    lppe->cntUsage            = 1;
    lppe->th32ProcessID       = (DWORD)req->pid;
    lppe->th32DefaultHeapID   = 0;  /* FIXME */ 
    lppe->th32ModuleID        = 0;  /* FIXME */
    lppe->cntThreads          = req->threads;
    lppe->th32ParentProcessID = 0;  /* FIXME */
    lppe->pcPriClassBase      = req->priority;
    lppe->dwFlags             = -1; /* FIXME */
    lppe->szExeFile[0]        = 0;  /* FIXME */
    return TRUE;
}


/***********************************************************************
 *		Process32First    (KERNEL32.555)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32First(HANDLE hSnapshot, LPPROCESSENTRY lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, TRUE );
}

/***********************************************************************
 *		Process32Next   (KERNEL32.556)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32Next(HANDLE hSnapshot, LPPROCESSENTRY lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, FALSE );
}

/***********************************************************************
 *		Module32First   (KERNEL32.527)
 *
 * Return info about the "first" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32First(HANDLE hSnapshot, LPMODULEENTRY lpme)
{
    FIXME("(%d,%p),stub!\n",hSnapshot,lpme);
    return FALSE;
}

/***********************************************************************
 *		Module32Next   (KERNEL32.528)
 *
 * Return info about the "next" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32Next(HANDLE hSnapshot, LPMODULEENTRY lpme)
{
    FIXME("(%d,%p),stub!\n",hSnapshot,lpme);
    return FALSE;
}

/************************************************************************
 *              GlobalMasterHandle16 (KERNEL.28)
 *
 *
 * Should return selector and handle of the information structure for 
 * the global heap. selector and handle are stored in the THHOOK as
 * pGlobalHeap and hGlobalHeap.
 * As Wine doesn't have this structure, we return both values as zero
 * Applications should interpret this as "No Global Heap"
 */
DWORD WINAPI GlobalMasterHandle16(void)
{
    FIXME(": stub\n");
    return 0;
}
