/*
 * Copyright (C) 1999 Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_NTSECAPI_H
#define __WINE_NTSECAPI_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Policy access rights */
#define POLICY_VIEW_LOCAL_INFORMATION           0x00000001L
#define POLICY_VIEW_AUDIT_INFORMATION           0x00000002L
#define POLICY_GET_PRIVATE_INFORMATION          0x00000004L
#define POLICY_TRUST_ADMIN                      0x00000008L
#define POLICY_CREATE_ACCOUNT                   0x00000010L
#define POLICY_CREATE_SECRET                    0x00000020L
#define POLICY_CREATE_PRIVILEGE                 0x00000040L
#define POLICY_SET_DEFAULT_QUOTA_LIMITS         0x00000080L
#define POLICY_SET_AUDIT_REQUIREMENTS           0x00000100L
#define POLICY_AUDIT_LOG_ADMIN                  0x00000200L
#define POLICY_SERVER_ADMIN                     0x00000400L
#define POLICY_LOOKUP_NAMES                     0x00000800L
#define POLICY_NOTIFICATION                     0x00001000L

#define POLICY_ALL_ACCESS                       ( \
    STANDARD_RIGHTS_REQUIRED | \
    POLICY_VIEW_LOCAL_INFORMATION | \
    POLICY_VIEW_AUDIT_INFORMATION | \
    POLICY_GET_PRIVATE_INFORMATION | \
    POLICY_TRUST_ADMIN | \
    POLICY_CREATE_ACCOUNT | \
    POLICY_CREATE_SECRET | \
    POLICY_CREATE_PRIVILEGE | \
    POLICY_SET_DEFAULT_QUOTA_LIMITS | \
    POLICY_SET_AUDIT_REQUIREMENTS | \
    POLICY_AUDIT_LOG_ADMIN | \
    POLICY_SERVER_ADMIN | \
    POLICY_LOOKUP_NAMES)


#define POLICY_READ                             ( \
    STANDARD_RIGHTS_READ | \
    POLICY_VIEW_AUDIT_INFORMATION | \
    POLICY_GET_PRIVATE_INFORMATION)

#define POLICY_WRITE                            ( \
   STANDARD_RIGHTS_WRITE | \
   POLICY_TRUST_ADMIN | \
   POLICY_CREATE_ACCOUNT | \
   POLICY_CREATE_SECRET | \
   POLICY_CREATE_PRIVILEGE | \
   POLICY_SET_DEFAULT_QUOTA_LIMITS | \
   POLICY_SET_AUDIT_REQUIREMENTS | \
   POLICY_AUDIT_LOG_ADMIN | \
   POLICY_SERVER_ADMIN)

#define POLICY_EXECUTE                          ( \
   STANDARD_RIGHTS_EXECUTE | \
   POLICY_VIEW_LOCAL_INFORMATION | \
   POLICY_LOOKUP_NAMES)

#define POLICY_AUDIT_EVENT_UNCHANGED 0x00000000L
#define POLICY_AUDIT_EVENT_SUCCESS   0x00000001L
#define POLICY_AUDIT_EVENT_FAILURE   0x00000002L
#define POLICY_AUDIT_EVENT_NONE      0x00000004L

#define POLICY_AUDIT_EVENT_MASK (POLICY_AUDIT_EVENT_SUCCESS | \
                                 POLICY_AUDIT_EVENT_FAILURE | \
                                 POLICY_AUDIT_EVENT_NONE)

/* logon rights names */
#define SE_BATCH_LOGON_NAME \
 TEXT("SeBatchLogonRight")
#define SE_INTERACTIVE_LOGON_NAME \
 TEXT("SeInteractiveLogonRight")
#define SE_NETWORK_LOGON_NAME \
 TEXT("SeNetworkLogonRight")
#define SE_REMOTE_INTERACTIVE_LOGON_NAME \
 TEXT("SeRemoteInteractiveLogonRight")
#define SE_SERVICE_LOGON_NAME \
 TEXT("SeServiceLogonRight")
#define SE_DENY_BATCH_LOGON_NAME \
 TEXT("SeDenyBatchLogonRight")
#define SE_DENY_INTERACTIVE_LOGON_NAME \
 TEXT("SeDenyInteractiveLogonRight")
#define SE_DENY_NETWORK_LOGON_NAME \
 TEXT("SeDenyNetworkLogonRight")
#define SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME \
 TEXT("SeDenyRemoteInteractiveLogonRight")
#define SE_DENY_SERVICE_LOGON_NAME \
 TEXT("SeDenyServiceLogonRight")

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif
#ifndef WINE_PNTSTATUS_DECLARED
#define WINE_PNTSTATUS_DECLARED
typedef NTSTATUS *PNTSTATUS;
#endif

