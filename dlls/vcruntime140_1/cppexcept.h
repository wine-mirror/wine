/*
 * vcruntime140_1 x86_64 C++ exception handling
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2020 Piotr Caban
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

typedef void (*vtable_ptr)(void);

/* type_info object, see cpp.c for implementation */
typedef struct __type_info
{
  const vtable_ptr *vtable;
  char              *name;        /* Unmangled name, allocated lazily */
  char               mangled[64]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

/* complete information about a C++ type */
typedef struct __cxx_type_info
{
    UINT flags;
    unsigned int type_info;
    this_ptr_offsets offsets;
    unsigned int size;
    unsigned int copy_ctor;
} cxx_type_info;

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
typedef struct __cxx_type_info_table
{
    UINT count;
    unsigned int info[3];
} cxx_type_info_table;

struct __cxx_exception_frame;
struct __cxx_function_descr;

/* type information for an exception object */
typedef struct
{
    UINT flags;
    unsigned int destructor;
    unsigned int custom_handler;
    unsigned int type_info_table;
} cxx_exception_type;

static inline const char *dbgstr_type_info( const type_info *info )
{
    if (!info) return "{}";
    return wine_dbg_sprintf( "{vtable=%p name=%s (%s)}",
                             info->vtable, info->mangled, info->name ? info->name : "" );
}

/* compute the this pointer for a base class of a given type */
static inline void *get_this_pointer( const this_ptr_offsets *off, void *object )
{
    if (!object) return NULL;

    if (off->vbase_descr >= 0)
    {
        int *offset_ptr;

        /* move this ptr to vbase descriptor */
        object = (char *)object + off->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)object + off->vbase_offset);
        object = (char *)object + *offset_ptr;
    }

    object = (char *)object + off->this_offset;
    return object;
}

typedef void (__cdecl *_se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);

typedef struct _frame_info
{
    void *object;
    struct _frame_info *next;
} frame_info;

typedef struct
{
    frame_info frame_info;
    EXCEPTION_RECORD *rec;
    CONTEXT *context;
} cxx_frame_info;

BOOL __cdecl __CxxRegisterExceptionObject(EXCEPTION_POINTERS*, cxx_frame_info*);
void __cdecl __CxxUnregisterExceptionObject(cxx_frame_info*, BOOL);
void __cdecl __DestructExceptionObject(EXCEPTION_RECORD *rec);

void** __cdecl __current_exception(void);
int* __cdecl __processing_throw(void);
void __cdecl terminate(void);
