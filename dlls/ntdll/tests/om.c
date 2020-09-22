/*
 * Unit test suite for object manager functions
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2005 Vitaliy Margolen
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

#include "ntdll_test.h"
#include "winternl.h"
#include "stdio.h"
#include "winnt.h"
#include "stdlib.h"

static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static NTSTATUS (WINAPI *pNtCreateEvent) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN, BOOLEAN);
static NTSTATUS (WINAPI *pNtOpenEvent)   ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtPulseEvent)  ( HANDLE, PLONG );
static NTSTATUS (WINAPI *pNtQueryEvent)  ( HANDLE, EVENT_INFORMATION_CLASS, PVOID, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtResetEvent)  ( HANDLE, LONG* );
static NTSTATUS (WINAPI *pNtSetEvent)    ( HANDLE, LONG* );
static NTSTATUS (WINAPI *pNtCreateJobObject)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtOpenJobObject)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateKey)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG,
                                        const UNICODE_STRING *, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtOpenKey)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtDeleteKey)( HANDLE );
static NTSTATUS (WINAPI *pNtCreateMailslotFile)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                                 ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtCreateMutant)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN );
static NTSTATUS (WINAPI *pNtOpenMutant)  ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtQueryMutant) ( HANDLE, MUTANT_INFORMATION_CLASS, PVOID, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtReleaseMutant)( HANDLE, PLONG );
static NTSTATUS (WINAPI *pNtCreateSemaphore)( PHANDLE, ACCESS_MASK,const POBJECT_ATTRIBUTES,LONG,LONG );
static NTSTATUS (WINAPI *pNtOpenSemaphore)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateTimer) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, TIMER_TYPE );
static NTSTATUS (WINAPI *pNtOpenTimer)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateSection)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, const PLARGE_INTEGER,
                                            ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtOpenSection)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtClose)       ( HANDLE );
static NTSTATUS (WINAPI *pNtCreateNamedPipeFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtOpenDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtOpenSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PUNICODE_STRING);
static NTSTATUS (WINAPI *pNtQuerySymbolicLinkObject)(HANDLE,PUNICODE_STRING,PULONG);
static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static NTSTATUS (WINAPI *pNtReleaseSemaphore)(HANDLE, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtCreateKeyedEvent)( HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *, ULONG );
static NTSTATUS (WINAPI *pNtOpenKeyedEvent)( HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES * );
static NTSTATUS (WINAPI *pNtWaitForKeyedEvent)( HANDLE, const void *, BOOLEAN, const LARGE_INTEGER * );
static NTSTATUS (WINAPI *pNtReleaseKeyedEvent)( HANDLE, const void *, BOOLEAN, const LARGE_INTEGER * );
static NTSTATUS (WINAPI *pNtCreateIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI *pNtOpenIoCompletion)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, void *, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQuerySystemTime)( LARGE_INTEGER * );
static NTSTATUS (WINAPI *pRtlWaitOnAddress)( const void *, const void *, SIZE_T, const LARGE_INTEGER * );
static void     (WINAPI *pRtlWakeAddressAll)( const void * );
static void     (WINAPI *pRtlWakeAddressSingle)( const void * );

#define KEYEDEVENT_WAIT       0x0001
#define KEYEDEVENT_WAKE       0x0002
#define KEYEDEVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x0003)

static void test_case_sensitive (void)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE Event, Mutant, h;

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\test");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08x)\n", status);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH /* Vista+ */, "got %#x\n", status);

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Test");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Event(%08x)\n", status);

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\TEst");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenMutant(&h, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtOpenMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);

    pNtClose(Mutant);

    pRtlInitUnicodeString(&str, L"\\BASENamedObjects\\test");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH /* Vista+ */, "got %#x\n", status);

    status = pNtCreateEvent(&h, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION got(%08x)\n", status);

    attr.Attributes = 0;
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);

    pNtClose(Event);
}

static void test_namespace_pipe(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    HANDLE pipe, h;

    timeout.QuadPart = -10000;

    pRtlInitUnicodeString(&str, L"\\??\\PIPE\\test\\pipe");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_SUCCESS, "Failed to create NamedPipe(%08x)\n", status);

    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08x)\n", status);

    pRtlInitUnicodeString(&str, L"\\??\\PIPE\\TEST\\PIPE");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08x)\n", status);

    h = CreateFileA("\\\\.\\pipe\\test\\pipe", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, 0 );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open NamedPipe (%u)\n", GetLastError());
    pNtClose(h);

    pRtlInitUnicodeString(&str, L"\\??\\pipe\\test\\pipe");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND ||
       status == STATUS_PIPE_NOT_AVAILABLE ||
       status == STATUS_OBJECT_NAME_INVALID || /* vista */
       status == STATUS_OBJECT_NAME_NOT_FOUND, /* win8 */
        "NtOpenFile should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);

    pRtlInitUnicodeString(&str, L"\\??\\pipe\\test");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND ||
       status == STATUS_OBJECT_NAME_INVALID, /* vista */
        "NtOpenFile should have failed with STATUS_OBJECT_NAME_NOT_FOUND got(%08x)\n", status);

    str.Length -= 4 * sizeof(WCHAR);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "NtOpenFile should have succeeded got %08x\n", status);
    pNtClose( h );

    str.Length -= sizeof(WCHAR);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "NtOpenFile should have succeeded got %08x\n", status);
    pNtClose( h );

    pNtClose(pipe);
}

#define DIRECTORY_QUERY (0x0001)
#define SYMBOLIC_LINK_QUERY 0x0001

#define check_create_open_dir(parent, name, status) check_create_open_dir_(__LINE__, parent, name, status)
static void check_create_open_dir_( int line, HANDLE parent, const WCHAR *name, NTSTATUS expect )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    NTSTATUS status;
    HANDLE h;

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes( &attr, &str, 0, parent, NULL );
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok_(__FILE__, line)( status == expect, "NtCreateDirectoryObject(%s) got %08x\n", debugstr_w(name), status );
    if (!status) pNtClose( h );

    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok_(__FILE__, line)( status == expect, "NtOpenDirectoryObject(%s) got %08x\n", debugstr_w(name), status );
    if (!status) pNtClose( h );
}

static BOOL is_correct_dir( HANDLE dir, const WCHAR *name )
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE h = 0;

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    if (h) pNtClose( h );
    return (status == STATUS_OBJECT_NAME_EXISTS);
}

/* return a handle to the BaseNamedObjects dir where kernel32 objects get created */
static HANDLE get_base_dir(void)
{
    static const WCHAR objname[] = L"om.c_get_base_dir_obj";
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h;
    WCHAR name[40];

    h = CreateMutexW( NULL, FALSE, objname );
    ok(h != 0, "CreateMutexA failed got ret=%p (%d)\n", h, GetLastError());
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    swprintf( name, ARRAY_SIZE(name), L"\\BaseNamedObjects\\Session\\%u", NtCurrentTeb()->Peb->SessionId );
    RtlInitUnicodeString( &str, name );
    status = pNtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
    ok(!status, "got %#x\n", status);
    ok(is_correct_dir( dir, objname ), "wrong dir\n");

    pNtClose( h );
    return dir;
}

