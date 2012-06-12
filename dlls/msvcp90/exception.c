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

#include "config.h"

#include <stdarg.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

/* dlls/msvcrt/cppexcept.h */
typedef void (*cxx_copy_ctor)(void);

/* complete information about a C++ type */
typedef struct __cxx_type_info
{
    UINT             flags;        /* flags (see CLASS_* flags below) */
    const type_info *type_info;    /* C++ type info */
    this_ptr_offsets offsets;      /* offsets for computing the this pointer */
    unsigned int     size;         /* object size */
    cxx_copy_ctor    copy_ctor;    /* copy constructor */
} cxx_type_info;
#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
typedef struct __cxx_type_info_table
{
    UINT                 count;     /* number of types */
    const cxx_type_info *info[3];   /* variable length, we declare it large enough for static RTTI */
} cxx_type_info_table;

/* type information for an exception object */
typedef struct __cxx_exception_type
{
    UINT                       flags;            /* TYPE_FLAG flags */
    void                     (*destructor)(void);/* exception object destructor */
    void* /*cxx_exc_custom_handler*/ custom_handler;   /* custom handler for this exception */
    const cxx_type_info_table *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;

void WINAPI _CxxThrowException(exception*,const cxx_exception_type*);

/* vtables */
extern const vtable_ptr MSVCP_exception_vtable;
extern const vtable_ptr MSVCP_bad_alloc_vtable;
extern const vtable_ptr MSVCP_logic_error_vtable;
extern const vtable_ptr MSVCP_length_error_vtable;
extern const vtable_ptr MSVCP_out_of_range_vtable;
extern const vtable_ptr MSVCP_invalid_argument_vtable;
extern const vtable_ptr MSVCP_runtime_error_vtable;
extern const vtable_ptr MSVCP_failure_vtable;

static void MSVCP_type_info_dtor(type_info * _this)
{
    free(_this->name);
}

/* Unexported */
DEFINE_THISCALL_WRAPPER(MSVCP_type_info_vector_dtor,8)
void * __thiscall MSVCP_type_info_vector_dtor(type_info * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCP_type_info_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCP_type_info_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

DEFINE_RTTI_DATA0( type_info, 0, ".?AVtype_info@@" );

DEFINE_THISCALL_WRAPPER(MSVCP_exception_ctor, 8)
exception* __thiscall MSVCP_exception_ctor(exception *this, const char **name)
{
    TRACE("(%p %s)\n", this, *name);

    this->vtable = &MSVCP_exception_vtable;
    if(*name) {
        unsigned int name_len = strlen(*name) + 1;
        this->name = malloc(name_len);
        memcpy(this->name, *name, name_len);
        this->do_free = TRUE;
    } else {
        this->name = NULL;
        this->do_free = FALSE;
    }
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_copy_ctor,8)
exception* __thiscall MSVCP_exception_copy_ctor(exception *this, const exception *rhs)
{
    TRACE("(%p,%p)\n", this, rhs);

    if(!rhs->do_free) {
        this->vtable = &MSVCP_exception_vtable;
        this->name = rhs->name;
        this->do_free = FALSE;
    } else
        MSVCP_exception_ctor(this, (const char**)&rhs->name);
    TRACE("name = %s\n", this->name);
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_dtor,4)
void __thiscall MSVCP_exception_dtor(exception *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &MSVCP_exception_vtable;
    if(this->do_free)
        free(this->name);
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_vector_dtor, 8)
void * __thiscall MSVCP_exception_vector_dtor(exception *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_exception_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        MSVCP_exception_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_RTTI_DATA0(exception, 0, ".?AVexception@std@@");

static const cxx_type_info exception_cxx_type_info = {
    0,
    &exception_type_info,
    { 0, -1, 0 },
    sizeof(exception),
    (cxx_copy_ctor)THISCALL(MSVCP_exception_dtor)
};

static const cxx_type_info_table exception_cxx_type_table = {
    1,
    {
        &exception_cxx_type_info,
        NULL,
        NULL
    }
};

static const cxx_exception_type exception_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_exception_copy_ctor),
    NULL,
    &exception_cxx_type_table
};

/* bad_alloc class data */
typedef exception bad_alloc;

DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_ctor, 8)
bad_alloc* __thiscall MSVCP_bad_alloc_ctor(bad_alloc *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    MSVCP_exception_ctor(this, name);
    this->vtable = &MSVCP_bad_alloc_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_copy_ctor, 8)
bad_alloc* __thiscall MSVCP_bad_alloc_copy_ctor(bad_alloc *this, const bad_alloc *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_exception_copy_ctor(this, rhs);
    this->vtable = &MSVCP_bad_alloc_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_dtor, 4)
