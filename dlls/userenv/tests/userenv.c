/*
 * Unit test suite for userenv functions
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"

#include "userenv.h"

#include "wine/test.h"

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))
#define expect_env(EXPECTED,GOT,VAR) ok((GOT)==(EXPECTED), "Expected %d, got %d for %s (%d)\n", (EXPECTED), (GOT), (VAR), j)
#define expect_gle(EXPECTED) ok(GetLastError() == (EXPECTED), "Expected %d, got %d\n", (EXPECTED), GetLastError())

struct profile_item
{
    const char * name;
    const int todo[4];
};

/* Helper function for retrieving environment variables */
static BOOL get_env(const WCHAR * env, const char * var, char ** result)
{
    const WCHAR * p = env;
    int envlen, varlen, buflen;
    char buf[256];

    if (!env || !var || !result) return FALSE;

    varlen = strlen(var);
    do
    {
        if (!WideCharToMultiByte( CP_ACP, 0, p, -1, buf, sizeof(buf), NULL, NULL )) buf[sizeof(buf)-1] = 0;
        envlen = strlen(buf);
        if (CompareStringA(GetThreadLocale(), NORM_IGNORECASE|LOCALE_USE_CP_ACP, buf, min(envlen, varlen), var, varlen) == CSTR_EQUAL)
        {
            if (buf[varlen] == '=')
            {
                buflen = strlen(buf);
                *result = HeapAlloc(GetProcessHeap(), 0, buflen + 1);
                if (!*result) return FALSE;
                memcpy(*result, buf, buflen + 1);
                return TRUE;
            }
        }
        while (*p) p++;
        p++;
    } while (*p);
    return FALSE;
}

static void test_create_env(void)
{
    BOOL r;
    HANDLE htok;
    WCHAR * env[4];
    char * st;
    int i, j;

    static const struct profile_item common_vars[] = {
        { "ComSpec", { 1, 1, 0, 0 } },
        { "COMPUTERNAME", { 1, 1, 1, 1 } },
        { "NUMBER_OF_PROCESSORS", { 1, 1, 0, 0 } },
        { "OS", { 1, 1, 0, 0 } },
        { "PROCESSOR_ARCHITECTURE", { 1, 1, 0, 0 } },
        { "PROCESSOR_IDENTIFIER", { 1, 1, 0, 0 } },
        { "PROCESSOR_LEVEL", { 1, 1, 0, 0 } },
        { "PROCESSOR_REVISION", { 1, 1, 0, 0 } },
        { "SystemDrive", { 1, 1, 0, 0 } },
        { "SystemRoot", { 1, 1, 0, 0 } },
        { "windir", { 1, 1, 0, 0 } }
    };
    static const struct profile_item common_post_nt4_vars[] = {
        { "ALLUSERSPROFILE", { 1, 1, 0, 0 } },
        { "TEMP", { 1, 1, 0, 0 } },
        { "TMP", { 1, 1, 0, 0 } },
        { "CommonProgramFiles", { 1, 1, 0, 0 } },
        { "ProgramFiles", { 1, 1, 0, 0 } }
    };
    static const struct profile_item htok_vars[] = {
        { "PATH", { 1, 1, 0, 0 } },
        { "USERPROFILE", { 1, 1, 0, 0 } }
    };

    r = SetEnvironmentVariableA("WINE_XYZZY", "ZZYZX");
    expect(TRUE, r);

    if (0)
    {
        /* Crashes on NT4 */
        r = CreateEnvironmentBlock(NULL, NULL, FALSE);
        expect(FALSE, r);
    }

    r = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &htok);
    expect(TRUE, r);

    if (0)
    {
        /* Crashes on NT4 */
        r = CreateEnvironmentBlock(NULL, htok, FALSE);
        expect(FALSE, r);
    }

    r = CreateEnvironmentBlock((LPVOID) &env[0], NULL, FALSE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[1], htok, FALSE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[2], NULL, TRUE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[3], htok, TRUE);
    expect(TRUE, r);

    /* Test for common environment variables (NT4 and higher) */
    for (i = 0; i < sizeof(common_vars)/sizeof(common_vars[0]); i++)
    {
        for (j = 0; j < 4; j++)
        {
            r = get_env(env[j], common_vars[i].name, &st);
            if (common_vars[i].todo[j])
                todo_wine expect_env(TRUE, r, common_vars[i].name);
            else
                expect_env(TRUE, r, common_vars[i].name);
            if (r) HeapFree(GetProcessHeap(), 0, st);
        }
    }

    /* Test for common environment variables (post NT4) */
    if (!GetEnvironmentVariableA("ALLUSERSPROFILE", NULL, 0))
    {
        win_skip("Some environment variables are not present on NT4\n");
    }
    else
    {
        for (i = 0; i < sizeof(common_post_nt4_vars)/sizeof(common_post_nt4_vars[0]); i++)
        {
            for (j = 0; j < 4; j++)
            {
                r = get_env(env[j], common_post_nt4_vars[i].name, &st);
                if (common_post_nt4_vars[i].todo[j])
                    todo_wine expect_env(TRUE, r, common_post_nt4_vars[i].name);
                else
                    expect_env(TRUE, r, common_post_nt4_vars[i].name);
                if (r) HeapFree(GetProcessHeap(), 0, st);
            }
        }
    }

    /* Test for environment variables with values that depends on htok */
    for (i = 0; i < sizeof(htok_vars)/sizeof(htok_vars[0]); i++)
    {
        for (j = 0; j < 4; j++)
        {
            r = get_env(env[j], htok_vars[i].name, &st);
            if (htok_vars[i].todo[j])
                todo_wine expect_env(TRUE, r, htok_vars[i].name);
            else
                expect_env(TRUE, r, htok_vars[i].name);
            if (r) HeapFree(GetProcessHeap(), 0, st);
        }
    }

    r = get_env(env[0], "WINE_XYZZY", &st);
    expect(FALSE, r);

    r = get_env(env[1], "WINE_XYZZY", &st);
    expect(FALSE, r);

    r = get_env(env[2], "WINE_XYZZY", &st);
    expect(TRUE, r);
    if (r) HeapFree(GetProcessHeap(), 0, st);

    r = get_env(env[3], "WINE_XYZZY", &st);
    expect(TRUE, r);
    if (r) HeapFree(GetProcessHeap(), 0, st);

    for (i = 0; i < sizeof(env) / sizeof(env[0]); i++)
    {
        r = DestroyEnvironmentBlock(env[i]);
        expect(TRUE, r);
    }
}

