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
#include <limits.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

typedef SIZE_T MSVCP_size_t;

/* basic_string<char, char_traits<char>, allocator<char>> */
typedef struct
{
    void *allocator;
    char *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_char;

/* basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> */
typedef struct
{
    void *allocator;
    wchar_t *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

typedef void (*vtable_ptr)(void);
typedef struct __exception
{
    const vtable_ptr *vtable;
    char *name;
    int do_free;
} exception;

typedef struct {
    exception e;
    basic_string_char str;
} runtime_error;

typedef struct {
    exception e;
    basic_string_char str;
} range_error;

typedef struct {
    exception e;
    basic_string_char str;
} logic_error;

static basic_string_char* (__cdecl *p_basic_string_char_concatenate)(basic_string_char*, const basic_string_char*, const basic_string_char*);
static basic_string_char* (__cdecl *p_basic_string_char_concatenate_cstr)(basic_string_char*, const basic_string_char*, const char*);

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

static basic_string_char* (__thiscall *p_basic_string_char_ctor)(basic_string_char*);
static basic_string_char* (__thiscall *p_basic_string_char_copy_ctor)(basic_string_char*, basic_string_char*);
static basic_string_char* (__thiscall *p_basic_string_char_ctor_cstr_alloc)(basic_string_char*, const char*, void*);
static void (__thiscall *p_basic_string_char_dtor)(basic_string_char*);
static basic_string_char* (__thiscall *p_basic_string_char_erase)(basic_string_char*, size_t, size_t);
static basic_string_char* (__thiscall *p_basic_string_char_assign_cstr_len)(basic_string_char*, const char*, size_t);
static const char* (__thiscall *p_basic_string_char_cstr)(basic_string_char*);
static const char* (__thiscall *p_basic_string_char_data)(basic_string_char*);
static size_t (__thiscall *p_basic_string_char_size)(basic_string_char*);
static size_t (__thiscall *p_basic_string_char_capacity)(basic_string_char*);
static void (__thiscall *p_basic_string_char_swap)(basic_string_char*, basic_string_char*);
static basic_string_char* (__thiscall *p_basic_string_char_append)(basic_string_char*, basic_string_char*);
static basic_string_char* (__thiscall *p_basic_string_char_append_substr)(basic_string_char*, basic_string_char*, size_t, size_t);
static int (__thiscall *p_basic_string_char_compare_substr_substr)(basic_string_char*, size_t, size_t, basic_string_char*, size_t, size_t);
static int (__thiscall *p_basic_string_char_compare_substr_cstr_len)(basic_string_char*, size_t, size_t, const char*, size_t);
static size_t (__thiscall *p_basic_string_char_find_cstr_substr)(basic_string_char*, const char*, size_t, size_t);
static size_t (__thiscall *p_basic_string_char_rfind_cstr_substr)(basic_string_char*, const char*, size_t, size_t);
static basic_string_char* (__thiscall *p_basic_string_char_replace_cstr)(basic_string_char*, size_t, size_t, const char*);

static size_t *p_basic_string_char_npos;

static basic_string_wchar* (__thiscall *p_basic_string_wchar_ctor)(basic_string_wchar*);
static basic_string_wchar* (__thiscall *p_basic_string_wchar_copy_ctor)(basic_string_wchar*, basic_string_wchar*);
static basic_string_wchar* (__thiscall *p_basic_string_wchar_ctor_cstr_alloc)(basic_string_wchar*, const wchar_t*, void*);
static void (__thiscall *p_basic_string_wchar_dtor)(basic_string_wchar*);
static basic_string_wchar* (__thiscall *p_basic_string_wchar_erase)(basic_string_wchar*, size_t, size_t);
static basic_string_wchar* (__thiscall *p_basic_string_wchar_assign_cstr_len)(basic_string_wchar*, const wchar_t*, size_t);
static const wchar_t* (__thiscall *p_basic_string_wchar_cstr)(basic_string_wchar*);
static const wchar_t* (__thiscall *p_basic_string_wchar_data)(basic_string_wchar*);
static size_t (__thiscall *p_basic_string_wchar_size)(basic_string_wchar*);
static size_t (__thiscall *p_basic_string_wchar_capacity)(basic_string_wchar*);
static void (__thiscall *p_basic_string_wchar_swap)(basic_string_wchar*, basic_string_wchar*);

static runtime_error* (__thiscall *p_runtime_error_ctor_bstr)(runtime_error*, const basic_string_char*);
static void (__thiscall *p_runtime_error_dtor)(runtime_error*);
static const char *(__thiscall *p_runtime_error_what)(runtime_error*);

static range_error* (__thiscall *p_range_error_ctor_bstr)(range_error*, const basic_string_char*);
static void (__thiscall *p_range_error_dtor)(range_error*);

static logic_error* (__thiscall *p_logic_error_ctor_bstr)(logic_error*, const basic_string_char*);
static void (__thiscall *p_logic_error_dtor)(logic_error*);
static const char *(__thiscall *p_logic_error_what)(logic_error*);

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
static void * (WINAPI *call_thiscall_func4)( void *func, void *this, const void *a, const void *b,
                                             const void *c );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, const void *a, const void *b,
                                             const void *c, const void *d );
static void * (WINAPI *call_thiscall_func6)( void *func, void *this, const void *a, const void *b,
                                             const void *c, const void *d, const void *e );

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
    call_thiscall_func4 = (void *)thunk;
    call_thiscall_func5 = (void *)thunk;
    call_thiscall_func6 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,(const void*)(a),\
        (const void*)(b),(const void*)(c))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(const void*)(a),\
        (const void*)(b),(const void*)(c),(const void*)(d))
