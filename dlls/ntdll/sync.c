/*
 *	Process synchronisation
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "debugstr.h"
#include "debug.h"

#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(ntdll)

/*
 *	Semaphore
 */

/******************************************************************************
 *  NtCreateSemaphore	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount) 
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),0x%08lx,0x%08lx) stub!\n",
	SemaphoreHandle, DesiredAccess, ObjectAttributes, 
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	InitialCount, MaximumCount);
	return 0;
}

/******************************************************************************
 *  NtOpenSemaphore	[NTDLL] 
 */
NTSTATUS WINAPI NtOpenSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME(ntdll,"(0x%08x,0x%08lx,%p(%s)) stub!\n",
	SemaphoreHandle, DesiredAcces, ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}

/******************************************************************************
 *  NtQuerySemaphore	[NTDLL] 
 */
NTSTATUS WINAPI NtQuerySemaphore(
	HANDLE SemaphoreHandle,
	PVOID SemaphoreInformationClass,
	OUT PVOID SemaphoreInformation,
	ULONG Length,
	PULONG ReturnLength) 
{
	FIXME(ntdll,"(0x%08x,%p,%p,0x%08lx,%p) stub!\n",
	SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, Length, ReturnLength);
	return 0;
}
/******************************************************************************
 *  NtReleaseSemaphore	[NTDLL] 
 */
NTSTATUS WINAPI NtReleaseSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount)
{
	FIXME(ntdll,"(0x%08x,0x%08lx,%p,) stub!\n",
	SemaphoreHandle, ReleaseCount, PreviousCount);
	return 0;
}

/*
 *	Event
 */
 
/**************************************************************************
 *		NtCreateEvent				[NTDLL.71]
 */
NTSTATUS WINAPI NtCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),%08x,%08x): empty stub\n",
	EventHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	ManualReset,InitialState);
	return 0;
}

/******************************************************************************
 *  NtOpenEvent		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s)),stub!\n",
	EventHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}

/******************************************************************************
 *  NtSetEvent		[NTDLL] 
 */
NTSTATUS WINAPI NtSetEvent(
	IN HANDLE EventHandle,
	PULONG NumberOfThreadsReleased)
{
	FIXME(ntdll,"(0x%08x,%p)\n",
	EventHandle, NumberOfThreadsReleased);
	return 0;
}