static void test_name_collisions(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h, h1, h2;
    DWORD winerr;
    LARGE_INTEGER size;

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_COLLISION, "NtCreateDirectoryObject got %08x\n", status );
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "NtCreateDirectoryObject got %08x\n", status );
    pNtClose(h);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);

    RtlInitUnicodeString(&str, L"\\??\\PIPE\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#x\n", status);

    dir = get_base_dir();
    RtlInitUnicodeString(&str, L"om.c-test");
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    h = CreateMutexA(NULL, FALSE, "om.c-test");
    ok(h != 0, "CreateMutexA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateMutant(&h1, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateMutant should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateMutexA(NULL, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateMutexA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    ok(h != 0, "CreateEventA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateEvent(&h1, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateEvent should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateEventA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    ok(h != 0, "CreateSemaphoreA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateSemaphore(&h1, GENERIC_ALL, &attr, 1, 2);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSemaphore should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateSemaphoreA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);
    
    h = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    ok(h != 0, "CreateWaitableTimerA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateTimer(&h1, GENERIC_ALL, &attr, NotificationTimer);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateTimer should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateWaitableTimerA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    ok(h != 0, "CreateFileMappingA failed got ret=%p (%d)\n", h, GetLastError());
    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection(&h1, SECTION_MAP_WRITE, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSection should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateFileMappingA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    pNtClose(dir);
}

static void test_all_kernel_objects( UINT line, OBJECT_ATTRIBUTES *attr,
                                     NTSTATUS create_expect, NTSTATUS open_expect )
{
    UNICODE_STRING target;
    LARGE_INTEGER size;
    NTSTATUS status, status2;
    HANDLE ret, ret2;

    RtlInitUnicodeString( &target, L"\\DosDevices" );
    size.QuadPart = 4096;

    status = pNtCreateMutant( &ret, GENERIC_ALL, attr, FALSE );
    ok( status == create_expect, "%u: NtCreateMutant failed %x\n", line, status );
    status2 = pNtOpenMutant( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenMutant failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, attr, 1, 2 );
    ok( status == create_expect, "%u: NtCreateSemaphore failed %x\n", line, status );
    status2 = pNtOpenSemaphore( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenSemaphore failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateEvent( &ret, GENERIC_ALL, attr, 1, 0 );
    ok( status == create_expect, "%u: NtCreateEvent failed %x\n", line, status );
    status2 = pNtOpenEvent( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenEvent failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, attr, 0 );
    ok( status == create_expect, "%u: NtCreateKeyedEvent failed %x\n", line, status );
    status2 = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenKeyedEvent failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateTimer( &ret, GENERIC_ALL, attr, NotificationTimer );
    ok( status == create_expect, "%u: NtCreateTimer failed %x\n", line, status );
    status2 = pNtOpenTimer( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenTimer failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, attr, 0 );
    ok( status == create_expect, "%u: NtCreateCompletion failed %x\n", line, status );
    status2 = pNtOpenIoCompletion( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenCompletion failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateJobObject( &ret, GENERIC_ALL, attr );
    ok( status == create_expect, "%u: NtCreateJobObject failed %x\n", line, status );
    status2 = pNtOpenJobObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenJobObject failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, attr );
    ok( status == create_expect, "%u: NtCreateDirectoryObject failed %x\n", line, status );
    status2 = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenDirectoryObject failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, attr, &target );
    ok( status == create_expect, "%u: NtCreateSymbolicLinkObject failed %x\n", line, status );
    status2 = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenSymbolicLinkObject failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == create_expect, "%u: NtCreateSection failed %x\n", line, status );
    status2 = pNtOpenSection( &ret2, SECTION_MAP_WRITE, attr );
    ok( status2 == open_expect, "%u: NtOpenSection failed %x\n", line, status2 );
    if (!status) pNtClose( ret );
    if (!status2) pNtClose( ret2 );
}

static void test_name_limits(void)
{
    static const WCHAR pipeW[]     = L"\\Device\\NamedPipe\\";
    static const WCHAR mailslotW[] = L"\\Device\\MailSlot\\";
    static const WCHAR registryW[] = L"\\REGISTRY\\Machine\\SOFTWARE\\Microsoft\\";
    OBJECT_ATTRIBUTES attr, attr2, attr3;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER size, timeout;
    UNICODE_STRING str, str2, target;
    NTSTATUS status;
    HANDLE ret, ret2;
    DWORD i;

    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    InitializeObjectAttributes( &attr2, &str, 0, (HANDLE)0xdeadbeef, NULL );
    InitializeObjectAttributes( &attr3, &str, 0, 0, NULL );
    str.Buffer = HeapAlloc( GetProcessHeap(), 0, 65536 + sizeof(registryW));
    str.MaximumLength = 65534;
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i] = 'a';
    size.QuadPart = 4096;
    RtlInitUnicodeString( &target, L"\\DosDevices" );

    attr.RootDirectory = get_base_dir();
    str.Length = 0;
    status = pNtCreateMutant( &ret, GENERIC_ALL, &attr2, FALSE );
    ok( status == STATUS_SUCCESS, "%u: NtCreateMutant failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenMutant( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenMutant failed %x\n", str.Length, status );
    status = pNtOpenMutant( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenMutant failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, &attr2, 1, 2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSemaphore failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSemaphore( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSemaphore failed %x\n", str.Length, status );
    status = pNtOpenSemaphore( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenSemaphore failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateEvent( &ret, GENERIC_ALL, &attr2, 1, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateEvent failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenEvent( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenEvent failed %x\n", str.Length, status );
    status = pNtOpenEvent( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenEvent failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, &attr2, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateKeyedEvent failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenKeyedEvent failed %x\n", str.Length, status );
    status = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenKeyedEvent failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateTimer( &ret, GENERIC_ALL, &attr2, NotificationTimer );
    ok( status == STATUS_SUCCESS, "%u: NtCreateTimer failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenTimer( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenTimer failed %x\n", str.Length, status );
    status = pNtOpenTimer( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenTimer failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, &attr2, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateCompletion failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenIoCompletion( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenCompletion failed %x\n", str.Length, status );
    status = pNtOpenIoCompletion( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenCompletion failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateJobObject( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateJobObject failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenJobObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenJobObject failed %x\n", str.Length, status );
    status = pNtOpenJobObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenJobObject failed %x\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateDirectoryObject failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_SUCCESS || broken(status == STATUS_ACCESS_DENIED), /* winxp */
        "%u: NtOpenDirectoryObject failed %x\n", str.Length, status );
    if (!status) pNtClose( ret2 );
    status = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_SUCCESS, "%u: NtOpenDirectoryObject failed %x\n", str.Length, status );
    pNtClose( ret2 );
    pNtClose( ret );
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, &attr2, &target );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSymbolicLinkObject failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSymbolicLinkObject failed %x\n", str.Length, status );
    status = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_SUCCESS, "%u: NtOpenSymbolicLinkObject failed %x\n", str.Length, status );
    pNtClose( ret2 );
    pNtClose( ret );
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, &attr2, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSection failed %x\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSection( &ret2, SECTION_MAP_WRITE, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSection failed %x\n", str.Length, status );
    status = pNtOpenSection( &ret2, SECTION_MAP_WRITE, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE /* < 7 */,
        "%u: NtOpenSection failed %x\n", str.Length, status );
    pNtClose( ret );

    str.Length = 67;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );

    str.Length = 65532;
    test_all_kernel_objects( __LINE__, &attr, STATUS_SUCCESS, STATUS_SUCCESS );

    str.Length = 65534;
    test_all_kernel_objects( __LINE__, &attr, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );

    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        if (attr.Length == sizeof(attr))
            test_all_kernel_objects( __LINE__, &attr, STATUS_SUCCESS, STATUS_SUCCESS );
        else
            test_all_kernel_objects( __LINE__, &attr, STATUS_INVALID_PARAMETER, STATUS_INVALID_PARAMETER );
    }
    attr.Length = sizeof(attr);

    /* null attributes or ObjectName, with or without RootDirectory */
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );
    test_all_kernel_objects( __LINE__, &attr3, STATUS_SUCCESS, STATUS_OBJECT_PATH_SYNTAX_BAD );

    attr3.ObjectName = &str2;
    pRtlInitUnicodeString( &str2, L"\\BaseNamedObjects\\Local" );
    status = pNtOpenSymbolicLinkObject( &ret, SYMBOLIC_LINK_QUERY, &attr3 );
    ok( status == STATUS_SUCCESS, "can't open BaseNamedObjects\\Local %x\n", status );
    attr3.ObjectName = &str;
    attr3.RootDirectory = ret;
    test_all_kernel_objects( __LINE__, &attr3, STATUS_OBJECT_TYPE_MISMATCH, STATUS_OBJECT_TYPE_MISMATCH );
    pNtClose( attr3.RootDirectory );

    status = pNtCreateMutant( &ret, GENERIC_ALL, NULL, FALSE );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateMutant failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenMutant( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenMutant failed %x\n", status );
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, NULL, 1, 2 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateSemaphore failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenSemaphore( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSemaphore failed %x\n", status );
    status = pNtCreateEvent( &ret, GENERIC_ALL, NULL, 1, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateEvent failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenEvent( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenEvent failed %x\n", status );
    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateKeyedEvent failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenKeyedEvent( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenKeyedEvent failed %x\n", status );
    status = pNtCreateTimer( &ret, GENERIC_ALL, NULL, NotificationTimer );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateTimer failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenTimer( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenTimer failed %x\n", status );
    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateCompletion failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenIoCompletion( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenCompletion failed %x\n", status );
    status = pNtCreateJobObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateJobObject failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenJobObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenJobObject failed %x\n", status );
    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateDirectoryObject failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenDirectoryObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenDirectoryObject failed %x\n", status );
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, NULL, &target );
    ok( status == STATUS_ACCESS_VIOLATION || broken( status == STATUS_SUCCESS), /* winxp */
        "NULL: NtCreateSymbolicLinkObject failed %x\n", status );
    if (!status) pNtClose( ret );
    status = pNtOpenSymbolicLinkObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSymbolicLinkObject failed %x\n", status );
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateSection failed %x\n", status );
    pNtClose( ret );
    status = pNtOpenSection( &ret, SECTION_MAP_WRITE, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSection failed %x\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* named pipes */
    wcscpy( str.Buffer, pipeW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen( pipeW )] = 'a';
    str.Length = 0;
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    timeout.QuadPart = -10000;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    str.Length = 67;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                      FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
        if (attr.Length == sizeof(attr))
        {
            ok( status == STATUS_SUCCESS, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
            pNtClose( ret );
        }
        else ok( status == STATUS_INVALID_PARAMETER,
                 "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    }
    attr.Length = sizeof(attr);
    str.Length = 65532;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_SUCCESS, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    pNtClose( ret );
    str.Length = 65534;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateNamedPipeFile failed %x\n", str.Length, status );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NULL: NtCreateNamedPipeFile failed %x\n", status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr3, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NULL: NtCreateNamedPipeFile failed %x\n", status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, NULL, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtCreateNamedPipeFile failed %x\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* mailslots */
    wcscpy( str.Buffer, mailslotW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen( mailslotW )] = 'a';
    str.Length = 0;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    str.Length = 67;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
        if (attr.Length == sizeof(attr))
        {
            ok( status == STATUS_SUCCESS, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
            pNtClose( ret );
        }
        else ok( status == STATUS_INVALID_PARAMETER,
                 "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    }
    attr.Length = sizeof(attr);
    str.Length = 65532;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    pNtClose( ret );
    str.Length = 65534;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateMailslotFile failed %x\n", str.Length, status );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NULL: NtCreateMailslotFile failed %x\n", status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr3, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NULL: NtCreateMailslotFile failed %x\n", status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, NULL, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtCreateMailslotFile failed %x\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* registry keys */
    wcscpy( str.Buffer, registryW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen(registryW)] = 'a';
    str.Length = 0;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    todo_wine
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr2, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtOpenKey failed %x\n", str.Length, status );
    str.Length = (wcslen( registryW ) + 250) * sizeof(WCHAR) + 1;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_SUCCESS ), /* wow64 */
        "%u: NtCreateKey failed %x\n", str.Length, status );
    if (!status)
    {
        pNtDeleteKey( ret );
        pNtClose( ret );
    }
    str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR);
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "%u: NtCreateKey failed %x\n", str.Length, status );
    if (!status)
    {
        status = pNtOpenKey( &ret2, KEY_READ, &attr );
        ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %x\n", str.Length, status );
        pNtClose( ret2 );
        attr3.RootDirectory = ret;
        str.Length = 0;
        status = pNtOpenKey( &ret2, KEY_READ, &attr3 );
        ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %x\n", str.Length, status );
        pNtClose( ret2 );
        pNtDeleteKey( ret );
        pNtClose( ret );

        str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR);
        for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
        {
            if (attr.Length == sizeof(attr))
            {
                status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
                ok( status == STATUS_SUCCESS, "%u: NtCreateKey failed %x\n", str.Length, status );
                status = pNtOpenKey( &ret2, KEY_READ, &attr );
                ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %x\n", str.Length, status );
                pNtClose( ret2 );
                pNtDeleteKey( ret );
                pNtClose( ret );
            }
            else
            {
                status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
                ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %x\n", str.Length, status );
                status = pNtOpenKey( &ret2, KEY_READ, &attr );
                ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %x\n", str.Length, status );
            }
        }
        attr.Length = sizeof(attr);
    }
    str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR) + 1;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_SUCCESS ), /* win7 */
        "%u: NtCreateKey failed %x\n", str.Length, status );
    if (!status)
    {
        pNtDeleteKey( ret );
        pNtClose( ret );
    }
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_OBJECT_NAME_NOT_FOUND ), /* wow64 */
        "%u: NtOpenKey failed %x\n", str.Length, status );
    str.Length++;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    todo_wine
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %x\n", str.Length, status );
    str.Length = 2000;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    todo_wine
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %x\n", str.Length, status );
    /* some Windows versions change the error past 2050 chars, others past 4066 chars, some don't */
    str.Length = 5000;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL ||
        status == STATUS_INVALID_PARAMETER,
        "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    todo_wine
    ok( status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL ||
        status == STATUS_INVALID_PARAMETER,
         "%u: NtOpenKey failed %x\n", str.Length, status );
    str.Length = 65534;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL,
        "%u: NtCreateKey failed %x\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    todo_wine
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL,
         "%u: NtOpenKey failed %x\n", str.Length, status );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr2, 0, NULL, 0, NULL );
    todo_wine
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE /* vista wow64 */,
        "NULL: NtCreateKey failed %x\n", status );
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr3, 0, NULL, 0, NULL );
    todo_wine
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKey failed %x\n", status );
    status = pNtCreateKey( &ret, GENERIC_ALL, NULL, 0, NULL, 0, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKey failed %x\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_HANDLE /* vista wow64 */,
        "NULL: NtOpenKey failed %x\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr3 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKey failed %x\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKey failed %x\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    HeapFree( GetProcessHeap(), 0, str.Buffer );
}

static void test_directory(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, dir1, h, h2;
    WCHAR buffer[256];
    ULONG len, full_len;

    /* No name and/or no attributes */
    status = pNtCreateDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* wow64 */, "got %#x\n", status);
    status = pNtOpenDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* wow64 */, "got %#x\n", status);

    status = pNtCreateDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_SUCCESS, "Failed to create Directory without attributes(%08x)\n", status);
    pNtClose(h);
    status = pNtOpenDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenDirectoryObject should have failed with STATUS_INVALID_PARAMETER got(%08x)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08x\n", status );

    /* Bad name */
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    RtlInitUnicodeString(&str, L"");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    pNtClose(h);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08x\n", status );
    pNtClose(dir);

    check_create_open_dir( NULL, L"BaseNamedObjects", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\\\BaseNamedObjects", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\\\om.c-test", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND );

    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    status = pNtOpenDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08x\n", status );
    pNtClose(h);
    pNtClose(dir1);


    /* Use of root directory */

    /* Can't use symlinks as a directory */
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Local");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&dir, SYMBOLIC_LINK_QUERY, &attr);

    ok(status == STATUS_SUCCESS, "Failed to open SymbolicLink(%08x)\n", status);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtCreateDirectoryObject got %08x\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\Local\\om.c-test" );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = pNtCreateDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    RtlInitUnicodeString( &str, L"om.c-test" );
    InitializeObjectAttributes( &attr, &str, 0, dir, NULL );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "Failed to open directory %08x\n", status );

    RtlInitUnicodeString( &str, L"om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, dir1, NULL );
    status = pNtCreateEvent( &h, GENERIC_ALL, &attr, 1, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateEvent failed %x\n", status );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenEvent failed %x\n", status );
    pNtClose( h2 );
    RtlInitUnicodeString( &str, L"om.c-test\\om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, dir, NULL );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtOpenEvent failed %x\n", status );
    RtlInitUnicodeString( &str, L"\\BasedNamedObjects\\Local\\om.c-test\\om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtOpenEvent failed %x\n", status );
    pNtClose( h );
    pNtClose( dir1 );

    str.Buffer = buffer;
    str.MaximumLength = sizeof(buffer);
    len = 0xdeadbeef;
    memset( buffer, 0xaa, sizeof(buffer) );
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08x\n", status );
    full_len = str.Length + sizeof(WCHAR);
    ok( len == full_len, "bad length %u/%u\n", len, full_len );
    ok( buffer[len / sizeof(WCHAR) - 1] == 0, "no terminating null\n" );

    str.MaximumLength = str.Length;
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08x\n", status );
    ok( len == full_len, "bad length %u/%u\n", len, full_len );

    str.MaximumLength = 0;
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08x\n", status );
    ok( len == full_len, "bad length %u/%u\n", len, full_len );

    str.MaximumLength = str.Length + sizeof(WCHAR);
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08x\n", status );
    ok( len == full_len, "bad length %u/%u\n", len, full_len );

    pNtClose(dir);

    RtlInitUnicodeString(&str, L"\\BaseNamedObjects");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08x\n", status );

    InitializeObjectAttributes(&attr, NULL, 0, dir, NULL);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenDirectoryObject got %08x\n", status );

    check_create_open_dir( dir, L"", STATUS_SUCCESS );
    check_create_open_dir( dir, L"\\", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"\\om.c-test", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"\\om.c-test\\", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND );

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"om.c-test");
    status = pNtCreateDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08x\n", status );

    pNtClose(h);
    pNtClose(dir1);
    pNtClose(dir);

    /* Nested directories */
    RtlInitUnicodeString(&str, L"\\");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08x\n", status );
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08x\n", status );
    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test");
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test\\one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    pNtClose(h);

    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Global\\om.c-test");
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Local\\om.c-test\\one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08x\n", status );
    pNtClose(h);
    pNtClose(dir);

    /* Create other objects using RootDirectory */

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects");
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08x\n", status );
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);

    /* Test invalid paths */
    RtlInitUnicodeString(&str, L"\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);
    RtlInitUnicodeString(&str, L"\\om.c-mutant\\");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);

    RtlInitUnicodeString(&str, L"om.c\\-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);

    RtlInitUnicodeString(&str, L"om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08x)\n", status);
    pNtClose(h);

    pNtClose(dir);
}

