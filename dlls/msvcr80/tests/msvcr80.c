/*
 * Copyright 2022 Paul Gofman for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#include <windef.h>
#include <winbase.h>
#include <errno.h>
#include "wine/test.h"

#define WX_OPEN           0x01
#define WX_ATEOF          0x02
#define WX_READNL         0x04
#define WX_PIPE           0x08
#define WX_DONTINHERIT    0x10
#define WX_APPEND         0x20
#define WX_TTY            0x40
#define WX_TEXT           0x80

#define _MB_CP_SBCS 0

#define MSVCRT_FD_BLOCK_SIZE 32

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(invalid_parameter_handler);

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %Ix\n", arg);
}

typedef struct
{
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
    CRITICAL_SECTION    crit;
    char textmode : 7;
    char unicode : 1;
    char pipech2[2];
    __int64 startpos;
    BOOL utf8translations;
    char dbcsBuffer;
    BOOL dbcsBufferUsed;
} ioinfo;

static ioinfo **__pioinfo;

static _invalid_parameter_handler (__cdecl *p__set_invalid_parameter_handler)(_invalid_parameter_handler);
static int (WINAPIV *p__open)(const char *, int, ...);
static int (__cdecl *p__close)(int);
static intptr_t (__cdecl *p__get_osfhandle)(int);
static int (__cdecl *p_strcmp)(const char *, const char *);
static int (__cdecl *p_strncmp)(const char *, const char *, size_t);
static int (__cdecl *p_dupenv_s)(char **, size_t *, const char *);
static int (__cdecl *p_wdupenv_s)(wchar_t **, size_t *, const wchar_t *);
static int* (__cdecl *p_errno)(void);
static errno_t (__cdecl *p__mbsncpy_s)(unsigned char*,size_t,const unsigned char*,size_t);
static int (__cdecl *p__ismbblead_l)(unsigned int,_locale_t);
static int (__cdecl *p__getmbcp)(void);
static int (__cdecl *p__setmbcp)(int);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HMODULE hcrt;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr80.dll");
    if (!hcrt) {
        win_skip("msvcr80.dll not installed (got %ld)\n", GetLastError());
        return FALSE;
    }

    SET(p__set_invalid_parameter_handler, "_set_invalid_parameter_handler");

    SET(__pioinfo, "__pioinfo");
    SET(p__open,"_open");
    SET(p__close,"_close");
    SET(p__get_osfhandle, "_get_osfhandle");

    SET(p_strcmp, "strcmp");
    SET(p_strncmp, "strncmp");
    SET(p_dupenv_s, "_dupenv_s");
    SET(p_wdupenv_s, "_wdupenv_s");
    SET(p_errno, "_errno");
    SET(p__mbsncpy_s, "_mbsncpy_s");
    SET(p__ismbblead_l, "_ismbblead_l");
    SET(p__getmbcp, "_getmbcp");
    SET(p__setmbcp, "_setmbcp");
    return TRUE;
}

static void test_ioinfo_flags(void)
{
    HANDLE handle;
    ioinfo *info;
    char *tempf;
    int tempfd;

    tempf = _tempnam(".","wne");

    tempfd = p__open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_WTEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    handle = (HANDLE)p__get_osfhandle(tempfd);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->handle == handle, "Unexpected handle %p, expected %p.\n", info->handle, handle);
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(info->unicode, "Unicode is not set.\n");
    ok(info->textmode == 2, "Unexpected textmode %d.\n", info->textmode);
    p__close(tempfd);

    ok(info->handle == INVALID_HANDLE_VALUE, "Unexpected handle %p.\n", info->handle);
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->unicode, "Unicode is not set.\n");
    ok(!info->wxflag, "Unexpected wxflag %#x.\n", info->wxflag);
    ok(info->textmode == 2, "Unexpected textmode %d.\n", info->textmode);

    info = &__pioinfo[(tempfd + 4) / MSVCRT_FD_BLOCK_SIZE][(tempfd + 4) % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->handle == INVALID_HANDLE_VALUE, "Unexpected handle %p.\n", info->handle);
    ok(!info->exflag, "Unexpected exflag %#x.\n", info->exflag);
    ok(!info->textmode, "Unexpected textmode %d.\n", info->textmode);

    tempfd = p__open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_TEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(!info->unicode, "Unicode is not set.\n");
    ok(!info->textmode, "Unexpected textmode %d.\n", info->textmode);
    p__close(tempfd);

    tempfd = p__open(tempf, _O_CREAT|_O_TRUNC|_O_WRONLY|_O_U8TEXT, _S_IWRITE);
    ok(tempfd != -1, "_open failed with error: %d\n", errno);
    info = &__pioinfo[tempfd / MSVCRT_FD_BLOCK_SIZE][tempfd % MSVCRT_FD_BLOCK_SIZE];
    ok(!!info, "NULL info.\n");
    ok(info->exflag == 1, "Unexpected exflag %#x.\n", info->exflag);
    ok(info->wxflag == (WX_TEXT | WX_OPEN), "Unexpected wxflag %#x.\n", info->wxflag);
    ok(!info->unicode, "Unicode is not set.\n");
    ok(info->textmode == 1, "Unexpected textmode %d.\n", info->textmode);
    p__close(tempfd);

    unlink(tempf);
    free(tempf);
}

static void test_strcmp(void)
{
    int ret = p_strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = p_strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
#ifdef _WIN64
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
#else
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == 0 - 'a', "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == 'c' - 0xa0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 0xb0 - 0xa0, "wrong ret %d\n", ret );
#endif
    ret = p_strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

static void test_dupenv_s(void)
{
    size_t len;
    char *tmp;
    int ret;

    len = 0xdeadbeef;
    tmp = (void *)0xdeadbeef;
    ret = p_dupenv_s( &tmp, &len, "nonexistent" );
    ok( !ret, "_dupenv_s returned %d\n", ret );
    ok( !len, "_dupenv_s returned length is %Id\n", len );
    ok( !tmp, "_dupenv_s returned pointer is %p\n", tmp );
}

static void test_wdupenv_s(void)
{
    wchar_t *tmp;
    size_t len;
    int ret;

    len = 0xdeadbeef;
    tmp = (void *)0xdeadbeef;
    ret = p_wdupenv_s( &tmp, &len, L"nonexistent" );
    ok( !ret, "_wdupenv_s returned %d\n", ret );
    ok( !len, "_wdupenv_s returned length is %Id\n", len );
    ok( !tmp, "_wdupenv_s returned pointer is %p\n", tmp );
}

#define expect_bin(buf, value, len) { ok(memcmp((buf), value, len) == 0, \
                                         "Binary buffer mismatch - expected %s, got %s\n", \
                                         debugstr_an(value, len), debugstr_an((char *)(buf), len)); }

static void test__mbsncpy_s(void)
{
    unsigned char *mbstring = (unsigned char *)"\xb0\xb1\xb2\xb3Q\xb4\xb5\x0";
    unsigned char *mbstring2 = (unsigned char *)"\xb0\x0";
    unsigned char buf[16];
    errno_t err;
    int oldcp;

    oldcp = p__getmbcp();
    if (p__setmbcp(936))
    {
        win_skip("Code page 936 is not available, skipping test.\n");
        return;
    }

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = p__mbsncpy_s(NULL, 0, mbstring, 0);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = p__mbsncpy_s(buf, 6, mbstring, 1);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\0\xcc", 4);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 6, mbstring, 2);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\xb2\xb3\0\xcc", 6);

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = p__mbsncpy_s(buf, 2, mbstring, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(err == STRUNCATE, "got %d.\n", err);
    expect_bin(buf, "\x00\xb1\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 2, mbstring, 1);
    ok(errno == err, "got %d.\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 2, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 1, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 0, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == EINVAL, "got %d.\n", err);
    expect_bin(buf, "\xcc", 1);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 0, mbstring, 0);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == EINVAL, "got %d.\n", err);
    expect_bin(buf, "\xcc", 1);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, -1, mbstring, 0);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, -1, mbstring, 256);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\xb2\xb3Q\xb4\xb5\x0\xcc", 9);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 1, mbstring2, 4);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 2, mbstring2, 4);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 1, mbstring2, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(err == STRUNCATE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 2, mbstring2, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\x0\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 1, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 2, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 3, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = p__mbsncpy_s(buf, 3, mbstring2, 2);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    p__setmbcp(oldcp);
}

START_TEST(msvcr80)
{
    if(!init())
        return;

    ok(p__set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    test_ioinfo_flags();
    test_strcmp();
    test_dupenv_s();
    test_wdupenv_s();
    test__mbsncpy_s();
}
