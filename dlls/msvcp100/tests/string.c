/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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
#include <limits.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

typedef struct
{
    char *str;
    char null_str;
} _Yarn_char;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

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

static _Yarn_char* (__thiscall *_Yarn_char_ctor_cstr)(_Yarn_char *this, const char *str);
static _Yarn_char* (__thiscall *_Yarn_char_copy_ctor)(_Yarn_char *this, const _Yarn_char *copy);
static void (__thiscall *_Yarn_char_dtor)(_Yarn_char *this);
static const char* (__thiscall *_Yarn_char_c_str)(const _Yarn_char *this);
static char (__thiscall *_Yarn_char_empty)(const _Yarn_char *this);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp100.dll");
    if(!msvcp) {
        win_skip("msvcp100.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(_Yarn_char_ctor_cstr, "??0?$_Yarn@D@std@@QEAA@PEBD@Z");
        SET(_Yarn_char_copy_ctor, "??0?$_Yarn@D@std@@QEAA@AEBV01@@Z");
        SET(_Yarn_char_dtor, "??1?$_Yarn@D@std@@QEAA@XZ");
        SET(_Yarn_char_c_str, "?_C_str@?$_Yarn@D@std@@QEBAPEBDXZ");
        SET(_Yarn_char_empty, "?_Empty@?$_Yarn@D@std@@QEBA_NXZ");
    }else {
#ifdef __arm__
        SET(_Yarn_char_ctor_cstr, "??0?$_Yarn@D@std@@QAA@PBD@Z");
        SET(_Yarn_char_copy_ctor, "??0?$_Yarn@D@std@@QAA@ABV01@@Z");
        SET(_Yarn_char_dtor, "??1?$_Yarn@D@std@@QAA@XZ");
        SET(_Yarn_char_c_str, "?_C_str@?$_Yarn@D@std@@QBAPBDXZ");
        SET(_Yarn_char_empty, "?_Empty@?$_Yarn@D@std@@QBA_NXZ");
#else
        SET(_Yarn_char_ctor_cstr, "??0?$_Yarn@D@std@@QAE@PBD@Z");
        SET(_Yarn_char_copy_ctor, "??0?$_Yarn@D@std@@QAE@ABV01@@Z");
        SET(_Yarn_char_dtor, "??1?$_Yarn@D@std@@QAE@XZ");
        SET(_Yarn_char_c_str, "?_C_str@?$_Yarn@D@std@@QBEPBDXZ");
        SET(_Yarn_char_empty, "?_Empty@?$_Yarn@D@std@@QBE_NXZ");
#endif /* __arm__ */
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test__Yarn_char(void)
{
    _Yarn_char c1, c2;
    char bool_ret;
    const char *str;

    call_func2(_Yarn_char_ctor_cstr, &c1, NULL);
    ok(!c1.str, "c1.str = %s\n", c1.str);
    ok(!c1.null_str, "c1.null_str = %d\n", c1.null_str);
    bool_ret = (char)(INT_PTR)call_func1(_Yarn_char_empty, &c1);
    ok(bool_ret == 1, "ret = %d\n", bool_ret);
    str = call_func1(_Yarn_char_c_str, &c1);
    ok(str == &c1.null_str, "str = %p, expected %p\n", str, &c1.null_str);
    call_func1(_Yarn_char_dtor, &c1);

    call_func2(_Yarn_char_ctor_cstr, &c1, "");
    ok(c1.str != NULL, "c1.str = NULL\n");
    ok(!strcmp(c1.str, ""), "c1.str = %s\n", c1.str);
    ok(!c1.null_str, "c1.null_str = %d\n", c1.null_str);
    bool_ret = (char)(INT_PTR)call_func1(_Yarn_char_empty, &c1);
    ok(bool_ret == 0, "ret = %d\n", bool_ret);
    str = call_func1(_Yarn_char_c_str, &c1);
    ok(str == c1.str, "str = %p, expected %p\n", str, c1.str);
    call_func1(_Yarn_char_dtor, &c1);

    call_func2(_Yarn_char_ctor_cstr, &c1, "test");
    ok(!strcmp(c1.str, "test"), "c1.str = %s\n", c1.str);
    ok(!c1.null_str, "c1.null_str = %d\n", c1.null_str);
    bool_ret = (char)(INT_PTR)call_func1(_Yarn_char_empty, &c1);
    ok(bool_ret == 0, "ret = %d\n", bool_ret);
    str = call_func1(_Yarn_char_c_str, &c1);
    ok(str == c1.str, "str = %p, expected %p\n", str, c1.str);

    call_func2(_Yarn_char_copy_ctor, &c2, &c1);
    ok(c1.str != c2.str, "c1.str = c2.str\n");
    ok(!strcmp(c2.str, "test"), "c2.str = %s\n", c2.str);
    ok(!c2.null_str, "c2.null_str = %d\n", c2.null_str);
    call_func1(_Yarn_char_dtor, &c2);

    c1.null_str = 'a';
    call_func2(_Yarn_char_copy_ctor, &c2, &c1);
    ok(c1.str != c2.str, "c1.str = c2.str\n");
    ok(!strcmp(c2.str, "test"), "c2.str = %s\n", c2.str);
    ok(!c2.null_str, "c2.null_str = %d\n", c2.null_str);
    call_func1(_Yarn_char_dtor, &c2);
    call_func1(_Yarn_char_dtor, &c1);
}

START_TEST(string)
{
    if(!init())
        return;

    test__Yarn_char();

    FreeLibrary(msvcp);
}
