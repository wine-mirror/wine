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


#include <stdarg.h>

#include "msvcp.h"
#include "stdio.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#define FROZEN '\xff'
#define FROZENW L'\xff'

/* _String_iterator<char> and _String_const_iterator<char> class */
typedef struct {
    char *ptr;
} String_reverse_iterator_char;

typedef struct {
    wchar_t *ptr;
} String_reverse_iterator_wchar;

/* allocator class */
typedef struct {
    char empty_struct;
} allocator;

/* ?_Xran@std@@YAXXZ */
void __cdecl _Xran(void)
{
    TRACE("\n");
    _Xout_of_range("invalid string position");
}

/* ?_Xlen@std@@YAXXZ */
void __cdecl _Xlen(void)
{
    TRACE("\n");
    _Xlength_error("string too long");
}

/* ?compare@?$char_traits@D@std@@SAHPBD0I@Z */
/* ?compare@?$char_traits@D@std@@SAHPEBD0_K@Z */
int CDECL MSVCP_char_traits_char_compare(
        const char *s1, const char *s2, size_t count)
{
    int ret = memcmp(s1, s2, count);
    return (ret>0 ? 1 : (ret<0 ? -1 : 0));
}

/* ?length@?$char_traits@D@std@@SAIPBD@Z */
/* ?length@?$char_traits@D@std@@SA_KPEBD@Z */
size_t CDECL MSVCP_char_traits_char_length(const char *str)
{
    return strlen(str);
}

/* ?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Copy_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
char* CDECL MSVCP_char_traits_char__Copy_s(char *dest,
        size_t size, const char *src, size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memcpy(dest, src, count);
}

/* ?copy@?$char_traits@D@std@@SAPADPADPBDI@Z */
/* ?copy@?$char_traits@D@std@@SAPEADPEADPEBD_K@Z */
char* CDECL MSVCP_char_traits_char_copy(
        char *dest, const char *src, size_t count)
{
    return MSVCP_char_traits_char__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@D@std@@SAPBDPBDIABD@Z */
/* ?find@?$char_traits@D@std@@SAPEBDPEBD_KAEBD@Z */
const char * CDECL MSVCP_char_traits_char_find(
        const char *str, size_t range, const char *c)
{
    return memchr(str, *c, range);
}

/* ?_Move_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Move_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
char* CDECL MSVCP_char_traits_char__Move_s(char *dest,
        size_t size, const char *src, size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memmove(dest, src, count);
}

/* ?move@?$char_traits@D@std@@SAPADPADPBDI@Z */
/* ?move@?$char_traits@D@std@@SAPEADPEADPEBD_K@Z */
char* CDECL MSVCP_char_traits_char_move(
        char *dest, const char *src, size_t count)
{
    return MSVCP_char_traits_char__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@D@std@@SAPADPADID@Z */
/* ?assign@?$char_traits@D@std@@SAPEADPEAD_KD@Z */
char* CDECL MSVCP_char_traits_char_assignn(char *str, size_t num, char c)
{
    return memset(str, c, num);
}

/* ?compare@?$char_traits@_W@std@@SAHPB_W0I@Z */
/* ?compare@?$char_traits@_W@std@@SAHPEB_W0_K@Z */
int CDECL MSVCP_char_traits_wchar_compare(const wchar_t *s1,
        const wchar_t *s2, size_t count)
{
    size_t i;
    int ret = 0;

    for (i = 0; i < count && !ret; i++) ret = s1[i] - s2[i];
    return (ret>0 ? 1 : (ret<0 ? -1 : 0));
}

/* ?length@?$char_traits@_W@std@@SAIPB_W@Z */
/* ?length@?$char_traits@_W@std@@SA_KPEB_W@Z */
size_t CDECL MSVCP_char_traits_wchar_length(const wchar_t *str)
{
    return wcslen((WCHAR*)str);
}

/* ?_Copy_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Copy_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
wchar_t* CDECL MSVCP_char_traits_wchar__Copy_s(wchar_t *dest,
        size_t size, const wchar_t *src, size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memcpy(dest, src, count * sizeof(wchar_t));
}

/* ?copy@?$char_traits@_W@std@@SAPA_WPA_WPB_WI@Z */
/* ?copy@?$char_traits@_W@std@@SAPEA_WPEA_WPEB_W_K@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_copy(wchar_t *dest,
        const wchar_t *src, size_t count)
{
    return MSVCP_char_traits_wchar__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@_W@std@@SAPB_WPB_WIAB_W@Z */
/* ?find@?$char_traits@_W@std@@SAPEB_WPEB_W_KAEB_W@Z */
const wchar_t* CDECL MSVCP_char_traits_wchar_find(
        const wchar_t *str, size_t range, const wchar_t *c)
{
    size_t i=0;

    for(i=0; i<range; i++)
        if(str[i] == *c)
            return str+i;

    return NULL;
}

/* ?_Move_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Move_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
wchar_t* CDECL MSVCP_char_traits_wchar__Move_s(wchar_t *dest,
        size_t size, const wchar_t *src, size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memmove(dest, src, count * sizeof(WCHAR));
}

/* ?move@?$char_traits@_W@std@@SAPA_WPA_WPB_WI@Z */
/* ?move@?$char_traits@_W@std@@SAPEA_WPEA_WPEB_W_K@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_move(wchar_t *dest,
        const wchar_t *src, size_t count)
{
    return MSVCP_char_traits_wchar__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@_W@std@@SAPA_WPA_WI_W@Z */
/* ?assign@?$char_traits@_W@std@@SAPEA_WPEA_W_K_W@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_assignn(wchar_t *str,
        size_t num, wchar_t c)
{
    size_t i;

    for(i=0; i<num; i++)
        str[i] = c;

    return str;
}

static bool basic_string_char_inside(
        basic_string_char *this, const char *ptr)
{
    return ptr>=this->ptr && ptr<this->ptr+this->size;
}

/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2IB */
/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2_KB */
const size_t MSVCP_basic_string_char_npos = -1;

/* ?_C@?1??_Nullstr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPBDXZ@4DB */
/* ?_C@?1??_Nullstr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPEBDXZ@4DB */
const char basic_string_char_nullbyte = '\0';

/* ?_Nullstr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPBDXZ */
/* ?_Nullstr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPEBDXZ */
const char* __cdecl basic_string_char__Nullstr(void)
{
    return &basic_string_char_nullbyte;
}

/* ?_Refcnt@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEAAEPBD@Z */
/* ?_Refcnt@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAAEAEPEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Refcnt, 8)
unsigned char* __thiscall basic_string_char__Refcnt(basic_string_char *this, const char *ptr)
{
    TRACE("(%p %p)\n", this, ptr);
    return (unsigned char*)ptr-1;
}

/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXI@Z */
/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Eos, 8)
void __thiscall basic_string_char__Eos(basic_string_char *this, size_t len)
{
    this->size = len;
    this->ptr[len] = 0;
}

/* ?clear@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_clear, 4)
void __thiscall MSVCP_basic_string_char_clear(basic_string_char *this)
{
    if(this->ptr)
        basic_string_char__Eos(this, 0);
}

/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEX_N@Z */
/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Tidy, 8)
void __thiscall basic_string_char__Tidy(basic_string_char *this, bool built)
{
    TRACE("(%p %d)\n", this, built);

    if(!built || !this->ptr);
    else if(!this->ptr[-1] || this->ptr[-1]==FROZEN)
        MSVCP_allocator_char_deallocate(NULL, this->ptr-1, this->res+2);
    else
        this->ptr[-1]--;

    memset(this, 0, sizeof(*this));
}

/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAE_NI_N@Z */
/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAA_N_K_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Grow, 12)
bool __thiscall basic_string_char__Grow(basic_string_char *this, size_t new_size, bool trim)
{
    if(!new_size) {
        if(trim)
            basic_string_char__Tidy(this, TRUE);
        else if(this->ptr)
            basic_string_char__Eos(this, 0);
    } else if(this->res<new_size || trim ||
            (this->ptr && this->ptr[-1] && this->ptr[-1]!=FROZEN)) {
        size_t new_res = new_size, len = this->size;
        char *ptr;

        if(!trim && this->ptr && !this->ptr[-1]) {
            new_res |= 0xf;
            if(new_res/3 < this->res/2)
                new_res = this->res + this->res/2;
        }

        ptr = MSVCP_allocator_char_allocate(this->allocator, new_res+2);
        if(!ptr) {
            new_res = new_size;
            ptr = MSVCP_allocator_char_allocate(this->allocator, new_size+2);
        }
        if(!ptr) {
            ERR("Out of memory\n");
            return FALSE;
        }

        if(len > new_res)
            len = new_res;

        *ptr = 0;
        if(this->ptr)
            MSVCP_char_traits_char__Copy_s(ptr+1, new_size, this->ptr, len);
        basic_string_char__Tidy(this, TRUE);
        this->ptr = ptr+1;
        this->res = new_res;
        basic_string_char__Eos(this, len);
    }

    return new_size>0;
}

