#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(ntdll)

/**************************************************************************
 *                 NtOpenFile				[NTDLL.127]
 * FUNCTION: Opens a file
 * ARGUMENTS:
 *  FileHandle		Variable that receives the file handle on return
 *  DesiredAccess	Access desired by the caller to the file
 *  ObjectAttributes	Structue describing the file to be opened
 *  IoStatusBlock	Receives details about the result of the operation
 *  ShareAccess		Type of shared access the caller requires
 *  OpenOptions		Options for the file open
 */
NTSTATUS WINAPI NtOpenFile(
	OUT PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),%p,0x%08lx,0x%08lx) stub\n",
	FileHandle, DesiredAccess, ObjectAttributes, 
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	IoStatusBlock, ShareAccess, OpenOptions);
	return 0;
}

/**************************************************************************
 *		NtCreateFile				[NTDLL.73]
 * FUNCTION: Either causes a new file or directory to be created, or it opens
 *  an existing file, device, directory or volume, giving the caller a handle
 *  for the file object. This handle can be used by subsequent calls to
 *  manipulate data within the file or the file object's state of attributes.
 * ARGUMENTS:
 *	FileHandle		Points to a variable which receives the file handle on return
 *	DesiredAccess		Desired access to the file
 *	ObjectAttributes	Structure describing the file
 *	IoStatusBlock		Receives information about the operation on return
 *	AllocationSize		Initial size of the file in bytes
 *	FileAttributes		Attributes to create the file with
 *	ShareAccess		Type of shared access the caller would like to the file
 *	CreateDisposition	Specifies what to do, depending on whether the file already exists
 *	CreateOptions		Options for creating a new file
 *	EaBuffer		Undocumented
 *	EaLength		Undocumented
 */
NTSTATUS WINAPI NtCreateFile(
	OUT PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocateSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength)  
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),%p,%p,0x%08lx,0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx) stub\n",
	FileHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	IoStatusBlock,AllocateSize,FileAttributes,
	ShareAccess,CreateDisposition,CreateOptions,EaBuffer,EaLength);
	return 0;
}

/******************************************************************************
 *  NtReadFile					[NTDLL] 
 *  ZwReadFile
 *
 * Parameters
 *   HANDLE32 		FileHandle
 *   HANDLE32 		Event 		OPTIONAL
 *   PIO_APC_ROUTINE 	ApcRoutine 	OPTIONAL
 *   PVOID 		ApcContext 	OPTIONAL
 *   PIO_STATUS_BLOCK 	IoStatusBlock
 *   PVOID 		Buffer
 *   ULONG 		Length
 *   PLARGE_INTEGER 	ByteOffset 	OPTIONAL
 *   PULONG 		Key 		OPTIONAL
 */
NTSTATUS WINAPI NtReadFile (
	HANDLE FileHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,%p,%p,%p,0x%08lx,%p,%p),stub!\n",
	FileHandle,EventHandle,ApcRoutine,ApcContext,IoStatusBlock,Buffer,Length,ByteOffset,Key);
	return 0;
}

/**************************************************************************
 *		NtDeviceIoControlFile			[NTDLL.94]
 */
NTSTATUS WINAPI NtDeviceIoControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
	IN PVOID UserApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG IoControlCode,
	IN PVOID InputBuffer,
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,%p,%p,0x%08lx,%p,0x%08lx,%p,0x%08lx): empty stub\n",
	DeviceHandle, Event, UserApcRoutine, UserApcContext,
	IoStatusBlock, IoControlCode, InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize);
	return 0;
}

/******************************************************************************
 * NtFsControlFile [NTDLL.108]
 */
NTSTATUS WINAPI NtFsControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG IoControlCode,
	IN PVOID InputBuffer,
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,%p,%p,0x%08lx,%p,0x%08lx,%p,0x%08lx): stub\n",
	DeviceHandle,Event,ApcRoutine,ApcContext,IoStatusBlock,IoControlCode,
	InputBuffer,InputBufferSize,OutputBuffer,OutputBufferSize);
	return 0;
}

/******************************************************************************
 *  NtSetVolumeInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtSetVolumeInformationFile(
	IN HANDLE FileHandle,
	IN PVOID VolumeInformationClass,
	PVOID VolumeInformation,
	ULONG Length) 
{
	FIXME(ntdll,"(0x%08x,%p,%p,0x%08lx) stub\n",
	FileHandle,VolumeInformationClass,VolumeInformation,Length);
	return 0;
}

/******************************************************************************
 *  NtQueryInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtQueryInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass)
{
	FIXME(ntdll,"(0x%08x,%p,%p,0x%08lx,0x%08x),stub!\n",
	FileHandle,IoStatusBlock,FileInformation,Length,FileInformationClass);
	return 0;
}

/******************************************************************************
 *  NtSetInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtSetInformationFile(
	HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass)
{
	FIXME(ntdll,"(0x%08x,%p,%p,0x%08lx,0x%08x)\n",
	FileHandle,IoStatusBlock,FileInformation,Length,FileInformationClass);
	return 0;
}

/******************************************************************************
 *  NtQueryDirectoryFile	[NTDLL]
 *  ZwQueryDirectoryFile
 */
NTSTATUS WINAPI NtQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan)
{
	FIXME (ntdll,"(0x%08x 0x%08x %p %p %p %p 0x%08lx 0x%08x 0x%08x %p 0x%08x\n",
	FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation,
	Length, FileInformationClass, ReturnSingleEntry, 
	debugstr_w(FileName->Buffer),RestartScan);
	return 0;
}
