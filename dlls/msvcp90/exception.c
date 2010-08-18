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
typedef struct __type_info
{
    const vtable_ptr *vtable;
    char              *name;        /* Unmangled name, allocated lazily */
    char               mangled[32]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

typedef void (*cxx_copy_ctor)(void);

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

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

void CDECL _CxxThrowException(exception*,const cxx_exception_type*);

/* exception class data */
static type_info exception_type_info = {
    NULL, /* set by set_exception_vtable */
    NULL,
    ".?AVexception@std@@"
};

DEFINE_THISCALL_WRAPPER(MSVCP_exception_ctor, 8)
exception* __stdcall MSVCP_exception_ctor(exception *this, const char **name)
{
    TRACE("(%p %s)\n", this, *name);

    this->vtable = exception_type_info.vtable;
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
exception* __stdcall MSVCP_exception_copy_ctor(exception *this, const exception *rhs)
{
    TRACE("(%p,%p)\n", this, rhs);

    if(!rhs->do_free) {
        this->vtable = exception_type_info.vtable;
        this->name = rhs->name;
        this->do_free = FALSE;
    } else
        MSVCP_exception_ctor(this, (const char**)&rhs->name);
    TRACE("name = %s\n", this->name);
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_dtor,4)
void __stdcall MSVCP_exception_dtor(exception *this)
{
    TRACE("(%p)\n", this);
    this->vtable = exception_type_info.vtable;
    if(this->do_free)
        free(this->name);
}

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

void set_exception_vtable(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");
    exception_type_info.vtable = (void*)GetProcAddress(hmod, "??_7exception@@6B@");
}

/* Internal: throws selected exception */
void throw_exception(exception_type et, const char *str)
{
    const char *addr = str;

    switch(et) {
    case EXCEPTION: {
        exception e;
        MSVCP_exception_ctor(&e, &addr);
        _CxxThrowException(&e, &exception_cxx_type);
        return;
    }
    }
}
