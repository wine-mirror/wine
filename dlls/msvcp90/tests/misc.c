/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

static void* (__cdecl *p_set_invalid_parameter_handler)(void*);

static void (__cdecl *p_char_assign)(void*, const void*);
static void (__cdecl *p_wchar_assign)(void*, const void*);
static void (__cdecl *p_short_assign)(void*, const void*);

static BYTE (__cdecl *p_char_eq)(const void*, const void*);
static BYTE (__cdecl *p_wchar_eq)(const void*, const void*);
static BYTE (__cdecl *p_short_eq)(const void*, const void*);

static char* (__cdecl *p_Copy_s)(char*, size_t, const char*, size_t);

#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

static char* (__thiscall *p_char_address)(void*, char*);
static void* (__thiscall *p_char_ctor)(void*);
static void (__thiscall *p_char_deallocate)(void*, char*, size_t);
static char* (__thiscall *p_char_allocate)(void*, size_t);
static void (__thiscall *p_char_construct)(void*, char*, const char*);
static size_t (__thiscall *p_char_max_size)(void*);

static int invalid_parameter = 0;
static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
    invalid_parameter++;
}

/* Emulate a __thiscall */
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

static void * (WINAPI *call_thiscall_func1)( void *func, void *this );
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, const void *a, const void *b );

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
    call_thiscall_func3 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)a)
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)a,(const void*)b)

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)

#endif /* __i386__ */

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HMODULE msvcr = LoadLibraryA("msvcr90.dll");
    HMODULE msvcp = LoadLibraryA("msvcp90.dll");
    if(!msvcr || !msvcp) {
        win_skip("msvcp90.dll or msvcrt90.dll not installed\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler = (void*)GetProcAddress(msvcr, "_set_invalid_parameter_handler");
    if(!p_set_invalid_parameter_handler) {
        win_skip("Error setting tests environment\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler(test_invalid_parameter_handler);

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_char_assign, "?assign@?$char_traits@D@std@@SAXAEADAEBD@Z");
        SET(p_wchar_assign, "?assign@?$char_traits@_W@std@@SAXAEA_WAEB_W@Z");
        SET(p_short_assign, "?assign@?$char_traits@G@std@@SAXAEAGAEBG@Z");

        SET(p_char_eq, "?eq@?$char_traits@D@std@@SA_NAEBD0@Z");
        SET(p_wchar_eq, "?eq@?$char_traits@_W@std@@SA_NAEB_W0@Z");
        SET(p_short_eq, "?eq@?$char_traits@G@std@@SA_NAEBG0@Z");

        SET(p_Copy_s, "?_Copy_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z");

        SET(p_char_address, "?address@?$allocator@D@std@@QEBAPEADAEAD@Z");
        SET(p_char_ctor, "??0?$allocator@D@std@@QEAA@XZ");
        SET(p_char_deallocate, "?deallocate@?$allocator@D@std@@QEAAXPEAD_K@Z");
        SET(p_char_allocate, "?allocate@?$allocator@D@std@@QEAAPEAD_K@Z");
        SET(p_char_construct, "?construct@?$allocator@D@std@@QEAAXPEADAEBD@Z");
        SET(p_char_max_size, "?max_size@?$allocator@D@std@@QEBA_KXZ");
    } else {
        SET(p_char_assign, "?assign@?$char_traits@D@std@@SAXAADABD@Z");
        SET(p_wchar_assign, "?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z");
        SET(p_short_assign, "?assign@?$char_traits@G@std@@SAXAAGABG@Z");

        SET(p_char_eq, "?eq@?$char_traits@D@std@@SA_NABD0@Z");
        SET(p_wchar_eq, "?eq@?$char_traits@_W@std@@SA_NAB_W0@Z");
        SET(p_short_eq, "?eq@?$char_traits@G@std@@SA_NABG0@Z");

        SET(p_Copy_s, "?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z");

        SET(p_char_address, "?address@?$allocator@D@std@@QBEPADAAD@Z");
        SET(p_char_ctor, "??0?$allocator@D@std@@QAE@XZ");
        SET(p_char_deallocate, "?deallocate@?$allocator@D@std@@QAEXPADI@Z");
        SET(p_char_allocate, "?allocate@?$allocator@D@std@@QAEPADI@Z");
        SET(p_char_construct, "?construct@?$allocator@D@std@@QAEXPADABD@Z");
        SET(p_char_max_size, "?max_size@?$allocator@D@std@@QBEIXZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test_assign(void)
{
    const char in[] = "abc";
    char out[4];

    out[1] = '#';
    p_char_assign(out, in);
    ok(out[0] == in[0], "out[0] = %c\n", out[0]);
    ok(out[1] == '#', "out[1] = %c\n", out[1]);

    out[2] = '#';
    p_wchar_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);

    out[2] = '#';
    p_short_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);
}

static void test_equal(void)
{
    static const char in1[] = "abc";
    static const char in2[] = "ab";
    static const char in3[] = "a";
    static const char in4[] = "b";
    BYTE ret;

    ret = p_char_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in3);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_wchar_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_short_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
}

static void test_Copy_s(void)
{
    static const char src[] = "abcd";
    char dest[32], *ret;

    dest[4] = '#';
    dest[5] = '\0';
    ret = p_Copy_s(dest, 4, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    ret = p_Copy_s(dest, 32, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    errno = 0xdeadbeef;
    dest[0] = '#';
    ret = p_Copy_s(dest, 3, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[0] == '\0', "dest[0] != 0\n");
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_Copy_s(NULL, 32, src, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_Copy_s(dest, 32, NULL, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);
}

static void test_allocator_char(void)
{
    void *allocator = (void*)0xdeadbeef;
    char *ptr;
    char val;
    unsigned int size;

    allocator = call_func1(p_char_ctor, allocator);
    ok(allocator == (void*)0xdeadbeef, "allocator = %p\n", allocator);

    ptr = call_func2(p_char_address, NULL, (void*)0xdeadbeef);
    ok(ptr == (void*)0xdeadbeef, "incorrect address (%p)\n", ptr);

    ptr = NULL;
    ptr = call_func2(p_char_allocate, allocator, 10);
    ok(ptr != NULL, "Memory allocation failed\n");

    ptr[0] = ptr[1] = '#';
    val = 'a';
    call_func3(p_char_construct, allocator, ptr, &val);
    val = 'b';
    call_func3(p_char_construct, allocator, (ptr+1), &val);
    ok(ptr[0] == 'a', "ptr[0] = %c\n", ptr[0]);
    ok(ptr[1] == 'b', "ptr[1] = %c\n", ptr[1]);

    call_func3(p_char_deallocate, allocator, ptr, -1);

    size = (unsigned int)call_func1(p_char_max_size, allocator);
    ok(size == (unsigned int)0xffffffff, "size = %x\n", size);
}

START_TEST(misc)
{
    if(!init())
        return;

    test_assign();
    test_equal();
    test_Copy_s();

    test_allocator_char();

    ok(!invalid_parameter, "invalid_parameter_handler was invoked too many times\n");
}
