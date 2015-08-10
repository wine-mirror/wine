/*
 * Copyright 2014 Yifu Wang for ESRI
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

typedef int MSVCRT_long;
typedef unsigned char MSVCP_bool;

/* xtime */
typedef struct {
    __time64_t sec;
    MSVCRT_long nsec;
} xtime;

typedef struct {
    unsigned page;
    int mb_max;
    int unk;
    BYTE isleadbyte[32];
} _Cvtvec;

struct space_info {
    ULONGLONG capacity;
    ULONGLONG free;
    ULONGLONG available;
};

enum file_type {
    status_unknown, file_not_found, regular_file, directory_file,
    symlink_file, block_file, character_file, fifo_file, socket_file,
    type_unknown
};

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
}

static char* (__cdecl *p_setlocale)(int, const char*);
static int (__cdecl *p__setmbcp)(int);
static int (__cdecl *p_isleadbyte)(int);

static MSVCRT_long (__cdecl *p__Xtime_diff_to_millis2)(const xtime*, const xtime*);
static int (__cdecl *p_xtime_get)(xtime*, int);
static _Cvtvec* (__cdecl *p__Getcvt)(_Cvtvec*);
static void (CDECL *p__Call_once)(int *once, void (CDECL *func)(void));
static void (CDECL *p__Call_onceEx)(int *once, void (CDECL *func)(void*), void *argv);
static void (CDECL *p__Do_call)(void *this);