#define call_func6(func,_this,a,b,c,d,e) call_thiscall_func6(func,_this,(const void*)(a),\
        (const void*)(b),(const void*)(c),(const void*)(d),(const void*)(e))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)
#define call_func6(func,_this,a,b,c,d,e) func(_this,a,b,c,d,e)

#endif /* __i386__ */

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp60.dll");
    if(!msvcp) {
        win_skip("msvcp60.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_basic_string_char_ctor,
                "??_F?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ");
        SET(p_basic_string_char_copy_ctor,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@@Z");
        SET(p_basic_string_char_ctor_cstr_alloc,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBDAEBV?$allocator@D@1@@Z");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ");
        SET(p_basic_string_char_erase,
                "?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0@Z");
        SET(p_basic_string_char_assign_cstr_len,
                "?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");
        SET(p_basic_string_char_data,
                "?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");
        SET(p_basic_string_char_size,
                "?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ");
        SET(p_basic_string_char_capacity,
                "?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ");
        SET(p_basic_string_char_swap,
                "?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEAV12@@Z");
        SET(p_basic_string_char_append,
                "?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@@Z");
        SET(p_basic_string_char_append_substr,
                "?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z");
        SET(p_basic_string_char_compare_substr_substr,
                "?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0AEBV12@00@Z");
        SET(p_basic_string_char_compare_substr_cstr_len,
                "?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0PEBD0@Z");
        SETNOFAIL(p_basic_string_char_concatenate,
                "??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@0@Z");
        SETNOFAIL(p_basic_string_char_concatenate_cstr,
                "??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@PEBD@Z");
        SET(p_basic_string_char_find_cstr_substr,
                "?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z");
        SET(p_basic_string_char_rfind_cstr_substr,
                "?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z");
        SET(p_basic_string_char_replace_cstr,
                "?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0PEBD@Z");
        SET(p_basic_string_char_npos,
                "?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2_KB");

        SET(p_basic_string_wchar_ctor,
                "??_F?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ");
        SET(p_basic_string_wchar_copy_ctor,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@@Z");
        SET(p_basic_string_wchar_ctor_cstr_alloc,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBGAEBV?$allocator@G@1@@Z");
        SET(p_basic_string_wchar_dtor,
                "??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ");
        SET(p_basic_string_wchar_erase,
                "?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0@Z");
        SET(p_basic_string_wchar_assign_cstr_len,
                "?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z");
        SET(p_basic_string_wchar_cstr,
                "?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ");
        SET(p_basic_string_wchar_data,
                "?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ");
        SET(p_basic_string_wchar_size,
                "?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ");
        SET(p_basic_string_wchar_capacity,
                "?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ");
        SET(p_basic_string_wchar_swap,
                "?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEAV12@@Z");

        SET(p_runtime_error_ctor_bstr,
                "??0runtime_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_runtime_error_dtor,
                "??1runtime_error@std@@UEAA@XZ");
        SET(p_runtime_error_what,
                "?what@runtime_error@std@@UEBAPEBDXZ");

        SET(p_range_error_ctor_bstr,
                "??0range_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_range_error_dtor,
                "??1range_error@std@@UEAA@XZ");

        SET(p_logic_error_ctor_bstr,
                "??0logic_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_logic_error_dtor,
                "??1logic_error@std@@UEAA@XZ");
        SET(p_logic_error_what,
                "?what@logic_error@std@@UEBAPEBDXZ");
    } else {
        SET(p_basic_string_char_ctor,
                "??_F?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ");
        SET(p_basic_string_char_copy_ctor,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z");
        SET(p_basic_string_char_ctor_cstr_alloc,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDABV?$allocator@D@1@@Z");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
        SET(p_basic_string_char_erase,
                "?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z");
        SET(p_basic_string_char_assign_cstr_len,
                "?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");
        SET(p_basic_string_char_data,
                "?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");
        SET(p_basic_string_char_size,
                "?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ");
        SET(p_basic_string_char_capacity,
                "?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ");
        SET(p_basic_string_char_swap,
                "?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXAAV12@@Z");
        SET(p_basic_string_char_append,
                "?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@@Z");
        SET(p_basic_string_char_append_substr,
                "?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z");
        SET(p_basic_string_char_compare_substr_substr,
                "?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIABV12@II@Z");
        SET(p_basic_string_char_compare_substr_cstr_len,
                "?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIPBDI@Z");
        SETNOFAIL(p_basic_string_char_concatenate,
                "??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@0@Z");
        SETNOFAIL(p_basic_string_char_concatenate_cstr,
                "??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@PBD@Z");
        SET(p_basic_string_char_find_cstr_substr,
                "?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z");
        SET(p_basic_string_char_rfind_cstr_substr,
                "?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z");
        SET(p_basic_string_char_replace_cstr,
                "?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIPBD@Z");
        SET(p_basic_string_char_npos,
                "?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2IB");

        SET(p_basic_string_wchar_ctor,
                "??_F?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ");
        SET(p_basic_string_wchar_copy_ctor,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@@Z");
        SET(p_basic_string_wchar_ctor_cstr_alloc,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGABV?$allocator@G@1@@Z");
        SET(p_basic_string_wchar_dtor,
                "??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ");
        SET(p_basic_string_wchar_erase,
                "?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@II@Z");
        SET(p_basic_string_wchar_assign_cstr_len,
                "?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z");
        SET(p_basic_string_wchar_cstr,
                "?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ");
        SET(p_basic_string_wchar_data,
                "?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ");
        SET(p_basic_string_wchar_size,
                "?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ");
        SET(p_basic_string_wchar_capacity,
                "?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ");
        SET(p_basic_string_wchar_swap,
                "?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXAAV12@@Z");

        SET(p_runtime_error_ctor_bstr,
                "??0runtime_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_runtime_error_dtor,
                "??1runtime_error@std@@UAE@XZ");
        SET(p_runtime_error_what,
                "?what@runtime_error@std@@UBEPBDXZ");

        SET(p_range_error_ctor_bstr,
                "??0range_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_range_error_dtor,
                "??1range_error@std@@UAE@XZ");

        SET(p_logic_error_ctor_bstr,
                "??0logic_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z");
        SET(p_logic_error_dtor,
                "??1logic_error@std@@UAE@XZ");
        SET(p_logic_error_what,
                "?what@logic_error@std@@UBEPBDXZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

static char fake_allocator;

static void test_basic_string_char(void) {
    basic_string_char str1, str2, *pstr;
    const char *str;
    size_t size, capacity;

    call_func1(p_basic_string_char_ctor, &str1);
    str = NULL;
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    str = call_func1(p_basic_string_char_data, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    call_func1(p_basic_string_char_dtor, &str1);

    pstr = call_func3(p_basic_string_char_ctor_cstr_alloc, &str1, "test", &fake_allocator);
    ok(pstr == &str1, "pstr != &str1\n");
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str1);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_char_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    pstr = call_func2(p_basic_string_char_copy_ctor, &str2, &str1);
    ok(pstr == &str2, "pstr != &str2\n");
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);

    call_func3(p_basic_string_char_erase, &str2, 1, 2);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "tt", 3), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "tt", 3), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_char_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    call_func3(p_basic_string_char_erase, &str2, 1, 100);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "t", 2), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "t", 2), "str = %s\n", str);
    size = (size_t)call_func1(p_basic_string_char_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_char_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    call_func3(p_basic_string_char_assign_cstr_len, &str2, "test", 4);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "test", 5), "str = %s\n", str);

    call_func3(p_basic_string_char_assign_cstr_len, &str2, (str+1), 2);
    str = call_func1(p_basic_string_char_cstr, &str2);
    ok(!memcmp(str, "es", 3), "str = %s\n", str);
    str = call_func1(p_basic_string_char_data, &str2);
    ok(!memcmp(str, "es", 3), "str = %s\n", str);

    call_func1(p_basic_string_char_dtor, &str1);
    call_func1(p_basic_string_char_dtor, &str2);
}

static void test_basic_string_char_swap(void) {
    basic_string_char str1, str2;
    char atmp1[32], atmp2[32];

    /* Swap self, local */
    strcpy(atmp1, "qwerty");
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str1, atmp1, &fake_allocator);
    call_func2(p_basic_string_char_swap, &str1, &str1);
    ok(strcmp(atmp1, call_func1(p_basic_string_char_cstr, &str1)) == 0, "Invalid value of str1\n");
    call_func2(p_basic_string_char_swap, &str1, &str1);
    ok(strcmp(atmp1, call_func1(p_basic_string_char_cstr, &str1)) == 0, "Invalid value of str1\n");
    call_func1(p_basic_string_char_dtor, &str1);

    /* str1 allocated, str2 local */
    strcpy(atmp1, "qwerty12345678901234567890");
    strcpy(atmp2, "asd");
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str1, atmp1, &fake_allocator);
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str2, atmp2, &fake_allocator);
    call_func2(p_basic_string_char_swap, &str1, &str2);
    ok(strcmp(atmp2, call_func1(p_basic_string_char_cstr, &str1)) == 0, "Invalid value of str1\n");
    ok(strcmp(atmp1, call_func1(p_basic_string_char_cstr, &str2)) == 0, "Invalid value of str2\n");
    call_func2(p_basic_string_char_swap, &str1, &str2);
    ok(strcmp(atmp1, call_func1(p_basic_string_char_cstr, &str1)) == 0, "Invalid value of str1\n");
    ok(strcmp(atmp2, call_func1(p_basic_string_char_cstr, &str2)) == 0, "Invalid value of str2\n");
    call_func1(p_basic_string_char_dtor, &str1);
    call_func1(p_basic_string_char_dtor, &str2);
}

