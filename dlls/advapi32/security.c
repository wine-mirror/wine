/*
 * dlls/advapi32/security.c
 */
#include "windows.h"
#include "winerror.h"
#include "debug.h"
#include "heap.h"
#include "ntdll.h"

/* FIXME: Should these be in a header file? */
BOOL32 WINAPI IsValidSid (LPSID pSid);
BOOL32 WINAPI EqualSid (LPSID pSid1, LPSID pSid2);
BOOL32 WINAPI EqualPrefixSid (LPSID pSid1, LPSID pSid2);
DWORD  WINAPI GetSidLengthRequired (BYTE nSubAuthorityCount);
BOOL32 WINAPI AllocateAndInitializeSid(LPSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD nSubAuthority0, DWORD nSubAuthority1, DWORD nSubAuthority2, DWORD nSubAuthority3,    DWORD nSubAuthority4, DWORD nSubAuthority5, DWORD nSubAuthority6, DWORD nSubAuthority7, LPSID *pSid);
VOID*  WINAPI FreeSid(LPSID pSid);
BOOL32 WINAPI InitializeSid (LPSID pSid, LPSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount);
LPSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority(LPSID pSid);
DWORD* WINAPI GetSidSubAuthority(LPSID pSid, DWORD nSubAuthority);
BYTE*  WINAPI GetSidSubAuthorityCount(LPSID pSid);
DWORD  WINAPI GetLengthSid(LPSID pSid);
BOOL32 WINAPI CopySid(DWORD nDestinationSidLength, LPSID pDestinationSid, LPSID pSourceSid);


/******************************************************************************
 * OpenProcessToken [ADVAPI32.109]
 * Opens the access token associated with a process
 *
 * PARAMS
 *   ProcessHandle [I] Handle to process
 *   DesiredAccess [I] Desired access to process
 *   TokenHandle   [O] Pointer to handle of open access token
 *
 * RETURNS STD
 */
BOOL32 WINAPI
OpenProcessToken( HANDLE32 ProcessHandle, DWORD DesiredAccess, 
                  HANDLE32 *TokenHandle )
{
    FIXME(advapi,"(%08x,%08lx,%p): stub\n",ProcessHandle,DesiredAccess,
          TokenHandle);
    return TRUE;
}

/******************************************************************************
 * OpenThreadToken [ADVAPI32.114]
 *
 * PARAMS
 *   thread        []
 *   desiredaccess []
 *   openasself    []
 *   thandle       []
 */
BOOL32 WINAPI
OpenThreadToken( HANDLE32 thread, DWORD desiredaccess, BOOL32 openasself,
                 HANDLE32 *thandle )
{
	FIXME(advapi,"(%08x,%08lx,%d,%p): stub!\n",
	      thread,desiredaccess,openasself,thandle);
	*thandle = 0; /* FIXME ... well, store something in there ;) */
	return TRUE;
}


/******************************************************************************
 * LookupPrivilegeValue32A [ADVAPI32.92]
 */
BOOL32 WINAPI
LookupPrivilegeValue32A( LPCSTR lpSystemName, LPCSTR lpName, LPVOID lpLuid )
{
    LPWSTR lpSystemNameW = HEAP_strdupAtoW(GetProcessHeap(), 0, lpSystemName);
    LPWSTR lpNameW = HEAP_strdupAtoW(GetProcessHeap(), 0, lpName);
    BOOL32 ret = LookupPrivilegeValue32W( lpSystemNameW, lpNameW, lpLuid);
    HeapFree(GetProcessHeap(), 0, lpNameW);
    HeapFree(GetProcessHeap(), 0, lpSystemNameW);
    return ret;
}

/******************************************************************************
 * LookupPrivilegeValue32W [ADVAPI32.93]
 * Retrieves LUID used on a system to represent the privilege name.
 *
 * NOTES
 *   lpLuid should be PLUID
 *
 * PARAMS
 *   lpSystemName [I] Address of string specifying the system
 *   lpName       [I] Address of string specifying the privilege
 *   lpLuid       [I] Address of locally unique identifier
 *
 * RETURNS STD
 */
