/*
 * Kernel Services Thread
 *
 * Copyright 1999 Ulrich Weigand
 */

#include <sys/time.h>
#include <unistd.h>

#include "services.h"
#include "process.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(timer)

typedef struct _SERVICE
{
   struct _SERVICE 	*next;
   HANDLE           	self;

   PAPCFUNC       	callback;
   ULONG_PTR      	callback_arg;

   BOOL                 disabled;
   HANDLE         	object;
} SERVICE;

typedef struct _SERVICETABLE
{
   HANDLE	     	thread;

   SERVICE 		*first;
   DWORD    		counter;

} SERVICETABLE;
 
/***********************************************************************
 *           SERVICE_Loop
 */
static DWORD CALLBACK SERVICE_Loop( SERVICETABLE *service )
{
    HANDLE 	handles[MAXIMUM_WAIT_OBJECTS];
    int 	count = 0;
    DWORD 	retval = WAIT_FAILED;

    while ( TRUE )
    {
        PAPCFUNC callback;
        ULONG_PTR callback_arg;
        SERVICE *s;

        /* Check whether some condition is fulfilled */

        HeapLock( GetProcessHeap() );

        callback = NULL;
        callback_arg = 0L;
        for ( s = service->first; s; s = s->next )
        {
            if (s->disabled) continue;

            if ( retval >= WAIT_OBJECT_0 && retval < WAIT_OBJECT_0 + count )
            {
                if ( handles[retval - WAIT_OBJECT_0] == s->object )
                {
                    retval = WAIT_TIMEOUT;
                    callback = s->callback;
                    callback_arg = s->callback_arg;
                    break;
                }
            }
        }

        HeapUnlock( GetProcessHeap() );
        
        /* If found, call callback routine */

        if ( callback )
        {
	    callback( callback_arg );
            continue;
        }

        /* If not found, determine wait condition */

        HeapLock( GetProcessHeap() );

        count = 0;
        for ( s = service->first; s; s = s->next )
        {
            if (s->disabled) continue;

            if ( count < MAXIMUM_WAIT_OBJECTS )
                handles[count++] = s->object;
        }

        HeapUnlock( GetProcessHeap() );


        /* Wait until some condition satisfied */

        TRACE("Waiting for %d objects\n", count );

        retval = WaitForMultipleObjectsEx( count, handles, FALSE, INFINITE, TRUE );

        TRACE("Wait returned: %ld\n", retval );
    }

    return 0L;
}

/***********************************************************************
 *           SERVICE_CreateServiceTable
 */
static	BOOL	SERVICE_CreateServiceTable( void )
{
    HANDLE 		thread;
    SERVICETABLE	*service_table;
    PDB			*pdb = PROCESS_Current();

    service_table = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SERVICETABLE) );
    if ( !service_table )
    {
        return FALSE;
    }

    /* service_table field in PDB must be set *BEFORE* calling CreateThread
     * otherwise the thread cleanup service will cause an infinite recursion
     * when installed
     */
    pdb->service_table = service_table;

    thread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)SERVICE_Loop, 
                           service_table, 0, NULL );
    if ( thread == INVALID_HANDLE_VALUE )
    {
        pdb->service_table = 0;
	HeapFree( GetProcessHeap(), 0, service_table );
        return FALSE;
    }
    
    service_table->thread = thread;

    return TRUE;
}

/***********************************************************************
 *           SERVICE_AddObject
 *
 * Warning: the object supplied by the caller must not be closed. It'll
 * be destroyed when the service is deleted. It's up to the caller
 * to ensure that object will not be destroyed in between.
 */
