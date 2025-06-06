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


#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <share.h>
#include <locale.h>

#include "msvcp.h"
#include "windef.h"
#include "winbase.h"
#include "windows.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

/* ?_Index@ios_base@std@@0HA */
int ios_base_Index = 0;
/* ?_Sync@ios_base@std@@0_NA */
bool ios_base_Sync = FALSE;

typedef struct {
    streamoff off;
    INT64 pos;
    int state;
} fpos_int;

static inline const char* debugstr_fpos_int(fpos_int *fpos)
{
    return wine_dbg_sprintf("fpos(%Id %I64d %d)", fpos->off, fpos->pos, fpos->state);
}

typedef struct {
    void (__cdecl *pfunc)(ios_base*, streamsize);
    streamsize arg;
} manip_streamsize;

typedef struct {
    void (__cdecl *pfunc)(ios_base*, int);
    int arg;
} manip_int;

typedef enum {
    INITFL_new   = 0,
    INITFL_open  = 1,
    INITFL_close = 2
} basic_filebuf__Initfl;

typedef struct {
    basic_streambuf_char base;
    codecvt_char *cvt;
    int state0;
    int state;
    basic_string_char *str;
    bool close;
    locale loc;
    FILE *file;
} basic_filebuf_char;

typedef struct {
    basic_streambuf_wchar base;
    codecvt_wchar *cvt;
    int state0;
    int state;
    basic_string_char *str;
    bool close;
    locale loc;
    FILE *file;
} basic_filebuf_wchar;

typedef enum {
    STRINGBUF_allocated = 1,
    STRINGBUF_no_write = 2,
    STRINGBUF_no_read = 4,
    STRINGBUF_append = 8,
    STRINGBUF_at_end = 16
} basic_stringbuf_state;

typedef struct {
    basic_streambuf_char base;
    char *pendsave;
    char *seekhigh;
    int alsize;
    int state;
    char allocator; /* empty struct */
} basic_stringbuf_char;

typedef struct {
    basic_streambuf_wchar base;
    wchar_t *pendsave;
    wchar_t *seekhigh;
    int alsize;
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
    basic_ostream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ofstream_wchar;

typedef struct {
    basic_istream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ifstream_char;

typedef struct {
    basic_istream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ifstream_wchar;

typedef struct {
    basic_iostream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_fstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_fstream_wchar;

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

typedef enum {
    STRSTATE_Allocated = 1,
    STRSTATE_Constant = 2,
    STRSTATE_Dynamic = 4,
    STRSTATE_Frozen = 8
} strstreambuf__Strstate;

typedef struct {
    basic_streambuf_char base;
    char *endsave;
    char *seekhigh;
    streamsize minsize;
    int strmode;
    void* (__cdecl __WINE_ALLOC_SIZE(1) *palloc)(size_t);
    void (__cdecl *pfree)(void*);
} strstreambuf;

typedef struct {
    basic_ostream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} ostrstream;

typedef struct {
    basic_istream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} istrstream;

typedef struct {
    basic_iostream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} strstream;

#define VBTABLE_ALIGN 4

extern const vtable_ptr iosb_vtable;

/* ??_7ios_base@std@@6B@ */
extern const vtable_ptr ios_base_vtable;

/* ??_7?$basic_ios@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ios_char_vtable;

/* ??_7?$basic_ios@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ios_short_vtable;

/* ??_7?$basic_streambuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_streambuf_char_vtable;

/* ??_7?$basic_streambuf@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_streambuf_short_vtable;

/* ??_7?$basic_filebuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_filebuf_char_vtable;

/* ??_7?$basic_filebuf@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_filebuf_short_vtable;

/* ??_7?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_stringbuf_char_vtable;

/* ??_7?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_stringbuf_short_vtable;

/* ??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ostream_char_vbtable[] = {0, ALIGNED_SIZE(sizeof(basic_ostream_char), VBTABLE_ALIGN)};
/* ??_7?$basic_ostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ostream_char_vtable;

/* ??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ostream_short_vbtable[] = {0, ALIGNED_SIZE(sizeof(basic_ostream_wchar), VBTABLE_ALIGN)};
/* ??_7?$basic_ostream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ostream_short_vtable;

/* ??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_istream_char_vbtable[] = {0, ALIGNED_SIZE(sizeof(basic_istream_char), VBTABLE_ALIGN)};
/* ??_7?$basic_istream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_istream_char_vtable;

/* ??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_istream_short_vbtable[] = {0, ALIGNED_SIZE(sizeof(basic_istream_wchar), VBTABLE_ALIGN)};
/* ??_7?$basic_istream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_istream_short_vtable;

/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable1[] = {0, ALIGNED_SIZE(sizeof(basic_iostream_char), VBTABLE_ALIGN)};
/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable2[] = {0, ALIGNED_SIZE(sizeof(basic_iostream_char), VBTABLE_ALIGN)-FIELD_OFFSET(basic_iostream_char, base2)};
/* ??_7?$basic_iostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_iostream_char_vtable;

/* ??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_iostream_short_vbtable1[] = {0, ALIGNED_SIZE(sizeof(basic_iostream_wchar), VBTABLE_ALIGN)};
/* ??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_iostream_short_vbtable2[] = {0, ALIGNED_SIZE(sizeof(basic_iostream_wchar), VBTABLE_ALIGN)-FIELD_OFFSET(basic_iostream_wchar, base2)};
/* ??_7?$basic_iostream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_iostream_short_vtable;

/* ??_8?$basic_ofstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ofstream_char_vbtable[] = {0, sizeof(basic_ofstream_char)};
/* ??_7?$basic_ofstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ofstream_char_vtable;

/* ??_8?$basic_ofstream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ofstream_short_vbtable[] = {0, sizeof(basic_ofstream_wchar)};
/* ??_7?$basic_ofstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ofstream_short_vtable;

/* ??_8?$basic_ifstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ifstream_char_vbtable[] = {0, sizeof(basic_ifstream_char)};
/* ??_7?$basic_ifstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ifstream_char_vtable;

/* ??_8?$basic_ifstream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ifstream_short_vbtable[] = {0, sizeof(basic_ifstream_wchar)};
/* ??_7?$basic_ifstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ifstream_short_vtable;

/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable1[] = {0, sizeof(basic_fstream_char)};
/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable2[] = {0, sizeof(basic_fstream_char)-FIELD_OFFSET(basic_fstream_char, base.base2)};
/* ??_7?$basic_fstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_fstream_char_vtable;

/* ??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_fstream_short_vbtable1[] = {0, sizeof(basic_fstream_wchar)};
/* ??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_fstream_short_vbtable2[] = {0, sizeof(basic_fstream_wchar)-FIELD_OFFSET(basic_fstream_wchar, base.base2)};
/* ??_7?$basic_fstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_fstream_short_vtable;

/* ??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_ostringstream_char_vbtable[] = {0, sizeof(basic_ostringstream_char)};
/* ??_7?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_ostringstream_char_vtable;

/* ??_8?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@ */
const int basic_ostringstream_short_vbtable[] = {0, sizeof(basic_ostringstream_wchar)};
/* ??_7?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_ostringstream_short_vtable;

/* ??_8?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_istringstream_char_vbtable[] = {0, sizeof(basic_istringstream_char)};
/* ??_7?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_istringstream_char_vtable;

/* ??_8?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@ */
const int basic_istringstream_short_vbtable[] = {0, sizeof(basic_istringstream_wchar)};
/* ??_7?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_istringstream_short_vtable;

/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable1[] = {0, sizeof(basic_stringstream_char)};
/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable2[] = {0, sizeof(basic_stringstream_char)-FIELD_OFFSET(basic_stringstream_char, base.base2)};
/* ??_7?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_stringstream_char_vtable;

/* ??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_stringstream_short_vbtable1[] = {0, sizeof(basic_stringstream_wchar)};
/* ??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_stringstream_short_vbtable2[] = {0, sizeof(basic_stringstream_wchar)-FIELD_OFFSET(basic_stringstream_wchar, base.base2)};
/* ??_7?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_stringstream_short_vtable;

/* ??_7strstreambuf@std@@6B */
extern const vtable_ptr strstreambuf_vtable;

static const int ostrstream_vbtable[] = {0, sizeof(ostrstream)};
extern const vtable_ptr ostrstream_vtable;

static const int istrstream_vbtable[] = {0, sizeof(istrstream)};

static const int strstream_vbtable1[] = {0, sizeof(strstream)};
static const int strstream_vbtable2[] = {0, sizeof(strstream)-FIELD_OFFSET(strstream, base.base2)};
extern const vtable_ptr strstream_vtable;

DEFINE_RTTI_DATA0(iosb, 0, ".?AV?$_Iosb@H@std@@")
DEFINE_RTTI_DATA1(ios_base, 0, &iosb_rtti_base_descriptor, ".?AV?$_Iosb@H@std@@")
DEFINE_RTTI_DATA2(basic_ios_char, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA2(basic_ios_short, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA0(basic_streambuf_char, 0,
        ".?AV?$basic_streambuf@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA0(basic_streambuf_short, 0,
        ".?AV?$basic_streambuf@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA1(basic_filebuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_filebuf@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA1(basic_filebuf_short, 0, &basic_streambuf_short_rtti_base_descriptor,
        ".?AV?$basic_filebuf@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA1(basic_stringbuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA1(basic_stringbuf_short, 0, &basic_streambuf_short_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA3(basic_ostream_char, sizeof(basic_ostream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA3(basic_ostream_short, sizeof(basic_ostream_wchar), &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA3(basic_istream_char, sizeof(basic_istream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA3(basic_istream_short, sizeof(basic_istream_wchar), &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA8(basic_iostream_char, sizeof(basic_iostream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA8(basic_iostream_short, sizeof(basic_iostream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ofstream_char, sizeof(basic_ofstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ofstream_short, sizeof(basic_ofstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ifstream_char, sizeof(basic_ifstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ifstream_short, sizeof(basic_ifstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA8(basic_fstream_char, sizeof(basic_fstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA8(basic_fstream_short, sizeof(basic_fstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ostringstream_char, sizeof(basic_ostringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA4(basic_ostringstream_short, sizeof(basic_ostringstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA4(basic_istringstream_char, sizeof(basic_istringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA4(basic_istringstream_short, sizeof(basic_istringstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA8(basic_stringstream_char, sizeof(basic_stringstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA8(basic_stringstream_short, sizeof(basic_stringstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA1(strstreambuf, sizeof(strstreambuf),
        &basic_streambuf_char_rtti_base_descriptor, ".?AVstrstreambuf@std@@")
DEFINE_RTTI_DATA4(ostrstream, sizeof(ostrstream),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        "?AVostrstream@std@@")
DEFINE_RTTI_DATA8(strstream, sizeof(strstream),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        "?AVstrstream@std@@")

__ASM_BLOCK_BEGIN(ios_vtables)
    __ASM_VTABLE(iosb,
            VTABLE_ADD_FUNC(iosb_vector_dtor));
    __ASM_VTABLE(ios_base,
            VTABLE_ADD_FUNC(ios_base_vector_dtor));
    __ASM_VTABLE(basic_ios_char,
            VTABLE_ADD_FUNC(basic_ios_char_vector_dtor));
    __ASM_VTABLE(basic_ios_short,
            VTABLE_ADD_FUNC(basic_ios_short_vector_dtor));
    __ASM_VTABLE(basic_streambuf_char,
            VTABLE_ADD_FUNC(basic_streambuf_char_vector_dtor)
            VTABLE_ADD_FUNC(basic_streambuf_char_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_streambuf_short,
            VTABLE_ADD_FUNC(basic_streambuf_wchar_vector_dtor)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_filebuf_char,
            VTABLE_ADD_FUNC(basic_filebuf_char_vector_dtor)
            VTABLE_ADD_FUNC(basic_filebuf_char_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_char_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_filebuf_short,
            VTABLE_ADD_FUNC(basic_filebuf_short_vector_dtor)
            VTABLE_ADD_FUNC(basic_filebuf_short_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_short_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_short_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_short_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_short_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_short_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_short_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_short_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_stringbuf_char,
            VTABLE_ADD_FUNC(basic_stringbuf_char_vector_dtor)
            VTABLE_ADD_FUNC(basic_stringbuf_char_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_stringbuf_short,
            VTABLE_ADD_FUNC(basic_stringbuf_short_vector_dtor)
            VTABLE_ADD_FUNC(basic_stringbuf_short_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_short_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_short_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_short_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_short_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_ostream_char,
            VTABLE_ADD_FUNC(basic_ostream_char_vector_dtor));
    __ASM_VTABLE(basic_ostream_short,
            VTABLE_ADD_FUNC(basic_ostream_short_vector_dtor));
    __ASM_VTABLE(basic_istream_char,
            VTABLE_ADD_FUNC(basic_istream_char_vector_dtor));
    __ASM_VTABLE(basic_istream_short,
            VTABLE_ADD_FUNC(basic_istream_short_vector_dtor));
    __ASM_VTABLE(basic_iostream_char,
            VTABLE_ADD_FUNC(basic_iostream_char_vector_dtor));
    __ASM_VTABLE(basic_iostream_short,
            VTABLE_ADD_FUNC(basic_iostream_short_vector_dtor));
    __ASM_VTABLE(basic_ofstream_char,
            VTABLE_ADD_FUNC(basic_ofstream_char_vector_dtor));
    __ASM_VTABLE(basic_ofstream_short,
            VTABLE_ADD_FUNC(basic_ofstream_short_vector_dtor));
    __ASM_VTABLE(basic_ifstream_char,
            VTABLE_ADD_FUNC(basic_ifstream_char_vector_dtor));
    __ASM_VTABLE(basic_ifstream_short,
            VTABLE_ADD_FUNC(basic_ifstream_short_vector_dtor));
    __ASM_VTABLE(basic_fstream_char,
            VTABLE_ADD_FUNC(basic_fstream_char_vector_dtor));
    __ASM_VTABLE(basic_fstream_short,
            VTABLE_ADD_FUNC(basic_fstream_short_vector_dtor));
    __ASM_VTABLE(basic_ostringstream_char,
            VTABLE_ADD_FUNC(basic_ostringstream_char_vector_dtor));
    __ASM_VTABLE(basic_ostringstream_short,
            VTABLE_ADD_FUNC(basic_ostringstream_short_vector_dtor));
    __ASM_VTABLE(basic_istringstream_char,
            VTABLE_ADD_FUNC(basic_istringstream_char_vector_dtor));
    __ASM_VTABLE(basic_istringstream_short,
            VTABLE_ADD_FUNC(basic_istringstream_short_vector_dtor));
    __ASM_VTABLE(basic_stringstream_char,
            VTABLE_ADD_FUNC(basic_stringstream_char_vector_dtor));
    __ASM_VTABLE(basic_stringstream_short,
            VTABLE_ADD_FUNC(basic_stringstream_short_vector_dtor));
    __ASM_VTABLE(strstreambuf,
            VTABLE_ADD_FUNC(strstreambuf_vector_dtor)
            VTABLE_ADD_FUNC(strstreambuf_overflow)
            VTABLE_ADD_FUNC(strstreambuf_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(strstreambuf_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(strstreambuf_seekoff)
            VTABLE_ADD_FUNC(strstreambuf_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(ostrstream,
            VTABLE_ADD_FUNC(ostrstream_vector_dtor));
    __ASM_VTABLE(strstream,
            VTABLE_ADD_FUNC(strstream_vector_dtor));
__ASM_BLOCK_END

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
    this->vtable = &basic_streambuf_char_vtable;
    return this;
}

/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_ctor, 4)
basic_streambuf_char* __thiscall basic_streambuf_char_ctor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &basic_streambuf_char_vtable;
    locale_ctor(IOS_LOCALE(this));
    basic_streambuf_char__Init_empty(this);

    return this;
}

/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_dtor, 4)
void __thiscall basic_streambuf_char_dtor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    locale_dtor(IOS_LOCALE(this));
}

DEFINE_THISCALL_WRAPPER(basic_streambuf_char_vector_dtor, 8)
basic_streambuf_char* __thiscall basic_streambuf_char_vector_dtor(basic_streambuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_streambuf_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
static streamsize basic_streambuf_char__Gnavail(const basic_streambuf_char *this)
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
static char* basic_streambuf_char__Gnpreinc(basic_streambuf_char *this)
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

/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
static streamsize basic_streambuf_char__Pnavail(const basic_streambuf_char *this)
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
#define call_basic_streambuf_char_underflow(this) CALL_VTBL_FUNC(this, 16, int, (basic_streambuf_char*), (this))
int __thiscall basic_streambuf_char_underflow(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return EOF;
}

/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_uflow, 4)
#define call_basic_streambuf_char_uflow(this) CALL_VTBL_FUNC(this, 20, int, (basic_streambuf_char*), (this))
int __thiscall basic_streambuf_char_uflow(basic_streambuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(call_basic_streambuf_char_underflow(this)==EOF)
        return EOF;

    ret = (unsigned char)**this->prpos;
    (*this->prsize)--;
    (*this->prpos)++;
    return ret;
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

/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_getloc, 8)
locale* __thiscall basic_streambuf_char_getloc(const basic_streambuf_char *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, IOS_LOCALE(this));
}

/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_imbue, 8)
#define call_basic_streambuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 48, void, (basic_streambuf_char*, const locale*), (this, loc))
void __thiscall basic_streambuf_char_imbue(basic_streambuf_char *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_overflow, 8)
#define call_basic_streambuf_char_overflow(this, ch) CALL_VTBL_FUNC(this, 4, int, (basic_streambuf_char*, int), (this, ch))
int __thiscall basic_streambuf_char_overflow(basic_streambuf_char *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return EOF;
}

/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbackfail, 8)
#define call_basic_streambuf_char_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, int, (basic_streambuf_char*, int), (this, ch))
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
    memcpy(ret, IOS_LOCALE(this), sizeof(locale));
    call_basic_streambuf_char_imbue(this, loc);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekoff, 20)
#define call_basic_streambuf_char_seekoff(this, ret, off, way, mode) CALL_VTBL_FUNC(this, 32, fpos_int*, (basic_streambuf_char*, fpos_int*, streamoff, int, int), (this, ret, off, way, mode))
fpos_int* __thiscall basic_streambuf_char_seekoff(basic_streambuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %Id %d %d)\n", this, off, way, mode);
    ret->off = -1;
    ret->pos = 0;
    ret->state = 0;
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JFF@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JFF@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff, 20)
fpos_int* __thiscall basic_streambuf_char_pubseekoff(basic_streambuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %Id %d %d)\n", this, off, way, mode);
    return call_basic_streambuf_char_seekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekpos, 36)
#define call_basic_streambuf_char_seekpos(this, ret, pos, mode) CALL_VTBL_FUNC(this, 36, fpos_int*, (basic_streambuf_char*, fpos_int*, fpos_int, int), (this, ret, pos, mode))
fpos_int* __thiscall basic_streambuf_char_seekpos(basic_streambuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    ret->off = -1;
    ret->pos = 0;
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

/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEPAV12@PADH@Z */
/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAPEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setbuf, 12)
#define call_basic_streambuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 40, basic_streambuf_char*, (basic_streambuf_char*, char*, streamsize), (this, buf, count))
basic_streambuf_char* __thiscall basic_streambuf_char_setbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PADH@Z */
/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsetbuf, 12)
basic_streambuf_char* __thiscall basic_streambuf_char_pubsetbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);
    return call_basic_streambuf_char_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sync, 4)
#define call_basic_streambuf_char_sync(this) CALL_VTBL_FUNC(this, 44, int, (basic_streambuf_char*), (this))
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

/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPADH@Z */
/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsgetn, 12)
#define call_basic_streambuf_char_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, streamsize, (basic_streambuf_char*, char*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_char_xsgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    streamsize copied, chunk;
    int c;

    TRACE("(%p %p %Id)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_char__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(ptr+copied, *this->prpos, chunk);
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
        }else if((c = call_basic_streambuf_char_uflow(this)) != EOF) {
            ptr[copied] = c;
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetn, 12)
streamsize __thiscall basic_streambuf_char_sgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, ptr, count);
    return call_basic_streambuf_char_xsgetn(this, ptr, count);
}