void __thiscall MSVCP_bad_alloc_dtor(bad_alloc *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_vector_dtor, 8)
void * __thiscall MSVCP_bad_alloc_vector_dtor(bad_alloc *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_bad_alloc_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        MSVCP_bad_alloc_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_what_exception,4)
const char* __thiscall MSVCP_what_exception(exception * this)
{
    TRACE("(%p) returning %s\n", this, this->name);
    return this->name ? this->name : "Unknown exception";
}

DEFINE_RTTI_DATA1(bad_alloc, 0, &exception_rtti_base_descriptor, ".?AVbad_alloc@std@@");

static const cxx_type_info bad_alloc_cxx_type_info = {
    0,
    &bad_alloc_type_info,
    { 0, -1, 0 },
    sizeof(bad_alloc),
    (cxx_copy_ctor)THISCALL(MSVCP_bad_alloc_copy_ctor)
};

static const cxx_type_info_table bad_alloc_cxx_type_table = {
    2,
    {
        &bad_alloc_cxx_type_info,
        &exception_cxx_type_info,
        NULL
    }
};

static const cxx_exception_type bad_alloc_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_bad_alloc_dtor),
    NULL,
    &bad_alloc_cxx_type_table
};

/* logic_error class data */
typedef struct _logic_error {
    exception e;
    basic_string_char str;
} logic_error;

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_ctor, 8)
logic_error* __thiscall MSVCP_logic_error_ctor(
        logic_error *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    this->e.vtable = &MSVCP_logic_error_vtable;
    this->e.name = NULL;
    this->e.do_free = FALSE;
    MSVCP_basic_string_char_ctor_cstr(&this->str, *name);
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_copy_ctor, 8)
logic_error* __thiscall MSVCP_logic_error_copy_ctor(
        logic_error *this, logic_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_exception_copy_ctor(&this->e, &rhs->e);
    MSVCP_basic_string_char_copy_ctor(&this->str, &rhs->str);
    this->e.vtable = &MSVCP_logic_error_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_dtor, 4)
void __thiscall MSVCP_logic_error_dtor(logic_error *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(&this->e);
    MSVCP_basic_string_char_dtor(&this->str);
}

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_vector_dtor, 8)
void* __thiscall MSVCP_logic_error_vector_dtor(
        logic_error *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_logic_error_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        MSVCP_logic_error_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_what, 4)
const char* __thiscall MSVCP_logic_error_what(logic_error *this)
{
    TRACE("%p\n", this);
    return MSVCP_basic_string_char_c_str(&this->str);
}

DEFINE_RTTI_DATA1(logic_error, 0, &exception_rtti_base_descriptor, ".?AVlogic_error@std@@");

static const cxx_type_info logic_error_cxx_type_info = {
    0,
    &logic_error_type_info,
    { 0, -1, 0 },
    sizeof(logic_error),
    (cxx_copy_ctor)THISCALL(MSVCP_logic_error_copy_ctor)
};

static const cxx_type_info_table logic_error_cxx_type_table = {
    2,
    {
        &logic_error_cxx_type_info,
        &exception_cxx_type_info,
        NULL
    }
};

static const cxx_exception_type logic_error_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_logic_error_dtor),
    NULL,
    &logic_error_cxx_type_table
};

/* length_error class data */
typedef logic_error length_error;

DEFINE_THISCALL_WRAPPER(MSVCP_length_error_ctor, 8)
length_error* __thiscall MSVCP_length_error_ctor(
        length_error *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &MSVCP_length_error_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_length_error_copy_ctor, 8)
length_error* __thiscall MSVCP_length_error_copy_ctor(
        length_error *this, length_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_logic_error_copy_ctor(this, rhs);
    this->e.vtable = &MSVCP_length_error_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_length_error_vector_dtor, 8)
void* __thiscall MSVCP_length_error_vector_dtor(
        length_error *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    return MSVCP_logic_error_vector_dtor(this, flags);
}

DEFINE_RTTI_DATA2(length_error, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVlength_error@std@@");

static const cxx_type_info length_error_cxx_type_info = {
    0,
    &length_error_type_info,
    { 0, -1, 0 },
    sizeof(length_error),
    (cxx_copy_ctor)THISCALL(MSVCP_length_error_copy_ctor)
};

static const cxx_type_info_table length_error_cxx_type_table = {
    3,
    {
        &length_error_cxx_type_info,
        &logic_error_cxx_type_info,
        &exception_cxx_type_info
    }
};

static const cxx_exception_type length_error_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_logic_error_dtor),
    NULL,
    &length_error_cxx_type_table
};

/* out_of_range class data */
typedef logic_error out_of_range;

