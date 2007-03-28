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
static NTSTATUS (WINAPI *pNtClose)( PHANDLE );

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
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");

    nt_mailslot_test();
}