static void test_get_profiles_dir(void)
{
    static const char ProfileListA[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
    static const char ProfilesDirectory[] = "ProfilesDirectory";
    BOOL r;
    DWORD cch, profiles_len;
    LONG l;
    HKEY key;
    char *profiles_dir, *buf, small_buf[1];

    l = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ProfileListA, 0, KEY_READ, &key);
    if (l)
    {
        win_skip("No ProfileList key (Win9x), skipping tests\n");
        return;
    }
    l = RegQueryValueExA(key, ProfilesDirectory, NULL, NULL, NULL, &cch);
    if (l)
    {
        win_skip("No ProfilesDirectory value, skipping tests\n");
        return;
    }
    buf = HeapAlloc(GetProcessHeap(), 0, cch);
    RegQueryValueExA(key, ProfilesDirectory, NULL, NULL, (BYTE *)buf, &cch);
    RegCloseKey(key);
    profiles_len = ExpandEnvironmentStringsA(buf, NULL, 0);
    profiles_dir = HeapAlloc(GetProcessHeap(), 0, profiles_len);
    ExpandEnvironmentStringsA(buf, profiles_dir, profiles_len);
    HeapFree(GetProcessHeap(), 0, buf);

    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryA(NULL, NULL);
    expect(FALSE, r);
    expect_gle(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryA(NULL, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    cch = 1;
    r = GetProfilesDirectoryA(small_buf, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INSUFFICIENT_BUFFER);
    /* MSDN claims the returned character count includes the NULL terminator
     * when the buffer is too small, but that's not in fact what gets returned.
     */
    ok(cch == profiles_len - 1, "expected %d, got %d\n", profiles_len - 1, cch);
    /* Allocate one more character than the return value to prevent a buffer
     * overrun.
     */
    buf = HeapAlloc(GetProcessHeap(), 0, cch + 1);
    r = GetProfilesDirectoryA(buf, &cch);
    /* Rather than a BOOL, the return value is also the number of characters
     * stored in the buffer.
     */
    expect(profiles_len - 1, r);
    ok(!strcmp(buf, profiles_dir), "expected %s, got %s\n", profiles_dir, buf);

    HeapFree(GetProcessHeap(), 0, buf);
    HeapFree(GetProcessHeap(), 0, profiles_dir);
}

START_TEST(userenv)
{
    test_create_env();
    test_get_profiles_dir();
}
