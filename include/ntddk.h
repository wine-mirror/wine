/*
	this file defines interfaces mainly exposed to device drivers and
	native nt dll's

*/
#ifndef __WINE_NTDDK_H
#define __WINE_NTDDK_H

#include "ntdef.h"
#include "winnt.h"
#include "winreg.h"
#include "winbase.h"	/* fixme: should be taken out sometimes */

#ifdef __cplusplus
extern "C" {
#endif

/****************** 
 * asynchronous I/O 
 */
#undef Status	/* conflict with X11-includes*/

typedef struct _IO_STATUS_BLOCK 
{
	union {
	  NTSTATUS Status;
	  PVOID Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;    

typedef VOID NTAPI (*PIO_APC_ROUTINE) ( PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved );

/*
	registry 
 */

 /* key information */
typedef struct _KEY_BASIC_INFORMATION {
	LARGE_INTEGER	LastWriteTime;
	ULONG		TitleIndex;
	ULONG		NameLength;
	WCHAR		Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION 
{
	LARGE_INTEGER	LastWriteTime;
	ULONG		TitleIndex;
	ULONG		ClassOffset;
	ULONG		ClassLength;
	ULONG		NameLength;
	WCHAR		Name[1];
/*	Class[1]; */
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION 
{
	LARGE_INTEGER	LastWriteTime;
	ULONG		TitleIndex;
	ULONG		ClassOffset;
	ULONG		ClassLength;
	ULONG		SubKeys;
	ULONG		MaxNameLen;
	ULONG		MaxClassLen;
	ULONG		Values;
	ULONG		MaxValueNameLen;
	ULONG		MaxValueDataLen;
	WCHAR		Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef enum _KEY_INFORMATION_CLASS 
{
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef struct _KEY_VALUE_ENTRY 
{
	PUNICODE_STRING	ValueName;
	ULONG		DataLength;
	ULONG		DataOffset;
	ULONG		Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

/* value information */
typedef struct _KEY_VALUE_BASIC_INFORMATION 
{
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   NameLength;
	WCHAR   Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION 
{
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   DataOffset;
	ULONG   DataLength;
	ULONG   NameLength;
	WCHAR   Name[1];
/*	UCHAR 	Data[1];*/
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION 
{
	ULONG   TitleIndex;
	ULONG   Type;
	ULONG   DataLength;
	UCHAR   Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS 
{
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

NTSTATUS WINAPI RtlFormatCurrentUserKeyPath(
	PUNICODE_STRING KeyPath);

/*	thread information */

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

/*	file information */

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

typedef enum _FSINFOCLASS {
	FileFsVolumeInformation = 1,
	FileFsLabelInformation,
	FileFsSizeInformation,
	FileFsDeviceInformation,
	FileFsAttributeInformation,
	FileFsControlInformation,
	FileFsFullSizeInformation,
	FileFsObjectIdInformation,
	FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _SECTION_INHERIT 
{
	ViewShare = 1,
	ViewUnmap = 2

} SECTION_INHERIT;
 
/*	object information */

typedef enum _OBJECT_INFORMATION_CLASS
{
	DunnoTheConstants1

} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;


/*	system information */

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

/* process information */

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

/*	token functions */
 
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

/*	sid functions */

BOOLEAN WINAPI RtlAllocateAndInitializeSid (
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount,
	DWORD nSubAuthority0, DWORD nSubAuthority1,
	DWORD nSubAuthority2, DWORD nSubAuthority3,
	DWORD nSubAuthority4, DWORD nSubAuthority5,
	DWORD nSubAuthority6, DWORD nSubAuthority7,
	PSID *pSid );
	
BOOL WINAPI RtlInitializeSid(
	PSID pSid,
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount);
	
DWORD WINAPI RtlFreeSid(
	PSID pSid);

BOOL WINAPI RtlEqualSid(
	PSID pSid1,
	PSID pSid2 );
	
DWORD WINAPI RtlLengthRequiredSid(
	DWORD nrofsubauths);

DWORD WINAPI RtlLengthSid(
	PSID sid);

LPDWORD WINAPI RtlSubAuthoritySid(
	PSID PSID,
	DWORD nr);

LPBYTE WINAPI RtlSubAuthorityCountSid(
	PSID pSid);

DWORD WINAPI RtlCopySid(
	DWORD len,
	PSID to,
	PSID from);
	
BOOL WINAPI RtlValidSid(
	PSID pSid);

BOOL WINAPI RtlEqualPrefixSid(
	PSID pSid1,
	PSID pSid2);

PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid(
	PSID pSid );

/*	security descriptor functions */

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

NTSTATUS WINAPI RtlMakeSelfRelativeSD(
	IN PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
	IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
	IN OUT LPDWORD lpdwBufferLength);

NTSTATUS WINAPI RtlGetControlSecurityDescriptor(
	PSECURITY_DESCRIPTOR  pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL pControl,
	LPDWORD lpdwRevision);

/*	acl functions */

NTSTATUS WINAPI RtlCreateAcl(
	PACL acl,
	DWORD size,
	DWORD rev);

BOOLEAN WINAPI RtlFirstFreeAce(
	PACL acl,
	PACE_HEADER *x);

NTSTATUS WINAPI RtlAddAce(
	PACL acl,
	DWORD rev,
	DWORD xnrofaces,
	PACE_HEADER acestart,
	DWORD acelen);
	
BOOL WINAPI RtlAddAccessAllowedAce(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AccessMask,
	IN PSID pSid);

BOOL WINAPI AddAccessAllowedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid);

DWORD WINAPI RtlGetAce(
	PACL pAcl,
	DWORD dwAceIndex,
	LPVOID *pAce );

/*	string functions */

DWORD       WINAPI RtlAnsiStringToUnicodeSize(const STRING*);
NTSTATUS    WINAPI RtlAnsiStringToUnicodeString(UNICODE_STRING*,const STRING *,BOOLEAN);
NTSTATUS    WINAPI RtlAppendAsciizToString(STRING*,LPCSTR);
NTSTATUS    WINAPI RtlAppendStringToString(STRING*,const STRING*);
NTSTATUS    WINAPI RtlAppendUnicodeStringToString(UNICODE_STRING*,const UNICODE_STRING*);
NTSTATUS    WINAPI RtlAppendUnicodeToString(UNICODE_STRING*,LPCWSTR);
LONG        WINAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);
LONG        WINAPI RtlCompareUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
void        WINAPI RtlCopyString(STRING*,const STRING*);
void        WINAPI RtlCopyUnicodeString(UNICODE_STRING*,const UNICODE_STRING*);
BOOLEAN     WINAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
BOOLEAN     WINAPI RtlCreateUnicodeStringFromAsciiz(PUNICODE_STRING,LPCSTR);
void        WINAPI RtlEraseUnicodeString(UNICODE_STRING*);
BOOLEAN     WINAPI RtlEqualString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN     WINAPI RtlEqualUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
void        WINAPI RtlFreeAnsiString(PSTRING);
void        WINAPI RtlFreeOemString(PSTRING);
void        WINAPI RtlFreeUnicodeString(PUNICODE_STRING);
void        WINAPI RtlInitAnsiString(PSTRING,LPCSTR);
void        WINAPI RtlInitString(PSTRING,LPCSTR);
void        WINAPI RtlInitUnicodeString(PUNICODE_STRING,LPCWSTR);
NTSTATUS    WINAPI RtlMultiByteToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSTATUS    WINAPI RtlMultiByteToUnicodeSize(DWORD*,LPCSTR,UINT);
UINT        WINAPI RtlOemStringToUnicodeSize(const STRING*);
NTSTATUS    WINAPI RtlOemStringToUnicodeString(UNICODE_STRING*,const STRING*,BOOLEAN);
NTSTATUS    WINAPI RtlOemToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
BOOLEAN     WINAPI RtlPrefixString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN     WINAPI RtlPrefixUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
DWORD       WINAPI RtlUnicodeStringToAnsiSize(const UNICODE_STRING*);
NTSTATUS    WINAPI RtlUnicodeStringToAnsiString(STRING*,const UNICODE_STRING*,BOOLEAN);
DWORD       WINAPI RtlUnicodeStringToOemSize(const UNICODE_STRING*);
NTSTATUS    WINAPI RtlUnicodeStringToOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS    WINAPI RtlUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS    WINAPI RtlUnicodeToMultiByteSize(DWORD*,LPCWSTR,UINT);
NTSTATUS    WINAPI RtlUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS    WINAPI RtlUpcaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING *,BOOLEAN);
NTSTATUS    WINAPI RtlUpcaseUnicodeStringToAnsiString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS    WINAPI RtlUpcaseUnicodeStringToOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS    WINAPI RtlUpcaseUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS    WINAPI RtlUpcaseUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);

DWORD WINAPI RtlIsTextUnicode(
	LPVOID buf,
	DWORD len,
	DWORD *pf);

INT __cdecl wcstol(LPCWSTR,LPWSTR*,INT);

/*	resource functions */

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

/*	time functions */

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
	
void    WINAPI NtQuerySystemTime( LARGE_INTEGER *time );

BOOLEAN WINAPI RtlTimeToSecondsSince1980( const FILETIME *time, LPDWORD res );
BOOLEAN WINAPI RtlTimeToSecondsSince1970( const FILETIME *time, LPDWORD res );
void    WINAPI RtlSecondsSince1970ToTime( DWORD time, FILETIME *res );
void    WINAPI RtlSecondsSince1980ToTime( DWORD time, FILETIME *res );

/*	heap functions */

/* Data structure for heap definition. This includes various
   sizing parameters and callback routines, which, if left NULL,
   result in default behavior */

typedef struct
{	ULONG	Length;		/* = sizeof(RTL_HEAP_DEFINITION) */
	ULONG	Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

HANDLE    WINAPI RtlCreateHeap(ULONG,PVOID,ULONG,ULONG,PVOID,PRTL_HEAP_DEFINITION);
HANDLE    WINAPI RtlDestroyHeap(HANDLE);
PVOID     WINAPI RtlAllocateHeap(HANDLE,ULONG,ULONG);
BOOLEAN   WINAPI RtlFreeHeap(HANDLE,ULONG,PVOID);
PVOID     WINAPI RtlReAllocateHeap(HANDLE,ULONG,PVOID,ULONG);
ULONG     WINAPI RtlCompactHeap(HANDLE,ULONG);
BOOLEAN   WINAPI RtlLockHeap(HANDLE);
BOOLEAN   WINAPI RtlUnlockHeap(HANDLE);
ULONG     WINAPI RtlSizeHeap(HANDLE,ULONG,PVOID);
BOOLEAN   WINAPI RtlValidateHeap(HANDLE,ULONG,PCVOID);
ULONG     WINAPI RtlGetProcessHeaps(ULONG,HANDLE*);
NTSTATUS  WINAPI RtlWalkHeap(HANDLE,PVOID);

/*	exception */

void WINAPI NtRaiseException(
	PEXCEPTION_RECORD,PCONTEXT,BOOL);

void WINAPI RtlRaiseException(
	PEXCEPTION_RECORD);

void WINAPI RtlRaiseStatus(
	NTSTATUS);

void WINAPI RtlUnwind(
	PEXCEPTION_FRAME,
	LPVOID,
	PEXCEPTION_RECORD,DWORD);

/*	process environment block  */
VOID WINAPI RtlAcquirePebLock(void);
VOID WINAPI RtlReleasePebLock(void);

/*	mathematics */
LONGLONG  WINAPI RtlConvertLongToLargeInteger( LONG a );
LONGLONG  WINAPI RtlEnlargedIntegerMultiply( INT a, INT b );
LONGLONG  WINAPI RtlExtendedMagicDivide( LONGLONG a, LONGLONG b, INT shift );
LONGLONG  WINAPI RtlExtendedIntegerMultiply( LONGLONG a, INT b );
LONGLONG  WINAPI RtlExtendedLargeIntegerDivide( LONGLONG a, INT b, INT *rem );
LONGLONG  WINAPI RtlLargeIntegerAdd( LONGLONG a, LONGLONG b );
LONGLONG  WINAPI RtlLargeIntegerArithmeticShift( LONGLONG a, INT count );
LONGLONG  WINAPI RtlLargeIntegerNegate( LONGLONG a );
LONGLONG  WINAPI RtlLargeIntegerShiftLeft( LONGLONG a, INT count );
LONGLONG  WINAPI RtlLargeIntegerShiftRight( LONGLONG a, INT count );
LONGLONG  WINAPI RtlLargeIntegerSubtract( LONGLONG a, LONGLONG b );
ULONGLONG WINAPI RtlEnlargedUnsignedMultiply( UINT a, UINT b );
UINT      WINAPI RtlEnlargedUnsignedDivide( ULONGLONG a, UINT b, UINT *remptr );
ULONGLONG WINAPI RtlConvertUlongToLargeInteger( ULONG a );
ULONGLONG WINAPI RtlLargeIntegerDivide( ULONGLONG a, ULONGLONG b, ULONGLONG *rem );

/*	environment */
DWORD WINAPI RtlCreateEnvironment(
	DWORD x1,
	DWORD x2);

DWORD WINAPI RtlDestroyEnvironment(
	DWORD x);

DWORD WINAPI RtlQueryEnvironmentVariable_U(
	DWORD x1,
	PUNICODE_STRING key,
	PUNICODE_STRING val) ;

DWORD WINAPI RtlSetEnvironmentVariable(
	DWORD x1,
	PUNICODE_STRING key,
	PUNICODE_STRING val);

/*	object security */

DWORD WINAPI RtlNewSecurityObject(
	DWORD x1,
	DWORD x2,
	DWORD x3,
	DWORD x4,
	DWORD x5,
	DWORD x6);

DWORD WINAPI RtlDeleteSecurityObject(
	DWORD x1);
	
NTSTATUS WINAPI 
NtQuerySecurityObject(
	IN HANDLE Object,
	IN SECURITY_INFORMATION RequestedInformation,
	OUT PSECURITY_DESCRIPTOR pSecurityDesriptor,
	IN ULONG Length,
	OUT PULONG ResultLength);

NTSTATUS WINAPI
NtSetSecurityObject(
        IN HANDLE Handle,
        IN SECURITY_INFORMATION SecurityInformation,
        IN PSECURITY_DESCRIPTOR SecurityDescriptor);

/*	registry functions */

NTSTATUS    WINAPI NtCreateKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,
                               const UNICODE_STRING*,ULONG,PULONG);
NTSTATUS    WINAPI NtDeleteKey(HANDLE);
NTSTATUS    WINAPI NtDeleteValueKey(HANDLE,const UNICODE_STRING*);
NTSTATUS    WINAPI NtOpenKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSTATUS    WINAPI NtQueryKey(HANDLE,KEY_INFORMATION_CLASS,void*,DWORD,DWORD*);
NTSTATUS    WINAPI NtSetValueKey(HANDLE,const UNICODE_STRING*,ULONG,ULONG,const void*,ULONG);
NTSTATUS    WINAPI NtEnumerateKey(HANDLE,ULONG,KEY_INFORMATION_CLASS,void*,DWORD,DWORD*);
NTSTATUS    WINAPI NtQueryValueKey(HANDLE,const UNICODE_STRING*,KEY_VALUE_INFORMATION_CLASS,
                                   void*,DWORD,DWORD*);
NTSTATUS    WINAPI NtLoadKey(const OBJECT_ATTRIBUTES*,const OBJECT_ATTRIBUTES*);


NTSTATUS WINAPI NtEnumerateValueKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength);

NTSTATUS WINAPI NtFlushKey(HANDLE KeyHandle);

NTSTATUS WINAPI NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous,
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree);

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HANDLE KeyHandle,
	PVALENTW ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength);

NTSTATUS WINAPI NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes);

NTSTATUS WINAPI NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags);

NTSTATUS WINAPI NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle);

NTSTATUS WINAPI NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength);

NTSTATUS WINAPI NtUnloadKey(
	IN HANDLE KeyHandle);

NTSTATUS WINAPI NtClose(
	HANDLE Handle);

NTSTATUS WINAPI NtTerminateProcess( HANDLE handle, LONG exit_code );
NTSTATUS WINAPI NtTerminateThread( HANDLE handle, LONG exit_code );

NTSTATUS WINAPI NtClearEvent(HANDLE);
NTSTATUS WINAPI NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,BOOLEAN,BOOLEAN);
NTSTATUS WINAPI NtCreateSemaphore(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,ULONG);
NTSTATUS WINAPI NtOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *attr);
NTSTATUS WINAPI NtPulseEvent(HANDLE,PULONG);
NTSTATUS WINAPI NtReleaseSemaphore(HANDLE,ULONG,PULONG);
NTSTATUS WINAPI NtResetEvent(HANDLE,PULONG);
NTSTATUS WINAPI NtSetEvent(HANDLE,PULONG);

