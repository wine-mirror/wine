/*
 * msvcr80 specific functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include <config.h>

#include <stdarg.h>

#include "stdio.h"
#include "windef.h"
#include "winbase.h"

#if defined(__i386__) && !defined(__arm__)

#define THISCALL(func) __thiscall_ ## func
#define __thiscall __stdcall
#define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
            "popl %eax\n\t" \
            "pushl %ecx\n\t" \
            "pushl %eax\n\t" \
            "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

extern void *call_thiscall_func;
__ASM_GLOBAL_FUNC(call_thiscall_func,
        "popl %eax\n\t"
        "popl %edx\n\t"
        "popl %ecx\n\t"
        "pushl %eax\n\t"
        "jmp *%edx\n\t")

#define call_func1(func,this) ((void* (WINAPI*)(void*,void*))&call_thiscall_func)(func,this)
#define call_func2(func,this,a) ((void* (WINAPI*)(void*,void*,const void*))&call_thiscall_func)(func,this,(const void*)(a))
#define call_func3(func,this,a,b) ((void* (WINAPI*)(void*,void*,const void*,const void*))&call_thiscall_func)(func,this,(const void*)(a),(const void*)(b))

#else /* __i386__ */

#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#define call_func1(func,this) func(this)
#define call_func2(func,this,a) func(this,a)
#define call_func3(func,this,a,b) func(this,a,b)

#endif /* __i386__ */

static void* (__thiscall *MSVCRT_exception_ctor)(void*, const char**);
static void* (__thiscall *MSVCRT_exception_ctor_noalloc)(void*, char**, int);
static void* (__thiscall *MSVCRT_exception_copy_ctor)(void*, const void*);
static void* (__thiscall *MSVCRT_exception_default_ctor)(void*);
static void (__thiscall *MSVCRT_exception_dtor)(void*);
static int (__thiscall *MSVCRT_type_info_opequals_equals)(void*, const void*);
static int (__thiscall *MSVCRT_type_info_opnot_equals)(void*, const void*);
static const char* (__thiscall *MSVCR100_type_info_name_internal_method)(void*, void*);

static void init_cxx_funcs(void)
{
    HMODULE hmsvcrt = GetModuleHandleA("msvcrt.dll");
    HMODULE hmsvcr100 = GetModuleHandleA("msvcr100.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_exception_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QEAA@AEBQEBD@Z");
        MSVCRT_exception_ctor_noalloc = (void*)GetProcAddress(hmsvcrt, "??0exception@@QEAA@AEBQEBDH@Z");
        MSVCRT_exception_copy_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QEAA@AEBV0@@Z");
        MSVCRT_exception_default_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QEAA@XZ");
        MSVCRT_exception_dtor = (void*)GetProcAddress(hmsvcrt, "??1exception@@UEAA@XZ");
        MSVCRT_type_info_opequals_equals = (void*)GetProcAddress(hmsvcrt, "??8type_info@@QEBAHAEBV0@@Z");
        MSVCRT_type_info_opnot_equals = (void*)GetProcAddress(hmsvcrt, "??9type_info@@QEBAHAEBV0@@Z");
        MSVCR100_type_info_name_internal_method = (void*)GetProcAddress(hmsvcr100,
                "?_name_internal_method@type_info@@QEBAPEBDPEAU__type_info_node@@@Z");
    }
    else
    {
#ifdef __arm__
        MSVCRT_type_info_opequals_equals = (void*)GetProcAddress(hmsvcrt, "??8type_info@@QBA_NABV0@@Z");
        MSVCRT_type_info_opnot_equals = (void*)GetProcAddress(hmsvcrt, "??9type_info@@QBA_NABV0@@Z");
        MSVCR100_type_info_name_internal_method = (void*)GetProcAddress(hmsvcr100,
                "?_name_internal_method@type_info@@QBAPBDPAU__type_info_node@@@Z");
#else
        MSVCRT_exception_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QEAA@AEBQEBD@Z");
        MSVCRT_exception_ctor_noalloc = (void*)GetProcAddress(hmsvcrt, "??0exception@@QAE@ABQBDH@Z");
        MSVCRT_exception_copy_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QAE@ABV0@@Z");
        MSVCRT_exception_default_ctor = (void*)GetProcAddress(hmsvcrt, "??0exception@@QAE@XZ");
        MSVCRT_exception_dtor = (void*)GetProcAddress(hmsvcrt, "??1exception@@UAE@XZ");
        MSVCRT_type_info_opequals_equals = (void*)GetProcAddress(hmsvcrt, "??8type_info@@QBEHABV0@@Z");
        MSVCRT_type_info_opnot_equals = (void*)GetProcAddress(hmsvcrt, "??9type_info@@QBEHABV0@@Z");
        MSVCR100_type_info_name_internal_method = (void*)GetProcAddress(hmsvcr100,
                "?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z");
#endif
    }
}

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        init_cxx_funcs();
        _set_printf_count_output(0);
    }
    return TRUE;
}

