/*
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winsafer.h"
#include "winternl.h"
#include "winioctl.h"
#include "accctrl.h"
#include "sddl.h"
#include "winsvc.h"
#include "aclapi.h"
#include "objbase.h"
#include "iads.h"
#include "advapi32_misc.h"
#include "lmcons.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static DWORD trustee_to_sid(DWORD nDestinationSidLength, PSID pDestinationSid, PTRUSTEEW pTrustee);

typedef struct _MAX_SID
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
} MAX_SID;

typedef struct _AccountSid {
    WELL_KNOWN_SID_TYPE type;
    LPCWSTR account;
    LPCWSTR domain;
    SID_NAME_USE name_use;
    LPCWSTR alias;
} AccountSid;

static const AccountSid ACCOUNT_SIDS[] = {
    { WinNullSid, L"NULL SID", L"", SidTypeWellKnownGroup },
    { WinWorldSid, L"Everyone", L"", SidTypeWellKnownGroup },
    { WinLocalSid, L"LOCAL", L"", SidTypeWellKnownGroup },
    { WinCreatorOwnerSid, L"CREATOR OWNER", L"", SidTypeWellKnownGroup },
    { WinCreatorGroupSid, L"CREATOR GROUP", L"", SidTypeWellKnownGroup },
    { WinCreatorOwnerServerSid, L"CREATOR OWNER SERVER", L"", SidTypeWellKnownGroup },
    { WinCreatorGroupServerSid, L"CREATOR GROUP SERVER", L"", SidTypeWellKnownGroup },
    { WinNtAuthoritySid, L"NT Pseudo Domain", L"NT Pseudo Domain", SidTypeDomain },
    { WinDialupSid, L"DIALUP", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinNetworkSid, L"NETWORK", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinBatchSid, L"BATCH", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinInteractiveSid, L"INTERACTIVE", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinServiceSid, L"SERVICE", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinAnonymousSid, L"ANONYMOUS LOGON", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinProxySid, L"PROXY", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinEnterpriseControllersSid, L"ENTERPRISE DOMAIN CONTROLLERS", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinSelfSid, L"SELF", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinAuthenticatedUserSid, L"Authenticated Users", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinRestrictedCodeSid, L"RESTRICTED", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinTerminalServerSid, L"TERMINAL SERVER USER", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinRemoteLogonIdSid, L"REMOTE INTERACTIVE LOGON", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinLocalSystemSid, L"SYSTEM", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinLocalServiceSid, L"LOCAL SERVICE", L"NT AUTHORITY", SidTypeWellKnownGroup, L"LOCALSERVICE" },
    { WinNetworkServiceSid, L"NETWORK SERVICE", L"NT AUTHORITY", SidTypeWellKnownGroup , L"NETWORKSERVICE"},
    { WinBuiltinDomainSid, L"BUILTIN", L"BUILTIN", SidTypeDomain },
    { WinBuiltinAdministratorsSid, L"Administrators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinUsersSid, L"Users", L"BUILTIN", SidTypeAlias },
    { WinBuiltinGuestsSid, L"Guests", L"BUILTIN", SidTypeAlias },
    { WinBuiltinPowerUsersSid, L"Power Users", L"BUILTIN", SidTypeAlias },
    { WinBuiltinAccountOperatorsSid, L"Account Operators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinSystemOperatorsSid, L"Server Operators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinPrintOperatorsSid, L"Print Operators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinBackupOperatorsSid, L"Backup Operators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinReplicatorSid, L"Replicators", L"BUILTIN", SidTypeAlias },
    { WinBuiltinPreWindows2000CompatibleAccessSid, L"Pre-Windows 2000 Compatible Access", L"BUILTIN", SidTypeAlias },
    { WinBuiltinRemoteDesktopUsersSid, L"Remote Desktop Users", L"BUILTIN", SidTypeAlias },
    { WinBuiltinNetworkConfigurationOperatorsSid, L"Network Configuration Operators", L"BUILTIN", SidTypeAlias },
    { WinNTLMAuthenticationSid, L"NTLM Authentication", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinDigestAuthenticationSid, L"Digest Authentication", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinSChannelAuthenticationSid, L"SChannel Authentication", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinThisOrganizationSid, L"This Organization", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinOtherOrganizationSid, L"Other Organization", L"NT AUTHORITY", SidTypeWellKnownGroup },
    { WinBuiltinPerfMonitoringUsersSid, L"Performance Monitor Users", L"BUILTIN", SidTypeAlias },
    { WinBuiltinPerfLoggingUsersSid, L"Performance Log Users", L"BUILTIN", SidTypeAlias },
    { WinBuiltinAnyPackageSid, L"ALL APPLICATION PACKAGES", L"APPLICATION PACKAGE AUTHORITY", SidTypeWellKnownGroup },
};

const char * debugstr_sid(PSID sid)
{
    int auth = 0;
    SID * psid = sid;

    if (psid == NULL)
        return "(null)";

    auth = psid->IdentifierAuthority.Value[5] +
           (psid->IdentifierAuthority.Value[4] << 8) +
           (psid->IdentifierAuthority.Value[3] << 16) +
           (psid->IdentifierAuthority.Value[2] << 24);

    switch (psid->SubAuthorityCount) {
    case 0:
        return wine_dbg_sprintf("S-%d-%d", psid->Revision, auth);
    case 1:
        return wine_dbg_sprintf("S-%d-%d-%lu", psid->Revision, auth,
            psid->SubAuthority[0]);
    case 2:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1]);
    case 3:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2]);
    case 4:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3]);
    case 5:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4]);
    case 6:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[3], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[0], psid->SubAuthority[4], psid->SubAuthority[5]);
    case 7:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6]);
    case 8:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6], psid->SubAuthority[7]);
    }
    return "(too-big)";
}

/* helper function for SE_FILE_OBJECT objects in [Get|Set]NamedSecurityInfo */
static inline DWORD get_security_file( LPCWSTR full_file_name, DWORD access, HANDLE *file )
{
    UNICODE_STRING file_nameW;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (!RtlDosPathNameToNtPathName_U( full_file_name, &file_nameW, NULL, NULL ))
        return ERROR_PATH_NOT_FOUND;
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &file_nameW;
    attr.SecurityDescriptor = NULL;
    status = NtCreateFile( file, access|SYNCHRONIZE, &attr, &io, NULL, FILE_FLAG_BACKUP_SEMANTICS,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN,
                           FILE_OPEN_FOR_BACKUP_INTENT, NULL, 0 );
    RtlFreeUnicodeString( &file_nameW );
    return RtlNtStatusToDosError( status );
}

/* helper function for SE_SERVICE objects in [Get|Set]NamedSecurityInfo */
static DWORD get_security_service( const WCHAR *full_service_name, DWORD access, HANDLE *service )
{
    SC_HANDLE manager = OpenSCManagerW( NULL, NULL, access );
    if (manager)
    {
        *service = OpenServiceW( manager, full_service_name, access);
        CloseServiceHandle( manager );
        if (*service)
            return ERROR_SUCCESS;
    }
    return GetLastError();
}

/* helper function for SE_REGISTRY_KEY objects in [Get|Set]NamedSecurityInfo */
static DWORD get_security_regkey( const WCHAR *full_key_name, DWORD access, HANDLE *key )
{
    const WCHAR *p = wcschr(full_key_name, '\\');
    int len = p-full_key_name;
    HKEY hParent;

    if (!p) return ERROR_INVALID_PARAMETER;
    if (!wcsncmp( full_key_name, L"CLASSES_ROOT", len ))
        hParent = HKEY_CLASSES_ROOT;
    else if (!wcsncmp( full_key_name, L"CURRENT_USER", len ))
        hParent = HKEY_CURRENT_USER;
    else if (!wcsncmp( full_key_name, L"MACHINE", len ))
        hParent = HKEY_LOCAL_MACHINE;
    else if (!wcsncmp( full_key_name, L"USERS", len ))
        hParent = HKEY_USERS;
    else
        return ERROR_INVALID_PARAMETER;
    return RegOpenKeyExW( hParent, p+1, 0, access, (HKEY *)key );
}


/************************************************************
 *                ADVAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Result;
    LPWSTR buf;

    if (!ServerName || !ServerName[0])
        return TRUE;

    buf = malloc(dwSize * sizeof(WCHAR));
    Result = GetComputerNameW(buf,  &dwSize);
    if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
        ServerName += 2;
    Result = Result && !wcscmp(ServerName, buf);
    free(buf);

    return Result;
}

/************************************************************
 *                ADVAPI_GetComputerSid
 */
BOOL ADVAPI_GetComputerSid(PSID sid)
{
    static const struct /* same fields as struct SID */
    {
        BYTE Revision;
        BYTE SubAuthorityCount;
        SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
        DWORD SubAuthority[4];
    } computer_sid =
    { SID_REVISION, 4, { SECURITY_NT_AUTHORITY }, { SECURITY_NT_NON_UNIQUE, 0, 0, 0 } };

    memcpy( sid, &computer_sid, sizeof(computer_sid) );
    return TRUE;
}

DWORD WINAPI
GetEffectiveRightsFromAclA( PACL pacl, PTRUSTEEA pTrustee, PACCESS_MASK pAccessRights )
{
    FIXME("%p %p %p - stub\n", pacl, pTrustee, pAccessRights);

    *pAccessRights = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
    return 0;
}

DWORD WINAPI
GetEffectiveRightsFromAclW( PACL pacl, PTRUSTEEW pTrustee, PACCESS_MASK pAccessRights )
{
    FIXME("%p %p %p - stub\n", pacl, pTrustee, pAccessRights);

    return 1;
}

/*	##############################################
	######	SECURITY DESCRIPTOR FUNCTIONS	######
	##############################################
*/

 /****************************************************************************** 
 * BuildSecurityDescriptorA [ADVAPI32.@]
 *
 * Builds a SD from 
 *
 * PARAMS
 *  pOwner                [I]
 *  pGroup                [I]
 *  cCountOfAccessEntries [I]
 *  pListOfAccessEntries  [I]
 *  cCountOfAuditEntries  [I]
 *  pListofAuditEntries   [I]
 *  pOldSD                [I]
 *  lpdwBufferLength      [I/O]
 *  pNewSD                [O]
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
DWORD WINAPI BuildSecurityDescriptorA(
    IN PTRUSTEEA pOwner,
    IN PTRUSTEEA pGroup,
    IN ULONG cCountOfAccessEntries,
    IN PEXPLICIT_ACCESSA pListOfAccessEntries,
    IN ULONG cCountOfAuditEntries,
    IN PEXPLICIT_ACCESSA pListofAuditEntries,
    IN PSECURITY_DESCRIPTOR pOldSD,
    IN OUT PULONG lpdwBufferLength,
    OUT PSECURITY_DESCRIPTOR* pNewSD)
{ 
    FIXME("(%p,%p,%ld,%p,%ld,%p,%p,%p,%p) stub!\n",pOwner,pGroup,
          cCountOfAccessEntries,pListOfAccessEntries,cCountOfAuditEntries,
          pListofAuditEntries,pOldSD,lpdwBufferLength,pNewSD);
 
    return ERROR_CALL_NOT_IMPLEMENTED;
} 
 
/******************************************************************************
 * BuildSecurityDescriptorW [ADVAPI32.@]
 *
 * See BuildSecurityDescriptorA.
 */