BOOL32 WINAPI
LookupPrivilegeValue32W( LPCWSTR lpSystemName, LPCWSTR lpName, LPVOID lpLuid )
{
    FIXME(advapi,"(%s,%s,%p): stub\n",debugstr_w(lpSystemName), 
                  debugstr_w(lpName), lpLuid);
    return TRUE;
}
/******************************************************************************
 * GetFileSecurity32A [ADVAPI32.45]
 *
 * Obtains Specified information about the security of a file or directory
 * The information obtained is constrained by the callers acces rights and
 * priviliges
 */
BOOL32 WINAPI
GetFileSecurity32A( LPCSTR lpFileName, 
                    SECURITY_INFORMATION RequestedInformation,
                    LPSECURITY_DESCRIPTOR pSecurityDescriptor,
                    DWORD nLength, LPDWORD lpnLengthNeeded )
{
  FIXME(advapi, "(%s) : stub\n", debugstr_a(lpFileName));
  return TRUE;
}

/******************************************************************************
 * GetFileSecurity32W [ADVAPI32.46]
 *
 * Obtains Specified information about the security of a file or directory
 * The information obtained is constrained by the callers acces rights and
 * priviliges
 *
 * PARAMS
 *   lpFileName           []
 *   RequestedInformation []
 *   pSecurityDescriptor  []
 *   nLength              []
 *   lpnLengthNeeded      []
 */
BOOL32 WINAPI
GetFileSecurity32W( LPCWSTR lpFileName, 
                    SECURITY_INFORMATION RequestedInformation,
                    LPSECURITY_DESCRIPTOR pSecurityDescriptor,
                    DWORD nLength, LPDWORD lpnLengthNeeded )
{
  FIXME(advapi, "(%s) : stub\n", debugstr_w(lpFileName) ); 
  return TRUE;
}


/******************************************************************************
 * AdjustTokenPrivileges [ADVAPI32.10]
 *
 * PARAMS
 *   TokenHandle          []
 *   DisableAllPrivileges []
 *   NewState             []
 *   BufferLength         []
 *   PreviousState        []
 *   ReturnLength         []
 */
BOOL32 WINAPI
AdjustTokenPrivileges( HANDLE32 TokenHandle, BOOL32 DisableAllPrivileges,
                       LPVOID NewState, DWORD BufferLength, 
                       LPVOID PreviousState, LPDWORD ReturnLength )
{
	return TRUE;
}

/******************************************************************************
 * CopySid [ADVAPI32.24]
 *
 * PARAMS
 *   nDestinationSidLength []
 *   pDestinationSid       []
 *   pSourceSid            []
 */
BOOL32 WINAPI
CopySid( DWORD nDestinationSidLength, LPSID pDestinationSid, LPSID pSourceSid )
{

    if (!IsValidSid(pSourceSid))
        return FALSE;

    if (nDestinationSidLength < GetLengthSid(pSourceSid))
        return FALSE;

    memcpy(pDestinationSid, pSourceSid, GetLengthSid(pSourceSid));

    return TRUE;
}

/******************************************************************************
 * IsValidSid [ADVAPI32.80]
 *
 * PARAMS
 *   pSid []
 */
BOOL32 WINAPI
IsValidSid( LPSID pSid )
{
    if (!pSid || pSid->Revision != SID_REVISION)
        return FALSE;

    return TRUE;
}

/******************************************************************************
 * EqualSid [ADVAPI32.40]
 *
 * PARAMS
 *   pSid1 []
 *   pSid2 []
 */
BOOL32 WINAPI
EqualSid( LPSID pSid1, LPSID pSid2 )
{
    if (!IsValidSid(pSid1) || !IsValidSid(pSid2))
        return FALSE;

    if (*GetSidSubAuthorityCount(pSid1) != *GetSidSubAuthorityCount(pSid2))
        return FALSE;

    if (memcmp(pSid1, pSid2, GetLengthSid(pSid1)) != 0)
        return FALSE;

    return TRUE;
}

/******************************************************************************
 * EqualPrefixSid [ADVAPI32.39]
 */
