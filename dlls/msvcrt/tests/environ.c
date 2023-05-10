/*
 * Unit tests for C library environment routines
 *
 * Copyright 2004 Mike Hearn <mh@codeweavers.com>
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

#include "wine/test.h"
#include <errno.h>
#include <stdlib.h>
#include <process.h>

static const char *a_very_long_env_string =
 "LIBRARY_PATH="
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/;"
 "/mingw/lib/gcc/mingw32/3.4.2/;"
 "/usr/lib/gcc/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../../mingw32/lib/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../../mingw32/lib/;"
 "/mingw/mingw32/lib/mingw32/3.4.2/;"
 "/mingw/mingw32/lib/;"
 "/mingw/lib/mingw32/3.4.2/;"
 "/mingw/lib/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../;"
 "/mingw/lib/mingw32/3.4.2/;"
 "/mingw/lib/;"
 "/lib/mingw32/3.4.2/;"
 "/lib/;"
 "/usr/lib/mingw32/3.4.2/;"
 "/usr/lib/";

static char ***(__cdecl *p__p__environ)(void);
static WCHAR ***(__cdecl *p__p__wenviron)(void);
static void (__cdecl *p_get_environ)(char ***);
static void (__cdecl *p_get_wenviron)(WCHAR ***);
static errno_t (__cdecl *p_putenv_s)(const char*, const char*);
static errno_t (__cdecl *p_wputenv_s)(const wchar_t*, const wchar_t*);
static errno_t (__cdecl *p_getenv_s)(size_t*, char*, size_t, const char*);

static char ***p_environ;
static WCHAR ***p_wenviron;

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__p__environ = (void *)GetProcAddress(hmod, "__p__environ");
    p__p__wenviron = (void *)GetProcAddress(hmod, "__p__wenviron");
    p_environ = (void *)GetProcAddress(hmod, "_environ");
    p_wenviron = (void *)GetProcAddress(hmod, "_wenviron");
    p_get_environ = (void *)GetProcAddress(hmod, "_get_environ");
    p_get_wenviron = (void *)GetProcAddress(hmod, "_get_wenviron");
    p_putenv_s = (void *)GetProcAddress(hmod, "_putenv_s");
    p_wputenv_s = (void *)GetProcAddress(hmod, "_wputenv_s");
    p_getenv_s = (void *)GetProcAddress(hmod, "getenv_s");
}

static void test_system(void)
{
    int ret = system(NULL);
    ok(ret == 1, "Expected system to return 1, got %d\n", ret);

    ret = system("echo OK");
    ok(ret == 0, "Expected system to return 0, got %d\n", ret);
}

static void test__environ(void)
{
    int argc;
    char **argv, **envp = NULL;
    int i, mode = 0;

    ok( p_environ != NULL, "Expected the pointer to _environ to be non-NULL\n" );
    if (p_environ)
        ok( *p_environ != NULL, "Expected _environ to be initialized on startup\n" );

    if (!p_environ || !*p_environ)
    {
        skip( "_environ pointers are not valid\n" );
        return;
    }

    if (sizeof(void*) != sizeof(int))
        ok( !p__p__environ, "__p__environ() should be 32-bit only\n");
    else
        ok( *p__p__environ() == *p_environ, "Expected _environ pointers to be identical\n" );

    if (p_get_environ)
    {
        char **retptr;
        p_get_environ(&retptr);
        ok( retptr == *p_environ,
            "Expected _environ pointers to be identical\n" );
    }
    else
        win_skip( "_get_environ() is not available\n" );

    /* Note that msvcrt from Windows versions older than Vista
     * expects the mode pointer parameter to be valid.*/
    __getmainargs(&argc, &argv, &envp, 0, &mode);

    ok( envp != NULL, "Expected initial environment block pointer to be non-NULL\n" );
    if (!envp)
    {
        skip( "Initial environment block pointer is not valid\n" );
        return;
    }

    for (i = 0; ; i++)
    {
        if ((*p_environ)[i])
        {
            ok( envp[i] != NULL, "Expected environment block pointer element to be non-NULL\n" );
            ok( !strcmp((*p_environ)[i], envp[i]),
                "Expected _environ and environment block pointer strings (%s vs. %s) to match\n",
                (*p_environ)[i], envp[i] );
        }
        else
        {
            ok( !envp[i], "Expected environment block pointer element to be NULL, got %p\n", envp[i] );
            break;
        }
    }
}

