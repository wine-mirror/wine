#ifndef __WINE_NTSECAPI_H
#define __WINE_NTSECAPI_H

#include "ntdef.h"
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

NTSTATUS WINAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);

NTSTATUS WINAPI LsaFreeMemory(PVOID);
NTSTATUS WINAPI LsaClose(IN LSA_HANDLE ObjectHandle);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_NTSECAPI_H) */
