/*
	this file defines interfaces mainly exposed to device drivers and
	native nt dll's

*/
#ifndef __WINE_NTDDK_H
#define __WINE_NTDDK_H

#include <ntdef.h>
#include <winnt.h>
#include "winbase.h"	/* fixme: should be taken out sometimes */

#ifdef __cplusplus
extern "C" {
#endif

/****************** 
 * asynchronous I/O 
 */
#undef Status	/* conflict with X11-includes*/

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
{
	ViewShare = 1,
	ViewUnmap = 2

} SECTION_INHERIT;
 
/*
	placeholder
*/
typedef enum _OBJECT_INFORMATION_CLASS
{
	DunnoTheConstants1

} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;


/*
 *	NtQuerySystemInformation
 */

typedef enum SYSTEM_INFORMATION_CLASS
{	Unknown1 = 1,
	Unknown2,
	Unknown3,
	Unknown4,
	SystemPerformanceInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

/* reading coffee grounds... */
typedef struct _THREAD_INFO
{	DWORD	Unknown1[6];
	DWORD	ThreadID;
	DWORD	Unknown2[3];
	DWORD	Status;
	DWORD	WaitReason;
	DWORD	Unknown3[4];
} THREAD_INFO, PTHREAD_INFO;

typedef struct _VM_COUNTERS_
{	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage;
	ULONG PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _PROCESS_INFO
{	DWORD		Offset;		/* 00 offset to next PROCESS_INFO ok*/
	DWORD		ThreadCount;	/* 04 number of ThreadInfo member ok */
	DWORD		Unknown1[6];
	FILETIME	CreationTime;	/* 20 */
	DWORD		Unknown2[5];
	PWCHAR		ProcessName;	/* 3c ok */
	DWORD		BasePriority;
	DWORD		ProcessID;	/* 44 ok*/
	DWORD		ParentProcessID;
	DWORD		HandleCount;
	DWORD		Unknown3[2];	/* 50 */
	ULONG		PeakVirtualSize;
	ULONG		VirtualSize;
	ULONG		PageFaultCount;
	ULONG		PeakWorkingSetSize;
	ULONG		WorkingSetSize;
	ULONG		QuotaPeakPagedPoolUsage;
	ULONG		QuotaPagedPoolUsage;
	ULONG		QuotaPeakNonPagedPoolUsage;
	ULONG		QuotaNonPagedPoolUsage;
	ULONG		PagefileUsage;
	ULONG		PeakPagefileUsage;
	DWORD		PrivateBytes;
	DWORD		Unknown6[4];
	THREAD_INFO 	ati[ANYSIZE_ARRAY];	/* 94 size=0x40*/
} PROCESS_INFO, PPROCESS_INFO;

NTSTATUS WINAPI NtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG Length,
	OUT PULONG ResultLength);

/*
 *	system configuration
 */


typedef struct _SYSTEM_TIME_ADJUSTMENT
{
	ULONG	TimeAdjustment;
	BOOLEAN	TimeAdjustmentDisabled;

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
 *	NtQueryProcessInformation
 */

/* parameter ProcessInformationClass */

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

/* parameter ProcessInformation (depending on ProcessInformationClass) */

typedef struct _PROCESS_BASIC_INFORMATION 
{	DWORD	ExitStatus;
	DWORD	PebBaseAddress;
	DWORD	AffinityMask;
	DWORD	BasePriority;
	ULONG	UniqueProcessId;
	ULONG	InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;

NTSTATUS WINAPI NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength);

#define NtCurrentProcess() ( (HANDLE) -1 )

/*
 *	timer
 */

typedef enum _TIMER_TYPE 
{
	NotificationTimer,
	SynchronizationTimer

} TIMER_TYPE;

/*
 *	token functions
 */
 
NTSTATUS WINAPI NtOpenProcessToken(
	HANDLE ProcessHandle,
	DWORD DesiredAccess, 
	HANDLE *TokenHandle);
	
NTSTATUS WINAPI NtOpenThreadToken(
	HANDLE ThreadHandle,
	DWORD DesiredAccess, 
	BOOLEAN OpenAsSelf,
	HANDLE *TokenHandle);

NTSTATUS WINAPI NtAdjustPrivilegesToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN DWORD BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState,
	OUT PDWORD ReturnLength);

NTSTATUS WINAPI NtQueryInformationToken(
	HANDLE token,
	DWORD tokeninfoclass, 
	LPVOID tokeninfo,
	DWORD tokeninfolength,
	LPDWORD retlen );

/*
 *	sid functions
 */

BOOLEAN WINAPI RtlAllocateAndInitializeSid (
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	DWORD nSubAuthorityCount,
	DWORD x3,
	DWORD x4,
	DWORD x5,
	DWORD x6,
	DWORD x7,
	DWORD x8,
	DWORD x9,
	DWORD x10,
	PSID pSid);
	
DWORD WINAPI RtlEqualSid(DWORD x1,DWORD x2);
DWORD WINAPI RtlFreeSid(DWORD x1);
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths);
DWORD WINAPI RtlLengthSid(PSID sid);
DWORD WINAPI RtlInitializeSid(PSID PSID,PSID_IDENTIFIER_AUTHORITY PSIDauth, DWORD c);
LPDWORD WINAPI RtlSubAuthoritySid(PSID PSID,DWORD nr);
LPBYTE WINAPI RtlSubAuthorityCountSid(PSID PSID);
DWORD WINAPI RtlCopySid(DWORD len,PSID to,PSID from);

/*
 *	security descriptor functions
 */

NTSTATUS WINAPI RtlCreateSecurityDescriptor(
	PSECURITY_DESCRIPTOR lpsd,
	DWORD rev);

NTSTATUS WINAPI RtlValidSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor);

