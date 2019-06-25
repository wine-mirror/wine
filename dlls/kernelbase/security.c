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
#include "winternl.h"
#include "winioctl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(security);


/******************************************************************************
 * SID functions
 ******************************************************************************/

typedef struct _MAX_SID
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
} MAX_SID;

typedef struct WELLKNOWNSID
{
    WELL_KNOWN_SID_TYPE Type;
    MAX_SID Sid;
} WELLKNOWNSID;

static const WELLKNOWNSID WellKnownSids[] =
{
    { WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { WinCreatorOwnerRightsSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RIGHTS_RID } } },
    { WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { WinLogonIdsSid, { SID_REVISION, SECURITY_LOGON_IDS_RID_COUNT, { SECURITY_NT_AUTHORITY }, { SECURITY_LOGON_IDS_RID } } },
    { WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
    { WinBuiltinAnyPackageSid, { SID_REVISION, 2, { SECURITY_APP_PACKAGE_AUTHORITY }, { SECURITY_APP_PACKAGE_BASE_RID, SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE } } },
};

/* these SIDs must be constructed as relative to some domain - only the RID is well-known */
typedef struct WELLKNOWNRID
{
    WELL_KNOWN_SID_TYPE Type;
    DWORD Rid;
} WELLKNOWNRID;

static const WELLKNOWNRID WellKnownRids[] =
{
    { WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};


static const char *debugstr_sid( PSID sid )
{
    int auth;
    SID * psid = sid;

    if (psid == NULL) return "(null)";

    auth = psid->IdentifierAuthority.Value[5] +
        (psid->IdentifierAuthority.Value[4] << 8) +
        (psid->IdentifierAuthority.Value[3] << 16) +
        (psid->IdentifierAuthority.Value[2] << 24);

    switch (psid->SubAuthorityCount) {
    case 0:
        return wine_dbg_sprintf("S-%d-%d", psid->Revision, auth);
    case 1:
        return wine_dbg_sprintf("S-%d-%d-%u", psid->Revision, auth,
                                psid->SubAuthority[0]);
    case 2:
        return wine_dbg_sprintf("S-%d-%d-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1]);
    case 3:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2]);
    case 4:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
                                psid->SubAuthority[3]);
    case 5:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
                                psid->SubAuthority[3], psid->SubAuthority[4]);
    case 6:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[3], psid->SubAuthority[1], psid->SubAuthority[2],
                                psid->SubAuthority[0], psid->SubAuthority[4], psid->SubAuthority[5]);
    case 7:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u-%u-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
                                psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
                                psid->SubAuthority[6]);
    case 8:
        return wine_dbg_sprintf("S-%d-%d-%u-%u-%u-%u-%u-%u-%u-%u", psid->Revision, auth,
                                psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
                                psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
                                psid->SubAuthority[6], psid->SubAuthority[7]);
    }
    return "(too-big)";
}

static BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/******************************************************************************
 * AllocateAndInitializeSid   (kernelbase.@)
 */
BOOL WINAPI AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY auth, BYTE count,
                                      DWORD auth0, DWORD auth1, DWORD auth2, DWORD auth3,
                                      DWORD auth4, DWORD auth5, DWORD auth6, DWORD auth7, PSID *sid )
{
    return set_ntstatus( RtlAllocateAndInitializeSid( auth, count, auth0, auth1, auth2, auth3,
                                                      auth4, auth5, auth6, auth7, sid ));
}

/***********************************************************************
 * AllocateLocallyUniqueId   (kernelbase.@)
 */
BOOL WINAPI AllocateLocallyUniqueId( PLUID luid )
{
    return set_ntstatus( NtAllocateLocallyUniqueId( luid ));
}

/******************************************************************************
 * CopySid   (kernelbase.@)
 */
BOOL WINAPI CopySid( DWORD len, PSID dest, PSID source )
{
    return RtlCopySid( len, dest, source );
}

/******************************************************************************
 * EqualPrefixSid   (kernelbase.@)
 */
BOOL WINAPI EqualPrefixSid( PSID sid1, PSID sid2 )
{
    return RtlEqualPrefixSid( sid1, sid2 );
}

/******************************************************************************
 * EqualSid   (kernelbase.@)
 */
