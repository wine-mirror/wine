/*
	this file defines interfaces mainly exposed to device drivers and
	native nt dll's

*/
#ifndef __WINE_NTDDK_H
#define __WINE_NTDDK_H

#include <ntdef.h>

/* fixme: put it elsewhere */
typedef long BOOL; 
/* end fixme */

/****************** 
 * asynchronous I/O 
 */
/* conflict with X11-includes*/

#undef Status
typedef struct _IO_STATUS_BLOCK 
{	union 
	{ NTSTATUS Status;
	  PVOID Pointer;
	} u;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;    

typedef VOID (NTAPI *PIO_APC_ROUTINE) ( PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved );

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS 
{	ProcessBasicInformation,
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority,
	ProcessRaisePriority,
	ProcessDebugPort,
	ProcessExceptionPort,
	ProcessAccessToken,
	ProcessLdtInformation,
	ProcessLdtSize,
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL,
	ProcessEnableAlignmentFaultFixup,
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount,
	ProcessAffinityMask,
	ProcessPriorityBoost,
	ProcessDeviceMap,
	ProcessSessionInformation,
	ProcessForegroundInformation,
	ProcessWow64Information,
	MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS 
{	ThreadBasicInformation,
	ThreadTimes,
	ThreadPriority,
	ThreadBasePriority,
	ThreadAffinityMask,
	ThreadImpersonationToken,
	ThreadDescriptorTableEntry,
	ThreadEnableAlignmentFaultFixup,
	ThreadEventPair_Reusable,
	ThreadQuerySetWin32StartAddress,
	ThreadZeroTlsCell,
	ThreadPerformanceCount,
	ThreadAmILastThread,
	ThreadIdealProcessor,
	ThreadPriorityBoost,
	ThreadSetTlsArrayAddress,
	ThreadIsIoPending,
	MaxThreadInfoClass
} THREADINFOCLASS;

typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileObjectIdInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileQuotaInformation,
	FileReparsePointInformation,
	FileNetworkOpenInformation,
	FileAttributeTagInformation,
	FileTrackingInformation,
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef enum _SECTION_INHERIT 
{	ViewShare = 1,
	ViewUnmap = 2
} SECTION_INHERIT;
 
/*
	placeholder
*/
typedef enum _OBJECT_INFORMATION_CLASS
{	DunnoTheConstants1
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef enum SYSTEM_INFORMATION_CLASS
{	DunnoTheConstants2
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

/*
 *	NtQuerySystemInformation interface
 */
typedef struct _SYSTEM_TIME_ADJUSTMENT
{
	ULONG	TimeAdjustment;
	BOOL	TimeAdjustmentDisabled;

} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

typedef struct _SYSTEM_CONFIGURATION_INFO 
{
	union 
	{ ULONG	OemId;
	  struct 
	  { WORD	ProcessorArchitecture;
	    WORD	Reserved;
	  } tag1;
	} tag2;
	ULONG	PageSize;
	PVOID	MinimumApplicationAddress;
	PVOID	MaximumApplicationAddress;
	ULONG	ActiveProcessorMask;
	ULONG	NumberOfProcessors;
	ULONG	ProcessorType;
	ULONG	AllocationGranularity;
	WORD	ProcessorLevel;
	WORD	ProcessorRevision;

} SYSTEM_CONFIGURATION_INFO, *PSYSTEM_CONFIGURATION_INFO;


typedef struct _SYSTEM_CACHE_INFORMATION 
{
	ULONG	CurrentSize;
	ULONG	PeakSize;
	ULONG	PageFaultCount;
	ULONG	MinimumWorkingSet;
	ULONG	MaximumWorkingSet;
	ULONG	Unused[4];

} SYSTEM_CACHE_INFORMATION;

/*
	timer
*/
typedef enum _TIMER_TYPE 
{	NotificationTimer,
	SynchronizationTimer
} TIMER_TYPE;

#endif
