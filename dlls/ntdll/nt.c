/*
 * NT basis DLL
 * 
 * This file contains the Nt* API functions of NTDLL.DLL.
 * In the original ntdll.dll they all seem to just call int 0x2e (down to the HAL)
 *
 * Copyright 1996-1998 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wintypes.h"
#include "windef.h"
#include "ntdll.h"
#include "ntdef.h"
#include "ntddk.h"
#include "debugstr.h"
#include "debug.h"

/* move to winbase.h */
typedef VOID (CALLBACK *PTIMERAPCROUTINE)(LPVOID lpArgToCompletionRoutine,DWORD dwTimerLowValue,DWORD dwTimerHighValue);   

/* move to somewhere */
typedef void * POBJDIR_INFORMATION;

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
/**************************************************************************
 *		NtCreateTimer				[NTDLL.87]
 */
NTSTATUS WINAPI NtCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN TIMER_TYPE TimerType)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),0x%08x) stub\n",
	TimerHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	TimerType);
	return 0;
}
/**************************************************************************
 *		NtSetTimer				[NTDLL.221]
 */
NTSTATUS WINAPI NtSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE TimerApcRoutine,
	IN PVOID TimerContext,
	IN BOOLEAN WakeTimer,
	IN ULONG Period OPTIONAL,
	OUT PBOOLEAN PreviousState OPTIONAL)
{
	FIXME(ntdll,"(0x%08x,%p,%p,%p,%08x,0x%08lx,%p) stub\n",
	TimerHandle,DueTime,TimerApcRoutine,TimerContext,WakeTimer,Period,PreviousState);
	return 0;
}

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
    FIXME(ntdll,"(%p,0x%08lx,%p(%s)): stub\n", 
    DirectoryHandle, DesiredAccess, ObjectAttributes,
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
	FIXME(ntdll,"(0x%08x,%p,0x%08lx,0x%08x,0x%08x,%p,%p) stub\n",
		DirObjHandle, DirObjInformation, BufferLength, GetNextIndex,
		IgnoreInputIndex, ObjectIndex, DataWritten);
    return 0xc0000000; /* We don't have any. Whatever. (Yet.) */
}

/******************************************************************************
 * NtQuerySystemInformation [NTDLL.168]
 *
 * ARGUMENTS:
 *  SystemInformationClass	Index to a certain information structure
 *	SystemTimeAdjustmentInformation       SYSTEM_TIME_ADJUSTMENT
 *	SystemCacheInformation                SYSTEM_CACHE_INFORMATION
 *	SystemConfigurationInformation        CONFIGURATION_INFORMATION
 *	observed (class/len): 
 *		0x0/0x2c
 *		0x12/0x18
 *		0x2/0x138
 *		0x8/0x600
 *  SystemInformation	caller supplies storage for the information structure
 *  Length		size of the structure
 *  ResultLength	Data written
 */
NTSTATUS WINAPI NtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,%p,0x%08lx,%p) stub\n",
	SystemInformationClass,SystemInformation,Length,ResultLength);
	ZeroMemory (SystemInformation, Length);
	return 0;
}

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
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,%p): stub\n",
	ObjectHandle, ObjectInformationClass, ObjectInformation, Length, ResultLength);
	return 0;
}


/******************************************************************************
 * NtSetInformationProcess [NTDLL.207]
 */
NTSTATUS WINAPI NtSetInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	IN PVOID ProcessInformation,
	IN ULONG ProcessInformationLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx) stub\n",
	ProcessHandle,ProcessInformationClass,ProcessInformation,ProcessInformationLength);
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
 * NtQueryTimerResolution [NTDLL.129]
 */