DWORD WINAPI BuildSecurityDescriptorW(
    IN PTRUSTEEW pOwner,
    IN PTRUSTEEW pGroup,
    IN ULONG cCountOfAccessEntries,
    IN PEXPLICIT_ACCESSW pListOfAccessEntries,
    IN ULONG cCountOfAuditEntries,
    IN PEXPLICIT_ACCESSW pListOfAuditEntries,
    IN PSECURITY_DESCRIPTOR pOldSD,
    IN OUT PULONG lpdwBufferLength,
    OUT PSECURITY_DESCRIPTOR* pNewSD)
{ 
    SECURITY_DESCRIPTOR desc;
    NTSTATUS status;
    DWORD ret = ERROR_SUCCESS;

    TRACE("(%p,%p,%ld,%p,%ld,%p,%p,%p,%p)\n", pOwner, pGroup,
          cCountOfAccessEntries, pListOfAccessEntries, cCountOfAuditEntries,
          pListOfAuditEntries, pOldSD, lpdwBufferLength, pNewSD);
 
    if (pOldSD)
    {
        SECURITY_DESCRIPTOR_CONTROL control;
        DWORD desc_size, dacl_size = 0, sacl_size = 0, owner_size = 0, group_size = 0;
        PACL dacl = NULL, sacl = NULL;
        PSID owner = NULL, group = NULL;
        DWORD revision;

        if ((status = RtlGetControlSecurityDescriptor( pOldSD, &control, &revision )) != STATUS_SUCCESS)
            return RtlNtStatusToDosError( status );
        if (!(control & SE_SELF_RELATIVE))
            return ERROR_INVALID_SECURITY_DESCR;

        desc_size = sizeof(desc);
        status = RtlSelfRelativeToAbsoluteSD( pOldSD, &desc, &desc_size, dacl, &dacl_size, sacl, &sacl_size,
                                              owner, &owner_size, group, &group_size );
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            if (dacl_size)
                dacl = LocalAlloc( LMEM_FIXED, dacl_size );
            if (sacl_size)
                sacl = LocalAlloc( LMEM_FIXED, sacl_size );
            if (owner_size)
                owner = LocalAlloc( LMEM_FIXED, owner_size );
            if (group_size)
                group = LocalAlloc( LMEM_FIXED, group_size );

            desc_size = sizeof(desc);
            status = RtlSelfRelativeToAbsoluteSD( pOldSD, &desc, &desc_size, dacl, &dacl_size, sacl, &sacl_size,
                                                  owner, &owner_size, group, &group_size );
        }
        if (status != STATUS_SUCCESS)
        {
            LocalFree( dacl );
            LocalFree( sacl );
            LocalFree( owner );
            LocalFree( group );
            return RtlNtStatusToDosError( status );
        }
    }
    else
    {
        if ((status = RtlCreateSecurityDescriptor( &desc, SECURITY_DESCRIPTOR_REVISION )) != STATUS_SUCCESS)
            return RtlNtStatusToDosError( status );
    }

    if (pOwner)
    {
        LocalFree( desc.Owner );
        desc.Owner = LocalAlloc( LMEM_FIXED, sizeof(MAX_SID) );
        if ((ret = trustee_to_sid( sizeof(MAX_SID), desc.Owner, pOwner )))
            goto done;
    }

    if (pGroup)
    {
        LocalFree( desc.Group );
        desc.Group = LocalAlloc( LMEM_FIXED, sizeof(MAX_SID) );
        if ((ret = trustee_to_sid( sizeof(MAX_SID), desc.Group, pGroup )))
            goto done;
    }

    if (pListOfAccessEntries)
    {
        PACL new_dacl;

        if ((ret = SetEntriesInAclW( cCountOfAccessEntries, pListOfAccessEntries, desc.Dacl, &new_dacl )))
            goto done;

        LocalFree( desc.Dacl );
        desc.Dacl = new_dacl;
        desc.Control |= SE_DACL_PRESENT;
    }

    if (pListOfAuditEntries)
    {
        PACL new_sacl;

        if ((ret = SetEntriesInAclW( cCountOfAuditEntries, pListOfAuditEntries, desc.Sacl, &new_sacl )))
            goto done;

        LocalFree( desc.Sacl );
        desc.Sacl = new_sacl;
        desc.Control |= SE_SACL_PRESENT;
    }

    *lpdwBufferLength = RtlLengthSecurityDescriptor( &desc );
    *pNewSD = LocalAlloc( LMEM_FIXED, *lpdwBufferLength );

    if ((status = RtlMakeSelfRelativeSD( &desc, *pNewSD, lpdwBufferLength )) != STATUS_SUCCESS)
    {
        ret = RtlNtStatusToDosError( status );
        LocalFree( *pNewSD );
        *pNewSD = NULL;
    }

done:
    /* free absolute descriptor */
    LocalFree( desc.Owner );
    LocalFree( desc.Group );
    LocalFree( desc.Sacl );
    LocalFree( desc.Dacl );
    return ret;
} 

static const WCHAR * const WellKnownPrivNames[SE_MAX_WELL_KNOWN_PRIVILEGE + 1] =
{
    NULL,
    NULL,
    L"SeCreateTokenPrivilege",
    L"SeAssignPrimaryTokenPrivilege",
    L"SeLockMemoryPrivilege",
    L"SeIncreaseQuotaPrivilege",
    L"SeMachineAccountPrivilege",
    L"SeTcbPrivilege",
    L"SeSecurityPrivilege",
    L"SeTakeOwnershipPrivilege",
    L"SeLoadDriverPrivilege",
    L"SeSystemProfilePrivilege",
    L"SeSystemtimePrivilege",
    L"SeProfileSingleProcessPrivilege",
    L"SeIncreaseBasePriorityPrivilege",
    L"SeCreatePagefilePrivilege",
    L"SeCreatePermanentPrivilege",
    L"SeBackupPrivilege",
    L"SeRestorePrivilege",
    L"SeShutdownPrivilege",
    L"SeDebugPrivilege",
    L"SeAuditPrivilege",
    L"SeSystemEnvironmentPrivilege",
    L"SeChangeNotifyPrivilege",
    L"SeRemoteShutdownPrivilege",
    L"SeUndockPrivilege",
    L"SeSyncAgentPrivilege",
    L"SeEnableDelegationPrivilege",
    L"SeManageVolumePrivilege",
    L"SeImpersonatePrivilege",
    L"SeCreateGlobalPrivilege",
};

const WCHAR *get_wellknown_privilege_name(const LUID *luid)
{
    if (luid->HighPart || luid->LowPart < SE_MIN_WELL_KNOWN_PRIVILEGE ||
            luid->LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE || !WellKnownPrivNames[luid->LowPart])
        return NULL;

    return WellKnownPrivNames[luid->LowPart];
}

/******************************************************************************
 * LookupPrivilegeValueW			[ADVAPI32.@]
 *
 * See LookupPrivilegeValueA.
 */
BOOL WINAPI
LookupPrivilegeValueW( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid )
{
    UINT i;

    TRACE("%s,%s,%p\n",debugstr_w(lpSystemName), debugstr_w(lpName), lpLuid);

    if (!ADVAPI_IsLocalComputer(lpSystemName))
    {
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        return FALSE;
    }
    if (!lpName)
    {
        SetLastError(ERROR_NO_SUCH_PRIVILEGE);
        return FALSE;
    }
    for( i=SE_MIN_WELL_KNOWN_PRIVILEGE; i<=SE_MAX_WELL_KNOWN_PRIVILEGE; i++ )
    {
        if( !WellKnownPrivNames[i] )
            continue;
        if( wcsicmp( WellKnownPrivNames[i], lpName) )
            continue;
        lpLuid->LowPart = i;
        lpLuid->HighPart = 0;
        TRACE( "%s -> %08lx-%08lx\n",debugstr_w( lpSystemName ),
               lpLuid->HighPart, lpLuid->LowPart );
        return TRUE;
    }
    SetLastError(ERROR_NO_SUCH_PRIVILEGE);
    return FALSE;
}

/******************************************************************************
 * LookupPrivilegeValueA			[ADVAPI32.@]
 *
 * Retrieves LUID used on a system to represent the privilege name.
 *
 * PARAMS
 *  lpSystemName [I] Name of the system
 *  lpName       [I] Name of the privilege
 *  lpLuid       [O] Destination for the resulting LUID
 *
 * RETURNS
 *  Success: TRUE. lpLuid contains the requested LUID.
 *  Failure: FALSE.
 */
BOOL WINAPI
LookupPrivilegeValueA( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid )
{
    UNICODE_STRING lpSystemNameW;
    UNICODE_STRING lpNameW;
    BOOL ret;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpNameW,lpName);
    ret = LookupPrivilegeValueW(lpSystemNameW.Buffer, lpNameW.Buffer, lpLuid);
    RtlFreeUnicodeString(&lpNameW);
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}

BOOL WINAPI LookupPrivilegeDisplayNameA( LPCSTR lpSystemName, LPCSTR lpName, LPSTR lpDisplayName,
                                         LPDWORD cchDisplayName, LPDWORD lpLanguageId )
{
    FIXME("%s %s %s %p %p - stub\n", debugstr_a(lpSystemName), debugstr_a(lpName),
          lpDisplayName, cchDisplayName, lpLanguageId);

    return FALSE;
}

BOOL WINAPI LookupPrivilegeDisplayNameW( LPCWSTR lpSystemName, LPCWSTR lpName, LPWSTR lpDisplayName,
                                         LPDWORD cchDisplayName, LPDWORD lpLanguageId )
{
    FIXME("%s %s %p %p %p - stub\n", debugstr_w(lpSystemName), debugstr_w(lpName),
          lpDisplayName, cchDisplayName, lpLanguageId);

    return FALSE;
}

/******************************************************************************
 * LookupPrivilegeNameA			[ADVAPI32.@]
 *
 * See LookupPrivilegeNameW.
 */
BOOL WINAPI
LookupPrivilegeNameA( LPCSTR lpSystemName, PLUID lpLuid, LPSTR lpName,
 LPDWORD cchName)
{
    UNICODE_STRING lpSystemNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %p %p %p\n", debugstr_a(lpSystemName), lpLuid, lpName, cchName);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, NULL, &wLen);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpNameW = malloc(wLen * sizeof(WCHAR));

        ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, lpNameW,
         &wLen);
        if (ret)
        {
            /* Windows crashes if cchName is NULL, so will I */
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpNameW, -1, lpName,
             *cchName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchName)
            {
                *cchName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchName = len - 1;
            }
        }
        free(lpNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}

/******************************************************************************
 * LookupPrivilegeNameW			[ADVAPI32.@]
 *
 * Retrieves the privilege name referred to by the LUID lpLuid.
 *
 * PARAMS
 *  lpSystemName [I]   Name of the system
 *  lpLuid       [I]   Privilege value
 *  lpName       [O]   Name of the privilege
 *  cchName      [I/O] Number of characters in lpName.
 *
 * RETURNS
 *  Success: TRUE. lpName contains the name of the privilege whose value is
 *  *lpLuid.
 *  Failure: FALSE.
 *
 * REMARKS
 *  Only well-known privilege names (those defined in winnt.h) can be retrieved
 *  using this function.
 *  If the length of lpName is too small, on return *cchName will contain the
 *  number of WCHARs needed to contain the privilege, including the NULL
 *  terminator, and GetLastError will return ERROR_INSUFFICIENT_BUFFER.
 *  On success, *cchName will contain the number of characters stored in
 *  lpName, NOT including the NULL terminator.
 */
