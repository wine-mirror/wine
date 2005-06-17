/* Unit test suite for *Information* Registry API functions
 *
 * Copyright 2005 Paul Vriens
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
 *
 */

#include "ntdll_test.h"

static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI * pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

static HMODULE hntdll = 0;

#define NTDLL_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(hntdll); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    if(!hntdll) {
      trace("Could not load ntdll.dll\n");
      return FALSE;
    }
    if (hntdll)
    {
      NTDLL_GET_PROC(NtQuerySystemInformation)
      NTDLL_GET_PROC(NtQueryInformationProcess)
    }
    return TRUE;
}

static void test_query_basic()
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_BASIC_INFORMATION sbi;

    /* This test also covers some basic parameter testing that should be the same for 
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQuerySystemInformation(-1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status);

    /* Use an existing class but with a zero-length buffer */
    trace("Check zero-length buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use an existing class, correct length but no SystemInformation buffer */
    trace("Check no SystemInformation buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, sizeof(sbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    /* Use a existing class, correct length, a pointer to a buffer but no ReturnLength pointer */
    trace("Check no ReturnLength pointer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check a too large buffer size */
    trace("Check a too large buffer size\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi) * 2, &ReturnLength);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sbi), ReturnLength);

    /* Check if we have some return values */
    trace("Number of Processors : %d\n", sbi.NumberOfProcessors);
    ok( sbi.NumberOfProcessors > 0, "Expected more than 0 processors, got %d\n", sbi.NumberOfProcessors);
}

static void test_query_cpu()
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_CPU_INFORMATION sci;

    status = pNtQuerySystemInformation(SystemCpuInformation, &sci, sizeof(sci), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sci) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sci), ReturnLength);
                                                                                                                       
    /* Check if we have some return values */
    trace("Processor FeatureSet : %08lx\n", sci.FeatureSet);
    ok( sci.FeatureSet != 0, "Expected some features for this processor, got %08lx\n", sci.FeatureSet);
}

static void test_query_timeofday()
{
    DWORD status;
    ULONG ReturnLength;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE {
        LARGE_INTEGER liKeBootTime;
        LARGE_INTEGER liKeSystemTime;
        LARGE_INTEGER liExpTimeZoneBias;
        ULONG uCurrentTimeZoneId;
        DWORD dwUnknown1[5];
    } SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE, *PSYSTEM_TIMEOFDAY_INFORMATION_PRIVATE;

    SYSTEM_TIMEOFDAY_INFORMATION_PRIVATE sti;
  
    /*  The struct size for NT (32 bytes) and Win2K/XP (48 bytes) differ.
     *
     *  Windows 2000 and XP return STATUS_INFO_LENGTH_MISMATCH if the given buffer size is greater
     *  then 48 and 0 otherwise
     *  Windows NT returns STATUS_INFO_LENGTH_MISMATCH when the given buffer size is not correct
     *  and 0 otherwise
     *
     *  Windows 2000 and XP copy the given buffer size into the provided buffer, if the return code is STATUS_SUCCESS
     *  NT only fills the buffer if the return code is STATUS_SUCCESS
     *
    */

    status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        trace("Windows version is NT, we have to cater for differences with W2K/WinXP\n");
 
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 0, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 28, &ReturnLength);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");

        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);
    }
    else
    {
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 0, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);

        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 24, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 24 == ReturnLength, "ReturnLength should be 24, it is (%ld)\n", ReturnLength);
        ok( 0xdeadbeef == sti.uCurrentTimeZoneId, "This part of the buffer should not have been filled\n");
    
        sti.uCurrentTimeZoneId = 0xdeadbeef;
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 32, &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( 32 == ReturnLength, "ReturnLength should be 32, it is (%ld)\n", ReturnLength);
        ok( 0xdeadbeef != sti.uCurrentTimeZoneId, "Buffer should have been partially filled\n");
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, 49, &ReturnLength);
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
        ok( 0 == ReturnLength, "ReturnLength should be 0, it is (%ld)\n", ReturnLength);
    
        status = pNtQuerySystemInformation(SystemTimeOfDayInformation, &sti, sizeof(sti), &ReturnLength);
        ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
        ok( sizeof(sti) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(sti), ReturnLength);
    }

    /* Check if we have some return values */
    trace("uCurrentTimeZoneId : (%ld)\n", sti.uCurrentTimeZoneId);
}