ULONG WINAPI RtlLengthSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSTATUS WINAPI RtlGetDaclSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT PBOOLEAN lpbDaclPresent,
	OUT PACL *pDacl,
	OUT PBOOLEAN lpbDaclDefaulted);

NTSTATUS WINAPI RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	BOOLEAN daclpresent,
	PACL dacl,
	BOOLEAN dacldefaulted );

NTSTATUS WINAPI RtlGetSaclSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT PBOOLEAN lpbSaclPresent,
	OUT PACL *pSacl,
	OUT PBOOLEAN lpbSaclDefaulted);

NTSTATUS WINAPI RtlSetSaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	BOOLEAN saclpresent,
	PACL sacl,
	BOOLEAN sacldefaulted);

NTSTATUS WINAPI RtlGetOwnerSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	PSID *Owner,
	PBOOLEAN OwnerDefaulted);

NTSTATUS WINAPI RtlSetOwnerSecurityDescriptor(
	PSECURITY_DESCRIPTOR lpsd,
	PSID owner,
	BOOLEAN ownerdefaulted);

NTSTATUS WINAPI RtlSetGroupSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	PSID group,
	BOOLEAN groupdefaulted);

NTSTATUS WINAPI RtlGetGroupSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	PSID *Group,
	PBOOLEAN GroupDefaulted);

/*	##############################
	######	ACL FUNCTIONS	######
	##############################
*/

DWORD WINAPI RtlCreateAcl(PACL acl,DWORD size,DWORD rev);

BOOLEAN WINAPI RtlFirstFreeAce(
	PACL acl,
	PACE_HEADER *x);

NTSTATUS WINAPI RtlAddAce(
	PACL acl,
	DWORD rev,
	DWORD xnrofaces,
	PACE_HEADER acestart,
	DWORD acelen);
	
DWORD WINAPI RtlAddAccessAllowedAce(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce );

/*
 *	string functions
 */

DWORD WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING uni,PANSI_STRING ansi,BOOLEAN doalloc);
DWORD WINAPI RtlOemStringToUnicodeString(PUNICODE_STRING uni,PSTRING ansi,BOOLEAN doalloc);
DWORD WINAPI RtlMultiByteToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen);
DWORD WINAPI RtlOemToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen);
VOID WINAPI RtlInitAnsiString(PANSI_STRING target,LPCSTR source);
VOID WINAPI RtlInitString(PSTRING target,LPCSTR source);
VOID WINAPI RtlInitUnicodeString(PUNICODE_STRING target,LPCWSTR source);
VOID WINAPI RtlFreeUnicodeString(PUNICODE_STRING str);
VOID WINAPI RtlFreeAnsiString(PANSI_STRING AnsiString);
DWORD WINAPI RtlUnicodeToOemN(LPSTR oemstr,DWORD oemlen,LPDWORD reslen,LPWSTR unistr,DWORD unilen);
DWORD WINAPI RtlUnicodeStringToOemString(PANSI_STRING oem,PUNICODE_STRING uni,BOOLEAN alloc);
DWORD WINAPI RtlUnicodeStringToAnsiString(PANSI_STRING oem,PUNICODE_STRING uni,BOOLEAN alloc);
DWORD WINAPI RtlEqualUnicodeString(PUNICODE_STRING s1,PUNICODE_STRING s2,DWORD x);
DWORD WINAPI RtlUpcaseUnicodeString(PUNICODE_STRING dest,PUNICODE_STRING src,BOOLEAN doalloc);
UINT WINAPI RtlxOemStringToUnicodeSize(PSTRING str);
UINT WINAPI RtlxAnsiStringToUnicodeSize(PANSI_STRING str);
DWORD WINAPI RtlIsTextUnicode(LPVOID buf, DWORD len, DWORD *pf);
NTSTATUS WINAPI RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive);

/*
 *	resource functions
 */

typedef struct _RTL_RWLOCK {
	CRITICAL_SECTION	rtlCS;
	HANDLE		hSharedReleaseSemaphore;
	UINT			uSharedWaiters;
	HANDLE		hExclusiveReleaseSemaphore;
	UINT			uExclusiveWaiters;
	INT			iNumberActive;
	HANDLE		hOwningThreadId;
	DWORD			dwTimeoutBoost;
	PVOID			pDebugInfo;
} RTL_RWLOCK, *LPRTL_RWLOCK;

VOID   WINAPI RtlInitializeResource(
	LPRTL_RWLOCK);
	
