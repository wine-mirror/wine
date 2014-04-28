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

#define ALIGNED_SIZE(size, alignment) (((size)+((alignment)-1))/(alignment)*(alignment))

typedef unsigned char MSVCP_bool;
typedef SIZE_T MSVCP_size_t;

#if _MSVCP_VER >= 100
typedef __int64 DECLSPEC_ALIGN(8) streamoff;
typedef __int64 DECLSPEC_ALIGN(8) streamsize;
#define STREAMOFF_BITS 64
#define STREAMSIZE_BITS 64
#else
typedef SSIZE_T streamoff;
typedef SSIZE_T streamsize;
#define STREAMOFF_BITS 32
#define STREAMSIZE_BITS 32
#endif

void __cdecl _invalid_parameter(const wchar_t*, const wchar_t*,
        const wchar_t*, unsigned int, uintptr_t);
BOOL __cdecl __uncaught_exception(void);

extern void* (__cdecl *MSVCRT_operator_new)(MSVCP_size_t);
extern void (__cdecl *MSVCRT_operator_delete)(void*);
extern void* (__cdecl *MSVCRT_set_new_handler)(void*);

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

basic_string_char* __thiscall MSVCP_basic_string_char_ctor(basic_string_char*);
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr(basic_string_char*, const char*);
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_len(basic_string_char*, const char*, MSVCP_size_t);
basic_string_char* __thiscall MSVCP_basic_string_char_copy_ctor(basic_string_char*, const basic_string_char*);
void __thiscall MSVCP_basic_string_char_dtor(basic_string_char*);
const char* __thiscall MSVCP_basic_string_char_c_str(const basic_string_char*);
void __thiscall MSVCP_basic_string_char_clear(basic_string_char*);
basic_string_char* __thiscall MSVCP_basic_string_char_append_ch(basic_string_char*, char);
MSVCP_size_t __thiscall MSVCP_basic_string_char_length(const basic_string_char*);
basic_string_char* __thiscall MSVCP_basic_string_char_assign(basic_string_char*, const basic_string_char*);

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

basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor(basic_string_wchar*);
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr(basic_string_wchar*, const wchar_t*);
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_len(basic_string_wchar*, const wchar_t*, MSVCP_size_t);
void __thiscall MSVCP_basic_string_wchar_dtor(basic_string_wchar*);
const wchar_t* __thiscall MSVCP_basic_string_wchar_c_str(const basic_string_wchar*);
void __thiscall MSVCP_basic_string_wchar_clear(basic_string_wchar*);
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_ch(basic_string_wchar*, wchar_t);
MSVCP_size_t __thiscall MSVCP_basic_string_wchar_length(const basic_string_wchar*);

char* __thiscall MSVCP_allocator_char_allocate(void*, MSVCP_size_t);
void __thiscall MSVCP_allocator_char_deallocate(void*, char*, MSVCP_size_t);
MSVCP_size_t __thiscall MSVCP_allocator_char_max_size(void*);
wchar_t* __thiscall MSVCP_allocator_wchar_allocate(void*, MSVCP_size_t);
void __thiscall MSVCP_allocator_wchar_deallocate(void*, wchar_t*, MSVCP_size_t);
MSVCP_size_t __thiscall MSVCP_allocator_wchar_max_size(void*);

typedef struct
{
    char *str;
    char null_str;
} _Yarn_char;

_Yarn_char* __thiscall _Yarn_char_ctor_cstr(_Yarn_char*, const char*);
_Yarn_char* __thiscall _Yarn_char_copy_ctor(_Yarn_char*, const _Yarn_char*);
const char* __thiscall _Yarn_char_c_str(const _Yarn_char*);
void __thiscall _Yarn_char_dtor(_Yarn_char*);
_Yarn_char* __thiscall _Yarn_char_op_assign(_Yarn_char*, const _Yarn_char*);

/* class locale::facet */
typedef struct {
    const vtable_ptr *vtable;
    MSVCP_size_t refs;
} locale_facet;

typedef enum {
    CODECVT_ok      = 0,
    CODECVT_partial = 1,
    CODECVT_error   = 2,
    CODECVT_noconv  = 3
} codecvt_base_result;

typedef struct {
#if _MSVCP_VER < 110
    LCID handle;
#endif
    unsigned page;
    const short *table;
    int delfl;
#if _MSVCP_VER >= 110
    wchar_t *name;
#endif
} _Ctypevec;

/* class codecvt_base */
typedef struct {
    locale_facet facet;
} codecvt_base;

/* class codecvt<char> */
typedef struct {
    codecvt_base base;
} codecvt_char;