static void test_basic_string_char_append(void) {
    basic_string_char str1, str2;
    const char *str;

    call_func3(p_basic_string_char_ctor_cstr_alloc, &str1, "", &fake_allocator);
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str2, "append", &fake_allocator);

    call_func2(p_basic_string_char_append, &str1, &str2);
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "append", 7), "str = %s\n", str);

    call_func4(p_basic_string_char_append_substr, &str1, &str2, 3, 1);
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "appende", 8), "str = %s\n", str);

    call_func4(p_basic_string_char_append_substr, &str1, &str2, 5, 100);
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "appended", 9), "str = %s\n", str);

    call_func4(p_basic_string_char_append_substr, &str1, &str2, 6, 100);
    str = call_func1(p_basic_string_char_cstr, &str1);
    ok(!memcmp(str, "appended", 9), "str = %s\n", str);

    call_func1(p_basic_string_char_dtor, &str1);
    call_func1(p_basic_string_char_dtor, &str2);
}

static void test_basic_string_char_compare(void) {
    basic_string_char str1, str2, str3;
    int ret;

    call_func3(p_basic_string_char_ctor_cstr_alloc, &str1, "str1str", &fake_allocator);
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str2, "str9str", &fake_allocator);
    call_func3(p_basic_string_char_ctor_cstr_alloc, &str3, "splash.png", &fake_allocator);

    ret = (int)call_func6(p_basic_string_char_compare_substr_substr,
            &str1, 0, 3, &str2, 0, 3);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func6(p_basic_string_char_compare_substr_substr,
            &str1, 4, 3, &str2, 4, 10);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func6(p_basic_string_char_compare_substr_substr,
            &str1, 1, 3, &str2, 1, 4);
    ok(ret == -1, "ret = %d\n", ret);

    ret = (int)call_func5(p_basic_string_char_compare_substr_cstr_len,
            &str1, 0, 1000, "str1str", 7);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func5(p_basic_string_char_compare_substr_cstr_len,
            &str3, 6, UINT_MAX, ".png", 4);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func5(p_basic_string_char_compare_substr_cstr_len,
            &str1, 1, 2, "tr", 2);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func5(p_basic_string_char_compare_substr_cstr_len,
            &str1, 1, 0, "aaa", 0);
    ok(ret == 0, "ret = %d\n", ret);
    ret = (int)call_func5(p_basic_string_char_compare_substr_cstr_len,
            &str1, 1, 0, "aaa", 1);
    ok(ret == -1, "ret = %d\n", ret);

    call_func1(p_basic_string_char_dtor, &str1);
    call_func1(p_basic_string_char_dtor, &str2);
    call_func1(p_basic_string_char_dtor, &str3);
}

