/*
 * Kernel Services Thread
 *
 * Copyright 1999 Ulrich Weigand
 */

#include <sys/time.h>
#include <unistd.h>

#include "services.h"
#include "debug.h"


#define SERVICE_USE_OBJECT      0x0001
#define SERVICE_USE_TIMEOUT     0x0002
#define SERVICE_DISABLED        0x8000

typedef struct _SERVICE
{
   struct _SERVICE *next;
   HANDLE           self;

   PAPCFUNC       callback;
   ULONG_PTR      callback_arg;

   int            flags;

   HANDLE         object;
   long           rate;

   struct timeval expire;

} SERVICE;

typedef struct
{
   HANDLE heap;
   HANDLE thread;

   SERVICE *first;
   DWORD    counter;

} SERVICETABLE;
 
static SERVICETABLE *Service = NULL;


/***********************************************************************
 *           SERVICE_AddTimeval
 */
static void SERVICE_AddTimeval( struct timeval *time, long delta )
{
    delta += time->tv_usec;
    time->tv_sec += delta / 1000000L;
    time->tv_usec = delta % 1000000L;
}

/***********************************************************************
 *           SERVICE_DiffTimeval
 */
static long SERVICE_DiffTimeval( struct timeval *time1, struct timeval *time2 )
{
    return   ( time1->tv_sec  - time2->tv_sec  ) * 1000000L
           + ( time1->tv_usec - time2->tv_usec ); 
}

/***********************************************************************
 *           SERVICE_Loop
 */
static DWORD CALLBACK SERVICE_Loop( SERVICETABLE *service )
{
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    int count = 0;
    DWORD timeout = INFINITE;
    DWORD retval = WAIT_FAILED;

    while ( TRUE )
    {
        PAPCFUNC callback;
        ULONG_PTR callback_arg;
        SERVICE *s;

        /* Check whether some condition is fulfilled */

        struct timeval curTime;
        gettimeofday( &curTime, NULL );

        HeapLock( service->heap );

        callback = NULL;
        callback_arg = 0L;
        for ( s = service->first; s; s = s->next )
        {
            if ( s->flags & SERVICE_DISABLED )
                continue;

            if ( s->flags & SERVICE_USE_OBJECT )
                if ( retval >= WAIT_OBJECT_0 && retval < WAIT_OBJECT_0 + count )
                    if ( handles[retval - WAIT_OBJECT_0] == s->object )
                    {
                        callback = s->callback;
                        callback_arg = s->callback_arg;
                        break;
                    }

            if ( s->flags & SERVICE_USE_TIMEOUT )
                if ( timercmp( &s->expire, &curTime, < ) )
                {
                    SERVICE_AddTimeval( &s->expire, s->rate );
                    callback = s->callback;
                    callback_arg = s->callback_arg;
                    break;
                }
        }

        HeapUnlock( service->heap );

        
        /* If found, call callback routine */

        if ( callback )
        {
            callback( callback_arg );
            continue;
        }


        /* If not found, determine wait condition */

        HeapLock( service->heap );

        count = 0;
        timeout = INFINITE;
        for ( s = service->first; s; s = s->next )
        {
            if ( s->flags & SERVICE_DISABLED )
                continue;

            if ( s->flags & SERVICE_USE_OBJECT )
                handles[count++] = s->object;

            if ( s->flags & SERVICE_USE_TIMEOUT )
            {
                long delta = SERVICE_DiffTimeval( &s->expire, &curTime );
                long time  = (delta + 999L) / 1000L;
                if ( time < 1 ) time = 1; 
                if ( time < timeout ) timeout =  time;
            }
        }

        HeapUnlock( service->heap );


        /* Wait until some condition satisfied */

        TRACE( timer, "Waiting for %d objects with timeout %ld\n", 
                      count, timeout );

        retval = WaitForMultipleObjectsEx( count, handles, 
                                           FALSE, timeout, TRUE );

        TRACE( timer, "Wait returned: %ld\n", retval );
    }

    return 0L;
}

/***********************************************************************
 *           SERVICE_Init
 */
BOOL SERVICE_Init( void )
{
    HANDLE heap, thread;

    heap = HeapCreate( HEAP_GROWABLE, 0x1000, 0 );
    if ( !heap ) return FALSE;

    Service = HeapAlloc( heap, HEAP_ZERO_MEMORY, sizeof(SERVICETABLE) );
    if ( !Service )
    {
        HeapDestroy( heap );
        return FALSE;
    }

    Service->heap = heap;

    thread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)SERVICE_Loop, 
                           Service, 0, NULL );
    if ( thread == INVALID_HANDLE_VALUE )
    {
        HeapDestroy( heap );
        return FALSE;
    }

    Service->thread = ConvertToGlobalHandle( thread );

    return TRUE;
}