static void test_symboliclink(void)
{
    NTSTATUS status;
    UNICODE_STRING str, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, link, h;
    IO_STATUS_BLOCK iosb;

    /* No name and/or no attributes */
    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    RtlInitUnicodeString(&target, L"\\DosDevices");
    status = pNtCreateSymbolicLinkObject( NULL, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok(status == STATUS_ACCESS_VIOLATION, "got %#x\n", status);
    status = pNtOpenSymbolicLinkObject( NULL, SYMBOLIC_LINK_QUERY, &attr );
    ok(status == STATUS_ACCESS_VIOLATION, "got %#x\n", status);

    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION,
        "NtCreateSymbolicLinkObject should have failed with STATUS_ACCESS_VIOLATION got(%08x)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenSymbolicLinkObject should have failed with STATUS_INVALID_PARAMETER got(%08x)\n", status);

    /* No attributes */
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, &target);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_VIOLATION, /* nt4 */
       "NtCreateSymbolicLinkObject failed(%08x)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    memset(&target, 0, sizeof(target));
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_INVALID_PARAMETER, "got %#x\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);

    /* Bad name */
    RtlInitUnicodeString(&target, L"anywhere");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    RtlInitUnicodeString(&str, L"");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08x)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);
    pNtClose(link);

    RtlInitUnicodeString(&str, L"\\");
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr, &target);
    todo_wine ok(status == STATUS_OBJECT_TYPE_MISMATCH,
                 "NtCreateSymbolicLinkObject should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);

    RtlInitUnicodeString( &target, L"->Somewhere");

    RtlInitUnicodeString( &str, L"BaseNamedObjects" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtCreateSymbolicLinkObject got %08x\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenSymbolicLinkObject got %08x\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08x\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08x\n", status );

    RtlInitUnicodeString( &str, L"\\\\BaseNamedObjects" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08x\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08x\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\\\om.c-test" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08x\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08x\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\om.c-test\\" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#x\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#x\n", status );

    /* Compound test */
    dir = get_base_dir();
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"test-link");
    RtlInitUnicodeString(&target, L"\\DosDevices");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08x)\n", status);

    RtlInitUnicodeString(&str, L"test-link\\NUL");
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "Failed to open NUL device(%08x)\n", status);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "Failed to open NUL device(%08x)\n", status);

    pNtClose(h);
    pNtClose(link);
    pNtClose(dir);
}