/* ?_Split@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXXZ */
/* ?_Split@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char__Split, 4)
void __thiscall basic_string_char__Split(basic_string_char *this)
{
    size_t len;
    char *ptr;

    TRACE("(%p)\n", this);

    if(!this->ptr || !this->ptr[-1] || this->ptr[-1]==FROZEN)
        return;

    ptr = this->ptr;
    len = this->size;
    basic_string_char__Tidy(this, TRUE);
    if(basic_string_char__Grow(this, len, FALSE)) {
        if(ptr)
            MSVCP_char_traits_char__Copy_s(this->ptr, this->res, ptr, len);
        basic_string_char__Eos(this, len);
    }
}

/* ?_Freeze@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXXZ */
/* ?_Freeze@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char__Freeze, 4)
void __thiscall basic_string_char__Freeze(basic_string_char *this)
{
    TRACE("(%p)\n", this);
    basic_string_char__Split(this);
    if(this->ptr)
        this->ptr[-1] = FROZEN;
}

/* ?_Copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXI@Z */
/* ?_Copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Copy, 8)
void __thiscall basic_string_char__Copy(basic_string_char *this, size_t copy_len)
{
    TRACE("%p %Iu\n", this, copy_len);

    if(!basic_string_char__Grow(this, copy_len, TRUE))
        return;
}

/* ?_Psum@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPADPADI@Z */
/* ?_Psum@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPEADPEAD_K@Z */
/* ?_Psum@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPBDPBDI@Z */
/* ?_Psum@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAPEBDPEBD_K@Z */
char* __cdecl basic_string_char__Psum(char *iter, size_t add)
{
    TRACE("(%p %Iu)\n", iter, add);
    return iter ? iter+add : iter;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_erase, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_erase(
        basic_string_char *this, size_t pos, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, pos, len);

    if(pos > this->size)
        _Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        basic_string_char__Split(this);
        MSVCP_char_traits_char__Move_s(this->ptr+pos, this->res-pos,
                this->ptr+pos+len, this->size-pos-len);
        basic_string_char__Eos(this, this->size-len);
    }

    return this;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEPADPAD@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAPEADPEAD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_erase_beg, 8)
