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

#include "windef.h"
#include "winternl.h"
#include "rtlsupportapi.h"
#include "wine/asm.h"

#ifdef __i386__
#undef CXX_USE_RVA
#else
#define CXX_USE_RVA 1
#endif

#ifndef _WIN64
#undef RTTI_USE_RVA
#else
#define RTTI_USE_RVA 1
#endif

#ifdef _WIN64

#define VTABLE_ADD_FUNC(name) "\t.quad " THISCALL_NAME(name) "\n"

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.balign 8\n" \
            "\t.quad " __ASM_NAME(#name "_rtti") "\n" \
            __ASM_GLOBL(__ASM_NAME(#name "_vtable")) "\n" \
            funcs "\n\t.text")

#else

#define VTABLE_ADD_FUNC(name) "\t.long " THISCALL_NAME(name) "\n"

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.balign 4\n" \
            "\t.long " __ASM_NAME(#name "_rtti") "\n" \
            __ASM_GLOBL(__ASM_NAME(#name "_vtable")) "\n" \
            funcs "\n\t.text")

#endif /* _WIN64 */

#ifndef RTTI_USE_RVA

#define DEFINE_RTTI_DATA(name, off, mangled_name, ...) \
static type_info name ## _type_info = { &type_info_vtable, NULL, mangled_name }; \
\
static const rtti_base_descriptor name ## _rtti_base_descriptor = \
    { &name ##_type_info, ARRAY_SIZE(((const void *[]){ __VA_ARGS__ })), { 0, -1, 0 }, 64 }; \
\
static const rtti_base_array name ## _rtti_base_array = \
    { { &name ## _rtti_base_descriptor, __VA_ARGS__ } }; \
\
static const rtti_object_hierarchy name ## _hierarchy = \
    { 0, 0, ARRAY_SIZE(((const void *[]){ NULL, __VA_ARGS__ })), &name ## _rtti_base_array }; \
\
const rtti_object_locator name ## _rtti = \
    { 0, off, 0, &name ## _type_info, &name ## _hierarchy };

#define INIT_RTTI(name,base) /* nothing to do */

#else  /* RTTI_USE_RVA */

#define DEFINE_RTTI_DATA(name, off, mangled_name, ...) \
static type_info name ## _type_info = { &type_info_vtable, NULL, mangled_name }; \
\
static rtti_base_descriptor name ## _rtti_base_descriptor = \
    { 0xdeadbeef, ARRAY_SIZE(((const void *[]){ __VA_ARGS__ })), { 0, -1, 0 }, 64 }; \
\
static rtti_base_array name ## _rtti_base_array; \
\
static rtti_object_hierarchy name ## _hierarchy = \
    { 0, 0, ARRAY_SIZE(((const void *[]){ NULL, __VA_ARGS__ })) }; \
\
rtti_object_locator name ## _rtti = \
    { 1, off, 0, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef }; \
\
static void init_ ## name ## _rtti(char *base) \
{ \
    const void * const name ## _rtti_bases[] = { &name ## _rtti_base_descriptor, __VA_ARGS__ }; \
    name ## _rtti_base_descriptor.type_descriptor = (char*)&name ## _type_info - base; \
    for (unsigned int i = 0; i < ARRAY_SIZE(name ## _rtti_bases); i++) \
        name ## _rtti_base_array.bases[i] = (char*)name ## _rtti_bases[i] - base; \
    name ## _hierarchy.base_classes = (char*)&name ## _rtti_base_array - base; \
    name ## _rtti.type_descriptor = (char*)&name ## _type_info - base; \
    name ## _rtti.type_hierarchy = (char*)&name ## _hierarchy - base; \
    name ## _rtti.object_locator = (char*)&name ## _rtti - base; \
}

#define INIT_RTTI(name,base) init_ ## name ## _rtti((void *)(base))

#endif  /* RTTI_USE_RVA */

#ifndef CXX_USE_RVA

#define DEFINE_CXX_TYPE(type, dtor, ...)  \
static const cxx_type_info type ## _cxx_type_info = \
    { 0, &type ##_type_info, { 0, -1, 0 }, sizeof(type), THISCALL(type ##_copy_ctor) }; \
\
static const cxx_type_info_table type ## _cxx_type_table = \
    { ARRAY_SIZE(((const void *[]){ NULL, __VA_ARGS__ })), { &type ## _cxx_type_info, __VA_ARGS__ } }; \
\
static const cxx_exception_type type ## _exception_type = \
    { 0, THISCALL(dtor), NULL, & type ## _cxx_type_table };

#define INIT_CXX_TYPE(name,base) (void)name ## _exception_type

#else  /* CXX_USE_RVA */

#define DEFINE_CXX_TYPE(type, dtor, ...)  \
static cxx_type_info type ## _cxx_type_info = \
    { 0, 0xdeadbeef, { 0, -1, 0 }, sizeof(type), 0xdeadbeef }; \
\
static const void * const type ## _cxx_type_classes[] = { &type ## _cxx_type_info, __VA_ARGS__ }; \
static cxx_type_info_table type ## _cxx_type_table = { ARRAY_SIZE(type ## _cxx_type_classes) }; \
static cxx_exception_type type ##_exception_type; \
\
static void init_ ## type ## _cxx(char *base) \
{ \
    type ## _cxx_type_info.type_info = (char *)&type ## _type_info - base; \
    type ## _cxx_type_info.copy_ctor = (char *)type ## _copy_ctor - base; \
    for (unsigned int i = 0; i < ARRAY_SIZE(type ## _cxx_type_classes); i++) \
        type ## _cxx_type_table.info[i] = (char *)type ## _cxx_type_classes[i] - base; \
    type ## _exception_type.destructor      = (char *)dtor - base; \
    type ## _exception_type.type_info_table = (char *)&type ## _cxx_type_table - base; \
}

#define INIT_CXX_TYPE(name,base) init_ ## name ## _cxx((void *)(base))

#endif  /* CXX_USE_RVA */

#ifdef __ASM_USE_THISCALL_WRAPPER

#define CALL_VTBL_FUNC(this, off, ret, type, args) ((ret (WINAPI*)type)&vtbl_wrapper_##off)args

extern void *vtbl_wrapper_0;
extern void *vtbl_wrapper_4;
extern void *vtbl_wrapper_8;
extern void *vtbl_wrapper_12;
extern void *vtbl_wrapper_16;
extern void *vtbl_wrapper_20;
extern void *vtbl_wrapper_24;
extern void *vtbl_wrapper_28;
extern void *vtbl_wrapper_32;
extern void *vtbl_wrapper_36;
extern void *vtbl_wrapper_40;
extern void *vtbl_wrapper_44;
extern void *vtbl_wrapper_48;
extern void *vtbl_wrapper_52;
extern void *vtbl_wrapper_56;

#else

#define CALL_VTBL_FUNC(this, off, ret, type, args) ((ret (__thiscall***)type)this)[0][off/4]args

#endif

/* exception object */
typedef void (*vtable_ptr)(void);
typedef struct __exception
{
    const vtable_ptr *vtable;
    char             *name;    /* Name of this exception, always a new copy for each object */
    int               do_free; /* Whether to free 'name' in our dtor */
} exception;

/* rtti */
typedef struct __type_info
{
    const vtable_ptr *vtable;
    char              *name;         /* Unmangled name, allocated lazily */
    char               mangled[128]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

#ifndef RTTI_USE_RVA

typedef struct _rtti_base_descriptor
{
    const type_info *type_descriptor;
    int num_base_classes;
    this_ptr_offsets offsets;    /* offsets for computing the this pointer */
    unsigned int attributes;
} rtti_base_descriptor;

typedef struct _rtti_base_array
{
    const rtti_base_descriptor *bases[10]; /* First element is the class itself */
} rtti_base_array;

typedef struct _rtti_object_hierarchy
{
    unsigned int signature;
    unsigned int attributes;
    int array_len; /* Size of the array pointed to by 'base_classes' */
    const rtti_base_array *base_classes;
} rtti_object_hierarchy;

typedef struct _rtti_object_locator
{
    unsigned int signature;
    int base_class_offset;
    unsigned int flags;
    const type_info *type_descriptor;
    const rtti_object_hierarchy *type_hierarchy;
} rtti_object_locator;

#else

typedef struct
{
    unsigned int type_descriptor;
    int num_base_classes;
    this_ptr_offsets offsets;    /* offsets for computing the this pointer */
    unsigned int attributes;
} rtti_base_descriptor;

typedef struct
{
    unsigned int bases[10]; /* First element is the class itself */
} rtti_base_array;

typedef struct
{
    unsigned int signature;
    unsigned int attributes;
    int array_len; /* Size of the array pointed to by 'base_classes' */
    unsigned int base_classes;
} rtti_object_hierarchy;

typedef struct
{
    unsigned int signature;
    int base_class_offset;
    unsigned int flags;
    unsigned int type_descriptor;
    unsigned int type_hierarchy;
    unsigned int object_locator;
} rtti_object_locator;

#endif  /* RTTI_USE_RVA */

#ifndef CXX_USE_RVA

typedef struct
{
    UINT flags;
    const type_info *type_info;
    this_ptr_offsets offsets;
    unsigned int size;
    void *copy_ctor;
} cxx_type_info;

typedef struct
{
    UINT count;
    const cxx_type_info *info[5];
} cxx_type_info_table;

typedef struct
{
    UINT flags;
    void *destructor;
    void *custom_handler;
    const cxx_type_info_table *type_info_table;
} cxx_exception_type;

#else

typedef struct
{
    UINT flags;
    unsigned int type_info;
    this_ptr_offsets offsets;
    unsigned int size;
    unsigned int copy_ctor;
} cxx_type_info;

typedef struct
{
    UINT count;
    unsigned int info[5];
} cxx_type_info_table;

typedef struct
{
    UINT flags;
    unsigned int destructor;
    unsigned int custom_handler;
    unsigned int type_info_table;
} cxx_exception_type;

#endif  /* CXX_USE_RVA */

extern const vtable_ptr type_info_vtable;

#ifdef CXX_USE_RVA

static inline uintptr_t cxx_rva_base( const void *ptr )
{
    void *base;
    return (uintptr_t)RtlPcToFileHeader( (void *)ptr, &base );
}

static inline void *cxx_rva( unsigned int rva, uintptr_t base )
{
    return (void *)(base + rva);
}

#else

static inline uintptr_t cxx_rva_base( const void *ptr )
{
    return 0;
}

static inline void *cxx_rva( const void *ptr, uintptr_t base )
{
    return (void *)ptr;
}

#endif

#define CREATE_TYPE_INFO_VTABLE \
DEFINE_THISCALL_WRAPPER(type_info_vector_dtor,8) \
void * __thiscall type_info_vector_dtor(type_info * _this, unsigned int flags) \
{ \
    if (flags & 2) \
    { \
        /* we have an array, with the number of elements stored before the first object */ \
        INT_PTR i, *ptr = (INT_PTR *)_this - 1; \
\
        for (i = *ptr - 1; i >= 0; i--) free(_this[i].name); \
        free(ptr); \
    } \
    else \
    { \
        free(_this->name); \
        if (flags & 1) free(_this); \
    } \
    return _this; \
} \
\
DEFINE_RTTI_DATA( type_info, 0, ".?AVtype_info@@" ) \
\
__ASM_BLOCK_BEGIN(type_info_vtables) \
    __ASM_VTABLE(type_info, \
            VTABLE_ADD_FUNC(type_info_vector_dtor)); \
__ASM_BLOCK_END
