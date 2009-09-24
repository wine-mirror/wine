/* Unit test suite for Ntdll directory functions
 *
 * Copyright 2007 Jeff Latimer
 * Copyright 2007 Andrey Turkin
 * Copyright 2008 Jeff Zaroyko
 * Copyright 2009 Dan Kegel
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

static NTSTATUS (WINAPI *pNtClose)( PHANDLE );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );

static void test_NtQueryDirectoryFile(void)
{
    NTSTATUS ret;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntdirname;
    char dirnameA[MAX_PATH];
    WCHAR dirnameW[MAX_PATH];
    static const char testdirA[] = "NtQueryDirectoryFile";
    static const char normalfileA[] = "n";
    static const char hiddenfileA[] = "h";
    static const char systemfileA[] = "s";
    char normalpathA[MAX_PATH];
    char hiddenpathA[MAX_PATH];
    char systempathA[MAX_PATH];
    HANDLE normalh = INVALID_HANDLE_VALUE;
    HANDLE hiddenh = INVALID_HANDLE_VALUE;
    HANDLE systemh = INVALID_HANDLE_VALUE;
    int normal_found;
    int hidden_found;
    int system_found;
    HANDLE dirh;
    IO_STATUS_BLOCK io;
    UINT data_pos;
    UINT data_len;    /* length of dir data */
    BYTE data[8192];  /* directory data */
    FILE_BOTH_DIRECTORY_INFORMATION *dir_info;
    DWORD status;
    int numfiles;

    ret = GetTempPathA(MAX_PATH, dirnameA);
    if (!ret)
    {
        ok(0, "couldn't get temp dir\n");
        return;
    }
    if (ret + sizeof(testdirA)-1 + sizeof(normalfileA)-1 >= MAX_PATH)
    {
        ok(0, "MAX_PATH exceeded in constructing paths\n");
        return;
    }

    strcat(dirnameA, testdirA);

    ret = CreateDirectoryA(dirnameA, NULL);
    ok(ret == TRUE, "couldn't create directory '%s', ret %x, error %d\n", dirnameA, ret, GetLastError());

    /* Create one normal file, one hidden, and one system file */
    GetTempFileNameA( dirnameA, normalfileA, 1, normalpathA );
    normalh = CreateFileA(normalpathA, GENERIC_READ | GENERIC_WRITE, 0, NULL,
          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( normalh != INVALID_HANDLE_VALUE, "failed to create temp file '%s'\n", normalpathA );

    GetTempFileNameA( dirnameA, hiddenfileA, 1, hiddenpathA );
    hiddenh = CreateFileA(hiddenpathA, GENERIC_READ | GENERIC_WRITE, 0, NULL,
          CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( hiddenh != INVALID_HANDLE_VALUE, "failed to create hidden temp file '%s'\n", hiddenpathA );

    GetTempFileNameA( dirnameA, systemfileA, 2, systempathA );
    systemh = CreateFileA(systempathA, GENERIC_READ | GENERIC_WRITE, 0, NULL,
          CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( systemh != INVALID_HANDLE_VALUE, "failed to create sys temp file '%s'\n", systempathA );
    if (normalh == INVALID_HANDLE_VALUE || hiddenh == INVALID_HANDLE_VALUE || systemh == INVALID_HANDLE_VALUE)
    {
       skip("can't test if we can't create files\n");
       goto done;
    }

    pRtlMultiByteToUnicodeN(dirnameW, sizeof(dirnameW), NULL, dirnameA, strlen(dirnameA)+1);
    if (!pRtlDosPathNameToNtPathName_U(dirnameW, &ntdirname, NULL, NULL))
    {
        ok(0,"RtlDosPathNametoNtPathName_U failed\n");
        goto done;
    }
    /* Now read the directory and make sure both are found */
    InitializeObjectAttributes(&attr, &ntdirname, 0, 0, NULL);
    status = pNtOpenFile( &dirh, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io,
                         FILE_OPEN,
                         FILE_SYNCHRONOUS_IO_NONALERT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_DIRECTORY_FILE);
    ok (status == STATUS_SUCCESS, "failed to open dir '%s', ret 0x%x, error %d\n", dirnameA, status, GetLastError());
    if (status != STATUS_SUCCESS) {
       skip("can't test if we can't open the directory\n");
       goto done;
    }

    pNtQueryDirectoryFile( dirh, NULL, NULL, NULL, &io, data, sizeof(data),
                       FileBothDirectoryInformation, FALSE, NULL, TRUE );
    ok (io.Status == STATUS_SUCCESS, "filed to query directory; status %x\n", io.Status);
    data_len = io.Information;
    ok (data_len >= sizeof(FILE_BOTH_DIRECTORY_INFORMATION), "not enough data in directory\n");

    normal_found = hidden_found = system_found = 0;

    data_pos = 0;
    numfiles = 0;
    while ((data_pos < data_len) && (numfiles < 5)) {
        DWORD attrib;
        dir_info = (FILE_BOTH_DIRECTORY_INFORMATION *)(data + data_pos);
        attrib = dir_info->FileAttributes & (FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN);
        switch (dir_info->FileName[0]) {
        case '.':
            break;
        case 'n':
            ok(dir_info->FileNameLength == 6*sizeof(WCHAR), "expected six-char name\n");
            ok(attrib == 0, "expected normal file\n");
            ok(normal_found == 0, "too many normal files\n");
            normal_found++;
            break;
        case 'h':
            ok(dir_info->FileNameLength == 6*sizeof(WCHAR), "expected six-char name\n");
            todo_wine ok(attrib == FILE_ATTRIBUTE_HIDDEN, "expected hidden file\n");
            ok(hidden_found == 0, "too many hidden files\n");
            hidden_found++;
            break;
        case 's':
            ok(dir_info->FileNameLength == 6*sizeof(WCHAR), "expected six-char name\n");
            todo_wine ok(attrib == FILE_ATTRIBUTE_SYSTEM, "expected system file\n");
            ok(system_found == 0, "too many system files\n");
            system_found++;
            break;
        default:
            ok(FALSE, "unexpected filename found\n");
        }
        if (dir_info->NextEntryOffset == 0) {
            pNtQueryDirectoryFile( dirh, 0, NULL, NULL, &io, data, sizeof(data),
                               FileBothDirectoryInformation, FALSE, NULL, FALSE );
            if (io.Status == STATUS_NO_MORE_FILES)
                break;
            ok (io.Status == STATUS_SUCCESS, "filed to query directory; status %x\n", io.Status);
            data_len = io.Information;
            if (data_len < sizeof(FILE_BOTH_DIRECTORY_INFORMATION))
                break;
            data_pos = 0;
        } else {
            data_pos += dir_info->NextEntryOffset;
        }
        numfiles++;
    }
    ok(numfiles < 5, "too many loops\n");
    ok(normal_found > 0, "no normal files found\n");
    ok(hidden_found > 0, "no hidden files found\n");
    ok(system_found > 0, "no system files found\n");

    pNtClose(dirh);
done:
    CloseHandle(normalh);
    CloseHandle(hiddenh);
    CloseHandle(systemh);
    RemoveDirectoryA(dirnameA);
}

START_TEST(directory)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(hntdll,"RtlMultiByteToUnicodeN");

    test_NtQueryDirectoryFile();
}
