/*
 * Copyright 2001 Mike McCormack
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2003 Juan Lang
 * Copyright 2005,2006 Paul Vriens
 * Copyright 2006 Robert Reif
 * Copyright 2013 Hans Leidekker for CodeWeavers
 * Copyright 2020 Dmitry Timoshkov
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
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"

#include "dsrole.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsrole);

/************************************************************
 *  DsRoleFreeMemory (dsrole.@)
 */
void WINAPI DsRoleFreeMemory(void *buffer)
{
    TRACE("(%p)\n", buffer);
    HeapFree(GetProcessHeap(), 0, buffer);
}

/************************************************************
 *  DsRoleGetPrimaryDomainInformation (dsrole.@)
 */
DWORD WINAPI DsRoleGetPrimaryDomainInformation(
    LPCWSTR lpServer, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PBYTE* Buffer)
{
    DWORD ret;

    FIXME("(%p, %d, %p) stub\n", lpServer, InfoLevel, Buffer);

    /* Check some input parameters */

    if (!Buffer) return ERROR_INVALID_PARAMETER;
    if ((InfoLevel < DsRolePrimaryDomainInfoBasic) || (InfoLevel > DsRoleOperationState)) return ERROR_INVALID_PARAMETER;

    *Buffer = NULL;
    switch (InfoLevel)
    {
        case DsRolePrimaryDomainInfoBasic:
        {
            LSA_OBJECT_ATTRIBUTES ObjectAttributes;
            LSA_HANDLE PolicyHandle;
            PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
            NTSTATUS NtStatus;
            int logon_domain_sz;
            DWORD size;
            PDSROLE_PRIMARY_DOMAIN_INFO_BASIC basic;

            ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
            NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
             POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
            if (NtStatus != STATUS_SUCCESS)
            {
                TRACE("LsaOpenPolicyFailed with NT status %lx\n",
                    LsaNtStatusToWinError(NtStatus));
                return ERROR_OUTOFMEMORY;
            }
            LsaQueryInformationPolicy(PolicyHandle,
             PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
            logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
            LsaClose(PolicyHandle);

            size = sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC) +
             logon_domain_sz * sizeof(WCHAR);
            basic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            if (basic)
            {
                basic->MachineRole = DsRole_RoleStandaloneWorkstation;
                basic->DomainNameFlat = (LPWSTR)((LPBYTE)basic +
                 sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
                lstrcpyW(basic->DomainNameFlat, DomainInfo->DomainName.Buffer);
                ret = ERROR_SUCCESS;
            }
            else
                ret = ERROR_OUTOFMEMORY;
            *Buffer = (PBYTE)basic;
            LsaFreeMemory(DomainInfo);
        }
        break;
    default:
        ret = ERROR_CALL_NOT_IMPLEMENTED;
    }
    return ret;
}
