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

#define MAX_NUMBER_OF_FDS 20

static inline int time_before( struct timeval *t1, struct timeval *t2 )
{
    return ((t1->tv_sec < t2->tv_sec) ||
            ((t1->tv_sec == t2->tv_sec) && (t1->tv_usec < t2->tv_usec)));
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
    NtSetEvent(ovp->lpOverlapped->hEvent,NULL);
    if(!ovp->completion_func) HeapFree(GetProcessHeap(), 0, ovp);
}

/***********************************************************************
 *           check_async_list
 *
 *  Create a list of fds for poll to check while waiting on the server
 *  FIXME: this loop is too large, cut into smaller functions
 *         perhaps we could share/steal some of the code in server/select.c?
 */
static void check_async_list(void)
{
    /* FIXME: should really malloc these two arrays */
    struct pollfd fds[MAX_NUMBER_OF_FDS];
    async_private *user[MAX_NUMBER_OF_FDS], *tmp;
    int i, n, r, timeout;
    async_private *ovp, *timeout_user;
    struct timeval now;

    while(1)
    {
        /* the first fd belongs to the server connection */
        fds[0].events=POLLIN;
        fds[0].revents=0;
        fds[0].fd = NtCurrentTeb()->wait_fd[0];

        ovp = NtCurrentTeb()->pending_list;
        timeout = -1;
        timeout_user = NULL;
        gettimeofday(&now,NULL);
        for(n=1; ovp && (n<MAX_NUMBER_OF_FDS); ovp = tmp)
        {
            tmp = ovp->next;

            if(ovp->lpOverlapped->Internal!=STATUS_PENDING)
            {
                finish_async(ovp,STATUS_UNSUCCESSFUL);
                continue;
            }

            if(ovp->timeout && time_before(&ovp->tv,&now))
            {
                finish_async(ovp,STATUS_TIMEOUT);
                continue;
            }

            fds[n].fd=ovp->fd;
            fds[n].events=ovp->event;
            fds[n].revents=0;
            user[n] = ovp;

            if(ovp->timeout && ( (!timeout_user) || time_before(&ovp->tv,&timeout_user->tv)))
            {
                timeout = (ovp->tv.tv_sec - now.tv_sec) * 1000
                        + (ovp->tv.tv_usec - now.tv_usec) / 1000;
                timeout_user = ovp;
            }

            n++;
        }

        /* if there aren't any active asyncs return */
        if(n==1)
            return;

        r = poll(fds, n, timeout);

        /* if there were any errors, return immediately */
        if( (r<0) || (fds[0].revents==POLLNVAL) )
            return;

        if( r==0 )
        {
            finish_async(timeout_user, STATUS_TIMEOUT);
            continue;
        }

        /* search for async operations that are ready */
        for( i=1; i<n; i++)
        {
            if (fds[i].revents)
                user[i]->func(user[i],fds[i].revents);

            if(user[i]->lpOverlapped->Internal!=STATUS_PENDING)
                finish_async(user[i],user[i]->lpOverlapped->Internal);
        }

        if(fds[0].revents == POLLIN)
            return;
    }
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
        if (NtCurrentTeb()->pending_list) check_async_list();
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
        SERVER_START_VAR_REQ( get_apc, sizeof(args) )
        {
            req->alertable = alertable;
            if (!SERVER_CALL())
            {
                type = req->type;
                proc = req->func;
                memcpy( args, server_data_ptr(req), server_data_size(req) );
            }
        }
        SERVER_END_VAR_REQ;

        switch(type)
        {
        case APC_NONE:
            return;  /* no more APCs */
        case APC_ASYNC:
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
    int i, ret, cookie;
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
        SERVER_START_VAR_REQ( select, count * sizeof(int) )
        {
            int *data = server_data_ptr( req );

            req->flags   = SELECT_INTERRUPTIBLE;
            req->cookie  = &cookie;
            req->sec     = tv.tv_sec;
            req->usec    = tv.tv_usec;
            for (i = 0; i < count; i++) data[i] = handles[i];

            if (wait_all) req->flags |= SELECT_ALL;
            if (alertable) req->flags |= SELECT_ALERTABLE;
            if (timeout != INFINITE) req->flags |= SELECT_TIMEOUT;

            ret = SERVER_CALL();
        }
        SERVER_END_VAR_REQ;
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
