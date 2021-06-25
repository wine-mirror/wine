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

static NTSTATUS (WINAPI *pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS,void*,ULONG,void*,ULONG,ULONG*);
static USHORT   (WINAPI *pRtlWow64GetCurrentMachine)(void);
static NTSTATUS (WINAPI *pRtlWow64GetProcessMachines)(HANDLE,WORD*,WORD*);
static NTSTATUS (WINAPI *pRtlWow64IsWowGuestMachineSupported)(USHORT,BOOLEAN*);
#ifdef _WIN64
static NTSTATUS (WINAPI *pRtlWow64GetCpuAreaInfo)(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
#endif

static BOOL is_wow64;

static void init(void)
{
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );

    if (!IsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

#define GET_PROC(func) p##func = (void *)GetProcAddress( ntdll, #func )
    GET_PROC( NtQuerySystemInformationEx );
    GET_PROC( RtlWow64GetCurrentMachine );
    GET_PROC( RtlWow64GetProcessMachines );
    GET_PROC( RtlWow64IsWowGuestMachineSupported );
#ifdef _WIN64
    GET_PROC( RtlWow64GetCpuAreaInfo );
#endif
#undef GET_PROC
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

#endif  /* _WIN64 */


START_TEST(wow64)
{
    init();
    test_query_architectures();
    test_peb_teb();
#ifdef _WIN64
    test_cpu_area();
#endif
}