BOOL32 WINAPI EqualPrefixSid (LPSID pSid1, LPSID pSid2) {
    if (!IsValidSid(pSid1) || !IsValidSid(pSid2))
        return FALSE;

    if (*GetSidSubAuthorityCount(pSid1) != *GetSidSubAuthorityCount(pSid2))
        return FALSE;

    if (memcmp(pSid1, pSid2, GetSidLengthRequired(pSid1->SubAuthorityCount - 1))
 != 0)
        return FALSE;

    return TRUE;
}

/******************************************************************************
 * GetSidLengthRequired [ADVAPI32.63]
 *
 * PARAMS
 *   nSubAuthorityCount []
 */
DWORD WINAPI
GetSidLengthRequired( BYTE nSubAuthorityCount )
{
    return sizeof (SID) + (nSubAuthorityCount - 1) * sizeof (DWORD);
}

/******************************************************************************
 * GetTokenInformation [ADVAPI32.66]
 *
 * PARAMS
 *   token           []
 *   tokeninfoclass  []
 *   tokeninfo       []
 *   tokeninfolength []
 *   retlen          []
 *
 * FIXME
 *   tokeninfoclas should be TOKEN_INFORMATION_CLASS
 */
BOOL32 WINAPI
GetTokenInformation( HANDLE32 token, DWORD tokeninfoclass, LPVOID tokeninfo,
                     DWORD tokeninfolength, LPDWORD retlen )
{
	FIXME(advapi,"(%08x,%ld,%p,%ld,%p): stub\n",
	      token,tokeninfoclass,tokeninfo,tokeninfolength,retlen);
	return TRUE;
}

/******************************************************************************
 * AllocateAndInitializeSid [ADVAPI32.11]
 *
 * PARAMS
 *   pIdentifierAuthority []
 *   nSubAuthorityCount   []
 *   nSubAuthority0       []
 *   nSubAuthority1       []
 *   nSubAuthority2       []
 *   nSubAuthority3       []
 *   nSubAuthority4       []
 *   nSubAuthority5       []
 *   nSubAuthority6       []
 *   nSubAuthority7       []
 *   pSid                 []
 */
BOOL32 WINAPI
AllocateAndInitializeSid( LPSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                          BYTE nSubAuthorityCount,
                          DWORD nSubAuthority0, DWORD nSubAuthority1,
                          DWORD nSubAuthority2, DWORD nSubAuthority3,
                          DWORD nSubAuthority4, DWORD nSubAuthority5,
                          DWORD nSubAuthority6, DWORD nSubAuthority7,
                          LPSID *pSid )
{
    if (!(*pSid = HeapAlloc( GetProcessHeap(), 0,
                             GetSidLengthRequired(nSubAuthorityCount))))
        return FALSE;
    (*pSid)->Revision = SID_REVISION;
    if (pIdentifierAuthority)
        memcpy(&(*pSid)->IdentifierAuthority, pIdentifierAuthority,
               sizeof (SID_IDENTIFIER_AUTHORITY));
    *GetSidSubAuthorityCount(*pSid) = nSubAuthorityCount;

    if (nSubAuthorityCount > 0)
        *GetSidSubAuthority(*pSid, 0) = nSubAuthority0;
    if (nSubAuthorityCount > 1)
        *GetSidSubAuthority(*pSid, 1) = nSubAuthority1;
    if (nSubAuthorityCount > 2)
        *GetSidSubAuthority(*pSid, 2) = nSubAuthority2;
    if (nSubAuthorityCount > 3)
        *GetSidSubAuthority(*pSid, 3) = nSubAuthority3;
    if (nSubAuthorityCount > 4)
        *GetSidSubAuthority(*pSid, 4) = nSubAuthority4;
    if (nSubAuthorityCount > 5)
        *GetSidSubAuthority(*pSid, 5) = nSubAuthority5;
    if (nSubAuthorityCount > 6)
        *GetSidSubAuthority(*pSid, 6) = nSubAuthority6;
    if (nSubAuthorityCount > 7)
        *GetSidSubAuthority(*pSid, 7) = nSubAuthority7;

    return TRUE;
}

