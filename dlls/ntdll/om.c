/*
 *	Object management functions
 */

#include <stdlib.h>
#include <string.h>
#include "debugtools.h"

#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(ntdll)

/* move to somewhere */
typedef void * POBJDIR_INFORMATION;

/*
 *	Generic object functions
 */
 
/******************************************************************************
 * NtQueryObject [NTDLL.161]
 */
NTSTATUS WINAPI NtQueryObject(
	IN HANDLE ObjectHandle,
	IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
	OUT PVOID ObjectInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	FIXME("(0x%08x,0x%08x,%p,0x%08lx,%p): stub\n",
	ObjectHandle, ObjectInformationClass, ObjectInformation, Length, ResultLength);
	return 0;
}

/******************************************************************************
 *  NtQuerySecurityObject	[NTDLL] 
 */
NTSTATUS WINAPI NtQuerySecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) 
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx) stub!\n",x1,x2,x3,x4,x5);
	return 0;
}
/******************************************************************************
 *  NtDuplicateObject		[NTDLL] 
 */
NTSTATUS WINAPI NtDuplicateObject(
	IN HANDLE SourceProcessHandle,
	IN PHANDLE SourceHandle,
	IN HANDLE TargetProcessHandle,
	OUT PHANDLE TargetHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN InheritHandle,
	ULONG Options)
{
	FIXME("(0x%08x,%p,0x%08x,%p,0x%08lx,0x%08x,0x%08lx) stub!\n",
	SourceProcessHandle,SourceHandle,TargetProcessHandle,TargetHandle,
	DesiredAccess,InheritHandle,Options);
	*TargetHandle = 0;
	return 0;
}

/**************************************************************************
 *                 NtClose				[NTDLL.65]
 * FUNCTION: Closes a handle reference to an object
 * ARGUMENTS:
 *	Handle	handle to close
 */
NTSTATUS WINAPI NtClose(
	HANDLE Handle) 
{
	FIXME("(0x%08x),stub!\n",Handle);
	return 1;
}

/******************************************************************************
 *  NtWaitForSingleObject		[NTDLL] 
 */
NTSTATUS WINAPI NtWaitForSingleObject(
	IN PHANDLE Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time)
{
	FIXME("(%p,0x%08x,%p),stub!\n",Object,Alertable,Time);
	return 0;
}

/*
 *	Directory functions
 */

/**************************************************************************
 * NtOpenDirectoryObject [NTDLL.124]
 * FUNCTION: Opens a namespace directory object
 * ARGUMENTS:
 *  DirectoryHandle	Variable which receives the directory handle
 *  DesiredAccess	Desired access to the directory
 *  ObjectAttributes	Structure describing the directory
 * RETURNS: Status
 */
NTSTATUS WINAPI NtOpenDirectoryObject(
	PHANDLE DirectoryHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
    FIXME("(%p,0x%08lx,%p(%s)): stub\n", 
    DirectoryHandle, DesiredAccess, ObjectAttributes,
    ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
    return 0;
}

/******************************************************************************
 *  NtCreateDirectoryObject	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateDirectoryObject(
	PHANDLE DirectoryHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes) 
{
	FIXME("(%p,0x%08lx,%p(%s)),stub!\n",
	DirectoryHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}

/******************************************************************************
 * NtQueryDirectoryObject [NTDLL.149] 
 * FUNCTION: Reads information from a namespace directory
 * ARGUMENTS:
 *  DirObjInformation	Buffer to hold the data read
 *  BufferLength	Size of the buffer in bytes
 *  GetNextIndex	If TRUE then set ObjectIndex to the index of the next object
 *			If FALSE then set ObjectIndex to the number of objects in the directory
 *  IgnoreInputIndex	If TRUE start reading at index 0
 *			If FALSE start reading at the index specified by object index
 *  ObjectIndex		Zero based index into the directory, interpretation depends on IgnoreInputIndex and GetNextIndex
 *  DataWritten		Caller supplied storage for the number of bytes written (or NULL)
 */
NTSTATUS WINAPI NtQueryDirectoryObject(
	IN HANDLE DirObjHandle,
	OUT POBJDIR_INFORMATION DirObjInformation,
	IN ULONG BufferLength,
	IN BOOLEAN GetNextIndex,
	IN BOOLEAN IgnoreInputIndex,
	IN OUT PULONG ObjectIndex,
	OUT PULONG DataWritten OPTIONAL)
{
	FIXME("(0x%08x,%p,0x%08lx,0x%08x,0x%08x,%p,%p) stub\n",
		DirObjHandle, DirObjInformation, BufferLength, GetNextIndex,
		IgnoreInputIndex, ObjectIndex, DataWritten);
    return 0xc0000000; /* We don't have any. Whatever. (Yet.) */
}

/*
 *	Link objects
 */
 
/******************************************************************************
 *  NtOpenSymbolicLinkObject	[NTDLL] 
 */
NTSTATUS WINAPI NtOpenSymbolicLinkObject(
	OUT PHANDLE LinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME("(%p,0x%08lx,%p(%s)) stub\n",
	LinkHandle, DesiredAccess, ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}

/******************************************************************************
 *  NtCreateSymbolicLinkObject	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING Name)
{
	FIXME("(%p,0x%08lx,%p(%s), %p) stub\n",
	SymbolicLinkHandle, DesiredAccess, ObjectAttributes, 
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	debugstr_w(Name->Buffer));
	return 0;
}

/******************************************************************************
 *  NtQuerySymbolicLinkObject	[NTDLL] 
 */
NTSTATUS WINAPI NtQuerySymbolicLinkObject(
	IN HANDLE LinkHandle,
	IN OUT PUNICODE_STRING LinkTarget,
	OUT PULONG ReturnedLength OPTIONAL)
{
	FIXME("(0x%08x,%p,%p) stub\n",
	LinkHandle, debugstr_w(LinkTarget->Buffer), ReturnedLength);

	return 0;
}

