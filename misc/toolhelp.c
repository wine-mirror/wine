/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "toolhelp.h"
#include "debug.h"
#include "heap.h"

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

    dprintf_info(toolhelp, "NotifyRegister(%x,%lx,%x) called.\n",
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
    
    dprintf_info(toolhelp, "NotifyUnregister(%x) called.\n", htask );
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