/***********************************************************************
 *           FreeSid  (ADVAPI.42)
 */
VOID* WINAPI FreeSid(LPSID pSid)
{
    HeapFree( GetProcessHeap(), 0, pSid );
    return NULL;
}

/***********************************************************************
 *           InitializeSecurityDescriptor  (ADVAPI.73)
 */
BOOL32 WINAPI InitializeSecurityDescriptor( SECURITY_DESCRIPTOR *pDescr,
                                            DWORD revision )
{
    FIXME(security, "(%p,%#lx): stub\n", pDescr, revision);
    return TRUE;
}

/***********************************************************************
 *           GetSecurityDescriptorLength  (ADVAPI.55)
 */
DWORD WINAPI GetSecurityDescriptorLength( SECURITY_DESCRIPTOR *pDescr)
{
    FIXME(security, "(%p), stub\n", pDescr);
    return 0;
}

/***********************************************************************
 *           GetSecurityDescriptorOwner  (ADVAPI.56)
 */
BOOL32 WINAPI GetSecurityDescriptorOwner (SECURITY_DESCRIPTOR *pDescr,LPSID *pOwner,LPBOOL32 lpbOwnerDefaulted)
{
    FIXME(security, "(%p,%p,%p), stub\n", pDescr,pOwner,lpbOwnerDefaulted);
    *lpbOwnerDefaulted = TRUE;
    return 0;
}

/***********************************************************************
 *           GetSecurityDescriptorGroup  (ADVAPI.54)
 */
BOOL32 WINAPI GetSecurityDescriptorGroup(SECURITY_DESCRIPTOR *pDescr,LPSID *pGroup,LPBOOL32 lpbOwnerDefaulted)
{
    FIXME(security, "(%p,%p,%p), stub\n", pDescr,pGroup,lpbOwnerDefaulted);
    *lpbOwnerDefaulted = TRUE;
    return 0;
}

/***********************************************************************
 *           InitializeSid  (ADVAPI.74)
 */
BOOL32 WINAPI InitializeSid (LPSID pSid, LPSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                    BYTE nSubAuthorityCount)
{
    int i;

    pSid->Revision = SID_REVISION;
    if (pIdentifierAuthority)
        memcpy(&pSid->IdentifierAuthority, pIdentifierAuthority,
               sizeof (SID_IDENTIFIER_AUTHORITY));
    *GetSidSubAuthorityCount(pSid) = nSubAuthorityCount;

    for (i = 0; i < nSubAuthorityCount; i++)
        *GetSidSubAuthority(pSid, i) = 0;

    return TRUE;
}

/******************************************************************************
 * GetSidIdentifierAuthority [ADVAPI32.62]
 *
 * PARAMS
 *   pSid []
 */
LPSID_IDENTIFIER_AUTHORITY WINAPI
GetSidIdentifierAuthority( LPSID pSid )
{
    return &pSid->IdentifierAuthority;
}

/******************************************************************************
 * GetSidSubAuthority [ADVAPI32.64]
 *
 * PARAMS
 *   pSid          []
 *   nSubAuthority []
 */
DWORD * WINAPI
GetSidSubAuthority( LPSID pSid, DWORD nSubAuthority )
{
    return &pSid->SubAuthority[nSubAuthority];
}

/******************************************************************************
 * GetSidSubAuthorityCount [ADVAPI32.65]
 *
 * PARAMS
 *   pSid []
 */
BYTE * WINAPI
GetSidSubAuthorityCount (LPSID pSid)
{
    return &pSid->SubAuthorityCount;
}

/******************************************************************************
 * GetLengthSid [ADVAPI32.48]
 *
 * PARAMS
 *   pSid []
 */
DWORD WINAPI
GetLengthSid (LPSID pSid)
{
    return GetSidLengthRequired(*GetSidSubAuthorityCount(pSid));
}

/******************************************************************************
 * IsValidSecurityDescriptor [ADVAPI32.79]
 *
 * PARAMS
 *   lpsecdesc []
 */
