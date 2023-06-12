/*
 * Copyright 2018 Daniel Lehman
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
#include <stdio.h>
#include <float.h>
#include <limits.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"

#include <locale.h>

typedef void (*vtable_ptr)(void);

typedef struct {
    const vtable_ptr *vtable;
} Context;

typedef struct {
    Context *ctx;
} _Context;

static char* (CDECL *p_setlocale)(int category, const char* locale);
static size_t (CDECL *p___strncnt)(const char *str, size_t count);

static unsigned int (CDECL *p_CurrentScheduler_GetNumberOfVirtualProcessors)(void);
static unsigned int (CDECL *p__CurrentScheduler__GetNumberOfVirtualProcessors)(void);
static unsigned int (CDECL *p_CurrentScheduler_Id)(void);
static unsigned int (CDECL *p__CurrentScheduler__Id)(void);

static Context* (__cdecl *p_Context_CurrentContext)(void);
static _Context* (__cdecl *p__Context__CurrentContext)(_Context*);

static int (__cdecl *p_strcmp)(const char *, const char *);
static int (__cdecl *p_strncmp)(const char *, const char *, size_t);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(module,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("msvcr110.dll");
    if (!module)
    {
        win_skip("msvcr110.dll not installed\n");
        return FALSE;
    }

    SET(p_setlocale, "setlocale");
    SET(p___strncnt, "__strncnt");
    SET(p_CurrentScheduler_GetNumberOfVirtualProcessors, "?GetNumberOfVirtualProcessors@CurrentScheduler@Concurrency@@SAIXZ");
    SET(p__CurrentScheduler__GetNumberOfVirtualProcessors, "?_GetNumberOfVirtualProcessors@_CurrentScheduler@details@Concurrency@@SAIXZ");
    SET(p_CurrentScheduler_Id, "?Id@CurrentScheduler@Concurrency@@SAIXZ");
    SET(p__CurrentScheduler__Id, "?_Id@_CurrentScheduler@details@Concurrency@@SAIXZ");

    SET(p__Context__CurrentContext, "?_CurrentContext@_Context@details@Concurrency@@SA?AV123@XZ");

    if(sizeof(void*) == 8)
    {
        SET(p_Context_CurrentContext, "?CurrentContext@Context@Concurrency@@SAPEAV12@XZ");
    }
    else
    {
        SET(p_Context_CurrentContext, "?CurrentContext@Context@Concurrency@@SAPAV12@XZ");
    }

    SET(p_strcmp, "strcmp");
    SET(p_strncmp, "strncmp");

    return TRUE;
}

static void test_CurrentScheduler(void)
{
    unsigned int id;
    unsigned int ncpus;
    unsigned int expect;
    SYSTEM_INFO si;

    expect = ~0;
    ncpus = p_CurrentScheduler_GetNumberOfVirtualProcessors();
    ok(ncpus == expect, "expected %x, got %x\n", expect, ncpus);
    id = p_CurrentScheduler_Id();
    ok(id == expect, "expected %u, got %u\n", expect, id);

    GetSystemInfo(&si);
    expect = si.dwNumberOfProcessors;
    /* these _CurrentScheduler calls trigger scheduler creation
       if either is commented out, the following CurrentScheduler (no _) tests will still work */
    ncpus = p__CurrentScheduler__GetNumberOfVirtualProcessors();
    id = p__CurrentScheduler__Id();
    ok(ncpus == expect, "expected %u, got %u\n", expect, ncpus);
    ok(id == 0, "expected 0, got %u\n", id);

    /* these CurrentScheduler tests assume scheduler is created */
    ncpus = p_CurrentScheduler_GetNumberOfVirtualProcessors();
    ok(ncpus == expect, "expected %u, got %u\n", expect, ncpus);
    id = p_CurrentScheduler_Id();
    ok(id == 0, "expected 0, got %u\n", id);
}