void * CDECL MSVCR80__decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

void * CDECL MSVCR80__encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

/* ??0exception@std@@QAE@ABQBD@Z */
/* ??0exception@std@@QEAA@AEBQEBD@Z */
DEFINE_THISCALL_WRAPPER(exception_ctor, 8)
void* __thiscall exception_ctor(void *this, const char **name)
{
    return call_func2(MSVCRT_exception_ctor, this, name);
}

/* ??0exception@std@@QAE@ABQBDH@Z */
/* ??0exception@std@@QEAA@AEBQEBDH@Z */
DEFINE_THISCALL_WRAPPER(exception_ctor_noalloc, 12)
void* __thiscall exception_ctor_noalloc(void *this, char **name, int noalloc)
{
    return call_func3(MSVCRT_exception_ctor_noalloc, this, name, noalloc);
}

/* ??0exception@std@@QAE@ABV01@@Z */
/* ??0exception@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(exception_copy_ctor, 8)
void* __thiscall exception_copy_ctor(void *this, const void *rhs)
{
    return call_func2(MSVCRT_exception_copy_ctor, this, rhs);
}

/* ??0exception@std@@QAE@XZ */
/* ??0exception@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(exception_default_ctor, 4)
void* __thiscall exception_default_ctor(void *this)
{
    return call_func1(MSVCRT_exception_default_ctor, this);
}

/* ??1exception@std@@UAE@XZ */
/* ??1exception@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(exception_dtor, 4)
void __thiscall exception_dtor(void *this)
{
    call_func1(MSVCRT_exception_dtor, this);
}

/* ??8type_info@@QBA_NABV0@@Z */
/* ??8type_info@@QBE_NABV0@@Z */
/* ??8type_info@@QEBA_NAEBV0@@Z */
DEFINE_THISCALL_WRAPPER(type_info_op_equals, 8)
int __thiscall type_info_op_equals(void *this, const void *rhs)
{
    return (int)call_func2(MSVCRT_type_info_opequals_equals, this, rhs);
}

/* ??9type_info@@QBA_NABV0@@Z */
/* ??9type_info@@QBE_NABV0@@Z */
/* ??9type_info@@QEBA_NAEBV0@@Z */
DEFINE_THISCALL_WRAPPER(type_info_opnot_equals, 8)
int __thiscall type_info_opnot_equals(void *this, const void *rhs)
{
    return (int)call_func2(MSVCRT_type_info_opnot_equals, this, rhs);
}

/* ?_name_internal_method@type_info@@QBAPBDPAU__type_info_node@@@Z */
/* ?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z */
/* ?_name_internal_method@type_info@@QEBAPEBDPEAU__type_info_node@@@Z */
DEFINE_THISCALL_WRAPPER(type_info_name_internal_method,8)
const char* __thiscall type_info_name_internal_method(void *this, void *node)
{
    return call_func2(MSVCR100_type_info_name_internal_method, this, node);
}