NTSTATUS WINAPI NtQueryTimerResolution(DWORD x1,DWORD x2,DWORD x3) 
{
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx), stub!\n",x1,x2,x3);
	return 1;
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
	FIXME(ntdll,"(0x%08x),stub!\n",Handle);
	return 1;
}
/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.] 
*
*/
NTSTATUS WINAPI NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,%p),stub!\n",
	ProcessHandle,ProcessInformationClass,ProcessInformation,ProcessInformationLength,ReturnLength);
	return 0;
}
/******************************************************************************
*  NtQueryInformationThread		[NTDLL.] 
*
*/
NTSTATUS WINAPI NtQueryInformationThread(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,%p),stub!\n",
		ThreadHandle, ThreadInformationClass, ThreadInformation,
		ThreadInformationLength, ReturnLength);
	return 0;
}
/******************************************************************************
*  NtQueryInformationToken		[NTDLL.156] 
*
*/
NTSTATUS WINAPI NtQueryInformationToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) 
{
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtCreatePagingFile		[NTDLL] 
 */
NTSTATUS WINAPI NtCreatePagingFile(
	IN PUNICODE_STRING PageFileName,
	IN ULONG MiniumSize,
	IN ULONG MaxiumSize,
	OUT PULONG ActualSize)
{
	FIXME(ntdll,"(%p,0x%08lx,0x%08lx,%p),stub!\n",
	PageFileName,MiniumSize,MaxiumSize,ActualSize);
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
	FIXME(ntdll,"(0x%08x,%p,0x%08x,%p,0x%08lx,0x%08x,0x%08lx) stub!\n",
	SourceProcessHandle,SourceHandle,TargetProcessHandle,TargetHandle,
	DesiredAccess,InheritHandle,Options);
	*TargetHandle = 0;
	return 0;
}

/******************************************************************************
 *  NtDuplicateToken		[NTDLL] 
 */
NTSTATUS WINAPI NtDuplicateToken(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtAdjustPrivilegesToken		[NTDLL] 
 *
 * FIXME: parameters unsafe
 */
NTSTATUS WINAPI NtAdjustPrivilegesToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN DWORD BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState,
	OUT PDWORD ReturnLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,%p,%p),stub!\n",
	TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
	return 0;
}

/******************************************************************************
 *  NtOpenProcessToken		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenProcessToken(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtSetInformationThread		[NTDLL] 
 */
NTSTATUS WINAPI NtSetInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx),stub!\n",
	ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
	return 0;
}

/******************************************************************************
 *  NtOpenThreadToken		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenThreadToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx) stub\n",x1,x2,x3,x4);
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
 *  NtCreatePort		[NTDLL] 
 */
NTSTATUS WINAPI NtCreatePort(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) 
{
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
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
 *  NtWaitForSingleObject		[NTDLL] 
 */
NTSTATUS WINAPI NtWaitForSingleObject(
	IN PHANDLE Object,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Time)
{
	FIXME(ntdll,"(%p,0x%08x,%p),stub!\n",Object,Alertable,Time);
	return 0;
}

/******************************************************************************
 *  NtConnectPort		[NTDLL] 
 */
NTSTATUS WINAPI NtConnectPort(DWORD x1,PUNICODE_STRING uni,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8) {
	FIXME(ntdll,"(0x%08lx,%s,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
	x1,debugstr_w(uni->Buffer),x3,x4,x5,x6,x7,x8);
	return 0;
}

/******************************************************************************
 *  NtListenPort		[NTDLL] 
 */
NTSTATUS WINAPI NtListenPort(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtRequestWaitReplyPort		[NTDLL] 
 */
NTSTATUS WINAPI NtRequestWaitReplyPort(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
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
	FIXME(ntdll,"(%p,0x%08lx,%p(%s)),stub!\n",
	DirectoryHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}

/******************************************************************************
 * NtMapViewOfSection	[NTDLL] 
 * FUNCTION: Maps a view of a section into the virtual address space of a process
 *
 * ARGUMENTS:
 *  SectionHandle	Handle of the section
 *  ProcessHandle	Handle of the process
 *  BaseAddress		Desired base address (or NULL) on entry
 *			Actual base address of the view on exit
 *  ZeroBits		Number of high order address bits that must be zero
 *  CommitSize		Size in bytes of the initially committed section of the view
 *  SectionOffset	Offset in bytes from the beginning of the section to the beginning of the view
 *  ViewSize		Desired length of map (or zero to map all) on entry 
 			Actual length mapped on exit
 *  InheritDisposition	Specified how the view is to be shared with
 *			child processes
 *  AllocateType	Type of allocation for the pages
 *  Protect		Protection for the committed region of the view
 */
NTSTATUS WINAPI NtMapViewOfSection(
	HANDLE SectionHandle,
	HANDLE ProcessHandle,
	PVOID* BaseAddress,
	ULONG ZeroBits,
	ULONG CommitSize,
	PLARGE_INTEGER SectionOffset,
	PULONG ViewSize,
	SECTION_INHERIT InheritDisposition,
	ULONG AllocationType,
	ULONG Protect)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,0x%08lx,%p,%p,0x%08x,0x%08lx,0x%08lx) stub\n",
	SectionHandle,ProcessHandle,BaseAddress,ZeroBits,CommitSize,SectionOffset,
	ViewSize,InheritDisposition,AllocationType,Protect);
	return 0;
}