typedef enum _SECURITY_LOGON_TYPE
{
    Interactive = 2,
    Network,
    Batch,
    Service,
    Proxy
} SECURITY_LOGON_TYPE, *PSECURITY_LOGON_TYPE;

typedef enum _POLICY_AUDIT_EVENT_TYPE
{
    AuditCategorySystem,
    AuditCategoryLogon,
    AuditCategoryObjectAccess,
    AuditCategoryPrivilegeUse,
    AuditCategoryDetailedTracking,
    AuditCategoryPolicyChange,
    AuditCategoryAccountManagement
} POLICY_AUDIT_EVENT_TYPE, *PPOLICY_AUDIT_EVENT_TYPE;

typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

typedef PVOID LSA_HANDLE, *PLSA_HANDLE;
typedef ULONG LSA_ENUMERATION_HANDLE, *PLSA_ENUMERATION_HANDLE;

typedef enum
{
	PolicyAuditLogInformation = 1,
	PolicyAuditEventsInformation,
	PolicyPrimaryDomainInformation,
	PolicyPdAccountInformation,
	PolicyAccountDomainInformation,
	PolicyLsaServerRoleInformation,
	PolicyReplicaSourceInformation,
	PolicyDefaultQuotaInformation,
	PolicyModificationInformation,
	PolicyAuditFullSetInformation,
	PolicyAuditFullQueryInformation,
	PolicyDnsDomainInformation
} POLICY_INFORMATION_CLASS, *PPOLICY_INFORMATION_CLASS;

typedef ULONG POLICY_AUDIT_EVENT_OPTIONS, *PPOLICY_AUDIT_EVENT_OPTIONS;

typedef struct _POLICY_AUDIT_EVENTS_INFO
{
	BOOLEAN AuditingMode;
	PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
	ULONG MaximumAuditEventCount;
} POLICY_AUDIT_EVENTS_INFO, *PPOLICY_AUDIT_EVENTS_INFO;

typedef struct _POLICY_PRIMARY_DOMAIN_INFO
{
    LSA_UNICODE_STRING Name;
    PSID Sid;
} POLICY_PRIMARY_DOMAIN_INFO, *PPOLICY_PRIMARY_DOMAIN_INFO;

typedef struct _POLICY_ACCOUNT_DOMAIN_INFO
{
    LSA_UNICODE_STRING DomainName;
    PSID DomainSid;
} POLICY_ACCOUNT_DOMAIN_INFO, *PPOLICY_ACCOUNT_DOMAIN_INFO;

typedef struct
{
    SID_NAME_USE Use;
    LSA_UNICODE_STRING Name;
    LONG DomainIndex;
} LSA_TRANSLATED_NAME, *PLSA_TRANSLATED_NAME;

typedef struct
{
    LSA_UNICODE_STRING Name;
    PSID Sid;
} LSA_TRUST_INFORMATION, *PLSA_TRUST_INFORMATION;

typedef struct
{
    ULONG Entries;
    PLSA_TRUST_INFORMATION Domains;
} LSA_REFERENCED_DOMAIN_LIST, *PLSA_REFERENCED_DOMAIN_LIST;

typedef struct _LSA_TRANSLATED_SID
{
    SID_NAME_USE Use;
    ULONG RelativeId;
    LONG DomainIndex;
} LSA_TRANSLATED_SID, *PLSA_TRANSLATED_SID;

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE,ULONG,PVOID,ULONG,PVOID*,PULONG,PNTSTATUS);
NTSTATUS WINAPI LsaClose(LSA_HANDLE);
NTSTATUS WINAPI LsaConnectUntrusted(PHANDLE);
NTSTATUS WINAPI LsaDeregisterLogonProcess(HANDLE);
NTSTATUS WINAPI LsaEnumerateTrustedDomains(LSA_HANDLE,PLSA_ENUMERATION_HANDLE,PVOID*,ULONG,PULONG);
NTSTATUS WINAPI LsaFreeMemory(PVOID);
NTSTATUS WINAPI LsaLookupNames(LSA_HANDLE,ULONG Count,PLSA_UNICODE_STRING,PLSA_REFERENCED_DOMAIN_LIST*,
                               PLSA_TRANSLATED_SID*);
NTSTATUS WINAPI LsaLookupSids(LSA_HANDLE,ULONG,PSID *,PLSA_REFERENCED_DOMAIN_LIST *,PLSA_TRANSLATED_NAME *);
ULONG WINAPI LsaNtStatusToWinError(NTSTATUS);
NTSTATUS WINAPI LsaOpenPolicy(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
NTSTATUS WINAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
NTSTATUS WINAPI LsaRetrievePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,PLSA_UNICODE_STRING*);
NTSTATUS WINAPI LsaSetInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID);
NTSTATUS WINAPI LsaStorePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,PLSA_UNICODE_STRING);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_NTSECAPI_H) */
