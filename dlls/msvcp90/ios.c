/*
 * Copyright 2011 Piotr Caban for CodeWeavers
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
#include <stdio.h>
#include <limits.h>
#include <share.h>

#include "msvcp90.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

/* ?_Index@ios_base@std@@0HA */
int ios_base_Index = 0;
/* ?_Sync@ios_base@std@@0_NA */
MSVCP_bool ios_base_Sync = FALSE;

typedef struct {
    streamoff off;
    __int64 DECLSPEC_ALIGN(8) pos;
    int state;
} fpos_int;

static inline const char* debugstr_fpos_int(fpos_int *fpos)
{
    return wine_dbg_sprintf("fpos(%ld %s %d)", fpos->off, wine_dbgstr_longlong(fpos->pos), fpos->state);
}

typedef struct {
    void (__cdecl *pfunc)(ios_base*, streamsize);
    streamsize arg;
} manip_streamsize;

typedef enum {
    INITFL_new   = 0,
    INITFL_open  = 1,
    INITFL_close = 2
} basic_filebuf__Initfl;

typedef struct {
    basic_streambuf_char base;
    codecvt_char *cvt;
    char putback;
    MSVCP_bool wrotesome;
    int state;
    MSVCP_bool close;
    FILE *file;
} basic_filebuf_char;

typedef enum {
    STRINGBUF_allocated = 1,
    STRINGBUF_no_write = 2,
    STRINGBUF_no_read = 4,
    STRINGBUF_append = 8,
    STRINGBUF_at_end = 16
} basic_stringbuf_state;

typedef struct {
    basic_streambuf_char base;
    char *seekhigh;
    int state;
    char allocator; /* empty struct */
} basic_stringbuf_char;

typedef struct {
    basic_streambuf_wchar base;
    wchar_t *seekhigh;
    int state;
    char allocator; /* empty struct */
} basic_stringbuf_wchar;

typedef struct {
    ios_base base;
    basic_streambuf_char *strbuf;
    struct _basic_ostream_char *stream;
    char fillch;
} basic_ios_char;

typedef struct {
    ios_base base;
    basic_streambuf_wchar *strbuf;
    struct _basic_ostream_wchar *stream;
    wchar_t fillch;
} basic_ios_wchar;

typedef struct _basic_ostream_char {
    const int *vbtable;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ostream_char;

typedef struct _basic_ostream_wchar {
    const int *vbtable;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ostream_wchar;

typedef struct {
    const int *vbtable;
    streamsize count;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_istream_char;

typedef struct {
    const int *vbtable;
    streamsize count;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_istream_wchar;

typedef struct {
    basic_istream_char base1;
    basic_ostream_char base2;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_iostream_char;

typedef struct {
    basic_istream_wchar base1;
    basic_ostream_wchar base2;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_iostream_wchar;

typedef struct {
    basic_ostream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ofstream_char;

typedef struct {
    basic_istream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ifstream_char;

typedef struct {
    basic_iostream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_fstream_char;

typedef struct {
    basic_ostream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ostringstream_char;

typedef struct {
    basic_ostream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ostringstream_wchar;

typedef struct {
    basic_istream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_istringstream_char;

typedef struct {
    basic_istream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_istringstream_wchar;

typedef struct {
    basic_iostream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_stringstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_stringstream_wchar;

extern const vtable_ptr MSVCP_iosb_vtable;

/* ??_7ios_base@std@@6B@ */
extern const vtable_ptr MSVCP_ios_base_vtable;

/* ??_7?$basic_ios@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ios_char_vtable;

/* ??_7?$basic_ios@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ios_wchar_vtable;

/* ??_7?$basic_ios@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ios_short_vtable;

/* ??_7?$basic_streambuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_streambuf_char_vtable;

/* ??_7?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_streambuf_wchar_vtable;

/* ??_7?$basic_streambuf@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_streambuf_short_vtable;

/* ??_7?$basic_filebuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_filebuf_char_vtable;

/* ??_7?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_stringbuf_char_vtable;

/* ??_7?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_stringbuf_wchar_vtable;

/* ??_7?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_stringbuf_short_vtable;

/* ??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ostream_char_vbtable[] = {0, sizeof(basic_ostream_char)};
/* ??_7?$basic_ostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ostream_char_vtable;

/* ??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_ostream_wchar_vbtable[] = {0, sizeof(basic_ostream_wchar)};
/* ??_7?$basic_ostream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ostream_wchar_vtable;

/* ??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_istream_char_vbtable[] = {0, sizeof(basic_istream_char)};
/* ??_7?$basic_istream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_istream_char_vtable;

/* ??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_istream_wchar_vbtable[] = {0, sizeof(basic_istream_wchar)};
/* ??_7?$basic_istream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_istream_wchar_vtable;

/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable1[] = {0, sizeof(basic_iostream_char)};
/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable2[] = {0, sizeof(basic_iostream_char)-FIELD_OFFSET(basic_iostream_char, base2)};
/* ??_7?$basic_iostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_iostream_char_vtable;

/* ??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_iostream_wchar_vbtable1[] = {0, sizeof(basic_iostream_wchar)};
/* ??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_iostream_wchar_vbtable2[] = {0, sizeof(basic_iostream_wchar)-FIELD_OFFSET(basic_iostream_wchar, base2)};
/* ??_7?$basic_iostream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_iostream_wchar_vtable;

/* ??_8?$basic_ofstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ofstream_char_vbtable[] = {0, sizeof(basic_ofstream_char)};
/* ??_7?$basic_ofstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ofstream_char_vtable;

/*??_8?$basic_ifstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ifstream_char_vbtable[] = {0, sizeof(basic_ifstream_char)};
/* ??_7?$basic_ifstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ifstream_char_vtable;

/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable1[] = {0, sizeof(basic_fstream_char)};
/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable2[] = {0, sizeof(basic_fstream_char)-FIELD_OFFSET(basic_fstream_char, base.base2)};
/* ??_7?$basic_fstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_fstream_char_vtable;

/* ??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_ostringstream_char_vbtable[] = {0, sizeof(basic_ostringstream_char)};
/* ??_7?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ostringstream_char_vtable;

/* ??_8?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@ */
const int basic_ostringstream_wchar_vbtable[] = {0, sizeof(basic_ostringstream_wchar)};
/* ??_7?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ostringstream_wchar_vtable;

/* ??_8?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_istringstream_char_vbtable[] = {0, sizeof(basic_istringstream_char)};
/* ??_7?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_istringstream_char_vtable;

/* ??_8?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@ */
const int basic_istringstream_wchar_vbtable[] = {0, sizeof(basic_istringstream_wchar)};
/* ??_7?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_istringstream_wchar_vtable;

/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable1[] = {0, sizeof(basic_stringstream_char)};
/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable2[] = {0, sizeof(basic_stringstream_char)-FIELD_OFFSET(basic_stringstream_char, base.base2)};
/* ??_7?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_stringstream_char_vtable;

/* ??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_stringstream_wchar_vbtable1[] = {0, sizeof(basic_stringstream_wchar)};
/* ??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_stringstream_wchar_vbtable2[] = {0, sizeof(basic_stringstream_wchar)-FIELD_OFFSET(basic_stringstream_wchar, base.base2)};
/* ??_7?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_stringstream_wchar_vtable;

DEFINE_RTTI_DATA0(iosb, 0, ".?AV?$_Iosb@H@std@@");
DEFINE_RTTI_DATA1(ios_base, 0, &iosb_rtti_base_descriptor, ".?AV?$_Iosb@H@std@@");
DEFINE_RTTI_DATA2(basic_ios_char, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA2(basic_ios_wchar, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@_WU?$char_traits@_W@std@@@std@@");
DEFINE_RTTI_DATA2(basic_ios_short, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@GU?$char_traits@G@std@@@std@@");
DEFINE_RTTI_DATA0(basic_streambuf_char, 0,
        ".?AV?$basic_streambuf@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA0(basic_streambuf_wchar, 0,
        ".?AV?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@");
DEFINE_RTTI_DATA0(basic_streambuf_short, 0,
        ".?AV?$basic_streambuf@GU?$char_traits@G@std@@@std@@");
DEFINE_RTTI_DATA1(basic_filebuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_filebuf@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA1(basic_stringbuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@");
DEFINE_RTTI_DATA1(basic_stringbuf_wchar, 0, &basic_streambuf_wchar_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@");
DEFINE_RTTI_DATA1(basic_stringbuf_short, 0, &basic_streambuf_short_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@");
DEFINE_RTTI_DATA3(basic_ostream_char, sizeof(basic_ostream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA3(basic_ostream_wchar, sizeof(basic_ostream_wchar), &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@_WU?$char_traits@_W@std@@@std@@");
DEFINE_RTTI_DATA3(basic_istream_char, sizeof(basic_istream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA3(basic_istream_wchar, sizeof(basic_istream_wchar), &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@_WU?$char_traits@_W@std@@@std@@");
DEFINE_RTTI_DATA8(basic_iostream_char, sizeof(basic_iostream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA8(basic_iostream_wchar, sizeof(basic_iostream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@_WU?$char_traits@_W@std@@@std@@");
DEFINE_RTTI_DATA4(basic_ofstream_char, sizeof(basic_ofstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA4(basic_ifstream_char, sizeof(basic_ifstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA8(basic_fstream_char, sizeof(basic_fstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@DU?$char_traits@D@std@@@std@@");
DEFINE_RTTI_DATA4(basic_ostringstream_char, sizeof(basic_ostringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@");
DEFINE_RTTI_DATA4(basic_ostringstream_wchar, sizeof(basic_ostringstream_wchar),
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@");
DEFINE_RTTI_DATA4(basic_istringstream_char, sizeof(basic_istringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@");
DEFINE_RTTI_DATA4(basic_istringstream_wchar, sizeof(basic_istringstream_wchar),
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@");
DEFINE_RTTI_DATA8(basic_stringstream_char, sizeof(basic_stringstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@");
DEFINE_RTTI_DATA8(basic_stringstream_wchar, sizeof(basic_stringstream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@");

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(iosb, "");
    __ASM_VTABLE(ios_base, "");
    __ASM_VTABLE(basic_ios_char, "");
    __ASM_VTABLE(basic_ios_wchar, "");
    __ASM_VTABLE(basic_ios_short, "");
    __ASM_VTABLE(basic_streambuf_char,
            VTABLE_ADD_FUNC(basic_streambuf_char_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_streambuf_wchar,
            VTABLE_ADD_FUNC(basic_streambuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_streambuf_short,
            VTABLE_ADD_FUNC(basic_streambuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_filebuf_char,
            VTABLE_ADD_FUNC(basic_filebuf_char_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_char_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_char_sync)
            VTABLE_ADD_FUNC(basic_filebuf_char_imbue));
    __ASM_VTABLE(basic_stringbuf_char,
            VTABLE_ADD_FUNC(basic_stringbuf_char_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_stringbuf_wchar,
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_stringbuf_short,
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_ostream_char, "");
    __ASM_VTABLE(basic_ostream_wchar, "");
    __ASM_VTABLE(basic_istream_char, "");
    __ASM_VTABLE(basic_istream_wchar, "");
    __ASM_VTABLE(basic_iostream_char, "");
    __ASM_VTABLE(basic_iostream_wchar, "");
    __ASM_VTABLE(basic_ofstream_char, "");
    __ASM_VTABLE(basic_ifstream_char, "");
    __ASM_VTABLE(basic_fstream_char, "");
    __ASM_VTABLE(basic_ostringstream_char, "");
    __ASM_VTABLE(basic_ostringstream_wchar, "");
    __ASM_VTABLE(basic_istringstream_char, "");
    __ASM_VTABLE(basic_istringstream_wchar, "");
    __ASM_VTABLE(basic_stringstream_char, "");
    __ASM_VTABLE(basic_stringstream_wchar, "");
#ifndef __GNUC__
}
#endif

/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD00@Z */
/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setp_next, 16)
void __thiscall basic_streambuf_char_setp_next(basic_streambuf_char *this, char *first, char *next, char *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->wbuf = first;
    this->wpos = next;
    this->wsize = last-next;
}

/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD0@Z */
/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD0@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setp, 12)
void __thiscall basic_streambuf_char_setp(basic_streambuf_char *this, char *first, char *last)
{
    basic_streambuf_char_setp_next(this, first, first, last);
}

/* ?setg@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD00@Z */
/* ?setg@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setg, 16)
void __thiscall basic_streambuf_char_setg(basic_streambuf_char *this, char *first, char *next, char *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->rbuf = first;
    this->rpos = next;
    this->rsize = last-next;
}

/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Init_empty, 4)
void __thiscall basic_streambuf_char__Init_empty(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    this->prbuf = &this->rbuf;
    this->pwbuf = &this->wbuf;
    this->prpos = &this->rpos;
    this->pwpos = &this->wpos;
    this->prsize = &this->rsize;
    this->pwsize = &this->wsize;

    basic_streambuf_char_setp(this, NULL, NULL);
    basic_streambuf_char_setg(this, NULL, NULL, NULL);
}

/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_ctor_uninitialized, 8)
basic_streambuf_char* __thiscall basic_streambuf_char_ctor_uninitialized(basic_streambuf_char *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    this->vtable = &MSVCP_basic_streambuf_char_vtable;
    mutex_ctor(&this->lock);
    return this;
}

/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_ctor, 4)
basic_streambuf_char* __thiscall basic_streambuf_char_ctor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &MSVCP_basic_streambuf_char_vtable;
    mutex_ctor(&this->lock);
    this->loc = MSVCRT_operator_new(sizeof(locale));
    locale_ctor(this->loc);
    basic_streambuf_char__Init_empty(this);

    return this;
}

/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_dtor, 4)
void __thiscall basic_streambuf_char_dtor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    mutex_dtor(&this->lock);
    locale_dtor(this->loc);
    MSVCRT_operator_delete(this->loc);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_streambuf_char_vector_dtor, 8)
basic_streambuf_char* __thiscall MSVCP_basic_streambuf_char_vector_dtor(basic_streambuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_streambuf_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gnavail, 4)
streamsize __thiscall basic_streambuf_char__Gnavail(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos ? *this->prsize : 0;
}

/* ?_Gndec@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gndec@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gndec, 4)
char* __thiscall basic_streambuf_char__Gndec(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)++;
    (*this->prpos)--;
    return *this->prpos;
}

/* ?_Gninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gninc, 4)
char* __thiscall basic_streambuf_char__Gninc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    return (*this->prpos)++;
}

/* ?_Gnpreinc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gnpreinc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gnpreinc, 4)
char* __thiscall basic_streambuf_char__Gnpreinc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    (*this->prpos)++;
    return *this->prpos;
}

/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAPAD0PAH001@Z */
/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAPEAD0PEAH001@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Init, 28)
void __thiscall basic_streambuf_char__Init(basic_streambuf_char *this, char **gf, char **gn, int *gc, char **pf, char **pn, int *pc)
{
    TRACE("(%p %p %p %p %p %p %p)\n", this, gf, gn, gc, pf, pn, pc);

    this->prbuf = gf;
    this->pwbuf = pf;
    this->prpos = gn;
    this->pwpos = pn;
    this->prsize = gc;
    this->pwsize = pc;
}

/* ?_Lock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Lock, 4)
void __thiscall basic_streambuf_char__Lock(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    mutex_lock(&this->lock);
}

/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Pnavail, 4)
streamsize __thiscall basic_streambuf_char__Pnavail(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos ? *this->pwsize : 0;
}

/* ?_Pninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Pninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Pninc, 4)
char* __thiscall basic_streambuf_char__Pninc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->pwsize)--;
    return (*this->pwpos)++;
}

/* ?underflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?underflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_underflow, 4)
#define call_basic_streambuf_char_underflow(this) CALL_VTBL_FUNC(this, 16, \
        int, (basic_streambuf_char*), (this))
int __thiscall basic_streambuf_char_underflow(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return EOF;
}

/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_uflow, 4)
#define call_basic_streambuf_char_uflow(this) CALL_VTBL_FUNC(this, 20, \
        int, (basic_streambuf_char*), (this))
int __thiscall basic_streambuf_char_uflow(basic_streambuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(call_basic_streambuf_char_underflow(this)==EOF)
        return EOF;

    ret = **this->prpos;
    (*this->prsize)--;
    (*this->prpos)++;
    return ret;
}

/* ?_Xsgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPADIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEAD_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Xsgetn_s, 16)
#define call_basic_streambuf_char__Xsgetn_s(this, ptr, size, count) CALL_VTBL_FUNC(this, 28, \
        streamsize, (basic_streambuf_char*, char*, MSVCP_size_t, streamsize), (this, ptr, size, count))
streamsize __thiscall basic_streambuf_char__Xsgetn_s(basic_streambuf_char *this, char *ptr, MSVCP_size_t size, streamsize count)
{
    streamsize copied, chunk;
    int c;

    TRACE("(%p %p %lu %ld)\n", this, ptr, size, count);

    for(copied=0; copied<count && size;) {
        chunk = basic_streambuf_char__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk) {
            memcpy_s(ptr+copied, size, *this->prpos, chunk);
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
            size -= chunk;
        }else if((c = call_basic_streambuf_char_uflow(this)) != EOF) {
            ptr[copied] = c;
            copied++;
            size--;
        }else {
            break;
        }
    }

    return copied;
}

/* ?_Sgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPADIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Sgetn_s, 16)
streamsize __thiscall basic_streambuf_char__Sgetn_s(basic_streambuf_char *this, char *ptr, MSVCP_size_t size, streamsize count)
{
    TRACE("(%p %p %lu %ld)\n", this, ptr, size, count);
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, size, count);
}

/* ?_Unlock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Unlock, 4)
void __thiscall basic_streambuf_char__Unlock(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    mutex_unlock(&this->lock);
}

/* ?eback@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?eback@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_eback, 4)
char* __thiscall basic_streambuf_char_eback(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prbuf;
}

/* ?gptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?gptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_gptr, 4)
char* __thiscall basic_streambuf_char_gptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos;
}

/* ?egptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?egptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_egptr, 4)
char* __thiscall basic_streambuf_char_egptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos+*this->prsize;
}

/* ?epptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?epptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_epptr, 4)
char* __thiscall basic_streambuf_char_epptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos+*this->pwsize;
}

/* ?gbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_gbump, 8)
void __thiscall basic_streambuf_char_gbump(basic_streambuf_char *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->prpos += off;
    *this->prsize -= off;
}

/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_getloc, 8)
locale* __thiscall basic_streambuf_char_getloc(const basic_streambuf_char *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, this->loc);
}

/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_imbue, 8)
#define call_basic_streambuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 52, \
        void, (basic_streambuf_char*, const locale*), (this, loc))
void __thiscall basic_streambuf_char_imbue(basic_streambuf_char *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_overflow, 8)
#define call_basic_streambuf_char_overflow(this, ch) CALL_VTBL_FUNC(this, 4, \
        int, (basic_streambuf_char*, int), (this, ch))
int __thiscall basic_streambuf_char_overflow(basic_streambuf_char *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return EOF;
}

/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbackfail, 8)
#define call_basic_streambuf_char_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, \
        int, (basic_streambuf_char*, int), (this, ch))
int __thiscall basic_streambuf_char_pbackfail(basic_streambuf_char *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return EOF;
}

/* ?pbase@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?pbase@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbase, 4)
char* __thiscall basic_streambuf_char_pbase(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwbuf;
}

/* ?pbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbump, 8)
void __thiscall basic_streambuf_char_pbump(basic_streambuf_char *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->pwpos += off;
    *this->pwsize -= off;
}

/* ?pptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?pptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pptr, 4)
char* __thiscall basic_streambuf_char_pptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos;
}

/* ?pubimbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubimbue, 12)
locale* __thiscall basic_streambuf_char_pubimbue(basic_streambuf_char *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    memcpy(ret, this->loc, sizeof(locale));
    call_basic_streambuf_char_imbue(this, loc);
    locale_copy_ctor(this->loc, loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekoff, 20)
#define call_basic_streambuf_char_seekoff(this, ret, off, way, mode) CALL_VTBL_FUNC(this, 36, \
        fpos_int*, (basic_streambuf_char*, fpos_int*, streamoff, int, int), (this, ret, off, way, mode))
fpos_int* __thiscall basic_streambuf_char_seekoff(basic_streambuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff, 20)
fpos_int* __thiscall basic_streambuf_char_pubseekoff(basic_streambuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    return call_basic_streambuf_char_seekoff(this, ret, off, way, mode);
}

/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff_old, 20)
fpos_int* __thiscall basic_streambuf_char_pubseekoff_old(basic_streambuf_char *this,
        fpos_int *ret, streamoff off, unsigned int way, unsigned int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    return basic_streambuf_char_pubseekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekpos, 36)
#define call_basic_streambuf_char_seekpos(this, ret, pos, mode) CALL_VTBL_FUNC(this, 40, \
        fpos_int*, (basic_streambuf_char*, fpos_int*, fpos_int, int), (this, ret, pos, mode))
fpos_int* __thiscall basic_streambuf_char_seekpos(basic_streambuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekpos, 36)
fpos_int* __thiscall basic_streambuf_char_pubseekpos(basic_streambuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    return call_basic_streambuf_char_seekpos(this, ret, pos, mode);
}

/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekpos_old, 36)
fpos_int* __thiscall basic_streambuf_char_pubseekpos_old(basic_streambuf_char *this,
        fpos_int *ret, fpos_int pos, unsigned int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    return basic_streambuf_char_pubseekpos(this, ret, pos, mode);
}

/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEPAV12@PADH@Z */
/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAPEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setbuf, 12)
#define call_basic_streambuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 44, \
        basic_streambuf_char*, (basic_streambuf_char*, char*, streamsize), (this, buf, count))
basic_streambuf_char* __thiscall basic_streambuf_char_setbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, buf, count);
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PADH@Z */
/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsetbuf, 12)
basic_streambuf_char* __thiscall basic_streambuf_char_pubsetbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, buf, count);
    return call_basic_streambuf_char_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sync, 4)
#define call_basic_streambuf_char_sync(this) CALL_VTBL_FUNC(this, 48, \
        int, (basic_streambuf_char*), (this))
int __thiscall basic_streambuf_char_sync(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?pubsync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsync, 4)
int __thiscall basic_streambuf_char_pubsync(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return call_basic_streambuf_char_sync(this);
}

/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetn, 12)
streamsize __thiscall basic_streambuf_char_sgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, -1, count);
}

/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_showmanyc, 4)
#define call_basic_streambuf_char_showmanyc(this) CALL_VTBL_FUNC(this, 12, \
        streamsize, (basic_streambuf_char*), (this))
