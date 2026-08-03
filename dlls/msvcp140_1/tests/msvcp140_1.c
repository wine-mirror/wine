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

#include <errno.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

typedef unsigned char MSVCP_bool;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

#define NEW_ALIGNMENT (2*sizeof(void*))

/* Emulate __thiscall */
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

static void* (WINAPI *call_thiscall_func1)(void *func, void *this);
static void* (WINAPI *call_thiscall_func2)(void *func, void *this, const void *a);
static void* (WINAPI *call_thiscall_func3)(void *func,
        void *this, const void *a, const void *b);
static void* (WINAPI *call_thiscall_func4)(void *func,
        void *this, const void *a, const void *b, const void *c);

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc(NULL, sizeof(*thunk),
            MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void*)thunk;
    call_thiscall_func2 = (void*)thunk;
    call_thiscall_func3 = (void*)thunk;
    call_thiscall_func4 = (void*)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,\
        (const void*)(a),(const void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,\
        (const void*)(a),(const void*)(b), (const void*)(c))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)

#endif /* __i386__ */

struct memory_resource_vtbl;

typedef struct {
    struct memory_resource_vtbl *vtbl;
} memory_resource;

struct memory_resource_vtbl
{
    void (__thiscall *dtor)(memory_resource *this);
    void* (__thiscall *do_allocate)(memory_resource *this,
            size_t bytes, size_t alignment);
    void (__thiscall *do_deallocate)(memory_resource *this,
            void *p, size_t bytes, size_t alignment);
    MSVCP_bool (__thiscall *do_is_equal)(memory_resource *this,
            memory_resource *other);
};

static HMODULE msvcp;
static memory_resource* (__cdecl *p__Aligned_new_delete_resource)(void);
static memory_resource* (__cdecl *p__Unaligned_new_delete_resource)(void);
static memory_resource* (__cdecl *p__Aligned_get_default_resource)(void);
static memory_resource* (__cdecl *p__Unaligned_get_default_resource)(void);
static memory_resource* (__cdecl *p__Aligned_set_default_resource)(memory_resource*);
static memory_resource* (__cdecl *p__Unaligned_set_default_resource)(memory_resource*);
static memory_resource* (__cdecl *p_null_memory_resource)(void);

static HMODULE ucrtbase;
static void* (__cdecl *p_malloc)(size_t size);
static void (__cdecl *p_free)(void *ptr);
static void* (__cdecl *p__aligned_malloc)(size_t size, size_t alignment);
static void (__cdecl *p__aligned_free)(void *ptr);

#define SETNOFAIL(lib,x,y) x = (void*)GetProcAddress(lib,y)
#define SET(lib,x,y) do { SETNOFAIL(lib,x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp140_1.dll");
    if(!msvcp)
    {
        win_skip("msvcp140_1.dll not installed\n");
        return FALSE;
    }

    ucrtbase = LoadLibraryA("ucrtbase.dll");
    if(!ucrtbase)
    {
        win_skip("ucrtbase.dll not installed\n");
        FreeLibrary(msvcp);
        return FALSE;
    }

    SET(msvcp, p__Aligned_new_delete_resource, "_Aligned_new_delete_resource");
    SET(msvcp, p__Unaligned_new_delete_resource, "_Unaligned_new_delete_resource");
    SET(msvcp, p_null_memory_resource, "null_memory_resource");
    SET(msvcp, p__Aligned_get_default_resource, "_Aligned_get_default_resource");
    SET(msvcp, p__Unaligned_get_default_resource, "_Unaligned_get_default_resource");
    SET(msvcp, p__Aligned_set_default_resource, "_Aligned_set_default_resource");
    SET(msvcp, p__Unaligned_set_default_resource, "_Unaligned_set_default_resource");

    SET(ucrtbase, p__aligned_malloc, "_aligned_malloc");
    SET(ucrtbase, p__aligned_free, "_aligned_free");
    SET(ucrtbase, p_malloc, "malloc");
    SET(ucrtbase, p_free, "free");

    init_thiscall_thunk();

    return TRUE;
}

