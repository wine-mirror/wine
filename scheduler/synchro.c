/*
 * Win32 process and thread synchronisation
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>

#include "file.h"  /* for DOSFS_UnixTimeToFileTime */
#include "thread.h"
#include "winerror.h"
#include "wine/server.h"


/***********************************************************************
 *              get_timeout
 */
inline static void get_timeout( struct timeval *when, int timeout )
{
    gettimeofday( when, 0 );
    if (timeout)
    {
        long sec = timeout / 1000;
        if ((when->tv_usec += (timeout - 1000*sec) * 1000) >= 1000000)
        {
            when->tv_usec -= 1000000;
            when->tv_sec++;
        }
        when->tv_sec += sec;
    }
}

static void CALLBACK call_completion_routine(ULONG_PTR data)
{
    async_private* ovp = (async_private*)data;

    ovp->completion_func(ovp->lpOverlapped->Internal,
                         ovp->lpOverlapped->InternalHigh,
                         ovp->lpOverlapped);
    ovp->completion_func=NULL;
    HeapFree(GetProcessHeap(), 0, ovp);
}

static void finish_async(async_private *ovp, DWORD status)
{
    ovp->lpOverlapped->Internal=status;

    /* call ReadFileEx/WriteFileEx's overlapped completion function */
    if(ovp->completion_func)
    {
        QueueUserAPC(call_completion_routine,GetCurrentThread(),(ULONG_PTR)ovp);
    }

    /* remove it from the active list */
    if(ovp->prev)
        ovp->prev->next = ovp->next;
    else
        NtCurrentTeb()->pending_list = ovp->next;

    if(ovp->next)
        ovp->next->prev = ovp->prev;

    ovp->next=NULL;
    ovp->prev=NULL;

    close(ovp->fd);
    if(!ovp->completion_func) HeapFree(GetProcessHeap(), 0, ovp);
}

/***********************************************************************
 *           check_async_list
 *
 * Process a status event from the server.
 */
void WINAPI check_async_list(LPOVERLAPPED overlapped, DWORD status)
{
    async_private *ovp;

    /* fprintf(stderr,"overlapped %p status %x\n",overlapped,status); */

    for(ovp = NtCurrentTeb()->pending_list; ovp; ovp = ovp->next)
        if(ovp->lpOverlapped == overlapped)
            break;

    if(!ovp)
            return;

    if(status != STATUS_ALERTED)
        ovp->lpOverlapped->Internal = status;

    if(ovp->lpOverlapped->Internal==STATUS_PENDING)
        {
        ovp->func(ovp);
        FILE_StartAsync(ovp->handle, ovp->lpOverlapped, ovp->type, 0, ovp->lpOverlapped->Internal);
        }

    if(ovp->lpOverlapped->Internal!=STATUS_PENDING)
        finish_async(ovp,ovp->lpOverlapped->Internal);
}


/***********************************************************************
 *              wait_reply
 *
 * Wait for a reply on the waiting pipe of the current thread.
 */
static int wait_reply( void *cookie )
{
    int signaled;
    struct wake_up_reply reply;
    for (;;)
    {
        int ret;
        ret = read( NtCurrentTeb()->wait_fd[0], &reply, sizeof(reply) );
        if (ret == sizeof(reply))
        {
            if (!reply.cookie) break;  /* thread got killed */
            if (reply.cookie == cookie) return reply.signaled;
            /* we stole another reply, wait for the real one */
            signaled = wait_reply( cookie );
            /* and now put the wrong one back in the pipe */
            for (;;)
            {
                ret = write( NtCurrentTeb()->wait_fd[1], &reply, sizeof(reply) );
                if (ret == sizeof(reply)) break;
                if (ret >= 0) server_protocol_error( "partial wakeup write %d\n", ret );
                if (errno == EINTR) continue;
                server_protocol_perror("wakeup write");
            }
            return signaled;
        }
        if (ret >= 0) server_protocol_error( "partial wakeup read %d\n", ret );
        if (errno == EINTR) continue;
        server_protocol_perror("wakeup read");
    }
    /* the server closed the connection; time to die... */
    SYSDEPS_ExitThread(0);
}


/***********************************************************************
 *              call_apcs
 *
 * Call outstanding APCs.
 */
static void call_apcs( BOOL alertable )
{
    FARPROC proc = NULL;
    FILETIME ft;
    void *args[4];

    for (;;)
    {
        int type = APC_NONE;
        SERVER_START_REQ( get_apc )
        {
            req->alertable = alertable;
            wine_server_set_reply( req, args, sizeof(args) );
            if (!wine_server_call( req ))
            {
                type = reply->type;
                proc = reply->func;
            }
        }
        SERVER_END_REQ;

        switch(type)
        {
        case APC_NONE:
            return;  /* no more APCs */
        case APC_ASYNC:
            proc( args[0], args[1]);
            break;
        case APC_USER:
            proc( args[0] );
            break;
        case APC_TIMER:
            /* convert sec/usec to NT time */
            DOSFS_UnixTimeToFileTime( (time_t)args[0], &ft, (DWORD)args[1] * 10 );
            proc( args[2], ft.dwLowDateTime, ft.dwHighDateTime );
            break;
        default:
            server_protocol_error( "get_apc_request: bad type %d\n", type );
            break;
        }
    }
}

/***********************************************************************
 *              Sleep  (KERNEL32.@)
 */
VOID WINAPI Sleep( DWORD timeout )
{
    WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.@)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL alertable )
{
    DWORD ret = WaitForMultipleObjectsEx( 0, NULL, FALSE, timeout, alertable );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObject( HANDLE handle, DWORD timeout )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE handle, DWORD timeout,
                                    BOOL alertable )
{
    return WaitForMultipleObjectsEx( 1, &handle, FALSE, timeout, alertable );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                     BOOL wait_all, DWORD timeout )
{
    return WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.@)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout,
                                       BOOL alertable )
{
    int ret, cookie;
    struct timeval tv;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (timeout == INFINITE) tv.tv_sec = tv.tv_usec = 0;
    else get_timeout( &tv, timeout );

    for (;;)
    {
        SERVER_START_REQ( select )
        {
            req->flags   = SELECT_INTERRUPTIBLE;
            req->cookie  = &cookie;
            req->sec     = tv.tv_sec;
            req->usec    = tv.tv_usec;
            wine_server_add_data( req, handles, count * sizeof(HANDLE) );

            if (wait_all) req->flags |= SELECT_ALL;
            if (alertable) req->flags |= SELECT_ALERTABLE;
            if (timeout != INFINITE) req->flags |= SELECT_TIMEOUT;

            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (ret == STATUS_PENDING) ret = wait_reply( &cookie );
        if (ret != STATUS_USER_APC) break;
        call_apcs( alertable );
        if (alertable) break;
    }
    if (HIWORD(ret))  /* is it an error code? */
    {
        SetLastError( RtlNtStatusToDosError(ret) );
        ret = WAIT_FAILED;
    }
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WaitForSingleObject16( HANDLE handle, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForSingleObject( handle, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WaitForMultipleObjects16( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL.495)
 */
DWORD WINAPI WaitForMultipleObjectsEx16( DWORD count, const HANDLE *handles,
                                         BOOL wait_all, DWORD timeout, BOOL alertable )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, alertable );
    RestoreThunkLock( mutex_count );
    return retval;
}