streamsize __thiscall basic_streambuf_char_showmanyc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?in_avail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_in_avail, 4)
streamsize __thiscall basic_streambuf_char_in_avail(basic_streambuf_char *this)
{
    streamsize ret;

    TRACE("(%p)\n", this);

    ret = basic_streambuf_char__Gnavail(this);
    return ret ? ret : call_basic_streambuf_char_showmanyc(this);
}

/* ?sputbackc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHD@Z */
/* ?sputbackc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputbackc, 8)
int __thiscall basic_streambuf_char_sputbackc(basic_streambuf_char *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    if(*this->prpos && *this->prpos>*this->prbuf && (*this->prpos)[-1]==ch) {
        (*this->prsize)++;
        (*this->prpos)--;
        return ch;
    }

    return call_basic_streambuf_char_pbackfail(this, ch);
}

/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHD@Z */
/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputc, 8)
int __thiscall basic_streambuf_char_sputc(basic_streambuf_char *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return basic_streambuf_char__Pnavail(this) ?
        (*basic_streambuf_char__Pninc(this) = ch) :
        call_basic_streambuf_char_overflow(this, ch);
}

/* ?sungetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sungetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sungetc, 4)
int __thiscall basic_streambuf_char_sungetc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    if(*this->prpos && *this->prpos>*this->prbuf) {
        (*this->prsize)++;
        (*this->prpos)--;
        return **this->prpos;
    }

    return call_basic_streambuf_char_pbackfail(this, EOF);
}

/* ?stossc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_stossc, 4)
void __thiscall basic_streambuf_char_stossc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    if(basic_streambuf_char__Gnavail(this))
        basic_streambuf_char__Gninc(this);
    else
        call_basic_streambuf_char_uflow(this);
}

/* ?sbumpc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sbumpc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sbumpc, 4)
int __thiscall basic_streambuf_char_sbumpc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_char__Gnavail(this) ?
        *basic_streambuf_char__Gninc(this) : call_basic_streambuf_char_uflow(this);
}

/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetc, 4)
int __thiscall basic_streambuf_char_sgetc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_char__Gnavail(this) ?
        *basic_streambuf_char_gptr(this) : call_basic_streambuf_char_underflow(this);
}

/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_snextc, 4)
int __thiscall basic_streambuf_char_snextc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    if(basic_streambuf_char__Gnavail(this) > 1)
        return *basic_streambuf_char__Gnpreinc(this);
    return basic_streambuf_char_sbumpc(this)==EOF ?
        EOF : basic_streambuf_char_sgetc(this);
}

/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPADH@Z */
/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsgetn, 12)
#define call_basic_streambuf_char_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, \
        streamsize, (basic_streambuf_char*, char*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_char_xsgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, -1, count);
}

/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPBDH@Z */
/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEBD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsputn, 12)
#define call_basic_streambuf_char_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 32, \
        streamsize, (basic_streambuf_char*, const char*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_char_xsputn(basic_streambuf_char *this, const char *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %ld)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_char__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk) {
            memcpy(*this->pwpos, ptr+copied, chunk);
            *this->pwpos += chunk;
            *this->pwsize -= chunk;
            copied += chunk;
        }else if(call_basic_streambuf_char_overflow(this, ptr[copied]) != EOF) {
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPBDH@Z */
/* ?sputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEBD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputn, 12)
streamsize __thiscall basic_streambuf_char_sputn(basic_streambuf_char *this, const char *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_char_xsputn(this, ptr, count);
}

/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W00@Z */
/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W00@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG00@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setp_next, 16)
void __thiscall basic_streambuf_wchar_setp_next(basic_streambuf_wchar *this, wchar_t *first, wchar_t *next, wchar_t *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->wbuf = first;
    this->wpos = next;
    this->wsize = last-next;
}

/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W0@Z */
/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W0@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG0@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG0@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setp, 12)
void __thiscall basic_streambuf_wchar_setp(basic_streambuf_wchar *this, wchar_t *first, wchar_t *last)
{
    basic_streambuf_wchar_setp_next(this, first, first, last);
}

/* ?setg@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W00@Z */
/* ?setg@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W00@Z */
/* ?setg@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG00@Z */
/* ?setg@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setg, 16)
void __thiscall basic_streambuf_wchar_setg(basic_streambuf_wchar *this, wchar_t *first, wchar_t *next, wchar_t *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->rbuf = first;
    this->rpos = next;
    this->rsize = last-next;
}

/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXXZ */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Init_empty, 4)
void __thiscall basic_streambuf_wchar__Init_empty(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    this->prbuf = &this->rbuf;
    this->pwbuf = &this->wbuf;
    this->prpos = &this->rpos;
    this->pwpos = &this->wpos;
    this->prsize = &this->rsize;
    this->pwsize = &this->wsize;

    basic_streambuf_wchar_setp(this, NULL, NULL);
    basic_streambuf_wchar_setg(this, NULL, NULL, NULL);
}

/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_ctor_uninitialized, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_ctor_uninitialized(basic_streambuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    this->vtable = &MSVCP_basic_streambuf_wchar_vtable;
    mutex_ctor(&this->lock);
    return this;
}

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor_uninitialized, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor_uninitialized(basic_streambuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    basic_streambuf_wchar_ctor_uninitialized(this, uninitialized);
    this->vtable = &MSVCP_basic_streambuf_short_vtable;
    return this;
}

/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_ctor, 4)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_ctor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &MSVCP_basic_streambuf_wchar_vtable;
    mutex_ctor(&this->lock);
    this->loc = MSVCRT_operator_new(sizeof(locale));
    locale_ctor(this->loc);
    basic_streambuf_wchar__Init_empty(this);

    return this;
}

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor, 4)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_streambuf_wchar_ctor(this);
    this->vtable = &MSVCP_basic_streambuf_short_vtable;
    return this;
}

/* ??1?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_dtor, 4)
void __thiscall basic_streambuf_wchar_dtor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    mutex_dtor(&this->lock);
    locale_dtor(this->loc);
    MSVCRT_operator_delete(this->loc);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_streambuf_wchar_vector_dtor, 8)
basic_streambuf_wchar* __thiscall MSVCP_basic_streambuf_wchar_vector_dtor(basic_streambuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_streambuf_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_streambuf_short_vector_dtor, 8)
basic_streambuf_wchar* __thiscall MSVCP_basic_streambuf_short_vector_dtor(basic_streambuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    return MSVCP_basic_streambuf_wchar_vector_dtor(this, flags);
}

/* ?_Gnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBA_JXZ */
/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gnavail, 4)
streamsize __thiscall basic_streambuf_wchar__Gnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos ? *this->prsize : 0;
}

/* ?_Gndec@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gndec@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gndec@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gndec@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gndec, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gndec(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)++;
    (*this->prpos)--;
    return *this->prpos;
}

/* ?_Gninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    return (*this->prpos)++;
}

/* ?_Gnpreinc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gnpreinc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gnpreinc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gnpreinc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    (*this->prpos)++;
    return *this->prpos;
}

/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPAPA_W0PAH001@Z */
/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEAPEA_W0PEAH001@Z */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAPAG0PAH001@Z */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAPEAG0PEAH001@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Init, 28)
void __thiscall basic_streambuf_wchar__Init(basic_streambuf_wchar *this, wchar_t **gf, wchar_t **gn, int *gc, wchar_t **pf, wchar_t **pn, int *pc)
{
    TRACE("(%p %p %p %p %p %p %p)\n", this, gf, gn, gc, pf, pn, pc);

    this->prbuf = gf;
    this->pwbuf = pf;
    this->prpos = gn;
    this->pwpos = pn;
    this->prsize = gc;
    this->pwsize = pc;
}

/* ?_Lock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?_Lock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Lock, 4)
void __thiscall basic_streambuf_wchar__Lock(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    mutex_lock(&this->lock);
}

/* ?_Pnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBA_JXZ */
/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Pnavail, 4)
streamsize __thiscall basic_streambuf_wchar__Pnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos ? *this->pwsize : 0;
}

/* ?_Pninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Pninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Pninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Pninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->pwsize)--;
    return (*this->pwpos)++;
}

/* ?underflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_underflow, 4)
#define call_basic_streambuf_wchar_underflow(this) CALL_VTBL_FUNC(this, 16, \
        unsigned short, (basic_streambuf_wchar*), (this))
unsigned short __thiscall basic_streambuf_wchar_underflow(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return WEOF;
}

/* ?uflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_uflow, 4)
#define call_basic_streambuf_wchar_uflow(this) CALL_VTBL_FUNC(this, 20, \
        unsigned short, (basic_streambuf_wchar*), (this))
unsigned short __thiscall basic_streambuf_wchar_uflow(basic_streambuf_wchar *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(call_basic_streambuf_wchar_underflow(this)==WEOF)
        return WEOF;

    ret = **this->prpos;
    (*this->prsize)--;
    (*this->prpos)++;
    return ret;
}

/* ?_Xsgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPA_WIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEA_W_K_J@Z */
/* ?_Xsgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPAGIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEAG_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Xsgetn_s, 16)
#define call_basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count) CALL_VTBL_FUNC(this, 28, \
        streamsize, (basic_streambuf_wchar*, wchar_t*, MSVCP_size_t, streamsize), (this, ptr, size, count))
streamsize __thiscall basic_streambuf_wchar__Xsgetn_s(basic_streambuf_wchar *this, wchar_t *ptr, MSVCP_size_t size, streamsize count)
{
    streamsize copied, chunk;
    unsigned short c;

    TRACE("(%p %p %lu %ld)\n", this, ptr, size, count);

    for(copied=0; copied<count && size;) {
        chunk = basic_streambuf_wchar__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk) {
            memcpy_s(ptr+copied, size, *this->prpos, chunk);
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
            size -= chunk;
        }else if((c = call_basic_streambuf_wchar_uflow(this)) != WEOF) {
            ptr[copied] = c;
            copied++;
            size--;
        }else {
            break;
        }
    }

    return copied;
}

/* ?_Sgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPA_WIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_K_J@Z */
/* ?_Sgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPAGIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Sgetn_s, 16)
streamsize __thiscall basic_streambuf_wchar__Sgetn_s(basic_streambuf_wchar *this, wchar_t *ptr, MSVCP_size_t size, streamsize count)
{
    TRACE("(%p %p %lu %ld)\n", this, ptr, size, count);
    return call_basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count);
}

/* ?_Unlock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?_Unlock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Unlock, 4)
void __thiscall basic_streambuf_wchar__Unlock(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    mutex_unlock(&this->lock);
}

/* ?eback@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?eback@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_eback, 4)
wchar_t* __thiscall basic_streambuf_wchar_eback(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prbuf;
}

/* ?gptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?gptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_gptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos;
}

/* ?egptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?egptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_egptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_egptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos+*this->prsize;
}

/* ?epptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?epptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_epptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_epptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos+*this->pwsize;
}

/* ?gbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXH@Z */
/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gbump, 8)
void __thiscall basic_streambuf_wchar_gbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->prpos += off;
    *this->prsize -= off;
}

/* ?getloc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEBA?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_getloc, 8)
locale* __thiscall basic_streambuf_wchar_getloc(const basic_streambuf_wchar *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, this->loc);
}

/* ?imbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAXAEBVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_imbue, 8)
#define call_basic_streambuf_wchar_imbue(this, loc) CALL_VTBL_FUNC(this, 52, \
        void, (basic_streambuf_wchar*, const locale*), (this, loc))
void __thiscall basic_streambuf_wchar_imbue(basic_streambuf_wchar *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_overflow, 8)
#define call_basic_streambuf_wchar_overflow(this, ch) CALL_VTBL_FUNC(this, 4, \
        unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
unsigned short __thiscall basic_streambuf_wchar_overflow(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbackfail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbackfail, 8)
#define call_basic_streambuf_wchar_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, \
        unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
unsigned short __thiscall basic_streambuf_wchar_pbackfail(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbase@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?pbase@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbase, 4)
wchar_t* __thiscall basic_streambuf_wchar_pbase(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwbuf;
}

/* ?pbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXH@Z */
/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbump, 8)
void __thiscall basic_streambuf_wchar_pbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->pwpos += off;
    *this->pwsize -= off;
}

/* ?pptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?pptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_pptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos;
}

/* ?pubimbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubimbue, 12)
locale* __thiscall basic_streambuf_wchar_pubimbue(basic_streambuf_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    memcpy(ret, this->loc, sizeof(locale));
    call_basic_streambuf_wchar_imbue(this, loc);
    locale_copy_ctor(this->loc, loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekoff, 20)
#define call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode) CALL_VTBL_FUNC(this, 36, \
        fpos_int*, (basic_streambuf_wchar*, fpos_int*, streamoff, int, int), (this, ret, off, way, mode))
fpos_int* __thiscall basic_streambuf_wchar_seekoff(basic_streambuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff, 20)
fpos_int* __thiscall basic_streambuf_wchar_pubseekoff(basic_streambuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    return call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode);
}

/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff_old, 20)
fpos_int* __thiscall basic_streambuf_wchar_pubseekoff_old(basic_streambuf_wchar *this,
        fpos_int *ret, streamoff off, unsigned int way, unsigned int mode)
{
    TRACE("(%p %ld %d %d)\n", this, off, way, mode);
    return basic_streambuf_wchar_pubseekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekpos, 36)
#define call_basic_streambuf_wchar_seekpos(this, ret, pos, mode) CALL_VTBL_FUNC(this, 40, \
        fpos_int*, (basic_streambuf_wchar*, fpos_int*, fpos_int, int), (this, ret, pos, mode))
fpos_int* __thiscall basic_streambuf_wchar_seekpos(basic_streambuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekpos, 36)
fpos_int* __thiscall basic_streambuf_wchar_pubseekpos(basic_streambuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    return call_basic_streambuf_wchar_seekpos(this, ret, pos, mode);
}

/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekpos_old, 36)
fpos_int* __thiscall basic_streambuf_wchar_pubseekpos_old(basic_streambuf_wchar *this,
        fpos_int *ret, fpos_int pos, unsigned int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    return basic_streambuf_wchar_pubseekpos(this, ret, pos, mode);
}

/* ?setbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEPAV12@PA_WH@Z */
/* ?setbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAPEAV12@PEA_W_J@Z */
/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEPAV12@PAGH@Z */
/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAPEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setbuf, 12)
#define call_basic_streambuf_wchar_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 44, \
        basic_streambuf_wchar*, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, buf, count))
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_setbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, buf, count);
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PA_WH@Z */
/* ?pubsetbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEA_W_J@Z */
/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PAGH@Z */
/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsetbuf, 12)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_pubsetbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, buf, count);
    return call_basic_streambuf_wchar_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAHXZ */
/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sync, 4)
#define call_basic_streambuf_wchar_sync(this) CALL_VTBL_FUNC(this, 48, \
        int, (basic_streambuf_wchar*), (this))
int __thiscall basic_streambuf_wchar_sync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?pubsync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAHXZ */
/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsync, 4)
int __thiscall basic_streambuf_wchar_pubsync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_basic_streambuf_wchar_sync(this);
}

/* ?sgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPA_WH@Z */
/* ?sgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_J@Z */
/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPAGH@Z */
/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetn, 12)
streamsize __thiscall basic_streambuf_wchar_sgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_wchar__Xsgetn_s(this, ptr, -1, count);
}

/* ?showmanyc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JXZ */
/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_showmanyc, 4)
#define call_basic_streambuf_wchar_showmanyc(this) CALL_VTBL_FUNC(this, 12, \
        streamsize, (basic_streambuf_wchar*), (this))
streamsize __thiscall basic_streambuf_wchar_showmanyc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?in_avail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JXZ */
/* ?in_avail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_in_avail, 4)
streamsize __thiscall basic_streambuf_wchar_in_avail(basic_streambuf_wchar *this)
{
    streamsize ret;

    TRACE("(%p)\n", this);

    ret = basic_streambuf_wchar__Gnavail(this);
    return ret ? ret : call_basic_streambuf_wchar_showmanyc(this);
}

/* ?sputbackc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEG_W@Z */
/* ?sputbackc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAG_W@Z */
/* ?sputbackc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?sputbackc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputbackc, 8)
unsigned short __thiscall basic_streambuf_wchar_sputbackc(basic_streambuf_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    if(*this->prpos && *this->prpos>*this->prbuf && (*this->prpos)[-1]==ch) {
        (*this->prsize)++;
        (*this->prpos)--;
        return ch;
    }

    return call_basic_streambuf_wchar_pbackfail(this, ch);
}

/* ?sputc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEG_W@Z */
/* ?sputc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAG_W@Z */
/* ?sputc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?sputc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAHG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputc, 8)
unsigned short __thiscall basic_streambuf_wchar_sputc(basic_streambuf_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return basic_streambuf_wchar__Pnavail(this) ?
        (*basic_streambuf_wchar__Pninc(this) = ch) :
        call_basic_streambuf_wchar_overflow(this, ch);
}

/* ?sungetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sungetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sungetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sungetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sungetc, 4)
unsigned short __thiscall basic_streambuf_wchar_sungetc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    if(*this->prpos && *this->prpos>*this->prbuf) {
        (*this->prsize)++;
        (*this->prpos)--;
        return **this->prpos;
    }

    return call_basic_streambuf_wchar_pbackfail(this, WEOF);
}

/* ?stossc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?stossc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_stossc, 4)
void __thiscall basic_streambuf_wchar_stossc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    if(basic_streambuf_wchar__Gnavail(this))
        basic_streambuf_wchar__Gninc(this);
    else
        call_basic_streambuf_wchar_uflow(this);
}

/* ?sbumpc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sbumpc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sbumpc, 4)
unsigned short __thiscall basic_streambuf_wchar_sbumpc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar__Gninc(this) : call_basic_streambuf_wchar_uflow(this);
}

/* ?sgetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sgetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetc, 4)
unsigned short __thiscall basic_streambuf_wchar_sgetc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar_gptr(this) : call_basic_streambuf_wchar_underflow(this);
}

/* ?snextc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?snextc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?snextc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?snextc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_snextc, 4)
unsigned short __thiscall basic_streambuf_wchar_snextc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(basic_streambuf_wchar__Gnavail(this) > 1)
        return *basic_streambuf_wchar__Gnpreinc(this);
    return basic_streambuf_wchar_sbumpc(this)==WEOF ?
        WEOF : basic_streambuf_wchar_sgetc(this);
}

/* ?xsgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPA_WH@Z */
/* ?xsgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEA_W_J@Z */
/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPAGH@Z */
/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsgetn, 12)
#define call_basic_streambuf_wchar_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, \
        streamsize, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_wchar_xsgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_wchar__Xsgetn_s(this, ptr, -1, count);
}

/* ?xsputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPB_WH@Z */
/* ?xsputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEB_W_J@Z */
/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPBGH@Z */
/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEBG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsputn, 12)
#define call_basic_streambuf_wchar_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 32, \
        streamsize, (basic_streambuf_wchar*, const wchar_t*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_wchar_xsputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %ld)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_wchar__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk) {
            memcpy(*this->pwpos, ptr+copied, chunk);
            *this->pwpos += chunk;
            *this->pwsize -= chunk;
            copied += chunk;
        }else if(call_basic_streambuf_wchar_overflow(this, ptr[copied]) != WEOF) {
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPB_WH@Z */
/* ?sputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEB_W_J@Z */
/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPBGH@Z */
/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEBG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputn, 12)
streamsize __thiscall basic_streambuf_wchar_sputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, ptr, count);
    return call_basic_streambuf_wchar_xsputn(this, ptr, count);
}

/* ?_Stinit@?1??_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@23@@Z@4HA */
/* ?_Stinit@?1??_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@23@@Z@4HA */
int basic_filebuf_char__Init__Stinit = 0;

