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