#define test_file_info(a) _test_file_info(__LINE__,a)
static void _test_file_info(unsigned line, HANDLE handle)
{
    IO_STATUS_BLOCK io;
    char buf[256];
    NTSTATUS status;

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), 0xdeadbeef);
    ok_(__FILE__,line)(status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* Vista+ */,
                       "expected STATUS_NOT_IMPLEMENTED, got %x\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), FileAccessInformation);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "FileAccessInformation returned %x\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf),
                                    FileIoCompletionNotificationInformation);
    ok_(__FILE__,line)(status == STATUS_SUCCESS || broken(status == STATUS_INVALID_INFO_CLASS) /* XP */,
                       "FileIoCompletionNotificationInformation returned %x\n", status);
}

#define test_no_file_info(a) _test_no_file_info(__LINE__,a)
static void _test_no_file_info(unsigned line, HANDLE handle)
{
    IO_STATUS_BLOCK io;
    char buf[256];
    NTSTATUS status;

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), 0xdeadbeef);
    ok_(__FILE__,line)(status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* Vista+ */,
                       "expected STATUS_NOT_IMPLEMENTED, got %x\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), FileAccessInformation);
    ok_(__FILE__,line)(status == STATUS_OBJECT_TYPE_MISMATCH,
                       "FileAccessInformation returned %x\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf),
                                    FileIoCompletionNotificationInformation);
    ok_(__FILE__,line)(status == STATUS_OBJECT_TYPE_MISMATCH || broken(status == STATUS_INVALID_INFO_CLASS) /* XP */,
                       "FileIoCompletionNotificationInformation returned %x\n", status);
}

