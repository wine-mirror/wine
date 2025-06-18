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

#include <stdio.h>
#include <locale.h>
#include <sys/stat.h>

#include <windef.h>
#include <winbase.h>
#include <share.h>
#include "wine/test.h"

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef unsigned char MSVCP_bool;
typedef SIZE_T MSVCP_size_t;
typedef SSIZE_T streamoff;
typedef SSIZE_T streamsize;

typedef void (*vtable_ptr)(void);

/* basic_string<char, char_traits<char>, allocator<char>> */
typedef struct
{
    void *allocator;
    char *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_char;

typedef struct
{
    void *allocator;
    wchar_t *ptr;
    MSVCP_size_t size;
    MSVCP_size_t res;
} basic_string_wchar;

/* class locale */
typedef struct
{
    struct _locale__Locimp *ptr;
} locale;

/* class locale::facet */
typedef struct {
    const vtable_ptr *vtable;
    MSVCP_size_t refs;
} locale_facet;

/* class codecvt_base */
typedef struct {
    locale_facet facet;
} codecvt_base;

/* class codecvt<char> */
typedef struct {
    codecvt_base base;
} codecvt_char;

typedef struct {
    LCID handle;
    unsigned page;
} _Cvtvec;

/* class codecvt<wchar> */
typedef struct {
    codecvt_base base;
    _Cvtvec cvt;
} codecvt_wchar;

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
    IOSB_iostate state;
    IOSB_iostate except;
    IOSB_fmtflags fmtfl;
    streamsize prec;
    streamsize wide;
    IOS_BASE_iosarray *arr;
    IOS_BASE_fnarray *calls;
    locale loc;
    MSVCP_size_t stdstr;
} ios_base;

/* class basic_streambuf<char> */
typedef struct {
    const vtable_ptr *vtable;
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
    locale loc;
} basic_streambuf_char;

/* class basic_streambuf<wchar> */
typedef struct {
    const vtable_ptr *vtable;
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
    locale loc;
} basic_streambuf_wchar;

typedef struct {
    basic_streambuf_char base;
    codecvt_char *cvt;
    int state0;
    int state;
    basic_string_char *str;
    MSVCP_bool close;
    locale loc;
    FILE *file;
} basic_filebuf_char;

typedef struct {
    basic_streambuf_wchar base;
    codecvt_wchar *cvt;
    int state0;
    int state;
    basic_string_char *str;
    MSVCP_bool close;
    locale loc;
    FILE *file;
} basic_filebuf_wchar;

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
    /* virtual inheritance */
    basic_ios_char basic_ios; /* here to reserve correct stack size */
} basic_fstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance */
    basic_ios_wchar basic_ios; /* here to reserve correct stack size */
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
    /* virtual inheritance */
    basic_ios_char basic_ios; /* here to reserve correct stack size */
} basic_stringstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance */
    basic_ios_wchar basic_ios; /* here to reserve correct stack size */
} basic_stringstream_wchar;

typedef struct {
    streamoff off;
    INT64 pos;
    int state;
} fpos_int;

/* stringstream */
static basic_stringstream_char* (*__thiscall p_basic_stringstream_char_ctor_mode)(basic_stringstream_char*, int, MSVCP_bool);
static basic_stringstream_char* (*__thiscall p_basic_stringstream_char_ctor_str)(basic_stringstream_char*, const basic_string_char*, int, MSVCP_bool);
static basic_string_char* (*__thiscall p_basic_stringstream_char_str_get)(const basic_stringstream_char*, basic_string_char*);
static void (*__thiscall p_basic_stringstream_char_vbase_dtor)(basic_stringstream_char*);

static basic_stringstream_wchar* (*__thiscall p_basic_stringstream_wchar_ctor_mode)(basic_stringstream_wchar*, int, MSVCP_bool);
static basic_stringstream_wchar* (*__thiscall p_basic_stringstream_wchar_ctor_str)(basic_stringstream_wchar*, const basic_string_wchar*, int, MSVCP_bool);
static basic_string_wchar* (*__thiscall p_basic_stringstream_wchar_str_get)(const basic_stringstream_wchar*, basic_string_wchar*);
static void (*__thiscall p_basic_stringstream_wchar_vbase_dtor)(basic_stringstream_wchar*);

/* fstream */
static basic_fstream_char* (*__thiscall p_basic_fstream_char_ctor_name)(basic_fstream_char*, const char*, int, MSVCP_bool);
static void (*__thiscall p_basic_fstream_char_vbase_dtor)(basic_fstream_char*);

static basic_fstream_wchar* (*__thiscall p_basic_fstream_wchar_ctor_name)(basic_fstream_wchar*, const char*, int, MSVCP_bool);
static void (*__thiscall p_basic_fstream_wchar_vbase_dtor)(basic_fstream_wchar*);

/* istream */
static basic_istream_char* (*__thiscall p_basic_istream_char_read_double)(basic_istream_char*, double*);
static int                 (*__thiscall p_basic_istream_char_get)(basic_istream_char*);
static MSVCP_bool          (*__thiscall p_basic_istream_char_ipfx)(basic_istream_char*, MSVCP_bool);
static basic_istream_char* (*__thiscall p_basic_istream_char_ignore)(basic_istream_char*, streamsize, int);
static basic_istream_char* (*__thiscall p_basic_istream_char_seekg)(basic_istream_char*, streamoff, int);
static basic_istream_char* (*__thiscall p_basic_istream_char_seekg_fpos)(basic_istream_char*, fpos_int);
static int                 (*__thiscall p_basic_istream_char_peek)(basic_istream_char*);
static fpos_int*           (*__thiscall p_basic_istream_char_tellg)(basic_istream_char*, fpos_int*);
static basic_istream_char* (*__cdecl    p_basic_istream_char_getline_bstr_delim)(basic_istream_char*, basic_string_char*, char);

static basic_istream_wchar* (*__thiscall p_basic_istream_wchar_read_double)(basic_istream_wchar*, double *);
static int                  (*__thiscall p_basic_istream_wchar_get)(basic_istream_wchar*);
static MSVCP_bool           (*__thiscall p_basic_istream_wchar_ipfx)(basic_istream_wchar*, MSVCP_bool);
static basic_istream_wchar* (*__thiscall p_basic_istream_wchar_ignore)(basic_istream_wchar*, streamsize, unsigned short);
static basic_istream_wchar* (*__thiscall p_basic_istream_wchar_seekg)(basic_istream_wchar*, streamoff, int);
static basic_istream_wchar* (*__thiscall p_basic_istream_wchar_seekg_fpos)(basic_istream_wchar*, fpos_int);
static unsigned short       (*__thiscall p_basic_istream_wchar_peek)(basic_istream_wchar*);
static fpos_int*            (*__thiscall p_basic_istream_wchar_tellg)(basic_istream_wchar*, fpos_int*);
static basic_istream_wchar* (*__cdecl    p_basic_istream_wchar_getline_bstr_delim)(basic_istream_wchar*, basic_string_wchar*, wchar_t);

