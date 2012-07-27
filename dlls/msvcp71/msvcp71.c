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
#include "wine/unicode.h"

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

basic_string_char* (__stdcall *pbasic_string_char_replace)(basic_string_char*,
        MSVCP_size_t, MSVCP_size_t, const char*, MSVCP_size_t);
basic_string_wchar* (__stdcall *pbasic_string_wchar_replace)(basic_string_wchar*,
        MSVCP_size_t, MSVCP_size_t, const MSVCP_wchar_t*, MSVCP_size_t);
void (__cdecl *p_String_base_Xlen)(void);
void (__cdecl *p_String_base_Xran)(void);

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

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_bstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_bstr(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, basic_string_char *str)
{
    return pbasic_string_char_replace(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, basic_string_char_ptr(str), str->size);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0ID@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_ch, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_ch(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, MSVCP_size_t count, char ch)
{
    /* TODO: add more efficient implementation */
    MSVCP_size_t off = beg.pos-basic_string_char_ptr(this);

    pbasic_string_char_replace(this, off, end.pos-beg.pos, NULL, 0);
    while(count--)
        pbasic_string_char_replace(this, off, 0, &ch, 1);
    return this;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_beg_end, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_beg_end(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *rbeg, const char *rend)
{
    return pbasic_string_char_replace(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, rbeg, rend-rbeg);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *str)
{
    return pbasic_string_char_replace(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, str, strlen(str));
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr_len, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr_len(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *str, MSVCP_size_t len)
{
    return pbasic_string_char_replace(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, str, len);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_iter, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end,
        basic_string_char_iterator rbeg, basic_string_char_iterator rend)
{
    return pbasic_string_char_replace(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, rbeg.pos, rend.pos-rbeg.pos);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_append_iter, 12)
basic_string_char* __thiscall basic_string_char_append_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end)
{
    return pbasic_string_char_replace(this, this->size, 0, beg.pos, end.pos-beg.pos);
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

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_bstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_bstr(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, basic_string_wchar *str)
{
    return pbasic_string_wchar_replace(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, basic_string_wchar_ptr(str), str->size);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0IG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0I_W@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_ch, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_ch(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, MSVCP_size_t count, MSVCP_wchar_t ch)
{
    /* TODO: add more efficient implementation */
    MSVCP_size_t off = beg.pos-basic_string_wchar_ptr(this);

    pbasic_string_wchar_replace(this, off, end.pos-beg.pos, NULL, 0);
    while(count--)
        pbasic_string_wchar_replace(this, off, 0, &ch, 1);
    return this;
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBG1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_W1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_beg_end, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_beg_end(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        const MSVCP_wchar_t *rbeg, const MSVCP_wchar_t *rend)
{
    return pbasic_string_wchar_replace(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, rbeg, rend-rbeg);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_W@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, const MSVCP_wchar_t *str)
{
    return pbasic_string_wchar_replace(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, str, strlenW(str));
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBGI@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr_len, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr_len(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        const MSVCP_wchar_t *str, MSVCP_size_t len)
{
    return pbasic_string_wchar_replace(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, str, len);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_iter, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        basic_string_wchar_iterator rbeg, basic_string_wchar_iterator rend)
{
    return pbasic_string_wchar_replace(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, rbeg.pos, rend.pos-rbeg.pos);
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_append_iter, 12)
basic_string_wchar* __thiscall basic_string_wchar_append_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
    return pbasic_string_wchar_replace(this, this->size, 0, beg.pos, end.pos-beg.pos);
}

/* ?_Xlen@_String_base@std@@QBEXXZ */
DEFINE_THISCALL_WRAPPER(_String_base__Xlen, 4)
void __thiscall _String_base__Xlen(const void/*_String_base*/ *this)
{
    p_String_base_Xlen();
}

/* ?_Xran@_String_base@std@@QBEXXZ */
DEFINE_THISCALL_WRAPPER(_String_base__Xran, 4)
void __thiscall _String_base__Xran(const void/*_String_base*/ *this)
{
    p_String_base_Xran();
}

static BOOL init_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcp90.dll");
    if(!hmod)
        return FALSE;

    pbasic_string_char_replace = (void*)GetProcAddress(hmod, "basic_string_char_replace_helper");
    pbasic_string_wchar_replace = (void*)GetProcAddress(hmod, "basic_string_wchar_replace_helper");

    p_String_base_Xlen = (void*)GetProcAddress(hmod, "?_Xlen@_String_base@std@@SAXXZ");
    p_String_base_Xran = (void*)GetProcAddress(hmod, "?_Xran@_String_base@std@@SAXXZ");

    return pbasic_string_char_replace && pbasic_string_wchar_replace;
}

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            if(!init_funcs())
                return FALSE;
    }
    return TRUE;
}