/* filesystem */
static ULONGLONG(__cdecl *p_tr2_sys__File_size)(char const*);
static ULONGLONG(__cdecl *p_tr2_sys__File_size_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Equivalent)(char const*, char const*);
static int (__cdecl *p_tr2_sys__Equivalent_wchar)(WCHAR const*, WCHAR const*);
static char* (__cdecl *p_tr2_sys__Current_get)(char *);
static WCHAR* (__cdecl *p_tr2_sys__Current_get_wchar)(WCHAR *);
static MSVCP_bool (__cdecl *p_tr2_sys__Current_set)(char const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Current_set_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Make_dir)(char const*);
static int (__cdecl *p_tr2_sys__Make_dir_wchar)(WCHAR const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Remove_dir)(char const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Remove_dir_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Copy_file)(char const*, char const*, MSVCP_bool);
static int (__cdecl *p_tr2_sys__Rename)(char const*, char const*);
static struct space_info (__cdecl *p_tr2_sys__Statvfs)(char const*);
static enum file_type (__cdecl *p_tr2_sys__Stat)(char const*, int *);
static enum file_type (__cdecl *p_tr2_sys__Lstat)(char const*, int *);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HANDLE msvcr;

    msvcp = LoadLibraryA("msvcp120.dll");
    if(!msvcp)
    {
        win_skip("msvcp120.dll not installed\n");
        return FALSE;
    }

    SET(p__Xtime_diff_to_millis2,
            "_Xtime_diff_to_millis2");
    SET(p_xtime_get,
            "xtime_get");
    SET(p__Getcvt,
            "_Getcvt");
    SET(p__Call_once,
            "_Call_once");
    SET(p__Call_onceEx,
            "_Call_onceEx");
    SET(p__Do_call,
            "_Do_call");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_tr2_sys__File_size,
                "?_File_size@sys@tr2@std@@YA_KPEBD@Z");
        SET(p_tr2_sys__File_size_wchar,
                "?_File_size@sys@tr2@std@@YA_KPEB_W@Z");
        SET(p_tr2_sys__Equivalent,
                "?_Equivalent@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Equivalent_wchar,
                "?_Equivalent@sys@tr2@std@@YAHPEB_W0@Z");
        SET(p_tr2_sys__Current_get,
                "?_Current_get@sys@tr2@std@@YAPEADAEAY0BAE@D@Z");
        SET(p_tr2_sys__Current_get_wchar,
                "?_Current_get@sys@tr2@std@@YAPEA_WAEAY0BAE@_W@Z");
        SET(p_tr2_sys__Current_set,
                "?_Current_set@sys@tr2@std@@YA_NPEBD@Z");
        SET(p_tr2_sys__Current_set_wchar,
                "?_Current_set@sys@tr2@std@@YA_NPEB_W@Z");
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPEBD@Z");
        SET(p_tr2_sys__Make_dir_wchar,
                "?_Make_dir@sys@tr2@std@@YAHPEB_W@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPEBD@Z");
        SET(p_tr2_sys__Remove_dir_wchar,
                "?_Remove_dir@sys@tr2@std@@YA_NPEB_W@Z");
        SET(p_tr2_sys__Copy_file,
                "?_Copy_file@sys@tr2@std@@YAHPEBD0_N@Z");
        SET(p_tr2_sys__Rename,
                "?_Rename@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Statvfs,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PEBD@Z");
        SET(p_tr2_sys__Stat,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z");
        SET(p_tr2_sys__Lstat,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z");
    } else {
        SET(p_tr2_sys__File_size,
                "?_File_size@sys@tr2@std@@YA_KPBD@Z");
        SET(p_tr2_sys__File_size_wchar,
                "?_File_size@sys@tr2@std@@YA_KPB_W@Z");
        SET(p_tr2_sys__Equivalent,
                "?_Equivalent@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Equivalent_wchar,
                "?_Equivalent@sys@tr2@std@@YAHPB_W0@Z");
        SET(p_tr2_sys__Current_get,
                "?_Current_get@sys@tr2@std@@YAPADAAY0BAE@D@Z");
        SET(p_tr2_sys__Current_get_wchar,
                "?_Current_get@sys@tr2@std@@YAPA_WAAY0BAE@_W@Z");
        SET(p_tr2_sys__Current_set,
                "?_Current_set@sys@tr2@std@@YA_NPBD@Z");
        SET(p_tr2_sys__Current_set_wchar,
                "?_Current_set@sys@tr2@std@@YA_NPB_W@Z");
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPBD@Z");
        SET(p_tr2_sys__Make_dir_wchar,
                "?_Make_dir@sys@tr2@std@@YAHPB_W@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPBD@Z");
        SET(p_tr2_sys__Remove_dir_wchar,
                "?_Remove_dir@sys@tr2@std@@YA_NPB_W@Z");
        SET(p_tr2_sys__Copy_file,
                "?_Copy_file@sys@tr2@std@@YAHPBD0_N@Z");
        SET(p_tr2_sys__Rename,
                "?_Rename@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Statvfs,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PBD@Z");
        SET(p_tr2_sys__Stat,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z");
        SET(p_tr2_sys__Lstat,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z");
    }

    msvcr = GetModuleHandleA("msvcr120.dll");
    p_setlocale = (void*)GetProcAddress(msvcr, "setlocale");
    p__setmbcp = (void*)GetProcAddress(msvcr, "_setmbcp");
    p_isleadbyte = (void*)GetProcAddress(msvcr, "isleadbyte");
    return TRUE;
}

static void test__Xtime_diff_to_millis2(void)
{
    struct {
        __time64_t sec_before;
        MSVCRT_long nsec_before;
        __time64_t sec_after;
        MSVCRT_long nsec_after;
        MSVCRT_long expect;
    } tests[] = {
        {1, 0, 2, 0, 1000},
        {0, 1000000000, 0, 2000000000, 1000},
        {1, 100000000, 2, 100000000, 1000},
        {1, 100000000, 1, 200000000, 100},
        {0, 0, 0, 1000000000, 1000},
        {0, 0, 0, 1200000000, 1200},
        {0, 0, 0, 1230000000, 1230},
        {0, 0, 0, 1234000000, 1234},
        {0, 0, 0, 1234100000, 1235},
        {0, 0, 0, 1234900000, 1235},
        {0, 0, 0, 1234010000, 1235},
        {0, 0, 0, 1234090000, 1235},
        {0, 0, 0, 1234000001, 1235},
        {0, 0, 0, 1234000009, 1235},
        {0, 0, -1, 0, 0},
        {0, 0, 0, -10000000, 0},
        {0, 0, -1, -100000000, 0},
        {-1, 0, 0, 0, 1000},
        {0, -100000000, 0, 0, 100},
        {-1, -100000000, 0, 0, 1100},
        {0, 0, -1, 2000000000, 1000},
        {0, 0, -2, 2000000000, 0},
        {0, 0, -2, 2100000000, 100}
    };
    int i;
    MSVCRT_long ret;
    xtime t1, t2;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); ++ i)
    {
        t1.sec = tests[i].sec_before;
        t1.nsec = tests[i].nsec_before;
        t2.sec = tests[i].sec_after;
        t2.nsec = tests[i].nsec_after;
        ret = p__Xtime_diff_to_millis2(&t2, &t1);
        ok(ret == tests[i].expect,
                "_Xtime_diff_to_millis2(): test: %d expect: %d, got: %d\n",
                i, tests[i].expect, ret);
    }
}

