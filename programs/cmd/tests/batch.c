/*
 * Copyright 2009 Dan Kegel
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include <windows.h>
#include <stdio.h>

#include "wine/test.h"

static char workdir[MAX_PATH];
static DWORD workdir_len;

/* Substitute escaped spaces with real ones */
static const char* replace_escaped_spaces(const char *data, DWORD size, DWORD *new_size)
{
    static const char escaped_space[] = {'@','s','p','a','c','e','@','\0'};
    const char *a, *b;
    char *new_data;
    DWORD len_space = sizeof(escaped_space) -1;

    a = b = data;
    *new_size = 0;

    new_data = HeapAlloc(GetProcessHeap(), 0, size*sizeof(char));
    ok(new_data != NULL, "HeapAlloc failed\n");
    if(!new_data)
        return NULL;

    while( (b = strstr(a, escaped_space)) )
    {
        strncpy(new_data + *new_size, a, b-a + 1);
        *new_size += b-a + 1;
        new_data[*new_size - 1] = ' ';
        a = b + len_space;
    }

    strncpy(new_data + *new_size, a, strlen(a) + 1);
    *new_size += strlen(a);

    return new_data;
}

static BOOL run_cmd(const char *cmd_data, DWORD cmd_size)
{
    SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, TRUE};
    char command[] = "test.cmd";
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE file,fileerr;
    DWORD size;
    BOOL bres;

    file = CreateFileA("test.cmd", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    bres = WriteFile(file, cmd_data, cmd_size, &size, NULL);
    CloseHandle(file);
    ok(bres, "Could not write to file: %u\n", GetLastError());
    if(!bres)
        return FALSE;

    file = CreateFileA("test.out", GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, &sa, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    fileerr = CreateFileA("test.err", GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, &sa, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(fileerr != INVALID_HANDLE_VALUE, "CreateFile stderr failed\n");
    if(fileerr == INVALID_HANDLE_VALUE)
        return FALSE;

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = file;
    si.hStdError = fileerr;
    bres = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    ok(bres, "CreateProcess failed: %u\n", GetLastError());
    if(!bres) {
        DeleteFileA("test.out");
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(file);
    CloseHandle(fileerr);
    DeleteFileA("test.cmd");
    return TRUE;
}

static DWORD map_file(const char *file_name, const char **ret)
{
    HANDLE file, map;
    DWORD size;

    file = CreateFileA(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %08x\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return 0;

    size = GetFileSize(file, NULL);

    map = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    ok(map != NULL, "CreateFileMapping(%s) failed: %u\n", file_name, GetLastError());
    if(!map)
        return 0;

    *ret = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    ok(*ret != NULL, "MapViewOfFile failed: %u\n", GetLastError());
    CloseHandle(map);
    if(!*ret)
        return 0;

    return size;
}

static const char *compare_line(const char *out_line, const char *out_end, const char *exp_line,
        const char *exp_end)
{
    const char *out_ptr = out_line, *exp_ptr = exp_line;
    const char *err = NULL;

    static const char pwd_cmd[] = {'@','p','w','d','@'};
    static const char todo_space_cmd[] = {'@','t','o','d','o','_','s','p','a','c','e','@'};
    static const char space_cmd[] = {'@','s','p','a','c','e','@'};
    static const char or_broken_cmd[] = {'@','o','r','_','b','r','o','k','e','n','@'};

    while(exp_ptr < exp_end) {
        if(*exp_ptr == '@') {
            if(exp_ptr+sizeof(pwd_cmd) <= exp_end
                    && !memcmp(exp_ptr, pwd_cmd, sizeof(pwd_cmd))) {
                exp_ptr += sizeof(pwd_cmd);
                if(out_end-out_ptr < workdir_len
                   || (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, out_ptr, workdir_len,
                       workdir, workdir_len) != CSTR_EQUAL)) {
                    err = out_ptr;
                }else {
                    out_ptr += workdir_len;
                    continue;
                }
            }else if(exp_ptr+sizeof(todo_space_cmd) <= exp_end
                    && !memcmp(exp_ptr, todo_space_cmd, sizeof(todo_space_cmd))) {
                exp_ptr += sizeof(todo_space_cmd);
                todo_wine ok(*out_ptr == ' ', "expected space\n");
                if(out_ptr < out_end && *out_ptr == ' ')
                    out_ptr++;
                continue;
            }else if(exp_ptr+sizeof(space_cmd) <= exp_end
                    && !memcmp(exp_ptr, space_cmd, sizeof(space_cmd))) {
                exp_ptr += sizeof(space_cmd);
                ok(*out_ptr == ' ', "expected space\n");
                if(out_ptr < out_end && *out_ptr == ' ')
                    out_ptr++;
                continue;
            }else if(exp_ptr+sizeof(or_broken_cmd) <= exp_end
                     && !memcmp(exp_ptr, or_broken_cmd, sizeof(or_broken_cmd))) {
                exp_ptr = exp_end;
                continue;
            }
        }else if(out_ptr == out_end || *out_ptr != *exp_ptr) {
            err = out_ptr;
        }

        if(err) {
            if(!broken(1))
                return err;

            while(exp_ptr+sizeof(or_broken_cmd) <= exp_end && memcmp(exp_ptr, or_broken_cmd, sizeof(or_broken_cmd)))
                exp_ptr++;
            if(!exp_ptr)
                return err;

            exp_ptr += sizeof(or_broken_cmd);
            out_ptr = out_line;
            err = NULL;
            continue;
        }

        exp_ptr++;
        out_ptr++;
    }

    return exp_ptr == exp_end ? NULL : out_ptr;
}

static void test_output(const char *out_data, DWORD out_size, const char *exp_data, DWORD exp_size)
{
    const char *out_ptr = out_data, *exp_ptr = exp_data, *out_nl, *exp_nl, *err;
    DWORD line = 0;

    while(out_ptr < out_data+out_size && exp_ptr < exp_data+exp_size) {
        line++;

        for(exp_nl = exp_ptr; exp_nl < exp_data+exp_size && *exp_nl != '\r' && *exp_nl != '\n'; exp_nl++);
        for(out_nl = out_ptr; out_nl < out_data+out_size && *out_nl != '\r' && *out_nl != '\n'; out_nl++);

        err = compare_line(out_ptr, out_nl, exp_ptr, exp_nl);
        if(err == out_nl)
            ok(0, "unexpected end of line %d (got '%.*s', wanted '%.*s')\n",
               line, (int)(out_nl-out_ptr), out_ptr, (int)(exp_nl-exp_ptr), exp_ptr);
        else if(err)
            ok(0, "unexpected char 0x%x position %d in line %d (got '%.*s', wanted '%.*s')\n",
               *err, (int)(err-out_ptr), line, (int)(out_nl-out_ptr), out_ptr, (int)(exp_nl-exp_ptr), exp_ptr);

        exp_ptr = exp_nl+1;
        out_ptr = out_nl+1;
        if(out_nl+1 < out_data+out_size && out_nl[0] == '\r' && out_nl[1] == '\n')
            out_ptr++;
        if(exp_nl+1 < exp_data+exp_size && exp_nl[0] == '\r' && exp_nl[1] == '\n')
            exp_ptr++;
    }

    ok(exp_ptr >= exp_data+exp_size, "unexpected end of output in line %d, missing %s\n", line, exp_ptr);
    ok(out_ptr >= out_data+out_size, "too long output, got additional %s\n", out_ptr);
}

static void run_test(const char *cmd_data, DWORD cmd_size, const char *exp_data, DWORD exp_size)
{
    const char *out_data, *actual_cmd_data;
    DWORD out_size, actual_cmd_size;

    actual_cmd_data = replace_escaped_spaces(cmd_data, cmd_size, &actual_cmd_size);
    if(!actual_cmd_size || !actual_cmd_data)
        goto cleanup;

    if(!run_cmd(actual_cmd_data, actual_cmd_size))
        goto cleanup;

    out_size = map_file("test.out", &out_data);
    if(out_size) {
        test_output(out_data, out_size, exp_data, exp_size);
        UnmapViewOfFile(out_data);
    }
    DeleteFileA("test.out");
    DeleteFileA("test.err");

cleanup:
    HeapFree(GetProcessHeap(), 0, (LPVOID)actual_cmd_data);
}

static void run_from_file(char *file_name)
{
    char out_name[MAX_PATH];
    const char *test_data, *out_data;
    DWORD test_size, out_size;

    test_size = map_file(file_name, &test_data);
    if(!test_size) {
        ok(0, "Could not map file %s: %u\n", file_name, GetLastError());
        return;
    }

    sprintf(out_name, "%s.exp", file_name);
    out_size = map_file(out_name, &out_data);
    if(!out_size) {
        ok(0, "Could not map file %s: %u\n", out_name, GetLastError());
        UnmapViewOfFile(test_data);
        return;
    }

    run_test(test_data, test_size, out_data, out_size);

    UnmapViewOfFile(test_data);
    UnmapViewOfFile(out_data);
}

static DWORD load_resource(const char *name, const char *type, const char **ret)
{
    const char *res;
    HRSRC src;
    DWORD size;

    src = FindResourceA(NULL, name, type);
    ok(src != NULL, "Could not find resource %s: %u\n", name, GetLastError());
    if(!src)
        return 0;

    res = LoadResource(NULL, src);
    size = SizeofResource(NULL, src);
    while(size && !res[size-1])
        size--;

    *ret = res;
    return size;
}

static BOOL WINAPI test_enum_proc(HMODULE module, LPCTSTR type, LPSTR name, LONG_PTR param)
{
    const char *cmd_data, *out_data;
    DWORD cmd_size, out_size;
    char res_name[100];

    trace("running %s test...\n", name);

    cmd_size = load_resource(name, type, &cmd_data);
    if(!cmd_size)
        return TRUE;

    sprintf(res_name, "%s.exp", name);
    out_size = load_resource(res_name, "TESTOUT", &out_data);
    if(!out_size)
        return TRUE;

    run_test(cmd_data, cmd_size, out_data, out_size);
    return TRUE;
}

static int cmd_available(void)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmd[] = "cmd /c exit 0";

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return TRUE;
    }
    return FALSE;
}

START_TEST(batch)
{
    int argc;
    char **argv;

    if (!cmd_available()) {
        win_skip("cmd not installed, skipping cmd tests\n");
        return;
    }

    workdir_len = GetCurrentDirectoryA(sizeof(workdir), workdir);

    argc = winetest_get_mainargs(&argv);
    if(argc > 2)
        run_from_file(argv[2]);
    else
        EnumResourceNamesA(NULL, "TESTCMD", test_enum_proc, 0);
}