/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_showmanyc, 4)
#define call_basic_streambuf_char_showmanyc(this) CALL_VTBL_FUNC(this, 12, streamsize, (basic_streambuf_char*), (this))
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
        return (unsigned char)ch;
    }

    return call_basic_streambuf_char_pbackfail(this, (unsigned char)ch);
}

/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHD@Z */
/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputc, 8)
int __thiscall basic_streambuf_char_sputc(basic_streambuf_char *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return basic_streambuf_char__Pnavail(this) ?
        (unsigned char)(*basic_streambuf_char__Pninc(this) = ch) :
        call_basic_streambuf_char_overflow(this, (unsigned char)ch);
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
        return (unsigned char)**this->prpos;
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
        (int)(unsigned char)*basic_streambuf_char__Gninc(this) : call_basic_streambuf_char_uflow(this);
}

/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetc, 4)
int __thiscall basic_streambuf_char_sgetc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_char__Gnavail(this) ?
        (int)(unsigned char)*basic_streambuf_char_gptr(this) : call_basic_streambuf_char_underflow(this);
}

/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_snextc, 4)
int __thiscall basic_streambuf_char_snextc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    if(basic_streambuf_char__Gnavail(this) > 1)
        return (unsigned char)*basic_streambuf_char__Gnpreinc(this);
    return basic_streambuf_char_sbumpc(this)==EOF ?
        EOF : basic_streambuf_char_sgetc(this);
}

/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPBDH@Z */
/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEBD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsputn, 12)
#define call_basic_streambuf_char_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 28, streamsize, (basic_streambuf_char*, const char*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_char_xsputn(basic_streambuf_char *this, const char *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %Id)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_char__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(*this->pwpos, ptr+copied, chunk);
            *this->pwpos += chunk;
            *this->pwsize -= chunk;
            copied += chunk;
        }else if(call_basic_streambuf_char_overflow(this, (unsigned char)ptr[copied]) != EOF) {
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
    TRACE("(%p %p %Id)\n", this, ptr, count);
    return call_basic_streambuf_char_xsputn(this, ptr, count);
}

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

/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG0@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG0@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setp, 12)
void __thiscall basic_streambuf_wchar_setp(basic_streambuf_wchar *this, wchar_t *first, wchar_t *last)
{
    basic_streambuf_wchar_setp_next(this, first, first, last);
}

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

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor_uninitialized, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor_uninitialized(basic_streambuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    this->vtable = &basic_streambuf_short_vtable;
    return this;
}

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor, 4)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &basic_streambuf_short_vtable;
    locale_ctor(IOS_LOCALE(this));
    basic_streambuf_wchar__Init_empty(this);

    return this;
}

/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_dtor, 4)
void __thiscall basic_streambuf_wchar_dtor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    locale_dtor(IOS_LOCALE(this));
}

DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_vector_dtor, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_vector_dtor(basic_streambuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_streambuf_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
static streamsize basic_streambuf_wchar__Gnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos ? *this->prsize : 0;
}

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

/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    return (*this->prpos)++;
}

/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
static wchar_t* basic_streambuf_wchar__Gnpreinc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    (*this->prpos)++;
    return *this->prpos;
}

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

/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
static streamsize basic_streambuf_wchar__Pnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos ? *this->pwsize : 0;
}

/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Pninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Pninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->pwsize)--;
    return (*this->pwpos)++;
}

/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_underflow, 4)
#define call_basic_streambuf_wchar_underflow(this) CALL_VTBL_FUNC(this, 16, unsigned short, (basic_streambuf_wchar*), (this))
unsigned short __thiscall basic_streambuf_wchar_underflow(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return WEOF;
}

/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_uflow, 4)
#define call_basic_streambuf_wchar_uflow(this) CALL_VTBL_FUNC(this, 20, unsigned short, (basic_streambuf_wchar*), (this))
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

/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_eback, 4)
wchar_t* __thiscall basic_streambuf_wchar_eback(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prbuf;
}

/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_gptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos;
}

/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_egptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_egptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos+*this->prsize;
}

/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_epptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_epptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos+*this->pwsize;
}

/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gbump, 8)
void __thiscall basic_streambuf_wchar_gbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->prpos += off;
    *this->prsize -= off;
}

/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_imbue, 8)
#define call_basic_streambuf_wchar_imbue(this, loc) CALL_VTBL_FUNC(this, 48, void, (basic_streambuf_wchar*, const locale*), (this, loc))
void __thiscall basic_streambuf_wchar_imbue(basic_streambuf_wchar *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_overflow, 8)
#define call_basic_streambuf_wchar_overflow(this, ch) CALL_VTBL_FUNC(this, 4, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
unsigned short __thiscall basic_streambuf_wchar_overflow(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbackfail, 8)
#define call_basic_streambuf_wchar_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
unsigned short __thiscall basic_streambuf_wchar_pbackfail(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbase, 4)
wchar_t* __thiscall basic_streambuf_wchar_pbase(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwbuf;
}

/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbump, 8)
void __thiscall basic_streambuf_wchar_pbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->pwpos += off;
    *this->pwsize -= off;
}

/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_pptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos;
}

/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubimbue, 12)
locale* __thiscall basic_streambuf_wchar_pubimbue(basic_streambuf_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    memcpy(ret, IOS_LOCALE(this), sizeof(locale));
    call_basic_streambuf_wchar_imbue(this, loc);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekoff, 20)
#define call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode) CALL_VTBL_FUNC(this, 32, fpos_int*, (basic_streambuf_wchar*, fpos_int*, streamoff, int, int), (this, ret, off, way, mode))
fpos_int* __thiscall basic_streambuf_wchar_seekoff(basic_streambuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %Id %d %d)\n", this, off, way, mode);
    ret->off = -1;
    ret->pos = 0;
    ret->state = 0;
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JFF@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JFF@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JW4seekdir@ioos_base@2@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff, 20)
fpos_int* __thiscall basic_streambuf_wchar_pubseekoff(basic_streambuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %Id %d %d)\n", this, off, way, mode);
    return call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekpos, 36)
#define call_basic_streambuf_wchar_seekpos(this, ret, pos, mode) CALL_VTBL_FUNC(this, 36, fpos_int*, (basic_streambuf_wchar*, fpos_int*, fpos_int, int), (this, ret, pos, mode))
fpos_int* __thiscall basic_streambuf_wchar_seekpos(basic_streambuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    ret->off = -1;
    ret->pos = 0;
    ret->state = 0;
    return ret;
}

/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekpos, 36)
fpos_int* __thiscall basic_streambuf_wchar_pubseekpos(basic_streambuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_int(&pos), mode);
    return call_basic_streambuf_wchar_seekpos(this, ret, pos, mode);
}

/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEPAV12@PAGH@Z */
/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAPEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setbuf, 12)
#define call_basic_streambuf_wchar_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 40, basic_streambuf_wchar*, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, buf, count))
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_setbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PAGH@Z */
/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsetbuf, 12)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_pubsetbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);
    return call_basic_streambuf_wchar_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sync, 4)
#define call_basic_streambuf_wchar_sync(this) CALL_VTBL_FUNC(this, 44, int, (basic_streambuf_wchar*), (this))
int __thiscall basic_streambuf_wchar_sync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsync, 4)
int __thiscall basic_streambuf_wchar_pubsync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_basic_streambuf_wchar_sync(this);
}

/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPAGH@Z */
/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsgetn, 12)
#define call_basic_streambuf_wchar_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, streamsize, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_wchar_xsgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    streamsize copied, chunk;
    unsigned short c;

    TRACE("(%p %p %Id)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_wchar__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(ptr+copied, *this->prpos, chunk*sizeof(wchar_t));
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
        }else if((c = call_basic_streambuf_wchar_uflow(this)) != WEOF) {
            ptr[copied] = c;
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPAGH@Z */
/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetn, 12)
streamsize __thiscall basic_streambuf_wchar_sgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, ptr, count);
    return call_basic_streambuf_wchar_xsgetn(this, ptr, count);
}

/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_showmanyc, 4)
#define call_basic_streambuf_wchar_showmanyc(this) CALL_VTBL_FUNC(this, 12, streamsize, (basic_streambuf_wchar*), (this))
streamsize __thiscall basic_streambuf_wchar_showmanyc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

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

/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sbumpc, 4)
unsigned short __thiscall basic_streambuf_wchar_sbumpc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar__Gninc(this) : call_basic_streambuf_wchar_uflow(this);
}

/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetc, 4)
unsigned short __thiscall basic_streambuf_wchar_sgetc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar_gptr(this) : call_basic_streambuf_wchar_underflow(this);
}

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

/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPBGH@Z */
/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEBG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsputn, 12)
#define call_basic_streambuf_wchar_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 28, streamsize, (basic_streambuf_wchar*, const wchar_t*, streamsize), (this, ptr, count))
streamsize __thiscall basic_streambuf_wchar_xsputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %Id)\n", this, ptr, count);

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_wchar__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(*this->pwpos, ptr+copied, chunk*sizeof(wchar_t));
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

/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPBGH@Z */
/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEBG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputn, 12)
streamsize __thiscall basic_streambuf_wchar_sputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, ptr, count);
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
    this->state0 = basic_filebuf_char__Init__Stinit;
    this->state = basic_filebuf_char__Init__Stinit;
    if(which == INITFL_new)
        this->str = NULL;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_char__Init_empty(&this->base);
    if(file)
        basic_streambuf_char__Init(&this->base, &file->_base, &file->_ptr,
                &file->_cnt, &file->_base, &file->_ptr, &file->_cnt);
}

/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXXZ */
/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Initcvt, 4)
void __thiscall basic_filebuf_char__Initcvt(basic_filebuf_char *this)
{
    codecvt_char *cvt = codecvt_char_use_facet(IOS_LOCALE(&this->base));

    TRACE("(%p)\n", this);

    locale__Addfac(&this->loc, &cvt->base.facet, codecvt_char_id.id, LC_CTYPE);
    if(codecvt_base_always_noconv(&cvt->base)) {
        this->cvt = NULL;
    }else {
        this->str = operator_new(sizeof(basic_string_char));
        MSVCP_basic_string_char_ctor(this->str);
        this->cvt = cvt;
    }
}

/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_close, 4)
basic_filebuf_char* __thiscall basic_filebuf_char_close(basic_filebuf_char *this)
{
    basic_filebuf_char *ret = this;

    TRACE("(%p)\n", this);

    if(!this->file || fclose(this->file))
        return NULL;

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
    this->base.vtable = &basic_filebuf_char_vtable;

    locale_ctor(&this->loc);
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

    basic_streambuf_char_ctor_uninitialized(&this->base, 0);
    this->base.vtable = &basic_filebuf_char_vtable;
    locale_ctor(&this->loc);
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
    if(this->str) {
        MSVCP_basic_string_char_dtor(this->str);
        operator_delete(this->str);
    }
    locale_dtor(&this->loc);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_filebuf_char_vector_dtor, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_vector_dtor(basic_filebuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_filebuf_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_filebuf_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_is_open, 4)
bool __thiscall basic_filebuf_char_is_open(const basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);
    return this->file != NULL;
}

/* ?_Fiopen@std@@YAPAU_iobuf@@PB_WHH@Z */
/* ?_Fiopen@std@@YAPEAU_iobuf@@PEB_WHH@Z */
static FILE* _Fiopen_wchar(const wchar_t *name, int mode, int prot)
{
    static const struct {
        int mode;
        const wchar_t str[4];
        const wchar_t str_bin[4];
    } str_mode[] = {
        {OPENMODE_out,                              L"w",   L"wb"},
        {OPENMODE_out|OPENMODE_app,                 L"a",   L"ab"},
        {OPENMODE_app,                              L"a",   L"ab"},
        {OPENMODE_out|OPENMODE_trunc,               L"w",   L"wb"},
        {OPENMODE_in,                               L"r",   L"rb"},
        {OPENMODE_in|OPENMODE_out,                  L"r+",  L"r+b"},
        {OPENMODE_in|OPENMODE_out|OPENMODE_trunc,   L"w+",  L"w+b"},
        {OPENMODE_in|OPENMODE_out|OPENMODE_app,     L"a+",  L"a+b"},
        {OPENMODE_in|OPENMODE_app,                  L"a+",  L"a+b"}
    };

    int real_mode = mode & ~(OPENMODE_ate|OPENMODE__Nocreate|OPENMODE__Noreplace|OPENMODE_binary);
    size_t mode_idx;
    FILE *f = NULL;

    TRACE("(%s %d %d)\n", debugstr_w(name), mode, prot);

    for(mode_idx=0; mode_idx<ARRAY_SIZE(str_mode); mode_idx++)
        if(str_mode[mode_idx].mode == real_mode)
            break;
    if(mode_idx == ARRAY_SIZE(str_mode))
        return NULL;

    if((mode & OPENMODE__Nocreate) && !(f = _wfopen(name, L"r")))
        return NULL;
    else if(f)
        fclose(f);

    if((mode & OPENMODE__Noreplace) && (mode & (OPENMODE_out|OPENMODE_app))
            && (f = _wfopen(name, L"r"))) {
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

/* ?__Fiopen@std@@YAPAU_iobuf@@PBDH@Z */
/* ?__Fiopen@std@@YAPEAU_iobuf@@PEBDH@Z */
FILE* __cdecl ___Fiopen(const char *name, int mode)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%p %d)\n", name, mode);

    if(!MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, FILENAME_MAX-1))
        return NULL;
    return _Fiopen_wchar(nameW, mode, _SH_DENYNO);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_mode, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_mode(basic_filebuf_char *this, const char *name, unsigned int mode)
{
    FILE *f = NULL;

    TRACE("(%p %s %d)\n", this, name, mode);

    if(basic_filebuf_char_is_open(this))
        return NULL;
    if(!(f = ___Fiopen(name, mode)))
        return NULL;

    basic_filebuf_char__Init(this, f, INITFL_open);
    basic_filebuf_char__Initcvt(this);
    return this;
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDF@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_mode_old, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_mode_old(basic_filebuf_char *this, const char *name, short mode)
{
    return basic_filebuf_char_open_mode(this, name, mode);
}

/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_overflow, 8)
int __thiscall basic_filebuf_char_overflow(basic_filebuf_char *this, int c)
{
    char *ptr, ch = c, *to_next;
    const char *from_next;
    int ret;


    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;
    if(c == EOF)
        return !c;

    if(!this->cvt)
        return fwrite(&ch, sizeof(char), 1, this->file) ? c : EOF;

    from_next = &ch;
    MSVCP_basic_string_char_clear(this->str);
    MSVCP_basic_string_char_append_len_ch(this->str, 8, '\0');
    ptr = this->str->ptr;
    ret = codecvt_char_out(this->cvt, &this->state, from_next, &ch+1, &from_next,
            ptr, ptr+MSVCP_basic_string_char_length(this->str), &to_next);

    switch(ret) {
    case CODECVT_partial:
        if(from_next == &ch)
            return EOF;
    case CODECVT_ok:
        if(!fwrite(ptr, to_next-ptr, 1, this->file))
            return EOF;
        return c;
    case CODECVT_noconv:
        return fwrite(&ch, sizeof(char), 1, this->file) ? c : EOF;
    default:
        return EOF;
    }
}