BOOL32 WINAPI
IsValidSecurityDescriptor( LPSECURITY_DESCRIPTOR lpsecdesc )
{
	FIXME(advapi,"(%p):stub\n",lpsecdesc);
	return TRUE;
}

/******************************************************************************
 * LookupAccountSid32A [ADVAPI32.86]
 */
BOOL32 WINAPI
LookupAccountSid32A( LPCSTR system, PSID sid, LPCSTR account,
                     LPDWORD accountSize, LPCSTR domain, LPDWORD domainSize,
                     PSID_NAME_USE name_use )
{
	FIXME(security,"(%s,%p,%p,%p,%p,%p,%p): stub\n",
	      system,sid,account,accountSize,domain,domainSize,name_use);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/******************************************************************************
 * LookupAccountSid32W [ADVAPI32.87]
 *
 * PARAMS
 *   system      []
 *   sid         []
 *   account     []
 *   accountSize []
 *   domain      []
 *   domainSize  []
 *   name_use    []
 */
BOOL32 WINAPI
LookupAccountSid32W( LPCWSTR system, PSID sid, LPCWSTR account, 
                     LPDWORD accountSize, LPCWSTR domain, LPDWORD domainSize,
                     PSID_NAME_USE name_use )
{
	FIXME(security,"(%p,%p,%p,%p,%p,%p,%p): stub\n",
	      system,sid,account,accountSize,domain,domainSize,name_use);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/******************************************************************************
 * SetFileSecurity32A [ADVAPI32.182]
 * Sets the security of a file or directory
 */
BOOL32 WINAPI SetFileSecurity32A( LPCSTR lpFileName,
                                SECURITY_INFORMATION RequestedInformation,
                                LPSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  FIXME(advapi, "(%s) : stub\n", debugstr_a(lpFileName));
  return TRUE;
}

/******************************************************************************
 * SetFileSecurity32W [ADVAPI32.183]
 * Sets the security of a file or directory
 *
 * PARAMS
 *   lpFileName           []
 *   RequestedInformation []
 *   pSecurityDescriptor  []
 */
BOOL32 WINAPI
SetFileSecurity32W( LPCWSTR lpFileName, 
                    SECURITY_INFORMATION RequestedInformation,
                    LPSECURITY_DESCRIPTOR pSecurityDescriptor )
{
  FIXME(advapi, "(%s) : stub\n", debugstr_w(lpFileName) ); 
  return TRUE;
}

/******************************************************************************
 * MakeSelfRelativeSD [ADVAPI32.95]
 *
 * PARAMS
 *   lpabssecdesc  []
 *   lpselfsecdesc []
 *   lpbuflen      []
 */
BOOL32 WINAPI
MakeSelfRelativeSD( LPSECURITY_DESCRIPTOR lpabssecdesc,
                    LPSECURITY_DESCRIPTOR lpselfsecdesc, LPDWORD lpbuflen )
{
	FIXME(advapi,"(%p,%p,%p),stub!\n",lpabssecdesc,lpselfsecdesc,lpbuflen);
	return TRUE;
}

/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.266]
 *
 * PARAMS
 *   x1 []
 */
BOOL32 WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	FIXME(advapi,"(%ld):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.265]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL32 WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	FIXME(advapi,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub\n",x1,x2,x3,x4);
	return TRUE;
}

/******************************************************************************
 * LsaOpenPolicy [ADVAPI32.200]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL32 WINAPI
LsaOpenPolicy( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
	FIXME(advapi,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub\n",x1,x2,x3,x4);
	return 0xc0000000; /* generic error */
}

/******************************************************************************
 * NotifyBootConfigStatus [ADVAPI32.97]
 *
 * PARAMS
 *   x1 []
 */
BOOL32 WINAPI
NotifyBootConfigStatus( DWORD x1 )
{
	FIXME(advapi,"(0x%08lx):stub\n",x1);
	return 1;
}

/******************************************************************************
 * RevertToSelf			[ADVAPI32]
 */
BOOL32 WINAPI RevertToSelf(void) {
	FIXME(advapi,"(), stub\n");
	return TRUE;
}