BOOL WINAPI
LookupPrivilegeNameW( LPCWSTR lpSystemName, PLUID lpLuid, LPWSTR lpName,
 LPDWORD cchName)
{
    size_t privNameLen;

    TRACE("%s,%p,%p,%p\n",debugstr_w(lpSystemName), lpLuid, lpName, cchName);

    if (!ADVAPI_IsLocalComputer(lpSystemName))
    {
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        return FALSE;
    }
    if (lpLuid->HighPart || (lpLuid->LowPart < SE_MIN_WELL_KNOWN_PRIVILEGE ||
     lpLuid->LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE))
    {
        SetLastError(ERROR_NO_SUCH_PRIVILEGE);
        return FALSE;
    }
    privNameLen = lstrlenW(WellKnownPrivNames[lpLuid->LowPart]);
    /* Windows crashes if cchName is NULL, so will I */
    if (*cchName <= privNameLen)
    {
        *cchName = privNameLen + 1;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    else
    {
        lstrcpyW(lpName, WellKnownPrivNames[lpLuid->LowPart]);
        *cchName = privNameLen;
        return TRUE;
    }
}

/******************************************************************************
 * GetFileSecurityA [ADVAPI32.@]
 *
 * Obtains Specified information about the security of a file or directory.
 *
 * PARAMS
 *  lpFileName           [I] Name of the file to get info for
 *  RequestedInformation [I] SE_ flags from "winnt.h"
 *  pSecurityDescriptor  [O] Destination for security information
 *  nLength              [I] Length of pSecurityDescriptor
 *  lpnLengthNeeded      [O] Destination for length of returned security information
 *
 * RETURNS
 *  Success: TRUE. pSecurityDescriptor contains the requested information.
 *  Failure: FALSE. lpnLengthNeeded contains the required space to return the info. 
 *
 * NOTES
 *  The information returned is constrained by the callers access rights and
 *  privileges.
 */
BOOL WINAPI
GetFileSecurityA( LPCSTR lpFileName,
                    SECURITY_INFORMATION RequestedInformation,
                    PSECURITY_DESCRIPTOR pSecurityDescriptor,
                    DWORD nLength, LPDWORD lpnLengthNeeded )
{
    BOOL r;
    LPWSTR name;

    name = strdupAW(lpFileName);
    r = GetFileSecurityW( name, RequestedInformation, pSecurityDescriptor,
                          nLength, lpnLengthNeeded );
    free( name );

    return r;
}

/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountSidA(
	IN LPCSTR system,
	IN PSID sid,
	OUT LPSTR account,
	IN OUT LPDWORD accountSize,
	OUT LPSTR domain,
	IN OUT LPDWORD domainSize,
	OUT PSID_NAME_USE name_use )
{
    DWORD len;
    BOOL r;
    LPWSTR systemW;
    LPWSTR accountW = NULL;
    LPWSTR domainW = NULL;
    DWORD accountSizeW = *accountSize;
    DWORD domainSizeW = *domainSize;

    systemW = strdupAW(system);
    if (account)
        accountW = malloc( accountSizeW * sizeof(WCHAR) );
    if (domain)
        domainW = malloc( domainSizeW * sizeof(WCHAR) );

    r = LookupAccountSidW( systemW, sid, accountW, &accountSizeW, domainW, &domainSizeW, name_use );

    if (r) {
        if (accountW && *accountSize) {
            len = WideCharToMultiByte( CP_ACP, 0, accountW, -1, NULL, 0, NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, accountW, -1, account, len, NULL, NULL );
            *accountSize = len;
        } else
            *accountSize = accountSizeW + 1;

        if (domainW && *domainSize) {
            len = WideCharToMultiByte( CP_ACP, 0, domainW, -1, NULL, 0, NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, domainW, -1, domain, len, NULL, NULL );
            *domainSize = len;
        } else
            *domainSize = domainSizeW + 1;
    }
    else
    {
        *accountSize = accountSizeW + 1;
        *domainSize = domainSizeW + 1;
    }

    free( systemW );
    free( accountW );
    free( domainW );

    return r;
}

/******************************************************************************
 * LookupAccountSidLocalA [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountSidLocalA(
	PSID sid,
	LPSTR account,
	LPDWORD accountSize,
	LPSTR domain,
	LPDWORD domainSize,
	PSID_NAME_USE name_use )
{
    return LookupAccountSidA(NULL, sid, account, accountSize, domain, domainSize, name_use);
}

/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
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

BOOL WINAPI
LookupAccountSidW(
	IN LPCWSTR system,
	IN PSID sid,
	OUT LPWSTR account,
	IN OUT LPDWORD accountSize,
	OUT LPWSTR domain,
	IN OUT LPDWORD domainSize,
	OUT PSID_NAME_USE name_use )
{
    unsigned int i, j;
    const WCHAR * ac = NULL;
    const WCHAR * dm = NULL;
    SID_NAME_USE use = 0;
    LPWSTR computer_name = NULL;
    LPWSTR account_name = NULL;

    TRACE("(%s,sid=%s,%p,%p(%lu),%p,%p(%lu),%p)\n",
	  debugstr_w(system),debugstr_sid(sid),
	  account,accountSize,accountSize?*accountSize:0,
	  domain,domainSize,domainSize?*domainSize:0,
	  name_use);

    if (!ADVAPI_IsLocalComputer(system)) {
        FIXME("Only local computer supported!\n");
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        return FALSE;
    }

    /* check the well known SIDs first */
    for (i = 0; i <= WinAccountProtectedUsersSid; i++) {
        if (IsWellKnownSid(sid, i)) {
            for (j = 0; j < ARRAY_SIZE(ACCOUNT_SIDS); j++) {
                if (ACCOUNT_SIDS[j].type == i) {
                    ac = ACCOUNT_SIDS[j].account;
                    dm = ACCOUNT_SIDS[j].domain;
                    use = ACCOUNT_SIDS[j].name_use;
                }
            }
            break;
        }
    }

    if (dm == NULL) {
        MAX_SID local;

        /* check for the local computer next */
        if (ADVAPI_GetComputerSid(&local)) {
            DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
            BOOL result;

            computer_name = malloc(size * sizeof(WCHAR));
            result = GetComputerNameW(computer_name,  &size);

            if (result) {
                if (EqualSid(sid, &local)) {
                    dm = computer_name;
                    ac = L"";
                    use = 3;
                } else {
                    local.SubAuthorityCount++;

                    if (EqualPrefixSid(sid, &local)) {
                        dm = computer_name;
                        use = 1;
                        switch (((MAX_SID *)sid)->SubAuthority[4]) {
                        case DOMAIN_USER_RID_ADMIN:
                            ac = L"Administrator";
                            break;
                        case DOMAIN_USER_RID_GUEST:
                            ac = L"Guest";
                            break;
                        case DOMAIN_GROUP_RID_ADMINS:
                            ac = L"Domain Admins";
                            break;
                        case DOMAIN_GROUP_RID_USERS:
                            ac = L"None";
                            use = SidTypeGroup;
                            break;
                        case DOMAIN_GROUP_RID_GUESTS:
                            ac = L"Domain Guests";
                            break;
                        case DOMAIN_GROUP_RID_COMPUTERS:
                            ac = L"Domain Computers";
                            break;
                        case DOMAIN_GROUP_RID_CONTROLLERS:
                            ac = L"Domain Controllers";
                            break;
                        case DOMAIN_GROUP_RID_CERT_ADMINS:
                            ac = L"Cert Publishers";
                            break;
                        case DOMAIN_GROUP_RID_SCHEMA_ADMINS:
                            ac = L"Schema Admins";
                            break;
                        case DOMAIN_GROUP_RID_ENTERPRISE_ADMINS:
                            ac = L"Enterprise Admins";
                            break;
                        case DOMAIN_GROUP_RID_POLICY_ADMINS:
                            ac = L"Group Policy Creator Owners";
                            break;
                        case DOMAIN_ALIAS_RID_RAS_SERVERS:
                            ac = L"RAS and IAS Servers";
                            break;
                        case 1000:	/* first user account */
                            size = UNLEN + 1;
                            account_name = malloc(size * sizeof(WCHAR));
                            if (GetUserNameW(account_name, &size))
                                ac = account_name;
                            else
                                dm = NULL;

                            break;
                        default:
                            dm = NULL;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (dm) {
        DWORD ac_len = lstrlenW(ac);
        DWORD dm_len = lstrlenW(dm);
        BOOL status = TRUE;

        if (*accountSize > ac_len) {
            if (account)
                lstrcpyW(account, ac);
        }
        if (*domainSize > dm_len) {
            if (domain)
                lstrcpyW(domain, dm);
        }
        if ((*accountSize && *accountSize < ac_len) ||
            (!account && !*accountSize && ac_len)   ||
            (*domainSize && *domainSize < dm_len)   ||
            (!domain && !*domainSize && dm_len))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            status = FALSE;
        }
        if (*domainSize)
            *domainSize = dm_len;
        else
            *domainSize = dm_len + 1;
        if (*accountSize)
            *accountSize = ac_len;
        else
            *accountSize = ac_len + 1;

        free(account_name);
        free(computer_name);
        if (status) *name_use = use;
        return status;
    }

    free(account_name);
    free(computer_name);
    SetLastError(ERROR_NONE_MAPPED);
    return FALSE;
}

/******************************************************************************
 * LookupAccountSidLocalW [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountSidLocalW(
	PSID sid,
	LPWSTR account,
	LPDWORD accountSize,
	LPWSTR domain,
	LPDWORD domainSize,
	PSID_NAME_USE name_use )
{
    return LookupAccountSidW(NULL, sid, account, accountSize, domain, domainSize, name_use);
}

/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 *
 * See SetFileSecurityW.
 */
BOOL WINAPI SetFileSecurityA( LPCSTR lpFileName,
                                SECURITY_INFORMATION RequestedInformation,
                                PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    BOOL r;
    LPWSTR name;

    name = strdupAW(lpFileName);
    r = SetFileSecurityW( name, RequestedInformation, pSecurityDescriptor );
    free( name );

    return r;
}

/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	FIXME("(%ld):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub\n",x1,x2,x3,x4);
	return TRUE;
}

/******************************************************************************
 * NotifyBootConfigStatus [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
NotifyBootConfigStatus( BOOL x1 )
{
	FIXME("(0x%08d):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountNameA(
	IN LPCSTR system,
	IN LPCSTR account,
	OUT PSID sid,
	OUT LPDWORD cbSid,
	LPSTR ReferencedDomainName,
	IN OUT LPDWORD cbReferencedDomainName,
	OUT PSID_NAME_USE name_use )
{
    BOOL ret;
    UNICODE_STRING lpSystemW;
    UNICODE_STRING lpAccountW;
    LPWSTR lpReferencedDomainNameW = NULL;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemW, system);
    RtlCreateUnicodeStringFromAsciiz(&lpAccountW, account);

    if (ReferencedDomainName)
        lpReferencedDomainNameW = malloc(*cbReferencedDomainName * sizeof(WCHAR));

    ret = LookupAccountNameW(lpSystemW.Buffer, lpAccountW.Buffer, sid, cbSid, lpReferencedDomainNameW,
        cbReferencedDomainName, name_use);

    if (ret && lpReferencedDomainNameW)
    {
        WideCharToMultiByte(CP_ACP, 0, lpReferencedDomainNameW, -1,
            ReferencedDomainName, *cbReferencedDomainName+1, NULL, NULL);
    }

    RtlFreeUnicodeString(&lpSystemW);
    RtlFreeUnicodeString(&lpAccountW);
    free(lpReferencedDomainNameW);

    return ret;
}

/******************************************************************************
 * lookup_user_account_name
 */
static BOOL lookup_user_account_name(PSID Sid, PDWORD cbSid, LPWSTR ReferencedDomainName,
                                     LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    char buffer[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    DWORD len = sizeof(buffer);
    HANDLE token;
    BOOL ret;
    PSID pSid;
    WCHAR domainName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD nameLen;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) return FALSE;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) return FALSE;
    }

    ret = GetTokenInformation(token, TokenUser, buffer, len, &len);
    CloseHandle( token );

    if (!ret) return FALSE;

    pSid = ((TOKEN_USER *)buffer)->User.Sid;

    if (Sid != NULL && (*cbSid >= GetLengthSid(pSid)))
       CopySid(*cbSid, Sid, pSid);
    if (*cbSid < GetLengthSid(pSid))
    {
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       ret = FALSE;
    }
    *cbSid = GetLengthSid(pSid);

    nameLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(domainName, &nameLen))
    {
        domainName[0] = 0;
        nameLen = 0;
    }
    if (*cchReferencedDomainName <= nameLen || !ret)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        nameLen += 1;
        ret = FALSE;
    }
    else if (ReferencedDomainName)
        lstrcpyW(ReferencedDomainName, domainName);

    *cchReferencedDomainName = nameLen;

    if (ret)
        *peUse = SidTypeUser;

    return ret;
}

/******************************************************************************
 * lookup_computer_account_name
 */
static BOOL lookup_computer_account_name(PSID Sid, PDWORD cbSid, LPWSTR ReferencedDomainName,
                                         LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    MAX_SID local;
    BOOL ret;
    WCHAR domainName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD nameLen;

    if ((ret = ADVAPI_GetComputerSid(&local)))
    {
        if (Sid != NULL && (*cbSid >= GetLengthSid(&local)))
           CopySid(*cbSid, Sid, &local);
        if (*cbSid < GetLengthSid(&local))
        {
           SetLastError(ERROR_INSUFFICIENT_BUFFER);
           ret = FALSE;
        }
        *cbSid = GetLengthSid(&local);
    }

    nameLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(domainName, &nameLen))
    {
        domainName[0] = 0;
        nameLen = 0;
    }
    if (*cchReferencedDomainName <= nameLen || !ret)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        nameLen += 1;
        ret = FALSE;
    }
    else if (ReferencedDomainName)
        lstrcpyW(ReferencedDomainName, domainName);

    *cchReferencedDomainName = nameLen;

    if (ret)
        *peUse = SidTypeDomain;

    return ret;
}

static void split_domain_account( const LSA_UNICODE_STRING *str, LSA_UNICODE_STRING *account,
                                  LSA_UNICODE_STRING *domain )
{
    WCHAR *p = str->Buffer + str->Length / sizeof(WCHAR) - 1;

    while (p > str->Buffer && *p != '\\') p--;

    if (*p == '\\')
    {
        domain->Buffer = str->Buffer;
        domain->Length = (p - str->Buffer) * sizeof(WCHAR);

        account->Buffer = p + 1;
        account->Length = str->Length - ((p - str->Buffer + 1) * sizeof(WCHAR));
    }
    else
    {
        domain->Buffer = NULL;
        domain->Length = 0;

        account->Buffer = str->Buffer;
        account->Length = str->Length;
    }
}

static BOOL match_domain( ULONG idx, const LSA_UNICODE_STRING *domain )
{
    ULONG len = lstrlenW( ACCOUNT_SIDS[idx].domain );

    if (len == domain->Length / sizeof(WCHAR) && !wcsnicmp( domain->Buffer, ACCOUNT_SIDS[idx].domain, len ))
        return TRUE;

    return FALSE;
}

static BOOL match_account( ULONG idx, const LSA_UNICODE_STRING *account )
{
    ULONG len = lstrlenW( ACCOUNT_SIDS[idx].account );

    if (len == account->Length / sizeof(WCHAR) && !wcsnicmp( account->Buffer, ACCOUNT_SIDS[idx].account, len ))
        return TRUE;

    if (ACCOUNT_SIDS[idx].alias)
    {
        len = lstrlenW( ACCOUNT_SIDS[idx].alias );
        if (len == account->Length / sizeof(WCHAR) && !wcsnicmp( account->Buffer, ACCOUNT_SIDS[idx].alias, len ))
            return TRUE;
    }
    return FALSE;
}

/*
 * Helper function for LookupAccountNameW
 */
