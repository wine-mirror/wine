/*
 *	Process synchronisation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "debugstr.h"
#include "debugtools.h"

#include "winerror.h"
#include "server.h"
#include "ntddk.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

/* copy a key name into the request buffer */
static inline NTSTATUS copy_nameU( LPWSTR Dest, const OBJECT_ATTRIBUTES *attr )
{
    if (attr && attr->ObjectName && attr->ObjectName->Buffer)
    {
        if ((attr->ObjectName->Length) > MAX_PATH) return STATUS_BUFFER_OVERFLOW;
        lstrcpyW( Dest, attr->ObjectName->Buffer );
    }
    else Dest[0] = 0;
    return STATUS_SUCCESS;
}

/*
 *	Semaphores
 */

/******************************************************************************
 *  NtCreateSemaphore
 */
NTSTATUS WINAPI NtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount) 
{
    struct create_semaphore_request *req = get_req_buffer();
    NTSTATUS ret;

    if ((MaximumCount <= 0) || (InitialCount < 0) || (InitialCount > MaximumCount))
        return STATUS_INVALID_PARAMETER;

    *SemaphoreHandle = 0;
    req->initial = InitialCount;
    req->max     = MaximumCount;
    req->inherit = ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT);
    if (!(ret = copy_nameU( req->name, ObjectAttributes )) &&
        !(ret = server_call_noerr( REQ_CREATE_SEMAPHORE ))) *SemaphoreHandle = req->handle;
    return ret;
}

/******************************************************************************
 *  NtOpenSemaphore
 */
NTSTATUS WINAPI NtOpenSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    struct open_semaphore_request *req = get_req_buffer();
    NTSTATUS ret;

    *SemaphoreHandle = 0;
    req->access  = DesiredAcces;
    req->inherit = ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT);
    if (!(ret = copy_nameU( req->name, ObjectAttributes )) &&
        !(ret = server_call_noerr( REQ_OPEN_SEMAPHORE ))) *SemaphoreHandle = req->handle;
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
NTSTATUS WINAPI NtReleaseSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount)
{
    struct release_semaphore_request *req = get_req_buffer();
    NTSTATUS ret;

    if (ReleaseCount < 0) return STATUS_INVALID_PARAMETER;

    req->handle = SemaphoreHandle;
    req->count  = ReleaseCount;
    if (!(ret = server_call_noerr( REQ_RELEASE_SEMAPHORE )))
    {
        if (PreviousCount) *PreviousCount = req->prev_count;
    }
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
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState)
{
    struct create_event_request *req = get_req_buffer();
    NTSTATUS ret;

    *EventHandle = 0;
    req->manual_reset = ManualReset;
    req->initial_state = InitialState;
    req->inherit = ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT);
    if (!(ret = copy_nameU( req->name, ObjectAttributes )) &&
        !(ret = server_call_noerr( REQ_CREATE_EVENT ))) *EventHandle = req->handle;
    return ret;
}

/******************************************************************************
 *  NtOpenEvent
 */
NTSTATUS WINAPI NtOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    struct open_event_request *req = get_req_buffer();
    NTSTATUS ret;

    *EventHandle = 0;
    req->access  = DesiredAccess;
    req->inherit = ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT);
    if (!(ret = copy_nameU( req->name, ObjectAttributes )) &&
        !(ret = server_call_noerr( REQ_OPEN_EVENT ))) *EventHandle = req->handle;
    return ret;
}


/******************************************************************************
 *  NtSetEvent
 */
NTSTATUS WINAPI NtSetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased)
{
    struct event_op_request *req = get_req_buffer();
    FIXME("(0x%08x,%p)\n", EventHandle, NumberOfThreadsReleased);
    req->handle = EventHandle;
    req->op     = SET_EVENT;
    return server_call_noerr( REQ_EVENT_OP );
}

/******************************************************************************
 *  NtResetEvent
 */
NTSTATUS WINAPI NtResetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased)
{
    struct event_op_request *req = get_req_buffer();
    FIXME("(0x%08x,%p)\n", EventHandle, NumberOfThreadsReleased);
    req->handle = EventHandle;
    req->op     = RESET_EVENT;
    return server_call_noerr( REQ_EVENT_OP );
}

/******************************************************************************
 *  NtClearEvent
 *
 * FIXME
 *   same as NtResetEvent ???
 */
NTSTATUS WINAPI NtClearEvent (
	IN HANDLE EventHandle)
{
    return NtResetEvent( EventHandle, NULL );
}

/******************************************************************************
 *  NtPulseEvent
 *
 * FIXME
 *   PulseCount
 */
NTSTATUS WINAPI NtPulseEvent(
	IN HANDLE EventHandle,
	IN PULONG PulseCount)
{
    struct event_op_request *req = get_req_buffer();
    FIXME("(0x%08x,%p)\n", EventHandle, PulseCount);
    req->handle = EventHandle;
    req->op     = PULSE_EVENT;
    return server_call_noerr( REQ_EVENT_OP );
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
