/*
 * Unit test suite Wow64 functions
 *
 * Copyright 2021 Alexandre Julliard
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
 *
 */

#include "ntdll_test.h"
#include "winioctl.h"

static NTSTATUS (WINAPI *pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS,void*,ULONG,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pRtlGetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static USHORT   (WINAPI *pRtlWow64GetCurrentMachine)(void);
static NTSTATUS (WINAPI *pRtlWow64GetProcessMachines)(HANDLE,WORD*,WORD*);
static NTSTATUS (WINAPI *pRtlWow64GetThreadContext)(HANDLE,WOW64_CONTEXT*);
static NTSTATUS (WINAPI *pRtlWow64IsWowGuestMachineSupported)(USHORT,BOOLEAN*);
#ifdef _WIN64
static NTSTATUS (WINAPI *pRtlWow64GetCpuAreaInfo)(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
static NTSTATUS (WINAPI *pRtlWow64GetThreadSelectorEntry)(HANDLE,THREAD_DESCRIPTOR_INFORMATION*,ULONG,ULONG*);
#else
static NTSTATUS (WINAPI *pNtWow64AllocateVirtualMemory64)(HANDLE,ULONG64*,ULONG64,ULONG64*,ULONG,ULONG);
static NTSTATUS (WINAPI *pNtWow64GetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pNtWow64ReadVirtualMemory64)(HANDLE,ULONG64,void*,ULONG64,ULONG64*);
static NTSTATUS (WINAPI *pNtWow64WriteVirtualMemory64)(HANDLE,ULONG64,const void *,ULONG64,ULONG64*);
#endif

static BOOL is_wow64;
static void *code_mem;

static void init(void)
{
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );

    if (!IsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

#define GET_PROC(func) p##func = (void *)GetProcAddress( ntdll, #func )
    GET_PROC( NtQuerySystemInformation );
    GET_PROC( NtQuerySystemInformationEx );
    GET_PROC( RtlGetNativeSystemInformation );
    GET_PROC( RtlWow64GetCurrentMachine );
    GET_PROC( RtlWow64GetProcessMachines );
    GET_PROC( RtlWow64GetThreadContext );
    GET_PROC( RtlWow64IsWowGuestMachineSupported );
#ifdef _WIN64
    GET_PROC( RtlWow64GetCpuAreaInfo );
    GET_PROC( RtlWow64GetThreadSelectorEntry );
#else
    GET_PROC( NtWow64AllocateVirtualMemory64 );
    GET_PROC( NtWow64GetNativeSystemInformation );
    GET_PROC( NtWow64ReadVirtualMemory64 );
    GET_PROC( NtWow64WriteVirtualMemory64 );
#endif
#undef GET_PROC

    code_mem = VirtualAlloc( NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
}

static void test_process_architecture( HANDLE process, USHORT expect_machine, USHORT expect_native )
{
    NTSTATUS status;
    ULONG i, len, buffer[8];

    len = 0xdead;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          &buffer, sizeof(buffer), &len );
    ok( !status, "failed %x\n", status );
    ok( !(len & 3), "wrong len %x\n", len );
    len /= sizeof(DWORD);
    for (i = 0; i < len - 1; i++)
    {
        USHORT flags = HIWORD(buffer[i]);
        USHORT machine = LOWORD(buffer[i]);

        if (flags & 8)
            ok( machine == expect_machine, "wrong current machine %x\n", buffer[i]);
        else
            ok( machine != expect_machine, "wrong machine %x\n", buffer[i]);

        /* FIXME: not quite sure what the other flags mean,
         * observed on amd64 Windows: (flags & 7) == 7 for MACHINE_AMD64 and 2 for MACHINE_I386
         */
        if (flags & 4)
            ok( machine == expect_native, "wrong native machine %x\n", buffer[i]);
        else
            ok( machine != expect_native, "wrong machine %x\n", buffer[i]);
    }
    ok( !buffer[i], "missing terminating null\n" );

    len = i * sizeof(DWORD);
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          &buffer, len, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "failed %x\n", status );
    ok( len == (i + 1) * sizeof(DWORD), "wrong len %u\n", len );

    if (pRtlWow64GetProcessMachines)
    {
        USHORT current = 0xdead, native = 0xbeef;
        status = pRtlWow64GetProcessMachines( process, &current, &native );
        ok( !status, "failed %x\n", status );
        if (expect_machine == expect_native)
            ok( current == 0, "wrong current machine %x / %x\n", current, expect_machine );
        else
            ok( current == expect_machine, "wrong current machine %x / %x\n", current, expect_machine );
        ok( native == expect_native, "wrong native machine %x / %x\n", native, expect_native );
    }
}

static void test_query_architectures(void)
{
#ifdef __i386__
    USHORT current_machine = IMAGE_FILE_MACHINE_I386;
    USHORT native_machine = is_wow64 ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386;
#elif defined __x86_64__
    USHORT current_machine = IMAGE_FILE_MACHINE_AMD64;
    USHORT native_machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined __arm__
    USHORT current_machine = IMAGE_FILE_MACHINE_ARMNT;
    USHORT native_machine = is_wow64 ? IMAGE_FILE_MACHINE_ARM64 : IMAGE_FILE_MACHINE_ARMNT;
#elif defined __aarch64__
    USHORT current_machine = IMAGE_FILE_MACHINE_ARM64;
    USHORT native_machine = IMAGE_FILE_MACHINE_ARM64;
#else
    USHORT current_machine = 0;
    USHORT native_machine = 0;
#endif
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { sizeof(si) };
    NTSTATUS status;
    HANDLE process;
    ULONG len, buffer[8];

    if (!pNtQuerySystemInformationEx) return;

    process = GetCurrentProcess();
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          &buffer, sizeof(buffer), &len );
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "SystemSupportedProcessorArchitectures not supported\n" );
        return;
    }
    ok( !status, "failed %x\n", status );

    process = (HANDLE)0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          &buffer, sizeof(buffer), &len );
    ok( status == STATUS_INVALID_HANDLE, "failed %x\n", status );
    process = (HANDLE)0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, 3,
                                          &buffer, sizeof(buffer), &len );
    ok( status == STATUS_INVALID_PARAMETER || broken(status == STATUS_INVALID_HANDLE),
        "failed %x\n", status );
    process = GetCurrentProcess();
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, 3,
                                          &buffer, sizeof(buffer), &len );
    ok( status == STATUS_INVALID_PARAMETER || broken( status == STATUS_SUCCESS),
        "failed %x\n", status );
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, NULL, 0,
                                          &buffer, sizeof(buffer), &len );
    ok( status == STATUS_INVALID_PARAMETER, "failed %x\n", status );

    test_process_architecture( GetCurrentProcess(), current_machine, native_machine );
    test_process_architecture( 0, 0, native_machine );

    if (CreateProcessA( "C:\\Program Files\\Internet Explorer\\iexplore.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        test_process_architecture( pi.hProcess, native_machine, native_machine );
        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
    if (CreateProcessA( "C:\\Program Files (x86)\\Internet Explorer\\iexplore.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        test_process_architecture( pi.hProcess, IMAGE_FILE_MACHINE_I386, native_machine );
        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    if (pRtlWow64GetCurrentMachine)
    {
        USHORT machine = pRtlWow64GetCurrentMachine();
        ok( machine == current_machine, "wrong machine %x / %x\n", machine, current_machine );
    }
    if (pRtlWow64IsWowGuestMachineSupported)
    {
        BOOLEAN ret = 0xcc;
        status = pRtlWow64IsWowGuestMachineSupported( IMAGE_FILE_MACHINE_I386, &ret );
        ok( !status, "failed %x\n", status );
        ok( ret == (native_machine == IMAGE_FILE_MACHINE_AMD64 ||
                    native_machine == IMAGE_FILE_MACHINE_ARM64), "wrong result %u\n", ret );
        ret = 0xcc;
        status = pRtlWow64IsWowGuestMachineSupported( IMAGE_FILE_MACHINE_ARMNT, &ret );
        ok( !status, "failed %x\n", status );
        ok( ret == (native_machine == IMAGE_FILE_MACHINE_ARM64), "wrong result %u\n", ret );
        ret = 0xcc;
        status = pRtlWow64IsWowGuestMachineSupported( IMAGE_FILE_MACHINE_AMD64, &ret );
        ok( !status, "failed %x\n", status );
        ok( !ret, "wrong result %u\n", ret );
        ret = 0xcc;
        status = pRtlWow64IsWowGuestMachineSupported( IMAGE_FILE_MACHINE_ARM64, &ret );
        ok( !status, "failed %x\n", status );
        ok( !ret, "wrong result %u\n", ret );
        ret = 0xcc;
        status = pRtlWow64IsWowGuestMachineSupported( 0xdead, &ret );
        ok( !status, "failed %x\n", status );
        ok( !ret, "wrong result %u\n", ret );
    }
}

static void test_peb_teb(void)
{
    PROCESS_BASIC_INFORMATION proc_info;
    THREAD_BASIC_INFORMATION info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    NTSTATUS status;
    void *redir;
    SIZE_T res;
    TEB teb;
    PEB peb;
    TEB32 teb32;
    PEB32 peb32;
    RTL_USER_PROCESS_PARAMETERS params;
    RTL_USER_PROCESS_PARAMETERS32 params32;

    Wow64DisableWow64FsRedirection( &redir );

    if (CreateProcessA( "C:\\windows\\syswow64\\notepad.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        memset( &info, 0xcc, sizeof(info) );
        status = NtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
        ok( !status, "ThreadBasicInformation failed %x\n", status );
        if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
        ok( res == sizeof(teb), "wrong len %lx\n", res );
        ok( teb.Tib.Self == info.TebBaseAddress, "wrong teb %p / %p\n", teb.Tib.Self, info.TebBaseAddress );
        if (is_wow64)
        {
            ok( !!teb.GdiBatchCount, "GdiBatchCount not set\n" );
            ok( (char *)info.TebBaseAddress + teb.WowTebOffset == ULongToPtr(teb.GdiBatchCount) ||
                broken(!NtCurrentTeb()->WowTebOffset),  /* pre-win10 */
                "wrong teb offset %d\n", teb.WowTebOffset );
        }
        else
        {
            ok( !teb.GdiBatchCount, "GdiBatchCount set\n" );
            ok( teb.WowTebOffset == 0x2000 ||
                broken( !teb.WowTebOffset || teb.WowTebOffset == 1 ),  /* pre-win10 */
                "wrong teb offset %d\n", teb.WowTebOffset );
            ok( (char *)teb.Tib.ExceptionList == (char *)info.TebBaseAddress + 0x2000,
                "wrong Tib.ExceptionList %p / %p\n",
                (char *)teb.Tib.ExceptionList, (char *)info.TebBaseAddress + 0x2000 );
            if (!ReadProcessMemory( pi.hProcess, teb.Tib.ExceptionList, &teb32, sizeof(teb32), &res )) res = 0;
            ok( res == sizeof(teb32), "wrong len %lx\n", res );
            ok( (char *)ULongToPtr(teb32.Peb) == (char *)teb.Peb + 0x1000 ||
                broken( ULongToPtr(teb32.Peb) != teb.Peb ), /* vista */
                "wrong peb %p / %p\n", ULongToPtr(teb32.Peb), teb.Peb );
        }

        status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation,
                                            &proc_info, sizeof(proc_info), NULL );
        ok( !status, "ProcessBasicInformation failed %x\n", status );
        ok( proc_info.PebBaseAddress == teb.Peb, "wrong peb %p / %p\n", proc_info.PebBaseAddress, teb.Peb );

        if (!ReadProcessMemory( pi.hProcess, proc_info.PebBaseAddress, &peb, sizeof(peb), &res )) res = 0;
        ok( res == sizeof(peb), "wrong len %lx\n", res );
        ok( !peb.BeingDebugged, "BeingDebugged is %u\n", peb.BeingDebugged );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(teb32.Peb), &peb32, sizeof(peb32), &res )) res = 0;
            ok( res == sizeof(peb32), "wrong len %lx\n", res );
            ok( !peb32.BeingDebugged, "BeingDebugged is %u\n", peb32.BeingDebugged );
        }

        if (!ReadProcessMemory( pi.hProcess, peb.ProcessParameters, &params, sizeof(params), &res )) res = 0;
        ok( res == sizeof(params), "wrong len %lx\n", res );
#define CHECK_STR(name) \
        ok( (char *)params.name.Buffer >= (char *)peb.ProcessParameters && \
            (char *)params.name.Buffer < (char *)peb.ProcessParameters + params.Size, \
            "wrong " #name " ptr %p / %p-%p\n", params.name.Buffer, peb.ProcessParameters, \
            (char *)peb.ProcessParameters + params.Size )
        CHECK_STR( ImagePathName );
        CHECK_STR( CommandLine );
        CHECK_STR( WindowTitle );
        CHECK_STR( Desktop );
        CHECK_STR( ShellInfo );
#undef CHECK_STR
        if (!is_wow64)
        {
            ok( peb32.ProcessParameters && ULongToPtr(peb32.ProcessParameters) != peb.ProcessParameters,
                "wrong ptr32 %p / %p\n", ULongToPtr(peb32.ProcessParameters), peb.ProcessParameters );
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(peb32.ProcessParameters), &params32, sizeof(params32), &res )) res = 0;
            ok( res == sizeof(params32), "wrong len %lx\n", res );
#define CHECK_STR(name) \
            ok( ULongToPtr(params32.name.Buffer) >= ULongToPtr(peb32.ProcessParameters) && \
                ULongToPtr(params32.name.Buffer) < ULongToPtr(peb32.ProcessParameters + params32.Size), \
                "wrong " #name " ptr %x / %x-%x\n", params32.name.Buffer, peb32.ProcessParameters, \
                peb32.ProcessParameters + params.Size ); \
            ok( params32.name.Length == params.name.Length, "wrong " #name "len %u / %u\n", \
                params32.name.Length, params.name.Length )
            CHECK_STR( ImagePathName );
            CHECK_STR( CommandLine );
            CHECK_STR( WindowTitle );
            CHECK_STR( Desktop );
            CHECK_STR( ShellInfo );
#undef CHECK_STR
            ok( params32.EnvironmentSize == params.EnvironmentSize, "wrong size %u / %lu\n",
                params32.EnvironmentSize, params.EnvironmentSize );
        }

        ok( DebugActiveProcess( pi.dwProcessId ), "debugging failed\n" );
        if (!ReadProcessMemory( pi.hProcess, proc_info.PebBaseAddress, &peb, sizeof(peb), &res )) res = 0;
        ok( res == sizeof(peb), "wrong len %lx\n", res );
        ok( peb.BeingDebugged == 1, "BeingDebugged is %u\n", peb.BeingDebugged );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(teb32.Peb), &peb32, sizeof(peb32), &res )) res = 0;
            ok( res == sizeof(peb32), "wrong len %lx\n", res );
            ok( peb32.BeingDebugged == 1, "BeingDebugged is %u\n", peb32.BeingDebugged );
        }

        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    if (CreateProcessA( "C:\\windows\\system32\\notepad.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        memset( &info, 0xcc, sizeof(info) );
        status = NtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
        ok( !status, "ThreadBasicInformation failed %x\n", status );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
            ok( res == sizeof(teb), "wrong len %lx\n", res );
            ok( teb.Tib.Self == info.TebBaseAddress, "wrong teb %p / %p\n",
                teb.Tib.Self, info.TebBaseAddress );
            ok( !teb.GdiBatchCount, "GdiBatchCount set\n" );
            ok( !teb.WowTebOffset || broken( teb.WowTebOffset == 1 ),  /* vista */
                "wrong teb offset %d\n", teb.WowTebOffset );
        }
        else ok( !info.TebBaseAddress, "got teb %p\n", info.TebBaseAddress );

        status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation,
                                            &proc_info, sizeof(proc_info), NULL );
        ok( !status, "ProcessBasicInformation failed %x\n", status );
        if (is_wow64)
            ok( !proc_info.PebBaseAddress ||
                broken( (char *)proc_info.PebBaseAddress >= (char *)0x7f000000 ), /* vista */
                "wrong peb %p\n", proc_info.PebBaseAddress );
        else
            ok( proc_info.PebBaseAddress == teb.Peb, "wrong peb %p / %p\n",
                proc_info.PebBaseAddress, teb.Peb );

        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    Wow64RevertWow64FsRedirection( redir );

