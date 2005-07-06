/*
 * Implementation of the Local Security Authority API
 *
 * Copyright 1999 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Mike McCormack
 * Copyright 2005 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "ntstatus.h"
#include "ntsecapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

#define ADVAPI_ForceLocalComputer(ServerName, FailureCode) \
    if (!ADVAPI_IsLocalComputer(ServerName)) \
{ \
        FIXME("Action Implemented for local computer only. " \
              "Requested for server %s\n", debugstr_w(ServerName)); \
        return FailureCode; \
}

static void dumpLsaAttributes(PLSA_OBJECT_ATTRIBUTES oa)
{
    if (oa)
    {
        TRACE("\n\tlength=%lu, rootdir=%p, objectname=%s\n\tattr=0x%08lx, sid=%p qos=%p\n",
              oa->Length, oa->RootDirectory,
              oa->ObjectName?debugstr_w(oa->ObjectName->Buffer):"null",
              oa->Attributes, oa->SecurityDescriptor, oa->SecurityQualityOfService);
    }
}

/************************************************************
 * ADVAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
static BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Result;
    LPWSTR buf;

    if (!ServerName || !ServerName[0])
        return TRUE;

    buf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    Result = GetComputerNameW(buf,  &dwSize);
    if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
        ServerName += 2;
    Result = Result && !lstrcmpW(ServerName, buf);
    HeapFree(GetProcessHeap(), 0, buf);

    return Result;
}

/******************************************************************************
 * LsaClose [ADVAPI32.@]
 *
 * Closes a handle to a Policy or TrustedDomain.
 *
 * PARAMS
 *  ObjectHandle [I] Handle to a Policy or TrustedDomain.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: NTSTATUS code.
 */
NTSTATUS WINAPI LsaClose(IN LSA_HANDLE ObjectHandle)
{
    FIXME("(%p) stub\n", ObjectHandle);
    return 0xc0000000;
}

/******************************************************************************
 * LsaEnumerateTrustedDomains [ADVAPI32.@]
 *
 * Returns the names and SIDs of trusted domains.
 *
 * PARAMS
 *  PolicyHandle          [I] Handle to a Policy object.
 *  EnumerationContext    [I] Pointer to an enumeration handle.
 *  Buffer                [O] Contains the names and SIDs of trusted domains.
 *  PreferedMaximumLength [I] Prefered maximum size in bytes of Buffer.
 *  CountReturned         [O] Number of elements in Buffer.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS,
 *           STATUS_MORE_ENTRIES,
 *           STATUS_NO_MORE_ENTRIES
 *  Failure: NTSTATUS code.
 *
 * NOTES
 *  LsaEnumerateTrustedDomains can be called multiple times to enumerate
 *  all trusted domains.
 */
