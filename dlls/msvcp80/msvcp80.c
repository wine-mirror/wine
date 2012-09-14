/*
 * msvcp80 specific functions
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

typedef SIZE_T MSVCP_size_t;
struct locale_facet;
struct locale;

/* basic_string<char, char_traits<char>, allocator<char>> */
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
        wchar_t buf[BUF_SIZE_WCHAR];
        wchar_t *ptr;
    } data;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

/* _String_iterator<char> and _String_const_iterator<char> class */
typedef struct {
    void *cont;
    const basic_string_char *bstr;
    const char *pos;
} basic_string_char_reverse_iter;

typedef struct {
    void *cont;
    const basic_string_wchar *bstr;
    const wchar_t *pos;
} basic_string_wchar_reverse_iter;

const struct locale* (CDECL *plocale_classic)(void);
MSVCP_size_t (CDECL *pcollate_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcollate_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcollate_short__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pctype_short__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcodecvt_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcodecvt_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pcodecvt_short__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_char__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_wchar__Getcat)(const struct locale_facet**,const struct locale*);
MSVCP_size_t (CDECL *pnumpunct_short__Getcat)(const struct locale_facet**,const struct locale*);

static BOOL init_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcp90.dll");
    if(!hmod)
        return FALSE;

    if(sizeof(void*) > sizeof(int)) { /* 64-bit has different names */
        plocale_classic = (void*)GetProcAddress(hmod, "?classic@locale@std@@SAAEBV12@XZ");
        pcollate_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcollate_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcollate_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pctype_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcodecvt_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcodecvt_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@_WDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pcodecvt_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@GDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
        pnumpunct_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z");
    }else {
        plocale_classic = (void*)GetProcAddress(hmod, "?classic@locale@std@@SAABV12@XZ");
        pcollate_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcollate_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcollate_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pctype_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcodecvt_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcodecvt_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@_WDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pcodecvt_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$codecvt@GDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_char__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_wchar__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
        pnumpunct_short__Getcat = (void*)GetProcAddress(hmod, "?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z");
    }

    return TRUE;
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

/* ?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_char__Getcat(const struct locale_facet **facet)
{
    return pcollate_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_wchar__Getcat(const struct locale_facet **facet)
{
    return pcollate_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl collate_short__Getcat(const struct locale_facet **facet)
{
    return pcollate_short__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_char__Getcat(const struct locale_facet **facet)
{
    return pctype_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_wchar__Getcat(const struct locale_facet **facet)
{
    return pctype_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl ctype_short__Getcat(const struct locale_facet **facet)
{
    return pctype_short__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl codecvt_char__Getcat(const struct locale_facet **facet)
{
    return pcodecvt_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$codecvt@_WDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@_WDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl codecvt_wchar__Getcat(const struct locale_facet **facet)
{
    return pcodecvt_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$codecvt@GDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@GDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl codecvt_short__Getcat(const struct locale_facet **facet)
{
    return pcodecvt_short__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_char__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_char__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_wchar__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_wchar__Getcat(facet, plocale_classic());
}

/* ?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
MSVCP_size_t __cdecl numpunct_short__Getcat(const struct locale_facet **facet)
{
    return pnumpunct_short__Getcat(facet, plocale_classic());
}

static const char* basic_string_char_const_ptr(const basic_string_char *this)
{
    if(this->res == BUF_SIZE_CHAR-1)
        return this->data.buf;
    return this->data.ptr;
}

static const wchar_t* basic_string_wchar_const_ptr(const basic_string_wchar *this)
{
        if(this->res == BUF_SIZE_WCHAR-1)
            return this->data.buf;
        return this->data.ptr;
}

/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_rbegin, 8)
basic_string_char_reverse_iter* __thiscall basic_string_char_rbegin(const basic_string_char *this, basic_string_char_reverse_iter *ret)
{
    ret->cont = NULL;
    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this)+this->size;
    return ret;
}

/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_rbegin, 8)
basic_string_wchar_reverse_iter* __thiscall basic_string_wchar_rbegin(const basic_string_wchar *this, basic_string_wchar_reverse_iter *ret)
{
    ret->cont = NULL;
    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this)+this->size;
    return ret;
}

/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_rend, 8)
basic_string_char_reverse_iter* __thiscall basic_string_char_rend(const basic_string_char *this, basic_string_char_reverse_iter *ret)
{
    ret->cont = NULL;
    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this);
    return ret;
}

/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_rend, 8)
basic_string_wchar_reverse_iter* __thiscall basic_string_wchar_rend(const basic_string_wchar *this, basic_string_wchar_reverse_iter *ret)
{
    ret->cont = NULL;
    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this);
    return ret;
}