static void test_query_process()
{
    DWORD status;
    ULONG ReturnLength;
    int i = 0, j = 0, k = 0;
    int is_nt = 0;
    SYSTEM_BASIC_INFORMATION sbi;

    /* Copy of our winternl.h structure turned into a private one */
    typedef struct _SYSTEM_PROCESS_INFORMATION_PRIVATE {
        DWORD dwOffset;
        DWORD dwThreadCount;
        DWORD dwUnknown1[6];
        FILETIME ftCreationTime;
        FILETIME ftUserTime;
        FILETIME ftKernelTime;
        UNICODE_STRING ProcessName;
        DWORD dwBasePriority;
        DWORD dwProcessID;
        DWORD dwParentProcessID;
        DWORD dwHandleCount;
        DWORD dwUnknown3;
        DWORD dwUnknown4;
        VM_COUNTERS vmCounters;
        IO_COUNTERS ioCounters;
        SYSTEM_THREAD_INFORMATION ti[1];
    } SYSTEM_PROCESS_INFORMATION_PRIVATE, *PSYSTEM_PROCESS_INFORMATION_PRIVATE;

    ULONG SystemInformationLength = sizeof(SYSTEM_PROCESS_INFORMATION_PRIVATE);
    SYSTEM_PROCESS_INFORMATION_PRIVATE* spi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* Only W2K3 returns the needed length, the rest returns 0, so we have to loop */

    for (;;)
    {
        status = pNtQuerySystemInformation(SystemProcessInformation, spi, SystemInformationLength, &ReturnLength);

        if (status != STATUS_INFO_LENGTH_MISMATCH) break;
        
        spi = HeapReAlloc(GetProcessHeap(), 0, spi , SystemInformationLength *= 2);
    }

    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Get the first dwOffset, from this we can deduce the OS version we're running
     *
     * W2K/WinXP/W2K3:
     *   dwOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * NT:
     *   dwOffset for a process is 136 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION)
     * Wine (with every windows version):
     *   dwOffset for a process is 0 if just this test is running
     *   dwOffset for a process is 184 + (no. of threads) * sizeof(SYSTEM_THREAD_INFORMATION) +
     *                             ProcessName.MaximumLength
     *     if more wine processes are running
     *
     * Note : On windows the first process is in fact the Idle 'process' with a thread for every processor
    */

    pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);

    is_nt = ( spi->dwOffset - (sbi.NumberOfProcessors * sizeof(SYSTEM_THREAD_INFORMATION)) == 136);

    if (is_nt) trace("Windows version is NT, we will skip thread tests\n");

    /* Check if we have some return values
     * 
     * On windows there will be several processes running (Including the always present Idle and System)
     * On wine we only have one (if this test is the only wine process running)
    */
    
    /* Loop through the processes */

    for (;;)
    {
        i++;

        ok( spi->dwThreadCount > 0, "Expected some threads for this process, got 0\"");

        /* Loop through the threads, skip NT4 for now */
        
        if (!is_nt)
        {
            for ( j = 0; j < spi->dwThreadCount; j++) 
            {
                k++;
                ok ( spi->ti[j].dwOwningPID == spi->dwProcessID, 
                     "The owning pid of the thread (%ld) doesn't equal the pid (%ld) of the process\n",
                     spi->ti[j].dwOwningPID, spi->dwProcessID);
            }
        }

        if (!spi->dwOffset) break;
                                                                                                                              
        spi = (SYSTEM_PROCESS_INFORMATION_PRIVATE*)((char*)spi + spi->dwOffset);
    }
    trace("Total number of running processes : %d\n", i);
    if (!is_nt) trace("Total number of running threads   : %d\n", k);

    HeapFree( GetProcessHeap(), 0, spi);
}

