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
#include <stdio.h>

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
DEFINE_EXPECT(yield_func);

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

#ifdef __i386__

#include "pshpack1.h"
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#include "poppack.h"

static ULONG_PTR (WINAPI *call_thiscall_func1)( void *func, void *this );
static ULONG_PTR (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc( NULL, sizeof(*thunk),
            MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef unsigned char MSVCRT_bool;

typedef enum
{
    SPINWAIT_INIT,
    SPINWAIT_SPIN,
    SPINWAIT_YIELD,
    SPINWAIT_DONE
} SpinWait_state;

typedef void (__cdecl *yield_func)(void);

typedef struct
{
    ULONG spin;
    ULONG unknown;
    SpinWait_state state;
    yield_func yield_func;
} SpinWait;

static int* (__cdecl *p_errno)(void);
static int (__cdecl *p_wmemcpy_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);
static int (__cdecl *p_wmemmove_s)(wchar_t *dest, size_t numberOfElements, const wchar_t *src, size_t count);
static FILE* (__cdecl *p_fopen)(const char*,const char*);
static int (__cdecl *p_fclose)(FILE*);
static size_t (__cdecl *p_fread_s)(void*,size_t,size_t,size_t,FILE*);
static void* (__cdecl *p__aligned_offset_malloc)(size_t, size_t, size_t);
static void (__cdecl *p__aligned_free)(void*);
static size_t (__cdecl *p__aligned_msize)(void*, size_t, size_t);
static int (__cdecl *p_atoi)(const char*);

static SpinWait* (__thiscall *pSpinWait_ctor_yield)(SpinWait*, yield_func);
static void (__thiscall *pSpinWait_dtor)(SpinWait*);
static void (__thiscall *pSpinWait__DoYield)(SpinWait*);
static ULONG (__thiscall *pSpinWait__NumberOfSpins)(SpinWait*);
static void (__thiscall *pSpinWait__SetSpinCount)(SpinWait*, unsigned int);
static MSVCRT_bool (__thiscall *pSpinWait__ShouldSpinAgain)(SpinWait*);
static MSVCRT_bool (__thiscall *pSpinWait__SpinOnce)(SpinWait*);

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
    SET(p_fopen, "fopen");
    SET(p_fclose, "fclose");
    SET(p_fread_s, "fread_s");
    SET(p__aligned_offset_malloc, "_aligned_offset_malloc");
    SET(p__aligned_free, "_aligned_free");
    SET(p__aligned_msize, "_aligned_msize");
    SET(p_atoi, "atoi");

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(pSpinWait_ctor_yield, "??0?$_SpinWait@$00@details@Concurrency@@QEAA@P6AXXZ@Z");
        SET(pSpinWait_dtor, "??_F?$_SpinWait@$00@details@Concurrency@@QEAAXXZ");
        SET(pSpinWait__DoYield, "?_DoYield@?$_SpinWait@$00@details@Concurrency@@IEAAXXZ");
        SET(pSpinWait__NumberOfSpins, "?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IEAAKXZ");
        SET(pSpinWait__SetSpinCount, "?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QEAAXI@Z");
        SET(pSpinWait__ShouldSpinAgain, "?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IEAA_NXZ");
        SET(pSpinWait__SpinOnce, "?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QEAA_NXZ");
    } else {
        SET(pSpinWait_ctor_yield, "??0?$_SpinWait@$00@details@Concurrency@@QAE@P6AXXZ@Z");
        SET(pSpinWait_dtor, "??_F?$_SpinWait@$00@details@Concurrency@@QAEXXZ");
        SET(pSpinWait__DoYield, "?_DoYield@?$_SpinWait@$00@details@Concurrency@@IAEXXZ");
        SET(pSpinWait__NumberOfSpins, "?_NumberOfSpins@?$_SpinWait@$00@details@Concurrency@@IAEKXZ");
        SET(pSpinWait__SetSpinCount, "?_SetSpinCount@?$_SpinWait@$00@details@Concurrency@@QAEXI@Z");
        SET(pSpinWait__ShouldSpinAgain, "?_ShouldSpinAgain@?$_SpinWait@$00@details@Concurrency@@IAE_NXZ");
        SET(pSpinWait__SpinOnce, "?_SpinOnce@?$_SpinWait@$00@details@Concurrency@@QAE_NXZ");
    }

    init_thiscall_thunk();
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

static void test_fread_s(void)
{
    static const char test_file[] = "fread_s.tst";
    int ret;
    char buf[10];

    FILE *f = fopen(test_file, "w");
    if(!f) {
        skip("Error creating test file\n");
        return;
    }
    fwrite("test", 1, 4, f);
    fclose(f);

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(buf, sizeof(buf), 1, 1, NULL);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    f = p_fopen(test_file, "r");
    errno = 0xdeadbeef;
    ret = p_fread_s(NULL, sizeof(buf), 0, 1, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d, expected 0xdeadbeef\n", errno);
    ret = p_fread_s(NULL, sizeof(buf), 1, 0, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d, expected 0xdeadbeef\n", errno);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(NULL, sizeof(buf), 1, 1, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(errno == EINVAL, "errno = %d, expected EINVAL\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    buf[1] = 'a';
    ret = p_fread_s(buf, 3, 1, 10, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(buf[0] == 0, "buf[0] = '%c', expected 0\n", buf[0]);
    ok(buf[1] == 0, "buf[1] = '%c', expected 0\n", buf[1]);
    ok(errno == ERANGE, "errno = %d, expected ERANGE\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p_fread_s(buf, 2, 1, 10, f);
    ok(ret == 0, "fread_s returned %d, expected 0\n", ret);
    ok(buf[0] == 0, "buf[0] = '%c', expected 0\n", buf[0]);
    ok(errno == ERANGE, "errno = %d, expected ERANGE\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    memset(buf, 'a', sizeof(buf));
    ret = p_fread_s(buf, sizeof(buf), 3, 10, f);
    ok(ret==1, "fread_s returned %d, expected 1\n", ret);
    ok(buf[0] == 'e', "buf[0] = '%c', expected 'e'\n", buf[0]);
    ok(buf[1] == 's', "buf[1] = '%c', expected 's'\n", buf[1]);
    ok(buf[2] == 't', "buf[2] = '%c', expected 't'\n", buf[2]);
    ok(buf[3] == 'a', "buf[3] = '%c', expected 'a'\n", buf[3]);
    p_fclose(f);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
    unlink(test_file);
}

static void test__aligned_msize(void)
{
    void *mem;
    int ret;

    mem = p__aligned_offset_malloc(23, 16, 7);
    ret = p__aligned_msize(mem, 16, 7);
    ok(ret == 23, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 15, 7);
    ok(ret == 24, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 11, 7);
    ok(ret == 28, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 1, 7);
    ok(ret == 39-sizeof(void*), "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 8, 0);
    todo_wine ok(ret == 32, "_aligned_msize returned %d\n", ret);
    p__aligned_free(mem);

    mem = p__aligned_offset_malloc(3, 16, 0);
    ret = p__aligned_msize(mem, 16, 0);
    ok(ret == 3, "_aligned_msize returned %d\n", ret);
    ret = p__aligned_msize(mem, 11, 0);
    ok(ret == 8, "_aligned_msize returned %d\n", ret);
    p__aligned_free(mem);
}

static void test_atoi(void)
{
    int r;

    r = p_atoi("0");
    ok(r == 0, "atoi(0) = %d\n", r);

    r = p_atoi("-1");
    ok(r == -1, "atoi(-1) = %d\n", r);

    r = p_atoi("1");
    ok(r == 1, "atoi(1) = %d\n", r);

    r = p_atoi("4294967296");
    ok(r == 2147483647, "atoi(4294967296) = %d\n", r);
}

static void __cdecl test_yield_func(void)
{
    CHECK_EXPECT(yield_func);
}

static void test__SpinWait(void)
{
    SpinWait sp;
    ULONG ul;
    MSVCRT_bool b;

    call_func2(pSpinWait_ctor_yield, &sp, test_yield_func);

    SET_EXPECT(yield_func);
    call_func1(pSpinWait__DoYield, &sp);
    CHECK_CALLED(yield_func);

    ul = call_func1(pSpinWait__NumberOfSpins, &sp);
    ok(ul == 1, "_SpinWait::_NumberOfSpins returned %u\n", ul);

    sp.spin = 2;
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == 1, "sp.spin = %u\n", sp.spin);
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(!b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %u\n", sp.spin);
    b = call_func1(pSpinWait__ShouldSpinAgain, &sp);
    ok(b, "_SpinWait::_ShouldSpinAgain returned %x\n", b);
    ok(sp.spin == -1, "sp.spin = %u\n", sp.spin);

    call_func2(pSpinWait__SetSpinCount, &sp, 2);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 1, "sp.spin = %u\n", sp.spin);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %u\n", sp.spin);
    SET_EXPECT(yield_func);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin == 0, "sp.spin = %u\n", sp.spin);
    CHECK_CALLED(yield_func);
    b = call_func1(pSpinWait__SpinOnce, &sp);
    ok(!b, "_SpinWait::_SpinOnce returned %x\n", b);
    ok(sp.spin==0 || sp.spin==4000, "sp.spin = %u\n", sp.spin);

    call_func1(pSpinWait_dtor, &sp);
}

START_TEST(msvcr100)
{
    if (!init())
        return;

    test_wmemcpy_s();
    test_wmemmove_s();
    test_fread_s();
    test__aligned_msize();
    test_atoi();
    test__SpinWait();
}