BOOL lookup_local_wellknown_name( const LSA_UNICODE_STRING *account_and_domain,
                                  PSID Sid, LPDWORD cbSid,
                                  LPWSTR ReferencedDomainName,
                                  LPDWORD cchReferencedDomainName,
                                  PSID_NAME_USE peUse, BOOL *handled )
{
    PSID pSid;
    LSA_UNICODE_STRING account, domain;
    BOOL ret = TRUE;
    ULONG i;

    *handled = FALSE;
    split_domain_account( account_and_domain, &account, &domain );

    for (i = 0; i < ARRAY_SIZE(ACCOUNT_SIDS); i++)
    {
        /* check domain first */
        if (domain.Buffer && !match_domain( i, &domain )) continue;

        if (match_account( i, &account ))
        {
            DWORD len, sidLen = SECURITY_MAX_SID_SIZE;

            if (!(pSid = malloc( sidLen ))) return FALSE;

            if ((ret = CreateWellKnownSid( ACCOUNT_SIDS[i].type, NULL, pSid, &sidLen )))
            {
                if (*cbSid < sidLen)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    ret = FALSE;
                }
                else if (Sid)
                {
                    CopySid(*cbSid, Sid, pSid);
                }
                *cbSid = sidLen;
            }

            len = lstrlenW( ACCOUNT_SIDS[i].domain );
            if (*cchReferencedDomainName <= len || !ret)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                len++;
                ret = FALSE;
            }
            else if (ReferencedDomainName)
            {
                lstrcpyW( ReferencedDomainName, ACCOUNT_SIDS[i].domain );
            }

            *cchReferencedDomainName = len;
            if (ret)
                *peUse = ACCOUNT_SIDS[i].name_use;

            free(pSid);
            *handled = TRUE;
            return ret;
        }
    }
    return ret;
}

BOOL lookup_local_user_name( const LSA_UNICODE_STRING *account_and_domain,
                             PSID Sid, LPDWORD cbSid,
                             LPWSTR ReferencedDomainName,
                             LPDWORD cchReferencedDomainName,
                             PSID_NAME_USE peUse, BOOL *handled )
{
    DWORD nameLen;
    LPWSTR userName = NULL;
    LSA_UNICODE_STRING account, domain;
    BOOL ret = TRUE;

    *handled = FALSE;
    split_domain_account( account_and_domain, &account, &domain );

    /* Let the current Unix user id masquerade as first Windows user account */

    nameLen = UNLEN + 1;
    if (!(userName = malloc( nameLen * sizeof(WCHAR) ))) return FALSE;

    if (domain.Buffer)
    {
        /* check to make sure this account is on this computer */
        if (GetComputerNameW( userName, &nameLen ) &&
            (domain.Length / sizeof(WCHAR) != nameLen || wcsncmp( domain.Buffer, userName, nameLen )))
        {
            SetLastError(ERROR_NONE_MAPPED);
            ret = FALSE;
        }
        nameLen = UNLEN + 1;
    }

    if (GetUserNameW( userName, &nameLen ) &&
        account.Length / sizeof(WCHAR) == nameLen - 1 && !wcsncmp( account.Buffer, userName, nameLen - 1 ))
    {
            ret = lookup_user_account_name( Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse );
            *handled = TRUE;
    }
    else
    {
        nameLen = UNLEN + 1;
        if (GetComputerNameW( userName, &nameLen ) &&
            account.Length / sizeof(WCHAR) == nameLen && !wcsncmp( account.Buffer, userName , nameLen ))
        {
            ret = lookup_computer_account_name( Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse );
            *handled = TRUE;
        }
    }

    free(userName);
    return ret;
}

/******************************************************************************
 * LookupAccountNameW [ADVAPI32.@]
 */
BOOL WINAPI LookupAccountNameW( LPCWSTR lpSystemName, LPCWSTR lpAccountName, PSID Sid,
                                LPDWORD cbSid, LPWSTR ReferencedDomainName,
                                LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    BOOL ret, handled;
    LSA_UNICODE_STRING account;

    TRACE("%s %s %p %p %p %p %p\n", debugstr_w(lpSystemName), debugstr_w(lpAccountName),
          Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse);

    if (!ADVAPI_IsLocalComputer( lpSystemName ))
    {
        FIXME("remote computer not supported\n");
        SetLastError( RPC_S_SERVER_UNAVAILABLE );
        return FALSE;
    }

    if (!lpAccountName || !wcscmp( lpAccountName, L"" ))
    {
        lpAccountName = L"BUILTIN";
    }

    RtlInitUnicodeString( &account, lpAccountName );

    /* Check well known SIDs first */
    ret = lookup_local_wellknown_name( &account, Sid, cbSid, ReferencedDomainName,
                                       cchReferencedDomainName, peUse, &handled );
    if (handled)
        return ret;

    /* Check user names */
    ret = lookup_local_user_name( &account, Sid, cbSid, ReferencedDomainName,
                                  cchReferencedDomainName, peUse, &handled);
    if (handled)
        return ret;

    SetLastError( ERROR_NONE_MAPPED );
    return FALSE;
}

/******************************************************************************
 * AccessCheckAndAuditAlarmA [ADVAPI32.@]
 */
BOOL WINAPI AccessCheckAndAuditAlarmA(LPCSTR Subsystem, LPVOID HandleId, LPSTR ObjectTypeName,
  LPSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess,
  PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess,
  LPBOOL AccessStatus, LPBOOL pfGenerateOnClose)
{
	FIXME("stub (%s,%p,%s,%s,%p,%08lx,%p,%x,%p,%p,%p)\n", debugstr_a(Subsystem),
		HandleId, debugstr_a(ObjectTypeName), debugstr_a(ObjectName),
		SecurityDescriptor, DesiredAccess, GenericMapping,
		ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose);
	return TRUE;
}

BOOL WINAPI ObjectCloseAuditAlarmA(LPCSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose)
{
    FIXME("stub (%s,%p,%x)\n", debugstr_a(SubsystemName), HandleId, GenerateOnClose);

    return TRUE;
}

BOOL WINAPI ObjectOpenAuditAlarmA(LPCSTR SubsystemName, LPVOID HandleId, LPSTR ObjectTypeName,
  LPSTR ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess,
  DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted,
  LPBOOL GenerateOnClose)
{
	FIXME("stub (%s,%p,%s,%s,%p,%p,0x%08lx,0x%08lx,%p,%x,%x,%p)\n", debugstr_a(SubsystemName),
		HandleId, debugstr_a(ObjectTypeName), debugstr_a(ObjectName), pSecurityDescriptor,
        ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted,
        GenerateOnClose);

    return TRUE;
}

BOOL WINAPI ObjectPrivilegeAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken,
  DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted)
{
    FIXME("stub (%s,%p,%p,0x%08lx,%p,%x)\n", debugstr_a(SubsystemName), HandleId, ClientToken,
          DesiredAccess, Privileges, AccessGranted);

    return TRUE;
}

BOOL WINAPI PrivilegedServiceAuditAlarmA( LPCSTR SubsystemName, LPCSTR ServiceName, HANDLE ClientToken,
                                   PPRIVILEGE_SET Privileges, BOOL AccessGranted)
{
    FIXME("stub (%s,%s,%p,%p,%x)\n", debugstr_a(SubsystemName), debugstr_a(ServiceName),
          ClientToken, Privileges, AccessGranted);

    return TRUE;
}

#define HKEY_SPECIAL_ROOT_FIRST   HKEY_CLASSES_ROOT
#define HKEY_SPECIAL_ROOT_LAST    HKEY_DYN_DATA

/******************************************************************************
 * GetSecurityInfo [ADVAPI32.@]
 *
 * Retrieves a copy of the security descriptor associated with an object.
 *
 * PARAMS
 *  hObject              [I] A handle for the object.
 *  ObjectType           [I] The type of object.
 *  SecurityInfo         [I] A bitmask indicating what info to retrieve.
 *  ppsidOwner           [O] If non-null, receives a pointer to the owner SID.
 *  ppsidGroup           [O] If non-null, receives a pointer to the group SID.
 *  ppDacl               [O] If non-null, receives a pointer to the DACL.
 *  ppSacl               [O] If non-null, receives a pointer to the SACL.
 *  ppSecurityDescriptor [O] Receives a pointer to the security descriptor,
 *                           which must be freed with LocalFree.
 *
 * RETURNS
 *  ERROR_SUCCESS if all's well, and a WIN32 error code otherwise.
 */
