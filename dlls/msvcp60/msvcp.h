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
#include "cxx.h"

typedef unsigned char MSVCP_bool;
typedef SIZE_T MSVCP_size_t;
typedef SIZE_T streamoff;
typedef SIZE_T streamsize;

void __cdecl _invalid_parameter(const wchar_t*, const wchar_t*,
        const wchar_t*, unsigned int, uintptr_t);

extern void* (__cdecl *MSVCRT_operator_new)(MSVCP_size_t);
extern void (__cdecl *MSVCRT_operator_delete)(void*);
extern void* (__cdecl *MSVCRT_set_new_handler)(void*);

/* basic_string<char, char_traits<char>, allocator<char>> */
typedef struct
{
    void *allocator;
    char *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_char;

basic_string_char* __stdcall MSVCP_basic_string_char_ctor_cstr(basic_string_char*, const char*);
basic_string_char* __stdcall MSVCP_basic_string_char_copy_ctor(basic_string_char*, const basic_string_char*);
void __stdcall MSVCP_basic_string_char_dtor(basic_string_char*);
const char* __stdcall MSVCP_basic_string_char_c_str(const basic_string_char*);

typedef struct
{
    void *allocator;
    wchar_t *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

char* __stdcall MSVCP_allocator_char_allocate(void*, MSVCP_size_t);
void __stdcall MSVCP_allocator_char_deallocate(void*, char*, MSVCP_size_t);
MSVCP_size_t __stdcall MSVCP_allocator_char_max_size(void*);
wchar_t* __stdcall MSVCP_allocator_wchar_allocate(void*, MSVCP_size_t);
void __stdcall MSVCP_allocator_wchar_deallocate(void*, wchar_t*, MSVCP_size_t);
MSVCP_size_t __stdcall MSVCP_allocator_wchar_max_size(void*);

/* class locale */
typedef struct
{
    struct _locale__Locimp *ptr;
} locale;

locale* __thiscall locale_ctor(locale*);
void __thiscall locale_dtor(locale*);

/* class _Lockit */
typedef struct {
    char empty_struct;
} _Lockit;

#define _LOCK_LOCALE 0
#define _LOCK_MALLOC 1
#define _LOCK_STREAM 2
#define _LOCK_DEBUG 3
#define _MAX_LOCK 4

void init_lockit(void);
void free_lockit(void);
void __thiscall _Lockit_dtor(_Lockit*);

/* class mutex */
typedef struct {
        void *mutex;
} mutex;

mutex* __thiscall mutex_ctor(mutex*);
void __thiscall mutex_dtor(mutex*);
void __thiscall mutex_lock(mutex*);
void __thiscall mutex_unlock(mutex*);
