/*
 * Copyright 2021 Arkadiusz Hiler for CodeWeavers
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
#include <stdbool.h>
#include <malloc.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "winnls.h"
#include "cxx.h"

#define NEW_ALIGNMENT (2*sizeof(void*))

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

CREATE_TYPE_INFO_VTABLE

static HMODULE msvcp140;

void (__cdecl *throw_bad_alloc)(void);

/* non-static, needed by type_info */
void* __cdecl MSVCRT_operator_new(size_t size)
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
    throw_bad_alloc();
    return NULL;
}

void __cdecl MSVCRT_operator_delete(void *mem)
{
    free(mem);
}

static void* MSVCRT_operator_new_aligned(size_t size, size_t alignment)
{
    void *retval;
    int freed;

    do
    {
        retval = _aligned_malloc(size, alignment);
        if (retval)
        {
            TRACE("(%Iu) returning %p\n", size, retval);
            return retval;
        }
        freed = _callnewh(size);
    } while (freed);

    TRACE("(%Iu) out of memory\n", size);
    throw_bad_alloc();
    return NULL;
}

static void MSVCRT_operator_delete_aligned(void *mem, size_t alignment)
{
    _aligned_free(mem);
}

typedef struct {
    const vtable_ptr *vtable;
} memory_resource;

extern const vtable_ptr aligned_resource_vtable;
extern const vtable_ptr unaligned_resource_vtable;
extern const vtable_ptr null_resource_vtable;

__ASM_BLOCK_BEGIN(vtables)
    __ASM_VTABLE(aligned_resource,
            VTABLE_ADD_FUNC(nop_dtor)
            VTABLE_ADD_FUNC(aligned_do_allocate)
            VTABLE_ADD_FUNC(aligned_do_deallocate)
            VTABLE_ADD_FUNC(do_is_equal));
    __ASM_VTABLE(unaligned_resource,
            VTABLE_ADD_FUNC(nop_dtor)
            VTABLE_ADD_FUNC(unaligned_do_allocate)
            VTABLE_ADD_FUNC(unaligned_do_deallocate)
            VTABLE_ADD_FUNC(do_is_equal));
    __ASM_VTABLE(null_resource,
            VTABLE_ADD_FUNC(nop_dtor)
            VTABLE_ADD_FUNC(null_do_allocate)
            VTABLE_ADD_FUNC(null_do_deallocate)
            VTABLE_ADD_FUNC(do_is_equal));
__ASM_BLOCK_END

DEFINE_RTTI_DATA(base_memory_resource, 0, ".?AVmemory_resource@pmr@std@@")
DEFINE_RTTI_DATA(_Identity_equal_resource, 0, ".?AV_Identity_equal_resource@pmr@std@@")
DEFINE_RTTI_DATA(aligned_resource, 0, ".?AV_Aligned_new_delete_resource_impl@pmr@std@@",
        _Identity_equal_resource_rtti_base_descriptor,
        base_memory_resource_rtti_base_descriptor)
DEFINE_RTTI_DATA(unaligned_resource, 0,
        ".?AV_Unaligned_new_delete_resource_impl@pmr@std@@",
        _Identity_equal_resource_rtti_base_descriptor,
        base_memory_resource_rtti_base_descriptor)
DEFINE_RTTI_DATA(null_resource, 0,
        ".?AV_Null_resource@?1??null_memory_resource@@YAPAVmemory_resource@pmr@std@@XZ",
        _Identity_equal_resource_rtti_base_descriptor,
        base_memory_resource_rtti_base_descriptor)

DEFINE_THISCALL_WRAPPER(nop_dtor, 4)
void __thiscall nop_dtor(void *this)
{
    /* nop */
}

DEFINE_THISCALL_WRAPPER(do_is_equal, 8)
bool __thiscall do_is_equal(memory_resource *this, memory_resource *other)
{
    return this == other;
}

DEFINE_THISCALL_WRAPPER(aligned_do_allocate, 12)
void* __thiscall aligned_do_allocate(memory_resource *this, size_t bytes, size_t alignment)
{
    if (alignment > NEW_ALIGNMENT)
        return MSVCRT_operator_new_aligned(bytes, alignment);
    else
        return MSVCRT_operator_new(bytes);
}