char* __thiscall basic_string_char_erase_beg(basic_string_char *this, char *beg)
{
    size_t pos = beg-this->ptr;
    MSVCP_basic_string_char_erase(this, pos, 1);
    return this->ptr+pos;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_substr(
        basic_string_char *this, const basic_string_char *assign,
        size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    if(assign->size < pos)
        _Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_char_erase(this, pos+len, MSVCP_basic_string_char_npos);
        MSVCP_basic_string_char_erase(this, 0, pos);
    } else if(basic_string_char__Grow(this, len, FALSE)) {
        if(assign->ptr)
            MSVCP_char_traits_char__Copy_s(this->ptr, this->res, assign->ptr+pos, len);
        basic_string_char__Eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_cstr_len(
        basic_string_char *this, const char *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(str, len), len);

    if(basic_string_char_inside(this, str))
        return MSVCP_basic_string_char_assign_substr(this, this, str-this->ptr, len);
    else if(basic_string_char__Grow(this, len, FALSE)) {
        MSVCP_char_traits_char__Copy_s(this->ptr, this->res, str, len);
        basic_string_char__Eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign(
        basic_string_char *this, const basic_string_char *assign)
{
    return MSVCP_basic_string_char_assign_substr(this, assign,
            0, MSVCP_basic_string_char_npos);
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_cstr(
        basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, str,
            MSVCP_char_traits_char_length(str));
}

/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@D@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@D@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_ch, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_ch(
        basic_string_char *this, char ch)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, &ch, 1);
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ID@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assignn, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assignn(
        basic_string_char *this, size_t count, char ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_char__Grow(this, count, FALSE);
    MSVCP_char_traits_char_assignn(this->ptr, count, ch);
    basic_string_char__Eos(this, count);
    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD0@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_ptr_ptr, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_ptr_ptr(
        basic_string_char *this, const char *first, const char *last)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, first, last-first);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDIABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD_KAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_len_alloc, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_len_alloc(
        basic_string_char *this, const char *str, size_t len, const void *alloc)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(str, len), len);

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assign_cstr_len(this, str, len);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDI@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_len(
        basic_string_char *this, const char *str, size_t len)
{
    return MSVCP_basic_string_char_ctor_cstr_len_alloc(this, str, len, NULL);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@IIABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@_K1AEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_substr_alloc, 20)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_substr_alloc(
        basic_string_char *this, const basic_string_char *assign,
        size_t pos, size_t len, const void *alloc)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assign_substr(this, assign, pos, len);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBDAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_alloc, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_alloc(
        basic_string_char *this, const char *str, const void *alloc)
{
    TRACE("%p %s\n", this, debugstr_a(str));

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assign_cstr(this, str);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr(
        basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_ctor_cstr_alloc(this, str, NULL);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@IDABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@_KDAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_ch_alloc, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_ch_alloc(basic_string_char *this,
        size_t count, char ch, const void *alloc)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assignn(this, count, ch);
    return this;
}

/* ??_F?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor, 4)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor(basic_string_char *this)
{
    TRACE("%p\n", this);

    basic_string_char__Tidy(this, FALSE);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_alloc, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_alloc(
        basic_string_char *this, const void *alloc)
{
    TRACE("%p %p\n", this, alloc);

    basic_string_char__Tidy(this, FALSE);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_copy_ctor, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_copy_ctor(
        basic_string_char *this, const basic_string_char *copy)
{
    TRACE("%p %p\n", this, copy);

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assign(this, copy);
    return this;
}

/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_dtor, 4)
void __thiscall MSVCP_basic_string_char_dtor(basic_string_char *this)
{
    TRACE("%p\n", this);
    basic_string_char__Tidy(this, TRUE);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIPBDI@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_cstr_len, 20)
int __thiscall MSVCP_basic_string_char_compare_substr_cstr_len(
        const basic_string_char *this, size_t pos, size_t num,
        const char *str, size_t count)
{
    int ans;

    TRACE("%p %Iu %Iu %s %Iu\n", this, pos, num, debugstr_an(str, count), count);

    if(this->size < pos)
        _Xran();

    if(num > this->size-pos)
        num = this->size-pos;

    ans = MSVCP_char_traits_char_compare(this->ptr+pos,
            str, num>count ? count : num);
    if(ans)
        return ans;

    if(num > count)
        ans = 1;
    else if(num < count)
        ans = -1;
    return ans;
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHPBD@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAHPEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_cstr, 8)
int __thiscall MSVCP_basic_string_char_compare_cstr(
        const basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, 0, this->size,
            str, MSVCP_char_traits_char_length(str));
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIABV12@II@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_substr, 24)
int __thiscall MSVCP_basic_string_char_compare_substr_substr(
        const basic_string_char *this, size_t pos, size_t num,
        const basic_string_char *compare, size_t off, size_t count)
{
    TRACE("%p %Iu %Iu %p %Iu %Iu\n", this, pos, num, compare, off, count);

    if(compare->size < off)
        _Xran();

    if(count > compare->size-off)
        count = compare->size-off;

    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            compare->ptr+off, count);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIABV12@@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr, 16)
int __thiscall MSVCP_basic_string_char_compare_substr(
        const basic_string_char *this, size_t pos, size_t num,
        const basic_string_char *compare)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            compare->ptr, compare->size);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHABV12@@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAHAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare, 8)
int __thiscall MSVCP_basic_string_char_compare(
        const basic_string_char *this, const basic_string_char *compare)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, 0, this->size,
            compare->ptr, compare->size);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIPBD@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_cstr, 16)
int __thiscall MSVCP_basic_string_char_compare_substr_cstr(const basic_string_char *this,
        size_t pos, size_t num, const char *str)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            str, MSVCP_char_traits_char_length(str));
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??8std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??8std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_equal(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) == 0;
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??8std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??8std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_equal_str_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) == 0;
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??8std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??8std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_equal_cstr_str(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) == 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??9std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??9std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_not_equal(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) != 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??9std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??9std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_not_equal_str_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) != 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??9std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??9std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_not_equal_cstr_str(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) != 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Mstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Mstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_lower(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) < 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??Mstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??Mstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_lower_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) < 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Mstd@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Mstd@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_lower_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) > 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Nstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Nstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_leq(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) <= 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??Nstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??Nstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_leq_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) <= 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Nstd@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Nstd@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_leq_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) >= 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Ostd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Ostd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_greater(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) > 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??Ostd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??Ostd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_greater_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) > 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Ostd@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Ostd@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_greater_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) < 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Pstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??Pstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_geq(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) >= 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??Pstd@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??Pstd@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_geq_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) >= 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Pstd@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??Pstd@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_geq_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) <= 0;
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_cstr_substr(
        const basic_string_char *this, const char *find, size_t pos, size_t len)
{
    const char *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_an(find, len), pos, len);

    if(len==0 && pos<=this->size)
        return pos;
    if(pos>=this->size || len>this->size)
        return MSVCP_basic_string_char_npos;

    end = this->ptr+this->size-len+1;
    for(p=this->ptr+pos; p<end; p++) {
        p = MSVCP_char_traits_char_find(p, end-p, find);
        if(!p)
            break;

        if(!MSVCP_char_traits_char_compare(p, find, len))
            return p-this->ptr;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_off, 12)
size_t __thiscall MSVCP_basic_string_char_find_off(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_cstr_substr(this, find->ptr, off, find->size);
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_ch(
        const basic_string_char *this, char ch, size_t pos)
{
    return MSVCP_basic_string_char_find_cstr_substr(this, &ch, pos, 1);
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_char_find_cstr_off(
        const basic_string_char *this, const char *find, size_t pos)
{
    return MSVCP_basic_string_char_find_cstr_substr(this, find, pos,
            MSVCP_char_traits_char_length(find));
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_rfind_cstr_substr(
        const basic_string_char *this, const char *find, size_t pos, size_t len)
{
    const char *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_an(find, len), pos, len);

    if(len==0)
        return pos<this->size ? pos : this->size;

    if(len > this->size)
        return MSVCP_basic_string_char_npos;

    if(pos > this->size-len)
        pos = this->size-len;
    end = this->ptr;
    for(p=end+pos; p>=end; p--) {
        if(*p==*find && !MSVCP_char_traits_char_compare(p, find, len))
            return p-this->ptr;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_off, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_off(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this, find->ptr, off, find->size);
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_ch, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_ch(
        const basic_string_char *this, char ch, size_t pos)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this, &ch, pos, 1);
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_cstr_off(
        const basic_string_char *this, const char *find, size_t pos)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this, find, pos,
            MSVCP_char_traits_char_length(find));
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(off<this->size) {
        end = this->ptr+this->size;
        for(p=this->ptr+off; p<end; p++)
            if(!MSVCP_char_traits_char_find(find, len, p))
                return p-this->ptr;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_first_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && off<this->size) {
        end = this->ptr+this->size;
        for(p=this->ptr+off; p<end; p++)
            if(MSVCP_char_traits_char_find(find, len, p))
                return p-this->ptr;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = this->ptr;
        for(p=beg+off; p>=beg; p--)
            if(!MSVCP_char_traits_char_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_last_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = this->ptr;
        for(p=beg+off; p>=beg; p--)
            if(MSVCP_char_traits_char_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_append_substr(basic_string_char *this,
        const basic_string_char *append, size_t offset, size_t count)
{
    TRACE("%p %p %Iu %Iu\n", this, append, offset, count);

    if(append->size < offset)
        _Xran();

    if(count > append->size-offset)
        count = append->size-offset;

    if(MSVCP_basic_string_char_npos-this->size<=count || this->size+count<this->size)
        _Xlen();

    if(basic_string_char__Grow(this, this->size+count, FALSE)) {
        if(append->ptr)
            MSVCP_char_traits_char__Copy_s(this->ptr+this->size, this->res-this->size,
                    append->ptr+offset, count);
        basic_string_char__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_cstr_len(
        basic_string_char *this, const char *append, size_t count)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(append, count), count);

    if(basic_string_char_inside(this, append))
        return MSVCP_basic_string_char_append_substr(this, this, append-this->ptr, count);

    if(MSVCP_basic_string_char_npos-this->size<=count || this->size+count<this->size)
        _Xlen();

    if(basic_string_char__Grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char__Copy_s(this->ptr+this->size,
                this->res-this->size, append, count);
        basic_string_char__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ID@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_len_ch, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_len_ch(
        basic_string_char *this, size_t count, char ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    if(MSVCP_basic_string_char_npos-this->size <= count)
        _Xlen();

    if(basic_string_char__Grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char_assignn(this->ptr+this->size, count, ch);
        basic_string_char__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@ABV01@@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append(
        basic_string_char *this, const basic_string_char *append)
{
    return MSVCP_basic_string_char_append_substr(this, append,
            0, MSVCP_basic_string_char_npos);
}

/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@D@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@D@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_ch, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append_ch(
        basic_string_char *this, char ch)
{
    return MSVCP_basic_string_char_append_len_ch(this, 1, ch);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD0@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_beg_end, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_beg_end(
        basic_string_char *this, const char *beg, const char *end)
{
    return MSVCP_basic_string_char_append_cstr_len(this, beg, end-beg);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append_cstr(
        basic_string_char *this, const char *append)
{
    return MSVCP_basic_string_char_append_cstr_len(this, append,
            MSVCP_char_traits_char_length(append));
}
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@0@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@0@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate(basic_string_char *ret,
        const basic_string_char *left, const basic_string_char *right)
{
    TRACE("%p %p\n", left, right);

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@D@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@D@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@D@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@D@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_bstr_ch(basic_string_char *ret,
        const basic_string_char *left, char right)
{
    TRACE("%p %c\n", left, right);

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append_ch(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@PBD@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@PEBD@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@PBD@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@PEBD@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_bstr_cstr(basic_string_char *ret,
        const basic_string_char *left, const char *right)
{
    TRACE("%p %s\n", left, debugstr_a(right));

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append_cstr(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DABV10@@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DAEBV10@@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_ch_bstr(basic_string_char *ret,
        char left, const basic_string_char *right)
{
    TRACE("%c %p\n", left, right);

    MSVCP_basic_string_char_ctor_cstr_len_alloc(ret, &left, 1, NULL);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBDABV10@@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBDAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBDABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBDAEBV10@@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_cstr_bstr(basic_string_char *ret,
        const char *left, const basic_string_char *right)
{
    TRACE("%s %p\n", debugstr_a(left), right);

    MSVCP_basic_string_char_ctor_cstr_alloc(ret, left, NULL);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADI@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_operator_at, 8)
char* __thiscall MSVCP_basic_string_char_operator_at(
        basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(!this->ptr || pos>this->size)
        return (char*)basic_string_char__Nullstr();

    basic_string_char__Freeze(this);
    return this->ptr+pos;
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDI@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAAEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_operator_at_const, 8)
const char* __thiscall MSVCP_basic_string_char_operator_at_const(
        const basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(!this->ptr)
        return basic_string_char__Nullstr();
    return this->ptr+pos;
}

/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADI@Z */
/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAD_K@Z */
/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDI@Z */
/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAAEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_at, 8)
char* __thiscall MSVCP_basic_string_char_at(
        basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        _Xran();

    return this->ptr+pos;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIPBDI@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0PEBD0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_cstr_len, 20)
basic_string_char* __thiscall basic_string_char_replace_cstr_len(basic_string_char *this,
        size_t off, size_t len, const char *str, size_t str_len)
{
    size_t inside_pos = -1;
    char *ptr = this->ptr;

    TRACE("%p %Iu %Iu %p %Iu\n", this, off, len, str, str_len);

    if(this->size < off)
        _Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_char_npos-str_len <= this->size-len)
        _Xlen();

    if(basic_string_char_inside(this, str))
        inside_pos = str-ptr;

    if(this->size-len+str_len)
        basic_string_char__Grow(this, this->size-len+str_len, FALSE);
    ptr = this->ptr;

    if(inside_pos == -1) {
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
        memcpy(ptr+off, str, str_len*sizeof(char));
    } else if(len >= str_len) {
        memmove(ptr+off, ptr+inside_pos, str_len*sizeof(char));
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
    } else {
        size_t size;

        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));

        if(inside_pos < off+len) {
            size = off+len-inside_pos;
            if(size > str_len)
                size = str_len;
            memmove(ptr+off, ptr+inside_pos, size*sizeof(char));
        } else {
            size = 0;
        }

        if(str_len > size)
            memmove(ptr+off+size, ptr+off+str_len, (str_len-size)*sizeof(char));
    }

    if(this->ptr)
        basic_string_char__Eos(this, this->size-len+str_len);
    return this;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIABV12@II@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_substr, 24)
