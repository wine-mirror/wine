/*
 * Copyright 2021 Piotr Caban for CodeWeavers
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

#include <malloc.h>
#include <stdarg.h>

#include "msvcrt.h"
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "details.h"

WINE_DEFAULT_DEBUG_CHANNEL(concrt);

CREATE_TYPE_INFO_VTABLE
CREATE_EXCEPTION_OBJECT(exception)

static HMODULE msvcp140;

extern const vtable_ptr exception_vtable;

int CDECL _callnewh(size_t size);
void (__cdecl *_Xmem)(void);
void (__cdecl *_Xout_of_range)(const char*);

void* __cdecl operator_new(size_t size)
{
    void *retval;
    int freed;

    do
    {
        retval = malloc(size);
        if (retval)
        {
            TRACE("(%Iu) returning %p\n", size, retval);
            return retval;
        }
        freed = _callnewh(size);
    } while (freed);

    TRACE("(%Iu) out of memory\n", size);
    _Xmem();
    return NULL;
}

void __cdecl operator_delete(void *mem)
{
    free(mem);
}

typedef exception runtime_error;
extern const vtable_ptr runtime_error_vtable;

DEFINE_THISCALL_WRAPPER(runtime_error_copy_ctor,8)
runtime_error * __thiscall runtime_error_copy_ctor(runtime_error *this, const runtime_error *rhs)
{
    return __exception_copy_ctor(this, rhs, &runtime_error_vtable);
}

typedef exception range_error;
extern const vtable_ptr range_error_vtable;

DEFINE_THISCALL_WRAPPER(range_error_copy_ctor,8)
range_error * __thiscall range_error_copy_ctor(range_error *this, const range_error *rhs)
{
    return __exception_copy_ctor(this, rhs, &range_error_vtable);
}

__ASM_BLOCK_BEGIN(vtables)
__ASM_VTABLE(runtime_error,
        VTABLE_ADD_FUNC(exception_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
__ASM_VTABLE(range_error,
        VTABLE_ADD_FUNC(exception_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
__ASM_BLOCK_END

DEFINE_CXX_DATA0( exception, exception_dtor )
DEFINE_RTTI_DATA1(runtime_error, 0, &exception_rtti_base_descriptor, ".?AVruntime_error@std@@")
DEFINE_CXX_DATA1(runtime_error, &exception_cxx_type_info, exception_dtor)
DEFINE_RTTI_DATA2(range_error, 0, &runtime_error_rtti_base_descriptor,
        &exception_rtti_base_descriptor, ".?AVrange_error@std@@")
DEFINE_CXX_DATA2(range_error, &runtime_error_cxx_type_info,
        &exception_cxx_type_info, exception_dtor)

void DECLSPEC_NORETURN throw_range_error(const char *str)
{
    range_error e;
    __exception_ctor(&e, str, &range_error_vtable);
    _CxxThrowException(&e, &range_error_exception_type);
}

void DECLSPEC_NORETURN throw_exception(const char* msg)
{
    exception e;
    __exception_ctor(&e, msg, &exception_vtable);
    _CxxThrowException(&e, &exception_exception_type);
}

static BOOL init_cxx_funcs(void)
{
    msvcp140 = LoadLibraryA("msvcp140.dll");
    if (!msvcp140)
    {
        FIXME("Failed to load msvcp140.dll\n");
        return FALSE;
    }

    _Xmem = (void*)GetProcAddress(msvcp140, "?_Xbad_alloc@std@@YAXXZ");
    _Xout_of_range = (void*)GetProcAddress(msvcp140, sizeof(void*) > sizeof(int) ?
            "?_Xout_of_range@std@@YAXPEBD@Z" : "?_Xout_of_range@std@@YAXPBD@Z");
    if (!_Xmem || !_Xout_of_range)
    {
        FreeLibrary(msvcp140);
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
       if (!init_cxx_funcs()) return FALSE;
       INIT_RTTI(exception, inst);
       INIT_RTTI(range_error, inst);
       INIT_RTTI(runtime_error, inst);
       INIT_RTTI(type_info, inst);

       INIT_CXX_TYPE(exception, inst);
       INIT_CXX_TYPE(runtime_error, inst);
       INIT_CXX_TYPE(range_error, inst);
       msvcrt_init_concurrency(inst);
       init_concurrency_details(inst);
       break;
   case DLL_PROCESS_DETACH:
       if (reserved) break;
       FreeLibrary(msvcp140);
       break;
   }
   return TRUE;
}