MSVCP_bool __thiscall codecvt_base_always_noconv(const codecvt_base*);
int __thiscall codecvt_char_unshift(const codecvt_char*, int*, char*, char*, char**);
int __thiscall codecvt_char_out(const codecvt_char*, int*, const char*,
        const char*, const char**, char*, char*, char**);
int __thiscall codecvt_char_in(const codecvt_char*, int*, const char*,
        const char*, const char**, char*, char*, char**);
int __thiscall codecvt_base_max_length(const codecvt_base*);

typedef struct {
    LCID handle;
    unsigned page;
} _Cvtvec;

/* class codecvt<wchar> */
typedef struct {
    codecvt_base base;
    _Cvtvec cvt;
} codecvt_wchar;

int __thiscall codecvt_wchar_unshift(const codecvt_wchar*, int*, char*, char*, char**);
int __thiscall codecvt_wchar_out(const codecvt_wchar*, int*, const wchar_t*,
        const wchar_t*, const wchar_t**, char*, char*, char**);
int __thiscall codecvt_wchar_in(const codecvt_wchar*, int*, const char*,
        const char*, const char**, wchar_t*, wchar_t*, wchar_t**);

/* class ctype_base */
typedef struct {
    locale_facet facet;
} ctype_base;

/* class ctype<char> */
typedef struct {
    ctype_base base;
    _Ctypevec ctype;
} ctype_char;

MSVCP_bool __thiscall ctype_char_is_ch(const ctype_char*, short, char);
char __thiscall ctype_char_narrow_ch(const ctype_char*, char, char);
char __thiscall ctype_char_widen_ch(const ctype_char*, char);

/* class ctype<wchar> */
typedef struct {
    ctype_base base;
    _Ctypevec ctype;
    _Cvtvec cvt;
} ctype_wchar;

MSVCP_bool __thiscall ctype_wchar_is_ch(const ctype_wchar*, short, wchar_t);
char __thiscall ctype_wchar_narrow_ch(const ctype_wchar*, wchar_t, char);
wchar_t __thiscall ctype_wchar_widen_ch(const ctype_wchar*, char);

/* class locale */
typedef struct
{
    struct _locale__Locimp *ptr;
} locale;

locale* __thiscall locale_ctor(locale*);
locale* __thiscall locale_copy_ctor(locale*, const locale*);
locale* __thiscall locale_operator_assign(locale*, const locale*);
void __thiscall locale_dtor(locale*);
void free_locale(void);
codecvt_char* codecvt_char_use_facet(const locale*);
codecvt_wchar* codecvt_wchar_use_facet(const locale*);
codecvt_wchar* codecvt_short_use_facet(const locale*);
ctype_char* ctype_char_use_facet(const locale*);
ctype_wchar* ctype_wchar_use_facet(const locale*);
ctype_wchar* ctype_short_use_facet(const locale*);

/* class _Lockit */
typedef struct {
    int locktype;
} _Lockit;

#define _LOCK_LOCALE 0
#define _LOCK_MALLOC 1
#define _LOCK_STREAM 2
#define _LOCK_DEBUG 3
#define _MAX_LOCK 4

void init_lockit(void);
void free_lockit(void);
_Lockit* __thiscall _Lockit_ctor_locktype(_Lockit*, int);
void __thiscall _Lockit_dtor(_Lockit*);

/* class mutex */
typedef struct {
    void *mutex;
} mutex;

mutex* __thiscall mutex_ctor(mutex*);
void __thiscall mutex_dtor(mutex*);
void __thiscall mutex_lock(mutex*);
void __thiscall mutex_unlock(mutex*);

typedef enum {
    FMTFLAG_skipws      = 0x0001,
    FMTFLAG_unitbuf     = 0x0002,
    FMTFLAG_uppercase   = 0x0004,
    FMTFLAG_showbase    = 0x0008,
    FMTFLAG_showpoint   = 0x0010,
    FMTFLAG_showpos     = 0x0020,
    FMTFLAG_left        = 0x0040,
    FMTFLAG_right       = 0x0080,
    FMTFLAG_internal    = 0x0100,
    FMTFLAG_dec         = 0x0200,
    FMTFLAG_oct         = 0x0400,
    FMTFLAG_hex         = 0x0800,
    FMTFLAG_scientific  = 0x1000,
    FMTFLAG_fixed       = 0x2000,
    FMTFLAG_hexfloat    = 0x3000,
    FMTFLAG_boolalpha   = 0x4000,
    FMTFLAG_stdio       = 0x8000,
    FMTFLAG_adjustfield = FMTFLAG_left|FMTFLAG_right|FMTFLAG_internal,
    FMTFLAG_basefield   = FMTFLAG_dec|FMTFLAG_oct|FMTFLAG_hex,
    FMTFLAG_floatfield  = FMTFLAG_scientific|FMTFLAG_fixed,
    FMTFLAG_mask        = 0xffff
} IOSB_fmtflags;

