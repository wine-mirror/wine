/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * netapi32 user functions
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

#include "winbase.h"
#include "nb30.h"
#include "lmwksta.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winerror.h"
#include "winternl.h"
#include "ntsecapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NetWkstaUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetWkstaUserGetInfo(LPWSTR reserved, DWORD level,
                                          PBYTE* bufptr)
{
    TRACE("(%s, %ld, %p): stub\n", debugstr_w(reserved), level, bufptr);
    switch (level)
    {
    case 0:
    {
        PWKSTA_USER_INFO_0 ui;
        DWORD dwSize = UNLEN + 1;

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_0) + dwSize * sizeof(WCHAR),
                             (LPVOID *) &bufptr);

        ui = (WKSTA_USER_INFO_0 *) *bufptr;
        ui->wkui0_username = (LPWSTR) (*bufptr + sizeof(WKSTA_USER_INFO_0));

        /* get data */
        if (GetUserNameW(ui->wkui0_username, &dwSize))
            return ERROR_NOT_ENOUGH_MEMORY;
        else
            return NERR_Success;
    }
    case 1:
    {
        PWKSTA_USER_INFO_1 ui;
        DWORD dwSize;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_HANDLE PolicyHandle;
        PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
        NTSTATUS NtStatus;

        /* sizes of the field buffers in WCHARS */
        int username_sz, logon_domain_sz, oth_domains_sz, logon_server_sz;

        FIXME("Level 1 processing is partially implemented\n");
        username_sz = UNLEN + 1;
        oth_domains_sz = 1;
        logon_server_sz = 1;
        /* get some information first to estimate size of the buffer */
        ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
        NtStatus = LsaOpenPolicy(NULL, &ObjectAttributes,
                                 POLICY_VIEW_LOCAL_INFORMATION,
                                 &PolicyHandle);
        if (NtStatus != STATUS_SUCCESS)
        {
            ERR("LsaOpenPolicyFailed with NT status %lx\n",
                LsaNtStatusToWinError(NtStatus));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation,
                                  (PVOID*) &DomainInfo);
        logon_domain_sz = lstrlenW(DomainInfo->DomainName.Buffer) + 1;
        LsaClose(PolicyHandle);

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1) +
                             (username_sz + logon_domain_sz +
                              oth_domains_sz + logon_server_sz) * sizeof(WCHAR),
                             (LPVOID *) &bufptr);
        ui = (WKSTA_USER_INFO_1 *) *bufptr;
        ui->wkui1_username = (LPWSTR) (*bufptr + sizeof(WKSTA_USER_INFO_1));
        ui->wkui1_logon_domain = (LPWSTR) (
            ((PBYTE) ui->wkui1_username) + username_sz * sizeof(WCHAR));
        ui->wkui1_oth_domains = (LPWSTR) (
            ((PBYTE) ui->wkui1_logon_domain) +
            logon_domain_sz * sizeof(WCHAR));
        ui->wkui1_logon_server = (LPWSTR) (
            ((PBYTE) ui->wkui1_oth_domains) +
            oth_domains_sz * sizeof(WCHAR));

        /* get data */
        dwSize = username_sz;
        if (GetUserNameW(ui->wkui1_username, &dwSize))
            return ERROR_NOT_ENOUGH_MEMORY;
        lstrcpynW(ui->wkui1_logon_domain, DomainInfo->DomainName.Buffer,
                logon_domain_sz);
        /* FIXME. Not implemented. Populated with empty strings */
        ui->wkui1_oth_domains[0] = 0;
        ui->wkui1_logon_server[0] = 0;
        return NERR_Success;
    }
    case 1101:
    {
        PWKSTA_USER_INFO_1101 ui;
        DWORD dwSize = 1;

        FIXME("Stub. Level 1101 processing is not implemented\n");
        /* FIXME see also wkui1_oth_domains for level 1 */

        /* set up buffer */
        NetApiBufferAllocate(sizeof(WKSTA_USER_INFO_1101) + dwSize * sizeof(WCHAR),
                             (LPVOID *) &bufptr);

        ui = (PWKSTA_USER_INFO_1101) *bufptr;
        ui->wkui1101_oth_domains = (LPWSTR)(ui + 1);

        /* get data */
        ui->wkui1101_oth_domains[0] = 0;
        return NERR_Success;
    }
    default:
        ERR("Invalid level %ld is specified\n", level);
        return ERROR_INVALID_LEVEL;
    }
}

/************************************************************
 *                NetpGetComputerName  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetpGetComputerName(LPWSTR *Buffer)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    TRACE("(%p)\n", Buffer);
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) Buffer);
    return !GetComputerNameW(*Buffer,  &dwSize);
}
