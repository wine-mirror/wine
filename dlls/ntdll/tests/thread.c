/*
 * Unit test suite for ntdll thread functions
 *
 * Copyright 2021 Paul Gofman for CodeWeavers
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

static BOOL is_wow64;
static BOOL old_wow64;

static NTSTATUS (WINAPI *pNtAllocateReserveObject)( HANDLE *, const OBJECT_ATTRIBUTES *, MEMORY_RESERVE_OBJECT_TYPE );
static NTSTATUS (WINAPI *pNtCreateThreadEx)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *,
                                             HANDLE, PRTL_THREAD_START_ROUTINE, void *,
                                             ULONG, ULONG_PTR, SIZE_T, SIZE_T, PS_ATTRIBUTE_LIST * );
static NTSTATUS  (WINAPI *pNtSuspendProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtResumeProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtQueueApcThreadEx)(HANDLE handle, HANDLE reserve_handle, PNTAPCFUNC func,
                                               ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3);
static NTSTATUS  (WINAPI *pNtQueueApcThreadEx2)(HANDLE handle, HANDLE reserve_handle, ULONG flags, PNTAPCFUNC func,
                                                ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3);

static int * (CDECL *p_errno)(void);

static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);

static void init_function_pointers(void)
{
    HMODULE hdll;
#define GET_FUNC(name) p##name = (void *)GetProcAddress( hdll, #name );
    hdll = GetModuleHandleA( "ntdll.dll" );
    GET_FUNC( NtAllocateReserveObject );
    GET_FUNC( NtCreateThreadEx );
    GET_FUNC( NtSuspendProcess );
    GET_FUNC( NtQueueApcThreadEx );
    GET_FUNC( NtQueueApcThreadEx2 );
    GET_FUNC( NtResumeProcess );
    GET_FUNC( _errno );

    hdll = GetModuleHandleA( "kernel32.dll" );
    GET_FUNC( IsWow64Process );
#undef GET_FUNC
}

static void CALLBACK test_NtCreateThreadEx_proc(void *param)
{
}

static void test_dbg_hidden_thread_creation(void)
{
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO create_info;
    PS_ATTRIBUTE_LIST ps_attr;
    WCHAR path[MAX_PATH + 4];
    HANDLE process, thread;
    UNICODE_STRING imageW;
    BOOLEAN dbg_hidden;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( !dbg_hidden, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER,
                                0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( dbg_hidden == 1, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    lstrcpyW( path, L"\\??\\" );
    GetModuleFileNameW( NULL, path + 4, MAX_PATH );

    RtlInitUnicodeString( &imageW, path );

    memset( &ps_attr, 0, sizeof(ps_attr) );
    ps_attr.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size = lstrlenW(path) * sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr = path;
    ps_attr.TotalLength = sizeof(ps_attr);

    status = RtlCreateProcessParametersEx( &params, &imageW, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    /* NtCreateUserProcess() may return STATUS_INVALID_PARAMETER with some uninitialized data in create_info. */
    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);

    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED
                                  | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );
    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
}

struct unique_teb_thread_args
{
    TEB *teb;
    HANDLE running_event;
    HANDLE quit_event;
};

static void CALLBACK test_unique_teb_proc(void *param)
{
    struct unique_teb_thread_args *args = param;
    args->teb = NtCurrentTeb();
    SetEvent( args->running_event );
    WaitForSingleObject( args->quit_event, INFINITE );
}