/* ?_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@12@@Z */
/* ?_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@12@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Init, 12)
void __thiscall basic_filebuf_char__Init(basic_filebuf_char *this, FILE *file, basic_filebuf__Initfl which)
{
    TRACE("(%p %p %d)\n", this, file, which);

    this->cvt = NULL;
    this->wrotesome = FALSE;
    this->state = basic_filebuf_char__Init__Stinit;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_char__Init_empty(&this->base);
    if(file)
        basic_streambuf_char__Init(&this->base, &file->_base, &file->_ptr,
                &file->_cnt, &file->_base, &file->_ptr, &file->_cnt);
}

/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAV?$codecvt@DDH@2@@Z */
/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$codecvt@DDH@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Initcvt, 8)
void __thiscall basic_filebuf_char__Initcvt(basic_filebuf_char *this, codecvt_char *cvt)
{
    TRACE("(%p %p)\n", this, cvt);

    if(codecvt_base_always_noconv(&cvt->base)) {
        this->cvt = NULL;
    }else {
        basic_streambuf_char__Init_empty(&this->base);
        this->cvt = cvt;
    }
}

/* ?_Endwrite@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAE_NXZ */
/* ?_Endwrite@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Endwrite, 4)
MSVCP_bool __thiscall basic_filebuf_char__Endwrite(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(!this->wrotesome || !this->cvt)
        return TRUE;


    if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
        return FALSE;

    while(1) {
        /* TODO: check if we need a dynamic buffer here */
        char buf[128];
        char *next;
        int ret;

        ret = codecvt_char_unshift(this->cvt, &this->state, buf, buf+sizeof(buf), &next);
        switch(ret) {
        case CODECVT_ok:
            this->wrotesome = FALSE;
            /* fall through */
        case CODECVT_partial:
            if(!fwrite(buf, next-buf, 1, this->file))
                return FALSE;
            if(this->wrotesome)
                break;
            /* fall through */
        case CODECVT_noconv:
            if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
                return FALSE;
            return TRUE;
        default:
            return FALSE;
        }
    }
}

/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_close, 4)
basic_filebuf_char* __thiscall basic_filebuf_char_close(basic_filebuf_char *this)
{
    basic_filebuf_char *ret = this;

    TRACE("(%p)\n", this);

    if(!this->file)
        return NULL;

    /* TODO: handle exceptions */
    if(!basic_filebuf_char__Endwrite(this))
        ret = NULL;
    if(!fclose(this->file))
        ret  = NULL;

    basic_filebuf_char__Init(this, NULL, INITFL_close);
    return ret;
}

/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor_file, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor_file(basic_filebuf_char *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_filebuf_char_vtable;

    basic_filebuf_char__Init(this, file, INITFL_new);
    return this;
}

/* ??_F?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_F?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor, 4)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor(basic_filebuf_char *this)
{
    return basic_filebuf_char_ctor_file(this, NULL);
}

/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor_uninitialized, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor_uninitialized(basic_filebuf_char *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_filebuf_char_vtable;
    return this;
}

/* ??1?$basic_filebuf@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_filebuf@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_dtor, 4)
void __thiscall basic_filebuf_char_dtor(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(this->close)
        basic_filebuf_char_close(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_filebuf_char_vector_dtor, 8)
basic_filebuf_char* __thiscall MSVCP_basic_filebuf_char_vector_dtor(basic_filebuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_filebuf_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_filebuf_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_is_open, 4)
MSVCP_bool __thiscall basic_filebuf_char_is_open(const basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);
    return this->file != NULL;
}

/* ?_Fiopen@std@@YAPAU_iobuf@@PB_WHH@Z */
/* ?_Fiopen@std@@YAPEAU_iobuf@@PEB_WHH@Z */
FILE* __cdecl _Fiopen_wchar(const wchar_t *name, int mode, int prot)
{
    static const wchar_t rW[] = {'r',0};
    static const struct {
        int mode;
        const wchar_t str[4];
        const wchar_t str_bin[4];
    } str_mode[] = {
        {OPENMODE_out,                            {'w',0},     {'w','b',0}},
        {OPENMODE_out|OPENMODE_app,               {'a',0},     {'a','b',0}},
        {OPENMODE_app,                            {'a',0},     {'a','b',0}},
        {OPENMODE_out|OPENMODE_trunc,             {'w',0},     {'w','b',0}},
        {OPENMODE_in,                             {'r',0},     {'r','b',0}},
        {OPENMODE_in|OPENMODE_out,                {'r','+',0}, {'r','+','b',0}},
        {OPENMODE_in|OPENMODE_out|OPENMODE_trunc, {'w','+',0}, {'w','+','b',0}},
        {OPENMODE_in|OPENMODE_out|OPENMODE_app,   {'a','+',0}, {'a','+','b',0}},
        {OPENMODE_in|OPENMODE_app,                {'a','+',0}, {'a','+','b',0}}
    };

    int real_mode = mode & ~(OPENMODE_ate|OPENMODE__Nocreate|OPENMODE__Noreplace|OPENMODE_binary);
    int mode_idx;
    FILE *f = NULL;

    TRACE("(%s %d %d)\n", debugstr_w(name), mode, prot);

    for(mode_idx=0; mode_idx<sizeof(str_mode)/sizeof(str_mode[0]); mode_idx++)
        if(str_mode[mode_idx].mode == real_mode)
            break;
    if(mode_idx == sizeof(str_mode)/sizeof(str_mode[0]))
        return NULL;

    if((mode & OPENMODE__Nocreate) && !(f = _wfopen(name, rW)))
        return NULL;
    else if(f)
        fclose(f);

    if((mode & OPENMODE__Noreplace) && (mode & (OPENMODE_out|OPENMODE_app))
            && (f = _wfopen(name, rW))) {
        fclose(f);
        return NULL;
    }

    f = _wfsopen(name, (mode & OPENMODE_binary) ? str_mode[mode_idx].str_bin
            : str_mode[mode_idx].str, prot);
    if(!f)
        return NULL;

    if((mode & OPENMODE_ate) && fseek(f, 0, SEEK_END)) {
        fclose(f);
        return NULL;
    }

    return f;
}

/* ?_Fiopen@std@@YAPAU_iobuf@@PBDHH@Z */
/* ?_Fiopen@std@@YAPEAU_iobuf@@PEBDHH@Z */
FILE* __cdecl _Fiopen(const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%s %d %d)\n", name, mode, prot);

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
    return _Fiopen_wchar(nameW, mode, prot);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PB_WHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEB_WHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBGHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_wchar, 16)
basic_filebuf_char* __thiscall basic_filebuf_char_open_wchar(basic_filebuf_char *this, const wchar_t *name, int mode, int prot)
{
    FILE *f = NULL;

    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(basic_filebuf_char_is_open(this))
        return NULL;

    if(!(f = _Fiopen_wchar(name, mode, prot)))
        return NULL;

    basic_filebuf_char__Init(this, f, INITFL_open);
    basic_filebuf_char__Initcvt(this, codecvt_char_use_facet(this->base.loc));
    return this;
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PB_WI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEB_WI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBGI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBGI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_wchar_mode, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_wchar_mode(basic_filebuf_char *this, const wchar_t *name, unsigned int mode)
{
    return basic_filebuf_char_open_wchar(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open, 16)
basic_filebuf_char* __thiscall basic_filebuf_char_open(basic_filebuf_char *this, const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
    return basic_filebuf_char_open_wchar(this, nameW, mode, prot);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_mode, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_mode(basic_filebuf_char *this, const char *name, unsigned int mode)
{
    return basic_filebuf_char_open(this, name, mode, SH_DENYNO);
}

/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
#define call_basic_filebuf_char_overflow(this, c) CALL_VTBL_FUNC(this, 4, \
        int, (basic_filebuf_char*, int), (this, c))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_overflow, 8)
int __thiscall basic_filebuf_char_overflow(basic_filebuf_char *this, int c)
{
    char buf[8], *dyn_buf;
    char ch = c, *to_next;
    const char *from_next;
    int ret, max_size;


    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;
    if(c == EOF)
        return !c;

    if(!this->cvt)
        return fwrite(&ch, sizeof(char), 1, this->file) ? c : EOF;

    from_next = &ch;
    do {
        ret = codecvt_char_out(this->cvt, &this->state, from_next, &ch+1,
                &from_next, buf, buf+sizeof(buf), &to_next);

        switch(ret) {
        case CODECVT_partial:
            if(to_next == buf)
                break;
            /* fall through */
        case CODECVT_ok:
            if(!fwrite(buf, to_next-buf, 1, this->file))
                return EOF;
            if(ret == CODECVT_partial)
                continue;
            return c;
        case CODECVT_noconv:
            return fwrite(&ch, sizeof(char), 1, this->file) ? c : EOF;
        default:
            return EOF;
        }
    } while(0);

    max_size = codecvt_base_max_length(&this->cvt->base);
    dyn_buf = malloc(max_size);
    if(!dyn_buf)
        return EOF;

    ret = codecvt_char_out(this->cvt, &this->state, from_next, &ch+1,
            &from_next, dyn_buf, dyn_buf+max_size, &to_next);

    switch(ret) {
    case CODECVT_ok:
        ret = fwrite(dyn_buf, to_next-dyn_buf, 1, this->file);
        free(dyn_buf);
        return ret ? c : EOF;
    case CODECVT_partial:
        ERR("buffer should be big enough to store all output\n");
        /* fall through */
    default:
        free(dyn_buf);
        return EOF;
    }
}

/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
#define call_basic_filebuf_char_pbackfail(this, c) CALL_VTBL_FUNC(this, 8, \
        int, (basic_filebuf_char*, int), (this, c))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_pbackfail, 8)
int __thiscall basic_filebuf_char_pbackfail(basic_filebuf_char *this, int c)
{
    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base)>basic_streambuf_char_eback(&this->base)
            && (c==EOF || basic_streambuf_char_gptr(&this->base)[-1]==(char)c)) {
        basic_streambuf_char__Gndec(&this->base);
        return c==EOF ? !c : c;
    }else if(c!=EOF && !this->cvt) {
        return ungetc(c, this->file);
    }

    return EOF;
}

/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
#define call_basic_filebuf_char_uflow(this) CALL_VTBL_FUNC(this, 20, \
        int, (basic_filebuf_char*), (this))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_uflow, 4)
int __thiscall basic_filebuf_char_uflow(basic_filebuf_char *this)
{
    char ch, buf[128], *to_next;
    const char *buf_next;
    int c, i;

    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return *basic_streambuf_char__Gninc(&this->base);

    c = fgetc(this->file);
    if(!this->cvt || c==EOF)
        return c;

    buf_next = buf;
    for(i=0; i < sizeof(buf)/sizeof(char); i++) {
        buf[i] = c;

        switch(codecvt_char_in(this->cvt, &this->state, buf_next,
                    buf+i+1, &buf_next, &ch, &ch+1, &to_next)) {
        case CODECVT_partial:
        case CODECVT_ok:
            if(to_next == &ch) {
                c = fgetc(this->file);
                if(c == EOF)
                    return EOF;
                continue;
            }

            for(i--; i>=buf_next-buf; i--)
                ungetc(buf[i], this->file);
            return ch;
        case CODECVT_noconv:
            return buf[0];
        default:
            return EOF;
        }
    }

    FIXME("buffer is too small\n");
    return EOF;
}

/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
#define call_basic_filebuf_char_underflow(this) CALL_VTBL_FUNC(this, 16, \
        int, (basic_filebuf_char*), (this))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_underflow, 4)
int __thiscall basic_filebuf_char_underflow(basic_filebuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return *basic_streambuf_char_gptr(&this->base);

    ret = call_basic_filebuf_char_uflow(this);
    if(ret != EOF)
        ret = call_basic_filebuf_char_pbackfail(this, ret);
    return ret;
}

/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#define call_basic_filebuf_char_seekoff(this, ret, off, way, mode) CALL_VTBL_FUNC(this, 36, \
        fpos_int*, (basic_filebuf_char*, fpos_int*, streamoff, int, int), (this, ret, off, way, mode))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekoff, 20)
fpos_int* __thiscall basic_filebuf_char_seekoff(basic_filebuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    fpos_t pos;

    TRACE("(%p %p %ld %d %d)\n", this, ret, off, way, mode);

    if(!basic_filebuf_char_is_open(this) || !basic_filebuf_char__Endwrite(this)
            || fseek(this->file, off, way)) {
        ret->off = 0;
        ret->pos = -1;
        ret->state = 0;
        return ret;
    }

    fgetpos(this->file, &pos);
    ret->off = 0;
    ret->pos = pos;
    ret->state = this->state;
    return ret;
}

/* ?seekpos@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
#define call_basic_filebuf_char_seekpos(this, ret, pos, mode) CALL_VTBL_FUNC(this, 40, \
        fpos_int*, (basic_filebuf_char*, fpos_int*, fpos_int, mode), (this, ret, pos, mode))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekpos, 36)
fpos_int* __thiscall basic_filebuf_char_seekpos(basic_filebuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    fpos_t fpos;

    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(!basic_filebuf_char_is_open(this) || !basic_filebuf_char__Endwrite(this)
            || fseek(this->file, (LONG)pos.pos, SEEK_SET)
            || (pos.off && fseek(this->file, pos.off, SEEK_CUR))) {
        ret->off = 0;
        ret->pos = -1;
        ret->state = 0;
        return ret;
    }

    fgetpos(this->file, &fpos);
    ret->off = 0;
    ret->pos = fpos;
    ret->state = this->state;
    return ret;
}

/* ?setbuf@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PADH@Z */
/* ?setbuf@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PEAD_J@Z */
#define call_basic_filebuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 44, \
        basic_streambuf_char*, (basic_filebuf_char*, char*, streamsize), (this, buf, count))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_setbuf, 12)
basic_streambuf_char* __thiscall basic_filebuf_char_setbuf(basic_filebuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %ld)\n", this, buf, count);

    if(!basic_filebuf_char_is_open(this))
        return NULL;

    if(setvbuf(this->file, buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count))
        return NULL;

    basic_filebuf_char__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
#define call_basic_filebuf_char_sync(this) CALL_VTBL_FUNC(this, 48, \
        int, (basic_filebuf_char*), (this))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_sync, 4)
int __thiscall basic_filebuf_char_sync(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return 0;

    if(call_basic_filebuf_char_overflow(this, EOF) == EOF)
        return 0;
    return fflush(this->file);
}

/* ?imbue@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAXAEBVlocale@2@@Z */
#define call_basic_filebuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 52, \
        void, (basic_filebuf_char*, const locale*), (this, loc))
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_imbue, 8)
void __thiscall basic_filebuf_char_imbue(basic_filebuf_char *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    basic_filebuf_char__Initcvt(this, codecvt_char_use_facet(loc));
}

/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Getstate, 8)
int __thiscall basic_stringbuf_char__Getstate(basic_stringbuf_char *this, IOSB_openmode mode)
{
    int state = 0;

    if(!(mode & OPENMODE_in))
        state |= STRINGBUF_no_read;

    if(!(mode & OPENMODE_out))
        state |= STRINGBUF_no_write;

    if(mode & OPENMODE_ate)
        state |= STRINGBUF_at_end;

    if(mode & OPENMODE_app)
        state |= STRINGBUF_append;

    return state;
}

/* ?_Init@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXPBDIH@Z */
/* ?_Init@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAXPEBD_KH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Init, 16)
void __thiscall basic_stringbuf_char__Init(basic_stringbuf_char *this, const char *str, MSVCP_size_t count, int state)
{
    TRACE("(%p, %p, %ld, %d)\n", this, str, count, state);

    basic_streambuf_char__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        char *buf = MSVCRT_operator_new(count);
        if(!buf) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
        }

        memcpy(buf, str, count);
        this->seekhigh = buf + count;

        this->state |= STRINGBUF_allocated;

        if(!(state & STRINGBUF_no_read))
            basic_streambuf_char_setg(&this->base, buf, buf, buf + count);

        if(!(state & STRINGBUF_no_write)) {
            basic_streambuf_char_setp_next(&this->base, buf, (state & STRINGBUF_at_end) ? buf + count : buf, buf + count);

            if(!basic_streambuf_char_gptr(&this->base))
                basic_streambuf_char_setg(&this->base, buf, 0, buf);
        }
    }
}

/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor_str, 12)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor_str(basic_stringbuf_char *this,
        const basic_string_char *str, IOSB_openmode mode)
{
    TRACE("(%p %p %d)\n", this, str, mode);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_stringbuf_char_vtable;

    basic_stringbuf_char__Init(this, MSVCP_basic_string_char_c_str(str),
            str->size, basic_stringbuf_char__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor_mode, 8)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor_mode(
        basic_stringbuf_char *this, IOSB_openmode mode)
{
    TRACE("(%p %d)\n", this, mode);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_stringbuf_char_vtable;

    basic_stringbuf_char__Init(this, NULL, 0, basic_stringbuf_char__Getstate(this, mode));
    return this;
}

/* ??_F?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor, 4)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor(basic_stringbuf_char *this)
{
    return basic_stringbuf_char_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ?_Tidy@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Tidy, 4)
void __thiscall basic_stringbuf_char__Tidy(basic_stringbuf_char *this)
{
    TRACE("(%p)\n", this);

    if(this->state & STRINGBUF_allocated) {
        MSVCRT_operator_delete(basic_streambuf_char_eback(&this->base));
        this->seekhigh = NULL;
        this->state &= ~STRINGBUF_allocated;
    }

    basic_streambuf_char__Init_empty(&this->base);
}

/* ??1?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_dtor, 4)
void __thiscall basic_stringbuf_char_dtor(basic_stringbuf_char *this)
{
    TRACE("(%p)\n", this);

    basic_stringbuf_char__Tidy(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_stringbuf_char_vector_dtor, 8)
basic_stringbuf_char* __thiscall MSVCP_basic_stringbuf_char_vector_dtor(basic_stringbuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_char_dtor(this+i);

        MSVCRT_operator_delete(ptr);
    }else {
        basic_stringbuf_char_dtor(this);

        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHH@Z */
/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_overflow, 8)
int __thiscall basic_stringbuf_char_overflow(basic_stringbuf_char *this, int meta)
{
    MSVCP_size_t oldsize, size;
    char *ptr, *buf;

    TRACE("(%p %x)\n", this, meta);

    if(meta == EOF)
        return !EOF;
    if(this->state & STRINGBUF_no_write)
        return EOF;

    ptr = basic_streambuf_char_pptr(&this->base);
    if((this->state&STRINGBUF_append) && ptr<this->seekhigh)
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                this->seekhigh, basic_streambuf_char_epptr(&this->base));

    if(ptr && ptr<basic_streambuf_char_epptr(&this->base))
        return (*basic_streambuf_char__Pninc(&this->base) = meta);

    oldsize = (ptr ? basic_streambuf_char_epptr(&this->base)-basic_streambuf_char_eback(&this->base): 0);
    size = oldsize|0xf;
    size += size/2;
    buf = MSVCRT_operator_new(size);
    if(!buf) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }

    if(!oldsize) {
        this->seekhigh = buf;
        basic_streambuf_char_setp(&this->base, buf, buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_char_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_char_setg(&this->base, buf, buf, buf+1);

        this->state |= STRINGBUF_allocated;
    }else {
        ptr = basic_streambuf_char_eback(&this->base);
        memcpy(buf, ptr, oldsize);

        this->seekhigh = buf+(this->seekhigh-ptr);
        basic_streambuf_char_setp_next(&this->base, buf,
                buf+(basic_streambuf_char_pptr(&this->base)-ptr), buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_char_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_char_setg(&this->base, buf,
                    buf+(basic_streambuf_char_gptr(&this->base)-ptr),
                    basic_streambuf_char_pptr(&this->base)+1);

        MSVCRT_operator_delete(ptr);
    }

    return (*basic_streambuf_char__Pninc(&this->base) = meta);
}

/* ?pbackfail@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_pbackfail, 8)
int __thiscall basic_stringbuf_char_pbackfail(basic_stringbuf_char *this, int c)
{
    char *cur;

    TRACE("(%p %x)\n", this, c);

    cur = basic_streambuf_char_gptr(&this->base);
    if(!cur || cur==basic_streambuf_char_eback(&this->base)
            || (c!=EOF && c!=cur[-1] && this->state&STRINGBUF_no_write))
        return EOF;

    if(c != EOF)
        cur[-1] = c;
    basic_streambuf_char_gbump(&this->base, -1);
    return c==EOF ? !EOF : c;
}

/* ?underflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHXZ */
/* ?underflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_underflow, 4)
int __thiscall basic_stringbuf_char_underflow(basic_stringbuf_char *this)
{
    char *ptr, *cur;

    TRACE("(%p)\n", this);

    cur = basic_streambuf_char_gptr(&this->base);
    if(!cur || this->state&STRINGBUF_no_read)
        return EOF;

    ptr  = basic_streambuf_char_pptr(&this->base);
    if(this->seekhigh < ptr)
        this->seekhigh = ptr;

    ptr = basic_streambuf_char_egptr(&this->base);
    if(this->seekhigh > ptr)
        basic_streambuf_char_setg(&this->base, basic_streambuf_char_eback(&this->base), cur, this->seekhigh);

    if(cur < this->seekhigh)
        return *cur;
    return EOF;
}

/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekoff, 20)
fpos_int* __thiscall basic_stringbuf_char_seekoff(basic_stringbuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    char *beg, *cur_r, *cur_w;

    TRACE("(%p %p %ld %d %d)\n", this, ret, off, way, mode);

    cur_w = basic_streambuf_char_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->state = 0;

    beg = basic_streambuf_char_eback(&this->base);
    cur_r = basic_streambuf_char_gptr(&this->base);
    if((mode & OPENMODE_in) && cur_r) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += cur_r-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg) {
            off = -1;
        }else {
            basic_streambuf_char_gbump(&this->base, beg-cur_r+off);
            if((mode & OPENMODE_out) && cur_w) {
                basic_streambuf_char_setp_next(&this->base, beg,
                        basic_streambuf_char_gptr(&this->base),
                        basic_streambuf_char_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && cur_w) {
        if(way == SEEKDIR_cur)
            off += cur_w-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg)
            off = -1;
        else
            basic_streambuf_char_pbump(&this->base, beg-cur_w+off);
    }else {
        off = -1;
    }

    ret->pos = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekpos, 36)
fpos_int* __thiscall basic_stringbuf_char_seekpos(basic_stringbuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(pos.off==0 && pos.pos==-1 && pos.state==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_char_seekoff(this, ret, pos.off, SEEKDIR_beg, mode);
}

/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_str_set, 8)
void __thiscall basic_stringbuf_char_str_set(basic_stringbuf_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);

    basic_stringbuf_char__Tidy(this);
    basic_stringbuf_char__Init(this, MSVCP_basic_string_char_c_str(str), str->size, this->state);
}

/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_str_get, 8)
basic_string_char* __thiscall basic_stringbuf_char_str_get(const basic_stringbuf_char *this, basic_string_char *ret)
{
    char *ptr;

    TRACE("(%p)\n", this);

    if(!(this->state & STRINGBUF_no_write) && basic_streambuf_char_pptr(&this->base)) {
        char *pptr;

        ptr = basic_streambuf_char_pbase(&this->base);
        pptr = basic_streambuf_char_pptr(&this->base);

        return MSVCP_basic_string_char_ctor_cstr_len(ret, ptr, (this->seekhigh < pptr ? pptr : this->seekhigh) - ptr);
    }

    if(!(this->state & STRINGBUF_no_read) && basic_streambuf_char_gptr(&this->base)) {
        ptr = basic_streambuf_char_eback(&this->base);
        return MSVCP_basic_string_char_ctor_cstr_len(ret, ptr, basic_streambuf_char_egptr(&this->base) - ptr);
    }

    return MSVCP_basic_string_char_ctor(ret);
}

/* ?_Getstate@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@AEAAHH@Z */
/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Getstate, 8)
int __thiscall basic_stringbuf_wchar__Getstate(basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    int state = 0;

    if(!(mode & OPENMODE_in))
        state |= STRINGBUF_no_read;

    if(!(mode & OPENMODE_out))
        state |= STRINGBUF_no_write;

    if(mode & OPENMODE_ate)
        state |= STRINGBUF_at_end;

    if(mode & OPENMODE_app)
        state |= STRINGBUF_append;

    return state;
}