#ifndef _WIN64
    if (is_wow64)
    {
        PEB64 *peb64;
        TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;

        ok( !!teb64, "GdiBatchCount not set\n" );
        ok( (char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset == (char *)teb64 ||
            broken(!NtCurrentTeb()->WowTebOffset),  /* pre-win10 */
            "wrong WowTebOffset %x (%p/%p)\n", NtCurrentTeb()->WowTebOffset, teb64, NtCurrentTeb() );
        ok( (char *)teb64 + 0x2000 == (char *)NtCurrentTeb(), "unexpected diff %p / %p\n",
            teb64, NtCurrentTeb() );
        ok( (char *)teb64 + teb64->WowTebOffset == (char *)NtCurrentTeb() ||
            broken( !teb64->WowTebOffset || teb64->WowTebOffset == 1 ),  /* pre-win10 */
            "wrong WowTebOffset %x (%p/%p)\n", teb64->WowTebOffset, teb64, NtCurrentTeb() );
        ok( !teb64->GdiBatchCount, "GdiBatchCount set %x\n", teb64->GdiBatchCount );
        ok( teb64->Tib.ExceptionList == PtrToUlong( NtCurrentTeb() ), "wrong Tib.ExceptionList %s / %p\n",
            wine_dbgstr_longlong(teb64->Tib.ExceptionList), NtCurrentTeb() );
        ok( teb64->Tib.Self == PtrToUlong( teb64 ), "wrong Tib.Self %s / %p\n",
            wine_dbgstr_longlong(teb64->Tib.Self), teb64 );
        ok( teb64->StaticUnicodeString.Buffer == PtrToUlong( teb64->StaticUnicodeBuffer ),
            "wrong StaticUnicodeString %s / %p\n",
            wine_dbgstr_longlong(teb64->StaticUnicodeString.Buffer), teb64->StaticUnicodeBuffer );
        ok( teb64->ClientId.UniqueProcess == GetCurrentProcessId(), "wrong pid %s / %x\n",
            wine_dbgstr_longlong(teb64->ClientId.UniqueProcess), GetCurrentProcessId() );
        ok( teb64->ClientId.UniqueThread == GetCurrentThreadId(), "wrong tid %s / %x\n",
            wine_dbgstr_longlong(teb64->ClientId.UniqueThread), GetCurrentThreadId() );
        peb64 = ULongToPtr( teb64->Peb );
        ok( peb64->ImageBaseAddress == PtrToUlong( NtCurrentTeb()->Peb->ImageBaseAddress ),
            "wrong ImageBaseAddress %s / %p\n",
            wine_dbgstr_longlong(peb64->ImageBaseAddress), NtCurrentTeb()->Peb->ImageBaseAddress);
        ok( peb64->OSBuildNumber == NtCurrentTeb()->Peb->OSBuildNumber, "wrong OSBuildNumber %x / %x\n",
            peb64->OSBuildNumber, NtCurrentTeb()->Peb->OSBuildNumber );
        ok( peb64->OSPlatformId == NtCurrentTeb()->Peb->OSPlatformId, "wrong OSPlatformId %x / %x\n",
            peb64->OSPlatformId, NtCurrentTeb()->Peb->OSPlatformId );
        return;
    }
#endif
    ok( !NtCurrentTeb()->GdiBatchCount, "GdiBatchCount set to %x\n", NtCurrentTeb()->GdiBatchCount );
    ok( !NtCurrentTeb()->WowTebOffset || broken( NtCurrentTeb()->WowTebOffset == 1 ), /* vista */
        "WowTebOffset set to %x\n", NtCurrentTeb()->WowTebOffset );
}

