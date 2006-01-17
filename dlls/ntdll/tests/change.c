/*
 * File change notification tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
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
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <winerror.h>
#include <stdio.h>
#include "wine/test.h"

typedef NTSTATUS (WINAPI *fnNtNotifyChangeDirectoryFile)(
                          HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,
                          PIO_STATUS_BLOCK,PVOID,ULONG,ULONG,BOOLEAN);
fnNtNotifyChangeDirectoryFile pNtNotifyChangeDirectoryFile;

static void test_ntncdf(void)
{
    HMODULE hntdll = GetModuleHandle("ntdll");
    NTSTATUS r;
    HANDLE hdir, hEvent;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    IO_STATUS_BLOCK iosb;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };

    pNtNotifyChangeDirectoryFile = (fnNtNotifyChangeDirectoryFile) 
        GetProcAddress(hntdll, "NtNotifyChangeDirectoryFile");
    if (!pNtNotifyChangeDirectoryFile)
        return;

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );
    
    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    todo_wine {
    r = pNtNotifyChangeDirectoryFile(NULL,NULL,NULL,NULL,NULL,NULL,0,0,0);
    ok(r==STATUS_ACCESS_VIOLATION, "should return access violation\n");
    }

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    hEvent = CreateEvent( NULL, 0, 0, NULL );

    todo_wine {
    r = pNtNotifyChangeDirectoryFile(hdir,NULL,NULL,NULL,&iosb,NULL,0,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,NULL,0,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    filter |= FILE_NOTIFY_CHANGE_SIZE;
    filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    filter |= FILE_NOTIFY_CHANGE_CREATION;
    filter |= FILE_NOTIFY_CHANGE_SECURITY;

    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should return invalid parameter\n");
    }

    r = WaitForSingleObject( hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "event ready\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    todo_wine {
    r = WaitForSingleObject( hEvent, 0 );
    ok( r == WAIT_OBJECT_0, "event ready\n" );

    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,NULL,0,filter,0);
    ok(r==STATUS_PENDING, "should return invalid parameter\n");

    r = WaitForSingleObject( hdir, 0 );
    ok( r == STATUS_TIMEOUT, "event ready\n" );
    }

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( hdir, 0 );
    ok( r == WAIT_OBJECT_0, "event ready\n" );

    todo_wine {
    r = RemoveDirectoryW( path );
    ok( r == FALSE, "failed to remove directory\n");

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
    }
}

START_TEST(change)
{
    test_ntncdf();
}