VOID   WINAPI RtlDeleteResource(
	LPRTL_RWLOCK);
	
BYTE   WINAPI RtlAcquireResourceExclusive(
	LPRTL_RWLOCK, BYTE fWait);
	
BYTE   WINAPI RtlAcquireResourceShared(
	LPRTL_RWLOCK, BYTE fWait);
	
VOID   WINAPI RtlReleaseResource(
	LPRTL_RWLOCK);
	
VOID   WINAPI RtlDumpResource(
	LPRTL_RWLOCK);

/*
	time functions
 */

typedef struct _TIME_FIELDS 
{   CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS;

typedef TIME_FIELDS *PTIME_FIELDS;

VOID WINAPI RtlSystemTimeToLocalTime(
	IN  PLARGE_INTEGER SystemTime,
	OUT PLARGE_INTEGER LocalTime);

VOID WINAPI RtlTimeToTimeFields(
	PLARGE_INTEGER liTime,
	PTIME_FIELDS TimeFields);

BOOLEAN WINAPI RtlTimeFieldsToTime(
	PTIME_FIELDS tfTimeFields,
	PLARGE_INTEGER Time);
	
VOID WINAPI RtlTimeToElapsedTimeFields(
	PLARGE_INTEGER liTime,
	PTIME_FIELDS TimeFields);
	
BOOLEAN WINAPI RtlTimeToSecondsSince1980(
	LPFILETIME ft,
	LPDWORD timeret);

BOOLEAN WINAPI RtlTimeToSecondsSince1970(
	LPFILETIME ft,
	LPDWORD timeret);

/*
	heap functions
*/

/* Data structure for heap definition. This includes various
   sizing parameters and callback routines, which, if left NULL,
   result in default behavior */

typedef struct
{	ULONG	Length;		/* = sizeof(RTL_HEAP_DEFINITION) */
	ULONG	Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

HANDLE WINAPI RtlCreateHeap(
	ULONG Flags,
	PVOID BaseAddress,
	ULONG SizeToReserve,
	ULONG SizeToCommit,
	PVOID Unknown,
	PRTL_HEAP_DEFINITION Definition);

PVOID WINAPI RtlAllocateHeap(
	HANDLE Heap,
	ULONG Flags,
	ULONG Size);


BOOLEAN WINAPI RtlFreeHeap(
	HANDLE Heap,
	ULONG Flags,
	PVOID Address);

/*
 *	misc
 */
void __cdecl DbgPrint(LPCSTR fmt,LPVOID args);
DWORD NtRaiseException ( DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments,CONST ULONG_PTR *lpArguments);
DWORD RtlRaiseException ( DWORD x);
VOID WINAPI RtlAcquirePebLock(void);
VOID WINAPI RtlReleasePebLock(void);
DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlIntegerToChar(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val);
DWORD WINAPI RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6);
DWORD WINAPI RtlDeleteSecurityObject(DWORD x1);
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type);
INT WINAPI RtlExtendedLargeIntegerDivide(LARGE_INTEGER dividend, DWORD divisor, LPDWORD rest);
long long WINAPI RtlExtendedIntegerMultiply(LARGE_INTEGER factor1,INT factor2);
DWORD WINAPI RtlFormatCurrentUserKeyPath(DWORD x);
DWORD WINAPI RtlOpenCurrentUser(DWORD x1, DWORD *x2);
BOOLEAN WINAPI RtlDosPathNameToNtPathName_U( LPWSTR from,PUNICODE_STRING us,DWORD x2,DWORD x3);
DWORD WINAPI RtlCreateEnvironment(DWORD x1,DWORD x2);
DWORD WINAPI RtlDestroyEnvironment(DWORD x);
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) ;

BOOL WINAPI IsValidSid(PSID);
BOOL WINAPI EqualSid(PSID,PSID);
BOOL WINAPI EqualPrefixSid(PSID,PSID);
DWORD  WINAPI GetSidLengthRequired(BYTE);
BOOL WINAPI AllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,
                                       DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,
                                       DWORD,PSID*);
VOID*  WINAPI FreeSid(PSID);
BOOL WINAPI InitializeSecurityDescriptor(SECURITY_DESCRIPTOR*,DWORD);
BOOL WINAPI InitializeSid(PSID,PSID_IDENTIFIER_AUTHORITY,BYTE);
DWORD* WINAPI GetSidSubAuthority(PSID,DWORD);
BYTE * WINAPI GetSidSubAuthorityCount(PSID);
DWORD  WINAPI GetLengthSid(PSID);
BOOL WINAPI CopySid(DWORD,PSID,PSID);
BOOL WINAPI LookupAccountSidA(LPCSTR,PSID,LPCSTR,LPDWORD,LPCSTR,LPDWORD,
                                  PSID_NAME_USE);
BOOL WINAPI LookupAccountSidW(LPCWSTR,PSID,LPCWSTR,LPDWORD,LPCWSTR,LPDWORD,
                                  PSID_NAME_USE);
PSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority(PSID);

#ifdef __cplusplus
}
#endif

#endif