static void test_xtime_get(void)
{
    static const MSVCRT_long tests[] = {1, 50, 100, 200, 500};
    MSVCRT_long diff;
    xtime before, after;
    int i;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i ++)
    {
        p_xtime_get(&before, 1);
        Sleep(tests[i]);
        p_xtime_get(&after, 1);

        diff = p__Xtime_diff_to_millis2(&after, &before);

        ok(diff >= tests[i],
                "xtime_get() not functioning correctly, test: %d, expect: ge %d, got: %d\n",
                i, tests[i], diff);
    }

    /* Test parameter and return value */
    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 0);
    ok(i == 0, "expect xtime_get() to return 0, got: %d\n", i);
    ok(before.sec == 0xdeadbeef && before.nsec == 0xdeadbeef,
            "xtime_get() shouldn't have modified the xtime struct with the given option\n");

    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 1);
    ok(i == 1, "expect xtime_get() to return 1, got: %d\n", i);
    ok(before.sec != 0xdeadbeef && before.nsec != 0xdeadbeef,
            "xtime_get() should have modified the xtime struct with the given option\n");
}

static void test__Getcvt(void)
{
    _Cvtvec cvtvec;
    int i;

    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 0, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 1, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    todo_wine ok(cvtvec.unk == 1, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    if(!p_setlocale(LC_ALL, ".936")) {
        win_skip("_Getcvt tests\n");
        return;
    }
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    p__setmbcp(936);
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++) {
        BYTE b = 0;
        int j;

        for(j=0; j<8; j++)
            b |= (p_isleadbyte(i*8+j) ? 1 : 0) << j;
        ok(cvtvec.isleadbyte[i] ==b, "cvtvec.isleadbyte[%d] = %x (%x)\n", i, cvtvec.isleadbyte[i], b);
    }
}

static int cnt;
static int once;

static void __cdecl call_once_func(void)
{
    ok(!once, "once != 0\n");
    cnt += 0x10000;
}

static void __cdecl call_once_ex_func(void *arg)
{
    int *i = arg;

    ok(!once, "once != 0\n");
    (*i)++;
}

static DWORD WINAPI call_once_thread(void *arg)
{
    p__Call_once(&once, call_once_func);
    return 0;
}

static DWORD WINAPI call_once_ex_thread(void *arg)
{
    p__Call_onceEx(&once, call_once_ex_func, &cnt);
    return 0;
}

static void test__Call_once(void)
{
    HANDLE h[4];
    int i;

    for(i=0; i<4; i++)
        h[i] = CreateThread(NULL, 0, call_once_thread, &once, 0, NULL);
    ok(WaitForMultipleObjects(4, h, TRUE, INFINITE) == WAIT_OBJECT_0,
            "error waiting for all threads to finish\n");
    ok(cnt == 0x10000, "cnt = %x\n", cnt);
    ok(once == 1, "once = %x\n", once);

    once = cnt = 0;
    for(i=0; i<4; i++)
        h[i] = CreateThread(NULL, 0, call_once_ex_thread, &once, 0, NULL);
    ok(WaitForMultipleObjects(4, h, TRUE, INFINITE) == WAIT_OBJECT_0,
            "error waiting for all threads to finish\n");
    ok(cnt == 1, "cnt = %x\n", cnt);
    ok(once == 1, "once = %x\n", once);
}

static void **vtbl_func0;
#ifdef __i386__
/* TODO: this should be a __thiscall function */
static void __stdcall thiscall_func(void)
{
    cnt = 1;
}
#else
static void __cdecl thiscall_func(void *this)
{
    ok(this == &vtbl_func0, "incorrect this value\n");
    cnt = 1;
}
#endif

