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
#include "k32obj.h"
#include "server.h"
#include "debug.h"


/* The K32 snapshot object object */
typedef struct
{
    K32OBJ header;
} SNAPSHOT_OBJECT;

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

BOOL16 WINAPI NotifyRegister( HTASK16 htask, FARPROC16 lpfnCallback,
                              WORD wFlags )
{
    int	i;

    TRACE(toolhelp, "(%x,%lx,%x) called.\n",
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

BOOL16 WINAPI NotifyUnregister( HTASK16 htask )
{
    int	i;
    
    TRACE(toolhelp, "(%x) called.\n", htask );
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

BOOL16 WINAPI StackTraceCSIPFirst(STACKTRACEENTRY *ste, WORD wSS, WORD wCS, WORD wIP, WORD wBP)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceFirst(STACKTRACEENTRY *ste, HTASK16 Task)
{
    return TRUE;
}

BOOL16 WINAPI StackTraceNext(STACKTRACEENTRY *ste)
{
    return TRUE;
}

/***********************************************************************
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook(FARPROC16 lpfnNotifyHandler)
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
HANDLE32 WINAPI CreateToolhelp32Snapshot( DWORD flags, DWORD process ) 
{
    SNAPSHOT_OBJECT *snapshot;
    struct create_snapshot_request req;
    struct create_snapshot_reply reply;

    TRACE( toolhelp, "%lx,%lx\n", flags, process );
    if (flags & (TH32CS_SNAPHEAPLIST|TH32CS_SNAPMODULE|TH32CS_SNAPTHREAD))
        FIXME( toolhelp, "flags %lx not implemented\n", flags );
    if (!(flags & TH32CS_SNAPPROCESS))
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return INVALID_HANDLE_VALUE32;
    }
    /* Now do the snapshot */
    if (!(snapshot = HeapAlloc( SystemHeap, 0, sizeof(*snapshot) )))
        return INVALID_HANDLE_VALUE32;
    snapshot->header.type = K32OBJ_TOOLHELP_SNAPSHOT;
    snapshot->header.refcount = 1;

    req.flags   = flags & ~TH32CS_INHERIT;
    req.inherit = (flags & TH32CS_INHERIT) != 0;
    CLIENT_SendRequest( REQ_CREATE_SNAPSHOT, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ))
    {
        HeapFree( SystemHeap, 0, snapshot );
        return INVALID_HANDLE_VALUE32;
    }
    return HANDLE_Alloc( PROCESS_Current(), &snapshot->header, 0, req.inherit, reply.handle );
}


/***********************************************************************
 *		TOOLHELP_Process32Next
 *
 * Implementation of Process32First/Next
 */
static BOOL32 TOOLHELP_Process32Next( HANDLE32 handle, LPPROCESSENTRY32 lppe, BOOL32 first )
{
    struct next_process_request req;
    struct next_process_reply reply;

    if (lppe->dwSize < sizeof (PROCESSENTRY32))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR (toolhelp, "Result buffer too small\n");
        return FALSE;
    }
    if ((req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                              K32OBJ_TOOLHELP_SNAPSHOT, 0 )) == -1)
        return FALSE;
    req.reset = first;
    CLIENT_SendRequest( REQ_NEXT_PROCESS, -1, 1, &req, sizeof(req) );
    if (CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL )) return FALSE;
    lppe->cntUsage            = 1;
    lppe->th32ProcessID       = (DWORD)reply.pid;
    lppe->th32DefaultHeapID   = 0;  /* FIXME */ 
    lppe->th32ModuleID        = 0;  /* FIXME */
    lppe->cntThreads          = reply.threads;
    lppe->th32ParentProcessID = 0;  /* FIXME */
    lppe->pcPriClassBase      = reply.priority;
    lppe->dwFlags             = -1; /* FIXME */
    lppe->szExeFile[0]        = 0;  /* FIXME */
    return TRUE;
}


/***********************************************************************
 *		Process32First    (KERNEL32.555)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL32 WINAPI Process32First(HANDLE32 hSnapshot, LPPROCESSENTRY32 lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, TRUE );
}

/***********************************************************************
 *		Process32Next   (KERNEL32.556)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL32 WINAPI Process32Next(HANDLE32 hSnapshot, LPPROCESSENTRY32 lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, FALSE );
}
