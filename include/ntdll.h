#ifndef __WINE_NTDLL_H
#define __WINE_NTDLL_H
/* ntdll.h 
 *
 * contains NT internal defines that don't show on the Win32 API level
 *
 * Copyright 1997 Marcus Meissner
 */

#include "winbase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD NTSTATUS;

#define	SID_REVISION			(1)	/* Current revision */
#define	SID_MAX_SUB_AUTHORITIES		(15)	/* current max subauths */
#define	SID_RECOMMENDED_SUB_AUTHORITIES	(1)	/* recommended subauths */

/* ACLs of NT */

#define	ACL_REVISION	2

#define	ACL_REVISION1	1
#define	ACL_REVISION2	2

/* ACEs, directly starting after an ACL */
typedef struct _ACE_HEADER {
	BYTE	AceType;
	BYTE	AceFlags;
	WORD	AceSize;
} ACE_HEADER,*LPACE_HEADER;

/* AceType */
#define	ACCESS_ALLOWED_ACE_TYPE		0
#define	ACCESS_DENIED_ACE_TYPE		1
#define	SYSTEM_AUDIT_ACE_TYPE		2
#define	SYSTEM_ALARM_ACE_TYPE		3

/* inherit AceFlags */
#define	OBJECT_INHERIT_ACE		0x01
#define	CONTAINER_INHERIT_ACE		0x02
#define	NO_PROPAGATE_INHERIT_ACE	0x04
#define	INHERIT_ONLY_ACE		0x08
#define	VALID_INHERIT_FLAGS		0x0F

/* AceFlags mask for what events we (should) audit */
#define	SUCCESSFUL_ACCESS_ACE_FLAG	0x40
#define	FAILED_ACCESS_ACE_FLAG		0x80

/* different ACEs depending on AceType 
 * SidStart marks the begin of a SID
 * so the thing finally looks like this:
 * 0: ACE_HEADER
 * 4: ACCESS_MASK
 * 8... : SID
 */
typedef struct _ACCESS_ALLOWED_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} ACCESS_ALLOWED_ACE,*LPACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} ACCESS_DENIED_ACE,*LPACCESS_DENIED_ACE;

typedef struct _SYSTEM_AUDIT_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} SYSTEM_AUDIT_ACE,*LPSYSTEM_AUDIT_ACE;

typedef struct _SYSTEM_ALARM_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} SYSTEM_ALARM_ACE,*LPSYSTEM_ALARM_ACE;

typedef enum tagSID_NAME_USE {
	SidTypeUser = 1,
	SidTypeGroup,
	SidTypeDomain,
	SidTypeAlias,
	SidTypeWellKnownGroup,
	SidTypeDeletedAccount,
	SidTypeInvalid,
	SidTypeUnknown
} SID_NAME_USE,*PSID_NAME_USE;

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

VOID   WINAPI RtlInitializeResource(LPRTL_RWLOCK);
VOID   WINAPI RtlDeleteResource(LPRTL_RWLOCK);
BYTE   WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK, BYTE fWait);
BYTE   WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK, BYTE fWait);
VOID   WINAPI RtlReleaseResource(LPRTL_RWLOCK);
VOID   WINAPI RtlDumpResource(LPRTL_RWLOCK);

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
INT       WINAPI AccessResource(HMODULE,HRSRC);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_NTDLL_H */
