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

typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

typedef PVOID LSA_HANDLE, *PLSA_HANDLE;

NTSTATUS WINAPI LsaOpenPolicy(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);

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


NTSTATUS WINAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);

NTSTATUS WINAPI LsaFreeMemory(PVOID);
NTSTATUS WINAPI LsaClose(IN LSA_HANDLE ObjectHandle);
ULONG WINAPI LsaNtStatusToWinError(NTSTATUS Status);


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_NTSECAPI_H) */
