/*
 * msvcrt C++ exception handling
 *
 * Copyright 2002 Alexandre Julliard
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

#ifndef __MSVCRT_CPPEXCEPT_H
#define __MSVCRT_CPPEXCEPT_H

#include "cxx.h"

#define CXX_FRAME_MAGIC_VC6 0x19930520
#define CXX_FRAME_MAGIC_VC7 0x19930521
#define CXX_FRAME_MAGIC_VC8 0x19930522
#define CXX_EXCEPTION       0xe06d7363

#define FUNC_DESCR_SYNCHRONOUS  1 /* synchronous exceptions only (built with /EHs and /EHsc) */
#define FUNC_DESCR_NOEXCEPT     4 /* noexcept function */

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

struct __cxx_exception_frame;
struct __cxx_function_descr;

typedef DWORD (*cxx_exc_custom_handler)( PEXCEPTION_RECORD, struct __cxx_exception_frame*,
                                         PCONTEXT, EXCEPTION_REGISTRATION_RECORD**,
                                         const struct __cxx_function_descr*, int nested_trylevel,
                                         EXCEPTION_REGISTRATION_RECORD *nested_frame, DWORD unknown3 );

void WINAPI _CxxThrowException(void*,const cxx_exception_type*);
int CDECL _XcptFilter(NTSTATUS, PEXCEPTION_POINTERS);

typedef struct
{
    EXCEPTION_RECORD *rec;
    LONG *ref; /* not binary compatible with native msvcr100 */
} exception_ptr;

void throw_exception(const char*);
void exception_ptr_from_record(exception_ptr*,EXCEPTION_RECORD*);

void __cdecl __ExceptionPtrCreate(exception_ptr*);
void __cdecl __ExceptionPtrDestroy(exception_ptr*);
void __cdecl __ExceptionPtrRethrow(const exception_ptr*);

BOOL __cdecl __uncaught_exception(void);

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

#if _MSVCR_VER >= 80
#define EXCEPTION_MANGLED_NAME ".?AVexception@std@@"
#else
#define EXCEPTION_MANGLED_NAME ".?AVexception@@"
#endif

#define CREATE_EXCEPTION_OBJECT(exception_name) \
static exception* __exception_ctor(exception *this, const char *str, const vtable_ptr *vtbl) \
{ \
    if (str) \
    { \
        unsigned int len = strlen(str) + 1; \
        this->name = malloc(len); \
        memcpy(this->name, str, len); \
        this->do_free = TRUE; \
    } \
    else \
    { \
        this->name = NULL; \
        this->do_free = FALSE; \
    } \
    this->vtable = vtbl; \
    return this; \
} \
\
static exception* __exception_copy_ctor(exception *this, const exception *rhs, const vtable_ptr *vtbl) \
{ \
    if (rhs->do_free) \
    { \
        __exception_ctor(this, rhs->name, vtbl); \
    } \
    else \
    { \
        *this = *rhs; \
        this->vtable = vtbl; \
    } \
    return this; \
} \
extern const vtable_ptr exception_name ## _vtable; \
DEFINE_THISCALL_WRAPPER(exception_name ## _copy_ctor,8) \
exception* __thiscall exception_name ## _copy_ctor(exception *this, const exception *rhs) \
{ \
    return __exception_copy_ctor(this, rhs, & exception_name ## _vtable); \
} \
\
DEFINE_THISCALL_WRAPPER(exception_name ## _dtor,4) \
void __thiscall exception_name ## _dtor(exception *this) \
{ \
    if (this->do_free) free(this->name); \
} \
\
DEFINE_THISCALL_WRAPPER(exception_name ## _vector_dtor,8) \
void* __thiscall exception_name ## _vector_dtor(exception *this, unsigned int flags) \
{ \
    if (flags & 2) \
    { \
        INT_PTR i, *ptr = (INT_PTR *)this - 1; \
\
        for (i = *ptr - 1; i >= 0; i--) exception_name ## _dtor(this + i); \
        operator_delete(ptr); \
    } \
    else \
    { \
        exception_name ## _dtor(this); \
        if (flags & 1) operator_delete(this); \
    } \
    return this; \
} \
\
DEFINE_THISCALL_WRAPPER(exception_name ## _what,4) \
const char* __thiscall exception_name ## _what(exception *this) \
{ \
    return this->name ? this->name : "Unknown exception"; \
} \
\
__ASM_BLOCK_BEGIN(exception_name ## _vtables) \
__ASM_VTABLE(exception_name, \
        VTABLE_ADD_FUNC(exception_name ## _vector_dtor) \
        VTABLE_ADD_FUNC(exception_name ## _what)); \
__ASM_BLOCK_END \
\
DEFINE_RTTI_DATA0(exception_name, 0, EXCEPTION_MANGLED_NAME)

#endif /* __MSVCRT_CPPEXCEPT_H */
