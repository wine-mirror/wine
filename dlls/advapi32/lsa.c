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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "sddl.h"
#include "advapi32_misc.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

#define ADVAPI_ForceLocalComputer(ServerName, FailureCode) \
    if (!ADVAPI_IsLocalComputer(ServerName)) \
{ \
        FIXME("Action Implemented for local computer only. " \
              "Requested for server %s\n", debugstr_w(ServerName)); \
        return FailureCode; \
}

static LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "(null)";
    return debugstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

static void dumpLsaAttributes(const LSA_OBJECT_ATTRIBUTES *oa)
{
    if (oa)
    {
        TRACE("\n\tlength=%lu, rootdir=%p, objectname=%s\n\tattr=0x%08lx, sid=%s qos=%p\n",
              oa->Length, oa->RootDirectory,
              oa->ObjectName?debugstr_w(oa->ObjectName->Buffer):"null",
              oa->Attributes, debugstr_sid(oa->SecurityDescriptor),
              oa->SecurityQualityOfService);
    }
}

static const char *debugstr_InformationClass( POLICY_INFORMATION_CLASS class )
{
    static const char * const names[] =
    {
        NULL,
        "PolicyAuditLogInformation",
        "PolicyAuditEventsInformation",
        "PolicyPrimaryDomainInformation",
        "PolicyPdAccountInformation",
        "PolicyAccountDomainInformation",
        "PolicyLsaServerRoleInformation",
        "PolicyReplicaSourceInformation",
        "PolicyDefaultQuotaInformation",
        "PolicyModificationInformation",
        "PolicyAuditFullSetInformation",
        "PolicyAuditFullQueryInformation",
        "PolicyDnsDomainInformation",
        "PolicyDnsDomainInformationInt",
        "PolicyLocalAccountDomainInformation",
        "PolicyMachineAccountInformation",
        "PolicyMachineAccountInformation2",
    };

    if (class < ARRAY_SIZE(names) && names[class]) return names[class];
    return wine_dbg_sprintf( "%u", class );
}

static void* ADVAPI_GetDomainName(unsigned sz, unsigned ofs)
{
    HKEY key;
    LONG ret;
    BYTE* ptr = NULL;
    UNICODE_STRING* ustr;

    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0, KEY_READ, &key);
    if (ret == ERROR_SUCCESS)
    {
        DWORD size = 0;
        ret = RegQueryValueExW(key, L"Workgroup", NULL, NULL, NULL, &size);
        if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
        {
            ptr = calloc(1, sz + size);
            if (!ptr) return NULL;
            ustr = (UNICODE_STRING*)(ptr + ofs);
            ustr->MaximumLength = size;
            ustr->Buffer = (WCHAR*)(ptr + sz);
            ret = RegQueryValueExW(key, L"Workgroup", NULL, NULL, (LPBYTE)ustr->Buffer, &size);
            if (ret != ERROR_SUCCESS)
            {
                free(ptr);
                ptr = NULL;
            }
            else ustr->Length = size - sizeof(WCHAR);
        }
        RegCloseKey(key);
    }
    if (!ptr)
    {
        ptr = calloc(1, sz + sizeof(L"DOMAIN"));
        if (!ptr) return NULL;
        ustr = (UNICODE_STRING*)(ptr + ofs);
        ustr->MaximumLength = sizeof(L"DOMAIN");
        ustr->Buffer = (WCHAR*)(ptr + sz);
        ustr->Length = sizeof(L"DOMAIN") - sizeof(WCHAR);
        memcpy(ustr->Buffer, L"DOMAIN", sizeof(L"DOMAIN"));
    }
    return ptr;
}

