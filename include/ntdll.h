#ifndef __WINE_NTDLL_H
#define __WINE_NTDLL_H
/* ntdll.h 
 *
 * contains NT internal defines that don't show on the Win32 API level
 *
 * Copyright 1997 Marcus Meissner
 */

/* assumes windows.h already included */

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD NTSTATUS;

/* Security Ids of NT */

/* Moved to windows.h
typedef struct {
	BYTE	Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;
*/

/* Moved to windows.h
typedef struct _SID {
	BYTE	Revision;
	BYTE	SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY	IdentifierAuthority;
	DWORD	SubAuthority[1];
} SID,*PSID,*LPSID;
*/

#define	SID_REVISION			(1)	/* Current revision */
#define	SID_MAX_SUB_AUTHORITIES		(15)	/* current max subauths */
#define	SID_RECOMMENDED_SUB_AUTHORITIES	(1)	/* recommended subauths */

/* ACLs of NT */

#define	ACL_REVISION	2

#define	ACL_REVISION1	1
#define	ACL_REVISION2	2

/* Moved to windows.h
typedef struct _ACL {
	BYTE	AclRevision;
	BYTE	Sbz1;
	WORD	AclSize;
	WORD	AceCount;
	WORD	Sbz2;
} ACL,*LPACL;
*/

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

#define	SECURITY_DESCRIPTOR_REVISION	1
#define	SECURITY_DESCRIPTOR_REVISION1	1

/*
typedef WORD SECURITY_DESCRIPTOR_CONTROL;
 */

#define	SE_OWNER_DEFAULTED	0x0001
#define	SE_GROUP_DEFAULTED	0x0002
#define	SE_DACL_PRESENT		0x0004
#define	SE_DACL_DEFAULTED	0x0008
#define	SE_SACL_PRESENT		0x0010
#define	SE_SACL_DEFAULTED	0x0020
#define	SE_SELF_RELATIVE	0x8000

/* This was moved to windows.h
typedef struct {
	BYTE	Revision;
	BYTE	Sbz1;
	SECURITY_DESCRIPTOR_CONTROL Control;
	LPSID	Owner;
	LPSID	Group;
	LPACL	Sacl;
	LPACL	Dacl;
} SECURITY_DESCRIPTOR,*PSECURITY_DESCRIPTOR,*LPSECURITY_DESCRIPTOR;
*/

typedef enum tagSID_NAME_USE {
	SidTypeUser = 1,
	SidTypeGroup,
	SidTypeDomain,
	SidTypeAlias,
	SidTypeWellKnownGroup,
	SidTypeDeletedAccount,
	SidTypeInvalid,
	SidTypeUnknown
} SID_NAME_USE,*PSID_NAME_USE,*LPSID_NAME_USE;

/* NT lowlevel Strings (handled by Rtl* functions in NTDLL)
 * If they are zero terminated, Length does not include the terminating 0.
 */

typedef struct _STRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPSTR	Buffer;
} STRING,*LPSTRING,ANSI_STRING,*LPANSI_STRING;

typedef struct _CSTRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPCSTR	Buffer;
} CSTRING,*LPCSTRING;

typedef struct _UNICODE_STRING {
	UINT16	Length;		/* bytes */
	UINT16	MaximumLength;	/* bytes */
	LPWSTR	Buffer;
} UNICODE_STRING,*LPUNICODE_STRING;

typedef struct _RTL_RWLOCK {
	CRITICAL_SECTION	rtlCS;
	HANDLE32		hSharedReleaseSemaphore;
	UINT32			uSharedWaiters;
	HANDLE32		hExclusiveReleaseSemaphore;
	UINT32			uExclusiveWaiters;
	INT32			iNumberActive;
	HANDLE32		hOwningThreadId;
	DWORD			dwTimeoutBoost;
	PVOID			pDebugInfo;
} RTL_RWLOCK, *LPRTL_RWLOCK;

VOID   WINAPI RtlInitializeResource(LPRTL_RWLOCK);
VOID   WINAPI RtlDeleteResource(LPRTL_RWLOCK);
BYTE   WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK, BYTE fWait);
BYTE   WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK, BYTE fWait);
VOID   WINAPI RtlReleaseResource(LPRTL_RWLOCK);
VOID   WINAPI RtlDumpResource(LPRTL_RWLOCK);

BOOL32 WINAPI IsValidSid(LPSID);
BOOL32 WINAPI EqualSid(LPSID,LPSID);
BOOL32 WINAPI EqualPrefixSid(LPSID,LPSID);
DWORD  WINAPI GetSidLengthRequired(BYTE);
BOOL32 WINAPI AllocateAndInitializeSid(LPSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,
                                       DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,
                                       DWORD,LPSID*);
VOID*  WINAPI FreeSid(LPSID);
BOOL32 WINAPI InitializeSecurityDescriptor(SECURITY_DESCRIPTOR*,DWORD);
BOOL32 WINAPI InitializeSid(LPSID,LPSID_IDENTIFIER_AUTHORITY,BYTE);
DWORD* WINAPI GetSidSubAuthority(LPSID,DWORD);
BYTE * WINAPI GetSidSubAuthorityCount(LPSID);
DWORD  WINAPI GetLengthSid(LPSID);
BOOL32 WINAPI CopySid(DWORD,LPSID,LPSID);
BOOL32 WINAPI LookupAccountSid32A(LPCSTR,PSID,LPCSTR,LPDWORD,LPCSTR,LPDWORD,
                                  PSID_NAME_USE);
BOOL32 WINAPI LookupAccountSid32W(LPCWSTR,PSID,LPCWSTR,LPDWORD,LPCWSTR,LPDWORD,
                                  PSID_NAME_USE);
LPSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority(LPSID);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_NTDLL_H */
