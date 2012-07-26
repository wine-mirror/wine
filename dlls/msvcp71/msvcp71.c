/*
 * msvcp71 specific functions
 *
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

#include "windef.h"
#include "winbase.h"

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
#else /* __i386__ */

#define THISCALL(func) func
#define THISCALL_NAME(func) __ASM_NAME(#func)
#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

/* Copied from dlls/msvcp90/msvcp90.h */
typedef SIZE_T MSVCP_size_t;
typedef unsigned short MSVCP_wchar_t;

#define BUF_SIZE_CHAR 16
typedef struct
{
    void *allocator;
    union {
        char buf[BUF_SIZE_CHAR];
        char *ptr;
    } data;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_char;

#define BUF_SIZE_WCHAR 8
typedef struct
{
    void *allocator;
    union {
        MSVCP_wchar_t buf[BUF_SIZE_WCHAR];
        MSVCP_wchar_t *ptr;
    } data;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

typedef struct {
    const char *pos;
} basic_string_char_iterator;

typedef struct {
    const MSVCP_wchar_t *pos;
} basic_string_wchar_iterator;

static char* basic_string_char_ptr(basic_string_char *this)
{
    if(this->res == BUF_SIZE_CHAR-1)
        return this->data.buf;
    return this->data.ptr;
}

static MSVCP_wchar_t* basic_string_wchar_ptr(basic_string_wchar *this)
{
    if(this->res == BUF_SIZE_WCHAR-1)
        return this->data.buf;
    return this->data.ptr;
}

/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_begin, 8)
basic_string_char_iterator* __thiscall basic_string_char_begin(
        basic_string_char *this, basic_string_char_iterator *ret)
{
    ret->pos = basic_string_char_ptr(this);
    return ret;
}

/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_end, 8)
basic_string_char_iterator* __thiscall basic_string_char_end(
        basic_string_char *this, basic_string_char_iterator *ret)
{
    ret->pos = basic_string_char_ptr(this)+this->size;
    return ret;
}


/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_begin, 8)
basic_string_wchar_iterator* __thiscall basic_string_wchar_begin(
        basic_string_wchar *this, basic_string_wchar_iterator *ret)
{
    ret->pos = basic_string_wchar_ptr(this);
    return ret;
}

/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_end, 8)
basic_string_wchar_iterator* __thiscall basic_string_wchar_end(
        basic_string_wchar *this, basic_string_wchar_iterator *ret)
{
    ret->pos = basic_string_wchar_ptr(this)+this->size;
    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
    }
    return TRUE;
}
