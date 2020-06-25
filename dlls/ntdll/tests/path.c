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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ntdll_test.h"
#include "winnls.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlUnicodeToMultiByteN)(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
static UINT (WINAPI *pRtlDetermineDosPathNameType_U)( PCWSTR path );
static ULONG (WINAPI *pRtlIsDosDeviceName_U)( PCWSTR dos_name );
static NTSTATUS (WINAPI *pRtlOemStringToUnicodeString)(UNICODE_STRING *, const STRING *, BOOLEAN );
static BOOLEAN (WINAPI *pRtlIsNameLegalDOS8Dot3)(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
static DWORD (WINAPI *pRtlGetFullPathName_U)(const WCHAR*,ULONG,WCHAR*,WCHAR**);
static BOOLEAN (WINAPI *pRtlDosPathNameToNtPathName_U)(const WCHAR*, UNICODE_STRING*, WCHAR**, CURDIR*);
static NTSTATUS (WINAPI *pRtlDosPathNameToNtPathName_U_WithStatus)(const WCHAR*, UNICODE_STRING*, WCHAR**, CURDIR*);

static void test_RtlDetermineDosPathNameType_U(void)
{
    struct test
    {
        const char *path;
        UINT ret;
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
        { "\\\\?\\foo", 6 },
        { "//?/foo", 6 },
        { "/\\?/foo", 6 },
        { "\\\\?foo", 1 },
        { "//?foo", 1 },
        { "\\\\?", 7 },
        { "//?", 7 },
        { "CONIN$", 5 },
        { "CONOUT$", 5 },
        { "CONERR$", 5 },
        { "\\\\.\\CONIN$", 6 },
        { "\\\\.\\CONOUT$", 6 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[MAX_PATH];
    UINT ret;

    if (!pRtlDetermineDosPathNameType_U)
    {
        win_skip("RtlDetermineDosPathNameType_U is not available\n");
        return;
    }

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlDetermineDosPathNameType_U( buffer );
        ok( ret == test->ret, "Wrong result %d/%d for %s\n", ret, test->ret, test->path );
    }
}


static void test_RtlIsDosDeviceName_U(void)
{
    struct test
    {
        const char *path;
        WORD pos;
        WORD len;
        BOOL fails;
    };

    static const struct test tests[] =
    {
        { "\\\\.\\CON",    8, 6, TRUE },  /* fails on win8 */
        { "\\\\.\\con",    8, 6, TRUE },  /* fails on win8 */
        { "\\\\.\\CON2",   0, 0 },
        { "",              0, 0 },
        { "\\\\foo\\nul",  0, 0 },
        { "c:\\nul:",      6, 6 },
        { "c:\\nul\\",     0, 0 },
        { "c:\\nul\\foo",  0, 0 },
        { "c:\\nul::",     6, 6, TRUE },  /* fails on nt4 */
        { "c:\\nul::::::", 6, 6, TRUE },  /* fails on nt4 */
        { "c:prn     ",    4, 6 },
        { "c:prn.......",  4, 6 },
        { "c:prn... ...",  4, 6 },
        { "c:NUL  ....  ", 4, 6, TRUE },  /* fails on nt4 */
        { "c: . . .",      0, 0 },
        { "c:",            0, 0 },
        { " . . . :",      0, 0 },
        { ":",             0, 0 },
        { "c:nul. . . :",  4, 6 },
        { "c:nul . . :",   4, 6, TRUE },  /* fails on nt4 */
        { "c:nul0",        0, 0 },
        { "c:prn:aaa",     4, 6, TRUE },  /* fails on win9x */
        { "c:PRN:.txt",    4, 6 },
        { "c:aux:.txt...", 4, 6 },
        { "c:prn:.txt:",   4, 6 },
        { "c:nul:aaa",     4, 6, TRUE },  /* fails on win9x */
        { "con:",          0, 6 },
        { "lpt1:",         0, 8 },
        { "c:com5:",       4, 8 },
        { "CoM4:",         0, 8 },
        { "lpt9:",         0, 8 },
        { "c:\\lpt0.txt",  0, 0 },
        { "CONIN$",        0, 12, TRUE }, /* fails on winxp */
        { "CONOUT$",       0, 14, TRUE }, /* fails on winxp */
        { "CONERR$",       0, 0 },
        { "CON",           0, 6 },
        { "PIPE",          0, 0 },
        { "\\??\\CONIN$",  8, 12, TRUE }, /* fails on winxp */
        { "\\??\\CONOUT$", 8, 14, TRUE }, /* fails on winxp */
        { "\\??\\CONERR$", 0, 0 },
        { "\\??\\CON",     8, 6 },
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

    if (!pRtlIsDosDeviceName_U)
    {
        win_skip("RtlIsDosDeviceName_U is not available\n");
        return;
    }

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlIsDosDeviceName_U( buffer );
        ok( ret == MAKELONG( test->len, test->pos ) ||
            (test->fails && broken( ret == 0 )),
            "Wrong result (%d,%d)/(%d,%d) for %s\n",
            HIWORD(ret), LOWORD(ret), test->pos, test->len, test->path );
    }
}

static void test_RtlIsNameLegalDOS8Dot3(void)
{
    struct test
    {
        const char *path;
        BOOLEAN result;
        BOOLEAN spaces;
    };

    static const struct test tests[] =
    {
        { "12345678",     TRUE,  FALSE },
        { "123 5678",     TRUE,  TRUE  },
        { "12345678.",    FALSE, 2 /*not set*/ },
        { "1234 678.",    FALSE, 2 /*not set*/ },
        { "12345678.a",   TRUE,  FALSE },
        { "12345678.a ",  FALSE, 2 /*not set*/ },
        { "12345678.a c", TRUE,  TRUE  },
        { " 2345678.a ",  FALSE, 2 /*not set*/ },
        { "1 345678.abc", TRUE,  TRUE },
        { "1      8.a c", TRUE,  TRUE },
        { "1 3 5 7 .abc", FALSE, 2 /*not set*/ },
        { "12345678.  c", TRUE,  TRUE },
        { "123456789.a",  FALSE, 2 /*not set*/ },
        { "12345.abcd",   FALSE, 2 /*not set*/ },
        { "12345.ab d",   FALSE, 2 /*not set*/ },
        { ".abc",         FALSE, 2 /*not set*/ },
        { "12.abc.d",     FALSE, 2 /*not set*/ },
        { ".",            TRUE,  FALSE },
        { "..",           TRUE,  FALSE },
        { "...",          FALSE, 2 /*not set*/ },
        { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", FALSE, 2 /*not set*/ },
        { NULL, 0 }
    };

    const struct test *test;
    UNICODE_STRING ustr;
    OEM_STRING oem, oem_ret;
    WCHAR buffer[200];
    char buff2[12];
    BOOLEAN ret, spaces;

    if (!pRtlIsNameLegalDOS8Dot3)
    {
        win_skip("RtlIsNameLegalDOS8Dot3 is not available\n");
        return;
    }

    ustr.MaximumLength = sizeof(buffer);
    ustr.Buffer = buffer;
    for (test = tests; test->path; test++)
    {
        char path[100];
        strcpy(path, test->path);
        oem.Buffer = path;
        oem.Length = strlen(test->path);
        oem.MaximumLength = oem.Length + 1;
        pRtlOemStringToUnicodeString( &ustr, &oem, FALSE );
        spaces = 2;
        oem_ret.Length = oem_ret.MaximumLength = sizeof(buff2);
        oem_ret.Buffer = buff2;
        ret = pRtlIsNameLegalDOS8Dot3( &ustr, &oem_ret, &spaces );
        ok( ret == test->result, "Wrong result %d/%d for '%s'\n", ret, test->result, test->path );
        ok( spaces == test->spaces, "Wrong spaces value %d/%d for '%s'\n", spaces, test->spaces, test->path );
        if (strlen(test->path) <= 12)
        {
            STRING test_str;
            char str[13];
            strcpy( str, test->path );
            RtlInitString( &test_str, str );
            RtlUpperString( &test_str, &test_str );
            ok( !RtlCompareString(&oem_ret, &test_str, FALSE),
                "Wrong string '%.*s'/'%s'\n", oem_ret.Length, oem_ret.Buffer, test->path );
        }
    }
}
static void test_RtlGetFullPathName_U(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'d','e','a','d','b','e','e','f',0};

    struct test
    {
        const char *path;
        const char *rname;
        const char *rfile;
        const char *alt_rname;
        const char *alt_rfile;
    };

    static const struct test tests[] =
        {
            { "c:/test",                     "c:\\test",         "test"},
            { "c:/test/",                    "c:\\test\\",       NULL},
            { "c:/test     ",                "c:\\test",         "test"},
            { "c:/test.",                    "c:\\test",         "test"},
            { "c:/test  ....   ..   ",       "c:\\test",         "test"},
            { "c:/test/  ....   ..   ",      "c:\\test\\",       NULL},
            { "c:/test/..",                  "c:\\",             NULL},
            { "c:/test/.. ",                 "c:\\test\\",       NULL},
            { "c:/TEST",                     "c:\\TEST",         "TEST"},
            { "c:/test/file",                "c:\\test\\file",   "file"},
            { "c:/test./file",               "c:\\test\\file",   "file"},
            { "c:/test.. /file",             "c:\\test.. \\file","file"},
            { "c:/test/././file",            "c:\\test\\file",   "file"},
            { "c:/test\\.\\.\\file",         "c:\\test\\file",   "file"},
            { "c:/test/\\.\\.\\file",        "c:\\test\\file",   "file"},
            { "c:/test\\\\.\\.\\file",       "c:\\test\\file",   "file"},
            { "c:/test\\test1\\..\\.\\file", "c:\\test\\file",   "file"},
            { "c:///test\\.\\.\\file//",     "c:\\test\\file\\", NULL,
                                             "c:\\test\\file",   "file"},  /* nt4 */
            { "c:///test\\..\\file\\..\\//", "c:\\",             NULL},
            { "c:/test../file",              "c:\\test.\\file",  "file",
                                             "c:\\test..\\file", "file"},  /* vista */
            { "c:\\test",                    "c:\\test",         "test"},
            { "C:\\test",                    "C:\\test",         "test"},
            { "c:/",                         "c:\\",             NULL},
            { "c:.",                         "C:\\windows",      "windows"},
            { "c:foo",                       "C:\\windows\\foo", "foo"},
            { "c:foo/bar",                   "C:\\windows\\foo\\bar", "bar"},
            { "c:./foo",                     "C:\\windows\\foo", "foo"},
            { "\\foo",                       "C:\\foo",          "foo"},
            { "foo",                         "C:\\windows\\foo", "foo"},
            { ".",                           "C:\\windows",      "windows"},
            { "..",                          "C:\\",             NULL},
            { "...",                         "C:\\windows\\",    NULL},
            { "./foo",                       "C:\\windows\\foo", "foo"},
            { "foo/..",                      "C:\\windows",      "windows"},
            { "AUX",                         "\\\\.\\AUX",       NULL},
            { "COM1",                        "\\\\.\\COM1",      NULL},
            { "?<>*\"|:",                    "C:\\windows\\?<>*\"|:", "?<>*\"|:"},

            { "\\\\foo",                     "\\\\foo",          NULL},
            { "//foo",                       "\\\\foo",          NULL},
            { "\\/foo",                      "\\\\foo",          NULL},
            { "//",                          "\\\\",             NULL},
            { "//foo/",                      "\\\\foo\\",        NULL},

            { "//.",                         "\\\\.\\",          NULL},
            { "//./",                        "\\\\.\\",          NULL},
            { "//.//",                       "\\\\.\\",          NULL},
            { "//./foo",                     "\\\\.\\foo",       "foo"},
            { "//./foo/",                    "\\\\.\\foo\\",     NULL},
            { "//./foo/bar",                 "\\\\.\\foo\\bar",  "bar"},
            { "//./foo/.",                   "\\\\.\\foo",       "foo"},
            { "//./foo/..",                  "\\\\.\\",          NULL},

            { "//?/",                        "\\\\?\\",          NULL},
            { "//?//",                       "\\\\?\\",          NULL},
            { "//?/foo",                     "\\\\?\\foo",       "foo"},
            { "//?/foo/",                    "\\\\?\\foo\\",     NULL},
            { "//?/foo/bar",                 "\\\\?\\foo\\bar",  "bar"},
            { "//?/foo/.",                   "\\\\?\\foo",       "foo"},
            { "//?/foo/..",                  "\\\\?\\",          NULL},

            { "CONIN$",                      "\\\\.\\CONIN$",    NULL,
                                             "C:\\windows\\CONIN$", "CONIN$"},
            { "CONOUT$",                     "\\\\.\\CONOUT$",   NULL,
                                             "C:\\windows\\CONOUT$", "CONOUT$"},

            /* RtlGetFullPathName_U() can't understand the global namespace prefix */
            { "\\??\\foo",                   "C:\\??\\foo",      "foo"},
            { 0 }
        };

    const struct test *test;
    WCHAR pathbufW[2*MAX_PATH], rbufferW[MAX_PATH];
    char rbufferA[MAX_PATH], rfileA[MAX_PATH], curdir[MAX_PATH];
    ULONG ret;
    WCHAR *file_part;
    DWORD reslen;
    UINT len;

    GetCurrentDirectoryA(sizeof(curdir), curdir);
    SetCurrentDirectoryA("C:\\windows\\");

    file_part = (WCHAR *)0xdeadbeef;
    lstrcpyW(rbufferW, deadbeefW);
    ret = pRtlGetFullPathName_U(NULL, MAX_PATH, rbufferW, &file_part);
    ok(!ret, "Expected RtlGetFullPathName_U to return 0, got %u\n", ret);
    ok(!lstrcmpW(rbufferW, deadbeefW),
       "Expected the output buffer to be untouched, got %s\n", wine_dbgstr_w(rbufferW));
    ok(file_part == (WCHAR *)0xdeadbeef ||
       file_part == NULL, /* Win7 */
       "Expected file part pointer to be untouched, got %p\n", file_part);

    file_part = (WCHAR *)0xdeadbeef;
    lstrcpyW(rbufferW, deadbeefW);
    ret = pRtlGetFullPathName_U(emptyW, MAX_PATH, rbufferW, &file_part);
    ok(!ret, "Expected RtlGetFullPathName_U to return 0, got %u\n", ret);
    ok(!lstrcmpW(rbufferW, deadbeefW),
       "Expected the output buffer to be untouched, got %s\n", wine_dbgstr_w(rbufferW));
    ok(file_part == (WCHAR *)0xdeadbeef ||
       file_part == NULL, /* Win7 */
       "Expected file part pointer to be untouched, got %p\n", file_part);

    for (test = tests; test->path; test++)
    {
        len= strlen(test->rname) * sizeof(WCHAR);
        pRtlMultiByteToUnicodeN(pathbufW , sizeof(pathbufW), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlGetFullPathName_U( pathbufW,MAX_PATH, rbufferW, &file_part);
        ok( ret == len || (test->alt_rname && ret == strlen(test->alt_rname)*sizeof(WCHAR)),
            "Wrong result %d/%d for \"%s\"\n", ret, len, test->path );
        ok(pRtlUnicodeToMultiByteN(rbufferA,MAX_PATH,&reslen,rbufferW,(lstrlenW(rbufferW) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
           "RtlUnicodeToMultiByteN failed\n");
        ok(!lstrcmpA(rbufferA,test->rname) || (test->alt_rname && !lstrcmpA(rbufferA,test->alt_rname)),
           "Got \"%s\" expected \"%s\"\n",rbufferA,test->rname);
        if (file_part)
        {
            ok(pRtlUnicodeToMultiByteN(rfileA,MAX_PATH,&reslen,file_part,(lstrlenW(file_part) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
               "RtlUnicodeToMultiByteN failed\n");
            ok((test->rfile && !lstrcmpA(rfileA,test->rfile)) ||
               (test->alt_rfile && !lstrcmpA(rfileA,test->alt_rfile)),
               "Got \"%s\" expected \"%s\"\n",rfileA,test->rfile);
        }
        else
        {
            ok( !test->rfile, "Got NULL expected \"%s\"\n", test->rfile );
        }
    }

    SetCurrentDirectoryA(curdir);
}

static void test_RtlDosPathNameToNtPathName_U(void)
{
    static const WCHAR broken_global_prefix[] = L"\\??\\C:\\??";

    char curdir[MAX_PATH];
    UNICODE_STRING nameW;
    WCHAR *file_part;
    NTSTATUS status;
    BOOL ret;
    int i;

    static const struct
    {
        const WCHAR *dos;
        const WCHAR *nt;
        int file_offset;    /* offset to file part */
        const WCHAR *alt_nt;
    }
    tests[] =
    {
        {L"c:\\",           L"\\??\\c:\\",                  -1},
        {L"c:/",            L"\\??\\c:\\",                  -1},
        {L"c:/foo",         L"\\??\\c:\\foo",                7},
        {L"c:/foo.",        L"\\??\\c:\\foo",                7},
        {L"c:/foo/",        L"\\??\\c:\\foo\\",             -1},
        {L"c:/foo//",       L"\\??\\c:\\foo\\",             -1},
        {L"C:/foo",         L"\\??\\C:\\foo",                7},
        {L"C:/foo/bar",     L"\\??\\C:\\foo\\bar",          11},
        {L"C:/foo/bar",     L"\\??\\C:\\foo\\bar",          11},
        {L"c:.",            L"\\??\\C:\\windows",            7},
        {L"c:foo",          L"\\??\\C:\\windows\\foo",      15},
        {L"c:foo/bar",      L"\\??\\C:\\windows\\foo\\bar", 19},
        {L"c:./foo",        L"\\??\\C:\\windows\\foo",      15},
        {L"c:/./foo",       L"\\??\\c:\\foo",                7},
        {L"c:/..",          L"\\??\\c:\\",                  -1},
        {L"c:/foo/.",       L"\\??\\c:\\foo",                7},
        {L"c:/foo/./bar",   L"\\??\\c:\\foo\\bar",          11},
        {L"c:/foo/../bar",  L"\\??\\c:\\bar",                7},
        {L"\\foo",          L"\\??\\C:\\foo",                7},
        {L"foo",            L"\\??\\C:\\windows\\foo",      15},
        {L".",              L"\\??\\C:\\windows",            7},
        {L"./",             L"\\??\\C:\\windows\\",         -1},
        {L"..",             L"\\??\\C:\\",                  -1},
        {L"...",            L"\\??\\C:\\windows\\",         -1},
        {L"./foo",          L"\\??\\C:\\windows\\foo",      15},
        {L"foo/..",         L"\\??\\C:\\windows",            7},
        {L"AUX" ,           L"\\??\\AUX",                   -1},
        {L"COM1" ,          L"\\??\\COM1",                  -1},
        {L"?<>*\"|:",       L"\\??\\C:\\windows\\?<>*\"|:", 15},
        {L"?:",             L"\\??\\?:\\",                  -1},

        {L"\\\\foo",        L"\\??\\UNC\\foo",              -1},
        {L"//foo",          L"\\??\\UNC\\foo",              -1},
        {L"\\/foo",         L"\\??\\UNC\\foo",              -1},
        {L"//",             L"\\??\\UNC\\",                 -1},
        {L"//foo/",         L"\\??\\UNC\\foo\\",            -1},

        {L"//.",            L"\\??\\",                      -1},
        {L"//./",           L"\\??\\",                      -1},
        {L"//.//",          L"\\??\\",                      -1},
        {L"//./foo",        L"\\??\\foo",                    4},
        {L"//./foo/",       L"\\??\\foo\\",                 -1},
        {L"//./foo/bar",    L"\\??\\foo\\bar",               8},
        {L"//./foo/.",      L"\\??\\foo",                    4},
        {L"//./foo/..",     L"\\??\\",                      -1},

        {L"//?",            L"\\??\\",                      -1},
        {L"//?/",           L"\\??\\",                      -1},
        {L"//?//",          L"\\??\\",                      -1},
        {L"//?/foo",        L"\\??\\foo",                    4},
        {L"//?/foo/",       L"\\??\\foo\\",                 -1},
        {L"//?/foo/bar",    L"\\??\\foo\\bar",               8},
        {L"//?/foo/.",      L"\\??\\foo",                    4},
        {L"//?/foo/..",     L"\\??\\",                      -1},

        {L"\\\\.",          L"\\??\\",                      -1},
        {L"\\\\.\\",        L"\\??\\",                      -1},
        {L"\\\\.\\/",       L"\\??\\",                      -1},
        {L"\\\\.\\foo",     L"\\??\\foo",                    4},
        {L"\\\\.\\foo/",    L"\\??\\foo\\",                 -1},
        {L"\\\\.\\foo/bar", L"\\??\\foo\\bar",               8},
        {L"\\\\.\\foo/.",   L"\\??\\foo",                    4},
        {L"\\\\.\\foo/..",  L"\\??\\",                      -1},

        {L"\\\\?",          L"\\??\\",                      -1},
        {L"\\\\?\\",        L"\\??\\",                      -1},

        {L"\\\\?\\/",       L"\\??\\/",                      4},
        {L"\\\\?\\foo",     L"\\??\\foo",                    4},
        {L"\\\\?\\foo/",    L"\\??\\foo/",                   4},
        {L"\\\\?\\foo/bar", L"\\??\\foo/bar",                4},
        {L"\\\\?\\foo/.",   L"\\??\\foo/.",                  4},
        {L"\\\\?\\foo/..",  L"\\??\\foo/..",                 4},
        {L"\\\\?\\\\",      L"\\??\\\\",                    -1},
        {L"\\\\?\\\\\\",    L"\\??\\\\\\",                  -1},
        {L"\\\\?\\foo\\",   L"\\??\\foo\\",                 -1},
        {L"\\\\?\\foo\\bar",L"\\??\\foo\\bar",               8},
        {L"\\\\?\\foo\\.",  L"\\??\\foo\\.",                 8},
        {L"\\\\?\\foo\\..", L"\\??\\foo\\..",                8},

        {L"\\??",           L"\\??\\C:\\??",                 7},
        {L"\\??\\",         L"\\??\\C:\\??\\",              -1},

        {L"\\??\\/",        L"\\??\\/",                      4},
        {L"\\??\\foo",      L"\\??\\foo",                    4},
        {L"\\??\\foo/",     L"\\??\\foo/",                   4},
        {L"\\??\\foo/bar",  L"\\??\\foo/bar",                4},
        {L"\\??\\foo/.",    L"\\??\\foo/.",                  4},
        {L"\\??\\foo/..",   L"\\??\\foo/..",                 4},
        {L"\\??\\\\",       L"\\??\\\\",                    -1},
        {L"\\??\\\\\\",     L"\\??\\\\\\",                  -1},
        {L"\\??\\foo\\",    L"\\??\\foo\\",                 -1},
        {L"\\??\\foo\\bar", L"\\??\\foo\\bar",               8},
        {L"\\??\\foo\\.",   L"\\??\\foo\\.",                 8},
        {L"\\??\\foo\\..",  L"\\??\\foo\\..",                8},

        {L"CONIN$",         L"\\??\\CONIN$",                -1, L"\\??\\C:\\windows\\CONIN$"  /* winxp */ },
        {L"CONOUT$",        L"\\??\\CONOUT$",               -1, L"\\??\\C:\\windows\\CONOUT$" /* winxp */ },
        {L"cOnOuT$",        L"\\??\\cOnOuT$",               -1, L"\\??\\C:\\windows\\cOnOuT$" /* winxp */ },
        {L"CONERR$",        L"\\??\\C:\\windows\\CONERR$",  15},
    };

    GetCurrentDirectoryA(sizeof(curdir), curdir);
    SetCurrentDirectoryA("C:\\windows\\");

    ret = pRtlDosPathNameToNtPathName_U(NULL, &nameW, &file_part, NULL);
    ok(!ret, "Got %d.\n", ret);

    ret = pRtlDosPathNameToNtPathName_U(L"", &nameW, &file_part, NULL);
    ok(!ret, "Got %d.\n", ret);

    ret = pRtlDosPathNameToNtPathName_U(L" ", &nameW, &file_part, NULL);
    ok(!ret, "Got %d.\n", ret);

    if (pRtlDosPathNameToNtPathName_U_WithStatus)
    {
        status = pRtlDosPathNameToNtPathName_U_WithStatus(NULL, &nameW, &file_part, NULL);
        ok(status == STATUS_OBJECT_NAME_INVALID || status == STATUS_OBJECT_PATH_NOT_FOUND /* 2003 */,
                "Got status %#x.\n", status);

        status = pRtlDosPathNameToNtPathName_U_WithStatus(L"", &nameW, &file_part, NULL);
        ok(status == STATUS_OBJECT_NAME_INVALID || status == STATUS_OBJECT_PATH_NOT_FOUND /* 2003 */,
                "Got status %#x.\n", status);

        status = pRtlDosPathNameToNtPathName_U_WithStatus(L" ", &nameW, &file_part, NULL);
        ok(status == STATUS_OBJECT_NAME_INVALID || status == STATUS_OBJECT_PATH_NOT_FOUND /* 2003 */,
                "Got status %#x.\n", status);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        ret = pRtlDosPathNameToNtPathName_U(tests[i].dos, &nameW, &file_part, NULL);
        ok(ret == TRUE, "%s: Got %d.\n", debugstr_w(tests[i].dos), ret);

        if (pRtlDosPathNameToNtPathName_U_WithStatus)
        {
            RtlFreeUnicodeString(&nameW);
            status = pRtlDosPathNameToNtPathName_U_WithStatus(tests[i].dos, &nameW, &file_part, NULL);
            ok(status == STATUS_SUCCESS, "%s: Got status %#x.\n", debugstr_w(tests[i].dos), status);
        }

        if (!wcsncmp(tests[i].dos, L"\\??\\", 4) && tests[i].dos[4] &&
            broken(!wcsncmp(nameW.Buffer, broken_global_prefix, wcslen(broken_global_prefix))))
        {
            /* Windows version prior to 2003 don't interpret the \??\ prefix */
            continue;
        }

        ok(!wcscmp(nameW.Buffer, tests[i].nt) || broken(!wcscmp(nameW.Buffer, tests[i].alt_nt)), "%s: Expected %s, got %s.\n",
            debugstr_w(tests[i].dos), debugstr_w(tests[i].nt), debugstr_w(nameW.Buffer));

        if (!wcscmp(nameW.Buffer, tests[i].nt))
        {
            if (tests[i].file_offset > 0)
                ok(file_part == nameW.Buffer + tests[i].file_offset,
                   "%s: Expected file part %s, got %s.\n", debugstr_w(tests[i].dos),
                   debugstr_w(nameW.Buffer + tests[i].file_offset), debugstr_w(file_part));
            else
                ok(file_part == NULL, "%s: Expected NULL file part, got %s.\n",
                   debugstr_w(tests[i].dos), debugstr_w(file_part));
        }

        RtlFreeUnicodeString(&nameW);
    }

    SetCurrentDirectoryA(curdir);
}

START_TEST(path)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    if (!mod)
    {
        win_skip("Not running on NT, skipping tests\n");
        return;
    }

    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlUnicodeToMultiByteN = (void *)GetProcAddress(mod,"RtlUnicodeToMultiByteN");
    pRtlDetermineDosPathNameType_U = (void *)GetProcAddress(mod,"RtlDetermineDosPathNameType_U");
    pRtlIsDosDeviceName_U = (void *)GetProcAddress(mod,"RtlIsDosDeviceName_U");
    pRtlOemStringToUnicodeString = (void *)GetProcAddress(mod,"RtlOemStringToUnicodeString");
    pRtlIsNameLegalDOS8Dot3 = (void *)GetProcAddress(mod,"RtlIsNameLegalDOS8Dot3");
    pRtlGetFullPathName_U = (void *)GetProcAddress(mod,"RtlGetFullPathName_U");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(mod, "RtlDosPathNameToNtPathName_U");
    pRtlDosPathNameToNtPathName_U_WithStatus = (void *)GetProcAddress(mod, "RtlDosPathNameToNtPathName_U_WithStatus");

    test_RtlDetermineDosPathNameType_U();
    test_RtlIsDosDeviceName_U();
    test_RtlIsNameLegalDOS8Dot3();
    test_RtlGetFullPathName_U();
    test_RtlDosPathNameToNtPathName_U();
}
