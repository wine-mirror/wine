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

static unsigned int page_size;

static NTSTATUS (WINAPI *pRtlCreateUserStack)(SIZE_T, SIZE_T, ULONG, SIZE_T, SIZE_T, INITIAL_TEB *);
static NTSTATUS (WINAPI *pRtlFreeUserStack)(void *);
static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static const BOOL is_win64 = sizeof(void*) != sizeof(int);

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs(&argv);
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "error: %u\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    return pi.hProcess;
}

static UINT_PTR get_zero_bits(UINT_PTR p)
{
    UINT_PTR z = 0;

#ifdef _WIN64
    if (p >= 0xffffffff)
        return (~(UINT_PTR)0) >> get_zero_bits(p >> 32);
#endif

    if (p == 0) return 0;
    while ((p >> (31 - z)) != 1) z++;
    return z;
}

static UINT_PTR get_zero_bits_mask(ULONG_PTR z)
{
    if (z >= 32)
    {
        z = get_zero_bits(z);
#ifdef _WIN64
        if (z >= 32) return z;
#endif
    }
    return (~(UINT32)0) >> z;
}

static void test_NtAllocateVirtualMemory(void)
{
    void *addr1, *addr2;
    NTSTATUS status;
    SIZE_T size;
    ULONG_PTR zero_bits;
    BOOL is_wow64;

    if (!pIsWow64Process || !pIsWow64Process(NtCurrentProcess(), &is_wow64)) is_wow64 = FALSE;

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

    /* 1 zero bits should zero 63-31 upper bits */
    size = 0x1000;
    addr2 = NULL;
    zero_bits = 1;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, zero_bits, &size,
                                     MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                                     PAGE_READWRITE);
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
                                         MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                                         PAGE_READWRITE);
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY ||
           broken(zero_bits == 20 && status == STATUS_CONFLICTING_ADDRESSES) /* w1064v1809 */,
           "NtAllocateVirtualMemory with %d zero_bits returned %08x\n", (int)zero_bits, status);
        if (status == STATUS_SUCCESS)
        {
            ok(((UINT_PTR)addr2 >> (32 - zero_bits)) == 0,
               "NtAllocateVirtualMemory with %d zero_bits returned address %p\n", (int)zero_bits, addr2);

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

    /* zero bits > 31 should be considered as a leading zeroes bitmask on 64bit and WoW64 */
    size = 0x1000;
    addr2 = NULL;
    zero_bits = 0x1aaaaaaa;
    status = NtAllocateVirtualMemory(NtCurrentProcess(), &addr2, zero_bits, &size,
                                      MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                                      PAGE_READWRITE);

    if (!is_win64 && !is_wow64)
    {
        ok(status == STATUS_INVALID_PARAMETER_3, "NtAllocateVirtualMemory returned %08x\n", status);
    }
    else
    {
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY,
           "NtAllocateVirtualMemory returned %08x\n", status);
        if (status == STATUS_SUCCESS)
        {
            ok(((UINT_PTR)addr2 & ~get_zero_bits_mask(zero_bits)) == 0 &&
               ((UINT_PTR)addr2 & ~zero_bits) != 0, /* only the leading zeroes matter */
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

static void test_RtlCreateUserStack(void)
{
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
    SIZE_T default_commit = nt->OptionalHeader.SizeOfStackCommit;
    SIZE_T default_reserve = nt->OptionalHeader.SizeOfStackReserve;
    INITIAL_TEB stack = {0};
    unsigned int i;
    NTSTATUS ret;

    struct
    {
        SIZE_T commit, reserve, commit_align, reserve_align, expect_commit, expect_reserve;
    }
    tests[] =
    {
        {       0,        0,      1,        1, default_commit, default_reserve},
        {  0x2000,        0,      1,        1,         0x2000, default_reserve},
        {  0x4000,        0,      1,        1,         0x4000, default_reserve},
        {       0, 0x200000,      1,        1, default_commit, 0x200000},
        {  0x4000, 0x200000,      1,        1,         0x4000, 0x200000},
        {0x100000, 0x100000,      1,        1,       0x100000, 0x100000},
        { 0x20000,  0x20000,      1,        1,        0x20000, 0x100000},

        {       0, 0x110000,      1,        1, default_commit, 0x110000},
        {       0, 0x110000,      1,  0x40000, default_commit, 0x140000},
        {       0, 0x140000,      1,  0x40000, default_commit, 0x140000},
        { 0x11000, 0x140000,      1,  0x40000,        0x11000, 0x140000},
        { 0x11000, 0x140000, 0x4000,  0x40000,        0x14000, 0x140000},
        {       0,        0, 0x4000, 0x400000,
                (default_commit + 0x3fff) & ~0x3fff,
                (default_reserve + 0x3fffff) & ~0x3fffff},
    };

    if (!pRtlCreateUserStack)
    {
        win_skip("RtlCreateUserStack() is missing\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&stack, 0xcc, sizeof(stack));
        ret = pRtlCreateUserStack(tests[i].commit, tests[i].reserve, 0,
                tests[i].commit_align, tests[i].reserve_align, &stack);
        ok(!ret, "%u: got status %#x\n", i, ret);
        ok(!stack.OldStackBase, "%u: got OldStackBase %p\n", i, stack.OldStackBase);
        ok(!stack.OldStackLimit, "%u: got OldStackLimit %p\n", i, stack.OldStackLimit);
        ok(!((ULONG_PTR)stack.DeallocationStack & (page_size - 1)),
                "%u: got unaligned memory %p\n", i, stack.DeallocationStack);
        ok((ULONG_PTR)stack.StackBase - (ULONG_PTR)stack.DeallocationStack == tests[i].expect_reserve,
                "%u: got reserve %#lx\n", i, (ULONG_PTR)stack.StackBase - (ULONG_PTR)stack.DeallocationStack);
        todo_wine ok((ULONG_PTR)stack.StackBase - (ULONG_PTR)stack.StackLimit == tests[i].expect_commit,
                "%u: got commit %#lx\n", i, (ULONG_PTR)stack.StackBase - (ULONG_PTR)stack.StackLimit);
        pRtlFreeUserStack(stack.DeallocationStack);
    }

    ret = pRtlCreateUserStack(0x11000, 0x110000, 0, 1, 0, &stack);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);

    ret = pRtlCreateUserStack(0x11000, 0x110000, 0, 0, 1, &stack);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret);
}

static void test_NtMapViewOfSection(void)
{
    static const char testfile[] = "testfile.xxx";
    static const char data[] = "test data for NtMapViewOfSection";
    char buffer[sizeof(data)];
    HANDLE file, mapping, process;
    void *ptr, *ptr2;
    BOOL is_wow64, ret;
    DWORD status, written;
    SIZE_T size, result;
    LARGE_INTEGER offset;
    ULONG_PTR zero_bits;

    if (!pIsWow64Process || !pIsWow64Process(NtCurrentProcess(), &is_wow64)) is_wow64 = FALSE;

    file = CreateFileA(testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create test file\n");
    WriteFile(file, data, sizeof(data), &written, NULL);
    SetFilePointer(file, 4096, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    /* read/write mapping */

    mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, 4096, NULL);
    ok(mapping != 0, "CreateFileMapping failed\n");

    process = create_target_process("sleep");
    ok(process != NULL, "Can't start process\n");

    ptr = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_SUCCESS, "NtMapViewOfSection returned %08x\n", status);
    ok(!((ULONG_PTR)ptr & 0xffff), "returned memory %p is not aligned to 64k\n", ptr);

    ret = ReadProcessMemory(process, ptr, buffer, sizeof(buffer), &result);
    ok(ret, "ReadProcessMemory failed\n");
    ok(result == sizeof(buffer), "ReadProcessMemory didn't read all data (%lx)\n", result);
    ok(!memcmp(buffer, data, sizeof(buffer)), "Wrong data read\n");

    /* 1 zero bits should zero 63-31 upper bits */
    ptr2 = NULL;
    size = 0;
    zero_bits = 1;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, zero_bits, 0, &offset, &size, 1, MEM_TOP_DOWN, PAGE_READWRITE);
    ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY,
       "NtMapViewOfSection returned %08x\n", status);
    if (status == STATUS_SUCCESS)
    {
        ok(((UINT_PTR)ptr2 >> (32 - zero_bits)) == 0,
           "NtMapViewOfSection returned address: %p\n", ptr2);

        status = NtUnmapViewOfSection(process, ptr2);
        ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);
    }

    for (zero_bits = 2; zero_bits <= 20; zero_bits++)
    {
        ptr2 = NULL;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, zero_bits, 0, &offset, &size, 1, MEM_TOP_DOWN, PAGE_READWRITE);
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY,
           "NtMapViewOfSection with %d zero_bits returned %08x\n", (int)zero_bits, status);
        if (status == STATUS_SUCCESS)
        {
            ok(((UINT_PTR)ptr2 >> (32 - zero_bits)) == 0,
               "NtMapViewOfSection with %d zero_bits returned address %p\n", (int)zero_bits, ptr2);

            status = NtUnmapViewOfSection(process, ptr2);
            ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);
        }
    }

    /* 21 zero bits never succeeds */
    ptr2 = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, 21, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_NO_MEMORY || status == STATUS_INVALID_PARAMETER,
       "NtMapViewOfSection returned %08x\n", status);

    /* 22 zero bits is invalid */
    ptr2 = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, 22, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_INVALID_PARAMETER_4 || status == STATUS_INVALID_PARAMETER,
       "NtMapViewOfSection returned %08x\n", status);

    /* zero bits > 31 should be considered as a leading zeroes bitmask on 64bit and WoW64 */
    ptr2 = NULL;
    size = 0;
    zero_bits = 0x1aaaaaaa;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, zero_bits, 0, &offset, &size, 1, MEM_TOP_DOWN, PAGE_READWRITE);

    if (!is_win64 && !is_wow64)
    {
        ok(status == STATUS_INVALID_PARAMETER_4, "NtMapViewOfSection returned %08x\n", status);
    }
    else
    {
        ok(status == STATUS_SUCCESS || status == STATUS_NO_MEMORY,
           "NtMapViewOfSection returned %08x\n", status);
        if (status == STATUS_SUCCESS)
        {
            ok(((UINT_PTR)ptr2 & ~get_zero_bits_mask(zero_bits)) == 0 &&
               ((UINT_PTR)ptr2 & ~zero_bits) != 0, /* only the leading zeroes matter */
               "NtMapViewOfSection returned address %p\n", ptr2);

            status = NtUnmapViewOfSection(process, ptr2);
            ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);
        }
    }

    /* mapping at the same page conflicts */
    ptr2 = ptr;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_CONFLICTING_ADDRESSES, "NtMapViewOfSection returned %08x\n", status);

    /* offset has to be aligned */
    ptr2 = ptr;
    size = 0;
    offset.QuadPart = 1;
    status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_MAPPED_ALIGNMENT, "NtMapViewOfSection returned %08x\n", status);

    /* ptr has to be aligned */
    ptr2 = (char *)ptr + 42;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_MAPPED_ALIGNMENT, "NtMapViewOfSection returned %08x\n", status);

    /* still not 64k aligned */
    ptr2 = (char *)ptr + 0x1000;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_MAPPED_ALIGNMENT, "NtMapViewOfSection returned %08x\n", status);

    /* when an address is passed, it has to satisfy the provided number of zero bits */
    ptr2 = (char *)ptr + 0x1000;
    size = 0;
    offset.QuadPart = 0;
    zero_bits = get_zero_bits(((UINT_PTR)ptr2) >> 1);
    status = NtMapViewOfSection(mapping, process, &ptr2, zero_bits, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_INVALID_PARAMETER_4 || status == STATUS_INVALID_PARAMETER,
       "NtMapViewOfSection returned %08x\n", status);

    ptr2 = (char *)ptr + 0x1000;
    size = 0;
    offset.QuadPart = 0;
    zero_bits = get_zero_bits((UINT_PTR)ptr2);
    status = NtMapViewOfSection(mapping, process, &ptr2, zero_bits, 0, &offset, &size, 1, 0, PAGE_READWRITE);
    ok(status == STATUS_MAPPED_ALIGNMENT, "NtMapViewOfSection returned %08x\n", status);

    if (!is_win64 && !is_wow64)
    {
        /* new memory region conflicts with previous mapping */
        ptr2 = ptr;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        ok(status == STATUS_CONFLICTING_ADDRESSES, "NtMapViewOfSection returned %08x\n", status);

        ptr2 = (char *)ptr + 42;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        ok(status == STATUS_CONFLICTING_ADDRESSES, "NtMapViewOfSection returned %08x\n", status);

        /* in contrary to regular NtMapViewOfSection, only 4kb align is enforced */
        ptr2 = (char *)ptr + 0x1000;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        ok(status == STATUS_SUCCESS, "NtMapViewOfSection returned %08x\n", status);
        ok((char *)ptr2 == (char *)ptr + 0x1000,
           "expected address %p, got %p\n", (char *)ptr + 0x1000, ptr2);
        status = NtUnmapViewOfSection(process, ptr2);
        ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);

        /* the address is rounded down if not on a page boundary */
        ptr2 = (char *)ptr + 0x1001;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        ok(status == STATUS_SUCCESS, "NtMapViewOfSection returned %08x\n", status);
        ok((char *)ptr2 == (char *)ptr + 0x1000,
           "expected address %p, got %p\n", (char *)ptr + 0x1000, ptr2);
        status = NtUnmapViewOfSection(process, ptr2);
        ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);

        ptr2 = (char *)ptr + 0x2000;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        ok(status == STATUS_SUCCESS, "NtMapViewOfSection returned %08x\n", status);
        ok((char *)ptr2 == (char *)ptr + 0x2000,
           "expected address %p, got %p\n", (char *)ptr + 0x2000, ptr2);
        status = NtUnmapViewOfSection(process, ptr2);
        ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);
    }
    else
    {
        ptr2 = (char *)ptr + 0x1000;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection(mapping, process, &ptr2, 0, 0, &offset,
                                    &size, 1, AT_ROUND_TO_PAGE, PAGE_READWRITE);
        todo_wine
        ok(status == STATUS_INVALID_PARAMETER_9 || status == STATUS_INVALID_PARAMETER,
           "NtMapViewOfSection returned %08x\n", status);
    }

    status = NtUnmapViewOfSection(process, ptr);
    ok(status == STATUS_SUCCESS, "NtUnmapViewOfSection returned %08x\n", status);

    NtClose(mapping);

    CloseHandle(file);
    DeleteFileA(testfile);

    TerminateProcess(process, 0);
    CloseHandle(process);
}

START_TEST(virtual)
{
    SYSTEM_BASIC_INFORMATION sbi;
    HMODULE mod;

    int argc;
    char **argv;
    argc = winetest_get_mainargs(&argv);

    if (argc >= 3)
    {
        if (!strcmp(argv[2], "sleep"))
        {
            Sleep(5000); /* spawned process runs for at most 5 seconds */
            return;
        }
        return;
    }

    mod = GetModuleHandleA("kernel32.dll");
    pIsWow64Process = (void *)GetProcAddress(mod, "IsWow64Process");

    mod = GetModuleHandleA("ntdll.dll");
    pRtlCreateUserStack = (void *)GetProcAddress(mod, "RtlCreateUserStack");
    pRtlFreeUserStack = (void *)GetProcAddress(mod, "RtlFreeUserStack");

    NtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    trace("system page size %#x\n", sbi.PageSize);
    page_size = sbi.PageSize;

    test_NtAllocateVirtualMemory();
    test_RtlCreateUserStack();
    test_NtMapViewOfSection();
}
