/*
 * Unit tests for lsa functions
 *
 * Copyright (c) 2006 Robert Reif
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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "ntsecapi.h"
#define INITGUID
#include "guiddef.h"
#include "wine/test.h"

static HMODULE hadvapi32;
static NTSTATUS (WINAPI *pLsaClose)(LSA_HANDLE);
static NTSTATUS (WINAPI *pLsaFreeMemory)(PVOID);
static NTSTATUS (WINAPI *pLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
static NTSTATUS (WINAPI *pLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);

static BOOL init(void)
{
    hadvapi32 = GetModuleHandle("advapi32.dll");

    if (hadvapi32) {
        pLsaClose = (void*)GetProcAddress(hadvapi32, "LsaClose");
        pLsaFreeMemory = (void*)GetProcAddress(hadvapi32, "LsaFreeMemory");
        pLsaOpenPolicy = (void*)GetProcAddress(hadvapi32, "LsaOpenPolicy");
        pLsaQueryInformationPolicy = (void*)GetProcAddress(hadvapi32, "LsaQueryInformationPolicy");

        if (pLsaClose && pLsaFreeMemory && pLsaOpenPolicy && pLsaQueryInformationPolicy)
            return TRUE;
    }

    return FALSE;
}

static void test_lsa(void)
{
    NTSTATUS status;
    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES object_attributes;

    ZeroMemory(&object_attributes, sizeof(object_attributes));
    object_attributes.Length = sizeof(object_attributes);

    status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
       "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08lx\n", status);

    /* try a more restricted access mask if necessary */
    if (status == STATUS_ACCESS_DENIED) {
        trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
        status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION, &handle);
        ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08lx\n", status);
    }

    if (status == STATUS_SUCCESS) {
        PPOLICY_AUDIT_EVENTS_INFO audit_events_info;
        PPOLICY_PRIMARY_DOMAIN_INFO primary_domain_info;
        PPOLICY_ACCOUNT_DOMAIN_INFO account_domain_info;
        PPOLICY_DNS_DOMAIN_INFO dns_domain_info;

        status = pLsaQueryInformationPolicy(handle, PolicyAuditEventsInformation, (PVOID*)&audit_events_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyAuditEventsInformation) failed, returned 0x%08lx\n", status);
        if (status == STATUS_SUCCESS) {
            pLsaFreeMemory((LPVOID)audit_events_info);
        }

        status = pLsaQueryInformationPolicy(handle, PolicyPrimaryDomainInformation, (PVOID*)&primary_domain_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyPrimaryDomainInformation) failed, returned 0x%08lx\n", status);
        if (status == STATUS_SUCCESS) {
            ok(primary_domain_info->Sid==0,"Sid should be NULL on the local computer\n");
            pLsaFreeMemory((LPVOID)primary_domain_info);
        }

        status = pLsaQueryInformationPolicy(handle, PolicyAccountDomainInformation, (PVOID*)&account_domain_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyAccountDomainInformation) failed, returned 0x%08lx\n", status);
        if (status == STATUS_SUCCESS) {
            pLsaFreeMemory((LPVOID)account_domain_info);
        }

        status = pLsaQueryInformationPolicy(handle, PolicyDnsDomainInformation, (PVOID*)&dns_domain_info);
        ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy(PolicyDnsDomainInformation) failed, returned 0x%08lx\n", status);
        if (status == STATUS_SUCCESS) {
            ok(IsEqualGUID(&dns_domain_info->DomainGuid, &GUID_NULL), "DomainGUID should be GUID_NULL on local computer\n");
            ok(dns_domain_info->Sid==0,"Sid should be NULL on the local computer\n");
            pLsaFreeMemory((LPVOID)dns_domain_info);
        }

        status = pLsaClose(handle);
        ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08lx\n", status);
    }
}

START_TEST(lsa)
{
    if (!init())
        return;

    test_lsa();
}