/******************************************************************************
 *  NtCreateMailSlotFile	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateMailslotFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7,x8);
	return 0;
}

/******************************************************************************
 *  NtReadFile/ZwReadFile			[NTDLL] 
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


/******************************************************************************
 *  NtCreateSection	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateSection(
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN PLARGE_INTEGER MaximumSize OPTIONAL,
	IN ULONG SectionPageProtection OPTIONAL,
	IN ULONG AllocationAttributes,
	IN HANDLE FileHandle OPTIONAL)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s),%p,0x%08lx,0x%08lx,0x%08x) stub\n",
	SectionHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL,
	MaximumSize,SectionPageProtection,AllocationAttributes,FileHandle);
	return 0;
}

/******************************************************************************
 *  NtResumeThread	[NTDLL] 
 */
NTSTATUS WINAPI NtResumeThread(
	IN HANDLE ThreadHandle,
	IN PULONG SuspendCount) 
{
	FIXME(ntdll,"(0x%08x,%p),stub!\n",
	ThreadHandle,SuspendCount);
	return 0;
}

/******************************************************************************
 *  NtReplyWaitReceivePort	[NTDLL] 
 */
NTSTATUS WINAPI NtReplyWaitReceivePort(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtAcceptConnectPort	[NTDLL] 
 */
NTSTATUS WINAPI NtAcceptConnectPort(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtCompleteConnectPort	[NTDLL] 
 */
NTSTATUS WINAPI NtCompleteConnectPort(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 0;
}

/******************************************************************************
 *  NtRegisterThreadTerminatePort	[NTDLL] 
 */
NTSTATUS WINAPI NtRegisterThreadTerminatePort(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 0;
}

/******************************************************************************
 *  NtTerminateThread	[NTDLL] 
 */
NTSTATUS WINAPI NtTerminateThread(
	IN HANDLE ThreadHandle,
	IN NTSTATUS ExitStatus)
{
	if ( TerminateThread(ThreadHandle,ExitStatus) )
	  return 0;

	return 0xc0000000; /* FIXME: lasterror->ntstatus */
}

/******************************************************************************
 *  NtSetIntervalProfile	[NTDLL] 
 */
NTSTATUS WINAPI NtSetIntervalProfile(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtOpenSection	[NTDLL] 
 */
NTSTATUS WINAPI NtOpenSection(
	PHANDLE SectionHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s)),stub!\n",
	SectionHandle,DesiredAccess,ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}
/******************************************************************************
 *  NtQueryPerformanceCounter	[NTDLL] 
 */
NTSTATUS WINAPI NtQueryPerformanceCounter(
	IN PLARGE_INTEGER Counter,
	IN PLARGE_INTEGER Frequency) 
{
	FIXME(ntdll,"(%p, 0%p) stub\n",
	Counter, Frequency);
	return 0;
}
/******************************************************************************
 *  NtQuerySection	[NTDLL] 
 */
NTSTATUS WINAPI NtQuerySection(
	IN HANDLE SectionHandle,
	IN PVOID SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,%p,%p,0x%08lx,%p) stub!\n",
	SectionHandle,SectionInformationClass,SectionInformation,Length,ResultLength);
	return 0;
}

/******************************************************************************
 *  NtQuerySecurityObject	[NTDLL] 
 */
NTSTATUS WINAPI NtQuerySecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx) stub!\n",x1,x2,x3,x4,x5);
	return 0;
}
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
 *  NtQuerySemaphore	[NTDLL] 
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

/******************************************************************************
 *  NtCreateSymbolicLinkObject	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING Name)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s), %p) stub\n",
	SymbolicLinkHandle, DesiredAccess, ObjectAttributes, 
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL, Name);
	return 0;
}

/******************************************************************************
 *  NtOpenSymbolicLinkObject	[NTDLL] 
 */
NTSTATUS WINAPI NtOpenSymbolicLinkObject(
	OUT PHANDLE LinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME(ntdll,"(%p,0x%08lx,%p(%s)) stub\n",
	LinkHandle, DesiredAccess, ObjectAttributes,
	ObjectAttributes ? debugstr_w(ObjectAttributes->ObjectName->Buffer) : NULL);
	return 0;
}
/******************************************************************************
 *  NtQuerySymbolicLinkObject	[NTDLL] 
 */
NTSTATUS NtQuerySymbolicLinkObject(
	IN HANDLE LinkHandle,
	IN OUT PUNICODE_STRING LinkTarget,
	OUT PULONG ReturnedLength OPTIONAL)
{
	FIXME(ntdll,"(0x%08x,%p,%p) stub\n",
	LinkHandle, LinkTarget, ReturnedLength);

	return 0;
}