static void test__Do_call(void)
{
    void *pfunc = thiscall_func;

    cnt = 0;
    vtbl_func0 = &pfunc;
    p__Do_call(&vtbl_func0);
    ok(cnt == 1, "func was not called\n");
}

static void test_tr2_sys__File_size(void)
{
    ULONGLONG val;
    HANDLE file;
    LARGE_INTEGER file_size;
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f1");
    ok(val == 7, "file_size is %s\n", debugstr_longlong(val));
    val = p_tr2_sys__File_size_wchar(testW);
    ok(val == 7, "file_size is %s\n", debugstr_longlong(val));

    file = CreateFileA("tr2_test_dir/f2", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f2");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));

    val = p_tr2_sys__File_size("tr2_test_dir");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size("tr2_test_dir/not_exists_file");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size(NULL);
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir/f2"), "expect tr2_test_dir/f2 to exist\n");
    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
}

static void test_tr2_sys__Equivalent(void)
{
    int val, i;
    HANDLE file;
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};
    WCHAR testW2[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','2',0};
    struct {
        char const *path1;
        char const *path2;
        int equivalent;
    } tests[] = {
        { NULL, NULL, -1 },
        { NULL, "f1", -1 },
        { "f1", NULL, -1 },
        { "f1", "tr2_test_dir", -1 },
        { "tr2_test_dir", "f1", -1 },
        { "tr2_test_dir", "tr2_test_dir", -1 },
        { "tr2_test_dir/./f1", "tr2_test_dir/f2", 0 },
        { "tr2_test_dir/f1"  , "tr2_test_dir/f1", 1 },
        { "not_exists_file"  , "tr2_test_dir/f1", 0 },
        { "tr2_test_dir\\f1" , "tr2_test_dir/./f1", 1 },
        { "not_exists_file"  , "not_exists_file",  -1 },
        { "tr2_test_dir/f1"  , "not_exists_file",   0 },
        { "tr2_test_dir/../tr2_test_dir/f1", "tr2_test_dir/f1", 1 }
    };

    memset(current_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, current_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    file = CreateFileA("tr2_test_dir/f2", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        val = p_tr2_sys__Equivalent(tests[i].path1, tests[i].path2);
        ok(tests[i].equivalent == val, "tr2_sys__Equivalent(): test %d expect: %d, got %d\n", i+1, tests[i].equivalent, val);
        ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    }

    val = p_tr2_sys__Equivalent_wchar(testW, testW);
    ok(val == 1, "tr2_sys__Equivalent(): expect: 1, got %d\n", val);
    val = p_tr2_sys__Equivalent_wchar(testW, testW2);
    ok(val == 0, "tr2_sys__Equivalent(): expect: 0, got %d\n", val);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir/f2"), "expect tr2_test_dir/f2 to exist\n");
    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static void test_tr2_sys__Current_get(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH], origin_path[MAX_PATH];
    char *temp;
    WCHAR temp_path_wchar[MAX_PATH], current_path_wchar[MAX_PATH];
    WCHAR *temp_wchar;
    memset(origin_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, origin_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);

    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    GetTempPathW(MAX_PATH, temp_path_wchar);
    ok(SetCurrentDirectoryW(temp_path_wchar), "SetCurrentDirectoryW to temp_path_wchar failed\n");
    memset(current_path_wchar, 0, MAX_PATH);
    temp_wchar = p_tr2_sys__Current_get_wchar(current_path_wchar);
    ok(temp_wchar == current_path_wchar, "p_tr2_sys__Current_get_wchar returned different buffer\n");
    temp_wchar[wcslen(temp_wchar)] = '\\';
    ok(!wcscmp(temp_path_wchar, current_path_wchar), "test_tr2_sys__Current_get(): expect: %s, got %s\n", wine_dbgstr_w(temp_path_wchar), wine_dbgstr_w(current_path_wchar));

    ok(SetCurrentDirectoryA(origin_path), "SetCurrentDirectoryA to origin_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Current_set(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH], origin_path[MAX_PATH];
    char *temp;
    WCHAR testW[] = {'.','/',0};
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    memset(origin_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, origin_path);
    temp = p_tr2_sys__Current_get(origin_path);
    ok(temp == origin_path, "p_tr2_sys__Current_get returned different buffer\n");

    ok(p_tr2_sys__Current_set(temp_path), "p_tr2_sys__Current_set to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    ok(p_tr2_sys__Current_set_wchar(testW), "p_tr2_sys__Current_set_wchar to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("not_exisist_dir"), "p_tr2_sys__Current_set to not_exist_dir succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("??invalid_name>>"), "p_tr2_sys__Current_set to ??invalid_name>> succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(p_tr2_sys__Current_set(origin_path), "p_tr2_sys__Current_set to origin_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Make_dir(void)
{
    int ret, i;
    WCHAR testW[] = {'w','d',0};
    struct {
        char const *path;
        int val;
    } tests[] = {
        { "tr2_test_dir", 1 },
        { "tr2_test_dir", 0 },
        { NULL, -1 },
        { "??invalid_name>>", -1 }
    };

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Make_dir(tests[i].path);
        ok(ret == tests[i].val, "tr2_sys__Make_dir(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Make_dir(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
    }
    ret = p_tr2_sys__Make_dir_wchar(testW);
    ok(ret == 1, "tr2_sys__Make_dir(): expect: 1, got %d\n", ret);

    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
    ok(p_tr2_sys__Remove_dir_wchar(testW), "expect wd to exist\n");
}

static void test_tr2_sys__Remove_dir(void)
{
    MSVCP_bool ret;
    int i;
    struct {
        char const *path;
        MSVCP_bool val;
    } tests[] = {
        { "tr2_test_dir", TRUE  },
        { "tr2_test_dir", FALSE },
        { NULL, FALSE },
        { "??invalid_name>>", FALSE }
    };

    ok(p_tr2_sys__Make_dir("tr2_test_dir"), "tr2_sys__Make_dir() failed\n");

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Remove_dir(tests[i].path);
        ok(ret == tests[i].val, "test_tr2_sys__Remove_dir(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Remove_dir(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
    }
}

static void test_tr2_sys__Copy_file(void)
{
    HANDLE file;
    int ret, i;
    LARGE_INTEGER file_size;
    struct {
        char const *source;
        char const *dest;
        MSVCP_bool fail_if_exists;
        int last_error;
        int last_error2;
        MSVCP_bool is_todo;
    } tests[] = {
        { "f1", "f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", FALSE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir", "f1", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED, FALSE },
        { "tr2_test_dir", "tr2_test_dir_copy", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED, FALSE },
        { NULL, "f1", TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER, TRUE },
        { "f1", NULL, TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER, TRUE },
        { "not_exist", "tr2_test_dir", TRUE, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND, FALSE },
        { "f1", "not_exist_dir\\f1_copy", TRUE, ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, FALSE },
        { "f1", "tr2_test_dir", TRUE, ERROR_ACCESS_DENIED, ERROR_FILE_EXISTS, FALSE }
    };

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Copy_file(tests[i].source, tests[i].dest, tests[i].fail_if_exists);
        if(tests[i].is_todo)
            todo_wine ok(ret == tests[i].last_error || ret == tests[i].last_error2,
                    "test_tr2_sys__Copy_file(): test %d expect: %d, got %d\n",
                    i+1, tests[i].last_error, ret);
        else
            ok(ret == tests[i].last_error || ret == tests[i].last_error2,
                    "test_tr2_sys__Copy_file(): test %d expect: %d, got %d\n",
                    i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Copy_file(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].source) == p_tr2_sys__File_size(tests[i].dest),
                    "test_tr2_sys__Copy_file(): test %d failed, two files' size are not equal\n", i+1);
    }

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    ok(DeleteFileA("f1_copy"), "expect f1_copy to exist\n");
    ok(DeleteFileA("tr2_test_dir/f1_copy"), "expect tr2_test_dir/f1 to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
}

static void test_tr2_sys__Rename(void)
{
    int ret, i;
    HANDLE file, h1, h2;
    BY_HANDLE_FILE_INFORMATION info1, info2;
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    LARGE_INTEGER file_size;
    struct {
        char const *old_path;
        char const *new_path;
        int val;
    } tests[] = {
        { "tr2_test_dir\\f1", "tr2_test_dir\\f1_rename", ERROR_SUCCESS },
        { "tr2_test_dir\\f1", NULL, ERROR_INVALID_PARAMETER },
        { "tr2_test_dir\\f1", "tr2_test_dir\\f1_rename", ERROR_FILE_NOT_FOUND },
        { NULL, "tr2_test_dir\\NULL_rename", ERROR_INVALID_PARAMETER },
        { "tr2_test_dir\\f1_rename", "tr2_test_dir\\??invalid_name>>", ERROR_INVALID_NAME },
        { "tr2_test_dir\\not_exist_file", "tr2_test_dir\\not_exist_rename", ERROR_FILE_NOT_FOUND }
    };

    memset(current_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, current_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    ret = p_tr2_sys__Make_dir("tr2_test_dir");

    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("tr2_test_dir\\f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    ret = p_tr2_sys__Rename("tr2_test_dir\\f1", "tr2_test_dir\\f1");
    todo_wine ok(ERROR_SUCCESS == ret, "test_tr2_sys__Rename(): expect: ERROR_SUCCESS, got %d\n", ret);
    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        if(tests[i].val == ERROR_SUCCESS) {
            h1 = CreateFileA(tests[i].old_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h1 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h1, &info1), "GetFileInformationByHandle failed\n");
            CloseHandle(h1);
        }
        SetLastError(0xdeadbeef);
        ret = p_tr2_sys__Rename(tests[i].old_path, tests[i].new_path);
        ok(ret == tests[i].val, "test_tr2_sys__Rename(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Rename(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS) {
            h2 = CreateFileA(tests[i].new_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h2 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h2, &info2), "GetFileInformationByHandle failed\n");
            CloseHandle(h2);
            ok(info1.nFileIndexHigh == info2.nFileIndexHigh
                    && info1.nFileIndexLow == info2.nFileIndexLow,
                    "test_tr2_sys__Rename(): test %d expect two files equivalent\n", i+1);
        }
    }

    file = CreateFileA("tr2_test_dir\\f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    ret = p_tr2_sys__Rename("tr2_test_dir\\f1", "tr2_test_dir\\f1_rename");
    ok(ret == ERROR_ALREADY_EXISTS, "test_tr2_sys__Rename(): expect: ERROR_ALREADY_EXISTS, got %d\n", ret);
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1") == 7, "test_tr2_sys__Rename(): expect: 7, got %s\n", debugstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1")));
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1_rename") == 0, "test_tr2_sys__Rename(): expect: 0, got %s\n",debugstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1_rename")));

    ok(DeleteFileA("tr2_test_dir\\f1"), "expect f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir\\f1_rename"), "expect f1_rename to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect %d got %d\n", 1, ret);
    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static void test_tr2_sys__Statvfs(void)
{
    struct space_info info;
    char current_path[MAX_PATH];
    memset(current_path, 0, MAX_PATH);
    p_tr2_sys__Current_get(current_path);

    info = p_tr2_sys__Statvfs(current_path);
    ok(info.capacity >= info.free, "test_tr2_sys__Statvfs(): info.capacity < info.free\n");
    ok(info.free >= info.available, "test_tr2_sys__Statvfs(): info.free < info.available\n");

    info = p_tr2_sys__Statvfs(NULL);
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, debugstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, debugstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, debugstr_longlong(info.free));

    info = p_tr2_sys__Statvfs("not_exist");
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, debugstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, debugstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, debugstr_longlong(info.free));
}