static void test__wenviron(void)
{
    int argc;
    char **argv, **envp = NULL;
    WCHAR **wargv, **wenvp = NULL;
    int i, mode = 0;

    ok( p_wenviron != NULL, "Expected the pointer to _wenviron to be non-NULL\n" );
    if (p_wenviron)
        ok( *p_wenviron == NULL, "Expected _wenviron to be NULL, got %p\n", *p_wenviron );
    else
    {
        win_skip( "Pointer to _wenviron is not valid\n" );
        return;
    }

    if (sizeof(void*) != sizeof(int))
        ok( !p__p__wenviron, "__p__wenviron() should be 32-bit only\n");
    else
        ok( *p__p__wenviron() == NULL, "Expected _wenviron pointers to be NULL\n" );

    if (p_get_wenviron)
    {
        WCHAR **retptr;
        p_get_wenviron(&retptr);
        ok( retptr == NULL,
            "Expected _wenviron pointers to be NULL\n" );
    }
    else
        win_skip( "_get_wenviron() is not available\n" );

    /* __getmainargs doesn't initialize _wenviron. */
    __getmainargs(&argc, &argv, &envp, 0, &mode);

    ok( *p_wenviron == NULL, "Expected _wenviron to be NULL, got %p\n", *p_wenviron);
    ok( envp != NULL, "Expected initial environment block pointer to be non-NULL\n" );
    if (!envp)
    {
        skip( "Initial environment block pointer is not valid\n" );
        return;
    }

    /* Neither does calling the non-Unicode environment manipulation functions. */
    ok( _putenv("cat=dog") == 0, "failed setting cat=dog\n" );
    ok( *p_wenviron == NULL, "Expected _wenviron to be NULL, got %p\n", *p_wenviron);
    ok( _putenv("cat=") == 0, "failed deleting cat\n" );

    /* _wenviron isn't initialized until __wgetmainargs is called or
     * one of the Unicode environment manipulation functions is called. */
    ok( _wputenv(L"cat=dog") == 0, "failed setting cat=dog\n" );
    ok( *p_wenviron != NULL, "Expected _wenviron to be non-NULL\n" );
    ok( _wputenv(L"cat=") == 0, "failed deleting cat\n" );

    __wgetmainargs(&argc, &wargv, &wenvp, 0, &mode);

    ok( *p_wenviron != NULL, "Expected _wenviron to be non-NULL\n" );
    ok( wenvp != NULL, "Expected initial environment block pointer to be non-NULL\n" );
    if (!wenvp)
    {
        skip( "Initial environment block pointer is not valid\n" );
        return;
    }

    /* Examine the returned pointer from __p__wenviron(),
     * if available, after _wenviron is initialized. */
    if (p__p__wenviron)
    {
        ok( *p__p__wenviron() == *p_wenviron,
            "Expected _wenviron pointers to be identical\n" );
    }

    if (p_get_wenviron)
    {
        WCHAR **retptr;
        p_get_wenviron(&retptr);
        ok( retptr == *p_wenviron,
            "Expected _wenviron pointers to be identical\n" );
    }

    for (i = 0; ; i++)
    {
        if ((*p_wenviron)[i])
        {
            ok( wenvp[i] != NULL, "Expected environment block pointer element to be non-NULL\n" );
            ok( !wcscmp((*p_wenviron)[i], wenvp[i]),
                "Expected _wenviron and environment block pointer strings (%s vs. %s) to match\n",
                wine_dbgstr_w((*p_wenviron)[i]), wine_dbgstr_w(wenvp[i]) );
        }
        else
        {
            ok( !wenvp[i], "Expected environment block pointer element to be NULL, got %p\n", wenvp[i] );
            break;
        }
    }
}

static void test_environment_manipulation(void)
{
    char buf[256];
    errno_t ret;
    size_t len;

    ok( _putenv("cat=") == 0, "_putenv failed on deletion of nonexistent environment variable\n" );
    ok( _putenv("cat=dog") == 0, "failed setting cat=dog\n" );
    ok( strcmp(getenv("cat"), "dog") == 0, "getenv did not return 'dog'\n" );
    if (p_getenv_s)
    {
        ret = p_getenv_s(&len, buf, sizeof(buf), "cat");
        ok( !ret, "getenv_s returned %d\n", ret );
        ok( len == 4, "getenv_s returned length is %Id\n", len);
        ok( !strcmp(buf, "dog"), "getenv_s did not return 'dog'\n");
    }
    ok( _putenv("cat=") == 0, "failed deleting cat\n" );

    ok( _putenv("=") == -1, "should not accept '=' as input\n" );
    ok( _putenv("=dog") == -1, "should not accept '=dog' as input\n" );
    ok( _putenv(a_very_long_env_string) == 0, "_putenv failed for long environment string\n");

    ok( getenv("nonexistent") == NULL, "getenv should fail with nonexistent var name\n" );

    if (p_putenv_s)
    {
        ret = p_putenv_s(NULL, "dog");
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret);
        ret = p_putenv_s("cat", NULL);
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret);
        ret = p_putenv_s("a=b", NULL);
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret);
        ret = p_putenv_s("cat", "a=b");
        ok( !ret, "_putenv_s returned %d\n", ret);
        ret = p_putenv_s("cat", "");
        ok( !ret, "_putenv_s returned %d\n", ret);
    }

    if (p_wputenv_s)
    {
        ret = p_wputenv_s(NULL, L"dog");
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret);
        ret = p_wputenv_s(L"cat", NULL);
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret);
        ret = p_wputenv_s(L"a=b", NULL);
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret);
        ret = p_wputenv_s(L"cat", L"a=b");
        ok( !ret, "_wputenv_s returned %d\n", ret);
        ret = p_wputenv_s(L"cat", L"");
        ok( !ret, "_wputenv_s returned %d\n", ret);
    }

    if (p_getenv_s)
    {
        buf[0] = 'x';
        len = 1;
        errno = 0xdeadbeef;
        ret = p_getenv_s(&len, buf, sizeof(buf), "nonexistent");
        ok( !ret, "_getenv_s returned %d\n", ret);
        ok( !len, "getenv_s returned length is %Id\n", len);
        ok( !buf[0], "buf = %s\n", buf);
        ok( errno == 0xdeadbeef, "errno = %d\n", errno);

        buf[0] = 'x';
        len = 1;
        errno = 0xdeadbeef;
        ret = p_getenv_s(&len, buf, sizeof(buf), NULL);
        ok( !ret, "_getenv_s returned %d\n", ret);
        ok( !len, "getenv_s returned length is %Id\n", len);
        ok( !buf[0], "buf = %s\n", buf);
        ok( errno == 0xdeadbeef, "errno = %d\n", errno);
    }
}

START_TEST(environ)
{
    init();

    /* The environ tests should always be run first, as they assume
     * that the process has not manipulated the environment. */
    test__environ();
    test__wenviron();
    test_environment_manipulation();
    test_system();
}