/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_pbackfail, 8)
int __thiscall basic_filebuf_char_pbackfail(basic_filebuf_char *this, int c)
{
    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base)>basic_streambuf_char_eback(&this->base)
            && (c==EOF || (int)(unsigned char)basic_streambuf_char_gptr(&this->base)[-1]==c)) {
        basic_streambuf_char__Gndec(&this->base);
        return c==EOF ? !c : c;
    }else if(c == EOF) {
        return EOF;
    }else if(!this->cvt) {
        return ungetc(c, this->file);
    }else if(MSVCP_basic_string_char_length(this->str)) {
        char *b, *e, *cur;

        e = this->str->ptr;
        b = e+this->str->size-1;
        for(cur = b; cur>=e; cur--) {
            if(ungetc(*cur, this->file) == EOF) {
                for(; cur<=b; cur++)
                    fgetc(this->file);
                return EOF;
            }
        }
        MSVCP_basic_string_char_clear(this->str);
        this->state = this->state0;
        return c;
    }

    return EOF;
}

/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_uflow, 4)
int __thiscall basic_filebuf_char_uflow(basic_filebuf_char *this)
{
    char ch, *to_next;
    const char *buf_next;
    int c;

    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)*basic_streambuf_char__Gninc(&this->base);

    c = fgetc(this->file);
    if(!this->cvt || c==EOF)
        return c;

    MSVCP_basic_string_char_clear(this->str);
    this->state0 = this->state;
    while(1) {
        MSVCP_basic_string_char_append_ch(this->str, c);
        this->state = this->state0;

        switch(codecvt_char_in(this->cvt, &this->state, this->str->ptr,
                    this->str->ptr+this->str->size, &buf_next, &ch, &ch+1, &to_next)) {
        case CODECVT_partial:
            break;
        case CODECVT_noconv:
            return (unsigned char)this->str->ptr[0];
        case CODECVT_ok:
            return (unsigned char)ch;
        default:
            return EOF;
        }

        c = fgetc(this->file);
        if(c == EOF)
            return EOF;
    }
}

/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_underflow, 4)
int __thiscall basic_filebuf_char_underflow(basic_filebuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)*basic_streambuf_char_gptr(&this->base);

    ret = call_basic_streambuf_char_uflow(&this->base);
    if(ret != EOF)
        ret = call_basic_streambuf_char_pbackfail(&this->base, ret);
    return ret;
}

/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekoff, 20)
fpos_int* __thiscall basic_filebuf_char_seekoff(basic_filebuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    fpos_t pos;

    TRACE("(%p %p %Id %d %d)\n", this, ret, off, way, mode);

    if(!basic_filebuf_char_is_open(this) || fseek(this->file, off, way)) {
        ret->off = -1;
        ret->pos = 0;
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
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekpos, 36)
fpos_int* __thiscall basic_filebuf_char_seekpos(basic_filebuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    fpos_t fpos;

    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(!basic_filebuf_char_is_open(this) || fseek(this->file, (LONG)pos.pos, SEEK_SET)
            || (pos.off && fseek(this->file, pos.off, SEEK_CUR))) {
        ret->off = -1;
        ret->pos = 0;
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
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_setbuf, 12)
basic_streambuf_char* __thiscall basic_filebuf_char_setbuf(basic_filebuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);

    if(!basic_filebuf_char_is_open(this))
        return NULL;

    if(setvbuf(this->file, buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count))
        return NULL;

    basic_filebuf_char__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_sync, 4)
int __thiscall basic_filebuf_char_sync(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return 0;

    if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
        return 0;
    return fflush(this->file);
}

/* ?_Stinit@?1??_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@23@@Z@4HA */
/* ?_Stinit@?1??_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@23@@Z@4HA */
int basic_filebuf_short__Init__Stinit = 0;

/* ?_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@12@@Z */
/* ?_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@12@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short__Init, 12)
void __thiscall basic_filebuf_short__Init(basic_filebuf_wchar *this, FILE *file, basic_filebuf__Initfl which)
{
    TRACE("(%p %p %d)\n", this, file, which);

    this->cvt = NULL;
    this->state0 = basic_filebuf_short__Init__Stinit;
    this->state = basic_filebuf_short__Init__Stinit;
    if(which == INITFL_new)
        this->str = NULL;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXXZ */
/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short__Initcvt, 4)
void __thiscall basic_filebuf_short__Initcvt(basic_filebuf_wchar *this)
{
    codecvt_wchar *cvt = codecvt_short_use_facet(IOS_LOCALE(&this->base));

    TRACE("(%p)\n", this);

    if(codecvt_base_always_noconv(&cvt->base)) {
        this->cvt = NULL;
    }else {
        basic_streambuf_wchar__Init_empty(&this->base);
        this->cvt = cvt;
    }
}

/* ?close@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_close, 4)
basic_filebuf_wchar* __thiscall basic_filebuf_short_close(basic_filebuf_wchar *this)
{
    basic_filebuf_wchar *ret = this;

    TRACE("(%p)\n", this);

    if(!this->file || fclose(this->file))
        return NULL;

    basic_filebuf_short__Init(this, NULL, INITFL_close);
    return ret;
}

/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor_file, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor_file(basic_filebuf_wchar *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);

    basic_streambuf_short_ctor(&this->base);
    this->base.vtable = &basic_filebuf_short_vtable;

    locale_ctor(&this->loc);
    basic_filebuf_short__Init(this, file, INITFL_new);
    return this;
}

/* ??_F?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_F?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor, 4)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor(basic_filebuf_wchar *this)
{
    return basic_filebuf_short_ctor_file(this, NULL);
}

/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor_uninitialized, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor_uninitialized(basic_filebuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);

    basic_streambuf_short_ctor_uninitialized(&this->base, 0);
    this->base.vtable = &basic_filebuf_short_vtable;
    locale_ctor(&this->loc);
    return this;
}

/* ??1?$basic_filebuf@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_filebuf@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_dtor, 4)
void __thiscall basic_filebuf_short_dtor(basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->close)
        basic_filebuf_short_close(this);
    if(this->str) {
        MSVCP_basic_string_char_dtor(this->str);
        operator_delete(this->str);
    }
    locale_dtor(&this->loc);
    basic_streambuf_wchar_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_filebuf_short_vector_dtor, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_short_vector_dtor(basic_filebuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_filebuf_short_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_filebuf_short_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?is_open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_is_open, 4)
bool __thiscall basic_filebuf_short_is_open(const basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->file != NULL;
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open_mode, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open_mode(basic_filebuf_wchar *this, const char *name, unsigned int mode)
{
    wchar_t nameW[FILENAME_MAX];
    FILE *f = NULL;

    TRACE("(%p %p %u)\n", this, name, mode);

    if(basic_filebuf_short_is_open(this))
        return NULL;

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;

    if(!(f = _Fiopen_wchar(nameW, mode, SH_DENYNO)))
        return NULL;

    basic_filebuf_short__Init(this, f, INITFL_open);
    basic_filebuf_short__Initcvt(this);
    return this;
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDF@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open_mode_old, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open_mode_old(basic_filebuf_wchar *this, const char *name, short mode)
{
    TRACE("(%p %p %d)\n", this, name, mode);
    return basic_filebuf_short_open_mode(this, name, mode);
}

/* ?overflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_overflow, 8)
unsigned short __thiscall basic_filebuf_short_overflow(basic_filebuf_wchar *this, unsigned short c)
{
    char *ptr, *to_next;
    wchar_t ch = c;
    const wchar_t *from_next;
    unsigned short ret;


    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_short_is_open(this))
        return WEOF;
    if(c == WEOF)
        return !c;

    if(!this->cvt)
        return fwrite(&ch, sizeof(wchar_t), 1, this->file) ? c : WEOF;

    from_next = &ch;
    MSVCP_basic_string_char_clear(this->str);
    MSVCP_basic_string_char_append_len_ch(this->str, 8, '\0');
    ptr = this->str->ptr;
    ret = codecvt_wchar_out(this->cvt, &this->state, &ch, &ch+1, &from_next,
            ptr, ptr+MSVCP_basic_string_char_length(this->str), &to_next);

    switch(ret) {
    case CODECVT_partial:
        if(from_next == &ch)
            return WEOF;
        /* fall through */
    case CODECVT_ok:
        if(!fwrite(ptr, to_next-ptr, 1, this->file))
            return WEOF;
        return c;
    case CODECVT_noconv:
        return fwrite(&ch, sizeof(wchar_t), 1, this->file) ? c : WEOF;
    default:
        return WEOF;
    }
}

/* ?pbackfail@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_pbackfail, 8)
unsigned short __thiscall basic_filebuf_short_pbackfail(basic_filebuf_wchar *this, unsigned short c)
{
    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_short_is_open(this))
        return WEOF;

    if(basic_streambuf_wchar_gptr(&this->base)>basic_streambuf_wchar_eback(&this->base)
            && (c==WEOF || basic_streambuf_wchar_gptr(&this->base)[-1]==c)) {
        basic_streambuf_wchar__Gndec(&this->base);
        return c==WEOF ? !c : c;
    }else if(c == WEOF) {
        return WEOF;
    }else if(!this->cvt) {
        return ungetwc(c, this->file);
    }else if(MSVCP_basic_string_char_length(this->str)) {
        char *b, *e, *cur;

        e = this->str->ptr;
        b = e+this->str->size-1;
        for(cur = b; cur>=e; cur--) {
            if(ungetc(*cur, this->file) == EOF) {
                for(; cur<=b; cur++)
                    fgetc(this->file);
                return WEOF;
            }
        }
        MSVCP_basic_string_char_clear(this->str);
        this->state = this->state0;
        return c;
    }

    return WEOF;
}

/* ?uflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_uflow, 4)
unsigned short __thiscall basic_filebuf_short_uflow(basic_filebuf_wchar *this)
{
    wchar_t ch, *to_next;
    const char *buf_next;
    int c;

    TRACE("(%p)\n", this);

    if(!basic_filebuf_short_is_open(this))
        return WEOF;

    if(basic_streambuf_wchar_gptr(&this->base) < basic_streambuf_wchar_egptr(&this->base))
        return *basic_streambuf_wchar__Gninc(&this->base);

    if(!this->cvt)
        return fgetwc(this->file);

    MSVCP_basic_string_char_clear(this->str);
    this->state0 = this->state;
    while(1) {
        if((c = fgetc(this->file)) == EOF)
            return WEOF;
        MSVCP_basic_string_char_append_ch(this->str, c);
        this->state = this->state0;

        switch(codecvt_wchar_in(this->cvt, &this->state, this->str->ptr,
                    this->str->ptr+this->str->size, &buf_next, &ch, &ch+1, &to_next)) {
        case CODECVT_partial:
            break;
        case CODECVT_noconv:
            if(this->str->size < sizeof(unsigned short)/sizeof(char))
                break;
            return *(unsigned short*)this->str->ptr;
        case CODECVT_ok:
            return (unsigned short)ch;
        default:
            return WEOF;
        }
    }
}

/* ?underflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_underflow, 4)
unsigned short __thiscall basic_filebuf_short_underflow(basic_filebuf_wchar *this)
{
    unsigned short ret;

    TRACE("(%p)\n", this);

    if(basic_streambuf_wchar_gptr(&this->base) < basic_streambuf_wchar_egptr(&this->base))
        return *basic_streambuf_wchar_gptr(&this->base);

    ret = call_basic_streambuf_wchar_uflow(&this->base);
    if(ret != WEOF)
        ret = call_basic_streambuf_wchar_pbackfail(&this->base, ret);
    return ret;
}

/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_seekoff, 20)
fpos_int* __thiscall basic_filebuf_short_seekoff(basic_filebuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    fpos_t pos;

    TRACE("(%p %p %Id %d %d)\n", this, ret, off, way, mode);

    if(!basic_filebuf_short_is_open(this) || fseek(this->file, off, way)) {
        ret->off = -1;
        ret->pos = 0;
        ret->state = 0;
        return ret;
    }

    fgetpos(this->file, &pos);
    ret->off = 0;
    ret->pos = pos;
    ret->state = this->state;
    return ret;
}

/* ?seekpos@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_seekpos, 36)
fpos_int* __thiscall basic_filebuf_short_seekpos(basic_filebuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    fpos_t fpos;

    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(!basic_filebuf_short_is_open(this) || fseek(this->file, (LONG)pos.pos, SEEK_SET)
            || (pos.off && fseek(this->file, pos.off, SEEK_CUR))) {
        ret->off = -1;
        ret->pos = 0;
        ret->state = 0;
        return ret;
    }

    fgetpos(this->file, &fpos);
    ret->off = 0;
    ret->pos = fpos;
    ret->state = this->state;
    return ret;
}

/* ?setbuf@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PAGH@Z */
/* ?setbuf@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_setbuf, 12)
basic_streambuf_wchar* __thiscall basic_filebuf_short_setbuf(basic_filebuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %Id)\n", this, buf, count);

    if(!basic_filebuf_short_is_open(this))
        return NULL;

    if(setvbuf(this->file, (char*)buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count*sizeof(wchar_t)))
        return NULL;

    basic_filebuf_short__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?sync@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_sync, 4)
int __thiscall basic_filebuf_short_sync(basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_short_is_open(this))
        return 0;

    if(call_basic_streambuf_wchar_overflow(&this->base, WEOF) == WEOF)
        return 0;
    return fflush(this->file);
}

/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAHH@Z */
/* ?_Mode@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEHH@Z */
/* ?_Mode@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAHH@Z */
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
void __thiscall basic_stringbuf_char__Init(basic_stringbuf_char *this, const char *str, size_t count, int state)
{
    TRACE("(%p, %p, %Iu, %d)\n", this, str, count, state);

    basic_streambuf_char__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        char *buf = operator_new(count);

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
    this->base.vtable = &basic_stringbuf_char_vtable;

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
    this->base.vtable = &basic_stringbuf_char_vtable;

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
        operator_delete(basic_streambuf_char_eback(&this->base));
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

DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_vector_dtor, 8)
basic_stringbuf_char* __thiscall basic_stringbuf_char_vector_dtor(basic_stringbuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_char_dtor(this+i);

        operator_delete(ptr);
    }else {
        basic_stringbuf_char_dtor(this);

        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHH@Z */
/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_overflow, 8)
int __thiscall basic_stringbuf_char_overflow(basic_stringbuf_char *this, int meta)
{
    size_t oldsize, size;
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
        return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = meta);

    oldsize = (ptr ? basic_streambuf_char_epptr(&this->base)-basic_streambuf_char_eback(&this->base): 0);
    size = oldsize|0xf;
    size += size/2;
    buf = operator_new(size);

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

        operator_delete(ptr);
    }

    return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = meta);
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
        return (unsigned char)*cur;
    return EOF;
}

/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekoff, 20)
fpos_int* __thiscall basic_stringbuf_char_seekoff(basic_stringbuf_char *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    char *beg, *cur_r, *cur_w;

    TRACE("(%p %p %Id %d %d)\n", this, ret, off, way, mode);

    cur_w = basic_streambuf_char_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->pos = 0;
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

    ret->off = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekpos, 36)
fpos_int* __thiscall basic_stringbuf_char_seekpos(basic_stringbuf_char *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && pos.state==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_char_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
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

/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAHH@Z */
/* ?_Mode@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEHH@Z */
/* ?_Mode@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short__Getstate, 8)
int __thiscall basic_stringbuf_short__Getstate(basic_stringbuf_wchar *this, IOSB_openmode mode)
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

/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXPBGIH@Z */
/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXPEBG_KH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short__Init, 16)
void __thiscall basic_stringbuf_short__Init(basic_stringbuf_wchar *this, const wchar_t *str, size_t count, int state)
{
    TRACE("(%p, %p, %Iu, %d)\n", this, str, count, state);

    basic_streambuf_wchar__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        wchar_t *buf = operator_new(count*sizeof(wchar_t));

        memcpy(buf, str, count*sizeof(wchar_t));
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

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_str, 12)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_str(basic_stringbuf_wchar *this,
        const basic_string_wchar *str, IOSB_openmode mode)
{
    TRACE("(%p %p %d)\n", this, str, mode);

    basic_streambuf_short_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_short_vtable;

    basic_stringbuf_short__Init(this, MSVCP_basic_string_wchar_c_str(str),
            str->size, basic_stringbuf_short__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_mode, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_mode(
        basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    TRACE("(%p %d)\n", this, mode);

    basic_streambuf_short_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_short_vtable;

    basic_stringbuf_short__Init(this, NULL, 0, basic_stringbuf_short__Getstate(this, mode));
    return this;
}

/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor, 4)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor(basic_stringbuf_wchar *this)
{
    return basic_stringbuf_short_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short__Tidy, 4)
void __thiscall basic_stringbuf_short__Tidy(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->state & STRINGBUF_allocated) {
        operator_delete(basic_streambuf_wchar_eback(&this->base));
        this->seekhigh = NULL;
        this->state &= ~STRINGBUF_allocated;
    }

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_dtor, 4)
void __thiscall basic_stringbuf_short_dtor(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_stringbuf_short__Tidy(this);
    basic_streambuf_wchar_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_vector_dtor, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_vector_dtor(basic_stringbuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_short_dtor(this+i);

        operator_delete(ptr);
    }else {
        basic_stringbuf_short_dtor(this);

        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_overflow, 8)
unsigned short __thiscall basic_stringbuf_short_overflow(basic_stringbuf_wchar *this, unsigned short meta)
{
    size_t oldsize, size;
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
    buf = operator_new(size*sizeof(wchar_t));

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
        memcpy(buf, ptr, oldsize*sizeof(wchar_t));

        this->seekhigh = buf+(this->seekhigh-ptr);
        basic_streambuf_wchar_setp_next(&this->base, buf,
                buf+(basic_streambuf_wchar_pptr(&this->base)-ptr), buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_wchar_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_wchar_setg(&this->base, buf,
                    buf+(basic_streambuf_wchar_gptr(&this->base)-ptr),
                    basic_streambuf_wchar_pptr(&this->base)+1);

        operator_delete(ptr);
    }

    return (*basic_streambuf_wchar__Pninc(&this->base) = meta);
}

/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_pbackfail, 8)
unsigned short __thiscall basic_stringbuf_short_pbackfail(basic_stringbuf_wchar *this, unsigned short c)
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

