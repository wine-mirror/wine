/*
 * Copyright 2015-2016 Iván Matellanes
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

#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <io.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <sys/stat.h>
#include "wine/test.h"

typedef void (*vtable_ptr)(void);
typedef LONG streamoff;
typedef LONG streampos;
typedef int filedesc;
typedef void* (__cdecl *allocFunction)(LONG);
typedef void (__cdecl *freeFunction)(void*);

typedef enum {
    IOSTATE_goodbit   = 0x0,
    IOSTATE_eofbit    = 0x1,
    IOSTATE_failbit   = 0x2,
    IOSTATE_badbit    = 0x4
} ios_io_state;

typedef enum {
    OPENMODE_in          = 0x1,
    OPENMODE_out         = 0x2,
    OPENMODE_ate         = 0x4,
    OPENMODE_app         = 0x8,
    OPENMODE_trunc       = 0x10,
    OPENMODE_nocreate    = 0x20,
    OPENMODE_noreplace   = 0x40,
    OPENMODE_binary      = 0x80
} ios_open_mode;

typedef enum {
    SEEKDIR_beg = 0,
    SEEKDIR_cur = 1,
    SEEKDIR_end = 2
} ios_seek_dir;

typedef enum {
    FLAGS_skipws     = 0x1,
    FLAGS_left       = 0x2,
    FLAGS_right      = 0x4,
    FLAGS_internal   = 0x8,
    FLAGS_dec        = 0x10,
    FLAGS_oct        = 0x20,
    FLAGS_hex        = 0x40,
    FLAGS_showbase   = 0x80,
    FLAGS_showpoint  = 0x100,
    FLAGS_uppercase  = 0x200,
    FLAGS_showpos    = 0x400,
    FLAGS_scientific = 0x800,
    FLAGS_fixed      = 0x1000,
    FLAGS_unitbuf    = 0x2000,
    FLAGS_stdio      = 0x4000
} ios_flags;

const int filebuf_sh_none = 0x800;
const int filebuf_sh_read = 0xa00;
const int filebuf_sh_write = 0xc00;
const int filebuf_openprot = 420;
const int filebuf_binary = _O_BINARY;
const int filebuf_text = _O_TEXT;

/* class streambuf */
typedef struct {
    const vtable_ptr *vtable;
    int allocated;
    int unbuffered;
    int stored_char;
    char *base;
    char *ebuf;
    char *pbase;
    char *pptr;
    char *epptr;
    char *eback;
    char *gptr;
    char *egptr;
    int do_lock;
    CRITICAL_SECTION lock;
} streambuf;

/* class filebuf */
typedef struct {
    streambuf base;
    filedesc fd;
    int close;
} filebuf;

/* class strstreambuf */
typedef struct {
    streambuf base;
    int dynamic;
    int increase;
    int unknown;
    int constant;
    allocFunction f_alloc;
    freeFunction f_free;
} strstreambuf;

/* class stdiobuf */
typedef struct {
    streambuf base;
    FILE *file;
} stdiobuf;

/* class ios */
struct _ostream;
typedef struct {
    const vtable_ptr *vtable;
    streambuf *sb;
    ios_io_state state;
    int special[4];
    int delbuf;
    struct _ostream *tie;
    ios_flags flags;
    int precision;
    char fill;
    int width;
    int do_lock;
    CRITICAL_SECTION lock;
} ios;

/* class ostream */
typedef struct _ostream {
    const int *vbtable;
    int unknown;
    ios base_ios; /* virtually inherited */
} ostream;

/* class istream */
typedef struct {
    const int *vbtable;
    int extract_delim;
    int count;
    ios base_ios; /* virtually inherited */
} istream;

/* class iostream */
typedef struct {
    struct { /* istream */
        const int *vbtable;
        int extract_delim;
        int count;
    } base1;
    struct { /* ostream */
        const int *vbtable;
        int unknown;
    } base2;
    ios base_ios; /* virtually inherited */
} iostream;

/* class exception */
typedef struct {
    const void *vtable;
    char *name;
    int do_free;
} exception;

/* class logic_error */
typedef struct {
    exception e;
} logic_error;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

static void* (__cdecl *p_operator_new)(unsigned int);
static void (__cdecl *p_operator_delete)(void*);

/* streambuf */
static streambuf* (*__thiscall p_streambuf_reserve_ctor)(streambuf*, char*, int);
static streambuf* (*__thiscall p_streambuf_ctor)(streambuf*);
static void (*__thiscall p_streambuf_dtor)(streambuf*);
static int (*__thiscall p_streambuf_allocate)(streambuf*);
static void (*__thiscall p_streambuf_clrclock)(streambuf*);
static int (*__thiscall p_streambuf_doallocate)(streambuf*);
static void (*__thiscall p_streambuf_gbump)(streambuf*, int);
static int (*__thiscall p_streambuf_in_avail)(const streambuf*);
static void (*__thiscall p_streambuf_lock)(streambuf*);
static int (*__thiscall p_streambuf_out_waiting)(const streambuf*);
static int (*__thiscall p_streambuf_pbackfail)(streambuf*, int);
static void (*__thiscall p_streambuf_pbump)(streambuf*, int);
static int (*__thiscall p_streambuf_sbumpc)(streambuf*);
static void (*__thiscall p_streambuf_setb)(streambuf*, char*, char*, int);
static void (*__thiscall p_streambuf_setlock)(streambuf*);
static streambuf* (*__thiscall p_streambuf_setbuf)(streambuf*, char*, int);
static int (*__thiscall p_streambuf_sgetc)(streambuf*);
static int (*__thiscall p_streambuf_snextc)(streambuf*);
static int (*__thiscall p_streambuf_sputc)(streambuf*, int);
static void (*__thiscall p_streambuf_stossc)(streambuf*);
static int (*__thiscall p_streambuf_sync)(streambuf*);
static void (*__thiscall p_streambuf_unlock)(streambuf*);
static int (*__thiscall p_streambuf_xsgetn)(streambuf*, char*, int);
static int (*__thiscall p_streambuf_xsputn)(streambuf*, const char*, int);

/* filebuf */
static filebuf* (*__thiscall p_filebuf_fd_ctor)(filebuf*, int);
static filebuf* (*__thiscall p_filebuf_fd_reserve_ctor)(filebuf*, int, char*, int);
static filebuf* (*__thiscall p_filebuf_ctor)(filebuf*);
static void (*__thiscall p_filebuf_dtor)(filebuf*);
static filebuf* (*__thiscall p_filebuf_attach)(filebuf*, filedesc);
static filebuf* (*__thiscall p_filebuf_open)(filebuf*, const char*, ios_open_mode, int);
static filebuf* (*__thiscall p_filebuf_close)(filebuf*);
static int (*__thiscall p_filebuf_setmode)(filebuf*, int);
static streambuf* (*__thiscall p_filebuf_setbuf)(filebuf*, char*, int);
static int (*__thiscall p_filebuf_sync)(filebuf*);
static int (*__thiscall p_filebuf_overflow)(filebuf*, int);
static int (*__thiscall p_filebuf_underflow)(filebuf*);
static streampos (*__thiscall p_filebuf_seekoff)(filebuf*, streamoff, ios_seek_dir, int);
static int (*__thiscall p_filebuf_is_open)(filebuf*);

/* strstreambuf */
static strstreambuf* (*__thiscall p_strstreambuf_dynamic_ctor)(strstreambuf*, int);
static strstreambuf* (*__thiscall p_strstreambuf_funcs_ctor)(strstreambuf*, allocFunction, freeFunction);
static strstreambuf* (*__thiscall p_strstreambuf_buffer_ctor)(strstreambuf*, char*, int, char*);
static strstreambuf* (*__thiscall p_strstreambuf_ubuffer_ctor)(strstreambuf*, unsigned char*, int, unsigned char*);
static strstreambuf* (*__thiscall p_strstreambuf_ctor)(strstreambuf*);
static void (*__thiscall p_strstreambuf_dtor)(strstreambuf*);
static int (*__thiscall p_strstreambuf_doallocate)(strstreambuf*);
static void (*__thiscall p_strstreambuf_freeze)(strstreambuf*, int);
static int (*__thiscall p_strstreambuf_overflow)(strstreambuf*, int);
static streampos (*__thiscall p_strstreambuf_seekoff)(strstreambuf*, streamoff, ios_seek_dir, int);
static streambuf* (*__thiscall p_strstreambuf_setbuf)(strstreambuf*, char*, int);
static int (*__thiscall p_strstreambuf_underflow)(strstreambuf*);

/* stdiobuf */
static stdiobuf* (*__thiscall p_stdiobuf_file_ctor)(stdiobuf*, FILE*);
static void (*__thiscall p_stdiobuf_dtor)(stdiobuf*);
static int (*__thiscall p_stdiobuf_overflow)(stdiobuf*, int);
static int (*__thiscall p_stdiobuf_pbackfail)(stdiobuf*, int);
static streampos (*__thiscall p_stdiobuf_seekoff)(stdiobuf*, streamoff, ios_seek_dir, int);
static int (*__thiscall p_stdiobuf_setrwbuf)(stdiobuf*, int, int);
static int (*__thiscall p_stdiobuf_sync)(stdiobuf*);
static int (*__thiscall p_stdiobuf_underflow)(stdiobuf*);

/* ios */
static ios* (*__thiscall p_ios_copy_ctor)(ios*, const ios*);
static ios* (*__thiscall p_ios_ctor)(ios*);
static ios* (*__thiscall p_ios_sb_ctor)(ios*, streambuf*);
static ios* (*__thiscall p_ios_assign)(ios*, const ios*);
static void (*__thiscall p_ios_init)(ios*, streambuf*);
static void (*__thiscall p_ios_dtor)(ios*);
static void (*__cdecl p_ios_clrlock)(ios*);
static void (*__cdecl p_ios_setlock)(ios*);
static void (*__cdecl p_ios_lock)(ios*);
static void (*__cdecl p_ios_unlock)(ios*);
static void (*__cdecl p_ios_lockbuf)(ios*);
static void (*__cdecl p_ios_unlockbuf)(ios*);
static CRITICAL_SECTION *p_ios_static_lock;
static void (*__cdecl p_ios_lockc)(void);
static void (*__cdecl p_ios_unlockc)(void);
static LONG (*__thiscall p_ios_flags_set)(ios*, LONG);
static LONG (*__thiscall p_ios_flags_get)(const ios*);
static LONG (*__thiscall p_ios_setf)(ios*, LONG);
static LONG (*__thiscall p_ios_setf_mask)(ios*, LONG, LONG);
static LONG (*__thiscall p_ios_unsetf)(ios*, LONG);
static int (*__thiscall p_ios_good)(const ios*);
static int (*__thiscall p_ios_bad)(const ios*);
static int (*__thiscall p_ios_eof)(const ios*);
static int (*__thiscall p_ios_fail)(const ios*);
static void (*__thiscall p_ios_clear)(ios*, int);
static LONG *p_ios_maxbit;
static LONG (*__cdecl p_ios_bitalloc)(void);
static int *p_ios_curindex;
static LONG *p_ios_statebuf;
static LONG* (*__thiscall p_ios_iword)(const ios*, int);
static void** (*__thiscall p_ios_pword)(const ios*, int);
static int (*__cdecl p_ios_xalloc)(void);
static void (*__cdecl p_ios_sync_with_stdio)(void);
static int *p_ios_sunk_with_stdio;
static int *p_ios_fLockcInit;

/* ostream */
static ostream* (*__thiscall p_ostream_copy_ctor)(ostream*, const ostream*, BOOL);
static ostream* (*__thiscall p_ostream_sb_ctor)(ostream*, streambuf*, BOOL);
static ostream* (*__thiscall p_ostream_ctor)(ostream*, BOOL);
static void (*__thiscall p_ostream_dtor)(ios*);
static ostream* (*__thiscall p_ostream_assign)(ostream*, const ostream*);
static ostream* (*__thiscall p_ostream_assign_sb)(ostream*, streambuf*);
static void (*__thiscall p_ostream_vbase_dtor)(ostream*);
static ostream* (*__thiscall p_ostream_flush)(ostream*);
static int (*__thiscall p_ostream_opfx)(ostream*);
static void (*__thiscall p_ostream_osfx)(ostream*);
static ostream* (*__thiscall p_ostream_put_char)(ostream*, char);
static ostream* (*__thiscall p_ostream_write_char)(ostream*, const char*, int);
static ostream* (*__thiscall p_ostream_seekp_offset)(ostream*, streamoff, ios_seek_dir);
static ostream* (*__thiscall p_ostream_seekp)(ostream*, streampos);
static streampos (*__thiscall p_ostream_tellp)(ostream*);
static ostream* (*__thiscall p_ostream_writepad)(ostream*, const char*, const char*);
static ostream* (*__thiscall p_ostream_print_char)(ostream*, char);
static ostream* (*__thiscall p_ostream_print_str)(ostream*, const char*);
static ostream* (*__thiscall p_ostream_print_int)(ostream*, int);
static ostream* (*__thiscall p_ostream_print_float)(ostream*, float);
static ostream* (*__thiscall p_ostream_print_double)(ostream*, double);
static ostream* (*__thiscall p_ostream_print_ptr)(ostream*, const void*);
static ostream* (*__thiscall p_ostream_print_streambuf)(ostream*, streambuf*);

/* ostream_withassign */
static ostream* (*__thiscall p_ostream_withassign_sb_ctor)(ostream*, streambuf*, BOOL);
static ostream* (*__thiscall p_ostream_withassign_copy_ctor)(ostream*, const ostream*, BOOL);
static ostream* (*__thiscall p_ostream_withassign_ctor)(ostream*, BOOL);
static void (*__thiscall p_ostream_withassign_dtor)(ios*);
static void (*__thiscall p_ostream_withassign_vbase_dtor)(ostream*);
static ostream* (*__thiscall p_ostream_withassign_assign_sb)(ostream*, streambuf*);
static ostream* (*__thiscall p_ostream_withassign_assign_os)(ostream*, const ostream*);
static ostream* (*__thiscall p_ostream_withassign_assign)(ostream*, const ostream*);

/* ostrstream */
static ostream* (*__thiscall p_ostrstream_copy_ctor)(ostream*, const ostream*, BOOL);
static ostream* (*__thiscall p_ostrstream_buffer_ctor)(ostream*, char*, int, int, BOOL);
static ostream* (*__thiscall p_ostrstream_ctor)(ostream*, BOOL);
static void (*__thiscall p_ostrstream_dtor)(ios*);
static void (*__thiscall p_ostrstream_vbase_dtor)(ostream*);
static ostream* (*__thiscall p_ostrstream_assign)(ostream*, const ostream*);
static int (*__thiscall p_ostrstream_pcount)(const ostream*);

/* ofstream */
static ostream* (*__thiscall p_ofstream_copy_ctor)(ostream*, const ostream*, BOOL);
static ostream* (*__thiscall p_ofstream_buffer_ctor)(ostream*, filedesc, char*, int, BOOL);
static ostream* (*__thiscall p_ofstream_fd_ctor)(ostream*, filedesc fd, BOOL virt_init);
static ostream* (*__thiscall p_ofstream_open_ctor)(ostream*, const char *name, ios_open_mode, int, BOOL);
static ostream* (*__thiscall p_ofstream_ctor)(ostream*, BOOL);
static void (*__thiscall p_ofstream_dtor)(ios*);
static void (*__thiscall p_ofstream_vbase_dtor)(ostream*);
static void (*__thiscall p_ofstream_attach)(ostream*, filedesc);
static void (*__thiscall p_ofstream_close)(ostream*);
static filedesc (*__thiscall p_ofstream_fd)(ostream*);
static int (*__thiscall p_ofstream_is_open)(const ostream*);
static void (*__thiscall p_ofstream_open)(ostream*, const char*, ios_open_mode, int);
static filebuf* (*__thiscall p_ofstream_rdbuf)(const ostream*);
static streambuf* (*__thiscall p_ofstream_setbuf)(ostream*, char*, int);
static int (*__thiscall p_ofstream_setmode)(ostream*, int);

/* istream */
static istream* (*__thiscall p_istream_copy_ctor)(istream*, const istream*, BOOL);
static istream* (*__thiscall p_istream_ctor)(istream*, BOOL);
static istream* (*__thiscall p_istream_sb_ctor)(istream*, streambuf*, BOOL);
static void (*__thiscall p_istream_dtor)(ios*);
static istream* (*__thiscall p_istream_assign_sb)(istream*, streambuf*);
static istream* (*__thiscall p_istream_assign)(istream*, const istream*);
static void (*__thiscall p_istream_vbase_dtor)(istream*);
static void (*__thiscall p_istream_eatwhite)(istream*);
static int (*__thiscall p_istream_ipfx)(istream*, int);
static istream* (*__thiscall p_istream_get_str_delim)(istream*, char*, int, int);
static istream* (*__thiscall p_istream_get_str)(istream*, char*, int, char);
static int (*__thiscall p_istream_get)(istream*);
static istream* (*__thiscall p_istream_get_char)(istream*, char*);
static istream* (*__thiscall p_istream_get_sb)(istream*, streambuf*, char);
static istream* (*__thiscall p_istream_getline)(istream*, char*, int, char);
static istream* (*__thiscall p_istream_ignore)(istream*, int, int);
static int (*__thiscall p_istream_peek)(istream*);
static istream* (*__thiscall p_istream_putback)(istream*, char);
static istream* (*__thiscall p_istream_read)(istream*, char*, int);
static istream* (*__thiscall p_istream_seekg)(istream*, streampos);
static istream* (*__thiscall p_istream_seekg_offset)(istream*, streamoff, ios_seek_dir);
static int (*__thiscall p_istream_sync)(istream*);
static streampos (*__thiscall p_istream_tellg)(istream*);
static int (*__thiscall p_istream_getint)(istream*, char*);
static int (*__thiscall p_istream_getdouble)(istream*, char*, int);
static istream* (*__thiscall p_istream_read_char)(istream*, char*);
static istream* (*__thiscall p_istream_read_str)(istream*, char*);
static istream* (*__thiscall p_istream_read_short)(istream*, short*);
static istream* (*__thiscall p_istream_read_unsigned_short)(istream*, unsigned short*);
static istream* (*__thiscall p_istream_read_int)(istream*, int*);
static istream* (*__thiscall p_istream_read_unsigned_int)(istream*, unsigned int*);
static istream* (*__thiscall p_istream_read_long)(istream*, LONG*);
static istream* (*__thiscall p_istream_read_unsigned_long)(istream*, ULONG*);
static istream* (*__thiscall p_istream_read_float)(istream*, float*);
static istream* (*__thiscall p_istream_read_double)(istream*, double*);
static istream* (*__thiscall p_istream_read_long_double)(istream*, double*);
static istream* (*__thiscall p_istream_read_streambuf)(istream*, streambuf*);

/* istream_withassign */
static istream* (*__thiscall p_istream_withassign_sb_ctor)(istream*, streambuf*, BOOL);
static istream* (*__thiscall p_istream_withassign_copy_ctor)(istream*, const istream*, BOOL);
static istream* (*__thiscall p_istream_withassign_ctor)(istream*, BOOL);
static void (*__thiscall p_istream_withassign_dtor)(ios*);
static void (*__thiscall p_istream_withassign_vbase_dtor)(istream*);
static istream* (*__thiscall p_istream_withassign_assign_sb)(istream*, streambuf*);
static istream* (*__thiscall p_istream_withassign_assign_is)(istream*, const istream*);
static istream* (*__thiscall p_istream_withassign_assign)(istream*, const istream*);

/* istrstream */
static istream* (*__thiscall p_istrstream_copy_ctor)(istream*, const istream*, BOOL);
static istream* (*__thiscall p_istrstream_str_ctor)(istream*, char*, BOOL);
static istream* (*__thiscall p_istrstream_buffer_ctor)(istream*, char*, int, BOOL);
static void (*__thiscall p_istrstream_dtor)(ios*);
static void (*__thiscall p_istrstream_vbase_dtor)(istream*);
static istream* (*__thiscall p_istrstream_assign)(istream*, const istream*);

/* iostream */
static iostream* (*__thiscall p_iostream_copy_ctor)(iostream*, const iostream*, BOOL);
static iostream* (*__thiscall p_iostream_sb_ctor)(iostream*, streambuf*, BOOL);
static iostream* (*__thiscall p_iostream_ctor)(iostream*, BOOL);
static void (*__thiscall p_iostream_dtor)(ios*);
static void (*__thiscall p_iostream_vbase_dtor)(iostream*);
static iostream* (*__thiscall p_iostream_assign_sb)(iostream*, streambuf*);
static iostream* (*__thiscall p_iostream_assign)(iostream*, const iostream*);

/* ifstream */
static istream* (*__thiscall p_ifstream_copy_ctor)(istream*, const istream*, BOOL);
static istream* (*__thiscall p_ifstream_buffer_ctor)(istream*, filedesc, char*, int, BOOL);
static istream* (*__thiscall p_ifstream_fd_ctor)(istream*, filedesc fd, BOOL virt_init);
static istream* (*__thiscall p_ifstream_open_ctor)(istream*, const char *name, ios_open_mode, int, BOOL);
static istream* (*__thiscall p_ifstream_ctor)(istream*, BOOL);
static void (*__thiscall p_ifstream_dtor)(ios*);
static void (*__thiscall p_ifstream_vbase_dtor)(istream*);
static void (*__thiscall p_ifstream_attach)(istream*, filedesc);
static void (*__thiscall p_ifstream_close)(istream*);
static filedesc (*__thiscall p_ifstream_fd)(istream*);
static int (*__thiscall p_ifstream_is_open)(const istream*);
static void (*__thiscall p_ifstream_open)(istream*, const char*, ios_open_mode, int);
static filebuf* (*__thiscall p_ifstream_rdbuf)(const istream*);
static streambuf* (*__thiscall p_ifstream_setbuf)(istream*, char*, int);
static int (*__thiscall p_ifstream_setmode)(istream*, int);

/* strstream */
static iostream* (*__thiscall p_strstream_copy_ctor)(iostream*, const iostream*, BOOL);
static iostream* (*__thiscall p_strstream_buffer_ctor)(iostream*, char*, int, int, BOOL);
static iostream* (*__thiscall p_strstream_ctor)(iostream*, BOOL);
static void (*__thiscall p_strstream_dtor)(ios*);
static void (*__thiscall p_strstream_vbase_dtor)(iostream*);
static iostream* (*__thiscall p_strstream_assign)(iostream*, const iostream*);

/* stdiostream */
static iostream* (*__thiscall p_stdiostream_copy_ctor)(iostream*, const iostream*, BOOL);
static iostream* (*__thiscall p_stdiostream_file_ctor)(iostream*, FILE*, BOOL);
static void (*__thiscall p_stdiostream_dtor)(ios*);
static void (*__thiscall p_stdiostream_vbase_dtor)(iostream*);
static iostream* (*__thiscall p_stdiostream_assign)(iostream*, const iostream*);

/* fstream */
static iostream* (*__thiscall p_fstream_copy_ctor)(iostream*, const iostream*, BOOL);
static iostream* (*__thiscall p_fstream_buffer_ctor)(iostream*, filedesc, char*, int, BOOL);
static iostream* (*__thiscall p_fstream_fd_ctor)(iostream*, filedesc fd, BOOL virt_init);
static iostream* (*__thiscall p_fstream_open_ctor)(iostream*, const char *name, ios_open_mode, int, BOOL);
static iostream* (*__thiscall p_fstream_ctor)(iostream*, BOOL);
static void (*__thiscall p_fstream_dtor)(ios*);
static void (*__thiscall p_fstream_vbase_dtor)(iostream*);
static void (*__thiscall p_fstream_attach)(iostream*, filedesc);
static void (*__thiscall p_fstream_close)(iostream*);
static filedesc (*__thiscall p_fstream_fd)(iostream*);
static int (*__thiscall p_fstream_is_open)(const iostream*);
static void (*__thiscall p_fstream_open)(iostream*, const char*, ios_open_mode, int);
static filebuf* (*__thiscall p_fstream_rdbuf)(const iostream*);
static streambuf* (*__thiscall p_fstream_setbuf)(iostream*, char*, int);
static int (*__thiscall p_fstream_setmode)(iostream*, int);

/* Iostream_init */
static void* (*__thiscall p_Iostream_init_ios_ctor)(void*, ios*, int);

/* exception */
static exception* (*__thiscall p_exception_ctor)(exception*, const char**);
static void (*__thiscall p_exception_dtor)(exception*);
static const char* (*__thiscall p_exception_what)(exception*);

static logic_error* (*__thiscall p_logic_error_ctor)(logic_error*, const char**);
static void (*__thiscall p_logic_error_dtor)(logic_error*);

/* locking */
static void (*__cdecl p__mtlock)(CRITICAL_SECTION *);
static void (*__cdecl p__mtunlock)(CRITICAL_SECTION *);

/* Predefined streams */
static istream *p_cin;
static ostream *p_cout, *p_cerr, *p_clog;

/* Emulate a __thiscall */
#ifdef __i386__

#include "pshpack1.h"
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#include "poppack.h"

static void * (WINAPI *call_thiscall_func1)( void *func, void *this );
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, void *a );
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, void *a, void *b );
static void * (WINAPI *call_thiscall_func4)( void *func, void *this, void *a, void *b,
        void *c );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, void *a, void *b,
        void *c, void *d );
static void * (WINAPI *call_thiscall_func2_ptr_dbl)( void *func, void *this, double a );
static void * (WINAPI *call_thiscall_func2_ptr_flt)( void *func, void *this, float a );

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
    call_thiscall_func2_ptr_flt  = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(void*)(a),(void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,(void*)(a),(void*)(b), \
        (void*)(c))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(void*)(a),(void*)(b), \
        (void*)(c), (void *)(d))
#define call_func2_ptr_dbl(func,_this,a)  call_thiscall_func2_ptr_dbl(func,_this,a)
#define call_func2_ptr_flt(func,_this,a)  call_thiscall_func2_ptr_flt(func,_this,a)

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)
#define call_func2_ptr_dbl   call_func2
#define call_func2_ptr_flt   call_func2

#endif /* __i386__ */

static HMODULE msvcrt, msvcirt;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcirt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcrt = LoadLibraryA("msvcrt.dll");
    msvcirt = LoadLibraryA("msvcirt.dll");
    if(!msvcirt) {
        win_skip("msvcirt.dll not installed\n");
        return FALSE;
    }

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        p_operator_new = (void*)GetProcAddress(msvcrt, "??2@YAPEAX_K@Z");
        p_operator_delete = (void*)GetProcAddress(msvcrt, "??3@YAXPEAX@Z");

        SET(p_streambuf_reserve_ctor, "??0streambuf@@IEAA@PEADH@Z");
        SET(p_streambuf_ctor, "??0streambuf@@IEAA@XZ");
        SET(p_streambuf_dtor, "??1streambuf@@UEAA@XZ");
        SET(p_streambuf_allocate, "?allocate@streambuf@@IEAAHXZ");
        SET(p_streambuf_clrclock, "?clrlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_doallocate, "?doallocate@streambuf@@MEAAHXZ");
        SET(p_streambuf_gbump, "?gbump@streambuf@@IEAAXH@Z");
        SET(p_streambuf_in_avail, "?in_avail@streambuf@@QEBAHXZ");
        SET(p_streambuf_lock, "?lock@streambuf@@QEAAXXZ");
        SET(p_streambuf_out_waiting, "?out_waiting@streambuf@@QEBAHXZ");
        SET(p_streambuf_pbackfail, "?pbackfail@streambuf@@UEAAHH@Z");
        SET(p_streambuf_pbump, "?pbump@streambuf@@IEAAXH@Z");
        SET(p_streambuf_sbumpc, "?sbumpc@streambuf@@QEAAHXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IEAAXPEAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UEAAPEAV1@PEADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_sgetc, "?sgetc@streambuf@@QEAAHXZ");
        SET(p_streambuf_snextc, "?snextc@streambuf@@QEAAHXZ");
        SET(p_streambuf_sputc, "?sputc@streambuf@@QEAAHH@Z");
        SET(p_streambuf_stossc, "?stossc@streambuf@@QEAAXXZ");
        SET(p_streambuf_sync, "?sync@streambuf@@UEAAHXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QEAAXXZ");
        SET(p_streambuf_xsgetn, "?xsgetn@streambuf@@UEAAHPEADH@Z");
        SET(p_streambuf_xsputn, "?xsputn@streambuf@@UEAAHPEBDH@Z");

        SET(p_filebuf_fd_ctor, "??0filebuf@@QEAA@H@Z");
        SET(p_filebuf_fd_reserve_ctor, "??0filebuf@@QEAA@HPEADH@Z");
        SET(p_filebuf_ctor, "??0filebuf@@QEAA@XZ");
        SET(p_filebuf_dtor, "??1filebuf@@UEAA@XZ");
        SET(p_filebuf_attach, "?attach@filebuf@@QEAAPEAV1@H@Z");
        SET(p_filebuf_open, "?open@filebuf@@QEAAPEAV1@PEBDHH@Z");
        SET(p_filebuf_close, "?close@filebuf@@QEAAPEAV1@XZ");
        SET(p_filebuf_setmode, "?setmode@filebuf@@QEAAHH@Z");
        SET(p_filebuf_setbuf, "?setbuf@filebuf@@UEAAPEAVstreambuf@@PEADH@Z");
        SET(p_filebuf_sync, "?sync@filebuf@@UEAAHXZ");
        SET(p_filebuf_overflow, "?overflow@filebuf@@UEAAHH@Z");
        SET(p_filebuf_underflow, "?underflow@filebuf@@UEAAHXZ");
        SET(p_filebuf_seekoff, "?seekoff@filebuf@@UEAAJJW4seek_dir@ios@@H@Z");
        SET(p_filebuf_is_open, "?is_open@filebuf@@QEBAHXZ");

        SET(p_strstreambuf_dynamic_ctor, "??0strstreambuf@@QEAA@H@Z");
        SET(p_strstreambuf_funcs_ctor, "??0strstreambuf@@QEAA@P6APEAXJ@ZP6AXPEAX@Z@Z");
        SET(p_strstreambuf_buffer_ctor, "??0strstreambuf@@QEAA@PEADH0@Z");
        SET(p_strstreambuf_ubuffer_ctor, "??0strstreambuf@@QEAA@PEAEH0@Z");
        SET(p_strstreambuf_ctor, "??0strstreambuf@@QEAA@XZ");
        SET(p_strstreambuf_dtor, "??1strstreambuf@@UEAA@XZ");
        SET(p_strstreambuf_doallocate, "?doallocate@strstreambuf@@MEAAHXZ");
        SET(p_strstreambuf_freeze, "?freeze@strstreambuf@@QEAAXH@Z");
        SET(p_strstreambuf_overflow, "?overflow@strstreambuf@@UEAAHH@Z");
        SET(p_strstreambuf_seekoff, "?seekoff@strstreambuf@@UEAAJJW4seek_dir@ios@@H@Z");
        SET(p_strstreambuf_setbuf, "?setbuf@strstreambuf@@UEAAPEAVstreambuf@@PEADH@Z");
        SET(p_strstreambuf_underflow, "?underflow@strstreambuf@@UEAAHXZ");

        SET(p_stdiobuf_file_ctor, "??0stdiobuf@@QEAA@PEAU_iobuf@@@Z");
        SET(p_stdiobuf_dtor, "??1stdiobuf@@UEAA@XZ");
        SET(p_stdiobuf_overflow, "?overflow@stdiobuf@@UEAAHH@Z");
        SET(p_stdiobuf_pbackfail, "?pbackfail@stdiobuf@@UEAAHH@Z");
        SET(p_stdiobuf_seekoff, "?seekoff@stdiobuf@@UEAAJJW4seek_dir@ios@@H@Z");
        SET(p_stdiobuf_setrwbuf, "?setrwbuf@stdiobuf@@QEAAHHH@Z");
        SET(p_stdiobuf_sync, "?sync@stdiobuf@@UEAAHXZ");
        SET(p_stdiobuf_underflow, "?underflow@stdiobuf@@UEAAHXZ");

        SET(p_ios_copy_ctor, "??0ios@@IEAA@AEBV0@@Z");
        SET(p_ios_ctor, "??0ios@@IEAA@XZ");
        SET(p_ios_sb_ctor, "??0ios@@QEAA@PEAVstreambuf@@@Z");
        SET(p_ios_assign, "??4ios@@IEAAAEAV0@AEBV0@@Z");
        SET(p_ios_init, "?init@ios@@IEAAXPEAVstreambuf@@@Z");
        SET(p_ios_dtor, "??1ios@@UEAA@XZ");
        SET(p_ios_clrlock, "?clrlock@ios@@QEAAXXZ");
        SET(p_ios_setlock, "?setlock@ios@@QEAAXXZ");
        SET(p_ios_lock, "?lock@ios@@QEAAXXZ");
        SET(p_ios_unlock, "?unlock@ios@@QEAAXXZ");
        SET(p_ios_lockbuf, "?lockbuf@ios@@QEAAXXZ");
        SET(p_ios_unlockbuf, "?unlockbuf@ios@@QEAAXXZ");
        SET(p_ios_flags_set, "?flags@ios@@QEAAJJ@Z");
        SET(p_ios_flags_get, "?flags@ios@@QEBAJXZ");
        SET(p_ios_setf, "?setf@ios@@QEAAJJ@Z");
        SET(p_ios_setf_mask, "?setf@ios@@QEAAJJJ@Z");
        SET(p_ios_unsetf, "?unsetf@ios@@QEAAJJ@Z");
        SET(p_ios_good, "?good@ios@@QEBAHXZ");
        SET(p_ios_bad, "?bad@ios@@QEBAHXZ");
        SET(p_ios_eof, "?eof@ios@@QEBAHXZ");
        SET(p_ios_fail, "?fail@ios@@QEBAHXZ");
        SET(p_ios_clear, "?clear@ios@@QEAAXH@Z");
        SET(p_ios_iword, "?iword@ios@@QEBAAEAJH@Z");
        SET(p_ios_pword, "?pword@ios@@QEBAAEAPEAXH@Z");

        SET(p_ostream_copy_ctor, "??0ostream@@IEAA@AEBV0@@Z");
        SET(p_ostream_sb_ctor, "??0ostream@@QEAA@PEAVstreambuf@@@Z");
        SET(p_ostream_ctor, "??0ostream@@IEAA@XZ");
        SET(p_ostream_dtor, "??1ostream@@UEAA@XZ");
        SET(p_ostream_assign, "??4ostream@@IEAAAEAV0@AEBV0@@Z");
        SET(p_ostream_assign_sb, "??4ostream@@IEAAAEAV0@PEAVstreambuf@@@Z");
        SET(p_ostream_vbase_dtor, "??_Dostream@@QEAAXXZ");
        SET(p_ostream_flush, "?flush@ostream@@QEAAAEAV1@XZ");
        SET(p_ostream_opfx, "?opfx@ostream@@QEAAHXZ");
        SET(p_ostream_osfx, "?osfx@ostream@@QEAAXXZ");
        SET(p_ostream_put_char, "?put@ostream@@QEAAAEAV1@D@Z");
        SET(p_ostream_write_char, "?write@ostream@@QEAAAEAV1@PEBDH@Z");
        SET(p_ostream_seekp_offset, "?seekp@ostream@@QEAAAEAV1@JW4seek_dir@ios@@@Z");
        SET(p_ostream_seekp, "?seekp@ostream@@QEAAAEAV1@J@Z");
        SET(p_ostream_tellp, "?tellp@ostream@@QEAAJXZ");
        SET(p_ostream_writepad, "?writepad@ostream@@AEAAAEAV1@PEBD0@Z");
        SET(p_ostream_print_char, "??6ostream@@QEAAAEAV0@D@Z");
        SET(p_ostream_print_str, "??6ostream@@QEAAAEAV0@PEBD@Z");
        SET(p_ostream_print_int, "??6ostream@@QEAAAEAV0@H@Z");
        SET(p_ostream_print_float, "??6ostream@@QEAAAEAV0@M@Z");
        SET(p_ostream_print_double, "??6ostream@@QEAAAEAV0@N@Z");
        SET(p_ostream_print_ptr, "??6ostream@@QEAAAEAV0@PEBX@Z");
        SET(p_ostream_print_streambuf, "??6ostream@@QEAAAEAV0@PEAVstreambuf@@@Z");

        SET(p_ostream_withassign_sb_ctor, "??0ostream_withassign@@QEAA@PEAVstreambuf@@@Z");
        SET(p_ostream_withassign_copy_ctor, "??0ostream_withassign@@QEAA@AEBV0@@Z");
        SET(p_ostream_withassign_ctor, "??0ostream_withassign@@QEAA@XZ");
        SET(p_ostream_withassign_dtor, "??1ostream_withassign@@UEAA@XZ");
        SET(p_ostream_withassign_vbase_dtor, "??_Dostream_withassign@@QEAAXXZ");
        SET(p_ostream_withassign_assign_sb, "??4ostream_withassign@@QEAAAEAVostream@@PEAVstreambuf@@@Z");
        SET(p_ostream_withassign_assign_os, "??4ostream_withassign@@QEAAAEAVostream@@AEBV1@@Z");
        SET(p_ostream_withassign_assign, "??4ostream_withassign@@QEAAAEAV0@AEBV0@@Z");

        SET(p_ostrstream_copy_ctor, "??0ostrstream@@QEAA@AEBV0@@Z");
        SET(p_ostrstream_buffer_ctor, "??0ostrstream@@QEAA@PEADHH@Z");
        SET(p_ostrstream_ctor, "??0ostrstream@@QEAA@XZ");
        SET(p_ostrstream_dtor, "??1ostrstream@@UEAA@XZ");
        SET(p_ostrstream_vbase_dtor, "??_Dostrstream@@QEAAXXZ");
        SET(p_ostrstream_assign, "??4ostrstream@@QEAAAEAV0@AEBV0@@Z");
        SET(p_ostrstream_pcount, "?pcount@ostrstream@@QEBAHXZ");

        SET(p_ofstream_copy_ctor, "??0ofstream@@QEAA@AEBV0@@Z");
        SET(p_ofstream_buffer_ctor, "??0ofstream@@QEAA@HPEADH@Z");
        SET(p_ofstream_fd_ctor, "??0ofstream@@QEAA@H@Z");
        SET(p_ofstream_open_ctor, "??0ofstream@@QEAA@PEBDHH@Z");
        SET(p_ofstream_ctor, "??0ofstream@@QEAA@XZ");
        SET(p_ofstream_dtor, "??1ofstream@@UEAA@XZ");
        SET(p_ofstream_vbase_dtor, "??_Dofstream@@QEAAXXZ");
        SET(p_ofstream_attach, "?attach@ofstream@@QEAAXH@Z");
        SET(p_ofstream_close, "?close@ofstream@@QEAAXXZ");
        SET(p_ofstream_fd, "?fd@ofstream@@QEBAHXZ");
        SET(p_ofstream_is_open, "?is_open@ofstream@@QEBAHXZ");
        SET(p_ofstream_open, "?open@ofstream@@QEAAXPEBDHH@Z");
        SET(p_ofstream_rdbuf, "?rdbuf@ofstream@@QEBAPEAVfilebuf@@XZ");
        SET(p_ofstream_setbuf, "?setbuf@ofstream@@QEAAPEAVstreambuf@@PEADH@Z");
        SET(p_ofstream_setmode, "?setmode@ofstream@@QEAAHH@Z");

        SET(p_istream_copy_ctor, "??0istream@@IEAA@AEBV0@@Z");
        SET(p_istream_ctor, "??0istream@@IEAA@XZ");
        SET(p_istream_sb_ctor, "??0istream@@QEAA@PEAVstreambuf@@@Z");
        SET(p_istream_dtor, "??1istream@@UEAA@XZ");
        SET(p_istream_assign_sb, "??4istream@@IEAAAEAV0@PEAVstreambuf@@@Z");
        SET(p_istream_assign, "??4istream@@IEAAAEAV0@AEBV0@@Z");
        SET(p_istream_vbase_dtor, "??_Distream@@QEAAXXZ");
        SET(p_istream_eatwhite, "?eatwhite@istream@@QEAAXXZ");
        SET(p_istream_ipfx, "?ipfx@istream@@QEAAHH@Z");
        SET(p_istream_get_str_delim, "?get@istream@@IEAAAEAV1@PEADHH@Z");
        SET(p_istream_get_str, "?get@istream@@QEAAAEAV1@PEADHD@Z");
        SET(p_istream_get, "?get@istream@@QEAAHXZ");
        SET(p_istream_get_char, "?get@istream@@QEAAAEAV1@AEAD@Z");
        SET(p_istream_get_sb, "?get@istream@@QEAAAEAV1@AEAVstreambuf@@D@Z");
        SET(p_istream_getline, "?getline@istream@@QEAAAEAV1@PEADHD@Z");
        SET(p_istream_ignore, "?ignore@istream@@QEAAAEAV1@HH@Z");
        SET(p_istream_peek, "?peek@istream@@QEAAHXZ");
        SET(p_istream_putback, "?putback@istream@@QEAAAEAV1@D@Z");
        SET(p_istream_read, "?read@istream@@QEAAAEAV1@PEADH@Z");
        SET(p_istream_seekg, "?seekg@istream@@QEAAAEAV1@J@Z");
        SET(p_istream_seekg_offset, "?seekg@istream@@QEAAAEAV1@JW4seek_dir@ios@@@Z");
        SET(p_istream_sync, "?sync@istream@@QEAAHXZ");
        SET(p_istream_tellg, "?tellg@istream@@QEAAJXZ");
        SET(p_istream_getint, "?getint@istream@@AEAAHPEAD@Z");
        SET(p_istream_getdouble, "?getdouble@istream@@AEAAHPEADH@Z");
        SET(p_istream_read_char, "??5istream@@QEAAAEAV0@AEAD@Z");
        SET(p_istream_read_str, "??5istream@@QEAAAEAV0@PEAD@Z");
        SET(p_istream_read_short, "??5istream@@QEAAAEAV0@AEAF@Z");
        SET(p_istream_read_unsigned_short, "??5istream@@QEAAAEAV0@AEAG@Z");
        SET(p_istream_read_int, "??5istream@@QEAAAEAV0@AEAH@Z");
        SET(p_istream_read_unsigned_int, "??5istream@@QEAAAEAV0@AEAI@Z");
        SET(p_istream_read_long, "??5istream@@QEAAAEAV0@AEAJ@Z");
        SET(p_istream_read_unsigned_long, "??5istream@@QEAAAEAV0@AEAK@Z");
        SET(p_istream_read_float, "??5istream@@QEAAAEAV0@AEAM@Z");
        SET(p_istream_read_double, "??5istream@@QEAAAEAV0@AEAN@Z");
        SET(p_istream_read_long_double, "??5istream@@QEAAAEAV0@AEAO@Z");
        SET(p_istream_read_streambuf, "??5istream@@QEAAAEAV0@PEAVstreambuf@@@Z");

        SET(p_istream_withassign_sb_ctor, "??0istream_withassign@@QEAA@PEAVstreambuf@@@Z");
        SET(p_istream_withassign_copy_ctor, "??0istream_withassign@@QEAA@AEBV0@@Z");
        SET(p_istream_withassign_ctor, "??0istream_withassign@@QEAA@XZ");
        SET(p_istream_withassign_dtor, "??1ostream_withassign@@UEAA@XZ");
        SET(p_istream_withassign_vbase_dtor, "??_Distream_withassign@@QEAAXXZ");
        SET(p_istream_withassign_assign_sb, "??4istream_withassign@@QEAAAEAVistream@@PEAVstreambuf@@@Z");
        SET(p_istream_withassign_assign_is, "??4istream_withassign@@QEAAAEAVistream@@AEBV1@@Z");
        SET(p_istream_withassign_assign, "??4istream_withassign@@QEAAAEAV0@AEBV0@@Z");

        SET(p_istrstream_copy_ctor, "??0istrstream@@QEAA@AEBV0@@Z");
        SET(p_istrstream_str_ctor, "??0istrstream@@QEAA@PEAD@Z");
        SET(p_istrstream_buffer_ctor, "??0istrstream@@QEAA@PEADH@Z");
        SET(p_istrstream_dtor, "??1istrstream@@UEAA@XZ");
        SET(p_istrstream_vbase_dtor, "??_Distrstream@@QEAAXXZ");
        SET(p_istrstream_assign, "??4istrstream@@QEAAAEAV0@AEBV0@@Z");

        SET(p_iostream_copy_ctor, "??0iostream@@IEAA@AEBV0@@Z");
        SET(p_iostream_sb_ctor, "??0iostream@@QEAA@PEAVstreambuf@@@Z");
        SET(p_iostream_ctor, "??0iostream@@IEAA@XZ");
        SET(p_iostream_dtor, "??1iostream@@UEAA@XZ");
        SET(p_iostream_vbase_dtor, "??_Diostream@@QEAAXXZ");
        SET(p_iostream_assign_sb, "??4iostream@@IEAAAEAV0@PEAVstreambuf@@@Z");
        SET(p_iostream_assign, "??4iostream@@IEAAAEAV0@AEAV0@@Z");

        SET(p_ifstream_copy_ctor, "??0ifstream@@QEAA@AEBV0@@Z");
        SET(p_ifstream_buffer_ctor, "??0ifstream@@QEAA@HPEADH@Z");
        SET(p_ifstream_fd_ctor, "??0ifstream@@QEAA@H@Z");
        SET(p_ifstream_open_ctor, "??0ifstream@@QEAA@PEBDHH@Z");
        SET(p_ifstream_ctor, "??0ifstream@@QEAA@XZ");
        SET(p_ifstream_dtor, "??1ifstream@@UEAA@XZ");
        SET(p_ifstream_vbase_dtor, "??_Difstream@@QEAAXXZ");
        SET(p_ifstream_attach, "?attach@ifstream@@QEAAXH@Z");
        SET(p_ifstream_close, "?close@ifstream@@QEAAXXZ");
        SET(p_ifstream_fd, "?fd@ifstream@@QEBAHXZ");
        SET(p_ifstream_is_open, "?is_open@ifstream@@QEBAHXZ");
        SET(p_ifstream_open, "?open@ifstream@@QEAAXPEBDHH@Z");
        SET(p_ifstream_rdbuf, "?rdbuf@ifstream@@QEBAPEAVfilebuf@@XZ");
        SET(p_ifstream_setbuf, "?setbuf@ifstream@@QEAAPEAVstreambuf@@PEADH@Z");
        SET(p_ifstream_setmode, "?setmode@ifstream@@QEAAHH@Z");

        SET(p_strstream_copy_ctor, "??0strstream@@QEAA@AEBV0@@Z");
        SET(p_strstream_buffer_ctor, "??0strstream@@QEAA@PEADHH@Z");
        SET(p_strstream_ctor, "??0strstream@@QEAA@XZ");
        SET(p_strstream_dtor, "??1strstream@@UEAA@XZ");
        SET(p_strstream_vbase_dtor, "??_Dstrstream@@QEAAXXZ");
        SET(p_strstream_assign, "??4strstream@@QEAAAEAV0@AEAV0@@Z");

        SET(p_stdiostream_copy_ctor, "??0stdiostream@@QEAA@AEBV0@@Z");
        SET(p_stdiostream_file_ctor, "??0stdiostream@@QEAA@PEAU_iobuf@@@Z");
        SET(p_stdiostream_dtor, "??1stdiostream@@UEAA@XZ");
        SET(p_stdiostream_vbase_dtor, "??_Dstdiostream@@QEAAXXZ");
        SET(p_stdiostream_assign, "??4stdiostream@@QEAAAEAV0@AEAV0@@Z");

        SET(p_fstream_copy_ctor, "??0fstream@@QEAA@AEBV0@@Z");
        SET(p_fstream_buffer_ctor, "??0fstream@@QEAA@HPEADH@Z");
        SET(p_fstream_fd_ctor, "??0fstream@@QEAA@H@Z");
        SET(p_fstream_open_ctor, "??0fstream@@QEAA@PEBDHH@Z");
        SET(p_fstream_ctor, "??0fstream@@QEAA@XZ");
        SET(p_fstream_dtor, "??1fstream@@UEAA@XZ");
        SET(p_fstream_vbase_dtor, "??_Dfstream@@QEAAXXZ");
        SET(p_fstream_attach, "?attach@fstream@@QEAAXH@Z");
        SET(p_fstream_close, "?close@fstream@@QEAAXXZ");
        SET(p_fstream_fd, "?fd@fstream@@QEBAHXZ");
        SET(p_fstream_is_open, "?is_open@fstream@@QEBAHXZ");
        SET(p_fstream_open, "?open@fstream@@QEAAXPEBDHH@Z");
        SET(p_fstream_rdbuf, "?rdbuf@fstream@@QEBAPEAVfilebuf@@XZ");
        SET(p_fstream_setbuf, "?setbuf@fstream@@QEAAPEAVstreambuf@@PEADH@Z");
        SET(p_fstream_setmode, "?setmode@fstream@@QEAAHH@Z");

        SET(p_Iostream_init_ios_ctor, "??0Iostream_init@@QEAA@AEAVios@@H@Z");

        SET(p_exception_ctor, "??0exception@@QEAA@AEBQEBD@Z");
        SET(p_exception_dtor, "??1exception@@UEAA@XZ");
        SET(p_exception_what, "?what@exception@@UEBAPEBDXZ");

        SET(p_logic_error_ctor, "??0logic_error@@QEAA@AEBQEBD@Z");
        SET(p_logic_error_dtor, "??1logic_error@@UEAA@XZ");
    } else {
        p_operator_new = (void*)GetProcAddress(msvcrt, "??2@YAPAXI@Z");
        p_operator_delete = (void*)GetProcAddress(msvcrt, "??3@YAXPAX@Z");

        SET(p_streambuf_reserve_ctor, "??0streambuf@@IAE@PADH@Z");
        SET(p_streambuf_ctor, "??0streambuf@@IAE@XZ");
        SET(p_streambuf_dtor, "??1streambuf@@UAE@XZ");
        SET(p_streambuf_allocate, "?allocate@streambuf@@IAEHXZ");
        SET(p_streambuf_clrclock, "?clrlock@streambuf@@QAEXXZ");
        SET(p_streambuf_doallocate, "?doallocate@streambuf@@MAEHXZ");
        SET(p_streambuf_gbump, "?gbump@streambuf@@IAEXH@Z");
        SET(p_streambuf_in_avail, "?in_avail@streambuf@@QBEHXZ");
        SET(p_streambuf_lock, "?lock@streambuf@@QAEXXZ");
        SET(p_streambuf_out_waiting, "?out_waiting@streambuf@@QBEHXZ");
        SET(p_streambuf_pbackfail, "?pbackfail@streambuf@@UAEHH@Z");
        SET(p_streambuf_pbump, "?pbump@streambuf@@IAEXH@Z");
        SET(p_streambuf_sbumpc, "?sbumpc@streambuf@@QAEHXZ");
        SET(p_streambuf_setb, "?setb@streambuf@@IAEXPAD0H@Z");
        SET(p_streambuf_setbuf, "?setbuf@streambuf@@UAEPAV1@PADH@Z");
        SET(p_streambuf_setlock, "?setlock@streambuf@@QAEXXZ");
        SET(p_streambuf_sgetc, "?sgetc@streambuf@@QAEHXZ");
        SET(p_streambuf_snextc, "?snextc@streambuf@@QAEHXZ");
        SET(p_streambuf_sputc, "?sputc@streambuf@@QAEHH@Z");
        SET(p_streambuf_stossc, "?stossc@streambuf@@QAEXXZ");
        SET(p_streambuf_sync, "?sync@streambuf@@UAEHXZ");
        SET(p_streambuf_unlock, "?unlock@streambuf@@QAEXXZ");
        SET(p_streambuf_xsgetn, "?xsgetn@streambuf@@UAEHPADH@Z");
        SET(p_streambuf_xsputn, "?xsputn@streambuf@@UAEHPBDH@Z");

        SET(p_filebuf_fd_ctor, "??0filebuf@@QAE@H@Z");
        SET(p_filebuf_fd_reserve_ctor, "??0filebuf@@QAE@HPADH@Z");
        SET(p_filebuf_ctor, "??0filebuf@@QAE@XZ");
        SET(p_filebuf_dtor, "??1filebuf@@UAE@XZ");
        SET(p_filebuf_attach, "?attach@filebuf@@QAEPAV1@H@Z");
        SET(p_filebuf_open, "?open@filebuf@@QAEPAV1@PBDHH@Z");
        SET(p_filebuf_close, "?close@filebuf@@QAEPAV1@XZ");
        SET(p_filebuf_setmode, "?setmode@filebuf@@QAEHH@Z");
        SET(p_filebuf_setbuf, "?setbuf@filebuf@@UAEPAVstreambuf@@PADH@Z");
        SET(p_filebuf_sync, "?sync@filebuf@@UAEHXZ");
        SET(p_filebuf_overflow, "?overflow@filebuf@@UAEHH@Z");
        SET(p_filebuf_underflow, "?underflow@filebuf@@UAEHXZ");
        SET(p_filebuf_seekoff, "?seekoff@filebuf@@UAEJJW4seek_dir@ios@@H@Z");
        SET(p_filebuf_is_open, "?is_open@filebuf@@QBEHXZ");

        SET(p_strstreambuf_dynamic_ctor, "??0strstreambuf@@QAE@H@Z");
        SET(p_strstreambuf_funcs_ctor, "??0strstreambuf@@QAE@P6APAXJ@ZP6AXPAX@Z@Z");
        SET(p_strstreambuf_buffer_ctor, "??0strstreambuf@@QAE@PADH0@Z");
        SET(p_strstreambuf_ubuffer_ctor, "??0strstreambuf@@QAE@PAEH0@Z");
        SET(p_strstreambuf_ctor, "??0strstreambuf@@QAE@XZ");
        SET(p_strstreambuf_dtor, "??1strstreambuf@@UAE@XZ");
        SET(p_strstreambuf_doallocate, "?doallocate@strstreambuf@@MAEHXZ");
        SET(p_strstreambuf_freeze, "?freeze@strstreambuf@@QAEXH@Z");
        SET(p_strstreambuf_overflow, "?overflow@strstreambuf@@UAEHH@Z");
        SET(p_strstreambuf_seekoff, "?seekoff@strstreambuf@@UAEJJW4seek_dir@ios@@H@Z");
        SET(p_strstreambuf_setbuf, "?setbuf@strstreambuf@@UAEPAVstreambuf@@PADH@Z");
        SET(p_strstreambuf_underflow, "?underflow@strstreambuf@@UAEHXZ");

        SET(p_stdiobuf_file_ctor, "??0stdiobuf@@QAE@PAU_iobuf@@@Z");
        SET(p_stdiobuf_dtor, "??1stdiobuf@@UAE@XZ");
        SET(p_stdiobuf_overflow, "?overflow@stdiobuf@@UAEHH@Z");
        SET(p_stdiobuf_pbackfail, "?pbackfail@stdiobuf@@UAEHH@Z");
        SET(p_stdiobuf_seekoff, "?seekoff@stdiobuf@@UAEJJW4seek_dir@ios@@H@Z");
        SET(p_stdiobuf_setrwbuf, "?setrwbuf@stdiobuf@@QAEHHH@Z");
        SET(p_stdiobuf_sync, "?sync@stdiobuf@@UAEHXZ");
        SET(p_stdiobuf_underflow, "?underflow@stdiobuf@@UAEHXZ");

        SET(p_ios_copy_ctor, "??0ios@@IAE@ABV0@@Z");
        SET(p_ios_ctor, "??0ios@@IAE@XZ");
        SET(p_ios_sb_ctor, "??0ios@@QAE@PAVstreambuf@@@Z");
        SET(p_ios_assign, "??4ios@@IAEAAV0@ABV0@@Z");
        SET(p_ios_init, "?init@ios@@IAEXPAVstreambuf@@@Z");
        SET(p_ios_dtor, "??1ios@@UAE@XZ");
        SET(p_ios_clrlock, "?clrlock@ios@@QAAXXZ");
        SET(p_ios_setlock, "?setlock@ios@@QAAXXZ");
        SET(p_ios_lock, "?lock@ios@@QAAXXZ");
        SET(p_ios_unlock, "?unlock@ios@@QAAXXZ");
        SET(p_ios_lockbuf, "?lockbuf@ios@@QAAXXZ");
        SET(p_ios_unlockbuf, "?unlockbuf@ios@@QAAXXZ");
        SET(p_ios_flags_set, "?flags@ios@@QAEJJ@Z");
        SET(p_ios_flags_get, "?flags@ios@@QBEJXZ");
        SET(p_ios_setf, "?setf@ios@@QAEJJ@Z");
        SET(p_ios_setf_mask, "?setf@ios@@QAEJJJ@Z");
        SET(p_ios_unsetf, "?unsetf@ios@@QAEJJ@Z");
        SET(p_ios_good, "?good@ios@@QBEHXZ");
        SET(p_ios_bad, "?bad@ios@@QBEHXZ");
        SET(p_ios_eof, "?eof@ios@@QBEHXZ");
        SET(p_ios_fail, "?fail@ios@@QBEHXZ");
        SET(p_ios_clear, "?clear@ios@@QAEXH@Z");
        SET(p_ios_iword, "?iword@ios@@QBEAAJH@Z");
        SET(p_ios_pword, "?pword@ios@@QBEAAPAXH@Z");

        SET(p_ostream_copy_ctor, "??0ostream@@IAE@ABV0@@Z");
        SET(p_ostream_sb_ctor, "??0ostream@@QAE@PAVstreambuf@@@Z");
        SET(p_ostream_ctor, "??0ostream@@IAE@XZ");
        SET(p_ostream_dtor, "??1ostream@@UAE@XZ");
        SET(p_ostream_assign, "??4ostream@@IAEAAV0@ABV0@@Z");
        SET(p_ostream_assign_sb, "??4ostream@@IAEAAV0@PAVstreambuf@@@Z");
        SET(p_ostream_vbase_dtor, "??_Dostream@@QAEXXZ");
        SET(p_ostream_flush, "?flush@ostream@@QAEAAV1@XZ");
        SET(p_ostream_opfx, "?opfx@ostream@@QAEHXZ");
        SET(p_ostream_osfx, "?osfx@ostream@@QAEXXZ");
        SET(p_ostream_put_char, "?put@ostream@@QAEAAV1@D@Z");
        SET(p_ostream_write_char, "?write@ostream@@QAEAAV1@PBDH@Z");
        SET(p_ostream_seekp_offset, "?seekp@ostream@@QAEAAV1@JW4seek_dir@ios@@@Z");
        SET(p_ostream_seekp, "?seekp@ostream@@QAEAAV1@J@Z");
        SET(p_ostream_tellp, "?tellp@ostream@@QAEJXZ");
        SET(p_ostream_writepad, "?writepad@ostream@@AAEAAV1@PBD0@Z");
        SET(p_ostream_print_char, "??6ostream@@QAEAAV0@D@Z");
        SET(p_ostream_print_str, "??6ostream@@QAEAAV0@PBD@Z");
        SET(p_ostream_print_int, "??6ostream@@QAEAAV0@H@Z");
        SET(p_ostream_print_float, "??6ostream@@QAEAAV0@M@Z");
        SET(p_ostream_print_double, "??6ostream@@QAEAAV0@N@Z");
        SET(p_ostream_print_ptr, "??6ostream@@QAEAAV0@PBX@Z");
        SET(p_ostream_print_streambuf, "??6ostream@@QAEAAV0@PAVstreambuf@@@Z");

        SET(p_ostream_withassign_sb_ctor, "??0ostream_withassign@@QAE@PAVstreambuf@@@Z");
        SET(p_ostream_withassign_copy_ctor, "??0ostream_withassign@@QAE@ABV0@@Z");
        SET(p_ostream_withassign_ctor, "??0ostream_withassign@@QAE@XZ");
        SET(p_ostream_withassign_dtor, "??1ostream_withassign@@UAE@XZ");
        SET(p_ostream_withassign_vbase_dtor, "??_Dostream_withassign@@QAEXXZ");
        SET(p_ostream_withassign_assign_sb, "??4ostream_withassign@@QAEAAVostream@@PAVstreambuf@@@Z");
        SET(p_ostream_withassign_assign_os, "??4ostream_withassign@@QAEAAVostream@@ABV1@@Z");
        SET(p_ostream_withassign_assign, "??4ostream_withassign@@QAEAAV0@ABV0@@Z");

        SET(p_ostrstream_copy_ctor, "??0ostrstream@@QAE@ABV0@@Z");
        SET(p_ostrstream_buffer_ctor, "??0ostrstream@@QAE@PADHH@Z");
        SET(p_ostrstream_ctor, "??0ostrstream@@QAE@XZ");
        SET(p_ostrstream_dtor, "??1ostrstream@@UAE@XZ");
        SET(p_ostrstream_vbase_dtor, "??_Dostrstream@@QAEXXZ");
        SET(p_ostrstream_assign, "??4ostrstream@@QAEAAV0@ABV0@@Z");
        SET(p_ostrstream_pcount, "?pcount@ostrstream@@QBEHXZ");

        SET(p_ofstream_copy_ctor, "??0ofstream@@QAE@ABV0@@Z");
        SET(p_ofstream_fd_ctor, "??0ofstream@@QAE@H@Z");
        SET(p_ofstream_buffer_ctor, "??0ofstream@@QAE@HPADH@Z");
        SET(p_ofstream_open_ctor, "??0ofstream@@QAE@PBDHH@Z");
        SET(p_ofstream_ctor, "??0ofstream@@QAE@XZ");
        SET(p_ofstream_dtor, "??1ofstream@@UAE@XZ");
        SET(p_ofstream_vbase_dtor, "??_Dofstream@@QAEXXZ");
        SET(p_ofstream_attach, "?attach@ofstream@@QAEXH@Z");
        SET(p_ofstream_close, "?close@ofstream@@QAEXXZ");
        SET(p_ofstream_fd, "?fd@ofstream@@QBEHXZ");
        SET(p_ofstream_is_open, "?is_open@ofstream@@QBEHXZ");
        SET(p_ofstream_open, "?open@ofstream@@QAEXPBDHH@Z");
        SET(p_ofstream_rdbuf, "?rdbuf@ofstream@@QBEPAVfilebuf@@XZ");
        SET(p_ofstream_setbuf, "?setbuf@ofstream@@QAEPAVstreambuf@@PADH@Z");
        SET(p_ofstream_setmode, "?setmode@ofstream@@QAEHH@Z");

        SET(p_istream_copy_ctor, "??0istream@@IAE@ABV0@@Z");
        SET(p_istream_ctor, "??0istream@@IAE@XZ");
        SET(p_istream_sb_ctor, "??0istream@@QAE@PAVstreambuf@@@Z");
        SET(p_istream_dtor, "??1istream@@UAE@XZ");
        SET(p_istream_assign_sb, "??4istream@@IAEAAV0@PAVstreambuf@@@Z");
        SET(p_istream_assign, "??4istream@@IAEAAV0@ABV0@@Z");
        SET(p_istream_vbase_dtor, "??_Distream@@QAEXXZ");
        SET(p_istream_eatwhite, "?eatwhite@istream@@QAEXXZ");
        SET(p_istream_ipfx, "?ipfx@istream@@QAEHH@Z");
        SET(p_istream_get_str_delim, "?get@istream@@IAEAAV1@PADHH@Z");
        SET(p_istream_get_str, "?get@istream@@QAEAAV1@PADHD@Z");
        SET(p_istream_get, "?get@istream@@QAEHXZ");
        SET(p_istream_get_char, "?get@istream@@QAEAAV1@AAD@Z");
        SET(p_istream_get_sb, "?get@istream@@QAEAAV1@AAVstreambuf@@D@Z");
        SET(p_istream_getline, "?getline@istream@@QAEAAV1@PADHD@Z");
        SET(p_istream_ignore, "?ignore@istream@@QAEAAV1@HH@Z");
        SET(p_istream_peek, "?peek@istream@@QAEHXZ");
        SET(p_istream_putback, "?putback@istream@@QAEAAV1@D@Z");
        SET(p_istream_read, "?read@istream@@QAEAAV1@PADH@Z");
        SET(p_istream_seekg, "?seekg@istream@@QAEAAV1@J@Z");
        SET(p_istream_seekg_offset, "?seekg@istream@@QAEAAV1@JW4seek_dir@ios@@@Z");
        SET(p_istream_sync, "?sync@istream@@QAEHXZ");
        SET(p_istream_tellg, "?tellg@istream@@QAEJXZ");
        SET(p_istream_getint, "?getint@istream@@AAEHPAD@Z");
        SET(p_istream_getdouble, "?getdouble@istream@@AAEHPADH@Z");
        SET(p_istream_read_char, "??5istream@@QAEAAV0@AAD@Z");
        SET(p_istream_read_str, "??5istream@@QAEAAV0@PAD@Z");
        SET(p_istream_read_short, "??5istream@@QAEAAV0@AAF@Z");
        SET(p_istream_read_unsigned_short, "??5istream@@QAEAAV0@AAG@Z");
        SET(p_istream_read_int, "??5istream@@QAEAAV0@AAH@Z");
        SET(p_istream_read_unsigned_int, "??5istream@@QAEAAV0@AAI@Z");
        SET(p_istream_read_long, "??5istream@@QAEAAV0@AAJ@Z");
        SET(p_istream_read_unsigned_long, "??5istream@@QAEAAV0@AAK@Z");
        SET(p_istream_read_float, "??5istream@@QAEAAV0@AAM@Z");
        SET(p_istream_read_double, "??5istream@@QAEAAV0@AAN@Z");
        SET(p_istream_read_long_double, "??5istream@@QAEAAV0@AAO@Z");
        SET(p_istream_read_streambuf, "??5istream@@QAEAAV0@PAVstreambuf@@@Z");

        SET(p_istream_withassign_sb_ctor, "??0istream_withassign@@QAE@PAVstreambuf@@@Z");
        SET(p_istream_withassign_copy_ctor, "??0istream_withassign@@QAE@ABV0@@Z");
        SET(p_istream_withassign_ctor, "??0istream_withassign@@QAE@XZ");
        SET(p_istream_withassign_dtor, "??1istream_withassign@@UAE@XZ");
        SET(p_istream_withassign_vbase_dtor, "??_Distream_withassign@@QAEXXZ");
        SET(p_istream_withassign_assign_sb, "??4istream_withassign@@QAEAAVistream@@PAVstreambuf@@@Z");
        SET(p_istream_withassign_assign_is, "??4istream_withassign@@QAEAAVistream@@ABV1@@Z");
        SET(p_istream_withassign_assign, "??4istream_withassign@@QAEAAV0@ABV0@@Z");

        SET(p_istrstream_copy_ctor, "??0istrstream@@QAE@ABV0@@Z");
        SET(p_istrstream_str_ctor, "??0istrstream@@QAE@PAD@Z");
        SET(p_istrstream_buffer_ctor, "??0istrstream@@QAE@PADH@Z");
        SET(p_istrstream_dtor, "??1istrstream@@UAE@XZ");
        SET(p_istrstream_vbase_dtor, "??_Distrstream@@QAEXXZ");
        SET(p_istrstream_assign, "??4istrstream@@QAEAAV0@ABV0@@Z");

        SET(p_iostream_copy_ctor, "??0iostream@@IAE@ABV0@@Z");
        SET(p_iostream_sb_ctor, "??0iostream@@QAE@PAVstreambuf@@@Z");
        SET(p_iostream_ctor, "??0iostream@@IAE@XZ");
        SET(p_iostream_dtor, "??1iostream@@UAE@XZ");
        SET(p_iostream_vbase_dtor, "??_Diostream@@QAEXXZ");
        SET(p_iostream_assign_sb, "??4iostream@@IAEAAV0@PAVstreambuf@@@Z");
        SET(p_iostream_assign, "??4iostream@@IAEAAV0@AAV0@@Z");

        SET(p_ifstream_copy_ctor, "??0ifstream@@QAE@ABV0@@Z");
        SET(p_ifstream_fd_ctor, "??0ifstream@@QAE@H@Z");
        SET(p_ifstream_buffer_ctor, "??0ifstream@@QAE@HPADH@Z");
        SET(p_ifstream_open_ctor, "??0ifstream@@QAE@PBDHH@Z");
        SET(p_ifstream_ctor, "??0ifstream@@QAE@XZ");
        SET(p_ifstream_dtor, "??1ifstream@@UAE@XZ");
        SET(p_ifstream_vbase_dtor, "??_Difstream@@QAEXXZ");
        SET(p_ifstream_attach, "?attach@ifstream@@QAEXH@Z");
        SET(p_ifstream_close, "?close@ifstream@@QAEXXZ");
        SET(p_ifstream_fd, "?fd@ifstream@@QBEHXZ");
        SET(p_ifstream_is_open, "?is_open@ifstream@@QBEHXZ");
        SET(p_ifstream_open, "?open@ifstream@@QAEXPBDHH@Z");
        SET(p_ifstream_rdbuf, "?rdbuf@ifstream@@QBEPAVfilebuf@@XZ");
        SET(p_ifstream_setbuf, "?setbuf@ifstream@@QAEPAVstreambuf@@PADH@Z");
        SET(p_ifstream_setmode, "?setmode@ifstream@@QAEHH@Z");

        SET(p_strstream_copy_ctor, "??0strstream@@QAE@ABV0@@Z");
        SET(p_strstream_buffer_ctor, "??0strstream@@QAE@PADHH@Z");
        SET(p_strstream_ctor, "??0strstream@@QAE@XZ");
        SET(p_strstream_dtor, "??1strstream@@UAE@XZ");
        SET(p_strstream_vbase_dtor, "??_Dstrstream@@QAEXXZ");
        SET(p_strstream_assign, "??4strstream@@QAEAAV0@AAV0@@Z");

        SET(p_stdiostream_copy_ctor, "??0stdiostream@@QAE@ABV0@@Z");
        SET(p_stdiostream_file_ctor, "??0stdiostream@@QAE@PAU_iobuf@@@Z");
        SET(p_stdiostream_dtor, "??1stdiostream@@UAE@XZ");
        SET(p_stdiostream_vbase_dtor, "??_Dstdiostream@@QAEXXZ");
        SET(p_stdiostream_assign, "??4stdiostream@@QAEAAV0@AAV0@@Z");

        SET(p_fstream_copy_ctor, "??0fstream@@QAE@ABV0@@Z");
        SET(p_fstream_fd_ctor, "??0fstream@@QAE@H@Z");
        SET(p_fstream_buffer_ctor, "??0fstream@@QAE@HPADH@Z");
        SET(p_fstream_open_ctor, "??0fstream@@QAE@PBDHH@Z");
        SET(p_fstream_ctor, "??0fstream@@QAE@XZ");
        SET(p_fstream_dtor, "??1fstream@@UAE@XZ");
        SET(p_fstream_vbase_dtor, "??_Dfstream@@QAEXXZ");
        SET(p_fstream_attach, "?attach@fstream@@QAEXH@Z");
        SET(p_fstream_close, "?close@fstream@@QAEXXZ");
        SET(p_fstream_fd, "?fd@fstream@@QBEHXZ");
        SET(p_fstream_is_open, "?is_open@fstream@@QBEHXZ");
        SET(p_fstream_open, "?open@fstream@@QAEXPBDHH@Z");
        SET(p_fstream_rdbuf, "?rdbuf@fstream@@QBEPAVfilebuf@@XZ");
        SET(p_fstream_setbuf, "?setbuf@fstream@@QAEPAVstreambuf@@PADH@Z");
        SET(p_fstream_setmode, "?setmode@fstream@@QAEHH@Z");

        SET(p_Iostream_init_ios_ctor, "??0Iostream_init@@QAE@AAVios@@H@Z");

        SET(p_exception_ctor, "??0exception@@QAE@ABQBD@Z");
        SET(p_exception_dtor, "??1exception@@UAE@XZ");
        SET(p_exception_what, "?what@exception@@UBEPBDXZ");

        SET(p_logic_error_ctor, "??0logic_error@@QAE@ABQBD@Z");
        SET(p_logic_error_dtor, "??1logic_error@@UAE@XZ");
    }
    SET(p_ios_static_lock, "?x_lockc@ios@@0U_CRT_CRITICAL_SECTION@@A");
    SET(p_ios_lockc, "?lockc@ios@@KAXXZ");
    SET(p_ios_unlockc, "?unlockc@ios@@KAXXZ");
    SET(p_ios_maxbit, "?x_maxbit@ios@@0JA");
    SET(p_ios_bitalloc, "?bitalloc@ios@@SAJXZ");
    SET(p_ios_curindex, "?x_curindex@ios@@0HA");
    SET(p_ios_statebuf, "?x_statebuf@ios@@0PAJA");
    SET(p_ios_xalloc, "?xalloc@ios@@SAHXZ");
    SET(p_ios_sync_with_stdio, "?sync_with_stdio@ios@@SAXXZ");
    SET(p_ios_sunk_with_stdio, "?sunk_with_stdio@ios@@0HA");
    SET(p_ios_fLockcInit, "?fLockcInit@ios@@0HA");
    SET(p_cin, "?cin@@3Vistream_withassign@@A");
    SET(p_cout, "?cout@@3Vostream_withassign@@A");
    SET(p_cerr, "?cerr@@3Vostream_withassign@@A");
    SET(p_clog, "?clog@@3Vostream_withassign@@A");

    SET(p__mtlock, "_mtlock");
    SET(p__mtunlock, "_mtunlock");

    init_thiscall_thunk();
    return TRUE;
}

static int overflow_count, underflow_count;
static streambuf *test_this;
static char test_get_buffer[24];
static int buffer_pos, get_end;

#ifdef __i386__
static int __thiscall test_streambuf_overflow(int ch)
#else
static int __thiscall test_streambuf_overflow(streambuf *this, int ch)
#endif
{
    overflow_count++;
    if (ch == 'L') /* simulate a failure */
        return EOF;
    if (!test_this->unbuffered)
        test_this->pptr = test_this->pbase + 5;
    return (unsigned char)ch;
}

#ifdef __i386__
static int __thiscall test_streambuf_underflow(void)
#else
static int __thiscall test_streambuf_underflow(streambuf *this)
#endif
{
    underflow_count++;
    if (test_this->unbuffered) {
        return (buffer_pos < 23) ? (unsigned char)test_get_buffer[buffer_pos++] : EOF;
    } else if (test_this->gptr < test_this->egptr) {
        return (unsigned char)*test_this->gptr;
    } else {
        return get_end ? EOF : (unsigned char)*(test_this->gptr = test_this->eback);
    }
}

struct streambuf_lock_arg
{
    streambuf *sb;
    HANDLE lock[4];
    HANDLE test[4];
};

static DWORD WINAPI lock_streambuf(void *arg)
{
    struct streambuf_lock_arg *lock_arg = arg;
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[0]);
    WaitForSingleObject(lock_arg->test[0], INFINITE);
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[1]);
    WaitForSingleObject(lock_arg->test[1], INFINITE);
    call_func1(p_streambuf_lock, lock_arg->sb);
    SetEvent(lock_arg->lock[2]);
    WaitForSingleObject(lock_arg->test[2], INFINITE);
    call_func1(p_streambuf_unlock, lock_arg->sb);
    SetEvent(lock_arg->lock[3]);
    WaitForSingleObject(lock_arg->test[3], INFINITE);
    call_func1(p_streambuf_unlock, lock_arg->sb);
    return 0;
}

static void test_streambuf(void)
{
    streambuf sb, sb2, sb3, *psb;
    vtable_ptr test_streambuf_vtbl[11];
    struct streambuf_lock_arg lock_arg;
    HANDLE thread;
    char reserve[16];
    int ret, i;
    BOOL locked;

    memset(&sb, 0xab, sizeof(streambuf));
    memset(&sb2, 0xab, sizeof(streambuf));
    memset(&sb3, 0xab, sizeof(streambuf));

    /* constructors */
    call_func1(p_streambuf_ctor, &sb);
    ok(sb.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb.allocated);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    ok(sb.lock.LockCount == -1, "wrong critical section state, expected -1 got %lu\n", sb.lock.LockCount);
    call_func3(p_streambuf_reserve_ctor, &sb2, reserve, 16);
    ok(sb2.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb2.allocated);
    ok(sb2.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb2.unbuffered);
    ok(sb2.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb2.base);
    ok(sb2.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb2.ebuf);
    ok(sb.lock.LockCount == -1, "wrong critical section state, expected -1 got %lu\n", sb.lock.LockCount);
    call_func1(p_streambuf_ctor, &sb3);
    ok(sb3.allocated == 0, "wrong allocate value, expected 0 got %d\n", sb3.allocated);
    ok(sb3.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb3.unbuffered);
    ok(sb3.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb3.base);
    ok(sb3.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb3.ebuf);

    memcpy(test_streambuf_vtbl, sb.vtable, sizeof(test_streambuf_vtbl));
    test_streambuf_vtbl[7] = (vtable_ptr)&test_streambuf_overflow;
    test_streambuf_vtbl[8] = (vtable_ptr)&test_streambuf_underflow;
    sb2.vtable = test_streambuf_vtbl;
    sb3.vtable = test_streambuf_vtbl;
    overflow_count = underflow_count = 0;
    strcpy(test_get_buffer, "CompuGlobalHyperMegaNet");
    buffer_pos = get_end = 0;

    /* setlock */
    ok(sb.do_lock == -1, "expected do_lock value -1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == -2, "expected do_lock value -2, got %d\n", sb.do_lock);
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == -3, "expected do_lock value -3, got %d\n", sb.do_lock);
    sb.do_lock = 3;
    call_func1(p_streambuf_setlock, &sb);
    ok(sb.do_lock == 2, "expected do_lock value 2, got %d\n", sb.do_lock);

    /* clrlock */
    sb.do_lock = -2;
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == -1, "expected do_lock value -1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 0, "expected do_lock value 0, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 1, "expected do_lock value 1, got %d\n", sb.do_lock);
    call_func1(p_streambuf_clrclock, &sb);
    ok(sb.do_lock == 1, "expected do_lock value 1, got %d\n", sb.do_lock);

    /* lock/unlock */
    lock_arg.sb = &sb;
    for (i = 0; i < 4; i++) {
        lock_arg.lock[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
        ok(lock_arg.lock[i] != NULL, "CreateEventW failed\n");
        lock_arg.test[i] = CreateEventW(NULL, FALSE, FALSE, NULL);
        ok(lock_arg.test[i] != NULL, "CreateEventW failed\n");
    }

    sb.do_lock = 0;
    thread = CreateThread(NULL, 0, lock_streambuf, (void*)&lock_arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(lock_arg.lock[0], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked != 0, "could not lock the streambuf\n");
    LeaveCriticalSection(&sb.lock);

    sb.do_lock = 1;
    SetEvent(lock_arg.test[0]);
    WaitForSingleObject(lock_arg.lock[1], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked != 0, "could not lock the streambuf\n");
    LeaveCriticalSection(&sb.lock);

    sb.do_lock = -1;
    SetEvent(lock_arg.test[1]);
    WaitForSingleObject(lock_arg.lock[2], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked == 0, "the streambuf was not locked before\n");

    sb.do_lock = 0;
    SetEvent(lock_arg.test[2]);
    WaitForSingleObject(lock_arg.lock[3], INFINITE);
    locked = TryEnterCriticalSection(&sb.lock);
    ok(locked == 0, "the streambuf was not locked before\n");
    sb.do_lock = -1;

    /* setb */
    sb.unbuffered = 1;
    call_func4(p_streambuf_setb, &sb, reserve, reserve+16, 0);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);
    call_func4(p_streambuf_setb, &sb, reserve, reserve+16, 4);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.allocated == 4, "wrong allocate value, expected 4 got %d\n", sb.allocated);
    sb.allocated = 0;
    sb.unbuffered = 0;
    call_func4(p_streambuf_setb, &sb, NULL, NULL, 3);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);

    /* setbuf */
    sb.unbuffered = 0;
    psb = call_func3(p_streambuf_setbuf, &sb, NULL, 5);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, reserve, 0);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", sb.unbuffered);
    ok(sb.base == NULL, "wrong base pointer, expected %p got %p\n", NULL, sb.base);
    ok(sb.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, reserve, 16);
    ok(psb == &sb, "wrong return value, expected %p got %p\n", &sb, psb);
    ok(sb.allocated == 3, "wrong allocate value, expected 3 got %d\n", sb.allocated);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);
    psb = call_func3(p_streambuf_setbuf, &sb, NULL, 8);
    ok(psb == NULL, "wrong return value, expected %p got %p\n", NULL, psb);
    ok(sb.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", sb.unbuffered);
    ok(sb.base == reserve, "wrong base pointer, expected %p got %p\n", reserve, sb.base);
    ok(sb.ebuf == reserve+16, "wrong ebuf pointer, expected %p got %p\n", reserve+16, sb.ebuf);

    /* allocate */
    ret = (int) call_func1(p_streambuf_allocate, &sb);
    ok(ret == 0, "wrong return value, expected 0 got %d\n", ret);
    sb.base = NULL;
    ret = (int) call_func1(p_streambuf_allocate, &sb);
    ok(ret == 1, "wrong return value, expected 1 got %d\n", ret);
    ok(sb.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb.allocated);
    ok(sb.ebuf - sb.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb.ebuf, sb.base);

    /* doallocate */
    ret = (int) call_func1(p_streambuf_doallocate, &sb2);
    ok(ret == 1, "doallocate failed, got %d\n", ret);
    ok(sb2.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb2.allocated);
    ok(sb2.ebuf - sb2.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb2.ebuf, sb2.base);
    ret = (int) call_func1(p_streambuf_doallocate, &sb3);
    ok(ret == 1, "doallocate failed, got %d\n", ret);
    ok(sb3.allocated == 1, "wrong allocate value, expected 1 got %d\n", sb3.allocated);
    ok(sb3.ebuf - sb3.base == 512 , "wrong reserve area size, expected 512 got %p-%p\n", sb3.ebuf, sb3.base);

    /* sb: buffered, space available */
    sb.eback = sb.gptr = sb.base;
    sb.egptr = sb.base + 256;
    sb.pbase = sb.pptr = sb.base + 256;
    sb.epptr = sb.base + 512;
    /* sb2: buffered, no space available */
    sb2.eback = sb2.base;
    sb2.gptr = sb2.egptr = sb2.base + 256;
    sb2.pbase = sb2.base + 256;
    sb2.pptr = sb2.epptr = sb2.base + 512;
    /* sb3: unbuffered */
    sb3.unbuffered = 1;

    /* gbump */
    call_func2(p_streambuf_gbump, &sb, 10);
    ok(sb.gptr == sb.eback + 10, "advance get pointer failed, expected %p got %p\n", sb.eback + 10, sb.gptr);
    call_func2(p_streambuf_gbump, &sb, -15);
    ok(sb.gptr == sb.eback - 5, "advance get pointer failed, expected %p got %p\n", sb.eback - 5, sb.gptr);
    sb.gptr = sb.eback;

    /* pbump */
    call_func2(p_streambuf_pbump, &sb, -2);
    ok(sb.pptr == sb.pbase - 2, "advance put pointer failed, expected %p got %p\n", sb.pbase - 2, sb.pptr);
    call_func2(p_streambuf_pbump, &sb, 20);
    ok(sb.pptr == sb.pbase + 18, "advance put pointer failed, expected %p got %p\n", sb.pbase + 18, sb.pptr);
    sb.pptr = sb.pbase;

    /* sync */
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == EOF, "sync failed, expected EOF got %d\n", ret);
    sb.gptr = sb.egptr;
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb.gptr = sb.egptr + 1;
    ret = (int) call_func1(p_streambuf_sync, &sb);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb.gptr = sb.eback;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == EOF, "sync failed, expected EOF got %d\n", ret);
    sb2.pptr = sb2.pbase;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb2.pptr = sb2.pbase - 1;
    ret = (int) call_func1(p_streambuf_sync, &sb2);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);
    sb2.pptr = sb2.epptr;
    ret = (int) call_func1(p_streambuf_sync, &sb3);
    ok(ret == 0, "sync failed, expected 0 got %d\n", ret);

    /* sgetc */
    strcpy(sb2.eback, "WorstTestEver");
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_sgetc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(underflow_count == 1, "expected call to underflow\n");
    ok(sb2.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb2.stored_char);
    sb2.gptr++;
    ret = (int) call_func1(p_streambuf_sgetc, &sb2);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(underflow_count == 2, "expected call to underflow\n");
    ok(sb2.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb2.stored_char);
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_sgetc, &sb3);
    ok(ret == 'C', "expected 'C' got '%c'\n", ret);
    ok(underflow_count == 3, "expected call to underflow\n");
    ok(sb3.stored_char == 'C', "wrong stored character, expected 'C' got %c\n", sb3.stored_char);
    sb3.stored_char = 'b';
    ret = (int) call_func1(p_streambuf_sgetc, &sb3);
    ok(ret == 'b', "expected 'b' got '%c'\n", ret);
    ok(underflow_count == 3, "no call to underflow expected\n");
    ok(sb3.stored_char == 'b', "wrong stored character, expected 'b' got %c\n", sb3.stored_char);

    /* sputc */
    *sb.pbase = 'a';
    ret = (int) call_func2(p_streambuf_sputc, &sb, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 0, "no call to overflow expected\n");
    ok(*sb.pbase == 'c', "expected 'c' in the put area, got %c\n", *sb.pbase);
    ok(sb.pptr == sb.pbase + 1, "wrong put pointer, expected %p got %p\n", sb.pbase + 1, sb.pptr);
    sb.pptr--;
    ret = (int) call_func2(p_streambuf_sputc, &sb, 150);
    ok(ret == 150, "wrong return value, expected 150 got %d\n", ret);
    ok(overflow_count == 0, "no call to overflow expected\n");
    ok((signed char)*sb.pbase == -106, "expected -106 in the put area, got %d\n", *sb.pbase);
    ok(sb.pptr == sb.pbase + 1, "wrong put pointer, expected %p got %p\n", sb.pbase + 1, sb.pptr);
    sb.pptr--;
    ret = (int) call_func2(p_streambuf_sputc, &sb, -50);
    ok(ret == 206, "wrong return value, expected 206 got %d\n", ret);
    ok(overflow_count == 0, "no call to overflow expected\n");
    ok((signed char)*sb.pbase == -50, "expected -50 in the put area, got %d\n", *sb.pbase);
    ok(sb.pptr == sb.pbase + 1, "wrong put pointer, expected %p got %p\n", sb.pbase + 1, sb.pptr);
    test_this = &sb2;
    ret = (int) call_func2(p_streambuf_sputc, &sb2, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 1, "expected call to overflow\n");
    ok(sb2.pptr == sb2.pbase + 5, "wrong put pointer, expected %p got %p\n", sb2.pbase + 5, sb2.pptr);
    test_this = &sb3;
    ret = (int) call_func2(p_streambuf_sputc, &sb3, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 2, "expected call to overflow\n");
    sb3.pbase = sb3.pptr = sb3.base;
    sb3.epptr = sb3.ebuf;
    ret = (int) call_func2(p_streambuf_sputc, &sb3, 'c');
    ok(ret == 'c', "wrong return value, expected 'c' got %d\n", ret);
    ok(overflow_count == 2, "no call to overflow expected\n");
    ok(*sb3.pbase == 'c', "expected 'c' in the put area, got %c\n", *sb3.pbase);
    sb3.pbase = sb3.pptr = sb3.epptr = NULL;

    /* xsgetn */
    sb2.gptr = sb2.egptr = sb2.eback + 13;
    test_this = &sb2;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 5);
    ok(ret == 5, "wrong return value, expected 5 got %d\n", ret);
    ok(!strncmp(reserve, "Worst", 5), "expected 'Worst' got %s\n", reserve);
    ok(sb2.gptr == sb2.eback + 5, "wrong get pointer, expected %p got %p\n", sb2.eback + 5, sb2.gptr);
    ok(underflow_count == 4, "expected call to underflow\n");
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 4);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(!strncmp(reserve, "Test", 4), "expected 'Test' got %s\n", reserve);
    ok(sb2.gptr == sb2.eback + 9, "wrong get pointer, expected %p got %p\n", sb2.eback + 9, sb2.gptr);
    ok(underflow_count == 5, "expected call to underflow\n");
    get_end = 1;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb2, reserve, 16);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(!strncmp(reserve, "Ever", 4), "expected 'Ever' got %s\n", reserve);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 7, "expected 2 calls to underflow, got %d\n", underflow_count - 5);
    test_this = &sb3;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 11);
    ok(ret == 11, "wrong return value, expected 11 got %d\n", ret);
    ok(!strncmp(reserve, "bompuGlobal", 11), "expected 'bompuGlobal' got %s\n", reserve);
    ok(sb3.stored_char == 'H', "wrong stored character, expected 'H' got %c\n", sb3.stored_char);
    ok(underflow_count == 18, "expected 11 calls to underflow, got %d\n", underflow_count - 7);
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 16);
    ok(ret == 12, "wrong return value, expected 12 got %d\n", ret);
    ok(!strncmp(reserve, "HyperMegaNet", 12), "expected 'HyperMegaNet' got %s\n", reserve);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 30, "expected 12 calls to underflow, got %d\n", underflow_count - 18);
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 3);
    ok(ret == 0, "wrong return value, expected 0 got %d\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 31, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func3(p_streambuf_xsgetn, &sb3, reserve, 5);
    ok(ret == 5, "wrong return value, expected 5 got %d\n", ret);
    ok(!strncmp(reserve, "Compu", 5), "expected 'Compu' got %s\n", reserve);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(underflow_count == 37, "expected 6 calls to underflow, got %d\n", underflow_count - 31);

    /* xsputn */
    ret = (int) call_func3(p_streambuf_xsputn, &sb, "Test\0ing", 8);
    ok(ret == 8, "wrong return value, expected 8 got %d\n", ret);
    ok(sb.pptr == sb.pbase + 9, "wrong put pointer, expected %p got %p\n", sb.pbase + 9, sb.pptr);
    test_this = &sb2;
    sb2.pptr = sb2.epptr - 7;
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb2.pptr == sb2.epptr, "wrong put pointer, expected %p got %p\n", sb2.epptr, sb2.pptr);
    ok(overflow_count == 2, "no call to overflow expected\n");
    sb2.pptr = sb2.epptr - 5;
    sb2.pbase[5] = 'a';
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb2.pbase[5] == 'g', "expected 'g' got %c\n", sb2.pbase[5]);
    ok(sb2.pptr == sb2.pbase + 6, "wrong put pointer, expected %p got %p\n", sb2.pbase + 6, sb2.pptr);
    ok(overflow_count == 3, "expected call to overflow\n");
    sb2.pptr = sb2.epptr - 4;
    ret = (int) call_func3(p_streambuf_xsputn, &sb2, "TestLing", 8);
    ok(ret == 4, "wrong return value, expected 4 got %d\n", ret);
    ok(sb2.pptr == sb2.epptr, "wrong put pointer, expected %p got %p\n", sb2.epptr, sb2.pptr);
    ok(overflow_count == 4, "expected call to overflow\n");
    test_this = &sb3;
    ret = (int) call_func3(p_streambuf_xsputn, &sb3, "Testing", 7);
    ok(ret == 7, "wrong return value, expected 7 got %d\n", ret);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(overflow_count == 11, "expected 7 calls to overflow, got %d\n", overflow_count - 4);
    ret = (int) call_func3(p_streambuf_xsputn, &sb3, "TeLephone", 9);
    ok(ret == 2, "wrong return value, expected 2 got %d\n", ret);
    ok(sb3.stored_char == 'G', "wrong stored character, expected 'G' got %c\n", sb3.stored_char);
    ok(overflow_count == 14, "expected 3 calls to overflow, got %d\n", overflow_count - 11);

    /* snextc */
    strcpy(sb.eback, "Test");
    ret = (int) call_func1(p_streambuf_snextc, &sb);
    ok(ret == 'e', "expected 'e' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    *sb.gptr-- = -50;
    ret = (int) call_func1(p_streambuf_snextc, &sb);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr + 1, "wrong get pointer, expected %p got %p\n", sb2.egptr + 1, sb2.gptr);
    ok(underflow_count == 39, "expected 2 calls to underflow, got %d\n", underflow_count - 37);
    sb2.gptr = sb2.egptr - 1;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 40, "expected call to underflow\n");
    get_end = 0;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 41, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    ret = (int) call_func1(p_streambuf_snextc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback, "wrong get pointer, expected %p got %p\n", sb2.eback, sb2.gptr);
    ok(underflow_count == 42, "expected call to underflow\n");
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'l', "expected 'l' got '%c'\n", ret);
    ok(sb3.stored_char == 'l', "wrong stored character, expected 'l' got %c\n", sb3.stored_char);
    ok(underflow_count == 43, "expected call to underflow\n");
    buffer_pos = 22;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 't', "expected 't' got '%c'\n", ret);
    ok(sb3.stored_char == 't', "wrong stored character, expected 't' got %c\n", sb3.stored_char);
    ok(underflow_count == 44, "expected call to underflow\n");
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 45, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'o', "expected 'o' got '%c'\n", ret);
    ok(sb3.stored_char == 'o', "wrong stored character, expected 'o' got %c\n", sb3.stored_char);
    ok(underflow_count == 47, "expected 2 calls to underflow, got %d\n", underflow_count - 45);
    sb3.stored_char = EOF;
    ret = (int) call_func1(p_streambuf_snextc, &sb3);
    ok(ret == 'p', "expected 'p' got '%c'\n", ret);
    ok(sb3.stored_char == 'p', "wrong stored character, expected 'p' got %c\n", sb3.stored_char);
    ok(underflow_count == 49, "expected 2 calls to underflow, got %d\n", underflow_count - 47);

    /* sbumpc */
    sb.gptr = sb.eback;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb);
    ok(ret == 'T', "expected 'T' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    ret = (int) call_func1(p_streambuf_sbumpc, &sb);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(sb.gptr == sb.eback + 2, "wrong get pointer, expected %p got %p\n", sb.eback + 2, sb.gptr);
    test_this = &sb2;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == 'W', "expected 'W' got '%c'\n", ret);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 50, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    *sb2.gptr = 't';
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == 't', "expected 't' got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 50, "no call to underflow expected\n");
    get_end = 1;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb2);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr + 1, "wrong get pointer, expected %p got %p\n", sb2.egptr + 1, sb2.gptr);
    ok(underflow_count == 51, "expected call to underflow\n");
    sb2.gptr = sb2.egptr;
    test_this = &sb3;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'p', "expected 'p' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 51, "no call to underflow expected\n");
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'u', "expected 'u' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 52, "expected call to underflow\n");
    buffer_pos = 23;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 53, "expected call to underflow\n");
    buffer_pos = 0;
    ret = (int) call_func1(p_streambuf_sbumpc, &sb3);
    ok(ret == 'C', "expected 'C' got '%c'\n", ret);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 54, "expected call to underflow\n");

    /* stossc */
    call_func1(p_streambuf_stossc, &sb);
    ok(sb.gptr == sb.eback + 3, "wrong get pointer, expected %p got %p\n", sb.eback + 3, sb.gptr);
    test_this = &sb2;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 55, "expected call to underflow\n");
    get_end = 0;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.eback + 1, "wrong get pointer, expected %p got %p\n", sb2.eback + 1, sb2.gptr);
    ok(underflow_count == 56, "expected call to underflow\n");
    sb2.gptr = sb2.egptr - 1;
    call_func1(p_streambuf_stossc, &sb2);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(underflow_count == 56, "no call to underflow expected\n");
    test_this = &sb3;
    call_func1(p_streambuf_stossc, &sb3);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 57, "expected call to underflow\n");
    sb3.stored_char = 'a';
    call_func1(p_streambuf_stossc, &sb3);
    ok(sb3.stored_char == EOF, "wrong stored character, expected EOF got %c\n", sb3.stored_char);
    ok(underflow_count == 57, "no call to underflow expected\n");

    /* pbackfail */
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, 'A');
    ok(ret == 'A', "expected 'A' got '%c'\n", ret);
    ok(sb.gptr == sb.eback + 2, "wrong get pointer, expected %p got %p\n", sb.eback + 2, sb.gptr);
    ok(*sb.gptr == 'A', "expected 'A' in the get area, got %c\n", *sb.gptr);
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, EOF);
    ok(ret == (char)EOF, "expected EOF got %d\n", ret);
    ok(sb.gptr == sb.eback + 1, "wrong get pointer, expected %p got %p\n", sb.eback + 1, sb.gptr);
    ok((signed char)*sb.gptr == EOF, "expected EOF in the get area, got %c\n", *sb.gptr);
    sb.gptr = sb.eback;
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, 'Z');
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb.gptr == sb.eback, "wrong get pointer, expected %p got %p\n", sb.eback, sb.gptr);
    ok(*sb.gptr == 'T', "expected 'T' in the get area, got %c\n", *sb.gptr);
    ret = (int) call_func2(p_streambuf_pbackfail, &sb, EOF);
    ok(ret == EOF, "expected EOF got '%c'\n", ret);
    ok(sb.gptr == sb.eback, "wrong get pointer, expected %p got %p\n", sb.eback, sb.gptr);
    ok(*sb.gptr == 'T', "expected 'T' in the get area, got %c\n", *sb.gptr);
    sb2.gptr = sb2.egptr + 1;
    ret = (int) call_func2(p_streambuf_pbackfail, &sb2, 'X');
    ok(ret == 'X', "expected 'X' got '%c'\n", ret);
    ok(sb2.gptr == sb2.egptr, "wrong get pointer, expected %p got %p\n", sb2.egptr, sb2.gptr);
    ok(*sb2.gptr == 'X', "expected 'X' in the get area, got %c\n", *sb2.gptr);

    /* out_waiting */
    ret = (int) call_func1(p_streambuf_out_waiting, &sb);
    ok(ret == 9, "expected 9 got %d\n", ret);
    sb.pptr = sb.pbase;
    ret = (int) call_func1(p_streambuf_out_waiting, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);
    sb.pptr = sb.pbase - 1;
    ret = (int) call_func1(p_streambuf_out_waiting, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);
    sb.pptr = NULL;
    ret = (int) call_func1(p_streambuf_out_waiting, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);
    sb.pptr = sb.epptr;
    sb.pbase = NULL;
    ret = (int) call_func1(p_streambuf_out_waiting, &sb);
    ok(ret == (int)(sb.pptr - sb.pbase), "expected %d got %d\n", (int)(sb.pptr - sb.pbase), ret);

    /* in_avail */
    ret = (int) call_func1(p_streambuf_in_avail, &sb);
    ok(ret == 256, "expected 256 got %d\n", ret);
    sb.gptr = sb.egptr;
    ret = (int) call_func1(p_streambuf_in_avail, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);
    sb.gptr = sb.egptr + 1;
    ret = (int) call_func1(p_streambuf_in_avail, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);
    sb.egptr = NULL;
    ret = (int) call_func1(p_streambuf_in_avail, &sb);
    ok(ret == 0, "expected 0 got %d\n", ret);

    SetEvent(lock_arg.test[3]);
    WaitForSingleObject(thread, INFINITE);

    call_func1(p_streambuf_dtor, &sb);
    call_func1(p_streambuf_dtor, &sb2);
    call_func1(p_streambuf_dtor, &sb3);
    for (i = 0; i < 4; i++) {
        CloseHandle(lock_arg.lock[i]);
        CloseHandle(lock_arg.test[i]);
    }
    CloseHandle(thread);
}

struct filebuf_lock_arg
{
    filebuf *fb1, *fb2, *fb3;
    HANDLE lock;
    HANDLE test;
};

static DWORD WINAPI lock_filebuf(void *arg)
{
    struct filebuf_lock_arg *lock_arg = arg;
    call_func1(p_streambuf_lock, &lock_arg->fb1->base);
    call_func1(p_streambuf_lock, &lock_arg->fb2->base);
    call_func1(p_streambuf_lock, &lock_arg->fb3->base);
    SetEvent(lock_arg->lock);
    WaitForSingleObject(lock_arg->test, INFINITE);
    call_func1(p_streambuf_unlock, &lock_arg->fb1->base);
    call_func1(p_streambuf_unlock, &lock_arg->fb2->base);
    call_func1(p_streambuf_unlock, &lock_arg->fb3->base);
    return 0;
}

static void test_filebuf(void)
{
    filebuf fb1, fb2, fb3, *pret;
    struct filebuf_lock_arg lock_arg;
    HANDLE thread;
    const char filename1[] = "test1";
    const char filename2[] = "test2";
    const char filename3[] = "test3";
    char read_buffer[16];
    int ret;

    memset(&fb1, 0xab, sizeof(filebuf));
    memset(&fb2, 0xab, sizeof(filebuf));
    memset(&fb3, 0xab, sizeof(filebuf));

    /* constructors */
    call_func2(p_filebuf_fd_ctor, &fb1, 1);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fb1.base.unbuffered);
    ok(fb1.fd == 1, "wrong fd, expected 1 got %d\n", fb1.fd);
    ok(fb1.close == 0, "wrong value, expected 0 got %d\n", fb1.close);
    call_func4(p_filebuf_fd_reserve_ctor, &fb2, 4, NULL, 0);
    ok(fb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb2.base.allocated);
    ok(fb2.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb2.base.unbuffered);
    ok(fb2.fd == 4, "wrong fd, expected 4 got %d\n", fb2.fd);
    ok(fb2.close == 0, "wrong value, expected 0 got %d\n", fb2.close);
    call_func1(p_filebuf_ctor, &fb3);
    ok(fb3.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb3.base.allocated);
    ok(fb3.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fb3.base.unbuffered);
    ok(fb3.fd == -1, "wrong fd, expected -1 got %d\n", fb3.fd);
    ok(fb3.close == 0, "wrong value, expected 0 got %d\n", fb3.close);

    lock_arg.fb1 = &fb1;
    lock_arg.fb2 = &fb2;
    lock_arg.fb3 = &fb3;
    lock_arg.lock = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.lock != NULL, "CreateEventW failed\n");
    lock_arg.test = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.test != NULL, "CreateEventW failed\n");
    thread = CreateThread(NULL, 0, lock_filebuf, (void*)&lock_arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(lock_arg.lock, INFINITE);

    /* setbuf */
    fb1.base.do_lock = 0;
    fb1.fd = -1; /* closed */
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fb1.base.unbuffered);
    ok(fb1.fd == -1, "wrong fd, expected -1 got %d\n", fb1.fd);
    fb1.base.unbuffered = 1;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer, 16);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer, "wrong buffer, expected %p got %p\n", read_buffer, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer + 8, 8);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer + 8, "wrong buffer, expected %p got %p\n", read_buffer + 8, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    fb1.base.unbuffered = 0;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, NULL, 0);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer + 8, "wrong buffer, expected %p got %p\n", read_buffer + 8, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    ok((int) call_func1(p_streambuf_doallocate, &fb1.base) == 1, "failed to allocate buffer\n");
    ok(fb1.base.allocated == 1, "wrong allocate value, expected 1 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base != NULL, "wrong buffer, expected not NULL got %p\n", fb1.base.base);
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer + 2, 14);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer + 2, "wrong buffer, expected %p got %p\n", read_buffer + 2, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    fb1.fd = 1; /* opened */
    fb1.base.unbuffered = 1;
    fb1.base.base = fb1.base.ebuf = NULL;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer, 16);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer, "wrong buffer, expected %p got %p\n", read_buffer, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer + 8, 8);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer, "wrong buffer, expected %p got %p\n", read_buffer, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    fb1.base.unbuffered = 0;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, NULL, 0);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer, "wrong buffer, expected %p got %p\n", read_buffer, fb1.base.base);
    ok(fb1.base.pbase == NULL, "wrong put area, expected %p got %p\n", NULL, fb1.base.pbase);
    fb1.base.unbuffered = 1;
    fb1.base.pbase = fb1.base.pptr = fb1.base.base;
    fb1.base.epptr = fb1.base.ebuf;
    fb1.base.do_lock = -1;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer, 16);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == read_buffer, "wrong buffer, expected %p got %p\n", read_buffer, fb1.base.base);
    ok(fb1.base.pbase == read_buffer, "wrong put area, expected %p got %p\n", read_buffer, fb1.base.pbase);
    fb1.base.base = fb1.base.ebuf = NULL;
    fb1.base.do_lock = 0;
    fb1.base.unbuffered = 0;
    pret = (filebuf*) call_func3(p_filebuf_setbuf, &fb1, read_buffer, 0);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fb1.base.unbuffered);
    ok(fb1.base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, fb1.base.base);
    ok(fb1.base.pbase == read_buffer, "wrong put area, expected %p got %p\n", read_buffer, fb1.base.pbase);
    fb1.base.pbase = fb1.base.pptr = fb1.base.epptr = NULL;
    fb1.base.unbuffered = 0;
    fb1.base.do_lock = -1;

    /* attach */
    pret = call_func2(p_filebuf_attach, &fb1, 2);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    ok(fb1.fd == 1, "wrong fd, expected 1 got %d\n", fb1.fd);
    fb2.fd = -1;
    fb2.base.do_lock = 0;
    pret = call_func2(p_filebuf_attach, &fb2, 3);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    ok(fb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb2.base.allocated);
    ok(fb2.fd == 3, "wrong fd, expected 3 got %d\n", fb2.fd);
    fb2.base.do_lock = -1;
    fb3.base.do_lock = 0;
    pret = call_func2(p_filebuf_attach, &fb3, 2);
    ok(pret == &fb3, "wrong return, expected %p got %p\n", &fb3, pret);
    ok(fb3.base.allocated == 1, "wrong allocate value, expected 1 got %d\n", fb3.base.allocated);
    ok(fb3.fd == 2, "wrong fd, expected 2 got %d\n", fb3.fd);
    fb3.base.do_lock = -1;

    /* open modes */
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_out, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    fb1.fd = -1;
    pret = call_func4(p_filebuf_open, &fb1, filename1,
        OPENMODE_ate|OPENMODE_nocreate|OPENMODE_noreplace|OPENMODE_binary, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", fb1.base.allocated);
    fb1.base.do_lock = 0;
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_out, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.base.allocated == 1, "wrong allocate value, expected 1 got %d\n", fb1.base.allocated);
    ok(_write(fb1.fd, "testing", 7) == 7, "_write failed\n");
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(fb1.fd == -1, "wrong fd, expected -1 got %d\n", fb1.fd);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_out, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_read(fb1.fd, read_buffer, 1) == -1, "file should not be open for reading\n");
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_app, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_read(fb1.fd, read_buffer, 1) == -1, "file should not be open for reading\n");
    ok(_write(fb1.fd, "testing", 7) == 7, "_write failed\n");
    ok(_lseek(fb1.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_write(fb1.fd, "append", 6) == 6, "_write failed\n");
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_out|OPENMODE_ate, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_read(fb1.fd, read_buffer, 1) == -1, "file should not be open for reading\n");
    ok(_lseek(fb1.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_write(fb1.fd, "ate", 3) == 3, "_write failed\n");
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_in, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_read(fb1.fd, read_buffer, 13) == 13, "read failed\n");
    read_buffer[13] = 0;
    ok(!strncmp(read_buffer, "atetingappend", 13), "wrong contents, expected 'atetingappend' got '%s'\n", read_buffer);
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_in|OPENMODE_trunc, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_read(fb1.fd, read_buffer, 1) == 0, "read failed\n");
    ok(_write(fb1.fd, "file1", 5) == 5, "_write failed\n");
    pret = call_func1(p_filebuf_close, &fb1);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    pret = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_in|OPENMODE_app, filebuf_openprot);
    ok(pret == &fb1, "wrong return, expected %p got %p\n", &fb1, pret);
    ok(_write(fb1.fd, "app", 3) == 3, "_write failed\n");
    ok(_read(fb1.fd, read_buffer, 1) == 0, "read failed\n");
    ok(_lseek(fb1.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_read(fb1.fd, read_buffer, 8) == 8, "read failed\n");
    read_buffer[8] = 0;
    ok(!strncmp(read_buffer, "file1app", 8), "wrong contents, expected 'file1app' got '%s'\n", read_buffer);
    fb1.base.do_lock = -1;

    fb2.fd = -1;
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_out|OPENMODE_nocreate, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_in|OPENMODE_nocreate, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    fb2.base.do_lock = 0;
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_in, filebuf_openprot);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    ok(_read(fb1.fd, read_buffer, 1) == 0, "read failed\n");
    pret = call_func1(p_filebuf_close, &fb2);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    fb2.base.do_lock = -1;
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_in|OPENMODE_noreplace, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_trunc|OPENMODE_noreplace, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    pret = call_func4(p_filebuf_open, &fb2, filename3, OPENMODE_out|OPENMODE_nocreate|OPENMODE_noreplace, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);

    /* open protection*/
    fb3.fd = -1;
    fb3.base.do_lock = 0;
    pret = call_func4(p_filebuf_open, &fb3, filename3, OPENMODE_in|OPENMODE_out, filebuf_openprot);
    ok(pret == &fb3, "wrong return, expected %p got %p\n", &fb3, pret);
    ok(_write(fb3.fd, "You wouldn't\nget this from\nany other guy", 40) == 40, "_write failed\n");
    fb2.base.do_lock = 0;
    pret = call_func4(p_filebuf_open, &fb2, filename3, OPENMODE_in|OPENMODE_out, filebuf_openprot);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    pret = call_func1(p_filebuf_close, &fb2);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    fb2.base.do_lock = -1;
    pret = call_func1(p_filebuf_close, &fb3);
    ok(pret == &fb3, "wrong return, expected %p got %p\n", &fb3, pret);
    pret = call_func4(p_filebuf_open, &fb3, filename3, OPENMODE_in, filebuf_sh_none);
    ok(pret == &fb3, "wrong return, expected %p got %p\n", &fb3, pret);
    pret = call_func4(p_filebuf_open, &fb2, filename3, OPENMODE_in, filebuf_openprot);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    fb3.base.do_lock = -1;

    /* setmode */
    fb1.base.do_lock = 0;
    fb1.base.pbase = fb1.base.pptr = fb1.base.base;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func2(p_filebuf_setmode, &fb1, filebuf_binary);
    ok(ret == filebuf_text, "wrong return, expected %d got %d\n", filebuf_text, ret);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ret = (int) call_func2(p_filebuf_setmode, &fb1, filebuf_binary);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    fb1.base.do_lock = -1;
    ret = (int) call_func2(p_filebuf_setmode, &fb1, 0x9000);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    fb2.base.do_lock = 0;
    ret = (int) call_func2(p_filebuf_setmode, &fb2, filebuf_text);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    fb2.base.do_lock = -1;

    /* sync */
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.pbase = fb1.base.pptr = fb1.base.base + 256;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "We're no strangers to love\n", 27);
    ok(ret == 27, "wrong return, expected 27 got %d\n", ret);
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.egptr = NULL;
    fb1.base.pbase = fb1.base.pptr = fb1.base.base;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "You know the rules and so do I\n", 31);
    ok(ret == 31, "wrong return, expected 31 got %d\n", ret);
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.base.eback = fb1.base.base;
    fb1.base.gptr = fb1.base.base + 190;
    fb1.base.egptr = fb1.base.pbase = fb1.base.pptr = fb1.base.base + 256;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_tell(fb1.fd) == 0, "_tell failed\n");
    fb1.base.pbase = fb1.base.pptr = fb1.base.base;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb1.base.epptr);
    fb1.base.pbase = fb1.base.base;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    fb1.base.pbase = fb1.base.base;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb1.base.epptr);
    fb1.base.eback = fb1.base.base;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.gptr);
    fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 100;
    fb1.base.pbase = fb1.base.base;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 100, "wrong get base, expected %p got %p\n", fb1.base.base + 100, fb1.base.egptr);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    fb1.base.egptr = NULL;
    ret = (int) call_func1(p_filebuf_sync, &fb1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.egptr);
    ret = (int) call_func1(p_filebuf_sync, &fb2);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    fb3.base.eback = fb3.base.base;
    fb3.base.gptr = fb3.base.egptr = fb3.base.pbase = fb3.base.pptr = fb3.base.base + 256;
    fb3.base.epptr = fb3.base.ebuf;
    ret = (int) call_func3(p_streambuf_xsputn, &fb3.base, "A full commitment's what I'm thinking of\n", 41);
    ok(ret == 41, "wrong return, expected 41 got %d\n", ret);
    ret = (int) call_func1(p_filebuf_sync, &fb3);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(fb3.base.gptr == fb3.base.base + 256, "wrong get pointer, expected %p got %p\n", fb3.base.base + 256, fb3.base.gptr);
    ok(fb3.base.pptr == fb3.base.base + 297, "wrong put pointer, expected %p got %p\n", fb3.base.base + 297, fb3.base.pptr);
    fb3.base.eback = fb3.base.base;
    fb3.base.gptr = fb3.base.egptr = fb3.base.pbase = fb3.base.pptr = fb3.base.base + 256;
    fb3.base.epptr = fb3.base.ebuf;
    ret = (int) call_func1(p_filebuf_sync, &fb3);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb3.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb3.base.gptr);
    ok(fb3.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb3.base.pptr);
    fb3.base.eback = fb3.base.base;
    fb3.base.gptr = fb3.base.base + 216;
    fb3.base.egptr = fb3.base.pbase = fb3.base.pptr = fb3.base.base + 256;
    fb3.base.epptr = fb3.base.ebuf;
    ok(_read(fb3.fd, fb3.base.gptr, 42) == 40, "read failed\n");
    ret = (int) call_func1(p_filebuf_sync, &fb3);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(fb3.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb3.base.gptr);
    ok(fb3.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb3.base.pptr);
    ok(_tell(fb3.fd) == 0, "_tell failed\n");
    fb3.base.eback = fb3.base.gptr = fb3.base.egptr = NULL;
    fb3.base.pbase = fb3.base.pptr = fb3.base.epptr = NULL;

    /* overflow */
    ret = (int) call_func2(p_filebuf_overflow, &fb1, EOF);
    ok(ret == 1, "wrong return, expected 1 got %d\n", ret);
    fb1.base.pbase = fb1.base.pptr = fb1.base.epptr = NULL;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.ebuf;
    ret = (int) call_func2(p_filebuf_overflow, &fb1, 'a');
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    fb1.base.gptr = fb1.base.egptr = fb1.base.pbase = fb1.base.pptr = fb1.base.base + 256;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "I just want to tell you how I'm feeling", 39);
    ok(ret == 39, "wrong return, expected 39 got %d\n", ret);
    ret = (int) call_func2(p_filebuf_overflow, &fb1, '\n');
    ok(ret == 1, "wrong return, expected 1 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 1, "wrong put pointer, expected %p got %p\n", fb1.base.base + 1, fb1.base.pptr);
    ok(fb1.base.epptr == fb1.base.ebuf, "wrong put end pointer, expected %p got %p\n", fb1.base.ebuf, fb1.base.epptr);
    ok(*fb1.base.pbase == '\n', "wrong character, expected '\\n' got '%c'\n", *fb1.base.pbase);
    ret = (int) call_func2(p_filebuf_overflow, &fb1, EOF);
    ok(ret == 1, "wrong return, expected 1 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base, "wrong put pointer, expected %p got %p\n", fb1.base.base, fb1.base.pptr);
    ok(fb1.base.epptr == fb1.base.ebuf, "wrong put end pointer, expected %p got %p\n", fb1.base.ebuf, fb1.base.epptr);
    ret = (int) call_func2(p_filebuf_overflow, &fb2, EOF);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    fb2.base.do_lock = 0;
    pret = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_in|OPENMODE_out, filebuf_openprot);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    fb2.base.do_lock = -1;
    ret = (int) call_func2(p_filebuf_overflow, &fb2, EOF);
    ok(ret == 1, "wrong return, expected 1 got %d\n", ret);

    /* underflow */
    ret = (int) call_func1(p_filebuf_underflow, &fb1);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.pbase = fb1.base.pptr = fb1.base.base + 256;
    fb1.base.epptr = fb1.base.ebuf;
    *fb1.base.gptr = 'A';
    ret = (int) call_func1(p_filebuf_underflow, &fb1);
    ok(ret == 'A', "wrong return, expected 'A' got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.pptr == fb1.base.base + 256, "wrong put pointer, expected %p got %p\n", fb1.base.base + 256, fb1.base.pptr);
    *fb1.base.gptr = -50;
    ret = (int) call_func1(p_filebuf_underflow, &fb1);
    ok(ret == 206, "wrong return, expected 206 got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.pptr == fb1.base.base + 256, "wrong put pointer, expected %p got %p\n", fb1.base.base + 256, fb1.base.pptr);
    fb1.base.gptr = fb1.base.ebuf;
    ret = (int) call_func1(p_filebuf_underflow, &fb1);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_lseek(fb1.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ret = (int) call_func1(p_filebuf_underflow, &fb1);
    ok(ret == 'f', "wrong return, expected 'f' got %d\n", ret);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 106, "wrong get end pointer, expected %p got %p\n", fb1.base.base + 106, fb1.base.egptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ret = (int) call_func1(p_filebuf_underflow, &fb2);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ok(_write(fb2.fd, "A\n", 2) == 2, "_write failed\n");
    ok(_lseek(fb2.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ret = (int) call_func1(p_filebuf_underflow, &fb2);
    ok(ret == 'A', "wrong return, expected 'A' got %d\n", ret);
    ret = (int) call_func1(p_filebuf_underflow, &fb2);
    ok(ret == '\n', "wrong return, expected '\\n' got %d\n", ret);
    ok(_lseek(fb2.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_write(fb2.fd, "\xCE\n", 2) == 2, "_write failed\n");
    ok(_lseek(fb2.fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ret = (int) call_func1(p_filebuf_underflow, &fb2);
    ok(ret == 206, "wrong return, expected 206 got %d\n", ret);
    fb2.base.do_lock = 0;
    pret = call_func1(p_filebuf_close, &fb2);
    ok(pret == &fb2, "wrong return, expected %p got %p\n", &fb2, pret);
    fb2.base.do_lock = -1;
    ret = (int) call_func1(p_filebuf_underflow, &fb2);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);

    /* seekoff */
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 5, SEEKDIR_beg, 0);
    ok(ret == 5, "wrong return, expected 5 got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.ebuf;
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 0, SEEKDIR_beg, 0);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    fb1.base.eback = fb1.base.gptr = fb1.base.egptr = NULL;
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 10, SEEKDIR_beg, OPENMODE_in|OPENMODE_out);
    ok(ret == 10, "wrong return, expected 10 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 0, SEEKDIR_cur, 0);
    ok(ret == 10, "wrong return, expected 10 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 200, SEEKDIR_cur, OPENMODE_in);
    ok(ret == 210, "wrong return, expected 210 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, -60, SEEKDIR_cur, 0);
    ok(ret == 150, "wrong return, expected 150 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 0, SEEKDIR_end, 0);
    ok(ret == 106, "wrong return, expected 106 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 20, SEEKDIR_end, OPENMODE_out);
    ok(ret == 126, "wrong return, expected 126 got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, -150, SEEKDIR_end, -1);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 10, 3, 0);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 16, SEEKDIR_beg, 0);
    ok(ret == 16, "wrong return, expected 16 got %d\n", ret);

    /* pbackfail */
    ret = (int) call_func2(p_streambuf_pbackfail, &fb1.base, 'B');
    ok(ret == 'B', "wrong return, expected 'B' got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_tell(fb1.fd) == 15, "_tell failed\n");
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.pbase = fb1.base.pptr = fb1.base.base + 256;
    fb1.base.epptr = fb1.base.ebuf;
    ret = (int) call_func2(p_streambuf_pbackfail, &fb1.base, 'C');
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_tell(fb1.fd) == 15, "_tell failed\n");
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 2;
    ret = (int) call_func2(p_streambuf_pbackfail, &fb1.base, 'C');
    ok(ret == 'C', "wrong return, expected 'C' got %d\n", ret);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_tell(fb1.fd) == 12, "_tell failed\n");
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.ebuf;
    *fb1.base.gptr++ = 'D';
    ret = (int) call_func2(p_streambuf_pbackfail, &fb1.base, 'C');
    ok(ret == 'C', "wrong return, expected 'C' got %d\n", ret);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(*fb1.base.gptr == 'C', "wrong character, expected 'C' got '%c'\n", *fb1.base.gptr);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(_tell(fb1.fd) == 12, "_tell failed\n");
    fb1.base.eback = fb1.base.gptr = fb1.base.egptr = NULL;

    /* close */
    pret = call_func1(p_filebuf_close, &fb2);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    fb3.base.do_lock = 0;
    pret = call_func1(p_filebuf_close, &fb3);
    ok(pret == &fb3, "wrong return, expected %p got %p\n", &fb3, pret);
    ok(fb3.fd == -1, "wrong fd, expected -1 got %d\n", fb3.fd);
    fb3.fd = 5;
    pret = call_func1(p_filebuf_close, &fb3);
    ok(pret == NULL, "wrong return, expected %p got %p\n", NULL, pret);
    ok(fb3.fd == 5, "wrong fd, expected 5 got %d\n", fb3.fd);
    fb3.base.do_lock = -1;

    SetEvent(lock_arg.test);
    WaitForSingleObject(thread, INFINITE);

    /* destructor */
    call_func1(p_filebuf_dtor, &fb1);
    ok(fb1.fd == -1, "wrong fd, expected -1 got %d\n", fb1.fd);
    call_func1(p_filebuf_dtor, &fb2);
    call_func1(p_filebuf_dtor, &fb3);

    ok(_unlink(filename1) == 0, "Couldn't unlink file named '%s'\n", filename1);
    ok(_unlink(filename2) == 0, "Couldn't unlink file named '%s'\n", filename2);
    ok(_unlink(filename3) == 0, "Couldn't unlink file named '%s'\n", filename3);
    CloseHandle(lock_arg.lock);
    CloseHandle(lock_arg.test);
    CloseHandle(thread);
}

static void test_strstreambuf(void)
{
    strstreambuf ssb1, ssb2;
    streambuf *pret;
    char buffer[64], *pbuffer;
    int ret;

    memset(&ssb1, 0xab, sizeof(strstreambuf));
    memset(&ssb2, 0xab, sizeof(strstreambuf));

    /* constructors/destructor */
    call_func2(p_strstreambuf_dynamic_ctor, &ssb1, 64);
    ok(ssb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb1.base.allocated);
    ok(ssb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb1.base.unbuffered);
    ok(ssb1.dynamic == 1, "expected 1, got %d\n", ssb1.dynamic);
    ok(ssb1.increase == 64, "expected 64, got %d\n", ssb1.increase);
    ok(ssb1.constant == 0, "expected 0, got %d\n", ssb1.constant);
    ok(ssb1.f_alloc == NULL, "expected %p, got %p\n", NULL, ssb1.f_alloc);
    ok(ssb1.f_free == NULL, "expected %p, got %p\n", NULL, ssb1.f_free);
    call_func1(p_strstreambuf_dtor, &ssb1);
    call_func3(p_strstreambuf_funcs_ctor, &ssb2, (allocFunction)p_operator_new, p_operator_delete);
    ok(ssb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb2.base.allocated);
    ok(ssb2.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb2.base.unbuffered);
    ok(ssb2.dynamic == 1, "expected 1, got %d\n", ssb2.dynamic);
    ok(ssb2.increase == 1, "expected 1, got %d\n", ssb2.increase);
    ok(ssb2.constant == 0, "expected 0, got %d\n", ssb2.constant);
    ok(ssb2.f_alloc == (allocFunction)p_operator_new, "expected %p, got %p\n", p_operator_new, ssb2.f_alloc);
    ok(ssb2.f_free == p_operator_delete, "expected %p, got %p\n", p_operator_delete, ssb2.f_free);
    call_func1(p_strstreambuf_dtor, &ssb2);
    memset(&ssb1, 0xab, sizeof(strstreambuf));
    memset(&ssb2, 0xab, sizeof(strstreambuf));
    call_func4(p_strstreambuf_buffer_ctor, &ssb1, buffer, 64, buffer + 20);
    ok(ssb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb1.base.allocated);
    ok(ssb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb1.base.unbuffered);
    ok(ssb1.base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, ssb1.base.base);
    ok(ssb1.base.ebuf == buffer + 64, "wrong buffer end, expected %p got %p\n", buffer + 64, ssb1.base.ebuf);
    ok(ssb1.base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, ssb1.base.eback);
    ok(ssb1.base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, ssb1.base.gptr);
    ok(ssb1.base.egptr == buffer + 20, "wrong get end, expected %p got %p\n", buffer + 20, ssb1.base.egptr);
    ok(ssb1.base.pbase == buffer + 20, "wrong put base, expected %p got %p\n", buffer + 20, ssb1.base.pbase);
    ok(ssb1.base.pptr == buffer + 20, "wrong put pointer, expected %p got %p\n", buffer + 20, ssb1.base.pptr);
    ok(ssb1.base.epptr == buffer + 64, "wrong put end, expected %p got %p\n", buffer + 64, ssb1.base.epptr);
    ok(ssb1.dynamic == 0, "expected 0, got %d\n", ssb1.dynamic);
    ok(ssb1.constant == 1, "expected 1, got %d\n", ssb1.constant);
    call_func1(p_strstreambuf_dtor, &ssb1);
    strcpy(buffer, "Test");
    call_func4(p_strstreambuf_buffer_ctor, &ssb2, buffer, 0, buffer + 20);
    ok(ssb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb2.base.allocated);
    ok(ssb2.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb2.base.unbuffered);
    ok(ssb2.base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, ssb2.base.base);
    ok(ssb2.base.ebuf == buffer + 4, "wrong buffer end, expected %p got %p\n", buffer + 4, ssb2.base.ebuf);
    ok(ssb2.base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, ssb2.base.eback);
    ok(ssb2.base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, ssb2.base.gptr);
    ok(ssb2.base.egptr == buffer + 20, "wrong get end, expected %p got %p\n", buffer + 20, ssb2.base.egptr);
    ok(ssb2.base.pbase == buffer + 20, "wrong put base, expected %p got %p\n", buffer + 20, ssb2.base.pbase);
    ok(ssb2.base.pptr == buffer + 20, "wrong put pointer, expected %p got %p\n", buffer + 20, ssb2.base.pptr);
    ok(ssb2.base.epptr == buffer + 4, "wrong put end, expected %p got %p\n", buffer + 4, ssb2.base.epptr);
    ok(ssb2.dynamic == 0, "expected 0, got %d\n", ssb2.dynamic);
    ok(ssb2.constant == 1, "expected 1, got %d\n", ssb2.constant);
    call_func1(p_strstreambuf_dtor, &ssb2);
    memset(&ssb1, 0xab, sizeof(strstreambuf));
    memset(&ssb2, 0xab, sizeof(strstreambuf));
    call_func4(p_strstreambuf_buffer_ctor, &ssb1, NULL, 16, buffer + 20);
    ok(ssb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb1.base.allocated);
    ok(ssb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb1.base.unbuffered);
    ok(ssb1.base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, ssb1.base.base);
    ok(ssb1.base.ebuf == (char*)16, "wrong buffer end, expected %p got %p\n", (char*)16, ssb1.base.ebuf);
    ok(ssb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, ssb1.base.eback);
    ok(ssb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, ssb1.base.gptr);
    ok(ssb1.base.egptr == buffer + 20, "wrong get end, expected %p got %p\n", buffer + 20, ssb1.base.egptr);
    ok(ssb1.base.pbase == buffer + 20, "wrong put base, expected %p got %p\n", buffer + 20, ssb1.base.pbase);
    ok(ssb1.base.pptr == buffer + 20, "wrong put pointer, expected %p got %p\n", buffer + 20, ssb1.base.pptr);
    ok(ssb1.base.epptr == (char*)16, "wrong put end, expected %p got %p\n", (char*)16, ssb1.base.epptr);
    ok(ssb1.dynamic == 0, "expected 0, got %d\n", ssb1.dynamic);
    ok(ssb1.constant == 1, "expected 1, got %d\n", ssb1.constant);
    call_func1(p_strstreambuf_dtor, &ssb1);
    call_func4(p_strstreambuf_buffer_ctor, &ssb2, buffer, 0, NULL);
    ok(ssb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb2.base.allocated);
    ok(ssb2.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb2.base.unbuffered);
    ok(ssb2.base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, ssb2.base.base);
    ok(ssb2.base.ebuf == buffer + 4, "wrong buffer end, expected %p got %p\n", buffer + 4, ssb2.base.ebuf);
    ok(ssb2.base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, ssb2.base.eback);
    ok(ssb2.base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, ssb2.base.gptr);
    ok(ssb2.base.egptr == buffer + 4, "wrong get end, expected %p got %p\n", buffer + 4, ssb2.base.egptr);
    ok(ssb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, ssb2.base.pbase);
    ok(ssb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, ssb2.base.pptr);
    ok(ssb2.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, ssb2.base.epptr);
    ok(ssb2.dynamic == 0, "expected 0, got %d\n", ssb2.dynamic);
    ok(ssb2.constant == 1, "expected 1, got %d\n", ssb2.constant);
    call_func1(p_strstreambuf_dtor, &ssb2);
    memset(&ssb1, 0xab, sizeof(strstreambuf));
    memset(&ssb2, 0xab, sizeof(strstreambuf));
    call_func4(p_strstreambuf_buffer_ctor, &ssb1, buffer, -5, buffer + 20);
    ok(ssb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb1.base.allocated);
    ok(ssb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb1.base.unbuffered);
    ok(ssb1.base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, ssb1.base.base);
    ok(ssb1.base.ebuf == (char *)((ULONG_PTR)buffer + 0x7fffffff) || ssb1.base.ebuf == (char*)-1,
        "wrong buffer end, expected %p + 0x7fffffff or -1, got %p\n", buffer, ssb1.base.ebuf);
    ok(ssb1.base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, ssb1.base.eback);
    ok(ssb1.base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, ssb1.base.gptr);
    ok(ssb1.base.egptr == buffer + 20, "wrong get end, expected %p got %p\n", buffer + 20, ssb1.base.egptr);
    ok(ssb1.base.pbase == buffer + 20, "wrong put base, expected %p got %p\n", buffer + 20, ssb1.base.pbase);
    ok(ssb1.base.pptr == buffer + 20, "wrong put pointer, expected %p got %p\n", buffer + 20, ssb1.base.pptr);
    ok(ssb1.base.epptr == (char *)((ULONG_PTR)buffer + 0x7fffffff) || ssb1.base.epptr == (char*)-1,
        "wrong put end, expected %p + 0x7fffffff or -1, got %p\n", buffer, ssb1.base.epptr);
    ok(ssb1.dynamic == 0, "expected 0, got %d\n", ssb1.dynamic);
    ok(ssb1.constant == 1, "expected 1, got %d\n", ssb1.constant);
    call_func1(p_strstreambuf_ctor, &ssb2);
    ok(ssb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", ssb2.base.allocated);
    ok(ssb2.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ssb2.base.unbuffered);
    ok(ssb2.dynamic == 1, "expected 1, got %d\n", ssb2.dynamic);
    ok(ssb2.increase == 1, "expected 1, got %d\n", ssb2.increase);
    ok(ssb2.constant == 0, "expected 0, got %d\n", ssb2.constant);
    ok(ssb2.f_alloc == NULL, "expected %p, got %p\n", NULL, ssb2.f_alloc);
    ok(ssb2.f_free == NULL, "expected %p, got %p\n", NULL, ssb2.f_free);

    /* freeze */
    call_func2(p_strstreambuf_freeze, &ssb1, 0);
    ok(ssb1.dynamic == 0, "expected 0, got %d\n", ssb1.dynamic);
    ssb1.constant = 0;
    call_func2(p_strstreambuf_freeze, &ssb1, 0);
    ok(ssb1.dynamic == 1, "expected 1, got %d\n", ssb1.dynamic);
    call_func2(p_strstreambuf_freeze, &ssb1, 3);
    ok(ssb1.dynamic == 0, "expected 0, got %d\n", ssb1.dynamic);
    ssb1.constant = 1;
    call_func2(p_strstreambuf_freeze, &ssb2, 5);
    ok(ssb2.dynamic == 0, "expected 0, got %d\n", ssb2.dynamic);
    call_func2(p_strstreambuf_freeze, &ssb2, 0);
    ok(ssb2.dynamic == 1, "expected 1, got %d\n", ssb2.dynamic);

    /* doallocate */
    ssb2.dynamic = 0;
    ssb2.increase = 5;
    ret = (int) call_func1(p_strstreambuf_doallocate, &ssb2);
    ok(ret == 1, "return value %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 5, "expected %p, got %p\n", ssb2.base.base + 5, ssb2.base.ebuf);
    ssb2.base.eback = ssb2.base.base;
    ssb2.base.gptr = ssb2.base.base + 2;
    ssb2.base.egptr = ssb2.base.base + 4;
    memcpy(ssb2.base.base, "Check", 5);
    ret = (int) call_func1(p_strstreambuf_doallocate, &ssb2);
    ok(ret == 1, "return value %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 10, "expected %p, got %p\n", ssb2.base.base + 10, ssb2.base.ebuf);
    ok(ssb2.base.eback == ssb2.base.base, "wrong get base, expected %p got %p\n", ssb2.base.base, ssb2.base.eback);
    ok(ssb2.base.gptr == ssb2.base.base + 2, "wrong get pointer, expected %p got %p\n", ssb2.base.base + 2, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.base + 4, "wrong get end, expected %p got %p\n", ssb2.base.base + 4, ssb2.base.egptr);
    ok(!strncmp(ssb2.base.base, "Check", 5), "strings are not equal\n");
    ssb2.base.pbase = ssb2.base.pptr = ssb2.base.base + 4;
    ssb2.base.epptr = ssb2.base.ebuf;
    ssb2.increase = -3;
    ret = (int) call_func1(p_strstreambuf_doallocate, &ssb2);
    ok(ret == 1, "return value %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 11, "expected %p, got %p\n", ssb2.base.base + 11, ssb2.base.ebuf);
    ok(ssb2.base.pbase == ssb2.base.base + 4, "wrong put base, expected %p got %p\n", ssb2.base.base + 4, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 4, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 4, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 10, "wrong put end, expected %p got %p\n", ssb2.base.base + 10, ssb2.base.epptr);
    ok(!strncmp(ssb2.base.base, "Check", 5), "strings are not equal\n");
    ssb2.dynamic = 1;

    /* setbuf */
    pret = call_func3(p_strstreambuf_setbuf, &ssb1, buffer + 16, 16);
    ok(pret == &ssb1.base, "expected %p got %p\n", &ssb1.base, pret);
    ok(ssb1.base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, ssb1.base.base);
    ok(ssb1.increase == 16, "expected 16, got %d\n", ssb1.increase);
    pret = call_func3(p_strstreambuf_setbuf, &ssb2, NULL, 2);
    ok(pret == &ssb2.base, "expected %p got %p\n", &ssb2.base, pret);
    ok(ssb2.increase == 2, "expected 2, got %d\n", ssb2.increase);
    pret = call_func3(p_strstreambuf_setbuf, &ssb2, buffer, 0);
    ok(pret == &ssb2.base, "expected %p got %p\n", &ssb2.base, pret);
    ok(ssb2.increase == 2, "expected 2, got %d\n", ssb2.increase);
    pret = call_func3(p_strstreambuf_setbuf, &ssb2, NULL, -2);
    ok(pret == &ssb2.base, "expected %p got %p\n", &ssb2.base, pret);
    ok(ssb2.increase == -2, "expected -2, got %d\n", ssb2.increase);

    /* underflow */
    ssb1.base.epptr = ssb1.base.ebuf = ssb1.base.base + 64;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb1);
    ok(ret == 'T', "expected 'T' got %d\n", ret);
    ssb1.base.gptr = ssb1.base.egptr;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ret = (int) call_func3(p_streambuf_xsputn, &ssb1.base, "Gotta make you understand", 5);
    ok(ret == 5, "expected 5 got %d\n", ret);
    ok(ssb1.base.pptr == buffer + 25, "wrong put pointer, expected %p got %p\n", buffer + 25, ssb1.base.pptr);
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb1);
    ok(ret == 'G', "expected 'G' got %d\n", ret);
    ok(ssb1.base.egptr == buffer + 25, "wrong get end, expected %p got %p\n", buffer + 25, ssb1.base.egptr);
    *ssb1.base.gptr = -50;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb1);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(ssb1.base.egptr == buffer + 25, "wrong get end, expected %p got %p\n", buffer + 25, ssb1.base.egptr);
    ssb1.base.gptr = ssb1.base.egptr = ssb1.base.pptr = ssb1.base.epptr;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ssb2.base.eback = ssb2.base.gptr = ssb2.base.egptr = NULL;
    ssb2.base.pbase = ssb2.base.pptr = ssb2.base.epptr = NULL;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ssb2.base.pbase = ssb2.base.pptr = ssb2.base.base;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb2.base.eback == ssb2.base.base, "wrong get base, expected %p got %p\n", ssb2.base.base, ssb2.base.eback);
    ok(ssb2.base.gptr == ssb2.base.base, "wrong get pointer, expected %p got %p\n", ssb2.base.base, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.base, "wrong get end, expected %p got %p\n", ssb2.base.base, ssb2.base.egptr);
    ssb2.base.pptr = ssb2.base.base + 3;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == 'C', "expected 'C' got %d\n", ret);
    ok(ssb2.base.eback == ssb2.base.base, "wrong get base, expected %p got %p\n", ssb2.base.base, ssb2.base.eback);
    ok(ssb2.base.gptr == ssb2.base.base, "wrong get pointer, expected %p got %p\n", ssb2.base.base, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.base + 3, "wrong get end, expected %p got %p\n", ssb2.base.base + 3, ssb2.base.egptr);
    ssb2.base.gptr = ssb2.base.base + 2;
    ssb2.base.egptr = NULL;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == 'e', "expected 'e' got %d\n", ret);
    ok(ssb2.base.eback == ssb2.base.base, "wrong get base, expected %p got %p\n", ssb2.base.base, ssb2.base.eback);
    ok(ssb2.base.gptr == ssb2.base.base + 2, "wrong get pointer, expected %p got %p\n", ssb2.base.base, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.base + 3, "wrong get end, expected %p got %p\n", ssb2.base.base + 3, ssb2.base.egptr);
    ssb2.base.eback = ssb2.base.egptr = ssb2.base.base + 1;
    ssb2.base.gptr = ssb2.base.base + 3;
    ssb2.base.pptr = ssb2.base.base + 5;
    *(ssb2.base.base + 2) = -50;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(ssb2.base.eback == ssb2.base.base, "wrong get base, expected %p got %p\n", ssb2.base.base, ssb2.base.eback);
    ok(ssb2.base.gptr == ssb2.base.base + 2, "wrong get pointer, expected %p got %p\n", ssb2.base.base, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.base + 5, "wrong get end, expected %p got %p\n", ssb2.base.base + 5, ssb2.base.egptr);
    ssb2.base.eback = ssb2.base.base;
    ssb2.base.gptr = ssb2.base.egptr = ssb2.base.ebuf;
    ssb2.base.pbase = ssb2.base.pptr = NULL;
    ret = (int) call_func1(p_strstreambuf_underflow, &ssb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);

    /* overflow */
    ssb1.base.pptr = ssb1.base.epptr - 1;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb1, EOF);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb1, 'A');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.epptr, "wrong put pointer, expected %p got %p\n", ssb1.base.epptr, ssb1.base.pptr);
    ok(*(ssb1.base.pptr - 1) == 'A', "expected 'A' got %d\n", *(ssb1.base.pptr - 1));
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb1, 'B');
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb1, EOF);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ssb2.dynamic = 0;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'C');
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ssb2.dynamic = 1;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'C');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 12, "expected %p got %p\n", ssb2.base.base + 12, ssb2.base.ebuf);
    ok(ssb2.base.gptr == ssb2.base.base + 11, "wrong get pointer, expected %p got %p\n", ssb2.base.base + 11, ssb2.base.gptr);
    ok(ssb2.base.pbase == ssb2.base.base + 11, "wrong put base, expected %p got %p\n", ssb2.base.base + 11, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 12, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 12, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 12, "wrong put end, expected %p got %p\n", ssb2.base.base + 12, ssb2.base.epptr);
    ok(*(ssb2.base.pptr - 1) == 'C', "expected 'C' got %d\n", *(ssb2.base.pptr - 1));
    ssb2.increase = 4;
    ssb2.base.eback = ssb2.base.gptr = ssb2.base.egptr = NULL;
    ssb2.base.pbase = ssb2.base.pptr = ssb2.base.epptr = NULL;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'D');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 16, "expected %p got %p\n", ssb2.base.base + 16, ssb2.base.ebuf);
    ok(ssb2.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, ssb2.base.gptr);
    ok(ssb2.base.pbase == ssb2.base.base, "wrong put base, expected %p got %p\n", ssb2.base.base, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 1, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 1, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 16, "wrong put end, expected %p got %p\n", ssb2.base.base + 16, ssb2.base.epptr);
    ok(*(ssb2.base.pptr - 1) == 'D', "expected 'D' got %d\n", *(ssb2.base.pptr - 1));
    ssb2.base.pbase = ssb2.base.base + 3;
    ssb2.base.pptr = ssb2.base.epptr + 5;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'E');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 20, "expected %p got %p\n", ssb2.base.base + 20, ssb2.base.ebuf);
    ok(ssb2.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, ssb2.base.gptr);
    ok(ssb2.base.pbase == ssb2.base.base + 3, "wrong put base, expected %p got %p\n", ssb2.base.base + 3, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 22, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 22, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 20, "wrong put end, expected %p got %p\n", ssb2.base.base + 20, ssb2.base.epptr);
    ok(*(ssb2.base.pptr - 1) == 'E', "expected 'E' got %d\n", *(ssb2.base.pptr - 1));
    ssb2.base.egptr = ssb2.base.base + 2;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'F');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 24, "expected %p got %p\n", ssb2.base.base + 24, ssb2.base.ebuf);
    ok(ssb2.base.gptr != NULL, "wrong get pointer, expected != NULL\n");
    ok(ssb2.base.pbase == ssb2.base.base + 3, "wrong put base, expected %p got %p\n", ssb2.base.base + 3, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 23, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 23, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 24, "wrong put end, expected %p got %p\n", ssb2.base.base + 24, ssb2.base.epptr);
    ok(*(ssb2.base.pptr - 1) == 'F', "expected 'F' got %d\n", *(ssb2.base.pptr - 1));
    ssb2.base.eback = ssb2.base.gptr = ssb2.base.base;
    ssb2.base.epptr = NULL;
    ret = (int) call_func2(p_strstreambuf_overflow, &ssb2, 'G');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb2.base.ebuf == ssb2.base.base + 28, "expected %p got %p\n", ssb2.base.base + 28, ssb2.base.ebuf);
    ok(ssb2.base.gptr == ssb2.base.base, "wrong get pointer, expected %p got %p\n", ssb2.base.base, ssb2.base.gptr);
    ok(ssb2.base.pbase == ssb2.base.base + 2, "wrong put base, expected %p got %p\n", ssb2.base.base + 2, ssb2.base.pbase);
    ok(ssb2.base.pptr == ssb2.base.base + 3, "wrong put pointer, expected %p got %p\n", ssb2.base.base + 3, ssb2.base.pptr);
    ok(ssb2.base.epptr == ssb2.base.base + 28, "wrong put end, expected %p got %p\n", ssb2.base.base + 28, ssb2.base.epptr);
    ok(*(ssb2.base.pptr - 1) == 'G', "expected 'G' got %d\n", *(ssb2.base.pptr - 1));

    /* seekoff */
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 0, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback, "wrong get pointer, expected %p got %p\n", ssb1.base.eback, ssb1.base.gptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 3, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 3, "expected 3 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 3, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 3, ssb1.base.gptr);
    ssb1.base.egptr = ssb1.base.pbase;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 25, SEEKDIR_beg, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 3, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 3, ssb1.base.gptr);
    ssb1.base.gptr = ssb1.base.egptr;
    ssb1.base.pptr = ssb1.base.pbase + 10;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 25, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 25, "expected 25 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 25, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 25, ssb1.base.gptr);
    ok(ssb1.base.egptr == ssb1.base.pptr, "wrong get end, expected %p got %p\n", ssb1.base.pptr, ssb1.base.egptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -24, SEEKDIR_cur, OPENMODE_in);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 1, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 1, ssb1.base.gptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -5, SEEKDIR_cur, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 1, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 1, ssb1.base.gptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 0, SEEKDIR_end, OPENMODE_in);
    ok(ret == 30, "expected 30 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.egptr, "wrong get pointer, expected %p got %p\n", ssb1.base.egptr, ssb1.base.gptr);
    ssb1.base.gptr = ssb1.base.eback;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -2, SEEKDIR_end, OPENMODE_in);
    ok(ret == 28, "expected 28 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.egptr - 2, "wrong get pointer, expected %p got %p\n", ssb1.base.egptr - 2, ssb1.base.gptr);
    ssb1.base.gptr = ssb1.base.egptr;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -5, SEEKDIR_end, OPENMODE_in);
    ok(ret == 25, "expected 25 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.egptr - 5, "wrong get pointer, expected %p got %p\n", ssb1.base.egptr - 5, ssb1.base.gptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 1, SEEKDIR_end, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.egptr - 5, "wrong get pointer, expected %p got %p\n", ssb1.base.egptr - 5, ssb1.base.gptr);
    ssb1.base.gptr = ssb1.base.egptr;
    ssb1.base.pptr = ssb1.base.egptr + 5;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 1, SEEKDIR_end, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.egptr - 5, "wrong get pointer, expected %p got %p\n", ssb1.base.egptr - 5, ssb1.base.gptr);
    ok(ssb1.base.egptr == ssb1.base.pptr, "wrong get end, expected %p got %p\n", ssb1.base.pptr, ssb1.base.egptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 3, SEEKDIR_beg, OPENMODE_out);
    ok(ret == 3, "expected 3 got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.pbase + 3, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 3, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -2, SEEKDIR_beg, OPENMODE_out);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.pbase + 3, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 3, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 50, SEEKDIR_beg, OPENMODE_out);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.pbase + 3, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 3, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 5, SEEKDIR_cur, OPENMODE_out);
    ok(ret == 8, "expected 8 got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.pbase + 8, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 8, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, -2, SEEKDIR_end, OPENMODE_out);
    ok(ret == 42, "expected 42 got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.epptr - 2, "wrong put pointer, expected %p got %p\n", ssb1.base.epptr - 2, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 0, SEEKDIR_end, OPENMODE_out);
    ok(ret == 44, "expected 44 got %d\n", ret);
    ok(ssb1.base.pptr == ssb1.base.epptr, "wrong put pointer, expected %p got %p\n", ssb1.base.epptr, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 5, SEEKDIR_beg, OPENMODE_in|OPENMODE_out);
    ok(ret == 5, "expected 5 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 5, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 5, ssb1.base.gptr);
    ok(ssb1.base.pptr == ssb1.base.pbase + 5, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 5, ssb1.base.pptr);
    ssb1.base.egptr = ssb1.base.pbase;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 21, SEEKDIR_beg, OPENMODE_in|OPENMODE_out);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 5, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 5, ssb1.base.gptr);
    ok(ssb1.base.pptr == ssb1.base.pbase + 5, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 5, ssb1.base.pptr);
    ssb1.base.egptr = ssb1.base.epptr;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 50, SEEKDIR_beg, OPENMODE_in|OPENMODE_out);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 50, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 50, ssb1.base.gptr);
    ok(ssb1.base.pptr == ssb1.base.pbase + 5, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 5, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb1, 2, SEEKDIR_cur, OPENMODE_in|OPENMODE_out);
    ok(ret == 7, "expected 7 got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 52, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 52, ssb1.base.gptr);
    ok(ssb1.base.pptr == ssb1.base.pbase + 7, "wrong put pointer, expected %p got %p\n", ssb1.base.pbase + 7, ssb1.base.pptr);
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb2, 0, SEEKDIR_beg, OPENMODE_in|OPENMODE_out);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(ssb2.base.gptr == ssb2.base.eback, "wrong get pointer, expected %p got %p\n", ssb2.base.eback, ssb2.base.gptr);
    ok(ssb2.base.pptr == ssb2.base.pbase, "wrong put pointer, expected %p got %p\n", ssb2.base.pbase, ssb2.base.pptr);
    ssb2.base.gptr = ssb2.base.egptr;
    ssb2.base.pptr += 10;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb2, 5, SEEKDIR_cur, OPENMODE_in|OPENMODE_out);
    ok(ret == 15, "expected 15 got %d\n", ret);
    ok(ssb2.base.gptr == ssb2.base.eback + 7, "wrong get pointer, expected %p got %p\n", ssb2.base.eback + 7, ssb2.base.gptr);
    ok(ssb2.base.egptr == ssb2.base.eback + 12, "wrong get end, expected %p got %p\n", ssb2.base.eback + 12, ssb2.base.egptr);
    ok(ssb2.base.pptr == ssb2.base.pbase + 15, "wrong put pointer, expected %p got %p\n", ssb2.base.pbase + 15, ssb2.base.pptr);
    ssb2.base.pptr = ssb2.base.epptr;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb2, -1, SEEKDIR_cur, OPENMODE_in|OPENMODE_out);
    ok(ret == 25, "expected 25 got %d\n", ret);
    ok(ssb2.base.gptr == ssb2.base.eback + 6, "wrong get pointer, expected %p got %p\n", ssb2.base.eback + 6, ssb2.base.gptr);
    ok(ssb2.base.pptr == ssb2.base.pbase + 25, "wrong put pointer, expected %p got %p\n", ssb2.base.pbase + 25, ssb2.base.pptr);
    pbuffer = ssb2.base.pbase;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb2, 50, SEEKDIR_beg, OPENMODE_out);
    ok(ret == 50, "expected 50 got %d\n", ret);
    ok(ssb2.base.gptr == ssb2.base.eback + 6, "wrong get pointer, expected %p got %p\n", ssb2.base.eback + 6, ssb2.base.gptr);
    ok(ssb2.base.pptr == pbuffer + 50, "wrong put pointer, expected %p got %p\n", pbuffer + 50, ssb2.base.pptr);
    ok(ssb2.increase == 50, "expected 50 got %d\n", ssb2.increase);
    ssb2.base.epptr = NULL;
    ssb2.increase = 8;
    ret = (int) call_func4(p_strstreambuf_seekoff, &ssb2, 0, SEEKDIR_end, OPENMODE_out);
    ok(ret == 74, "expected 74 got %d\n", ret);
    ok(ssb2.base.pptr == ssb2.base.epptr, "wrong put pointer, expected %p got %p\n", ssb2.base.epptr, ssb2.base.pptr);

    /* pbackfail */
    *(ssb1.base.gptr-1) = 'A';
    ret = (int) call_func2(p_streambuf_pbackfail, &ssb1.base, 'X');
    ok(ret == 'X', "wrong return, expected 'X' got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback + 51, "wrong get pointer, expected %p got %p\n", ssb1.base.eback + 51, ssb1.base.gptr);
    ok(*ssb1.base.gptr == 'X', "expected 'X' got '%c'\n", *ssb1.base.gptr);
    ssb1.base.gptr = ssb1.base.eback;
    ret = (int) call_func2(p_streambuf_pbackfail, &ssb1.base, 'Y');
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ssb1.base.gptr = ssb1.base.eback + 1;
    ret = (int) call_func2(p_streambuf_pbackfail, &ssb1.base, EOF);
    ok(ret == (char)EOF, "wrong return, expected EOF got %d\n", ret);
    ok(ssb1.base.gptr == ssb1.base.eback, "wrong get pointer, expected %p got %p\n", ssb1.base.eback, ssb1.base.gptr);
    ok((signed char)*ssb1.base.gptr == EOF, "expected EOF got '%c'\n", *ssb1.base.gptr);

    call_func1(p_strstreambuf_dtor, &ssb1);
    call_func1(p_strstreambuf_dtor, &ssb2);
}

static void test_stdiobuf(void)
{
    stdiobuf stb1, stb2;
    FILE *file1, *file2;
    const char filename1[] = "stdiobuf_test1";
    const char filename2[] = "stdiobuf_test2";
    int ret, last;
    char buffer[64];

    memset(&stb1, 0xab, sizeof(stdiobuf));
    memset(&stb2, 0xab, sizeof(stdiobuf));

    file1 = fopen(filename1, "w");
    fputs("Never gonna give you up, never gonna let you down", file1);
    fclose(file1);
    file1 = fopen(filename1, "r");
    ok(file1 != NULL, "Couldn't open the file named '%s'\n", filename1);
    file2 = fopen(filename2, "w+");
    ok(file2 != NULL, "Couldn't open the file named '%s'\n", filename2);

    /* constructors/destructor */
    call_func2(p_stdiobuf_file_ctor, &stb1, NULL);
    ok(stb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", stb1.base.allocated);
    ok(stb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", stb1.base.unbuffered);
    ok(stb1.file == NULL, "wrong file pointer, expected %p got %p\n", NULL, stb1.file);
    call_func1(p_stdiobuf_dtor, &stb1);
    call_func2(p_stdiobuf_file_ctor, &stb1, file1);
    ok(stb1.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", stb1.base.allocated);
    ok(stb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", stb1.base.unbuffered);
    ok(stb1.file == file1, "wrong file pointer, expected %p got %p\n", file1, stb1.file);
    call_func2(p_stdiobuf_file_ctor, &stb2, file2);
    ok(stb2.base.allocated == 0, "wrong allocate value, expected 0 got %d\n", stb2.base.allocated);
    ok(stb2.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", stb2.base.unbuffered);
    ok(stb2.file == file2, "wrong file pointer, expected %p got %p\n", file2, stb2.file);

    /* overflow */
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, EOF);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, 'a');
    ok(ret == EOF, "expected EOF got %d\n", ret);
    stb1.base.unbuffered = 0;
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, 'a');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb1.base.allocated == 1, "wrong allocate value, expected 1 got %d\n", stb1.base.allocated);
    ok(stb1.base.ebuf == stb1.base.base + 512, "expected %p got %p\n", stb1.base.base + 512, stb1.base.ebuf);
    ok(stb1.base.pbase == stb1.base.base + 256, "wrong put base, expected %p got %p\n", stb1.base.base + 256, stb1.base.pbase);
    ok(stb1.base.pptr == stb1.base.pbase + 1, "wrong put pointer, expected %p got %p\n", stb1.base.pbase + 1, stb1.base.pptr);
    ok(stb1.base.epptr == stb1.base.ebuf, "wrong put end, expected %p got %p\n", stb1.base.ebuf, stb1.base.epptr);
    ok(*(stb1.base.pptr-1) == 'a', "expected 'a', got '%c'\n", *(stb1.base.pptr-1));
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, EOF);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb1.base.pptr == stb1.base.pbase + 1, "wrong put pointer, expected %p got %p\n", stb1.base.pbase + 1, stb1.base.pptr);
    stb1.base.pptr = stb1.base.pbase;
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, EOF);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb1.base.pptr == stb1.base.pbase, "wrong put pointer, expected %p got %p\n", stb1.base.pbase, stb1.base.pptr);
    ret = (int) call_func2(p_stdiobuf_overflow, &stb1, 'b');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb1.base.pptr == stb1.base.pbase + 1, "wrong put pointer, expected %p got %p\n", stb1.base.pbase + 1, stb1.base.pptr);
    ok(*(stb1.base.pptr-1) == 'b', "expected 'b', got '%c'\n", *(stb1.base.pptr-1));
    stb1.base.unbuffered = 1;
    ret = (int) call_func2(p_stdiobuf_overflow, &stb2, EOF);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ret = (int) call_func2(p_stdiobuf_overflow, &stb2, 'a');
    ok(ret == 'a', "expected 'a' got %d\n", ret);
    stb2.base.unbuffered = 0;
    stb2.base.base = buffer;
    stb2.base.pbase = stb2.base.pptr = buffer + 32;
    stb2.base.ebuf = stb2.base.epptr = buffer + 64;
    ret = (int) call_func3(p_streambuf_xsputn, &stb2.base, "Never gonna run around", 22);
    ok(ret == 22, "expected 22 got %d\n", ret);
    ret = (int) call_func2(p_stdiobuf_overflow, &stb2, 'c');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase + 1, "wrong put pointer, expected %p got %p\n", stb2.base.pbase + 1, stb2.base.pptr);
    ok(*(stb2.base.pptr-1) == 'c', "expected 'c', got '%c'\n", *(stb2.base.pptr-1));
    stb2.base.pbase = stb2.base.pptr + 3;
    ret = (int) call_func2(p_stdiobuf_overflow, &stb2, 'd');
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase - 2, "wrong put pointer, expected %p got %p\n", stb2.base.pbase - 2, stb2.base.pptr);
    ok(*(stb2.base.pptr-1) == 'd', "expected 'd', got '%c'\n", *(stb2.base.pptr-1));
    stb2.base.pbase = stb2.base.pptr = buffer + 32;
    stb2.base.epptr = buffer + 30;
    ret = (int) call_func2(p_stdiobuf_overflow, &stb2, 'd');
    ok(ret == 'd', "expected 'd' got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase, "wrong put pointer, expected %p got %p\n", stb2.base.pbase, stb2.base.pptr);
    stb2.base.epptr = buffer + 64;
    stb2.base.unbuffered = 1;

    /* underflow */
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == 'N', "expected 'N' got %d\n", ret);
    stb1.base.unbuffered = 0;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == 'e', "expected 'e' got %d\n", ret);
    ok(stb1.base.eback == stb1.base.base, "wrong get base, expected %p got %p\n", stb1.base.base, stb1.base.eback);
    ok(stb1.base.gptr == stb1.base.egptr - 47, "wrong get pointer, expected %p got %p\n", stb1.base.egptr - 47, stb1.base.gptr);
    ok(stb1.base.egptr == stb1.base.base + 256, "wrong get end, expected %p got %p\n", stb1.base.base + 256, stb1.base.egptr);
    ok(*stb1.base.gptr == 'v', "expected 'v' got '%c'\n", *stb1.base.gptr);
    stb1.base.pbase = stb1.base.pptr = stb1.base.epptr = NULL;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == 'v', "expected 'v' got %d\n", ret);
    ok(stb1.base.gptr == stb1.base.egptr - 46, "wrong get pointer, expected %p got %p\n", stb1.base.egptr - 46, stb1.base.gptr);
    *stb1.base.gptr = -50;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(stb1.base.gptr == stb1.base.egptr - 45, "wrong get pointer, expected %p got %p\n", stb1.base.egptr - 45, stb1.base.gptr);
    stb1.base.gptr = stb1.base.egptr;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    stb1.base.unbuffered = 1;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    rewind(stb2.file);
    ret = (int) call_func1(p_stdiobuf_underflow, &stb2);
    ok(ret == 'a', "expected 'a' got %d\n", ret);
    stb2.base.unbuffered = 0;
    stb2.base.eback = stb2.base.gptr = buffer + 16;
    stb2.base.egptr = buffer + 32;
    strcpy(buffer + 16, "and desert you");
    ret = (int) call_func1(p_stdiobuf_underflow, &stb2);
    ok(ret == 'a', "expected 'a' got %d\n", ret);
    ok(stb2.base.gptr == buffer + 17, "wrong get pointer, expected %p got %p\n", buffer + 17, stb2.base.gptr);
    stb2.base.eback = buffer;
    stb2.base.gptr = stb2.base.egptr;
    stb2.base.pbase = stb2.base.pptr = stb2.base.epptr = NULL;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb2);
    ok(ret == 'N', "expected 'N' got %d\n", ret);
    ok(stb2.base.gptr == stb2.base.egptr - 22, "wrong get pointer, expected %p got %p\n", stb2.base.egptr - 22, stb2.base.gptr);
    ok(ftell(stb2.file) == 24, "ftell failed\n");
    stb2.base.gptr = stb2.base.egptr;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    stb2.base.unbuffered = 1;
    ret = (int) call_func1(p_stdiobuf_underflow, &stb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);

    /* sync */
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    stb1.base.gptr = stb1.base.eback;
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    stb1.base.unbuffered = 0;
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb1.base.pbase == stb1.base.base + 256, "wrong put base, expected %p got %p\n", stb1.base.base + 256, stb1.base.pbase);
    ok(stb1.base.pptr == stb1.base.base + 256, "wrong put pointer, expected %p got %p\n", stb1.base.base + 256, stb1.base.pptr);
    ok(stb1.base.epptr == stb1.base.base + 512, "wrong put end, expected %p got %p\n", stb1.base.base + 512, stb1.base.epptr);
    stb1.base.gptr = stb1.base.egptr;
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    stb1.base.eback = stb1.base.gptr = stb1.base.egptr = NULL;
    stb1.base.pptr = stb1.base.epptr;
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, stb1.base.gptr);
    stb1.base.pptr = stb1.base.pbase;
    ret = (int) call_func1(p_stdiobuf_sync, &stb1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(stb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, stb1.base.gptr);
    stb1.base.unbuffered = 1;
    stb2.base.unbuffered = 0;
    stb2.base.egptr = stb2.base.ebuf;
    ret = (int) call_func1(p_stdiobuf_sync, &stb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb2.base.pbase == stb2.base.base + 32, "wrong put base, expected %p got %p\n", stb2.base.base + 32, stb2.base.pbase);
    ok(stb2.base.pptr == stb2.base.base + 32, "wrong put pointer, expected %p got %p\n", stb2.base.base + 32, stb2.base.pptr);
    ok(stb2.base.epptr == stb2.base.base + 64, "wrong put end, expected %p got %p\n", stb2.base.base + 64, stb2.base.epptr);
    stb2.base.egptr = stb2.base.pbase;
    stb2.base.gptr = stb2.base.egptr - 25;
    ret = (int) call_func1(p_stdiobuf_sync, &stb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ret = (int) call_func3(p_streambuf_xsputn, &stb2.base, "Never gonna make you cry", 24);
    ok(ret == 24, "expected 24 got %d\n", ret);
    ret = (int) call_func1(p_stdiobuf_sync, &stb2);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(ftell(stb2.file) == 23, "ftell failed\n");
    ok(fseek(stb2.file, 3, SEEK_SET) == 0, "fseek failed\n");
    stb2.base.gptr = stb2.base.egptr - 3;
    strcpy(stb2.base.gptr, "a\nc");
    ret = (int) call_func1(p_stdiobuf_sync, &stb2);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb2.base.gptr == stb2.base.egptr - 3, "wrong get pointer, expected %p got %p\n", stb2.base.egptr - 3, stb2.base.gptr);
    *(stb2.base.gptr+1) = 'b';
    ret = (int) call_func1(p_stdiobuf_sync, &stb2);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(stb2.base.gptr == stb2.base.egptr, "wrong get pointer, expected %p got %p\n", stb2.base.egptr, stb2.base.gptr);
    stb2.base.unbuffered = 1;

    /* setrwbuf */
    ret = (int) call_func3(p_stdiobuf_setrwbuf, &stb1, 100, 60);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb1.base.allocated == 1, "wrong allocate value, expected 1 got %d\n", stb1.base.allocated);
    ok(stb1.base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", stb1.base.unbuffered);
    ok(stb1.base.ebuf == stb1.base.base + 160, "expected %p got %p\n", stb1.base.base + 160, stb1.base.ebuf);
    ok(stb1.base.eback == stb1.base.base, "wrong get base, expected %p got %p\n", stb1.base.base, stb1.base.eback);
    ok(stb1.base.gptr == stb1.base.base + 100, "wrong get pointer, expected %p got %p\n", stb1.base.base + 100, stb1.base.gptr);
    ok(stb1.base.egptr == stb1.base.base + 100, "wrong get end, expected %p got %p\n", stb1.base.base + 100, stb1.base.egptr);
    ok(stb1.base.pbase == stb1.base.base + 100, "wrong put base, expected %p got %p\n", stb1.base.base + 100, stb1.base.pbase);
    ok(stb1.base.pptr == stb1.base.base + 100, "wrong put pointer, expected %p got %p\n", stb1.base.base + 100, stb1.base.pptr);
    ok(stb1.base.epptr == stb1.base.base + 160, "wrong put end, expected %p got %p\n", stb1.base.base + 160, stb1.base.epptr);
    ret = (int) call_func3(p_stdiobuf_setrwbuf, &stb1, -1, 64);
    ok(ret == 0 || ret == 1, "expected 0 or 1, got %d\n", ret);
    ret = (int) call_func3(p_stdiobuf_setrwbuf, &stb1, 32, -8);
    ok(ret == 0 || ret == 1, "expected 0 or 1, got %d\n", ret);
    ret = (int) call_func3(p_stdiobuf_setrwbuf, &stb1, 0, 64);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(stb1.base.ebuf == stb1.base.base + 64, "expected %p got %p\n", stb1.base.base + 64, stb1.base.ebuf);
    ok(stb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, stb1.base.eback);
    ok(stb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, stb1.base.gptr);
    ok(stb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, stb1.base.egptr);
    ok(stb1.base.pbase == stb1.base.base, "wrong put base, expected %p got %p\n", stb1.base.base, stb1.base.pbase);
    ok(stb1.base.pptr == stb1.base.base, "wrong put pointer, expected %p got %p\n", stb1.base.base, stb1.base.pptr);
    ok(stb1.base.epptr == stb1.base.base + 64, "wrong put end, expected %p got %p\n", stb1.base.base + 64, stb1.base.epptr);
    ret = (int) call_func3(p_stdiobuf_setrwbuf, &stb1, 0, 0);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(stb1.base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", stb1.base.unbuffered);

    /* seekoff */
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 0, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 0, "expected 0 got %d\n", ret);
    stb1.base.unbuffered = 0;
    ret = (int) call_func3(p_streambuf_xsputn, &stb1.base, "Never gonna say goodbye", 22);
    ok(ret == 22, "expected 22 got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 5, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 5, "expected 5 got %d\n", ret);
    ok(stb1.base.pptr == stb1.base.pbase + 22, "wrong put pointer, expected %p got %p\n", stb1.base.pbase + 22, stb1.base.pptr);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 300, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 300, "expected 300 got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, -310, SEEKDIR_cur, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    stb1.base.eback = stb1.base.base;
    stb1.base.gptr = stb1.base.egptr = stb1.base.pbase = stb1.base.base + 10;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, -280, SEEKDIR_cur, OPENMODE_in);
    ok(ret == 20, "expected 20 got %d\n", ret);
    stb1.base.gptr = stb1.base.eback;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 80, SEEKDIR_cur, OPENMODE_in);
    ok(ret == 100, "expected 100 got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 0, SEEKDIR_end, OPENMODE_in);
    ok(ret == 49, "expected 49 got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, 1, SEEKDIR_end, OPENMODE_in);
    ok(ret == 50, "expected 50 got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, -60, SEEKDIR_end, OPENMODE_in);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb1, -20, SEEKDIR_end, OPENMODE_out);
    ok(ret == 29, "expected 29 got %d\n", ret);
    stb1.base.eback = stb1.base.gptr = stb1.base.egptr = NULL;
    stb1.base.pptr = stb1.base.pbase;
    stb1.base.unbuffered = 1;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, 0, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 0, "expected 0 got %d\n", ret);
    stb2.base.pptr += 10;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, 20, SEEKDIR_beg, OPENMODE_in);
    ok(ret == 20, "expected 20 got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase + 10, "wrong put pointer, expected %p got %p\n", stb2.base.pbase + 10, stb2.base.pptr);
    stb2.base.unbuffered = 0;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, 10, SEEKDIR_cur, OPENMODE_in);
    ok(ret == 40, "expected 40 got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase, "wrong put pointer, expected %p got %p\n", stb2.base.pbase, stb2.base.pptr);
    stb2.base.gptr = stb2.base.eback;
    stb2.base.pptr += 5;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, -50, SEEKDIR_cur, OPENMODE_in|OPENMODE_out);
    ok(ret == EOF, "expected EOF got %d\n", ret);
    ok(stb2.base.pptr == stb2.base.pbase, "wrong put pointer, expected %p got %p\n", stb2.base.pbase, stb2.base.pptr);
    stb2.base.pbase = stb2.base.pptr = stb2.base.epptr = NULL;
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, 0, SEEKDIR_end, OPENMODE_in|OPENMODE_out);
    ok(ret == 48, "expected 48 got %d\n", ret);
    ok(stb2.base.pbase == stb2.base.base + 32, "wrong put pointer, expected %p got %p\n", stb2.base.base + 32, stb2.base.pbase);
    ok(stb2.base.pptr == stb2.base.base + 32, "wrong put pointer, expected %p got %p\n", stb2.base.base + 32, stb2.base.pptr);
    ok(stb2.base.epptr == stb2.base.ebuf, "wrong put pointer, expected %p got %p\n", stb2.base.ebuf, stb2.base.epptr);
    ret = (int) call_func4(p_stdiobuf_seekoff, &stb2, -28, SEEKDIR_end, -1);
    ok(ret == 20, "expected 20 got %d\n", ret);
    stb2.base.gptr = stb2.base.egptr;
    stb2.base.unbuffered = 1;

    /* pbackfail */
    last = fgetc(stb1.file);
    ok(last == 'r', "expected 'r' got %d\n", last);
    ok(ftell(stb1.file) == 30, "ftell failed\n");
    ret = (int) call_func2(p_stdiobuf_pbackfail, &stb1, 'i');
    ok(ret == 'i', "expected 'i' got %d\n", ret);
    ok(ftell(stb1.file) == 29, "ftell failed\n");
    last = fgetc(stb1.file);
    ok(last == 'r', "expected 'r' got %d\n", last);
    ok(ftell(stb1.file) == 30, "ftell failed\n");
    stb1.base.eback = stb1.base.base;
    stb1.base.gptr = stb1.base.egptr = stb1.base.base + 9;
    strcpy(stb1.base.eback, "pbackfail");
    ret = (int) call_func2(p_stdiobuf_pbackfail, &stb1, 'j');
    ok(ret == 'j', "expected 'j' got %d\n", ret);
    ok(stb1.base.gptr == stb1.base.base + 8, "wrong get pointer, expected %p got %p\n", stb1.base.base + 8, stb1.base.gptr);
    ok(strncmp(stb1.base.eback, "pbackfaij", 9) == 0, "strncmp failed\n");
    ok(ftell(stb1.file) == 30, "ftell failed\n");
    stb1.base.gptr = stb1.base.eback;
    ret = (int) call_func2(p_stdiobuf_pbackfail, &stb1, 'k');
    ok(ret == 'k', "expected 'k' got %d\n", ret);
    ok(stb1.base.gptr == stb1.base.base, "wrong get pointer, expected %p got %p\n", stb1.base.base, stb1.base.gptr);
    ok(strncmp(stb1.base.eback, "pbackfaij", 9) == 0, "strncmp failed\n");
    ok(ftell(stb1.file) == 29, "ftell failed\n");
    stb1.base.unbuffered = 0;
    ret = (int) call_func2(p_stdiobuf_pbackfail, &stb1, 'l');
    ok(ret == 'l', "expected 'l' got %d\n", ret);
    ok(strncmp(stb1.base.eback, "lpbackfai", 9) == 0, "strncmp failed\n");
    ok(ftell(stb1.file) == 28, "ftell failed\n");
    stb1.base.egptr = NULL;
    ret = (int) call_func2(p_stdiobuf_pbackfail, &stb1, 'm');
    ok(ret == 'm', "expected 'm' got %d\n", ret);
    ok(strncmp(stb1.base.eback, "lpbackfai", 9) == 0, "strncmp failed\n");
    ok(ftell(stb1.file) == 27, "ftell failed\n");
    stb1.base.unbuffered = 1;

    call_func1(p_stdiobuf_dtor, &stb1);
    call_func1(p_stdiobuf_dtor, &stb2);
    fclose(file1);
    fclose(file2);
    ok(_unlink(filename1) == 0, "Couldn't unlink file named '%s'\n", filename1);
    ok(_unlink(filename2) == 0, "Couldn't unlink file named '%s'\n", filename2);
}

struct ios_lock_arg
{
    ios *ios_obj;
    HANDLE lock;
    HANDLE release[3];
};

static DWORD WINAPI lock_ios(void *arg)
{
    struct ios_lock_arg *lock_arg = arg;
    p_ios_lock(lock_arg->ios_obj);
    p_ios_lockbuf(lock_arg->ios_obj);
    p_ios_lockc();
    SetEvent(lock_arg->lock);
    WaitForSingleObject(lock_arg->release[0], INFINITE);
    p_ios_unlockc();
    WaitForSingleObject(lock_arg->release[1], INFINITE);
    p_ios_unlockbuf(lock_arg->ios_obj);
    WaitForSingleObject(lock_arg->release[2], INFINITE);
    p_ios_unlock(lock_arg->ios_obj);
    return 0;
}

static void test_ios(void)
{
    ios ios_obj, ios_obj2;
    streambuf *psb;
    struct ios_lock_arg lock_arg;
    HANDLE thread;
    BOOL locked;
    LONG *pret, expected, ret;
    void **pret2;
    int i;

    memset(&ios_obj, 0xab, sizeof(ios));
    memset(&ios_obj2, 0xab, sizeof(ios));
    psb = p_operator_new(sizeof(streambuf));
    ok(psb != NULL, "failed to allocate streambuf object\n");
    call_func1(p_streambuf_ctor, psb);

    /* constructor/destructor */
    ok(*p_ios_fLockcInit == 4, "expected 4 got %d\n", *p_ios_fLockcInit);
    call_func2(p_ios_sb_ctor, &ios_obj, NULL);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == IOSTATE_badbit, "expected %x got %x\n", IOSTATE_badbit, ios_obj.state);
    ok(ios_obj.special[0] == 0, "expected 0 got %d\n", ios_obj.special[0]);
    ok(ios_obj.special[1] == 0, "expected 0 got %d\n", ios_obj.special[1]);
    ok(ios_obj.delbuf == 0, "expected 0 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == NULL, "expected %p got %p\n", NULL, ios_obj.tie);
    ok(ios_obj.flags == 0, "expected 0 got %x\n", ios_obj.flags);
    ok(ios_obj.precision == 6, "expected 6 got %d\n", ios_obj.precision);
    ok(ios_obj.fill == ' ', "expected ' ' got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0, "expected 0 got %d\n", ios_obj.width);
    ok(ios_obj.do_lock == -1, "expected -1 got %d\n", ios_obj.do_lock);
    ok(ios_obj.lock.LockCount == -1, "expected -1 got %lu\n", ios_obj.lock.LockCount);
    ok(*p_ios_fLockcInit == 5, "expected 5 got %d\n", *p_ios_fLockcInit);
    ios_obj.state = 0x8;
    call_func1(p_ios_dtor, &ios_obj);
    ok(ios_obj.state == IOSTATE_badbit, "expected %x got %x\n", IOSTATE_badbit, ios_obj.state);
    ok(*p_ios_fLockcInit == 4, "expected 4 got %d\n", *p_ios_fLockcInit);
    memset(&ios_obj, 0xab, sizeof(ios));
    call_func2(p_ios_sb_ctor, &ios_obj, psb);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == IOSTATE_goodbit, "expected %x got %x\n", IOSTATE_goodbit, ios_obj.state);
    ok(ios_obj.special[0] == 0, "expected 0 got %d\n", ios_obj.special[0]);
    ok(ios_obj.special[1] == 0, "expected 0 got %d\n", ios_obj.special[1]);
    ok(ios_obj.delbuf == 0, "expected 0 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == NULL, "expected %p got %p\n", NULL, ios_obj.tie);
    ok(ios_obj.flags == 0, "expected 0 got %x\n", ios_obj.flags);
    ok(ios_obj.precision == 6, "expected 6 got %d\n", ios_obj.precision);
    ok(ios_obj.fill == ' ', "expected ' ' got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0, "expected 0 got %d\n", ios_obj.width);
    ok(ios_obj.do_lock == -1, "expected -1 got %d\n", ios_obj.do_lock);
    ok(*p_ios_fLockcInit == 5, "expected 5 got %d\n", *p_ios_fLockcInit);
    ios_obj.state = 0x8;
    call_func1(p_ios_dtor, &ios_obj);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == IOSTATE_badbit, "expected %x got %x\n", IOSTATE_badbit, ios_obj.state);
    ok(*p_ios_fLockcInit == 4, "expected 4 got %d\n", *p_ios_fLockcInit);
    memset(&ios_obj, 0xab, sizeof(ios));
    call_func1(p_ios_ctor, &ios_obj);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == IOSTATE_badbit, "expected %x got %x\n", IOSTATE_badbit, ios_obj.state);
    ok(ios_obj.special[0] == 0, "expected 0 got %d\n", ios_obj.special[0]);
    ok(ios_obj.special[1] == 0, "expected 0 got %d\n", ios_obj.special[1]);
    ok(ios_obj.delbuf == 0, "expected 0 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == NULL, "expected %p got %p\n", NULL, ios_obj.tie);
    ok(ios_obj.flags == 0, "expected 0 got %x\n", ios_obj.flags);
    ok(ios_obj.precision == 6, "expected 6 got %d\n", ios_obj.precision);
    ok(ios_obj.fill == ' ', "expected ' ' got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0, "expected 0 got %d\n", ios_obj.width);
    ok(ios_obj.do_lock == -1, "expected -1 got %d\n", ios_obj.do_lock);
    ok(*p_ios_fLockcInit == 5, "expected 5 got %d\n", *p_ios_fLockcInit);

    /* init */
    ios_obj.state |= 0x8;
    call_func2(p_ios_init, &ios_obj, psb);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0x8, "expected %x got %x\n", 0x8, ios_obj.state);
    call_func2(p_ios_init, &ios_obj, NULL);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == (0x8|IOSTATE_badbit), "expected %x got %x\n", (0x8|IOSTATE_badbit), ios_obj.state);
    ios_obj.sb = psb;
    ios_obj.delbuf = 0;
    call_func2(p_ios_init, &ios_obj, psb);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0x8, "expected %x got %x\n", 0x8, ios_obj.state);
    call_func1(p_ios_dtor, &ios_obj);

    /* copy constructor */
    memset(&ios_obj, 0xcd, sizeof(ios));
    call_func2(p_ios_copy_ctor, &ios_obj, &ios_obj2);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == (ios_obj2.state|IOSTATE_badbit), "expected %x got %x\n",
        ios_obj2.state|IOSTATE_badbit, ios_obj.state);
    ok(ios_obj.delbuf == 0, "expected 0 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == ios_obj2.tie, "expected %p got %p\n", ios_obj2.tie, ios_obj.tie);
    ok(ios_obj.flags == ios_obj2.flags, "expected %x got %x\n", ios_obj2.flags, ios_obj.flags);
    ok(ios_obj.precision == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.precision);
    ok(ios_obj.fill == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.fill);
    ok(ios_obj.width == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.width);
    ok(ios_obj.do_lock == -1, "expected -1 got %d\n", ios_obj.do_lock);
    ok(*p_ios_fLockcInit == 5, "expected 5 got %d\n", *p_ios_fLockcInit);

    /* assignment */
    ios_obj.state = 0xcdcdcdcd;
    ios_obj.delbuf = 0xcdcdcdcd;
    ios_obj.tie = (ostream*) 0xcdcdcdcd;
    ios_obj.flags = 0xcdcdcdcd;
    ios_obj.precision = 0xcdcdcdcd;
    ios_obj.fill = 0xcd;
    ios_obj.width = 0xcdcdcdcd;
    ios_obj.do_lock = 0xcdcdcdcd;
    call_func2(p_ios_assign, &ios_obj, &ios_obj2);
    ok(ios_obj.sb == NULL, "expected %p got %p\n", NULL, ios_obj.sb);
    ok(ios_obj.state == (ios_obj2.state|IOSTATE_badbit), "expected %x got %x\n",
        ios_obj2.state|IOSTATE_badbit, ios_obj.state);
    ok(ios_obj.delbuf == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios_obj.delbuf);
    ok(ios_obj.tie == ios_obj2.tie, "expected %p got %p\n", ios_obj2.tie, ios_obj.tie);
    ok(ios_obj.flags == ios_obj2.flags, "expected %x got %x\n", ios_obj2.flags, ios_obj.flags);
    ok(ios_obj.precision == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.precision);
    ok(ios_obj.fill == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.fill);
    ok(ios_obj.width == (char)0xab, "expected %d got %d\n", (char)0xab, ios_obj.width);
    ok(ios_obj.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios_obj.do_lock);

    /* locking */
    ios_obj.sb = psb;
    ios_obj.do_lock = 1;
    ios_obj.sb->do_lock = 0;
    p_ios_clrlock(&ios_obj);
    ok(ios_obj.do_lock == 1, "expected 1 got %d\n", ios_obj.do_lock);
    ok(ios_obj.sb->do_lock == 1, "expected 1 got %d\n", ios_obj.sb->do_lock);
    ios_obj.do_lock = 0;
    p_ios_clrlock(&ios_obj);
    ok(ios_obj.do_lock == 1, "expected 1 got %d\n", ios_obj.do_lock);
    ok(ios_obj.sb->do_lock == 1, "expected 1 got %d\n", ios_obj.sb->do_lock);
    p_ios_setlock(&ios_obj);
    ok(ios_obj.do_lock == 0, "expected 0 got %d\n", ios_obj.do_lock);
    ok(ios_obj.sb->do_lock == 0, "expected 0 got %d\n", ios_obj.sb->do_lock);
    ios_obj.do_lock = -1;
    p_ios_setlock(&ios_obj);
    ok(ios_obj.do_lock == -2, "expected -2 got %d\n", ios_obj.do_lock);
    ok(ios_obj.sb->do_lock == -1, "expected -1 got %d\n", ios_obj.sb->do_lock);

    lock_arg.ios_obj = &ios_obj;
    lock_arg.lock = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.lock != NULL, "CreateEventW failed\n");
    lock_arg.release[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.release[0] != NULL, "CreateEventW failed\n");
    lock_arg.release[1] = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.release[1] != NULL, "CreateEventW failed\n");
    lock_arg.release[2] = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(lock_arg.release[2] != NULL, "CreateEventW failed\n");
    thread = CreateThread(NULL, 0, lock_ios, (void*)&lock_arg, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(lock_arg.lock, INFINITE);

    locked = TryEnterCriticalSection(&ios_obj.lock);
    ok(locked == 0, "the ios object was not locked before\n");
    locked = TryEnterCriticalSection(&ios_obj.sb->lock);
    ok(locked == 0, "the streambuf was not locked before\n");
    locked = TryEnterCriticalSection(p_ios_static_lock);
    ok(locked == 0, "the static critical section was not locked before\n");

    /* flags */
    ios_obj.flags = 0x8000;
    ret = (LONG) call_func1(p_ios_flags_get, &ios_obj);
    ok(ret == 0x8000, "expected 0x8000 got %lx\n", ret);
    ret = (LONG) call_func2(p_ios_flags_set, &ios_obj, 0x444);
    ok(ret == 0x8000, "expected 0x8000 got %lx\n", ret);
    ok(ios_obj.flags == 0x444, "expected 0x444 got %x\n", ios_obj.flags);
    ret = (LONG) call_func2(p_ios_flags_set, &ios_obj, 0);
    ok(ret == 0x444, "expected 0x444 got %lx\n", ret);
    ok(ios_obj.flags == 0, "expected %x got %x\n", 0, ios_obj.flags);

    /* setf */
    ios_obj.do_lock = 0;
    ios_obj.flags = 0x8400;
    ret = (LONG) call_func2(p_ios_setf, &ios_obj, 0x444);
    ok(ret == 0x8400, "expected 0x8400 got %lx\n", ret);
    ok(ios_obj.flags == 0x8444, "expected 0x8444 got %x\n", ios_obj.flags);
    ret = (LONG) call_func3(p_ios_setf_mask, &ios_obj, 0x111, 0);
    ok(ret == 0x8444, "expected 0x8444 got %lx\n", ret);
    ok(ios_obj.flags == 0x8444, "expected 0x8444 got %x\n", ios_obj.flags);
    ret = (LONG) call_func3(p_ios_setf_mask, &ios_obj, 0x111, 0x105);
    ok(ret == 0x8444, "expected 0x8444 got %lx\n", ret);
    ok(ios_obj.flags == 0x8541, "expected 0x8541 got %x\n", ios_obj.flags);

    /* unsetf */
    ret = (LONG) call_func2(p_ios_unsetf, &ios_obj, 0x1111);
    ok(ret == 0x8541, "expected 0x8541 got %lx\n", ret);
    ok(ios_obj.flags == 0x8440, "expected 0x8440 got %x\n", ios_obj.flags);
    ret = (LONG) call_func2(p_ios_unsetf, &ios_obj, 0x8008);
    ok(ret == 0x8440, "expected 0x8440 got %lx\n", ret);
    ok(ios_obj.flags == 0x440, "expected 0x440 got %x\n", ios_obj.flags);
    ios_obj.do_lock = -1;

    /* state */
    ios_obj.state = 0x8;
    ret = (LONG) call_func1(p_ios_good, &ios_obj);
    ok(ret == 0, "expected 0 got %ld\n", ret);
    ios_obj.state = IOSTATE_goodbit;
    ret = (LONG) call_func1(p_ios_good, &ios_obj);
    ok(ret == 1, "expected 1 got %ld\n", ret);
    ret = (LONG) call_func1(p_ios_bad, &ios_obj);
    ok(ret == 0, "expected 0 got %ld\n", ret);
    ios_obj.state = (IOSTATE_eofbit|IOSTATE_badbit);
    ret = (LONG) call_func1(p_ios_bad, &ios_obj);
    ok(ret == IOSTATE_badbit, "expected 4 got %ld\n", ret);
    ret = (LONG) call_func1(p_ios_eof, &ios_obj);
    ok(ret == IOSTATE_eofbit, "expected 1 got %ld\n", ret);
    ios_obj.state = 0x8;
    ret = (LONG) call_func1(p_ios_eof, &ios_obj);
    ok(ret == 0, "expected 0 got %ld\n", ret);
    ret = (LONG) call_func1(p_ios_fail, &ios_obj);
    ok(ret == 0, "expected 0 got %ld\n", ret);
    ios_obj.state = IOSTATE_badbit;
    ret = (LONG) call_func1(p_ios_fail, &ios_obj);
    ok(ret == IOSTATE_badbit, "expected 4 got %ld\n", ret);
    ios_obj.state = (IOSTATE_eofbit|IOSTATE_failbit);
    ret = (LONG) call_func1(p_ios_fail, &ios_obj);
    ok(ret == IOSTATE_failbit, "expected 2 got %ld\n", ret);
    ios_obj.state = (IOSTATE_eofbit|IOSTATE_failbit|IOSTATE_badbit);
    ret = (LONG) call_func1(p_ios_fail, &ios_obj);
    ok(ret == (IOSTATE_failbit|IOSTATE_badbit), "expected 6 got %ld\n", ret);
    ios_obj.do_lock = 0;
    call_func2(p_ios_clear, &ios_obj, 0);
    ok(ios_obj.state == IOSTATE_goodbit, "expected 0 got %d\n", ios_obj.state);
    call_func2(p_ios_clear, &ios_obj, 0x8|IOSTATE_eofbit);
    ok(ios_obj.state == (0x8|IOSTATE_eofbit), "expected 9 got %d\n", ios_obj.state);
    ios_obj.do_lock = -1;

    SetEvent(lock_arg.release[0]);

    /* bitalloc */
    expected = 0x10000;
    for (i = 0; i < 20; i++) {
        ret = p_ios_bitalloc();
        ok(ret == expected, "expected %lx got %lx\n", expected, ret);
        ok(*p_ios_maxbit == expected, "expected %lx got %lx\n", expected, *p_ios_maxbit);
        expected <<= 1;
    }

    /* xalloc/pword/iword */
    ok(*p_ios_curindex == -1, "expected -1 got %d\n", *p_ios_curindex);
    expected = 0;
    for (i = 0; i < 8; i++) {
        ret = p_ios_xalloc();
        ok(ret == expected, "expected %ld got %ld\n", expected, ret);
        ok(*p_ios_curindex == expected, "expected %ld got %d\n", expected, *p_ios_curindex);
        pret = call_func2(p_ios_iword, &ios_obj, ret);
        ok(pret == &p_ios_statebuf[ret], "expected %p got %p\n", &p_ios_statebuf[ret], pret);
        ok(*pret == 0, "expected 0 got %ld\n", *pret);
        pret2 = call_func2(p_ios_pword, &ios_obj, ret);
        ok(pret2 == (void**)&p_ios_statebuf[ret], "expected %p got %p\n", (void**)&p_ios_statebuf[ret], pret2);
        expected++;
    }
    ret = p_ios_xalloc();
    ok(ret == -1, "expected -1 got %ld\n", ret);
    ok(*p_ios_curindex == 7, "expected 7 got %d\n", *p_ios_curindex);

    SetEvent(lock_arg.release[1]);
    SetEvent(lock_arg.release[2]);
    WaitForSingleObject(thread, INFINITE);

    ios_obj.delbuf = 1;
    call_func1(p_ios_dtor, &ios_obj);
    ok(ios_obj.state == IOSTATE_badbit, "expected %x got %x\n", IOSTATE_badbit, ios_obj.state);
    ok(*p_ios_fLockcInit == 4, "expected 4 got %d\n", *p_ios_fLockcInit);
    CloseHandle(lock_arg.lock);
    CloseHandle(lock_arg.release[0]);
    CloseHandle(lock_arg.release[1]);
    CloseHandle(lock_arg.release[2]);
    CloseHandle(thread);
}

static void test_ostream(void) {
    ostream os1, os2, *pos;
    filebuf fb1, fb2, *pfb;
    const char filename1[] = "test1";
    const char filename2[] = "test2";
    int fd, ret;

    memset(&os1, 0xab, sizeof(ostream));
    memset(&os2, 0xab, sizeof(ostream));
    memset(&fb1, 0xab, sizeof(filebuf));
    memset(&fb2, 0xab, sizeof(filebuf));

    /* constructors/destructors */
    pos = call_func2(p_ostream_ctor, &os1, TRUE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    call_func1(p_ostream_vbase_dtor, &os1);
    memset(&os1, 0xab, sizeof(ostream));
    pos = call_func3(p_ostream_sb_ctor, &os1, &fb1.base, TRUE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == &fb1.base, "expected %p got %p\n", &fb1.base, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    ok(fb1.base.allocated == 0xabababab, "expected %d got %d\n", 0xabababab, fb1.base.allocated);
    call_func1(p_ostream_vbase_dtor, &os1);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    memset(&os1.base_ios, 0xab, sizeof(ios));
    os1.unknown = 0xabababab;
    os1.base_ios.state = 0xabababab | IOSTATE_badbit;
    pos = call_func2(p_ostream_ctor, &os1, FALSE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == os2.base_ios.sb, "expected %p got %p\n", os2.base_ios.sb, os1.base_ios.sb);
    ok(os1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.delbuf);
    ok(os1.base_ios.tie == os2.base_ios.tie, "expected %p got %p\n", os2.base_ios.tie, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os1.base_ios.flags);
    ok(os1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.precision);
    ok(os1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    call_func1(p_ostream_dtor, &os1.base_ios);
    os1.unknown = 0xabababab;
    os1.base_ios.delbuf = 0;
    call_func1(p_filebuf_ctor, &fb1);
    pfb = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_out, filebuf_openprot);
    ok(pfb == &fb1, "wrong return, expected %p got %p\n", &fb1, pfb);
    ok(fb1.base.allocated == 1, "expected %d got %d\n", 1, fb1.base.allocated);
    pos = call_func3(p_ostream_sb_ctor, &os1, &fb1.base, FALSE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == &fb1.base, "expected %p got %p\n", &fb1.base, os1.base_ios.sb);
    ok(os1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == os2.base_ios.tie, "expected %p got %p\n", os2.base_ios.tie, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os1.base_ios.flags);
    ok(os1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.precision);
    ok(os1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    call_func1(p_ostream_dtor, &os1.base_ios);
    memset(&os1, 0xab, sizeof(ostream));
    pos = call_func3(p_ostream_sb_ctor, &os1, NULL, TRUE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    call_func1(p_ostream_vbase_dtor, &os1);
    memset(&os1.base_ios, 0xcd, sizeof(ios));
    os1.unknown = 0xcdcdcdcd;
    pos = call_func3(p_ostream_copy_ctor, &os2, &os1, TRUE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == os1.base_ios.sb, "expected %p got %p\n", os1.base_ios.sb, os2.base_ios.sb);
    ok(os2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == NULL, "expected %p got %p\n", NULL, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0, "expected 0 got %x\n", os2.base_ios.flags);
    ok(os2.base_ios.precision == 6, "expected 6 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.fill == ' ', "expected 32 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == 0, "expected 0 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == -1, "expected -1 got %d\n", os2.base_ios.do_lock);
    call_func1(p_ostream_vbase_dtor, &os2);
    memset(&os2.base_ios, 0xab, sizeof(ios));
    os2.unknown = 0xabababab;
    os2.base_ios.state = 0xabababab | IOSTATE_badbit;
    os2.base_ios.delbuf = 0;
    os2.base_ios.tie = &os2;
    pos = call_func3(p_ostream_copy_ctor, &os2, &os1, FALSE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == os1.base_ios.sb, "expected %p got %p\n", os1.base_ios.sb, os2.base_ios.sb);
    ok(os2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == &os2, "expected %p got %p\n", &os2, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os2.base_ios.flags);
    ok(os2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.precision);
    ok(os2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.width);
    ok(os2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.do_lock);
    call_func1(p_ostream_dtor, &os2.base_ios);
    os1.base_ios.sb = NULL;
    pos = call_func3(p_ostream_copy_ctor, &os2, &os1, TRUE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == NULL, "expected %p got %p\n", NULL, os2.base_ios.sb);
    ok(os2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == NULL, "expected %p got %p\n", NULL, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0, "expected 0 got %x\n", os2.base_ios.flags);
    ok(os2.base_ios.precision == 6, "expected 6 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.fill == ' ', "expected 32 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == 0, "expected 0 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == -1, "expected -1 got %d\n", os2.base_ios.do_lock);

    /* assignment */
    pos = call_func2(p_ostream_ctor, &os1, TRUE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    os1.unknown = 0xabababab;
    os1.base_ios.state = 0xabababab;
    os1.base_ios.special[0] = 0xabababab;
    os1.base_ios.delbuf = 0xabababab;
    os1.base_ios.tie = (ostream*) 0xabababab;
    os1.base_ios.flags = 0xabababab;
    os1.base_ios.precision = 0xabababab;
    os1.base_ios.width = 0xabababab;
    os1.base_ios.do_lock = 0xabababab;
    pos = call_func2(p_ostream_assign_sb, &os1, &fb1.base);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0xabababab, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == &fb1.base, "expected %p got %p\n", &fb1.base, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.fill);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    os1.base_ios.state = 0x8000;
    pos = call_func2(p_ostream_assign_sb, &os1, NULL);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0xabababab, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    ok(os1.base_ios.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.fill);
    ok(os1.base_ios.delbuf == 0, "expected 0 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    os2.unknown = 0xcdcdcdcd;
    os2.base_ios.state = 0xcdcdcdcd;
    os2.base_ios.special[0] = 0xcdcdcdcd;
    os2.base_ios.delbuf = 0xcdcdcdcd;
    os2.base_ios.tie = (ostream*) 0xcdcdcdcd;
    os2.base_ios.flags = 0xcdcdcdcd;
    os2.base_ios.precision = 0xcdcdcdcd;
    os2.base_ios.width = 0xcdcdcdcd;
    os2.base_ios.do_lock = 0xcdcdcdcd;
    pos = call_func2(p_ostream_assign, &os2, &os1);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0xcdcdcdcd, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == NULL, "expected %p got %p\n", NULL, os2.base_ios.sb);
    ok(os2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os2.base_ios.state);
    ok(os2.base_ios.special[0] == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, os2.base_ios.fill);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == NULL, "expected %p got %p\n", NULL, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0, "expected 0 got %x\n", os2.base_ios.flags);
    ok(os2.base_ios.precision == 6, "expected 6 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.width == 0, "expected 0 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, os2.base_ios.do_lock);

    /* flush */
    if (0) /* crashes on native */
        pos = call_func1(p_ostream_flush, &os1);
    os1.base_ios.sb = &fb2.base;
    call_func1(p_filebuf_ctor, &fb2);
    pos = call_func1(p_ostream_flush, &os1);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.sb = &fb1.base;
    os1.base_ios.state = IOSTATE_eofbit;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "Never gonna tell a lie", 22);
    ok(ret == 22, "expected 22 got %d\n", ret);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 22, "wrong put pointer, expected %p got %p\n", fb1.base.base + 22, fb1.base.pptr);
    pos = call_func1(p_ostream_flush, &os1);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, os1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    os1.base_ios.tie = &os2;
    os2.base_ios.sb = &fb2.base;
    os2.base_ios.state = IOSTATE_goodbit;
    pfb = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_out, filebuf_openprot);
    ok(pfb == &fb2, "wrong return, expected %p got %p\n", &fb2, pfb);
    ret = (int) call_func3(p_streambuf_xsputn, &fb2.base, "and hurt you", 12);
    ok(ret == 12, "expected 12 got %d\n", ret);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 12, "wrong put pointer, expected %p got %p\n", fb2.base.base + 12, fb2.base.pptr);
    pos = call_func1(p_ostream_flush, &os1);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 12, "wrong put pointer, expected %p got %p\n", fb2.base.base + 12, fb2.base.pptr);

    /* opfx */
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.sb = &fb1.base;
    os1.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
       IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.state = IOSTATE_failbit;
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(os1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.state = IOSTATE_goodbit;
    os1.base_ios.tie = &os2;
    os2.base_ios.sb = NULL;
    if (0) /* crashes on native */
        ret = (int) call_func1(p_ostream_opfx, &os1);
    os2.base_ios.sb = &fb2.base;
    os2.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "We've known each other", 22);
    ok(ret == 22, "expected 22 got %d\n", ret);
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os2.base_ios.state);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 22, "wrong put pointer, expected %p got %p\n", fb1.base.base + 22, fb1.base.pptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);

    /* osfx */
    os1.base_ios.state = IOSTATE_badbit;
    os1.base_ios.width = 0xab;
    call_func1(p_ostream_osfx, &os1);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 22, "wrong put pointer, expected %p got %p\n", fb1.base.base + 22, fb1.base.pptr);
    os1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func1(p_ostream_opfx, &os1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    os1.base_ios.sb = NULL;
    if (0) /* crashes on native */
        call_func1(p_ostream_osfx, &os1);
    os1.base_ios.sb = &fb1.base;
    os1.base_ios.flags = FLAGS_unitbuf;
    call_func1(p_ostream_osfx, &os1);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.flags == FLAGS_unitbuf, "expected %d got %d\n", FLAGS_unitbuf, os1.base_ios.flags);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);

    /* put */
    ret = (int) call_func3(p_streambuf_xsputn, &fb2.base, "for so long", 11);
    ok(ret == 11, "expected 11 got %d\n", ret);
    os1.base_ios.state = IOSTATE_badbit;
    os1.base_ios.width = 2;
    pos = call_func2(p_ostream_put_char, &os1, 'a');
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ok(os1.base_ios.width == 2, "expected 2 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 11, "wrong put pointer, expected %p got %p\n", fb2.base.base + 11, fb2.base.pptr);
    os1.base_ios.state = IOSTATE_goodbit;
    pos = call_func2(p_ostream_put_char, &os1, 'a');
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    os1.base_ios.flags = 0;
    pos = call_func2(p_ostream_put_char, &os1, 'b');
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 1, "wrong put pointer, expected %p got %p\n", fb1.base.base + 1, fb1.base.pptr);
    os1.base_ios.sb = NULL;
    if (0) /* crashes on native */
        pos = call_func2(p_ostream_put_char, &os1, 'c');
    os1.base_ios.sb = &fb1.base;
    os1.base_ios.width = 5;
    call_func1(p_filebuf_sync, &fb1);
    fd = fb1.fd;
    fb1.fd = -1;
    pos = call_func2(p_ostream_put_char, &os1, 'c');
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.fd = fd;

    /* write */
    pos = call_func3(p_ostream_write_char, &os1, "Your", 4);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    os1.base_ios.state = IOSTATE_goodbit;
    os1.base_ios.width = 1;
    pos = call_func3(p_ostream_write_char, &os1, "heart's", 7);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 7, "wrong put pointer, expected %p got %p\n", fb1.base.base + 7, fb1.base.pptr);
    os1.base_ios.sb = NULL;
    if (0) /* crashes on native */
        pos = call_func3(p_ostream_write_char, &os1, "been", 4);
    os1.base_ios.sb = &fb1.base;
    os1.base_ios.width = 5;
    call_func1(p_filebuf_sync, &fb1);
    fd = fb1.fd;
    fb1.fd = -1;
    pos = call_func3(p_ostream_write_char, &os1, "aching,", 7);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    fb1.fd = fd;

    /* seekp */
    os1.base_ios.state = IOSTATE_eofbit;
    pos = call_func3(p_ostream_seekp_offset, &os1, 0, SEEKDIR_beg);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, os1.base_ios.state);
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "but", 3);
    ok(ret == 3, "expected 3 got %d\n", ret);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 3, "wrong put pointer, expected %p got %p\n", fb1.base.base + 3, fb1.base.pptr);
    pos = call_func3(p_ostream_seekp_offset, &os1, 0, SEEKDIR_end);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, os1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    pos = call_func3(p_ostream_seekp_offset, &os1, -1, SEEKDIR_beg);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, os1.base_ios.state);
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "You're too shy", 14);
    ok(ret == 14, "expected 14 got %d\n", ret);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 14, "wrong put pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.pptr);
    pos = call_func2(p_ostream_seekp, &os1, 0);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, os1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    os1.base_ios.state = IOSTATE_badbit;
    pos = call_func2(p_ostream_seekp, &os1, -1);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);

    /* tellp */
    ret = (int) call_func1(p_ostream_tellp, &os1);
    ok(ret == 0, "wrong return, expected 0 got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "to say it", 9);
    ok(ret == 9, "expected 9 got %d\n", ret);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 9, "wrong put pointer, expected %p got %p\n", fb1.base.base + 9, fb1.base.pptr);
    ret = (int) call_func1(p_ostream_tellp, &os1);
    ok(ret == 9, "wrong return, expected 9 got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    os1.base_ios.state = IOSTATE_eofbit;
    fd = fb1.fd;
    fb1.fd = -1;
    ret = (int) call_func1(p_ostream_tellp, &os1);
    ok(ret == EOF, "wrong return, expected EOF got %d\n", ret);
    ok(os1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, os1.base_ios.state);
    fb1.fd = fd;

    /* writepad */
    os1.base_ios.state = IOSTATE_eofbit;
    os1.base_ios.flags =
        FLAGS_right|FLAGS_hex|FLAGS_showbase|FLAGS_showpoint|FLAGS_showpos|FLAGS_uppercase|FLAGS_fixed;
    os1.base_ios.fill = 'z';
    os1.base_ios.width = 9;
    pos = call_func3(p_ostream_writepad, &os1, "a", "b");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 9, "zzzzzzzab", 9), "expected 'zzzzzzzab' got '%s'\n", fb1.base.pptr - 9);
    pos = call_func3(p_ostream_writepad, &os1, "aa", "bb");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 9, "zzzzzaabb", 9), "expected 'zzzzzaabb' got '%s'\n", fb1.base.pptr - 9);
    pos = call_func3(p_ostream_writepad, &os1, "aaaaa", "bbbbb");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 10, "aaaaabbbbb", 10), "expected 'aaaaabbbbb' got '%s'\n", fb1.base.pptr - 10);
    os1.base_ios.flags |= FLAGS_internal;
    pos = call_func3(p_ostream_writepad, &os1, "aa", "bb");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 9, "aazzzzzbb", 9), "expected 'aazzzzzbb' got '%s'\n", fb1.base.pptr - 9);
    os1.base_ios.flags &= ~FLAGS_right;
    pos = call_func3(p_ostream_writepad, &os1, "a", "b");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 9, "azzzzzzzb", 9), "expected 'azzzzzzzb' got '%s'\n", fb1.base.pptr - 9);
    os1.base_ios.width = 6;
    pos = call_func3(p_ostream_writepad, &os1, "1", "2");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "1zzzz2", 6), "expected '1zzzz2' got '%s'\n", fb1.base.pptr - 6);
    pos = call_func3(p_ostream_writepad, &os1, "12345678", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 8, "12345678", 8), "expected '12345678' got '%s'\n", fb1.base.pptr - 8);
    os1.base_ios.flags |= FLAGS_left;
    pos = call_func3(p_ostream_writepad, &os1, "z1", "2z");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "z1zz2z", 6), "expected 'z1zz2z' got '%s'\n", fb1.base.pptr - 6);
    os1.base_ios.flags &= ~FLAGS_internal;
    pos = call_func3(p_ostream_writepad, &os1, "hell", "o");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "helloz", 6), "expected 'helloz' got '%s'\n", fb1.base.pptr - 6);
    os1.base_ios.flags |= FLAGS_right;
    pos = call_func3(p_ostream_writepad, &os1, "a", "b");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "abzzzz", 6), "expected 'abzzzz' got '%s'\n", fb1.base.pptr - 6);
    if (0) /* crashes on native */
        pos = call_func3(p_ostream_writepad, &os1, NULL, "o");
    pos = call_func3(p_ostream_writepad, &os1, "", "hello");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "helloz", 6), "expected 'helloz' got '%s'\n", fb1.base.pptr - 6);
    os1.base_ios.fill = '*';
    pos = call_func3(p_ostream_writepad, &os1, "", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "******", 6), "expected '******' got '%s'\n", fb1.base.pptr - 6);
    pos = call_func3(p_ostream_writepad, &os1, "aa", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "aa****", 6), "expected 'aa****' got '%s'\n", fb1.base.pptr - 6);
    os1.base_ios.flags = 0;
    pos = call_func3(p_ostream_writepad, &os1, "a", "b");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(!strncmp(fb1.base.pptr - 6, "****ab", 6), "expected '****ab' got '%s'\n", fb1.base.pptr - 6);
    call_func1(p_filebuf_sync, &fb1);
    fb1.fd = -1;
    pos = call_func3(p_ostream_writepad, &os1, "aa", "bb");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit, os1.base_ios.state);
    os1.base_ios.state = IOSTATE_goodbit;
    pos = call_func3(p_ostream_writepad, &os1, "", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    os1.base_ios.state = IOSTATE_goodbit;
    os1.base_ios.width = 0;
    pos = call_func3(p_ostream_writepad, &os1, "", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    pos = call_func3(p_ostream_writepad, &os1, "a", "");
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, os1.base_ios.state);
    fb1.fd = fd;

    call_func1(p_ostream_vbase_dtor, &os1);
    call_func1(p_ostream_vbase_dtor, &os2);
    call_func1(p_filebuf_dtor, &fb1);
    call_func1(p_filebuf_dtor, &fb2);
    ok(_unlink(filename1) == 0, "Couldn't unlink file named '%s'\n", filename1);
    ok(_unlink(filename2) == 0, "Couldn't unlink file named '%s'\n", filename2);
}

static void test_ostream_print(void)
{
    const BOOL is_64 = (sizeof(void*) == 8);
    ostream os, *pos;
    strstreambuf ssb, ssb_test1, ssb_test2, ssb_test3, *pssb;
    LONG length, expected_length;
    int ret, i;

    char param_char[] = {'a', '9', 'e'};
    const char* param_str[] = {"Test", "800", "3.14159", " Test"};
    int param_int[] = {0, 7, 10, 24 ,55, 1024, 1023, 65536, 2147483647, 2147483648u, 4294967295u, -20};
    float param_float[] = {1.0f, 0.0f, 4.25f, 3.999f, 12.0005f, 15.33582f, 15.0f, 15.22f, 21.123f, 0.1f,
        13.14159f, 0.00013f, 0.000013f, INFINITY, -INFINITY, NAN};
    double param_double[] = {1.0, 3.141592653589793238, 314.1592653589793238, 314.159265358979,
        1231314.269811862199, 9.961472e6, 8.98846567431e307, DBL_MAX};
    void* param_ptr[] = {NULL, (void*) 0xdeadbeef, (void*) 0x1234cdef, (void*) 0x1, (void*) 0xffffffff};
    streambuf* param_streambuf[] = {NULL, &ssb_test1.base, &ssb_test2.base, &ssb_test3.base};
    struct ostream_print_test {
        enum { type_chr, type_str, type_int, type_flt, type_dbl, type_ptr, type_sbf } type;
        int param_index;
        ios_io_state state;
        ios_flags flags;
        int precision;
        int fill;
        int width;
        const char *expected_text;
        ios_io_state expected_flags;
    } tests[] = {
        /* char */
        {type_chr, /* 'a' */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_chr, /* 'a' */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, 0, 6, ' ', 0, "a", IOSTATE_goodbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, 0, 6, ' ', 4, "   a", IOSTATE_goodbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, 0, 6, 'x', 3, "xxa", IOSTATE_goodbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, FLAGS_left, 6, ' ', 4, "a   ", IOSTATE_goodbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, FLAGS_left|FLAGS_internal, 6, ' ', 4, "   a", IOSTATE_goodbit},
        {type_chr, /* 'a' */ 0, IOSTATE_goodbit, FLAGS_internal|FLAGS_hex|FLAGS_showbase, 6, ' ', 4, "   a", IOSTATE_goodbit},
        {type_chr, /* '9' */ 1, IOSTATE_goodbit, FLAGS_oct|FLAGS_showbase|FLAGS_uppercase, 6, 'i', 2, "i9", IOSTATE_goodbit},
        {type_chr, /* '9' */ 1, IOSTATE_goodbit, FLAGS_showpos|FLAGS_scientific, 0, 'i', 2, "i9", IOSTATE_goodbit},
        {type_chr, /* 'e' */ 2, IOSTATE_goodbit, FLAGS_left|FLAGS_right|FLAGS_uppercase, 0, '*', 8, "e*******", IOSTATE_goodbit},
        /* const char* */
        {type_str, /* "Test" */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_str, /* "Test" */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        {type_str, /* "Test" */ 0, IOSTATE_goodbit, 0, 6, ' ', 0, "Test", IOSTATE_goodbit},
        {type_str, /* "Test" */ 0, IOSTATE_goodbit, 0, 6, ' ', 6, "  Test", IOSTATE_goodbit},
        {type_str, /* "Test" */ 0, IOSTATE_goodbit, FLAGS_internal, 6, 'x', 6, "xxTest", IOSTATE_goodbit},
        {type_str, /* "Test" */ 0, IOSTATE_goodbit, FLAGS_left, 6, ' ', 5, "Test ", IOSTATE_goodbit},
        {type_str, /* "Test" */ 0, IOSTATE_goodbit, FLAGS_left|FLAGS_hex|FLAGS_showpoint, 6, '?', 6, "Test??", IOSTATE_goodbit},
        {type_str, /* "800" */ 1, IOSTATE_goodbit, FLAGS_showbase|FLAGS_showpos, 6, ' ', 4, " 800", IOSTATE_goodbit},
        {type_str, /* "3.14159" */ 2, IOSTATE_goodbit, FLAGS_scientific, 2, 'x', 2, "3.14159", IOSTATE_goodbit},
        {type_str, /* " Test" */ 3, IOSTATE_goodbit, FLAGS_skipws, 6, 'x', 2, " Test", IOSTATE_goodbit},
        /* int */
        {type_int, /* 0 */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_int, /* 0 */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        {type_int, /* 0 */ 0, IOSTATE_goodbit, 0, 6, ' ', 4, "   0", IOSTATE_goodbit},
        {type_int, /* 7 */ 1, IOSTATE_goodbit, 0, 6, '0', 3, "007", IOSTATE_goodbit},
        {type_int, /* 10 */ 2, IOSTATE_goodbit, FLAGS_left, 6, ' ', 5, "10   ", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_left|FLAGS_hex, 6, ' ', 0, "18", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_left|FLAGS_hex|FLAGS_showbase, 6, ' ', 0, "0x18", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_internal|FLAGS_hex|FLAGS_showbase, 6, '*', 8, "0x****18", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_oct|FLAGS_showbase, 6, '*', 4, "*030", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_oct|FLAGS_dec|FLAGS_showbase, 6, ' ', 0, "030", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_oct|FLAGS_dec|FLAGS_hex|FLAGS_showbase, 6, ' ', 0, "0x18", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_left|FLAGS_oct|FLAGS_hex, 6, ' ', 5, "18   ", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_dec|FLAGS_hex|FLAGS_showbase, 6, ' ', 0, "0x18", IOSTATE_goodbit},
        {type_int, /* 24 */ 3, IOSTATE_goodbit, FLAGS_dec|FLAGS_showbase, 6, ' ', 0, "24", IOSTATE_goodbit},
        {type_int, /* 55 */ 4, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase|FLAGS_uppercase, 6, ' ', 0, "0X37", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase|FLAGS_showpoint|FLAGS_uppercase, 6, ' ', 0, "0X400", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase|FLAGS_showpos, 6, ' ', 0, "0x400", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_hex|FLAGS_showpos, 6, ' ', 0, "400", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_oct|FLAGS_showpos, 6, ' ', 0, "2000", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_internal|FLAGS_oct|FLAGS_showbase, 6, 'x', 8, "0xxx2000", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_showbase|FLAGS_showpoint|FLAGS_showpos, 6, ' ', 0, "+1024", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_dec|FLAGS_showbase|FLAGS_showpos, 6, 'a', 6, "a+1024", IOSTATE_goodbit},
        {type_int, /* 1024 */ 5, IOSTATE_goodbit, FLAGS_internal|FLAGS_showbase|FLAGS_showpos, 6, ' ', 8, "+   1024", IOSTATE_goodbit},
        {type_int, /* 1023 */ 6, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase|FLAGS_uppercase, 6, ' ', 0, "0X3FF", IOSTATE_goodbit},
        {type_int, /* 1023 */ 6, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase|FLAGS_scientific, 6, ' ', 0, "0x3ff", IOSTATE_goodbit},
        {type_int, /* 65536 */ 7, IOSTATE_goodbit, FLAGS_right|FLAGS_showpos|FLAGS_fixed, 6, ' ', 8, "  +65536", IOSTATE_goodbit},
        {type_int, /* 2147483647 */ 8, IOSTATE_goodbit, FLAGS_hex|FLAGS_showbase, 6, ' ', 0, "0x7fffffff", IOSTATE_goodbit},
        {type_int, /* 2147483648 */ 9, IOSTATE_goodbit, FLAGS_dec, 6, ' ', 0, "-2147483648", IOSTATE_goodbit},
        {type_int, /* 4294967295 */ 10, IOSTATE_goodbit, FLAGS_internal, 6, ' ', 8, "      -1", IOSTATE_goodbit},
        {type_int, /* -20 */ 11, IOSTATE_goodbit, FLAGS_internal|FLAGS_oct|FLAGS_showbase, 6, ' ', 8, "037777777754", IOSTATE_goodbit},
        {type_int, /* -20 */ 11, IOSTATE_goodbit, FLAGS_dec|FLAGS_showpos, 6, ' ', 0, "-20", IOSTATE_goodbit},
        {type_int, /* 0 */ 0, IOSTATE_goodbit, FLAGS_internal|FLAGS_showpos, 6, ' ', 8, "       0", IOSTATE_goodbit},
        /* float */
        {type_flt, /* 1.0f */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_flt, /* 1.0f */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        {type_flt, /* 1.0f */ 0, IOSTATE_goodbit, 0, 6, ' ', 0, "1", IOSTATE_goodbit},
        {type_flt, /* 0.0f */ 1, IOSTATE_goodbit, 0, 6, ' ', 0, "0", IOSTATE_goodbit},
        {type_flt, /* 4.25f */ 2, IOSTATE_goodbit, 0, 6, ' ', 0, "4.25", IOSTATE_goodbit},
        {type_flt, /* 3.999f */ 3, IOSTATE_goodbit, 0, 6, ' ', 0, "3.999", IOSTATE_goodbit},
        {type_flt, /* 3.999f */ 3, IOSTATE_goodbit, 0, 3, ' ', 0, "4", IOSTATE_goodbit},
        {type_flt, /* 12.0005f */ 4, IOSTATE_goodbit, 0, 6, ' ', 0, "12.0005", IOSTATE_goodbit},
        {type_flt, /* 12.0005f */ 4, IOSTATE_goodbit, 0, 5, ' ', 0, "12", IOSTATE_goodbit},
        {type_flt, /* 15.33582f */ 5, IOSTATE_goodbit, 0, 4, ' ', 0, "15.34", IOSTATE_goodbit},
        {type_flt, /* 15.0f */ 6, IOSTATE_goodbit, FLAGS_internal|FLAGS_hex|FLAGS_showbase, 6, ' ', 4, "  15", IOSTATE_goodbit},
        {type_flt, /* 15.22f */ 7, IOSTATE_goodbit, FLAGS_left|FLAGS_hex|FLAGS_showbase, 3, ' ', 6, "15.2  ", IOSTATE_goodbit},
        {type_flt, /* 15.22 */ 7, IOSTATE_goodbit, FLAGS_internal, 3, 'x', 6, "xx15.2", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_showpoint, 9, ' ', 0, "21.1230", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_showpos, 9, ' ', 0, "+21.123", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_internal|FLAGS_showpos, 9, ' ', 8, "+ 21.123", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_showpos, 0, ' ', 0, "+2e+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, 0, 1, ' ', 0, "2e+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_showpos, 2, ' ', 0, "+21", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, 0, 4, ' ', 0, "21.12", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_scientific, 2, ' ', 0, "2.11e+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_scientific, 4, ' ', 0, "2.1123e+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_scientific, 6, ' ', 0, "2.112300e+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_scientific|FLAGS_uppercase, 2, ' ', 0, "2.11E+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_scientific|FLAGS_uppercase, 2, '*', 10, "*2.11E+001", IOSTATE_goodbit},
        {type_flt, /* 21.123f */ 8, IOSTATE_goodbit, FLAGS_fixed, 6, ' ', 0, "21.122999", IOSTATE_goodbit},
        {type_flt, /* 0.1f */ 9, IOSTATE_goodbit, FLAGS_fixed, 6, ' ', 0, "0.100000", IOSTATE_goodbit},
        {type_flt, /* 0.1f */ 9, IOSTATE_goodbit, FLAGS_scientific, 6, ' ', 0, "1.000000e-001", IOSTATE_goodbit},
        {type_flt, /* 13.14159f */ 10, IOSTATE_goodbit, FLAGS_fixed, 3, ' ', 0, "13.142", IOSTATE_goodbit},
        {type_flt, /* 13.14159f */ 10, IOSTATE_goodbit, FLAGS_fixed, 8, ' ', 0, "13.141590", IOSTATE_goodbit},
        {type_flt, /* 13.14159f */ 10, IOSTATE_goodbit, FLAGS_fixed|FLAGS_showpoint, 8, ' ', 0, "13.141590", IOSTATE_goodbit},
        {type_flt, /* 13.14159f */ 10, IOSTATE_goodbit, FLAGS_scientific|FLAGS_fixed, 8, ' ', 0, "13.1416", IOSTATE_goodbit},
        {type_flt, /* 13.14159f */ 10, IOSTATE_goodbit, FLAGS_scientific|FLAGS_fixed, 2, ' ', 0, "13", IOSTATE_goodbit},
        {type_flt, /* 0.00013f */ 11, IOSTATE_goodbit, 0, -1, ' ', 0, "0.00013", IOSTATE_goodbit},
        {type_flt, /* 0.00013f */ 11, IOSTATE_goodbit, 0, -1, ' ', 0, "0.00013", IOSTATE_goodbit},
        {type_flt, /* 0.00013f */ 11, IOSTATE_goodbit, 0, -1, ' ', 0, "0.00013", IOSTATE_goodbit},
        {type_flt, /* 0.000013f */ 12, IOSTATE_goodbit, 0, 4, ' ', 0, "1.3e-005", IOSTATE_goodbit},
        {type_flt, /* 0.000013f */ 12, IOSTATE_goodbit, FLAGS_showpoint, 4, ' ', 0, "1.300e-005", IOSTATE_goodbit},
        {type_flt, /* 0.000013f */ 12, IOSTATE_goodbit, FLAGS_showpoint, 6, ' ', 0, "1.30000e-005", IOSTATE_goodbit},
        {type_flt, /* INFINITY */ 13, IOSTATE_goodbit, 0, 6, ' ', 0, "1.#INF", IOSTATE_goodbit},
        {type_flt, /* INFINITY */ 13, IOSTATE_goodbit, 0, 4, ' ', 0, "1.#IO", IOSTATE_goodbit},
        {type_flt, /* -INFINITY */ 14, IOSTATE_goodbit, 0, 6, ' ', 0, "-1.#INF", IOSTATE_goodbit},
        {type_flt, /* NAN */ 15, IOSTATE_goodbit, 0, 6, ' ', 0, "1.#QNAN", IOSTATE_goodbit},
        /* double */
        {type_dbl, /* 1.0 */ 0, IOSTATE_goodbit, 0, 6, ' ', 0, "1", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, 0, 6, ' ', 0, "3.14159", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, 0, 9, ' ', 0, "3.14159265", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, 0, 12, ' ', 0, "3.14159265359", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, 0, 15, ' ', 0, "3.14159265358979", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, 0, 16, ' ', 0, "3.14159265358979", IOSTATE_goodbit},
        {type_dbl, /* 314.1592653589793238 */ 2, IOSTATE_goodbit, 0, 16, ' ', 0, "314.159265358979", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, FLAGS_scientific, 16, ' ', 0, "3.141592653589793e+000", IOSTATE_goodbit},
        {type_dbl, /* 314.1592653589793238 */ 2, IOSTATE_goodbit, FLAGS_scientific, 16, ' ', 0, "3.141592653589793e+002", IOSTATE_goodbit},
        {type_dbl, /* 3.141592653589793238 */ 1, IOSTATE_goodbit, FLAGS_fixed, -1, ' ', 0, "3.141592653589793", IOSTATE_goodbit},
        {type_dbl, /* 314.159265358979 */ 3, IOSTATE_goodbit, FLAGS_fixed, 12, ' ', 0, "314.159265358979", IOSTATE_goodbit},
        {type_dbl, /* 1231314.269811862199 */ 4, IOSTATE_goodbit, FLAGS_fixed, 10, ' ', 0, "1231314.2698118621", IOSTATE_goodbit},
        {type_dbl, /* 9.961472e6 */ 5, IOSTATE_goodbit, FLAGS_fixed, 500, ' ', 0, "9961472.000000000000000", IOSTATE_goodbit},
        /* crashes on XP/2k3 {type_dbl, 8.98846567431e307 6, IOSTATE_goodbit, FLAGS_fixed, 500, ' ', 0, "", IOSTATE_goodbit}, */
        /* crashes on XP/2k3 {type_dbl, DBL_MAX 7, IOSTATE_goodbit, FLAGS_fixed|FLAGS_showpos, 500, ' ', 5, "     ", IOSTATE_goodbit}, */
        {type_dbl, /* DBL_MAX */ 7, IOSTATE_goodbit, FLAGS_showpoint, 500, ' ', 0, "1.79769313486232e+308", IOSTATE_goodbit},
        /* void* */
        {type_ptr, /* NULL */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_ptr, /* NULL */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        {type_ptr, /* NULL */ 0, IOSTATE_goodbit, 0, 6, ' ', 0,
            is_64 ? "0x0000000000000000" : "0x00000000", IOSTATE_goodbit},
        {type_ptr, /* 0xdeadbeef */ 1, IOSTATE_goodbit, 0, 6, ' ', 0,
            is_64 ? "0x00000000DEADBEEF" : "0xDEADBEEF", IOSTATE_goodbit},
        {type_ptr, /* 0xdeadbeef */ 1, IOSTATE_goodbit, 0, 6, '*', 12,
            is_64 ? "0x00000000DEADBEEF" : "**0xDEADBEEF", IOSTATE_goodbit},
        {type_ptr, /* 0xdeadbeef */ 1, IOSTATE_goodbit, FLAGS_internal, 6, ' ', 14,
            is_64 ? "0x00000000DEADBEEF" : "0x    DEADBEEF", IOSTATE_goodbit},
        {type_ptr, /* 0xdeadbeef */ 1, IOSTATE_goodbit, FLAGS_left, 6, 'x', 11,
            is_64 ? "0x00000000DEADBEEF" : "0xDEADBEEFx", IOSTATE_goodbit},
        {type_ptr, /* 0x1234cdef */ 2, IOSTATE_goodbit, FLAGS_dec|FLAGS_showpos, 6, ' ', 0,
            is_64 ? "0x000000001234CDEF" : "0x1234CDEF", IOSTATE_goodbit},
        {type_ptr, /* 0x1 */ 3, IOSTATE_goodbit, FLAGS_oct|FLAGS_showbase, 6, ' ', 0,
            is_64 ? "0x0000000000000001" : "0x00000001", IOSTATE_goodbit},
        {type_ptr, /* 0xffffffff */ 4, IOSTATE_goodbit, FLAGS_hex|FLAGS_showpoint, 6, ' ', 0,
            is_64 ? "0x00000000FFFFFFFF" : "0xFFFFFFFF", IOSTATE_goodbit},
        {type_ptr, /* 0xffffffff */ 4, IOSTATE_goodbit, FLAGS_uppercase|FLAGS_fixed, 6, ' ', 0,
            is_64 ? "0X00000000FFFFFFFF" : "0XFFFFFFFF", IOSTATE_goodbit},
        {type_ptr, /* 0x1 */ 3, IOSTATE_goodbit, FLAGS_uppercase, 6, ' ', 0,
            is_64 ? "0X0000000000000001" : "0X00000001", IOSTATE_goodbit},
        {type_ptr, /* NULL */ 0, IOSTATE_goodbit, FLAGS_uppercase, 6, ' ', 0,
            is_64 ? "0x0000000000000000" : "0x00000000", IOSTATE_goodbit},
        {type_ptr, /* NULL */ 0, IOSTATE_goodbit, FLAGS_uppercase|FLAGS_showbase, 12, 'x', 12,
            is_64 ? "0x0000000000000000" : "xx0x00000000", IOSTATE_goodbit},
        {type_ptr, /* NULL */ 0, IOSTATE_goodbit, FLAGS_internal|FLAGS_uppercase|FLAGS_showbase, 6, '?', 20,
            is_64 ? "0x??0000000000000000" : "0x??????????00000000", IOSTATE_goodbit},
        /* streambuf* */
        {type_sbf, /* NULL */ 0, IOSTATE_badbit, 0, 6, ' ', 0, "", IOSTATE_badbit|IOSTATE_failbit},
        {type_sbf, /* NULL */ 0, IOSTATE_eofbit, 0, 6, ' ', 0, "", IOSTATE_eofbit|IOSTATE_failbit},
        /* crashes on native {STREAMBUF, NULL 0, IOSTATE_goodbit, 0, 6, ' ', 0, "", IOSTATE_goodbit}, */
        {type_sbf, /* &ssb_test1.base */ 1, IOSTATE_goodbit, FLAGS_skipws|FLAGS_showpos|FLAGS_uppercase,
            6, ' ', 0, "  we both know what's been going on ", IOSTATE_goodbit},
        {type_sbf, /* &ssb_test1.base */ 2, IOSTATE_goodbit, FLAGS_left|FLAGS_hex|FLAGS_showbase,
            6, '*', 50, "123 We know the game and", IOSTATE_goodbit},
        {type_sbf, /* &ssb_test1.base */ 3, IOSTATE_goodbit, FLAGS_internal|FLAGS_scientific|FLAGS_showpoint,
            6, '*', 50, "we're gonna play it 3.14159", IOSTATE_goodbit}
    };

    pssb = call_func1(p_strstreambuf_ctor, &ssb);
    ok(pssb == &ssb, "wrong return, expected %p got %p\n", &ssb, pssb);
    pos = call_func3(p_ostream_sb_ctor, &os, &ssb.base, TRUE);
    ok(pos == &os, "wrong return, expected %p got %p\n", &os, pos);
    pssb = call_func1(p_strstreambuf_ctor, &ssb_test1);
    ok(pssb == &ssb_test1, "wrong return, expected %p got %p\n", &ssb_test1, pssb);
    ret = (int) call_func3(p_streambuf_xsputn, &ssb_test1.base, "  we both know what's been going on ", 36);
    ok(ret == 36, "expected 36 got %d\n", ret);
    pssb = call_func1(p_strstreambuf_ctor, &ssb_test2);
    ok(pssb == &ssb_test2, "wrong return, expected %p got %p\n", &ssb_test2, pssb);
    ret = (int) call_func3(p_streambuf_xsputn, &ssb_test2.base, "123 We know the game and", 24);
    ok(ret == 24, "expected 24 got %d\n", ret);
    pssb = call_func1(p_strstreambuf_ctor, &ssb_test3);
    ok(pssb == &ssb_test3, "wrong return, expected %p got %p\n", &ssb_test3, pssb);
    ret = (int) call_func3(p_streambuf_xsputn, &ssb_test3.base, "we're gonna play it 3.14159", 27);
    ok(ret == 27, "expected 27 got %d\n", ret);

    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        os.base_ios.state = tests[i].state;
        os.base_ios.flags = tests[i].flags;
        os.base_ios.precision = tests[i].precision;
        os.base_ios.fill = tests[i].fill;
        os.base_ios.width = tests[i].width;
        ssb.base.pptr = ssb.base.pbase;

        switch (tests[i].type) {
        case type_chr:
            pos = call_func2(p_ostream_print_char, &os, (int) param_char[tests[i].param_index]); break;
        case type_str:
            pos = call_func2(p_ostream_print_str, &os, param_str[tests[i].param_index]); break;
        case type_int:
            pos = call_func2(p_ostream_print_int, &os, param_int[tests[i].param_index]); break;
        case type_flt:
            pos = call_func2_ptr_flt(p_ostream_print_float, &os, param_float[tests[i].param_index]); break;
        case type_dbl:
            pos = call_func2_ptr_dbl(p_ostream_print_double, &os, param_double[tests[i].param_index]); break;
        case type_ptr:
            pos = call_func2(p_ostream_print_ptr, &os, param_ptr[tests[i].param_index]); break;
        case type_sbf:
            pos = call_func2(p_ostream_print_streambuf, &os, param_streambuf[tests[i].param_index]); break;
        }

        length = ssb.base.pptr - ssb.base.pbase;
        expected_length = strlen(tests[i].expected_text);
        ok(pos == &os, "Test %d: wrong return, expected %p got %p\n", i, &os, pos);
        ok(os.base_ios.state == tests[i].expected_flags, "Test %d: expected %d got %d\n", i,
            tests[i].expected_flags, os.base_ios.state);
        ok(os.base_ios.width == 0, "Test %d: expected 0 got %d\n", i, os.base_ios.width);
        ok(expected_length == length, "Test %d: wrong output length, expected %ld got %ld\n", i, expected_length, length);
        ok(!strncmp(tests[i].expected_text, ssb.base.pbase, length),
            "Test %d: wrong output, expected '%s' got '%s'\n", i, tests[i].expected_text, ssb.base.pbase);
    }

    call_func1(p_ostream_vbase_dtor, &os);
    call_func1(p_strstreambuf_dtor, &ssb);
    call_func1(p_strstreambuf_dtor, &ssb_test1);
    call_func1(p_strstreambuf_dtor, &ssb_test2);
    call_func1(p_strstreambuf_dtor, &ssb_test3);
}

static void test_ostream_withassign(void)
{
    ostream osa1, osa2, *posa, *pos;
    streambuf sb, *psb;

    memset(&osa1, 0xab, sizeof(ostream));
    memset(&osa2, 0xab, sizeof(ostream));

    /* constructors/destructors */
    posa = call_func3(p_ostream_withassign_sb_ctor, &osa1, NULL, TRUE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == -1, "expected -1 got %d\n", osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_vbase_dtor, &osa1);
    osa1.unknown = 0xabababab;
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.delbuf = 0;
    posa = call_func3(p_ostream_withassign_sb_ctor, &osa1, NULL, FALSE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa1.base_ios.sb);
    ok(osa1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == osa2.base_ios.tie, "expected %p got %p\n", osa2.base_ios.tie, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_dtor, &osa1.base_ios);
    osa1.unknown = 0xabababab;
    osa1.base_ios.sb = (streambuf*) 0xabababab;
    posa = call_func3(p_ostream_withassign_sb_ctor, &osa1, &sb, TRUE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == -1, "expected -1 got %d\n", osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_vbase_dtor, &osa1);
    osa1.unknown = 0xabababab;
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.state |= IOSTATE_badbit;
    osa1.base_ios.delbuf = 0;
    posa = call_func3(p_ostream_withassign_sb_ctor, &osa1, &sb, FALSE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == osa2.base_ios.tie, "expected %p got %p\n", osa2.base_ios.tie, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_dtor, &osa1.base_ios);
    posa = call_func2(p_ostream_withassign_ctor, &osa2, TRUE);
    ok(posa == &osa2, "wrong return, expected %p got %p\n", &osa2, posa);
    ok(osa2.unknown == 0, "expected 0 got %d\n", osa2.unknown);
    ok(osa2.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa2.base_ios.sb);
    ok(osa2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, osa2.base_ios.state);
    ok(osa2.base_ios.delbuf == 0, "expected 0 got %d\n", osa2.base_ios.delbuf);
    ok(osa2.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa2.base_ios.tie);
    ok(osa2.base_ios.flags == 0, "expected 0 got %x\n", osa2.base_ios.flags);
    ok(osa2.base_ios.precision == 6, "expected 6 got %d\n", osa2.base_ios.precision);
    ok(osa2.base_ios.fill == ' ', "expected 32 got %d\n", osa2.base_ios.fill);
    ok(osa2.base_ios.width == 0, "expected 0 got %d\n", osa2.base_ios.width);
    ok(osa2.base_ios.do_lock == -1, "expected -1 got %d\n", osa2.base_ios.do_lock);
    call_func1(p_ostream_withassign_vbase_dtor, &osa2);
    osa2.unknown = 0xcdcdcdcd;
    memset(&osa2.base_ios, 0xcd, sizeof(ios));
    osa2.base_ios.delbuf = 0;
    psb = osa2.base_ios.sb;
    pos = osa2.base_ios.tie;
    posa = call_func2(p_ostream_withassign_ctor, &osa2, FALSE);
    ok(posa == &osa2, "wrong return, expected %p got %p\n", &osa2, posa);
    ok(osa2.unknown == 0, "expected 0 got %d\n", osa2.unknown);
    ok(osa2.base_ios.sb == psb, "expected %p got %p\n", psb, osa2.base_ios.sb);
    ok(osa2.base_ios.state == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, osa2.base_ios.state);
    ok(osa2.base_ios.delbuf == 0, "expected 0 got %d\n", osa2.base_ios.delbuf);
    ok(osa2.base_ios.tie == pos, "expected %p got %p\n", pos, osa2.base_ios.tie);
    ok(osa2.base_ios.flags == 0xcdcdcdcd, "expected %d got %x\n", 0xcdcdcdcd, osa2.base_ios.flags);
    ok(osa2.base_ios.precision == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, osa2.base_ios.precision);
    ok(osa2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", osa2.base_ios.fill);
    ok(osa2.base_ios.width == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, osa2.base_ios.width);
    ok(osa2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, osa2.base_ios.do_lock);
    call_func1(p_ostream_withassign_dtor, &osa2.base_ios);
    osa1.unknown = 0xabababab;
    osa2.unknown = osa2.base_ios.delbuf = 0xcdcdcdcd;
    posa = call_func3(p_ostream_withassign_copy_ctor, &osa1, &osa2, TRUE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == osa2.base_ios.sb, "expected %p got %p\n", osa2.base_ios.sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == osa2.base_ios.tie, "expected %p got %p\n", osa2.base_ios.tie, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == -1, "expected -1 got %d\n", osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_vbase_dtor, &osa1);
    osa1.unknown = 0xabababab;
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.state |= IOSTATE_badbit;
    osa1.base_ios.delbuf = 0;
    pos = osa1.base_ios.tie;
    posa = call_func3(p_ostream_withassign_copy_ctor, &osa1, &osa2, FALSE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == osa2.base_ios.sb, "expected %p got %p\n", osa2.base_ios.sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == pos, "expected %p got %p\n", pos, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_dtor, &osa1.base_ios);
    osa1.unknown = 0xabababab;
    osa2.base_ios.sb = NULL;
    posa = call_func3(p_ostream_withassign_copy_ctor, &osa1, &osa2, TRUE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa1.base_ios.sb);
    ok(osa1.base_ios.state == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == osa2.base_ios.tie, "expected %p got %p\n", osa2.base_ios.tie, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == (char) 0xcd, "expected -51 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == -1, "expected -1 got %d\n", osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_vbase_dtor, &osa1);
    osa1.unknown = 0xabababab;
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.delbuf = 0;
    posa = call_func3(p_ostream_withassign_copy_ctor, &osa1, &osa2, FALSE);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0, "expected 0 got %d\n", osa1.unknown);
    ok(osa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa1.base_ios.sb);
    ok(osa1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == pos, "expected %p got %p\n", pos, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.precision);
    ok(osa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    call_func1(p_ostream_withassign_dtor, &osa1.base_ios);

    /* assignment */
    osa1.unknown = 0xabababab;
    osa1.base_ios.sb = (streambuf*) 0xabababab;
    if (0) /* crashes on native */
        osa1.base_ios.delbuf = 0xabababab;
    posa = call_func2(p_ostream_withassign_assign_sb, &osa1, &sb);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.unknown);
    ok(osa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.delbuf = 0;
    posa = call_func2(p_ostream_withassign_assign_sb, &osa1, NULL);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.unknown);
    ok(osa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.delbuf = 0;
    osa2.base_ios.sb = &sb;
    posa = call_func2(p_ostream_withassign_assign_os, &osa1, &osa2);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.unknown);
    ok(osa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
    memset(&osa1.base_ios, 0xab, sizeof(ios));
    osa1.base_ios.delbuf = 0;
    posa = call_func2(p_ostream_withassign_assign, &osa1, &osa2);
    ok(posa == &osa1, "wrong return, expected %p got %p\n", &osa1, posa);
    ok(osa1.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.unknown);
    ok(osa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, osa1.base_ios.sb);
    ok(osa1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, osa1.base_ios.state);
    ok(osa1.base_ios.delbuf == 0, "expected 0 got %d\n", osa1.base_ios.delbuf);
    ok(osa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, osa1.base_ios.tie);
    ok(osa1.base_ios.flags == 0, "expected 0 got %x\n", osa1.base_ios.flags);
    ok(osa1.base_ios.precision == 6, "expected 6 got %d\n", osa1.base_ios.precision);
    ok(osa1.base_ios.fill == ' ', "expected 32 got %d\n", osa1.base_ios.fill);
    ok(osa1.base_ios.width == 0, "expected 0 got %d\n", osa1.base_ios.width);
    ok(osa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, osa1.base_ios.do_lock);
}

static void test_ostrstream(void)
{
    ostream os1, os2, *pos, *pos2;
    strstreambuf *pssb;
    char buffer[32];
    int ret;

    memset(&os1, 0xab, sizeof(ostream));
    memset(&os2, 0xab, sizeof(ostream));

    /* constructors/destructors */
    pos = call_func2(p_ostrstream_ctor, &os1, TRUE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    ok(pssb->dynamic == 1, "expected 1, got %d\n", pssb->dynamic);
    ok(pssb->increase == 1, "expected 1, got %d\n", pssb->increase);
    ok(pssb->constant == 0, "expected 0, got %d\n", pssb->constant);
    ok(pssb->f_alloc == NULL, "expected %p, got %p\n", NULL, pssb->f_alloc);
    ok(pssb->f_free == NULL, "expected %p, got %p\n", NULL, pssb->f_free);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pssb->base.base);
    ok(pssb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pssb->base.ebuf);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os1);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    os1.unknown = 0xabababab;
    memset(&os1.base_ios, 0xab, sizeof(ios));
    os1.base_ios.delbuf = 0;
    pos = call_func2(p_ostrstream_ctor, &os1, FALSE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == os2.base_ios.tie, "expected %p got %p\n", os2.base_ios.tie, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os1.base_ios.flags);
    ok(os1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.precision);
    ok(os1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    ok(pssb->dynamic == 1, "expected 1, got %d\n", pssb->dynamic);
    ok(pssb->increase == 1, "expected 1, got %d\n", pssb->increase);
    ok(pssb->constant == 0, "expected 0, got %d\n", pssb->constant);
    ok(pssb->f_alloc == NULL, "expected %p, got %p\n", NULL, pssb->f_alloc);
    ok(pssb->f_free == NULL, "expected %p, got %p\n", NULL, pssb->f_free);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pssb->base.base);
    ok(pssb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pssb->base.ebuf);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_dtor, &os1.base_ios);
    ok(os1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, os1.base_ios.sb);
    ok(os1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);

    memset(&os1, 0xab, sizeof(ostream));
    pos = call_func5(p_ostrstream_buffer_ctor, &os1, buffer, 32, OPENMODE_in|OPENMODE_out, TRUE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 32, "wrong buffer end, expected %p got %p\n", buffer + 32, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 32, "wrong put end, expected %p got %p\n", buffer + 32, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os1);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    strcpy(buffer, "Test");
    memset(&os1, 0xab, sizeof(ostream));
    pos = call_func5(p_ostrstream_buffer_ctor, &os1, buffer, 16, OPENMODE_ate, TRUE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 16, "wrong buffer end, expected %p got %p\n", buffer + 16, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer + 4, "wrong put pointer, expected %p got %p\n", buffer + 4, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 16, "wrong put end, expected %p got %p\n", buffer + 16, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os1);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);
    os1.unknown = 0xabababab;
    memset(&os1.base_ios, 0xab, sizeof(ios));
    os1.base_ios.delbuf = 0;
    pos = call_func5(p_ostrstream_buffer_ctor, &os1, buffer, -1, OPENMODE_app, FALSE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == os2.base_ios.tie, "expected %p got %p\n", os2.base_ios.tie, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os1.base_ios.flags);
    ok(os1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.precision);
    ok(os1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.width);
    ok(os1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == (char *)((ULONG_PTR)buffer + 0x7fffffff) || pssb->base.ebuf == (char*) -1,
        "wrong buffer end, expected %p + 0x7fffffff or -1, got %p\n", buffer, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer + 4, "wrong put pointer, expected %p got %p\n", buffer + 4, pssb->base.pptr);
    ok(pssb->base.epptr == (char *)((ULONG_PTR)buffer + 0x7fffffff) || pssb->base.epptr == (char*) -1,
        "wrong buffer end, expected %p + 0x7fffffff or -1, got %p\n", buffer, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_dtor, &os1.base_ios);
    ok(os1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, os1.base_ios.sb);
    ok(os1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);
    memset(&os1, 0xab, sizeof(ostream));
    pos = call_func5(p_ostrstream_buffer_ctor, &os1, buffer, 0, OPENMODE_trunc, TRUE);
    pssb = (strstreambuf*) os1.base_ios.sb;
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ok(os1.unknown == 0, "expected 0 got %d\n", os1.unknown);
    ok(os1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os1.base_ios.state);
    ok(os1.base_ios.delbuf == 1, "expected 1 got %d\n", os1.base_ios.delbuf);
    ok(os1.base_ios.tie == NULL, "expected %p got %p\n", NULL, os1.base_ios.tie);
    ok(os1.base_ios.flags == 0, "expected 0 got %x\n", os1.base_ios.flags);
    ok(os1.base_ios.precision == 6, "expected 6 got %d\n", os1.base_ios.precision);
    ok(os1.base_ios.fill == ' ', "expected 32 got %d\n", os1.base_ios.fill);
    ok(os1.base_ios.width == 0, "expected 0 got %d\n", os1.base_ios.width);
    ok(os1.base_ios.do_lock == -1, "expected -1 got %d\n", os1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 4, "wrong buffer end, expected %p got %p\n", buffer + 4, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 4, "wrong put end, expected %p got %p\n", buffer + 4, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os1);
    ok(os1.base_ios.sb == NULL, "expected %p got %p\n", NULL, os1.base_ios.sb);
    ok(os1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os1.base_ios.state);

    os1.unknown = 0xcdcdcdcd;
    memset(&os1.base_ios, 0xcd, sizeof(ios));
    pos = call_func3(p_ostrstream_copy_ctor, &os2, &os1, TRUE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == os1.base_ios.sb, "expected %p got %p\n", os1.base_ios.sb, os2.base_ios.sb);
    ok(os2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == os1.base_ios.tie, "expected %p got %p\n", os1.base_ios.tie, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, os2.base_ios.flags);
    ok(os2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == -1, "expected -1 got %d\n", os2.base_ios.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os2);
    ok(os2.base_ios.sb == NULL, "expected %p got %p\n", NULL, os2.base_ios.sb);
    ok(os2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os2.base_ios.state);
    os2.unknown = 0xabababab;
    memset(&os2.base_ios, 0xab, sizeof(ios));
    os2.base_ios.delbuf = 0;
    pos2 = os2.base_ios.tie;
    pos = call_func3(p_ostrstream_copy_ctor, &os2, &os1, FALSE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == os1.base_ios.sb, "expected %p got %p\n", os1.base_ios.sb, os2.base_ios.sb);
    ok(os2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == pos2, "expected %p got %p\n", pos2, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, os2.base_ios.flags);
    ok(os2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.precision);
    ok(os2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.width);
    ok(os2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.do_lock);
    call_func1(p_ostrstream_dtor, &os2.base_ios);
    os1.base_ios.sb = NULL;
    memset(&os2, 0xab, sizeof(ostream));
    pos = call_func3(p_ostrstream_copy_ctor, &os2, &os1, TRUE);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0, "expected 0 got %d\n", os2.unknown);
    ok(os2.base_ios.sb == NULL, "expected %p got %p\n", NULL, os2.base_ios.sb);
    ok(os2.base_ios.state == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == os1.base_ios.tie, "expected %p got %p\n", os1.base_ios.tie, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, os2.base_ios.flags);
    ok(os2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == -1, "expected -1 got %d\n", os2.base_ios.do_lock);
    call_func1(p_ostrstream_vbase_dtor, &os2);

    /* assignment */
    os2.unknown = 0xabababab;
    memset(&os2.base_ios, 0xab, sizeof(ios));
    os2.base_ios.delbuf = 0;
    pos = call_func2(p_ostrstream_assign, &os2, &os1);
    ok(pos == &os2, "wrong return, expected %p got %p\n", &os2, pos);
    ok(os2.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, os2.unknown);
    ok(os2.base_ios.sb == NULL, "expected %p got %p\n", NULL, os2.base_ios.sb);
    ok(os2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, os2.base_ios.state);
    ok(os2.base_ios.delbuf == 0, "expected 0 got %d\n", os2.base_ios.delbuf);
    ok(os2.base_ios.tie == NULL, "expected %p got %p\n", NULL, os2.base_ios.tie);
    ok(os2.base_ios.flags == 0, "expected %x got %x\n", 0, os2.base_ios.flags);
    ok(os2.base_ios.precision == 6, "expected 6 got %d\n", os2.base_ios.precision);
    ok(os2.base_ios.fill == ' ', "expected 32 got %d\n", os2.base_ios.fill);
    ok(os2.base_ios.width == 0, "expected 0 got %d\n", os2.base_ios.width);
    ok(os2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, os2.base_ios.do_lock);

    /* pcount */
    pos = call_func2(p_ostrstream_ctor, &os1, TRUE);
    ok(pos == &os1, "wrong return, expected %p got %p\n", &os1, pos);
    ret = (int) call_func1(p_ostrstream_pcount, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    pssb = (strstreambuf*) os1.base_ios.sb;
    pssb->base.pptr = (char*) 5;
    ret = (int) call_func1(p_ostrstream_pcount, &os1);
    ok(ret == 5, "expected 5 got %d\n", ret);
    pssb->base.pbase = (char*) 2;
    ret = (int) call_func1(p_ostrstream_pcount, &os1);
    ok(ret == 3, "expected 3 got %d\n", ret);
    pssb->base.pptr = NULL;
    ret = (int) call_func1(p_ostrstream_pcount, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    if (0) /* crashes on native */
        os1.base_ios.sb = NULL;
    pssb->base.pptr = (char*) 1;
    ret = (int) call_func1(p_ostrstream_pcount, &os1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    call_func1(p_ostrstream_vbase_dtor, &os1);
}

static void test_ofstream(void)
{
    const char *filename = "ofstream_test";
    ostream ofs, ofs_copy, *pofs;
    streambuf *psb;
    filebuf *pfb;
    char buffer[64];
    char st[8];
    int fd, ret;

    memset(&ofs, 0xab, sizeof(ostream));

    /* constructors/destructors */
    pofs = call_func2(p_ofstream_ctor, &ofs, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    ok(ofs.unknown == 0, "expected 0 got %d\n", ofs.unknown);
    ok(ofs.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ofs.base_ios.sb);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(ofs.base_ios.delbuf == 1, "expected 1 got %d\n", ofs.base_ios.delbuf);
    ok(ofs.base_ios.tie == NULL, "expected %p got %p\n", NULL, ofs.base_ios.tie);
    ok(ofs.base_ios.flags == 0x0, "expected %x got %x\n", 0x0, ofs.base_ios.flags);
    ok(ofs.base_ios.precision == 6, "expected 6 got %d\n", ofs.base_ios.precision);
    ok(ofs.base_ios.fill == ' ', "expected 32 got %d\n", ofs.base_ios.fill);
    ok(ofs.base_ios.width == 0, "expected 0 got %d\n", ofs.base_ios.width);
    ok(ofs.base_ios.do_lock == -1, "expected -1 got %d\n", ofs.base_ios.do_lock);
    ok(pfb->fd == -1, "wrong fd, expected -1 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == -1, "wrong fd, expected 0 got %d\n", pfb->fd);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    pofs = call_func3(p_ofstream_fd_ctor, &ofs, 42, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(ofs.base_ios.delbuf == 1, "expected 1 got %d\n", ofs.base_ios.delbuf);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 42, "wrong fd, expected 42 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);

    pofs = call_func3(p_ofstream_copy_ctor, &ofs_copy, &ofs, TRUE);
    pfb = (filebuf*) ofs_copy.base_ios.sb;
    ok(pofs == &ofs_copy, "wrong return, expected %p got %p\n", &ofs_copy, pofs);
    ok(ofs_copy.base_ios.sb == ofs.base_ios.sb, "expected shared streambuf\n");
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(ofs_copy.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs_copy.base_ios.state);

    call_func1(p_ofstream_vbase_dtor, &ofs_copy);
    call_func1(p_ofstream_dtor, &ofs.base_ios);

    pofs = call_func5(p_ofstream_buffer_ctor, &ofs, 53, buffer, ARRAY_SIZE(buffer), TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(ofs.base_ios.delbuf == 1, "expected 1 got %d\n", ofs.base_ios.delbuf);
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pfb->base.base);
    ok(pfb->base.ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), pfb->base.ebuf);
    ok(pfb->fd == 53, "wrong fd, expected 53 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_ofstream_dtor, &ofs.base_ios);

    pofs = call_func5(p_ofstream_buffer_ctor, &ofs, 64, NULL, 0, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(ofs.base_ios.delbuf == 1, "expected 1 got %d\n", ofs.base_ios.delbuf);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 64, "wrong fd, expected 64 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    pofs = call_func5(p_ofstream_open_ctor, &ofs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(ofs.base_ios.delbuf == 1, "expected 1 got %d\n", ofs.base_ios.delbuf);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    ok(pfb->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base != NULL, "wrong buffer, expected not %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf != NULL, "wrong ebuf, expected not %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd != -1, "wrong fd, expected not -1 got %d\n", pfb->fd);
    fd = pfb->fd;
    ok(pfb->close == 1, "wrong value, expected 1 got %d\n", pfb->close);
    call_func1(p_ofstream_vbase_dtor, &ofs);
    ok(_close(fd) == -1, "expected ofstream to close opened file\n");
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    /* setbuf */
    call_func5(p_ofstream_buffer_ctor, &ofs, -1, NULL, 0, TRUE);
    ok(ofs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);

    psb = call_func3(p_ofstream_setbuf, &ofs, buffer, ARRAY_SIZE(buffer));
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, NULL, 0);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    call_func2(p_ofstream_ctor, &ofs, TRUE);
    ok(ofs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, buffer, ARRAY_SIZE(buffer));
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, NULL, 0);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, buffer + 8, ARRAY_SIZE(buffer) - 8);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, buffer + 8, 0);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, buffer + 4, ARRAY_SIZE(buffer) - 4);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    psb = call_func3(p_ofstream_setbuf, &ofs, NULL, 5);
    ok(psb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, psb);
    ok(ofs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    /* setbuf - seems to be a nop and always return NULL in those other cases */
    pofs = call_func5(p_ofstream_buffer_ctor, &ofs, 42, NULL, 0, TRUE);
    ok(ofs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);

    ofs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ofstream_setbuf, &ofs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ofs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    ofs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ofstream_setbuf, &ofs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ofs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_ofstream_vbase_dtor, &ofs);

    pofs = call_func5(p_ofstream_open_ctor, &ofs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    ofs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ofstream_setbuf, &ofs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ofs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->ebuf != NULL, "wrong ebuf value, expected NULL got %p\n", ofs.base_ios.sb->ebuf);
    ok(ofs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    ofs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ofstream_setbuf, &ofs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ofs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->base != buffer, "wrong base value, expected not %p got %p\n", buffer, ofs.base_ios.sb->base);
    ok(ofs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ofs.base_ios.sb->unbuffered);
    ok(ofs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", ofs.base_ios.sb->allocated);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_ofstream_vbase_dtor, &ofs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    /* attach */
    pofs = call_func2(p_ofstream_ctor, &ofs, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    call_func2(p_ofstream_attach, &ofs, 42);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "attaching on vanilla stream set some state bits\n");
    fd = (int) call_func1(p_ofstream_fd, &ofs);
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    ok(pfb->close == 0, "wrong close value, expected 0 got %d\n", pfb->close);
    ofs.base_ios.state = IOSTATE_eofbit;
    call_func2(p_ofstream_attach, &ofs, 53);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    /* fd */
    pofs = call_func2(p_ofstream_ctor, &ofs, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    fd = (int) call_func1(p_ofstream_fd, &ofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(fd == -1, "wrong fd, expected -1 but got %d\n", fd);
    call_func1(p_ofstream_vbase_dtor, &ofs);

    pofs = call_func5(p_ofstream_open_ctor, &ofs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    pfb = (filebuf*) ofs.base_ios.sb;
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    fd = (int) call_func1(p_ofstream_fd, &ofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ok(fd == pfb->fd, "wrong fd, expected %d but got %d\n", pfb->fd, fd);

    /* rdbuf */
    pfb = (filebuf*) call_func1(p_ofstream_rdbuf, &ofs);
    ok((streambuf*) pfb == ofs.base_ios.sb, "wrong return, expected %p got %p\n", ofs.base_ios.sb, pfb);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    /* setmode */
    ret = (int) call_func2(p_ofstream_setmode, &ofs, filebuf_binary);
    ok(ret == filebuf_text, "wrong return, expected %d got %d\n", filebuf_text, ret);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ret = (int) call_func2(p_ofstream_setmode, &ofs, filebuf_binary);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ret = (int) call_func2(p_ofstream_setmode, &ofs, filebuf_text);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);
    ret = (int) call_func2(p_ofstream_setmode, &ofs, 0x9000);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ofs.base_ios.state);

    /* close && is_open */
    ok((int) call_func1(p_ofstream_is_open, &ofs) == 1, "expected ofstream to be open\n");
    ofs.base_ios.state = IOSTATE_eofbit | IOSTATE_failbit;
    call_func1(p_ofstream_close, &ofs);
    ok(ofs.base_ios.state == IOSTATE_goodbit, "close did not clear state = %d\n", ofs.base_ios.state);
    ofs.base_ios.state = IOSTATE_eofbit;
    call_func1(p_ofstream_close, &ofs);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "close on a closed stream did not set failbit\n");
    ok((int) call_func1(p_ofstream_is_open, &ofs) == 0, "expected ofstream to not be open\n");
    ok(_close(fd) == -1, "expected close to close the opened file\n");

    /* open */
    ofs.base_ios.state = IOSTATE_eofbit;
    call_func4(p_ofstream_open, &ofs, filename, OPENMODE_out, filebuf_openprot);
    fd = (int) call_func1(p_ofstream_fd, &ofs);
    ok(fd != -1, "wrong fd, expected not -1 got %d\n", fd);
    ok(ofs.base_ios.state == IOSTATE_eofbit, "open did not succeed\n");
    call_func4(p_ofstream_open, &ofs, filename, OPENMODE_out, filebuf_openprot);
    ok(ofs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "second open did not set failbit\n");
    call_func1(p_ofstream_close, &ofs);

    /* integration with parent ostream - writing */
    ofs.base_ios.state = IOSTATE_goodbit; /* open doesn't clear the state */
    call_func4(p_ofstream_open, &ofs, filename, OPENMODE_in, filebuf_openprot); /* make sure that OPENMODE_out is implicit */
    pofs = call_func2(p_ostream_print_str, &ofs, "test ");
    ok(pofs == &ofs, "stream operation returned wrong pointer, expected %p got %p\n", &ofs, pofs);
    pofs = call_func2(p_ostream_print_int, &ofs, 12);
    ok(pofs == &ofs, "stream operation returned wrong pointer, expected %p got %p\n", &ofs, pofs);
    call_func1(p_ofstream_close, &ofs);

    /* read what we wrote */
    memset(st, 'A', sizeof(st));
    st[7] = 0;
    fd = _open(filename, _O_RDONLY, _S_IREAD);
    ok(fd != -1, "_open failed\n");
    ok(_read(fd, st, 7) == 7, "_read failed\n");
    ok(_close(fd) == 0, "_close failed\n");
    ok(!strcmp(st, "test 12"), "expected 'test 12' got '%s'\n", st);
    call_func1(p_ofstream_vbase_dtor, &ofs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    /* make sure that OPENMODE_out is implicit with open_ctor */
    pofs = call_func5(p_ofstream_open_ctor, &ofs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    ok(pofs == &ofs, "wrong return, expected %p got %p\n", &ofs, pofs);
    pofs = call_func2(p_ostream_print_str, &ofs, "test ");
    ok(pofs == &ofs, "stream operation returned wrong pointer, expected %p got %p\n", &ofs, pofs);
    pofs = call_func2(p_ostream_print_int, &ofs, 12);
    ok(pofs == &ofs, "stream operation returned wrong pointer, expected %p got %p\n", &ofs, pofs);
    call_func1(p_ofstream_close, &ofs);

    memset(st, 'A', sizeof(st));
    st[7] = 0;
    fd = _open(filename, _O_RDONLY, _S_IREAD);
    ok(fd != -1, "_open failed\n");
    ok(_read(fd, st, 7) == 7, "_read failed\n");
    ok(_close(fd) == 0, "_close failed\n");
    ok(!strcmp(st, "test 12"), "expected 'test 12' got '%s'\n", st);
    call_func1(p_ofstream_vbase_dtor, &ofs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);
}

static void test_istream(void)
{
    istream is1, is2, *pis;
    ostream os, *pos;
    filebuf fb1, fb2, *pfb;
    const char filename1[] = "test1";
    const char filename2[] = "test2";
    int fd, ret;
    char buffer[32], c;

    memset(&is1, 0xab, sizeof(istream));
    memset(&is2, 0xab, sizeof(istream));
    memset(&fb1, 0xab, sizeof(filebuf));
    memset(&fb2, 0xab, sizeof(filebuf));

    /* constructors/destructors */
    pis = call_func3(p_istream_sb_ctor, &is1, NULL, TRUE);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb == NULL, "expected %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    ok(is1.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is1.base_ios.flags);
    call_func1(p_istream_vbase_dtor, &is1);
    is1.extract_delim = is1.count = 0xabababab;
    memset(&is1.base_ios, 0xab, sizeof(ios));
    is1.base_ios.delbuf = 0;
    pis = call_func3(p_istream_sb_ctor, &is1, NULL, FALSE);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb == NULL, "expected %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, is1.base_ios.state);
    ok(is1.base_ios.flags == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.flags);
    call_func1(p_istream_dtor, &is1.base_ios);
    pis = call_func3(p_istream_sb_ctor, &is1, &fb1.base, FALSE);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb == &fb1.base, "expected %p got %p\n", &fb1.base, is1.base_ios.sb);
    ok(is1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.state);
    ok(is1.base_ios.flags == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.flags);
    call_func1(p_istream_dtor, &is1.base_ios);
    call_func1(p_filebuf_ctor, &fb1);
    pfb = call_func4(p_filebuf_open, &fb1, filename1, OPENMODE_in|OPENMODE_out, filebuf_openprot);
    ok(pfb == &fb1, "wrong return, expected %p got %p\n", &fb1, pfb);
    ok(fb1.base.allocated == 1, "expected %d got %d\n", 1, fb1.base.allocated);
    pis = call_func3(p_istream_sb_ctor, &is1, &fb1.base, TRUE);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb == &fb1.base, "expected %p got %p\n", &fb1.base, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(is1.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is1.base_ios.flags);
    pis = call_func2(p_istream_ctor, &is2, TRUE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == NULL, "expected %p got %p\n", NULL, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is2.base_ios.state);
    ok(is2.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is2.base_ios.flags);
    call_func1(p_istream_vbase_dtor, &is2);
    is2.extract_delim = is2.count = 0xabababab;
    memset(&is2.base_ios, 0xab, sizeof(ios));
    is2.base_ios.flags &= ~FLAGS_skipws;
    pis = call_func2(p_istream_ctor, &is2, FALSE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is2.base_ios.sb);
    ok(is2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.state);
    ok(is2.base_ios.flags == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.flags);
    call_func1(p_istream_dtor, &is2.base_ios);
    is1.extract_delim = is1.count = 0xcdcdcdcd;
    is1.base_ios.state = 0xcdcdcdcd;
    is1.base_ios.flags &= ~FLAGS_skipws;
    is2.extract_delim = is2.count = 0xabababab;
    memset(&is2.base_ios, 0xab, sizeof(ios));
    is2.base_ios.flags &= ~FLAGS_skipws;
    is2.base_ios.delbuf = 0;
    pis = call_func3(p_istream_copy_ctor, &is2, &is1, FALSE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.state);
    ok(is2.base_ios.flags == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.flags);
    call_func1(p_istream_dtor, &is2.base_ios);
    is2.extract_delim = is2.count = 0xabababab;
    memset(&is2.base_ios, 0xab, sizeof(ios));
    pis = call_func3(p_istream_copy_ctor, &is2, &is1, TRUE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is2.base_ios.state);
    ok(is2.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is2.base_ios.flags);

    /* assignment */
    is2.extract_delim = is2.count = 0xabababab;
    is2.base_ios.sb = (streambuf*) 0xabababab;
    is2.base_ios.state = 0xabababab;
    is2.base_ios.special[0] = 0xabababab;
    is2.base_ios.delbuf = 0;
    is2.base_ios.tie = (ostream*) 0xabababab;
    is2.base_ios.flags = 0xabababab;
    is2.base_ios.precision = 0xabababab;
    is2.base_ios.width = 0xabababab;
    pis = call_func2(p_istream_assign, &is2, &is1);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is2.base_ios.state);
    ok(is2.base_ios.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.fill);
    ok(is2.base_ios.delbuf == 0, "expected 0 got %d\n", is2.base_ios.delbuf);
    ok(is2.base_ios.tie == NULL, "expected %p got %p\n", NULL, is2.base_ios.tie);
    ok(is2.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is2.base_ios.flags);
    ok(is2.base_ios.precision == 6, "expected 6 got %d\n", is2.base_ios.precision);
    ok(is2.base_ios.width == 0, "expected 0 got %d\n", is2.base_ios.width);
    if (0) /* crashes on native */
        pis = call_func2(p_istream_assign, &is2, NULL);
    is2.extract_delim = is2.count = 0xabababab;
    is2.base_ios.sb = (streambuf*) 0xabababab;
    is2.base_ios.state = 0xabababab;
    is2.base_ios.special[0] = 0xabababab;
    is2.base_ios.delbuf = 0;
    is2.base_ios.tie = (ostream*) 0xabababab;
    is2.base_ios.flags = 0xabababab;
    is2.base_ios.precision = 0xabababab;
    is2.base_ios.width = 0xabababab;
    pis = call_func2(p_istream_assign_sb, &is2, NULL);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == NULL, "expected %p got %p\n", NULL, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is2.base_ios.state);
    ok(is2.base_ios.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.fill);
    ok(is2.base_ios.delbuf == 0, "expected 0 got %d\n", is2.base_ios.delbuf);
    ok(is2.base_ios.tie == NULL, "expected %p got %p\n", NULL, is2.base_ios.tie);
    ok(is2.base_ios.flags == FLAGS_skipws, "expected %d got %d\n", FLAGS_skipws, is2.base_ios.flags);
    ok(is2.base_ios.precision == 6, "expected 6 got %d\n", is2.base_ios.precision);
    ok(is2.base_ios.width == 0, "expected 0 got %d\n", is2.base_ios.width);
    call_func1(p_filebuf_ctor, &fb2);
    pfb = call_func4(p_filebuf_open, &fb2, filename2, OPENMODE_in|OPENMODE_out, filebuf_openprot);
    ok(pfb == &fb2, "wrong return, expected %p got %p\n", &fb2, pfb);
    ok(fb2.base.allocated == 1, "expected %d got %d\n", 1, fb2.base.allocated);
    pis = call_func2(p_istream_assign_sb, &is2, &fb2.base);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == &fb2.base, "expected %p got %p\n", &fb2.base, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is2.base_ios.state);

    /* eatwhite */
    is1.extract_delim = is1.count = 0;
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    is1.base_ios.state = IOSTATE_badbit;
    is1.base_ios.flags = 0;
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_eofbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_eofbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_failbit;
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == (IOSTATE_failbit|IOSTATE_eofbit), "expected %d got %d\n",
        IOSTATE_failbit|IOSTATE_eofbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func3(p_streambuf_xsputn, &fb1.base, "And if \tyou ask\n\v\f\r  me", 23);
    ok(ret == 23, "expected 23 got %d\n", ret);
    ok(fb1.base.pbase == fb1.base.base, "wrong put base, expected %p got %p\n", fb1.base.base, fb1.base.pbase);
    ok(fb1.base.pptr == fb1.base.base + 23, "wrong put pointer, expected %p got %p\n", fb1.base.base + 23, fb1.base.pptr);
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb1.base.pbase);
    ok(fb1.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb1.base.pptr);
    ok(fb1.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb1.base.epptr);
    is1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 0, SEEKDIR_beg, 0);
    ok(ret == 0, "expected 0 got %d\n", ret);
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 23, "wrong get end, expected %p got %p\n", fb1.base.base + 23, fb1.base.egptr);
    ret = (int) call_func4(p_filebuf_seekoff, &fb1, 3, SEEKDIR_beg, 0);
    ok(ret == 3, "expected 3 got %d\n", ret);
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 1, "wrong get pointer, expected %p got %p\n", fb1.base.base + 1, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 20, "wrong get end, expected %p got %p\n", fb1.base.base + 20, fb1.base.egptr);
    fb1.base.gptr += 2;
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 20, "wrong get end, expected %p got %p\n", fb1.base.base + 20, fb1.base.egptr);
    fb1.base.gptr += 7;
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 18, "wrong get pointer, expected %p got %p\n", fb1.base.base + 18, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 20, "wrong get end, expected %p got %p\n", fb1.base.base + 20, fb1.base.egptr);
    fb1.base.gptr += 9;
    call_func1(p_istream_eatwhite, &is1);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);

    /* ipfx */
    is1.count = 0xabababab;
    ret = (int) call_func2(p_istream_ipfx, &is1, 0);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.count == 0xabababab, "expected %d got %d\n", 0xabababab, is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func2(p_istream_ipfx, &is1, 1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    is1.base_ios.state = IOSTATE_goodbit;
    is1.base_ios.tie = &os;
    pos = call_func3(p_ostream_sb_ctor, &os, &fb2.base, TRUE);
    ok(pos == &os, "wrong return, expected %p got %p\n", &os, pos);
    ret = (int) call_func3(p_streambuf_xsputn, &fb2.base, "how I'm feeling", 15);
    ok(ret == 15, "expected 15 got %d\n", ret);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 15, "wrong put pointer, expected %p got %p\n", fb2.base.base + 15, fb2.base.pptr);
    is1.count = -1;
    ret = (int) call_func2(p_istream_ipfx, &is1, 0);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.count == -1, "expected -1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ret = (int) call_func3(p_streambuf_xsputn, &fb2.base, "Don't tell me", 13);
    ok(ret == 13, "expected 13 got %d\n", ret);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 13, "wrong put pointer, expected %p got %p\n", fb2.base.base + 13, fb2.base.pptr);
    ret = (int) call_func2(p_istream_ipfx, &is1, 1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ret = (int) call_func3(p_streambuf_xsputn, &fb2.base, "you're too blind to see", 23);
    ok(ret == 23, "expected 23 got %d\n", ret);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 23, "wrong put pointer, expected %p got %p\n", fb2.base.base + 23, fb2.base.pptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 15;
    strcpy(fb1.base.eback, "Never \t  gonna ");
    ret = (int) call_func2(p_istream_ipfx, &is1, 1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 23, "wrong put pointer, expected %p got %p\n", fb2.base.base + 23, fb2.base.pptr);
    fb1.base.gptr = fb1.base.base + 4;
    is1.count = 10;
    ret = (int) call_func2(p_istream_ipfx, &is1, 11);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 23, "wrong put pointer, expected %p got %p\n", fb2.base.base + 23, fb2.base.pptr);
    fd = fb2.fd;
    fb2.fd = -1;
    ret = (int) call_func2(p_istream_ipfx, &is1, 12);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(os.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, os.base_ios.state);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 23, "wrong put pointer, expected %p got %p\n", fb2.base.base + 23, fb2.base.pptr);
    os.base_ios.state = IOSTATE_goodbit;
    fb2.fd = fd;
    ret = (int) call_func2(p_istream_ipfx, &is1, 12);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(os.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, os.base_ios.state);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    is1.base_ios.flags = 0;
    fb1.base.gptr = fb1.base.base + 5;
    ret = (int) call_func2(p_istream_ipfx, &is1, 0);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    ret = (int) call_func2(p_istream_ipfx, &is1, 1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    is1.base_ios.flags = FLAGS_skipws;
    ret = (int) call_func2(p_istream_ipfx, &is1, 1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    ret = (int) call_func2(p_istream_ipfx, &is1, -1);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    ret = (int) call_func2(p_istream_ipfx, &is1, 0);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 9, "wrong get pointer, expected %p got %p\n", fb1.base.base + 9, fb1.base.gptr);
    fb1.base.gptr = fb1.base.base + 14;
    fb2.base.pbase = fb2.base.base;
    fb2.base.pptr = fb2.base.epptr = fb2.base.base + 23;
    ret = (int) call_func2(p_istream_ipfx, &is1, 0);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);

    /* get_str_delim */
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_badbit;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 30, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_goodbit;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 30, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_goodbit;
    is1.base_ios.flags = 0;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 0, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(buffer[0] == 'A', "expected 65 got %d\n", buffer[0]);
    is1.extract_delim = is1.count = 0xabababab;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    strcpy(fb1.base.base, "   give  \n you 11 ! up\t.      ");
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 30, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 29, "expected 29 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 29, "wrong get pointer, expected %p got %p\n", fb1.base.base + 29, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 29), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[29] == 0, "expected 0 got %d\n", buffer[29]);
    fb1.base.gptr = fb1.base.egptr;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 1, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.base_ios.flags = FLAGS_skipws;
    fb1.base.gptr = fb1.base.base;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 19, "expected 19 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 19, "wrong get pointer, expected %p got %p\n", fb1.base.base + 19, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 19), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[19] == 0, "expected 0 got %d\n", buffer[19]);
    fb1.base.gptr = fb1.base.base + 20;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 10, "expected 10 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 20, 10), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[10] == 0, "expected 0 got %d\n", buffer[10]);
    is1.extract_delim = 1;
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.base;
    fb1.base.gptr = fb1.base.base + 20;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, -1);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 10, "expected 10 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 20, 10), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[10] == 0, "expected 0 got %d\n", buffer[10]);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 9, "expected 9 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 9, "wrong get pointer, expected %p got %p\n", fb1.base.base + 9, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 9), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[9] == 0, "expected 0 got %d\n", buffer[9]);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 9, "wrong get pointer, expected %p got %p\n", fb1.base.base + 9, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = 0xabababab;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 10, "wrong get pointer, expected %p got %p\n", fb1.base.base + 10, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    *fb1.base.gptr = -50;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 2, -50);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 11, "wrong get pointer, expected %p got %p\n", fb1.base.base + 11, fb1.base.gptr);
    ok((signed char)buffer[0] == -50, "expected 0 got %d\n", buffer[0]);
    ok(buffer[1] == 0, "expected 0 got %d\n", buffer[1]);
    *fb1.base.gptr = -50;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 2, 206);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 11, "wrong get pointer, expected %p got %p\n", fb1.base.base + 11, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = 3;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 2, 206);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 12, "wrong get pointer, expected %p got %p\n", fb1.base.base + 12, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 20, '!');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 6, "expected 6 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 18, "wrong get pointer, expected %p got %p\n", fb1.base.base + 18, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 12, 6), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[6] == 0, "expected 0 got %d\n", buffer[6]);
    pis = call_func4(p_istream_get_str_delim, &is1, NULL, 5, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 4, "expected 4 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 22, "wrong get pointer, expected %p got %p\n", fb1.base.base + 22, fb1.base.gptr);
    fb1.base.gptr = fb1.base.egptr;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str_delim, &is1, buffer, 10, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    if (0) /* crashes on native */
        pis = call_func4(p_istream_get_str_delim, &is1, (char*) 0x1, 5, 0);

    /* get_str */
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_eofbit;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str, &is1, buffer, 10, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str, &is1, buffer, 20, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 19, "expected 19 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 19, "wrong get pointer, expected %p got %p\n", fb1.base.base + 19, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 19), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[19] == 0, "expected 0 got %d\n", buffer[19]);
    is1.extract_delim = -1;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str, &is1, buffer, 20, '\t');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 4, "expected 4 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 23, "wrong get pointer, expected %p got %p\n", fb1.base.base + 23, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 19, 3), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[3] == 0, "expected 0 got %d\n", buffer[3]);
    *fb1.base.gptr = -50;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str, &is1, buffer, 5, -50);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 23, "wrong get pointer, expected %p got %p\n", fb1.base.base + 23, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    *(fb1.base.gptr + 1) = -40;
    *(fb1.base.gptr + 2) = -30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_get_str, &is1, buffer, 5, -30);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 2, "expected 2 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 25, "wrong get pointer, expected %p got %p\n", fb1.base.base + 25, fb1.base.gptr);
    ok((signed char)buffer[0] == -50, "expected -50 got %d\n", buffer[0]);
    ok((signed char)buffer[1] == -40, "expected -40 got %d\n", buffer[1]);
    ok(buffer[2] == 0, "expected 0 got %d\n", buffer[2]);

    /* get */
    is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_eofbit;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.egptr = NULL;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == ' ', "expected %d got %d\n", ' ', ret);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 1, "wrong get pointer, expected %p got %p\n", fb1.base.base + 1, fb1.base.gptr);
    *fb1.base.gptr = '\n';
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == '\n', "expected %d got %d\n", '\n', ret);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 2, "wrong get pointer, expected %p got %p\n", fb1.base.base + 2, fb1.base.gptr);
    *fb1.base.gptr = -50;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 3, "wrong get pointer, expected %p got %p\n", fb1.base.base + 3, fb1.base.gptr);
    fb1.base.gptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_get, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.gptr == (char*) 1, "wrong get pointer, expected %p got %p\n", (char*) 1, fb1.base.gptr);

    /* get_char */
    is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_badbit;
    c = 0xab;
    pis = call_func2(p_istream_get_char, &is1, &c);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(c == (char) 0xab, "expected %d got %d\n", (char) 0xab, c);
    is1.base_ios.state = IOSTATE_goodbit;
    pis = call_func2(p_istream_get_char, &is1, &c);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok((signed char)c == EOF, "expected -1 got %d\n", c);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func2(p_istream_get_char, &is1, &c);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 1, "wrong get pointer, expected %p got %p\n", fb1.base.base + 1, fb1.base.gptr);
    ok(c == ' ', "expected %d got %d\n", ' ', c);
    fb1.base.gptr = fb1.base.base + 2;
    pis = call_func2(p_istream_get_char, &is1, &c);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 3, "wrong get pointer, expected %p got %p\n", fb1.base.base + 3, fb1.base.gptr);
    ok((signed char)c == -50, "expected %d got %d\n", -50, c);
    if (0) /* crashes on native */
        pis = call_func2(p_istream_get_char, &is1, NULL);
    fb1.base.gptr = fb1.base.base + 30;
    pis = call_func2(p_istream_get_char, &is1, &c);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == (char*) 1, "wrong get pointer, expected %p got %p\n", (char*) 1, fb1.base.gptr);
    ok((signed char)c == EOF, "expected -1 got %d\n", c);
    is1.base_ios.state = IOSTATE_failbit;
    pis = call_func2(p_istream_get_char, &is1, NULL);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == (char*) 1, "wrong get pointer, expected %p got %p\n", (char*) 1, fb1.base.gptr);

    /* get_sb */
    is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_badbit;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    strcpy(fb1.base.base, "  Never gonna \nlet you \r down?");
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 30, "expected 30 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 30, "wrong put pointer, expected %p got %p\n", fb2.base.base + 30, fb2.base.pptr);
    ok(fb2.base.epptr == fb2.base.ebuf, "wrong put end, expected %p got %p\n", fb2.base.ebuf, fb2.base.epptr);
    ok(!strncmp(fb2.base.pbase, "  Never gonna \nlet you \r down?", 30), "unexpected sb content, got '%s'\n", fb2.base.pbase);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 14, "expected 14 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 30, "wrong get end, expected %p got %p\n", fb1.base.base + 30, fb1.base.egptr);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 44, "wrong put pointer, expected %p got %p\n", fb2.base.base + 44, fb2.base.pptr);
    ok(fb2.base.epptr == fb2.base.ebuf, "wrong put end, expected %p got %p\n", fb2.base.ebuf, fb2.base.epptr);
    ok(!strncmp(fb2.base.base + 30, "  Never gonna ", 14), "unexpected sb content, got '%s'\n", fb2.base.pbase);
    fd = fb2.fd;
    fb2.fd = -1;
    fb2.base.pbase = fb2.base.pptr = fb2.base.epptr = NULL;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 30, "wrong get end, expected %p got %p\n", fb1.base.base + 30, fb1.base.egptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ok(fb2.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb2.base.epptr);
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 16, "expected 16 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ok(fb2.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb2.base.epptr);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 14, "expected 14 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 30, "wrong get end, expected %p got %p\n", fb1.base.base + 30, fb1.base.egptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ok(fb2.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb2.base.epptr);
    fb2.fd = fd;
    is1.base_ios.state = IOSTATE_goodbit;
    pis = call_func3(p_istream_get_sb, &is1, NULL, '\n');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.eback == fb1.base.base, "wrong get base, expected %p got %p\n", fb1.base.base, fb1.base.eback);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(fb1.base.egptr == fb1.base.base + 30, "wrong get end, expected %p got %p\n", fb1.base.base + 30, fb1.base.egptr);
    ok(fb2.base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, fb2.base.pbase);
    ok(fb2.base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, fb2.base.pptr);
    ok(fb2.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb2.base.epptr);
    is1.base_ios.state = IOSTATE_goodbit;
    if (0) /* crashes on native */
        pis = call_func3(p_istream_get_sb, &is1, NULL, '?');
    *fb1.base.gptr = -50;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, -50);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 16, "expected 16 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 16, "wrong put pointer, expected %p got %p\n", fb2.base.base + 16, fb2.base.pptr);
    ok(fb2.base.epptr == fb2.base.ebuf, "wrong put end, expected %p got %p\n", fb2.base.ebuf, fb2.base.epptr);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func3(p_istream_get_sb, &is1, &fb2.base, (char)206);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 30, "expected 30 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, fb1.base.eback);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.egptr == NULL, "wrong get end, expected %p got %p\n", NULL, fb1.base.egptr);
    ok(fb2.base.pbase == fb2.base.base, "wrong put base, expected %p got %p\n", fb2.base.base, fb2.base.pbase);
    ok(fb2.base.pptr == fb2.base.base + 46, "wrong put pointer, expected %p got %p\n", fb2.base.base + 46, fb2.base.pptr);
    ok(fb2.base.epptr == fb2.base.ebuf, "wrong put end, expected %p got %p\n", fb2.base.ebuf, fb2.base.epptr);

    /* getline */
    is1.extract_delim = is1.count = 0xabababab;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 10, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_goodbit;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 10, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 10, 'r');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 7, "expected 7 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 7, "wrong get pointer, expected %p got %p\n", fb1.base.base + 7, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 6), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[6] == 0, "expected 0 got %d\n", buffer[6]);
    is1.extract_delim = -1;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 10, -50);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 7, "expected 7 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 7, 7), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[7] == 0, "expected 0 got %d\n", buffer[7]);
    is1.extract_delim = -1;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 10, (char)206);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok(buffer[0] == 0, "expected 0 got %d\n", buffer[0]);
    is1.extract_delim = -2;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 20, '\r');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 10, "expected 10 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 24, "wrong get pointer, expected %p got %p\n", fb1.base.base + 24, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 14, 9), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[9] == 0, "expected 0 got %d\n", buffer[9]);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, NULL, 20, '?');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 6, "expected 6 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func4(p_istream_getline, &is1, buffer, 0, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    ok(buffer[0] == 'A', "expected 'A' got %d\n", buffer[0]);

    /* ignore */
    is1.count = is1.extract_delim = 0xabababab;
    is1.base_ios.state = IOSTATE_badbit;
    pis = call_func3(p_istream_ignore, &is1, 10, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    is1.count = is1.extract_delim = 0xabababab;
    is1.base_ios.state = IOSTATE_goodbit;
    pis = call_func3(p_istream_ignore, &is1, 0, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    pis = call_func3(p_istream_ignore, &is1, 1, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func3(p_istream_ignore, &is1, 5, -1);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 5, "expected 5 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 5, "wrong get pointer, expected %p got %p\n", fb1.base.base + 5, fb1.base.gptr);
    is1.extract_delim = 0;
    pis = call_func3(p_istream_ignore, &is1, 10, 'g');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 4, "expected 4 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 9, "wrong get pointer, expected %p got %p\n", fb1.base.base + 9, fb1.base.gptr);
    is1.extract_delim = -1;
    pis = call_func3(p_istream_ignore, &is1, 10, 'a');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 3, "expected 3 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 12, "wrong get pointer, expected %p got %p\n", fb1.base.base + 12, fb1.base.gptr);
    is1.extract_delim = -1;
    pis = call_func3(p_istream_ignore, &is1, 10, 206);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 2, "expected 2 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    pis = call_func3(p_istream_ignore, &is1, 10, -50);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 10, "expected 10 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 24, "wrong get pointer, expected %p got %p\n", fb1.base.base + 24, fb1.base.gptr);
    is1.extract_delim = -1;
    pis = call_func3(p_istream_ignore, &is1, 10, -1);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 6, "expected 6 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);

    /* peek */
    is1.count = 0xabababab;
    ret = (int) call_func1(p_istream_peek, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func1(p_istream_peek, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_peek, &is1);
    ok(ret == ' ', "expected ' ' got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    fb1.base.gptr = fb1.base.base + 14;
    ret = (int) call_func1(p_istream_peek, &is1);
    ok(ret == 206, "expected 206 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    fb1.base.gptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_peek, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);

    /* putback */
    is1.base_ios.state = IOSTATE_goodbit;
    pis = call_func2(p_istream_putback, &is1, 'a');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    fd = fb1.fd;
    fb1.fd = -1;
    pis = call_func2(p_istream_putback, &is1, 'a');
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    fb1.fd = fd;
    fb1.base.eback = fb1.base.base;
    fb1.base.gptr = fb1.base.base + 15;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func2(p_istream_putback, &is1, -40);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 15, "wrong get pointer, expected %p got %p\n", fb1.base.base + 15, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_badbit;
    pis = call_func2(p_istream_putback, &is1, -40);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 15, "wrong get pointer, expected %p got %p\n", fb1.base.base + 15, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_eofbit;
    pis = call_func2(p_istream_putback, &is1, -40);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_eofbit, "expected %d got %d\n", IOSTATE_eofbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 15, "wrong get pointer, expected %p got %p\n", fb1.base.base + 15, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_goodbit;
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    pis = call_func2(p_istream_putback, &is1, -40);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 14, "wrong get pointer, expected %p got %p\n", fb1.base.base + 14, fb1.base.gptr);
    ok((signed char)*fb1.base.gptr == -40, "expected -40 got %d\n", *fb1.base.gptr);

    /* read */
    is1.extract_delim = is1.count = 0xabababab;
    is1.base_ios.state = IOSTATE_badbit;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func3(p_istream_read, &is1, buffer, 10);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(buffer[0] == 'A', "expected 'A' got %d\n", buffer[0]);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.gptr = fb1.base.base;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func3(p_istream_read, &is1, buffer, 10);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 10, "expected 10 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 10, "wrong get pointer, expected %p got %p\n", fb1.base.base + 10, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 10), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[10] == 'A', "expected 'A' got %d\n", buffer[10]);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func3(p_istream_read, &is1, buffer, 20);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 20, "expected 20 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base + 30, "wrong get pointer, expected %p got %p\n", fb1.base.base + 30, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base + 10, 20), "unexpected buffer content, got '%s'\n", buffer);
    ok((signed char)buffer[4] == -40, "expected -40 got %d\n", buffer[4]);
    ok(buffer[20] == 'A', "expected 'A' got %d\n", buffer[20]);
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func3(p_istream_read, &is1, buffer, 5);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 1, "expected 1 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(buffer[0] == 'e', "expected 'e' got %d\n", buffer[0]);
    ok(buffer[1] == 'A', "expected 'A' got %d\n", buffer[1]);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    pis = call_func3(p_istream_read, &is1, buffer, 35);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 30, "expected 30 got %d\n", is1.count);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(!strncmp(buffer, fb1.base.base, 30), "unexpected buffer content, got '%s'\n", buffer);
    ok(buffer[30] == 'A', "expected 'A' got %d\n", buffer[30]);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    memset(buffer, 'A', sizeof(buffer));
    if (0) /* crashes on native */
        pis = call_func3(p_istream_read, &is1, buffer, -1);
    pis = call_func3(p_istream_read, &is1, buffer, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    ok(buffer[0] == 'A', "expected 'A' got %d\n", buffer[0]);
    fb1.base.gptr = fb1.base.egptr;

    /* seekg */
    is1.extract_delim = is1.count = 0xabababab;
    pis = call_func2(p_istream_seekg, &is1, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(_tell(fb1.fd) == 0, "expected 0 got %ld\n", _tell(fb1.fd));
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    pis = call_func2(p_istream_seekg, &is1, -5);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(_tell(fb1.fd) == 0, "expected 0 got %ld\n", _tell(fb1.fd));
    fb1.base.epptr = fb1.base.ebuf;
    pis = call_func2(p_istream_seekg, &is1, 5);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(fb1.base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, fb1.base.epptr);
    ok(_tell(fb1.fd) == 5, "expected 5 got %ld\n", _tell(fb1.fd));
    is1.base_ios.state = IOSTATE_goodbit;
    fd = fb1.fd;
    fb1.fd = -1;
    pis = call_func2(p_istream_seekg, &is1, 0);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    pis = call_func3(p_istream_seekg_offset, &is1, 0, SEEKDIR_end);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_goodbit;
    fb1.fd = fd;
    pis = call_func3(p_istream_seekg_offset, &is1, 0, SEEKDIR_end);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    fb1.base.gptr = fb1.base.egptr;
    pis = call_func3(p_istream_seekg_offset, &is1, 0, SEEKDIR_end);
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    ok(_tell(fb1.fd) == 24, "expected 24 got %ld\n", _tell(fb1.fd));

    /* sync */
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_badbit;
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == fb1.base.base, "wrong get pointer, expected %p got %p\n", fb1.base.base, fb1.base.gptr);
    fb1.base.gptr = fb1.base.egptr;
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit, is1.base_ios.state);
    ok(fb1.base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, fb1.base.gptr);
    is1.base_ios.state = IOSTATE_eofbit;
    fd = fb1.fd;
    fb1.fd = -1;
    ret = (int) call_func1(p_istream_sync, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit, is1.base_ios.state);
    fb1.fd = fd;

    /* tellg */
    ret = (int) call_func1(p_istream_tellg, &is1);
    ok(ret == 24, "expected 24 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit), "expected %d got %d\n",
        IOSTATE_badbit|IOSTATE_failbit|IOSTATE_eofbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_goodbit;
    ret = (int) call_func1(p_istream_tellg, &is1);
    ok(ret == 24, "expected 24 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    if (0) /* crashes on native */
        is1.base_ios.sb = NULL;
    fb1.base.eback = fb1.base.gptr = fb1.base.base;
    fb1.base.egptr = fb1.base.base + 30;
    ret = (int) call_func1(p_istream_tellg, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.base_ios.state == IOSTATE_failbit, "expected %d got %d\n", IOSTATE_failbit, is1.base_ios.state);
    is1.base_ios.state = IOSTATE_eofbit;
    ret = (int) call_func1(p_istream_tellg, &is1);
    ok(ret == EOF, "expected -1 got %d\n", ret);
    ok(is1.base_ios.state == (IOSTATE_eofbit|IOSTATE_failbit), "expected %d got %d\n",
        IOSTATE_eofbit|IOSTATE_failbit, is1.base_ios.state);
    fb1.base.eback = fb1.base.gptr = fb1.base.egptr = NULL;

    call_func1(p_istream_vbase_dtor, &is1);
    call_func1(p_istream_vbase_dtor, &is2);
    call_func1(p_ostream_vbase_dtor, &os);
    call_func1(p_filebuf_dtor, &fb1);
    call_func1(p_filebuf_dtor, &fb2);
    ok(_unlink(filename1) == 0, "Couldn't unlink file named '%s'\n", filename1);
    ok(_unlink(filename2) == 0, "Couldn't unlink file named '%s'\n", filename2);
}

static void test_istream_getint(void)
{
    istream is, *pis;
    strstreambuf ssb, *pssb;
    int i, len, ret;
    char buffer[32];

    struct istream_getint_test {
        const char *stream_content;
        ios_io_state initial_state;
        ios_flags flags;
        int expected_return;
        ios_io_state expected_state;
        int expected_offset;
        const char *expected_buffer;
    } tests[] = {
        {"", IOSTATE_badbit, FLAGS_skipws, 0, IOSTATE_badbit|IOSTATE_failbit, 0, ""},
        {"", IOSTATE_eofbit, FLAGS_skipws, 0, IOSTATE_eofbit|IOSTATE_failbit, 0, ""},
        {"", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit|IOSTATE_failbit, 0, ""},
        {" 0 ", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 2, "0"},
        {" \n0", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit, 3, "0"},
        {"-0", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit, 2, "-0"},
        {"000\n", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 3, "000"},
        {"015 16", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 3, "015"},
        {"099", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 1, "0"},
        {" 12345", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit, 6, "12345"},
        {"12345\r", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 5, "12345"},
        {"0xab ", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_goodbit, 4, "0xab"},
        {" 0xefg", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_goodbit, 5, "0xef"},
        {"0XABc", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_eofbit, 5, "0XABc"},
        {"0xzzz", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_failbit, 0, ""},
        {"0y123", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 1, "0"},
        {"\t+42 ", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 4, "+42"},
        {"+\t42", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, ""},
        {"+4\t2", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 2, "+4"},
        {"+0xc", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_eofbit, 4, "+0xc"},
        {" -1 ", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 3, "-1"},
        {" -005 ", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 5, "-005"},
        {" 2-0 ", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 2, "2"},
        {"--3 ", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, ""},
        {"+-7", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, ""},
        {"+", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, ""},
        {"-0x123abc", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_eofbit, 9, "-0x123abc"},
        {"0-x123abc", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 1, "0"},
        {"0x-123abc", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_failbit, 0, ""},
        {"2147483648", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit, 10, "2147483648"},
        {"99999999999999", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit, 14, "99999999999999"},
        {"999999999999999", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 15, "999999999999999"},
        {"123456789123456789", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 15, "123456789123456"},
        {"000000000000000000", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 15, "000000000000000"},
        {"-0xffffffffffffffffff", IOSTATE_goodbit, FLAGS_skipws, 16, IOSTATE_goodbit, 15, "-0xffffffffffff"},
        {"3.14159", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 1, "3"},
        {"deadbeef", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, ""},
        {"0deadbeef", IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 1, "0"},
        {"98765L", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 5, "98765"},
        {"9999l", IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_goodbit, 4, "9999"},
        {" 1", IOSTATE_goodbit, 0, 0, IOSTATE_failbit, 0, ""},
        {"1 ", IOSTATE_goodbit, 0, 0, IOSTATE_goodbit, 1, "1"},
        {"010", IOSTATE_goodbit, FLAGS_dec, 10, IOSTATE_eofbit, 3, "010"},
        {"0x123", IOSTATE_goodbit, FLAGS_dec, 10, IOSTATE_goodbit, 1, "0"},
        {"x1", IOSTATE_goodbit, FLAGS_dec, 10, IOSTATE_failbit, 0, ""},
        {"33", IOSTATE_goodbit, FLAGS_dec|FLAGS_oct, 10, IOSTATE_eofbit, 2, "33"},
        {"abc", IOSTATE_goodbit, FLAGS_dec|FLAGS_hex, 10, IOSTATE_failbit, 0, ""},
        {"33", IOSTATE_goodbit, FLAGS_oct, 8, IOSTATE_eofbit, 2, "33"},
        {"9", IOSTATE_goodbit, FLAGS_oct, 8, IOSTATE_failbit, 0, ""},
        {"0", IOSTATE_goodbit, FLAGS_oct, 8, IOSTATE_eofbit, 1, "0"},
        {"x1", IOSTATE_goodbit, FLAGS_oct, 8, IOSTATE_failbit, 0, ""},
        {"9", IOSTATE_goodbit, FLAGS_oct|FLAGS_hex, 16, IOSTATE_eofbit, 1, "9"},
        {"abc", IOSTATE_goodbit, FLAGS_oct|FLAGS_hex, 16, IOSTATE_eofbit, 3, "abc"},
        {"123 ", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_goodbit, 3, "123"},
        {"x123 ", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_failbit, 0, ""},
        {"0x123 ", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_goodbit, 5, "0x123"},
        {"-a", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_eofbit, 2, "-a"},
        {"-j", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_failbit, 0, ""},
        {"-0x-1", IOSTATE_goodbit, FLAGS_hex, 16, IOSTATE_failbit, 0, ""},
        {"0", IOSTATE_goodbit, FLAGS_dec|FLAGS_oct|FLAGS_hex, 10, IOSTATE_eofbit, 1, "0"},
        {"0z", IOSTATE_goodbit, FLAGS_dec|FLAGS_oct|FLAGS_hex, 10, IOSTATE_goodbit, 1, "0"}
    };

    pssb = call_func2(p_strstreambuf_dynamic_ctor, &ssb, 64);
    ok(pssb == &ssb, "wrong return, expected %p got %p\n", &ssb, pssb);
    ret = (int) call_func1(p_streambuf_allocate, &ssb.base);
    ok(ret == 1, "expected 1 got %d\n", ret);
    pis = call_func3(p_istream_sb_ctor, &is, &ssb.base, TRUE);
    ok(pis == &is, "wrong return, expected %p got %p\n", &is, pis);

    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        len = strlen(tests[i].stream_content);
        is.base_ios.state = tests[i].initial_state;
        is.base_ios.flags = tests[i].flags;
        ssb.base.eback = ssb.base.gptr = ssb.base.base;
        ssb.base.egptr = ssb.base.base + len;
        memcpy(ssb.base.base, tests[i].stream_content, len);

        ret = (int) call_func2(p_istream_getint, &is, buffer);
        ok(ret == tests[i].expected_return, "Test %d: wrong return, expected %d got %d\n", i,
            tests[i].expected_return, ret);
        ok(is.base_ios.state == tests[i].expected_state, "Test %d: expected %d got %d\n", i,
            tests[i].expected_state, is.base_ios.state);
        ok(ssb.base.gptr == ssb.base.base + tests[i].expected_offset, "Test %d: expected %p got %p\n", i,
            ssb.base.base + tests[i].expected_offset, ssb.base.gptr);
        ok(!strncmp(buffer, tests[i].expected_buffer, strlen(tests[i].expected_buffer)),
            "Test %d: unexpected buffer content, got '%s'\n", i, buffer);
    }

    call_func1(p_istream_vbase_dtor, &is);
    call_func1(p_strstreambuf_dtor, &ssb);
}

static void test_istream_getdouble(void)
{
    istream is, *pis;
    strstreambuf ssb, *pssb;
    int i, len, ret;
    char buffer[32];

    struct istream_getdouble_test {
        const char *stream_content;
        int count;
        ios_io_state initial_state;
        ios_flags flags;
        int expected_return;
        ios_io_state expected_state;
        int expected_offset;
        const char *expected_buffer;
        BOOL broken;
    } tests[] = {
        {"", 32, IOSTATE_badbit, FLAGS_skipws, 0, IOSTATE_badbit|IOSTATE_failbit, 0, "", FALSE},
        {"", 0, IOSTATE_badbit, FLAGS_skipws, 0, IOSTATE_badbit|IOSTATE_failbit, 0, "", FALSE},
        {"", 32, IOSTATE_eofbit, FLAGS_skipws, 0, IOSTATE_eofbit|IOSTATE_failbit, 0, "", FALSE},
        {"", 32, IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit|IOSTATE_failbit, 0, "", FALSE},
        {" ", 32, IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_eofbit|IOSTATE_failbit, 1, "", FALSE},
        {" 0", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_eofbit, 2, "0", FALSE},
        {"156", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_eofbit, 3, "156", FALSE},
        {"123 ", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_goodbit, 3, "123", FALSE},
        {"+4 5", 32, IOSTATE_goodbit, FLAGS_skipws, 2, IOSTATE_goodbit, 2, "+4", FALSE},
        {"-88a", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_goodbit, 3, "-88", FALSE},
        {"-+5", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_failbit, 1, "-", FALSE},
        {"++7", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_failbit, 1, "+", FALSE},
        {"+", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_eofbit|IOSTATE_failbit, 1, "+", FALSE},
        {"abc", 32, IOSTATE_goodbit, FLAGS_skipws, 0, IOSTATE_failbit, 0, "", FALSE},
        {"0xabc", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_goodbit, 1, "0", FALSE},
        {"01", 32, IOSTATE_goodbit, FLAGS_skipws, 2, IOSTATE_eofbit, 2, "01", FALSE},
        {"10000000000000000000", 32, IOSTATE_goodbit, FLAGS_skipws, 20, IOSTATE_eofbit, 20, "10000000000000000000", FALSE},
        {"1.2", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_eofbit, 3, "1.2", FALSE},
        {"\t-0.4444f", 32, IOSTATE_goodbit, FLAGS_skipws, 7, IOSTATE_goodbit, 8, "-0.4444", FALSE},
        {"3.-14159", 32, IOSTATE_goodbit, FLAGS_skipws, 2, IOSTATE_goodbit, 2, "3.", FALSE},
        {"3.14.159", 32, IOSTATE_goodbit, FLAGS_skipws, 4, IOSTATE_goodbit, 4, "3.14", FALSE},
        {"3.000", 32, IOSTATE_goodbit, FLAGS_skipws, 5, IOSTATE_eofbit, 5, "3.000", FALSE},
        {".125f", 32, IOSTATE_goodbit, FLAGS_skipws, 4, IOSTATE_goodbit, 4, ".125", FALSE},
        {"-.125f", 32, IOSTATE_goodbit, FLAGS_skipws, 5, IOSTATE_goodbit, 5, "-.125", FALSE},
        {"5L", 32, IOSTATE_goodbit, FLAGS_skipws, 1, IOSTATE_goodbit, 1, "5", FALSE},
        {"1.", 32, IOSTATE_goodbit, FLAGS_skipws, 2, IOSTATE_eofbit, 2, "1.", FALSE},
        {"55.!", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_goodbit, 3, "55.", FALSE},
        {"99.99999999999", 32, IOSTATE_goodbit, FLAGS_skipws, 14, IOSTATE_eofbit, 14, "99.99999999999", FALSE},
        {"9.9999999999999999999", 32, IOSTATE_goodbit, FLAGS_skipws, 21, IOSTATE_eofbit, 21, "9.9999999999999999999", FALSE},
        {"0.0000000000000f", 32, IOSTATE_goodbit, FLAGS_skipws, 15, IOSTATE_goodbit, 15, "0.0000000000000", FALSE},
        {"1.0000e5 ", 32, IOSTATE_goodbit, FLAGS_skipws, 8, IOSTATE_goodbit, 8, "1.0000e5", FALSE},
        {"-2.12345e1000 ", 32, IOSTATE_goodbit, FLAGS_skipws, 13, IOSTATE_goodbit, 13, "-2.12345e1000", FALSE},
        {"  8E1", 32, IOSTATE_goodbit, FLAGS_skipws, 3, IOSTATE_eofbit, 5, "8E1", FALSE},
        {"99.99E-99E5", 32, IOSTATE_goodbit, FLAGS_skipws, 9, IOSTATE_goodbit, 9, "99.99E-99", FALSE},
        {"0e0", 32, IOSTATE_goodbit, FLAGS_skipws|FLAGS_uppercase, 3, IOSTATE_eofbit, 3, "0e0", FALSE},
        {"1.e8.5", 32, IOSTATE_goodbit, 0, 4, IOSTATE_goodbit, 4, "1.e8", FALSE},
        {"1.0e-1000000000000000000 ", 32, IOSTATE_goodbit, 0, 24, IOSTATE_goodbit, 24, "1.0e-1000000000000000000", FALSE},
        {"1.e+f", 32, IOSTATE_goodbit, 0, 3, IOSTATE_goodbit, 3, "1.e", FALSE},
        {"1.ef", 32, IOSTATE_goodbit, 0, 2, IOSTATE_goodbit, 2, "1.", FALSE},
        {"1.E-z", 32, IOSTATE_goodbit, 0, 3, IOSTATE_goodbit, 3, "1.E", FALSE},
        {".", 32, IOSTATE_goodbit, 0, 1, IOSTATE_eofbit|IOSTATE_failbit, 1, ".", FALSE},
        {".e", 32, IOSTATE_goodbit, 0, 1, IOSTATE_failbit, 1, ".", FALSE},
        {".e.", 32, IOSTATE_goodbit, 0, 1, IOSTATE_failbit, 1, ".", FALSE},
        {".e5", 32, IOSTATE_goodbit, 0, 3, IOSTATE_eofbit|IOSTATE_failbit, 3, ".e5", FALSE},
        {".2e5", 32, IOSTATE_goodbit, 0, 4, IOSTATE_eofbit, 4, ".2e5", FALSE},
        {"9.e", 32, IOSTATE_goodbit, 0, 2, IOSTATE_goodbit, 2, "9.", FALSE},
        {"0.0e-0", 32, IOSTATE_goodbit, 0, 6, IOSTATE_eofbit, 6, "0.0e-0", FALSE},
        {"e5.2", 32, IOSTATE_goodbit, 0, 2, IOSTATE_failbit, 2, "e5", FALSE},
        {"1.0000", 5, IOSTATE_goodbit, 0, 4, IOSTATE_failbit, 5, "1.00", TRUE},
        {"-123456", 5, IOSTATE_goodbit, 0, 4, IOSTATE_failbit, 5, "-123", TRUE},
        {"3.5e2", 5, IOSTATE_goodbit, 0, 4, IOSTATE_failbit, 5, "3.5e", TRUE},
        {"3.e25", 5, IOSTATE_goodbit, 0, 4, IOSTATE_failbit, 5, "3.e2", TRUE},
        {"1.11f", 5, IOSTATE_goodbit, 0, 4, IOSTATE_goodbit, 4, "1.11", FALSE},
        {".5e-5", 5, IOSTATE_goodbit, 0, 4, IOSTATE_failbit, 5, ".5e-", TRUE},
        {".5e-", 5, IOSTATE_goodbit, 0, 3, IOSTATE_goodbit, 3, ".5e", FALSE},
        {".5e2", 5, IOSTATE_goodbit, 0, 4, IOSTATE_eofbit, 4, ".5e2", FALSE},
        {"1", 0, IOSTATE_goodbit, 0, -1, IOSTATE_failbit, 0, "", TRUE},
        {"x", 0, IOSTATE_goodbit, 0, -1, IOSTATE_failbit, 0, "", TRUE},
        {"", 0, IOSTATE_goodbit, 0, -1, IOSTATE_failbit, 0, "", TRUE},
        {"", 1, IOSTATE_goodbit, 0, 0, IOSTATE_eofbit|IOSTATE_failbit, 0, "", FALSE},
        {"1.0", 1, IOSTATE_goodbit, 0, 0, IOSTATE_failbit, 1, "", TRUE},
        {"1.0", 2, IOSTATE_goodbit, 0, 1, IOSTATE_failbit, 2, "1", TRUE}
    };

    pssb = call_func2(p_strstreambuf_dynamic_ctor, &ssb, 64);
    ok(pssb == &ssb, "wrong return, expected %p got %p\n", &ssb, pssb);
    ret = (int) call_func1(p_streambuf_allocate, &ssb.base);
    ok(ret == 1, "expected 1 got %d\n", ret);
    pis = call_func3(p_istream_sb_ctor, &is, &ssb.base, TRUE);
    ok(pis == &is, "wrong return, expected %p got %p\n", &is, pis);

    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        len = strlen(tests[i].stream_content);
        is.base_ios.state = tests[i].initial_state;
        is.base_ios.flags = tests[i].flags;
        ssb.base.eback = ssb.base.gptr = ssb.base.base;
        ssb.base.egptr = ssb.base.base + len;
        memcpy(ssb.base.base, tests[i].stream_content, len);

        ret = (int) call_func3(p_istream_getdouble, &is, buffer, tests[i].count);
        ok(ret == tests[i].expected_return ||
            /* xp, 2k3 */ broken(tests[i].broken && ret == tests[i].expected_return + 1),
            "Test %d: wrong return, expected %d got %d\n", i, tests[i].expected_return, ret);
        ok(is.base_ios.state == tests[i].expected_state, "Test %d: expected %d got %d\n", i,
            tests[i].expected_state, is.base_ios.state);
        ok(ssb.base.gptr == ssb.base.base + tests[i].expected_offset, "Test %d: expected %p got %p\n",
            i, ssb.base.base + tests[i].expected_offset, ssb.base.gptr);
        ok(!strncmp(buffer, tests[i].expected_buffer, strlen(tests[i].expected_buffer)),
            "Test %d: unexpected buffer content, got '%s'\n", i, buffer);
    }

    call_func1(p_istream_vbase_dtor, &is);
    call_func1(p_strstreambuf_dtor, &ssb);
}

static void test_istream_read(void)
{
    istream is, *pis;
    strstreambuf ssb, ssb_test, *pssb;
    int len, ret, i;

    /* makes tables narrower */
    const ios_io_state IOSTATE_faileof = IOSTATE_failbit|IOSTATE_eofbit;

    char c, st[8], char_out[] = {-85, ' ', 'a', -50};
    const char *str_out[] = {"AAAAAAA", "abc", "a", "abc", "ab", "abcde"};
    short s, short_out[] = {32767, -32768};
    unsigned short us, ushort_out[] = {65535u, 65534u, 32768u};
    int n, int_out[] = {123456789, 0, 1, -500, 0x8000, 2147483646, 2147483647, -2147483647, -2147483647-1, -1};
    unsigned un, uint_out[] = {4294967295u, 4294967294u, 2147483648u, 1u};
    LONG l, long_out[] = {2147483647l, -2147483647l-1};
    ULONG ul, ulong_out[] = {4294967295ul, 4294967294ul, 2147483648ul, 1ul};
    float f, float_out[] = {123.456f, 0.0f, 1.0f, 0.1f, -1.0f, -0.1f, FLT_MIN, -FLT_MIN, FLT_MAX, -FLT_MAX};
    double d, double_out[] = {1.0, 0.1, 0.0, INFINITY, -INFINITY};
    const char *sbf_out[] = {"", "abcd\n", "abcdef"};
    struct istream_read_test {
        enum { type_chr, type_str, type_shrt, type_ushrt, type_int, type_uint,
            type_long, type_ulong, type_flt, type_dbl, type_ldbl, type_sbf } type;
        const char *stream_content;
        ios_flags flags;
        int width;
        int expected_val;
        ios_io_state expected_state;
        int expected_width;
        int expected_offset;
        BOOL broken;
    } tests[] = {
        /* char */
        {type_chr, "",      FLAGS_skipws, 6, /* -85 */ 0, IOSTATE_faileof, 6, 0, FALSE},
        {type_chr, "  ",    FLAGS_skipws, 6, /* -85 */ 0, IOSTATE_faileof, 6, 2, FALSE},
        {type_chr, " abc ", 0,            6, /* ' ' */ 1, IOSTATE_goodbit, 6, 1, FALSE},
        {type_chr, " abc ", FLAGS_skipws, 6, /* 'a' */ 2, IOSTATE_goodbit, 6, 2, FALSE},
        {type_chr, " a",    FLAGS_skipws, 0, /* 'a' */ 2, IOSTATE_goodbit, 0, 2, FALSE},
        {type_chr, "\xce",  0,            6, /* -50 */ 3, IOSTATE_goodbit, 6, 1, FALSE},
        /* str */
        {type_str, "",        FLAGS_skipws, 6, /* "AAAAAAA" */ 0, IOSTATE_faileof, 6, 0, FALSE},
        {type_str, " ",       FLAGS_skipws, 6, /* "AAAAAAA" */ 0, IOSTATE_faileof, 6, 1, FALSE},
        {type_str, " abc",    FLAGS_skipws, 6, /* "abc" */     1, IOSTATE_eofbit,  0, 4, FALSE},
        {type_str, " abc ",   FLAGS_skipws, 6, /* "abc" */     1, IOSTATE_goodbit, 0, 4, FALSE},
        {type_str, " a\tc",   FLAGS_skipws, 6, /* "a" */       2, IOSTATE_goodbit, 0, 2, FALSE},
        {type_str, " a\tc",   0,            6, /* "AAAAAAA" */ 0, IOSTATE_failbit, 0, 0, FALSE},
        {type_str, "abcde\n", 0,            4, /* "abc" */     3, IOSTATE_goodbit, 0, 3, FALSE},
        {type_str, "abc\n",   0,            4, /* "abc" */     3, IOSTATE_goodbit, 0, 3, FALSE},
        {type_str, "ab\r\n",  0,            3, /* "ab" */      4, IOSTATE_goodbit, 0, 2, FALSE},
        {type_str, "abc",     0,            4, /* "abc" */     3, IOSTATE_goodbit, 0, 3, FALSE},
        {type_str, "abc",     0,            1, /* "AAAAAAA" */ 0, IOSTATE_failbit, 0, 0, FALSE},
        {type_str, "\n",      0,            1, /* "AAAAAAA" */ 0, IOSTATE_failbit, 0, 0, FALSE},
        {type_str, "abcde\n", 0,            0, /* "abcde" */   5, IOSTATE_goodbit, 0, 5, FALSE},
        {type_str, "\n",      0,            0, /* "AAAAAAA" */ 0, IOSTATE_failbit, 0, 0, FALSE},
        {type_str, "abcde",   0,           -1, /* "abcde" */   5, IOSTATE_eofbit,  0, 5, FALSE},
        /* short */
        {type_shrt, "32767",       0, 6, /* 32767 */  0, IOSTATE_eofbit,  6, 5,  FALSE},
        {type_shrt, "32768",       0, 6, /* 32767 */  0, IOSTATE_faileof, 6, 5,  FALSE},
        {type_shrt, "2147483648",  0, 6, /* 32767 */  0, IOSTATE_faileof, 6, 10, FALSE},
        {type_shrt, "4294967296",  0, 6, /* 32767 */  0, IOSTATE_faileof, 6, 10, FALSE},
        {type_shrt, "-32768",      0, 6, /* -32768 */ 1, IOSTATE_eofbit,  6, 6,  FALSE},
        {type_shrt, "-32769",      0, 6, /* -32768 */ 1, IOSTATE_faileof, 6, 6,  FALSE},
        {type_shrt, "-2147483648", 0, 6, /* -32768 */ 1, IOSTATE_faileof, 6, 11, FALSE},
        /* unsigned short */
        {type_ushrt, "65535",          0, 6, /* 65535 */ 0, IOSTATE_eofbit,  6, 5,  FALSE},
        {type_ushrt, "65536",          0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 5,  FALSE},
        {type_ushrt, "12345678",       0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 8,  FALSE},
        {type_ushrt, "2147483648",     0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 10, FALSE},
        {type_ushrt, "4294967296",     0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 10, FALSE},
        {type_ushrt, "99999999999999", 0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 14, FALSE},
        {type_ushrt, "-1",             0, 6, /* 65535 */ 0, IOSTATE_eofbit,  6, 2,  TRUE},
        {type_ushrt, "-2",             0, 6, /* 65534 */ 1, IOSTATE_eofbit,  6, 2,  FALSE},
        {type_ushrt, "-32768",         0, 6, /* 32768 */ 2, IOSTATE_eofbit,  6, 6,  FALSE},
        {type_ushrt, "-32769",         0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 6,  FALSE},
        {type_ushrt, "-2147483648",    0, 6, /* 65535 */ 0, IOSTATE_faileof, 6, 11, FALSE},
        /* int */
        {type_int, "",            FLAGS_skipws, 6, /* 123456789 */   0, IOSTATE_faileof, 6, 0,  FALSE},
        {type_int, " 0",          FLAGS_skipws, 6, /* 0 */           1, IOSTATE_eofbit,  6, 2,  FALSE},
        {type_int, " 0",          0,            6, /* 0 */           1, IOSTATE_failbit, 6, 0,  FALSE},
        {type_int, "+1 ",         0,            6, /* 1 */           2, IOSTATE_goodbit, 6, 2,  FALSE},
        {type_int, "1L",          0,            6, /* 1 */           2, IOSTATE_goodbit, 6, 1,  FALSE},
        {type_int, "-500.0",      0,            6, /* -500 */        3, IOSTATE_goodbit, 6, 4,  FALSE},
        {type_int, "0x8000",      0,            6, /* 0x8000 */      4, IOSTATE_eofbit,  6, 6,  FALSE},
        {type_int, "0xtest",      0,            6, /* 0 */           1, IOSTATE_failbit, 6, 0,  FALSE},
        {type_int, "0test",       0,            6, /* 0 */           1, IOSTATE_goodbit, 6, 1,  FALSE},
        {type_int, "0x7ffffffe",  0,            6, /* 2147483646 */  5, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "0x7fffffff",  0,            6, /* 2147483647 */  6, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "0x80000000",  0,            6, /* 2147483647 */  6, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "0xdeadbeef",  0,            6, /* 2147483647 */  6, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "2147483648",  0,            6, /* 2147483647 */  6, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "4294967295",  0,            6, /* 2147483647 */  6, IOSTATE_eofbit,  6, 10, FALSE},
        {type_int, "-2147483647", 0,            6, /* -2147483647 */ 7, IOSTATE_eofbit,  6, 11, FALSE},
        {type_int, "-2147483648", 0,            6, /* -2147483648 */ 8, IOSTATE_eofbit,  6, 11, FALSE},
        {type_int, "-2147483649", 0,            6, /* -2147483648 */ 8, IOSTATE_eofbit,  6, 11, FALSE},
        {type_int, "-1f",         FLAGS_dec,    6, /* -1 */          9, IOSTATE_goodbit, 6, 2,  FALSE},
        /* unsigned int */
        {type_uint, "4294967295",     0, 6, /* 4294967295 */ 0, IOSTATE_eofbit,  6, 10, TRUE},
        {type_uint, "4294967296",     0, 6, /* 4294967295 */ 0, IOSTATE_faileof, 6, 10, FALSE},
        {type_uint, "99999999999999", 0, 6, /* 4294967295 */ 0, IOSTATE_faileof, 6, 14, FALSE},
        {type_uint, "-1",             0, 6, /* 4294967295 */ 0, IOSTATE_eofbit,  6, 2,  TRUE},
        {type_uint, "-2",             0, 6, /* 4294967294 */ 1, IOSTATE_eofbit,  6, 2,  FALSE},
        {type_uint, "-2147483648",    0, 6, /* 2147483648 */ 2, IOSTATE_eofbit,  6, 11, FALSE},
        {type_uint, "-4294967295",    0, 6, /* 1 */          3, IOSTATE_eofbit,  6, 11, FALSE},
        {type_uint, "-9999999999999", 0, 6, /* 1 */          3, IOSTATE_eofbit,  6, 14, FALSE},
        /* long */
        {type_long, "2147483647",     0, 6, /* 2147483647 */  0, IOSTATE_eofbit,  6, 10, TRUE},
        {type_long, "2147483648",     0, 6, /* 2147483647 */  0, IOSTATE_faileof, 6, 10, FALSE},
        {type_long, "4294967295",     0, 6, /* 2147483647 */  0, IOSTATE_faileof, 6, 10, FALSE},
        {type_long, "-2147483648",    0, 6, /* -2147483648 */ 1, IOSTATE_eofbit,  6, 11, TRUE},
        {type_long, "-2147483649",    0, 6, /* -2147483648 */ 1, IOSTATE_faileof, 6, 11, FALSE},
        {type_long, "-9999999999999", 0, 6, /* -2147483648 */ 1, IOSTATE_faileof, 6, 14, FALSE},
        /* unsigned long */
        {type_ulong, "4294967295",     0, 6, /* 4294967295 */ 0, IOSTATE_eofbit,  6, 10, TRUE},
        {type_ulong, "4294967296",     0, 6, /* 4294967295 */ 0, IOSTATE_faileof, 6, 10, FALSE},
        {type_ulong, "99999999999999", 0, 6, /* 4294967295 */ 0, IOSTATE_faileof, 6, 14, FALSE},
        {type_ulong, "-1",             0, 6, /* 4294967295 */ 0, IOSTATE_eofbit,  6, 2,  TRUE},
        {type_ulong, "-2",             0, 6, /* 4294967294 */ 1, IOSTATE_eofbit,  6, 2,  FALSE},
        {type_ulong, "-2147483648",    0, 6, /* 2147483648 */ 2, IOSTATE_eofbit,  6, 11, FALSE},
        {type_ulong, "-4294967295",    0, 6, /* 1 */          3, IOSTATE_eofbit,  6, 11, FALSE},
        {type_ulong, "-9999999999999", 0, 6, /* 1 */          3, IOSTATE_eofbit,  6, 14, FALSE},
        /* float */
        {type_flt, "",                      FLAGS_skipws, 6, /* 123.456 */  0, IOSTATE_faileof, 6, 0,  FALSE},
        {type_flt, "",                      0,            6, /* 123.456 */  0, IOSTATE_faileof, 6, 0,  FALSE},
        {type_flt, " 0",                    0,            6, /* 123.456 */  0, IOSTATE_failbit, 6, 0,  FALSE},
        {type_flt, " 0",                    FLAGS_skipws, 6, /* 0.0 */      1, IOSTATE_eofbit,  6, 2,  FALSE},
        {type_flt, "-0 ",                   0,            6, /* 0.0 */      1, IOSTATE_goodbit, 6, 2,  FALSE},
        {type_flt, "+1.0",                  0,            6, /* 1.0 */      2, IOSTATE_eofbit,  6, 4,  FALSE},
        {type_flt, "1.#INF",                0,            6, /* 1.0 */      2, IOSTATE_goodbit, 6, 2,  FALSE},
        {type_flt, "0.100000000000000e1",   0,            6, /* 1.0 */      2, IOSTATE_eofbit,  6, 19, FALSE},
/* crashes on xp
        {type_flt, "0.1000000000000000e1",  0,            6,    0.1         3, IOSTATE_failbit, 6, 20, FALSE}, */
        {type_flt, "0.10000000000000000e1", 0,            6, /* 0.1 */      3, IOSTATE_failbit, 6, 20, TRUE},
        {type_flt, "-0.10000000000000e1",   0,            6, /* -1.0 */     4, IOSTATE_eofbit,  6, 19, FALSE},
/* crashes on xp
        {type_flt, "-0.100000000000000e1 ", 0,            6,    -0.1        5, IOSTATE_failbit, 6, 20, FALSE}, */
        {type_flt, "-0.1000000000000000e1", 0,            6, /* -0.1 */     5, IOSTATE_failbit, 6, 20, TRUE},
        {type_flt, "5.1691126e-77",         0,            6, /* FLT_MIN */  6, IOSTATE_eofbit,  6, 13, FALSE},
        {type_flt, "-2.49873e-41f",         0,            6, /* -FLT_MIN */ 7, IOSTATE_goodbit, 6, 12, FALSE},
        {type_flt, "1.23456789e1234",       0,            6, /* FLT_MAX */  8, IOSTATE_eofbit,  6, 15, FALSE},
        {type_flt, "-1.23456789e1234",      0,            6, /* -FLT_MAX */ 9, IOSTATE_eofbit,  6, 16, FALSE},
        /* double */
        {type_dbl, "0.10000000000000000000000e1",   0, 6, /* 1.0 */  0, IOSTATE_eofbit,  6, 27, FALSE},
/* crashes on xp
        {type_dbl, "0.100000000000000000000000e1",  0, 6,    0.1     1, IOSTATE_failbit, 6, 28, FALSE}, */
        {type_dbl, "0.1000000000000000000000000e1", 0, 6, /* 0.1 */  1, IOSTATE_failbit, 6, 28, TRUE},
        {type_dbl, "3.698124698114778e-6228",       0, 6, /* 0.0 */  2, IOSTATE_eofbit,  6, 23, FALSE},
        {type_dbl, "-3.698124698114778e-6228",      0, 6, /* 0.0 */  2, IOSTATE_eofbit,  6, 24, FALSE},
        {type_dbl, "3.698124698114778e6228",        0, 6, /* INF */  3, IOSTATE_eofbit,  6, 22, FALSE},
        {type_dbl, "-3.698124698114778e6228A",      0, 6, /* -INF */ 4, IOSTATE_goodbit, 6, 23, FALSE},
        /* long double */
        {type_ldbl, "0.100000000000000000000000000e1",   0, 6, /* 1.0 */  0, IOSTATE_eofbit,  6, 31, FALSE},
/* crashes on xp
        {type_ldbl, "0.1000000000000000000000000000e1",  0, 6,    0.1     1, IOSTATE_failbit, 6, 32, FALSE}, */
        {type_ldbl, "0.10000000000000000000000000000e1", 0, 6, /* 0.1 */  1, IOSTATE_failbit, 6, 32, TRUE},
        {type_ldbl, "1.69781699841e-1475",               0, 6, /* 0.0 */  2, IOSTATE_eofbit,  6, 19, FALSE},
        {type_ldbl, "-1.69781699841e-1475l",             0, 6, /* 0.0 */  2, IOSTATE_goodbit, 6, 20, FALSE},
        {type_ldbl, "1.69781699841e1475",                0, 6, /* INF */  3, IOSTATE_eofbit,  6, 18, FALSE},
        {type_ldbl, "-1.69781699841e1475",               0, 6, /* -INF */ 4, IOSTATE_eofbit,  6, 19, FALSE},
        /* streambuf */
        {type_sbf, "",           FLAGS_skipws, 6, /* "" */      0, IOSTATE_faileof, 6, 0, FALSE},
        {type_sbf, "  ",         FLAGS_skipws, 6, /* "" */      0, IOSTATE_faileof, 6, 2, FALSE},
        {type_sbf, "\r\nabcd\n", FLAGS_skipws, 6, /* "abc\n" */ 1, IOSTATE_goodbit, 6, 8, FALSE},
        {type_sbf, "abcdefg\n",  0,            6, /* "abcde" */ 2, IOSTATE_failbit, 6, 9, FALSE},
        {type_sbf, "abcdefg\n",  0,            0, /* "" */      0, IOSTATE_failbit, 0, 9, FALSE}
    };

    pssb = call_func2(p_strstreambuf_dynamic_ctor, &ssb_test, 64);
    ok(pssb == &ssb_test, "wrong return, expected %p got %p\n", &ssb_test, pssb);
    ret = (int) call_func1(p_streambuf_allocate, &ssb_test.base);
    ok(ret == 1, "expected 1 got %d\n", ret);
    ssb_test.dynamic = 0;
    pssb = call_func2(p_strstreambuf_dynamic_ctor, &ssb, 64);
    ok(pssb == &ssb, "wrong return, expected %p got %p\n", &ssb, pssb);
    ret = (int) call_func1(p_streambuf_allocate, &ssb.base);
    ok(ret == 1, "expected 1 got %d\n", ret);
    pis = call_func3(p_istream_sb_ctor, &is, &ssb.base, TRUE);
    ok(pis == &is, "wrong return, expected %p got %p\n", &is, pis);

    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        len = strlen(tests[i].stream_content);
        is.base_ios.state = IOSTATE_goodbit;
        is.base_ios.flags = tests[i].flags;
        is.base_ios.width = tests[i].width;
        ssb.base.eback = ssb.base.gptr = ssb.base.base;
        ssb.base.egptr = ssb.base.base + len;
        memcpy(ssb.base.base, tests[i].stream_content, len);

        switch (tests[i].type) {
        case type_chr:
            c = -85;
            pis = call_func2(p_istream_read_char, &is, &c);
            ok(c == char_out[tests[i].expected_val], "Test %d: expected %d got %d\n", i,
                char_out[tests[i].expected_val], c);
            break;
        case type_str:
            memset(st, 'A', sizeof(st));
            st[7] = 0;
            pis = call_func2(p_istream_read_str, &is, st);
            ok(!strcmp(st, str_out[tests[i].expected_val]), "Test %d: expected %s got %s\n", i,
                str_out[tests[i].expected_val], st);
            break;
        case type_shrt:
            s = 12345;
            pis = call_func2(p_istream_read_short, &is, &s);
            ok(s == short_out[tests[i].expected_val], "Test %d: expected %hd got %hd\n", i,
                short_out[tests[i].expected_val], s);
            break;
        case type_ushrt:
            us = 12345u;
            pis = call_func2(p_istream_read_unsigned_short, &is, &us);
            ok(us == ushort_out[tests[i].expected_val], "Test %d: expected %hu got %hu\n", i,
                ushort_out[tests[i].expected_val], us);
            break;
        case type_int:
            n = 123456789;
            pis = call_func2(p_istream_read_int, &is, &n);
            ok(n == int_out[tests[i].expected_val], "Test %d: expected %d got %d\n", i,
                int_out[tests[i].expected_val], n);
            break;
        case type_uint:
            un = 123456789u;
            pis = call_func2(p_istream_read_unsigned_int, &is, &un);
            ok(un == uint_out[tests[i].expected_val], "Test %d: expected %u got %u\n", i,
                uint_out[tests[i].expected_val], un);
            break;
        case type_long:
            l = 123456789l;
            pis = call_func2(p_istream_read_long, &is, &l);
            ok(l == long_out[tests[i].expected_val], "Test %d: expected %ld got %ld\n", i,
                long_out[tests[i].expected_val], l);
            break;
        case type_ulong:
            ul = 123456789ul;
            pis = call_func2(p_istream_read_unsigned_long, &is, &ul);
            ok(ul == ulong_out[tests[i].expected_val], "Test %d: expected %lu got %lu\n", i,
                ulong_out[tests[i].expected_val], ul);
            break;
        case type_flt:
            f = 123.456f;
            pis = call_func2(p_istream_read_float, &is, &f);
            ok(f == float_out[tests[i].expected_val], "Test %d: expected %f got %f\n", i,
                float_out[tests[i].expected_val], f);
            break;
        case type_dbl:
            d = 123.456;
            pis = call_func2(p_istream_read_double, &is, &d);
            ok(d == double_out[tests[i].expected_val], "Test %d: expected %f got %f\n", i,
                double_out[tests[i].expected_val], d);
            break;
        case type_ldbl:
            d = 123.456;
            pis = call_func2(p_istream_read_long_double, &is, &d);
            ok(d == double_out[tests[i].expected_val], "Test %d: expected %f got %f\n", i,
                double_out[tests[i].expected_val], d);
            break;
        case type_sbf:
            ssb_test.base.pbase = ssb_test.base.pptr = ssb_test.base.base;
            ssb_test.base.epptr = ssb_test.base.base + tests[i].width;
            pis = call_func2(p_istream_read_streambuf, &is, &ssb_test.base);
            len = strlen(sbf_out[tests[i].expected_val]);
            ok(ssb_test.base.pptr == ssb_test.base.pbase + len, "Test %d: wrong put pointer, expected %p got %p\n",
                i, ssb_test.base.pbase + len, ssb_test.base.pptr);
            ok(!strncmp(ssb_test.base.pbase, sbf_out[tests[i].expected_val], len),
                "Test %d: expected %s got %s\n", i, sbf_out[tests[i].expected_val], ssb_test.base.pbase);
            break;
        }

        ok(pis == &is, "Test %d: wrong return, expected %p got %p\n", i, &is, pis);
        ok(is.base_ios.state == tests[i].expected_state || /* xp, 2k3 */ broken(tests[i].broken),
            "Test %d: expected %d got %d\n", i, tests[i].expected_state, is.base_ios.state);
        ok(is.base_ios.width == tests[i].expected_width, "Test %d: expected %d got %d\n", i,
            tests[i].expected_width, is.base_ios.width);
        ok(ssb.base.gptr == ssb.base.base + tests[i].expected_offset ||
            /* xp, 2k3 */ broken(tests[i].broken), "Test %d: expected %p got %p\n", i,
            ssb.base.base + tests[i].expected_offset, ssb.base.gptr);
    }

    ssb_test.dynamic = 1;
    call_func1(p_istream_vbase_dtor, &is);
    call_func1(p_strstreambuf_dtor, &ssb);
    call_func1(p_strstreambuf_dtor, &ssb_test);
}

static void test_istream_withassign(void)
{
    istream isa1, isa2, *pisa;
    ostream *pos;
    streambuf sb;

    memset(&isa1, 0xab, sizeof(istream));
    memset(&isa2, 0xab, sizeof(istream));

    /* constructors/destructors */
    pisa = call_func3(p_istream_withassign_sb_ctor, &isa1, NULL, TRUE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa1.base_ios.sb);
    ok(isa1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0, "expected 0 got %d\n", isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 6, "expected 6 got %d\n", isa1.base_ios.precision);
    ok(isa1.base_ios.fill == ' ', "expected 32 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0, "expected 0 got %d\n", isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == -1, "expected -1 got %d\n", isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_vbase_dtor, &isa1);
    isa1.extract_delim = isa1.count = 0xabababab;
    memset(&isa1.base_ios, 0xab, sizeof(ios));
    isa1.base_ios.delbuf = 0;
    pisa = call_func3(p_istream_withassign_sb_ctor, &isa1, NULL, FALSE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa1.base_ios.sb);
    ok(isa1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0, "expected 0 got %d\n", isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == isa2.base_ios.tie, "expected %p got %p\n", isa2.base_ios.tie, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.precision);
    ok(isa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_dtor, &isa1.base_ios);
    isa1.extract_delim = isa1.count = 0xabababab;
    memset(&isa1.base_ios, 0xab, sizeof(ios));
    pisa = call_func3(p_istream_withassign_sb_ctor, &isa1, &sb, TRUE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, isa1.base_ios.sb);
    ok(isa1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0, "expected 0 got %d\n", isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 6, "expected 6 got %d\n", isa1.base_ios.precision);
    ok(isa1.base_ios.fill == ' ', "expected 32 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0, "expected 0 got %d\n", isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == -1, "expected -1 got %d\n", isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_vbase_dtor, &isa1);
    isa1.extract_delim = isa1.count = 0xabababab;
    memset(&isa1.base_ios, 0xab, sizeof(ios));
    isa1.base_ios.delbuf = 0;
    isa1.base_ios.state = 0xabababab | IOSTATE_badbit;
    pisa = call_func3(p_istream_withassign_sb_ctor, &isa1, &sb, FALSE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == &sb, "expected %p got %p\n", &sb, isa1.base_ios.sb);
    ok(isa1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0, "expected 0 got %d\n", isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == isa2.base_ios.tie, "expected %p got %p\n", isa2.base_ios.tie, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.precision);
    ok(isa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_dtor, &isa1.base_ios);
    isa1.extract_delim = isa1.count = 0xabababab;
    pisa = call_func2(p_istream_withassign_ctor, &isa1, TRUE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa1.base_ios.sb);
    ok(isa1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0, "expected 0 got %d\n", isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 6, "expected 6 got %d\n", isa1.base_ios.precision);
    ok(isa1.base_ios.fill == ' ', "expected 32 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0, "expected 0 got %d\n", isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == -1, "expected -1 got %d\n", isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_vbase_dtor, &isa1);
    isa1.extract_delim = isa1.count = 0xabababab;
    memset(&isa1.base_ios, 0xab, sizeof(ios));
    pisa = call_func2(p_istream_withassign_ctor, &isa1, FALSE);
    ok(pisa == &isa1, "wrong return, expected %p got %p\n", &isa1, pisa);
    ok(isa1.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa1.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa1.base_ios.sb == isa2.base_ios.sb, "expected %p got %p\n", isa2.base_ios.sb, isa1.base_ios.sb);
    ok(isa1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.state);
    ok(isa1.base_ios.delbuf == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.delbuf);
    ok(isa1.base_ios.tie == isa2.base_ios.tie, "expected %p got %p\n", isa2.base_ios.tie, isa1.base_ios.tie);
    ok(isa1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, isa1.base_ios.flags);
    ok(isa1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.precision);
    ok(isa1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", isa1.base_ios.fill);
    ok(isa1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.width);
    ok(isa1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa1.base_ios.do_lock);
    call_func1(p_istream_withassign_dtor, &isa1.base_ios);
    isa1.extract_delim = isa1.count = 0xcdcdcdcd;
    memset(&isa1.base_ios, 0xcd, sizeof(ios));
    pisa = call_func3(p_istream_withassign_copy_ctor, &isa2, &isa1, TRUE);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0, "expected 0 got %d\n", isa1.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa1.count);
    ok(isa2.base_ios.sb == isa1.base_ios.sb, "expected %p got %p\n", isa1.base_ios.sb, isa2.base_ios.sb);
    ok(isa2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == isa1.base_ios.tie, "expected %p got %p\n", isa1.base_ios.tie, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", isa2.base_ios.precision);
    ok(isa2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == -1, "expected -1 got %d\n", isa2.base_ios.do_lock);
    call_func1(p_istream_withassign_vbase_dtor, &isa2);
    isa1.base_ios.sb = NULL;
    isa2.extract_delim = isa2.count = 0xabababab;
    memset(&isa2.base_ios, 0xab, sizeof(ios));
    isa2.base_ios.delbuf = 0;
    isa2.base_ios.flags &= ~FLAGS_skipws;
    pos = isa2.base_ios.tie;
    pisa = call_func3(p_istream_withassign_copy_ctor, &isa2, &isa1, FALSE);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0, "expected 0 got %d\n", isa2.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa2.count);
    ok(isa2.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa2.base_ios.sb);
    ok(isa2.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == pos, "expected %p got %p\n", pos, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.precision);
    ok(isa2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.do_lock);
    call_func1(p_istream_withassign_dtor, &isa2.base_ios);

    /* assignment */
    isa2.extract_delim = isa2.count = 0xabababab;
    isa2.base_ios.delbuf = 0xabababab;
    pisa = call_func2(p_istream_withassign_assign_sb, &isa2, &sb);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa2.count);
    ok(isa2.base_ios.sb == &sb, "expected %p got %p\n", &sb, isa2.base_ios.sb);
    ok(isa2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == 6, "expected 6 got %d\n", isa2.base_ios.precision);
    ok(isa2.base_ios.fill == ' ', "expected 32 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == 0, "expected 0 got %d\n", isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.do_lock);
    isa2.count = 0xabababab;
    memset(&isa2.base_ios, 0xab, sizeof(ios));
    isa2.base_ios.delbuf = 0;
    pisa = call_func2(p_istream_withassign_assign_sb, &isa2, NULL);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa2.count);
    ok(isa2.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa2.base_ios.sb);
    ok(isa2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == 6, "expected 6 got %d\n", isa2.base_ios.precision);
    ok(isa2.base_ios.fill == ' ', "expected 32 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == 0, "expected 0 got %d\n", isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.do_lock);
    isa2.count = 0xabababab;
    memset(&isa2.base_ios, 0xab, sizeof(ios));
    isa2.base_ios.delbuf = 0;
    pisa = call_func2(p_istream_withassign_assign_is, &isa2, &isa1);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa2.count);
    ok(isa2.base_ios.sb == NULL, "expected %p got %p\n", NULL, isa2.base_ios.sb);
    ok(isa2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == 6, "expected 6 got %d\n", isa2.base_ios.precision);
    ok(isa2.base_ios.fill == ' ', "expected 32 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == 0, "expected 0 got %d\n", isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.do_lock);
    isa1.base_ios.sb = &sb;
    isa2.count = 0xabababab;
    memset(&isa2.base_ios, 0xab, sizeof(ios));
    isa2.base_ios.delbuf = 0;
    pisa = call_func2(p_istream_withassign_assign, &isa2, &isa1);
    ok(pisa == &isa2, "wrong return, expected %p got %p\n", &isa2, pisa);
    ok(isa2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.extract_delim);
    ok(isa2.count == 0, "expected 0 got %d\n", isa2.count);
    ok(isa2.base_ios.sb == &sb, "expected %p got %p\n", &sb, isa2.base_ios.sb);
    ok(isa2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, isa2.base_ios.state);
    ok(isa2.base_ios.delbuf == 0, "expected 0 got %d\n", isa2.base_ios.delbuf);
    ok(isa2.base_ios.tie == NULL, "expected %p got %p\n", NULL, isa2.base_ios.tie);
    ok(isa2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, isa2.base_ios.flags);
    ok(isa2.base_ios.precision == 6, "expected 6 got %d\n", isa2.base_ios.precision);
    ok(isa2.base_ios.fill == ' ', "expected 32 got %d\n", isa2.base_ios.fill);
    ok(isa2.base_ios.width == 0, "expected 0 got %d\n", isa2.base_ios.width);
    ok(isa2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, isa2.base_ios.do_lock);
}

static void test_istrstream(void)
{
    istream is1, is2, *pis;
    ostream *pos;
    strstreambuf *pssb;
    char buffer[32];

    memset(&is1, 0xab, sizeof(istream));
    memset(&is2, 0xab, sizeof(istream));

    /* constructors/destructors */
    pis = call_func4(p_istrstream_buffer_ctor, &is1, buffer, 32, TRUE);
    pssb = (strstreambuf*) is1.base_ios.sb;
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(is1.base_ios.delbuf == 1, "expected 1 got %d\n", is1.base_ios.delbuf);
    ok(is1.base_ios.tie == NULL, "expected %p got %p\n", NULL, is1.base_ios.tie);
    ok(is1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, is1.base_ios.flags);
    ok(is1.base_ios.precision == 6, "expected 6 got %d\n", is1.base_ios.precision);
    ok(is1.base_ios.fill == ' ', "expected 32 got %d\n", is1.base_ios.fill);
    ok(is1.base_ios.width == 0, "expected 0 got %d\n", is1.base_ios.width);
    ok(is1.base_ios.do_lock == -1, "expected -1 got %d\n", is1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 32, "wrong buffer end, expected %p got %p\n", buffer + 32, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer + 32, "wrong get end, expected %p got %p\n", buffer + 32, pssb->base.egptr);
    ok(pssb->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pssb->base.pbase);
    ok(pssb->base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, pssb->base.pptr);
    ok(pssb->base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_istrstream_vbase_dtor, &is1);
    ok(is1.base_ios.sb == NULL, "expected %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    memset(&is1, 0xab, sizeof(istream));
    pis = call_func4(p_istrstream_buffer_ctor, &is1, NULL, -1, TRUE);
    pssb = (strstreambuf*) is1.base_ios.sb;
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(is1.base_ios.delbuf == 1, "expected 1 got %d\n", is1.base_ios.delbuf);
    ok(is1.base_ios.tie == NULL, "expected %p got %p\n", NULL, is1.base_ios.tie);
    ok(is1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, is1.base_ios.flags);
    ok(is1.base_ios.precision == 6, "expected 6 got %d\n", is1.base_ios.precision);
    ok(is1.base_ios.fill == ' ', "expected 32 got %d\n", is1.base_ios.fill);
    ok(is1.base_ios.width == 0, "expected 0 got %d\n", is1.base_ios.width);
    ok(is1.base_ios.do_lock == -1, "expected -1 got %d\n", is1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pssb->base.base);
    ok(pssb->base.ebuf == (char*) 0x7fffffff || pssb->base.ebuf == (char*) -1,
        "wrong buffer end, expected 0x7fffffff or -1, got %p\n", pssb->base.ebuf);
    ok(pssb->base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, pssb->base.eback);
    ok(pssb->base.gptr == NULL, "wrong get pointer, expected %p got %p\n", NULL, pssb->base.gptr);
    ok(pssb->base.egptr == (char*) 0x7fffffff || pssb->base.egptr == (char*) -1,
        "wrong get end, expected 0x7fffffff or -1, got %p\n", pssb->base.egptr);
    ok(pssb->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pssb->base.pbase);
    ok(pssb->base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, pssb->base.pptr);
    ok(pssb->base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_istrstream_vbase_dtor, &is1);
    ok(is1.base_ios.sb == NULL, "expected %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    is1.extract_delim = is1.count = 0xabababab;
    memset(&is1.base_ios, 0xab, sizeof(ios));
    is1.base_ios.delbuf = 0;
    strcpy(buffer, "Test");
    pis = call_func4(p_istrstream_buffer_ctor, &is1, buffer, 0, FALSE);
    pssb = (strstreambuf*) is1.base_ios.sb;
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.state);
    ok(is1.base_ios.delbuf == 1, "expected 1 got %d\n", is1.base_ios.delbuf);
    ok(is1.base_ios.tie == is2.base_ios.tie, "expected %p got %p\n", is2.base_ios.tie, is1.base_ios.tie);
    ok(is1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, is1.base_ios.flags);
    ok(is1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.precision);
    ok(is1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", is1.base_ios.fill);
    ok(is1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.width);
    ok(is1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 4, "wrong buffer end, expected %p got %p\n", buffer + 4, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer + 4, "wrong get end, expected %p got %p\n", buffer + 4, pssb->base.egptr);
    ok(pssb->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pssb->base.pbase);
    ok(pssb->base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, pssb->base.pptr);
    ok(pssb->base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_istrstream_dtor, &is1.base_ios);
    ok(is1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, is1.base_ios.sb);
    ok(is1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);

    memset(&is1, 0xab, sizeof(istream));
    buffer[0] = 0;
    pis = call_func3(p_istrstream_str_ctor, &is1, buffer, TRUE);
    pssb = (strstreambuf*) is1.base_ios.sb;
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is1.base_ios.state);
    ok(is1.base_ios.delbuf == 1, "expected 1 got %d\n", is1.base_ios.delbuf);
    ok(is1.base_ios.tie == NULL, "expected %p got %p\n", NULL, is1.base_ios.tie);
    ok(is1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, is1.base_ios.flags);
    ok(is1.base_ios.precision == 6, "expected 6 got %d\n", is1.base_ios.precision);
    ok(is1.base_ios.fill == ' ', "expected 32 got %d\n", is1.base_ios.fill);
    ok(is1.base_ios.width == 0, "expected 0 got %d\n", is1.base_ios.width);
    ok(is1.base_ios.do_lock == -1, "expected -1 got %d\n", is1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer, "wrong buffer end, expected %p got %p\n", buffer, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pssb->base.pbase);
    ok(pssb->base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, pssb->base.pptr);
    ok(pssb->base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_istrstream_vbase_dtor, &is1);
    ok(is1.base_ios.sb == NULL, "expected %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is1.base_ios.state);
    if (0) /* crashes on native */
        pis = call_func3(p_istrstream_str_ctor, &is1, NULL, TRUE);
    is1.extract_delim = is1.count = 0xabababab;
    memset(&is1.base_ios, 0xab, sizeof(ios));
    is1.base_ios.delbuf = 0;
    pis = call_func3(p_istrstream_str_ctor, &is1, buffer, FALSE);
    pssb = (strstreambuf*) is1.base_ios.sb;
    ok(pis == &is1, "wrong return, expected %p got %p\n", &is1, pis);
    ok(is1.extract_delim == 0, "expected 0 got %d\n", is1.extract_delim);
    ok(is1.count == 0, "expected 0 got %d\n", is1.count);
    ok(is1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, is1.base_ios.sb);
    ok(is1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.state);
    ok(is1.base_ios.delbuf == 1, "expected 1 got %d\n", is1.base_ios.delbuf);
    ok(is1.base_ios.tie == is2.base_ios.tie, "expected %p got %p\n", is2.base_ios.tie, is1.base_ios.tie);
    ok(is1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, is1.base_ios.flags);
    ok(is1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.precision);
    ok(is1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", is1.base_ios.fill);
    ok(is1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.width);
    ok(is1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer, "wrong buffer end, expected %p got %p\n", buffer, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pssb->base.pbase);
    ok(pssb->base.pptr == NULL, "wrong put pointer, expected %p got %p\n", NULL, pssb->base.pptr);
    ok(pssb->base.epptr == NULL, "wrong put end, expected %p got %p\n", NULL, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_istrstream_dtor, &is1.base_ios);
    ok(is1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, is1.base_ios.sb);
    ok(is1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);

    is1.extract_delim = is1.count = 0xcdcdcdcd;
    memset(&is1.base_ios, 0xcd, sizeof(ios));
    pis = call_func3(p_istrstream_copy_ctor, &is2, &is1, TRUE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, is2.base_ios.state);
    ok(is2.base_ios.delbuf == 0, "expected 0 got %d\n", is2.base_ios.delbuf);
    ok(is2.base_ios.tie == is1.base_ios.tie, "expected %p got %p\n", is1.base_ios.tie, is2.base_ios.tie);
    ok(is2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, is2.base_ios.flags);
    ok(is2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", is2.base_ios.precision);
    ok(is2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", is2.base_ios.fill);
    ok(is2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", is2.base_ios.width);
    ok(is2.base_ios.do_lock == -1, "expected -1 got %d\n", is2.base_ios.do_lock);
    call_func1(p_istrstream_vbase_dtor, &is2);
    ok(is2.base_ios.sb == NULL, "expected %p got %p\n", NULL, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, is2.base_ios.state);
    is2.extract_delim = is2.count = 0xabababab;
    memset(&is2.base_ios, 0xab, sizeof(ios));
    is2.base_ios.delbuf = 0;
    pos = is2.base_ios.tie;
    pis = call_func3(p_istrstream_copy_ctor, &is2, &is1, FALSE);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0, "expected 0 got %d\n", is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.state);
    ok(is2.base_ios.delbuf == 0, "expected 0 got %d\n", is2.base_ios.delbuf);
    ok(is2.base_ios.tie == pos, "expected %p got %p\n", pos, is2.base_ios.tie);
    ok(is2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, is2.base_ios.flags);
    ok(is2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.precision);
    ok(is2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", is2.base_ios.fill);
    ok(is2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.width);
    ok(is2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.do_lock);
    call_func1(p_istrstream_dtor, &is2.base_ios);

    /* assignment */
    is2.extract_delim = is2.count = 0xabababab;
    memset(&is2.base_ios, 0xab, sizeof(ios));
    is2.base_ios.delbuf = 0;
    pis = call_func2(p_istrstream_assign, &is2, &is1);
    ok(pis == &is2, "wrong return, expected %p got %p\n", &is2, pis);
    ok(is2.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, is2.extract_delim);
    ok(is2.count == 0, "expected 0 got %d\n", is2.count);
    ok(is2.base_ios.sb == is1.base_ios.sb, "expected %p got %p\n", is1.base_ios.sb, is2.base_ios.sb);
    ok(is2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, is2.base_ios.state);
    ok(is2.base_ios.delbuf == 0, "expected 0 got %d\n", is2.base_ios.delbuf);
    ok(is2.base_ios.tie == NULL, "expected %p got %p\n", NULL, is2.base_ios.tie);
    ok(is2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, is2.base_ios.flags);
    ok(is2.base_ios.precision == 6, "expected 6 got %d\n", is2.base_ios.precision);
    ok(is2.base_ios.fill == ' ', "expected 32 got %d\n", is2.base_ios.fill);
    ok(is2.base_ios.width == 0, "expected 0 got %d\n", is2.base_ios.width);
    ok(is2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, is2.base_ios.do_lock);
}

static void test_iostream(void)
{
    iostream ios1, ios2, *pios;
    ostream *pos;
    streambuf sb;

    memset(&ios1, 0xab, sizeof(iostream));
    memset(&ios2, 0xab, sizeof(iostream));

    /* constructors/destructors */
    pios = call_func3(p_iostream_sb_ctor, &ios1, NULL, TRUE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0, "expected 0 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    call_func1(p_iostream_vbase_dtor, &ios1);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    ios1.base_ios.delbuf = 0;
    pios = call_func3(p_iostream_sb_ctor, &ios1, NULL, FALSE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == (0xabababab|IOSTATE_badbit), "expected %d got %d\n",
        0xabababab|IOSTATE_badbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0, "expected 0 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    call_func1(p_iostream_dtor, &ios1.base_ios);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func3(p_iostream_sb_ctor, &ios1, &sb, TRUE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == &sb, "expected %p got %p\n", &sb, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0, "expected 0 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    call_func1(p_iostream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    ios1.base_ios.delbuf = 0;
    pios = call_func3(p_iostream_sb_ctor, &ios1, &sb, FALSE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == &sb, "expected %p got %p\n", &sb, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0, "expected 0 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    call_func1(p_iostream_dtor, &ios1.base_ios);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func2(p_iostream_ctor, &ios1, TRUE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0, "expected 0 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    call_func1(p_iostream_vbase_dtor, &ios1);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    pios = call_func2(p_iostream_ctor, &ios1, FALSE);
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb == ios2.base_ios.sb, "expected %p got %p\n", ios2.base_ios.sb, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    call_func1(p_iostream_dtor, &ios1.base_ios);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios2, 0xcd, sizeof(iostream));
    pios = call_func3(p_iostream_copy_ctor, &ios2, &ios1, TRUE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == -1, "expected -1 got %d\n", ios2.base_ios.do_lock);
    call_func1(p_iostream_vbase_dtor, &ios2);
    ios1.base_ios.sb = NULL;
    memset(&ios2, 0xcd, sizeof(iostream));
    pios = call_func3(p_iostream_copy_ctor, &ios2, &ios1, TRUE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == -1, "expected -1 got %d\n", ios2.base_ios.do_lock);
    call_func1(p_iostream_vbase_dtor, &ios2);
    ios1.base_ios.sb = &sb;
    ios2.base1.extract_delim = ios2.base1.count = 0xcdcdcdcd;
    ios2.base2.unknown = 0xcdcdcdcd;
    memset(&ios2.base_ios, 0xcd, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pos = ios2.base_ios.tie;
    pios = call_func3(p_iostream_copy_ctor, &ios2, &ios1, FALSE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == &sb, "expected %p got %p\n", &sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == pos, "expected %p got %p\n", pos, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.precision);
    ok(ios2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.do_lock);
    call_func1(p_iostream_dtor, &ios2.base_ios);

    /* assignment */
    ios2.base1.extract_delim = ios2.base1.count = 0xcdcdcdcd;
    ios2.base2.unknown = 0xcdcdcdcd;
    ios2.base_ios.sb = (streambuf*) 0xcdcdcdcd;
    ios2.base_ios.state = 0xcdcdcdcd;
    pios = call_func2(p_iostream_assign_sb, &ios2, &sb);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base2.unknown);
    ok(ios2.base_ios.sb == &sb, "expected %p got %p\n", &sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0, "expected 0 got %x\n", ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.do_lock);
    ios2.base1.count = 0xcdcdcdcd;
    memset(&ios2.base_ios, 0xcd, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pios = call_func2(p_iostream_assign_sb, &ios2, NULL);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base2.unknown);
    ok(ios2.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0, "expected 0 got %x\n", ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.do_lock);
    ios2.base1.count = 0xcdcdcdcd;
    memset(&ios2.base_ios, 0xcd, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    if (0) /* crashes on native */
        pios = call_func2(p_iostream_assign, &ios2, NULL);
    pios = call_func2(p_iostream_assign, &ios2, &ios1);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base2.unknown);
    ok(ios2.base_ios.sb == &sb, "expected %p got %p\n", &sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0, "expected 0 got %x\n", ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xcdcdcdcd, "expected %d got %d\n", 0xcdcdcdcd, ios2.base_ios.do_lock);
}

static void test_ifstream(void)
{
    const char *filename = "ifstream_test";
    istream ifs, ifs_copy, *pifs;
    streambuf *psb;
    filebuf *pfb;
    char buffer[64];
    char st[8];
    int fd, ret, i;

    memset(&ifs, 0xab, sizeof(istream));

    /* constructors/destructors */
    pifs = call_func2(p_ifstream_ctor, &ifs, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(ifs.extract_delim == 0, "expected 0 got %d\n", ifs.extract_delim);
    ok(ifs.count == 0, "expected 0 got %d\n", ifs.count);
    ok(ifs.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ifs.base_ios.sb);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(ifs.base_ios.delbuf == 1, "expected 1 got %d\n", ifs.base_ios.delbuf);
    ok(ifs.base_ios.tie == NULL, "expected %p got %p\n", NULL, ifs.base_ios.tie);
    ok(ifs.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ifs.base_ios.flags);
    ok(ifs.base_ios.precision == 6, "expected 6 got %d\n", ifs.base_ios.precision);
    ok(ifs.base_ios.fill == ' ', "expected 32 got %d\n", ifs.base_ios.fill);
    ok(ifs.base_ios.width == 0, "expected 0 got %d\n", ifs.base_ios.width);
    ok(ifs.base_ios.do_lock == -1, "expected -1 got %d\n", ifs.base_ios.do_lock);
    ok(pfb->fd == -1, "wrong fd, expected -1 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == -1, "wrong fd, expected 0 got %d\n", pfb->fd);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    pifs = call_func3(p_ifstream_fd_ctor, &ifs, 42, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(ifs.base_ios.delbuf == 1, "expected 1 got %d\n", ifs.base_ios.delbuf);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 42, "wrong fd, expected 42 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);

    pifs = call_func3(p_ifstream_copy_ctor, &ifs_copy, &ifs, TRUE);
    pfb = (filebuf*) ifs_copy.base_ios.sb;
    ok(pifs == &ifs_copy, "wrong return, expected %p got %p\n", &ifs_copy, pifs);
    ok(ifs_copy.base_ios.sb == ifs.base_ios.sb, "expected shared streambuf\n");
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(ifs_copy.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs_copy.base_ios.state);

    call_func1(p_ifstream_vbase_dtor, &ifs_copy);
    call_func1(p_ifstream_dtor, &ifs.base_ios);

    pifs = call_func5(p_ifstream_buffer_ctor, &ifs, 53, buffer, ARRAY_SIZE(buffer), TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(ifs.base_ios.delbuf == 1, "expected 1 got %d\n", ifs.base_ios.delbuf);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pfb->base.base);
    ok(pfb->base.ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), pfb->base.ebuf);
    ok(pfb->fd == 53, "wrong fd, expected 53 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_ifstream_dtor, &ifs.base_ios);

    pifs = call_func5(p_ifstream_buffer_ctor, &ifs, 64, NULL, 0, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(ifs.base_ios.delbuf == 1, "expected 1 got %d\n", ifs.base_ios.delbuf);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 64, "wrong fd, expected 64 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    pifs = call_func5(p_ifstream_open_ctor, &ifs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(ifs.base_ios.delbuf == 1, "expected 1 got %d\n", ifs.base_ios.delbuf);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(pfb->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base != NULL, "wrong buffer, expected not %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf != NULL, "wrong ebuf, expected not %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd != -1, "wrong fd, expected not -1 got %d\n", pfb->fd);
    fd = pfb->fd;
    ok(pfb->close == 1, "wrong value, expected 1 got %d\n", pfb->close);
    call_func1(p_ifstream_vbase_dtor, &ifs);
    ok(_close(fd) == -1, "expected ifstream to close opened file\n");

    /* setbuf */
    call_func5(p_ifstream_buffer_ctor, &ifs, -1, NULL, 0, TRUE);
    ok(ifs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pfb->base.unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);

    psb = call_func3(p_ifstream_setbuf, &ifs, buffer, ARRAY_SIZE(buffer));
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, NULL, 0);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    call_func2(p_ifstream_ctor, &ifs, TRUE);
    ok(ifs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, buffer, ARRAY_SIZE(buffer));
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, NULL, 0);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, buffer + 8, ARRAY_SIZE(buffer) - 8);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, buffer + 8, 0);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, buffer + 4, ARRAY_SIZE(buffer) - 4);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    psb = call_func3(p_ifstream_setbuf, &ifs, NULL, 5);
    ok(psb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, psb);
    ok(ifs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    /* setbuf - seems to be a nop and always return NULL in those other cases */
    pifs = call_func5(p_ifstream_buffer_ctor, &ifs, 42, NULL, 0, TRUE);
    ok(ifs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);

    ifs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ifstream_setbuf, &ifs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ifs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    ifs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ifstream_setbuf, &ifs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ifs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_ifstream_vbase_dtor, &ifs);

    pifs = call_func5(p_ifstream_open_ctor, &ifs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    ifs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ifstream_setbuf, &ifs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ifs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->ebuf != NULL, "wrong ebuf value, expected NULL got %p\n", ifs.base_ios.sb->ebuf);
    ok(ifs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    ifs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_ifstream_setbuf, &ifs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(ifs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->base != buffer, "wrong base value, expected not %p got %p\n", buffer, ifs.base_ios.sb->base);
    ok(ifs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", ifs.base_ios.sb->unbuffered);
    ok(ifs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", ifs.base_ios.sb->allocated);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_ifstream_vbase_dtor, &ifs);

    /* attach */
    pifs = call_func2(p_ifstream_ctor, &ifs, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    call_func2(p_ifstream_attach, &ifs, 42);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "attaching on vanilla stream set some state bits\n");
    fd = (int) call_func1(p_ifstream_fd, &ifs);
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    ok(pfb->close == 0, "wrong close value, expected 0 got %d\n", pfb->close);
    ifs.base_ios.state = IOSTATE_eofbit;
    call_func2(p_ifstream_attach, &ifs, 53);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    /* fd */
    pifs = call_func2(p_ifstream_ctor, &ifs, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    fd = (int) call_func1(p_ifstream_fd, &ifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(fd == -1, "wrong fd, expected -1 but got %d\n", fd);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    pifs = call_func5(p_ifstream_open_ctor, &ifs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    pfb = (filebuf*) ifs.base_ios.sb;
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    fd = (int) call_func1(p_ifstream_fd, &ifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ok(fd == pfb->fd, "wrong fd, expected %d but got %d\n", pfb->fd, fd);

    /* rdbuf */
    pfb = (filebuf*) call_func1(p_ifstream_rdbuf, &ifs);
    ok((streambuf*) pfb == ifs.base_ios.sb, "wrong return, expected %p got %p\n", ifs.base_ios.sb, pfb);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    /* setmode */
    ret = (int) call_func2(p_ifstream_setmode, &ifs, filebuf_binary);
    ok(ret == filebuf_text, "wrong return, expected %d got %d\n", filebuf_text, ret);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ret = (int) call_func2(p_ifstream_setmode, &ifs, filebuf_binary);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ret = (int) call_func2(p_ifstream_setmode, &ifs, filebuf_text);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);
    ret = (int) call_func2(p_ifstream_setmode, &ifs, 0x9000);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ifs.base_ios.state);

    /* close && is_open */
    ok((int) call_func1(p_ifstream_is_open, &ifs) == 1, "expected ifstream to be open\n");
    ifs.base_ios.state = IOSTATE_eofbit | IOSTATE_failbit;
    call_func1(p_ifstream_close, &ifs);
    ok(ifs.base_ios.state == IOSTATE_goodbit, "close did not clear state = %d\n", ifs.base_ios.state);
    ifs.base_ios.state = IOSTATE_eofbit;
    call_func1(p_ifstream_close, &ifs);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "close on a closed stream did not set failbit\n");
    ok((int) call_func1(p_ifstream_is_open, &ifs) == 0, "expected ifstream to not be open\n");
    ok(_close(fd) == -1, "expected close to close the opened file\n");

    /* open */
    ifs.base_ios.state = IOSTATE_eofbit;
    call_func4(p_ifstream_open, &ifs, filename, OPENMODE_in, filebuf_openprot);
    fd = (int) call_func1(p_ifstream_fd, &ifs);
    ok(fd != -1, "wrong fd, expected not -1 got %d\n", fd);
    ok(ifs.base_ios.state == IOSTATE_eofbit, "open did not succeed\n");
    call_func4(p_ifstream_open, &ifs, filename, OPENMODE_in, filebuf_openprot);
    ok(ifs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "second open did not set failbit\n");
    call_func1(p_ifstream_close, &ifs);

    /* write something we can read later */
    fd = _open(filename, _O_TRUNC|_O_CREAT|_O_RDWR, _S_IWRITE);
    ok(fd != -1, "_open failed\n");
    ok(_write(fd, "test 12", 7) == 7, "_write failed\n");
    ok(_close(fd) == 0, "_close failed\n");

    /* integration with parent istream - reading */
    ifs.base_ios.state = IOSTATE_goodbit; /* open doesn't clear the state */
    call_func4(p_ifstream_open, &ifs, filename, OPENMODE_out, filebuf_openprot); /* make sure that OPENMODE_in is implicit */
    memset(st, 'A', sizeof(st));
    st[7] = 0;
    pifs = call_func2(p_istream_read_str, &ifs, st);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(!strcmp(st, "test"), "expected 'test' got '%s'\n", st);

    i = 12345;
    pifs = call_func2(p_istream_read_int, pifs, &i);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(i == 12, "expected 12 got %d\n", i);
    call_func1(p_ifstream_vbase_dtor, &ifs);

    /* make sure that OPENMODE_in is implicit with open_ctor */
    pifs = call_func5(p_ifstream_open_ctor, &ifs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    memset(st, 'A', sizeof(st));
    st[7] = 0;
    pifs = call_func2(p_istream_read_str, &ifs, st);
    ok(pifs == &ifs, "wrong return, expected %p got %p\n", &ifs, pifs);
    ok(!strcmp(st, "test"), "expected 'test' got '%s'\n", st);
    call_func1(p_ifstream_vbase_dtor, &ifs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);
}

static void test_strstream(void)
{
    iostream ios1, ios2, *pios;
    ostream *pos;
    strstreambuf *pssb;
    char buffer[32];

    memset(&ios1, 0xab, sizeof(iostream));
    memset(&ios2, 0xab, sizeof(iostream));

    /* constructors/destructors */
    pios = call_func2(p_strstream_ctor, &ios1, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 1, "expected 1, got %d\n", pssb->dynamic);
    ok(pssb->increase == 1, "expected 1, got %d\n", pssb->increase);
    ok(pssb->constant == 0, "expected 0, got %d\n", pssb->constant);
    ok(pssb->f_alloc == NULL, "expected %p, got %p\n", NULL, pssb->f_alloc);
    ok(pssb->f_free == NULL, "expected %p, got %p\n", NULL, pssb->f_free);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pssb->base.base);
    ok(pssb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pssb->base.ebuf);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    ios1.base_ios.delbuf = 0;
    pios = call_func2(p_strstream_ctor, &ios1, FALSE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    ok(pssb->dynamic == 1, "expected 1, got %d\n", pssb->dynamic);
    ok(pssb->increase == 1, "expected 1, got %d\n", pssb->increase);
    ok(pssb->constant == 0, "expected 0, got %d\n", pssb->constant);
    ok(pssb->f_alloc == NULL, "expected %p, got %p\n", NULL, pssb->f_alloc);
    ok(pssb->f_free == NULL, "expected %p, got %p\n", NULL, pssb->f_free);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pssb->base.base);
    ok(pssb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pssb->base.ebuf);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_dtor, &ios1.base_ios);
    ok(ios1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);

    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, 32, 0, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 32, "wrong buffer end, expected %p got %p\n", buffer + 32, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 32, "wrong put end, expected %p got %p\n", buffer + 32, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, 16, OPENMODE_in, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 16, "wrong buffer end, expected %p got %p\n", buffer + 16, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 16, "wrong put end, expected %p got %p\n", buffer + 16, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, -1, OPENMODE_in|OPENMODE_out, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == (char *)((ULONG_PTR)buffer + 0x7fffffff) || pssb->base.ebuf == (char*) -1,
        "wrong buffer end, expected %p + 0x7fffffff or -1, got %p\n", buffer, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == (char *)((ULONG_PTR)buffer + 0x7fffffff) || pssb->base.epptr == (char*) -1,
        "wrong buffer end, expected %p + 0x7fffffff or -1, got %p\n", buffer, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    ios1.base_ios.delbuf = 0;
    strcpy(buffer, "Test");
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, 0, OPENMODE_in|OPENMODE_app, FALSE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 4, "wrong buffer end, expected %p got %p\n", buffer + 4, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 4, "wrong put end, expected %p got %p\n", buffer + 4, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_dtor, &ios1.base_ios);
    ok(ios1.base_ios.sb == &pssb->base, "expected %p got %p\n", &pssb->base, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    call_func1(p_strstreambuf_dtor, pssb);
    p_operator_delete(pssb);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, 16, OPENMODE_ate, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 16, "wrong buffer end, expected %p got %p\n", buffer + 16, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer, "wrong put pointer, expected %p got %p\n", buffer, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 16, "wrong put end, expected %p got %p\n", buffer + 16, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    pios = call_func5(p_strstream_buffer_ctor, &ios1, buffer, 16, OPENMODE_out|OPENMODE_ate, TRUE);
    pssb = (strstreambuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pssb->dynamic == 0, "expected 0, got %d\n", pssb->dynamic);
    ok(pssb->constant == 1, "expected 1, got %d\n", pssb->constant);
    ok(pssb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pssb->base.allocated);
    ok(pssb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pssb->base.unbuffered);
    ok(pssb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pssb->base.base);
    ok(pssb->base.ebuf == buffer + 16, "wrong buffer end, expected %p got %p\n", buffer + 16, pssb->base.ebuf);
    ok(pssb->base.eback == buffer, "wrong get base, expected %p got %p\n", buffer, pssb->base.eback);
    ok(pssb->base.gptr == buffer, "wrong get pointer, expected %p got %p\n", buffer, pssb->base.gptr);
    ok(pssb->base.egptr == buffer, "wrong get end, expected %p got %p\n", buffer, pssb->base.egptr);
    ok(pssb->base.pbase == buffer, "wrong put base, expected %p got %p\n", buffer, pssb->base.pbase);
    ok(pssb->base.pptr == buffer + 4, "wrong put pointer, expected %p got %p\n", buffer + 4, pssb->base.pptr);
    ok(pssb->base.epptr == buffer + 16, "wrong put end, expected %p got %p\n", buffer + 16, pssb->base.epptr);
    ok(pssb->base.do_lock == -1, "expected -1 got %d\n", pssb->base.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);

    ios1.base1.extract_delim = ios1.base1.count = 0xcdcdcdcd;
    ios1.base2.unknown = 0xcdcdcdcd;
    memset(&ios1.base_ios, 0xcd, sizeof(ios));
    pios = call_func3(p_strstream_copy_ctor, &ios2, &ios1, TRUE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == ios1.base_ios.tie, "expected %p got %p\n", ios1.base_ios.tie, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == -1, "expected -1 got %d\n", ios2.base_ios.do_lock);
    call_func1(p_strstream_vbase_dtor, &ios2);
    ios2.base1.extract_delim = ios2.base1.count = 0xabababab;
    ios2.base2.unknown = 0xabababab;
    memset(&ios2.base_ios, 0xab, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pos = ios2.base_ios.tie;
    pios = call_func3(p_strstream_copy_ctor, &ios2, &ios1, FALSE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == pos, "expected %p got %p\n", pos, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.precision);
    ok(ios2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.do_lock);
    call_func1(p_strstream_dtor, &ios2.base_ios);

    /* assignment */
    ios2.base1.extract_delim = ios2.base1.count = 0xabababab;
    ios2.base2.unknown = 0xabababab;
    memset(&ios2.base_ios, 0xab, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pios = call_func2(p_strstream_assign, &ios2, &ios1);
    ok(ios2.base1.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0, "expected 0 got %x\n", ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.do_lock);
}

static void test_stdiostream(void)
{
    iostream ios1, ios2, *pios;
    ostream *pos;
    stdiobuf *pstb;
    FILE *file;
    const char filename[] = "stdiostream_test";

    memset(&ios1, 0xab, sizeof(iostream));
    memset(&ios2, 0xab, sizeof(iostream));

    file = fopen(filename, "w+");
    ok(file != NULL, "Couldn't open the file named '%s'\n", filename);

    /* constructors/destructors */
    pios = call_func3(p_stdiostream_file_ctor, &ios1, file, TRUE);
    pstb = (stdiobuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pstb->file == file, "expected %p, got %p\n", file, pstb->file);
    ok(pstb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pstb->base.allocated);
    ok(pstb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pstb->base.unbuffered);
    ok(pstb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pstb->base.base);
    ok(pstb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pstb->base.ebuf);
    ok(pstb->base.do_lock == -1, "expected -1 got %d\n", pstb->base.do_lock);
    call_func1(p_stdiostream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);
    ios1.base1.extract_delim = ios1.base1.count = 0xabababab;
    ios1.base2.unknown = 0xabababab;
    memset(&ios1.base_ios, 0xab, sizeof(ios));
    ios1.base_ios.delbuf = 0;
    pios = call_func3(p_stdiostream_file_ctor, &ios1, file, FALSE);
    pstb = (stdiobuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == ios2.base_ios.tie, "expected %p got %p\n", ios2.base_ios.tie, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.precision);
    ok(ios1.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.do_lock);
    ok(pstb->file == file, "expected %p, got %p\n", file, pstb->file);
    ok(pstb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pstb->base.allocated);
    ok(pstb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pstb->base.unbuffered);
    ok(pstb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pstb->base.base);
    ok(pstb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pstb->base.ebuf);
    ok(pstb->base.do_lock == -1, "expected -1 got %d\n", pstb->base.do_lock);
    call_func1(p_stdiostream_dtor, &ios1.base_ios);
    ok(ios1.base_ios.sb == &pstb->base, "expected %p got %p\n", &pstb->base, ios1.base_ios.sb);
    ok(ios1.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios1.base_ios.state);
    call_func1(p_stdiobuf_dtor, pstb);
    p_operator_delete(pstb);
    memset(&ios1, 0xab, sizeof(iostream));
    pios = call_func3(p_stdiostream_file_ctor, &ios1, NULL, TRUE);
    pstb = (stdiobuf*) ios1.base_ios.sb;
    ok(pios == &ios1, "wrong return, expected %p got %p\n", &ios1, pios);
    ok(ios1.base1.extract_delim == 0, "expected 0 got %d\n", ios1.base1.extract_delim);
    ok(ios1.base1.count == 0, "expected 0 got %d\n", ios1.base1.count);
    ok(ios1.base2.unknown == 0, "expected 0 got %d\n", ios1.base2.unknown);
    ok(ios1.base_ios.sb != NULL, "expected not %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios1.base_ios.state);
    ok(ios1.base_ios.delbuf == 1, "expected 1 got %d\n", ios1.base_ios.delbuf);
    ok(ios1.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios1.base_ios.tie);
    ok(ios1.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, ios1.base_ios.flags);
    ok(ios1.base_ios.precision == 6, "expected 6 got %d\n", ios1.base_ios.precision);
    ok(ios1.base_ios.fill == ' ', "expected 32 got %d\n", ios1.base_ios.fill);
    ok(ios1.base_ios.width == 0, "expected 0 got %d\n", ios1.base_ios.width);
    ok(ios1.base_ios.do_lock == -1, "expected -1 got %d\n", ios1.base_ios.do_lock);
    ok(pstb->file == NULL, "expected %p, got %p\n", NULL, pstb->file);
    ok(pstb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pstb->base.allocated);
    ok(pstb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pstb->base.unbuffered);
    ok(pstb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pstb->base.base);
    ok(pstb->base.ebuf == NULL, "wrong buffer end, expected %p got %p\n", NULL, pstb->base.ebuf);
    ok(pstb->base.do_lock == -1, "expected -1 got %d\n", pstb->base.do_lock);
    call_func1(p_stdiostream_vbase_dtor, &ios1);
    ok(ios1.base_ios.sb == NULL, "expected %p got %p\n", NULL, ios1.base_ios.sb);
    ok(ios1.base_ios.state == IOSTATE_badbit, "expected %d got %d\n", IOSTATE_badbit, ios1.base_ios.state);

    ios1.base1.extract_delim = ios1.base1.count = 0xcdcdcdcd;
    ios1.base2.unknown = 0xcdcdcdcd;
    memset(&ios1.base_ios, 0xcd, sizeof(ios));
    pios = call_func3(p_stdiostream_copy_ctor, &ios2, &ios1, TRUE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == 0xcdcdcdc9, "expected %d got %d\n", 0xcdcdcdc9, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == ios1.base_ios.tie, "expected %p got %p\n", ios1.base_ios.tie, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0xcdcdcdcd, "expected %x got %x\n", 0xcdcdcdcd, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == (char) 0xcd, "expected -51 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == -1, "expected -1 got %d\n", ios2.base_ios.do_lock);
    call_func1(p_stdiostream_vbase_dtor, &ios2);
    ios2.base1.extract_delim = ios2.base1.count = 0xabababab;
    ios2.base2.unknown = 0xabababab;
    memset(&ios2.base_ios, 0xab, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pos = ios2.base_ios.tie;
    pios = call_func3(p_stdiostream_copy_ctor, &ios2, &ios1, FALSE);
    ok(pios == &ios2, "wrong return, expected %p got %p\n", &ios2, pios);
    ok(ios2.base1.extract_delim == 0, "expected 0 got %d\n", ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0, "expected 0 got %d\n", ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == pos, "expected %p got %p\n", pos, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0xabababab, "expected %x got %x\n", 0xabababab, ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.precision);
    ok(ios2.base_ios.fill == (char) 0xab, "expected -85 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.do_lock);
    call_func1(p_stdiostream_dtor, &ios2.base_ios);

    /* assignment */
    ios2.base1.extract_delim = ios2.base1.count = 0xabababab;
    ios2.base2.unknown = 0xabababab;
    memset(&ios2.base_ios, 0xab, sizeof(ios));
    ios2.base_ios.delbuf = 0;
    pios = call_func2(p_stdiostream_assign, &ios2, &ios1);
    ok(ios2.base1.extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base1.extract_delim);
    ok(ios2.base1.count == 0, "expected 0 got %d\n", ios2.base1.count);
    ok(ios2.base2.unknown == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base2.unknown);
    ok(ios2.base_ios.sb == ios1.base_ios.sb, "expected %p got %p\n", ios1.base_ios.sb, ios2.base_ios.sb);
    ok(ios2.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, ios2.base_ios.state);
    ok(ios2.base_ios.delbuf == 0, "expected 0 got %d\n", ios2.base_ios.delbuf);
    ok(ios2.base_ios.tie == NULL, "expected %p got %p\n", NULL, ios2.base_ios.tie);
    ok(ios2.base_ios.flags == 0, "expected 0 got %x\n", ios2.base_ios.flags);
    ok(ios2.base_ios.precision == 6, "expected 6 got %d\n", ios2.base_ios.precision);
    ok(ios2.base_ios.fill == ' ', "expected 32 got %d\n", ios2.base_ios.fill);
    ok(ios2.base_ios.width == 0, "expected 0 got %d\n", ios2.base_ios.width);
    ok(ios2.base_ios.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios2.base_ios.do_lock);

    fclose(file);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);
}

static void test_Iostream_init(void)
{
    ios ios_obj;
    ostream *pos;
    streambuf *psb;
    void *pinit;

    memset(&ios_obj, 0xab, sizeof(ios));
    psb = ios_obj.sb;
    pinit = call_func3(p_Iostream_init_ios_ctor, NULL, &ios_obj, 0);
    ok(pinit == NULL, "wrong return, expected %p got %p\n", NULL, pinit);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.state);
    ok(ios_obj.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[0]);
    ok(ios_obj.special[1] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[1]);
    ok(ios_obj.delbuf == 1, "expected 1 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == p_cout, "expected %p got %p\n", p_cout, ios_obj.tie);
    ok(ios_obj.flags == 0xabababab, "expected %d got %x\n", 0xabababab, ios_obj.flags);
    ok(ios_obj.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.precision);
    ok(ios_obj.fill == (char) 0xab, "expected -85 got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.width);
    ok(ios_obj.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.do_lock);

    memset(&ios_obj, 0xab, sizeof(ios));
    pos = ios_obj.tie;
    pinit = call_func3(p_Iostream_init_ios_ctor, NULL, &ios_obj, -1);
    ok(pinit == NULL, "wrong return, expected %p got %p\n", NULL, pinit);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.state);
    ok(ios_obj.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[0]);
    ok(ios_obj.special[1] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[1]);
    ok(ios_obj.delbuf == 1, "expected 1 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == pos, "expected %p got %p\n", pos, ios_obj.tie);
    ok(ios_obj.flags == 0xabababab, "expected %d got %x\n", 0xabababab, ios_obj.flags);
    ok(ios_obj.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.precision);
    ok(ios_obj.fill == (char) 0xab, "expected -85 got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.width);
    ok(ios_obj.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.do_lock);

    memset(&ios_obj, 0xab, sizeof(ios));
    pinit = call_func3(p_Iostream_init_ios_ctor, NULL, &ios_obj, -100);
    ok(pinit == NULL, "wrong return, expected %p got %p\n", NULL, pinit);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.state);
    ok(ios_obj.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[0]);
    ok(ios_obj.special[1] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[1]);
    ok(ios_obj.delbuf == 1, "expected 1 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == pos, "expected %p got %p\n", pos, ios_obj.tie);
    ok(ios_obj.flags == 0xabababab, "expected %d got %x\n", 0xabababab, ios_obj.flags);
    ok(ios_obj.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.precision);
    ok(ios_obj.fill == (char) 0xab, "expected -85 got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.width);
    ok(ios_obj.do_lock == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.do_lock);

    if (0) /* crashes on native */
        pinit = call_func3(p_Iostream_init_ios_ctor, NULL, NULL, -1);

    memset(&ios_obj, 0xab, sizeof(ios));
    ios_obj.flags = 0xcdcdcdcd;
    ios_obj.do_lock = 0x34343434;
    pinit = call_func3(p_Iostream_init_ios_ctor, (void*) 0xdeadbeef, &ios_obj, 1);
    ok(pinit == (void*) 0xdeadbeef, "wrong return, expected %p got %p\n", (void*) 0xdeadbeef, pinit);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.state);
    ok(ios_obj.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[0]);
    ok(ios_obj.special[1] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[1]);
    ok(ios_obj.delbuf == 1, "expected 1 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == p_cout, "expected %p got %p\n", p_cout, ios_obj.tie);
    ok(ios_obj.flags == (0xcdcdcdcd|FLAGS_unitbuf), "expected %d got %x\n",
        0xcdcdcdcd|FLAGS_unitbuf, ios_obj.flags);
    ok(ios_obj.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.precision);
    ok(ios_obj.fill == (char) 0xab, "expected -85 got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.width);
    ok(ios_obj.do_lock == 0x34343434, "expected %d got %d\n", 0x34343434, ios_obj.do_lock);

    memset(&ios_obj, 0xab, sizeof(ios));
    ios_obj.flags = 0xcdcdcdcd;
    ios_obj.do_lock = 0x34343434;
    pinit = call_func3(p_Iostream_init_ios_ctor, (void*) 0xabababab, &ios_obj, 5);
    ok(pinit == (void*) 0xabababab, "wrong return, expected %p got %p\n", (void*) 0xabababab, pinit);
    ok(ios_obj.sb == psb, "expected %p got %p\n", psb, ios_obj.sb);
    ok(ios_obj.state == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.state);
    ok(ios_obj.special[0] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[0]);
    ok(ios_obj.special[1] == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.special[1]);
    ok(ios_obj.delbuf == 1, "expected 1 got %d\n", ios_obj.delbuf);
    ok(ios_obj.tie == p_cout, "expected %p got %p\n", p_cout, ios_obj.tie);
    ok(ios_obj.flags == (0xcdcdcdcd|FLAGS_unitbuf), "expected %d got %x\n",
        0xcdcdcdcd|FLAGS_unitbuf, ios_obj.flags);
    ok(ios_obj.precision == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.precision);
    ok(ios_obj.fill == (char) 0xab, "expected -85 got %d\n", ios_obj.fill);
    ok(ios_obj.width == 0xabababab, "expected %d got %d\n", 0xabababab, ios_obj.width);
    ok(ios_obj.do_lock == 0x34343434, "expected %d got %d\n", 0x34343434, ios_obj.do_lock);
}

static void test_std_streams(void)
{
    filebuf *pfb_cin = (filebuf*) p_cin->base_ios.sb;
    filebuf *pfb_cout = (filebuf*) p_cout->base_ios.sb;
    filebuf *pfb_cerr = (filebuf*) p_cerr->base_ios.sb;
    filebuf *pfb_clog = (filebuf*) p_clog->base_ios.sb;
    stdiobuf *pstb_cin, *pstb_cout, *pstb_cerr, *pstb_clog;

    ok(p_cin->extract_delim == 0, "expected 0 got %d\n", p_cin->extract_delim);
    ok(p_cin->count == 0, "expected 0 got %d\n", p_cin->count);
    ok(p_cin->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cin->base_ios.state);
    ok(p_cin->base_ios.delbuf == 1, "expected 1 got %d\n", p_cin->base_ios.delbuf);
    ok(p_cin->base_ios.tie == p_cout, "expected %p got %p\n", p_cout, p_cin->base_ios.tie);
    ok(p_cin->base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, p_cin->base_ios.flags);
    ok(p_cin->base_ios.precision == 6, "expected 6 got %d\n", p_cin->base_ios.precision);
    ok(p_cin->base_ios.fill == ' ', "expected 32 got %d\n", p_cin->base_ios.fill);
    ok(p_cin->base_ios.width == 0, "expected 0 got %d\n", p_cin->base_ios.width);
    ok(p_cin->base_ios.do_lock == -1, "expected -1 got %d\n", p_cin->base_ios.do_lock);
    ok(pfb_cin->fd == 0, "wrong fd, expected 0 got %d\n", pfb_cin->fd);
    ok(pfb_cin->close == 0, "wrong value, expected 0 got %d\n", pfb_cin->close);
    ok(pfb_cin->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb_cin->base.allocated);
    ok(pfb_cin->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb_cin->base.unbuffered);

    ok(p_cout->unknown == 0, "expected 0 got %d\n", p_cout->unknown);
    ok(p_cout->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cout->base_ios.state);
    ok(p_cout->base_ios.delbuf == 1, "expected 1 got %d\n", p_cout->base_ios.delbuf);
    ok(p_cout->base_ios.tie == NULL, "expected %p got %p\n", NULL, p_cout->base_ios.tie);
    ok(p_cout->base_ios.flags == 0, "expected 0 got %x\n", p_cout->base_ios.flags);
    ok(p_cout->base_ios.precision == 6, "expected 6 got %d\n", p_cout->base_ios.precision);
    ok(p_cout->base_ios.fill == ' ', "expected 32 got %d\n", p_cout->base_ios.fill);
    ok(p_cout->base_ios.width == 0, "expected 0 got %d\n", p_cout->base_ios.width);
    ok(p_cout->base_ios.do_lock == -1, "expected -1 got %d\n", p_cout->base_ios.do_lock);
    ok(pfb_cout->fd == 1, "wrong fd, expected 1 got %d\n", pfb_cout->fd);
    ok(pfb_cout->close == 0, "wrong value, expected 0 got %d\n", pfb_cout->close);
    ok(pfb_cout->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb_cout->base.allocated);
    ok(pfb_cout->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb_cout->base.unbuffered);

    ok(p_cerr->unknown == 0, "expected 0 got %d\n", p_cerr->unknown);
    ok(p_cerr->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cerr->base_ios.state);
    ok(p_cerr->base_ios.delbuf == 1, "expected 1 got %d\n", p_cerr->base_ios.delbuf);
    ok(p_cerr->base_ios.tie == p_cout, "expected %p got %p\n", p_cout, p_cerr->base_ios.tie);
    ok(p_cerr->base_ios.flags == FLAGS_unitbuf, "expected %x got %x\n", FLAGS_unitbuf, p_cerr->base_ios.flags);
    ok(p_cerr->base_ios.precision == 6, "expected 6 got %d\n", p_cerr->base_ios.precision);
    ok(p_cerr->base_ios.fill == ' ', "expected 32 got %d\n", p_cerr->base_ios.fill);
    ok(p_cerr->base_ios.width == 0, "expected 0 got %d\n", p_cerr->base_ios.width);
    ok(p_cerr->base_ios.do_lock == -1, "expected -1 got %d\n", p_cerr->base_ios.do_lock);
    ok(pfb_cerr->fd == 2, "wrong fd, expected 2 got %d\n", pfb_cerr->fd);
    ok(pfb_cerr->close == 0, "wrong value, expected 0 got %d\n", pfb_cerr->close);
    ok(pfb_cerr->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb_cerr->base.allocated);
    ok(pfb_cerr->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb_cerr->base.unbuffered);

    ok(p_clog->unknown == 0, "expected 0 got %d\n", p_clog->unknown);
    ok(p_clog->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_clog->base_ios.state);
    ok(p_clog->base_ios.delbuf == 1, "expected 1 got %d\n", p_clog->base_ios.delbuf);
    ok(p_clog->base_ios.tie == p_cout, "expected %p got %p\n", p_cout, p_clog->base_ios.tie);
    ok(p_clog->base_ios.flags == 0, "expected 0 got %x\n", p_clog->base_ios.flags);
    ok(p_clog->base_ios.precision == 6, "expected 6 got %d\n", p_clog->base_ios.precision);
    ok(p_clog->base_ios.fill == ' ', "expected 32 got %d\n", p_clog->base_ios.fill);
    ok(p_clog->base_ios.width == 0, "expected 0 got %d\n", p_clog->base_ios.width);
    ok(p_clog->base_ios.do_lock == -1, "expected -1 got %d\n", p_clog->base_ios.do_lock);
    ok(pfb_clog->fd == 2, "wrong fd, expected 2 got %d\n", pfb_clog->fd);
    ok(pfb_clog->close == 0, "wrong value, expected 0 got %d\n", pfb_clog->close);
    ok(pfb_clog->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb_clog->base.allocated);
    ok(pfb_clog->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb_clog->base.unbuffered);

    /* sync_with_stdio */
    ok(*p_ios_sunk_with_stdio == 0, "expected 0 got %d\n", *p_ios_sunk_with_stdio);
    p_cin->extract_delim = p_cin->count = 0xabababab;
    p_cin->base_ios.state = 0xabababab;
    p_cin->base_ios.fill = 0xab;
    p_cin->base_ios.precision = p_cin->base_ios.width = 0xabababab;
    p_cout->unknown = 0xabababab;
    p_cout->base_ios.state = 0xabababab;
    p_cout->base_ios.fill = 0xab;
    p_cout->base_ios.precision = p_cout->base_ios.width = 0xabababab;
    p_cerr->unknown = 0xabababab;
    p_cerr->base_ios.state = 0xabababab;
    p_cerr->base_ios.fill = 0xab;
    p_cerr->base_ios.precision = p_cerr->base_ios.width = 0xabababab;
    p_clog->unknown = 0xabababab;
    p_clog->base_ios.state = 0xabababab;
    p_clog->base_ios.fill = 0xab;
    p_clog->base_ios.precision = p_clog->base_ios.width = 0xabababab;
    p_ios_sync_with_stdio();
    ok(*p_ios_sunk_with_stdio == 1, "expected 1 got %d\n", *p_ios_sunk_with_stdio);

    pstb_cin = (stdiobuf*) p_cin->base_ios.sb;
    ok(p_cin->extract_delim == 0xabababab, "expected %d got %d\n", 0xabababab, p_cin->extract_delim);
    ok(p_cin->count == 0, "expected 0 got %d\n", p_cin->count);
    ok(p_cin->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cin->base_ios.state);
    ok(p_cin->base_ios.delbuf == 1, "expected 1 got %d\n", p_cin->base_ios.delbuf);
    ok(p_cin->base_ios.tie == NULL, "expected %p got %p\n", NULL, p_cin->base_ios.tie);
    ok(p_cin->base_ios.flags == (FLAGS_skipws|FLAGS_stdio), "expected %x got %x\n",
        FLAGS_skipws|FLAGS_stdio, p_cin->base_ios.flags);
    ok(p_cin->base_ios.precision == 6, "expected 6 got %d\n", p_cin->base_ios.precision);
    ok(p_cin->base_ios.fill == ' ', "expected 32 got %d\n", p_cin->base_ios.fill);
    ok(p_cin->base_ios.width == 0, "expected 0 got %d\n", p_cin->base_ios.width);
    ok(p_cin->base_ios.do_lock == -1, "expected -1 got %d\n", p_cin->base_ios.do_lock);
    ok(pstb_cin->file == stdin, "wrong file pointer, expected %p got %p\n", stdin, pstb_cin->file);
    ok(pstb_cin->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pstb_cin->base.allocated);
    ok(pstb_cin->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pstb_cin->base.unbuffered);
    ok(pstb_cin->base.ebuf == NULL, "wrong ebuf pointer, expected %p got %p\n", NULL, pstb_cin->base.eback);
    ok(pstb_cin->base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, pstb_cin->base.eback);
    ok(pstb_cin->base.pbase == NULL, "wrong put base, expected %p got %p\n", NULL, pstb_cin->base.pbase);

    pstb_cout = (stdiobuf*) p_cout->base_ios.sb;
    ok(p_cout->unknown == 0xabababab, "expected %d got %d\n", 0xabababab, p_cout->unknown);
    ok(p_cout->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cout->base_ios.state);
    ok(p_cout->base_ios.delbuf == 1, "expected 1 got %d\n", p_cout->base_ios.delbuf);
    ok(p_cout->base_ios.tie == NULL, "expected %p got %p\n", NULL, p_cout->base_ios.tie);
    ok(p_cout->base_ios.flags == (FLAGS_unitbuf|FLAGS_stdio), "expected %x got %x\n",
        FLAGS_unitbuf|FLAGS_stdio, p_cout->base_ios.flags);
    ok(p_cout->base_ios.precision == 6, "expected 6 got %d\n", p_cout->base_ios.precision);
    ok(p_cout->base_ios.fill == ' ', "expected 32 got %d\n", p_cout->base_ios.fill);
    ok(p_cout->base_ios.width == 0, "expected 0 got %d\n", p_cout->base_ios.width);
    ok(p_cout->base_ios.do_lock == -1, "expected -1 got %d\n", p_cout->base_ios.do_lock);
    ok(pstb_cout->file == stdout, "wrong file pointer, expected %p got %p\n", stdout, pstb_cout->file);
    ok(pstb_cout->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pstb_cout->base.allocated);
    ok(pstb_cout->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pstb_cout->base.unbuffered);
    ok(pstb_cout->base.ebuf == pstb_cout->base.base + 80, "wrong ebuf pointer, expected %p got %p\n",
        pstb_cout->base.base + 80, pstb_cout->base.eback);
    ok(pstb_cout->base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, pstb_cout->base.eback);
    ok(pstb_cout->base.pbase == pstb_cout->base.base, "wrong put base, expected %p got %p\n",
        pstb_cout->base.base, pstb_cout->base.pbase);
    ok(pstb_cout->base.pptr == pstb_cout->base.base, "wrong put pointer, expected %p got %p\n",
        pstb_cout->base.base, pstb_cout->base.pptr);
    ok(pstb_cout->base.epptr == pstb_cout->base.base + 80, "wrong put end, expected %p got %p\n",
        pstb_cout->base.base + 80, pstb_cout->base.epptr);

    pstb_cerr = (stdiobuf*) p_cerr->base_ios.sb;
    ok(p_cerr->unknown == 0xabababab, "expected %d got %d\n", 0xabababab, p_cerr->unknown);
    ok(p_cerr->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_cerr->base_ios.state);
    ok(p_cerr->base_ios.delbuf == 1, "expected 1 got %d\n", p_cerr->base_ios.delbuf);
    ok(p_cerr->base_ios.tie == NULL, "expected %p got %p\n", NULL, p_cerr->base_ios.tie);
    ok(p_cerr->base_ios.flags == (FLAGS_unitbuf|FLAGS_stdio), "expected %x got %x\n",
        FLAGS_unitbuf|FLAGS_stdio, p_cerr->base_ios.flags);
    ok(p_cerr->base_ios.precision == 6, "expected 6 got %d\n", p_cerr->base_ios.precision);
    ok(p_cerr->base_ios.fill == ' ', "expected 32 got %d\n", p_cerr->base_ios.fill);
    ok(p_cerr->base_ios.width == 0, "expected 0 got %d\n", p_cerr->base_ios.width);
    ok(p_cerr->base_ios.do_lock == -1, "expected -1 got %d\n", p_cerr->base_ios.do_lock);
    ok(pstb_cerr->file == stderr, "wrong file pointer, expected %p got %p\n", stderr, pstb_cerr->file);
    ok(pstb_cerr->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pstb_cerr->base.allocated);
    ok(pstb_cerr->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pstb_cerr->base.unbuffered);
    ok(pstb_cerr->base.ebuf == pstb_cerr->base.base + 80, "wrong ebuf pointer, expected %p got %p\n",
        pstb_cerr->base.base + 80, pstb_cerr->base.eback);
    ok(pstb_cerr->base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, pstb_cerr->base.eback);
    ok(pstb_cerr->base.pbase == pstb_cerr->base.base, "wrong put base, expected %p got %p\n",
        pstb_cerr->base.base, pstb_cerr->base.pbase);
    ok(pstb_cerr->base.pptr == pstb_cerr->base.base, "wrong put pointer, expected %p got %p\n",
        pstb_cerr->base.base, pstb_cerr->base.pptr);
    ok(pstb_cerr->base.epptr == pstb_cerr->base.base + 80, "wrong put end, expected %p got %p\n",
        pstb_cerr->base.base + 80, pstb_cerr->base.epptr);

    pstb_clog = (stdiobuf*) p_clog->base_ios.sb;
    ok(p_clog->unknown == 0xabababab, "expected %d got %d\n", 0xabababab, p_clog->unknown);
    ok(p_clog->base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, p_clog->base_ios.state);
    ok(p_clog->base_ios.delbuf == 1, "expected 1 got %d\n", p_clog->base_ios.delbuf);
    ok(p_clog->base_ios.tie == NULL, "expected %p got %p\n", NULL, p_clog->base_ios.tie);
    ok(p_clog->base_ios.flags == FLAGS_stdio, "expected %x got %x\n", FLAGS_stdio, p_clog->base_ios.flags);
    ok(p_clog->base_ios.precision == 6, "expected 6 got %d\n", p_clog->base_ios.precision);
    ok(p_clog->base_ios.fill == ' ', "expected 32 got %d\n", p_clog->base_ios.fill);
    ok(p_clog->base_ios.width == 0, "expected 0 got %d\n", p_clog->base_ios.width);
    ok(p_clog->base_ios.do_lock == -1, "expected -1 got %d\n", p_clog->base_ios.do_lock);
    ok(pstb_clog->file == stderr, "wrong file pointer, expected %p got %p\n", stderr, pstb_clog->file);
    ok(pstb_clog->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pstb_clog->base.allocated);
    ok(pstb_clog->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pstb_clog->base.unbuffered);
    ok(pstb_clog->base.ebuf == pstb_clog->base.base + 512, "wrong ebuf pointer, expected %p got %p\n",
        pstb_clog->base.base + 512, pstb_clog->base.eback);
    ok(pstb_clog->base.eback == NULL, "wrong get base, expected %p got %p\n", NULL, pstb_clog->base.eback);
    ok(pstb_clog->base.pbase == pstb_clog->base.base, "wrong put base, expected %p got %p\n",
        pstb_clog->base.base, pstb_clog->base.pbase);
    ok(pstb_clog->base.pptr == pstb_clog->base.base, "wrong put pointer, expected %p got %p\n",
        pstb_clog->base.base, pstb_clog->base.pptr);
    ok(pstb_clog->base.epptr == pstb_clog->base.base + 512, "wrong put end, expected %p got %p\n",
        pstb_clog->base.base + 512, pstb_clog->base.epptr);

    p_cin->count = 0xabababab;
    p_ios_sync_with_stdio();
    ok(*p_ios_sunk_with_stdio == 1, "expected 1 got %d\n", *p_ios_sunk_with_stdio);
    ok(p_cin->count == 0xabababab, "expected %d got %d\n", 0xabababab, p_cin->count);
    p_ios_sync_with_stdio();
    ok(*p_ios_sunk_with_stdio == 1, "expected 1 got %d\n", *p_ios_sunk_with_stdio);
    ok(p_cin->count == 0xabababab, "expected %d got %d\n", 0xabababab, p_cin->count);
}

static void test_fstream(void)
{
    const char *filename = "fstream_test";
    iostream fs, fs_copy, *pfs;
    char buffer[64];
    streambuf *psb;
    filebuf *pfb;
    ostream *pos;
    istream *pis;
    char st[8];
    int i, ret, fd;

    memset(&fs, 0xab, sizeof(iostream));

    /* constructors/destructors */
    pfs = call_func2(p_fstream_ctor, &fs, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(fs.base1.extract_delim == 0, "expected 0 got %d\n", fs.base1.extract_delim);
    ok(fs.base1.count == 0, "expected 0 got %d\n", fs.base1.count);
    ok(fs.base2.unknown == 0, "expected 0 got %d\n", fs.base2.unknown);
    ok(fs.base_ios.sb != NULL, "expected not %p got %p\n", NULL, fs.base_ios.sb);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(fs.base_ios.tie == NULL, "expected %p got %p\n", NULL, fs.base_ios.tie);
    ok(fs.base_ios.flags == FLAGS_skipws, "expected %x got %x\n", FLAGS_skipws, fs.base_ios.flags);
    ok(fs.base_ios.precision == 6, "expected 6 got %d\n", fs.base_ios.precision);
    ok(fs.base_ios.fill == ' ', "expected 32 got %d\n", fs.base_ios.fill);
    ok(fs.base_ios.width == 0, "expected 0 got %d\n", fs.base_ios.width);
    ok(fs.base_ios.do_lock == -1, "expected -1 got %d\n", fs.base_ios.do_lock);
    ok(pfb->fd == -1, "wrong fd, expected -1 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == -1, "wrong fd, expected 0 got %d\n", pfb->fd);
    call_func1(p_fstream_vbase_dtor, &fs);

    pfs = call_func3(p_fstream_fd_ctor, &fs, 42, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 42, "wrong fd, expected 42 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);

    pfs = call_func3(p_fstream_copy_ctor, &fs_copy, &fs, TRUE);
    pfb = (filebuf*) fs_copy.base_ios.sb;
    ok(pfs == &fs_copy, "wrong return, expected %p got %p\n", &fs_copy, pfs);
    ok(fs_copy.base_ios.sb == fs.base_ios.sb, "expected shared streambuf\n");
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fs_copy.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs_copy.base_ios.state);

    call_func1(p_fstream_vbase_dtor, &fs_copy);
    call_func1(p_fstream_dtor, &fs.base_ios);

    pfs = call_func5(p_fstream_buffer_ctor, &fs, 53, buffer, ARRAY_SIZE(buffer), TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == buffer, "wrong buffer, expected %p got %p\n", buffer, pfb->base.base);
    ok(pfb->base.ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), pfb->base.ebuf);
    ok(pfb->fd == 53, "wrong fd, expected 53 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_fstream_dtor, &fs.base_ios);

    pfs = call_func5(p_fstream_buffer_ctor, &fs, 64, NULL, 0, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(pfb->base.allocated == 0, "wrong allocate value, expected 0 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base == NULL, "wrong buffer, expected %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf == NULL, "wrong ebuf, expected %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd == 64, "wrong fd, expected 64 got %d\n", pfb->fd);
    ok(pfb->close == 0, "wrong value, expected 0 got %d\n", pfb->close);
    call_func1(p_fstream_vbase_dtor, &fs);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(pfb->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base != NULL, "wrong buffer, expected not %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf != NULL, "wrong ebuf, expected not %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd != -1, "wrong fd, expected not -1 got %d\n", pfb->fd);
    fd = pfb->fd;
    ok(pfb->close == 1, "wrong value, expected 1 got %d\n", pfb->close);
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_close(fd) == -1, "expected fstream to close opened file\n");
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(fs.base_ios.delbuf == 1, "expected 1 got %d\n", fs.base_ios.delbuf);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    ok(pfb->base.allocated == 1, "wrong allocate value, expected 1 got %d\n", pfb->base.allocated);
    ok(pfb->base.unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", pfb->base.unbuffered);
    ok(pfb->base.base != NULL, "wrong buffer, expected not %p got %p\n", NULL, pfb->base.base);
    ok(pfb->base.ebuf != NULL, "wrong ebuf, expected not %p got %p\n", NULL, pfb->base.ebuf);
    ok(pfb->fd != -1, "wrong fd, expected not -1 got %d\n", pfb->fd);
    fd = pfb->fd;
    ok(pfb->close == 1, "wrong value, expected 1 got %d\n", pfb->close);
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_close(fd) == -1, "expected fstream to close opened file\n");
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    /* setbuf */
    call_func5(p_fstream_buffer_ctor, &fs, -1, NULL, 0, TRUE);
    ok(fs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);

    psb = call_func3(p_fstream_setbuf, &fs, buffer, ARRAY_SIZE(buffer));
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, NULL, 0);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    call_func1(p_fstream_vbase_dtor, &fs);

    call_func2(p_fstream_ctor, &fs, TRUE);
    ok(fs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, buffer, ARRAY_SIZE(buffer));
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, NULL, 0);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer, "wrong buffer, expected %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, buffer + 8, ARRAY_SIZE(buffer) - 8);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, buffer + 8, 0);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer + 8, "wrong buffer, expected %p got %p\n", buffer + 8, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, buffer + 4, ARRAY_SIZE(buffer) - 4);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    psb = call_func3(p_fstream_setbuf, &fs, NULL, 5);
    ok(psb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, psb);
    ok(fs.base_ios.sb->base == buffer + 4, "wrong buffer, expected %p got %p\n", buffer + 4, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == buffer + ARRAY_SIZE(buffer), "wrong ebuf, expected %p got %p\n", buffer + ARRAY_SIZE(buffer), fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    call_func1(p_fstream_vbase_dtor, &fs);

    /* setbuf - seems to be a nop and always return NULL in those other cases */
    pfs = call_func5(p_fstream_buffer_ctor, &fs, 42, NULL, 0, TRUE);
    ok(fs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);

    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base == NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf == NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 1, "wrong unbuffered value, expected 1 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 0, "wrong allocated value, expected 0 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_fstream_vbase_dtor, &fs);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf != NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->base != buffer, "wrong base value, expected not %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, NULL, 0);
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->ebuf != NULL, "wrong ebuf value, expected NULL got %p\n", fs.base_ios.sb->ebuf);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");

    fs.base_ios.state = IOSTATE_eofbit;
    psb = call_func3(p_fstream_setbuf, &fs, buffer, ARRAY_SIZE(buffer));
    ok(psb == NULL, "wrong return, expected NULL got %p\n", psb);
    ok(fs.base_ios.sb->base != NULL, "wrong base value, expected NULL got %p\n", fs.base_ios.sb->base);
    ok(fs.base_ios.sb->base != buffer, "wrong base value, expected not %p got %p\n", buffer, fs.base_ios.sb->base);
    ok(fs.base_ios.sb->unbuffered == 0, "wrong unbuffered value, expected 0 got %d\n", fs.base_ios.sb->unbuffered);
    ok(fs.base_ios.sb->allocated == 1, "wrong allocated value, expected 1 got %d\n", fs.base_ios.sb->allocated);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    /* attach */
    pfs = call_func2(p_fstream_ctor, &fs, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    call_func2(p_fstream_attach, &fs, 42);
    ok(fs.base_ios.state == IOSTATE_goodbit, "attaching on vanilla stream set some state bits\n");
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    ok(pfb->close == 0, "wrong close value, expected 0 got %d\n", pfb->close);
    fs.base_ios.state = IOSTATE_eofbit;
    call_func2(p_fstream_attach, &fs, 53);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "attaching on already setup stream did not set failbit\n");
    ok(fd == 42, "wrong fd, expected 42 got %d\n", fd);
    call_func1(p_fstream_vbase_dtor, &fs);

    /* fd */
    pfs = call_func2(p_fstream_ctor, &fs, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fd == -1, "wrong fd, expected -1 but got %d\n", fd);
    call_func1(p_fstream_vbase_dtor, &fs);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fd == pfb->fd, "wrong fd, expected %d but got %d\n", pfb->fd, fd);
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    pfs = call_func2(p_fstream_ctor, &fs, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fd == -1, "wrong fd, expected -1 but got %d\n", fd);
    call_func1(p_fstream_vbase_dtor, &fs);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    pfb = (filebuf*) fs.base_ios.sb;
    ok(pfs == &fs, "wrong return, expected %p got %p\n", &fs, pfs);
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ok(fd == pfb->fd, "wrong fd, expected %d but got %d\n", pfb->fd, fd);

    /* rdbuf */
    pfb = (filebuf*) call_func1(p_fstream_rdbuf, &fs);
    ok((streambuf*) pfb == fs.base_ios.sb, "wrong return, expected %p got %p\n", fs.base_ios.sb, pfb);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    /* setmode */
    ret = (int) call_func2(p_fstream_setmode, &fs, filebuf_binary);
    ok(ret == filebuf_text, "wrong return, expected %d got %d\n", filebuf_text, ret);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ret = (int) call_func2(p_fstream_setmode, &fs, filebuf_binary);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ret = (int) call_func2(p_fstream_setmode, &fs, filebuf_text);
    ok(ret == filebuf_binary, "wrong return, expected %d got %d\n", filebuf_binary, ret);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    ret = (int) call_func2(p_fstream_setmode, &fs, 0x9000);
    ok(ret == -1, "wrong return, expected -1 got %d\n", ret);
    ok(fs.base_ios.state == IOSTATE_goodbit, "expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);

    /* close && is_open */
    ok((int) call_func1(p_fstream_is_open, &fs) == 1, "expected fstream to be open\n");
    fs.base_ios.state = IOSTATE_eofbit | IOSTATE_failbit;
    call_func1(p_fstream_close, &fs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "close did not clear state = %d\n", fs.base_ios.state);
    fs.base_ios.state = IOSTATE_eofbit;
    call_func1(p_fstream_close, &fs);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "close on a closed stream did not set failbit\n");
    ok((int) call_func1(p_fstream_is_open, &fs) == 0, "expected fstream to not be open\n");
    ok(_close(fd) == -1, "expected close to close the opened file\n");

    /* open */
    fs.base_ios.state = IOSTATE_eofbit;
    call_func4(p_fstream_open, &fs, filename, OPENMODE_out, filebuf_openprot);
    fd = (int) call_func1(p_fstream_fd, &fs);
    ok(fd != -1, "wrong fd, expected not -1 got %d\n", fd);
    ok(fs.base_ios.state == IOSTATE_eofbit, "open did not succeed\n");
    call_func4(p_fstream_open, &fs, filename, OPENMODE_out, filebuf_openprot);
    ok(fs.base_ios.state == (IOSTATE_eofbit | IOSTATE_failbit), "second open did not set failbit\n");
    call_func1(p_fstream_close, &fs);
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s'\n", filename);

    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_out, filebuf_openprot, TRUE);
    ok(pfs == &fs, "constructor returned wrong pointer, expected %p got %p\n", &fs, pfs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "wrong stream state, expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    pfb = (filebuf*) fs.base_ios.sb;
    ok((int) call_func1(p_filebuf_is_open, pfb) == TRUE, "expected filebuf to be open\n");
    ok(fs.base_ios.delbuf == 1, "internal filebuf not marked for deletion\n");

    /* integration with ostream */
    pos = call_func2(p_ostream_print_str, (ostream*) &fs.base2, "ftest ");
    ok(pos == (ostream*) &fs.base2, "stream operation returned wrong pointer, expected %p got %p\n", &fs, &fs.base2);
    pos = call_func2(p_ostream_print_int, (ostream*) &fs.base2, 15);
    ok(pos == (ostream*) &fs.base2, "stream operation returned wrong pointer, expected %p got %p\n", &fs, &fs.base2);

    /* make sure that OPENMODE_in is not implied */
    ok(_lseek(pfb->fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_read(pfb->fd, st, 1) == -1, "_read succeeded on OPENMODE_out only fstream\n");

    /* reopen the file for reading */
    call_func1(p_fstream_vbase_dtor, &fs);
    pfs = call_func5(p_fstream_open_ctor, &fs, filename, OPENMODE_in, filebuf_openprot, TRUE);
    ok(pfs == &fs, "constructor returned wrong pointer, expected %p got %p\n", &fs, pfs);
    ok(fs.base_ios.state == IOSTATE_goodbit, "wrong stream state, expected %d got %d\n", IOSTATE_goodbit, fs.base_ios.state);
    pfb = (filebuf*) fs.base_ios.sb;
    ok((int) call_func1(p_filebuf_is_open, pfb) == TRUE, "expected filebuf to be open\n");

    /* integration with istream */
    memset(st, 'A', sizeof(st));
    pis = call_func2(p_istream_read_str, (istream*) &fs.base1, st);
    ok(pis == (istream*) &fs.base1, "stream operation returned wrong pointer, expected %p got %p\n", &fs, &fs.base1);
    st[7] = 0;
    ok(!strcmp(st, "ftest"), "expected 'ftest' got '%s'\n", st);

    i = 12345;
    pis = call_func2(p_istream_read_int, (istream*) &fs.base1, &i);
    ok(pis == (istream*) &fs.base1, "stream operation returned wrong pointer, expected %p got %p\n", &fs, &fs.base1);
    ok(i == 15, "expected 12 got %d\n", i);

    /* make sure that OPENMODE_out is not implied */
    ok(_lseek(pfb->fd, 0, SEEK_SET) == 0, "_lseek failed\n");
    ok(_write(pfb->fd, "blabla", 6) == -1, "_write succeeded on OPENMODE_in fstream\n");

    /* cleanup */
    call_func1(p_fstream_vbase_dtor, &fs);
    ok(_unlink(filename) == 0, "Couldn't unlink file named '%s', some filedescs are still open?\n", filename);
}

static void test_exception(void)
{
    const char *unknown = "Unknown exception";
    const char *test = "test";
    const char *what;
    logic_error le;
    exception e;

    /* exception */
    memset(&e, 0, sizeof(e));
    what = call_func1(p_exception_what, (void*) &e);
    ok(!strcmp(what, unknown), "expected %s got %s\n", unknown, what);

    call_func2(p_exception_ctor, (void*) &e, &test);
    what = call_func1(p_exception_what, (void*) &e);
    ok(!strcmp(what, test), "expected %s got %s\n", test, what);
    call_func1(p_exception_dtor, (void*) &e);

    /* logic_error */
    memset(&le, 0xff, sizeof(le));
    call_func2(p_logic_error_ctor, (void*) &le, &test);
    ok(!strcmp(le.e.name, test), "expected %s got %s\n", test, le.e.name);
    ok(le.e.do_free, "expected TRUE, got FALSE\n");
    what = call_func1(p_exception_what, (void*) &le.e);
    ok(!strcmp(what, test), "expected %s got %s\n", test, what);
    call_func1(p_logic_error_dtor, (void*) &le);
}

static DWORD WINAPI _try_enter_critical(void *crit)
{
    BOOL ret = TryEnterCriticalSection(crit);

    if (ret)
        LeaveCriticalSection(crit);

    return ret;
}

static void test_mtlock_mtunlock(void)
{
    CRITICAL_SECTION crit;
    HANDLE thread;
    DWORD exit_code, ret;

    InitializeCriticalSection(&crit);

    p__mtlock(&crit);

    thread = CreateThread(NULL, 0, _try_enter_critical, &crit, 0, NULL);
    ok(thread != NULL, "failed to create a thread, error: %lx\n", GetLastError());
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "failed to wait for the thread, ret: %ld, error: %lx\n", ret, GetLastError());
    ok(GetExitCodeThread(thread, &exit_code), "failed to get exit code of the thread\n");
    ok(exit_code == FALSE, "the thread entered critical section\n");
    ret = CloseHandle(thread);
    ok(ret, "failed to close thread's handle, error: %lx\n", GetLastError());

    p__mtunlock(&crit);

    thread = CreateThread(NULL, 0, _try_enter_critical, &crit, 0, NULL);
    ok(thread != NULL, "failed to create a thread, error: %lx\n", GetLastError());
    ret = WaitForSingleObject(thread, 1000);
    ok(ret == WAIT_OBJECT_0, "failed to wait for the thread, ret: %ld, error: %lx\n", ret, GetLastError());
    ok(GetExitCodeThread(thread, &exit_code), "failed to get exit code of the thread\n");
    ok(exit_code == TRUE, "the thread was not able to enter critical section\n");
    ret = CloseHandle(thread);
    ok(ret, "failed to close thread's handle, error: %lx\n", GetLastError());
}

START_TEST(msvcirt)
{
    if(!init())
        return;

    test_streambuf();
    test_filebuf();
    test_strstreambuf();
    test_stdiobuf();
    test_ios();
    test_ostream();
    test_ostream_print();
    test_ostream_withassign();
    test_ostrstream();
    test_ofstream();
    test_istream();
    test_istream_getint();
    test_istream_getdouble();
    test_istream_read();
    test_istream_withassign();
    test_istrstream();
    test_iostream();
    test_ifstream();
    test_strstream();
    test_stdiostream();
    test_Iostream_init();
    test_std_streams();
    test_fstream();
    test_exception();
    test_mtlock_mtunlock();

    FreeLibrary(msvcrt);
    FreeLibrary(msvcirt);
}
