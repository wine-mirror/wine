/*
 * Unit test suite for ntdll path functions
 *
 * Copyright 2002 Alexandre Julliard
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

#include "wine/test.h"
#include "winnt.h"
#include "winternl.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static UINT (WINAPI *pRtlDetermineDosPathNameType_U)( PCWSTR path );
static ULONG (WINAPI *pRtlIsDosDeviceName_U)( PCWSTR dos_name );

static void test_RtlDetermineDosPathNameType(void)
{
    struct test
    {
        const char *path;
        int ret;
    };

    static const struct test tests[] =
    {
        { "\\\\foo", 1 },
        { "//foo", 1 },
        { "\\/foo", 1 },
        { "/\\foo", 1 },
        { "\\\\", 1 },
        { "//", 1 },
        { "c:\\foo", 2 },
        { "c:/foo", 2 },
        { "c://foo", 2 },
        { "c:\\", 2 },
        { "c:/", 2 },
        { "c:foo", 3 },
        { "c:f\\oo", 3 },
        { "c:foo/bar", 3 },
        { "\\foo", 4 },
        { "/foo", 4 },
        { "\\", 4 },
        { "/", 4 },
        { "foo", 5 },
        { "", 5 },
        { "\0:foo", 5 },
        { "\\\\.\\foo", 6 },
        { "//./foo", 6 },
        { "/\\./foo", 6 },
        { "\\\\.foo", 1 },
        { "//.foo", 1 },
        { "\\\\.", 7 },
        { "//.", 7 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[MAX_PATH];
    UINT ret;

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlDetermineDosPathNameType_U( buffer );
        ok( ret == test->ret, "Wrong result %d/%d for %s", ret, test->ret, test->path );
    }
}


static void test_RtlIsDosDeviceName(void)
{
    struct test
    {
        const char *path;
        WORD pos;
        WORD len;
    };

    static const struct test tests[] =
    {
        { "\\\\.\\CON",    8, 6 },
        { "\\\\.\\con",    8, 6 },
        { "\\\\.\\CON2",   0, 0 },
        { "",              0, 0 },
        { "\\\\foo\\nul",  0, 0 },
        { "c:\\nul:",      6, 6 },
        { "c:\\nul::",     0, 0 },
        { "c:prn     ",    4, 6 },
        { "c:prn.......",  4, 6 },
        { "c:prn... ...",  4, 6 },
        { "c:NUL  ....  ", 0, 0 },
        { "c: . . .",      0, 0 },
        { "c:",            0, 0 },
        { " . . . :",      0, 0 },
        { ":",             0, 0 },
        { "c:nul. . . :",  4, 6 },
        { "c:nul . . :",   0, 0 },
        { "c:nul0",        0, 0 },
        { "c:prn:aaa",     0, 0 },
        { "c:PRN:.txt",    4, 6 },
        { "c:aux:.txt...", 4, 6 },
        { "c:prn:.txt:",   4, 6 },
        { "c:nul:aaa",     0, 0 },
        { "con:",          0, 6 },
        { "lpt1:",         0, 8 },
        { "c:com5:",       4, 8 },
        { "CoM4:",         0, 8 },
        { "lpt9:",         0, 8 },
        { "c:\\lpt0.txt",  0, 0 },
        { "c:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\nul.txt", 1000, 6 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[2000];
    ULONG ret;

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlIsDosDeviceName_U( buffer );
        ok( ret == MAKELONG( test->len, test->pos ),
            "Wrong result (%d,%d)/(%d,%d) for %s",
            HIWORD(ret), LOWORD(ret), test->pos, test->len, test->path );
    }
}



START_TEST(path)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlDetermineDosPathNameType_U = (void *)GetProcAddress(mod,"RtlDetermineDosPathNameType_U");
    pRtlIsDosDeviceName_U = (void *)GetProcAddress(mod,"RtlIsDosDeviceName_U");
    test_RtlDetermineDosPathNameType();
    test_RtlIsDosDeviceName();
}