DEFINE_THISCALL_WRAPPER(aligned_do_deallocate, 16)
void __thiscall aligned_do_deallocate(memory_resource *this,
        void *p, size_t bytes, size_t alignment)
{
    if (alignment > NEW_ALIGNMENT)
        MSVCRT_operator_delete_aligned(p, alignment);
    else
        MSVCRT_operator_delete(p);
}

DEFINE_THISCALL_WRAPPER(unaligned_do_allocate, 12)
void* __thiscall unaligned_do_allocate(memory_resource *this,
        size_t bytes, size_t alignment)
{
    if (alignment > NEW_ALIGNMENT)
        throw_bad_alloc();

    return MSVCRT_operator_new(bytes);
}

DEFINE_THISCALL_WRAPPER(unaligned_do_deallocate, 16)
void __thiscall unaligned_do_deallocate(memory_resource *this,
        void *p, size_t bytes, size_t alignment)
{
    MSVCRT_operator_delete(p);
}

DEFINE_THISCALL_WRAPPER(null_do_allocate, 12)
void* __thiscall null_do_allocate(memory_resource *this,
        size_t bytes, size_t alignment)
{
    throw_bad_alloc();
    return NULL;
}

DEFINE_THISCALL_WRAPPER(null_do_deallocate, 16)
void __thiscall null_do_deallocate(memory_resource *this,
        void *p, size_t bytes, size_t alignment)
{
    /* nop */
}

static memory_resource *default_resource;

/* EXPORTS */

memory_resource* __cdecl _Aligned_new_delete_resource(void)
{
    static memory_resource impl = { &aligned_resource_vtable };
    return &impl;
}

memory_resource* __cdecl _Unaligned_new_delete_resource(void)
{
    static memory_resource impl = { &unaligned_resource_vtable };
    return &impl;
}

memory_resource* __cdecl _Aligned_get_default_resource(void)
{
    if (default_resource) return default_resource;
    return _Aligned_new_delete_resource();
}

memory_resource* __cdecl _Aligned_set_default_resource(memory_resource *res)
{
    memory_resource *ret = InterlockedExchangePointer((void**)&default_resource, res);
    if (!ret) ret = _Aligned_new_delete_resource();
    return ret;
}

memory_resource* __cdecl _Unaligned_get_default_resource(void)
{
    if (default_resource) return default_resource;
    return _Unaligned_new_delete_resource();
}

memory_resource* __cdecl _Unaligned_set_default_resource(memory_resource *res)
{
    memory_resource *ret = InterlockedExchangePointer((void**)&default_resource, res);
    if (!ret) ret = _Unaligned_new_delete_resource();
    return ret;
}

memory_resource* __cdecl null_memory_resource(void)
{
    static memory_resource impl = { &null_resource_vtable };
    return &impl;
}

/* DLL INIT */

static BOOL init_cxx_funcs(void)
{
    msvcp140 = LoadLibraryA("msvcp140.dll");
    if (!msvcp140)
    {
        FIXME("Failed to load msvcp140.dll\n");
        return FALSE;
    }

    throw_bad_alloc = (void*)GetProcAddress(msvcp140, "?_Xbad_alloc@std@@YAXXZ");
    if (!throw_bad_alloc)
    {
        FIXME("Failed to get address of ?_Xbad_alloc@std@@YAXXZ\n");
        FreeLibrary(msvcp140);
        return FALSE;
    }

    return TRUE;
}

static void init_rtti(void *base)
{
    INIT_RTTI(type_info, base);
    INIT_RTTI(base_memory_resource, base);
    INIT_RTTI(_Identity_equal_resource, base);
    INIT_RTTI(aligned_resource, base);
    INIT_RTTI(unaligned_resource, base);
    INIT_RTTI(null_resource, base);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
       if (!init_cxx_funcs()) return FALSE;
       init_rtti(inst);
       break;
   case DLL_PROCESS_DETACH:
       if (reserved) break;
       FreeLibrary(msvcp140);
       break;
   }
   return TRUE;
}