/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGXZ */
/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_underflow, 4)
unsigned short __thiscall basic_stringbuf_short_underflow(basic_stringbuf_wchar *this)
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

/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_seekoff, 20)
fpos_int* __thiscall basic_stringbuf_short_seekoff(basic_stringbuf_wchar *this,
        fpos_int *ret, streamoff off, int way, int mode)
{
    wchar_t *beg, *cur_r, *cur_w;

    TRACE("(%p %p %Id %d %d)\n", this, ret, off, way, mode);

    cur_w = basic_streambuf_wchar_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->pos = 0;
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

    ret->off = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_seekpos, 36)
fpos_int* __thiscall basic_stringbuf_short_seekpos(basic_stringbuf_wchar *this,
        fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && pos.state==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_short_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
}

/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_str_set, 8)
void __thiscall basic_stringbuf_short_str_set(basic_stringbuf_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);

    basic_stringbuf_short__Tidy(this);
    basic_stringbuf_short__Init(this, MSVCP_basic_string_wchar_c_str(str), str->size, this->state);
}

/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_str_get, 8)
basic_string_wchar* __thiscall basic_stringbuf_short_str_get(const basic_stringbuf_wchar *this, basic_string_wchar *ret)
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
    this->vtable = &ios_base_vtable;
    locale_ctor_uninitialized(IOS_LOCALE(this), 0);
    return this;
}

/* ??0ios_base@std@@QAE@ABV01@@Z */
/* ??0ios_base@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copy_ctor, 8)
ios_base* __thiscall ios_base_copy_ctor(ios_base *this, const ios_base *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->vtable = &ios_base_vtable;
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

/* ?_Tidy@ios_base@std@@AAEXXZ */
/* ?_Tidy@ios_base@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_Tidy, 4)
void __thiscall ios_base_Tidy(ios_base *this)
{
    IOS_BASE_iosarray *arr_cur, *arr_next;
    IOS_BASE_fnarray *event_cur, *event_next;

    TRACE("(%p)\n", this);

    ios_base_Callfns(this, EVENT_erase_event);

    for(arr_cur=this->arr; arr_cur; arr_cur=arr_next) {
        arr_next = arr_cur->next;
        operator_delete(arr_cur);
    }
    this->arr = NULL;

    for(event_cur=this->calls; event_cur; event_cur=event_next) {
        event_next = event_cur->next;
        operator_delete(event_cur);
    }
    this->calls = NULL;
}

/* ??1ios_base@std@@UAE@XZ */
/* ??1ios_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_dtor, 4)
void __thiscall ios_base_dtor(ios_base *this)
{
    TRACE("(%p)\n", this);
    locale_dtor(IOS_LOCALE(this));
    ios_base_Tidy(this);
}

DEFINE_THISCALL_WRAPPER(ios_base_vector_dtor, 8)
ios_base* __thiscall ios_base_vector_dtor(ios_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ios_base_dtor(this+i);
        operator_delete(ptr);
    } else {
        ios_base_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(iosb_vector_dtor, 8)
void* __thiscall iosb_vector_dtor(void *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        INT_PTR *ptr = (INT_PTR *)this-1;
        operator_delete(ptr);
    } else {
        if(flags & 1)
            operator_delete(this);
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

    p = operator_new(sizeof(IOS_BASE_iosarray));
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

    event = operator_new(sizeof(IOS_BASE_fnarray));
    event->next = this->calls;
    event->index = index;
    event->event_handler = callback;
    this->calls = event;
}

/* ?clear@ios_base@std@@QAEXH_N@Z */
/* ?clear@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_reraise, 12)
void __thiscall ios_base_clear_reraise(ios_base *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    this->state = state & IOSTATE_mask;
    if(!(this->state & this->except))
        return;

    if(reraise)
        _CxxThrowException(NULL, NULL);
    else if(this->state & this->except & IOSTATE_eofbit)
        throw_failure("eofbit is set");
    else if(this->state & this->except & IOSTATE_failbit)
        throw_failure("failbit is set");
    else if(this->state & this->except & IOSTATE_badbit)
        throw_failure("badbit is set");
    else if(this->state & this->except & IOSTATE__Hardfail)
        throw_failure("_Hardfail is set");
}

/* ?clear@ios_base@std@@QAEXF@Z */
/* ?clear@ios_base@std@@QEAAXF@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear, 8)
void __thiscall ios_base_clear(ios_base *this, IOSB_iostate state)
{
    ios_base_clear_reraise(this, state, FALSE);
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
        locale_operator_assign(IOS_LOCALE(this), IOS_LOCALE(rhs));

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
bool __thiscall ios_base_fail(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & (IOSTATE_failbit|IOSTATE_badbit)) != 0;
}

/* ??7ios_base@std@@QBE_NXZ */
/* ??7ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_succ, 4)
bool __thiscall ios_base_op_succ(const ios_base *this)
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

/* ?_Addstd@ios_base@std@@IAEXXZ */
/* ?_Addstd@ios_base@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_Addstd, 4)
void __thiscall ios_base_Addstd(ios_base *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?_Init@ios_base@std@@IAEXXZ */
/* ?_Init@ios_base@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base__Init, 4)
void __thiscall ios_base__Init(ios_base *this)
{
    TRACE("(%p)\n", this);

    this->stdstr = 0;
    this->state = this->except = IOSTATE_goodbit;
    this->fmtfl = FMTFLAG_skipws | FMTFLAG_dec;
    this->prec = 6;
    this->wide = 0;
    this->arr = NULL;
    this->calls = NULL;
    locale_ctor(IOS_LOCALE(this));
}

/* ?bad@ios_base@std@@QBE_NXZ */
/* ?bad@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_bad, 4)
bool __thiscall ios_base_bad(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_badbit) != 0;
}

/* ?eof@ios_base@std@@QBE_NXZ */
/* ?eof@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_eof, 4)
bool __thiscall ios_base_eof(const ios_base *this)
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
    return locale_copy_ctor(ret, IOS_LOCALE(this));
}

/* ?good@ios_base@std@@QBE_NXZ */
/* ?good@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_good, 4)
bool __thiscall ios_base_good(const ios_base *this)
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
    *ret = *IOS_LOCALE(this);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?precision@ios_base@std@@QAEHH@Z */
/* ?precision@ios_base@std@@QEAA_J_J@Z */
DEFINE_THISCALL_WRAPPER(ios_base_precision_set, 8)
streamsize __thiscall ios_base_precision_set(ios_base *this, streamsize precision)
{
    streamsize ret = this->prec;

    TRACE("(%p %Id)\n", this, precision);

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
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x)\n", this, flags);

    this->fmtfl |= flags & FMTFLAG_mask;
    return ret;
}

/* ?setstate@ios_base@std@@QAEXH_N@Z */
/* ?setstate@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate_reraise, 12)
void __thiscall ios_base_setstate_reraise(ios_base *this, IOSB_iostate state, bool reraise)
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

/* ?sync_with_stdio@ios_base@std@@SA_N_N@Z */
bool __cdecl ios_base_sync_with_stdio(bool sync)
{
    _Lockit lock;
    bool ret;

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

    TRACE("(%p %Id)\n", this, width);

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
int __cdecl ios_base_xalloc(void)
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
    this->base.vtable = &basic_ios_char_vtable;
    return this;
}

/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IAEXPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_init, 12)
void __thiscall basic_ios_char_init(basic_ios_char *this, basic_streambuf_char *streambuf, bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base__Init(&this->base);
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

DEFINE_THISCALL_WRAPPER(basic_ios_char_vector_dtor, 8)
basic_ios_char* __thiscall basic_ios_char_vector_dtor(basic_ios_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ios_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_clear_reraise, 12)
void __thiscall basic_ios_char_clear_reraise(basic_ios_char *this, IOSB_iostate state, bool reraise)
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
    return ctype_char_narrow_ch(ctype_char_use_facet(IOS_LOCALE(this->strbuf)), ch, def);
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
void __thiscall basic_ios_char_setstate_reraise(basic_ios_char *this, IOSB_iostate state, bool reraise)
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
    return ctype_char_widen_ch(ctype_char_use_facet(IOS_LOCALE(this->strbuf)), ch);
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor, 4)
basic_ios_wchar* __thiscall basic_ios_short_ctor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);

    ios_base_ctor(&this->base);
    this->base.vtable = &basic_ios_short_vtable;
    return this;
}

/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IAEXPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IEAAXPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_init, 12)
void __thiscall basic_ios_short_init(basic_ios_wchar *this, basic_streambuf_wchar *streambuf, bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base__Init(&this->base);
    this->strbuf = streambuf;
    this->stream = NULL;
    this->fillch = ' ';

    if(!streambuf)
        ios_base_setstate(&this->base, IOSTATE_badbit);

    if(isstd)
        FIXME("standard streams not handled yet\n");
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor_streambuf, 8)
basic_ios_wchar* __thiscall basic_ios_short_ctor_streambuf(basic_ios_wchar *this, basic_streambuf_wchar *strbuf)
{
    TRACE("(%p %p)\n", this, strbuf);

    basic_ios_short_ctor(this);
    basic_ios_short_init(this, strbuf, FALSE);
    return this;
}

/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_dtor, 4)
void __thiscall basic_ios_short_dtor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    ios_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_ios_short_vector_dtor, 8)
basic_ios_wchar* __thiscall basic_ios_short_vector_dtor(basic_ios_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_short_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ios_short_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_clear_reraise, 12)
void __thiscall basic_ios_short_clear_reraise(basic_ios_wchar *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);
    ios_base_clear_reraise(&this->base, state | (this->strbuf ? IOSTATE_goodbit : IOSTATE_badbit), reraise);
}

/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_clear, 8)
void __thiscall basic_ios_short_clear(basic_ios_wchar *this, unsigned int state)
{
    basic_ios_short_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_copyfmt, 8)
basic_ios_wchar* __thiscall basic_ios_short_copyfmt(basic_ios_wchar *this, basic_ios_wchar *copy)
{
    TRACE("(%p %p)\n", this, copy);
    if(this == copy)
        return this;

    this->stream = copy->stream;
    this->fillch = copy->fillch;
    ios_base_copyfmt(&this->base, &copy->base);
    return this;
}

/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_fill_set, 8)
wchar_t __thiscall basic_ios_short_fill_set(basic_ios_wchar *this, wchar_t fill)
{
    wchar_t ret = this->fillch;

    TRACE("(%p %c)\n", this, fill);

    this->fillch = fill;
    return ret;
}

/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGXZ */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_fill_get, 4)
wchar_t __thiscall basic_ios_short_fill_get(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->fillch;
}

/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_imbue, 12)
locale *__thiscall basic_ios_short_imbue(basic_ios_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p %p)\n", this, ret, loc);

    if(this->strbuf) {
        basic_streambuf_wchar_pubimbue(this->strbuf, ret, loc);
        locale_dtor(ret);
    }

    return ios_base_imbue(&this->base, ret, loc);
}

/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEDGD@Z */
/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_narrow, 12)
char __thiscall basic_ios_short_narrow(basic_ios_wchar *this, wchar_t ch, char def)
{
    TRACE("(%p %c %c)\n", this, ch, def);
    return ctype_wchar_narrow_ch(ctype_wchar_use_facet(IOS_LOCALE(this->strbuf)), ch, def);
}

/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_rdbuf_set, 8)
basic_streambuf_wchar* __thiscall basic_ios_short_rdbuf_set(basic_ios_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_streambuf_wchar *ret = this->strbuf;

    TRACE("(%p %p)\n", this, streambuf);

    this->strbuf = streambuf;
    basic_ios_short_clear(this, IOSTATE_goodbit);
    return ret;
}

/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_rdbuf_get, 4)
basic_streambuf_wchar* __thiscall basic_ios_short_rdbuf_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->strbuf;
}

/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_setstate_reraise, 12)
void __thiscall basic_ios_short_setstate_reraise(basic_ios_wchar *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        basic_ios_short_clear_reraise(this, this->base.state | state, reraise);
}

/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_setstate, 8)
void __thiscall basic_ios_short_setstate(basic_ios_wchar *this, IOSB_iostate state)
{
    basic_ios_short_setstate_reraise(this, state, FALSE);
}

/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_tie_set, 8)
basic_ostream_wchar* __thiscall basic_ios_short_tie_set(basic_ios_wchar *this, basic_ostream_wchar *ostream)
{
    basic_ostream_wchar *ret = this->stream;

    TRACE("(%p %p)\n", this, ostream);

    this->stream = ostream;
    return ret;
}

/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_tie_get, 4)
basic_ostream_wchar* __thiscall basic_ios_short_tie_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->stream;
}

/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGD@Z */
/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_widen, 8)
wchar_t __thiscall basic_ios_short_widen(basic_ios_wchar *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ctype_wchar_widen_ch(ctype_wchar_use_facet(IOS_LOCALE(this->strbuf)), ch);
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

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor, 20)
basic_ostream_char* __thiscall basic_ostream_char_ctor(basic_ostream_char *this,
        basic_streambuf_char *strbuf, bool isstd, bool init, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d %d)\n", this, strbuf, isstd, init, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_char_vtable;
    if(init)
        basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor_uninitialized, 12)
basic_ostream_char* __thiscall basic_ostream_char_ctor_uninitialized(basic_ostream_char *this,
        int uninitialized, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %d)\n", this, uninitialized);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_char_vtable;
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

DEFINE_THISCALL_WRAPPER(basic_ostream_char_vector_dtor, 8)
basic_ostream_char* __thiscall basic_ostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostream_char *this = basic_ostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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