static void test__Aligned_new_delete_resource(void)
{
    void *ptr;
    memory_resource *resource = p__Aligned_new_delete_resource();
    ok(resource != NULL, "Failed to get aligned new delete memory resource.\n");

    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, 64);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, 64);

    /* up to the NEW_ALIGNMENT it should use the non-aligned new/delete */
    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    p_free(ptr); /* delete, crashes with aligned */

    ptr = p_malloc(140);
    ok(ptr != NULL, "Failed to allocate memory using _aligned_malloc.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, NEW_ALIGNMENT);

    /* past the NEW_ALIGNMENT it should use the aligned new/delete */
    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT * 2);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    p__aligned_free(ptr); /* aligned delete, crashes with non-aligned */

    ptr = p__aligned_malloc(140, NEW_ALIGNMENT * 2);
    ok(ptr != NULL, "Failed to allocate memory using _aligned_malloc.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, NEW_ALIGNMENT * 2);

    /* until the NEW_ALIGNMENT it doesn't have to be a power of two */
    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT - 1);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, NEW_ALIGNMENT - 1);

    /* crashes with alignment not being a power of two past the NEW_ALIGNMENT */
    /* ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT * 2 + 1); */

    ok((MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource),
            "Resource should be equal to itself.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, NULL),
            "Resource should not be equal to NULL.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource+1),
            "Resource should not be equal to a random pointer.\n");
}

static void test__Unaligned_new_delete_resource(void)
{
    void *ptr;
    memory_resource *resource = p__Unaligned_new_delete_resource();
    ok(resource != NULL, "Failed to get unaligned new delete memory resource.\n");

    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, NEW_ALIGNMENT);

    /* up to the NEW_ALIGNMENT it is using non-aligned new/delete */
    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    p_free(ptr); /* delete */

    /* alignment doesn't have to be a power of two */
    ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT - 1);
    ok(ptr != NULL, "Failed to allocate memory using memory resource.\n");
    p_free(ptr); /* delete */

    ptr = p_malloc(140);
    ok(ptr != NULL, "Failed to allocate memory using malloc.\n");
    call_func4(resource->vtbl->do_deallocate, resource, ptr, 140, NEW_ALIGNMENT);

    /* alignment past NEW_ALIGNMENT results in bad alloc exception */
    /* ptr = call_func3(resource->vtbl->do_allocate, resource, 140, NEW_ALIGNMENT * 2); */

    ok((MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource),
            "Resource should be equal to itself.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, NULL),
            "Resource should not be equal to NULL.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource+1),
            "Resource should not be equal to a random pointer.\n");
}

static void test_null_memory_resource(void)
{
    memory_resource *resource = p_null_memory_resource();
    ok(resource != NULL, "Failed to get null memory resource.\n");

    /* should result in bad alloc exception */
    /* call_func3(resource->vtbl->do_allocate, resource, 140, 8); */

    /* harmless nop */
    call_func4(resource->vtbl->do_deallocate, resource, (void*)(INT_PTR)-1, 140, 2);

    ok((MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource),
            "Resource should be equal to itself.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, NULL),
            "Resource should not be equal to NULL.\n");
    ok(!(MSVCP_bool)(INT_PTR)call_func2(resource->vtbl->do_is_equal, resource, resource+1),
            "Resource should not be equal to a random pointer.\n");
}

static void test_get_set_default_resource(
        memory_resource *(__cdecl *new_delete_resource)(void),
        memory_resource *(__cdecl *set_default_resource)(memory_resource*))
{
    ok(p__Unaligned_get_default_resource() == p__Unaligned_new_delete_resource(),
            "The default aligned resource should be equal to new/delete one.\n");
    ok(p__Aligned_get_default_resource() == p__Aligned_new_delete_resource(),
            "The default unaligned resource should be equal to new/delete one.\n");

    ok(set_default_resource((void*)0xdeadbeef) == new_delete_resource(),
            "Setting default resource should return the old one.\n");

    /* the value is shared */
    ok(p__Unaligned_get_default_resource() == (void*)0xdeadbeef,
            "Setting resource should change the default unaligned resource.\n");
    ok(p__Aligned_get_default_resource() == (void*)0xdeadbeef,
            "Setting resource should change the default aligned resource.\n");

    ok(set_default_resource(NULL) == (void*)0xdeadbeef,
            "Setting default resource should return the old one.\n");

    ok(p__Unaligned_get_default_resource() == p__Unaligned_new_delete_resource(),
            "Setting the default resource to NULL should reset the unaligned default.\n");
    ok(p__Aligned_get_default_resource() == p__Aligned_new_delete_resource(),
            "Setting the default resource to NULL should reset the aligned default.\n");
}

START_TEST(msvcp140_1)
{
    if (!init()) return;

    test__Aligned_new_delete_resource();
    test__Unaligned_new_delete_resource();

    test_null_memory_resource();

    test_get_set_default_resource(p__Aligned_new_delete_resource,
            p__Aligned_set_default_resource);
    test_get_set_default_resource(p__Unaligned_new_delete_resource,
            p__Unaligned_set_default_resource);

    FreeLibrary(msvcp);
    FreeLibrary(ucrtbase);
}