static void test_basic_string_char_concatenate(void) {
    basic_string_char str, ret;
    const char *cstr;

    if(!p_basic_string_char_concatenate || !p_basic_string_char_concatenate_cstr) {
        win_skip("basic_string<char>::concatenate is not available\n");
        return;
    }

    call_func3(p_basic_string_char_ctor_cstr_alloc, &str, "test ", &fake_allocator);
    /* CDECL calling convention with return bigger than 8 bytes */
    p_basic_string_char_concatenate(&ret, &str, &str);
    cstr = call_func1(p_basic_string_char_cstr, &ret);
    ok(cstr != NULL, "cstr = NULL\n");
    ok(!strcmp(cstr, "test test "), "cstr = %s\n", cstr);
    call_func1(p_basic_string_char_dtor, &ret);

    p_basic_string_char_concatenate_cstr(&ret, &str, "passed");
    cstr = call_func1(p_basic_string_char_cstr, &ret);
    ok(cstr != NULL, "cstr = NULL\n");
    ok(!strcmp(cstr, "test passed"), "cstr = %s\n", cstr);
    call_func1(p_basic_string_char_dtor, &ret);

    call_func1(p_basic_string_char_dtor, &str);
}

static void test_basic_string_char_find(void) {
    basic_string_char str;
    size_t ret;

    call_func1(p_basic_string_char_ctor, &str);
    call_func3(p_basic_string_char_assign_cstr_len, &str, "aaa\0bbb", 7);
    ret = (size_t)call_func4(p_basic_string_char_find_cstr_substr, &str, "aaa", 0, 3);
    ok(ret == 0, "ret = %lu\n", (unsigned long)ret);
    ret = (size_t)call_func4(p_basic_string_char_find_cstr_substr, &str, "aaa", 1, 3);
    ok(ret == -1, "ret = %lu\n", (unsigned long)ret);
    ret = (size_t)call_func4(p_basic_string_char_find_cstr_substr, &str, "bbb", 0, 3);
    ok(ret == 4, "ret = %lu\n", (unsigned long)ret);
    call_func1(p_basic_string_char_dtor, &str);
}