/******************************************************************************
 * LsaGetUserName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaGetUserName(PUNICODE_STRING *user_name, PUNICODE_STRING *domain_name)
{
    UNICODE_STRING *user;
    DWORD user_size;

    user_size = 0;
    if (GetUserNameW(NULL, &user_size) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
       return STATUS_UNSUCCESSFUL;

    user = malloc(sizeof(*user) + user_size * sizeof(WCHAR));
    if (!user) return STATUS_NO_MEMORY;

    user->Buffer = (WCHAR *)(user + 1);
    user->MaximumLength = user_size * sizeof(WCHAR);
    user->Length = user->MaximumLength - sizeof(WCHAR);
    if (!GetUserNameW(user->Buffer, &user_size))
    {
        free(user);
        return STATUS_UNSUCCESSFUL;
    }

    if (domain_name)
    {
        UNICODE_STRING *domain;
        WCHAR computer[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD domain_size;

        domain_size = ARRAY_SIZE(computer);
        if (!GetComputerNameW(computer, &domain_size))
        {
            free(user);
            return STATUS_UNSUCCESSFUL;
        }

        domain = malloc(sizeof(*domain) + (domain_size + 1) * sizeof(WCHAR));
        if (!domain)
        {
            free(user);
            return STATUS_NO_MEMORY;
        }

        domain->Buffer = (WCHAR *)(domain + 1);
        domain->Length = domain_size * sizeof(WCHAR);
        domain->MaximumLength = domain->Length + sizeof(WCHAR);
        wcscpy(domain->Buffer, computer);

        *domain_name = domain;
    }

    *user_name = user;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaAddAccountRights [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaAddAccountRights(
    LSA_HANDLE policy,
    PSID sid,
    PLSA_UNICODE_STRING rights,
    ULONG count)
{
    FIXME("(%p,%p,%p,0x%08lx) stub\n", policy, sid, rights, count);
    return STATUS_SUCCESS;
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
    WARN("(%p) stub\n", ObjectHandle);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaCreateTrustedDomainEx [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaCreateTrustedDomainEx(
    LSA_HANDLE policy,
    PTRUSTED_DOMAIN_INFORMATION_EX domain_info,
    PTRUSTED_DOMAIN_AUTH_INFORMATION auth_info,
    ACCESS_MASK access,
    PLSA_HANDLE domain)
{
    FIXME("(%p,%p,%p,0x%08lx,%p) stub\n", policy, domain_info, auth_info,
          access, domain);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaDeleteTrustedDomain [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaDeleteTrustedDomain(LSA_HANDLE policy, PSID sid)
{
    FIXME("(%p,%p) stub\n", policy, sid);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaEnumerateAccountRights [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaEnumerateAccountRights(
    LSA_HANDLE policy,
    PSID sid,
    PLSA_UNICODE_STRING *rights,
    PULONG count)
{
    FIXME("(%p,%p,%p,%p) stub\n", policy, sid, rights, count);
    *rights = 0;
    *count = 0;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/******************************************************************************
 * LsaEnumerateAccounts [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaEnumerateAccounts(
    LSA_HANDLE policy,
    PLSA_ENUMERATION_HANDLE context,
    PVOID *buffer,
    ULONG maxlen,
    PULONG count)
{
    FIXME("(%p,%p,%p,%ld,%p) stub\n", policy, context, buffer, maxlen, count);
    if (count) *count = 0;
    return STATUS_NO_MORE_ENTRIES;
}

/******************************************************************************
 * LsaEnumerateAccountsWithUserRight [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaEnumerateAccountsWithUserRight(
    LSA_HANDLE policy,
    PLSA_UNICODE_STRING rights,
    PVOID *buffer,
    PULONG count)
{
    FIXME("(%p,%p,%p,%p) stub\n", policy, rights, buffer, count);
    return STATUS_NO_MORE_ENTRIES;
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
 *  PreferredMaximumLength[I] Preferred maximum size in bytes of Buffer.
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
    IN ULONG PreferredMaximumLength,
    OUT PULONG CountReturned)
{
    FIXME("(%p,%p,%p,0x%08lx,%p) stub\n", PolicyHandle, EnumerationContext,
          Buffer, PreferredMaximumLength, CountReturned);

    if (CountReturned) *CountReturned = 0;
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaEnumerateTrustedDomainsEx [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaEnumerateTrustedDomainsEx(
    LSA_HANDLE policy,
    PLSA_ENUMERATION_HANDLE context,
    PVOID *buffer,
    ULONG length,
    PULONG count)
{
    FIXME("(%p,%p,%p,0x%08lx,%p) stub\n", policy, context, buffer, length, count);

    if (count) *count = 0;
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

    free(Buffer);
    return STATUS_SUCCESS;
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

static BOOL lookup_name( LSA_UNICODE_STRING *name, SID *sid, DWORD *sid_size, WCHAR *domain,
                         DWORD *domain_size, SID_NAME_USE *use, BOOL *handled )
{
    BOOL ret;

    ret = lookup_local_wellknown_name( name, sid, sid_size, domain, domain_size, use, handled );
    if (!*handled)
        ret = lookup_local_user_name( name, sid, sid_size, domain, domain_size, use, handled );

    return ret;
}

/* Adds domain info to referenced domain list.
   Domain list is stored as plain buffer, layout is:

       LSA_REFERENCED_DOMAIN_LIST,
       LSA_TRUST_INFORMATION array,
       domain data array of
           {
               domain name data (WCHAR buffer),
               SID data
           }

   Parameters:
       list   [I]  referenced list pointer
       domain [I]  domain name string
       data   [IO] pointer to domain data array
*/
static LONG lsa_reflist_add_domain(LSA_REFERENCED_DOMAIN_LIST *list, LSA_UNICODE_STRING *domain, char **data)
{
    ULONG sid_size = 0,domain_size = 0;
    BOOL handled = FALSE;
    SID_NAME_USE use;
    LONG i;

    for (i = 0; i < list->Entries; i++)
    {
        /* try to reuse index */
        if ((list->Domains[i].Name.Length == domain->Length) &&
            (!wcsnicmp(list->Domains[i].Name.Buffer, domain->Buffer, (domain->Length / sizeof(WCHAR)))))
        {
            return i;
        }
    }

    /* no matching domain found, store name */
    list->Domains[list->Entries].Name.Length = domain->Length;
    list->Domains[list->Entries].Name.MaximumLength = domain->MaximumLength;
    list->Domains[list->Entries].Name.Buffer = (WCHAR*)*data;
    memcpy(list->Domains[list->Entries].Name.Buffer, domain->Buffer, domain->MaximumLength);
    *data += domain->MaximumLength;

    /* get and store SID data */
    list->Domains[list->Entries].Sid = *data;
    lookup_name(domain, NULL, &sid_size, NULL, &domain_size, &use, &handled);
    domain_size = 0;
    lookup_name(domain, list->Domains[list->Entries].Sid, &sid_size, NULL, &domain_size, &use, &handled);
    *data += sid_size;

    return list->Entries++;
}

