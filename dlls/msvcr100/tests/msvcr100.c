/*
 * Copyright 2012 Dan Kegel
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

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

static _invalid_parameter_handler (__cdecl *p_set_invalid_parameter_handler)(_invalid_parameter_handler);

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
}

static int* (__cdecl *p_errno)(void);
static int (__cdecl *p_wmemcpy_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);
static int (__cdecl *p_wmemmove_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE hcrt;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr100.dll");
    if (!hcrt) {
        win_skip("msvcr100.dll not installed (got %d)\n", GetLastError());
        return FALSE;
    }

    SET(p_errno, "_errno");
    SET(p_set_invalid_parameter_handler, "_set_invalid_parameter_handler");
    SET(p_wmemcpy_s, "wmemcpy_s");
    SET(p_wmemmove_s, "wmemmove_s");

    return TRUE;
}

#define NUMELMS(array) (sizeof(array)/sizeof((array)[0]))

#define okwchars(dst, b0, b1, b2, b3, b4, b5, b6, b7) \
    ok(dst[0] == b0 && dst[1] == b1 && dst[2] == b2 && dst[3] == b3 && \
       dst[4] == b4 && dst[5] == b5 && dst[6] == b6 && dst[7] == b7, \
       "Bad result: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",\
       dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7])

static void test_wmemcpy_s(void)
{
    static wchar_t dest[8];
    static const wchar_t tiny[] = {'T',0,'I','N','Y',0};
    static const wchar_t big[] = {'a','t','o','o','l','o','n','g','s','t','r','i','n','g',0};
    const wchar_t XX = 0x5858;     /* two 'X' bytes */
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, NUMELMS(dest), tiny, NUMELMS(tiny));
    ok(ret == 0, "Copying a buffer into a big enough destination returned %d, expected 0\n", ret);
    okwchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], XX, XX);

    /* Vary source size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, NUMELMS(dest), big, NUMELMS(big));
    ok(errno == ERANGE, "Copying a big buffer to a small destination errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Copying a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    okwchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, NUMELMS(dest), NULL, NUMELMS(tiny));
    ok(errno == EINVAL, "Copying a NULL source buffer errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a NULL source buffer returned %d, expected EINVAL\n", ret);
    okwchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK_CALLED(invalid_parameter_handler);

    /* Vary dest size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, 0, tiny, NUMELMS(tiny));
    ok(errno == ERANGE, "Copying into a destination of size 0 errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Copying into a destination of size 0 returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wmemcpy_s(NULL, NUMELMS(dest), tiny, NUMELMS(tiny));
    ok(errno == EINVAL, "Copying a tiny buffer to a big NULL destination errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* Combinations */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemcpy_s(dest, 0, NULL, NUMELMS(tiny));
    ok(errno == EINVAL, "Copying a NULL buffer into a destination of size 0 errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Copying a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test_wmemmove_s(void)
{
    static wchar_t dest[8];
    static const wchar_t tiny[] = {'T',0,'I','N','Y',0};
    static const wchar_t big[] = {'a','t','o','o','l','o','n','g','s','t','r','i','n','g',0};
    const wchar_t XX = 0x5858;     /* two 'X' bytes */
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, NUMELMS(dest), tiny, NUMELMS(tiny));
    ok(ret == 0, "Moving a buffer into a big enough destination returned %d, expected 0\n", ret);
    okwchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], XX, XX);

    /* Overlapping */
    memcpy(dest, big, sizeof(dest));
    ret = p_wmemmove_s(dest+1, NUMELMS(dest)-1, dest, NUMELMS(dest)-1);
    ok(ret == 0, "Moving a buffer up one char returned %d, expected 0\n", ret);
    okwchars(dest, big[0], big[0], big[1], big[2], big[3], big[4], big[5], big[6]);

    /* Vary source size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, NUMELMS(dest), big, NUMELMS(big));
    ok(errno == ERANGE, "Moving a big buffer to a small destination errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Moving a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, NUMELMS(dest), NULL, NUMELMS(tiny));
    ok(errno == EINVAL, "Moving a NULL source buffer errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a NULL source buffer returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Vary dest size */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, 0, tiny, NUMELMS(tiny));
    ok(errno == ERANGE, "Moving into a destination of size 0 errno %d, expected ERANGE\n", errno);
    ok(ret == ERANGE, "Moving into a destination of size 0 returned %d, expected ERANGE\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wmemmove_s(NULL, NUMELMS(dest), tiny, NUMELMS(tiny));
    ok(errno == EINVAL, "Moving a tiny buffer to a big NULL destination errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* Combinations */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    memset(dest, 'X', sizeof(dest));
    ret = p_wmemmove_s(dest, 0, NULL, NUMELMS(tiny));
    ok(errno == EINVAL, "Moving a NULL buffer into a destination of size 0 errno %d, expected EINVAL\n", errno);
    ok(ret == EINVAL, "Moving a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    okwchars(dest, XX, XX, XX, XX, XX, XX, XX, XX);
    CHECK_CALLED(invalid_parameter_handler);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

START_TEST(msvcr100)
{
    if (!init())
        return;

    test_wmemcpy_s();
    test_wmemmove_s();
}