static void test_basic_string_char_rfind(void) {
    struct rfind_char_test {
        const char *str;
        const char *find;
        size_t pos;
        size_t len;
        size_t ret;
    };

    int i;
    basic_string_char str;
    size_t ret;
    struct rfind_char_test tests[] = {
        { "",    "a",   0, 1, *p_basic_string_char_npos }, /* empty string */
        { "a",   "",    0, 0, 0 }, /* empty find */
        { "aaa", "aaa", 0, 3, 0 }, /* simple case */
        { "aaa", "a",   0, 1, 0 }, /* start of string */
        { "aaa", "a",   2, 1, 2 }, /* end of string */
        { "aaa", "a",   *p_basic_string_char_npos, 1, 2 }, /* off == npos */
        { "aaa", "z",   0, 1, *p_basic_string_char_npos }  /* can't find */
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);

        ret = (size_t)call_func4(p_basic_string_char_rfind_cstr_substr, &str,
            tests[i].find, tests[i].pos, tests[i].len);
        ok(ret == tests[i].ret, "str = '%s' find = '%s' ret = %lu\n",
            tests[i].str, tests[i].find, (unsigned long)ret);

        call_func1(p_basic_string_char_dtor, &str);
    }
}

static void test_basic_string_char_replace(void) {
    struct replace_char_test {
        const char *str;
        size_t off;
        size_t len;
        const char *replace;
        const char *ret;
    };

    int i;
    basic_string_char str;
    basic_string_char *ret;
    struct replace_char_test tests[] = {
        { "", 0, 0,  "", "" },  /* empty string */
        { "", 0, 10, "", "" },  /* empty string with invalid len */

        { "ABCDEF", 0, 0, "",  "ABCDEF" },  /* replace with empty string */
        { "ABCDEF", 0, 0, "-", "-ABCDEF"},  /* replace with 0 len */
        { "ABCDEF", 0, 1, "-", "-BCDEF" },  /* replace 1 at beginning */
        { "ABCDEF", 0, 3, "-", "-DEF" },    /* replace 3 at beginning */
        { "ABCDEF", 0, 42, "-", "-" },      /* replace whole string with invalid long len */
        { "ABCDEF", 0, *p_basic_string_char_npos, "-", "-" }, /* replace whole string with npos */

        { "ABCDEF", 5, 0, "",   "ABCDEF" },  /* replace at end with empty string */
        { "ABCDEF", 5, 0, "-",  "ABCDE-F"},  /* replace at end with 0 len */
        { "ABCDEF", 5, 1, "-",  "ABCDE-" },  /* replace 1 at end */
        { "ABCDEF", 5, 42, "-", "ABCDE-" },  /* replace end with invalid long len */
        { "ABCDEF", 5, *p_basic_string_char_npos, "-", "ABCDE-" }, /* replace end with npos */

        { "ABCDEF", 6, 0, "",   "ABCDEF" },   /* replace after end with empty string */
        { "ABCDEF", 6, 0, "-",  "ABCDEF-"},   /* replace after end with 0 len */
        { "ABCDEF", 6, 1, "-",  "ABCDEF-" },  /* replace 1 after end */
        { "ABCDEF", 6, 42, "-", "ABCDEF-" },  /* replace after end with invalid long len */
        { "ABCDEF", 6, *p_basic_string_char_npos, "-", "ABCDEF-" }, /* replace after end with npos */
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);

        ret = call_func4(p_basic_string_char_replace_cstr, &str, tests[i].off, tests[i].len, tests[i].replace);
        ok(ret == &str, "str = %p ret = %p\n", ret, &str);
        ok(strcmp(tests[i].ret, call_func1(p_basic_string_char_cstr, ret)) == 0, "str = %s ret = %s\n",
                  tests[i].ret, (const char*)call_func1(p_basic_string_char_cstr, ret));

        call_func1(p_basic_string_char_dtor, &str);
    }
}

