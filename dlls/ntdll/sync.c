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

    SERVER_START_VAR_REQ( create_semaphore, len )
    {
        req->initial = InitialCount;
        req->max     = MaximumCount;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = SERVER_CALL();
        *SemaphoreHandle = req->handle;
    }
    SERVER_END_VAR_REQ;
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

    SERVER_START_VAR_REQ( open_semaphore, len )
    {
        req->access  = access;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = SERVER_CALL();
        *SemaphoreHandle = req->handle;
    }
    SERVER_END_VAR_REQ;
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
    SERVER_START_REQ( release_semaphore )
    {
        req->handle = handle;
        req->count  = count;
        if (!(ret = SERVER_CALL()))
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

    SERVER_START_VAR_REQ( create_event, len )
    {
        req->manual_reset = ManualReset;
        req->initial_state = InitialState;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = SERVER_CALL();
        *EventHandle = req->handle;
    }
    SERVER_END_VAR_REQ;
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

    SERVER_START_VAR_REQ( open_event, len )
    {
        req->access  = DesiredAccess;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        if (len) memcpy( server_data_ptr(req), attr->ObjectName->Buffer, len );
        ret = SERVER_CALL();
        *EventHandle = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/******************************************************************************
 *  NtSetEvent
 */
NTSTATUS WINAPI NtSetEvent( HANDLE handle, PULONG NumberOfThreadsReleased )
{
    NTSTATUS ret;

    /* FIXME: set NumberOfThreadsReleased */

    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = SET_EVENT;
        ret = SERVER_CALL();
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

    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = RESET_EVENT;
        ret = SERVER_CALL();
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
    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = PULSE_EVENT;
        ret = SERVER_CALL();
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