basic_string_char* __thiscall basic_string_char_replace_substr(basic_string_char *this, size_t off,
        size_t len, const basic_string_char *str, size_t str_off, size_t str_len)
{
    if(str->size < str_off)
        _Xran();

    if(str_len > str->size-str_off)
        str_len = str->size-str_off;

    return basic_string_char_replace_cstr_len(this, off, len,
            str->ptr+str_off, str_len);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIABV12@@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace, 16)
basic_string_char* __thiscall basic_string_char_replace(basic_string_char *this,
        size_t off, size_t len, const basic_string_char *str)
{
    return basic_string_char_replace_cstr_len(this, off, len,
            str->ptr, str->size);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIID@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K00D@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_ch, 20)
basic_string_char* __thiscall basic_string_char_replace_ch(basic_string_char *this,
        size_t off, size_t len, size_t count, char ch)
{
    char *ptr;

    TRACE("%p %Iu %Iu %Iu %c\n", this, off, len, count, ch);

    if(this->size < off)
        _Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_char_npos-count <= this->size-len)
        _Xlen();

    if(this->size-len+count)
        basic_string_char__Grow(this, this->size-len+count, FALSE);
    ptr = this->ptr;

    memmove(ptr+off+count, ptr+off+len, (this->size-off-len)*sizeof(char));
    MSVCP_char_traits_char_assignn(ptr+off, count, ch);
    basic_string_char__Eos(this, this->size-len+count);

    return this;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIPBD@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0PEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_cstr, 16)
basic_string_char* __thiscall basic_string_char_replace_cstr(basic_string_char *this,
        size_t off, size_t len, const char *str)
{
    return basic_string_char_replace_cstr_len(this, off, len, str,
            MSVCP_char_traits_char_length(str));
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IABV12@@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert, 12)
basic_string_char* __thiscall basic_string_char_insert(basic_string_char *this,
        size_t off, const basic_string_char *str)
{
    return basic_string_char_replace(this, off, 0, str);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IABV12@II@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KAEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_substr, 20)
basic_string_char* __thiscall basic_string_char_insert_substr(
        basic_string_char *this, size_t off, const basic_string_char *str,
        size_t str_off, size_t str_count)
{
    return basic_string_char_replace_substr(this, off, 0, str, str_off, str_count);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IPBD@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KPEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_cstr, 12)
basic_string_char* __thiscall basic_string_char_insert_cstr(
        basic_string_char *this, size_t off, const char *str)
{
    return basic_string_char_replace_cstr(this, off, 0, str);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IPBDI@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KPEBD0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_cstr_len, 16)
basic_string_char* __thiscall basic_string_char_insert_cstr_len(basic_string_char *this,
        size_t off, const char *str, size_t str_len)
{
    return basic_string_char_replace_cstr_len(this, off, 0, str, str_len);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IID@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0D@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_chn, 16)
basic_string_char* __thiscall basic_string_char_insert_chn(basic_string_char *this,
        size_t off, size_t count, char ch)
{
    return basic_string_char_replace_ch(this, off, 0, count, ch);
}

/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXID@Z */
/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_resize_ch, 12)
void __thiscall MSVCP_basic_string_char_resize_ch(
        basic_string_char *this, size_t size, char ch)
{
    TRACE("%p %Iu %c\n", this, size, ch);

    if(size <= this->size)
        MSVCP_basic_string_char_erase(this, size, this->size);
    else
        MSVCP_basic_string_char_append_len_ch(this, size-this->size, ch);
}

/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXI@Z */
/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_resize, 8)
void __thiscall MSVCP_basic_string_char_resize(
        basic_string_char *this, size_t size)
{
    MSVCP_basic_string_char_resize_ch(this, size, '\0');
}

/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_c_str, 4)
const char* __thiscall MSVCP_basic_string_char_c_str(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->ptr ? this->ptr : basic_string_char__Nullstr();
}

/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_length, 4)
size_t __thiscall MSVCP_basic_string_char_length(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ?max_size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?max_size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_max_size, 4)
size_t __thiscall basic_string_char_max_size(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return MSVCP_allocator_char_max_size(NULL)-1;
}

/* ?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_capacity, 4)
size_t __thiscall MSVCP_basic_string_char_capacity(basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->res;
}

/* ?reserve@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXI@Z */
/* ?reserve@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_reserve, 8)
void __thiscall MSVCP_basic_string_char_reserve(basic_string_char *this, size_t size)
{
    size_t len;

    TRACE("%p %Iu\n", this, size);

    len = this->size;
    if(len > size)
        return;

    if(basic_string_char__Grow(this, size, FALSE))
        basic_string_char__Eos(this, len);
}

/* ?empty@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE_NXZ */
/* ?empty@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_empty, 4)
bool __thiscall MSVCP_basic_string_char_empty(basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->size == 0;
}

/* ?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_swap, 8)
void __thiscall MSVCP_basic_string_char_swap(basic_string_char *this, basic_string_char *str)
{
    basic_string_char tmp;
    TRACE("%p %p\n", this, str);

    tmp = *this;
    *this = *str;
    *str = tmp;
}

/* ?substr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV12@II@Z */
/* ?substr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_substr(basic_string_char *this,
        basic_string_char *ret, size_t off, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, off, len);

    MSVCP_basic_string_char_ctor_substr_alloc(ret, this, off, len, NULL);
    return ret;
}

/* ?copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPADII@Z */
/* ?copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEAD_K1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_copy, 16)
size_t __thiscall basic_string_char_copy(const basic_string_char *this,
        char *dest, size_t count, size_t off)
{
    TRACE("%p %p %Iu %Iu\n", this, dest, count, off);

    if(off > this->size)
        _Xran();
    if(count > this->size-off)
        count = this->size-off;
    if(this->ptr)
        MSVCP_char_traits_char__Copy_s(dest, count, this->ptr+off, count);
    return count;
}

/* ?get_allocator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$allocator@D@2@XZ */
/* ?get_allocator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$allocator@D@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_get_allocator, 8)
allocator* __thiscall basic_string_char_get_allocator(const basic_string_char *this, allocator *ret)
{
    TRACE("%p\n", this);
    return ret;
}

static bool basic_string_wchar_inside(
        basic_string_wchar *this, const wchar_t *ptr)
{
    return ptr>=this->ptr && ptr<this->ptr+this->size;
}

/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2IB */
/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2_KB */
const size_t MSVCP_basic_string_wchar_npos = -1;

/* ?_C@?1??_Nullstr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPBGXZ@4GB */
/* ?_C@?1??_Nullstr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPEBGXZ@4GB */
const wchar_t basic_string_wchar_nullbyte = '\0';

/* ?_Nullstr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPBGXZ */
/* ?_Nullstr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPEBGXZ */
const wchar_t* __cdecl basic_string_wchar__Nullstr(void)
{
    return &basic_string_wchar_nullbyte;
}

/* ?_Refcnt@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEAAEPBG@Z */
/* ?_Refcnt@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAAEAEPEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Refcnt, 8)
unsigned short* __thiscall basic_string_wchar__Refcnt(basic_string_wchar *this, const wchar_t *ptr)
{
    TRACE("(%p %p)\n", this, ptr);
    return (unsigned short*)ptr-1;
}

/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEXI@Z */
/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Eos, 8)
void __thiscall basic_string_wchar__Eos(basic_string_wchar *this, size_t len)
{
    this->size = len;
    this->ptr[len] = 0;
}

/* ?clear@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
/* ?clear@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_clear, 4)
void __thiscall MSVCP_basic_string_wchar_clear(basic_string_wchar *this)
{
    if(this->ptr)
        basic_string_wchar__Eos(this, 0);
}

/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEX_N@Z */
/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Tidy, 8)
void __thiscall basic_string_wchar__Tidy(basic_string_wchar *this, bool built)
{
    TRACE("(%p %d)\n", this, built);

    if(!built || !this->ptr);
    else if(!this->ptr[-1] || this->ptr[-1]==FROZENW)
        MSVCP_allocator_wchar_deallocate(NULL, this->ptr-1, this->res+2);
    else
        this->ptr[-1]--;

    memset(this, 0, sizeof(*this));
}

/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAE_NI_N@Z */
/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAA_N_K_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Grow, 12)
bool __thiscall basic_string_wchar__Grow(basic_string_wchar *this, size_t new_size, bool trim)
{
    if(!new_size) {
        if(trim)
            basic_string_wchar__Tidy(this, TRUE);
        else if(this->ptr)
            basic_string_wchar__Eos(this, 0);
    } else if(this->res<new_size || trim ||
            (this->ptr && this->ptr[-1] && this->ptr[-1]!=FROZENW)) {
        size_t new_res = new_size, len = this->size;
        wchar_t *ptr;

        if(!trim && this->ptr && !this->ptr[-1]) {
            new_res |= 0xf;
            if(new_res/3 < this->res/2)
                new_res = this->res + this->res/2;
        }

        ptr = MSVCP_allocator_wchar_allocate(this->allocator, new_res+2);
        if(!ptr) {
            new_res = new_size;
            ptr = MSVCP_allocator_wchar_allocate(this->allocator, new_size+2);
        }
        if(!ptr) {
            ERR("Out of memory\n");
            return FALSE;
        }

        if(len > new_res)
            len = new_res;

        *ptr = 0;
        if(this->ptr)
            MSVCP_char_traits_wchar__Copy_s(ptr+1, new_size, this->ptr, len);
        basic_string_wchar__Tidy(this, TRUE);
        this->ptr = ptr+1;
        this->res = new_res;
        basic_string_wchar__Eos(this, len);
    }

    return new_size>0;
}

