/*
	this file defines interfaces mainly exposed to device drivers and
	native nt dll's

*/
#ifndef __WINE_NTDDK_H
#define __WINE_NTDDK_H

#include <ntdef.h>

/* end fixme */

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
	timer
*/
typedef enum _TIMER_TYPE 
{	NotificationTimer,
	SynchronizationTimer
} TIMER_TYPE;

/*	##############################
	######	SID FUNCTIONS	######
	##############################
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

/*	##############################################
	######	SECURITY DESCRIPTOR FUNCTIONS	######
	##############################################
*/

NTSTATUS WINAPI RtlCreateSecurityDescriptor(
	PSECURITY_DESCRIPTOR lpsd,
	DWORD rev);

BOOLEAN WINAPI RtlValidSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor);

DWORD WINAPI RtlGetDaclSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT PBOOLEAN lpbDaclPresent,
	OUT PACL *pDacl,
	OUT PBOOLEAN lpbDaclDefaulted);

NTSTATUS WINAPI RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	BOOLEAN daclpresent,
	PACL dacl,
	BOOLEAN dacldefaulted );

ULONG WINAPI RtlLengthSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor);

DWORD  WINAPI RtlSetSaclSecurityDescriptor (
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
	LPACE_HEADER *x);
NTSTATUS WINAPI RtlAddAce(
	PACL acl,
	DWORD rev,
	DWORD xnrofaces,
	LPACE_HEADER acestart,
	DWORD acelen);
	
DWORD WINAPI RtlAddAccessAllowedAce(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce );

/*	######################################
	######	STRING FUNCTIONS	######
	######################################
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
UINT32 WINAPI RtlxOemStringToUnicodeSize(PSTRING str);
UINT32 WINAPI RtlxAnsiStringToUnicodeSize(PANSI_STRING str);
DWORD WINAPI RtlIsTextUnicode(LPVOID buf, DWORD len, DWORD *pf);
NTSTATUS WINAPI RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive);

/*	######################################
	######	RESOURCE FUNCTIONS	######
	######################################
*/
void WINAPI RtlInitializeResource(LPRTL_RWLOCK rwl);
void WINAPI RtlDeleteResource(LPRTL_RWLOCK rwl);
BYTE WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK rwl, BYTE fWait);
BYTE WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK rwl, BYTE fWait);
void WINAPI RtlReleaseResource(LPRTL_RWLOCK rwl);
void WINAPI RtlDumpResource(LPRTL_RWLOCK rwl);

void __cdecl DbgPrint(LPCSTR fmt,LPVOID args);
DWORD NtRaiseException ( DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments,CONST ULONG_PTR *lpArguments);
DWORD RtlRaiseException ( DWORD x);
VOID WINAPI RtlAcquirePebLock(void);
VOID WINAPI RtlReleasePebLock(void);
DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlIntegerToChar(DWORD x1,DWORD x2,DWORD x3,DWORD x4);
DWORD WINAPI RtlSystemTimeToLocalTime(DWORD x1,DWORD x2);
DWORD WINAPI RtlTimeToTimeFields(DWORD x1,DWORD x2);
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val);
DWORD WINAPI RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6);
DWORD WINAPI RtlDeleteSecurityObject(DWORD x1);
BOOLEAN WINAPI RtlTimeToSecondsSince1980(LPFILETIME ft,LPDWORD timeret);
BOOLEAN WINAPI RtlTimeToSecondsSince1970(LPFILETIME ft,LPDWORD timeret);
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x);
DWORD WINAPI RtlNtStatusToDosError(DWORD error);
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type);
DWORD WINAPI RtlTimeToElapsedTimeFields( DWORD x1, DWORD x2 );
INT32 WINAPI RtlExtendedLargeIntegerDivide(LARGE_INTEGER dividend, DWORD divisor, LPDWORD rest);
LARGE_INTEGER WINAPI RtlExtendedIntegerMultiply(LARGE_INTEGER factor1,INT32 factor2);
DWORD WINAPI RtlFormatCurrentUserKeyPath(DWORD x);
DWORD WINAPI RtlOpenCurrentUser(DWORD x1, DWORD *x2);
BOOLEAN WINAPI RtlDosPathNameToNtPathName_U( LPWSTR from,PUNICODE_STRING us,DWORD x2,DWORD x3);
DWORD WINAPI RtlCreateEnvironment(DWORD x1,DWORD x2);
DWORD WINAPI RtlDestroyEnvironment(DWORD x);
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) ;



#endif