static BOOL compare_unicode_string( const UNICODE_STRING *string, const WCHAR *expect )
{
    return string->Length == wcslen( expect ) * sizeof(WCHAR)
            && !wcsnicmp( string->Buffer, expect, string->Length / sizeof(WCHAR) );
}

#define test_object_type(a,b) _test_object_type(__LINE__,a,b)
static void _test_object_type( unsigned line, HANDLE handle, const WCHAR *expected_name )
{
    char buffer[1024];
    UNICODE_STRING *str = (UNICODE_STRING *)buffer, expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__,line)( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok_(__FILE__,line)( len > sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    ok_(__FILE__,line)( len >= sizeof(OBJECT_TYPE_INFORMATION) + str->Length, "unexpected len %u\n", len );
    ok_(__FILE__,line)(compare_unicode_string( str, expected_name ), "wrong name %s\n", debugstr_w( str->Buffer ));
}

#define test_object_name(a,b,c) _test_object_name(__LINE__,a,b,c)
static void _test_object_name( unsigned line, HANDLE handle, const WCHAR *expected_name, BOOL todo )
{
    char buffer[1024];
    UNICODE_STRING *str = (UNICODE_STRING *)buffer, expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__,line)( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok_(__FILE__,line)( len >= sizeof(OBJECT_NAME_INFORMATION) + str->Length, "unexpected len %u\n", len );
    todo_wine_if (todo)
        ok_(__FILE__,line)(compare_unicode_string( str, expected_name ), "wrong name %s\n", debugstr_w( str->Buffer ));
}

static void test_query_object(void)
{
    static const WCHAR name[] = L"\\BaseNamedObjects\\test_event";
    HANDLE handle, client;
    char buffer[1024];
    NTSTATUS status;
    ULONG len, expected_len;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING path, *str;
    char dir[MAX_PATH], tmp_path[MAX_PATH], file1[MAX_PATH + 16];
    WCHAR expect[100];
    LARGE_INTEGER size;
    BOOL ret;

    InitializeObjectAttributes( &attr, &path, 0, 0, 0 );

    handle = CreateEventA( NULL, FALSE, FALSE, "test_event" );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(OBJECT_TYPE_INFORMATION), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR) == len, "unexpected len %u\n", len );
    ok( str->Length >= sizeof(name) - sizeof(WCHAR), "unexpected len %u\n", str->Length );
    ok( len > sizeof(UNICODE_STRING) + sizeof("\\test_event") * sizeof(WCHAR),
        "name too short %s\n", wine_dbgstr_w(str->Buffer) );
    /* check for \\Sessions prefix in the name */
    swprintf( expect, ARRAY_SIZE(expect), L"\\Sessions\\%u%s", NtCurrentTeb()->Peb->SessionId, name );
    ok( (str->Length == wcslen( expect ) * sizeof(WCHAR) && !wcscmp( str->Buffer, expect )) ||
        broken( !wcscmp( str->Buffer, name )), /* winxp */
        "wrong name %s\n", wine_dbgstr_w(str->Buffer) );
    trace( "got %s len %u\n", wine_dbgstr_w(str->Buffer), len );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, len, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %u\n", len );

    test_object_type( handle, L"Event" );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, len, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %u\n", len );

    test_no_file_info( handle );
    pNtClose( handle );

    handle = CreateEventA( NULL, FALSE, FALSE, NULL );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len == sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( str->Length == 0, "unexpected len %u\n", len );
    ok( str->Buffer == NULL, "unexpected ptr %p\n", str->Buffer );
    test_no_file_info( handle );
    pNtClose( handle );

    GetWindowsDirectoryA( dir, MAX_PATH );
    handle = CreateFileA( dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS, 0 );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    expected_len = sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR);
    ok( len == expected_len, "unexpected len %u\n", len );
    trace( "got %s len %u\n", wine_dbgstr_w(str->Buffer), len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#x\n", status );
    ok( len == expected_len || broken(!len /* XP */ || len == sizeof(UNICODE_STRING) /* 2003 */),
        "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_BUFFER_OVERFLOW, "got %#x\n", status);
    ok( len == expected_len, "unexpected len %u\n", len );

    test_object_type( handle, L"File" );

    pNtClose( handle );

    GetTempPathA(MAX_PATH, tmp_path);
    GetTempFileNameA(tmp_path, "foo", 0, file1);
    handle = CreateFileA(file1, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    test_object_type(handle, L"File");
    DeleteFileA( file1 );
    test_file_info( handle );
    pNtClose( handle );

    status = pNtCreateIoCompletion( &handle, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateIoCompletion failed %x\n", status);

    test_object_type( handle, L"IoCompletion" );
    test_no_file_info( handle );

    pNtClose( handle );

    status = pNtCreateDirectoryObject( &handle, DIRECTORY_QUERY, NULL );
    ok(status == STATUS_SUCCESS, "Failed to create Directory %08x\n", status);

    test_object_type( handle, L"Directory" );
    test_no_file_info( handle );

    pNtClose( handle );

    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection( &handle, SECTION_MAP_WRITE, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS , "NtCreateSection returned %x\n", status );

    test_object_type( handle, L"Section" );
    test_no_file_info( handle );

    pNtClose( handle );

    handle = CreateMailslotA( "\\\\.\\mailslot\\test_mailslot", 100, 1000, NULL );
    ok( handle != INVALID_HANDLE_VALUE, "CreateMailslot failed err %u\n", GetLastError() );

    test_object_name( handle, L"\\Device\\Mailslot\\test_mailslot", FALSE );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    client = CreateFileA( "\\\\.\\mailslot\\test_mailslot", 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    len = 0;
    status = pNtQueryObject( client, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    str = (UNICODE_STRING *)buffer;
    ok( len == sizeof(UNICODE_STRING) + str->MaximumLength, "unexpected len %u\n", len );
    todo_wine
        ok( compare_unicode_string( str, L"\\Device\\Mailslot" ) ||
            compare_unicode_string( str, L"\\Device\\Mailslot\\test_mailslot" ) /* win8+ */,
            "wrong name %s\n", debugstr_w( str->Buffer ));

    test_object_type( client, L"File" );
    test_file_info( client );

    pNtClose( client );
    pNtClose( handle );

    handle = CreateFileA( "\\\\.\\mailslot", 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    test_object_name( handle, L"\\Device\\Mailslot", FALSE );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    pNtClose( handle );

    handle = CreateNamedPipeA( "\\\\.\\pipe\\test_pipe", PIPE_ACCESS_DUPLEX, PIPE_READMODE_BYTE,
                               1, 1000, 1000, 1000, NULL );
    ok( handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed err %u\n", GetLastError() );

    test_object_name( handle, L"\\Device\\NamedPipe\\test_pipe", FALSE );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    client = CreateFileA( "\\\\.\\pipe\\test_pipe", GENERIC_READ | GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING, 0, 0 );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    test_object_type( client, L"File" );
    test_file_info( client );

    pNtClose( client );
    pNtClose( handle );

    handle = CreateFileA( "\\\\.\\pipe", 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    test_object_name( handle, L"\\Device\\NamedPipe", FALSE );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    pNtClose( handle );

    RtlInitUnicodeString( &path, L"\\REGISTRY\\Machine" );
    status = pNtCreateKey( &handle, KEY_READ, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed status %x\n", status );

    test_object_name( handle, L"\\REGISTRY\\MACHINE", FALSE );
    test_object_type( handle, L"Key" );

    pNtClose( handle );

    test_object_name( GetCurrentProcess(), L"", FALSE );
    test_object_type( GetCurrentProcess(), L"Process" );
    test_no_file_info( GetCurrentProcess() );

    test_object_name( GetCurrentThread(), L"", FALSE );
    test_object_type( GetCurrentThread(), L"Thread" );
    test_no_file_info( GetCurrentThread() );

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &handle);
    ok(ret, "OpenProcessToken failed: %u\n", GetLastError());

    test_object_name( handle, L"", FALSE );
    test_object_type( handle, L"Token" );
    test_no_file_info( handle );

    pNtClose(handle);
}

static void test_type_mismatch(void)
{
    HANDLE h;
    NTSTATUS res;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    res = pNtCreateEvent( &h, 0, &attr, 0, 0 );
    ok(!res, "can't create event: %x\n", res);

    res = pNtReleaseSemaphore( h, 30, NULL );
    ok(res == STATUS_OBJECT_TYPE_MISMATCH, "expected 0xc0000024, got %x\n", res);

    pNtClose( h );
}

static void test_event(void)
{
    HANDLE Event;
    HANDLE Event2;
    LONG prev_state = 0xdeadbeef;
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    EVENT_BASIC_INFORMATION info;

    pRtlInitUnicodeString( &str, L"\\BaseNamedObjects\\testEvent" );
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, 1, 0);
    ok( status == STATUS_SUCCESS, "NtCreateEvent failed %08x\n", status );

    status = pNtPulseEvent(Event, &prev_state);
    ok( status == STATUS_SUCCESS, "NtPulseEvent failed %08x\n", status );
    ok( !prev_state, "prev_state = %x\n", prev_state );

    status = pNtQueryEvent(Event, EventBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryEvent failed %08x\n", status );
    ok( info.EventType == 1 && info.EventState == 0,
        "NtQueryEvent failed, expected 1 0, got %d %d\n", info.EventType, info.EventState );

    status = pNtOpenEvent(&Event2, GENERIC_ALL, &attr);
    ok( status == STATUS_SUCCESS, "NtOpenEvent failed %08x\n", status );

    pNtClose(Event);
    Event = Event2;

    status = pNtQueryEvent(Event, EventBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryEvent failed %08x\n", status );
    ok( info.EventType == 1 && info.EventState == 0,
        "NtQueryEvent failed, expected 1 0, got %d %d\n", info.EventType, info.EventState );

    status = pNtSetEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtSetEvent failed: %08x\n", status );
    ok( !prev_state, "prev_state = %x\n", prev_state );

    status = pNtSetEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtSetEvent failed: %08x\n", status );
    ok( prev_state == 1, "prev_state = %x\n", prev_state );

    status = pNtResetEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtSetEvent failed: %08x\n", status );
    ok( prev_state == 1, "prev_state = %x\n", prev_state );

    status = pNtResetEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtSetEvent failed: %08x\n", status );
    ok( !prev_state, "prev_state = %x\n", prev_state );

    status = pNtPulseEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtPulseEvent failed %08x\n", status );
    ok( !prev_state, "prev_state = %x\n", prev_state );

    status = pNtSetEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtSetEvent failed: %08x\n", status );
    ok( !prev_state, "prev_state = %x\n", prev_state );

    status = pNtPulseEvent( Event, &prev_state );
    ok( status == STATUS_SUCCESS, "NtPulseEvent failed %08x\n", status );
    ok( prev_state == 1, "prev_state = %x\n", prev_state );

    pNtClose(Event);
}

static const WCHAR keyed_nameW[] = L"\\BaseNamedObjects\\WineTestEvent";

static DWORD WINAPI keyed_event_thread( void *arg )
{
    HANDLE handle;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    ULONG_PTR i;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &str;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &str, keyed_nameW );

    status = pNtOpenKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS, &attr );
    ok( !status, "NtOpenKeyedEvent failed %x\n", status );

    for (i = 0; i < 20; i++)
    {
        if (i & 1)
            status = pNtWaitForKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        else
            status = pNtReleaseKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        ok( status == STATUS_SUCCESS, "%li: failed %x\n", i, status );
        Sleep( 20 - i );
    }

    status = pNtReleaseKeyedEvent( handle, (void *)0x1234, 0, NULL );
    ok( status == STATUS_SUCCESS, "NtReleaseKeyedEvent %x\n", status );

    timeout.QuadPart = -10000;
    status = pNtWaitForKeyedEvent( handle, (void *)0x5678, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)0x9abc, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    NtClose( handle );
    return 0;
}

static void test_keyed_events(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE handle, event, thread;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    ULONG_PTR i;

    if (!pNtCreateKeyedEvent)
    {
        win_skip( "Keyed events not supported\n" );
        return;
    }

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &str;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &str, keyed_nameW );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS | SYNCHRONIZE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );

    status = WaitForSingleObject( handle, 1000 );
    ok( status == 0, "WaitForSingleObject %x\n", status );

    timeout.QuadPart = -100000;
    status = pNtWaitForKeyedEvent( handle, (void *)255, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)255, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( handle, (void *)254, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)254, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( handle, NULL, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, NULL, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( NULL, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT || broken(status == STATUS_INVALID_HANDLE), /* XP/2003 */
        "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( NULL, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT || broken(status == STATUS_INVALID_HANDLE), /* XP/2003 */
        "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( (HANDLE)0xdeadbeef, (void *)9, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( (HANDLE)0xdeadbeef, (void *)9, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( (HANDLE)0xdeadbeef, (void *)8, 0, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( (HANDLE)0xdeadbeef, (void *)8, 0, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "NtReleaseKeyedEvent %x\n", status );

    thread = CreateThread( NULL, 0, keyed_event_thread, 0, 0, NULL );
    for (i = 0; i < 20; i++)
    {
        if (i & 1)
            status = pNtReleaseKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        else
            status = pNtWaitForKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        ok( status == STATUS_SUCCESS, "%li: failed %x\n", i, status );
        Sleep( i );
    }
    status = pNtWaitForKeyedEvent( handle, (void *)0x1234, 0, &timeout );
    ok( status == STATUS_SUCCESS, "NtWaitForKeyedEvent %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)0x5678, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)0x9abc, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    ok( WaitForSingleObject( thread, 30000 ) == 0, "wait failed\n" );

    NtClose( handle );

    /* test access rights */

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_WAIT, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_WAKE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = WaitForSingleObject( handle, 1000 );
    ok( status == WAIT_FAILED && GetLastError() == ERROR_ACCESS_DENIED,
        "WaitForSingleObject %x err %u\n", status, GetLastError() );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    /* GENERIC_READ gives wait access */
    status = pNtCreateKeyedEvent( &handle, GENERIC_READ, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    /* GENERIC_WRITE gives wake access */
    status = pNtCreateKeyedEvent( &handle, GENERIC_WRITE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    /* it's not an event */
    status = pNtPulseEvent( handle, NULL );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtPulseEvent %x\n", status );

    status = pNtCreateEvent( &event, GENERIC_ALL, &attr, FALSE, FALSE );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH /* 7+ */,
        "CreateEvent %x\n", status );

    NtClose( handle );

    status = pNtCreateEvent( &event, GENERIC_ALL, &attr, FALSE, FALSE );
    ok( status == 0, "CreateEvent %x\n", status );
    status = pNtWaitForKeyedEvent( event, (void *)8, 0, &timeout );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( event, (void *)8, 0, &timeout );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtReleaseKeyedEvent %x\n", status );
    NtClose( event );
}

static void test_null_device(void)
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING str;
    NTSTATUS status;
    DWORD num_bytes;
    OVERLAPPED ov;
    char buf[64];
    HANDLE null;
    BOOL ret;

    memset(buf, 0xAA, sizeof(buf));
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    RtlInitUnicodeString(&str, L"\\Device\\Null");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&null, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
       "expected STATUS_OBJECT_TYPE_MISMATCH, got %08x\n", status);

    status = pNtOpenFile(&null, GENERIC_READ | GENERIC_WRITE, &attr, &iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS,
       "expected STATUS_SUCCESS, got %08x\n", status);

    test_object_type(null, L"File");

    SetLastError(0xdeadbeef);
    ret = WriteFile(null, buf, sizeof(buf), &num_bytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(null, buf, sizeof(buf), &num_bytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WriteFile(null, buf, sizeof(buf), &num_bytes, &ov);
    ok(ret, "got error %u\n", GetLastError());
    ok(num_bytes == sizeof(buf), "expected num_bytes = %u, got %u\n",
       (DWORD)sizeof(buf), num_bytes);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(null, buf, sizeof(buf), &num_bytes, &ov);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_HANDLE_EOF, "got error %u\n", GetLastError());

    pNtClose(null);

    null = CreateFileA("\\\\.\\Null", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(null == INVALID_HANDLE_VALUE, "CreateFileA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "expected ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    null = CreateFileA("\\\\.\\Device\\Null", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(null == INVALID_HANDLE_VALUE, "CreateFileA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND,
       "expected ERROR_PATH_NOT_FOUND, got %u\n", GetLastError());

    CloseHandle(ov.hEvent);
}

static DWORD WINAPI mutant_thread( void *arg )
{
    MUTANT_BASIC_INFORMATION info;
    NTSTATUS status;
    HANDLE mutant;
    DWORD ret;

    mutant = arg;
    ret = WaitForSingleObject( mutant, 1000 );
    ok( ret == WAIT_OBJECT_0, "WaitForSingleObject failed %08x\n", ret );

    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == 0, "expected 0, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == TRUE, "expected TRUE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == FALSE, "expected FALSE, got %d\n", info.AbandonedState );
    /* abandon mutant */

    return 0;
}

static void test_mutant(void)
{
    MUTANT_BASIC_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    NTSTATUS status;
    HANDLE mutant;
    HANDLE thread;
    DWORD ret;
    ULONG len;
    LONG prev;

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\test_mutant");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateMutant(&mutant, GENERIC_ALL, &attr, TRUE);
    ok( status == STATUS_SUCCESS, "Failed to create Mutant(%08x)\n", status );

    /* bogus */
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH,
        "Failed to NtQueryMutant, expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status );
    status = pNtQueryMutant(mutant, 0x42, &info, sizeof(info), NULL);
    ok( status == STATUS_INVALID_INFO_CLASS || broken(status == STATUS_NOT_IMPLEMENTED), /* 32-bit on Vista/2k8 */
        "Failed to NtQueryMutant, expected STATUS_INVALID_INFO_CLASS, got %08x\n", status );
    status = pNtQueryMutant((HANDLE)0xdeadbeef, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_INVALID_HANDLE,
        "Failed to NtQueryMutant, expected STATUS_INVALID_HANDLE, got %08x\n", status );

    /* new */
    len = -1;
    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), &len);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == 0, "expected 0, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == TRUE, "expected TRUE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == FALSE, "expected FALSE, got %d\n", info.AbandonedState );
    ok( len == sizeof(info), "got %u\n", len );

    ret = WaitForSingleObject( mutant, 1000 );
    ok( ret == WAIT_OBJECT_0, "WaitForSingleObject failed %08x\n", ret );

    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == -1, "expected -1, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == TRUE, "expected TRUE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == FALSE, "expected FALSE, got %d\n", info.AbandonedState );

    prev = 0xdeadbeef;
    status = pNtReleaseMutant(mutant, &prev);
    ok( status == STATUS_SUCCESS, "NtQueryRelease failed %08x\n", status );
    ok( prev == -1, "NtQueryRelease failed, expected -1, got %d\n", prev );

    prev = 0xdeadbeef;
    status = pNtReleaseMutant(mutant, &prev);
    ok( status == STATUS_SUCCESS, "NtQueryRelease failed %08x\n", status );
    ok( prev == 0, "NtQueryRelease failed, expected 0, got %d\n", prev );

    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == 1, "expected 1, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == FALSE, "expected FALSE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == FALSE, "expected FALSE, got %d\n", info.AbandonedState );

    /* abandoned */
    thread = CreateThread( NULL, 0, mutant_thread, mutant, 0, NULL );
    ret = WaitForSingleObject( thread, 1000 );
    ok( ret == WAIT_OBJECT_0, "WaitForSingleObject failed %08x\n", ret );
    CloseHandle( thread );

    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == 1, "expected 0, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == FALSE, "expected FALSE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == TRUE, "expected TRUE, got %d\n", info.AbandonedState );

    ret = WaitForSingleObject( mutant, 1000 );
    ok( ret == WAIT_ABANDONED_0, "WaitForSingleObject failed %08x\n", ret );

    memset(&info, 0xcc, sizeof(info));
    status = pNtQueryMutant(mutant, MutantBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryMutant failed %08x\n", status );
    ok( info.CurrentCount == 0, "expected 0, got %d\n", info.CurrentCount );
    ok( info.OwnedByCaller == TRUE, "expected TRUE, got %d\n", info.OwnedByCaller );
    ok( info.AbandonedState == FALSE, "expected FALSE, got %d\n", info.AbandonedState );

    NtClose( mutant );
}

static void test_wait_on_address(void)
{
    DWORD ticks;
    SIZE_T size;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    LONG64 address, compare;

    if (!pRtlWaitOnAddress)
    {
        win_skip("RtlWaitOnAddress not supported, skipping test\n");
        return;
    }

    if (0) /* crash on Windows */
    {
        pRtlWaitOnAddress(&address, NULL, 8, NULL);
        pRtlWaitOnAddress(NULL, &compare, 8, NULL);
        pRtlWaitOnAddress(NULL, NULL, 8, NULL);
    }

    /* don't crash */
    pRtlWakeAddressSingle(NULL);
    pRtlWakeAddressAll(NULL);

    /* invalid values */
    address = 0;
    compare = 0;
    status = pRtlWaitOnAddress(&address, &compare, 5, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "got %x\n", status);

    /* values match */
    address = 0;
    compare = 0;
    pNtQuerySystemTime(&timeout);
    timeout.QuadPart += 100*10000;
    ticks = GetTickCount();
    status = pRtlWaitOnAddress(&address, &compare, 8, &timeout);
    ticks = GetTickCount() - ticks;
    ok(status == STATUS_TIMEOUT, "got 0x%08x\n", status);
    ok(ticks >= 90 && ticks <= 1000, "got %u\n", ticks);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));
    ok(compare == 0, "got %s\n", wine_dbgstr_longlong(compare));

    /* different address size */
    for (size = 1; size <= 4; size <<= 1)
    {
        compare = ~0;
        compare <<= size * 8;

        timeout.QuadPart = -100 * 10000;
        ticks = GetTickCount();
        status = pRtlWaitOnAddress(&address, &compare, size, &timeout);
        ticks = GetTickCount() - ticks;
        ok(status == STATUS_TIMEOUT, "got 0x%08x\n", status);
        ok(ticks >= 90 && ticks <= 1000, "got %u\n", ticks);

        status = pRtlWaitOnAddress(&address, &compare, size << 1, &timeout);
        ok(!status, "got 0x%08x\n", status);
    }
    address = 0;
    compare = 1;
    status = pRtlWaitOnAddress(&address, &compare, 8, NULL);
    ok(!status, "got 0x%08x\n", status);

    /* no waiters */
    address = 0;
    pRtlWakeAddressSingle(&address);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));
    pRtlWakeAddressAll(&address);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));
}