DWORD WINAPI GetSecurityInfo( HANDLE handle, SE_OBJECT_TYPE type, SECURITY_INFORMATION SecurityInfo,
                              PSID *ppsidOwner, PSID *ppsidGroup, PACL *ppDacl, PACL *ppSacl,
                              PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{
    PSECURITY_DESCRIPTOR sd;
    NTSTATUS status;
    ULONG size;
    BOOL present, defaulted;
    HKEY key = NULL;

    if (!handle)
        return ERROR_INVALID_HANDLE;

    /* A NULL descriptor is allowed if any one of the other pointers is not NULL */
    if (!(ppsidOwner||ppsidGroup||ppDacl||ppSacl||ppSecurityDescriptor)) return ERROR_INVALID_PARAMETER;

    /* If no descriptor, we have to check that there's a pointer for the requested information */
    if( !ppSecurityDescriptor && (
        ((SecurityInfo & OWNER_SECURITY_INFORMATION) && !ppsidOwner)
    ||  ((SecurityInfo & GROUP_SECURITY_INFORMATION) && !ppsidGroup)
    ||  ((SecurityInfo & DACL_SECURITY_INFORMATION)  && !ppDacl)
    ||  ((SecurityInfo & SACL_SECURITY_INFORMATION)  && !ppSacl)  ))
        return ERROR_INVALID_PARAMETER;

    switch (type)
    {
    case SE_SERVICE:
        if (!QueryServiceObjectSecurity( handle, SecurityInfo, NULL, 0, &size )
            && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return GetLastError();

        if (!(sd = LocalAlloc( 0, size ))) return ERROR_NOT_ENOUGH_MEMORY;

        if (!QueryServiceObjectSecurity( handle, SecurityInfo, sd, size, &size ))
        {
            LocalFree(sd);
            return GetLastError();
        }
        break;

    case SE_WINDOW_OBJECT:
        if (!GetUserObjectSecurity( handle, &SecurityInfo, NULL, 0, &size )
                && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return GetLastError();

        if (!(sd = LocalAlloc( 0, size )))
            return ERROR_NOT_ENOUGH_MEMORY;

        if (!GetUserObjectSecurity( handle, &SecurityInfo, sd, size, &size ))
        {
            LocalFree( sd );
            return GetLastError();
        }
        break;

    case SE_KERNEL_OBJECT:
    case SE_FILE_OBJECT:
    case SE_WMIGUID_OBJECT:
    case SE_REGISTRY_KEY:
        if (type == SE_REGISTRY_KEY && (HandleToUlong(handle) >= HandleToUlong(HKEY_SPECIAL_ROOT_FIRST))
                && (HandleToUlong(handle) <= HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
        {
            REGSAM access = READ_CONTROL;
            DWORD ret;

            if (SecurityInfo & SACL_SECURITY_INFORMATION)
                access |= ACCESS_SYSTEM_SECURITY;

            if ((ret = RegCreateKeyExW( handle, NULL, 0, NULL, 0, access, NULL, &key, NULL )))
                return ret;

            handle = key;
        }

        status = NtQuerySecurityObject( handle, SecurityInfo, NULL, 0, &size );
        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
        {
            RegCloseKey( key );
            return RtlNtStatusToDosError( status );
        }

        if (!(sd = LocalAlloc( 0, size )))
        {
            RegCloseKey( key );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ((status = NtQuerySecurityObject( handle, SecurityInfo, sd, size, &size )))
        {
            RegCloseKey( key );
            LocalFree(sd);
            return RtlNtStatusToDosError( status );
        }
        RegCloseKey( key );
        break;

    default:
        FIXME("unimplemented type %u\n", type);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (ppsidOwner)
    {
        *ppsidOwner = NULL;
        GetSecurityDescriptorOwner(sd, ppsidOwner, &defaulted);
    }
    if (ppsidGroup)
    {
        *ppsidGroup = NULL;
        GetSecurityDescriptorGroup(sd, ppsidGroup, &defaulted);
    }
    if (ppDacl)
    {
        *ppDacl = NULL;
        GetSecurityDescriptorDacl(sd, &present, ppDacl, &defaulted);
    }
    if (ppSacl)
    {
        *ppSacl = NULL;
        GetSecurityDescriptorSacl(sd, &present, ppSacl, &defaulted);
    }
    if (ppSecurityDescriptor)
        *ppSecurityDescriptor = sd;

    /* The security descriptor (sd) cannot be freed if ppSecurityDescriptor is
     * NULL, because native happily returns the SIDs and ACLs that are requested
     * in this case.
     */

    return ERROR_SUCCESS;
}

/******************************************************************************
 * GetSecurityInfoExA [ADVAPI32.@]
 */
DWORD WINAPI GetSecurityInfoExA(
	HANDLE hObject, SE_OBJECT_TYPE ObjectType,
	SECURITY_INFORMATION SecurityInfo, LPCSTR lpProvider,
	LPCSTR lpProperty, PACTRL_ACCESSA *ppAccessList,
	PACTRL_AUDITA *ppAuditList, LPSTR *lppOwner, LPSTR *lppGroup
)
{
  FIXME("stub!\n");
  return ERROR_BAD_PROVIDER;
}

/******************************************************************************
 * GetSecurityInfoExW [ADVAPI32.@]
 */
DWORD WINAPI GetSecurityInfoExW(
	HANDLE hObject, SE_OBJECT_TYPE ObjectType, 
	SECURITY_INFORMATION SecurityInfo, LPCWSTR lpProvider,
	LPCWSTR lpProperty, PACTRL_ACCESSW *ppAccessList, 
	PACTRL_AUDITW *ppAuditList, LPWSTR *lppOwner, LPWSTR *lppGroup
)
{
  FIXME("stub!\n");
  return ERROR_BAD_PROVIDER; 
}

/******************************************************************************
 * BuildExplicitAccessWithNameA [ADVAPI32.@]
 */
VOID WINAPI BuildExplicitAccessWithNameA( PEXPLICIT_ACCESSA pExplicitAccess,
                                          LPSTR pTrusteeName, DWORD AccessPermissions,
                                          ACCESS_MODE AccessMode, DWORD Inheritance )
{
    TRACE("%p %s 0x%08lx 0x%08x 0x%08lx\n", pExplicitAccess, debugstr_a(pTrusteeName),
          AccessPermissions, AccessMode, Inheritance);

    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}

/******************************************************************************
 * BuildExplicitAccessWithNameW [ADVAPI32.@]
 */
VOID WINAPI BuildExplicitAccessWithNameW( PEXPLICIT_ACCESSW pExplicitAccess,
                                          LPWSTR pTrusteeName, DWORD AccessPermissions,
                                          ACCESS_MODE AccessMode, DWORD Inheritance )
{
    TRACE("%p %s 0x%08lx 0x%08x 0x%08lx\n", pExplicitAccess, debugstr_w(pTrusteeName),
          AccessPermissions, AccessMode, Inheritance);

    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndNameA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndNameA( PTRUSTEEA pTrustee, POBJECTS_AND_NAME_A pObjName,
                                             SE_OBJECT_TYPE ObjectType, LPSTR ObjectTypeName,
                                             LPSTR InheritedObjectTypeName, LPSTR Name )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
          ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_a(Name));

    /* Fill the OBJECTS_AND_NAME structure */
    pObjName->ObjectType = ObjectType;
    if (ObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;
    if (InheritedObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ObjectsPresent = ObjectsPresent;
    pObjName->ptstrName = Name;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR)pObjName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndNameW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndNameW( PTRUSTEEW pTrustee, POBJECTS_AND_NAME_W pObjName,
                                             SE_OBJECT_TYPE ObjectType, LPWSTR ObjectTypeName,
                                             LPWSTR InheritedObjectTypeName, LPWSTR Name )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
          ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_w(Name));

    /* Fill the OBJECTS_AND_NAME structure */
    pObjName->ObjectType = ObjectType;
    if (ObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;
    if (InheritedObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ObjectsPresent = ObjectsPresent;
    pObjName->ptstrName = Name;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR)pObjName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndSidA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndSidA( PTRUSTEEA pTrustee, POBJECTS_AND_SID pObjSid,
                                            GUID* pObjectGuid, GUID* pInheritedObjectGuid, PSID pSid )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    /* Fill the OBJECTS_AND_SID structure */
    if (pObjectGuid != NULL)
    {
        pObjSid->ObjectTypeGuid = *pObjectGuid;
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->ObjectTypeGuid,
                   sizeof(GUID));
    }

    if (pInheritedObjectGuid != NULL)
    {
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->InheritedObjectTypeGuid,
                   sizeof(GUID));
    }

    pObjSid->ObjectsPresent = ObjectsPresent;
    pObjSid->pSid = pSid;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pObjSid;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndSidW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndSidW( PTRUSTEEW pTrustee, POBJECTS_AND_SID pObjSid,
                                            GUID* pObjectGuid, GUID* pInheritedObjectGuid, PSID pSid )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    /* Fill the OBJECTS_AND_SID structure */
    if (pObjectGuid != NULL)
    {
        pObjSid->ObjectTypeGuid = *pObjectGuid;
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->ObjectTypeGuid,
                   sizeof(GUID));
    }

    if (pInheritedObjectGuid != NULL)
    {
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->InheritedObjectTypeGuid,
                   sizeof(GUID));
    }

    pObjSid->ObjectsPresent = ObjectsPresent;
    pObjSid->pSid = pSid;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pObjSid;
}

/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidA(PTRUSTEEA pTrustee, PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = pSid;
}

/******************************************************************************
 * BuildTrusteeWithSidW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidW(PTRUSTEEW pTrustee, PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = pSid;
}

/******************************************************************************
 * BuildTrusteeWithNameA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameA(PTRUSTEEA pTrustee, LPSTR name)
{
    TRACE("%p %s\n", pTrustee, debugstr_a(name) );

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/******************************************************************************
 * BuildTrusteeWithNameW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameW(PTRUSTEEW pTrustee, LPWSTR name)
{
    TRACE("%p %s\n", pTrustee, debugstr_w(name) );

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/****************************************************************************** 
 * GetTrusteeFormA [ADVAPI32.@] 
 */ 
TRUSTEE_FORM WINAPI GetTrusteeFormA(PTRUSTEEA pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_BAD_FORM; 
  
    return pTrustee->TrusteeForm; 
}  
  
/****************************************************************************** 
 * GetTrusteeFormW [ADVAPI32.@] 
 */ 
TRUSTEE_FORM WINAPI GetTrusteeFormW(PTRUSTEEW pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_BAD_FORM; 
  
    return pTrustee->TrusteeForm; 
}  
  
/****************************************************************************** 
 * GetTrusteeNameA [ADVAPI32.@] 
 */ 
LPSTR WINAPI GetTrusteeNameA(PTRUSTEEA pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return NULL; 
  
    return pTrustee->ptstrName; 
}  
  
/****************************************************************************** 
 * GetTrusteeNameW [ADVAPI32.@] 
 */ 
LPWSTR WINAPI GetTrusteeNameW(PTRUSTEEW pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return NULL; 
  
    return pTrustee->ptstrName; 
}  
  
/****************************************************************************** 
 * GetTrusteeTypeA [ADVAPI32.@] 
 */ 
TRUSTEE_TYPE WINAPI GetTrusteeTypeA(PTRUSTEEA pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_IS_UNKNOWN; 
  
    return pTrustee->TrusteeType; 
}  
  
/****************************************************************************** 
 * GetTrusteeTypeW [ADVAPI32.@] 
 */ 
TRUSTEE_TYPE WINAPI GetTrusteeTypeW(PTRUSTEEW pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_IS_UNKNOWN; 
  
    return pTrustee->TrusteeType; 
} 
 
static DWORD trustee_name_A_to_W(TRUSTEE_FORM form, char *trustee_nameA, WCHAR **ptrustee_nameW)
{
    switch (form)
    {
    case TRUSTEE_IS_NAME:
    {
        *ptrustee_nameW = strdupAW(trustee_nameA);
        return ERROR_SUCCESS;
    }
    case TRUSTEE_IS_OBJECTS_AND_NAME:
    {
        OBJECTS_AND_NAME_A *objA = (OBJECTS_AND_NAME_A *)trustee_nameA;
        OBJECTS_AND_NAME_W *objW = NULL;

        if (objA)
        {
            if (!(objW = malloc( sizeof(OBJECTS_AND_NAME_W) )))
                return ERROR_NOT_ENOUGH_MEMORY;

            objW->ObjectsPresent = objA->ObjectsPresent;
            objW->ObjectType = objA->ObjectType;
            objW->ObjectTypeName = strdupAW(objA->ObjectTypeName);
            objW->InheritedObjectTypeName = strdupAW(objA->InheritedObjectTypeName);
            objW->ptstrName = strdupAW(objA->ptstrName);
        }

        *ptrustee_nameW = (WCHAR *)objW;
        return ERROR_SUCCESS;
    }
    /* These forms do not require conversion. */
    case TRUSTEE_IS_SID:
    case TRUSTEE_IS_OBJECTS_AND_SID:
        *ptrustee_nameW = (WCHAR *)trustee_nameA;
        return ERROR_SUCCESS;
    default:
        return ERROR_INVALID_PARAMETER;
    }
}

static void free_trustee_name(TRUSTEE_FORM form, WCHAR *trustee_nameW)
{
    switch (form)
    {
    case TRUSTEE_IS_NAME:
        free( trustee_nameW );
        break;
    case TRUSTEE_IS_OBJECTS_AND_NAME:
    {
        OBJECTS_AND_NAME_W *objW = (OBJECTS_AND_NAME_W *)trustee_nameW;

        if (objW)
        {
            free( objW->ptstrName );
            free( objW->InheritedObjectTypeName );
            free( objW->ObjectTypeName );
            free( objW );
        }

        break;
    }
    /* Other forms did not require allocation, so no freeing is necessary. */
    default:
        break;
    }
}

static DWORD trustee_to_sid( DWORD nDestinationSidLength, PSID pDestinationSid, PTRUSTEEW pTrustee )
{
    if (pTrustee->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
    {
        WARN("bad multiple trustee operation %d\n", pTrustee->MultipleTrusteeOperation);
        return ERROR_INVALID_PARAMETER;
    }

    switch (pTrustee->TrusteeForm)
    {
    case TRUSTEE_IS_SID:
        if (!CopySid(nDestinationSidLength, pDestinationSid, pTrustee->ptstrName))
        {
            WARN("bad sid %p\n", pTrustee->ptstrName);
            return ERROR_INVALID_PARAMETER;
        }
        break;
    case TRUSTEE_IS_NAME:
    {
        DWORD sid_size = nDestinationSidLength;
        DWORD domain_size = MAX_COMPUTERNAME_LENGTH + 1;
        SID_NAME_USE use;
        if (!wcscmp( pTrustee->ptstrName, L"CURRENT_USER" ))
        {
            if (!lookup_user_account_name( pDestinationSid, &sid_size, NULL, &domain_size, &use ))
            {
                return GetLastError();
            }
        }
        else if (!LookupAccountNameW(NULL, pTrustee->ptstrName, pDestinationSid, &sid_size, NULL, &domain_size, &use))
        {
            WARN("bad user name %s\n", debugstr_w(pTrustee->ptstrName));
            return ERROR_INVALID_PARAMETER;
        }
        break;
    }
    case TRUSTEE_IS_OBJECTS_AND_SID:
        FIXME("TRUSTEE_IS_OBJECTS_AND_SID unimplemented\n");
        break;
    case TRUSTEE_IS_OBJECTS_AND_NAME:
        FIXME("TRUSTEE_IS_OBJECTS_AND_NAME unimplemented\n");
        break;
    default:
        WARN("bad trustee form %d\n", pTrustee->TrusteeForm);
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

/******************************************************************************
 * SetEntriesInAclA [ADVAPI32.@]
 */
DWORD WINAPI SetEntriesInAclA( ULONG count, PEXPLICIT_ACCESSA pEntries,
                               PACL OldAcl, PACL* NewAcl )
{
    DWORD err = ERROR_SUCCESS;
    EXPLICIT_ACCESSW *pEntriesW;
    UINT alloc_index, free_index;

    TRACE("%ld %p %p %p\n", count, pEntries, OldAcl, NewAcl);

    if (NewAcl)
        *NewAcl = NULL;

    if (!count && !OldAcl)
        return ERROR_SUCCESS;

    pEntriesW = malloc( count * sizeof(EXPLICIT_ACCESSW) );
    if (!pEntriesW)
        return ERROR_NOT_ENOUGH_MEMORY;

    for (alloc_index = 0; alloc_index < count; ++alloc_index)
    {
        pEntriesW[alloc_index].grfAccessPermissions = pEntries[alloc_index].grfAccessPermissions;
        pEntriesW[alloc_index].grfAccessMode = pEntries[alloc_index].grfAccessMode;
        pEntriesW[alloc_index].grfInheritance = pEntries[alloc_index].grfInheritance;
        pEntriesW[alloc_index].Trustee.pMultipleTrustee = NULL; /* currently not supported */
        pEntriesW[alloc_index].Trustee.MultipleTrusteeOperation = pEntries[alloc_index].Trustee.MultipleTrusteeOperation;
        pEntriesW[alloc_index].Trustee.TrusteeForm = pEntries[alloc_index].Trustee.TrusteeForm;
        pEntriesW[alloc_index].Trustee.TrusteeType = pEntries[alloc_index].Trustee.TrusteeType;

        err = trustee_name_A_to_W( pEntries[alloc_index].Trustee.TrusteeForm,
                                   pEntries[alloc_index].Trustee.ptstrName,
                                   &pEntriesW[alloc_index].Trustee.ptstrName );
        if (err != ERROR_SUCCESS)
        {
            if (err == ERROR_INVALID_PARAMETER)
                WARN("bad trustee form %d for trustee %d\n",
                     pEntries[alloc_index].Trustee.TrusteeForm, alloc_index);

            goto cleanup;
        }
    }

    err = SetEntriesInAclW( count, pEntriesW, OldAcl, NewAcl );

cleanup:
    /* Free any previously allocated trustee name buffers, taking into account
     * a possible out-of-memory condition while building the EXPLICIT_ACCESSW
     * list. */
    for (free_index = 0; free_index < alloc_index; ++free_index)
        free_trustee_name( pEntriesW[free_index].Trustee.TrusteeForm, pEntriesW[free_index].Trustee.ptstrName );

    free( pEntriesW );
    return err;
}

/******************************************************************************
 * SetEntriesInAclW [ADVAPI32.@]
 */
DWORD WINAPI SetEntriesInAclW( ULONG count, PEXPLICIT_ACCESSW pEntries,
                               PACL OldAcl, PACL* NewAcl )
{
    ULONG i;
    PSID *ppsid;
    DWORD ret = ERROR_SUCCESS;
    DWORD acl_size = sizeof(ACL);
    NTSTATUS status;

    TRACE("%ld %p %p %p\n", count, pEntries, OldAcl, NewAcl);

    if (NewAcl)
        *NewAcl = NULL;

    if (!count && !OldAcl)
        return ERROR_SUCCESS;

    /* allocate array of maximum sized sids allowed */
    ppsid = malloc(count * (sizeof(SID *) + FIELD_OFFSET(SID, SubAuthority[SID_MAX_SUB_AUTHORITIES])));
    if (!ppsid)
        return ERROR_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        ppsid[i] = (char *)&ppsid[count] + i * FIELD_OFFSET(SID, SubAuthority[SID_MAX_SUB_AUTHORITIES]);

        TRACE("[%ld]:\n\tgrfAccessPermissions = 0x%lx\n\tgrfAccessMode = %d\n\tgrfInheritance = 0x%lx\n\t"
              "Trustee.pMultipleTrustee = %p\n\tMultipleTrusteeOperation = %d\n\tTrusteeForm = %d\n\t"
              "Trustee.TrusteeType = %d\n\tptstrName = %p\n", i,
              pEntries[i].grfAccessPermissions, pEntries[i].grfAccessMode, pEntries[i].grfInheritance,
              pEntries[i].Trustee.pMultipleTrustee, pEntries[i].Trustee.MultipleTrusteeOperation,
              pEntries[i].Trustee.TrusteeForm, pEntries[i].Trustee.TrusteeType,
              pEntries[i].Trustee.ptstrName);

        ret = trustee_to_sid( FIELD_OFFSET(SID, SubAuthority[SID_MAX_SUB_AUTHORITIES]), ppsid[i], &pEntries[i].Trustee);
        if (ret)
            goto exit;

        /* Note: we overestimate the ACL size here as a tradeoff between
         * instructions (simplicity) and memory */
        switch (pEntries[i].grfAccessMode)
        {
        case GRANT_ACCESS:
        case SET_ACCESS:
            acl_size += FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + GetLengthSid(ppsid[i]);
            break;
        case DENY_ACCESS:
            acl_size += FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart) + GetLengthSid(ppsid[i]);
            break;
        case SET_AUDIT_SUCCESS:
        case SET_AUDIT_FAILURE:
            acl_size += FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart) + GetLengthSid(ppsid[i]);
            break;
        case REVOKE_ACCESS:
            break;
        default:
            WARN("bad access mode %d for trustee %ld\n", pEntries[i].grfAccessMode, i);
            ret = ERROR_INVALID_PARAMETER;
            goto exit;
        }
    }

    if (OldAcl)
    {
        ACL_SIZE_INFORMATION size_info;

        status = RtlQueryInformationAcl(OldAcl, &size_info, sizeof(size_info), AclSizeInformation);
        if (status != STATUS_SUCCESS)
        {
            ret = RtlNtStatusToDosError(status);
            goto exit;
        }
        acl_size += size_info.AclBytesInUse - sizeof(ACL);
    }

    *NewAcl = LocalAlloc(0, acl_size);
    if (!*NewAcl)
    {
        ret = ERROR_OUTOFMEMORY;
        goto exit;
    }

    status = RtlCreateAcl( *NewAcl, acl_size, ACL_REVISION );
    if (status != STATUS_SUCCESS)
    {
        ret = RtlNtStatusToDosError(status);
        goto exit;
    }

    for (i = 0; i < count; i++)
    {
        switch (pEntries[i].grfAccessMode)
        {
        case GRANT_ACCESS:
            status = RtlAddAccessAllowedAceEx(*NewAcl, ACL_REVISION,
                                              pEntries[i].grfInheritance,
                                              pEntries[i].grfAccessPermissions,
                                              ppsid[i]);
            break;
        case SET_ACCESS:
        {
            ULONG j;
            BOOL add = TRUE;
            if (OldAcl)
            {
                for (j = 0; ; j++)
                {
                    const ACE_HEADER *existing_ace_header;
                    status = RtlGetAce(OldAcl, j, (LPVOID *)&existing_ace_header);
                    if (status != STATUS_SUCCESS)
                        break;
                    if (pEntries[i].grfAccessMode == SET_ACCESS &&
                        existing_ace_header->AceType == ACCESS_ALLOWED_ACE_TYPE &&
                        EqualSid(ppsid[i], &((ACCESS_ALLOWED_ACE *)existing_ace_header)->SidStart))
                    {
                        add = FALSE;
                        break;
                    }
                }
            }
            if (add)
                status = RtlAddAccessAllowedAceEx(*NewAcl, ACL_REVISION,
                                                  pEntries[i].grfInheritance,
                                                  pEntries[i].grfAccessPermissions,
                                                  ppsid[i]);
            break;
        }
        case DENY_ACCESS:
            status = RtlAddAccessDeniedAceEx(*NewAcl, ACL_REVISION,
                                             pEntries[i].grfInheritance,
                                             pEntries[i].grfAccessPermissions,
                                             ppsid[i]);
            break;
        case SET_AUDIT_SUCCESS:
            status = RtlAddAuditAccessAceEx(*NewAcl, ACL_REVISION,
                                            pEntries[i].grfInheritance,
                                            pEntries[i].grfAccessPermissions,
                                            ppsid[i], TRUE, FALSE);
            break;
        case SET_AUDIT_FAILURE:
            status = RtlAddAuditAccessAceEx(*NewAcl, ACL_REVISION,
                                            pEntries[i].grfInheritance,
                                            pEntries[i].grfAccessPermissions,
                                            ppsid[i], FALSE, TRUE);
            break;
        default:
            FIXME("unhandled access mode %d\n", pEntries[i].grfAccessMode);
        }
    }

    if (OldAcl)
    {
        for (i = 0; ; i++)
        {
            BOOL add = TRUE;
            ULONG j;
            const ACE_HEADER *old_ace_header;
            status = RtlGetAce(OldAcl, i, (LPVOID *)&old_ace_header);
            if (status != STATUS_SUCCESS) break;
            for (j = 0; j < count; j++)
            {
                if (pEntries[j].grfAccessMode == SET_ACCESS &&
                    old_ace_header->AceType == ACCESS_ALLOWED_ACE_TYPE &&
                    EqualSid(ppsid[j], &((ACCESS_ALLOWED_ACE *)old_ace_header)->SidStart))
                {
                    status = RtlAddAccessAllowedAceEx(*NewAcl, ACL_REVISION, pEntries[j].grfInheritance, pEntries[j].grfAccessPermissions, ppsid[j]);
                    add = FALSE;
                    break;
                }
                else if (pEntries[j].grfAccessMode == REVOKE_ACCESS)
                {
                    switch (old_ace_header->AceType)
                    {
                    case ACCESS_ALLOWED_ACE_TYPE:
                        if (EqualSid(ppsid[j], &((ACCESS_ALLOWED_ACE *)old_ace_header)->SidStart))
                            add = FALSE;
                        break;
                    case ACCESS_DENIED_ACE_TYPE:
                        /* REVOKE_ACCESS does not affect ACCESS_DENIED_ACE. */
                        break;
                    case SYSTEM_AUDIT_ACE_TYPE:
                        if (EqualSid(ppsid[j], &((SYSTEM_AUDIT_ACE *)old_ace_header)->SidStart))
                            add = FALSE;
                        break;
                    case SYSTEM_ALARM_ACE_TYPE:
                        if (EqualSid(ppsid[j], &((SYSTEM_ALARM_ACE *)old_ace_header)->SidStart))
                            add = FALSE;
                        break;
                    default:
                        FIXME("unhandled ace type %d\n", old_ace_header->AceType);
                    }

                    if (!add)
                        break;
                }
            }
            if (add)
                status = RtlAddAce(*NewAcl, ACL_REVISION, 1, (PACE_HEADER)old_ace_header, old_ace_header->AceSize);
            if (status != STATUS_SUCCESS)
            {
                WARN("RtlAddAce failed with error 0x%08lx\n", status);
                ret = RtlNtStatusToDosError(status);
                break;
            }
        }
    }

exit:
    free(ppsid);
    return ret;
}

/******************************************************************************
 * SetNamedSecurityInfoA [ADVAPI32.@]
 */
DWORD WINAPI SetNamedSecurityInfoA(LPSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl)
{
    LPWSTR wstr;
    DWORD r;

    TRACE("%s %d %ld %p %p %p %p\n", debugstr_a(pObjectName), ObjectType,
           SecurityInfo, psidOwner, psidGroup, pDacl, pSacl);

    wstr = strdupAW(pObjectName);
    r = SetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, psidOwner,
                           psidGroup, pDacl, pSacl );

    free( wstr );

    return r;
}

/******************************************************************************
 * SetNamedSecurityInfoW [ADVAPI32.@]
 */
DWORD WINAPI SetNamedSecurityInfoW(LPWSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl)
{
    DWORD access = 0;
    HANDLE handle;
    DWORD err;

    TRACE( "%s %d %ld %p %p %p %p\n", debugstr_w(pObjectName), ObjectType,
           SecurityInfo, psidOwner, psidGroup, pDacl, pSacl);

    if (!pObjectName) return ERROR_INVALID_PARAMETER;

    if (SecurityInfo & (OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION))
        access |= WRITE_OWNER;
    if (SecurityInfo & DACL_SECURITY_INFORMATION)
        access |= WRITE_DAC;
    if (SecurityInfo & SACL_SECURITY_INFORMATION)
        access |= ACCESS_SYSTEM_SECURITY;

    switch (ObjectType)
    {
    case SE_SERVICE:
        if (!(err = get_security_service( pObjectName, access, &handle )))
        {
            err = SetSecurityInfo( handle, ObjectType, SecurityInfo, psidOwner, psidGroup, pDacl, pSacl );
            CloseServiceHandle( handle );
        }
        break;
    case SE_REGISTRY_KEY:
        if (!(err = get_security_regkey( pObjectName, access, &handle )))
        {
            err = SetSecurityInfo( handle, ObjectType, SecurityInfo, psidOwner, psidGroup, pDacl, pSacl );
            RegCloseKey( handle );
        }
        break;
    case SE_FILE_OBJECT:
        if (SecurityInfo & DACL_SECURITY_INFORMATION)
            access |= READ_CONTROL;
        if (!(err = get_security_file( pObjectName, access, &handle )))
        {
            err = SetSecurityInfo( handle, ObjectType, SecurityInfo, psidOwner, psidGroup, pDacl, pSacl );
            CloseHandle( handle );
        }
        break;
    default:
        FIXME( "Object type %d is not currently supported.\n", ObjectType );
        return ERROR_SUCCESS;
    }
    return err;
}

/******************************************************************************
 * GetExplicitEntriesFromAclA [ADVAPI32.@]
 */
DWORD WINAPI GetExplicitEntriesFromAclA( PACL pacl, PULONG pcCountOfExplicitEntries,
        PEXPLICIT_ACCESSA* pListOfExplicitEntries)
{
    FIXME("%p %p %p\n",pacl, pcCountOfExplicitEntries, pListOfExplicitEntries);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * GetExplicitEntriesFromAclW [ADVAPI32.@]
 */
DWORD WINAPI GetExplicitEntriesFromAclW( PACL pacl, PULONG count, PEXPLICIT_ACCESSW *list )
{
    ACL_SIZE_INFORMATION sizeinfo;
    EXPLICIT_ACCESSW *entries;
    MAX_SID *sid_entries;
    ACE_HEADER *ace;
    NTSTATUS status;
    int i;

    TRACE("%p %p %p\n",pacl, count, list);

    if (!count || !list)
        return ERROR_INVALID_PARAMETER;

    status = RtlQueryInformationAcl(pacl, &sizeinfo, sizeof(sizeinfo), AclSizeInformation);
    if (status) return RtlNtStatusToDosError(status);

    if (!sizeinfo.AceCount)
    {
        *count = 0;
        *list = NULL;
        return ERROR_SUCCESS;
    }

    entries = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (sizeof(EXPLICIT_ACCESSW) + sizeof(MAX_SID)) * sizeinfo.AceCount);
    if (!entries) return ERROR_OUTOFMEMORY;
    sid_entries = (MAX_SID *)(entries + sizeinfo.AceCount);

    for (i = 0; i < sizeinfo.AceCount; i++)
    {
        status = RtlGetAce(pacl, i, (void**)&ace);
        if (status) goto error;

        switch (ace->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            {
                ACCESS_ALLOWED_ACE *allow = (ACCESS_ALLOWED_ACE *)ace;
                entries[i].grfAccessMode = GRANT_ACCESS;
                entries[i].grfInheritance = ace->AceFlags;
                entries[i].grfAccessPermissions = allow->Mask;

                CopySid(sizeof(MAX_SID), (PSID)&sid_entries[i], (PSID)&allow->SidStart);
                entries[i].Trustee.pMultipleTrustee = NULL;
                entries[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                entries[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                entries[i].Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
                entries[i].Trustee.ptstrName = (WCHAR *)&sid_entries[i];
                break;
            }

            case ACCESS_DENIED_ACE_TYPE:
            {
                ACCESS_DENIED_ACE *deny = (ACCESS_DENIED_ACE *)ace;
                entries[i].grfAccessMode = DENY_ACCESS;
                entries[i].grfInheritance = ace->AceFlags;
                entries[i].grfAccessPermissions = deny->Mask;

                CopySid(sizeof(MAX_SID), (PSID)&sid_entries[i], (PSID)&deny->SidStart);
                entries[i].Trustee.pMultipleTrustee = NULL;
                entries[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                entries[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                entries[i].Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
                entries[i].Trustee.ptstrName = (WCHAR *)&sid_entries[i];
                break;
            }

            default:
                FIXME("Unhandled ace type %d\n", ace->AceType);
                entries[i].grfAccessMode = NOT_USED_ACCESS;
                continue;
        }
    }

    *count = sizeinfo.AceCount;
    *list = entries;
    return ERROR_SUCCESS;

error:
    LocalFree(entries);
    return RtlNtStatusToDosError(status);
}

/******************************************************************************
 * GetAuditedPermissionsFromAclA [ADVAPI32.@]
 */
DWORD WINAPI GetAuditedPermissionsFromAclA( PACL pacl, PTRUSTEEA pTrustee, PACCESS_MASK pSuccessfulAuditedRights,
        PACCESS_MASK pFailedAuditRights)
{
    FIXME("%p %p %p %p\n",pacl, pTrustee, pSuccessfulAuditedRights, pFailedAuditRights);
    return ERROR_CALL_NOT_IMPLEMENTED;

}

/******************************************************************************
 * GetAuditedPermissionsFromAclW [ADVAPI32.@]
 */
DWORD WINAPI GetAuditedPermissionsFromAclW( PACL pacl, PTRUSTEEW pTrustee, PACCESS_MASK pSuccessfulAuditedRights,
        PACCESS_MASK pFailedAuditRights)
{
    FIXME("%p %p %p %p\n",pacl, pTrustee, pSuccessfulAuditedRights, pFailedAuditRights);
    return ERROR_CALL_NOT_IMPLEMENTED;

}

/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorA(
        LPCSTR StringSecurityDescriptor,
        DWORD StringSDRevision,
        PSECURITY_DESCRIPTOR* SecurityDescriptor,
        PULONG SecurityDescriptorSize)
{
    BOOL ret;
    LPWSTR StringSecurityDescriptorW;

    TRACE("%s, %lu, %p, %p\n", debugstr_a(StringSecurityDescriptor), StringSDRevision,
          SecurityDescriptor, SecurityDescriptorSize);

    if(!StringSecurityDescriptor)
        return FALSE;

    StringSecurityDescriptorW = strdupAW(StringSecurityDescriptor);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSecurityDescriptorW,
                                                               StringSDRevision, SecurityDescriptor,
                                                               SecurityDescriptorSize);
    free(StringSecurityDescriptorW);

    return ret;
}

/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorA [ADVAPI32.@]
 */
BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorA(PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD SDRevision, SECURITY_INFORMATION Information, LPSTR *OutputString, PULONG OutputLen)
{
    LPWSTR wstr;
    ULONG len;
    if (ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityDescriptor, SDRevision, Information, &wstr, &len))
    {
        int lenA;

        lenA = WideCharToMultiByte(CP_ACP, 0, wstr, len, NULL, 0, NULL, NULL);
        *OutputString = malloc(lenA);
        WideCharToMultiByte(CP_ACP, 0, wstr, len, *OutputString, lenA, NULL, NULL);
        LocalFree(wstr);

        if (OutputLen != NULL)
            *OutputLen = lenA;
        return TRUE;
    }
    else
    {
        *OutputString = NULL;
        if (OutputLen)
            *OutputLen = 0;
        return FALSE;
    }
}

/******************************************************************************
 * ConvertStringSidToSidA [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSidToSidA(LPCSTR StringSid, PSID* Sid)
{
    BOOL bret = FALSE;

    TRACE("%s, %p\n", debugstr_a(StringSid), Sid);
    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (!StringSid || !Sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        WCHAR *wStringSid = strdupAW(StringSid);
        bret = ConvertStringSidToSidW(wStringSid, Sid);
        free(wStringSid);
    }
    return bret;
}

/******************************************************************************
 * ConvertSidToStringSidA [ADVAPI32.@]
 */
BOOL WINAPI ConvertSidToStringSidA(PSID pSid, LPSTR *pstr)
{
    LPWSTR wstr = NULL;
    LPSTR str;
    UINT len;

    TRACE("%p %p\n", pSid, pstr );

    if( !ConvertSidToStringSidW( pSid, &wstr ) )
        return FALSE;

    len = WideCharToMultiByte( CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL );
    str = LocalAlloc( 0, len );
    WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, len, NULL, NULL );
    LocalFree( wstr );

    *pstr = str;

    return TRUE;
}

/******************************************************************************
 * CreateProcessWithLogonW [ADVAPI32.@]
 */
BOOL WINAPI CreateProcessWithLogonW( LPCWSTR user_name, LPCWSTR domain, LPCWSTR password,
                                     DWORD logon_flags, LPCWSTR application_name, LPWSTR command_line,
                                     DWORD creation_flags, void *environment, LPCWSTR current_directory,
                                     STARTUPINFOW *startup_info, PROCESS_INFORMATION *process_information )
{
    HANDLE token;

    FIXME("%s %s %p 0x%08lx %s %s 0x%08lx %p %s %p %p: semi-stub\n", debugstr_w(user_name), debugstr_w(domain),
          password, logon_flags, debugstr_w(application_name), debugstr_w(command_line), creation_flags,
          environment, debugstr_w(current_directory), startup_info, process_information);

    if (LogonUserW(user_name, domain, password, 0, 0, &token))
    {
        BOOL ret = CreateProcessAsUserW( token, application_name, command_line, NULL, NULL, FALSE, creation_flags,
                                         environment, current_directory, startup_info, process_information );
        CloseHandle(token);
        return ret;
    }

    return FALSE;
}

/******************************************************************************
 * CreateProcessWithTokenW [ADVAPI32.@]
 */
BOOL WINAPI CreateProcessWithTokenW(HANDLE token, DWORD logon_flags, LPCWSTR application_name, LPWSTR command_line,
        DWORD creation_flags, void *environment, LPCWSTR current_directory, STARTUPINFOW *startup_info,
        PROCESS_INFORMATION *process_information )
{
    FIXME("%p 0x%08lx %s %s 0x%08lx %p %s %p %p - semi-stub\n", token,
          logon_flags, debugstr_w(application_name), debugstr_w(command_line),
          creation_flags, environment, debugstr_w(current_directory),
          startup_info, process_information);

    /* FIXME: check if handles should be inherited */
    return CreateProcessAsUserW( token, application_name, command_line, NULL, NULL, FALSE, creation_flags,
                                 environment, current_directory, startup_info, process_information );
}

/******************************************************************************
 * GetNamedSecurityInfoA [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoA(const char *pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID* ppsidOwner, PSID* ppsidGroup, PACL* ppDacl, PACL* ppSacl,
        PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    LPWSTR wstr;
    DWORD r;

    TRACE("%s %d %ld %p %p %p %p %p\n", debugstr_a(pObjectName), ObjectType, SecurityInfo,
        ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor);

    wstr = strdupAW(pObjectName);
    r = GetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, ppsidOwner,
                           ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor );

    free( wstr );

    return r;
}

/******************************************************************************
 * GetNamedSecurityInfoW [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoW( const WCHAR *name, SE_OBJECT_TYPE type,
    SECURITY_INFORMATION info, PSID* owner, PSID* group, PACL* dacl,
    PACL* sacl, PSECURITY_DESCRIPTOR* descriptor )
{
    DWORD access = 0;
    HANDLE handle;
    DWORD err;

    TRACE( "%s %d %ld %p %p %p %p %p\n", debugstr_w(name), type, info, owner,
           group, dacl, sacl, descriptor );

    /* A NULL descriptor is allowed if any one of the other pointers is not NULL */
    if (!name || !(owner||group||dacl||sacl||descriptor) ) return ERROR_INVALID_PARAMETER;

    /* If no descriptor, we have to check that there's a pointer for the requested information */
    if( !descriptor && (
        ((info & OWNER_SECURITY_INFORMATION) && !owner)
    ||  ((info & GROUP_SECURITY_INFORMATION) && !group)
    ||  ((info & DACL_SECURITY_INFORMATION)  && !dacl)
    ||  ((info & SACL_SECURITY_INFORMATION)  && !sacl)  ))
        return ERROR_INVALID_PARAMETER;

    if (info & (OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION))
        access |= READ_CONTROL;
    if (info & SACL_SECURITY_INFORMATION)
        access |= ACCESS_SYSTEM_SECURITY;

    switch (type)
    {
    case SE_SERVICE:
        if (!(err = get_security_service( name, access, &handle )))
        {
            err = GetSecurityInfo( handle, type, info, owner, group, dacl, sacl, descriptor );
            CloseServiceHandle( handle );
        }
        break;
    case SE_REGISTRY_KEY:
        if (!(err = get_security_regkey( name, access, &handle )))
        {
            err = GetSecurityInfo( handle, type, info, owner, group, dacl, sacl, descriptor );
            RegCloseKey( handle );
        }
        break;
    case SE_FILE_OBJECT:
        if (!(err = get_security_file( name, access, &handle )))
        {
            err = GetSecurityInfo( handle, type, info, owner, group, dacl, sacl, descriptor );
            CloseHandle( handle );
        }
        break;
    default:
        FIXME( "Object type %d is not currently supported.\n", type );
        if (owner) *owner = NULL;
        if (group) *group = NULL;
        if (dacl) *dacl = NULL;
        if (sacl) *sacl = NULL;
        if (descriptor) *descriptor = NULL;
        return ERROR_SUCCESS;
    }
    return err;
}

/******************************************************************************
 * GetNamedSecurityInfoExW [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoExW( LPCWSTR object, SE_OBJECT_TYPE type,
    SECURITY_INFORMATION info, LPCWSTR provider, LPCWSTR property,
    PACTRL_ACCESSW* access_list, PACTRL_AUDITW* audit_list, LPWSTR* owner, LPWSTR* group )
{
    FIXME("(%s, %d, %ld, %s, %s, %p, %p, %p, %p) stub\n", debugstr_w(object), type, info,
        debugstr_w(provider), debugstr_w(property), access_list, audit_list, owner, group);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * GetNamedSecurityInfoExA [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoExA( LPCSTR object, SE_OBJECT_TYPE type,
    SECURITY_INFORMATION info, LPCSTR provider, LPCSTR property,
    PACTRL_ACCESSA* access_list, PACTRL_AUDITA* audit_list, LPSTR* owner, LPSTR* group )
{
    FIXME("(%s, %d, %ld, %s, %s, %p, %p, %p, %p) stub\n", debugstr_a(object), type, info,
        debugstr_a(provider), debugstr_a(property), access_list, audit_list, owner, group);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * DecryptFileW [ADVAPI32.@]
 */
BOOL WINAPI DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved)
{
    FIXME("(%s, %08lx): stub\n", debugstr_w(lpFileName), dwReserved);
    return TRUE;
}

/******************************************************************************
 * DecryptFileA [ADVAPI32.@]
 */
BOOL WINAPI DecryptFileA(LPCSTR lpFileName, DWORD dwReserved)
{
    FIXME("(%s, %08lx): stub\n", debugstr_a(lpFileName), dwReserved);
    return TRUE;
}

/******************************************************************************
 * EncryptFileW [ADVAPI32.@]
 */
BOOL WINAPI EncryptFileW(LPCWSTR lpFileName)
{
    FIXME("(%s): stub\n", debugstr_w(lpFileName));
    return TRUE;
}

/******************************************************************************
 * EncryptFileA [ADVAPI32.@]
 */
BOOL WINAPI EncryptFileA(LPCSTR lpFileName)
{
    FIXME("(%s): stub\n", debugstr_a(lpFileName));
    return TRUE;
}

/******************************************************************************
 * FileEncryptionStatusW [ADVAPI32.@]
 */
BOOL WINAPI FileEncryptionStatusW(LPCWSTR lpFileName, LPDWORD lpStatus)
{
    FIXME("(%s %p): stub\n", debugstr_w(lpFileName), lpStatus);
    if (!lpStatus)
        return FALSE;
    *lpStatus = FILE_SYSTEM_NOT_SUPPORT;
    return TRUE;
}

/******************************************************************************
 * FileEncryptionStatusA [ADVAPI32.@]
 */
BOOL WINAPI FileEncryptionStatusA(LPCSTR lpFileName, LPDWORD lpStatus)
{
    FIXME("(%s %p): stub\n", debugstr_a(lpFileName), lpStatus);
    if (!lpStatus)
        return FALSE;
    *lpStatus = FILE_SYSTEM_NOT_SUPPORT;
    return TRUE;
}

static NTSTATUS combine_dacls(ACL *parent, ACL *child, ACL **result)
{
    NTSTATUS status;
    ACL *combined;
    int i;

    /* initialize a combined DACL containing both inherited and new ACEs */
    combined = calloc(1, child->AclSize + parent->AclSize);
    if (!combined)
        return STATUS_NO_MEMORY;

    status = RtlCreateAcl(combined, parent->AclSize+child->AclSize, ACL_REVISION);
    if (status != STATUS_SUCCESS)
    {
        free(combined);
        return status;
    }

    /* copy the new ACEs */
    for (i=0; i<child->AceCount; i++)
    {
        ACE_HEADER *ace;

        if (!GetAce(child, i, (void*)&ace))
            continue;
        if (!AddAce(combined, ACL_REVISION, MAXDWORD, ace, ace->AceSize))
            WARN("error adding new ACE\n");
    }

    /* copy the inherited ACEs */
    for (i=0; i<parent->AceCount; i++)
    {
        ACE_HEADER *ace;

        if (!GetAce(parent, i, (void*)&ace))
            continue;
        if (!(ace->AceFlags & (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE)))
            continue;
        if ((ace->AceFlags & (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE)) !=
                (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE))
        {
            FIXME("unsupported flags: %x\n", ace->AceFlags);
            continue;
        }

        if (ace->AceFlags & NO_PROPAGATE_INHERIT_ACE)
            ace->AceFlags &= ~(OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE|NO_PROPAGATE_INHERIT_ACE);
        ace->AceFlags &= ~INHERIT_ONLY_ACE;
        ace->AceFlags |= INHERITED_ACE;

        if (!AddAce(combined, ACL_REVISION, MAXDWORD, ace, ace->AceSize))
            WARN("error adding inherited ACE\n");
    }

    *result = combined;
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SetSecurityInfo [ADVAPI32.@]
 */
DWORD WINAPI SetSecurityInfo(HANDLE handle, SE_OBJECT_TYPE ObjectType, 
                      SECURITY_INFORMATION SecurityInfo, PSID psidOwner,
                      PSID psidGroup, PACL pDacl, PACL pSacl)
{
    SECURITY_DESCRIPTOR sd;
    PACL dacl = pDacl;
    NTSTATUS status;

    if (!handle)
        return ERROR_INVALID_HANDLE;

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        return ERROR_INVALID_SECURITY_DESCR;

    if (SecurityInfo & OWNER_SECURITY_INFORMATION)
        SetSecurityDescriptorOwner(&sd, psidOwner, FALSE);
    if (SecurityInfo & GROUP_SECURITY_INFORMATION)
        SetSecurityDescriptorGroup(&sd, psidGroup, FALSE);
    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        if (ObjectType == SE_FILE_OBJECT && pDacl)
        {
            SECURITY_DESCRIPTOR_CONTROL control;
            PSECURITY_DESCRIPTOR psd;
            OBJECT_NAME_INFORMATION *name_info;
            DWORD size, rev;

            status = NtQuerySecurityObject(handle, SecurityInfo, NULL, 0, &size);
            if (status != STATUS_BUFFER_TOO_SMALL)
                return RtlNtStatusToDosError(status);

            psd = malloc(size);
            if (!psd)
                return ERROR_NOT_ENOUGH_MEMORY;

            status = NtQuerySecurityObject(handle, SecurityInfo, psd, size, &size);
            if (status)
            {
                free(psd);
                return RtlNtStatusToDosError(status);
            }

            status = RtlGetControlSecurityDescriptor(psd, &control, &rev);
            free(psd);
            if (status)
                return RtlNtStatusToDosError(status);
            /* TODO: copy some control flags to new sd */

            /* inherit parent directory DACL */
            if (!(control & SE_DACL_PROTECTED))
            {
                status = NtQueryObject(handle, ObjectNameInformation, NULL, 0, &size);
                if (status != STATUS_INFO_LENGTH_MISMATCH)
                    return RtlNtStatusToDosError(status);

                name_info = malloc(size);
                if (!name_info)
                    return ERROR_NOT_ENOUGH_MEMORY;

                status = NtQueryObject(handle, ObjectNameInformation, name_info, size, NULL);
                if (status)
                {
                    free(name_info);
                    return RtlNtStatusToDosError(status);
                }

                if (name_info->Name.Length && name_info->Name.Buffer[(name_info->Name.Length / 2) - 1] == '\\')
                    name_info->Name.Length -= 2;
                while (name_info->Name.Length && name_info->Name.Buffer[(name_info->Name.Length / 2) - 1] != '\\')
                    name_info->Name.Length -= 2;

                if (name_info->Name.Length)
                {
                    OBJECT_ATTRIBUTES attr;
                    IO_STATUS_BLOCK io;
                    HANDLE parent;
                    PSECURITY_DESCRIPTOR parent_sd;
                    ACL *parent_dacl;
                    DWORD err = ERROR_ACCESS_DENIED;

                    name_info->Name.Buffer[name_info->Name.Length/2] = 0;

                    attr.Length = sizeof(attr);
                    attr.RootDirectory = 0;
                    attr.Attributes = 0;
                    attr.ObjectName = &name_info->Name;
                    attr.SecurityDescriptor = NULL;
                    status = NtOpenFile(&parent, READ_CONTROL|SYNCHRONIZE, &attr, &io,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            FILE_OPEN_FOR_BACKUP_INTENT);
                    free(name_info);
                    if (!status)
                    {
                        err = GetSecurityInfo(parent, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                NULL, NULL, &parent_dacl, NULL, &parent_sd);
                        CloseHandle(parent);
                    }

                    if (!err)
                    {
                        status = combine_dacls(parent_dacl, pDacl, &dacl);
                        LocalFree(parent_sd);
                        if (status != STATUS_SUCCESS)
                            return RtlNtStatusToDosError(status);
                    }
                }
                else
                    free(name_info);
            }
        }

        SetSecurityDescriptorDacl(&sd, TRUE, dacl, FALSE);
    }
    if (SecurityInfo & SACL_SECURITY_INFORMATION)
        SetSecurityDescriptorSacl(&sd, TRUE, pSacl, FALSE);

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_KERNEL_OBJECT:
    case SE_WMIGUID_OBJECT:
    case SE_REGISTRY_KEY:
        status = NtSetSecurityObject(handle, SecurityInfo, &sd);
        break;

    default:
        FIXME("unimplemented type %u, returning success\n", ObjectType);
        status = STATUS_SUCCESS;
        break;

    }
    if (dacl != pDacl)
        free(dacl);
    return RtlNtStatusToDosError(status);
}

