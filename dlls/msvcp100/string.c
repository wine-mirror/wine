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

#include "msvcp.h"
#include "stdio.h"
#include "assert.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

/* _String_iterator<char> and _String_const_iterator<char> class */
/* char_traits<char> */
/* ?assign@?$char_traits@D@std@@SAXAADABD@Z */
/* ?assign@?$char_traits@D@std@@SAXAEADAEBD@Z */
static void MSVCP_char_traits_char_assign(char *ch, const char *assign)
{
    *ch = *assign;
}

/* ?assign@?$char_traits@D@std@@SAPADPADID@Z */
/* ?assign@?$char_traits@D@std@@SAPEADPEAD_KD@Z */
static char* MSVCP_char_traits_char_assignn(char *str, MSVCP_size_t num, char c)
{
    return memset(str, c, num);
}

/* ?length@?$char_traits@D@std@@SAIPBD@Z */
/* ?length@?$char_traits@D@std@@SA_KPEBD@Z */
static MSVCP_size_t MSVCP_char_traits_char_length(const char *str)
{
    return strlen(str);
}

/* ?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Copy_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
static char* MSVCP_char_traits_char__Copy_s(char *dest,
        MSVCP_size_t size, const char *src, MSVCP_size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memcpy(dest, src, count);
}

/* ?_Move_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Move_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
static char* MSVCP_char_traits_char__Move_s(char *dest,
        MSVCP_size_t size, const char *src, MSVCP_size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memmove(dest, src, count);
}

/* char_traits<wchar_t> */
/* ?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z */
/* ?assign@?$char_traits@_W@std@@SAXAEA_WAEB_W@Z */
static void MSVCP_char_traits_wchar_assign(wchar_t *ch,
        const wchar_t *assign)
{
    *ch = *assign;
}

/* ?length@?$char_traits@_W@std@@SAIPB_W@Z */
/* ?length@?$char_traits@_W@std@@SA_KPEB_W@Z */
static MSVCP_size_t MSVCP_char_traits_wchar_length(const wchar_t *str)
{
    return wcslen((WCHAR*)str);
}

/* ?_Copy_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Copy_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
static wchar_t* MSVCP_char_traits_wchar__Copy_s(wchar_t *dest,
        MSVCP_size_t size, const wchar_t *src, MSVCP_size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memcpy(dest, src, count * sizeof(wchar_t));
}

/* ?_Move_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Move_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
static wchar_t* MSVCP_char_traits_wchar__Move_s(wchar_t *dest,
        MSVCP_size_t size, const wchar_t *src, MSVCP_size_t count)
{
    if(!dest || !src || size<count) {
        if(dest && size)
            dest[0] = '\0';
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return dest;
    }

    return memmove(dest, src, count * sizeof(WCHAR));
}

/* _String_base */
/* ?_Xlen@_String_base@std@@SAXXZ */
static void MSVCP__String_base_Xlen(void)
{
    static const char msg[] = "string too long";

    TRACE("\n");
    throw_exception(EXCEPTION_LENGTH_ERROR, msg);
}

/* _String_base */
/* ?_Xran@_String_base@std@@SAXXZ */
static void MSVCP__String_base_Xran(void)
{
    static const char msg[] = "invalid string position";

    TRACE("\n");
    throw_exception(EXCEPTION_OUT_OF_RANGE, msg);
}


/* basic_string<char, char_traits<char>, allocator<char>> */
/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2IB */
/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2_KB */
static const MSVCP_size_t MSVCP_basic_string_char_npos = -1;