START_TEST(om)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");

    pNtCreateEvent          = (void *)GetProcAddress(hntdll, "NtCreateEvent");
    pNtCreateJobObject      = (void *)GetProcAddress(hntdll, "NtCreateJobObject");
    pNtOpenJobObject        = (void *)GetProcAddress(hntdll, "NtOpenJobObject");
    pNtCreateKey            = (void *)GetProcAddress(hntdll, "NtCreateKey");
    pNtOpenKey              = (void *)GetProcAddress(hntdll, "NtOpenKey");
    pNtDeleteKey            = (void *)GetProcAddress(hntdll, "NtDeleteKey");
    pNtCreateMailslotFile   = (void *)GetProcAddress(hntdll, "NtCreateMailslotFile");
    pNtCreateMutant         = (void *)GetProcAddress(hntdll, "NtCreateMutant");
    pNtOpenEvent            = (void *)GetProcAddress(hntdll, "NtOpenEvent");
    pNtQueryEvent           = (void *)GetProcAddress(hntdll, "NtQueryEvent");
    pNtPulseEvent           = (void *)GetProcAddress(hntdll, "NtPulseEvent");
    pNtResetEvent           = (void *)GetProcAddress(hntdll, "NtResetEvent");
    pNtSetEvent             = (void *)GetProcAddress(hntdll, "NtSetEvent");
    pNtOpenMutant           = (void *)GetProcAddress(hntdll, "NtOpenMutant");
    pNtQueryMutant          = (void *)GetProcAddress(hntdll, "NtQueryMutant");
    pNtReleaseMutant        = (void *)GetProcAddress(hntdll, "NtReleaseMutant");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pNtCreateNamedPipeFile  = (void *)GetProcAddress(hntdll, "NtCreateNamedPipeFile");
    pNtOpenDirectoryObject  = (void *)GetProcAddress(hntdll, "NtOpenDirectoryObject");
    pNtCreateDirectoryObject= (void *)GetProcAddress(hntdll, "NtCreateDirectoryObject");
    pNtOpenSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtOpenSymbolicLinkObject");
    pNtCreateSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtCreateSymbolicLinkObject");
    pNtQuerySymbolicLinkObject  = (void *)GetProcAddress(hntdll, "NtQuerySymbolicLinkObject");
    pNtCreateSemaphore      =  (void *)GetProcAddress(hntdll, "NtCreateSemaphore");
    pNtOpenSemaphore        =  (void *)GetProcAddress(hntdll, "NtOpenSemaphore");
    pNtCreateTimer          =  (void *)GetProcAddress(hntdll, "NtCreateTimer");
    pNtOpenTimer            =  (void *)GetProcAddress(hntdll, "NtOpenTimer");
    pNtCreateSection        =  (void *)GetProcAddress(hntdll, "NtCreateSection");
    pNtOpenSection          =  (void *)GetProcAddress(hntdll, "NtOpenSection");
    pNtQueryObject          =  (void *)GetProcAddress(hntdll, "NtQueryObject");
    pNtReleaseSemaphore     =  (void *)GetProcAddress(hntdll, "NtReleaseSemaphore");
    pNtCreateKeyedEvent     =  (void *)GetProcAddress(hntdll, "NtCreateKeyedEvent");
    pNtOpenKeyedEvent       =  (void *)GetProcAddress(hntdll, "NtOpenKeyedEvent");
    pNtWaitForKeyedEvent    =  (void *)GetProcAddress(hntdll, "NtWaitForKeyedEvent");
    pNtReleaseKeyedEvent    =  (void *)GetProcAddress(hntdll, "NtReleaseKeyedEvent");
    pNtCreateIoCompletion   =  (void *)GetProcAddress(hntdll, "NtCreateIoCompletion");
    pNtOpenIoCompletion     =  (void *)GetProcAddress(hntdll, "NtOpenIoCompletion");
    pNtQueryInformationFile =  (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtQuerySystemTime      =  (void *)GetProcAddress(hntdll, "NtQuerySystemTime");
    pRtlWaitOnAddress       =  (void *)GetProcAddress(hntdll, "RtlWaitOnAddress");
    pRtlWakeAddressAll      =  (void *)GetProcAddress(hntdll, "RtlWakeAddressAll");
    pRtlWakeAddressSingle   =  (void *)GetProcAddress(hntdll, "RtlWakeAddressSingle");

    test_case_sensitive();
    test_namespace_pipe();
    test_name_collisions();
    test_name_limits();
    test_directory();
    test_symboliclink();
    test_query_object();
    test_type_mismatch();
    test_event();
    test_mutant();
    test_keyed_events();
    test_null_device();
    test_wait_on_address();
}