typedef enum {
    OPENMODE_in         = 0x01,
    OPENMODE_out        = 0x02,
    OPENMODE_ate        = 0x04,
    OPENMODE_app        = 0x08,
    OPENMODE_trunc      = 0x10,
    OPENMODE__Nocreate  = 0x40,
    OPENMODE__Noreplace = 0x80,
    OPENMODE_binary     = 0x20,
    OPENMODE_mask       = 0xff
} IOSB_openmode;

typedef enum {
    SEEKDIR_beg  = 0x0,
    SEEKDIR_cur  = 0x1,
    SEEKDIR_end  = 0x2,
    SEEKDIR_mask = 0x3
} IOSB_seekdir;

typedef enum {
    IOSTATE_goodbit   = 0x00,
    IOSTATE_eofbit    = 0x01,
    IOSTATE_failbit   = 0x02,
    IOSTATE_badbit    = 0x04,
    IOSTATE__Hardfail = 0x10,
    IOSTATE_mask      = 0x17
} IOSB_iostate;

typedef struct _iosarray {
    struct _iosarray *next;
    int index;
    int long_val;
    void *ptr_val;
} IOS_BASE_iosarray;

typedef enum {
    EVENT_erase_event,
    EVENT_imbue_event,
    EVENT_copyfmt_event
} IOS_BASE_event;

struct _ios_base;
typedef void (CDECL *IOS_BASE_event_callback)(IOS_BASE_event, struct _ios_base*, int);
typedef struct _fnarray {
    struct _fnarray *next;
    int index;
    IOS_BASE_event_callback event_handler;
} IOS_BASE_fnarray;

/* class ios_base */
typedef struct _ios_base {
    const vtable_ptr *vtable;
#if _MSVCP_VER >= 100
    MSVCP_size_t DECLSPEC_ALIGN(8) stdstr;
#else
    MSVCP_size_t stdstr;
#endif
    IOSB_iostate state;
    IOSB_iostate except;
    IOSB_fmtflags fmtfl;
    streamsize prec;
    streamsize wide;
    IOS_BASE_iosarray *arr;
    IOS_BASE_fnarray *calls;
    locale *loc;
} ios_base;

/* class basic_streambuf<char> */
typedef struct {
    const vtable_ptr *vtable;
#if _MSVCP_VER <= 100
    mutex lock;
#endif
    char *rbuf;
    char *wbuf;
    char **prbuf;
    char **pwbuf;
    char *rpos;
    char *wpos;
    char **prpos;
    char **pwpos;
    int rsize;
    int wsize;
    int *prsize;
    int *pwsize;
    locale *loc;
} basic_streambuf_char;

typedef struct {
    basic_streambuf_char *strbuf;
    MSVCP_bool      got;
    char            val;
} istreambuf_iterator_char;

typedef struct {
#if _MSVCP_VER == 80
    void *iter_container;
#endif
    MSVCP_bool failed;
    basic_streambuf_char *strbuf;
} ostreambuf_iterator_char;

int __thiscall basic_streambuf_char_sgetc(basic_streambuf_char*);
int __thiscall basic_streambuf_char_sbumpc(basic_streambuf_char*);
int __thiscall basic_streambuf_char_sputc(basic_streambuf_char*, char);

/* class basic_streambuf<wchar> */
typedef struct {
    const vtable_ptr *vtable;
#if _MSVCP_VER <= 100
    mutex lock;
#endif
    wchar_t *rbuf;
    wchar_t *wbuf;
    wchar_t **prbuf;
    wchar_t **pwbuf;
    wchar_t *rpos;
    wchar_t *wpos;
    wchar_t **prpos;
    wchar_t **pwpos;
    int rsize;
    int wsize;
    int *prsize;
    int *pwsize;
    locale *loc;
} basic_streambuf_wchar;

typedef struct {
    basic_streambuf_wchar *strbuf;
    MSVCP_bool got;
    wchar_t val;
} istreambuf_iterator_wchar;

typedef struct {
#if _MSVCP_VER == 80
    void *iter_container;
#endif
    MSVCP_bool failed;
    basic_streambuf_wchar *strbuf;
} ostreambuf_iterator_wchar;

