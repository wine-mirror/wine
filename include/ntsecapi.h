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

#include "winnt.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

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

typedef struct
{
	BOOLEAN AuditingMode;
	PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
	ULONG MaximumAuditEventCount;
} POLICY_AUDIT_EVENTS_INFO, *PPOLICY_AUDIT_EVENTS_INFO;

typedef struct
{
    LSA_UNICODE_STRING Name;
    PSID Sid;
} POLICY_PRIMARY_DOMAIN_INFO, *PPOLICY_PRIMARY_DOMAIN_INFO;


NTSTATUS WINAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);

NTSTATUS WINAPI LsaFreeMemory(PVOID);
NTSTATUS WINAPI LsaClose(IN LSA_HANDLE ObjectHandle);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_NTSECAPI_H) */
