/*
 *	Process synchronisation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "debugtools.h"

#include "winerror.h"
#include "wine/unicode.h"
#include "server.h"
#include "ntddk.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);


/*
 *	Semaphores
 */

/******************************************************************************
 *  NtCreateSemaphore
 */
NTSTATUS WINAPI NtCreateSemaphore( OUT PHANDLE SemaphoreHandle,
                                   IN ACCESS_MASK access,
                                   IN const OBJECT_ATTRIBUTES *attr OPTIONAL,
                                   IN ULONG InitialCount,
                                   IN ULONG MaximumCount )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if ((MaximumCount <= 0) || (InitialCount < 0) || (InitialCount > MaximumCount))
        return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ
    {
        struct create_semaphore_request *req = server_alloc_req( sizeof(*req), len );
        req->initial = InitialCount;
        req->max     = MaximumCount;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = server_call_noerr( REQ_CREATE_SEMAPHORE );
        *SemaphoreHandle = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtOpenSemaphore
 */
NTSTATUS WINAPI NtOpenSemaphore( OUT PHANDLE SemaphoreHandle,
                                 IN ACCESS_MASK access,
                                 IN const OBJECT_ATTRIBUTES *attr )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    SERVER_START_REQ
    {
        struct open_semaphore_request *req = server_alloc_req( sizeof(*req), len );
        req->access  = access;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = server_call_noerr( REQ_OPEN_SEMAPHORE );
        *SemaphoreHandle = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQuerySemaphore
 */
NTSTATUS WINAPI NtQuerySemaphore(
	HANDLE SemaphoreHandle,
	PVOID SemaphoreInformationClass,
	OUT PVOID SemaphoreInformation,
	ULONG Length,
	PULONG ReturnLength) 
{
	FIXME("(0x%08x,%p,%p,0x%08lx,%p) stub!\n",
	SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, Length, ReturnLength);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtReleaseSemaphore
 */
NTSTATUS WINAPI NtReleaseSemaphore( HANDLE handle, ULONG count, PULONG previous )
{
    NTSTATUS ret;
    SERVER_START_REQ
    {
        struct release_semaphore_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->count  = count;
        if (!(ret = server_call_noerr( REQ_RELEASE_SEMAPHORE )))
        {
            if (previous) *previous = req->prev_count;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*
 *	Events
 */
 
/**************************************************************************
 * NtCreateEvent
 */
NTSTATUS WINAPI NtCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN const OBJECT_ATTRIBUTES *attr,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState)
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    SERVER_START_REQ
    {
        struct create_event_request *req = server_alloc_req( sizeof(*req), len );
        req->manual_reset = ManualReset;
        req->initial_state = InitialState;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = server_call_noerr( REQ_CREATE_EVENT );
        *EventHandle = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtOpenEvent
 */
NTSTATUS WINAPI NtOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN const OBJECT_ATTRIBUTES *attr )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    SERVER_START_REQ
    {
        struct open_event_request *req = server_alloc_req( sizeof(*req), len );

        req->access  = DesiredAccess;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = server_call_noerr( REQ_OPEN_EVENT );
        *EventHandle = req->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  NtSetEvent
 */
NTSTATUS WINAPI NtSetEvent( HANDLE handle, PULONG NumberOfThreadsReleased )
{
    NTSTATUS ret;

    /* FIXME: set NumberOfThreadsReleased */

    SERVER_START_REQ
    {
        struct event_op_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->op     = SET_EVENT;
        ret = server_call_noerr( REQ_EVENT_OP );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtResetEvent
 */
NTSTATUS WINAPI NtResetEvent( HANDLE handle, PULONG NumberOfThreadsReleased )
{
    NTSTATUS ret;

    /* resetting an event can't release any thread... */
    if (NumberOfThreadsReleased) *NumberOfThreadsReleased = 0;

    SERVER_START_REQ
    {
        struct event_op_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->op     = RESET_EVENT;
        ret = server_call_noerr( REQ_EVENT_OP );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtClearEvent
 *
 * FIXME
 *   same as NtResetEvent ???
 */
NTSTATUS WINAPI NtClearEvent ( HANDLE handle )
{
    return NtResetEvent( handle, NULL );
}

/******************************************************************************
 *  NtPulseEvent
 *
 * FIXME
 *   PulseCount
 */
NTSTATUS WINAPI NtPulseEvent( HANDLE handle, PULONG PulseCount )
{
    NTSTATUS ret;
    FIXME("(0x%08x,%p)\n", handle, PulseCount);
    SERVER_START_REQ
    {
        struct event_op_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        req->op     = PULSE_EVENT;
        ret = server_call_noerr( REQ_EVENT_OP );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQueryEvent
 */
NTSTATUS WINAPI NtQueryEvent (
	IN  HANDLE EventHandle,
	IN  UINT EventInformationClass,
	OUT PVOID EventInformation,
	IN  ULONG EventInformationLength,
	OUT PULONG  ReturnLength)
{
	FIXME("(0x%08x)\n", EventHandle);
	return STATUS_SUCCESS;
}