/***********************************************************************
 *           SERVICE_AddObject
 */
HANDLE SERVICE_AddObject( HANDLE object, 
                          PAPCFUNC callback, ULONG_PTR callback_arg )
{
    SERVICE *s;
    HANDLE handle;

    if ( !Service || object == INVALID_HANDLE_VALUE || !callback ) 
        return INVALID_HANDLE_VALUE;

    s = HeapAlloc( Service->heap, HEAP_ZERO_MEMORY, sizeof(SERVICE) );
    if ( !s ) return INVALID_HANDLE_VALUE;

    HeapLock( Service->heap );

    s->callback = callback;
    s->callback_arg = callback_arg;
    s->object = object;
    s->flags = SERVICE_USE_OBJECT;

    s->self = handle = (HANDLE)++Service->counter;
    s->next = Service->first;
    Service->first = s;

    HeapUnlock( Service->heap );

    QueueUserAPC( NULL, Service->thread, 0L );

    return handle;
}

/***********************************************************************
 *           SERVICE_AddTimer
 */
HANDLE SERVICE_AddTimer( LONG rate, 
                         PAPCFUNC callback, ULONG_PTR callback_arg )
{
    SERVICE *s;
    HANDLE handle;

    if ( !Service || !rate || !callback ) 
        return INVALID_HANDLE_VALUE;

    s = HeapAlloc( Service->heap, HEAP_ZERO_MEMORY, sizeof(SERVICE) );
    if ( !s ) return INVALID_HANDLE_VALUE;

    HeapLock( Service->heap );

    s->callback = callback;
    s->callback_arg = callback_arg;
    s->rate = rate;
    s->flags = SERVICE_USE_TIMEOUT;

    gettimeofday( &s->expire, NULL );
    SERVICE_AddTimeval( &s->expire, s->rate );
            
    s->self = handle = (HANDLE)++Service->counter;
    s->next = Service->first;
    Service->first = s;

    HeapUnlock( Service->heap );

    QueueUserAPC( NULL, Service->thread, 0L );

    return handle;
}

/***********************************************************************
 *           SERVICE_Delete
 */
BOOL SERVICE_Delete( HANDLE service )
{
    BOOL retv = TRUE;
    SERVICE **s;

    if ( !Service ) return retv;

    HeapLock( Service->heap );

    for ( s = &Service->first; *s; s = &(*s)->next )
        if ( (*s)->self == service )
        {
            *s = (*s)->next;
            HeapFree( Service->heap, 0, *s );
            retv = FALSE;
            break;
        }

    HeapUnlock( Service->heap );

    QueueUserAPC( NULL, Service->thread, 0L );

    return retv;
}

/***********************************************************************
 *           SERVICE_Enable
 */
BOOL SERVICE_Enable( HANDLE service )
{
    BOOL retv = TRUE;
    SERVICE *s;

    if ( !Service ) return retv;

    HeapLock( Service->heap );

    for ( s = Service->first; s; s = s->next )
        if ( s->self == service )
        {
            if ( s->flags & SERVICE_DISABLED )
            {
                s->flags &= ~SERVICE_DISABLED;

                if ( s->flags & SERVICE_USE_TIMEOUT )
                {
                    long delta;
                    struct timeval curTime;
                    gettimeofday( &curTime, NULL );

                    delta = SERVICE_DiffTimeval( &s->expire, &curTime );
                    if ( delta > 0 )
                        SERVICE_AddTimeval( &s->expire, 
                                           (delta / s->rate) * s->rate );
                }
            }
            retv = FALSE;
            break;
        }

    HeapUnlock( Service->heap );

    QueueUserAPC( NULL, Service->thread, 0L );

    return retv;
}

/***********************************************************************
 *           SERVICE_Disable
 */
BOOL SERVICE_Disable( HANDLE service )
{
    BOOL retv = TRUE;
    SERVICE *s;

    if ( !Service ) return retv;

    HeapLock( Service->heap );

    for ( s = Service->first; s; s = s->next )
        if ( s->self == service )
        {
            s->flags |= SERVICE_DISABLED;
            retv = FALSE;
            break;
        }

    HeapUnlock( Service->heap );

    QueueUserAPC( NULL, Service->thread, 0L );

    return retv;
}