DEFINE_THISCALL_WRAPPER(MSVCP_out_of_range_ctor, 8)
out_of_range* __thiscall MSVCP_out_of_range_ctor(
        out_of_range *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &MSVCP_out_of_range_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_out_of_range_copy_ctor, 8)
out_of_range* __thiscall MSVCP_out_of_range_copy_ctor(
        out_of_range *this, out_of_range *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_logic_error_copy_ctor(this, rhs);
    this->e.vtable = &MSVCP_out_of_range_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_out_of_range_vector_dtor, 8)
void* __thiscall MSVCP_out_of_range_vector_dtor(
        out_of_range *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    return MSVCP_logic_error_vector_dtor(this, flags);
}

DEFINE_RTTI_DATA2(out_of_range, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVout_of_range@std@@");

static const cxx_type_info out_of_range_cxx_type_info = {
    0,
    &out_of_range_type_info,
    { 0, -1, 0 },
    sizeof(out_of_range),
    (cxx_copy_ctor)THISCALL(MSVCP_out_of_range_copy_ctor)
};

static const cxx_type_info_table out_of_range_cxx_type_table = {
    3,
    {
        &out_of_range_cxx_type_info,
        &logic_error_cxx_type_info,
        &exception_cxx_type_info
    }
};

static const cxx_exception_type out_of_range_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_logic_error_dtor),
    NULL,
    &out_of_range_cxx_type_table
};

/* invalid_argument class data */
typedef logic_error invalid_argument;

DEFINE_THISCALL_WRAPPER(MSVCP_invalid_argument_ctor, 8)
invalid_argument* __thiscall MSVCP_invalid_argument_ctor(
        invalid_argument *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &MSVCP_invalid_argument_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_invalid_argument_copy_ctor, 8)
invalid_argument* __thiscall MSVCP_invalid_argument_copy_ctor(
        invalid_argument *this, invalid_argument *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_logic_error_copy_ctor(this, rhs);
    this->e.vtable = &MSVCP_invalid_argument_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_invalid_argument_vector_dtor, 8)
void* __thiscall MSVCP_invalid_argument_vector_dtor(
        invalid_argument *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    return MSVCP_logic_error_vector_dtor(this, flags);
}

DEFINE_RTTI_DATA2(invalid_argument, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVinvalid_argument@std@@");

static const cxx_type_info invalid_argument_cxx_type_info = {
    0,
    &invalid_argument_type_info,
    { 0, -1, 0 },
    sizeof(invalid_argument),
    (cxx_copy_ctor)THISCALL(MSVCP_invalid_argument_copy_ctor)
};

static const cxx_type_info_table invalid_argument_cxx_type_table = {
    3,
    {
        &invalid_argument_cxx_type_info,
        &logic_error_cxx_type_info,
        &exception_cxx_type_info
    }
};

static const cxx_exception_type invalid_argument_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_logic_error_dtor),
    NULL,
    &invalid_argument_cxx_type_table
};

/* runtime_error class data */
typedef struct {
    exception e;
    basic_string_char str;
} runtime_error;

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_ctor, 8)
runtime_error* __thiscall MSVCP_runtime_error_ctor(
        runtime_error *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    this->e.vtable = &MSVCP_runtime_error_vtable;
    this->e.name = NULL;
    this->e.do_free = FALSE;
    MSVCP_basic_string_char_ctor_cstr(&this->str, *name);
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_copy_ctor, 8)
runtime_error* __thiscall MSVCP_runtime_error_copy_ctor(
        runtime_error *this, runtime_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_exception_copy_ctor(&this->e, &rhs->e);
    MSVCP_basic_string_char_copy_ctor(&this->str, &rhs->str);
    this->e.vtable = &MSVCP_runtime_error_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_dtor, 4)
void __thiscall MSVCP_runtime_error_dtor(runtime_error *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(&this->e);
    MSVCP_basic_string_char_dtor(&this->str);
}

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_vector_dtor, 8)
void* __thiscall MSVCP_runtime_error_vector_dtor(
        runtime_error *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_runtime_error_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        MSVCP_runtime_error_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_what, 4)
const char* __thiscall MSVCP_runtime_error_what(runtime_error *this)
{
    TRACE("%p\n", this);
    return MSVCP_basic_string_char_c_str(&this->str);
}

DEFINE_RTTI_DATA1(runtime_error, 0, &exception_rtti_base_descriptor, ".?AVruntime_error@std@@");

static const cxx_type_info runtime_error_cxx_type_info = {
    0,
    &runtime_error_type_info,
    { 0, -1, 0 },
    sizeof(runtime_error),
    (cxx_copy_ctor)THISCALL(MSVCP_runtime_error_copy_ctor)
};