static void test_selectors(void)
{
    THREAD_DESCRIPTOR_INFORMATION info;
    NTSTATUS status;
    ULONG base, limit, sel, retlen;
    I386_CONTEXT context = { CONTEXT_I386_CONTROL | CONTEXT_I386_SEGMENTS };

#ifdef _WIN64
    if (!pRtlWow64GetThreadSelectorEntry)
    {
        win_skip( "RtlWow64GetThreadSelectorEntry not supported\n" );
        return;
    }
    if (!pRtlWow64GetThreadContext || pRtlWow64GetThreadContext( GetCurrentThread(), &context ))
    {
        /* hardcoded values */
        context.SegCs = 0x23;
#ifdef __x86_64__
        __asm__( "movw %%fs,%0" : "=m" (context.SegFs) );
        __asm__( "movw %%ss,%0" : "=m" (context.SegSs) );
#else
        context.SegSs = 0x2b;
        context.SegFs = 0x53;
#endif
    }
#define GET_ENTRY(info,size,ret) \
    pRtlWow64GetThreadSelectorEntry( GetCurrentThread(), info, size, ret )

#else
    GetThreadContext( GetCurrentThread(), &context );
#define GET_ENTRY(info,size,ret) \
    NtQueryInformationThread( GetCurrentThread(), ThreadDescriptorTableEntry, info, size, ret )
#endif

    trace( "cs %04x ss %04x fs %04x\n", context.SegCs, context.SegSs, context.SegFs );
    retlen = 0xdeadbeef;
    info.Selector = 0;
    status = GET_ENTRY( &info, sizeof(info) - 1, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %x\n", status );
    ok( retlen == 0xdeadbeef, "len set %u\n", retlen );

    retlen = 0xdeadbeef;
    status = GET_ENTRY( &info, sizeof(info) + 1, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %x\n", status );
    ok( retlen == 0xdeadbeef, "len set %u\n", retlen );

    retlen = 0xdeadbeef;
    status = GET_ENTRY( NULL, 0, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %x\n", status );
    ok( retlen == 0xdeadbeef, "len set %u\n", retlen );

    status = GET_ENTRY( &info, sizeof(info), NULL );
    ok( !status, "wrong status %x\n", status );

    for (info.Selector = 0; info.Selector < 0x100; info.Selector++)
    {
        retlen = 0xdeadbeef;
        status = GET_ENTRY( &info, sizeof(info), &retlen );
        base = (info.Entry.BaseLow |
                (info.Entry.HighWord.Bytes.BaseMid << 16) |
                (info.Entry.HighWord.Bytes.BaseHi << 24));
        limit = (info.Entry.LimitLow | info.Entry.HighWord.Bits.LimitHi << 16);
        sel = info.Selector | 3;

        if (sel == 0x03)  /* null selector */
        {
            ok( !status, "wrong status %x\n", status );
            ok( retlen == sizeof(info.Entry), "len set %u\n", retlen );
            ok( !base, "wrong base %x\n", base );
            ok( !limit, "wrong limit %x\n", limit );
            ok( !info.Entry.HighWord.Bytes.Flags1, "wrong flags1 %x\n", info.Entry.HighWord.Bytes.Flags1 );
            ok( !info.Entry.HighWord.Bytes.Flags2, "wrong flags2 %x\n", info.Entry.HighWord.Bytes.Flags2 );
        }
        else if (sel == context.SegCs)  /* 32-bit code selector */
        {
            ok( !status, "wrong status %x\n", status );
            ok( retlen == sizeof(info.Entry), "len set %u\n", retlen );
            ok( !base, "wrong base %x\n", base );
            ok( limit == 0xfffff, "wrong limit %x\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x1b, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (sel == context.SegSs)  /* 32-bit data selector */
        {
            ok( !status, "wrong status %x\n", status );
            ok( retlen == sizeof(info.Entry), "len set %u\n", retlen );
            ok( !base, "wrong base %x\n", base );
            ok( limit == 0xfffff, "wrong limit %x\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x13, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (sel == context.SegFs)  /* TEB selector */
        {
            ok( !status, "wrong status %x\n", status );
            ok( retlen == sizeof(info.Entry), "len set %u\n", retlen );
#ifdef _WIN64
            if (NtCurrentTeb()->WowTebOffset == 0x2000)
                ok( base == (ULONG_PTR)NtCurrentTeb() + 0x2000, "wrong base %x / %p\n",
                    base, NtCurrentTeb() );
#else
            ok( base == (ULONG_PTR)NtCurrentTeb(), "wrong base %x / %p\n", base, NtCurrentTeb() );
#endif
            ok( limit == 0xfff || broken(limit == 0x4000),  /* <= win8 */
                "wrong limit %x\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x13, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( !info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (!status)
        {
            ok( retlen == sizeof(info.Entry), "len set %u\n", retlen );
            trace( "succeeded for %x base %x limit %x type %x\n",
                   sel, base, limit, info.Entry.HighWord.Bits.Type );
        }
        else
        {
            ok( status == STATUS_UNSUCCESSFUL ||
                ((sel & 4) && (status == STATUS_NO_LDT)) ||
                broken( status == STATUS_ACCESS_VIOLATION),  /* <= win8 */
                "%x: wrong status %x\n", info.Selector, status );
            ok( retlen == 0xdeadbeef, "len set %u\n", retlen );
        }
    }
#undef GET_ENTRY
}

#ifdef _WIN64

static void test_cpu_area(void)
{
    if (pRtlWow64GetCpuAreaInfo)
    {
        static const struct
        {
            USHORT machine;
            NTSTATUS expect;
            ULONG align, size, offset, flag;
        } tests[] =
        {
            { IMAGE_FILE_MACHINE_I386,  0,  4, 0x2cc, 0x00, 0x00010000 },
            { IMAGE_FILE_MACHINE_AMD64, 0, 16, 0x4d0, 0x30, 0x00100000 },
            { IMAGE_FILE_MACHINE_ARMNT, 0,  8, 0x1a0, 0x00, 0x00200000 },
            { IMAGE_FILE_MACHINE_ARM64, 0, 16, 0x390, 0x00, 0x00400000 },
            { IMAGE_FILE_MACHINE_ARM,   STATUS_INVALID_PARAMETER },
            { IMAGE_FILE_MACHINE_THUMB, STATUS_INVALID_PARAMETER },
        };
        USHORT buffer[2048];
        WOW64_CPURESERVED *cpu;
        WOW64_CPU_AREA_INFO info;
        ULONG i, j;
        NTSTATUS status;
#define ALIGN(ptr,align) ((void *)(((ULONG_PTR)(ptr) + (align) - 1) & ~((align) - 1)))

        for (i = 0; i < ARRAY_SIZE(tests); i++)
        {
            for (j = 0; j < 8; j++)
            {
                cpu = (WOW64_CPURESERVED *)(buffer + j);
                cpu->Flags = 0;
                cpu->Machine = tests[i].machine;
                status = pRtlWow64GetCpuAreaInfo( cpu, 0, &info );
                ok( status == tests[i].expect, "%u:%u: failed %x\n", i, j, status );
                if (status) continue;
                ok( info.Context == ALIGN( cpu + 1, tests[i].align ), "%u:%u: wrong offset %u\n",
                    i, j, (ULONG)((char *)info.Context - (char *)cpu) );
                ok( info.ContextEx == ALIGN( (char *)info.Context + tests[i].size, sizeof(void*) ),
                    "%u:%u: wrong ex offset %u\n", i, j, (ULONG)((char *)info.ContextEx - (char *)cpu) );
                ok( info.ContextFlagsLocation == (char *)info.Context + tests[i].offset,
                    "%u:%u: wrong flags offset %u\n",
                    i, j, (ULONG)((char *)info.ContextFlagsLocation - (char *)info.Context) );
                ok( info.CpuReserved == cpu, "%u:%u: wrong cpu %p / %p\n", info.CpuReserved, cpu );
                ok( info.ContextFlag == tests[i].flag, "%u:%u: wrong flag %08x\n", i, j, info.ContextFlag );
                ok( info.Machine == tests[i].machine, "%u:%u: wrong machine %x\n", i, j, info.Machine );
            }
        }
#undef ALIGN
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );
}

#else  /* _WIN64 */

static const BYTE call_func64_code[] =
{
    0x58,                               /* pop %eax */
    0x0e,                               /* push %cs */
    0x50,                               /* push %eax */
    0x6a, 0x33,                         /* push $0x33 */
    0xe8, 0x00, 0x00, 0x00, 0x00,       /* call 1f */
    0x83, 0x04, 0x24, 0x05,             /* 1: addl $0x5,(%esp) */
    0xcb,                               /* lret */
    /* in 64-bit mode: */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0x55,                               /* push %rbp */
    0x48, 0x89, 0xe5,                   /* mov %rsp,%rbp */
    0x56,                               /* push %rsi */
    0x57,                               /* push %rdi */
    0x41, 0x8b, 0x4e, 0x10,             /* mov 0x10(%r14),%ecx */
    0x41, 0x8b, 0x76, 0x14,             /* mov 0x14(%r14),%esi */
    0x67, 0x8d, 0x04, 0xcd, 0, 0, 0, 0, /* lea 0x0(,%ecx,8),%eax */
    0x83, 0xf8, 0x20,                   /* cmp $0x20,%eax */
    0x7d, 0x05,                         /* jge 1f */
    0xb8, 0x20, 0x00, 0x00, 0x00,       /* mov $0x20,%eax */
    0x48, 0x29, 0xc4,                   /* 1: sub %rax,%rsp */
    0x48, 0x83, 0xe4, 0xf0,             /* and $~15,%rsp */
    0x48, 0x89, 0xe7,                   /* mov %rsp,%rdi */
    0xf3, 0x48, 0xa5,                   /* rep movsq */
    0x48, 0x8b, 0x0c, 0x24,             /* mov (%rsp),%rcx */
    0x48, 0x8b, 0x54, 0x24, 0x08,       /* mov 0x8(%rsp),%rdx */
    0x4c, 0x8b, 0x44, 0x24, 0x10,       /* mov 0x10(%rsp),%r8 */
    0x4c, 0x8b, 0x4c, 0x24, 0x18,       /* mov 0x18(%rsp),%r9 */
    0x41, 0xff, 0x56, 0x08,             /* callq *0x8(%r14) */
    0x48, 0x8d, 0x65, 0xf0,             /* lea -0x10(%rbp),%rsp */
    0x5f,                               /* pop %rdi */
    0x5e,                               /* pop %rsi */
    0x5d,                               /* pop %rbp */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0xcb,                               /* lret */
};

static NTSTATUS call_func64( ULONG64 func64, int nb_args, ULONG64 *args )
{
    NTSTATUS (WINAPI *func)( ULONG64 func64, int nb_args, ULONG64 *args ) = code_mem;

    memcpy( code_mem, call_func64_code, sizeof(call_func64_code) );
    return func( func64, nb_args, args );
}

static ULONG64 main_module, ntdll_module, wow64_module, wow64cpu_module, wow64win_module;

static void enum_modules64( void (*func)(ULONG64,const WCHAR *) )
{
    typedef struct
    {
        LIST_ENTRY64     InLoadOrderLinks;
        LIST_ENTRY64     InMemoryOrderLinks;
        LIST_ENTRY64     InInitializationOrderLinks;
        ULONG64          DllBase;
        ULONG64          EntryPoint;
        ULONG            SizeOfImage;
        UNICODE_STRING64 FullDllName;
        UNICODE_STRING64 BaseDllName;
        /* etc. */
    } LDR_DATA_TABLE_ENTRY64;

    TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
    PEB64 peb64;
    ULONG64 ptr;
    PEB_LDR_DATA64 ldr;
    LDR_DATA_TABLE_ENTRY64 entry;
    NTSTATUS status;
    HANDLE process;

    process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok( process != 0, "failed to open current process %u\n", GetLastError() );
    status = pNtWow64ReadVirtualMemory64( process, teb64->Peb, &peb64, sizeof(peb64), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    todo_wine
    ok( peb64.LdrData, "LdrData not initialized\n" );
    if (!peb64.LdrData) goto done;
    status = pNtWow64ReadVirtualMemory64( process, peb64.LdrData, &ldr, sizeof(ldr), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    ptr = ldr.InLoadOrderModuleList.Flink;
    for (;;)
    {
        WCHAR buffer[256];
        status = pNtWow64ReadVirtualMemory64( process, ptr, &entry, sizeof(entry), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        status = pNtWow64ReadVirtualMemory64( process, entry.BaseDllName.Buffer, buffer, sizeof(buffer), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        if (status) break;
        func( entry.DllBase, buffer );
        ptr = entry.InLoadOrderLinks.Flink;
        if (ptr == peb64.LdrData + offsetof( PEB_LDR_DATA64, InLoadOrderModuleList )) break;
    }
done:
    NtClose( process );
}

static ULONG64 get_proc_address64( ULONG64 module, const char *name )
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    IMAGE_EXPORT_DIRECTORY exports;
    ULONG i, *names, *funcs;
    USHORT *ordinals;
    NTSTATUS status;
    HANDLE process;
    ULONG64 ret = 0;
    char buffer[64];

    if (!module) return 0;
    process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok( process != 0, "failed to open current process %u\n", GetLastError() );
    status = pNtWow64ReadVirtualMemory64( process, module, &dos, sizeof(dos), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + dos.e_lfanew, &nt, sizeof(nt), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                                          &exports, sizeof(exports), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    names = calloc( exports.NumberOfNames, sizeof(*names) );
    ordinals = calloc( exports.NumberOfNames, sizeof(*ordinals) );
    funcs = calloc( exports.NumberOfFunctions, sizeof(*funcs) );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfNames,
                                          names, exports.NumberOfNames * sizeof(*names), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfNameOrdinals,
                                          ordinals, exports.NumberOfNames * sizeof(*ordinals), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfFunctions,
                                          funcs, exports.NumberOfFunctions * sizeof(*funcs), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
    for (i = 0; i < exports.NumberOfNames && !ret; i++)
    {
        status = pNtWow64ReadVirtualMemory64( process, module + names[i], buffer, sizeof(buffer), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        if (!strcmp( buffer, name )) ret = module + funcs[ordinals[i]];
    }
    free( funcs );
    free( ordinals );
    free( names );
    NtClose( process );
    return ret;
}

static void check_module( ULONG64 base, const WCHAR *name )
{
    if (base == (ULONG_PTR)GetModuleHandleW(0))
    {
        WCHAR *p, module[MAX_PATH];

        GetModuleFileNameW( 0, module, MAX_PATH );
        if ((p = wcsrchr( module, '\\' ))) p++;
        else p = module;
        ok( !wcsicmp( name, p ), "wrong name %s / %s\n", debugstr_w(name), debugstr_w(module));
        main_module = base;
        return;
    }
#define CHECK_MODULE(mod) if (!wcsicmp( name, L"" #mod ".dll" )) { mod ## _module = base; return; }
    CHECK_MODULE(ntdll);
    CHECK_MODULE(wow64);
    CHECK_MODULE(wow64cpu);
    CHECK_MODULE(wow64win);
#undef CHECK_MODULE
    ok( 0, "unknown module %s %s found\n", wine_dbgstr_longlong(base), wine_dbgstr_w(name));
}

static void test_modules(void)
{
    if (!is_wow64) return;
    if (!pNtWow64ReadVirtualMemory64) return;
    enum_modules64( check_module );
    todo_wine
    {
    ok( main_module, "main module not found\n" );
    ok( ntdll_module, "64-bit ntdll not found\n" );
    ok( wow64_module, "wow64.dll not found\n" );
    ok( wow64cpu_module, "wow64cpu.dll not found\n" );
    ok( wow64win_module, "wow64win.dll not found\n" );
    }
}

static void test_nt_wow64(void)
{
    const char str[] = "hello wow64";
    char buffer[100];
    NTSTATUS status;
    ULONG64 res;
    HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );

    ok( process != 0, "failed to open current process %u\n", GetLastError() );
    if (pNtWow64ReadVirtualMemory64)
    {
        status = pNtWow64ReadVirtualMemory64( process, (ULONG_PTR)str, buffer, sizeof(str), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        ok( res == sizeof(str), "wrong size %s\n", wine_dbgstr_longlong(res) );
        ok( !strcmp( buffer, str ), "wrong data %s\n", debugstr_a(buffer) );
        status = pNtWow64WriteVirtualMemory64( process, (ULONG_PTR)buffer, " bye ", 5, &res );
        ok( !status, "NtWow64WriteVirtualMemory64 failed %x\n", status );
        ok( res == 5, "wrong size %s\n", wine_dbgstr_longlong(res) );
        ok( !strcmp( buffer, " bye  wow64" ), "wrong data %s\n", debugstr_a(buffer) );
        /* current process pseudo-handle is broken on some Windows versions */
        status = pNtWow64ReadVirtualMemory64( GetCurrentProcess(), (ULONG_PTR)str, buffer, sizeof(str), &res );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64ReadVirtualMemory64 failed %x\n", status );
        status = pNtWow64WriteVirtualMemory64( GetCurrentProcess(), (ULONG_PTR)buffer, " bye ", 5, &res );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64WriteVirtualMemory64 failed %x\n", status );
    }
    else win_skip( "NtWow64ReadVirtualMemory64 not supported\n" );

    if (pNtWow64AllocateVirtualMemory64)
    {
        ULONG64 ptr = 0;
        ULONG64 size = 0x2345;

        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( !status, "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        ok( ptr, "ptr not set\n" );
        ok( size == 0x3000, "size not set %s\n", wine_dbgstr_longlong(size) );
        ptr += 0x1000;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        ok( status == STATUS_CONFLICTING_ADDRESSES, "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        ptr = 0;
        size = 0;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_4,
            "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        size = 0x1000;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 22, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_3,
            "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 33, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_3,
            "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0x3fffffff, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        todo_wine_if( !is_wow64 )
        ok( !status, "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        ok( ptr < 0x40000000, "got wrong ptr %s\n", wine_dbgstr_longlong(ptr) );
        if (!status && pNtWow64WriteVirtualMemory64)
        {
            status = pNtWow64WriteVirtualMemory64( process, ptr, str, sizeof(str), &res );
            ok( !status, "NtWow64WriteVirtualMemory64 failed %x\n", status );
            ok( res == sizeof(str), "wrong size %s\n", wine_dbgstr_longlong(res) );
            ok( !strcmp( (char *)(ULONG_PTR)ptr, str ), "wrong data %s\n",
                debugstr_a((char *)(ULONG_PTR)ptr) );
            ptr = 0;
            status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                      MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
            ok( !status, "NtWow64AllocateVirtualMemory64 failed %x\n", status );
            status = pNtWow64WriteVirtualMemory64( process, ptr, str, sizeof(str), &res );
            todo_wine
            ok( status == STATUS_PARTIAL_COPY || broken( status == STATUS_ACCESS_VIOLATION ),
                "NtWow64WriteVirtualMemory64 failed %x\n", status );
            todo_wine
            ok( !res, "wrong size %s\n", wine_dbgstr_longlong(res) );
        }
        ptr = 0x9876543210ull;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        todo_wine
        ok( !status || broken( status == STATUS_CONFLICTING_ADDRESSES ),
            "NtWow64AllocateVirtualMemory64 failed %x\n", status );
        if (!status) ok( ptr == 0x9876540000ull, "wrong ptr %s\n", wine_dbgstr_longlong(ptr) );
        ptr = 0;
        status = pNtWow64AllocateVirtualMemory64( GetCurrentProcess(), &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64AllocateVirtualMemory64 failed %x\n", status );
    }
    else win_skip( "NtWow64AllocateVirtualMemory64 not supported\n" );

    if (pNtWow64GetNativeSystemInformation)
    {
        ULONG i, len;
        SYSTEM_BASIC_INFORMATION sbi, sbi2, sbi3;

        memset( &sbi, 0xcc, sizeof(sbi) );
        status = pNtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), &len );
        ok( status == STATUS_SUCCESS, "failed %x\n", status );
        ok( len == sizeof(sbi), "wrong length %d\n", len );

        memset( &sbi2, 0xcc, sizeof(sbi2) );
        status = pRtlGetNativeSystemInformation( SystemBasicInformation, &sbi2, sizeof(sbi2), &len );
        ok( status == STATUS_SUCCESS, "failed %x\n", status );
        ok( len == sizeof(sbi2), "wrong length %d\n", len );

        ok( sbi.HighestUserAddress == (void *)0x7ffeffff, "wrong limit %p\n", sbi.HighestUserAddress);
        todo_wine_if( is_wow64 )
        ok( sbi2.HighestUserAddress == (is_wow64 ? (void *)0xfffeffff : (void *)0x7ffeffff),
            "wrong limit %p\n", sbi.HighestUserAddress);

        memset( &sbi3, 0xcc, sizeof(sbi3) );
        status = pNtWow64GetNativeSystemInformation( SystemBasicInformation, &sbi3, sizeof(sbi3), &len );
        ok( status == STATUS_SUCCESS, "failed %x\n", status );
        ok( len == sizeof(sbi3), "wrong length %d\n", len );
        ok( !memcmp( &sbi2, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );

        memset( &sbi3, 0xcc, sizeof(sbi3) );
        status = pNtWow64GetNativeSystemInformation( SystemEmulationBasicInformation, &sbi3, sizeof(sbi3), &len );
        ok( status == STATUS_SUCCESS, "failed %x\n", status );
        ok( len == sizeof(sbi3), "wrong length %d\n", len );
        ok( !memcmp( &sbi, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );

        for (i = 0; i < 256; i++)
        {
            NTSTATUS expect = pNtQuerySystemInformation( i, NULL, 0, &len );
            status = pNtWow64GetNativeSystemInformation( i, NULL, 0, &len );
            switch (i)
            {
            case SystemNativeBasicInformation:
                ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_INFO_LENGTH_MISMATCH ||
                    broken(status == STATUS_NOT_IMPLEMENTED) /* vista */, "%u: %x / %x\n", i, status, expect );
                break;
            case SystemBasicInformation:
            case SystemCpuInformation:
            case SystemEmulationBasicInformation:
            case SystemEmulationProcessorInformation:
                ok( status == expect, "%u: %x / %x\n", i, status, expect );
                break;
            default:
                if (is_wow64)  /* only a few info classes are supported on Wow64 */
                    ok( status == STATUS_INVALID_INFO_CLASS ||
                        broken(status == STATUS_NOT_IMPLEMENTED), /* vista */
                        "%u: %x\n", i, status );
                else
                    ok( status == expect, "%u: %x / %x\n", i, status, expect );
                break;
            }
        }
    }
    else win_skip( "NtWow64GetNativeSystemInformation not supported\n" );

    NtClose( process );
}

static void test_init_block(void)
{
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
    ULONG i, size = 0, *init_block;
    ULONG64 ptr64, *block64;
    void *ptr;

    if (!is_wow64) return;
    if ((ptr = GetProcAddress( ntdll, "LdrSystemDllInitBlock" )))
    {
        init_block = ptr;
        trace( "got init block %08x\n", init_block[0] );
#define CHECK_FUNC(val,func) \
            ok( (val) == (ULONG_PTR)GetProcAddress( ntdll, func ), \
                "got %p for %s %p\n", (void *)(ULONG_PTR)(val), func, GetProcAddress( ntdll, func ))
        switch (init_block[0])
        {
        case 0x44:  /* vistau64 */
            CHECK_FUNC( init_block[1], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[2], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[3], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[4], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[5], "LdrHotPatchRoutine" );
            CHECK_FUNC( init_block[6], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[7], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[9], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[10], "RtlpQueryProcessDebugInformationRemote" );
            CHECK_FUNC( init_block[11], "EtwpNotificationThread" );
            ok( init_block[12] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[12], ntdll );
            size = 13 * sizeof(*init_block);
            break;
        case 0x50:  /* win7 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[8], "LdrHotPatchRoutine" );
            CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[11], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[12], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[13], "RtlpQueryProcessDebugInformationRemote" );
            CHECK_FUNC( init_block[14], "EtwpNotificationThread" );
            ok( init_block[15] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[15], ntdll );
            CHECK_FUNC( init_block[16], "LdrSystemDllInitBlock" );
            size = 17 * sizeof(*init_block);
            break;
        case 0x70:  /* win8 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[11], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[12], "RtlpQueryProcessDebugInformationRemote" );
            ok( init_block[13] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[13], ntdll );
            CHECK_FUNC( init_block[14], "LdrSystemDllInitBlock" );
            size = 15 * sizeof(*init_block);
            break;
        case 0x80:  /* win10 1507 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[11], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[12], "RtlpQueryProcessDebugInformationRemote" );
            ok( init_block[13] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[13], ntdll );
            CHECK_FUNC( init_block[14], "LdrSystemDllInitBlock" );
            size = 15 * sizeof(*init_block);
            break;
        case 0xe0:  /* win10 1809 */
        case 0xf0:  /* win10 2004 */
            block64 = ptr;
            CHECK_FUNC( block64[3], "LdrInitializeThunk" );
            CHECK_FUNC( block64[4], "KiUserExceptionDispatcher" );
            CHECK_FUNC( block64[5], "KiUserApcDispatcher" );
            todo_wine CHECK_FUNC( block64[6], "KiUserCallbackDispatcher" );
            CHECK_FUNC( block64[7], "RtlUserThreadStart" );
            CHECK_FUNC( block64[8], "RtlpQueryProcessDebugInformationRemote" );
            todo_wine ok( block64[9] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)block64[9], ntdll );
            CHECK_FUNC( block64[10], "LdrSystemDllInitBlock" );
            CHECK_FUNC( block64[11], "RtlpFreezeTimeBias" );
            size = 12 * sizeof(*block64);
            break;
        default:
            ok( 0, "unknown init block %08x\n", init_block[0] );
            for (i = 0; i < 32; i++) trace("%04x: %08x\n", i, init_block[i]);
            break;
        }
#undef CHECK_FUNC

        if (size && (ptr64 = get_proc_address64( ntdll_module, "LdrSystemDllInitBlock" )))
        {
            DWORD buffer[64];
            HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
            NTSTATUS status = pNtWow64ReadVirtualMemory64( process, ptr64, buffer, size, NULL );
            ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
            ok( !memcmp( buffer, init_block, size ), "wrong 64-bit init block\n" );
            NtClose( process );
        }
    }
    else todo_wine win_skip( "LdrSystemDllInitBlock not supported\n" );
}


static void test_iosb(void)
{
    static const char pipe_name[] = "\\\\.\\pipe\\wow64iosbnamedpipe";
    HANDLE client, server;
    NTSTATUS status;
    ULONG64 func;
    DWORD id;
    IO_STATUS_BLOCK iosb32;
    struct
    {
        union
        {
            NTSTATUS Status;
            ULONG64 Pointer;
        };
        ULONG64 Information;
    } iosb64;
    ULONG64 args[] = { 0, 0, 0, 0, (ULONG_PTR)&iosb64, FSCTL_PIPE_LISTEN, 0, 0, 0, 0 };

    if (!is_wow64) return;
    if (!ntdll_module) return;
    func = get_proc_address64( ntdll_module, "NtFsControlFile" );

    /* async calls set iosb32 but not iosb64 */

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %u\n", GetLastError() );

    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    args[0] = (LONG_PTR)server;
    status = call_func64( func, ARRAY_SIZE(args), args );
    ok( status == STATUS_PENDING, "NtFsControlFile returned %x\n", status );
    ok( U(iosb32).Status == 0x55555555, "status changed to %x\n", U(iosb32).Status );
    ok( U(iosb64).Pointer == PtrToUlong(&iosb32), "status changed to %x\n", U(iosb64).Status );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %lx\n", (ULONG_PTR)iosb64.Information );

    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError() );

    ok( U(iosb32).Status == 0, "Wrong iostatus %x\n", U(iosb32).Status );
    ok( U(iosb64).Pointer == PtrToUlong(&iosb32), "status changed to %x\n", U(iosb64).Status );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %lx\n", (ULONG_PTR)iosb64.Information );

    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;
    id = 0xdeadbeef;

    args[5] = FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE;
    args[6] = (ULONG_PTR)"ClientProcessId";
    args[7] = sizeof("ClientProcessId");
    args[8] = (ULONG_PTR)&id;
    args[9] = sizeof(id);

    status = call_func64( func, ARRAY_SIZE(args), args );
    ok( status == STATUS_PENDING || status == STATUS_SUCCESS, "NtFsControlFile returned %x\n", status );
    todo_wine
    {
    ok( U(iosb32).Status == STATUS_SUCCESS, "status changed to %x\n", U(iosb32).Status );
    ok( iosb32.Information == sizeof(id), "info changed to %lx\n", iosb32.Information );
    ok( U(iosb64).Pointer == PtrToUlong(&iosb32), "status changed to %x\n", U(iosb64).Status );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %lx\n", (ULONG_PTR)iosb64.Information );
    }
    ok( id == GetCurrentProcessId(), "wrong id %x / %x\n", id, GetCurrentProcessId() );
    CloseHandle( client );
    CloseHandle( server );

    /* synchronous calls set iosb64 but not iosb32 */

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %u\n", GetLastError() );

    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError() );

    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;
    id = 0xdeadbeef;

    args[0] = (LONG_PTR)server;
    status = call_func64( func, ARRAY_SIZE(args), args );
    ok( status == STATUS_SUCCESS, "NtFsControlFile returned %x\n", status );
    ok( U(iosb32).Status == 0x55555555, "status changed to %x\n", U(iosb32).Status );
    ok( iosb32.Information == 0x55555555, "info changed to %lx\n", iosb32.Information );
    ok( U(iosb64).Pointer == STATUS_SUCCESS, "status changed to %x\n", U(iosb64).Status );
    ok( iosb64.Information == sizeof(id), "info changed to %lx\n", (ULONG_PTR)iosb64.Information );
    ok( id == GetCurrentProcessId(), "wrong id %x / %x\n", id, GetCurrentProcessId() );
    CloseHandle( client );
    CloseHandle( server );
}

static NTSTATUS invoke_syscall( const char *name, ULONG args32[] )
{
    ULONG64 args64[] = { -1, PtrToUlong( args32 ) };
    ULONG64 func = get_proc_address64( wow64_module, "Wow64SystemServiceEx" );
    BYTE *syscall = (BYTE *)GetProcAddress( GetModuleHandleA("ntdll.dll"), name );

    ok( syscall != NULL, "syscall %s not found\n", name );
    if (syscall[0] == 0xb8)
        args64[0] = *(DWORD *)(syscall + 1);
    else
        win_skip( "syscall thunk %s not recognized\n", name );

    return call_func64( func, ARRAY_SIZE(args64), args64 );
}

static void test_syscalls(void)
{
    ULONG64 func;
    ULONG args32[8];
    HANDLE event, event2;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    NTSTATUS status;

    if (!is_wow64) return;
    if (!ntdll_module) return;

    func = get_proc_address64( wow64_module, "Wow64SystemServiceEx" );
    ok( func, "Wow64SystemServiceEx not found\n" );

    event = CreateEventA( NULL, FALSE, FALSE, NULL );

    status = NtSetEvent( event, NULL );
    ok( !status, "NtSetEvent failed %x\n", status );
    args32[0] = HandleToLong( event );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %x\n", status );
    status = NtSetEvent( event, NULL );
    ok( status == STATUS_INVALID_HANDLE, "NtSetEvent failed %x\n", status );
    status = invoke_syscall( "NtClose", args32 );
    ok( status == STATUS_INVALID_HANDLE, "syscall failed %x\n", status );
    args32[0] = 0xdeadbeef;
    status = invoke_syscall( "NtClose", args32 );
    ok( status == STATUS_INVALID_HANDLE, "syscall failed %x\n", status );

    RtlInitUnicodeString( &name, L"\\BaseNamedObjects\\wow64-test");
    InitializeObjectAttributes( &attr, &name, OBJ_OPENIF, 0, NULL );
    event = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong(&event );
    args32[1] = EVENT_ALL_ACCESS;
    args32[2] = PtrToUlong( &attr );
    args32[3] = NotificationEvent;
    args32[4] = 0;
    status = invoke_syscall( "NtCreateEvent", args32 );
    ok( !status, "syscall failed %x\n", status );
    status = NtSetEvent( event, NULL );
    ok( !status, "NtSetEvent failed %x\n", status );

    event2 = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong( &event2 );
    status = invoke_syscall( "NtOpenEvent", args32 );
    ok( !status, "syscall failed %x\n", status );
    status = NtSetEvent( event2, NULL );
    ok( !status, "NtSetEvent failed %x\n", status );
    args32[0] = HandleToLong( event2 );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %x\n", status );

    event2 = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong( &event2 );
    status = invoke_syscall( "NtCreateEvent", args32 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "syscall failed %x\n", status );
    status = NtSetEvent( event2, NULL );
    ok( !status, "NtSetEvent failed %x\n", status );
    args32[0] = HandleToLong( event2 );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %x\n", status );

    status = NtClose( event );
    ok( !status, "NtClose failed %x\n", status );

    if (pNtWow64ReadVirtualMemory64)
    {
        TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
        PEB64 peb64, peb64_2;
        ULONG64 res, res2;
        HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
        ULONG args32[] = { HandleToLong( process ), (ULONG)teb64->Peb, teb64->Peb >> 32,
                           PtrToUlong(&peb64_2), sizeof(peb64_2), 0, PtrToUlong(&res2) };

        ok( process != 0, "failed to open current process %u\n", GetLastError() );
        status = pNtWow64ReadVirtualMemory64( process, teb64->Peb, &peb64, sizeof(peb64), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        status = invoke_syscall( "NtWow64ReadVirtualMemory64", args32 );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %x\n", status );
        ok( res2 == res, "wrong len %s / %s\n", wine_dbgstr_longlong(res), wine_dbgstr_longlong(res2) );
        ok( !memcmp( &peb64, &peb64_2, res ), "data is different\n" );
        NtClose( process );
    }
}

static void test_cpu_area(void)
{
    TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
    ULONG64 ptr;
    NTSTATUS status;

    if (!is_wow64) return;
    if (!ntdll_module) return;

    if ((ptr = get_proc_address64( ntdll_module, "RtlWow64GetCurrentCpuArea" )))
    {
        USHORT machine = 0xdead;
        ULONG64 context, context_ex;
        ULONG64 args[] = { (ULONG_PTR)&machine, (ULONG_PTR)&context, (ULONG_PTR)&context_ex };

        status = call_func64( ptr, ARRAY_SIZE(args), args );
        ok( !status, "RtlWow64GetCpuAreaInfo failed %x\n", status );
        ok( machine == IMAGE_FILE_MACHINE_I386, "wrong machine %x\n", machine );
        ok( context == teb64->TlsSlots[WOW64_TLS_CPURESERVED] + 4, "wrong context %s / %s\n",
            wine_dbgstr_longlong(context), wine_dbgstr_longlong(teb64->TlsSlots[WOW64_TLS_CPURESERVED]) );
        ok( !context_ex, "got context_ex %s\n", wine_dbgstr_longlong(context_ex) );
        args[0] = args[1] = args[2] = 0;
        status = call_func64( ptr, ARRAY_SIZE(args), args );
        ok( !status, "RtlWow64GetCpuAreaInfo failed %x\n", status );
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );

}

#endif  /* _WIN64 */


START_TEST(wow64)
{
    init();
    test_query_architectures();
    test_peb_teb();
    test_selectors();
#ifndef _WIN64
    test_nt_wow64();
    test_modules();
    test_init_block();
    test_iosb();
    test_syscalls();
#endif
    test_cpu_area();
}