static void test_unique_teb(void)
{
    HANDLE threads[2], running_events[2];
    struct unique_teb_thread_args args1, args2;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    args1.running_event = running_events[0] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args1.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args2.running_event = running_events[1] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args2.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args1.quit_event = args2.quit_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    ok( args1.quit_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    status = pNtCreateThreadEx( &threads[0], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args1, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    status = pNtCreateThreadEx( &threads[1], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args2, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForMultipleObjects( 2, running_events, TRUE, INFINITE );
    SetEvent( args1.quit_event );

    WaitForMultipleObjects( 2, threads, TRUE, INFINITE );
    CloseHandle( threads[0] );
    CloseHandle( threads[1] );
    CloseHandle( args1.running_event );
    CloseHandle( args2.running_event );
    CloseHandle( args1.quit_event );

    ok( NtCurrentTeb() != args1.teb, "Multiple threads have TEB %p.\n", args1.teb );
    ok( NtCurrentTeb() != args2.teb, "Multiple threads have TEB %p.\n", args2.teb );
    ok( args1.teb != args2.teb, "Multiple threads have TEB %p.\n", args1.teb );
}

static void test_errno(void)
{
    int val;

    if (!p_errno)
    {
        win_skip( "_errno not available\n" );
        return;
    }
    ok( NtCurrentTeb()->Peb->TlsBitmap->Buffer[0] & (1 << 16), "TLS entry 16 not allocated\n" );
    *p_errno() = 0xdead;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xdead, "wrong value %x\n", val );
    *p_errno() = 0xbeef;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xbeef, "wrong value %x\n", val );
}

static void test_NtCreateUserProcess(void)
{
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO create_info;
    PS_ATTRIBUTE_LIST ps_attr;
    WCHAR path[MAX_PATH + 4];
    HANDLE process, thread;
    UNICODE_STRING imageW;
    NTSTATUS status;

    lstrcpyW( path, L"\\??\\" );
    GetModuleFileNameW( NULL, path + 4, MAX_PATH );

    RtlInitUnicodeString( &imageW, path );

    memset( &ps_attr, 0, sizeof(ps_attr) );
    ps_attr.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size = lstrlenW(path) * sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr = path;
    ps_attr.TotalLength = sizeof(ps_attr);

    status = RtlCreateProcessParametersEx( &params, &imageW, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);
    status = NtCreateUserProcess( &process, &thread, PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, SYNCHRONIZE,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
}

static void CALLBACK test_thread_bypass_process_freeze_proc(void *param)
{
    pNtSuspendProcess(NtCurrentProcess());
    /* The current process will be suspended forever here if THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE is nonfunctional. */
    pNtResumeProcess(NtCurrentProcess());
}

static void test_thread_bypass_process_freeze(void)
{
    HANDLE thread;
    NTSTATUS status;

    if (!pNtCreateThreadEx || !pNtSuspendProcess || !pNtResumeProcess)
    {
        win_skip( "NtCreateThreadEx/NtSuspendProcess/NtResumeProcess are not available.\n" );
        return;
    }

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_thread_bypass_process_freeze_proc,
                                NULL, THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS ||
        broken(status == STATUS_INVALID_PARAMETER_7) /* <= Win10-1809 */,
        "Got unexpected status %#lx.\n", status );

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
}

static int apc_count;

static void CALLBACK apc_func( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    ++apc_count;
}

static void test_NtQueueApcThreadEx(void)
{
    NTSTATUS status, expected;
    HANDLE reserve;

    if (!pNtQueueApcThreadEx)
    {
        win_skip( "NtQueueApcThreadEx is not available.\n" );
        return;
    }

    status = pNtQueueApcThreadEx( GetCurrentThread(), (HANDLE)QUEUE_USER_APC_CALLBACK_DATA_CONTEXT, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx, expected %#lx.\n", status, STATUS_INVALID_HANDLE );

    status = pNtQueueApcThreadEx( GetCurrentThread(), (HANDLE)QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    todo_wine_if(old_wow64)
    ok( status == STATUS_SUCCESS || status == STATUS_INVALID_HANDLE /* wow64 and win64 on Win version before Win10 1809 */,
        "got %#lx.\n", status );

    status = pNtQueueApcThreadEx( GetCurrentThread(), GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "got %#lx.\n", status );

    status = pNtAllocateReserveObject( &reserve, NULL, MemoryReserveObjectTypeUserApc );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = pNtQueueApcThreadEx( GetCurrentThread(), reserve, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( !status, "got %#lx.\n", status );
    status = pNtQueueApcThreadEx( GetCurrentThread(), reserve, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == STATUS_INVALID_PARAMETER_2, "got %#lx.\n", status );
    SleepEx( 0, TRUE );
    status = pNtQueueApcThreadEx( GetCurrentThread(), reserve, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( !status, "got %#lx.\n", status );

    NtClose( reserve );

    status = pNtAllocateReserveObject( &reserve, NULL, MemoryReserveObjectTypeIoCompletion );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = pNtQueueApcThreadEx( GetCurrentThread(), reserve, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "got %#lx.\n", status );
    NtClose( reserve );

    SleepEx( 0, TRUE );

    if (!pNtQueueApcThreadEx2)
    {
        win_skip( "NtQueueApcThreadEx2 is not available.\n" );
        return;
    }
    expected = is_wow64 ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
    status = pNtQueueApcThreadEx2( GetCurrentThread(), NULL, QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == expected, "got %#lx, expected %#lx.\n", status, expected );

    status = pNtQueueApcThreadEx2( GetCurrentThread(), (HANDLE)QUEUE_USER_APC_CALLBACK_DATA_CONTEXT, 0, apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx.\n", status );

    SleepEx( 0, TRUE );
}

static void extract_resource(const char *name, const char *type, const char *path)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", path, GetLastError());

    res = FindResourceA(NULL, name, type);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

struct skip_thread_attach_args
{
    BOOL teb_flag;
    PVOID teb_tls_pointer;
    PVOID teb_fls_slots;
};

static void CALLBACK test_skip_thread_attach_proc(void *param)
{
    struct skip_thread_attach_args *args = param;
    args->teb_flag = NtCurrentTeb()->SkipThreadAttach;
    args->teb_tls_pointer = NtCurrentTeb()->ThreadLocalStoragePointer;
    args->teb_fls_slots = NtCurrentTeb()->FlsSlots;
}

static void test_skip_thread_attach(void)
{
    BOOL *seen_thread_attach, *seen_thread_detach;
    struct skip_thread_attach_args args;
    HANDLE thread;
    NTSTATUS status;
    char path_dll_local[MAX_PATH + 11];
    char path_tmp[MAX_PATH];
    HMODULE module;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    GetTempPathA(sizeof(path_tmp), path_tmp);

    sprintf(path_dll_local, "%s%s", path_tmp, "testdll.dll");
    extract_resource("testdll.dll", "TESTDLL", path_dll_local);

    module = LoadLibraryA(path_dll_local);
    if (!module) {
        trace("Could not load testdll.\n");
        goto delete;
    }

    seen_thread_attach = (BOOL *)GetProcAddress(module, "seen_thread_attach");
    seen_thread_detach = (BOOL *)GetProcAddress(module, "seen_thread_detach");

    ok( !*seen_thread_attach, "Unexpected\n" );
    ok( !*seen_thread_detach, "Unexpected\n" );

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_skip_thread_attach_proc,
                                &args, THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForSingleObject( thread, INFINITE );

    CloseHandle( thread );

    ok( !*seen_thread_attach, "Unexpected\n" );
    ok( !*seen_thread_detach, "Unexpected\n" );
    ok( args.teb_flag, "Unexpected\n" );
    ok( !args.teb_tls_pointer, "Unexpected\n" );
    ok( !args.teb_fls_slots, "Unexpected\n" );

    FreeLibrary(module);
delete:
    DeleteFileA(path_dll_local);
}

START_TEST(thread)
{
    init_function_pointers();

    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;
    if (is_wow64)
    {
        TEB64 *teb64 = ULongToPtr( NtCurrentTeb()->GdiBatchCount );

        if (teb64)
        {
            PEB64 *peb64 = ULongToPtr(teb64->Peb);
            old_wow64 = !peb64->LdrData;
        }
    }

    test_dbg_hidden_thread_creation();
    test_unique_teb();
    test_errno();
    test_NtCreateUserProcess();
    test_thread_bypass_process_freeze();
    test_NtQueueApcThreadEx();
    test_skip_thread_attach();
}
