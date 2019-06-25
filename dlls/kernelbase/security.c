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
