/* Unit test suite for Ntdll file functions
 *
 * Copyright 2007 Jeff Latimer
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
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdio.h>
#include <stdarg.h>

#include "ntstatus.h"
/* Define WIN32_NO_STATUS so MSVC does not give us duplicate macro
 * definition errors when we get to winnt.h
 */
#define WIN32_NO_STATUS

#include "wine/test.h"
#include "winternl.h"

static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI *pNtCreateMailslotFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtReadFile)(HANDLE hFile, HANDLE hEvent,
                                      PIO_APC_ROUTINE apc, void* apc_user,
                                      PIO_STATUS_BLOCK io_status, void* buffer, ULONG length,
                                      PLARGE_INTEGER offset, PULONG key);
static NTSTATUS (WINAPI *pNtWriteFile)(HANDLE hFile, HANDLE hEvent,
                                       PIO_APC_ROUTINE apc, void* apc_user,
                                       PIO_STATUS_BLOCK io_status,
                                       const void* buffer, ULONG length,
                                       PLARGE_INTEGER offset, PULONG key);
static NTSTATUS (WINAPI *pNtClose)( PHANDLE );

static inline BOOL is_signaled( HANDLE obj )
{
    return WaitForSingleObject( obj, 0 ) == 0;
}

#define PIPENAME "\\\\.\\pipe\\ntdll_tests_file.c"

static BOOL create_pipe( HANDLE *read, HANDLE *write, ULONG flags, ULONG size )
{
    *read = CreateNamedPipe(PIPENAME, PIPE_ACCESS_INBOUND | flags, PIPE_TYPE_BYTE | PIPE_WAIT,
                            1, size, size, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(*read != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    *write = CreateFileA(PIPENAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(*write != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    return TRUE;
}

static HANDLE create_temp_file( ULONG flags )
{
    char buffer[MAX_PATH];
    HANDLE handle;

    GetTempFileNameA( ".", "foo", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         flags | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    return (handle == INVALID_HANDLE_VALUE) ? 0 : handle;
}

static void WINAPI apc( void *arg, IO_STATUS_BLOCK *iosb, ULONG reserved )
{
    int *count = arg;

    trace( "apc called block %p iosb.status %x iosb.info %lu\n",
           iosb, U(*iosb).Status, iosb->Information );
    (*count)++;
    ok( !reserved, "reserved is not 0: %x\n", reserved );
}

static void read_file_test(void)
{
    const char text[] = "foobar";
    HANDLE handle, read, write;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    DWORD written;
    int apc_count = 0;
    char buffer[128];
    LARGE_INTEGER offset;
    HANDLE event = CreateEventA( NULL, TRUE, FALSE, NULL );

    if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;

    /* try read with no data */
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( is_signaled( read ), "read handle is not signaled\n" );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* iosb updated here by async i/o */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    apc_count = 0;
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* with no event, the pipe handle itself gets signaled */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( !is_signaled( read ), "read handle is not signaled\n" );
    status = pNtReadFile( read, 0, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* iosb updated here by async i/o */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( read ), "read handle is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    apc_count = 0;
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* now read with data ready */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ResetEvent( event );
    WriteFile( write, buffer, 1, &written, NULL );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %x\n", status );
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* try read with no data */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( is_signaled( event ), "event is not signaled\n" ); /* check that read resets the event */
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* partial read is good enough */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read from disconnected pipe */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    CloseHandle( write );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PIPE_BROKEN, "wrong status %x\n", status );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );
    CloseHandle( read );

    /* read from closed handle */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    SetEvent( event );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_INVALID_HANDLE, "wrong status %x\n", status );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );  /* not reset on invalid handle */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    /* disconnect while async read is in progress */
    if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    CloseHandle( write );
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == STATUS_PIPE_BROKEN, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );
    CloseHandle( read );

    /* now try a real file */
    if (!(handle = create_temp_file( FILE_FLAG_OVERLAPPED ))) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read beyond eof */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok( status == STATUS_END_OF_FILE, "wrong status %x\n", status );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );
    CloseHandle( handle );

    /* now a non-overlapped file */
    if (!(handle = create_temp_file(0))) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_END_OF_FILE, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    todo_wine ok( !apc_count, "apc was called\n" );

    /* read beyond eof */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok( status == STATUS_END_OF_FILE, "wrong status %x\n", status );
    todo_wine ok( U(iosb).Status == STATUS_END_OF_FILE, "wrong status %x\n", U(iosb).Status );
    todo_wine ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    todo_wine ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    CloseHandle( handle );

    CloseHandle( event );
}

static void nt_mailslot_test(void)
{
    HANDLE hslot;
    ACCESS_MASK DesiredAccess;
    OBJECT_ATTRIBUTES attr;

    ULONG CreateOptions;
    ULONG MailslotQuota;
    ULONG MaxMessageSize;
    LARGE_INTEGER TimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS rc;
    UNICODE_STRING str;
    WCHAR buffer1[] = { '\\','?','?','\\','M','A','I','L','S','L','O','T','\\',
                        'R',':','\\','F','R','E','D','\0' };

    TimeOut.QuadPart = -1;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    DesiredAccess = CreateOptions = MailslotQuota = MaxMessageSize = 0;

    /*
     * Check for NULL pointer handling
     */
    rc = pNtCreateMailslotFile(NULL, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_ACCESS_VIOLATION, "rc = %x not c0000005 STATUS_ACCESS_VIOLATION\n", rc);

    /*
     * Test to see if the Timeout can be NULL
     */
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         NULL);
    ok( rc == STATUS_SUCCESS, "rc = %x not STATUS_SUCCESS\n", rc);
    ok( hslot != 0, "Handle is invalid\n");

    if  ( rc == STATUS_SUCCESS ) rc = pNtClose(hslot);

    /*
     * Test that the length field is checked properly
     */
    attr.Length = 0;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    todo_wine ok( rc == STATUS_INVALID_PARAMETER, "rc = %x not c000000d STATUS_INVALID_PARAMETER\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

    attr.Length = sizeof(OBJECT_ATTRIBUTES)+1;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    todo_wine ok( rc == STATUS_INVALID_PARAMETER, "rc = %x not c000000d STATUS_INVALID_PARAMETER\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

    /*
     * Test handling of a NULL unicode string in ObjectName
     */
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    attr.ObjectName = NULL;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_OBJECT_PATH_SYNTAX_BAD, "rc = %x not c000003b STATUS_OBJECT_PATH_SYNTAX_BAD\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

    /*
     * Test a valid call
     */
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_SUCCESS, "Create MailslotFile failed rc = %x %u\n", rc, GetLastError());
    ok( hslot != 0, "Handle is invalid\n");

    rc = pNtClose(hslot);
    ok( rc == STATUS_SUCCESS, "NtClose failed\n");

    pRtlFreeUnicodeString(&str);
}

START_TEST(file)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pNtCreateMailslotFile   = (void *)GetProcAddress(hntdll, "NtCreateMailslotFile");
    pNtReadFile             = (void *)GetProcAddress(hntdll, "NtReadFile");
    pNtWriteFile            = (void *)GetProcAddress(hntdll, "NtWriteFile");
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");

    read_file_test();
    nt_mailslot_test();
}