/* ?_Init@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXPB_WIH@Z */
/* ?_Init@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAXPEB_W_KH@Z */
/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXPBGIH@Z */
/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXPEBG_KH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Init, 16)
void __thiscall basic_stringbuf_wchar__Init(basic_stringbuf_wchar *this, const wchar_t *str, MSVCP_size_t count, int state)
{
    TRACE("(%p, %p, %ld, %d)\n", this, str, count, state);

    basic_streambuf_wchar__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        wchar_t *buf = MSVCRT_operator_new(count);
        if(!buf) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
        }

        memcpy(buf, str, count);
        this->seekhigh = buf + count;

        this->state |= STRINGBUF_allocated;

        if(!(state & STRINGBUF_no_read))
            basic_streambuf_wchar_setg(&this->base, buf, buf, buf + count);

        if(!(state & STRINGBUF_no_write)) {
            basic_streambuf_wchar_setp_next(&this->base, buf, (state & STRINGBUF_at_end) ? buf + count : buf, buf + count);

            if(!basic_streambuf_wchar_gptr(&this->base))
                basic_streambuf_wchar_setg(&this->base, buf, 0, buf);
        }
    }
}

/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor_str, 12)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor_str(basic_stringbuf_wchar *this,
        const basic_string_wchar *str, IOSB_openmode mode)
{
    TRACE("(%p %p %d)\n", this, str, mode);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_stringbuf_wchar_vtable;

    basic_stringbuf_wchar__Init(this, MSVCP_basic_string_wchar_c_str(str),
            str->size, basic_stringbuf_wchar__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_str, 12)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_str(basic_stringbuf_wchar *this,
        const basic_string_wchar *str, IOSB_openmode mode)
{
    basic_stringbuf_wchar_ctor_str(this, str, mode);
    this->base.vtable = &MSVCP_basic_stringbuf_short_vtable;
    return this;
}

/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor_mode, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor_mode(
        basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    TRACE("(%p %d)\n", this, mode);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_stringbuf_wchar_vtable;

    basic_stringbuf_wchar__Init(this, NULL, 0, basic_stringbuf_wchar__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_mode, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_mode(
        basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    basic_stringbuf_wchar_ctor_mode(this, mode);
    this->base.vtable = &MSVCP_basic_stringbuf_short_vtable;
    return this;
}

/* ??_F?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor, 4)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor(basic_stringbuf_wchar *this)
{
    return basic_stringbuf_wchar_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor, 4)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor(basic_stringbuf_wchar *this)
{
    return basic_stringbuf_short_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ?_Tidy@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAXXZ */
/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Tidy, 4)
void __thiscall basic_stringbuf_wchar__Tidy(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->state & STRINGBUF_allocated) {
        MSVCRT_operator_delete(basic_streambuf_wchar_eback(&this->base));
        this->seekhigh = NULL;
        this->state &= ~STRINGBUF_allocated;
    }

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ??1?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_dtor, 4)
void __thiscall basic_stringbuf_wchar_dtor(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_stringbuf_wchar__Tidy(this);
    basic_streambuf_wchar_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_stringbuf_wchar_vector_dtor, 8)
basic_stringbuf_wchar* __thiscall MSVCP_basic_stringbuf_wchar_vector_dtor(basic_stringbuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_wchar_dtor(this+i);

        MSVCRT_operator_delete(ptr);
    }else {
        basic_stringbuf_wchar_dtor(this);

        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_stringbuf_short_vector_dtor, 8)
basic_stringbuf_wchar* __thiscall MSVCP_basic_stringbuf_short_vector_dtor(basic_stringbuf_wchar *this, unsigned int flags)
{
    return MSVCP_basic_stringbuf_wchar_vector_dtor(this, flags);
}

/* ?overflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGG@Z */
/* ?overflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGG@Z */
/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_overflow, 8)
unsigned short __thiscall basic_stringbuf_wchar_overflow(basic_stringbuf_wchar *this, unsigned short meta)
{
    MSVCP_size_t oldsize, size;
    wchar_t *ptr, *buf;

    TRACE("(%p %x)\n", this, meta);

    if(meta == WEOF)
        return !WEOF;
    if(this->state & STRINGBUF_no_write)
        return WEOF;

    ptr = basic_streambuf_wchar_pptr(&this->base);
    if((this->state&STRINGBUF_append) && ptr<this->seekhigh)
        basic_streambuf_wchar_setp_next(&this->base, basic_streambuf_wchar_pbase(&this->base),
                this->seekhigh, basic_streambuf_wchar_epptr(&this->base));

    if(ptr && ptr<basic_streambuf_wchar_epptr(&this->base))
        return (*basic_streambuf_wchar__Pninc(&this->base) = meta);

    oldsize = (ptr ? basic_streambuf_wchar_epptr(&this->base)-basic_streambuf_wchar_eback(&this->base): 0);
    size = oldsize|0xf;
    size += size/2;
    buf = MSVCRT_operator_new(size);
    if(!buf) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }

    if(!oldsize) {
        this->seekhigh = buf;
        basic_streambuf_wchar_setp(&this->base, buf, buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_wchar_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_wchar_setg(&this->base, buf, buf, buf+1);

        this->state |= STRINGBUF_allocated;
    }else {
        ptr = basic_streambuf_wchar_eback(&this->base);
        memcpy(buf, ptr, oldsize);

        this->seekhigh = buf+(this->seekhigh-ptr);
        basic_streambuf_wchar_setp_next(&this->base, buf,
                buf+(basic_streambuf_wchar_pptr(&this->base)-ptr), buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_wchar_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_wchar_setg(&this->base, buf,
                    buf+(basic_streambuf_wchar_gptr(&this->base)-ptr),
                    basic_streambuf_wchar_pptr(&this->base)+1);

        MSVCRT_operator_delete(ptr);
    }

    return (*basic_streambuf_wchar__Pninc(&this->base) = meta);
}

/* ?pbackfail@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGG@Z */
/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_pbackfail, 8)
unsigned short __thiscall basic_stringbuf_wchar_pbackfail(basic_stringbuf_wchar *this, unsigned short c)
{
    wchar_t *cur;

    TRACE("(%p %x)\n", this, c);

    cur = basic_streambuf_wchar_gptr(&this->base);
    if(!cur || cur==basic_streambuf_wchar_eback(&this->base)
            || (c!=WEOF && c!=cur[-1] && this->state&STRINGBUF_no_write))
        return WEOF;

    if(c != WEOF)
        cur[-1] = c;
    basic_streambuf_wchar_gbump(&this->base, -1);
    return c==WEOF ? !WEOF : c;
}

/* ?underflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGXZ */
/* ?underflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGXZ */
/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGXZ */
/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_underflow, 4)
unsigned short __thiscall basic_stringbuf_wchar_underflow(basic_stringbuf_wchar *this)
{
    wchar_t *ptr, *cur;

    TRACE("(%p)\n", this);

    cur = basic_streambuf_wchar_gptr(&this->base);
    if(!cur || this->state&STRINGBUF_no_read)
        return WEOF;

    ptr  = basic_streambuf_wchar_pptr(&this->base);
    if(this->seekhigh < ptr)
        this->seekhigh = ptr;

    ptr = basic_streambuf_wchar_egptr(&this->base);
    if(this->seekhigh > ptr)
        basic_streambuf_wchar_setg(&this->base, basic_streambuf_wchar_eback(&this->base), cur, this->seekhigh);

    if(cur < this->seekhigh)
        return *cur;
    return WEOF;
}

/* ?seekoff@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_seekoff, 20)
fpos_int* __thiscall basic_stringbuf_wchar_seekoff(basic_stringbuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    wchar_t *beg, *cur_r, *cur_w;

    TRACE("(%p %p %ld %d %d)\n", this, ret, off, way, mode);

    cur_w = basic_streambuf_wchar_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->state = 0;

    beg = basic_streambuf_wchar_eback(&this->base);
    cur_r = basic_streambuf_wchar_gptr(&this->base);
    if((mode & OPENMODE_in) && cur_r) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += cur_r-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg) {
            off = -1;
        }else {
            basic_streambuf_wchar_gbump(&this->base, beg-cur_r+off);
            if((mode & OPENMODE_out) && cur_w) {
                basic_streambuf_wchar_setp_next(&this->base, beg,
                        basic_streambuf_wchar_gptr(&this->base),
                        basic_streambuf_wchar_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && cur_w) {
        if(way == SEEKDIR_cur)
            off += cur_w-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg)
            off = -1;
        else
            basic_streambuf_wchar_pbump(&this->base, beg-cur_w+off);
    }else {
        off = -1;
    }

    ret->pos = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_seekpos, 36)
fpos_int* __thiscall basic_stringbuf_wchar_seekpos(basic_stringbuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(pos.off==0 && pos.pos==-1 && pos.state==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_wchar_seekoff(this, ret, pos.off, SEEKDIR_beg, mode);
}

/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_str_set, 8)
void __thiscall basic_stringbuf_wchar_str_set(basic_stringbuf_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);

    basic_stringbuf_wchar__Tidy(this);
    basic_stringbuf_wchar__Init(this, MSVCP_basic_string_wchar_c_str(str), str->size, this->state);
}

/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_stringbuf_wchar_str_get(const basic_stringbuf_wchar *this, basic_string_wchar *ret)
{
    wchar_t *ptr;

    TRACE("(%p)\n", this);

    if(!(this->state & STRINGBUF_no_write) && basic_streambuf_wchar_pptr(&this->base)) {
        wchar_t *pptr;

        ptr = basic_streambuf_wchar_pbase(&this->base);
        pptr = basic_streambuf_wchar_pptr(&this->base);

        return MSVCP_basic_string_wchar_ctor_cstr_len(ret, ptr, (this->seekhigh < pptr ? pptr : this->seekhigh) - ptr);
    }

    if(!(this->state & STRINGBUF_no_read) && basic_streambuf_wchar_gptr(&this->base)) {
        ptr = basic_streambuf_wchar_eback(&this->base);
        return MSVCP_basic_string_wchar_ctor_cstr_len(ret, ptr, basic_streambuf_wchar_egptr(&this->base) - ptr);
    }

    return MSVCP_basic_string_wchar_ctor(ret);
}

/* ??0ios_base@std@@IAE@XZ */
/* ??0ios_base@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_ctor, 4)
ios_base* __thiscall ios_base_ctor(ios_base *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &MSVCP_ios_base_vtable;
    return this;
}

/* ??0ios_base@std@@QAE@ABV01@@Z */
/* ??0ios_base@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copy_ctor, 8)
ios_base* __thiscall ios_base_copy_ctor(ios_base *this, const ios_base *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->vtable = &MSVCP_ios_base_vtable;
    return this;
}

/* ?_Callfns@ios_base@std@@AAEXW4event@12@@Z */
/* ?_Callfns@ios_base@std@@AEAAXW4event@12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Callfns, 8)
void __thiscall ios_base_Callfns(ios_base *this, IOS_BASE_event event)
{
    IOS_BASE_fnarray *cur;

    TRACE("(%p %x)\n", this, event);

    for(cur=this->calls; cur; cur=cur->next)
        cur->event_handler(event, this, cur->index);
}

/* ?_Tidy@ios_base@std@@AAAXXZ */
/* ?_Tidy@ios_base@std@@AEAAXXZ */
void CDECL ios_base_Tidy(ios_base *this)
{
    IOS_BASE_iosarray *arr_cur, *arr_next;
    IOS_BASE_fnarray *event_cur, *event_next;

    TRACE("(%p)\n", this);

    ios_base_Callfns(this, EVENT_erase_event);

    for(arr_cur=this->arr; arr_cur; arr_cur=arr_next) {
        arr_next = arr_cur->next;
        MSVCRT_operator_delete(arr_cur);
    }
    this->arr = NULL;

    for(event_cur=this->calls; event_cur; event_cur=event_next) {
        event_next = event_cur->next;
        MSVCRT_operator_delete(event_cur);
    }
    this->calls = NULL;
}

/* ?_Ios_base_dtor@ios_base@std@@CAXPAV12@@Z */
/* ?_Ios_base_dtor@ios_base@std@@CAXPEAV12@@Z */
void CDECL ios_base_Ios_base_dtor(ios_base *obj)
{
    TRACE("(%p)\n", obj);
    if(obj->loc) {
        locale_dtor(obj->loc);
        MSVCRT_operator_delete(obj->loc);
    }
    ios_base_Tidy(obj);
}

/* ??1ios_base@std@@UAE@XZ */
/* ??1ios_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_dtor, 4)
void __thiscall ios_base_dtor(ios_base *this)
{
    ios_base_Ios_base_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_ios_base_vector_dtor, 8)
ios_base* __thiscall MSVCP_ios_base_vector_dtor(ios_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ios_base_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        ios_base_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_iosb_vector_dtor, 8)
void* __thiscall MSVCP_iosb_vector_dtor(void *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        int *ptr = (int *)this-1;
        MSVCRT_operator_delete(ptr);
    } else {
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Findarr@ios_base@std@@AAEAAU_Iosarray@12@H@Z */
/* ?_Findarr@ios_base@std@@AEAAAEAU_Iosarray@12@H@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Findarr, 8)
IOS_BASE_iosarray* __thiscall ios_base_Findarr(ios_base *this, int index)
{
    IOS_BASE_iosarray *p;

    TRACE("(%p %d)\n", this, index);

    for(p=this->arr; p; p=p->next) {
        if(p->index == index)
            return p;
    }

    for(p=this->arr; p; p=p->next) {
        if(!p->long_val && !p->ptr_val) {
            p->index = index;
            return p;
        }
    }

    p = MSVCRT_operator_new(sizeof(IOS_BASE_iosarray));
    p->next = this->arr;
    p->index = index;
    p->long_val = 0;
    p->ptr_val = NULL;
    this->arr = p;
    return p;
}

/* ?iword@ios_base@std@@QAEAAJH@Z */
/* ?iword@ios_base@std@@QEAAAEAJH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_iword, 8)
LONG* __thiscall ios_base_iword(ios_base *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return &ios_base_Findarr(this, index)->long_val;
}

/* ?pword@ios_base@std@@QAEAAPAXH@Z */
/* ?pword@ios_base@std@@QEAAAEAPEAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_pword, 8)
void** __thiscall ios_base_pword(ios_base *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return &ios_base_Findarr(this, index)->ptr_val;
}

/* ?register_callback@ios_base@std@@QAEXP6AXW4event@12@AAV12@H@ZH@Z */
/* ?register_callback@ios_base@std@@QEAAXP6AXW4event@12@AEAV12@H@ZH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_register_callback, 12)
void __thiscall ios_base_register_callback(ios_base *this, IOS_BASE_event_callback callback, int index)
{
    IOS_BASE_fnarray *event;

    TRACE("(%p %p %d)\n", this, callback, index);

    event = MSVCRT_operator_new(sizeof(IOS_BASE_fnarray));
    event->next = this->calls;
    event->index = index;
    event->event_handler = callback;
    this->calls = event;
}

/* ?clear@ios_base@std@@QAEXH_N@Z */
/* ?clear@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_reraise, 12)
void __thiscall ios_base_clear_reraise(ios_base *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    this->state = state & IOSTATE_mask;
    if(!(this->state & this->except))
        return;

    if(reraise)
        throw_exception(EXCEPTION_RERAISE, NULL);
    else if(this->state & this->except & IOSTATE_eofbit)
        throw_exception(EXCEPTION_FAILURE, "eofbit is set");
    else if(this->state & this->except & IOSTATE_failbit)
        throw_exception(EXCEPTION_FAILURE, "failbit is set");
    else if(this->state & this->except & IOSTATE_badbit)
        throw_exception(EXCEPTION_FAILURE, "badbit is set");
    else if(this->state & this->except & IOSTATE__Hardfail)
        throw_exception(EXCEPTION_FAILURE, "_Hardfail is set");
}

/* ?clear@ios_base@std@@QAEXH@Z */
/* ?clear@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear, 8)
void __thiscall ios_base_clear(ios_base *this, IOSB_iostate state)
{
    ios_base_clear_reraise(this, state, FALSE);
}

/* ?clear@ios_base@std@@QAEXI@Z */
/* ?clear@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_unsigned, 8)
void __thiscall ios_base_clear_unsigned(ios_base *this, unsigned int state)
{
    ios_base_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?exceptions@ios_base@std@@QAEXH@Z */
/* ?exceptions@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_set, 8)
void __thiscall ios_base_exceptions_set(ios_base *this, IOSB_iostate state)
{
    TRACE("(%p %x)\n", this, state);
    this->except = state & IOSTATE_mask;
    ios_base_clear(this, this->state);
}

/* ?exceptions@ios_base@std@@QAEXI@Z */
/* ?exceptions@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_set_unsigned, 8)
void __thiscall ios_base_exceptions_set_unsigned(ios_base *this, unsigned int state)
{
    TRACE("(%p %x)\n", this, state);
    ios_base_exceptions_set(this, state);
}

/* ?exceptions@ios_base@std@@QBEHXZ */
/* ?exceptions@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_get, 4)
IOSB_iostate __thiscall ios_base_exceptions_get(ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->except;
}

/* ?copyfmt@ios_base@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@ios_base@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copyfmt, 8)
ios_base* __thiscall ios_base_copyfmt(ios_base *this, const ios_base *rhs)
{
    TRACE("(%p %p)\n", this, rhs);

    if(this != rhs) {
        IOS_BASE_iosarray *arr_cur;
        IOS_BASE_fnarray *event_cur;

        ios_base_Tidy(this);

        for(arr_cur=rhs->arr; arr_cur; arr_cur=arr_cur->next) {
            if(arr_cur->long_val)
                *ios_base_iword(this, arr_cur->index) = arr_cur->long_val;
            if(arr_cur->ptr_val)
                *ios_base_pword(this, arr_cur->index) = arr_cur->ptr_val;
        }
        this->stdstr = rhs->stdstr;
        this->fmtfl = rhs->fmtfl;
        this->prec = rhs->prec;
        this->wide = rhs->wide;
        locale_operator_assign(this->loc, rhs->loc);

        for(event_cur=rhs->calls; event_cur; event_cur=event_cur->next)
            ios_base_register_callback(this, event_cur->event_handler, event_cur->index);

        ios_base_Callfns(this, EVENT_copyfmt_event);
        ios_base_exceptions_set(this, rhs->except);
    }

    return this;
}

/* ??4ios_base@std@@QAEAAV01@ABV01@@Z */
/* ??4ios_base@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_assign, 8)
ios_base* __thiscall ios_base_assign(ios_base *this, const ios_base *right)
{
    TRACE("(%p %p)\n", this, right);

    if(this != right) {
        this->state = right->state;
        ios_base_copyfmt(this, right);
    }

    return this;
}

/* ?fail@ios_base@std@@QBE_NXZ */
/* ?fail@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_fail, 4)
MSVCP_bool __thiscall ios_base_fail(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & (IOSTATE_failbit|IOSTATE_badbit)) != 0;
}

/* ??7ios_base@std@@QBE_NXZ */
/* ??7ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_succ, 4)
MSVCP_bool __thiscall ios_base_op_succ(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return ios_base_fail(this);
}

/* ??Bios_base@std@@QBEPAXXZ */
/* ??Bios_base@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_fail, 4)
void* __thiscall ios_base_op_fail(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return ios_base_fail(this) ? NULL : (void*)this;
}

/* ?_Addstd@ios_base@std@@SAXPAV12@@Z */
/* ?_Addstd@ios_base@std@@SAXPEAV12@@Z */
void CDECL ios_base_Addstd(ios_base *add)
{
    FIXME("(%p) stub\n", add);
}