HANDLE SERVICE_AddObject( HANDLE object, 
                          PAPCFUNC callback, ULONG_PTR callback_arg )
{
    SERVICE 		*s;
    SERVICETABLE 	*service_table;
    HANDLE 		handle;

    if ( object == INVALID_HANDLE_VALUE || !callback ) 
        return INVALID_HANDLE_VALUE;

    if (PROCESS_Current()->service_table == 0 && !SERVICE_CreateServiceTable())
       return INVALID_HANDLE_VALUE;
    service_table = PROCESS_Current()->service_table;

    s = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SERVICE) );
    if ( !s ) return INVALID_HANDLE_VALUE;

    s->callback = callback;
    s->callback_arg = callback_arg;
    s->object = object;
    s->disabled = FALSE;

    HeapLock( GetProcessHeap() );

    s->self = handle = (HANDLE)++service_table->counter;
    s->next = service_table->first;
    service_table->first = s;

    HeapUnlock( GetProcessHeap() );

    QueueUserAPC( NULL, service_table->thread, 0L );

    return handle;
}

/***********************************************************************
 *           SERVICE_AddTimer
 */
HANDLE SERVICE_AddTimer( LONG rate, 
                         PAPCFUNC callback, ULONG_PTR callback_arg )
{
    HANDLE handle, ret;
    LARGE_INTEGER when;

    if ( !rate || !callback ) 
        return INVALID_HANDLE_VALUE;

    handle = CreateWaitableTimerA( NULL, FALSE, NULL );
    if (!handle) return INVALID_HANDLE_VALUE;

    rate = (rate + 500) / 1000;  /* us -> ms */
    if (!rate) rate = 1;
    when.s.LowPart = when.s.HighPart = 0;
    if (!SetWaitableTimer( handle, &when, rate, NULL, NULL, FALSE ))
    {
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE;
    }

    if ((ret = SERVICE_AddObject( handle, callback, callback_arg )) == INVALID_HANDLE_VALUE)
    {
        CloseHandle( handle );
        return INVALID_HANDLE_VALUE;
    }
    return ret;
}

/***********************************************************************
 *           SERVICE_Delete
 */
BOOL SERVICE_Delete( HANDLE service )
{
    HANDLE 		handle = INVALID_HANDLE_VALUE;
    BOOL 		retv = FALSE;
    SERVICE 		**s, *next;
    SERVICETABLE 	*service_table;

    /* service table must have been created on previous SERVICE_Add??? call */
    if ((service_table = PROCESS_Current()->service_table) == 0)
       return retv;

    HeapLock( GetProcessHeap() );

    for ( s = &service_table->first; *s; s = &(*s)->next )
    {
        if ( (*s)->self == service )
        {
            handle = (*s)->object;
            next = (*s)->next;
            HeapFree( GetProcessHeap(), 0, *s );
            *s = next;
            retv = TRUE;
            break;
        }
    }

    HeapUnlock( GetProcessHeap() );

    if ( handle != INVALID_HANDLE_VALUE )
        CloseHandle( handle );

    QueueUserAPC( NULL, service_table->thread, 0L );

    return retv;
}

/***********************************************************************
 *           SERVICE_Enable
 */
BOOL SERVICE_Enable( HANDLE service )
{
    BOOL 		retv = FALSE;
    SERVICE 		*s;
    SERVICETABLE 	*service_table;

    /* service table must have been created on previous SERVICE_Add??? call */
    if ((service_table = PROCESS_Current()->service_table) == 0)
       return retv;

    HeapLock( GetProcessHeap() );

    for ( s = service_table->first; s; s = s->next ) 
    {
        if ( s->self == service )
        {
            s->disabled = FALSE;
            retv = TRUE;
            break;
        }
    }

    HeapUnlock( GetProcessHeap() );

    QueueUserAPC( NULL, service_table->thread, 0L );

    return retv;
}

/***********************************************************************
 *           SERVICE_Disable
 */
BOOL SERVICE_Disable( HANDLE service )
{
    BOOL 		retv = TRUE;
    SERVICE 		*s;
    SERVICETABLE 	*service_table;

    /* service table must have been created on previous SERVICE_Add??? call */
    if ((service_table = PROCESS_Current()->service_table) == 0)
       return retv;

    HeapLock( GetProcessHeap() );

    for ( s = service_table->first; s; s = s->next ) 
    {
        if ( s->self == service )
        {
            s->disabled = TRUE;
            retv = TRUE;
            break;
        }
    }

    HeapUnlock( GetProcessHeap() );

    QueueUserAPC( NULL, service_table->thread, 0L );

    return retv;
}