BOOL WINAPI EqualSid( PSID sid1, PSID sid2 )
{
    BOOL ret = RtlEqualSid( sid1, sid2 );
    SetLastError(ERROR_SUCCESS);
    return ret;
}

/******************************************************************************
 * FreeSid   (kernelbase.@)
 */
void * WINAPI FreeSid( PSID pSid )
{
    RtlFreeSid(pSid);
    return NULL; /* is documented like this */
}

/******************************************************************************
 * GetLengthSid   (kernelbase.@)
 */
DWORD WINAPI GetLengthSid( PSID sid )
{
    return RtlLengthSid( sid );
}

/******************************************************************************
 * GetSidIdentifierAuthority   (kernelbase.@)
 */
PSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority( PSID sid )
{
    SetLastError(ERROR_SUCCESS);
    return RtlIdentifierAuthoritySid( sid );
}

/******************************************************************************
 * GetSidLengthRequired   (kernelbase.@)
 */
DWORD WINAPI GetSidLengthRequired( BYTE count )
{
    return RtlLengthRequiredSid( count );
}

/******************************************************************************
 * GetSidSubAuthority   (kernelbase.@)
 */
PDWORD WINAPI GetSidSubAuthority( PSID sid, DWORD auth )
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthoritySid( sid, auth );
}

/******************************************************************************
 * GetSidSubAuthorityCount   (kernelbase.@)
 */
PUCHAR WINAPI GetSidSubAuthorityCount( PSID sid )
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthorityCountSid( sid );
}

/******************************************************************************
 * GetWindowsAccountDomainSid    (kernelbase.@)
 */