/* ?_Index_func@ios_base@std@@CAAAHXZ */
/* ?_Index_func@ios_base@std@@CAAEAHXZ */
int* CDECL ios_base_Index_func(void)
{
    TRACE("\n");
    return &ios_base_Index;
}

/* ?_Init@ios_base@std@@IAEXXZ */
/* ?_Init@ios_base@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_Init, 4)
void __thiscall ios_base_Init(ios_base *this)
{
    TRACE("(%p)\n", this);

    this->stdstr = 0;
    this->state = this->except = IOSTATE_goodbit;
    this->fmtfl = FMTFLAG_skipws | FMTFLAG_dec;
    this->prec = 6;
    this->wide = 0;
    this->arr = NULL;
    this->calls = NULL;
    this->loc = MSVCRT_operator_new(sizeof(locale));
    locale_ctor(this->loc);
}

/* ?_Sync_func@ios_base@std@@CAAA_NXZ */
/* ?_Sync_func@ios_base@std@@CAAEA_NXZ */
MSVCP_bool* CDECL ios_base_Sync_func(void)
{
    TRACE("\n");
    return &ios_base_Sync;
}

/* ?bad@ios_base@std@@QBE_NXZ */
/* ?bad@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_bad, 4)
MSVCP_bool __thiscall ios_base_bad(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_badbit) != 0;
}

/* ?eof@ios_base@std@@QBE_NXZ */
/* ?eof@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_eof, 4)
MSVCP_bool __thiscall ios_base_eof(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_eofbit) != 0;
}

/* ?flags@ios_base@std@@QAEHH@Z */
/* ?flags@ios_base@std@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_flags_set, 8)
IOSB_fmtflags __thiscall ios_base_flags_set(ios_base *this, IOSB_fmtflags flags)
{
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x)\n", this, flags);

    this->fmtfl = flags & FMTFLAG_mask;
    return ret;
}

/* ?flags@ios_base@std@@QBEHXZ */
/* ?flags@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_flags_get, 4)
IOSB_fmtflags __thiscall ios_base_flags_get(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->fmtfl;
}

/* ?getloc@ios_base@std@@QBE?AVlocale@2@XZ */
/* ?getloc@ios_base@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_getloc, 8)
locale* __thiscall ios_base_getloc(const ios_base *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, this->loc);
}

/* ?good@ios_base@std@@QBE_NXZ */
/* ?good@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_good, 4)
MSVCP_bool __thiscall ios_base_good(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->state == IOSTATE_goodbit;
}

/* ?imbue@ios_base@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@ios_base@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_imbue, 12)
locale* __thiscall ios_base_imbue(ios_base *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    *ret = *this->loc;
    locale_copy_ctor(this->loc, loc);
    return ret;
}

/* ?precision@ios_base@std@@QAEHH@Z */
/* ?precision@ios_base@std@@QEAA_J_J@Z */
DEFINE_THISCALL_WRAPPER(ios_base_precision_set, 8)
streamsize __thiscall ios_base_precision_set(ios_base *this, streamsize precision)
{
    streamsize ret = this->prec;

    TRACE("(%p %ld)\n", this, precision);

    this->prec = precision;
    return ret;
}

/* ?precision@ios_base@std@@QBEHXZ */
/* ?precision@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_precision_get, 4)
streamsize __thiscall ios_base_precision_get(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->prec;
}

/* ?rdstate@ios_base@std@@QBEHXZ */
/* ?rdstate@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_rdstate, 4)
IOSB_iostate __thiscall ios_base_rdstate(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->state;
}

/* ?setf@ios_base@std@@QAEHHH@Z */
/* ?setf@ios_base@std@@QEAAHHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setf_mask, 12)
IOSB_fmtflags __thiscall ios_base_setf_mask(ios_base *this, IOSB_fmtflags flags, IOSB_fmtflags mask)
{
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x %x)\n", this, flags, mask);

    this->fmtfl = (this->fmtfl & (~mask)) | (flags & mask & FMTFLAG_mask);
    return ret;
}

/* ?setf@ios_base@std@@QAEHH@Z */
/* ?setf@ios_base@std@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setf, 8)
IOSB_fmtflags __thiscall ios_base_setf(ios_base *this, IOSB_fmtflags flags)
{
    return ios_base_setf_mask(this, flags, ~0);
}

/* ?setstate@ios_base@std@@QAEXH_N@Z */
/* ?setstate@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate_reraise, 12)
void __thiscall ios_base_setstate_reraise(ios_base *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        ios_base_clear_reraise(this, this->state | state, reraise);
}

/* ?setstate@ios_base@std@@QAEXH@Z */
/* ?setstate@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate, 8)
void __thiscall ios_base_setstate(ios_base *this, IOSB_iostate state)
{
    ios_base_setstate_reraise(this, state, FALSE);
}

/* ?setstate@ios_base@std@@QAEXI@Z */
/* ?setstate@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate_unsigned, 8)
void __thiscall ios_base_setstate_unsigned(ios_base *this, unsigned int state)
{
    ios_base_setstate_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?sync_with_stdio@ios_base@std@@SA_N_N@Z */
MSVCP_bool CDECL ios_base_sync_with_stdio(MSVCP_bool sync)
{
    _Lockit lock;
    MSVCP_bool ret;

    TRACE("(%x)\n", sync);

    _Lockit_ctor_locktype(&lock, _LOCK_STREAM);
    ret = ios_base_Sync;
    ios_base_Sync = sync;
    _Lockit_dtor(&lock);
    return ret;
}

/* ?unsetf@ios_base@std@@QAEXH@Z */
/* ?unsetf@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_unsetf, 8)
void __thiscall ios_base_unsetf(ios_base *this, IOSB_fmtflags flags)
{
    TRACE("(%p %x)\n", this, flags);
    this->fmtfl &= ~flags;
}

/* ?width@ios_base@std@@QAEHH@Z */
/* ?width@ios_base@std@@QEAA_J_J@Z */
DEFINE_THISCALL_WRAPPER(ios_base_width_set, 8)
streamsize __thiscall ios_base_width_set(ios_base *this, streamsize width)
{
    streamsize ret = this->wide;

    TRACE("(%p %ld)\n", this, width);

    this->wide = width;
    return ret;
}

/* ?width@ios_base@std@@QBEHXZ */
/* ?width@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_width_get, 4)
streamsize __thiscall ios_base_width_get(ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->wide;
}

/* ?xalloc@ios_base@std@@SAHXZ */
int CDECL ios_base_xalloc(void)
{
    _Lockit lock;
    int ret;

    TRACE("\n");

    _Lockit_ctor_locktype(&lock, _LOCK_STREAM);
    ret = ios_base_Index++;
    _Lockit_dtor(&lock);
    return ret;
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_ctor, 4)
basic_ios_char* __thiscall basic_ios_char_ctor(basic_ios_char *this)
{
    TRACE("(%p)\n", this);

    ios_base_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_ios_char_vtable;
    return this;
}

/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IAEXPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_init, 12)
void __thiscall basic_ios_char_init(basic_ios_char *this, basic_streambuf_char *streambuf, MSVCP_bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base_Init(&this->base);
    this->strbuf = streambuf;
    this->stream = NULL;
    this->fillch = ' ';

    if(!streambuf)
        ios_base_setstate(&this->base, IOSTATE_badbit);

    if(isstd)
        FIXME("standard streams not handled yet\n");
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_ctor_streambuf, 8)
basic_ios_char* __thiscall basic_ios_char_ctor_streambuf(basic_ios_char *this, basic_streambuf_char *strbuf)
{
    TRACE("(%p %p)\n", this, strbuf);

    basic_ios_char_ctor(this);
    basic_ios_char_init(this, strbuf, FALSE);
    return this;
}

/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_dtor, 4)
void __thiscall basic_ios_char_dtor(basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    ios_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ios_char_vector_dtor, 8)
basic_ios_char* __thiscall MSVCP_basic_ios_char_vector_dtor(basic_ios_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ios_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_clear_reraise, 12)
void __thiscall basic_ios_char_clear_reraise(basic_ios_char *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);
    ios_base_clear_reraise(&this->base, state | (this->strbuf ? IOSTATE_goodbit : IOSTATE_badbit), reraise);
}

/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_clear, 8)
void __thiscall basic_ios_char_clear(basic_ios_char *this, unsigned int state)
{
    basic_ios_char_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?copyfmt@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_copyfmt, 8)
basic_ios_char* __thiscall basic_ios_char_copyfmt(basic_ios_char *this, basic_ios_char *copy)
{
    TRACE("(%p %p)\n", this, copy);
    if(this == copy)
        return this;

    this->stream = copy->stream;
    this->fillch = copy->fillch;
    ios_base_copyfmt(&this->base, &copy->base);
    return this;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEDD@Z */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_set, 8)
char __thiscall basic_ios_char_fill_set(basic_ios_char *this, char fill)
{
    char ret = this->fillch;

    TRACE("(%p %c)\n", this, fill);

    this->fillch = fill;
    return ret;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDXZ */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_get, 4)
char __thiscall basic_ios_char_fill_get(basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->fillch;
}

/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_imbue, 12)
locale *__thiscall basic_ios_char_imbue(basic_ios_char *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p %p)\n", this, ret, loc);

    if(this->strbuf) {
        basic_streambuf_char_pubimbue(this->strbuf, ret, loc);
        locale_dtor(ret);
    }

    return ios_base_imbue(&this->base, ret, loc);
}

/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDDD@Z */
/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_narrow, 12)
char __thiscall basic_ios_char_narrow(basic_ios_char *this, char ch, char def)
{
    TRACE("(%p %c %c)\n", this, ch, def);
    return ctype_char_narrow_ch(ctype_char_use_facet(this->strbuf->loc), ch, def);
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_set, 8)
basic_streambuf_char* __thiscall basic_ios_char_rdbuf_set(basic_ios_char *this, basic_streambuf_char *streambuf)
{
    basic_streambuf_char *ret = this->strbuf;

    TRACE("(%p %p)\n", this, streambuf);

    this->strbuf = streambuf;
    basic_ios_char_clear(this, IOSTATE_goodbit);
    return ret;
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_get, 4)
basic_streambuf_char* __thiscall basic_ios_char_rdbuf_get(const basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->strbuf;
}

/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_setstate_reraise, 12)
void __thiscall basic_ios_char_setstate_reraise(basic_ios_char *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        basic_ios_char_clear_reraise(this, this->base.state | state, reraise);
}

/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_setstate, 8)
void __thiscall basic_ios_char_setstate(basic_ios_char *this, unsigned int state)
{
    basic_ios_char_setstate_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEPAV?$basic_ostream@DU?$char_traits@D@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAPEAV?$basic_ostream@DU?$char_traits@D@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_tie_set, 8)
basic_ostream_char* __thiscall basic_ios_char_tie_set(basic_ios_char *this, basic_ostream_char *ostream)
{
    basic_ostream_char *ret = this->stream;

    TRACE("(%p %p)\n", this, ostream);

    this->stream = ostream;
    return ret;
}

/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_tie_get, 4)
basic_ostream_char* __thiscall basic_ios_char_tie_get(const basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->stream;
}

/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDD@Z */
/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_widen, 8)
char __thiscall basic_ios_char_widen(basic_ios_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ctype_char_widen_ch(ctype_char_use_facet(this->strbuf->loc), ch);
}


/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_ctor, 4)
basic_ios_wchar* __thiscall basic_ios_wchar_ctor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);

    ios_base_ctor(&this->base);
    this->base.vtable = &MSVCP_basic_ios_wchar_vtable;
    return this;
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor, 4)
basic_ios_wchar* __thiscall basic_ios_short_ctor(basic_ios_wchar *this)
{
    basic_ios_wchar_ctor(this);
    this->base.vtable = &MSVCP_basic_ios_short_vtable;
    return this;
}

/* ?init@?$basic_ios@_WU?$char_traits@_W@std@@@std@@IAEXPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_N@Z */
/* ?init@?$basic_ios@_WU?$char_traits@_W@std@@@std@@IEAAXPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_N@Z */
/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IAEXPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IEAAXPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_init, 12)
void __thiscall basic_ios_wchar_init(basic_ios_wchar *this, basic_streambuf_wchar *streambuf, MSVCP_bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base_Init(&this->base);
    this->strbuf = streambuf;
    this->stream = NULL;
    this->fillch = ' ';

    if(!streambuf)
        ios_base_setstate(&this->base, IOSTATE_badbit);

    if(isstd)
        FIXME("standard streams not handled yet\n");
}

/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_ctor_streambuf, 8)
basic_ios_wchar* __thiscall basic_ios_wchar_ctor_streambuf(basic_ios_wchar *this, basic_streambuf_wchar *strbuf)
{
    TRACE("(%p %p)\n", this, strbuf);

    basic_ios_wchar_ctor(this);
    basic_ios_wchar_init(this, strbuf, FALSE);
    return this;
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor_streambuf, 8)
basic_ios_wchar* __thiscall basic_ios_short_ctor_streambuf(basic_ios_wchar *this, basic_streambuf_wchar *strbuf)
{
    basic_ios_wchar_ctor_streambuf(this, strbuf);
    this->base.vtable = &MSVCP_basic_ios_short_vtable;
    return this;
}

/* ??1?$basic_ios@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_dtor, 4)
void __thiscall basic_ios_wchar_dtor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    ios_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ios_wchar_vector_dtor, 8)
basic_ios_wchar* __thiscall MSVCP_basic_ios_wchar_vector_dtor(basic_ios_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ios_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ios_short_vector_dtor, 8)
basic_ios_wchar* __thiscall MSVCP_basic_ios_short_vector_dtor(basic_ios_wchar *this, unsigned int flags)
{
    return MSVCP_basic_ios_wchar_vector_dtor(this, flags);
}

/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXH_N@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_clear_reraise, 12)
void __thiscall basic_ios_wchar_clear_reraise(basic_ios_wchar *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);
    ios_base_clear_reraise(&this->base, state | (this->strbuf ? IOSTATE_goodbit : IOSTATE_badbit), reraise);
}

/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXI@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_clear, 8)
void __thiscall basic_ios_wchar_clear(basic_ios_wchar *this, unsigned int state)
{
    basic_ios_wchar_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?copyfmt@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEBV12@@Z */
/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_copyfmt, 8)
basic_ios_wchar* __thiscall basic_ios_wchar_copyfmt(basic_ios_wchar *this, basic_ios_wchar *copy)
{
    TRACE("(%p %p)\n", this, copy);
    if(this == copy)
        return this;

    this->stream = copy->stream;
    this->fillch = copy->fillch;
    ios_base_copyfmt(&this->base, &copy->base);
    return this;
}

/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE_W_W@Z */
/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA_W_W@Z */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_fill_set, 8)
wchar_t __thiscall basic_ios_wchar_fill_set(basic_ios_wchar *this, wchar_t fill)
{
    wchar_t ret = this->fillch;

    TRACE("(%p %c)\n", this, fill);

    this->fillch = fill;
    return ret;
}

/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBE_WXZ */
/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBA_WXZ */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGXZ */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_fill_get, 4)
wchar_t __thiscall basic_ios_wchar_fill_get(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->fillch;
}

/* ?imbue@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_imbue, 12)
locale *__thiscall basic_ios_wchar_imbue(basic_ios_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p %p)\n", this, ret, loc);

    if(this->strbuf)
        return basic_streambuf_wchar_pubimbue(this->strbuf, ret, loc);

    locale_copy_ctor(ret, loc);
    return ret;
}

/* ?narrow@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBED_WD@Z */
/* ?narrow@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAD_WD@Z */
/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEDGD@Z */
/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_narrow, 12)
char __thiscall basic_ios_wchar_narrow(basic_ios_wchar *this, wchar_t ch, char def)
{
    TRACE("(%p %c %c)\n", this, ch, def);
    return ctype_wchar_narrow_ch(ctype_wchar_use_facet(this->strbuf->loc), ch, def);
}

/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PEAV32@@Z */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_rdbuf_set, 8)
basic_streambuf_wchar* __thiscall basic_ios_wchar_rdbuf_set(basic_ios_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_streambuf_wchar *ret = this->strbuf;

    TRACE("(%p %p)\n", this, streambuf);

    this->strbuf = streambuf;
    basic_ios_wchar_clear(this, IOSTATE_goodbit);
    return ret;
}

/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_rdbuf_get, 4)
basic_streambuf_wchar* __thiscall basic_ios_wchar_rdbuf_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->strbuf;
}

/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXH_N@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_setstate_reraise, 12)
void __thiscall basic_ios_wchar_setstate_reraise(basic_ios_wchar *this, IOSB_iostate state, MSVCP_bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        basic_ios_wchar_clear_reraise(this, this->base.state | state, reraise);
}

/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXI@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_setstate, 8)
void __thiscall basic_ios_wchar_setstate(basic_ios_wchar *this, IOSB_iostate state)
{
    basic_ios_wchar_setstate_reraise(this, state, FALSE);
}

/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEPAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAPEAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@PEAV32@@Z */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_tie_set, 8)
basic_ostream_wchar* __thiscall basic_ios_wchar_tie_set(basic_ios_wchar *this, basic_ostream_wchar *ostream)
{
    basic_ostream_wchar *ret = this->stream;

    TRACE("(%p %p)\n", this, ostream);

    this->stream = ostream;
    return ret;
}

/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@XZ */
/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@XZ */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_tie_get, 4)
basic_ostream_wchar* __thiscall basic_ios_wchar_tie_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->stream;
}

/* ?widen@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBE_WD@Z */
/* ?widen@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBA_WD@Z */
/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGD@Z */
/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_widen, 8)
wchar_t __thiscall basic_ios_wchar_widen(basic_ios_wchar *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ctype_wchar_widen_ch(ctype_wchar_use_facet(this->strbuf->loc), ch);
}

/* Caution: basic_ostream uses virtual inheritance.
 * All constructors have additional parameter that says if base class should be initialized.
 * Base class needs to be accessed using vbtable.
 */
static inline basic_ios_char* basic_ostream_char_get_basic_ios(basic_ostream_char *this)
{
    return (basic_ios_char*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_char* basic_ostream_char_to_basic_ios(basic_ostream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ostream_char_vbtable[1]);
}

static inline basic_ostream_char* basic_ostream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ostream_char*)((char*)ptr-basic_ostream_char_vbtable[1]);
}

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor, 16)
basic_ostream_char* __thiscall basic_ostream_char_ctor(basic_ostream_char *this,
        basic_streambuf_char *strbuf, MSVCP_bool isstd, MSVCP_bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_ostream_char_vtable;
    basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@_N@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor_uninitialized, 16)
basic_ostream_char* __thiscall basic_ostream_char_ctor_uninitialized(basic_ostream_char *this,
        int uninitialized, MSVCP_bool addstd, MSVCP_bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %d %x)\n", this, uninitialized, addstd);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_ostream_char_vtable;
    if(addstd)
        ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_ostream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_dtor, 4)
void __thiscall basic_ostream_char_dtor(basic_ios_char *base)
{
    basic_ostream_char *this = basic_ostream_char_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_vbase_dtor, 4)
void __thiscall basic_ostream_char_vbase_dtor(basic_ostream_char *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_ostream_char_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ostream_char_vector_dtor, 8)
basic_ostream_char* __thiscall MSVCP_basic_ostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostream_char *this = basic_ostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ostream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?flush@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_flush, 4)
basic_ostream_char* __thiscall basic_ostream_char_flush(basic_ostream_char *this)
{
    /* this function is not matching C++ specification */
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(basic_ios_char_rdbuf_get(base) && ios_base_good(&base->base)
            && basic_streambuf_char_pubsync(basic_ios_char_rdbuf_get(base))==-1)
        basic_ios_char_setstate(base, IOSTATE_badbit);
    return this;
}