static void test_query_handle()
{
    DWORD status;
    ULONG ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION);
    SYSTEM_HANDLE_INFORMATION* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* Request the needed length : a SystemInformationLength greater than one struct sets ReturnLength */
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);

    /* The following check assumes more than one handle on any given system */
    todo_wine
    {
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    }
    ok( ReturnLength > 0, "Expected ReturnLength to be > 0, it was %ld\n", ReturnLength);

    SystemInformationLength = ReturnLength;
    shi = HeapReAlloc(GetProcessHeap(), 0, shi , SystemInformationLength);
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check if we have some return values */
    trace("Number of Handles : %ld\n", shi->Count);
    todo_wine
    {
        /* our implementation is a stub for now */
        ok( shi->Count > 1, "Expected more than 1 handles, got (%ld)\n", shi->Count);
    }

    HeapFree( GetProcessHeap(), 0, shi);
}

static void test_query_process_basic()
{
    DWORD status;
    ULONG ReturnLength;

    typedef struct _PROCESS_BASIC_INFORMATION_PRIVATE {
        DWORD ExitStatus;
        DWORD PebBaseAddress;
        DWORD AffinityMask;
        DWORD BasePriority;
        ULONG UniqueProcessId;
        ULONG InheritedFromUniqueProcessId;
    } PROCESS_BASIC_INFORMATION_PRIVATE, *PPROCESS_BASIC_INFORMATION_PRIVATE;

    PROCESS_BASIC_INFORMATION_PRIVATE pbi;

    /* This test also covers some basic parameter testing that should be the same for
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQueryInformationProcess(NULL, -1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status);

    /* Do not give a handle and buffer */
    trace("Check NULL handle and buffer and zero-length buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use a correct info class and buffer size, but still no handle and buffer */
    trace("Check NULL handle and buffer\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, NULL, sizeof(pbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_HANDLE(W2K3), got %08lx\n", status);

    /* Use a correct info class and buffer size, but still no handle */
    trace("Check NULL handle\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %08lx\n", status);

    /* Use a greater buffer size */
    trace("Check NULL handle and too large buffersize\n");
    status = pNtQueryInformationProcess(NULL, ProcessBasicInformation, &pbi, sizeof(pbi) * 2, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use no ReturnLength */
    trace("Check NULL ReturnLength\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(pbi) == ReturnLength, "Inconsistent length (%d) <-> (%ld)\n", sizeof(pbi), ReturnLength);
                                                                                                                                               
    /* Check if we have some return values */
    trace("ProcessID : %ld\n", pbi.UniqueProcessId);
    ok( pbi.UniqueProcessId > 0, "Expected a ProcessID > 0, got 0, got %ld\n", pbi.UniqueProcessId);
}

START_TEST(info)
{
    if(!InitFunctionPtrs())
        return;

    /* NtQuerySystemInformation */

    /* 0x0 SystemBasicInformation */
    trace("Starting test_query_basic()\n");
    test_query_basic();

    /* 0x1 SystemCpuInformation */
    trace("Starting test_query_cpu()\n");
    test_query_cpu();

    /* 0x3 SystemCpuInformation */
    trace("Starting test_query_timeofday()\n");
    test_query_timeofday();

    /* 0x5 SystemProcessInformation */
    trace("Starting test_query_process()\n");
    test_query_process();

    /* 0x10 SystemHandleInformation */
    trace("Starting test_query_handle()\n");
    test_query_handle();

    /* NtQueryInformationProcess */

    trace("Starting test_query_process_basic()\n");
    test_query_process_basic();

    FreeLibrary(hntdll);
}