BOOL WINAPI GetWindowsAccountDomainSid( PSID sid, PSID domain_sid, DWORD *size )
{
    SID_IDENTIFIER_AUTHORITY domain_ident = { SECURITY_NT_AUTHORITY };
    DWORD required_size;
    int i;

    FIXME( "(%p %p %p): semi-stub\n", sid, domain_sid, size );

    if (!sid || !IsValidSid( sid ))
    {
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }

    if (!size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (*GetSidSubAuthorityCount( sid ) < 4)
    {
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }

    required_size = GetSidLengthRequired( 4 );
    if (*size < required_size || !domain_sid)
    {
        *size = required_size;
        SetLastError( domain_sid ? ERROR_INSUFFICIENT_BUFFER : ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    InitializeSid( domain_sid, &domain_ident, 4 );
    for (i = 0; i < 4; i++)
        *GetSidSubAuthority( domain_sid, i ) = *GetSidSubAuthority( sid, i );

    *size = required_size;
    return TRUE;
}

/******************************************************************************
 * InitializeSid   (kernelbase.@)
 */
BOOL WINAPI InitializeSid ( PSID sid, PSID_IDENTIFIER_AUTHORITY auth, BYTE count )
{
    return RtlInitializeSid( sid, auth, count );
}

/******************************************************************************
 * IsValidSid   (kernelbase.@)
 */
BOOL WINAPI IsValidSid( PSID sid )
{
    return RtlValidSid( sid );
}

/******************************************************************************
 * CreateWellKnownSid   (kernelbase.@)
 */
BOOL WINAPI CreateWellKnownSid( WELL_KNOWN_SID_TYPE type, PSID domain, PSID sid, DWORD *size )
{
    unsigned int i;

    TRACE("(%d, %s, %p, %p)\n", type, debugstr_sid(domain), sid, size);

    if (size == NULL || (domain && !IsValidSid(domain)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
    {
        if (WellKnownSids[i].Type == type)
        {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*size < length)
            {
                *size = length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, &WellKnownSids[i].Sid.Revision, length);
            *size = length;
            return TRUE;
        }
    }

    if (domain == NULL || *GetSidSubAuthorityCount(domain) == SID_MAX_SUB_AUTHORITIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownRids); i++)
    {
        if (WellKnownRids[i].Type == type)
        {
            UCHAR domain_subauth = *GetSidSubAuthorityCount(domain);
            DWORD domain_sid_length = GetSidLengthRequired(domain_subauth);
            DWORD output_sid_length = GetSidLengthRequired(domain_subauth + 1);

            if (*size < output_sid_length)
            {
                *size = output_sid_length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, domain, domain_sid_length);
            (*GetSidSubAuthorityCount(sid))++;
            (*GetSidSubAuthority(sid, domain_subauth)) = WellKnownRids[i].Rid;
            *size = output_sid_length;
            return TRUE;
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/******************************************************************************
 * IsWellKnownSid   (kernelbase.@)
 */
BOOL WINAPI IsWellKnownSid( PSID sid, WELL_KNOWN_SID_TYPE type )
{
    unsigned int i;

    TRACE("(%s, %d)\n", debugstr_sid(sid), type);

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
        if (WellKnownSids[i].Type == type)
            if (EqualSid(sid, (PSID)&WellKnownSids[i].Sid.Revision))
                return TRUE;

    return FALSE;
}


/******************************************************************************
 * Token functions
 ******************************************************************************/


/******************************************************************************
 * AdjustTokenGroups    (kernelbase.@)
 */
BOOL WINAPI AdjustTokenGroups( HANDLE token, BOOL reset, PTOKEN_GROUPS new,
                               DWORD len, PTOKEN_GROUPS prev, PDWORD ret_len )
{
    return set_ntstatus( NtAdjustGroupsToken( token, reset, new, len, prev, ret_len ));
}

/******************************************************************************
 * AdjustTokenPrivileges    (kernelbase.@)
 */
BOOL WINAPI AdjustTokenPrivileges( HANDLE token, BOOL disable, PTOKEN_PRIVILEGES new, DWORD len,
                                   PTOKEN_PRIVILEGES prev, PDWORD ret_len )
{
    NTSTATUS status;

    TRACE("(%p %d %p %d %p %p)\n", token, disable, new, len, prev, ret_len );

    status = NtAdjustPrivilegesToken( token, disable, new, len, prev, ret_len );
    SetLastError( RtlNtStatusToDosError( status ));
    return (status == STATUS_SUCCESS) || (status == STATUS_NOT_ALL_ASSIGNED);
}

/******************************************************************************
 * CheckTokenMembership    (kernelbase.@)
 */
BOOL WINAPI CheckTokenMembership( HANDLE token, PSID sid_to_check, PBOOL is_member )
{
    PTOKEN_GROUPS token_groups = NULL;
    HANDLE thread_token = NULL;
    DWORD size, i;
    BOOL ret;

    TRACE("(%p %s %p)\n", token, debugstr_sid(sid_to_check), is_member);

    *is_member = FALSE;

    if (!token)
    {
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &thread_token))
        {
            HANDLE process_token;
            ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &process_token);
            if (!ret)
                goto exit;
            ret = DuplicateTokenEx(process_token, TOKEN_QUERY, NULL, SecurityImpersonation,
                                   TokenImpersonation, &thread_token);
            CloseHandle(process_token);
            if (!ret)
                goto exit;
        }
        token = thread_token;
    }
    else
    {
        TOKEN_TYPE type;

        ret = GetTokenInformation(token, TokenType, &type, sizeof(TOKEN_TYPE), &size);
        if (!ret) goto exit;

        if (type == TokenPrimary)
        {
            SetLastError(ERROR_NO_IMPERSONATION_TOKEN);
            return FALSE;
        }
    }

    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto exit;

    token_groups = heap_alloc(size);
    if (!token_groups)
    {
        ret = FALSE;
        goto exit;
    }

    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    if (!ret)
        goto exit;

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        TRACE("Groups[%d]: {0x%x, %s}\n", i,
            token_groups->Groups[i].Attributes,
            debugstr_sid(token_groups->Groups[i].Sid));
        if ((token_groups->Groups[i].Attributes & SE_GROUP_ENABLED) &&
            EqualSid(sid_to_check, token_groups->Groups[i].Sid))
        {
            *is_member = TRUE;
            TRACE("sid enabled and found in token\n");
            break;
        }
    }

exit:
    heap_free(token_groups);
    if (thread_token != NULL) CloseHandle(thread_token);
    return ret;
}

/*************************************************************************
 * CreateRestrictedToken    (kernelbase.@)
 */
BOOL WINAPI CreateRestrictedToken( HANDLE token, DWORD flags,
                                   DWORD disable_count, PSID_AND_ATTRIBUTES disable_sids,
                                   DWORD delete_count, PLUID_AND_ATTRIBUTES delete_privs,
                                   DWORD restrict_count, PSID_AND_ATTRIBUTES restrict_sids, PHANDLE ret )
{
    TOKEN_TYPE type;
    SECURITY_IMPERSONATION_LEVEL level = SecurityAnonymous;
    DWORD size;

    FIXME("(%p, 0x%x, %u, %p, %u, %p, %u, %p, %p): stub\n",
          token, flags, disable_count, disable_sids, delete_count, delete_privs,
          restrict_count, restrict_sids, ret );

    size = sizeof(type);
    if (!GetTokenInformation( token, TokenType, &type, size, &size )) return FALSE;
    if (type == TokenImpersonation)
    {
        size = sizeof(level);
        if (!GetTokenInformation( token, TokenImpersonationLevel, &level, size, &size ))
            return FALSE;
    }
    return DuplicateTokenEx( token, MAXIMUM_ALLOWED, NULL, level, type, ret );
}

/******************************************************************************
 * DuplicateToken    (kernelbase.@)
 */
BOOL WINAPI DuplicateToken( HANDLE token, SECURITY_IMPERSONATION_LEVEL level, PHANDLE ret )
{
    return DuplicateTokenEx( token, TOKEN_IMPERSONATE|TOKEN_QUERY, NULL, level, TokenImpersonation, ret );
}

/******************************************************************************
 * DuplicateTokenEx    (kernelbase.@)
 */
BOOL WINAPI DuplicateTokenEx( HANDLE token, DWORD access, LPSECURITY_ATTRIBUTES sa,
                              SECURITY_IMPERSONATION_LEVEL level, TOKEN_TYPE type, PHANDLE ret )
{
    OBJECT_ATTRIBUTES attr;

    TRACE("%p 0x%08x 0x%08x 0x%08x %p\n", token, access, level, type, ret );

    InitializeObjectAttributes( &attr, NULL, (sa && sa->bInheritHandle) ? OBJ_INHERIT : 0,
                                NULL, sa ? sa->lpSecurityDescriptor : NULL );
    return set_ntstatus( NtDuplicateToken( token, access, &attr, level, type, ret ));
}

/******************************************************************************
 * GetTokenInformation    (kernelbase.@)
 */
BOOL WINAPI GetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                 LPVOID info, DWORD len, LPDWORD retlen )
{
    TRACE("(%p, %s, %p, %d, %p):\n",
          token,
          (class == TokenUser) ? "TokenUser" :
          (class == TokenGroups) ? "TokenGroups" :
          (class == TokenPrivileges) ? "TokenPrivileges" :
          (class == TokenOwner) ? "TokenOwner" :
          (class == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (class == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (class == TokenSource) ? "TokenSource" :
          (class == TokenType) ? "TokenType" :
          (class == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (class == TokenStatistics) ? "TokenStatistics" :
          (class == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (class == TokenSessionId) ? "TokenSessionId" :
          (class == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (class == TokenSessionReference) ? "TokenSessionReference" :
          (class == TokenSandBoxInert) ? "TokenSandBoxInert" :
          "Unknown",
          info, len, retlen);

    return set_ntstatus( NtQueryInformationToken( token, class, info, len, retlen ));
}

/******************************************************************************
 * ImpersonateAnonymousToken    (kernelbase.@)
 */
BOOL WINAPI ImpersonateAnonymousToken( HANDLE thread )
{
    TRACE("(%p)\n", thread);
    return set_ntstatus( NtImpersonateAnonymousToken( thread ) );
}

/******************************************************************************
 * ImpersonateLoggedOnUser    (kernelbase.@)
 */
BOOL WINAPI ImpersonateLoggedOnUser( HANDLE token )
{
    DWORD size;
    BOOL ret;
    HANDLE dup;
    TOKEN_TYPE type;
    static BOOL warn = TRUE;

    if (warn)
    {
        FIXME( "(%p)\n", token );
        warn = FALSE;
    }
    if (!GetTokenInformation( token, TokenType, &type, sizeof(type), &size )) return FALSE;

    if (type == TokenPrimary)
    {
        if (!DuplicateToken( token, SecurityImpersonation, &dup )) return FALSE;
        ret = SetThreadToken( NULL, dup );
        NtClose( dup );
    }
    else ret = SetThreadToken( NULL, token );

    return ret;
}

/******************************************************************************
 * ImpersonateNamedPipeClient    (kernelbase.@)
 */
BOOL WINAPI ImpersonateNamedPipeClient( HANDLE pipe )
{
    IO_STATUS_BLOCK io_block;

    return set_ntstatus( NtFsControlFile( pipe, NULL, NULL, NULL, &io_block,
                                          FSCTL_PIPE_IMPERSONATE, NULL, 0, NULL, 0 ));
}

/******************************************************************************
 * ImpersonateSelf    (kernelbase.@)
 */
BOOL WINAPI ImpersonateSelf( SECURITY_IMPERSONATION_LEVEL level )
{
    return set_ntstatus( RtlImpersonateSelf( level ) );
}

/******************************************************************************
 * IsTokenRestricted    (kernelbase.@)
 */
BOOL WINAPI IsTokenRestricted( HANDLE token )
{
    TOKEN_GROUPS *groups;
    DWORD size;
    NTSTATUS status;
    BOOL restricted;

    TRACE("(%p)\n", token);

    status = NtQueryInformationToken(token, TokenRestrictedSids, NULL, 0, &size);
    if (status != STATUS_BUFFER_TOO_SMALL) return set_ntstatus(status);

    groups = heap_alloc(size);
    if (!groups)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    status = NtQueryInformationToken(token, TokenRestrictedSids, groups, size, &size);
    if (status != STATUS_SUCCESS)
    {
        heap_free(groups);
        return set_ntstatus(status);
    }

    restricted = groups->GroupCount > 0;
    heap_free(groups);

    return restricted;
}

/******************************************************************************
 * OpenProcessToken    (kernelbase.@)
 */
BOOL WINAPI OpenProcessToken( HANDLE process, DWORD access, HANDLE *handle )
{
    return set_ntstatus( NtOpenProcessToken( process, access, handle ));
}

/******************************************************************************
 * OpenThreadToken    (kernelbase.@)
 */
BOOL WINAPI OpenThreadToken( HANDLE thread, DWORD access, BOOL self, HANDLE *handle )
{
    return set_ntstatus( NtOpenThreadToken( thread, access, self, handle ));
}

/******************************************************************************
 * PrivilegeCheck    (kernelbase.@)
 */
BOOL WINAPI PrivilegeCheck( HANDLE token, PPRIVILEGE_SET privs, LPBOOL result )
{
    BOOLEAN res;
    BOOL ret = set_ntstatus( NtPrivilegeCheck( token, privs, &res ));
    if (ret) *result = res;
    return ret;
}

/******************************************************************************
 * RevertToSelf    (kernelbase.@)
 */
BOOL WINAPI RevertToSelf(void)
{
    return SetThreadToken( NULL, 0 );
}

/*************************************************************************
 * SetThreadToken    (kernelbase.@)
 */
BOOL WINAPI SetThreadToken( PHANDLE thread, HANDLE token )
{
    return set_ntstatus( NtSetInformationThread( thread ? *thread : GetCurrentThread(),
                                                 ThreadImpersonationToken, &token, sizeof(token) ));
}

/******************************************************************************
 * SetTokenInformation    (kernelbase.@)
 */
BOOL WINAPI SetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS class, LPVOID info, DWORD len )
{
    TRACE("(%p, %s, %p, %d)\n",
          token,
          (class == TokenUser) ? "TokenUser" :
          (class == TokenGroups) ? "TokenGroups" :
          (class == TokenPrivileges) ? "TokenPrivileges" :
          (class == TokenOwner) ? "TokenOwner" :
          (class == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (class == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (class == TokenSource) ? "TokenSource" :
          (class == TokenType) ? "TokenType" :
          (class == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (class == TokenStatistics) ? "TokenStatistics" :
          (class == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (class == TokenSessionId) ? "TokenSessionId" :
          (class == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (class == TokenSessionReference) ? "TokenSessionReference" :
          (class == TokenSandBoxInert) ? "TokenSandBoxInert" :
          "Unknown",
          info, len);

    return set_ntstatus( NtSetInformationToken( token, class, info, len ));
}