/* ostream */
static basic_ostream_char* (*__thiscall p_basic_ostream_char_print_double)(basic_ostream_char*, double);

static basic_ostream_wchar* (*__thiscall p_basic_ostream_wchar_print_double)(basic_ostream_wchar*, double);

static basic_ostream_wchar* (*__thiscall p_basic_ostream_short_print_ushort)(basic_ostream_wchar*, unsigned short);

/* basic_ios */
static locale*  (*__thiscall p_basic_ios_char_imbue)(basic_ios_char*, locale*, const locale*);

static locale*  (*__thiscall p_basic_ios_wchar_imbue)(basic_ios_wchar*, locale*, const locale*);

/* ios_base */
static IOSB_iostate  (*__thiscall p_ios_base_rdstate)(const ios_base*);
static IOSB_fmtflags (*__thiscall p_ios_base_setf_mask)(ios_base*, IOSB_fmtflags, IOSB_fmtflags);
static void          (*__thiscall p_ios_base_unsetf)(ios_base*, IOSB_fmtflags);
static streamsize    (*__thiscall p_ios_base_precision_set)(ios_base*, streamsize);

/* locale */
static locale*  (*__thiscall p_locale_ctor_cstr)(locale*, const char*, int /* FIXME: category */);
static void     (*__thiscall p_locale_dtor)(locale *this);

/* basic_string */
static basic_string_char* (__thiscall *p_basic_string_char_ctor_cstr_alloc)(basic_string_char*, const char*, void*);
static const char* (__thiscall *p_basic_string_char_cstr)(basic_string_char*);
static void (__thiscall *p_basic_string_char_dtor)(basic_string_char*);

static basic_string_wchar* (__thiscall *p_basic_string_wchar_ctor_cstr_alloc)(basic_string_wchar*, const wchar_t*, void*);
static const wchar_t* (__thiscall *p_basic_string_wchar_cstr)(basic_string_wchar*);
static void (__thiscall *p_basic_string_wchar_dtor)(basic_string_wchar*);

/* Emulate a __thiscall */
#ifdef __i386__

#pragma pack(push,1)
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#pragma pack(pop)

static void * (WINAPI *call_thiscall_func1)( void *func, void *this );
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, const void *a, const void *b );
static void * (WINAPI *call_thiscall_func4)( void *func, void *this, const void *a, const void *b,
        const void *c );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, const void *a, const void *b,
        const void *c, const void *d );

/* to silence compiler errors */
static void * (WINAPI *call_thiscall_func2_ptr_dbl)( void *func, void *this, double a );
static void * (WINAPI *call_thiscall_func2_ptr_fpos)( void *func, void *this, fpos_int a );

struct thiscall_thunk_retptr *thunk_retptr;

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc( NULL, sizeof(*thunk),
                                                 MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
    call_thiscall_func3 = (void *)thunk;
    call_thiscall_func4 = (void *)thunk;
    call_thiscall_func5 = (void *)thunk;

    call_thiscall_func2_ptr_dbl  = (void *)thunk;
    call_thiscall_func2_ptr_fpos = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,(const void*)(a),(const void*)(b), \
        (const void*)(c))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(const void*)(a),(const void*)(b), \
        (const void*)(c), (const void *)(d))

#define call_func2_ptr_dbl(func,_this,a)  call_thiscall_func2_ptr_dbl(func,_this,a)
#define call_func2_ptr_fpos(func,_this,a) call_thiscall_func2_ptr_fpos(func,_this,a)

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)

#define call_func2_ptr_dbl   call_func2
#define call_func2_ptr_fpos  call_func2