static const cxx_type_info_table runtime_error_cxx_type_table = {
    2,
    {
        &runtime_error_cxx_type_info,
        &exception_cxx_type_info,
        NULL
    }
};

static const cxx_exception_type runtime_error_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_runtime_error_dtor),
    NULL,
    &runtime_error_cxx_type_table
};

/* failure class data */
typedef runtime_error failure;

DEFINE_THISCALL_WRAPPER(MSVCP_failure_ctor, 8)
failure* __thiscall MSVCP_failure_ctor(
        failure *this, const char **name)
{
    TRACE("%p %s\n", this, *name);
    MSVCP_runtime_error_ctor(this, name);
    this->e.vtable = &MSVCP_failure_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_copy_ctor, 8)
failure* __thiscall MSVCP_failure_copy_ctor(
        failure *this, failure *rhs)
{
    TRACE("%p %p\n", this, rhs);
    MSVCP_runtime_error_copy_ctor(this, rhs);
    this->e.vtable = &MSVCP_failure_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_dtor, 4)
void __thiscall MSVCP_failure_dtor(failure *this)
{
    TRACE("%p\n", this);
    MSVCP_runtime_error_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_vector_dtor, 8)
void* __thiscall MSVCP_failure_vector_dtor(
        failure *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    return MSVCP_runtime_error_vector_dtor(this, flags);
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_what, 4)
const char* __thiscall MSVCP_failure_what(failure *this)
{
    TRACE("%p\n", this);
    return MSVCP_runtime_error_what(this);
}

DEFINE_RTTI_DATA2(failure, 0, &runtime_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVfailure@std@@");

static const cxx_type_info failure_cxx_type_info = {
    0,
    &failure_type_info,
    { 0, -1, 0 },
    sizeof(failure),
    (cxx_copy_ctor)THISCALL(MSVCP_failure_copy_ctor)
};

static const cxx_type_info_table failure_cxx_type_table = {
    3,
    {
        &failure_cxx_type_info,
        &runtime_error_cxx_type_info,
        &exception_cxx_type_info
    }
};

static const cxx_exception_type failure_cxx_type = {
    0,
    (cxx_copy_ctor)THISCALL(MSVCP_failure_dtor),
    NULL,
    &failure_cxx_type_table
};

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(type_info, "");
    __ASM_VTABLE(exception, VTABLE_ADD_FUNC(MSVCP_what_exception));
    __ASM_VTABLE(bad_alloc, VTABLE_ADD_FUNC(MSVCP_what_exception));
    __ASM_VTABLE(logic_error, VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    __ASM_VTABLE(length_error, VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    __ASM_VTABLE(out_of_range, VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    __ASM_VTABLE(invalid_argument, VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    __ASM_VTABLE(runtime_error, VTABLE_ADD_FUNC(MSVCP_runtime_error_what));
    __ASM_VTABLE(failure, VTABLE_ADD_FUNC(MSVCP_failure_what));
#ifndef __GNUC__
}
#endif

/* Internal: throws selected exception */
void throw_exception(exception_type et, const char *str)
{
    const char *addr = str;

    switch(et) {
    case EXCEPTION_RERAISE:
        _CxxThrowException(NULL, NULL);
    case EXCEPTION: {
        exception e;
        MSVCP_exception_ctor(&e, &addr);
        _CxxThrowException(&e, &exception_cxx_type);
    }
    case EXCEPTION_BAD_ALLOC: {
        bad_alloc e;
        MSVCP_bad_alloc_ctor(&e, &addr);
        _CxxThrowException(&e, &bad_alloc_cxx_type);
    }
    case EXCEPTION_LOGIC_ERROR: {
        logic_error e;
        MSVCP_logic_error_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &logic_error_cxx_type);
    }
    case EXCEPTION_LENGTH_ERROR: {
        length_error e;
        MSVCP_length_error_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &length_error_cxx_type);
    }
    case EXCEPTION_OUT_OF_RANGE: {
        out_of_range e;
        MSVCP_out_of_range_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &out_of_range_cxx_type);
    }
    case EXCEPTION_INVALID_ARGUMENT: {
        invalid_argument e;
        MSVCP_invalid_argument_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &invalid_argument_cxx_type);
    }
    case EXCEPTION_RUNTIME_ERROR: {
        runtime_error e;
        MSVCP_runtime_error_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &runtime_error_cxx_type);
    }
    case EXCEPTION_FAILURE: {
        failure e;
        MSVCP_failure_ctor(&e, &addr);
        _CxxThrowException((exception*)&e, &failure_cxx_type);
    }
    }
}