/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEPADXZ */
/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAPEADXZ */
static char* basic_string_char_ptr(basic_string_char *this)
{
    if(this->res < BUF_SIZE_CHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IBEPBDXZ */
/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEBAPEBDXZ */
static const char* basic_string_char_const_ptr(const basic_string_char *this)
{
    if(this->res < BUF_SIZE_CHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_K@Z */
static void basic_string_char_eos(basic_string_char *this, MSVCP_size_t len)
{
    static const char nullbyte = '\0';

    this->size = len;
    MSVCP_char_traits_char_assign(basic_string_char_ptr(this)+len, &nullbyte);
}

/* ?_Inside@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAE_NPBD@Z */
/* ?_Inside@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAA_NPEBD@Z */
static MSVCP_bool basic_string_char_inside(
        basic_string_char *this, const char *ptr)
{
    char *cstr = basic_string_char_ptr(this);

    return ptr>=cstr && ptr<cstr+this->size;
}

/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_N_K@Z */
static void basic_string_char_tidy(basic_string_char *this,
        MSVCP_bool built, MSVCP_size_t new_size)
{
    if(built && BUF_SIZE_CHAR<=this->res) {
        char *ptr = this->data.ptr;

        if(new_size > 0)
            MSVCP_char_traits_char__Copy_s(this->data.buf, BUF_SIZE_CHAR, ptr, new_size);
        MSVCP_allocator_char_deallocate(this->allocator, ptr, this->res+1);
    }

    this->res = BUF_SIZE_CHAR-1;
    basic_string_char_eos(this, new_size);
}

/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAA_N_K_N@Z */
static MSVCP_bool basic_string_char_grow(
        basic_string_char *this, MSVCP_size_t new_size, MSVCP_bool trim)
{
    if(this->res < new_size) {
        MSVCP_size_t new_res = new_size, len = this->size;
        char *ptr;

        new_res |= 0xf;

        if(new_res/3 < this->res/2)
            new_res = this->res + this->res/2;

        ptr = MSVCP_allocator_char_allocate(this->allocator, new_res+1);
        if(!ptr)
            ptr = MSVCP_allocator_char_allocate(this->allocator, new_size+1);
        else
            new_size = new_res;
        if(!ptr) {
            ERR("Out of memory\n");
            basic_string_char_tidy(this, TRUE, 0);
            return FALSE;
        }

        MSVCP_char_traits_char__Copy_s(ptr, new_size,
                basic_string_char_ptr(this), this->size);
        basic_string_char_tidy(this, TRUE, 0);
        this->data.ptr = ptr;
        this->res = new_size;
        basic_string_char_eos(this, len);
    } else if(trim && new_size < BUF_SIZE_CHAR)
        basic_string_char_tidy(this, TRUE,
                new_size<this->size ? new_size : this->size);
    else if(new_size == 0)
        basic_string_char_eos(this, 0);

    return (new_size>0);
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0@Z */
static basic_string_char* MSVCP_basic_string_char_erase(
        basic_string_char *this, MSVCP_size_t pos, MSVCP_size_t len)
{
    TRACE("%p %lu %lu\n", this, pos, len);

    if(pos > this->size)
        MSVCP__String_base_Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        MSVCP_char_traits_char__Move_s(basic_string_char_ptr(this)+pos,
                this->res-pos, basic_string_char_ptr(this)+pos+len,
                this->size-pos-len);
        basic_string_char_eos(this, this->size-len);
    }

    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
static basic_string_char* MSVCP_basic_string_char_assign_substr(
        basic_string_char *this, const basic_string_char *assign,
        MSVCP_size_t pos, MSVCP_size_t len)
{
    TRACE("%p %p %lu %lu\n", this, assign, pos, len);

    if(assign->size < pos)
        MSVCP__String_base_Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_char_erase(this, pos+len, MSVCP_basic_string_char_npos);
        MSVCP_basic_string_char_erase(this, 0, pos);
    } else if(basic_string_char_grow(this, len, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this),
                this->res, basic_string_char_const_ptr(assign)+pos, len);
        basic_string_char_eos(this, len);
    }

    return this;
}

/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@AEBV01@@Z */
static basic_string_char* MSVCP_basic_string_char_assign(
        basic_string_char *this, const basic_string_char *assign)
{
    return MSVCP_basic_string_char_assign_substr(this, assign,
            0, MSVCP_basic_string_char_npos);
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z */
static basic_string_char* MSVCP_basic_string_char_assign_cstr_len(
        basic_string_char *this, const char *str, MSVCP_size_t len)
{
    TRACE("%p %s %lu\n", this, debugstr_an(str, len), len);

    if(basic_string_char_inside(this, str))
        return MSVCP_basic_string_char_assign_substr(this, this,
                str-basic_string_char_ptr(this), len);
    else if(basic_string_char_grow(this, len, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this),
                this->res, str, len);
        basic_string_char_eos(this, len);
    }

    return this;
}

/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@PEBD@Z */
static basic_string_char* MSVCP_basic_string_char_assign_cstr(
        basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, str,
            MSVCP_char_traits_char_length(str));
}

/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
const char* MSVCP_basic_string_char_c_str(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return basic_string_char_const_ptr(this);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@@Z */
basic_string_char* MSVCP_basic_string_char_copy_ctor(
    basic_string_char *this, const basic_string_char *copy)
{
    TRACE("%p %p\n", this, copy);

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign(this, copy);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ */
basic_string_char* MSVCP_basic_string_char_ctor(basic_string_char *this)
{
    TRACE("%p\n", this);

    basic_string_char_tidy(this, FALSE, 0);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD@Z */
basic_string_char* MSVCP_basic_string_char_ctor_cstr(
        basic_string_char *this, const char *str)
{
    TRACE("%p %s\n", this, debugstr_a(str));

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign_cstr(this, str);
    return this;
}

/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ */
void MSVCP_basic_string_char_dtor(basic_string_char *this)
{
    TRACE("%p\n", this);
    basic_string_char_tidy(this, TRUE, 0);
}

/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
MSVCP_size_t MSVCP_basic_string_char_length(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ID@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KD@Z */
static basic_string_char* MSVCP_basic_string_char_append_len_ch(basic_string_char *this, MSVCP_size_t count, char ch)
{
    TRACE("%p %lu %c\n", this, count, ch);

    if(MSVCP_basic_string_char_npos-this->size <= count)
        MSVCP__String_base_Xlen();

    if(basic_string_char_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char_assignn(basic_string_char_ptr(this)+this->size, count, ch);
        basic_string_char_eos(this, this->size+count);
    }

    return this;
}

/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@D@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@D@Z */
/* ?push_back@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXD@Z */
/* ?push_back@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXD@Z */
basic_string_char* MSVCP_basic_string_char_append_ch(basic_string_char *this, char ch)
{
    return MSVCP_basic_string_char_append_len_ch(this, 1, ch);
}

/* basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> */
/* basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short>> */
/* ?npos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@2IB */
/* ?npos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@2_KB */
/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2IB */
/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2_KB */
const MSVCP_size_t MSVCP_basic_string_wchar_npos = -1;

/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEPA_WXZ */
/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAPEA_WXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEPAGXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAPEAGXZ */
static wchar_t* basic_string_wchar_ptr(basic_string_wchar *this)
{
    if(this->res < BUF_SIZE_WCHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IBEPB_WXZ */
/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEBAPEB_WXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IBEPBGXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEBAPEBGXZ */
static const wchar_t* basic_string_wchar_const_ptr(const basic_string_wchar *this)
{
    if(this->res < BUF_SIZE_WCHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Eos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_K@Z */
/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_K@Z */
static void basic_string_wchar_eos(basic_string_wchar *this, MSVCP_size_t len)
{
    static const wchar_t nullbyte_w = '\0';

    this->size = len;
    MSVCP_char_traits_wchar_assign(basic_string_wchar_ptr(this)+len, &nullbyte_w);
}

/* ?_Inside@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAE_NPB_W@Z */
/* ?_Inside@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAA_NPEB_W@Z */
/* ?_Inside@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAE_NPBG@Z */
/* ?_Inside@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAA_NPEBG@Z */
static MSVCP_bool basic_string_wchar_inside(
        basic_string_wchar *this, const wchar_t *ptr)
{
    wchar_t *cstr = basic_string_wchar_ptr(this);

    return ptr>=cstr && ptr<cstr+this->size;
}

/* ?_Tidy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_N_K@Z */
/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_N_K@Z */
static void basic_string_wchar_tidy(basic_string_wchar *this,
        MSVCP_bool built, MSVCP_size_t new_size)
{
    if(built && BUF_SIZE_WCHAR<=this->res) {
        wchar_t *ptr = this->data.ptr;

        if(new_size > 0)
            MSVCP_char_traits_wchar__Copy_s(this->data.buf, BUF_SIZE_WCHAR, ptr, new_size);
        MSVCP_allocator_wchar_deallocate(this->allocator, ptr, this->res+1);
    }

    this->res = BUF_SIZE_WCHAR-1;
    basic_string_wchar_eos(this, new_size);
}

/* ?_Grow@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAA_N_K_N@Z */
/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAA_N_K_N@Z */
static MSVCP_bool basic_string_wchar_grow(
        basic_string_wchar *this, MSVCP_size_t new_size, MSVCP_bool trim)
{
    if(this->res < new_size) {
        MSVCP_size_t new_res = new_size, len = this->size;
        wchar_t *ptr;

        new_res |= 0xf;

        if(new_res/3 < this->res/2)
            new_res = this->res + this->res/2;

        ptr = MSVCP_allocator_wchar_allocate(this->allocator, new_res+1);
        if(!ptr)
            ptr = MSVCP_allocator_wchar_allocate(this->allocator, new_size+1);
        else
            new_size = new_res;
        if(!ptr) {
            ERR("Out of memory\n");
            basic_string_wchar_tidy(this, TRUE, 0);
            return FALSE;
        }

        MSVCP_char_traits_wchar__Copy_s(ptr, new_size,
                basic_string_wchar_ptr(this), this->size);
        basic_string_wchar_tidy(this, TRUE, 0);
        this->data.ptr = ptr;
        this->res = new_size;
        basic_string_wchar_eos(this, len);
    } else if(trim && new_size < BUF_SIZE_WCHAR)
        basic_string_wchar_tidy(this, TRUE,
                new_size<this->size ? new_size : this->size);
    else if(new_size == 0)
        basic_string_wchar_eos(this, 0);

    return (new_size>0);
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0@Z */
static basic_string_wchar* MSVCP_basic_string_wchar_erase(
            basic_string_wchar *this, MSVCP_size_t pos, MSVCP_size_t len)
{
    TRACE("%p %lu %lu\n", this, pos, len);

    if(pos > this->size)
        MSVCP__String_base_Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        MSVCP_char_traits_wchar__Move_s(basic_string_wchar_ptr(this)+pos,
                this->res-pos, basic_string_wchar_ptr(this)+pos+len,
                this->size-pos-len);
        basic_string_wchar_eos(this, this->size-len);
    }

    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
static basic_string_wchar* MSVCP_basic_string_wchar_assign_substr(
            basic_string_wchar *this, const basic_string_wchar *assign,
            MSVCP_size_t pos, MSVCP_size_t len)
{
    TRACE("%p %p %lu %lu\n", this, assign, pos, len);

    if(assign->size < pos)
        MSVCP__String_base_Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_wchar_erase(this, pos+len, MSVCP_basic_string_wchar_npos);
        MSVCP_basic_string_wchar_erase(this, 0, pos);
    } else if(basic_string_wchar_grow(this, len, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this),
                this->res, basic_string_wchar_const_ptr(assign)+pos, len);
        basic_string_wchar_eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_WI@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W_K@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z */
static basic_string_wchar* MSVCP_basic_string_wchar_assign_cstr_len(
            basic_string_wchar *this, const wchar_t *str, MSVCP_size_t len)
{
    TRACE("%p %s %lu\n", this, debugstr_wn(str, len), len);

    if(basic_string_wchar_inside(this, str))
        return MSVCP_basic_string_wchar_assign_substr(this, this,
                str-basic_string_wchar_ptr(this), len);
    else if(basic_string_wchar_grow(this, len, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this),
                this->res, str, len);
        basic_string_wchar_eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_W@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@PB_W@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@PEB_W@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@PBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@PEBG@Z */
static basic_string_wchar* MSVCP_basic_string_wchar_assign_cstr(
            basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, str,
            MSVCP_char_traits_wchar_length(str));
}

/* ?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ */
/* ?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ */
/* ?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ */
/* ?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ */
/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
const wchar_t* MSVCP_basic_string_wchar_c_str(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return basic_string_wchar_const_ptr(this);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_W@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBG@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG@Z */
basic_string_wchar* MSVCP_basic_string_wchar_ctor_cstr(
            basic_string_wchar *this, const wchar_t *str)
{
    TRACE("%p %s\n", this, debugstr_w(str));

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assign_cstr(this, str);
    return this;
}

/* ??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@XZ */
/* ??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@XZ */
/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ */
/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ */
void MSVCP_basic_string_wchar_dtor(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    basic_string_wchar_tidy(this, TRUE, 0);
}

/* ?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?length@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
MSVCP_size_t MSVCP_basic_string_wchar_length(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ??0?$_Yarn@D@std@@QAE@XZ */
/* ??0?$_Yarn@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_ctor, 4)
_Yarn_char* __thiscall _Yarn_char_ctor(_Yarn_char *this)
{
    TRACE("(%p)\n", this);

    this->str = NULL;
    this->null_str = '\0';
    return this;
}

/* ?_Tidy@?$_Yarn@D@std@@AAEXXZ */
/* ?_Tidy@?$_Yarn@D@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char__Tidy, 4)
void __thiscall _Yarn_char__Tidy(_Yarn_char *this)
{
    TRACE("(%p)\n", this);

    if(this->str)
        MSVCRT_operator_delete(this->str);
    this->str = NULL;
}

/* ??4?$_Yarn@D@std@@QAEAAV01@PBD@Z */
/* ??4?$_Yarn@D@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_op_assign_cstr, 8)
_Yarn_char* __thiscall _Yarn_char_op_assign_cstr(_Yarn_char *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);

    _Yarn_char__Tidy(this);

    if(str) {
        MSVCP_size_t len = strlen(str);

        this->str = MSVCRT_operator_new((len+1)*sizeof(char));
        if(!this->str) {
            ERR("out of memory\n");
            return NULL;
        }
        memcpy(this->str, str, (len+1)*sizeof(char));
    }
    return this;
}

/* ??0?$_Yarn@D@std@@QAE@PBD@Z */
/* ??0?$_Yarn@D@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_ctor_cstr, 8)
_Yarn_char* __thiscall _Yarn_char_ctor_cstr(_Yarn_char *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);

    _Yarn_char_ctor(this);
    return _Yarn_char_op_assign_cstr(this, str);
}

/* ??4?$_Yarn@D@std@@QAEAAV01@ABV01@@Z */
/* ??4?$_Yarn@D@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_op_assign, 8)
_Yarn_char* __thiscall _Yarn_char_op_assign(_Yarn_char *this, const _Yarn_char *rhs)
{
    TRACE("(%p %p)\n", this, rhs);

    return _Yarn_char_op_assign_cstr(this, rhs->str);
}

/* ??0?$_Yarn@D@std@@QAE@ABV01@@Z */
/* ??0?$_Yarn@D@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_copy_ctor, 8)
_Yarn_char* __thiscall _Yarn_char_copy_ctor(_Yarn_char *this, const _Yarn_char *copy)
{
    TRACE("(%p %p)\n", this, copy);

    _Yarn_char_ctor(this);
    return _Yarn_char_op_assign(this, copy);
}

/* ??1?$_Yarn@D@std@@QAE@XZ */
/* ??1?$_Yarn@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_dtor, 4)
void __thiscall _Yarn_char_dtor(_Yarn_char *this)
{
    TRACE("(%p)\n", this);
    _Yarn_char__Tidy(this);
}

/* ?_C_str@?$_Yarn@D@std@@QBEPBDXZ */
/* ?_C_str@?$_Yarn@D@std@@QEBAPEBDXZ */
/* ?c_str@?$_Yarn@D@std@@QBEPBDXZ */
/* ?c_str@?$_Yarn@D@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_c_str, 4)
const char* __thiscall _Yarn_char_c_str(const _Yarn_char *this)
{
    TRACE("(%p)\n", this);
    return this->str ? this->str : &this->null_str;
}

/* ?_Empty@?$_Yarn@D@std@@QBE_NXZ */
/* ?_Empty@?$_Yarn@D@std@@QEBA_NXZ */
/* ?empty@?$_Yarn@D@std@@QBE_NXZ */
/* ?empty@?$_Yarn@D@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_empty, 4)
MSVCP_bool __thiscall _Yarn_char_empty(const _Yarn_char *this)
{
    TRACE("(%p)\n", this);
    return !this->str;
}
