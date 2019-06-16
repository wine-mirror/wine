/*
 * Unit test suite for the virtual memory APIs.
 *
 * Copyright 2019 Remi Bernon for CodeWeavers
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

#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/test.h"

static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);

static void test_AllocateVirtualMemory(void)
{
    void *addr1, *addr2;
    NTSTATUS status;
    SIZE_T size;
    ULONG zero_bits;
    BOOL is_wow64;

    /* simple allocation should success */
    size = 0x1000;
    addr1 = NULL;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr1, 0, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(status == STATUS_SUCCESS, "NtAllocateVirtualMemory returned %08x\n", status);

    /* allocation conflicts because of 64k align */
    size = 0x1000;
    addr2 = (char *)addr1 + 0x1000;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 0, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(status == STATUS_CONFLICTING_ADDRESSES, "NtAllocateVirtualMemory returned %08x\n", status);

    /* it should conflict, even when zero_bits is explicitly set */
    size = 0x1000;
    addr2 = (char *)addr1 + 0x1000;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 12, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(status == STATUS_CONFLICTING_ADDRESSES, "NtAllocateVirtualMemory returned %08x\n", status);
    if (status == STATUS_SUCCESS)
    {
        size = 0;
        status = NtFreeVirtualMemory(NtCurrentProcess(), &addr2, &size, MEM_RELEASE);
        ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory return %08x, addr2: %p\n", status, addr2);
    }

    /* 1 zero bits should zero 63-31 upper bits */
    size = 0x1000;
    addr2 = NULL;
    zero_bits = 1;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 1, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY ||
       broken(status == STATUS_INVALID_PARAMETER_3) /* winxp */,
       "NtAllocateVirtualMemory returned %08x\n", status);
    if (status == STATUS_SUCCESS)
    {
        ok(((UINT_PTR)addr2 >> (32 - zero_bits)) == 0,
           "NtAllocateVirtualMemory returned address: %p\n", addr2);

        size = 0;
        status = NtFreeVirtualMemory(NtCurrentProcess(), &addr2, &size, MEM_RELEASE);
        ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory return %08x, addr2: %p\n", status, addr2);
    }

    for (zero_bits = 2; zero_bits <= 20; zero_bits++)
    {
        size = 0x1000;
        addr2 = NULL;
        status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, zero_bits, &size,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY ||
           broken(zero_bits == 20 && status == STATUS_CONFLICTING_ADDRESSES) /* w1064v1809 */,
           "NtAllocateVirtualMemory with %d zero_bits returned %08x\n", zero_bits, status);
        if (status == STATUS_SUCCESS)
        {
            todo_wine_if((UINT_PTR)addr2 >> (32 - zero_bits))
            ok(((UINT_PTR)addr2 >> (32 - zero_bits)) == 0,
               "NtAllocateVirtualMemory with %d zero_bits returned address %p\n", zero_bits, addr2);

            size = 0;
            status = NtFreeVirtualMemory(NtCurrentProcess(), &addr2, &size, MEM_RELEASE);
            ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory return %08x, addr2: %p\n", status, addr2);
        }
    }

    /* 21 zero bits never succeeds */
    size = 0x1000;
    addr2 = NULL;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 21, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    todo_wine
    ok(status == STATUS_NO_MEMORY || status == STATUS_INVALID_PARAMETER,
       "NtAllocateVirtualMemory returned %08x\n", status);
    if (status == STATUS_SUCCESS)
    {
        size = 0;
        status = NtFreeVirtualMemory(NtCurrentProcess(), &addr2, &size, MEM_RELEASE);
        ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory return %08x, addr2: %p\n", status, addr2);
    }

    /* 22 zero bits is invalid */
    size = 0x1000;
    addr2 = NULL;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 22, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(status == STATUS_INVALID_PARAMETER_3 || status == STATUS_INVALID_PARAMETER,
       "NtAllocateVirtualMemory returned %08x\n", status);

    /* zero bits > 31 should be considered as bitmask on 64bit and WoW64 */
    size = 0x1000;
    addr2 = NULL;
    zero_bits = 0x1fffffff;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, zero_bits, &size,
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (sizeof(void *) == sizeof(int) && (!pIsWow64Process ||
        !pIsWow64Process(NtCurrentProcess(), &is_wow64) || !is_wow64))
    {
        ok(status == STATUS_INVALID_PARAMETER_3, "NtAllocateVirtualMemory returned %08x\n", status);
    }
    else
    {
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY,
           "NtAllocateVirtualMemory returned %08x\n", status);
        if (status == STATUS_SUCCESS)
        {
            todo_wine_if((UINT_PTR)addr2 & ~zero_bits)
            ok(((UINT_PTR)addr2 & ~zero_bits) == 0,
               "NtAllocateVirtualMemory returned address %p\n", addr2);

            size = 0;
            status = NtFreeVirtualMemory(NtCurrentProcess(), &addr2, &size, MEM_RELEASE);
            ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory return %08x, addr2: %p\n", status, addr2);
        }
    }

    /* AT_ROUND_TO_PAGE flag is not supported for NtAllocateVirtualMemory */
    size = 0x1000;
    addr2 = (char *)addr1 + 0x1000;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, 0, &size,
                                     MEM_RESERVE | MEM_COMMIT | AT_ROUND_TO_PAGE, PAGE_EXECUTE_READWRITE);
    ok(status == STATUS_INVALID_PARAMETER_5 || status == STATUS_INVALID_PARAMETER,
       "NtAllocateVirtualMemory returned %08x\n", status);

    size = 0;
    status = NtFreeVirtualMemory(NtCurrentProcess(), &addr1, &size, MEM_RELEASE);
    ok(status == STATUS_SUCCESS, "NtFreeVirtualMemory failed\n");
}

START_TEST(virtual)
{
    SYSTEM_BASIC_INFORMATION sbi;
    HMODULE hkernel32;

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pIsWow64Process = (void *)GetProcAddress(hkernel32, "IsWow64Process");

    NtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    trace("system page size %#x\n", sbi.PageSize);

    test_AllocateVirtualMemory();
}