NTSTATUS WINAPI LsaEnumerateTrustedDomains(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID* Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned)
{
    FIXME("(%p,%p,%p,0x%08lx,%p) stub\n", PolicyHandle, EnumerationContext,
          Buffer, PreferedMaximumLength, CountReturned);

    if (CountReturned) *CountReturned = 0;
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaFreeMemory [ADVAPI32.@]
 *
 * Frees memory allocated by a LSA function.
 *
 * PARAMS
 *  Buffer [I] Memory buffer to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: NTSTATUS code.
 */
NTSTATUS WINAPI LsaFreeMemory(IN PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    return HeapFree(GetProcessHeap(), 0, Buffer);
}

/******************************************************************************
 * LsaLookupNames [ADVAPI32.@]
 *
 * Returns the SIDs of an array of user, group, or local group names.
 *
 * PARAMS
 *  PolicyHandle      [I] Handle to a Policy object.
 *  Count             [I] Number of names in Names.
 *  Names             [I] Array of names to lookup.
 *  ReferencedDomains [O] Array of domains where the names were found.
 *  Sids              [O] Array of SIDs corresponding to Names.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS,
 *           STATUS_SOME_NOT_MAPPED
 *  Failure: STATUS_NONE_MAPPED or NTSTATUS code.
 */
NTSTATUS WINAPI LsaLookupNames(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSA_UNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST* ReferencedDomains,
    OUT PLSA_TRANSLATED_SID* Sids)
{
    FIXME("(%p,0x%08lx,%p,%p,%p) stub\n", PolicyHandle, Count, Names,
          ReferencedDomains, Sids);

    return STATUS_NONE_MAPPED;
}

/******************************************************************************
 * LsaLookupSids [ADVAPI32.@]
 *
 * Looks up the names that correspond to an array of SIDs.
 *
 * PARAMS
 *  PolicyHandle      [I] Handle to a Policy object.
 *  Count             [I] Number of SIDs in the Sids array.
 *  Sids              [I] Array of SIDs to lookup.
 *  ReferencedDomains [O] Array of domains where the sids were found.
 *  Names             [O] Array of names corresponding to Sids.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS,
 *           STATUS_SOME_NOT_MAPPED
 *  Failure: STATUS_NONE_MAPPED or NTSTATUS code.
 */
NTSTATUS WINAPI LsaLookupSids(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME *Names )
{
    FIXME("(%p,%lu,%p,%p,%p) stub\n", PolicyHandle, Count, Sids,
          ReferencedDomains, Names);

    return FALSE;
}

/******************************************************************************
 * LsaNtStatusToWinError [ADVAPI32.@]
 *
 * Converts an LSA NTSTATUS code to a Windows error code.
 *
 * PARAMS
 *  Status [I] NTSTATUS code.
 *
 * RETURNS
 *  Success: Corresponding Windows error code.
 *  Failure: ERROR_MR_MID_NOT_FOUND.
 */
ULONG WINAPI LsaNtStatusToWinError(NTSTATUS Status)
{
    return RtlNtStatusToDosError(Status);
}

/******************************************************************************
 * LsaOpenPolicy [ADVAPI32.@]
 *
 * Opens a handle to the Policy object on a local or remote system.
 *
 * PARAMS
 *  SystemName       [I] Name of the target system.
 *  ObjectAttributes [I] Connection attributes.
 *  DesiredAccess    [I] Requested access rights.
 *  PolicyHandle     [I/O] Handle to the Policy object.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: NTSTATUS code.
 *
 * NOTES
 *  Set SystemName to NULL to open the local Policy object.
 */
NTSTATUS WINAPI LsaOpenPolicy(
    IN PLSA_UNICODE_STRING SystemName,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle)
{
    FIXME("(%s,%p,0x%08lx,%p) stub\n",
          SystemName?debugstr_w(SystemName->Buffer):"(null)",
          ObjectAttributes, DesiredAccess, PolicyHandle);

    ADVAPI_ForceLocalComputer(SystemName ? SystemName->Buffer : NULL,
                              STATUS_ACCESS_VIOLATION);
    dumpLsaAttributes(ObjectAttributes);

    if(PolicyHandle) *PolicyHandle = (LSA_HANDLE)0xcafe;
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaQueryInformationPolicy [ADVAPI32.@]
 *
 * Returns information about a Policy object.
 *
 * PARAMS
 *  PolicyHandle     [I] Handle to a Policy object.
 *  InformationClass [I] Type of information to retrieve.
 *  Buffer           [O] Pointer to the requested information.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: NTSTATUS code.
 */
NTSTATUS WINAPI LsaQueryInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer)
{
    FIXME("(%p,0x%08x,%p) stub\n", PolicyHandle, InformationClass, Buffer);

    if(!Buffer) return FALSE;
    switch (InformationClass)
    {
        case PolicyAuditEventsInformation: /* 2 */
        {
            PPOLICY_AUDIT_EVENTS_INFO p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                    sizeof(POLICY_AUDIT_EVENTS_INFO));
            p->AuditingMode = FALSE; /* no auditing */
            *Buffer = p;
        }
        break;
        case PolicyPrimaryDomainInformation: /* 3 */
        case PolicyAccountDomainInformation: /* 5 */
        {
            struct di
            {
                POLICY_PRIMARY_DOMAIN_INFO ppdi;
                SID sid;
            };

            SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};

            struct di * xdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(xdi));
            HKEY key;
            BOOL useDefault = TRUE;
            LONG ret;

            if ((ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                 "System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0,
                 KEY_READ, &key)) == ERROR_SUCCESS)
            {
                DWORD size = 0;
                static const WCHAR wg[] = { 'W','o','r','k','g','r','o','u','p',0 };

                ret = RegQueryValueExW(key, wg, NULL, NULL, NULL, &size);
                if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
                {
                    xdi->ppdi.Name.Buffer = HeapAlloc(GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY, size);

                    if ((ret = RegQueryValueExW(key, wg, NULL, NULL,
                         (LPBYTE)xdi->ppdi.Name.Buffer, &size)) == ERROR_SUCCESS)
                    {
                        xdi->ppdi.Name.Length = (USHORT)size;
                        useDefault = FALSE;
                    }
                    else
                    {
                        HeapFree(GetProcessHeap(), 0, xdi->ppdi.Name.Buffer);
                        xdi->ppdi.Name.Buffer = NULL;
                    }
                }
                RegCloseKey(key);
            }
            if (useDefault)
                RtlCreateUnicodeStringFromAsciiz(&(xdi->ppdi.Name), "DOMAIN");

            TRACE("setting domain to %s\n", debugstr_w(xdi->ppdi.Name.Buffer));

            xdi->ppdi.Sid = &(xdi->sid);
            xdi->sid.Revision = SID_REVISION;
            xdi->sid.SubAuthorityCount = 1;
            xdi->sid.IdentifierAuthority = localSidAuthority;
            xdi->sid.SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
            *Buffer = xdi;
        }
        break;
        case  PolicyAuditLogInformation:
        case  PolicyPdAccountInformation:
        case  PolicyLsaServerRoleInformation:
        case  PolicyReplicaSourceInformation:
        case  PolicyDefaultQuotaInformation:
        case  PolicyModificationInformation:
        case  PolicyAuditFullSetInformation:
        case  PolicyAuditFullQueryInformation:
        case  PolicyDnsDomainInformation:
        {
            FIXME("category not implemented\n");
            return FALSE;
        }
    }
    return TRUE;
}

