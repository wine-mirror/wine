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
static inline NTSTATUS copy_nameU( LPWSTR Dest, PUNICODE_STRING Name )
{
        if (Name->Buffer)
        {
          if ((Name->Length) > MAX_PATH) return STATUS_BUFFER_OVERFLOW;
          lstrcpyW( Dest, Name->Buffer );
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
	HRESULT ret;

	FIXME("(%p,0x%08lx,%p,0x%08lx,0x%08lx) stub!\n",
	SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
	dump_ObjectAttributes(ObjectAttributes);

	if ((MaximumCount <= 0) || (InitialCount < 0) || (InitialCount > MaximumCount))
	  return STATUS_INVALID_PARAMETER;

	*SemaphoreHandle = 0;
	req->initial = InitialCount;
	req->max     = MaximumCount;
	req->inherit = ObjectAttributes->Attributes & OBJ_INHERIT;
	copy_nameU( req->name, ObjectAttributes->ObjectName );
	if (!(ret = server_call_noerr( REQ_CREATE_SEMAPHORE )))
            *SemaphoreHandle = req->handle;
	return ret;
}

/******************************************************************************
 *  NtOpenSemaphore
 *
 * FIXME
 */
NTSTATUS WINAPI NtOpenSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	struct open_semaphore_request *req = get_req_buffer();
	HRESULT ret;

	FIXME("(0x%08x,0x%08lx,%p) stub!\n",
	SemaphoreHandle, DesiredAcces, ObjectAttributes);
	dump_ObjectAttributes(ObjectAttributes);

	req->access  = DesiredAcces;
	req->inherit = ObjectAttributes->Attributes & OBJ_INHERIT;
	copy_nameU( req->name, ObjectAttributes->ObjectName );
	if ((ret = server_call_noerr( REQ_OPEN_SEMAPHORE )) != STATUS_SUCCESS) return -1;
	return req->handle;
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
	HRESULT ret;

	FIXME("(0x%08x,0x%08lx,%p,) stub!\n",
	SemaphoreHandle, ReleaseCount, PreviousCount);

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
	HRESULT ret;

	FIXME("(%p,0x%08lx,%p,%08x,%08x): empty stub\n",
	EventHandle,DesiredAccess,ObjectAttributes,ManualReset,InitialState);
	dump_ObjectAttributes(ObjectAttributes);

	*EventHandle = 0;
	req->manual_reset = ManualReset;
	req->initial_state = InitialState;
	req->inherit = ObjectAttributes->Attributes & OBJ_INHERIT;
	copy_nameU( req->name, ObjectAttributes->ObjectName );
	if (!(ret = server_call_noerr( REQ_CREATE_EVENT ))) *EventHandle = req->handle;
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
	HRESULT ret;

	FIXME("(%p,0x%08lx,%p),stub!\n",
	EventHandle,DesiredAccess,ObjectAttributes);
	dump_ObjectAttributes(ObjectAttributes);

	*EventHandle = 0;
	req->access  = DesiredAccess;
	req->inherit = ObjectAttributes->Attributes & OBJ_INHERIT;
	copy_nameU( req->name, ObjectAttributes->ObjectName );
	if (!(ret = server_call_noerr( REQ_OPEN_EVENT ))) *EventHandle = req->handle;
	return ret;
}

/***********************************************************************
 * EVENT_Operation
 *
 * Execute an event operation (set,reset,pulse).
 */
static NTSTATUS EVENT_Operation(
	HANDLE handle,
	PULONG NumberOfThreadsReleased,
	enum event_op op )
{
	struct event_op_request *req = get_req_buffer();
	req->handle = handle;
	req->op     = op;
	return server_call_noerr( REQ_EVENT_OP );
}

/******************************************************************************
 *  NtSetEvent
 */
NTSTATUS WINAPI NtSetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased)
{
	FIXME("(0x%08x,%p)\n", EventHandle, NumberOfThreadsReleased);

	return EVENT_Operation(EventHandle, NumberOfThreadsReleased, SET_EVENT);
}

/******************************************************************************
 *  NtResetEvent
 */
NTSTATUS WINAPI NtResetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased)
{
	FIXME("(0x%08x,%p)\n", EventHandle, NumberOfThreadsReleased);

	return EVENT_Operation(EventHandle, NumberOfThreadsReleased, RESET_EVENT);
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
	FIXME("(0x%08x)\n", EventHandle);
	return EVENT_Operation(EventHandle, NULL, RESET_EVENT);
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
	FIXME("(0x%08x,%p)\n", EventHandle, PulseCount);

	return EVENT_Operation(EventHandle, NULL, PULSE_EVENT);
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