/* ?_Split@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEXXZ */
/* ?_Split@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Split, 4)
void __thiscall basic_string_wchar__Split(basic_string_wchar *this)
{
    size_t len;
    wchar_t *ptr;

    TRACE("(%p)\n", this);

    if(!this->ptr || !this->ptr[-1] || this->ptr[-1]==FROZENW)
        return;

    ptr = this->ptr;
    len = this->size;
    basic_string_wchar__Tidy(this, TRUE);
    if(basic_string_wchar__Grow(this, len, FALSE)) {
        if(ptr)
            MSVCP_char_traits_wchar__Copy_s(this->ptr, this->res, ptr, len);
        basic_string_wchar__Eos(this, len);
    }
}

/* ?_Freeze@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEXXZ */
/* ?_Freeze@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Freeze, 4)
void __thiscall basic_string_wchar__Freeze(basic_string_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_string_wchar__Split(this);
    if(this->ptr)
        this->ptr[-1] = FROZENW;
}

/* ?_Copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEXI@Z */
/* ?_Copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Copy, 8)
void __thiscall basic_string_wchar__Copy(basic_string_wchar *this, size_t copy_len)
{
    TRACE("%p %Iu\n", this, copy_len);

    if(!basic_string_wchar__Grow(this, copy_len, TRUE))
        return;
}

/* ?_Psum@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPAGPAGI@Z */
/* ?_Psum@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPEAGPEAG_K@Z */
/* ?_Psum@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPBGPBGI@Z */
/* ?_Psum@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAPEBGPEBG_K@Z */
wchar_t* __cdecl basic_string_wchar__Psum(wchar_t *iter, size_t add)
{
    TRACE("(%p %Iu)\n", iter, add);
    return iter ? iter+add : iter;
}

/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_erase, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_erase(
        basic_string_wchar *this, size_t pos, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, pos, len);

    if(pos > this->size)
        _Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        basic_string_wchar__Split(this);
        MSVCP_char_traits_wchar__Move_s(this->ptr+pos, this->res-pos,
                this->ptr+pos+len, this->size-pos-len);
        basic_string_wchar__Eos(this, this->size-len);
    }

    return this;
}

/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEPAGPAG@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAPEAGPEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_erase_beg, 8)
wchar_t* __thiscall basic_string_wchar_erase_beg(basic_string_wchar *this, wchar_t *beg)
{
    size_t pos = beg-this->ptr;
    MSVCP_basic_string_wchar_erase(this, pos, 1);
    return this->ptr+pos;
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_substr(
        basic_string_wchar *this, const basic_string_wchar *assign,
        size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    if(assign->size < pos)
        _Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_wchar_erase(this, pos+len, MSVCP_basic_string_wchar_npos);
        MSVCP_basic_string_wchar_erase(this, 0, pos);
    } else if(basic_string_wchar__Grow(this, len, FALSE)) {
        if(assign->ptr)
            MSVCP_char_traits_wchar__Copy_s(this->ptr, this->res,
                    assign->ptr+pos, len);
        basic_string_wchar__Eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_cstr_len(
        basic_string_wchar *this, const wchar_t *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(str, len), len);

    if(basic_string_wchar_inside(this, str))
        return MSVCP_basic_string_wchar_assign_substr(this, this, str-this->ptr, len);
    else if(basic_string_wchar__Grow(this, len, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(this->ptr, this->res, str, len);
        basic_string_wchar__Eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign(
        basic_string_wchar *this, const basic_string_wchar *assign)
{
    return MSVCP_basic_string_wchar_assign_substr(this, assign,
            0, MSVCP_basic_string_wchar_npos);
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@PBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_cstr(
        basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, str,
            MSVCP_char_traits_wchar_length(str));
}

/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@G@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_ch, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_ch(
        basic_string_wchar *this, wchar_t ch)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, &ch, 1);
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IG@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assignn, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assignn(
        basic_string_wchar *this, size_t count, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_wchar__Grow(this, count, FALSE);
    MSVCP_char_traits_wchar_assignn(this->ptr, count, ch);
    basic_string_wchar__Eos(this, count);
    return this;
}

/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_ptr_ptr, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_ptr_ptr(
        basic_string_wchar *this, const wchar_t *first, const wchar_t *last)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, first, last-first);
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGIABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG_KAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_len_alloc, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_len_alloc(
        basic_string_wchar *this, const wchar_t *str, size_t len, const void *alloc)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(str, len), len);

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assign_cstr_len(this, str, len);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_WI@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W_K@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGI@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_len(
        basic_string_wchar *this, const wchar_t *str, size_t len)
{
    return MSVCP_basic_string_wchar_ctor_cstr_len_alloc(this, str, len, NULL);
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@IIABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@_K1AEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_substr_alloc, 20)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_substr_alloc(
        basic_string_wchar *this, const basic_string_wchar *assign,
        size_t pos, size_t len, const void *alloc)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assign_substr(this, assign, pos, len);
    return this;
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBGAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_alloc, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_alloc(
        basic_string_wchar *this, const wchar_t *str, const void *alloc)
{
    TRACE("%p %s\n", this, debugstr_w(str));

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assign_cstr(this, str);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_W@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBG@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr(
            basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_ctor_cstr_alloc(this, str, NULL);
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@IGABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@_KGAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_ch_alloc, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_ch_alloc(basic_string_wchar *this,
        size_t count, wchar_t ch, const void *alloc)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assignn(this, count, ch);
    return this;
}

/* ??_F?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor, 4)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor(basic_string_wchar *this)
{
    TRACE("%p\n", this);

    basic_string_wchar__Tidy(this, FALSE);
    return this;
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_alloc, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_alloc(
        basic_string_wchar *this, const void *alloc)
{
    TRACE("%p %p\n", this, alloc);

    basic_string_wchar__Tidy(this, FALSE);
    return this;
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_copy_ctor, 8)
basic_string_wchar* __thiscall basic_string_wchar_copy_ctor(
        basic_string_wchar *this, const basic_string_wchar *copy)
{
    TRACE("%p %p\n", this, copy);

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assign(this, copy);
    return this;
}

/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ */
/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_dtor, 4)
void __thiscall MSVCP_basic_string_wchar_dtor(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    basic_string_wchar__Tidy(this, TRUE);
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIPBGI@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_cstr_len, 20)
int __thiscall MSVCP_basic_string_wchar_compare_substr_cstr_len(
        const basic_string_wchar *this, size_t pos, size_t num,
        const wchar_t *str, size_t count)
{
    int ans;

    TRACE("%p %Iu %Iu %s %Iu\n", this, pos, num, debugstr_wn(str, count), count);

    if(this->size < pos)
        _Xran();

    if(num > this->size-pos)
        num = this->size-pos;

    ans = MSVCP_char_traits_wchar_compare(this->ptr+pos,
            str, num>count ? count : num);
    if(ans)
        return ans;

    if(num > count)
        ans = 1;
    else if(num < count)
        ans = -1;
    return ans;
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHPBG@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAHPEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_cstr, 8)
int __thiscall MSVCP_basic_string_wchar_compare_cstr(
        const basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, 0, this->size,
            str, MSVCP_char_traits_wchar_length(str));
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIABV12@II@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_substr, 24)
int __thiscall MSVCP_basic_string_wchar_compare_substr_substr(
        const basic_string_wchar *this, size_t pos, size_t num,
        const basic_string_wchar *compare, size_t off, size_t count)
{
    TRACE("%p %Iu %Iu %p %Iu %Iu\n", this, pos, num, compare, off, count);

    if(compare->size < off)
        _Xran();

    if(count > compare->size-off)
        count = compare->size-off;

    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            compare->ptr+off, count);
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIABV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr, 16)
int __thiscall MSVCP_basic_string_wchar_compare_substr(
        const basic_string_wchar *this, size_t pos, size_t num,
        const basic_string_wchar *compare)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            compare->ptr, compare->size);
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHABV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAHAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare, 8)
int __thiscall MSVCP_basic_string_wchar_compare(
        const basic_string_wchar *this, const basic_string_wchar *compare)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, 0, this->size,
            compare->ptr, compare->size);
}

