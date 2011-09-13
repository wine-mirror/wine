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

#include "stdlib.h"
#include "windef.h"

typedef unsigned char MSVCP_bool;
typedef SIZE_T MSVCP_size_t;
typedef SIZE_T streamoff;
typedef SIZE_T streamsize;

void __cdecl _invalid_parameter(const wchar_t*, const wchar_t*,
        const wchar_t*, unsigned int, uintptr_t);

extern void* (__cdecl *MSVCRT_operator_new)(MSVCP_size_t);
extern void (__cdecl *MSVCRT_operator_delete)(void*);
extern void* (__cdecl *MSVCRT_set_new_handler)(void*);

/* Copied from dlls/msvcrt/cpp.c */
#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define THISCALL_NAME(func) __ASM_NAME("__thiscall_" #func)
#define __thiscall __stdcall
#define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                      "popl %eax\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )

#define DEFINE_THISCALL_WRAPPER_RETPTR(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                      "popl %eax\n\t" \
                      "popl %edx\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %edx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )
#else /* __i386__ */

#define THISCALL(func) func
#define THISCALL_NAME(func) __ASM_NAME(#func)
#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */
#define DEFINE_THISCALL_WRAPPER_RETPTR(func,args)  /* nothing */

#endif /* __i386__ */

#ifdef _WIN64

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.align 8\n" \
            "\t.quad " __ASM_NAME(#name "_rtti") "\n" \
            "\t.globl " __ASM_NAME("MSVCP_" #name "_vtable") "\n" \
            __ASM_NAME("MSVCP_" #name "_vtable") ":\n" \
            "\t.quad " THISCALL_NAME(MSVCP_ ## name ## _vector_dtor) "\n" \
            funcs "\n\t.text");

#else

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.align 4\n" \
            "\t.long " __ASM_NAME(#name "_rtti") "\n" \
            "\t.globl " __ASM_NAME("MSVCP_" #name "_vtable") "\n" \
            __ASM_NAME("MSVCP_" #name "_vtable") ":\n" \
            "\t.long " THISCALL_NAME(MSVCP_ ## name ## _vector_dtor) "\n" \
            funcs "\n\t.text");

#endif /* _WIN64 */

/* exception object */
typedef void (*vtable_ptr)(void);
typedef struct __exception
{
    const vtable_ptr *vtable;
    char             *name;    /* Name of this exception, always a new copy for each object */
    int               do_free; /* Whether to free 'name' in our dtor */
} exception;

/* Internal: throws selected exception */
typedef enum __exception_type {
    EXCEPTION,
    EXCEPTION_BAD_ALLOC,
    EXCEPTION_LOGIC_ERROR,
    EXCEPTION_LENGTH_ERROR,
    EXCEPTION_OUT_OF_RANGE,
    EXCEPTION_INVALID_ARGUMENT
} exception_type;
void throw_exception(exception_type, const char *);
void set_exception_vtable(void);

/* rtti */
typedef struct __type_info
{
    const vtable_ptr *vtable;
    char              *name;        /* Unmangled name, allocated lazily */
    char               mangled[64]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

typedef struct _rtti_base_descriptor
{
    const type_info *type_descriptor;
    int num_base_classes;
    this_ptr_offsets offsets;    /* offsets for computing the this pointer */
    unsigned int attributes;
} rtti_base_descriptor;

typedef struct _rtti_base_array
{
    const rtti_base_descriptor *bases[4]; /* First element is the class itself */
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

/* basic_string<char, char_traits<char>, allocator<char>> */
#define BUF_SIZE_CHAR 16
typedef struct _basic_string_char
{
    void *allocator;
    union {
        char buf[BUF_SIZE_CHAR];
        char *ptr;
    } data;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_char;

basic_string_char* __stdcall MSVCP_basic_string_char_ctor_cstr(basic_string_char*, const char*);
basic_string_char* __stdcall MSVCP_basic_string_char_copy_ctor(basic_string_char*, const basic_string_char*);
void __stdcall MSVCP_basic_string_char_dtor(basic_string_char*);
const char* __stdcall MSVCP_basic_string_char_c_str(basic_string_char*);

#define BUF_SIZE_WCHAR 8
typedef struct _basic_string_wchar
{
    void *allocator;
    union {
        wchar_t buf[BUF_SIZE_WCHAR];
        wchar_t *ptr;
    } data;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

char* __stdcall MSVCP_allocator_char_allocate(void*, MSVCP_size_t);
void __stdcall MSVCP_allocator_char_deallocate(void*, char*, MSVCP_size_t);
wchar_t* __stdcall MSVCP_allocator_wchar_allocate(void*, MSVCP_size_t);
void __stdcall MSVCP_allocator_wchar_deallocate(void*, wchar_t*, MSVCP_size_t);

/* class locale */
typedef struct
{
    struct locale__Locimp *ptr;
} locale;

locale* __thiscall locale_ctor(locale*);
void __thiscall locale_dtor(locale*);

/* class _Lockit */
typedef struct {
    int locktype;
} _Lockit;

#define _LOCK_LOCALE 0
#define _LOCK_MALLOC 1
#define _LOCK_STREAM 2
#define _LOCK_DEBUG 3
#define _MAX_LOCK 4

void init_lockit(void);
void free_lockit(void);
_Lockit* __thiscall _Lockit_ctor_locktype(_Lockit*, int);
void __thiscall _Lockit_dtor(_Lockit*);

/* class mutex */
typedef struct {
        void *mutex;
} mutex;

mutex* __thiscall mutex_ctor(mutex*);
void __thiscall mutex_dtor(mutex*);
void __thiscall mutex_lock(mutex*);
void __thiscall mutex_unlock(mutex*);