unsigned short __thiscall basic_streambuf_wchar_sgetc(basic_streambuf_wchar*);
unsigned short __thiscall basic_streambuf_wchar_sbumpc(basic_streambuf_wchar*);
unsigned short __thiscall basic_streambuf_wchar_sputc(basic_streambuf_wchar*, wchar_t);

/* class num_get<char> */
typedef struct {
    locale_facet facet;
    _Cvtvec cvt;
} num_get;

num_get* num_get_char_use_facet(const locale*);
istreambuf_iterator_char* __thiscall num_get_char_get_long(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONG*);
istreambuf_iterator_char* __thiscall num_get_char_get_ushort(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned short*);
istreambuf_iterator_char* __thiscall num_get_char_get_uint(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned int*);
istreambuf_iterator_char* __thiscall num_get_char_get_ulong(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONG*);
istreambuf_iterator_char* __thiscall num_get_char_get_float(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, float*);
istreambuf_iterator_char *__thiscall num_get_char_get_double(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*);
istreambuf_iterator_char *__thiscall num_get_char_get_ldouble(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*);
istreambuf_iterator_char *__thiscall num_get_char_get_void(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, void**);
istreambuf_iterator_char *__thiscall num_get_char_get_int64(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONGLONG*);
istreambuf_iterator_char *__thiscall num_get_char_get_uint64(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONGLONG*);
istreambuf_iterator_char *__thiscall num_get_char_get_bool(const num_get*, istreambuf_iterator_char*,
        istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, MSVCP_bool*);

num_get* num_get_wchar_use_facet(const locale*);
num_get* num_get_short_use_facet(const locale*);
istreambuf_iterator_wchar* __thiscall num_get_wchar_get_long(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONG*);
istreambuf_iterator_wchar* __thiscall num_get_wchar_get_ushort(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned short*);
istreambuf_iterator_wchar* __thiscall num_get_wchar_get_uint(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned int*);
istreambuf_iterator_wchar* __thiscall num_get_wchar_get_ulong(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONG*);
istreambuf_iterator_wchar* __thiscall num_get_wchar_get_float(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, float*);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_double(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ldouble(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_void(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, void**);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_int64(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONGLONG*);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_uint64(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONGLONG*);
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_bool(const num_get*, istreambuf_iterator_wchar*,
        istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, MSVCP_bool*);

/* class num_put<char> */
/* class num_put<wchar> */
typedef struct {
    locale_facet facet;
    _Cvtvec cvt;
} num_put;

num_put* num_put_char_use_facet(const locale*);
ostreambuf_iterator_char* __thiscall num_put_char_put_long(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, LONG);
ostreambuf_iterator_char* __thiscall num_put_char_put_ulong(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, ULONG);
ostreambuf_iterator_char* __thiscall num_put_char_put_double(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, double);
ostreambuf_iterator_char* __thiscall num_put_char_put_ldouble(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, double);
ostreambuf_iterator_char* __thiscall num_put_char_put_ptr(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, const void*);
ostreambuf_iterator_char* __thiscall num_put_char_put_int64(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, __int64);
ostreambuf_iterator_char* __thiscall num_put_char_put_uint64(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, unsigned __int64);
ostreambuf_iterator_char* __thiscall num_put_char_put_bool(const num_put*, ostreambuf_iterator_char*,
        ostreambuf_iterator_char, ios_base*, char, MSVCP_bool);

num_put* num_put_wchar_use_facet(const locale*);
num_put* num_put_short_use_facet(const locale*);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_long(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, LONG);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ulong(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, ULONG);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_double(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, double);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ldouble(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, double);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ptr(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, const void*);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_int64(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, __int64);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_uint64(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, unsigned __int64);
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_bool(const num_put*, ostreambuf_iterator_wchar*,
        ostreambuf_iterator_wchar, ios_base*, wchar_t, MSVCP_bool);

void init_exception(void*);
void init_locale(void*);
void init_io(void*);
void free_io(void);

/* class complex<float> */
typedef struct {
    float real;
    float imag;
} complex_float;

/* class complex<double> */
/* class complex<long double> */
typedef struct {
    double real;
    double imag;
} complex_double;

#if _MSVCP_VER < 80
#define memcpy_s( dst, size, src, count ) (memcpy( (dst), (src), (count) ), 0)
#define memmove_s( dst, size, src, count ) (memmove( (dst), (src), (count) ), 0)
#define mbstowcs_s( ret, wcs, size, mbs, count ) (mbstowcs( (wcs), (mbs), (count) ), 0)
#define hypotf( x, y ) ((float)hypot( (double)(x), (double)(y) ))
#endif