/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIPBG@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_cstr, 16)
int __thiscall MSVCP_basic_string_wchar_compare_substr_cstr(const basic_string_wchar *this,
        size_t pos, size_t num, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            str, MSVCP_char_traits_wchar_length(str));
}

/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??8std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??8std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_equal(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) == 0;
}

/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??8std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??8std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_equal_str_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) == 0;
}

/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??8std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??8std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_equal_cstr_str(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) == 0;
}

/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??9std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??9std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) != 0;
}

/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??9std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??9std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal_str_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) != 0;
}

/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??9std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??9std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal_cstr_str(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) != 0;
}

/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Mstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Mstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_lower(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) < 0;
}

/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??Mstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??Mstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_lower_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) < 0;
}

/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Mstd@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Mstd@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_lower_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) > 0;
}

/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Nstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Nstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_leq(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) <= 0;
}

/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??Nstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??Nstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_leq_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) <= 0;
}

/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Nstd@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Nstd@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_leq_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) >= 0;
}

/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Ostd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Ostd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_greater(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) > 0;
}

/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??Ostd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??Ostd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_greater_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) > 0;
}

/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Ostd@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Ostd@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_greater_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) < 0;
}

/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Pstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??Pstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_geq(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) >= 0;
}

/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??Pstd@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??Pstd@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_geq_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) >= 0;
}

/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Pstd@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??Pstd@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_geq_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) <= 0;
}

/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t pos, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_wn(find, len), pos, len);

    if(len==0 && pos<=this->size)
        return pos;
    if(pos>=this->size || len>this->size)
        return MSVCP_basic_string_wchar_npos;

    end = this->ptr+this->size-len+1;
    for(p=this->ptr+pos; p<end; p++) {
        p = MSVCP_char_traits_wchar_find(p, end-p, find);
        if(!p)
            break;

        if(!MSVCP_char_traits_wchar_compare(p, find, len))
            return p-this->ptr;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_off(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this, find->ptr, off, find->size);
}

/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_ch(
        const basic_string_wchar *this, wchar_t ch, size_t pos)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this, &ch, pos, 1);
}

/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_cstr_off(
        const basic_string_wchar *this, const wchar_t *find, size_t pos)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this, find, pos,
            MSVCP_char_traits_wchar_length(find));
}

/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_rfind_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t pos, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_wn(find, len), pos, len);

    if(len==0)
        return pos<this->size ? pos : this->size;

    if(len > this->size)
        return MSVCP_basic_string_wchar_npos;

    if(pos > this->size-len)
        pos = this->size-len;
    end = this->ptr;
    for(p=end+pos; p>=end; p--) {
        if(*p==*find && !MSVCP_char_traits_wchar_compare(p, find, len))
            return p-this->ptr;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_off(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this, find->ptr, off, find->size);
}

/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_ch(
        const basic_string_wchar *this, wchar_t ch, size_t pos)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this, &ch, pos, 1);
}

/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_cstr_off(
        const basic_string_wchar *this, const wchar_t *find, size_t pos)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this, find, pos,
            MSVCP_char_traits_wchar_length(find));
}