static void test_tr2_sys__Stat(void)
{
    int i, err_code, ret;
    HANDLE file;
    enum file_type val;
    struct {
        char const *path;
        enum file_type ret;
        int err_code;
        int is_todo;
    } tests[] = {
        { NULL, status_unknown, ERROR_INVALID_PARAMETER, FALSE },
        { "tr2_test_dir",    directory_file, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1",  regular_file, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\not_exist_file  ", file_not_found, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\??invalid_name>>", file_not_found, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1_link" ,   regular_file, ERROR_SUCCESS, TRUE },
        { "tr2_test_dir\\dir_link", directory_file, ERROR_SUCCESS, TRUE },
    };

    CreateDirectoryA("tr2_test_dir", NULL);
    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(CloseHandle(file), "CloseHandle\n");
    SetLastError(0xdeadbeef);
    ret = CreateSymbolicLinkA("tr2_test_dir/f1_link", "tr2_test_dir/f1", 0);
    if(!ret && (GetLastError()==ERROR_PRIVILEGE_NOT_HELD||GetLastError()==ERROR_INVALID_FUNCTION)) {
        tests[5].ret = tests[6].ret = file_not_found;
        win_skip("Privilege not held or symbolic link not supported, skipping symbolic link tests.\n");
    }else {
        ok(ret, "CreateSymbolicLinkA failed\n");
        ok(CreateSymbolicLinkA("tr2_test_dir/dir_link", "tr2_test_dir", 1), "CreateSymbolicLinkA failed\n");
    }

    file = CreateNamedPipeA("\\\\.\\PiPe\\tests_pipe.c",
            PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 2, 1024, 1024,
            NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Stat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    todo_wine ok(regular_file == val, "tr2_sys__Stat(): expect: regular_file, got %d\n", val);
    todo_wine ok(ERROR_SUCCESS == err_code, "tr2_sys__Stat(): err_code expect: ERROR_SUCCESS, got %d\n", err_code);
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    ok(status_unknown == val, "tr2_sys__Lstat(): expect: status_unknown, got %d\n", val);
    todo_wine ok(ERROR_PIPE_BUSY == err_code, "tr2_sys__Lstat(): err_code expect: ERROR_PIPE_BUSY, got %d\n", err_code);
    ok(CloseHandle(file), "CloseHandle\n");
    file = CreateNamedPipeA("\\\\.\\PiPe\\tests_pipe.c",
            PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 2, 1024, 1024,
            NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    todo_wine ok(regular_file == val, "tr2_sys__Lstat(): expect: regular_file, got %d\n", val);
    todo_wine ok(ERROR_SUCCESS == err_code, "tr2_sys__Lstat(): err_code expect: ERROR_SUCCESS, got %d\n", err_code);
    ok(CloseHandle(file), "CloseHandle\n");

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        err_code = 0xdeadbeef;
        val = p_tr2_sys__Stat(tests[i].path, &err_code);
        if(tests[i].is_todo)
            todo_wine ok(tests[i].ret == val, "tr2_sys__Stat(): test %d expect: %d, got %d\n",
                    i+1, tests[i].ret, val);
        else
            ok(tests[i].ret == val, "tr2_sys__Stat(): test %d expect: %d, got %d\n", i+1, tests[i].ret, val);
        ok(tests[i].err_code == err_code, "tr2_sys__Stat(): test %d err_code expect: %d, got %d\n",
                i+1, tests[i].err_code, err_code);

        /* test tr2_sys__Lstat */
        err_code = 0xdeadbeef;
        val = p_tr2_sys__Lstat(tests[i].path, &err_code);
        if(tests[i].is_todo)
            todo_wine ok(tests[i].ret == val, "tr2_sys__Lstat(): test %d expect: %d, got %d\n",
                    i+1, tests[i].ret, val);
        else
            ok(tests[i].ret == val, "tr2_sys__Lstat(): test %d expect: %d, got %d\n", i+1, tests[i].ret, val);

        ok(tests[i].err_code == err_code, "tr2_sys__Lstat(): test %d err_code expect: %d, got %d\n",
                i+1, tests[i].err_code, err_code);
    }

    if(ret) {
        todo_wine ok(DeleteFileA("tr2_test_dir/f1_link"), "expect tr2_test_dir/f1_link to exist\n");
        todo_wine ok(RemoveDirectoryA("tr2_test_dir/dir_link"), "expect tr2_test_dir/dir_link to exist\n");
    }
    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(RemoveDirectoryA("tr2_test_dir"), "expect tr2_test_dir to exist\n");
}

START_TEST(msvcp120)
{
    if(!init()) return;
    test__Xtime_diff_to_millis2();
    test_xtime_get();
    test__Getcvt();
    test__Call_once();
    test__Do_call();

    test_tr2_sys__File_size();
    test_tr2_sys__Equivalent();
    test_tr2_sys__Current_get();
    test_tr2_sys__Current_set();
    test_tr2_sys__Make_dir();
    test_tr2_sys__Remove_dir();
    test_tr2_sys__Copy_file();
    test_tr2_sys__Rename();
    test_tr2_sys__Statvfs();
    test_tr2_sys__Stat();
    FreeLibrary(msvcp);
}
