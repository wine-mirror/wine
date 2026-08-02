/*
 * msvcr90 specific functions
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

#ifdef __i386__

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

typedef void (__cdecl *MSVCRT__se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);

static void* (__cdecl *MSVCRT_operator_new)(size_t);
static void (__cdecl *MSVCRT_operator_delete)(void*);
static MSVCRT__se_translator_function (__cdecl *MSVCRT__set_se_translator)(MSVCRT__se_translator_function);
static void* (__thiscall *MSVCRT_exception_ctor)(void*, const char**);
static void* (__thiscall *MSVCRT_exception_ctor_noalloc)(void*, char**, int);
static void* (__thiscall *MSVCRT_exception_copy_ctor)(void*, const void*);
static void (__thiscall *MSVCRT_exception_dtor)(void*);

static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPEAX_K@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
        MSVCRT__set_se_translator = (void*)GetProcAddress(hmod,
                "?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z");
        MSVCRT_exception_ctor = (void*)GetProcAddress(hmod, "??0exception@@QEAA@AEBQEBD@Z");
        MSVCRT_exception_ctor_noalloc = (void*)GetProcAddress(hmod, "??0exception@@QEAA@AEBQEBDH@Z");
        MSVCRT_exception_copy_ctor = (void*)GetProcAddress(hmod, "??0exception@@QEAA@AEBV0@@Z");
        MSVCRT_exception_dtor = (void*)GetProcAddress(hmod, "??1exception@@UEAA@XZ");
    }
    else
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPAXI@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
        MSVCRT__set_se_translator = (void*)GetProcAddress(hmod,
                "?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z");
        MSVCRT_exception_ctor = (void*)GetProcAddress(hmod, "??0exception@@QAE@ABQBD@Z");
        MSVCRT_exception_ctor_noalloc = (void*)GetProcAddress(hmod, "??0exception@@QAE@ABQBDH@Z");
        MSVCRT_exception_copy_ctor = (void*)GetProcAddress(hmod, "??0exception@@QAE@ABV0@@Z");
        MSVCRT_exception_dtor = (void*)GetProcAddress(hmod, "??1exception@@UAE@XZ");
    }
}

/*********************************************************************
 *  DllMain (MSVCR90.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        init_cxx_funcs();
        _set_printf_count_output(0);
    }
    return TRUE;
}

/*********************************************************************
 *  _decode_pointer (MSVCR90.@)
 *
 * cdecl version of DecodePointer
 *
 */
void * CDECL MSVCR90_decode_pointer(void * ptr)
{
    return DecodePointer(ptr);
}

/*********************************************************************
 *  _encode_pointer (MSVCR90.@)
 *
 * cdecl version of EncodePointer
 *
 */
void * CDECL MSVCR90_encode_pointer(void * ptr)
{
    return EncodePointer(ptr);
}

/*********************************************************************
 *  ??2@YAPAXI@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
void* CDECL MSVCR90_operator_new(size_t size)
{
    return MSVCRT_operator_new(size);
}

/*********************************************************************
 *  ??3@YAXPAX@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
void CDECL MSVCR90_operator_delete(void *ptr)
{
    MSVCRT_operator_delete(ptr);
}

/*********************************************************************
 *  ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z (MSVCR90.@)
 *
 * Naver LINE expects that this function is implemented inside msvcr90
 */
MSVCRT__se_translator_function CDECL MSVCR90__set_se_translator(MSVCRT__se_translator_function func)
{
    return MSVCRT__set_se_translator(func);
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

/* ??1exception@std@@UAE@XZ */
/* ??1exception@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(exception_dtor, 4)
void __thiscall exception_dtor(void *this)
{
    call_func1(MSVCRT_exception_dtor, this);
}