NTSTATUS WINAPI RtlInitializeCriticalSection( RTL_CRITICAL_SECTION *crit );
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount( RTL_CRITICAL_SECTION *crit, DWORD spincount );
NTSTATUS WINAPI RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit );
NTSTATUS WINAPI RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit );
NTSTATUS WINAPI RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit );
NTSTATUS WINAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit );
BOOL     WINAPI RtlTryEnterCriticalSection( RTL_CRITICAL_SECTION *crit );
NTSTATUS WINAPI RtlLeaveCriticalSection( RTL_CRITICAL_SECTION *crit );

/* string functions */
extern LPSTR _strlwr( LPSTR str );
extern LPSTR _strupr( LPSTR str );

/*	misc */

#if defined(__i386__) && defined(__GNUC__)
static inline void WINAPI DbgBreakPoint(void) { __asm__ __volatile__("int3"); }
static inline void WINAPI DbgUserBreakPoint(void) { __asm__ __volatile__("int3"); }
#else  /* __i386__ && __GNUC__ */
void WINAPI DbgBreakPoint(void);
void WINAPI DbgUserBreakPoint(void);
#endif  /* __i386__ && __GNUC__ */
void WINAPIV DbgPrint(LPCSTR fmt, ...);

DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlIntegerToChar(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type);
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE hModule);

DWORD WINAPI RtlOpenCurrentUser(
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE KeyHandle);

BOOLEAN WINAPI RtlDosPathNameToNtPathName_U( LPWSTR from,PUNICODE_STRING us,DWORD x2,DWORD x3);
BOOL WINAPI RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

NTSTATUS WINAPI 
NtAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAccess,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PPRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus);

#ifdef __cplusplus
}
#endif

#endif