/* ?_Osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char__Osfx, 4)
void __thiscall basic_ostream_char__Osfx(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_char_flush(this);
}

/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_osfx, 4)
void __thiscall basic_ostream_char_osfx(basic_ostream_char *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_char__Osfx(this);
}

static BOOL basic_ostream_char_sentry_create(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(basic_ios_char_rdbuf_get(base))
        basic_streambuf_char__Lock(base->strbuf);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_char_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_char_sentry_destroy(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_char_osfx(ostr);

    if(basic_ios_char_rdbuf_get(base))
        basic_streambuf_char__Unlock(base->strbuf);
}

/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_opfx, 4)
MSVCP_bool __thiscall basic_ostream_char_opfx(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_char_flush(base->stream);
    return ios_base_good(&base->base);
}

/* ?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@D@Z */
/* ?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@D@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_put, 8)
basic_ostream_char* __thiscall basic_ostream_char_put(basic_ostream_char *this, char ch)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if(!basic_ostream_char_sentry_create(this)
            || basic_streambuf_char_sputc(base->strbuf, ch)==EOF) {
        basic_ostream_char_sentry_destroy(this);
        basic_ios_char_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_char_sentry_destroy(this);
    return this;
}

/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_seekp, 12)
basic_ostream_char* __thiscall basic_ostream_char_seekp(basic_ostream_char *this, streamoff off, int way)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %ld %d)\n", this, off, way);

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==0 && seek.pos==-1 && seek.state==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_seekp_fpos, 28)
basic_ostream_char* __thiscall basic_ostream_char_seekp_fpos(basic_ostream_char *this, fpos_int pos)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_char_pubseekpos(basic_ios_char_rdbuf_get(base),
                &seek, pos, OPENMODE_out);
        if(seek.off==0 && seek.pos==-1 && seek.state==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?tellp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_tellp, 8)
fpos_int* __thiscall basic_ostream_char_tellp(basic_ostream_char *this, fpos_int *ret)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_out);
    }else {
        ret->off = 0;
        ret->pos = -1;
        ret->state = 0;
    }
    return ret;
}

/* ?write@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@PBDH@Z */
/* ?write@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEBD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_write, 12)
basic_ostream_char* __thiscall basic_ostream_char_write(basic_ostream_char *this, const char *str, streamsize count)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %s %ld)\n", this, debugstr_a(str), count);

    if(!basic_ostream_char_sentry_create(this)
            || basic_streambuf_char_sputn(base->strbuf, str, count)!=count) {
        basic_ostream_char_sentry_destroy(this);
        basic_ios_char_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_char_sentry_destroy(this);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_short, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_short(basic_ostream_char *this, short val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_long(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base),
                (ios_base_flags_get(&base->base) & FMTFLAG_basefield & (FMTFLAG_oct | FMTFLAG_hex))
                ? (LONG)((unsigned short)val) : (LONG)val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ushort, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_ushort(basic_ostream_char *this, unsigned short val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_ulong(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_int, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_int(basic_ostream_char *this, int val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_long(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_uint, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_uint(basic_ostream_char *this, unsigned int val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_ulong(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_float, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_float(basic_ostream_char *this, float val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %f)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_double(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_double, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_double(basic_ostream_char *this, double val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_double(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ldouble, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_ldouble(basic_ostream_char *this, double val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_ldouble(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_streambuf, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_streambuf(basic_ostream_char *this, basic_streambuf_char *val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_badbit;
    int c = '\n';

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        for(c = basic_streambuf_char_sgetc(val); c!=EOF;
                c = basic_streambuf_char_snextc(val)) {
            state = IOSTATE_goodbit;

            if(basic_streambuf_char_sputc(base->strbuf, c) == EOF) {
                state = IOSTATE_badbit;
                break;
            }
        }
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(this);

    ios_base_width_set(&base->base, 0);
    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ptr, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_ptr(basic_ostream_char *this, const void *val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_ptr(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_int64, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_int64(basic_ostream_char *this, __int64 val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_int64(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_uint64, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_uint64(basic_ostream_char *this, unsigned __int64 val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_uint64(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_bool, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_bool(basic_ostream_char *this, MSVCP_bool val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(strbuf->loc);
        ostreambuf_iterator_char dest = {0, strbuf};

        num_put_char_put_bool(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?endl@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl basic_ostream_char_endl(basic_ostream_char *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_char_put(ostr, '\n');
    basic_ostream_char_flush(ostr);
    return ostr;
}

/* $?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_bstr(basic_ostream_char *ostr, const basic_string_char *str)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_char_sentry_create(ostr)) {
        MSVCP_size_t len = MSVCP_basic_string_char_length(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputn(base->strbuf, MSVCP_basic_string_char_c_str(str), len) != len)
                    state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@C@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@C@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@D@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@D@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@E@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@E@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_ch(basic_ostream_char *ostr, char ch)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", ostr, ch);

    if(basic_ostream_char_sentry_create(ostr)) {
        streamsize pad = (base->base.wide>1 ? base->base.wide-1 : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputc(base->strbuf, ch) == EOF)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBC@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBC@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBD@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBD@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBE@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBE@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_str(basic_ostream_char *ostr, const char *str)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %s)\n", ostr, str);

    if(basic_ostream_char_sentry_create(ostr)) {
        MSVCP_size_t len = strlen(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputn(base->strbuf, str, len) != len)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func(basic_ostream_char *this,
        basic_ostream_char* (__cdecl *pfunc)(basic_ostream_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@DU?$char_traits@D@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@DU?$char_traits@D@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func_basic_ios, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func_basic_ios(basic_ostream_char *this,
        basic_ios_char* (__cdecl *pfunc)(basic_ios_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_ostream_char_get_basic_ios(this));
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func_ios_base, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func_ios_base(
        basic_ostream_char *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_ostream_char_get_basic_ios(this)->base);
    return this;
}

/* Caution: basic_ostream uses virtual inheritance. */
static inline basic_ios_wchar* basic_ostream_wchar_get_basic_ios(basic_ostream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_ostream_wchar_to_basic_ios(basic_ostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostream_wchar_vbtable[1]);
}

static inline basic_ostream_wchar* basic_ostream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostream_wchar*)((char*)ptr-basic_ostream_wchar_vbtable[1]);
}

/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_ctor, 16)
basic_ostream_wchar* __thiscall basic_ostream_wchar_ctor(basic_ostream_wchar *this,
        basic_streambuf_wchar *strbuf, MSVCP_bool isstd, MSVCP_bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_wchar_vbtable;
        base = basic_ostream_wchar_get_basic_ios(this);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_ostream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_ostream_wchar_vtable;
    basic_ios_wchar_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE@W4_Uninitialized@1@_N@Z */
/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA@W4_Uninitialized@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_ctor_uninitialized, 16)
basic_ostream_wchar* __thiscall basic_ostream_wchar_ctor_uninitialized(basic_ostream_wchar *this,
        int uninitialized, MSVCP_bool addstd, MSVCP_bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d %x)\n", this, uninitialized, addstd);

    if(virt_init) {
        this->vbtable = basic_ostream_wchar_vbtable;
        base = basic_ostream_wchar_get_basic_ios(this);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_ostream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_ostream_wchar_vtable;
    if(addstd)
        ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_ostream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_dtor, 4)
void __thiscall basic_ostream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ostream_wchar *this = basic_ostream_wchar_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_vbase_dtor, 4)
void __thiscall basic_ostream_wchar_vbase_dtor(basic_ostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_ostream_wchar_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ostream_wchar_vector_dtor, 8)
basic_ostream_wchar* __thiscall MSVCP_basic_ostream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostream_wchar *this = basic_ostream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ostream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?flush@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_flush, 4)
basic_ostream_wchar* __thiscall basic_ostream_wchar_flush(basic_ostream_wchar *this)
{
    /* this function is not matching C++ specification */
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(basic_ios_wchar_rdbuf_get(base) && ios_base_good(&base->base)
            && basic_streambuf_wchar_pubsync(basic_ios_wchar_rdbuf_get(base))==-1)
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
    return this;
}

/* ?_Osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar__Osfx, 4)
void __thiscall basic_ostream_wchar__Osfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_wchar_flush(this);
}

/* ?osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_osfx, 4)
void __thiscall basic_ostream_wchar_osfx(basic_ostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_wchar__Osfx(this);
}

static BOOL basic_ostream_wchar_sentry_create(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Lock(base->strbuf);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_wchar_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_wchar_sentry_destroy(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_wchar_osfx(ostr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Unlock(base->strbuf);
}

/* ?opfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_opfx, 4)
MSVCP_bool __thiscall basic_ostream_wchar_opfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_wchar_flush(base->stream);
    return ios_base_good(&base->base);
}

/* ?put@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@_W@Z */
/* ?put@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_W@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_put, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_put(basic_ostream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if(!basic_ostream_wchar_sentry_create(this)
            || basic_streambuf_wchar_sputc(base->strbuf, ch)==WEOF) {
        basic_ostream_wchar_sentry_destroy(this);
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_wchar_sentry_destroy(this);
    return this;
}

/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_seekp, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_seekp(basic_ostream_wchar *this, streamoff off, int way)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %ld %d)\n", this, off, way);

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==0 && seek.pos==-1 && seek.state==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_seekp_fpos, 28)
basic_ostream_wchar* __thiscall basic_ostream_wchar_seekp_fpos(basic_ostream_wchar *this, fpos_int pos)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_wchar_pubseekpos(basic_ios_wchar_rdbuf_get(base),
                &seek, pos, OPENMODE_out);
        if(seek.off==0 && seek.pos==-1 && seek.state==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?tellp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_tellp, 8)
fpos_int* __thiscall basic_ostream_wchar_tellp(basic_ostream_wchar *this, fpos_int *ret)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_out);
    }else {
        ret->off = 0;
        ret->pos = -1;
        ret->state = 0;
    }
    return ret;
}

/* ?write@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PB_WH@Z */
/* ?write@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEB_W_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_write, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_write(basic_ostream_wchar *this, const wchar_t *str, streamsize count)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %s %ld)\n", this, debugstr_w(str), count);

    if(!basic_ostream_wchar_sentry_create(this)
            || basic_streambuf_wchar_sputn(base->strbuf, str, count)!=count) {
        basic_ostream_wchar_sentry_destroy(this);
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_wchar_sentry_destroy(this);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_short, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_short(basic_ostream_wchar *this, short val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base),
                (ios_base_flags_get(&base->base) & FMTFLAG_basefield & (FMTFLAG_oct | FMTFLAG_hex))
                ? (LONG)((unsigned short)val) : (LONG)val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ushort, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ushort(basic_ostream_wchar *this, unsigned short val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_int, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_int(basic_ostream_wchar *this, int val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_uint, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_uint(basic_ostream_wchar *this, unsigned int val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_float, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_float(basic_ostream_wchar *this, float val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %f)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_double, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_double(basic_ostream_wchar *this, double val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ldouble, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ldouble(basic_ostream_wchar *this, double val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_ldouble(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_streambuf, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_streambuf(basic_ostream_wchar *this, basic_streambuf_wchar *val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_badbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        for(c = basic_streambuf_wchar_sgetc(val); c!=WEOF;
                c = basic_streambuf_wchar_snextc(val)) {
            state = IOSTATE_goodbit;

            if(basic_streambuf_wchar_sputc(base->strbuf, c) == WEOF) {
                state = IOSTATE_badbit;
                break;
            }
        }
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(this);

    ios_base_width_set(&base->base, 0);
    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ptr, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ptr(basic_ostream_wchar *this, const void *val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_ptr(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_int64, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_int64(basic_ostream_wchar *this, __int64 val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_int64(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_uint64, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_uint64(basic_ostream_wchar *this, unsigned __int64 val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_uint64(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_bool, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_bool(basic_ostream_wchar *this, MSVCP_bool val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_put *numput = num_put_wchar_use_facet(strbuf->loc);
        ostreambuf_iterator_wchar dest = {0, strbuf};

        num_put_wchar_put_bool(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?endl@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_endl(basic_ostream_wchar *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_wchar_put(ostr, '\n');
    basic_ostream_wchar_flush(ostr);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?6_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_bstr(basic_ostream_wchar *ostr, const basic_string_wchar *str)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_wchar_sentry_create(ostr)) {
        MSVCP_size_t len = MSVCP_basic_string_wchar_length(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputn(base->strbuf, MSVCP_basic_string_wchar_c_str(str), len) != len)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@_W@Z */
/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@_W@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_ch(basic_ostream_wchar *ostr, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", ostr, ch);

    if(basic_ostream_wchar_sentry_create(ostr)) {
        streamsize pad = (base->base.wide>1 ? base->base.wide-1 : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputc(base->strbuf, ch) == WEOF)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@PB_W@Z */
/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@PEB_W@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_str(basic_ostream_wchar *ostr, const wchar_t *str)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %s)\n", ostr, debugstr_w(str));

    if(basic_ostream_wchar_sentry_create(ostr)) {
        MSVCP_size_t len = wcslen(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputn(base->strbuf, str, len) != len)
                                        state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func(basic_ostream_wchar *this,
        basic_ostream_wchar* (__cdecl *pfunc)(basic_ostream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func_basic_ios, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func_basic_ios(basic_ostream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_ostream_wchar_get_basic_ios(this));
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func_ios_base, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func_ios_base(
        basic_ostream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_ostream_wchar_get_basic_ios(this)->base);
    return this;
}

/* Caution: basic_istream uses virtual inheritance. */
static inline basic_ios_char* basic_istream_char_get_basic_ios(basic_istream_char *this)
{
    return (basic_ios_char*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_char* basic_istream_char_to_basic_ios(basic_istream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_istream_char_vbtable[1]);
}

static inline basic_istream_char* basic_istream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_istream_char*)((char*)ptr-basic_istream_char_vbtable[1]);
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor_init, 20)
basic_istream_char* __thiscall basic_istream_char_ctor_init(basic_istream_char *this, basic_streambuf_char *strbuf, MSVCP_bool isstd, MSVCP_bool noinit, MSVCP_bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d %d)\n", this, strbuf, isstd, noinit, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_char_vbtable;
        base = basic_istream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_istream_char_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_istream_char_vtable;
    this->count = 0;
    if(!noinit)
        basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor, 16)
basic_istream_char* __thiscall basic_istream_char_ctor(basic_istream_char *this, basic_streambuf_char *strbuf, MSVCP_bool isstd, MSVCP_bool virt_init)
{
    return basic_istream_char_ctor_init(this, strbuf, isstd, FALSE, virt_init);
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor_uninitialized, 12)
basic_istream_char* __thiscall basic_istream_char_ctor_uninitialized(basic_istream_char *this, int uninitialized, MSVCP_bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %d %d)\n", this, uninitialized, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_char_vbtable;
        base = basic_istream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_istream_char_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_istream_char_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_istream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_dtor, 4)
void __thiscall basic_istream_char_dtor(basic_ios_char *base)
{
    basic_istream_char *this = basic_istream_char_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_vbase_dtor, 4)
void __thiscall basic_istream_char_vbase_dtor(basic_istream_char *this)
{
    TRACE("(%p)\n", this);
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_istream_char_vector_dtor, 8)
basic_istream_char* __thiscall MSVCP_basic_istream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istream_char *this = basic_istream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_istream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z */
/* ?_Ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Ipfx, 8)
MSVCP_bool __thiscall basic_istream_char__Ipfx(basic_istream_char *this, MSVCP_bool noskip)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(!ios_base_good(&base->base)) {
        basic_ios_char_setstate(base, IOSTATE_failbit);
        return FALSE;
    }

    if(basic_ios_char_tie_get(base))
        basic_ostream_char_flush(basic_ios_char_tie_get(base));

    if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const ctype_char *ctype = ctype_char_use_facet(base->strbuf->loc);
        int ch;

        for(ch = basic_streambuf_char_sgetc(strbuf); ;
                ch = basic_streambuf_char_snextc(strbuf)) {
            if(ch == EOF) {
                basic_ios_char_setstate(base, IOSTATE_eofbit);
                return FALSE;
            }

            if(!ctype_char_is_ch(ctype, _SPACE|_BLANK, ch))
                break;
        }
    }

    return TRUE;
}

/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ipfx, 8)
MSVCP_bool __thiscall basic_istream_char_ipfx(basic_istream_char *this, MSVCP_bool noskip)
{
    return basic_istream_char__Ipfx(this, noskip);
}

/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_isfx, 4)
void __thiscall basic_istream_char_isfx(basic_istream_char *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_char_sentry_create(basic_istream_char *istr, MSVCP_bool noskip)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istr);

    if(basic_ios_char_rdbuf_get(base))
        basic_streambuf_char__Lock(base->strbuf);

    return basic_istream_char_ipfx(istr, noskip);
}

static void basic_istream_char_sentry_destroy(basic_istream_char *istr)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istr);

    if(basic_ios_char_rdbuf_get(base))
        basic_streambuf_char__Unlock(base->strbuf);
}

/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_gcount, 4)
int __thiscall basic_istream_char_gcount(const basic_istream_char *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get, 4)
int __thiscall basic_istream_char_get(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(!basic_istream_char_sentry_create(this, TRUE)) {
        basic_istream_char_sentry_destroy(this);
        return EOF;
    }

    ret = basic_streambuf_char_sbumpc(basic_ios_char_rdbuf_get(base));
    basic_istream_char_sentry_destroy(this);
    if(ret == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit|IOSTATE_failbit);
    else
        this->count++;

    return ret;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAD@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAD@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_ch, 8)
basic_istream_char* __thiscall basic_istream_char_get_ch(basic_istream_char *this, char *ch)
{
    int ret;

    TRACE("(%p %p)\n", this, ch);

    ret = basic_istream_char_get(this);
    if(ret != EOF)
        *ch = (char)ret;
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADHD@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_JD@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str_delim, 16)
basic_istream_char* __thiscall basic_istream_char_get_str_delim(basic_istream_char *this, char *str, streamsize count, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %p %ld %c)\n", this, str, count, delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        for(ch = basic_streambuf_char_sgetc(strbuf); count>1;
                ch = basic_streambuf_char_snextc(strbuf)) {
            if(ch==EOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str, 12)
basic_istream_char* __thiscall basic_istream_char_get_str(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char_get_str_delim(this, str, count, '\n');
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAV?$basic_streambuf@DU?$char_traits@D@std@@@2@D@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@D@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_streambuf_delim, 12)
basic_istream_char* __thiscall basic_istream_char_get_streambuf_delim(basic_istream_char *this, basic_streambuf_char *strbuf, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %p %c)\n", this, strbuf, delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf_read = basic_ios_char_rdbuf_get(base);

        for(ch = basic_streambuf_char_sgetc(strbuf_read); ;
                ch = basic_streambuf_char_snextc(strbuf_read)) {
            if(ch==EOF || ch==delim)
                break;

            if(basic_streambuf_char_sputc(strbuf, ch) == EOF)
                break;
            this->count++;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAV?$basic_streambuf@DU?$char_traits@D@std@@@2@@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_streambuf, 8)
basic_istream_char* __thiscall basic_istream_char_get_streambuf(basic_istream_char *this, basic_streambuf_char *strbuf)
{
    return basic_istream_char_get_streambuf_delim(this, strbuf, '\n');
}

/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADHD@Z */
/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_JD@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline_delim, 16)
basic_istream_char* __thiscall basic_istream_char_getline_delim(basic_istream_char *this, char *str, streamsize count, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %p %ld %c)\n", this, str, count, delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        while(count > 1) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }

        if(ch == delim)
            this->count++;
        else if(ch != EOF) {
            ch = basic_streambuf_char_sgetc(strbuf);

            if(ch == delim) {
                basic_streambuf_char__Gninc(strbuf);
                this->count++;
            }
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=delim && ch!=EOF) ? IOSTATE_failbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline, 12)
basic_istream_char* __thiscall basic_istream_char_getline(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char_getline_delim(this, str, count, '\n');
}