/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(off<this->size) {
        end = this->ptr+this->size;
        for(p=this->ptr+off; p<end; p++)
            if(!MSVCP_char_traits_wchar_find(find, len, p))
                return p-this->ptr;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && off<this->size) {
        end = this->ptr+this->size;
        for(p=this->ptr+off; p<end; p++)
            if(MSVCP_char_traits_wchar_find(find, len, p))
                return p-this->ptr;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = this->ptr;
        for(p=beg+off; p>=beg; p--)
            if(!MSVCP_char_traits_wchar_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = this->ptr;
        for(p=beg+off; p>=beg; p--)
            if(MSVCP_char_traits_wchar_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(this,
            find->ptr, off, find->size);
}

/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_substr(basic_string_wchar *this,
        const basic_string_wchar *append, size_t offset, size_t count)
{
    TRACE("%p %p %Iu %Iu\n", this, append, offset, count);

    if(append->size < offset)
        _Xran();

    if(count > append->size-offset)
        count = append->size-offset;

    if(MSVCP_basic_string_wchar_npos-this->size<=count || this->size+count<this->size)
        _Xlen();

    if(basic_string_wchar__Grow(this, this->size+count, FALSE)) {
        if(append->ptr)
            MSVCP_char_traits_wchar__Copy_s(this->ptr+this->size, this->res-this->size,
                    append->ptr+offset, count);
        basic_string_wchar__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_cstr_len(
        basic_string_wchar *this, const wchar_t *append, size_t count)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(append, count), count);

    if(basic_string_wchar_inside(this, append))
        return MSVCP_basic_string_wchar_append_substr(this, this, append-this->ptr, count);

    if(MSVCP_basic_string_wchar_npos-this->size<=count || this->size+count<this->size)
        _Xlen();

    if(basic_string_wchar__Grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(this->ptr+this->size,
                this->res-this->size, append, count);
        basic_string_wchar__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IG@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_len_ch, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_len_ch(
        basic_string_wchar *this, size_t count, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    if(MSVCP_basic_string_wchar_npos-this->size <= count)
        _Xlen();

    if(basic_string_wchar__Grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_wchar_assignn(this->ptr+this->size, count, ch);
        basic_string_wchar__Eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@ABV01@@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append(
        basic_string_wchar *this, const basic_string_wchar *append)
{
    return MSVCP_basic_string_wchar_append_substr(this, append,
            0, MSVCP_basic_string_wchar_npos);
}

/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@G@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_ch, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_ch(
        basic_string_wchar *this, wchar_t ch)
{
    return MSVCP_basic_string_wchar_append_len_ch(this, 1, ch);
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_beg_end, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_beg_end(
        basic_string_wchar *this, const wchar_t *beg, const wchar_t *end)
{
    return MSVCP_basic_string_wchar_append_cstr_len(this, beg, end-beg);
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@PBG@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_cstr(
        basic_string_wchar *this, const wchar_t *append)
{
    return MSVCP_basic_string_wchar_append_cstr_len(this, append,
            MSVCP_char_traits_wchar_length(append));
}
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@0@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@0@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate(basic_string_wchar *ret,
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    TRACE("%p %p\n", left, right);

    basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@G@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@G@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@G@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@G@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_bstr_ch(basic_string_wchar *ret,
        const basic_string_wchar *left, wchar_t right)
{
    TRACE("%p %c\n", left, right);

    basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append_ch(ret, right);
    return ret;
}

/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@PBG@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@PEBG@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@PBG@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@PEBG@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_bstr_cstr(basic_string_wchar *ret,
        const basic_string_wchar *left, const wchar_t *right)
{
    TRACE("%p %s\n", left, debugstr_w(right));

    basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append_cstr(ret, right);
    return ret;
}

/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GABV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GAEBV10@@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_ch_bstr(basic_string_wchar *ret,
        wchar_t left, const basic_string_wchar *right)
{
    TRACE("%c %p\n", left, right);

    MSVCP_basic_string_wchar_ctor_cstr_len_alloc(ret, &left, 1, NULL);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBGABV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBGAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBGABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBGAEBV10@@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_cstr_bstr(basic_string_wchar *ret,
        const wchar_t *left, const basic_string_wchar *right)
{
    TRACE("%s %p\n", debugstr_w(left), right);

    MSVCP_basic_string_wchar_ctor_cstr_alloc(ret, left, NULL);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGI@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_operator_at, 8)
wchar_t* __thiscall MSVCP_basic_string_wchar_operator_at(
        basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(!this->ptr || pos>this->size)
        return (wchar_t*)basic_string_wchar__Nullstr();

    basic_string_wchar__Freeze(this);
    return this->ptr+pos;
}

/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGI@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAAEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_operator_at_const, 8)
const wchar_t* __thiscall MSVCP_basic_string_wchar_operator_at_const(
        const basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(!this->ptr)
        return basic_string_wchar__Nullstr();
    return this->ptr+pos;
}

/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGI@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAG_K@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGI@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAAEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_at, 8)
wchar_t* __thiscall MSVCP_basic_string_wchar_at(
        basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        _Xran();

    return this->ptr+pos;
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIPBGI@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0PEBG0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_cstr_len, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_cstr_len(basic_string_wchar *this,
        size_t off, size_t len, const wchar_t *str, size_t str_len)
{
    size_t inside_pos = -1;
    wchar_t *ptr = this->ptr;

    TRACE("%p %Iu %Iu %p %Iu\n", this, off, len, str, str_len);

    if(this->size < off)
        _Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_wchar_npos-str_len <= this->size-len)
        _Xlen();

    if(basic_string_wchar_inside(this, str))
        inside_pos = str-ptr;

    if(this->size-len+str_len)
        basic_string_wchar__Grow(this, this->size-len+str_len, FALSE);
    ptr = this->ptr;

    if(inside_pos == -1) {
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
        memcpy(ptr+off, str, str_len*sizeof(char));
    } else if(len >= str_len) {
        memmove(ptr+off, ptr+inside_pos, str_len*sizeof(char));
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
    } else {
        size_t size;

        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));

        if(inside_pos < off+len) {
            size = off+len-inside_pos;
            if(size > str_len)
                size = str_len;
            memmove(ptr+off, ptr+inside_pos, size*sizeof(char));
        } else {
            size = 0;
        }

        if(str_len > size)
            memmove(ptr+off+size, ptr+off+str_len, (str_len-size)*sizeof(char));
    }

    if(this->ptr)
        basic_string_wchar__Eos(this, this->size-len+str_len);
    return this;
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIABV12@II@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_substr, 24)
basic_string_wchar* __thiscall basic_string_wchar_replace_substr(basic_string_wchar *this, size_t off,
        size_t len, const basic_string_wchar *str, size_t str_off, size_t str_len)
{
    if(str->size < str_off)
        _Xran();

    if(str_len > str->size-str_off)
        str_len = str->size-str_off;

    return basic_string_wchar_replace_cstr_len(this, off, len,
            str->ptr+str_off, str_len);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIABV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace(basic_string_wchar *this,
        size_t off, size_t len, const basic_string_wchar *str)
{
    return basic_string_wchar_replace_cstr_len(this, off, len,
            str->ptr, str->size);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIIG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K00G@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_ch, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_ch(basic_string_wchar *this,
        size_t off, size_t len, size_t count, wchar_t ch)
{
    wchar_t *ptr;

    TRACE("%p %Iu %Iu %Iu %c\n", this, off, len, count, ch);

    if(this->size < off)
        _Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_wchar_npos-count <= this->size-len)
        _Xlen();

    if(this->size-len+count)
        basic_string_wchar__Grow(this, this->size-len+count, FALSE);
    ptr = this->ptr;

    memmove(ptr+off+count, ptr+off+len, (this->size-off-len)*sizeof(char));
    MSVCP_char_traits_wchar_assignn(ptr+off, count, ch);
    basic_string_wchar__Eos(this, this->size-len+count);

    return this;
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIPBG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0PEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_cstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_cstr(basic_string_wchar *this,
        size_t off, size_t len, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr_len(this, off, len, str,
            MSVCP_char_traits_wchar_length(str));
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IABV12@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert, 12)
basic_string_wchar* __thiscall basic_string_wchar_insert(basic_string_wchar *this,
        size_t off, const basic_string_wchar *str)
{
    return basic_string_wchar_replace(this, off, 0, str);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IABV12@II@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KAEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_substr, 20)
basic_string_wchar* __thiscall basic_string_wchar_insert_substr(
        basic_string_wchar *this, size_t off, const basic_string_wchar *str,
        size_t str_off, size_t str_count)
{
    return basic_string_wchar_replace_substr(this, off, 0, str, str_off, str_count);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IPBG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KPEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_cstr, 12)
basic_string_wchar* __thiscall basic_string_wchar_insert_cstr(
        basic_string_wchar *this, size_t off, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr(this, off, 0, str);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IPBGI@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KPEBG0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_cstr_len, 16)
basic_string_wchar* __thiscall basic_string_wchar_insert_cstr_len(basic_string_wchar *this,
        size_t off, const wchar_t *str, size_t str_len)
{
    return basic_string_wchar_replace_cstr_len(this, off, 0, str, str_len);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0G@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_chn, 16)
basic_string_wchar* __thiscall basic_string_wchar_insert_chn(basic_string_wchar *this,
        size_t off, size_t count, wchar_t ch)
{
    return basic_string_wchar_replace_ch(this, off, 0, count, ch);
}

/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXIG@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_resize_ch, 12)
void __thiscall MSVCP_basic_string_wchar_resize_ch(
        basic_string_wchar *this, size_t size, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, size, ch);

    if(size <= this->size)
        MSVCP_basic_string_wchar_erase(this, size, this->size);
    else
        MSVCP_basic_string_wchar_append_len_ch(this, size-this->size, ch);
}

/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXI@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_resize, 8)
void __thiscall MSVCP_basic_string_wchar_resize(
        basic_string_wchar *this, size_t size)
{
    MSVCP_basic_string_wchar_resize_ch(this, size, '\0');
}

/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_c_str, 4)
const wchar_t* __thiscall MSVCP_basic_string_wchar_c_str(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->ptr ? this->ptr : basic_string_wchar__Nullstr();
}

/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_length, 4)
size_t __thiscall MSVCP_basic_string_wchar_length(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ?max_size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?max_size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_max_size, 4)
size_t __thiscall basic_string_wchar_max_size(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return MSVCP_allocator_wchar_max_size(NULL)-1;
}

/* ?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_capacity, 4)
size_t __thiscall MSVCP_basic_string_wchar_capacity(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->res;
}

/* ?reserve@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXI@Z */
/* ?reserve@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_reserve, 8)
void __thiscall MSVCP_basic_string_wchar_reserve(basic_string_wchar *this, size_t size)
{
    size_t len;

    TRACE("%p %Iu\n", this, size);

    len = this->size;
    if(len > size)
        return;

    if(basic_string_wchar__Grow(this, size, FALSE))
        basic_string_wchar__Eos(this, len);
}

/* ?empty@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE_NXZ */
/* ?empty@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_empty, 4)
bool __thiscall MSVCP_basic_string_wchar_empty(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->size == 0;
}

/* ?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_swap, 8)
void __thiscall basic_string_wchar_swap(basic_string_wchar *this, basic_string_wchar *str)
{
    basic_string_wchar tmp;
    TRACE("%p %p\n", this, str);

    tmp = *this;
    *this = *str;
    *str = tmp;
}

/* ?substr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV12@II@Z */
/* ?substr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_substr, 16)
basic_string_wchar* __thiscall basic_string_wchar_substr(basic_string_wchar *this,
        basic_string_wchar *ret, size_t off, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, off, len);

    MSVCP_basic_string_wchar_ctor_substr_alloc(ret, this, off, len, NULL);
    return ret;
}

/* ?copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPAGII@Z */
/* ?copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEAG_K1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_copy, 16)
size_t __thiscall basic_string_wchar_copy(const basic_string_wchar *this,
        wchar_t *dest, size_t count, size_t off)
{
    TRACE("%p %p %Iu %Iu\n", this, dest, count, off);

    if(off > this->size)
        _Xran();
    if(count > this->size-off)
        count = this->size-off;
    if(this->ptr)
        MSVCP_char_traits_wchar__Copy_s(dest, count, this->ptr+off, count);
    return count;
}

/* ?get_allocator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$allocator@G@2@XZ */
/* ?get_allocator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$allocator@G@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_get_allocator, 8)
allocator* __thiscall basic_string_wchar_get_allocator(const basic_string_wchar *this, allocator *ret)
{
    TRACE("%p\n", this);
    return ret;
}

/* Old iterator functions */

/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAIPBD0@Z */
/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CA_KPEBD0@Z */
size_t __cdecl basic_string_char__Pdif(const char *i1, const char *i2)
{
    TRACE("(%p %p)\n", i1, i2);
    return !i1 ? 0 : i1-i2;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEPADPAD0@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAPEADPEAD0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_erase_iter, 12)
char* __thiscall basic_string_char_erase_iter(basic_string_char *this, char *beg, char *end)
{
    size_t pos = basic_string_char__Pdif(beg, this->ptr);
    MSVCP_basic_string_char_erase(this, pos, basic_string_char__Pdif(end, beg));
    return basic_string_char__Psum(this->ptr, pos);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PAD0PBD1@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEAD0PEBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_iter, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_iter(basic_string_char *this,
        char *beg1, char *end1, const char *beg2, const char *end2)
{
    return basic_string_char_replace_cstr_len(this, basic_string_char__Pdif(beg1, this->ptr),
            basic_string_char__Pdif(end1, beg1), beg2, basic_string_char__Pdif(end2, beg2));
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PAD0ABV12@@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEAD0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_bstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_bstr(basic_string_char *this,
        char *beg, char *end, const basic_string_char *str)
{
    return basic_string_char_replace(this, basic_string_char__Pdif(beg, this->ptr),
            basic_string_char__Pdif(end, beg), str);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PAD0ID@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEAD0_KD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_chn, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_chn(basic_string_char *this,
        char *beg, char *end, size_t count, char ch)
{
    return basic_string_char_replace_ch(this, basic_string_char__Pdif(beg, this->ptr),
            basic_string_char__Pdif(end, beg), count, ch);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PAD0PBD@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEAD0PEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr(basic_string_char *this,
        char *beg, char *end, const char *str)
{
    return basic_string_char_replace_cstr(this, basic_string_char__Pdif(beg, this->ptr),
            basic_string_char__Pdif(end, beg), str);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PAD0PBDI@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEAD0PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr_len, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr_len(basic_string_char *this,
        char *beg, char *end, const char *str, size_t len)
{
    return basic_string_char_replace_cstr_len(this, basic_string_char__Pdif(beg, this->ptr),
            basic_string_char__Pdif(end, beg), str, len);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXPADID@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXPEAD_KD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_chn, 16)
void __thiscall basic_string_char_insert_iter_chn(basic_string_char *this,
        char *pos, size_t n, char ch)
{
    basic_string_char_insert_chn(this, basic_string_char__Pdif(pos, this->ptr), n, ch);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEPADPADD@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAPEADPEADD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_ch, 12)
char* __thiscall basic_string_char_insert_iter_ch(basic_string_char *this, char *pos, char ch)
{
    size_t off = basic_string_char__Pdif(pos, this->ptr);
    basic_string_char_insert_chn(this, off, 1, ch);
    return basic_string_char__Psum(this->ptr, off);
}

/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEPADXZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAPEADXZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_begin, 4)
char* __thiscall basic_string_char_begin(basic_string_char *this)
{
    TRACE("(%p)\n", this);
    basic_string_char__Freeze(this);
    return this->ptr;
}

/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEPADXZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAPEADXZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_end, 4)
char* __thiscall basic_string_char_end(basic_string_char *this)
{
    TRACE("(%p)\n", this);
    basic_string_char__Freeze(this);
    return this->ptr+this->size;
}

/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@PADDAADPADH@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@PEADDAEADPEAD_J@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@PBDDABDPBDH@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@PEBDDAEBDPEBD_J@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_rbegin, 8)
String_reverse_iterator_char* __thiscall basic_string_char_rbegin(
        basic_string_char *this, String_reverse_iterator_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    ret->ptr = basic_string_char_end(this);
    return ret;
}

/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@PADDAADPADH@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@PEADDAEADPEAD_J@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@PBDDABDPBDH@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@PEBDDAEBDPEBD_J@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_rend, 8)
String_reverse_iterator_char* __thiscall basic_string_char_rend(
        basic_string_char *this, String_reverse_iterator_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    ret->ptr = basic_string_char_begin(this);
    return ret;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD0ABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD0AEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_ctor_iter, 16)
basic_string_char* __thiscall basic_string_char_ctor_iter(basic_string_char *this,
        const char *first, const char *last, allocator *alloc)
{
    TRACE("(%p %p %p %p)\n", this, first, last, alloc);

    basic_string_char__Tidy(this, FALSE);
    MSVCP_basic_string_char_assign_cstr_len(this, first, basic_string_char__Pdif(last, first));
    return this;
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXPADPBD1@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXPEADPEBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter, 16)
void __thiscall basic_string_char_insert_iter(basic_string_char *this,
        char *pos, const char *beg, const char *end)
{
    basic_string_char_insert_cstr_len(this, basic_string_char__Pdif(pos, this->ptr),
            beg, basic_string_char__Pdif(end, beg));
}

/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAIPBG0@Z */
/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CA_KPEBG0@Z */
size_t __cdecl basic_string_wchar__Pdif(const wchar_t *i1, const wchar_t *i2)
{
    TRACE("(%p %p)\n", i1, i2);
    return !i1 ? 0 : i1-i2;
}

/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEPAGPAG0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAPEAGPEAG0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_erase_iter, 12)
wchar_t* __thiscall basic_string_wchar_erase_iter(basic_string_wchar *this, wchar_t *beg, wchar_t *end)
{
    size_t pos = basic_string_wchar__Pdif(beg, this->ptr);
    MSVCP_basic_string_wchar_erase(this, pos, basic_string_wchar__Pdif(end, beg));
    return basic_string_wchar__Psum(this->ptr, pos);
}

/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBG0ABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG0AEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_ctor_iter, 16)
basic_string_wchar* __thiscall basic_string_wchar_ctor_iter(basic_string_wchar *this,
        const wchar_t *first, const wchar_t *last, allocator *alloc)
{
    TRACE("(%p %p %p %p)\n", this, first, last, alloc);

    basic_string_wchar__Tidy(this, FALSE);
    MSVCP_basic_string_wchar_assign_cstr_len(this, first, basic_string_wchar__Pdif(last, first));
    return this;
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PAG0PBG1@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEAG0PEBG1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_iter, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_iter(basic_string_wchar *this,
        wchar_t *beg1, wchar_t *end1, const wchar_t *beg2, const wchar_t *end2)
{
    return basic_string_wchar_replace_cstr_len(this, basic_string_wchar__Pdif(beg1, this->ptr),
            basic_string_wchar__Pdif(end1, beg1), beg2, basic_string_wchar__Pdif(end2, beg2));
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PAG0ABV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEAG0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_bstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_bstr(basic_string_wchar *this,
        wchar_t *beg, wchar_t *end, const basic_string_wchar *str)
{
    return basic_string_wchar_replace(this, basic_string_wchar__Pdif(beg, this->ptr),
            basic_string_wchar__Pdif(end, beg), str);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PAG0IG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEAG0_KG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_chn, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_chn(basic_string_wchar *this,
        wchar_t *beg, wchar_t *end, size_t count, wchar_t ch)
{
    return basic_string_wchar_replace_ch(this, basic_string_wchar__Pdif(beg, this->ptr),
            basic_string_wchar__Pdif(end, beg), count, ch);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PAG0PBG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEAG0PEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr(basic_string_wchar *this,
        wchar_t *beg, wchar_t *end, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr(this, basic_string_wchar__Pdif(beg, this->ptr),
            basic_string_wchar__Pdif(end, beg), str);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PAG0PBGI@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEAG0PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr_len, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr_len(basic_string_wchar *this,
        wchar_t *beg, wchar_t *end, const wchar_t *str, size_t len)
{
    return basic_string_wchar_replace_cstr_len(this, basic_string_wchar__Pdif(beg, this->ptr),
            basic_string_wchar__Pdif(end, beg), str, len);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXPAGIG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXPEAG_KG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_chn, 16)
void __thiscall basic_string_wchar_insert_iter_chn(basic_string_wchar *this,
        wchar_t *pos, size_t n, wchar_t ch)
{
    basic_string_wchar_insert_chn(this, basic_string_wchar__Pdif(pos, this->ptr), n, ch);
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEPAGPAGG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAPEAGPEAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_ch, 12)
wchar_t* __thiscall basic_string_wchar_insert_iter_ch(basic_string_wchar *this, wchar_t *pos, wchar_t ch)
{
    size_t off = basic_string_wchar__Pdif(pos, this->ptr);
    basic_string_wchar_insert_chn(this, off, 1, ch);
    return basic_string_wchar__Psum(this->ptr, off);
}

/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEPAGXZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAPEAGXZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_begin, 4)
wchar_t* __thiscall basic_string_wchar_begin(basic_string_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_string_wchar__Freeze(this);
    return this->ptr;
}

/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEPAGXZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAPEAGXZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_end, 4)
wchar_t* __thiscall basic_string_wchar_end(basic_string_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_string_wchar__Freeze(this);
    return this->ptr+this->size;
}

/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@PAGGAAGPAGH@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@PEAGGAEAGPEAG_J@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@PBGGABGPBGH@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@PEBGGAEBGPEBG_J@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_rbegin, 8)
String_reverse_iterator_wchar* __thiscall basic_string_wchar_rbegin(
        basic_string_wchar *this, String_reverse_iterator_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    ret->ptr = basic_string_wchar_end(this);
    return ret;
}

/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@PAGGAAGPAGH@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@PEAGGAEAGPEAG_J@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@PBGGABGPBGH@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@PEBGGAEBGPEBG_J@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_rend, 8)
String_reverse_iterator_wchar* __thiscall basic_string_wchar_rend(
        basic_string_wchar *this, String_reverse_iterator_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    ret->ptr = basic_string_wchar_begin(this);
    return ret;
}

/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXPAGPBG1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXPEAGPEBG1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter, 16)
void __thiscall basic_string_wchar_insert_iter(basic_string_wchar *this,
        wchar_t *pos, const wchar_t *beg, const wchar_t *end)
{
    basic_string_wchar_insert_cstr_len(this, basic_string_wchar__Pdif(pos, this->ptr),
            beg, basic_string_wchar__Pdif(end, beg));
}
