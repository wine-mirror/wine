/*
 * Copyright 2015 YongHao Hu
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

#include <locale.h>
#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"

typedef unsigned char MSVCP_bool;

#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
}

static int (__cdecl *p_tr2_sys__Make_dir)(char const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Remove_dir)(char const*);
static __int64 (__cdecl *p_tr2_sys__Last_write_time)(char const*);
static void (__cdecl *p_tr2_sys__Last_write_time_set)(char const*, __int64);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp110.dll");
    if(!msvcp)
    {
        win_skip("msvcp110.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPEBD@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPEBD@Z");
        SET(p_tr2_sys__Last_write_time,
                "?_Last_write_time@sys@tr2@std@@YA_JPEBD@Z");
        SET(p_tr2_sys__Last_write_time_set,
                "?_Last_write_time@sys@tr2@std@@YAXPEBD_J@Z");
    } else {
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPBD@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPBD@Z");
        SET(p_tr2_sys__Last_write_time,
                "?_Last_write_time@sys@tr2@std@@YA_JPBD@Z");
        SET(p_tr2_sys__Last_write_time_set,
                "?_Last_write_time@sys@tr2@std@@YAXPBD_J@Z");
    }
    return TRUE;
}

static void test_tr2_sys__Last_write_time(void)
{
    HANDLE file;
    int ret;
    FILETIME lwt;
    __int64 last_write_time, newtime, margin_of_error = 10 * TICKSPERSEC;
    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    last_write_time = p_tr2_sys__Last_write_time("tr2_test_dir/f1");
    newtime = last_write_time + 222222;
    p_tr2_sys__Last_write_time_set("tr2_test_dir/f1", newtime);
    ok(last_write_time != p_tr2_sys__Last_write_time("tr2_test_dir/f1"),
            "last_write_time before modfied should not equal to last_write_time %s\n",
            debugstr_longlong(last_write_time));

    /* test the formula */
    file = CreateFileA("tr2_test_dir/f1", 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(GetFileTime(file, 0, 0, &lwt), "GetFileTime failed\n");
    CloseHandle(file);
    last_write_time = (((__int64)lwt.dwHighDateTime)<< 32) + lwt.dwLowDateTime;
    last_write_time -= TICKS_1601_TO_1970;
    last_write_time /= TICKSPERSEC;
    ok(newtime-margin_of_error<=last_write_time && last_write_time<=newtime+margin_of_error,
            "don't fit the formula, last_write_time is %s\n", debugstr_longlong(last_write_time));

    newtime = 0;
    p_tr2_sys__Last_write_time_set("tr2_test_dir/f1", newtime);
    newtime = p_tr2_sys__Last_write_time("tr2_test_dir/f1");
    file = CreateFileA("tr2_test_dir/f1", 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(GetFileTime(file, 0, 0, &lwt), "GetFileTime failed\n");
    CloseHandle(file);
    last_write_time = (((__int64)lwt.dwHighDateTime)<< 32) + lwt.dwLowDateTime;
    last_write_time -= TICKS_1601_TO_1970;
    last_write_time /= TICKSPERSEC;
    ok(newtime-margin_of_error<=last_write_time && last_write_time<=newtime+margin_of_error,
            "don't fit the formula, last_write_time is %s\n", debugstr_longlong(last_write_time));

    newtime = 123456789;
    p_tr2_sys__Last_write_time_set("tr2_test_dir/f1", newtime);
    newtime = p_tr2_sys__Last_write_time("tr2_test_dir/f1");
    file = CreateFileA("tr2_test_dir/f1", 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(GetFileTime(file, 0, 0, &lwt), "GetFileTime failed\n");
    CloseHandle(file);
    last_write_time = (((__int64)lwt.dwHighDateTime)<< 32) + lwt.dwLowDateTime;
    last_write_time -= TICKS_1601_TO_1970;
    last_write_time /= TICKSPERSEC;
    ok(newtime-margin_of_error<=last_write_time && last_write_time<=newtime+margin_of_error,
            "don't fit the formula, last_write_time is %s\n", debugstr_longlong(last_write_time));

    errno = 0xdeadbeef;
    last_write_time = p_tr2_sys__Last_write_time("not_exist");
    ok(errno == 0xdeadbeef, "tr2_sys__Last_write_time(): errno expect 0xdeadbeef, got %d\n", errno);
    ok(last_write_time == 0, "expect 0 got %s\n", debugstr_longlong(last_write_time));
    last_write_time = p_tr2_sys__Last_write_time(NULL);
    ok(last_write_time == 0, "expect 0 got %s\n", debugstr_longlong(last_write_time));

    p_tr2_sys__Last_write_time_set("not_exist", newtime);
    errno = 0xdeadbeef;
    p_tr2_sys__Last_write_time_set(NULL, newtime);
    ok(errno == 0xdeadbeef, "tr2_sys__Last_write_time(): errno expect 0xdeadbeef, got %d\n", errno);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
}

START_TEST(msvcp110)
{
    if(!init()) return;
    test_tr2_sys__Last_write_time();
    FreeLibrary(msvcp);
}
