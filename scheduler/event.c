/*
 * Win32 events
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "k32obj.h"
#include "process.h"
#include "thread.h"
#include "heap.h"
#include "syslevel.h"
#include "server/request.h"
#include "server.h"

typedef struct
{
    K32OBJ        header;
} EVENT;


/***********************************************************************
 *           CreateEvent32A    (KERNEL32.156)
 */
HANDLE WINAPI CreateEventA( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                BOOL initial_state, LPCSTR name )
{
    struct create_event_request req;
    struct create_event_reply reply;
    int len = name ? strlen(name) + 1 : 0;
    HANDLE handle;
    EVENT *event;

    req.manual_reset = manual_reset;
    req.initial_state = initial_state;
    req.inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);

    CLIENT_SendRequest( REQ_CREATE_EVENT, -1, 2, &req, sizeof(req), name, len );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle == -1) return 0;

    SYSTEM_LOCK();
    event = (EVENT *)K32OBJ_Create( K32OBJ_EVENT, sizeof(*event), name,
                                    reply.handle, EVENT_ALL_ACCESS, sa, &handle );
    if (event)
        K32OBJ_DecCount( &event->header );
    SYSTEM_UNLOCK();
    if (handle == INVALID_HANDLE_VALUE) handle = 0;
    return handle;
}


/***********************************************************************
 *           CreateEvent32W    (KERNEL32.157)
 */
HANDLE WINAPI CreateEventW( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                BOOL initial_state, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = CreateEventA( sa, manual_reset, initial_state, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}

/***********************************************************************
 *           WIN16_CreateEvent    (KERNEL.457)
 */
HANDLE WINAPI WIN16_CreateEvent( BOOL manual_reset, BOOL initial_state )
{
    return CreateEventA( NULL, manual_reset, initial_state, NULL );
}


/***********************************************************************
 *           OpenEvent32A    (KERNEL32.536)
 */
HANDLE WINAPI OpenEventA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE handle = 0;
    K32OBJ *obj;
    struct open_named_obj_request req;
    struct open_named_obj_reply reply;
    int len = name ? strlen(name) + 1 : 0;

    req.type    = OPEN_EVENT;
    req.access  = access;
    req.inherit = inherit;
    CLIENT_SendRequest( REQ_OPEN_NAMED_OBJ, -1, 2, &req, sizeof(req), name, len );
    CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL );
    if (reply.handle != -1)
    {
        SYSTEM_LOCK();
        if ((obj = K32OBJ_FindNameType( name, K32OBJ_EVENT )) != NULL)
        {
            handle = HANDLE_Alloc( PROCESS_Current(), obj, access, inherit, reply.handle );
            K32OBJ_DecCount( obj );
            if (handle == INVALID_HANDLE_VALUE)
                handle = 0; /* must return 0 on failure, not -1 */
        }
        else CLIENT_CloseHandle( reply.handle );
        SYSTEM_UNLOCK();
    }
    return handle;
}


/***********************************************************************
 *           OpenEvent32W    (KERNEL32.537)
 */
HANDLE WINAPI OpenEventW( DWORD access, BOOL inherit, LPCWSTR name )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HANDLE ret = OpenEventA( access, inherit, nameA );
    if (nameA) HeapFree( GetProcessHeap(), 0, nameA );
    return ret;
}


/***********************************************************************
 *           EVENT_Operation
 *
 * Execute an event operation (set,reset,pulse).
 */
static BOOL EVENT_Operation( HANDLE handle, enum event_op op )
{
    struct event_op_request req;

    req.handle = HANDLE_GetServerHandle( PROCESS_Current(), handle,
                                         K32OBJ_EVENT, EVENT_MODIFY_STATE );
    if (req.handle == -1) return FALSE;
    req.op = op;
    CLIENT_SendRequest( REQ_EVENT_OP, -1, 1, &req, sizeof(req) );
    return !CLIENT_WaitReply( NULL, NULL, 0 );
}


/***********************************************************************
 *           PulseEvent    (KERNEL32.557)
 */
BOOL WINAPI PulseEvent( HANDLE handle )
{
    return EVENT_Operation( handle, PULSE_EVENT );
}


/***********************************************************************
 *           SetEvent    (KERNEL32.644)
 */
BOOL WINAPI SetEvent( HANDLE handle )
{
    return EVENT_Operation( handle, SET_EVENT );
}


/***********************************************************************
 *           ResetEvent    (KERNEL32.586)
 */
BOOL WINAPI ResetEvent( HANDLE handle )
{
    return EVENT_Operation( handle, RESET_EVENT );
}


/***********************************************************************
 * NOTE: The Win95 VWin32_Event routines given below are really low-level
 *       routines implemented directly by VWin32. The user-mode libraries
 *       implement Win32 synchronisation routines on top of these low-level
 *       primitives. We do it the other way around here :-)
 */

/***********************************************************************
 *       VWin32_EventCreate	(KERNEL.442)
 */
HANDLE WINAPI VWin32_EventCreate(VOID)
{
    HANDLE hEvent = CreateEventA( NULL, FALSE, 0, NULL );
    return ConvertToGlobalHandle( hEvent );
}

/***********************************************************************
 *       VWin32_EventDestroy	(KERNEL.443)
 */
VOID WINAPI VWin32_EventDestroy(HANDLE event)
{
    CloseHandle( event );
}

/***********************************************************************
 *       VWin32_EventWait	(KERNEL.450) 
 */
VOID WINAPI VWin32_EventWait(HANDLE event)
{
    SYSLEVEL_ReleaseWin16Lock();
    WaitForSingleObject( event, INFINITE );
    SYSLEVEL_RestoreWin16Lock();
}

/***********************************************************************
 *       VWin32_EventSet	(KERNEL.451)
 */
VOID WINAPI VWin32_EventSet(HANDLE event)
{
    SetEvent( event );
}