static void test_basic_string_wchar(void) {
    basic_string_wchar str1, str2, *pstr;
    const wchar_t *str;
    size_t size, capacity;

    call_func1(p_basic_string_wchar_ctor, &str1);
    str = NULL;
    str = call_func1(p_basic_string_wchar_cstr, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    str = call_func1(p_basic_string_wchar_data, &str1);
    ok(str != NULL, "str = NULL\n");
    ok(*str == '\0', "*str = %c\n", *str);
    call_func1(p_basic_string_wchar_dtor, &str1);

    pstr = call_func3(p_basic_string_wchar_ctor_cstr_alloc, &str1, L"test", &fake_allocator);
    ok(pstr == &str1, "pstr != &str1\n");
    str = call_func1(p_basic_string_wchar_cstr, &str1);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str1);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_wchar_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    memset(&str2, 0, sizeof(basic_string_wchar));
    pstr = call_func2(p_basic_string_wchar_copy_ctor, &str2, &str1);
    ok(pstr == &str2, "pstr != &str2\n");
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));

    call_func3(p_basic_string_wchar_erase, &str2, 1, 2);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='t' && str[1]=='t' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='t' && str[1]=='t' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_wchar_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    call_func3(p_basic_string_wchar_erase, &str2, 1, 100);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='t' && str[1]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='t' && str[1]=='\0', "str = %s\n", wine_dbgstr_w(str));
    size = (size_t)call_func1(p_basic_string_wchar_size, &str1);
    ok(size == 4, "size = %lu\n", (unsigned long)size);
    capacity = (size_t)call_func1(p_basic_string_wchar_capacity, &str1);
    ok(capacity >= size, "capacity = %lu < size = %lu\n", (unsigned long)capacity, (unsigned long)size);

    call_func3(p_basic_string_wchar_assign_cstr_len, &str2, L"test", 4);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(!memcmp(str, L"test", 5*sizeof(wchar_t)), "str = %s\n", wine_dbgstr_w(str));

    call_func3(p_basic_string_wchar_assign_cstr_len, &str2, (str+1), 2);
    str = call_func1(p_basic_string_wchar_cstr, &str2);
    ok(str[0]=='e' && str[1]=='s' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));
    str = call_func1(p_basic_string_wchar_data, &str2);
    ok(str[0]=='e' && str[1]=='s' && str[2]=='\0', "str = %s\n", wine_dbgstr_w(str));

    call_func1(p_basic_string_wchar_dtor, &str1);
    call_func1(p_basic_string_wchar_dtor, &str2);
}