#endif /* __i386__ */

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp60.dll");
    if(!msvcp) {
        win_skip("msvcp60.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_basic_stringstream_char_ctor_mode,
            "??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z");
        SET(p_basic_stringstream_char_ctor_str,
            "??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z");
        SET(p_basic_stringstream_char_str_get,
            "?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ");
        SET(p_basic_stringstream_char_vbase_dtor,
            "??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ");

        SET(p_basic_stringstream_wchar_ctor_mode,
            "??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z");
        SET(p_basic_stringstream_wchar_ctor_str,
            "??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z");
        SET(p_basic_stringstream_wchar_str_get,
            "?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ");
        SET(p_basic_stringstream_wchar_vbase_dtor,
            "??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ");

        SET(p_basic_fstream_char_ctor_name,
            "??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z");
        SET(p_basic_fstream_char_vbase_dtor,
            "??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXXZ");

        SET(p_basic_fstream_wchar_ctor_name,
            "??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDH@Z");
        SET(p_basic_fstream_wchar_vbase_dtor,
            "??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXXZ");

        SET(p_basic_istream_char_read_double,
            "??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAN@Z");
        SET(p_basic_istream_char_get,
            "?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ");
        SET(p_basic_istream_char_ipfx,
            "?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z");
        SET(p_basic_istream_char_ignore,
            "?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z");
        SET(p_basic_istream_char_seekg,
            "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z");
        SET(p_basic_istream_char_seekg_fpos,
            "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z");
        SET(p_basic_istream_char_peek,
            "?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ");
        SET(p_basic_istream_char_tellg,
            "?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@XZ");
        SETNOFAIL(p_basic_istream_char_getline_bstr_delim,
            "??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z");

        SET(p_basic_istream_wchar_read_double,
            "??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAN@Z");
        SET(p_basic_istream_wchar_get,
            "?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ");
        SET(p_basic_istream_wchar_ipfx,
            "?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_N_N@Z");
        SET(p_basic_istream_wchar_ignore,
            "?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JG@Z");
        SET(p_basic_istream_wchar_seekg,
            "?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z");
        SET(p_basic_istream_wchar_seekg_fpos,
            "?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z");
        SET(p_basic_istream_wchar_peek,
            "?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ");
        SET(p_basic_istream_wchar_tellg,
            "?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@XZ");
        SETNOFAIL(p_basic_istream_wchar_getline_bstr_delim,
            "??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z");

        SET(p_basic_ostream_char_print_double,
            "??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@N@Z");

        SET(p_basic_ostream_wchar_print_double,
            "??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@N@Z");

        SET(p_basic_ostream_short_print_ushort,
            "??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@G@Z");

        SET(p_ios_base_rdstate,
            "?rdstate@ios_base@std@@QEBAHXZ");
        SET(p_ios_base_setf_mask,
            "?setf@ios_base@std@@QEAAHHH@Z");
        SET(p_ios_base_unsetf,
            "?unsetf@ios_base@std@@QEAAXH@Z");
        SET(p_ios_base_precision_set,
            "?precision@ios_base@std@@QEAA_JH@Z");

        SET(p_basic_ios_char_imbue,
            "?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z");

        SET(p_basic_ios_wchar_imbue,
            "?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z");

        SET(p_locale_ctor_cstr,
            "??0locale@std@@QEAA@PEBDH@Z");
        SET(p_locale_dtor,
            "??1locale@std@@QEAA@XZ");

        SET(p_basic_string_char_ctor_cstr_alloc,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBDAEBV?$allocator@D@1@@Z");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ");

        SET(p_basic_string_wchar_ctor_cstr_alloc,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBGAEBV?$allocator@G@1@@Z");
        SET(p_basic_string_wchar_cstr,
                "?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ");
        SET(p_basic_string_wchar_dtor,
                "??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ");
    } else {
        SET(p_basic_stringstream_char_ctor_mode,
            "??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z");
        SET(p_basic_stringstream_char_ctor_str,
            "??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z");
        SET(p_basic_stringstream_char_str_get,
            "?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ");
        SET(p_basic_stringstream_char_vbase_dtor,
            "??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ");

        SET(p_basic_stringstream_wchar_ctor_mode,
            "??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z");
        SET(p_basic_stringstream_wchar_ctor_str,
            "??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z");
        SET(p_basic_stringstream_wchar_str_get,
            "?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ");
        SET(p_basic_stringstream_wchar_vbase_dtor,
            "??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ");

        SET(p_basic_fstream_char_ctor_name,
            "??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z");
        SET(p_basic_fstream_char_vbase_dtor,
            "??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXXZ");

        SET(p_basic_fstream_wchar_ctor_name,
            "??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PBDH@Z");
        SET(p_basic_fstream_wchar_vbase_dtor,
            "??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXXZ");

        SET(p_basic_istream_char_read_double,
            "??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAN@Z");
        SET(p_basic_istream_char_get,
            "?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ");
        SET(p_basic_istream_char_ipfx,
            "?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z");
        SET(p_basic_istream_char_ignore,
            "?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@HH@Z");
        SET(p_basic_istream_char_seekg,
            "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z");
        SET(p_basic_istream_char_seekg_fpos,
            "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z");
        SET(p_basic_istream_char_peek,
            "?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ");
        SET(p_basic_istream_char_tellg,
            "?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@XZ");
        SETNOFAIL(p_basic_istream_char_getline_bstr_delim,
            "??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z");

        SET(p_basic_istream_wchar_read_double,
            "??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAN@Z");
        SET(p_basic_istream_wchar_get,
            "?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ");
        SET(p_basic_istream_wchar_ipfx,
            "?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE_N_N@Z");
        SET(p_basic_istream_wchar_ignore,
            "?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@HG@Z");
        SET(p_basic_istream_wchar_seekg,
            "?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z");
        SET(p_basic_istream_wchar_seekg_fpos,
            "?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z");
        SET(p_basic_istream_wchar_peek,
            "?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ");
        SET(p_basic_istream_wchar_tellg,
            "?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@XZ");
        SETNOFAIL(p_basic_istream_wchar_getline_bstr_delim,
            "??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z");

        SET(p_basic_ostream_char_print_double,
            "??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@N@Z");

        SET(p_basic_ostream_wchar_print_double,
            "??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@N@Z");

        SET(p_basic_ostream_short_print_ushort,
            "??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@G@Z");

        SET(p_ios_base_rdstate,
            "?rdstate@ios_base@std@@QBEHXZ");
        SET(p_ios_base_setf_mask,
            "?setf@ios_base@std@@QAEHHH@Z");
        SET(p_ios_base_unsetf,
            "?unsetf@ios_base@std@@QAEXH@Z");
        SET(p_ios_base_precision_set,
            "?precision@ios_base@std@@QAEHH@Z");

        SET(p_basic_ios_char_imbue,
            "?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z");

        SET(p_basic_ios_wchar_imbue,
            "?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z");

        SET(p_locale_ctor_cstr,
            "??0locale@std@@QAE@PBDH@Z");
        SET(p_locale_dtor,
            "??1locale@std@@QAE@XZ");

        SET(p_basic_string_char_ctor_cstr_alloc,
                "??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDABV?$allocator@D@1@@Z");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");

        SET(p_basic_string_wchar_ctor_cstr_alloc,
                "??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGABV?$allocator@G@1@@Z");
        SET(p_basic_string_wchar_cstr,
                "?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ");
        SET(p_basic_string_wchar_dtor,
                "??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

char fake_allocator;

/* convert a dll name A->W without depending on the current codepage */
static wchar_t *AtoW( wchar_t *nameW, const char *nameA, unsigned int len )
{
    unsigned int i;

    for (i = 0; i < len; i++) nameW[i] = nameA[i];
    nameW[i] = 0;
    return nameW;
}

static void test_num_get_get_double(void)
{
    unsigned short testus, nextus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    locale lcl, retlcl;
    wchar_t wide[64];
    int i, next;
    double val;

    /* makes tables narrower */
    const IOSB_iostate IOSTATE_faileof = IOSTATE_failbit|IOSTATE_eofbit;

    struct _test_num_get {
        const char    *str;
        const char    *lcl;
        IOSB_iostate  state;
        double        val;
        int           next;
    } tests[] = {
        /* simple cases */
        { "0",     NULL, IOSTATE_eofbit,  0.0,  EOF },
        { "10",    NULL, IOSTATE_eofbit,  10.0, EOF },
        { "+10",   NULL, IOSTATE_eofbit,  10.0, EOF },
        { "-10",   NULL, IOSTATE_eofbit, -10.0, EOF },
        { "+010",  NULL, IOSTATE_eofbit,  10.0, EOF }, /* leading zero */

        /* test grouping - default/"C" has no grouping, named English/German locales do */
        { "1,000", NULL,         IOSTATE_goodbit,  1.0,      ',' }, /* with comma */
        { "1,000", "English",    IOSTATE_goodbit,  1.0,      ',' }, /* msvcp60 doesn't support grouping */
        { "1,000", "German",     IOSTATE_eofbit,   1.0,      EOF },

        /* extra digits and stuff */
        { "00000.123456", NULL,  IOSTATE_eofbit,  0.123456, EOF },
        { "0.1234560000", NULL,  IOSTATE_eofbit,  0.123456, EOF },
        { "100aaaa",      NULL,  IOSTATE_goodbit, 100.0,    'a' },

        /* exponent */
        { "10e10",       NULL,      IOSTATE_eofbit,    10e10,      EOF }, /* lowercase e */
        { "10E10",       NULL,      IOSTATE_eofbit,    10E10,      EOF }, /* uppercase E */
        { "10e+10",      NULL,      IOSTATE_eofbit,    10e10,      EOF }, /* sign */
        { "10e-10",      NULL,      IOSTATE_eofbit,    10e-10,     EOF },
        { "10.e10",      NULL,      IOSTATE_eofbit,    10e10,      EOF }, /* trailing decimal before exponent */
        { "-10.e-10",    NULL,      IOSTATE_eofbit,   -10e-10,     EOF },
        { "-12.345e-10", NULL,      IOSTATE_eofbit,   -12.345e-10, EOF },
        { "1,234e10",    NULL,      IOSTATE_goodbit,   1.0,        ',' },
        { "1,234e10",    "German",  IOSTATE_eofbit,    1.234e10,   EOF },
        { "1.0e999",     NULL,      IOSTATE_faileof,   42.0,       EOF }, /* too big   */
        { "1.0e-999",    NULL,      IOSTATE_faileof,   42.0,       EOF }, /* too small */

        /* bad form */
        { "1.0ee10", NULL,      IOSTATE_failbit, 42.0,  EOF }, /* dup exp */
        { "1.0e1.0", NULL,      IOSTATE_goodbit, 10.0,  '.' }, /* decimal in exponent */
        { "1.0e1,0", NULL,      IOSTATE_goodbit, 10.0,  ',' }, /* group in exponent */
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        if(tests[i].lcl) {
            call_func3(p_locale_ctor_cstr, &lcl, tests[i].lcl, 0x3f /* FIXME: support categories */);
            call_func3(p_basic_ios_char_imbue, &ss.basic_ios, &retlcl, &lcl);
        }

        val = 42.0;
        call_func2(p_basic_istream_char_read_double, &ss.base.base1, &val);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        if(tests[i].lcl && state==IOSTATE_badbit) {
            win_skip("strange behavior when non-C locale is used\n");
            continue;
        }

        ok(tests[i].state == state, "%d) wrong state, expected = %x found = %x\n", i, tests[i].state, state);
        ok(tests[i].val   == val,   "%d) wrong val, expected = %g found %g\n", i, tests[i].val, val);
        ok(tests[i].next  == next,  "%d) wrong next, expected = %c (%i) found = %c (%i)\n", i, tests[i].next, tests[i].next, next, next);

        if(tests[i].lcl)
            call_func1(p_locale_dtor, &lcl);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        if(tests[i].lcl) {
            call_func3(p_locale_ctor_cstr, &lcl, tests[i].lcl, 0x3f /* FIXME: support categories */);
            call_func3(p_basic_ios_wchar_imbue, &wss.basic_ios, &retlcl, &lcl);
        }

        val = 42.0;
        call_func2(p_basic_istream_wchar_read_double, &wss.base.base1, &val);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "%d) wrong state, expected = %x found = %x\n", i, tests[i].state, state);
        ok(tests[i].val == val, "%d) wrong val, expected = %g found %g\n", i, tests[i].val, val);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "%d) wrong next, expected = %c (%i) found = %c (%i)\n", i, testus, testus, nextus, nextus);

        if(tests[i].lcl)
            call_func1(p_locale_dtor, &lcl);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_num_put_put_double(void)
{
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar pwstr;
    basic_string_char pstr;
    locale lcl, retlcl;
    const wchar_t *wstr;
    const char *str;
    wchar_t wide[64];
    int i;

    struct _test_num_get {
        double        val;
        const char    *lcl;
        streamsize    prec;  /* set to -1 for default */
        IOSB_fmtflags fmtfl; /* FMTFLAG_scientific, FMTFLAG_fixed */
        const char    *str;
    } tests[] = {
        { 0.0, NULL, -1, 0, "0" },

        /* simple cases */
        { 0.123, NULL, -1, 0, "0.123" },
        { 0.123, NULL,  6, 0, "0.123" },
        { 0.123, NULL,  0, 0, "0.123" },

        /* fixed format */
        { 0.123, NULL, -1, FMTFLAG_fixed, "0.123000" },
        { 0.123, NULL,  6, FMTFLAG_fixed, "0.123000" },
        { 0.123, NULL,  0, FMTFLAG_fixed, "0" },

        /* scientific format */
        { 123456.789, NULL,    -1, FMTFLAG_scientific, "1.234568e+005"    },
        { 123456.789, NULL,     0, FMTFLAG_scientific, "1.234568e+005"    },
        { 123456.789, NULL,     9, FMTFLAG_scientific, "1.234567890e+005" },
        { 123456.789, "German", 9, FMTFLAG_scientific, "1,234567890e+005" },

        /* different locales */
        { 0.123, "C",       -1, 0, "0.123" },
        { 0.123, "English", -1, 0, "0.123" },
        { 0.123, "German",  -1, 0, "0,123" },

        { 123456.789, "C",       -1, 0, "123457"  },
        { 123456.789, "English", -1, 0, "123457" },
        { 123456.789, "German",  -1, 0, "123457" },

        /* signs and exponents */
        {  1.0e-9, NULL, -1, 0, "1e-009"  },
        {  1.0e-9, NULL,  9, 0, "1e-009"  },
        { -1.0e9,  NULL, -1, 0, "-1e+009" },
        { -1.0e9,  NULL,  9, 0, "-1e+009" },

        {  1.0e-9, NULL, 0, FMTFLAG_fixed, "0"                  },
        {  1.0e-9, NULL, 6, FMTFLAG_fixed, "0.000000"           },
        {  1.0e-9, NULL, 9, FMTFLAG_fixed, "0.000000001"        },
        { -1.0e9,  NULL, 0, FMTFLAG_fixed, "-1000000000"        },
        { -1.0e9,  NULL, 6, FMTFLAG_fixed, "-1000000000.000000" },

        { -1.23456789e9,  NULL, 0, 0,             "-1.23457e+009"      },
        { -1.23456789e9,  NULL, 0, FMTFLAG_fixed, "-1234567890"        },
        { -1.23456789e9,  NULL, 6, FMTFLAG_fixed, "-1234567890.000000" },
        { -1.23456789e-9, NULL, 6, FMTFLAG_fixed, "-0.000000"          },
        { -1.23456789e-9, NULL, 9, FMTFLAG_fixed, "-0.000000001"       }
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_stringstream_char_ctor_mode, &ss, OPENMODE_in|OPENMODE_out, TRUE);

        if(tests[i].lcl) {
            call_func3(p_locale_ctor_cstr, &lcl, tests[i].lcl, 0x3f /* FIXME: support categories */);
            call_func3(p_basic_ios_char_imbue, &ss.basic_ios, &retlcl, &lcl);
        }

        /* set format and precision only if specified, so we can try defaults */
        if(tests[i].fmtfl)
            call_func3(p_ios_base_setf_mask, &ss.basic_ios.base, tests[i].fmtfl, FMTFLAG_floatfield);
        if(tests[i].prec != -1)
            call_func2(p_ios_base_precision_set, &ss.basic_ios.base, tests[i].prec);
        call_func2_ptr_dbl(p_basic_ostream_char_print_double, &ss.base.base2, tests[i].val);

        call_func2(p_basic_stringstream_char_str_get, &ss, &pstr);
        str = call_func1(p_basic_string_char_cstr, &pstr);

        ok(!strcmp(tests[i].str, str), "wrong output, expected = %s found = %s\n", tests[i].str, str);
        call_func1(p_basic_string_char_dtor, &pstr);

        if(tests[i].lcl)
            call_func1(p_locale_dtor, &lcl);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);

        /* wchar_t version */
        call_func3(p_basic_stringstream_wchar_ctor_mode, &wss, OPENMODE_in|OPENMODE_out, TRUE);

        if(tests[i].lcl) {
            call_func3(p_locale_ctor_cstr, &lcl, tests[i].lcl, 0x3f /* FIXME: support categories */);
            call_func3(p_basic_ios_wchar_imbue, &wss.basic_ios, &retlcl, &lcl);
        }

        /* set format and precision only if specified, so we can try defaults */
        if(tests[i].fmtfl)
            call_func3(p_ios_base_setf_mask, &wss.basic_ios.base, tests[i].fmtfl, FMTFLAG_floatfield);
        if(tests[i].prec != -1)
            call_func2(p_ios_base_precision_set, &wss.basic_ios.base, tests[i].prec);
        call_func2_ptr_dbl(p_basic_ostream_wchar_print_double, &wss.base.base2, tests[i].val);

        call_func2(p_basic_stringstream_wchar_str_get, &wss, &pwstr);
        wstr = call_func1(p_basic_string_wchar_cstr, &pwstr);

        AtoW(wide, tests[i].str, strlen(tests[i].str));
        ok(!lstrcmpW(wide, wstr), "wrong output, expected = %s found = %s\n", tests[i].str, wine_dbgstr_w(wstr));
        call_func1(p_basic_string_wchar_dtor, &pwstr);

        if(tests[i].lcl)
            call_func1(p_locale_dtor, &lcl);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
    }
}


static void test_istream_ipfx(void)
{
    unsigned short testus, nextus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    wchar_t wide[64];
    int i, next;
    MSVCP_bool ret;

    struct _test_istream_ipfx {
        const char  *str;
        int          unset_skipws;
        int          noskip;
        MSVCP_bool   ret;
        IOSB_iostate state;
        int          next;
    } tests[] = {
        /* string       unset  noskip return state            next char */
        { "",           FALSE, FALSE, TRUE,  IOSTATE_goodbit, EOF  }, /* empty string */
        { "   ",        FALSE, FALSE, TRUE,  IOSTATE_goodbit, EOF  }, /* just ws */
        { "\t \n \f ",  FALSE, FALSE, TRUE,  IOSTATE_goodbit, EOF  }, /* different ws */
        { "simple",     FALSE, FALSE, TRUE,  IOSTATE_goodbit, 's'  },
        { "  simple",   FALSE, FALSE, TRUE,  IOSTATE_goodbit, 's'  },
        { "  simple",   TRUE,  FALSE, TRUE,  IOSTATE_goodbit, ' '  }, /* unset skipws */
        { "  simple",   FALSE, TRUE,  TRUE,  IOSTATE_goodbit, ' '  }, /* ipfx(true) */
        { "  simple",   TRUE,  TRUE,  TRUE,  IOSTATE_goodbit, ' '  }, /* both */
        { "\n\t ws",    FALSE, FALSE, TRUE,  IOSTATE_goodbit, 'w'  },
        { "\n\t ws",    TRUE,  FALSE, TRUE,  IOSTATE_goodbit, '\n' },
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        /* set format and precision only if specified, so we can try defaults */
        if(tests[i].unset_skipws)
            call_func2(p_ios_base_unsetf, &ss.basic_ios.base, TRUE);

        ret   = (MSVCP_bool)(INT_PTR)call_func2(p_basic_istream_char_ipfx, &ss.base.base1, tests[i].noskip);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        ok(tests[i].ret   == ret,   "%d) wrong return, expected = %i found = %i\n", i, tests[i].ret, ret);
        ok(tests[i].state == state, "%d) wrong state, expected = %x found = %x\n", i, tests[i].state, state);
        ok(tests[i].next  == next,  "%d) wrong next, expected = %c (%i) found = %c (%i)\n", i, tests[i].next, tests[i].next, next, next);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        /* set format and precision only if specified, so we can try defaults */
        if(tests[i].unset_skipws)
            call_func2(p_ios_base_unsetf, &wss.basic_ios.base, TRUE);

        ret    = (MSVCP_bool)(INT_PTR)call_func2(p_basic_istream_wchar_ipfx, &wss.base.base1, tests[i].noskip);
        state  = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        ok(tests[i].ret == ret, "wrong return, expected = %i found = %i\n", tests[i].ret, ret);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "wrong next, expected = %c (%i) found = %c (%i)\n", testus, testus, nextus, nextus);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_istream_ignore(void)
{
    unsigned short testus, nextus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    wchar_t wide[64];
    int i, next;

    struct _test_istream_ignore {
        const char  *str;
        streamsize   count;
        int          delim;
        IOSB_iostate state;
        int          next;
    } tests[] = {
        /* string       count delim state            next */
        { "",           0,    '\n', IOSTATE_goodbit, EOF }, /* empty string */

        /* different counts */
        { "ABCDEF",     2,    '\n', IOSTATE_goodbit, 'C' }, /* easy case */
        { "ABCDEF",     42,   '\n', IOSTATE_eofbit,  EOF }, /* ignore too much */
        { "ABCDEF",    -2,    '\n', IOSTATE_goodbit, 'A' }, /* negative count */
        { "ABCDEF",     6,    '\n', IOSTATE_goodbit, EOF }, /* is eof not set at end */
        { "ABCDEF",     7,    '\n', IOSTATE_eofbit,  EOF }, /* eof is set just after end */

        /* different delimiters */
        { "ABCDEF",       42, '\0', IOSTATE_eofbit,  EOF }, /* null as delim */
        { "ABC DEF GHI",  0,  ' ',  IOSTATE_goodbit, 'A' },
        { "ABC DEF GHI",  42, ' ',  IOSTATE_goodbit, 'D' },
        { "ABC DEF\tGHI", 42, '\t', IOSTATE_goodbit, 'G' },
        { "ABC ",         42, ' ',  IOSTATE_goodbit, EOF }, /* delim at end */
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        call_func3(p_basic_istream_char_ignore, &ss.base.base1, tests[i].count, tests[i].delim);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        ok(tests[i].next  == next,  "wrong next, expected = %c (%i) found = %c (%i)\n", tests[i].next, tests[i].next, next, next);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        call_func3(p_basic_istream_wchar_ignore, &wss.base.base1, tests[i].count, tests[i].delim);
        state  = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "wrong next, expected = %c (%i) found = %c (%i)\n", testus, testus, nextus, nextus);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_istream_seekg(void)
{
    unsigned short testus, nextus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    wchar_t wide[64];
    int i, next;

    struct _test_istream_seekg {
        const char  *str;
        streamoff    off;
        IOSB_seekdir dir;
        IOSB_iostate state;
        int          next;
    } tests[] = {
        { "ABCDEFGHIJ",  0, SEEKDIR_beg, IOSTATE_goodbit, 'A' },
        { "ABCDEFGHIJ",  1, SEEKDIR_beg, IOSTATE_goodbit, 'B' },
        { "ABCDEFGHIJ",  5, SEEKDIR_cur, IOSTATE_goodbit, 'F' },
        { "ABCDEFGHIJ", -3, SEEKDIR_end, IOSTATE_goodbit, 'H' },

        /* bad offsets */
        { "ABCDEFGHIJ", -1, SEEKDIR_beg, IOSTATE_goodbit, 'A' },
        { "ABCDEFGHIJ", 42, SEEKDIR_cur, IOSTATE_goodbit, 'A' },
        { "ABCDEFGHIJ", 42, SEEKDIR_end, IOSTATE_goodbit, 'A' },
        { "",            0, SEEKDIR_beg, IOSTATE_goodbit, EOF },
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        call_func3(p_basic_istream_char_seekg, &ss.base.base1, tests[i].off, tests[i].dir);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        ok(tests[i].next  == next,  "wrong next, expected = %c (%i) found = %c (%i)\n", tests[i].next, tests[i].next, next, next);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        call_func3(p_basic_istream_wchar_seekg, &wss.base.base1, tests[i].off, tests[i].dir);
        state  = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "wrong next, expected = %c (%i) found = %c (%i)\n", testus, testus, nextus, nextus);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_istream_seekg_fpos(void)
{
    unsigned short testus, nextus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    wchar_t wide[64];
    fpos_int pos;
    int i, next;

    struct _test_istream_seekg_fpos {
        const char  *str;
        streamoff    off;
        IOSB_iostate state;
        int          next;
    } tests[] = {
        { "ABCDEFGHIJ", 0,  IOSTATE_goodbit, 'A' },
        { "ABCDEFGHIJ", 9,  IOSTATE_goodbit, 'J' },
        { "ABCDEFGHIJ", 10, IOSTATE_goodbit, EOF }, /* beyond end, but still good */
        { "ABCDEFGHIJ", -1, IOSTATE_goodbit, 'A' },
        { "",           0,  IOSTATE_goodbit, EOF },
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        pos.off   = tests[i].off;
        pos.pos   = 0; /* FIXME: a later patch will test this with filebuf */
        pos.state = 0;
        call_func2_ptr_fpos(p_basic_istream_char_seekg_fpos, &ss.base.base1, pos);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        ok(tests[i].next  == next,  "wrong next, expected = %c (%i) found = %c (%i)\n", tests[i].next, tests[i].next, next, next);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        pos.off   = tests[i].off;
        pos.pos   = 0; /* FIXME: a later patch will test this with filebuf */
        pos.state = 0;
        call_func2_ptr_fpos(p_basic_istream_wchar_seekg_fpos, &wss.base.base1, pos);
        state  = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "wrong next, expected = %c (%i) found = %c (%i)\n", testus, testus, nextus, nextus);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_istream_peek(void)
{
    unsigned short testus, nextus, peekus;
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    int i, next, peek;
    wchar_t wide[64];

    struct _test_istream_peek {
        const char  *str;
        int          peek;
        int          next;
        IOSB_iostate state;
    } tests[] = {
        { "",       EOF, EOF, IOSTATE_eofbit  },
        { "ABCDEF", 'A', 'A', IOSTATE_goodbit },
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        peek  = (int)call_func1(p_basic_istream_char_peek, &ss.base.base1);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        next  = (int)call_func1(p_basic_istream_char_get, &ss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        ok(tests[i].next  == next,  "wrong next, expected = %c (%i) found = %c (%i)\n", tests[i].next, tests[i].next, next, next);
        ok(peek == next, "wrong peek, expected %c (%i) found = %c (%i)\n", peek, peek, next, next);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        peekus = (unsigned short)(int)call_func1(p_basic_istream_wchar_peek, &wss.base.base1);
        state  = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        nextus = (unsigned short)(int)call_func1(p_basic_istream_wchar_get, &wss.base.base1);

        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);
        testus = tests[i].next == EOF ? WEOF : (unsigned short)tests[i].next;
        ok(testus == nextus, "wrong next, expected = %c (%i) found = %c (%i)\n", testus, testus, nextus, nextus);
        ok(peekus == nextus, "wrong peek, expected %c (%i) found = %c (%i)\n", peekus, peekus, nextus, nextus);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}


static void test_istream_tellg(void)
{
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_fstream_wchar wfs;
    basic_fstream_char fs;
    basic_string_wchar wstr;
    basic_string_char str;
    fpos_int spos, tpos, *rpos;
    wchar_t wide[64];
    FILE *file;
    int i;

    const char *testfile = "file.txt";

    struct _test_istream_tellg_fpos {
        const char  *str;
        streamoff    seekoff;
        streamoff    telloff_ss; /* offset for stringstream */
        streamoff    telloff_fs; /* offset for fstream */
        __int64      tellpos;
    } tests[] = {
        /* empty strings */
        { "", -1, -1,  0,  0 }, /* tellg on defaults */
        { "",  0, -1,  0,  0 }, /* tellg after seek 0 */
        { "", 42, -1,  0, 42 }, /* tellg after seek beyond end */
        { "", -6, -1,  0,  0 }, /* tellg after seek beyond beg */

        /* non-empty strings */
        { "ABCDEFGHIJ", -1,  0,  0,  0 },
        { "ABCDEFGHIJ",  3,  3,  0,  3 },
        { "ABCDEFGHIJ", 42,  0,  0, 42 },
        { "ABCDEFGHIJ", -6,  0,  0,  0 }
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* stringstream<char> version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        spos.off   = tests[i].seekoff;
        spos.pos   = 0;
        spos.state = 0;

        tpos.off   = 0xdeadbeef;
        tpos.pos   = 0xdeadbeef;
        tpos.state = 0xdeadbeef;

        if (tests[i].seekoff != -1) /* to test without seek */
            call_func2_ptr_fpos(p_basic_istream_char_seekg_fpos, &ss.base.base1, spos);
        rpos = call_func2(p_basic_istream_char_tellg, &ss.base.base1, &tpos);

        ok(tests[i].telloff_ss == tpos.off, "wrong offset, expected = %Id found = %Id\n", tests[i].telloff_ss, tpos.off);
        ok(rpos == &tpos, "wrong return fpos, expected = %p found = %p\n", rpos, &tpos);
        ok(tpos.pos == 0, "wrong position, expected = 0 found = %s\n", wine_dbgstr_longlong(tpos.pos));
        ok(tpos.state == 0, "wrong state, expected = 0 found = %d\n", tpos.state);
        if(tests[i].seekoff == -1) {
            ok(ss.basic_ios.base.state == IOSTATE_goodbit,
                    "ss.basic_ios.base.state = %x\n", ss.basic_ios.base.state);
        }

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* stringstream<wchar_t> version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        spos.off   = tests[i].seekoff;
        spos.pos   = 0; /* FIXME: a later patch will test this with filebuf */
        spos.state = 0;

        tpos.off   = 0xdeadbeef;
        tpos.pos   = 0xdeadbeef;
        tpos.state = 0xdeadbeef;

        if (tests[i].seekoff != -1) /* to test without seek */
            call_func2_ptr_fpos(p_basic_istream_wchar_seekg_fpos, &wss.base.base1, spos);
        rpos = call_func2(p_basic_istream_wchar_tellg, &wss.base.base1, &tpos);

        ok(tests[i].telloff_ss == tpos.off, "wrong offset, expected = %Id found = %Id\n", tests[i].telloff_ss, tpos.off);
        ok(rpos == &tpos, "wrong return fpos, expected = %p found = %p\n", rpos, &tpos);
        ok(tpos.pos == 0, "wrong position, expected = 0 found = %s\n", wine_dbgstr_longlong(tpos.pos));
        ok(tpos.state == 0, "wrong state, expected = 0 found = %d\n", tpos.state);
        if(tests[i].seekoff == -1) {
            ok(ss.basic_ios.base.state == IOSTATE_goodbit,
                    "ss.basic_ios.base.state = %x\n", ss.basic_ios.base.state);
        }

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);

        /* filebuf */
        file = fopen(testfile, "wt");
        fprintf(file, "%s", tests[i].str);
        fclose(file);

        /* fstream<char> version */
        call_func4(p_basic_fstream_char_ctor_name, &fs, testfile, OPENMODE_out|OPENMODE_in, TRUE);

        spos.off   = tests[i].seekoff;
        spos.pos   = 0;
        spos.state = 0;

        tpos.off   = 0xdeadbeef;
        tpos.pos   = 0xdeadbeef;
        tpos.state = 0xdeadbeef;

        if (tests[i].seekoff != -1) /* to test without seek */
            call_func2_ptr_fpos(p_basic_istream_char_seekg_fpos, &fs.base.base1, spos);
        rpos = call_func2(p_basic_istream_char_tellg, &fs.base.base1, &tpos);

        ok(tests[i].tellpos == tpos.pos, "wrong filepos, expected = %s found = %s\n",
            wine_dbgstr_longlong(tests[i].tellpos), wine_dbgstr_longlong(tpos.pos));
        ok(rpos == &tpos, "wrong return fpos, expected = %p found = %p\n", rpos, &tpos);
        ok(tpos.off == tests[i].telloff_fs, "wrong offset, expected %Id found %Id\n", tests[i].telloff_fs, tpos.off);
        ok(tpos.state == 0, "wrong state, expected = 0 found = %d\n", tpos.state);

        call_func1(p_basic_fstream_char_vbase_dtor, &fs);

        /* fstream<wchar_t> version */
        call_func4(p_basic_fstream_wchar_ctor_name, &wfs, testfile, OPENMODE_out|OPENMODE_in, TRUE);

        spos.off   = tests[i].seekoff;
        spos.pos   = 0;
        spos.state = 0;

        tpos.off   = 0xdeadbeef;
        tpos.pos   = 0xdeadbeef;
        tpos.state = 0xdeadbeef;

        if (tests[i].seekoff != -1) /* to test without seek */
            call_func2_ptr_fpos(p_basic_istream_wchar_seekg_fpos, &wfs.base.base1, spos);
        rpos = call_func2(p_basic_istream_wchar_tellg, &wfs.base.base1, &tpos);

        ok(tests[i].tellpos == tpos.pos, "wrong filepos, expected = %s found = %s\n",
            wine_dbgstr_longlong(tests[i].tellpos), wine_dbgstr_longlong(tpos.pos));
        ok(rpos == &tpos, "wrong return fpos, expected = %p found = %p\n", rpos, &tpos);
        ok(tpos.off == tests[i].telloff_fs, "wrong offset, expected %Id found %Id\n", tests[i].telloff_fs, tpos.off);
        ok(tpos.state == 0, "wrong state, expected = 0 found = %d\n", tpos.state);

        call_func1(p_basic_fstream_wchar_vbase_dtor, &wfs);

        unlink(testfile);
    }
}


static void test_istream_getline(void)
{
    basic_stringstream_wchar wss;
    basic_stringstream_char ss;
    basic_string_wchar wstr;
    basic_string_char str;
    IOSB_iostate state;
    wchar_t wide[64];
    int i;
    const char *cstr;
    const wchar_t *wcstr;

    /* makes tables narrower */
    const IOSB_iostate IOSTATE_faileof = IOSTATE_failbit|IOSTATE_eofbit;

    struct _test_istream_getline {
        const char  *str;
        const char  *line;
        int          delim;
        IOSB_iostate state;
        const char  *nextline;
        int          nextdelim;
        IOSB_iostate nextstate;
    } tests[] = {
        { "", "", '\n', IOSTATE_faileof, "", '\n', IOSTATE_faileof },

        { "this\n",                 "this", '\n', IOSTATE_goodbit, "",   '\n', IOSTATE_faileof },
        { "this\nis\nsome\ntext\n", "this", '\n', IOSTATE_goodbit, "is", '\n', IOSTATE_goodbit },

        { "this is some text\n", "this", ' ',  IOSTATE_goodbit, "is",           ' ',  IOSTATE_goodbit },
        { "this is some text\n", "this", ' ',  IOSTATE_goodbit, "is some text", '\n', IOSTATE_goodbit },

        { "this is some text\n", "this is some text",   '\n', IOSTATE_goodbit, "",                    '\n', IOSTATE_faileof },
        { "this is some text\n", "this is some text\n", '\0', IOSTATE_eofbit,  "", '\n', IOSTATE_faileof },
    };

    if(!p_basic_istream_char_getline_bstr_delim || !p_basic_istream_wchar_getline_bstr_delim) {
        win_skip("basic_istream::getline(basic_string, delim) is not available\n");
        return;
    }

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        /* char version */
        call_func3(p_basic_string_char_ctor_cstr_alloc, &str, tests[i].str, &fake_allocator);
        call_func4(p_basic_stringstream_char_ctor_str, &ss, &str, OPENMODE_out|OPENMODE_in, TRUE);

        p_basic_istream_char_getline_bstr_delim(&ss.base.base1, &str, tests[i].delim);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        cstr  = call_func1(p_basic_string_char_cstr, &str);

        ok(!strcmp(tests[i].line, cstr), "wrong line, expected = %s found = %s\n", tests[i].line, cstr);
        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);

        /* next line */
        p_basic_istream_char_getline_bstr_delim(&ss.base.base1, &str, tests[i].nextdelim);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &ss.basic_ios.base);
        cstr  = call_func1(p_basic_string_char_cstr, &str);

        ok(!strcmp(tests[i].nextline, cstr), "wrong next line, expected = %s found = %s\n", tests[i].nextline, cstr);
        ok(tests[i].nextstate == state, "wrong next state, expected = %x found = %x\n", tests[i].nextstate, state);

        call_func1(p_basic_stringstream_char_vbase_dtor, &ss);
        call_func1(p_basic_string_char_dtor, &str);

        /* wchar_t version */
        AtoW(wide, tests[i].str, strlen(tests[i].str));
        call_func3(p_basic_string_wchar_ctor_cstr_alloc, &wstr, wide, &fake_allocator);
        call_func4(p_basic_stringstream_wchar_ctor_str, &wss, &wstr, OPENMODE_out|OPENMODE_in, TRUE);

        p_basic_istream_wchar_getline_bstr_delim(&wss.base.base1, &wstr, tests[i].delim);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        wcstr = call_func1(p_basic_string_wchar_cstr, &wstr);

        AtoW(wide, tests[i].line, strlen(tests[i].line));
        ok(!lstrcmpW(wide, wcstr), "wrong line, expected = %s found = %s\n", tests[i].line, wine_dbgstr_w(wcstr));
        ok(tests[i].state == state, "wrong state, expected = %x found = %x\n", tests[i].state, state);

        /* next line */
        p_basic_istream_wchar_getline_bstr_delim(&wss.base.base1, &wstr, tests[i].nextdelim);
        state = (IOSB_iostate)call_func1(p_ios_base_rdstate, &wss.basic_ios.base);
        wcstr = call_func1(p_basic_string_wchar_cstr, &wstr);

        AtoW(wide, tests[i].nextline, strlen(tests[i].nextline));
        ok(!lstrcmpW(wide, wcstr), "wrong line, expected = %s found = %s\n", tests[i].nextline, wine_dbgstr_w(wcstr));
        ok(tests[i].nextstate == state, "wrong state, expected = %x found = %x\n", tests[i].nextstate, state);

        call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
        call_func1(p_basic_string_wchar_dtor, &wstr);
    }
}

static void test_ostream_print_ushort(void)
{
    basic_stringstream_wchar wss;
    basic_string_wchar pwstr;
    const wchar_t *wstr;

    call_func3(p_basic_stringstream_wchar_ctor_mode, &wss, OPENMODE_in|OPENMODE_out, TRUE);
    call_func2(p_basic_ostream_short_print_ushort, &wss.base.base2, 65);

    call_func2(p_basic_stringstream_wchar_str_get, &wss, &pwstr);
    wstr = call_func1(p_basic_string_wchar_cstr, &pwstr);
    ok(!lstrcmpW(L"65", wstr), "wstr = %s\n", wine_dbgstr_w(wstr));

    call_func1(p_basic_string_wchar_dtor, &pwstr);
    call_func1(p_basic_stringstream_wchar_vbase_dtor, &wss);
}

static struct {
    int value[2];
    const char* export_name;
} vbtable_size_exports_list[] = {
    {{0x60, 0xb0}, "??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x58, 0xa0}, "??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x60, 0xb0}, "??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x58, 0xa0}, "??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x5c, 0xa8}, "??_8?$basic_ifstream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x5c, 0xa8}, "??_8?$basic_ifstream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0xc, 0x18}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{ 0x4,  0x8}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{ 0xc, 0x18}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{ 0x4,  0x8}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{ 0x8, 0x10}, "??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@"},
    {{0x58, 0xa0}, "??_8?$basic_ofstream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x58, 0xa0}, "??_8?$basic_ofstream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x4,  0x8}, "??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x4,  0x8}, "??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x50, 0x90}, "??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@"},
    {{0x50, 0x90}, "??_8?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@"},
    {{0x58, 0xa0}, "??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x50, 0x90}, "??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x58, 0xa0}, "??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x50, 0x90}, "??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x00, 0x00}, 0}
};

static void test_vbtable_size_exports(void)
{
    int i;
    const int *p_vbtable;
    int arch_idx = (sizeof(void*) == 8);

    for (i = 0; vbtable_size_exports_list[i].export_name; i++)
    {
        SET(p_vbtable, vbtable_size_exports_list[i].export_name);

        ok(p_vbtable[0] == 0, "vbtable[0] wrong, got 0x%x\n", p_vbtable[0]);
        ok(p_vbtable[1] == vbtable_size_exports_list[i].value[arch_idx],
                "%d: %s[1] wrong, got 0x%x\n", i, vbtable_size_exports_list[i].export_name, p_vbtable[1]);
    }
}


START_TEST(ios)
{
    if(!init())
        return;

    test_num_get_get_double();
    test_num_put_put_double();
    test_istream_ipfx();
    test_istream_ignore();
    test_istream_seekg();
    test_istream_seekg_fpos();
    test_istream_peek();
    test_istream_tellg();
    test_istream_getline();
    test_ostream_print_ushort();

    test_vbtable_size_exports();

    FreeLibrary(msvcp);
}