/******************************************************************************
 * LsaLookupNames2 [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaLookupNames2( LSA_HANDLE policy, ULONG flags, ULONG count,
                                 PLSA_UNICODE_STRING names, PLSA_REFERENCED_DOMAIN_LIST *domains,
                                 PLSA_TRANSLATED_SID2 *sids )
{
    ULONG i, sid_size_total = 0, domain_size_max = 0, size, domainname_size_total = 0;
    ULONG sid_size, domain_size, mapped;
    LSA_UNICODE_STRING domain;
    BOOL handled = FALSE;
    char *domain_data;
    SID_NAME_USE use;
    SID *sid;

    TRACE("(%p,0x%08lx,0x%08lx,%p,%p,%p)\n", policy, flags, count, names, domains, sids);

    mapped = 0;
    for (i = 0; i < count; i++)
    {
        handled = FALSE;
        sid_size = domain_size = 0;
        lookup_name( &names[i], NULL, &sid_size, NULL, &domain_size, &use, &handled );
        if (handled)
        {
            sid_size_total += sid_size;
            domainname_size_total += domain_size;
            if (domain_size)
            {
                if (domain_size > domain_size_max)
                    domain_size_max = domain_size;
            }
            mapped++;
        }
    }
    TRACE("mapped %lu out of %lu\n", mapped, count);

    size = sizeof(LSA_TRANSLATED_SID2) * count + sid_size_total;
    if (!(*sids = malloc(size))) return STATUS_NO_MEMORY;

    sid = (SID *)(*sids + count);

    /* use maximum domain count */
    if (!(*domains = malloc(sizeof(LSA_REFERENCED_DOMAIN_LIST) + sizeof(LSA_TRUST_INFORMATION) * count +
                            sid_size_total + domainname_size_total * sizeof(WCHAR))))
    {
        free(*sids);
        return STATUS_NO_MEMORY;
    }
    (*domains)->Entries = 0;
    (*domains)->Domains = (LSA_TRUST_INFORMATION*)((char*)*domains + sizeof(LSA_REFERENCED_DOMAIN_LIST));
    domain_data = (char*)(*domains)->Domains + sizeof(LSA_TRUST_INFORMATION)*count;

    domain.Buffer = malloc(domain_size_max * sizeof(WCHAR));
    for (i = 0; i < count; i++)
    {
        domain.Length = domain_size_max*sizeof(WCHAR);
        domain.MaximumLength = domain_size_max*sizeof(WCHAR);

        (*sids)[i].Use = SidTypeUnknown;
        (*sids)[i].DomainIndex = -1;
        (*sids)[i].Flags = 0;

        handled = FALSE;
        sid_size = sid_size_total;
        domain_size = domain_size_max;
        lookup_name( &names[i], sid, &sid_size, domain.Buffer, &domain_size, &use, &handled );
        if (handled)
        {
            (*sids)[i].Sid = sid;
            (*sids)[i].Use = use;

            sid = (SID *)((char *)sid + sid_size);
            sid_size_total -= sid_size;
            if (domain_size)
            {
                domain.Length = domain_size * sizeof(WCHAR);
                domain.MaximumLength = (domain_size + 1) * sizeof(WCHAR);
                (*sids)[i].DomainIndex = lsa_reflist_add_domain(*domains, &domain, &domain_data);
            }
        }
    }
    free(domain.Buffer);

    if (mapped == count) return STATUS_SUCCESS;
    if (mapped > 0 && mapped < count) return STATUS_SOME_NOT_MAPPED;
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
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PSID *Sids,
    LSA_REFERENCED_DOMAIN_LIST **ReferencedDomains,
    LSA_TRANSLATED_NAME **Names)
{
    ULONG i, mapped, name_fullsize, domain_fullsize;
    ULONG name_size, domain_size;
    LSA_UNICODE_STRING domain;
    WCHAR *name_buffer;
    char *domain_data;
    SID_NAME_USE use;
    WCHAR *strsid;

    TRACE("(%p, %lu, %p, %p, %p)\n", PolicyHandle, Count, Sids, ReferencedDomains, Names);

    /* this length does not include actual string length yet */
    name_fullsize = sizeof(LSA_TRANSLATED_NAME) * Count;
    if (!(*Names = malloc(name_fullsize))) return STATUS_NO_MEMORY;
    /* maximum count of stored domain infos is Count, allocate it like that cause really needed
       count could only be computed after sid data is retrieved */
    domain_fullsize = sizeof(LSA_REFERENCED_DOMAIN_LIST) + sizeof(LSA_TRUST_INFORMATION)*Count;
    if (!(*ReferencedDomains = malloc(domain_fullsize)))
    {
        free(*Names);
        return STATUS_NO_MEMORY;
    }
    (*ReferencedDomains)->Entries = 0;
    (*ReferencedDomains)->Domains = (LSA_TRUST_INFORMATION*)((char*)*ReferencedDomains + sizeof(LSA_REFERENCED_DOMAIN_LIST));

    /* Get full names data length and full length needed to store domain name and SID */
    for (i = 0; i < Count; i++)
    {
        (*Names)[i].Use = SidTypeUnknown;
        (*Names)[i].DomainIndex = -1;
        RtlInitUnicodeStringEx(&(*Names)[i].Name, NULL);

        memset(&(*ReferencedDomains)->Domains[i], 0, sizeof(LSA_TRUST_INFORMATION));

        name_size = domain_size = 0;
        if (!LookupAccountSidW(NULL, Sids[i], NULL, &name_size, NULL, &domain_size, &use) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            if (name_size)
            {
                (*Names)[i].Name.Length = (name_size - 1) * sizeof(WCHAR);
                (*Names)[i].Name.MaximumLength = name_size * sizeof(WCHAR);
            }
            else
            {
                (*Names)[i].Name.Length = 0;
                (*Names)[i].Name.MaximumLength = sizeof(WCHAR);
            }

            name_fullsize += (*Names)[i].Name.MaximumLength;

            /* This potentially allocates more than needed, cause different names will reuse same domain index.
               Also it's not possible to store domain name length right here for the same reason. */
            if (domain_size)
            {
                ULONG sid_size = 0;
                BOOL handled = FALSE;
                WCHAR *name;

                domain_fullsize += domain_size * sizeof(WCHAR);

                /* get domain SID size too */
                name = malloc(domain_size * sizeof(WCHAR));
                *name = 0;
                LookupAccountSidW(NULL, Sids[i], NULL, &name_size, name, &domain_size, &use);

                domain.Buffer = name;
                domain.Length = domain_size * sizeof(WCHAR);
                domain.MaximumLength = domain_size * sizeof(WCHAR);

                lookup_name(&domain, NULL, &sid_size, NULL, &domain_size, &use, &handled);
                domain_fullsize += sid_size;

                free(name);
            }
            else
            {
                /* If we don't have a domain name, use a zero-length entry rather than a null value. */
                domain_fullsize += sizeof(WCHAR);
                domain.Length = 0;
                domain.MaximumLength = sizeof(WCHAR);
            }
        }
        else if (ConvertSidToStringSidW(Sids[i], &strsid))
        {
            (*Names)[i].Name.Length = lstrlenW(strsid) * sizeof(WCHAR);
            (*Names)[i].Name.MaximumLength = (lstrlenW(strsid) + 1) * sizeof(WCHAR);
            name_fullsize += (lstrlenW(strsid) + 1) * sizeof(WCHAR);

            LocalFree(strsid);
        }
    }

    /* now we have full length needed for both */
    *Names = realloc(*Names, name_fullsize);
    name_buffer = (WCHAR*)((char*)*Names + sizeof(LSA_TRANSLATED_NAME)*Count);

    *ReferencedDomains = realloc(*ReferencedDomains, domain_fullsize);
    /* fix pointer after reallocation */
    (*ReferencedDomains)->Domains = (LSA_TRUST_INFORMATION*)((char*)*ReferencedDomains + sizeof(LSA_REFERENCED_DOMAIN_LIST));
    domain_data = (char*)(*ReferencedDomains)->Domains + sizeof(LSA_TRUST_INFORMATION)*Count;

    mapped = 0;
    for (i = 0; i < Count; i++)
    {
        name_size = domain_size = 0;

        (*Names)[i].Name.Buffer = name_buffer;

        if (!LookupAccountSidW(NULL, Sids[i], NULL, &name_size, NULL, &domain_size, &use) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            mapped++;

            if (domain_size)
            {
                domain.Length = (domain_size - 1) * sizeof(WCHAR);
                domain.MaximumLength = domain_size * sizeof(WCHAR);
            }
            else
            {
                /* Use a zero-length buffer */
                domain.Length = 0;
                domain.MaximumLength = sizeof(WCHAR);
            }

            domain.Buffer = malloc(domain.MaximumLength);

            LookupAccountSidW(NULL, Sids[i], (*Names)[i].Name.Buffer, &name_size, domain.Buffer, &domain_size, &use);
            (*Names)[i].Use = use;

            (*Names)[i].DomainIndex = lsa_reflist_add_domain(*ReferencedDomains, &domain, &domain_data);
            free(domain.Buffer);
        }
        else if (ConvertSidToStringSidW(Sids[i], &strsid))
        {
            lstrcpyW((*Names)[i].Name.Buffer, strsid);
            LocalFree(strsid);
        }

        name_buffer += lstrlenW(name_buffer) + 1;
    }
    TRACE("mapped %lu out of %lu\n", mapped, Count);

    if (mapped == Count) return STATUS_SUCCESS;
    if (mapped) return STATUS_SOME_NOT_MAPPED;
    return STATUS_NONE_MAPPED;
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
    WARN("(%s,%p,0x%08lx,%p) stub\n",
          SystemName?debugstr_w(SystemName->Buffer):"(null)",
          ObjectAttributes, DesiredAccess, PolicyHandle);

    ADVAPI_ForceLocalComputer(SystemName ? SystemName->Buffer : NULL,
                              STATUS_ACCESS_VIOLATION);
    dumpLsaAttributes(ObjectAttributes);

    if(PolicyHandle) *PolicyHandle = (LSA_HANDLE)0xcafe;
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaOpenTrustedDomainByName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaOpenTrustedDomainByName(
    LSA_HANDLE policy,
    PLSA_UNICODE_STRING name,
    ACCESS_MASK access,
    PLSA_HANDLE handle)
{
    FIXME("(%p,%p,0x%08lx,%p) stub\n", policy, name, access, handle);
    return STATUS_OBJECT_NAME_NOT_FOUND;
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
    TRACE("(%p,%s,%p)\n", PolicyHandle, debugstr_InformationClass(InformationClass), Buffer);

    if(!Buffer) return STATUS_INVALID_PARAMETER;
    switch (InformationClass)
    {
        case PolicyAuditEventsInformation: /* 2 */
        {
            PPOLICY_AUDIT_EVENTS_INFO p = calloc(1, sizeof(POLICY_AUDIT_EVENTS_INFO));
            p->AuditingMode = FALSE; /* no auditing */
            *Buffer = p;
        }
        break;
        case PolicyPrimaryDomainInformation: /* 3 */
        {
            /* Only the domain name is valid for the local computer.
             * All other fields are zero.
             */
            PPOLICY_PRIMARY_DOMAIN_INFO pinfo;

            pinfo = ADVAPI_GetDomainName(sizeof(*pinfo), offsetof(POLICY_PRIMARY_DOMAIN_INFO, Name));

            TRACE("setting domain to %s\n", debugstr_w(pinfo->Name.Buffer));

            *Buffer = pinfo;
        }
        break;
        case PolicyAccountDomainInformation: /* 5 */
        {
            struct di
            {
                POLICY_ACCOUNT_DOMAIN_INFO info;
                SID sid;
                DWORD padding[3];
                WCHAR domain[MAX_COMPUTERNAME_LENGTH + 1];
            };

            DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
            struct di * xdi = calloc(1, sizeof(*xdi));

            xdi->info.DomainName.MaximumLength = dwSize * sizeof(WCHAR);
            xdi->info.DomainName.Buffer = xdi->domain;
            if (GetComputerNameW(xdi->info.DomainName.Buffer, &dwSize))
                xdi->info.DomainName.Length = dwSize * sizeof(WCHAR);

            TRACE("setting name to %s\n", debugstr_w(xdi->info.DomainName.Buffer));

            xdi->info.DomainSid = &xdi->sid;

            if (!ADVAPI_GetComputerSid(&xdi->sid))
            {
                free(xdi);

                WARN("Computer SID not found\n");

                return STATUS_UNSUCCESSFUL;
            }

            TRACE("setting SID to %s\n", debugstr_sid(&xdi->sid));

            *Buffer = xdi;
        }
        break;
        case  PolicyDnsDomainInformation:	/* 12 (0xc) */
        {
            struct
            {
                POLICY_DNS_DOMAIN_INFO info;
                struct
                {
                    SID sid;
                    DWORD sid_subauthority[3];
                } domain_sid;
                WCHAR domain_name[256];
                WCHAR dns_domain_name[256];
                WCHAR dns_forest_name[256];
            } *xdi;
            struct
            {
                SID sid;
                DWORD sid_subauthority[3];
            } computer_sid;
            DWORD dwSize;

            xdi = calloc(1, sizeof(*xdi));
            if (!xdi) return STATUS_NO_MEMORY;

            dwSize = 256;
            if (GetComputerNameExW(ComputerNamePhysicalDnsDomain, xdi->domain_name, &dwSize))
            {
                WCHAR *dot;

                dot = wcsrchr(xdi->domain_name, '.');
                if (dot) *dot = 0;
                wcsupr(xdi->domain_name);
                xdi->info.Name.Buffer = xdi->domain_name;
                xdi->info.Name.Length = lstrlenW(xdi->domain_name) * sizeof(WCHAR);
                xdi->info.Name.MaximumLength = xdi->info.Name.Length + sizeof(WCHAR);
                TRACE("setting Name to %s\n", debugstr_w(xdi->info.Name.Buffer));
            }

            dwSize = 256;
            if (GetComputerNameExW(ComputerNameDnsDomain, xdi->dns_domain_name, &dwSize))
            {
                xdi->info.DnsDomainName.Buffer = xdi->dns_domain_name;
                xdi->info.DnsDomainName.Length = dwSize * sizeof(WCHAR);
                xdi->info.DnsDomainName.MaximumLength = (dwSize + 1) * sizeof(WCHAR);
                TRACE("setting DnsDomainName to %s\n", debugstr_w(xdi->info.DnsDomainName.Buffer));

                xdi->info.DnsForestName.Buffer = xdi->dns_domain_name;
                xdi->info.DnsForestName.Length = dwSize * sizeof(WCHAR);
                xdi->info.DnsForestName.MaximumLength = (dwSize + 1) * sizeof(WCHAR);
                TRACE("setting DnsForestName to %s\n", debugstr_w(xdi->info.DnsForestName.Buffer));
            }

            dwSize = sizeof(xdi->domain_sid);
            if (ADVAPI_GetComputerSid(&computer_sid.sid) && GetWindowsAccountDomainSid(&computer_sid.sid, &xdi->domain_sid.sid, &dwSize))
            {
                xdi->info.Sid = &xdi->domain_sid.sid;
                TRACE("setting SID to %s\n", debugstr_sid(&xdi->domain_sid.sid));
            }

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
        {
            FIXME("category %d not implemented\n", InformationClass);
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaQueryTrustedDomainInfo [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaQueryTrustedDomainInfo(
    LSA_HANDLE policy,
    PSID sid,
    TRUSTED_INFORMATION_CLASS class,
    PVOID *buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", policy, sid, class, buffer);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/******************************************************************************
 * LsaQueryTrustedDomainInfoByName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaQueryTrustedDomainInfoByName(
    LSA_HANDLE policy,
    PLSA_UNICODE_STRING name,
    TRUSTED_INFORMATION_CLASS class,
    PVOID *buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", policy, name, class, buffer);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/******************************************************************************
 * LsaRegisterPolicyChangeNotification [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaRegisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS class,
    HANDLE event)
{
    FIXME("(%d,%p) stub\n", class, event);
    return STATUS_UNSUCCESSFUL;
}

/******************************************************************************
 * LsaRemoveAccountRights [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaRemoveAccountRights(
    LSA_HANDLE policy,
    PSID sid,
    BOOLEAN all,
    PLSA_UNICODE_STRING rights,
    ULONG count)
{
    FIXME("(%p,%p,%d,%p,0x%08lx) stub\n", policy, sid, all, rights, count);
    return STATUS_SUCCESS;
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
    FIXME("(%p,%s,%p) stub\n", PolicyHandle, debugstr_InformationClass(InformationClass), Buffer);

    return STATUS_UNSUCCESSFUL;
}

/******************************************************************************
 * LsaSetSecret [ADVAPI32.@]
 *
 * Set old and new values on a secret handle
 *
 * PARAMS
 *  SecretHandle          [I] Handle to a secret object.
 *  EncryptedCurrentValue [I] Pointer to encrypted new value, can be NULL
 *  EncryptedOldValue     [I] Pointer to encrypted old value, can be NULL
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: NTSTATUS code.
 */
NTSTATUS WINAPI LsaSetSecret(
    IN LSA_HANDLE SecretHandle,
    IN PLSA_UNICODE_STRING EncryptedCurrentValue,
    IN PLSA_UNICODE_STRING EncryptedOldValue)
{
    FIXME("(%p,%p,%p) stub\n", SecretHandle, EncryptedCurrentValue,
            EncryptedOldValue);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaSetTrustedDomainInfoByName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaSetTrustedDomainInfoByName(
    LSA_HANDLE policy,
    PLSA_UNICODE_STRING name,
    TRUSTED_INFORMATION_CLASS class,
    PVOID buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", policy, name, class, buffer);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaSetTrustedDomainInformation [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaSetTrustedDomainInformation(
    LSA_HANDLE policy,
    PSID sid,
    TRUSTED_INFORMATION_CLASS class,
    PVOID buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", policy, sid, class, buffer);
    return STATUS_SUCCESS;
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

/******************************************************************************
 * LsaUnregisterPolicyChangeNotification [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaUnregisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS class,
    HANDLE event)
{
    FIXME("(%d,%p) stub\n", class, event);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaLookupPrivilegeName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaLookupPrivilegeName(LSA_HANDLE handle, LUID *luid, LSA_UNICODE_STRING **name)
{
    const WCHAR *privnameW;
    DWORD length;
    WCHAR *strW;

    TRACE("(%p,%p,%p)\n", handle, luid, name);

    if (!luid || !handle)
        return STATUS_INVALID_PARAMETER;

    *name = NULL;

    if (!(privnameW = get_wellknown_privilege_name(luid)))
        return STATUS_NO_SUCH_PRIVILEGE;

    length = lstrlenW(privnameW);
    *name = malloc(sizeof(**name) + (length + 1) * sizeof(WCHAR));
    if (!*name)
        return STATUS_NO_MEMORY;

    strW = (WCHAR *)(*name + 1);
    memcpy(strW, privnameW, length * sizeof(WCHAR));
    strW[length] = 0;
    RtlInitUnicodeString(*name, strW);

    return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaLookupPrivilegeDisplayName [ADVAPI32.@]
 *
 */
NTSTATUS WINAPI LsaLookupPrivilegeDisplayName(LSA_HANDLE handle, LSA_UNICODE_STRING *name,
    LSA_UNICODE_STRING **display_name, SHORT *language)
{
    FIXME("(%p, %s, %p, %p)\n", handle, debugstr_us(name), display_name, language);

    return STATUS_NO_SUCH_PRIVILEGE;
}

/******************************************************************************
 * AuditQuerySystemPolicy [ADVAPI32.@]
 *
 */
BOOLEAN WINAPI AuditQuerySystemPolicy(const GUID* guids, ULONG count, AUDIT_POLICY_INFORMATION** policy)
{

    FIXME("(%p, %ld, %p)\n", guids, count, policy);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