static void test_basic_string_wchar_swap(void) {
    basic_string_wchar str1, str2;
    wchar_t wtmp1[32], wtmp2[32];

    /* Swap self, local */
    mbstowcs(wtmp1, "qwerty", 32);
    call_func3(p_basic_string_wchar_ctor_cstr_alloc, &str1, wtmp1, &fake_allocator);
    call_func2(p_basic_string_wchar_swap, &str1, &str1);
    ok(wcscmp(wtmp1, call_func1(p_basic_string_wchar_cstr, &str1)) == 0, "Invalid value of str1\n");
    call_func2(p_basic_string_wchar_swap, &str1, &str1);
    ok(wcscmp(wtmp1, call_func1(p_basic_string_wchar_cstr, &str1)) == 0, "Invalid value of str1\n");
    call_func1(p_basic_string_wchar_dtor, &str1);

    /* str1 allocated, str2 local */
    mbstowcs(wtmp1, "qwerty12345678901234567890", 32);
    mbstowcs(wtmp2, "asd", 32);
    call_func3(p_basic_string_wchar_ctor_cstr_alloc, &str1, wtmp1, &fake_allocator);
    call_func3(p_basic_string_wchar_ctor_cstr_alloc, &str2, wtmp2, &fake_allocator);
    call_func2(p_basic_string_wchar_swap, &str1, &str2);
    ok(wcscmp(wtmp2, call_func1(p_basic_string_wchar_cstr, &str1)) == 0, "Invalid value of str1\n");
    ok(wcscmp(wtmp1, call_func1(p_basic_string_wchar_cstr, &str2)) == 0, "Invalid value of str2\n");
    call_func2(p_basic_string_wchar_swap, &str1, &str2);
    ok(wcscmp(wtmp1, call_func1(p_basic_string_wchar_cstr, &str1)) == 0, "Invalid value of str1\n");
    ok(wcscmp(wtmp2, call_func1(p_basic_string_wchar_cstr, &str2)) == 0, "Invalid value of str2\n");
    call_func1(p_basic_string_wchar_dtor, &str1);
    call_func1(p_basic_string_wchar_dtor, &str2);
}