/******************************************************************************
 * SaferCreateLevel   [ADVAPI32.@]
 */
BOOL WINAPI SaferCreateLevel(DWORD ScopeId, DWORD LevelId, DWORD OpenFlags,
                             SAFER_LEVEL_HANDLE* LevelHandle, LPVOID lpReserved)
{
    FIXME("(%lu, %lx, %lu, %p, %p) stub\n", ScopeId, LevelId, OpenFlags, LevelHandle, lpReserved);

    *LevelHandle = (SAFER_LEVEL_HANDLE)0xdeadbeef;
    return TRUE;
}

/******************************************************************************
 * SaferComputeTokenFromLevel   [ADVAPI32.@]
 */
BOOL WINAPI SaferComputeTokenFromLevel(SAFER_LEVEL_HANDLE handle, HANDLE token, PHANDLE access_token,
                                       DWORD flags, LPVOID reserved)
{
    FIXME("(%p, %p, %p, %lx, %p) stub\n", handle, token, access_token, flags, reserved);

    *access_token = (flags & SAFER_TOKEN_NULL_IF_EQUAL) ? NULL : (HANDLE)0xdeadbeef;
    return TRUE;
}

/******************************************************************************
 * SaferCloseLevel   [ADVAPI32.@]
 */
BOOL WINAPI SaferCloseLevel(SAFER_LEVEL_HANDLE handle)
{
    FIXME("(%p) stub\n", handle);
    return TRUE;
}