/* ?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@HH@Z */
/* ?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ignore, 12)
basic_istream_char* __thiscall basic_istream_char_ignore(basic_istream_char *this, streamsize count, int delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %ld %d)\n", this, count, delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        while(count > 0) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF || ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }
    basic_istream_char_sentry_destroy(this);

    if(ch == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit);
    return this;
}

/* ?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_peek, 4)
int __thiscall basic_istream_char_peek(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ret = EOF;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE))
        ret = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
    basic_istream_char_sentry_destroy(this);
    return ret;
}

/* ?_Read_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADIH@Z */
/* ?_Read_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Read_s, 16)
basic_istream_char* __thiscall basic_istream_char__Read_s(basic_istream_char *this, char *str, MSVCP_size_t size, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %lu %ld)\n", this, str, size, count);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        this->count = basic_streambuf_char__Sgetn_s(strbuf, str, size, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read, 12)
basic_istream_char* __thiscall basic_istream_char_read(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char__Read_s(this, str, count, count);
}

/* ?_Readsome_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHPADIH@Z */
/* ?_Readsome_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Readsome_s, 16)
streamsize __thiscall basic_istream_char__Readsome_s(basic_istream_char *this, char *str, MSVCP_size_t size, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %lu %ld)\n", this, str, size, count);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_char_in_avail(basic_ios_char_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_char__Read_s(this, str, size, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this->count;
}

/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_readsome, 12)
streamsize __thiscall basic_istream_char_readsome(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char__Readsome_s(this, str, count, count);
}

/* ?putback@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@D@Z */
/* ?putback@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@D@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_putback, 8)
basic_istream_char* __thiscall basic_istream_char_putback(basic_istream_char *this, char ch)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %c)\n", this, ch);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_char_sputbackc(strbuf, ch)==EOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?unget@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_unget, 4)
basic_istream_char* __thiscall basic_istream_char_unget(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_char_sungetc(strbuf)==EOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?sync@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_sync, 4)
int __thiscall basic_istream_char_sync(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

    TRACE("(%p)\n", this);

    if(!strbuf)
        return -1;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        if(basic_streambuf_char_pubsync(strbuf) != -1) {
            basic_istream_char_sentry_destroy(this);
            return 0;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, IOSTATE_badbit);
    return -1;
}

/* ?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_tellg, 8)
fpos_int* __thiscall basic_istream_char_tellg(basic_istream_char *this, fpos_int *ret)
{
    TRACE("(%p %p)\n", this, ret);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_ios_char *base = basic_istream_char_get_basic_ios(this);
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                    ret, 0, SEEKDIR_cur, OPENMODE_in);
            basic_istream_char_sentry_destroy(this);

            if(ret->off==0 && ret->pos==-1 && ret->state==0)
                basic_ios_char_setstate(base, IOSTATE_failbit);
            return ret;
        }
    }
    basic_istream_char_sentry_destroy(this);

    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg, 12)
basic_istream_char* __thiscall basic_istream_char_seekg(basic_istream_char *this, streamoff off, int dir)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %ld %d)\n", this, off, dir);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
            fpos_int ret;

            basic_streambuf_char_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);
            basic_istream_char_sentry_destroy(this);

            if(ret.off==0 && ret.pos==-1 && ret.state==0)
                basic_ios_char_setstate(base, IOSTATE_failbit);
            else
                basic_ios_char_clear(base, IOSTATE_goodbit);
            return this;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, IOSTATE_failbit);
    return this;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg_fpos, 28)
basic_istream_char* __thiscall basic_istream_char_seekg_fpos(basic_istream_char *this, fpos_int pos)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(basic_istream_char_sentry_create(this, TRUE)) {
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
            fpos_int ret;

            basic_streambuf_char_pubseekpos(strbuf, &ret, pos, OPENMODE_in);
            basic_istream_char_sentry_destroy(this);

            if(ret.off==0 && ret.pos==-1 && ret.state==0)
                basic_ios_char_setstate(base, IOSTATE_failbit);
            else
                basic_ios_char_clear(base, IOSTATE_goodbit);
            return this;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, IOSTATE_failbit);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_short, 8)
basic_istream_char* __thiscall basic_istream_char_read_short(basic_istream_char *this, short *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};
        LONG tmp;

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, &tmp);

        if(!(state&IOSTATE_failbit) && tmp==(LONG)((short)tmp))
            *v = tmp;
        else
            state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAG@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ushort, 8)
basic_istream_char* __thiscall basic_istream_char_read_ushort(basic_istream_char *this, unsigned short *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ushort(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_int, 8)
basic_istream_char* __thiscall basic_istream_char_read_int(basic_istream_char *this, int *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_uint, 8)
basic_istream_char* __thiscall basic_istream_char_read_uint(basic_istream_char *this, unsigned int *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_uint(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_long, 8)
basic_istream_char* __thiscall basic_istream_char_read_long(basic_istream_char *this, LONG *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ulong, 8)
basic_istream_char* __thiscall basic_istream_char_read_ulong(basic_istream_char *this, ULONG *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ulong(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_float, 8)
basic_istream_char* __thiscall basic_istream_char_read_float(basic_istream_char *this, float *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_float(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_double, 8)
basic_istream_char* __thiscall basic_istream_char_read_double(basic_istream_char *this, double *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_double(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ldouble, 8)
basic_istream_char* __thiscall basic_istream_char_read_ldouble(basic_istream_char *this, double *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ldouble(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ptr, 8)
basic_istream_char* __thiscall basic_istream_char_read_ptr(basic_istream_char *this, void **v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_void(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_int64, 8)
basic_istream_char* __thiscall basic_istream_char_read_int64(basic_istream_char *this, __int64 *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_int64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_uint64, 8)
basic_istream_char* __thiscall basic_istream_char_read_uint64(basic_istream_char *this, unsigned __int64 *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_uint64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_bool, 8)
basic_istream_char* __thiscall basic_istream_char_read_bool(basic_istream_char *this, MSVCP_bool *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(strbuf->loc);
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_bool(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z */
/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z */
basic_istream_char* __cdecl basic_istream_char_getline_bstr_delim(
        basic_istream_char *istream, basic_string_char *str, char delim)
{
    IOSB_iostate state = IOSTATE_failbit;
    int c = delim;

    TRACE("(%p %p %c)\n", istream, str, delim);

    if(basic_istream_char_sentry_create(istream, TRUE)) {
        MSVCP_basic_string_char_clear(str);

        for(c = basic_istream_char_get(istream); c!=delim && c!=EOF;
                c = basic_istream_char_get(istream)) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_char_append_ch(str, c);
        }
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(basic_istream_char_get_basic_ios(istream),
        state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_istream_char* __cdecl basic_istream_char_getline_bstr(
        basic_istream_char *istream, basic_string_char *str)
{
    return basic_istream_char_getline_bstr_delim(istream, str, '\n');
}

/* ??$?5DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?5DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_istream_char* __cdecl basic_istream_char_read_bstr(
        basic_istream_char *istream, basic_string_char *str)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        const ctype_char *ctype = ctype_char_use_facet(base->strbuf->loc);
        MSVCP_size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_char_clear(str);

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_char_append_ch(str, c);
        }

        ios_base_width_set(&base->base, 0);
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5DU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAD@Z */
/* ??$?5DU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAD@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAE@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAE@Z */
basic_istream_char* __cdecl basic_istream_char_read_str(basic_istream_char *istream, char *str)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        const ctype_char *ctype = ctype_char_use_facet(base->strbuf->loc);
        MSVCP_size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }

        *str = 0;
        ios_base_width_set(&base->base, 0);
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5DU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAD@Z */
/* ??$?5DU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAD@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAE@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAE@Z */
basic_istream_char* __cdecl basic_istream_char_read_ch(basic_istream_char *istream, char *ch)
{
    IOSB_iostate state = IOSTATE_failbit;
    int c = 0;

    TRACE("(%p %p)\n", istream, ch);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        c = basic_streambuf_char_sbumpc(basic_ios_char_rdbuf_get(
                    basic_istream_char_get_basic_ios(istream)));
        if(c != EOF) {
            state = IOSTATE_goodbit;
            *ch = c;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(basic_istream_char_get_basic_ios(istream),
            state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_streambuf, 8)
basic_istream_char* __thiscall basic_istream_char_read_streambuf(
        basic_istream_char *this, basic_streambuf_char *streambuf)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", this, streambuf);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base)); c!=EOF;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base))) {
            state = IOSTATE_goodbit;
            if(basic_streambuf_char_sputc(streambuf, c) == EOF)
                break;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func, 8)
basic_istream_char* __thiscall basic_istream_char_read_func(basic_istream_char *this,
        basic_istream_char* (__cdecl *pfunc)(basic_istream_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@DU?$char_traits@D@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@DU?$char_traits@D@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func_basic_ios, 8)
basic_istream_char* __thiscall basic_istream_char_read_func_basic_ios(basic_istream_char *this,
        basic_ios_char* (__cdecl *pfunc)(basic_ios_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_istream_char_get_basic_ios(this));
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func_ios_base, 8)
basic_istream_char* __thiscall basic_istream_char_read_func_ios_base(basic_istream_char *this,
        ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_istream_char_get_basic_ios(this)->base);
    return this;
}

/* Caution: basic_istream uses virtual inheritance. */
static inline basic_ios_wchar* basic_istream_wchar_get_basic_ios(basic_istream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_istream_wchar_to_basic_ios(basic_istream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istream_wchar_vbtable[1]);
}

static inline basic_istream_wchar* basic_istream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istream_wchar*)((char*)ptr-basic_istream_wchar_vbtable[1]);
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N1@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor_init, 20)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor_init(basic_istream_wchar *this, basic_streambuf_wchar *strbuf, MSVCP_bool isstd, MSVCP_bool noinit, MSVCP_bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d %d)\n", this, strbuf, isstd, noinit, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_wchar_vbtable;
        base = basic_istream_wchar_get_basic_ios(this);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_istream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_istream_wchar_vtable;
    this->count = 0;
    if(!noinit)
        basic_ios_wchar_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor, 16)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor(basic_istream_wchar *this, basic_streambuf_wchar *strbuf, MSVCP_bool isstd, MSVCP_bool virt_init)
{
    return basic_istream_wchar_ctor_init(this, strbuf, isstd, FALSE, virt_init);
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor_uninitialized, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor_uninitialized(basic_istream_wchar *this, int uninitialized, MSVCP_bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d %d)\n", this, uninitialized, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_wchar_vbtable;
        base = basic_istream_wchar_get_basic_ios(this);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_istream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &MSVCP_basic_istream_wchar_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_istream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_dtor, 4)
void __thiscall basic_istream_wchar_dtor(basic_ios_wchar *base)
{
    basic_istream_wchar *this = basic_istream_wchar_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_vbase_dtor, 4)
void __thiscall basic_istream_wchar_vbase_dtor(basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_istream_wchar_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_istream_wchar_vector_dtor, 8)
basic_istream_wchar* __thiscall MSVCP_basic_istream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istream_wchar *this = basic_istream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_istream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE_N_N@Z */
/* ?_Ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Ipfx, 8)
MSVCP_bool __thiscall basic_istream_wchar__Ipfx(basic_istream_wchar *this, MSVCP_bool noskip)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(!ios_base_good(&base->base)) {
        basic_ios_wchar_setstate(base, IOSTATE_failbit);
        return FALSE;
    }

    if(basic_ios_wchar_tie_get(base))
        basic_ostream_wchar_flush(basic_ios_wchar_tie_get(base));

    if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const ctype_wchar *ctype = ctype_wchar_use_facet(base->strbuf->loc);
        int ch;

        for(ch = basic_streambuf_wchar_sgetc(strbuf); ;
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch == WEOF) {
                basic_ios_wchar_setstate(base, IOSTATE_eofbit);
                return FALSE;
            }

            if(!ctype_wchar_is_ch(ctype, _SPACE|_BLANK, ch))
                break;
        }
    }

    return TRUE;
}

/* ?ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ipfx, 8)
MSVCP_bool __thiscall basic_istream_wchar_ipfx(basic_istream_wchar *this, MSVCP_bool noskip)
{
    return basic_istream_wchar__Ipfx(this, noskip);
}

/* ?isfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_isfx, 4)
void __thiscall basic_istream_wchar_isfx(basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_wchar_sentry_create(basic_istream_wchar *istr, MSVCP_bool noskip)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Lock(base->strbuf);

    return basic_istream_wchar_ipfx(istr, noskip);
}

static void basic_istream_wchar_sentry_destroy(basic_istream_wchar *istr)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Unlock(base->strbuf);
}

/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_gcount, 4)
int __thiscall basic_istream_wchar_gcount(const basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get, 4)
unsigned short __thiscall basic_istream_wchar_get(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(!basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_istream_wchar_sentry_destroy(this);
        return WEOF;
    }

    ret = basic_streambuf_wchar_sbumpc(basic_ios_wchar_rdbuf_get(base));
    basic_istream_wchar_sentry_destroy(this);
    if(ret == WEOF)
        basic_ios_wchar_setstate(base, IOSTATE_eofbit|IOSTATE_failbit);
    else
        this->count++;

    return ret;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AA_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEA_W@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_ch, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_get_ch(basic_istream_wchar *this, wchar_t *ch)
{
    unsigned short ret;

    TRACE("(%p %p)\n", this, ch);

    ret = basic_istream_wchar_get(this);
    if(ret != WEOF)
        *ch = (wchar_t)ret;
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J_W@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str_delim, 16)
basic_istream_wchar* __thiscall basic_istream_wchar_get_str_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %ld %c)\n", this, str, count, delim);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf); count>1;
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch==WEOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_get_str(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar_get_str_delim(this, str, count, '\n');
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_W@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_streambuf_delim, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_get_streambuf_delim(basic_istream_wchar *this, basic_streambuf_wchar *strbuf, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %c)\n", this, strbuf, delim);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf_read = basic_ios_wchar_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf_read); ;
                ch = basic_streambuf_wchar_snextc(strbuf_read)) {
            if(ch==WEOF || ch==delim)
                break;

            if(basic_streambuf_wchar_sputc(strbuf, ch) == WEOF)
                break;
            this->count++;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_get_streambuf(basic_istream_wchar *this, basic_streambuf_wchar *strbuf)
{
    return basic_istream_wchar_get_streambuf_delim(this, strbuf, '\n');
}

/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH_W@Z */
/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J_W@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline_delim, 16)
basic_istream_wchar* __thiscall basic_istream_wchar_getline_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %ld %c)\n", this, str, count, delim);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        while(count > 1) {
            ch = basic_streambuf_wchar_sbumpc(strbuf);

            if(ch==WEOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }

        if(ch == delim)
            this->count++;
        else if(ch != WEOF) {
            ch = basic_streambuf_wchar_sgetc(strbuf);

            if(ch == delim) {
                basic_streambuf_wchar__Gninc(strbuf);
                this->count++;
            }
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=delim && ch!=WEOF) ? IOSTATE_failbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_getline(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar_getline_delim(this, str, count, '\n');
}

/* ?ignore@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@HG@Z */
/* ?ignore@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ignore, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_ignore(basic_istream_wchar *this, streamsize count, unsigned short delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %ld %d)\n", this, count, delim);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        while(count > 0) {
            ch = basic_streambuf_wchar_sbumpc(strbuf);

            if(ch==WEOF || ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    if(ch == WEOF)
        basic_ios_wchar_setstate(base, IOSTATE_eofbit);
    return this;
}

/* ?peek@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?peek@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_peek, 4)
unsigned short __thiscall basic_istream_wchar_peek(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ret = WEOF;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE))
        ret = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
    basic_istream_wchar_sentry_destroy(this);
    return ret;
}

/* ?_Read_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WIH@Z */
/* ?_Read_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Read_s, 16)
basic_istream_wchar* __thiscall basic_istream_wchar__Read_s(basic_istream_wchar *this, wchar_t *str, MSVCP_size_t size, streamsize count)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %lu %ld)\n", this, str, size, count);

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        this->count = basic_streambuf_wchar__Sgetn_s(strbuf, str, size, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?read@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?read@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_read(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar__Read_s(this, str, count, count);
}

/* ?_Readsome_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHPA_WIH@Z */
/* ?_Readsome_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_K_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Readsome_s, 16)
streamsize __thiscall basic_istream_wchar__Readsome_s(basic_istream_wchar *this, wchar_t *str, MSVCP_size_t size, streamsize count)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %lu %ld)\n", this, str, size, count);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_wchar_in_avail(basic_ios_wchar_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_wchar__Read_s(this, str, size, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this->count;
}

/* ?readsome@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHPA_WH@Z */
/* ?readsome@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_readsome, 12)
streamsize __thiscall basic_istream_wchar_readsome(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar__Readsome_s(this, str, count, count);
}

/* ?putback@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@_W@Z */
/* ?putback@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_W@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_putback, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_putback(basic_istream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %c)\n", this, ch);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sputbackc(strbuf, ch)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?unget@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_unget, 4)
basic_istream_wchar* __thiscall basic_istream_wchar_unget(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sungetc(strbuf)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?sync@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_sync, 4)
int __thiscall basic_istream_wchar_sync(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

    TRACE("(%p)\n", this);

    if(!strbuf)
        return -1;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        if(basic_streambuf_wchar_pubsync(strbuf) != -1) {
            basic_istream_wchar_sentry_destroy(this);
            return 0;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, IOSTATE_badbit);
    return -1;
}

/* ?tellg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_tellg, 8)
fpos_int* __thiscall basic_istream_wchar_tellg(basic_istream_wchar *this, fpos_int *ret)
{
    TRACE("(%p %p)\n", this, ret);

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                    ret, 0, SEEKDIR_cur, OPENMODE_in);
            basic_istream_wchar_sentry_destroy(this);

            if(ret->off==0 && ret->pos==-1 && ret->state==0)
                basic_ios_wchar_setstate(base, IOSTATE_failbit);
            return ret;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    ret->off = 0;
    ret->pos = -1;
    ret->state = 0;
    return ret;
}

/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_seekg, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_seekg(basic_istream_wchar *this, streamoff off, int dir)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);

    TRACE("(%p %ld %d)\n", this, off, dir);

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
            fpos_int ret;

            basic_streambuf_wchar_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);
            basic_istream_wchar_sentry_destroy(this);

            if(ret.off==0 && ret.pos==-1 && ret.state==0)
                basic_ios_wchar_setstate(base, IOSTATE_failbit);
            else
                basic_ios_wchar_clear(base, IOSTATE_goodbit);
            return this;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, IOSTATE_failbit);
    return this;
}

/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_seekg_fpos, 28)
basic_istream_wchar* __thiscall basic_istream_wchar_seekg_fpos(basic_istream_wchar *this, fpos_int pos)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        if(!ios_base_fail(&base->base)) {
            basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
            fpos_int ret;

            basic_streambuf_wchar_pubseekpos(strbuf, &ret, pos, OPENMODE_in);
            basic_istream_wchar_sentry_destroy(this);

            if(ret.off==0 && ret.pos==-1 && ret.state==0)
                basic_ios_wchar_setstate(base, IOSTATE_failbit);
            else
                basic_ios_wchar_clear(base, IOSTATE_goodbit);
            return this;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, IOSTATE_failbit);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_short, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_short(basic_istream_wchar *this, short *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};
        LONG tmp;

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, &tmp);

        if(!(state&IOSTATE_failbit) && tmp==(LONG)((short)tmp))
            *v = tmp;
        else
            state |= IOSTATE_failbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAG@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ushort, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ushort(basic_istream_wchar *this, unsigned short *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ushort(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_int, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_int(basic_istream_wchar *this, int *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_uint, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_uint(basic_istream_wchar *this, unsigned int *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_long, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_long(basic_istream_wchar *this, LONG *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ulong, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ulong(basic_istream_wchar *this, ULONG *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ulong(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_float, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_float(basic_istream_wchar *this, float *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_float(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_double, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_double(basic_istream_wchar *this, double *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_double(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ldouble, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ldouble(basic_istream_wchar *this, double *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ldouble(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ptr, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ptr(basic_istream_wchar *this, void **v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_void(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_int64, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_int64(basic_istream_wchar *this, __int64 *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_int64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_uint64, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_uint64(basic_istream_wchar *this, unsigned __int64 *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_bool, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_bool(basic_istream_wchar *this, MSVCP_bool *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(strbuf->loc);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_bool(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_W@Z */
/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_W@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_getline_bstr_delim(
        basic_istream_wchar *istream, basic_string_wchar *str, wchar_t delim)
{
    IOSB_iostate state = IOSTATE_failbit;
    int c = delim;

    TRACE("(%p %p %c)\n", istream, str, delim);

    if(basic_istream_wchar_sentry_create(istream, TRUE)) {
        MSVCP_basic_string_wchar_clear(str);

        for(c = basic_istream_wchar_get(istream); c!=delim && c!=WEOF;
                c = basic_istream_wchar_get(istream)) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_wchar_append_ch(str, c);
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(basic_istream_wchar_get_basic_ios(istream),
            state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_getline_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    return basic_istream_wchar_getline_bstr_delim(istream, str, '\n');
}

/* ??$?5_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?5_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        const ctype_wchar *ctype = ctype_wchar_use_facet(base->strbuf->loc);
        MSVCP_size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_wchar_clear(str);

        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_wchar_append_ch(str, c);
        }

        ios_base_width_set(&base->base, 0);
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@PA_W@Z */
/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@PEA_W@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_str(basic_istream_wchar *istream, wchar_t *str)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        const ctype_wchar *ctype = ctype_wchar_use_facet(base->strbuf->loc);
        MSVCP_size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }

        *str = 0;
        ios_base_width_set(&base->base, 0);
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AA_W@Z */
/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEA_W@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_ch(basic_istream_wchar *istream, wchar_t *ch)
{
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = 0;

    TRACE("(%p %p)\n", istream, ch);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        c = basic_streambuf_wchar_sbumpc(basic_ios_wchar_rdbuf_get(
                    basic_istream_wchar_get_basic_ios(istream)));
        if(c != WEOF) {
            state = IOSTATE_goodbit;
            *ch = c;
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(basic_istream_wchar_get_basic_ios(istream),
            state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_streambuf(
        basic_istream_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, streambuf);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base)); c!=WEOF;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base))) {
            state = IOSTATE_goodbit;
            if(basic_streambuf_wchar_sputc(streambuf, c) == WEOF)
                break;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func(basic_istream_wchar *this,
        basic_istream_wchar* (__cdecl *pfunc)(basic_istream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func_basic_ios, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func_basic_ios(basic_istream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_istream_wchar_get_basic_ios(this));
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func_ios_base, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func_ios_base(
        basic_istream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_istream_wchar_get_basic_ios(this)->base);
    return this;
}

static inline basic_ios_char* basic_iostream_char_to_basic_ios(basic_iostream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_iostream_char_vbtable1[1]);
}

static inline basic_iostream_char* basic_iostream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_iostream_char*)((char*)ptr-basic_iostream_char_vbtable1[1]);
}