static void test_setlocale(void)
{
    int i;
    char *ret;
    static const char *names[] =
    {
        "en-us",
        "en-US",
        "EN-US",
        "syr-SY",
        "uz-Latn-uz",
    };

    for(i=0; i<ARRAY_SIZE(names); i++) {
        ret = p_setlocale(LC_ALL, names[i]);
        ok(ret != NULL, "expected success, but got NULL\n");
        if(!strcmp(names[i], "syr-SY") && GetACP() == CP_UTF8)
        {
            ok(!strcmp(ret, "LC_COLLATE=syr-SY;LC_CTYPE=EN-US;LC_MONETARY=syr-SY;"
                        "LC_NUMERIC=syr-SY;LC_TIME=syr-SY"), "got %s\n", ret);
        }
        else
        {
            ok(!strcmp(ret, names[i]), "expected %s, got %s\n", names[i], ret);
        }
    }

    ret = p_setlocale(LC_ALL, "en-us.1250");
    ok(!ret, "setlocale(en-us.1250) succeeded (%s)\n", ret);

    ret = p_setlocale(LC_ALL, "zh-Hans");
    ok((ret != NULL
        || broken(ret == NULL)), /* Vista */
        "expected success, but got NULL\n");
    if (ret)
        ok(!strcmp(ret, "zh-Hans"), "setlocale zh-Hans failed\n");

    ret = p_setlocale(LC_ALL, "zh-Hant");
    ok((ret != NULL
        || broken(ret == NULL)), /* Vista */
        "expected success, but got NULL\n");
    if (ret)
        ok(!strcmp(ret, "zh-Hant"), "setlocale zh-Hant failed\n");

    /* used to return Chinese (Simplified)_China.936 */
    ret = p_setlocale(LC_ALL, "chinese");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Chinese_China.936")
            || broken(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")) /* Vista */
            || broken(!strcmp(ret, "Chinese_People's Republic of China.936"))), /* 7 */
            "setlocale chinese failed, got %s\n", ret);

    /* used to return Chinese (Simplified)_China.936 */
    ret = p_setlocale(LC_ALL, "Chinese_China.936");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Chinese_China.936")
            || broken(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")) /* Vista */
            || broken(!strcmp(ret, "Chinese_People's Republic of China.936"))), /* 7 */
            "setlocale Chinese_China.936 failed, got %s\n", ret);

    /* used to return Chinese (Simplified)_China.936 */
    ret = p_setlocale(LC_ALL, "chinese-simplified");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Chinese_China.936")
            || broken(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936"))), /* Vista */
            "setlocale chinese-simplified failed, got %s\n", ret);

    /* used to return Chinese (Simplified)_China.936 */
    ret = p_setlocale(LC_ALL, "chs");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Chinese_China.936")
            || broken(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936"))), /* Vista */
            "setlocale chs failed, got %s\n", ret);

    /* used to return Chinese (Traditional)_Taiwan.950 */
    ret = p_setlocale(LC_ALL, "cht");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        todo_wine ok((!strcmp(ret, "Chinese (Traditional)_Hong Kong SAR.950")
            || broken(!strcmp(ret, "Chinese (Traditional)_Taiwan.950"))), /* Vista - 7 */
            "setlocale cht failed, got %s\n", ret);

    /* used to return Chinese (Traditional)_Taiwan.950 */
    ret = p_setlocale(LC_ALL, "chinese-traditional");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        todo_wine ok((!strcmp(ret, "Chinese (Traditional)_Hong Kong SAR.950")
            || broken(!strcmp(ret, "Chinese (Traditional)_Taiwan.950"))), /* Vista - 7 */
            "setlocale chinese-traditional failed, got %s\n", ret);

    ret = p_setlocale(LC_ALL, "norwegian-nynorsk");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Norwegian Nynorsk_Norway.1252")
            || broken(!strcmp(ret, "Norwegian (Nynorsk)_Norway.1252"))), /* Vista - 7 */
             "setlocale norwegian-nynorsk failed, got %s\n", ret);

    ret = p_setlocale(LC_ALL, "non");
    ok(ret != NULL, "expected success, but got NULL\n");
    if (ret)
        ok((!strcmp(ret, "Norwegian Nynorsk_Norway.1252")
            || broken(!strcmp(ret, "Norwegian (Nynorsk)_Norway.1252"))), /* Vista - 7 */
             "setlocale norwegian-nynorsk failed, got %s\n", ret);

    p_setlocale(LC_ALL, "C");
}

static void test___strncnt(void)
{
    static const struct
    {
        const char *str;
        size_t size;
        size_t ret;
    }
    strncnt_tests[] =
    {
        { NULL, 0, 0 },
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    if (0) /* crashes */
        ret = p___strncnt(NULL, 1);

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = p___strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }
}

static void test_CurrentContext(void)
{
    _Context _ctx, *ret;
    Context *ctx;

    ctx = p_Context_CurrentContext();
    ok(!!ctx, "got NULL\n");

    memset(&_ctx, 0xcc, sizeof(_ctx));
    ret = p__Context__CurrentContext(&_ctx);
    ok(_ctx.ctx == ctx, "expected %p, got %p\n", ctx, _ctx.ctx);
    ok(ret == &_ctx, "expected %p, got %p\n", &_ctx, ret);
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
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

START_TEST(msvcr110)
{
    if (!init()) return;
    test_CurrentScheduler(); /* MUST be first (at least among Concurrency tests) */
    test_setlocale();
    test___strncnt();
    test_CurrentContext();
    test_strcmp();
}
