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

#include "ntdll_test.h"

static NTSTATUS (WINAPI *pNtCreateThreadEx)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *,
                                             HANDLE, PRTL_THREAD_START_ROUTINE, void *,
                                             ULONG, ULONG_PTR, SIZE_T, SIZE_T, PS_ATTRIBUTE_LIST * );

static void init_function_pointers(void)
{
    HMODULE hntdll = GetModuleHandleA( "ntdll.dll" );
#define GET_FUNC(name) p##name = (void *)GetProcAddress( hntdll, #name );
    GET_FUNC( NtCreateThreadEx );
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
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
    ok( !dbg_hidden, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER,
                                0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
    ok( dbg_hidden == 1, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
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
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );

    /* NtCreateUserProcess() may return STATUS_INVALID_PARAMETER with some uninitialized data in create_info. */
    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);

    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED
                                  | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#x.\n", status );
    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#x.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
}

START_TEST(thread)
{
    init_function_pointers();

    test_dbg_hidden_thread_creation();
}