/******************************************************************************
 * LsaRetrievePrivateData [ADVAPI32.@]
 *
 * Retrieves data stored by LsaStorePrivateData.
 *
 * PARAMS
 *  PolicyHandle [I] Handle to a Policy object.
 *  KeyName      [I] Name of the key where the data is stored.
 *  PrivateData  [O] Pointer to the private data.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: STATUS_OBJECT_NAME_NOT_FOUND or NTSTATUS code.
 */
NTSTATUS WINAPI LsaRetrievePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    OUT PLSA_UNICODE_STRING* PrivateData)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, KeyName, PrivateData);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/******************************************************************************
 * LsaSetInformationPolicy [ADVAPI32.@]
 *
 * Modifies information in a Policy object.
 *
 * PARAMS
 *  PolicyHandle     [I] Handle to a Policy object.
 *  InformationClass [I] Type of information to set.
 *  Buffer           [I] Pointer to the information to set.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: NTSTATUS code.
 */
NTSTATUS WINAPI LsaSetInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer)
{
    FIXME("(%p,0x%08x,%p) stub\n", PolicyHandle, InformationClass, Buffer);

    return STATUS_UNSUCCESSFUL;
}

/******************************************************************************
 * LsaStorePrivateData [ADVAPI32.@]
 *
 * Stores or deletes a Policy object's data under the specified reg key.
 *
 * PARAMS
 *  PolicyHandle [I] Handle to a Policy object.
 *  KeyName      [I] Name of the key where the data will be stored.
 *  PrivateData  [O] Pointer to the private data.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: STATUS_OBJECT_NAME_NOT_FOUND or NTSTATUS code.
 */
NTSTATUS WINAPI LsaStorePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    IN PLSA_UNICODE_STRING PrivateData)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, KeyName, PrivateData);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}