static void test_exception(void)
{
    const char *name = "test";
    const char *what;
    basic_string_char str;
    runtime_error re;
    range_error ra;
    logic_error le;

    call_func3(p_basic_string_char_ctor_cstr_alloc, &str, name, &fake_allocator);

    /* runtime_error */
    memset(&re, 0, sizeof(re));
    what = call_func1(p_runtime_error_what, &re);
    ok(!strcmp("", what), "what = %s\n", what);

    memset(&re, 0xff, sizeof(re));
    call_func2(p_runtime_error_ctor_bstr, &re, &str);
    ok(!strcmp(re.e.name, ""), "re.e.name = %s\n", re.e.name);
    ok(re.e.do_free, "re.e.do_free == FALSE\n");
    what = call_func1(p_runtime_error_what, &re);
    ok(!strcmp(name, what), "what = %s\n", what);
    call_func1(p_runtime_error_dtor, &re);

    /* range_error */
    memset(&ra, 0, sizeof(ra));
    what = call_func1(p_runtime_error_what, (runtime_error*)&ra);
    ok(!strcmp("", what), "what = %s\n", what);

    memset(&ra, 0xff, sizeof(ra));
    call_func2(p_range_error_ctor_bstr, &ra, &str);
    ok(!strcmp(ra.e.name, ""), "ra.e.name = %s\n", ra.e.name);
    ok(ra.e.do_free, "ra.e.do_free == FALSE\n");
    what = call_func1(p_runtime_error_what, (runtime_error*)&ra);
    ok(!strcmp(name, what), "what = %s\n", what);
    call_func1(p_range_error_dtor, &ra);

    /* logic_error */
    memset(&le, 0, sizeof(le));
    what = call_func1(p_logic_error_what, &le);
    ok(!strcmp("", what), "what = %s\n", what);

    memset(&le, 0xff, sizeof(le));
    call_func2(p_logic_error_ctor_bstr, &le, &str);
    ok(!strcmp(le.e.name, ""), "le.e.name = %s\n", le.e.name);
    ok(le.e.do_free, "le.e.do_free == FALSE\n");
    what = call_func1(p_logic_error_what, &le);
    ok(!strcmp(name, what), "what = %s\n", what);
    call_func1(p_logic_error_dtor, &le);

    call_func1(p_basic_string_char_dtor, &str);
}

START_TEST(string)
{
    if(!init())
        return;

    test_basic_string_char();
    test_basic_string_char_swap();
    test_basic_string_char_append();
    test_basic_string_char_compare();
    test_basic_string_char_concatenate();
    test_basic_string_char_find();
    test_basic_string_char_rfind();
    test_basic_string_char_replace();
    test_basic_string_wchar();
    test_basic_string_wchar_swap();

    test_exception();

    FreeLibrary(msvcp);
}