/* ??0?$basic_iostream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??0?$basic_iostream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_ctor, 12)
basic_iostream_char* __thiscall basic_iostream_char_ctor(basic_iostream_char *this, basic_streambuf_char *strbuf, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, strbuf, virt_init);

    if(virt_init) {
        this->base1.vbtable = basic_iostream_char_vbtable1;
        this->base2.vbtable = basic_iostream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base1);
    }

    basic_ios->base.vtable = &MSVCP_basic_iostream_char_vtable;

    basic_istream_char_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_char_ctor_uninitialized(&this->base2, 0, FALSE, FALSE);
    return this;
}

/* ??1?$basic_iostream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_dtor, 4)
void __thiscall basic_iostream_char_dtor(basic_ios_char *base)
{
    basic_iostream_char *this = basic_iostream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);
    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base2));
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base1));
}

/* ??_D?$basic_iostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_vbase_dtor, 4)
void __thiscall basic_iostream_char_vbase_dtor(basic_iostream_char *this)
{
    TRACE("(%p)\n", this);
    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base1));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_iostream_char_vector_dtor, 8)
basic_iostream_char* __thiscall MSVCP_basic_iostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_iostream_char *this = basic_iostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_iostream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

static inline basic_ios_wchar* basic_iostream_wchar_to_basic_ios(basic_iostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_iostream_wchar_vbtable1[1]);
}

static inline basic_iostream_wchar* basic_iostream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_iostream_wchar*)((char*)ptr-basic_iostream_wchar_vbtable1[1]);
}

/* ??0?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??0?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_ctor, 12)
basic_iostream_wchar* __thiscall basic_iostream_wchar_ctor(basic_iostream_wchar *this, basic_streambuf_wchar *strbuf, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, strbuf, virt_init);

    if(virt_init) {
        this->base1.vbtable = basic_iostream_wchar_vbtable1;
        this->base2.vbtable = basic_iostream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base1);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base1);
    }

    basic_ios->base.vtable = &MSVCP_basic_iostream_wchar_vtable;

    basic_istream_wchar_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_wchar_ctor_uninitialized(&this->base2, 0, FALSE, FALSE);
    return this;
}

/* ??1?$basic_iostream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_dtor, 4)
void __thiscall basic_iostream_wchar_dtor(basic_ios_wchar *base)
{
    basic_iostream_wchar *this = basic_iostream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);
    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(&this->base2));
    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(&this->base1));
}

/* ??_D?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_vbase_dtor, 4)
void __thiscall basic_iostream_wchar_vbase_dtor(basic_iostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_iostream_wchar_dtor(basic_iostream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_istream_wchar_get_basic_ios(&this->base1));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_iostream_wchar_vector_dtor, 8)
basic_iostream_wchar* __thiscall MSVCP_basic_iostream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_iostream_wchar *this = basic_iostream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_iostream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

static inline basic_ios_char* basic_ofstream_char_to_basic_ios(basic_ofstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ofstream_char_vbtable[1]);
}

static inline basic_ofstream_char* basic_ofstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ofstream_char*)((char*)ptr-basic_ofstream_char_vbtable[1]);
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor, 8)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor(basic_ofstream_char *this, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_file, 12)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_file(
        basic_ofstream_char *this, FILE *file, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_name, 20)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_name(basic_ofstream_char *this,
        const char *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ofstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_name_wchar, 20)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_name_wchar(basic_ofstream_char *this,
        const wchar_t *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ofstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ofstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ofstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_dtor, 4)
void __thiscall basic_ofstream_char_dtor(basic_ios_char *base)
{
    basic_ofstream_char *this = basic_ofstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_vbase_dtor, 4)
void __thiscall basic_ofstream_char_vbase_dtor(basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_ofstream_char_dtor(basic_ofstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_ostream_char_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ofstream_char_vector_dtor, 8)
basic_ofstream_char* __thiscall MSVCP_basic_ofstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ofstream_char *this = basic_ofstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ofstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ofstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_close, 4)
void __thiscall basic_ofstream_char_close(basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_is_open, 4)
MSVCP_bool __thiscall basic_ofstream_char_is_open(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open, 16)
void __thiscall basic_ofstream_char_open(basic_ofstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_old, 12)
void __thiscall basic_ofstream_char_open_old(basic_ofstream_char *this,
        const char *name, unsigned int mode)
{
    basic_ofstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_wchar, 16)
void __thiscall basic_ofstream_char_open_wchar(basic_ofstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_wchar_old, 12)
void __thiscall basic_ofstream_char_open_wchar_old(basic_ofstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ofstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ofstream_char_rdbuf(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_char* basic_ifstream_char_to_basic_ios(basic_ifstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ifstream_char_vbtable[1]);
}

static inline basic_ifstream_char* basic_ifstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ifstream_char*)((char*)ptr-basic_ifstream_char_vbtable[1]);
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor, 8)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor(basic_ifstream_char *this, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_istream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_file, 12)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_file(
        basic_ifstream_char *this, FILE *file, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_istream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name, 20)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name(basic_ifstream_char *this,
        const char *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ifstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name_wchar, 20)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name_wchar(basic_ifstream_char *this,
        const wchar_t *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ifstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ifstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ifstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_dtor, 4)
void __thiscall basic_ifstream_char_dtor(basic_ios_char *base)
{
    basic_ifstream_char *this = basic_ifstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_vbase_dtor, 4)
void __thiscall basic_ifstream_char_vbase_dtor(basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_ifstream_char_dtor(basic_ifstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ifstream_char_vector_dtor, 8)
basic_ifstream_char* __thiscall MSVCP_basic_ifstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ifstream_char *this = basic_ifstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ifstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ifstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_close, 4)
void __thiscall basic_ifstream_char_close(basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_is_open, 4)
MSVCP_bool __thiscall basic_ifstream_char_is_open(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open, 16)
void __thiscall basic_ifstream_char_open(basic_ifstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_old, 12)
void __thiscall basic_ifstream_char_open_old(basic_ifstream_char *this,
        const char *name, unsigned int mode)
{
    basic_ifstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_wchar, 16)
void __thiscall basic_ifstream_char_open_wchar(basic_ifstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_wchar_old, 12)
void __thiscall basic_ifstream_char_open_wchar_old(basic_ifstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ifstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ifstream_char_rdbuf(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_char* basic_fstream_char_to_basic_ios(basic_fstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_fstream_char_vbtable1[1]);
}

static inline basic_fstream_char* basic_fstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_fstream_char*)((char*)ptr-basic_fstream_char_vbtable1[1]);
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor, 8)
basic_fstream_char* __thiscall basic_fstream_char_ctor(basic_fstream_char *this, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_char_vbtable1;
        this->base.base2.vbtable = basic_fstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_iostream_char_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_file, 12)
basic_fstream_char* __thiscall basic_fstream_char_ctor_file(basic_fstream_char *this,
        FILE *file, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_char_vbtable1;
        this->base.base2.vbtable = basic_fstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_iostream_char_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name, 20)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name(basic_fstream_char *this,
        const char *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_fstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name_wchar, 20)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name_wchar(basic_fstream_char *this,
        const wchar_t *name, int mode, int prot, MSVCP_bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_fstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_fstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_fstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_dtor, 4)
void __thiscall basic_fstream_char_dtor(basic_ios_char *base)
{
    basic_fstream_char *this = basic_fstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_vbase_dtor, 4)
void __thiscall basic_fstream_char_vbase_dtor(basic_fstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_fstream_char_dtor(basic_fstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_fstream_char_vector_dtor, 8)
basic_fstream_char* __thiscall MSVCP_basic_fstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_fstream_char *this = basic_fstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_fstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_fstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_close, 4)
void __thiscall basic_fstream_char_close(basic_fstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_is_open, 4)
MSVCP_bool __thiscall basic_fstream_char_is_open(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open, 16)
void __thiscall basic_fstream_char_open(basic_fstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_old, 12)
void __thiscall basic_fstream_char_open_old(basic_fstream_char *this,
        const char *name, unsigned int mode)
{
    basic_fstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_wchar, 16)
void __thiscall basic_fstream_char_open_wchar(basic_fstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_wchar_old, 12)
void __thiscall basic_fstream_char_open_wchar_old(basic_fstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_fstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_fstream_char_rdbuf(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_char* basic_ostringstream_char_to_basic_ios(basic_ostringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ostringstream_char_vbtable[1]);
}

static inline basic_ostringstream_char* basic_ostringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ostringstream_char*)((char*)ptr-basic_ostringstream_char_vbtable[1]);
}

/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor_str, 16)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor_str(basic_ostringstream_char *this,
        const basic_string_char *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode|OPENMODE_out);
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ostringstream_char_vtable;
    return this;
}

/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor_mode, 12)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor_mode(
        basic_ostringstream_char *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode|OPENMODE_out);
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ostringstream_char_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor, 8)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor(
        basic_ostringstream_char *this, MSVCP_bool virt_init)
{
    return basic_ostringstream_char_ctor_mode(this, 0, virt_init);
}

/* ??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_dtor, 4)
void __thiscall basic_ostringstream_char_dtor(basic_ios_char *base)
{
    basic_ostringstream_char *this = basic_ostringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_char_dtor(&this->strbuf);
    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
}

/* ??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_vbase_dtor, 4)
void __thiscall basic_ostringstream_char_vbase_dtor(basic_ostringstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_ostringstream_char_dtor(basic_ostringstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_ostream_char_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ostringstream_char_vector_dtor, 8)
basic_ostringstream_char* __thiscall MSVCP_basic_ostringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostringstream_char *this = basic_ostringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ostringstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_ostringstream_char_rdbuf(const basic_ostringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_str_set, 8)
void __thiscall basic_ostringstream_char_str_set(basic_ostringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_str_get, 8)
basic_string_char* __thiscall basic_ostringstream_char_str_get(const basic_ostringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_ostringstream_wchar_to_basic_ios(basic_ostringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostringstream_wchar_vbtable[1]);
}

static inline basic_ostringstream_wchar* basic_ostringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostringstream_wchar*)((char*)ptr-basic_ostringstream_wchar_vbtable[1]);
}

/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor_str, 16)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor_str(basic_ostringstream_wchar *this,
        const basic_string_wchar *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode|OPENMODE_out);
    basic_ostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ostringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor_mode, 12)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor_mode(
        basic_ostringstream_wchar *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode|OPENMODE_out);
    basic_ostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_ostringstream_wchar_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor, 8)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor(
        basic_ostringstream_wchar *this, MSVCP_bool virt_init)
{
    return basic_ostringstream_wchar_ctor_mode(this, 0, virt_init);
}

/* ??1?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_dtor, 4)
void __thiscall basic_ostringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ostringstream_wchar *this = basic_ostringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_wchar_dtor(&this->strbuf);
    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(&this->base));
}

/* ??_D?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_vbase_dtor, 4)
void __thiscall basic_ostringstream_wchar_vbase_dtor(basic_ostringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_ostringstream_wchar_dtor(basic_ostringstream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_ostream_wchar_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ostringstream_wchar_vector_dtor, 8)
basic_ostringstream_wchar* __thiscall MSVCP_basic_ostringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostringstream_wchar *this = basic_ostringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_ostringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_ostringstream_wchar_rdbuf(const basic_ostringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_str_set, 8)
void __thiscall basic_ostringstream_wchar_str_set(basic_ostringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_ostringstream_wchar_str_get(const basic_ostringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

static inline basic_ios_char* basic_istringstream_char_to_basic_ios(basic_istringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_istringstream_char_vbtable[1]);
}

static inline basic_istringstream_char* basic_istringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_istringstream_char*)((char*)ptr-basic_istringstream_char_vbtable[1]);
}

/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor_str, 16)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor_str(basic_istringstream_char *this,
        const basic_string_char *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode|OPENMODE_in);
    basic_istream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_istringstream_char_vtable;
    return this;
}

/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor_mode, 12)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor_mode(
        basic_istringstream_char *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode|OPENMODE_in);
    basic_istream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_istringstream_char_vtable;
    return this;
}

/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor, 8)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor(
        basic_istringstream_char *this, MSVCP_bool virt_init)
{
    return basic_istringstream_char_ctor_mode(this, 0, virt_init);
}

/* ??1?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_dtor, 4)
void __thiscall basic_istringstream_char_dtor(basic_ios_char *base)
{
    basic_istringstream_char *this = basic_istringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_char_dtor(&this->strbuf);
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
}

/* ??_D?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_vbase_dtor, 4)
void __thiscall basic_istringstream_char_vbase_dtor(basic_istringstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_istringstream_char_dtor(basic_istringstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_istringstream_char_vector_dtor, 8)
basic_istringstream_char* __thiscall MSVCP_basic_istringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istringstream_char *this = basic_istringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_istringstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_istringstream_char_rdbuf(const basic_istringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_str_set, 8)
void __thiscall basic_istringstream_char_str_set(basic_istringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_str_get, 8)
basic_string_char* __thiscall basic_istringstream_char_str_get(const basic_istringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_istringstream_wchar_to_basic_ios(basic_istringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istringstream_wchar_vbtable[1]);
}

static inline basic_istringstream_wchar* basic_istringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istringstream_wchar*)((char*)ptr-basic_istringstream_wchar_vbtable[1]);
}

/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor_str, 16)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor_str(basic_istringstream_wchar *this,
        const basic_string_wchar *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode|OPENMODE_in);
    basic_istream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_istringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor_mode, 12)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor_mode(
        basic_istringstream_wchar *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode|OPENMODE_in);
    basic_istream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_istringstream_wchar_vtable;
    return this;
}

/* ??_F?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor, 8)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor(
        basic_istringstream_wchar *this, MSVCP_bool virt_init)
{
    return basic_istringstream_wchar_ctor_mode(this, 0, virt_init);
}

/* ??1?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_dtor, 4)
void __thiscall basic_istringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_istringstream_wchar *this = basic_istringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_wchar_dtor(&this->strbuf);
    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(&this->base));
}

/* ??_D?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_vbase_dtor, 4)
void __thiscall basic_istringstream_wchar_vbase_dtor(basic_istringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_istringstream_wchar_dtor(basic_istringstream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_istream_wchar_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_istringstream_wchar_vector_dtor, 8)
basic_istringstream_wchar* __thiscall MSVCP_basic_istringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istringstream_wchar *this = basic_istringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_istringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_istringstream_wchar_rdbuf(const basic_istringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_str_set, 8)
void __thiscall basic_istringstream_wchar_str_set(basic_istringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_istringstream_wchar_str_get(const basic_istringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

static inline basic_ios_char* basic_stringstream_char_to_basic_ios(basic_stringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_stringstream_char_vbtable1[1]);
}

static inline basic_stringstream_char* basic_stringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_stringstream_char*)((char*)ptr-basic_stringstream_char_vbtable1[1]);
}

/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor_str, 16)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor_str(basic_stringstream_char *this,
        const basic_string_char *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_char_vbtable1;
        this->base.base2.vbtable = basic_stringstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode);
    basic_iostream_char_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_stringstream_char_vtable;
    return this;
}

/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor_mode, 12)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor_mode(
        basic_stringstream_char *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_char_vbtable1;
        this->base.base2.vbtable = basic_stringstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode);
    basic_iostream_char_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_stringstream_char_vtable;
    return this;
}

/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor, 8)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor(
        basic_stringstream_char *this, MSVCP_bool virt_init)
{
    return basic_stringstream_char_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, virt_init);
}

/* ??1?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_dtor, 4)
void __thiscall basic_stringstream_char_dtor(basic_ios_char *base)
{
    basic_stringstream_char *this = basic_stringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    basic_stringbuf_char_dtor(&this->strbuf);
}

/* ??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_vbase_dtor, 4)
void __thiscall basic_stringstream_char_vbase_dtor(basic_stringstream_char *this)
{
    TRACE("(%p)\n", this);

    basic_stringstream_char_dtor(basic_stringstream_char_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_stringstream_char_vector_dtor, 8)
basic_stringstream_char* __thiscall MSVCP_basic_stringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_stringstream_char *this = basic_stringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_char_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_stringstream_char_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_stringstream_char_rdbuf(const basic_stringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_str_set, 8)
void __thiscall basic_stringstream_char_str_set(basic_stringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_str_get, 8)
basic_string_char* __thiscall basic_stringstream_char_str_get(const basic_stringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_stringstream_wchar_to_basic_ios(basic_stringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_stringstream_wchar_vbtable1[1]);
}

static inline basic_stringstream_wchar* basic_stringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_stringstream_wchar*)((char*)ptr-basic_stringstream_wchar_vbtable1[1]);
}

/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor_str, 16)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor_str(basic_stringstream_wchar *this,
        const basic_string_wchar *str, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_stringstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode);
    basic_iostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_stringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor_mode, 12)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor_mode(
        basic_stringstream_wchar *this, int mode, MSVCP_bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_stringstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode);
    basic_iostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &MSVCP_basic_stringstream_wchar_vtable;
    return this;
}

/* ??_F?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor, 8)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor(
        basic_stringstream_wchar *this, MSVCP_bool virt_init)
{
    return basic_stringstream_wchar_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, virt_init);
}

/* ??1?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_dtor, 4)
void __thiscall basic_stringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_stringstream_wchar *this = basic_stringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_wchar_dtor(basic_iostream_wchar_to_basic_ios(&this->base));
    basic_stringbuf_wchar_dtor(&this->strbuf);
}

/* ??_D?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_vbase_dtor, 4)
void __thiscall basic_stringstream_wchar_vbase_dtor(basic_stringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_stringstream_wchar_dtor(basic_stringstream_wchar_to_basic_ios(this));
    basic_ios_wchar_dtor(basic_istream_wchar_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_stringstream_wchar_vector_dtor, 8)
basic_stringstream_wchar* __thiscall MSVCP_basic_stringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_stringstream_wchar *this = basic_stringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_wchar_vbase_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        basic_stringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_stringstream_wchar_rdbuf(const basic_stringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_str_set, 8)
void __thiscall basic_stringstream_wchar_str_set(basic_stringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_stringstream_wchar_str_get(const basic_stringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

static void __cdecl setprecision_func(ios_base *base, streamsize prec)
{
    ios_base_precision_set(base, prec);
}

/* ?setprecision@std@@YA?AU?$_Smanip@H@1@H@Z */
/* ?setprecision@std@@YA?AU?$_Smanip@_J@1@_J@Z */
manip_streamsize* __cdecl setprecision(manip_streamsize *ret, streamsize prec)
{
    TRACE("(%p %ld)\n", ret, prec);

    ret->pfunc = setprecision_func;
    ret->arg = prec;
    return ret;
}

static void __cdecl setw_func(ios_base *base, streamsize width)
{
    ios_base_width_set(base, width);
}

/* ?setw@std@@YA?AU?$_Smanip@H@1@H@Z */
/* ?setw@std@@YA?AU?$_Smanip@_J@1@_J@Z */
manip_streamsize* __cdecl setw(manip_streamsize *ret, streamsize width)
{
    TRACE("(%p %ld)\n", ret, width);

    ret->pfunc = setw_func;
    ret->arg = width;
    return ret;
}

static basic_filebuf_char filebuf_stdin;
/* ?cin@std@@3V?$basic_istream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_istream_char obj;
    basic_ios_char vbase;
} cin = { { 0 } };
/* ?_Ptr_cin@std@@3PAV?$basic_istream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cin@std@@3PEAV?$basic_istream@DU?$char_traits@D@std@@@1@EA */
basic_istream_char *_Ptr_cin = &cin.obj;

static basic_filebuf_char filebuf_stdout;
/* ?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} cout = { { 0 } };
/* ?_Ptr_cout@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cout@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_cout = &cout.obj;

static basic_filebuf_char filebuf_stderr;
/* ?cerr@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} cerr = { { 0 } };
/* ?_Ptr_cerr@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cerr@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_cerr = &cerr.obj;

static basic_filebuf_char filebuf_log;
/* ?clog@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} clog = { { 0 } };
/* ?_Ptr_clog@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_clog@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_clog = &clog.obj;

void init_io(void)
{
    basic_filebuf_char_ctor_file(&filebuf_stdin, stdin);
    basic_istream_char_ctor(&cin.obj, &filebuf_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_stdout, stdout);
    basic_ostream_char_ctor(&cout.obj, &filebuf_stdout.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_stderr, stderr);
    basic_ostream_char_ctor(&cerr.obj, &filebuf_stderr.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_log, stderr);
    basic_ostream_char_ctor(&clog.obj, &filebuf_log.base, FALSE/*FIXME*/, TRUE);
}

void free_io(void)
{
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&cin.obj));
    basic_filebuf_char_dtor(&filebuf_stdin);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&cout.obj));
    basic_filebuf_char_dtor(&filebuf_stdout);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&cerr.obj));
    basic_filebuf_char_dtor(&filebuf_stderr);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&clog.obj));
    basic_filebuf_char_dtor(&filebuf_log);
}