/******************************************************************************
 * TreeSetNamedSecurityInfoW   [ADVAPI32.@]
 */
DWORD WINAPI TreeSetNamedSecurityInfoW(WCHAR *name, SE_OBJECT_TYPE type, SECURITY_INFORMATION info,
                                       SID *owner, SID *group, ACL *dacl, ACL *sacl, DWORD action,
                                       FN_PROGRESS progress, PROG_INVOKE_SETTING pis, void *args)
{
    FIXME("(%s, %d, %lu, %p, %p, %p, %p, %lu, %p, %d, %p) stub\n",
          debugstr_w(name), type, info, owner, group, dacl, sacl, action, progress, pis, args);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * TreeResetNamedSecurityInfoW   [ADVAPI32.@]
 */
DWORD WINAPI TreeResetNamedSecurityInfoW( LPWSTR pObjectName,
                SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
                PSID pOwner, PSID pGroup, PACL pDacl, PACL pSacl,
                BOOL KeepExplicit, FN_PROGRESS fnProgress,
                PROG_INVOKE_SETTING ProgressInvokeSetting, PVOID Args)
{
    FIXME("(%s, %i, %li, %p, %p, %p, %p, %i, %p, %i, %p) stub\n",
        debugstr_w(pObjectName), ObjectType, SecurityInfo, pOwner, pGroup,
        pDacl, pSacl, KeepExplicit, fnProgress, ProgressInvokeSetting, Args);

    return ERROR_SUCCESS;
}

/******************************************************************************
 * SaferGetPolicyInformation   [ADVAPI32.@]
 */
BOOL WINAPI SaferGetPolicyInformation(DWORD scope, SAFER_POLICY_INFO_CLASS class, DWORD size,
                                      PVOID buffer, PDWORD required, LPVOID lpReserved)
{
    FIXME("(%lu %u %lu %p %p %p) stub\n", scope, class, size, buffer, required, lpReserved);
    return FALSE;
}

/******************************************************************************
 * SaferIdentifyLevel   [ADVAPI32.@]
 */
BOOL WINAPI SaferIdentifyLevel(DWORD count, SAFER_CODE_PROPERTIES *properties, SAFER_LEVEL_HANDLE *handle,
                               void *reserved)
{
    FIXME("(%lu %p %p %p) stub\n", count, properties, handle, reserved);
    *handle = (SAFER_LEVEL_HANDLE)0xdeadbeef;
    return TRUE;
}

/******************************************************************************
 * SaferSetLevelInformation   [ADVAPI32.@]
 */
BOOL WINAPI SaferSetLevelInformation(SAFER_LEVEL_HANDLE handle, SAFER_OBJECT_INFO_CLASS infotype,
                                     LPVOID buffer, DWORD size)
{
    FIXME("(%p %u %p %lu) stub\n", handle, infotype, buffer, size);
    return FALSE;
}

/******************************************************************************
 * LookupSecurityDescriptorPartsA   [ADVAPI32.@]
 */
DWORD WINAPI LookupSecurityDescriptorPartsA(TRUSTEEA *owner, TRUSTEEA *group, ULONG *access_count,
                                            EXPLICIT_ACCESSA *access_list, ULONG *audit_count,
                                            EXPLICIT_ACCESSA *audit_list, SECURITY_DESCRIPTOR *descriptor)
{
    FIXME("(%p %p %p %p %p %p %p) stub\n", owner, group, access_count,
          access_list, audit_count, audit_list, descriptor);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * LookupSecurityDescriptorPartsW   [ADVAPI32.@]
 */
DWORD WINAPI LookupSecurityDescriptorPartsW(TRUSTEEW *owner, TRUSTEEW *group, ULONG *access_count,
                                            EXPLICIT_ACCESSW *access_list, ULONG *audit_count,
                                            EXPLICIT_ACCESSW *audit_list, SECURITY_DESCRIPTOR *descriptor)
{
    FIXME("(%p %p %p %p %p %p %p) stub\n", owner, group, access_count,
          access_list, audit_count, audit_list, descriptor);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