/* ?flush@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?flush@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl flush_ostream_char(basic_ostream_char *ostream)
{
    return basic_ostream_char_flush(ostream);
}

/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_osfx, 4)
void __thiscall basic_ostream_char_osfx(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_char_flush(this);
}

static BOOL basic_ostream_char_sentry_create(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_char_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_char_sentry_destroy(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_char_osfx(ostr);
}

/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_opfx, 4)
bool __thiscall basic_ostream_char_opfx(basic_ostream_char *this)
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

    TRACE("(%p %Id %d)\n", this, off, way);

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && seek.state==0)
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
        if(seek.off==-1 && seek.pos==0 && seek.state==0)
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
        ret->off = -1;
        ret->pos = 0;
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

    TRACE("(%p %s %Id)\n", this, debugstr_a(str), count);

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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
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
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_uint64(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_bool, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_bool(basic_ostream_char *this, bool val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_bool(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?ends@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?ends@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl basic_ostream_char_ends(basic_ostream_char *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_char_put(ostr, 0);
    return ostr;
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

/* ??$?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_bstr(basic_ostream_char *ostr, const basic_string_char *str)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_char_sentry_create(ostr)) {
        size_t len = MSVCP_basic_string_char_length(str);
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
        size_t len = strlen(str);
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
static inline basic_ios_wchar* basic_ostream_short_get_basic_ios(basic_ostream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_ostream_short_to_basic_ios(basic_ostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostream_short_vbtable[1]);
}

static inline basic_ostream_wchar* basic_ostream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostream_wchar*)((char*)ptr-basic_ostream_short_vbtable[1]);
}

/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N1@Z */
/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_ctor, 20)
basic_ostream_wchar* __thiscall basic_ostream_short_ctor(basic_ostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool init, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_short_vbtable;
        base = basic_ostream_short_get_basic_ios(this);
        basic_ios_short_ctor(base);
    }else {
        base = basic_ostream_short_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_short_vtable;
    if(init)
        basic_ios_short_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_ctor_uninitialized, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_ctor_uninitialized(basic_ostream_wchar *this,
        int uninitialized, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d)\n", this, uninitialized);

    if(virt_init) {
        this->vbtable = basic_ostream_short_vbtable;
        base = basic_ostream_short_get_basic_ios(this);
        basic_ios_short_ctor(base);
    }else {
        base = basic_ostream_short_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_short_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_ostream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_dtor, 4)
void __thiscall basic_ostream_short_dtor(basic_ios_wchar *base)
{
    basic_ostream_wchar *this = basic_ostream_short_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_vbase_dtor, 4)
void __thiscall basic_ostream_short_vbase_dtor(basic_ostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_short_dtor(basic_ostream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_ostream_short_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(basic_ostream_short_vector_dtor, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostream_wchar *this = basic_ostream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?flush@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_flush, 4)
basic_ostream_wchar* __thiscall basic_ostream_short_flush(basic_ostream_wchar *this)
{
    /* this function is not matching C++ specification */
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(basic_ios_short_rdbuf_get(base) && ios_base_good(&base->base)
            && basic_streambuf_wchar_pubsync(basic_ios_short_rdbuf_get(base))==-1)
        basic_ios_short_setstate(base, IOSTATE_badbit);
    return this;
}

/* ?flush@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?flush@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl flush_ostream_short(basic_ostream_wchar *ostream)
{
    return basic_ostream_short_flush(ostream);
}

/* ?osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_osfx, 4)
void __thiscall basic_ostream_short_osfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_short_flush(this);
}

static BOOL basic_ostream_short_sentry_create(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_short_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_short_sentry_destroy(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_short_osfx(ostr);
}

/* ?opfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_opfx, 4)
bool __thiscall basic_ostream_short_opfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_short_flush(base->stream);
    return ios_base_good(&base->base);
}

/* ?put@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@G@Z */
/* ?put@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_put, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_put(basic_ostream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if(!basic_ostream_short_sentry_create(this)
            || basic_streambuf_wchar_sputc(base->strbuf, ch)==WEOF) {
        basic_ostream_short_sentry_destroy(this);
        basic_ios_short_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_short_sentry_destroy(this);
    return this;
}

/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_seekp, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_seekp(basic_ostream_wchar *this, streamoff off, int way)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p %Id %d)\n", this, off, way);

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_wchar_pubseekoff(basic_ios_short_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && seek.state==0)
            basic_ios_short_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_seekp_fpos, 28)
basic_ostream_wchar* __thiscall basic_ostream_short_seekp_fpos(basic_ostream_wchar *this, fpos_int pos)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(!ios_base_fail(&base->base)) {
        fpos_int seek;

        basic_streambuf_wchar_pubseekpos(basic_ios_short_rdbuf_get(base),
                &seek, pos, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && seek.state==0)
            basic_ios_short_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?tellp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_tellp, 8)
fpos_int* __thiscall basic_ostream_short_tellp(basic_ostream_wchar *this, fpos_int *ret)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar_pubseekoff(basic_ios_short_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_out);
    }else {
        ret->off = -1;
        ret->pos = 0;
        ret->state = 0;
    }
    return ret;
}

/* ?write@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@PBGH@Z */
/* ?write@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEBG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_write, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_write(basic_ostream_wchar *this, const wchar_t *str, streamsize count)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);

    TRACE("(%p %s %Id)\n", this, debugstr_w(str), count);

    if(!basic_ostream_short_sentry_create(this)
            || basic_streambuf_wchar_sputn(base->strbuf, str, count)!=count) {
        basic_ostream_short_sentry_destroy(this);
        basic_ios_short_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_short_sentry_destroy(this);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_short, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_short(basic_ostream_wchar *this, short val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base),
                (ios_base_flags_get(&base->base) & FMTFLAG_basefield & (FMTFLAG_oct | FMTFLAG_hex))
                ? (LONG)((unsigned short)val) : (LONG)val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ushort, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ushort(basic_ostream_wchar *this, unsigned short val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_int, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_int(basic_ostream_wchar *this, int val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_uint, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_uint(basic_ostream_wchar *this, unsigned int val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_float, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_float(basic_ostream_wchar *this, float val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %f)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_double, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_double(basic_ostream_wchar *this, double val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ldouble, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ldouble(basic_ostream_wchar *this, double val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ldouble(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_streambuf, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_streambuf(basic_ostream_wchar *this, basic_streambuf_wchar *val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_badbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
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
    basic_ostream_short_sentry_destroy(this);

    ios_base_width_set(&base->base, 0);
    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ptr, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ptr(basic_ostream_wchar *this, const void *val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ptr(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_int64, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_int64(basic_ostream_wchar *this, __int64 val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_int64(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_uint64, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_uint64(basic_ostream_wchar *this, unsigned __int64 val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_uint64(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_bool, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_bool(basic_ostream_wchar *this, bool val)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(this);
    const num_put *numput = num_put_short_use_facet(IOS_LOCALE(basic_ios_short_rdbuf_get(base)));
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_short_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_bool(numput, &dest, dest, &base->base, basic_ios_short_fill_get(base), val);
    }
    basic_ostream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ?ends@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?ends@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_ends(basic_ostream_wchar *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_short_put(ostr, 0);
    return ostr;
}

/* ?endl@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_endl(basic_ostream_wchar *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_short_put(ostr, '\n');
    basic_ostream_short_flush(ostr);
    return ostr;
}

/* ??$?6GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?6GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_print_bstr(basic_ostream_wchar *ostr, const basic_string_wchar *str)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_short_sentry_create(ostr)) {
        size_t len = MSVCP_basic_string_wchar_length(str);
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
    basic_ostream_short_sentry_destroy(ostr);

    basic_ios_short_setstate(base, state);
    return ostr;
}

/* ??$?6GU?$char_traits@G@std@@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@G@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@G@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_print_ch(basic_ostream_wchar *ostr, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", ostr, ch);

    if(basic_ostream_short_sentry_create(ostr)) {
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
    basic_ostream_short_sentry_destroy(ostr);

    basic_ios_short_setstate(base, state);
    return ostr;
}

/* ??$?6GU?$char_traits@G@std@@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@PBG@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@PEBG@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_print_str(basic_ostream_wchar *ostr, const wchar_t *str)
{
    basic_ios_wchar *base = basic_ostream_short_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %s)\n", ostr, debugstr_w(str));

    if(basic_ostream_short_sentry_create(ostr)) {
        size_t len = wcslen(str);
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
    basic_ostream_short_sentry_destroy(ostr);

    basic_ios_short_setstate(base, state);
    return ostr;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_func, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_func(basic_ostream_wchar *this,
        basic_ostream_wchar* (__cdecl *pfunc)(basic_ostream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@GU?$char_traits@G@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@GU?$char_traits@G@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_func_basic_ios, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_func_basic_ios(basic_ostream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_ostream_short_get_basic_ios(this));
    return this;
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_func_ios_base, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_func_ios_base(
        basic_ostream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_ostream_short_get_basic_ios(this)->base);
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

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor, 16)
basic_istream_char* __thiscall basic_istream_char_ctor(basic_istream_char *this, basic_streambuf_char *strbuf, bool isstd, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_char_vbtable;
        base = basic_istream_char_get_basic_ios(this);
        basic_ios_char_ctor(base);
    }else {
        base = basic_istream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_char_vtable;
    this->count = 0;
    basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor_uninitialized, 12)
basic_istream_char* __thiscall basic_istream_char_ctor_uninitialized(basic_istream_char *this, int uninitialized, bool virt_init)
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

    base->base.vtable = &basic_istream_char_vtable;
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

DEFINE_THISCALL_WRAPPER(basic_istream_char_vector_dtor, 8)
basic_istream_char* __thiscall basic_istream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istream_char *this = basic_istream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ipfx, 8)
bool __thiscall basic_istream_char_ipfx(basic_istream_char *this, bool noskip)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(ios_base_good(&base->base)) {
        if(basic_ios_char_tie_get(base))
            basic_ostream_char_flush(basic_ios_char_tie_get(base));

        if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
            basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
            const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
            int ch;

            for(ch = basic_streambuf_char_sgetc(strbuf); ;
                    ch = basic_streambuf_char_snextc(strbuf)) {
                if(ch==EOF || !ctype_char_is_ch(ctype, _SPACE|_BLANK, ch))
                    break;
            }
        }
    }

    if(!ios_base_good(&base->base)) {
        basic_ios_char_setstate(base, IOSTATE_failbit);
        return FALSE;
    }

    return TRUE;
}

/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_isfx, 4)
void __thiscall basic_istream_char_isfx(basic_istream_char *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_char_sentry_create(basic_istream_char *istr, bool noskip)
{
    return basic_istream_char_ipfx(istr, noskip);
}

static void basic_istream_char_sentry_destroy(basic_istream_char *istr)
{
}

/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QEBA_JXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBA_JXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBE_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_gcount, 4)
streamsize __thiscall basic_istream_char_gcount(const basic_istream_char *this)
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

    TRACE("(%p %p %Id %s)\n", this, str, count, debugstr_an(&delim, 1));

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

    TRACE("(%p %p %s)\n", this, strbuf, debugstr_an(&delim, 1));

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
    int ch = (unsigned char)delim;

    TRACE("(%p %p %Id %s)\n", this, str, count, debugstr_an(&delim, 1));

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        while(count > 1) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF || ch==(unsigned char)delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }

        if(ch == (unsigned char)delim)
            this->count++;
        else if(ch != EOF) {
            ch = basic_streambuf_char_sgetc(strbuf);

            if(ch == (unsigned char)delim) {
                basic_streambuf_char__Gninc(strbuf);
                this->count++;
            }
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=(unsigned char)delim && ch!=EOF) ? IOSTATE_failbit : IOSTATE_goodbit));
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
    int ch = (unsigned char)delim;
    unsigned int state;

    TRACE("(%p %Id %d)\n", this, count, delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        state = IOSTATE_goodbit;

        while(count > 0) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF) {
                state = IOSTATE_eofbit;
                break;
            }

            if(ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }else
        state = IOSTATE_failbit;
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?ws@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?ws@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_istream_char* __cdecl ws_basic_istream_char(basic_istream_char *istream)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    int ch = '\n';

    TRACE("(%p)\n", istream);

    if(basic_istream_char_sentry_create(istream, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(strbuf));

        for(ch = basic_streambuf_char_sgetc(strbuf); ctype_char_is_ch(ctype, _SPACE, ch);
                ch = basic_streambuf_char_snextc(strbuf)) {
            if(ch == EOF)
                break;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    if(ch == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit);
    return istream;
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

    if (ret == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit);

    return ret;
}

/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read, 12)
basic_istream_char* __thiscall basic_istream_char_read(basic_istream_char *this, char *str, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Id)\n", this, str, count);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        this->count = basic_streambuf_char_sgetn(strbuf, str, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_readsome, 12)
streamsize __thiscall basic_istream_char_readsome(basic_istream_char *this, char *str, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Id)\n", this, str, count);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_char_in_avail(basic_ios_char_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_char_read(this, str, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this->count;
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
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %p)\n", this, ret);

    if(ios_base_fail(&base->base)) {
        ret->off = -1;
        ret->pos = 0;
        ret->state = 0;
        return ret;
    }

    basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
            ret, 0, SEEKDIR_cur, OPENMODE_in);

    return ret;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg, 12)
basic_istream_char* __thiscall basic_istream_char_seekg(basic_istream_char *this, streamoff off, int dir)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %Id %d)\n", this, off, dir);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_int ret;

        basic_streambuf_char_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);
    }

    return this;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg_fpos, 28)
basic_istream_char* __thiscall basic_istream_char_seekg_fpos(basic_istream_char *this, fpos_int pos)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_int ret;

        basic_streambuf_char_pubseekpos(strbuf, &ret, pos, OPENMODE_in);
    }

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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, (LONG*)v);
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
basic_istream_char* __thiscall basic_istream_char_read_bool(basic_istream_char *this, bool *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
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
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_goodbit;
    int c = (unsigned char)delim;

    TRACE("(%p %p %s)\n", istream, str, debugstr_an(&delim, 1));

    MSVCP_basic_string_char_clear(str);
    if(basic_istream_char_sentry_create(istream, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        c = basic_streambuf_char_sgetc(strbuf);
        for(; c!=(unsigned char)delim && c!=EOF; c = basic_streambuf_char_snextc(strbuf))
            MSVCP_basic_string_char_append_ch(str, c);
        if(c==EOF) state |= IOSTATE_eofbit;
        else if(c==(unsigned char)delim) basic_streambuf_char_sbumpc(strbuf);

        if(!MSVCP_basic_string_char_length(str) && c!=(unsigned char)delim) state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(basic_istream_char_get_basic_ios(istream), state);
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
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
        size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_char_clear(str);

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_char_append_ch(str, c);
        }
    }
    basic_istream_char_sentry_destroy(istream);

    ios_base_width_set(&base->base, 0);
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
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
        size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    *str = 0;
    ios_base_width_set(&base->base, 0);
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
static inline basic_ios_wchar* basic_istream_short_get_basic_ios(basic_istream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_istream_short_to_basic_ios(basic_istream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istream_short_vbtable[1]);
}

static inline basic_istream_wchar* basic_istream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istream_wchar*)((char*)ptr-basic_istream_short_vbtable[1]);
}

/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ctor, 16)
basic_istream_wchar* __thiscall basic_istream_short_ctor(basic_istream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_short_vbtable;
        base = basic_istream_short_get_basic_ios(this);
        basic_ios_short_ctor(base);
    }else {
        base = basic_istream_short_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_short_vtable;
    this->count = 0;
    basic_ios_short_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ctor_uninitialized, 12)
basic_istream_wchar* __thiscall basic_istream_short_ctor_uninitialized(
        basic_istream_wchar *this, int uninitialized, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d %d)\n", this, uninitialized, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_short_vbtable;
        base = basic_istream_short_get_basic_ios(this);
        basic_ios_short_ctor(base);
    }else {
        base = basic_istream_short_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_short_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_istream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_dtor, 4)
void __thiscall basic_istream_short_dtor(basic_ios_wchar *base)
{
    basic_istream_wchar *this = basic_istream_short_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_istream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_vbase_dtor, 4)
void __thiscall basic_istream_short_vbase_dtor(basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_istream_short_dtor(basic_istream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(this));
}

DEFINE_THISCALL_WRAPPER(basic_istream_short_vector_dtor, 8)
basic_istream_wchar* __thiscall basic_istream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istream_wchar *this = basic_istream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ipfx, 8)
bool __thiscall basic_istream_short_ipfx(basic_istream_wchar *this, bool noskip)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(ios_base_good(&base->base)) {
        if(basic_ios_short_tie_get(base))
            basic_ostream_short_flush(basic_ios_short_tie_get(base));

        if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
            basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
            const ctype_wchar *ctype = ctype_wchar_use_facet(IOS_LOCALE(base->strbuf));
            int ch;

            for(ch = basic_streambuf_wchar_sgetc(strbuf); ;
                    ch = basic_streambuf_wchar_snextc(strbuf)) {
                if(ch==WEOF || !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, ch))
                    break;
            }
        }
    }

    if(!ios_base_good(&base->base)) {
        basic_ios_short_setstate(base, IOSTATE_failbit);
        return FALSE;
    }
    return TRUE;
}

/* ?isfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_isfx, 4)
void __thiscall basic_istream_short_isfx(basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_short_sentry_create(basic_istream_wchar *istr, bool noskip)
{
    return basic_istream_short_ipfx(istr, noskip);
}

static void basic_istream_short_sentry_destroy(basic_istream_wchar *istr)
{
}

/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QEBA_JXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBA_JXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBE_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_gcount, 4)
streamsize __thiscall basic_istream_short_gcount(const basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get, 4)
unsigned short __thiscall basic_istream_short_get(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(!basic_istream_short_sentry_create(this, TRUE)) {
        basic_istream_short_sentry_destroy(this);
        return WEOF;
    }

    ret = basic_streambuf_wchar_sbumpc(basic_ios_short_rdbuf_get(base));
    basic_istream_short_sentry_destroy(this);
    if(ret == WEOF)
        basic_ios_short_setstate(base, IOSTATE_eofbit|IOSTATE_failbit);
    else
        this->count++;

    return ret;
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAG@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get_ch, 8)
basic_istream_wchar* __thiscall basic_istream_short_get_ch(basic_istream_wchar *this, wchar_t *ch)
{
    unsigned short ret;

    TRACE("(%p %p)\n", this, ch);

    ret = basic_istream_short_get(this);
    if(ret != WEOF)
        *ch = (wchar_t)ret;
    return this;
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGHG@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_JG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get_str_delim, 16)
basic_istream_wchar* __thiscall basic_istream_short_get_str_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %Id %s)\n", this, str, count, debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf); count>1;
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch==WEOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get_str, 12)
basic_istream_wchar* __thiscall basic_istream_short_get_str(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_short_get_str_delim(this, str, count, '\n');
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAV?$basic_streambuf@GU?$char_traits@G@std@@@2@G@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@G@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get_streambuf_delim, 12)
basic_istream_wchar* __thiscall basic_istream_short_get_streambuf_delim(basic_istream_wchar *this, basic_streambuf_wchar *strbuf, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %s)\n", this, strbuf, debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf_read = basic_ios_short_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf_read); ;
                ch = basic_streambuf_wchar_snextc(strbuf_read)) {
            if(ch==WEOF || ch==delim)
                break;

            if(basic_streambuf_wchar_sputc(strbuf, ch) == WEOF)
                break;
            this->count++;
        }
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAV?$basic_streambuf@GU?$char_traits@G@std@@@2@@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_get_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_short_get_streambuf(basic_istream_wchar *this, basic_streambuf_wchar *strbuf)
{
    return basic_istream_short_get_streambuf_delim(this, strbuf, '\n');
}

/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGHG@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_JG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_getline_delim, 16)
basic_istream_wchar* __thiscall basic_istream_short_getline_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %Id %s)\n", this, str, count, debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

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
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=delim && ch!=WEOF) ? IOSTATE_failbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_getline, 12)
basic_istream_wchar* __thiscall basic_istream_short_getline(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_short_getline_delim(this, str, count, '\n');
}

/* ?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@HG@Z */
/* ?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ignore, 12)
basic_istream_wchar* __thiscall basic_istream_short_ignore(basic_istream_wchar *this, streamsize count, unsigned short delim)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    unsigned short ch = delim;
    unsigned int state;

    TRACE("(%p %Id %d)\n", this, count, delim);

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        state = IOSTATE_goodbit;

        while(count > 0) {
            ch = basic_streambuf_wchar_sbumpc(strbuf);

            if(ch==WEOF) {
                state = IOSTATE_eofbit;
                break;
            }

            if(ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }else
        state = IOSTATE_failbit;
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ?ws@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?ws@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_istream_wchar* __cdecl ws_basic_istream_short(basic_istream_wchar *istream)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(istream);
    unsigned short ch = '\n';

    TRACE("(%p)\n", istream);

    if(basic_istream_short_sentry_create(istream, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        const ctype_wchar *ctype = ctype_wchar_use_facet(IOS_LOCALE(strbuf));

        for(ch = basic_streambuf_wchar_sgetc(strbuf); ctype_wchar_is_ch(ctype, _SPACE, ch);
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch == WEOF)
                break;
        }
    }
    basic_istream_short_sentry_destroy(istream);

    if(ch == WEOF)
        basic_ios_short_setstate(base, IOSTATE_eofbit);
    return istream;
}

/* ?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_peek, 4)
unsigned short __thiscall basic_istream_short_peek(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    unsigned short ret = WEOF;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE))
        ret = basic_streambuf_wchar_sgetc(basic_ios_short_rdbuf_get(base));
    basic_istream_short_sentry_destroy(this);

    if (ret == WEOF)
        basic_ios_short_setstate(base, IOSTATE_eofbit);

    return ret;
}

/* ?read@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?read@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read, 12)
basic_istream_wchar* __thiscall basic_istream_short_read(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Id)\n", this, str, count);

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

        this->count = basic_streambuf_wchar_sgetn(strbuf, str, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ?readsome@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEHPAGH@Z */
/* ?readsome@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_readsome, 12)
streamsize __thiscall basic_istream_short_readsome(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Id)\n", this, str, count);

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_wchar_in_avail(basic_ios_short_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_short_read(this, str, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this->count;
}

/* ?putback@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@G@Z */
/* ?putback@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@G@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_putback, 8)
basic_istream_wchar* __thiscall basic_istream_short_putback(basic_istream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %c)\n", this, ch);

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sputbackc(strbuf, ch)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ?unget@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_unget, 4)
basic_istream_wchar* __thiscall basic_istream_short_unget(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sungetc(strbuf)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ?sync@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_sync, 4)
int __thiscall basic_istream_short_sync(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

    TRACE("(%p)\n", this);

    if(!strbuf)
        return -1;

    if(basic_istream_short_sentry_create(this, TRUE)) {
        if(basic_streambuf_wchar_pubsync(strbuf) != -1) {
            basic_istream_short_sentry_destroy(this);
            return 0;
        }
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, IOSTATE_badbit);
    return -1;
}

/* ?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_short_tellg, 8)
fpos_int* __thiscall basic_istream_short_tellg(basic_istream_wchar *this, fpos_int *ret)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);

    TRACE("(%p %p)\n", this, ret);

    if(ios_base_fail(&base->base)) {
        ret->off = -1;
        ret->pos = 0;
        ret->state = 0;
        return ret;
    }

    basic_streambuf_wchar_pubseekoff(basic_ios_short_rdbuf_get(base),
            ret, 0, SEEKDIR_cur, OPENMODE_in);

    return ret;
}

/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_seekg, 12)
basic_istream_wchar* __thiscall basic_istream_short_seekg(basic_istream_wchar *this, streamoff off, int dir)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);

    TRACE("(%p %Id %d)\n", this, off, dir);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        fpos_int ret;

        basic_streambuf_wchar_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);
    }

    return this;
}

/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_seekg_fpos, 28)
basic_istream_wchar* __thiscall basic_istream_short_seekg_fpos(basic_istream_wchar *this, fpos_int pos)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_int(&pos));

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        fpos_int ret;

        basic_streambuf_wchar_pubseekpos(strbuf, &ret, pos, OPENMODE_in);
    }

    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_short, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_short(basic_istream_wchar *this, short *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};
        LONG tmp;

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, &tmp);

        if(!(state&IOSTATE_failbit) && tmp==(LONG)((short)tmp))
            *v = tmp;
        else
            state |= IOSTATE_failbit;
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_int, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_int(basic_istream_wchar *this, int *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, (LONG*)v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_uint, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_uint(basic_istream_wchar *this, unsigned int *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_long, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_long(basic_istream_wchar *this, LONG *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ulong, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ulong(basic_istream_wchar *this, ULONG *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ulong(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_float, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_float(basic_istream_wchar *this, float *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_float(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_double, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_double(basic_istream_wchar *this, double *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_double(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ldouble, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ldouble(basic_istream_wchar *this, double *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ldouble(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ptr, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ptr(basic_istream_wchar *this, void **v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_void(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_int64, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_int64(basic_istream_wchar *this, __int64 *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_int64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_uint64, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_uint64(basic_istream_wchar *this, unsigned __int64 *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_bool, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_bool(basic_istream_wchar *this, bool *v)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    const num_get *numget = num_get_short_use_facet(IOS_LOCALE(base->strbuf));
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_bool(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state);
    return this;
}

/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z */
basic_istream_wchar* __cdecl basic_istream_short_getline_bstr_delim(
        basic_istream_wchar *istream, basic_string_wchar *str, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_goodbit;
    int c = delim;

    TRACE("(%p %p %s)\n", istream, str, debugstr_wn(&delim, 1));

    MSVCP_basic_string_wchar_clear(str);
    if(basic_istream_short_sentry_create(istream, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_short_rdbuf_get(base);

        c = basic_streambuf_wchar_sgetc(strbuf);
        for(; c!=delim && c!=WEOF; c = basic_streambuf_wchar_snextc(strbuf))
            MSVCP_basic_string_wchar_append_ch(str, c);
        if(c==delim) basic_streambuf_wchar_sbumpc(strbuf);
        else if(c==WEOF) state |= IOSTATE_eofbit;

        if(!MSVCP_basic_string_wchar_length(str) && c!=delim) state |= IOSTATE_failbit;
    }
    basic_istream_short_sentry_destroy(istream);

    basic_ios_short_setstate(basic_istream_short_get_basic_ios(istream), state);
    return istream;
}

/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_short_getline_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    return basic_istream_short_getline_bstr_delim(istream, str, '\n');
}

/* ??$?5GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?5GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_short_read_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(istream);
    const ctype_wchar *ctype = ctype_short_use_facet(IOS_LOCALE(base->strbuf));
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_short_sentry_create(istream, FALSE)) {
        size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_wchar_clear(str);

        for(c = basic_streambuf_wchar_sgetc(basic_ios_short_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_short_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_wchar_append_ch(str, c);
        }
    }
    basic_istream_short_sentry_destroy(istream);

    ios_base_width_set(&base->base, 0);
    basic_ios_short_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5GU?$char_traits@G@std@@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@PAG@Z */
/* ??$?5GU?$char_traits@G@std@@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@PEAG@Z */
basic_istream_wchar* __cdecl basic_istream_short_read_str(basic_istream_wchar *istream, wchar_t *str)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(istream);
    const ctype_wchar *ctype = ctype_short_use_facet(IOS_LOCALE(base->strbuf));
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_short_sentry_create(istream, FALSE)) {
        size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_wchar_sgetc(basic_ios_short_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_short_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }
    }
    basic_istream_short_sentry_destroy(istream);

    *str = 0;
    ios_base_width_set(&base->base, 0);
    basic_ios_short_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5GU?$char_traits@G@std@@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAG@Z */
/* ??$?5GU?$char_traits@G@std@@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAG@Z */
basic_istream_wchar* __cdecl basic_istream_short_read_ch(basic_istream_wchar *istream, wchar_t *ch)
{
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = 0;

    TRACE("(%p %p)\n", istream, ch);

    if(basic_istream_short_sentry_create(istream, FALSE)) {
        c = basic_streambuf_wchar_sbumpc(basic_ios_short_rdbuf_get(
                    basic_istream_short_get_basic_ios(istream)));
        if(c != WEOF) {
            state = IOSTATE_goodbit;
            *ch = c;
        }
    }
    basic_istream_short_sentry_destroy(istream);

    basic_ios_short_setstate(basic_istream_short_get_basic_ios(istream),
            state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_streambuf(
        basic_istream_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_ios_wchar *base = basic_istream_short_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, streambuf);

    if(basic_istream_short_sentry_create(this, FALSE)) {
        for(c = basic_streambuf_wchar_sgetc(basic_ios_short_rdbuf_get(base)); c!=WEOF;
                c = basic_streambuf_wchar_snextc(basic_ios_short_rdbuf_get(base))) {
            state = IOSTATE_goodbit;
            if(basic_streambuf_wchar_sputc(streambuf, c) == WEOF)
                break;
        }
    }
    basic_istream_short_sentry_destroy(this);

    basic_ios_short_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_func, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_func(basic_istream_wchar *this,
        basic_istream_wchar* (__cdecl *pfunc)(basic_istream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@GU?$char_traits@G@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@GU?$char_traits@G@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_func_basic_ios, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_func_basic_ios(basic_istream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_istream_short_get_basic_ios(this));
    return this;
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_func_ios_base, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_func_ios_base(
        basic_istream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_istream_short_get_basic_ios(this)->base);
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
basic_iostream_char* __thiscall basic_iostream_char_ctor(basic_iostream_char *this, basic_streambuf_char *strbuf, bool virt_init)
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

    basic_ios->base.vtable = &basic_iostream_char_vtable;

    basic_istream_char_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_char_ctor(&this->base2, NULL, FALSE, FALSE, FALSE);
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

DEFINE_THISCALL_WRAPPER(basic_iostream_char_vector_dtor, 8)
basic_iostream_char* __thiscall basic_iostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_iostream_char *this = basic_iostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_iostream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

static inline basic_ios_wchar* basic_iostream_short_to_basic_ios(basic_iostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_iostream_short_vbtable1[1]);
}

static inline basic_iostream_wchar* basic_iostream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_iostream_wchar*)((char*)ptr-basic_iostream_short_vbtable1[1]);
}

/* ??0?$basic_iostream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??0?$basic_iostream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_short_ctor, 12)
basic_iostream_wchar* __thiscall basic_iostream_short_ctor(basic_iostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, strbuf, virt_init);

    if(virt_init) {
        this->base1.vbtable = basic_iostream_short_vbtable1;
        this->base2.vbtable = basic_iostream_short_vbtable2;
        basic_ios = basic_istream_short_get_basic_ios(&this->base1);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base1);
    }

    basic_istream_short_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_short_ctor(&this->base2, NULL, FALSE, FALSE, FALSE);

    basic_ios->base.vtable = &basic_iostream_short_vtable;
    return this;
}

/* ??1?$basic_iostream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_short_dtor, 4)
void __thiscall basic_iostream_short_dtor(basic_ios_wchar *base)
{
    basic_iostream_wchar *this = basic_iostream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);
    basic_ostream_short_dtor(basic_ostream_short_to_basic_ios(&this->base2));
    basic_istream_short_dtor(basic_istream_short_to_basic_ios(&this->base1));
}

/* ??_D?$basic_iostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_short_vbase_dtor, 4)
void __thiscall basic_iostream_short_vbase_dtor(basic_iostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_iostream_short_dtor(basic_iostream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(&this->base1));
}

DEFINE_THISCALL_WRAPPER(basic_iostream_short_vector_dtor, 8)
basic_iostream_wchar* __thiscall basic_iostream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_iostream_wchar *this = basic_iostream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_iostream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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
basic_ofstream_char* __thiscall basic_ofstream_char_ctor(basic_ofstream_char *this, bool virt_init)
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
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_copy_ctor, 12)
basic_ofstream_char* __thiscall basic_ofstream_char_copy_ctor(basic_ofstream_char *this,
        basic_ofstream_char *copy, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_name, 16)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_name(basic_ofstream_char *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_ofstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode|OPENMODE_out)) {
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

DEFINE_THISCALL_WRAPPER(basic_ofstream_char_vector_dtor, 8)
basic_ofstream_char* __thiscall basic_ofstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ofstream_char *this = basic_ofstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ofstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ofstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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
bool __thiscall basic_ofstream_char_is_open(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open, 12)
void __thiscall basic_ofstream_char_open(basic_ofstream_char *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode|OPENMODE_out)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDF@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_old, 12)
void __thiscall basic_ofstream_char_open_old(basic_ofstream_char *this,
        const char *name, short mode)
{
    basic_ofstream_char_open(this, name, mode);
}

/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ofstream_char_rdbuf(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_ofstream_short_to_basic_ios(basic_ofstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ofstream_short_vbtable[1]);
}

static inline basic_ofstream_wchar* basic_ofstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ofstream_wchar*)((char*)ptr-basic_ofstream_short_vbtable[1]);
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor, 8)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor(basic_ofstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_short_vbtable;
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
    }

    basic_filebuf_short_ctor(&this->filebuf);
    basic_ostream_short_ctor(&this->base, &this->filebuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_copy_ctor, 12)
basic_ofstream_wchar* __thiscall basic_ofstream_short_copy_ctor(basic_ofstream_wchar *this,
        basic_ofstream_wchar *copy, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_short_vbtable;
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
    }

    basic_filebuf_short_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_ostream_short_ctor(&this->base, &this->filebuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor_name, 16)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor_name(basic_ofstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_ofstream_short_ctor(this, virt_init);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode|OPENMODE_out)) {
        basic_ios_wchar *basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ofstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ofstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_dtor, 4)
void __thiscall basic_ofstream_short_dtor(basic_ios_wchar *base)
{
    basic_ofstream_wchar *this = basic_ofstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_short_dtor(basic_ostream_short_to_basic_ios(&this->base));
    basic_filebuf_short_dtor(&this->filebuf);
}

/* ??_D?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_vbase_dtor, 4)
void __thiscall basic_ofstream_short_vbase_dtor(basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_ofstream_short_dtor(basic_ofstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_ostream_short_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(basic_ofstream_short_vector_dtor, 8)
basic_ofstream_wchar* __thiscall basic_ofstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ofstream_wchar *this = basic_ofstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ofstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ofstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_close, 4)
void __thiscall basic_ofstream_short_close(basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_short_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_is_open, 4)
bool __thiscall basic_ofstream_short_is_open(const basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_short_is_open(&this->filebuf);
}

/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_open, 12)
void __thiscall basic_ofstream_short_open(basic_ofstream_wchar *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode|OPENMODE_out)) {
        basic_ios_wchar *basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBDF@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_open_old, 12)
void __thiscall basic_ofstream_short_open_old(basic_ofstream_wchar *this,
        const char *name, int mode)
{
    basic_ofstream_short_open(this, name, mode);
}

/* ?rdbuf@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_ofstream_short_rdbuf(const basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
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
basic_ifstream_char* __thiscall basic_ifstream_char_ctor(basic_ifstream_char *this, bool virt_init)
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
    basic_ios->base.vtable = &basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_copy_ctor, 12)
basic_ifstream_char* __thiscall basic_ifstream_char_copy_ctor(basic_ifstream_char *this,
        const basic_ifstream_char *copy, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_istream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name, 16)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name(basic_ifstream_char *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_ifstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode|OPENMODE_in)) {
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

DEFINE_THISCALL_WRAPPER(basic_ifstream_char_vector_dtor, 8)
basic_ifstream_char* __thiscall basic_ifstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ifstream_char *this = basic_ifstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ifstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ifstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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
bool __thiscall basic_ifstream_char_is_open(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open, 12)
void __thiscall basic_ifstream_char_open(basic_ifstream_char *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode|OPENMODE_in)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDF@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_old, 12)
void __thiscall basic_ifstream_char_open_old(basic_ifstream_char *this,
        const char *name, short mode)
{
    basic_ifstream_char_open(this, name, mode);
}

/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ifstream_char_rdbuf(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_ifstream_short_to_basic_ios(basic_ifstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ifstream_short_vbtable[1]);
}

static inline basic_ifstream_wchar* basic_ifstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ifstream_wchar*)((char*)ptr-basic_ifstream_short_vbtable[1]);
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor, 8)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor(basic_ifstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_short_vbtable;
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
    }

    basic_filebuf_short_ctor(&this->filebuf);
    basic_istream_short_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_copy_ctor, 12)
basic_ifstream_wchar* __thiscall basic_ifstream_short_copy_ctor(basic_ifstream_wchar *this,
        basic_ifstream_wchar *copy, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_short_vbtable;
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
    }

    basic_filebuf_short_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_istream_short_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor_name, 16)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor_name(basic_ifstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_ifstream_short_ctor(this, virt_init);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode|OPENMODE_in)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ifstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ifstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_dtor, 4)
void __thiscall basic_ifstream_short_dtor(basic_ios_wchar *base)
{
    basic_ifstream_wchar *this = basic_ifstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_short_dtor(basic_istream_short_to_basic_ios(&this->base));
    basic_filebuf_short_dtor(&this->filebuf);
}

/* ??_D?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_vbase_dtor, 4)
void __thiscall basic_ifstream_short_vbase_dtor(basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_ifstream_short_dtor(basic_ifstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(basic_ifstream_short_vector_dtor, 8)
basic_ifstream_wchar* __thiscall basic_ifstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ifstream_wchar *this = basic_ifstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ifstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ifstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_close, 4)
void __thiscall basic_ifstream_short_close(basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_short_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_is_open, 4)
bool __thiscall basic_ifstream_short_is_open(const basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_short_is_open(&this->filebuf);
}

/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_open, 12)
void __thiscall basic_ifstream_short_open(basic_ifstream_wchar *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode|OPENMODE_in)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBDF@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_open_old, 12)
void __thiscall basic_ifstream_short_open_old(basic_ifstream_wchar *this,
        const char *name, short mode)
{
    basic_ifstream_short_open(this, name, mode);
}

/* ?rdbuf@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_ifstream_short_rdbuf(const basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
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
basic_fstream_char* __thiscall basic_fstream_char_ctor(basic_fstream_char *this, bool virt_init)
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
    basic_ios->base.vtable = &basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_copy_ctor, 12)
basic_fstream_char* __thiscall basic_fstream_char_copy_ctor(basic_fstream_char *this,
        basic_fstream_char *copy, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_char_vbtable1;
        this->base.base2.vbtable = basic_fstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_iostream_char_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name, 16)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name(basic_fstream_char *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_fstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode)) {
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

DEFINE_THISCALL_WRAPPER(basic_fstream_char_vector_dtor, 8)
basic_fstream_char* __thiscall basic_fstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_fstream_char *this = basic_fstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_fstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_fstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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
bool __thiscall basic_fstream_char_is_open(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open, 12)
void __thiscall basic_fstream_char_open(basic_fstream_char *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_char_open_mode(&this->filebuf, name, mode)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDF@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_old, 12)
void __thiscall basic_fstream_char_open_old(basic_fstream_char *this,
        const char *name, int mode)
{
    basic_fstream_char_open(this, name, mode);
}

/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_fstream_char_rdbuf(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_fstream_short_to_basic_ios(basic_fstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_fstream_short_vbtable1[1]);
}

static inline basic_fstream_wchar* basic_fstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_fstream_wchar*)((char*)ptr-basic_fstream_short_vbtable1[1]);
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor, 8)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor(basic_fstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_short_vbtable1;
        this->base.base2.vbtable = basic_fstream_short_vbtable2;
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_short_ctor(&this->filebuf);
    basic_iostream_short_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@ABV01@@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_copy_ctor, 12)
basic_fstream_wchar* __thiscall basic_fstream_short_copy_ctor(basic_fstream_wchar *this,
        basic_fstream_wchar *copy, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_short_vbtable1;
        this->base.base2.vbtable = basic_fstream_short_vbtable2;
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_short_ctor_file(&this->filebuf, copy->filebuf.file);
    basic_iostream_short_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor_name, 16)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor_name(basic_fstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, virt_init);

    basic_fstream_short_ctor(this, virt_init);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_fstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_fstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_dtor, 4)
void __thiscall basic_fstream_short_dtor(basic_ios_wchar *base)
{
    basic_fstream_wchar *this = basic_fstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_short_dtor(basic_iostream_short_to_basic_ios(&this->base));
    basic_filebuf_short_dtor(&this->filebuf);
}

/* ??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_vbase_dtor, 4)
void __thiscall basic_fstream_short_vbase_dtor(basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_fstream_short_dtor(basic_fstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(basic_fstream_short_vector_dtor, 8)
basic_fstream_wchar* __thiscall basic_fstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_fstream_wchar *this = basic_fstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_fstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_fstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_close, 4)
void __thiscall basic_fstream_short_close(basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_short_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_is_open, 4)
bool __thiscall basic_fstream_short_is_open(const basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_short_is_open(&this->filebuf);
}

/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPBDH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_open, 12)
void __thiscall basic_fstream_short_open(basic_fstream_wchar *this,
        const char *name, int mode)
{
    TRACE("(%p %s %d)\n", this, name, mode);

    if(!basic_filebuf_short_open_mode(&this->filebuf, name, mode)) {
        basic_ios_wchar *basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDF@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_open_old, 12)
void __thiscall basic_fstream_short_open_old(basic_fstream_wchar *this,
        const char *name, int mode)
{
    basic_fstream_short_open(this, name, mode);
}

/* ?rdbuf@?$basic_fstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_fstream_short_rdbuf(const basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
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
        const basic_string_char *str, int mode, bool virt_init)
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
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_char_vtable;
    return this;
}

/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor_mode, 12)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor_mode(
        basic_ostringstream_char *this, int mode, bool virt_init)
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
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_char_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor, 4)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor(
        basic_ostringstream_char *this)
{
    return basic_ostringstream_char_ctor_mode(this, 0, TRUE);
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

DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_vector_dtor, 8)
basic_ostringstream_char* __thiscall basic_ostringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostringstream_char *this = basic_ostringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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

static inline basic_ios_wchar* basic_ostringstream_short_to_basic_ios(basic_ostringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostringstream_short_vbtable[1]);
}

static inline basic_ostringstream_wchar* basic_ostringstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostringstream_wchar*)((char*)ptr-basic_ostringstream_short_vbtable[1]);
}

/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor_str, 16)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor_str(basic_ostringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_short_vbtable;
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
    }

    basic_stringbuf_short_ctor_str(&this->strbuf, str, mode|OPENMODE_out);
    basic_ostream_short_ctor(&this->base, &this->strbuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_short_vtable;
    return this;
}

/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor_mode, 12)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor_mode(
        basic_ostringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_short_vbtable;
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_short_get_basic_ios(&this->base);
    }

    basic_stringbuf_short_ctor_mode(&this->strbuf, mode|OPENMODE_out);
    basic_ostream_short_ctor(&this->base, &this->strbuf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_short_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor, 4)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor(
        basic_ostringstream_wchar *this)
{
    return basic_ostringstream_short_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_dtor, 4)
void __thiscall basic_ostringstream_short_dtor(basic_ios_wchar *base)
{
    basic_ostringstream_wchar *this = basic_ostringstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_short_dtor(&this->strbuf);
    basic_ostream_short_dtor(basic_ostream_short_to_basic_ios(&this->base));
}

/* ??_D?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_vbase_dtor, 4)
void __thiscall basic_ostringstream_short_vbase_dtor(basic_ostringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_ostringstream_short_dtor(basic_ostringstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_ostream_short_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_vector_dtor, 8)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostringstream_wchar *this = basic_ostringstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostringstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_ostringstream_short_rdbuf(const basic_ostringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_str_set, 8)
void __thiscall basic_ostringstream_short_str_set(basic_ostringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_short_str_set(&this->strbuf, str);
}

/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_str_get, 8)
basic_string_wchar* __thiscall basic_ostringstream_short_str_get(const basic_ostringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_short_str_get(&this->strbuf, ret);
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
        const basic_string_char *str, int mode, bool virt_init)
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
    basic_ios->base.vtable = &basic_istringstream_char_vtable;
    return this;
}

/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor_mode, 12)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor_mode(
        basic_istringstream_char *this, int mode, bool virt_init)
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
    basic_ios->base.vtable = &basic_istringstream_char_vtable;
    return this;
}

/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor, 4)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor(
        basic_istringstream_char *this)
{
    return basic_istringstream_char_ctor_mode(this, 0, TRUE);
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

DEFINE_THISCALL_WRAPPER(basic_istringstream_char_vector_dtor, 8)
basic_istringstream_char* __thiscall basic_istringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istringstream_char *this = basic_istringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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

static inline basic_ios_wchar* basic_istringstream_short_to_basic_ios(basic_istringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istringstream_short_vbtable[1]);
}

static inline basic_istringstream_wchar* basic_istringstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istringstream_wchar*)((char*)ptr-basic_istringstream_short_vbtable[1]);
}

/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor_str, 16)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor_str(basic_istringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_short_vbtable;
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
    }

    basic_stringbuf_short_ctor_str(&this->strbuf, str, mode|OPENMODE_in);
    basic_istream_short_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_short_vtable;
    return this;
}

/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor_mode, 12)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor_mode(
        basic_istringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_short_vbtable;
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base);
    }

    basic_stringbuf_short_ctor_mode(&this->strbuf, mode|OPENMODE_in);
    basic_istream_short_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_short_vtable;
    return this;
}

/* ??_F?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor, 4)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor(
        basic_istringstream_wchar *this)
{
    return basic_istringstream_short_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_dtor, 4)
void __thiscall basic_istringstream_short_dtor(basic_ios_wchar *base)
{
    basic_istringstream_wchar *this = basic_istringstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_short_dtor(&this->strbuf);
    basic_istream_short_dtor(basic_istream_short_to_basic_ios(&this->base));
}

/* ??_D?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_vbase_dtor, 4)
void __thiscall basic_istringstream_short_vbase_dtor(basic_istringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_istringstream_short_dtor(basic_istringstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(basic_istringstream_short_vector_dtor, 8)
basic_istringstream_wchar* __thiscall basic_istringstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istringstream_wchar *this = basic_istringstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istringstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_istringstream_short_rdbuf(const basic_istringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_str_set, 8)
void __thiscall basic_istringstream_short_str_set(basic_istringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_short_str_set(&this->strbuf, str);
}

/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_str_get, 8)
basic_string_wchar* __thiscall basic_istringstream_short_str_get(const basic_istringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_short_str_get(&this->strbuf, ret);
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
        const basic_string_char *str, int mode, bool virt_init)
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
    basic_ios->base.vtable = &basic_stringstream_char_vtable;
    return this;
}

/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor_mode, 12)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor_mode(
        basic_stringstream_char *this, int mode, bool virt_init)
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
    basic_ios->base.vtable = &basic_stringstream_char_vtable;
    return this;
}

/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor, 4)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor(
        basic_stringstream_char *this)
{
    return basic_stringstream_char_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, TRUE);
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

DEFINE_THISCALL_WRAPPER(basic_stringstream_char_vector_dtor, 8)
basic_stringstream_char* __thiscall basic_stringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_stringstream_char *this = basic_stringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_stringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
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

static inline basic_ios_wchar* basic_stringstream_short_to_basic_ios(basic_stringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_stringstream_short_vbtable1[1]);
}

static inline basic_stringstream_wchar* basic_stringstream_short_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_stringstream_wchar*)((char*)ptr-basic_stringstream_short_vbtable1[1]);
}

/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor_str, 16)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor_str(basic_stringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_short_vbtable1;
        this->base.base2.vbtable = basic_stringstream_short_vbtable2;
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_short_ctor_str(&this->strbuf, str, mode);
    basic_iostream_short_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_short_vtable;
    return this;
}

/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor_mode, 12)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor_mode(
        basic_stringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_short_vbtable1;
        this->base.base2.vbtable = basic_stringstream_short_vbtable2;
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
        basic_ios_short_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_short_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_short_ctor_mode(&this->strbuf, mode);
    basic_iostream_short_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_short_vtable;
    return this;
}

/* ??_F?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor, 4)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor(
        basic_stringstream_wchar *this)
{
    return basic_stringstream_short_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, TRUE);
}

/* ??1?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_dtor, 4)
void __thiscall basic_stringstream_short_dtor(basic_ios_wchar *base)
{
    basic_stringstream_wchar *this = basic_stringstream_short_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_short_dtor(basic_iostream_short_to_basic_ios(&this->base));
    basic_stringbuf_short_dtor(&this->strbuf);
}

/* ??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_vbase_dtor, 4)
void __thiscall basic_stringstream_short_vbase_dtor(basic_stringstream_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_stringstream_short_dtor(basic_stringstream_short_to_basic_ios(this));
    basic_ios_short_dtor(basic_istream_short_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(basic_stringstream_short_vector_dtor, 8)
basic_stringstream_wchar* __thiscall basic_stringstream_short_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_stringstream_wchar *this = basic_stringstream_short_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_short_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_stringstream_short_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_stringstream_short_rdbuf(const basic_stringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_str_set, 8)
void __thiscall basic_stringstream_short_str_set(basic_stringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_short_str_set(&this->strbuf, str);
}

/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_str_get, 8)
basic_string_wchar* __thiscall basic_stringstream_short_str_get(const basic_stringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_short_str_get(&this->strbuf, ret);
}

/* ?_Init@strstreambuf@std@@IAEXHPAD0H@Z */
/* ?_Init@strstreambuf@std@@IEAAX_JPEAD1H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf__Init, 20)
void __thiscall strstreambuf__Init(strstreambuf *this, streamsize len, char *g, char *p, int mode)
{
    TRACE("(%p %Id %p %p %d)\n", this, len, g, p, mode);

    this->minsize = 32;
    this->endsave = NULL;
    this->strmode = mode;
    this->palloc = NULL;
    this->pfree = NULL;

    if(!g) {
        this->strmode |= STRSTATE_Dynamic;
        if(len > this->minsize)
            this->minsize = len;
        this->seekhigh = NULL;
        return;
    }

    if(len < 0)
        len = INT_MAX;
    else if(!len)
        len = strlen(g);

    this->seekhigh = g+len;
    basic_streambuf_char_setg(&this->base, g, g, p ? p : this->seekhigh);
    if(p)
        basic_streambuf_char_setp(&this->base, p, this->seekhigh);
}

static strstreambuf* strstreambuf_ctor_get_put(strstreambuf *this, char *g, streamsize len, char *p)
{
    TRACE("(%p %p %Id %p)\n", this, g, len, p);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &strstreambuf_vtable;

    strstreambuf__Init(this, len, g, p, 0);
    return this;
}

/* ?_Tidy@strstreambuf@std@@IAEXXZ */
/* ?_Tidy@strstreambuf@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf__Tidy, 4)
void __thiscall strstreambuf__Tidy(strstreambuf *this)
{
    TRACE("(%p)\n", this);

    if((this->strmode & STRSTATE_Allocated) && !(this->strmode & STRSTATE_Frozen)) {
        if(this->pfree)
            this->pfree(basic_streambuf_char_eback(&this->base));
        else
            operator_delete(basic_streambuf_char_eback(&this->base));
    }

    this->endsave = NULL;
    this->seekhigh = NULL;
    this->strmode &= ~(STRSTATE_Allocated | STRSTATE_Frozen);
    basic_streambuf_char_setg(&this->base, NULL, NULL, NULL);
    basic_streambuf_char_setp(&this->base, NULL, NULL);
}

/* ??1strstreambuf@std@@UAE@XZ */
/* ??1strstreambuf@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_dtor, 4)
void __thiscall strstreambuf_dtor(strstreambuf *this)
{
    TRACE("(%p)\n", this);

    strstreambuf__Tidy(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(strstreambuf_vector_dtor, 8)
strstreambuf* __thiscall strstreambuf_vector_dtor(strstreambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            strstreambuf_dtor(this+i);
        operator_delete(ptr);
    } else {
        strstreambuf_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?freeze@strstreambuf@std@@QAEX_N@Z */
/* ?freeze@strstreambuf@std@@QEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_freeze, 8)
void __thiscall strstreambuf_freeze(strstreambuf *this, bool freeze)
{
    TRACE("(%p %d)\n", this, freeze);

    if(!freeze == !(this->strmode & STRSTATE_Frozen))
        return;

    if(freeze) {
        this->strmode |= STRSTATE_Frozen;
        this->endsave = basic_streambuf_char_epptr(&this->base);
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                basic_streambuf_char_pptr(&this->base), basic_streambuf_char_eback(&this->base));
    }else {
        this->strmode &= ~STRSTATE_Frozen;
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                basic_streambuf_char_pptr(&this->base), this->endsave);
    }
}

/* ?overflow@strstreambuf@std@@MAEHH@Z */
/* ?overflow@strstreambuf@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_overflow, 8)
int __thiscall strstreambuf_overflow(strstreambuf *this, int c)
{
    size_t old_size, size;
    char *ptr, *buf;

    TRACE("(%p %d)\n", this, c);

    if(c == EOF)
        return !EOF;

    if(this->strmode & STRSTATE_Frozen)
        return EOF;

    ptr = basic_streambuf_char_pptr(&this->base);
    if(ptr && ptr<basic_streambuf_char_epptr(&this->base))
        return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = c);

    if(!(this->strmode & STRSTATE_Dynamic) || (this->strmode & STRSTATE_Constant))
        return EOF;

    ptr = basic_streambuf_char_eback(&this->base);
    old_size = ptr ? basic_streambuf_char_epptr(&this->base) - ptr : 0;

    size = old_size + old_size/2;
    if(size < this->minsize)
        size = this->minsize;

    if(this->palloc)
        buf = this->palloc(size);
    else
        buf = operator_new(size);
    if(!buf)
        return EOF;

    memcpy(buf, ptr, old_size);
    if(this->strmode & STRSTATE_Allocated) {
        if(this->pfree)
            this->pfree(ptr);
        else
            operator_delete(ptr);
    }

    this->strmode |= STRSTATE_Allocated;
    if(!old_size) {
        this->seekhigh = buf;
        basic_streambuf_char_setp(&this->base, buf, buf+size);
        basic_streambuf_char_setg(&this->base, buf, buf, buf);
    }else {
        this->seekhigh = this->seekhigh-ptr+buf;
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base)-ptr+buf,
                basic_streambuf_char_pptr(&this->base)-ptr+buf, buf+size);
        basic_streambuf_char_setg(&this->base, buf, basic_streambuf_char_gptr(&this->base)-ptr+buf,
                basic_streambuf_char_pptr(&this->base));
    }

    return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = c);
}

/* ?pbackfail@strstreambuf@std@@MAEHH@Z */
/* ?pbackfail@strstreambuf@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_pbackfail, 8)
int __thiscall strstreambuf_pbackfail(strstreambuf *this, int c)
{
    char *ptr = basic_streambuf_char_gptr(&this->base);

    TRACE("(%p %d)\n", this, c);

    if(ptr<=basic_streambuf_char_eback(&this->base)
            || ((this->strmode & STRSTATE_Constant) && c!=ptr[-1]))
        return EOF;

    basic_streambuf_char_gbump(&this->base, -1);
    if(c == EOF)
        return !EOF;
    if(this->strmode & STRSTATE_Constant)
        return (unsigned char)c;

    return (unsigned char)(ptr[0] = c);
}

/* ?seekoff@strstreambuf@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@strstreambuf@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@strstreambuf@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@strstreambuf@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_seekoff, 20)
fpos_int* __thiscall strstreambuf_seekoff(strstreambuf *this, fpos_int *ret, streamoff off, int way, int mode)
{
    char *eback = basic_streambuf_char_eback(&this->base);
    char *pptr = basic_streambuf_char_pptr(&this->base);
    char *gptr = basic_streambuf_char_gptr(&this->base);

    TRACE("(%p %p %Id %d %d)\n", this, ret, off, way, mode);

    ret->off = 0;
    ret->state = 0;

    if(pptr > this->seekhigh)
        this->seekhigh = pptr;

    if((mode & OPENMODE_in) && gptr) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += gptr-eback;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-eback;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-eback) {
            off = -1;
        }else {
            basic_streambuf_char_gbump(&this->base, eback-gptr+off);
            if((mode & OPENMODE_out) && pptr) {
                basic_streambuf_char_setp_next(&this->base, eback,
                        gptr, basic_streambuf_char_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && pptr) {
        if(way == SEEKDIR_cur)
            off += pptr-eback;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-eback;
        else if(way != SEEKDIR_beg)
            off = -1;

         if(off<0 || off>this->seekhigh-eback)
             off = -1;
         else
             basic_streambuf_char_pbump(&this->base, eback-pptr+off);
    }else {
        off = -1;
    }

    ret->pos = off;
    return ret;
}

/* ?seekpos@strstreambuf@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@strstreambuf@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_seekpos, 36)
fpos_int* __thiscall strstreambuf_seekpos(strstreambuf *this, fpos_int *ret, fpos_int pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_int(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && pos.state==0) {
        *ret = pos;
        return ret;
    }

    return strstreambuf_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
}

/* ?underflow@strstreambuf@std@@MAEHXZ */
/* ?underflow@strstreambuf@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_underflow, 4)
int __thiscall strstreambuf_underflow(strstreambuf *this)
{
    char *gptr = basic_streambuf_char_gptr(&this->base);
    char *pptr;

    TRACE("(%p)\n", this);

    if(!gptr)
        return EOF;

    if(gptr < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)(*gptr);

    pptr = basic_streambuf_char_gptr(&this->base);
    if(pptr > this->seekhigh)
        this->seekhigh = pptr;

    if(this->seekhigh <= gptr)
        return EOF;

    basic_streambuf_char_setg(&this->base, basic_streambuf_char_eback(&this->base),
            gptr, this->seekhigh);
    return (unsigned char)(*gptr);
}

static inline basic_ios_char* ostrstream_to_basic_ios(ostrstream *ptr)
{
    return (basic_ios_char*)((char*)ptr+ostrstream_vbtable[1]);
}

static inline ostrstream* ostrstream_from_basic_ios(basic_ios_char *ptr)
{
    return (ostrstream*)((char*)ptr-ostrstream_vbtable[1]);
}

/* ??0ostrstream@std@@QAE@PADHH@Z */
DEFINE_THISCALL_WRAPPER(ostrstream_ctor, 20)
ostrstream* __thiscall ostrstream_ctor(ostrstream *this, char *buf, streamsize size, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %Id %d %d)\n", this, buf, size, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = ostrstream_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    strstreambuf_ctor_get_put(&this->buf, buf, size,
            buf && (mode & OPENMODE_app) ? buf+strlen(buf) : buf);
    basic_ostream_char_ctor(&this->base, &this->buf.base, FALSE, TRUE, FALSE);
    basic_ios->base.vtable = &ostrstream_vtable;
    return this;
}

/* ??1ostrstream@std@@UAE@XZ */
/* ??1ostrstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostrstream_dtor, 4)
void __thiscall ostrstream_dtor(basic_ios_char *base)
{
    ostrstream *this = ostrstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static void ostrstream_vbase_dtor(ostrstream *this)
{
    TRACE("(%p)\n", this);

    ostrstream_dtor(ostrstream_to_basic_ios(this));
    basic_ios_char_dtor(basic_ostream_char_get_basic_ios(&this->base));
}

DEFINE_THISCALL_WRAPPER(ostrstream_vector_dtor, 8)
ostrstream* __thiscall ostrstream_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    ostrstream *this = ostrstream_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ostrstream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        ostrstream_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

static inline istrstream* istrstream_from_basic_ios(basic_ios_char *ptr)
{
    return (istrstream*)((char*)ptr-istrstream_vbtable[1]);
}

/* ??1istrstream@std@@UAE@XZ */
/* ??1istrstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(istrstream_dtor, 4)
void __thiscall istrstream_dtor(basic_ios_char *base)
{
    istrstream *this = istrstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static inline basic_ios_char* strstream_to_basic_ios(strstream *ptr)
{
    return (basic_ios_char*)((char*)ptr+strstream_vbtable1[1]);
}

static inline strstream* strstream_from_basic_ios(basic_ios_char *ptr)
{
    return (strstream*)((char*)ptr-strstream_vbtable1[1]);
}

/* ??$?6MDU?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$complex@M@0@@Z  */
/* ??$?6MDU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@M@0@@Z  */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_float(basic_ostream_char *ostr, const complex_float *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_float(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_float(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ??$?6NDU?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$complex@N@0@@Z */
/* ??$?6NDU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@N@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_double(basic_ostream_char *ostr, const complex_double *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_double(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_double(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ??$?6odu?$char_traits@d@std@@@std@@yaaav?$basic_ostream@du?$char_traits@d@std@@@0@aav10@abv?$complex@o@0@@Z */
/* ??$?6ODU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@O@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_ldouble(basic_ostream_char *ostr, const complex_double *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_ldouble(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_ldouble(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ??0strstream@std@@QAE@PADHH@Z */
/* ??0strstream@std@@QEAA@PEAD_JH@Z */
DEFINE_THISCALL_WRAPPER(strstream_ctor, 20)
strstream* __thiscall strstream_ctor(strstream *this, char *buf, streamsize size, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %Id %d %d)\n", this, buf, size, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = strstream_vbtable1;
        this->base.base2.vbtable = strstream_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    strstreambuf_ctor_get_put(&this->buf, buf, size,
            buf && (mode & OPENMODE_app) ? buf+strlen(buf) : buf);
    basic_iostream_char_ctor(&this->base, &this->buf.base, FALSE);
    basic_ios->base.vtable = &strstream_vtable;
    return this;
}

/* ??1strstream@std@@UAE@XZ */
/* ??1strstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstream_dtor, 4)
void __thiscall strstream_dtor(basic_ios_char *base)
{
    strstream *this = strstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static void strstream_vbase_dtor(strstream *this)
{
    TRACE("(%p)\n", this);

    strstream_dtor(strstream_to_basic_ios(this));
    basic_ios_char_dtor(basic_istream_char_get_basic_ios(&this->base.base1));
}

DEFINE_THISCALL_WRAPPER(strstream_vector_dtor, 8)
strstream* __thiscall strstream_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    strstream *this = strstream_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            strstream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        strstream_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

static void __cdecl setprecision_func(ios_base *base, streamsize prec)
{
    ios_base_precision_set(base, prec);
}

/* ?setprecision@std@@YA?AU?$_Smanip@H@1@H@Z */
/* ?setprecision@std@@YA?AU?$_Smanip@_J@1@_J@Z */
manip_streamsize* __cdecl setprecision(manip_streamsize *ret, streamsize prec)
{
    TRACE("(%p %Id)\n", ret, prec);

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
    TRACE("(%p %Id)\n", ret, width);

    ret->pfunc = setw_func;
    ret->arg = width;
    return ret;
}

static void __cdecl resetioflags_func(ios_base *base, int mask)
{
    ios_base_setf_mask(base, 0, mask);
}

/* ?resetiosflags@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl resetiosflags(manip_int *ret, int mask)
{
    TRACE("(%p %d)\n", ret, mask);

    ret->pfunc = resetioflags_func;
    ret->arg = mask;
    return ret;
}

static void __cdecl setiosflags_func(ios_base *base, int mask)
{
    ios_base_setf_mask(base, FMTFLAG_mask, mask);
}

/* ?setiosflags@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl setiosflags(manip_int *ret, int mask)
{
    TRACE("(%p %d)\n", ret, mask);

    ret->pfunc = setiosflags_func;
    ret->arg = mask;
    return ret;
}

static void __cdecl setbase_func(ios_base *base, int set_base)
{
    if(set_base == 10)
        set_base = FMTFLAG_dec;
    else if(set_base == 8)
        set_base = FMTFLAG_oct;
    else if(set_base == 16)
        set_base = FMTFLAG_hex;
    else
        set_base = 0;

    ios_base_setf_mask(base, set_base, FMTFLAG_basefield);
}

/* ?setbase@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl setbase(manip_int *ret, int base)
{
    TRACE("(%p %d)\n", ret, base);

    ret->pfunc = setbase_func;
    ret->arg = base;
    return ret;
}

static basic_filebuf_char filebuf_char_stdin;
/* ?cin@std@@3V?$basic_istream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_istream_char obj;
    basic_ios_char vbase;
} cin = { { 0 } };

static basic_filebuf_wchar filebuf_short_stdin;
/* ?wcin@std@@3V?$basic_istream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_istream_wchar obj;
    basic_ios_wchar vbase;
} ucin = { { 0 } };

static basic_filebuf_char filebuf_char_stdout;
/* ?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} cout = { { 0 } };

static basic_filebuf_wchar filebuf_short_stdout;
/* ?wcout@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
    basic_ios_wchar vbase;
} ucout = { { 0 } };

static basic_filebuf_char filebuf_char_stderr;
/* ?cerr@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} cerr = { { 0 } };

static basic_filebuf_wchar filebuf_short_stderr;
/* ?wcerr@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
    basic_ios_wchar vbase;
} ucerr = { { 0 } };

static basic_filebuf_char filebuf_char_log;
/* ?clog@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
    basic_ios_char vbase;
} MSVCP_clog = { { 0 } };

static basic_filebuf_wchar filebuf_short_log;
/* ?wclog@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
    basic_ios_wchar vbase;
} uclog = { { 0 } };

/* ?_Init_cnt@Init@ios_base@std@@0HA */
int ios_base_Init__Init_cnt = -1;

/* ??0Init@ios_base@std@@QAE@XZ */
/* ??0Init@ios_base@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_Init_ctor, 4)
void* __thiscall ios_base_Init_ctor(void *this)
{
    TRACE("(%p)\n", this);

    if(ios_base_Init__Init_cnt < 0)
        ios_base_Init__Init_cnt = 1;
    else
        ios_base_Init__Init_cnt++;
    return this;
}

/* ??1Init@ios_base@std@@QAE@XZ */
/* ??1Init@ios_base@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_Init_dtor, 4)
void __thiscall ios_base_Init_dtor(void *this)
{
    TRACE("(%p)\n", this);

    ios_base_Init__Init_cnt--;
    if(!ios_base_Init__Init_cnt) {
        basic_ostream_char_flush(&cout.obj);
        basic_ostream_char_flush(&cerr.obj);
        basic_ostream_char_flush(&MSVCP_clog.obj);
    }
}

/* ??4Init@ios_base@std@@QAEAAV012@ABV012@@Z */
/* ??4Init@ios_base@std@@QEAAAEAV012@AEBV012@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Init_op_assign, 8)
void* __thiscall ios_base_Init_op_assign(void *this, void *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    return this;
}

/* ?_Init_cnt@_Winit@std@@0HA */
int _Winit__Init_cnt = -1;

/* ??0_Winit@std@@QAE@XZ */
/* ??0_Winit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Winit_ctor, 4)
void* __thiscall _Winit_ctor(void *this)
{
    TRACE("(%p)\n", this);

    if(_Winit__Init_cnt < 0)
        _Winit__Init_cnt = 1;
    else
        _Winit__Init_cnt++;

    return this;
}

/* ??1_Winit@std@@QAE@XZ */
/* ??1_Winit@std@@QAE@XZ */
DEFINE_THISCALL_WRAPPER(_Winit_dtor, 4)
void __thiscall _Winit_dtor(void *this)
{
    TRACE("(%p)\n", this);

    _Winit__Init_cnt--;
    if(!_Winit__Init_cnt) {
        basic_ostream_short_flush(&ucout.obj);
        basic_ostream_short_flush(&ucerr.obj);
        basic_ostream_short_flush(&uclog.obj);
    }
}

/* ??4_Winit@std@@QAEAAV01@ABV01@@Z */
/* ??4_Winit@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Winit_op_assign, 8)
void* __thiscall _Winit_op_assign(void *this, void *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    return this;
}

void init_io(void *base)
{
    INIT_RTTI(iosb, base);
    INIT_RTTI(ios_base, base);
    INIT_RTTI(basic_ios_char, base);
    INIT_RTTI(basic_ios_short, base);
    INIT_RTTI(basic_streambuf_char, base);
    INIT_RTTI(basic_streambuf_short, base);
    INIT_RTTI(basic_filebuf_char, base);
    INIT_RTTI(basic_filebuf_short, base);
    INIT_RTTI(basic_stringbuf_char, base);
    INIT_RTTI(basic_stringbuf_short, base);
    INIT_RTTI(basic_ostream_char, base);
    INIT_RTTI(basic_ostream_short, base);
    INIT_RTTI(basic_istream_char, base);
    INIT_RTTI(basic_istream_short, base);
    INIT_RTTI(basic_iostream_char, base);
    INIT_RTTI(basic_iostream_short, base);
    INIT_RTTI(basic_ofstream_char, base);
    INIT_RTTI(basic_ofstream_short, base);
    INIT_RTTI(basic_ifstream_char, base);
    INIT_RTTI(basic_ifstream_short, base);
    INIT_RTTI(basic_fstream_char, base);
    INIT_RTTI(basic_fstream_short, base);
    INIT_RTTI(basic_ostringstream_char, base);
    INIT_RTTI(basic_ostringstream_short, base);
    INIT_RTTI(basic_istringstream_char, base);
    INIT_RTTI(basic_istringstream_short, base);
    INIT_RTTI(basic_stringstream_char, base);
    INIT_RTTI(basic_stringstream_short, base);
    INIT_RTTI(strstreambuf, base);
    INIT_RTTI(strstream, base);
    INIT_RTTI(ostrstream, base);

    basic_filebuf_char_ctor_file(&filebuf_char_stdin, stdin);
    basic_istream_char_ctor(&cin.obj, &filebuf_char_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stdin, stdin);
    basic_istream_short_ctor(&ucin.obj, &filebuf_short_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_stdout, stdout);
    basic_ostream_char_ctor(&cout.obj, &filebuf_char_stdout.base, FALSE/*FIXME*/, TRUE, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stdout, stdout);
    basic_ostream_short_ctor(&ucout.obj, &filebuf_short_stdout.base, FALSE/*FIXME*/, TRUE, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_stderr, stderr);
    basic_ostream_char_ctor(&cerr.obj, &filebuf_char_stderr.base, FALSE/*FIXME*/, TRUE, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stderr, stderr);
    basic_ostream_short_ctor(&ucerr.obj, &filebuf_short_stderr.base, FALSE/*FIXME*/, TRUE, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_log, stderr);
    basic_ostream_char_ctor(&MSVCP_clog.obj, &filebuf_char_log.base, FALSE/*FIXME*/, TRUE, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_log, stderr);
    basic_ostream_short_ctor(&uclog.obj, &filebuf_short_log.base, FALSE/*FIXME*/, TRUE, TRUE);
}

void free_io(void)
{
    basic_istream_char_vbase_dtor(&cin.obj);
    basic_filebuf_char_dtor(&filebuf_char_stdin);

    basic_istream_short_vbase_dtor(&ucin.obj);
    basic_filebuf_short_dtor(&filebuf_short_stdin);

    basic_ostream_char_vbase_dtor(&cout.obj);
    basic_filebuf_char_dtor(&filebuf_char_stdout);

    basic_ostream_short_vbase_dtor(&ucout.obj);
    basic_filebuf_short_dtor(&filebuf_short_stdout);

    basic_ostream_char_vbase_dtor(&cerr.obj);
    basic_filebuf_char_dtor(&filebuf_char_stderr);

    basic_ostream_short_vbase_dtor(&ucerr.obj);
    basic_filebuf_short_dtor(&filebuf_short_stderr);

    basic_ostream_char_vbase_dtor(&MSVCP_clog.obj);
    basic_filebuf_char_dtor(&filebuf_char_log);

    basic_ostream_short_vbase_dtor(&uclog.obj);
    basic_filebuf_short_dtor(&filebuf_short_log);
}
