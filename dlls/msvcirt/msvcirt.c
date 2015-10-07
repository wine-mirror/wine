/*
 * Copyright (C) 2007 Alexandre Julliard
 * Copyright (C) 2015 Iván Matellanes
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

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include "msvcirt.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcirt);

#define RESERVE_SIZE 512
#define STATEBUF_SIZE 8

/* ?sh_none@filebuf@@2HB */
const int filebuf_sh_none = 0x800;
/* ?sh_read@filebuf@@2HB */
const int filebuf_sh_read = 0xa00;
/* ?sh_write@filebuf@@2HB */
const int filebuf_sh_write = 0xc00;
/* ?openprot@filebuf@@2HB */
const int filebuf_openprot = 420;
/* ?binary@filebuf@@2HB */
const int filebuf_binary = _O_BINARY;
/* ?text@filebuf@@2HB */
const int filebuf_text = _O_TEXT;

/* ?adjustfield@ios@@2JB */
const LONG ios_adjustfield = FLAGS_left | FLAGS_right | FLAGS_internal;
/* ?basefield@ios@@2JB */
const LONG ios_basefield = FLAGS_dec | FLAGS_oct | FLAGS_hex;
/* ?floatfield@ios@@2JB */
const LONG ios_floatfield = FLAGS_scientific | FLAGS_fixed;
/* ?fLockcInit@ios@@0HA */
/* FIXME: should be initialized to 0 and increased on construction of cin, cout, cerr and clog */
int ios_fLockcInit = 4;
/* ?x_lockc@ios@@0U_CRT_CRITICAL_SECTION@@A */
extern CRITICAL_SECTION ios_static_lock;
CRITICAL_SECTION_DEBUG ios_static_lock_debug =
{
    0, 0, &ios_static_lock,
    { &ios_static_lock_debug.ProcessLocksList, &ios_static_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ios_static_lock") }
};
CRITICAL_SECTION ios_static_lock = { &ios_static_lock_debug, -1, 0, 0, 0, 0 };
/* ?x_maxbit@ios@@0JA */
LONG ios_maxbit = 0x8000;
/* ?x_curindex@ios@@0HA */
int ios_curindex = -1;
/* ?x_statebuf@ios@@0PAJA */
LONG ios_statebuf[STATEBUF_SIZE] = {0};

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

void __thiscall streambuf_setb(streambuf*, char*, char*, int);
streambuf* __thiscall streambuf_setbuf(streambuf*, char*, int);
void __thiscall streambuf_setg(streambuf*, char*, char*, char*);
void __thiscall streambuf_setp(streambuf*, char*, char*);

/* class filebuf */
typedef struct {
    streambuf base;
    filedesc fd;
    int close;
} filebuf;

filebuf* __thiscall filebuf_close(filebuf*);

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

ios* __thiscall ios_assign(ios*, const ios*);
int __thiscall ios_fail(const ios*);
void __cdecl ios_lock(ios*);
void __cdecl ios_lockc(void);
LONG __thiscall ios_setf_mask(ios*, LONG, LONG);
void __cdecl ios_unlock(ios*);
void __cdecl ios_unlockc(void);

/* class ostream */
typedef struct _ostream {
    const vtable_ptr *vtable;
} ostream;

/* ??_7streambuf@@6B@ */
extern const vtable_ptr MSVCP_streambuf_vtable;
/* ??_7filebuf@@6B@ */
extern const vtable_ptr MSVCP_filebuf_vtable;
/* ??_7strstreambuf@@6B@ */
extern const vtable_ptr MSVCP_strstreambuf_vtable;
/* ??_7stdiobuf@@6B@ */
extern const vtable_ptr MSVCP_stdiobuf_vtable;
/* ??_7ios@@6B@ */
extern const vtable_ptr MSVCP_ios_vtable;

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(streambuf,
            VTABLE_ADD_FUNC(streambuf_vector_dtor)
            VTABLE_ADD_FUNC(streambuf_sync)
            VTABLE_ADD_FUNC(streambuf_setbuf)
            VTABLE_ADD_FUNC(streambuf_seekoff)
            VTABLE_ADD_FUNC(streambuf_seekpos)
            VTABLE_ADD_FUNC(streambuf_xsputn)
            VTABLE_ADD_FUNC(streambuf_xsgetn)
            VTABLE_ADD_FUNC(streambuf_overflow)
            VTABLE_ADD_FUNC(streambuf_underflow)
            VTABLE_ADD_FUNC(streambuf_pbackfail)
            VTABLE_ADD_FUNC(streambuf_doallocate));
    __ASM_VTABLE(filebuf,
            VTABLE_ADD_FUNC(filebuf_vector_dtor)
            VTABLE_ADD_FUNC(filebuf_sync)
            VTABLE_ADD_FUNC(filebuf_setbuf)
            VTABLE_ADD_FUNC(filebuf_seekoff)
            VTABLE_ADD_FUNC(streambuf_seekpos)
            VTABLE_ADD_FUNC(streambuf_xsputn)
            VTABLE_ADD_FUNC(streambuf_xsgetn)
            VTABLE_ADD_FUNC(filebuf_overflow)
            VTABLE_ADD_FUNC(filebuf_underflow)
            VTABLE_ADD_FUNC(streambuf_pbackfail)
            VTABLE_ADD_FUNC(streambuf_doallocate));
    __ASM_VTABLE(strstreambuf,
            VTABLE_ADD_FUNC(strstreambuf_vector_dtor)
            VTABLE_ADD_FUNC(strstreambuf_sync)
            VTABLE_ADD_FUNC(strstreambuf_setbuf)
            VTABLE_ADD_FUNC(strstreambuf_seekoff)
            VTABLE_ADD_FUNC(streambuf_seekpos)
            VTABLE_ADD_FUNC(streambuf_xsputn)
            VTABLE_ADD_FUNC(streambuf_xsgetn)
            VTABLE_ADD_FUNC(strstreambuf_overflow)
            VTABLE_ADD_FUNC(strstreambuf_underflow)
            VTABLE_ADD_FUNC(streambuf_pbackfail)
            VTABLE_ADD_FUNC(strstreambuf_doallocate));
    __ASM_VTABLE(stdiobuf,
            VTABLE_ADD_FUNC(stdiobuf_vector_dtor)
            VTABLE_ADD_FUNC(stdiobuf_sync)
            VTABLE_ADD_FUNC(streambuf_setbuf)
            VTABLE_ADD_FUNC(stdiobuf_seekoff)
            VTABLE_ADD_FUNC(streambuf_seekpos)
            VTABLE_ADD_FUNC(streambuf_xsputn)
            VTABLE_ADD_FUNC(streambuf_xsgetn)
            VTABLE_ADD_FUNC(stdiobuf_overflow)
            VTABLE_ADD_FUNC(stdiobuf_underflow)
            VTABLE_ADD_FUNC(stdiobuf_pbackfail)
            VTABLE_ADD_FUNC(streambuf_doallocate));
    __ASM_VTABLE(ios,
            VTABLE_ADD_FUNC(ios_vector_dtor));
#ifndef __GNUC__
}
#endif

DEFINE_RTTI_DATA0(streambuf, 0, ".?AVstreambuf@@")
DEFINE_RTTI_DATA1(filebuf, 0, &streambuf_rtti_base_descriptor, ".?AVfilebuf@@")
DEFINE_RTTI_DATA1(strstreambuf, 0, &streambuf_rtti_base_descriptor, ".?AVstrstreambuf@@")
DEFINE_RTTI_DATA1(stdiobuf, 0, &streambuf_rtti_base_descriptor, ".?AVstdiobuf@@")
DEFINE_RTTI_DATA0(ios, 0, ".?AVios@@")

/* ??0streambuf@@IAE@PADH@Z */
/* ??0streambuf@@IEAA@PEADH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_reserve_ctor, 12)
streambuf* __thiscall streambuf_reserve_ctor(streambuf *this, char *buffer, int length)
{
    TRACE("(%p %p %d)\n", this, buffer, length);
    this->vtable = &MSVCP_streambuf_vtable;
    this->allocated = 0;
    this->stored_char = EOF;
    this->do_lock = -1;
    this->base = NULL;
    streambuf_setbuf(this, buffer, length);
    streambuf_setg(this, NULL, NULL, NULL);
    streambuf_setp(this, NULL, NULL);
    InitializeCriticalSection(&this->lock);
    return this;
}

/* ??0streambuf@@IAE@XZ */
/* ??0streambuf@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(streambuf_ctor, 4)
streambuf* __thiscall streambuf_ctor(streambuf *this)
{
    streambuf_reserve_ctor(this, NULL, 0);
    this->unbuffered = 0;
    return this;
}

/* ??0streambuf@@QAE@ABV0@@Z */
/* ??0streambuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(streambuf_copy_ctor, 8)
streambuf* __thiscall streambuf_copy_ctor(streambuf *this, const streambuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->vtable = &MSVCP_streambuf_vtable;
    return this;
}

/* ??1streambuf@@UAE@XZ */
/* ??1streambuf@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(streambuf_dtor, 4)
void __thiscall streambuf_dtor(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->allocated)
        MSVCRT_operator_delete(this->base);
    DeleteCriticalSection(&this->lock);
}

/* ??4streambuf@@QAEAAV0@ABV0@@Z */
/* ??4streambuf@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(streambuf_assign, 8)
streambuf* __thiscall streambuf_assign(streambuf *this, const streambuf *rhs)
{
    streambuf_dtor(this);
    return streambuf_copy_ctor(this, rhs);
}

/* ??_Estreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(streambuf_vector_dtor, 8)
#define call_streambuf_vector_dtor(this, flags) CALL_VTBL_FUNC(this, 0,\
        streambuf*, (streambuf*, unsigned int), (this, flags))
streambuf* __thiscall streambuf_vector_dtor(streambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            streambuf_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        streambuf_dtor(this);
        if (flags & 1)
            MSVCRT_operator_delete(this);
    }
    return this;
}

/* ??_Gstreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(streambuf_scalar_dtor, 8)
streambuf* __thiscall streambuf_scalar_dtor(streambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    streambuf_dtor(this);
    if (flags & 1) MSVCRT_operator_delete(this);
    return this;
}

/* ?doallocate@streambuf@@MAEHXZ */
/* ?doallocate@streambuf@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_doallocate, 4)
#define call_streambuf_doallocate(this) CALL_VTBL_FUNC(this, 40, int, (streambuf*), (this))
int __thiscall streambuf_doallocate(streambuf *this)
{
    char *reserve;

    TRACE("(%p)\n", this);
    reserve = MSVCRT_operator_new(RESERVE_SIZE);
    if (!reserve)
        return EOF;

    streambuf_setb(this, reserve, reserve + RESERVE_SIZE, 1);
    return 1;
}

/* ?allocate@streambuf@@IAEHXZ */
/* ?allocate@streambuf@@IEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_allocate, 4)
int __thiscall streambuf_allocate(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->base != NULL || this->unbuffered)
        return 0;
    return call_streambuf_doallocate(this);
}

/* ?base@streambuf@@IBEPADXZ */
/* ?base@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_base, 4)
char* __thiscall streambuf_base(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->base;
}

/* ?blen@streambuf@@IBEHXZ */
/* ?blen@streambuf@@IEBAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_blen, 4)
int __thiscall streambuf_blen(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->ebuf - this->base;
}

/* ?eback@streambuf@@IBEPADXZ */
/* ?eback@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_eback, 4)
char* __thiscall streambuf_eback(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->eback;
}

/* ?ebuf@streambuf@@IBEPADXZ */
/* ?ebuf@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_ebuf, 4)
char* __thiscall streambuf_ebuf(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->ebuf;
}

/* ?egptr@streambuf@@IBEPADXZ */
/* ?egptr@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_egptr, 4)
char* __thiscall streambuf_egptr(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->egptr;
}

/* ?epptr@streambuf@@IBEPADXZ */
/* ?epptr@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_epptr, 4)
char* __thiscall streambuf_epptr(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->epptr;
}

/* ?gptr@streambuf@@IBEPADXZ */
/* ?gptr@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_gptr, 4)
char* __thiscall streambuf_gptr(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->gptr;
}

/* ?pbase@streambuf@@IBEPADXZ */
/* ?pbase@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_pbase, 4)
char* __thiscall streambuf_pbase(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->pbase;
}

/* ?pptr@streambuf@@IBEPADXZ */
/* ?pptr@streambuf@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(streambuf_pptr, 4)
char* __thiscall streambuf_pptr(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->pptr;
}

/* ?clrlock@streambuf@@QAEXXZ */
/* ?clrlock@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_clrlock, 4)
void __thiscall streambuf_clrlock(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock <= 0)
        this->do_lock++;
}

/* ?lock@streambuf@@QAEXXZ */
/* ?lock@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_lock, 4)
void __thiscall streambuf_lock(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock < 0)
        EnterCriticalSection(&this->lock);
}

/* ?lockptr@streambuf@@IAEPAU_CRT_CRITICAL_SECTION@@XZ */
/* ?lockptr@streambuf@@IEAAPEAU_CRT_CRITICAL_SECTION@@XZ */
DEFINE_THISCALL_WRAPPER(streambuf_lockptr, 4)
CRITICAL_SECTION* __thiscall streambuf_lockptr(streambuf *this)
{
    TRACE("(%p)\n", this);
    return &this->lock;
}

/* ?gbump@streambuf@@IAEXH@Z */
/* ?gbump@streambuf@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_gbump, 8)
void __thiscall streambuf_gbump(streambuf *this, int count)
{
    TRACE("(%p %d)\n", this, count);
    this->gptr += count;
}

/* ?pbump@streambuf@@IAEXH@Z */
/* ?pbump@streambuf@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_pbump, 8)
void __thiscall streambuf_pbump(streambuf *this, int count)
{
    TRACE("(%p %d)\n", this, count);
    this->pptr += count;
}

/* ?in_avail@streambuf@@QBEHXZ */
/* ?in_avail@streambuf@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_in_avail, 4)
int __thiscall streambuf_in_avail(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->egptr - this->gptr;
}

/* ?out_waiting@streambuf@@QBEHXZ */
/* ?out_waiting@streambuf@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_out_waiting, 4)
int __thiscall streambuf_out_waiting(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->pptr - this->pbase;
}

/* Unexported */
DEFINE_THISCALL_WRAPPER(streambuf_overflow, 8)
#define call_streambuf_overflow(this, c) CALL_VTBL_FUNC(this, 28, int, (streambuf*, int), (this, c))
int __thiscall streambuf_overflow(streambuf *this, int c)
{
    ERR("overflow is not implemented in streambuf\n");
    return EOF;
}

/* ?seekoff@streambuf@@UAEJJW4seek_dir@ios@@H@Z */
/* ?seekoff@streambuf@@UEAAJJW4seek_dir@ios@@H@Z */
DEFINE_THISCALL_WRAPPER(streambuf_seekoff, 16)
#define call_streambuf_seekoff(this, off, dir, mode) CALL_VTBL_FUNC(this, 12, streampos, (streambuf*, streamoff, ios_seek_dir, int), (this, off, dir, mode))
streampos __thiscall streambuf_seekoff(streambuf *this, streamoff offset, ios_seek_dir dir, int mode)
{
    TRACE("(%p %d %d %d)\n", this, offset, dir, mode);
    return EOF;
}

/* ?seekpos@streambuf@@UAEJJH@Z */
/* ?seekpos@streambuf@@UEAAJJH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_seekpos, 12)
streampos __thiscall streambuf_seekpos(streambuf *this, streampos pos, int mode)
{
    TRACE("(%p %d %d)\n", this, pos, mode);
    return call_streambuf_seekoff(this, pos, SEEKDIR_beg, mode);
}

/* ?pbackfail@streambuf@@UAEHH@Z */
/* ?pbackfail@streambuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_pbackfail, 8)
#define call_streambuf_pbackfail(this, c) CALL_VTBL_FUNC(this, 36, int, (streambuf*, int), (this, c))
int __thiscall streambuf_pbackfail(streambuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    if (this->gptr > this->eback)
        return *--this->gptr = c;
    if (call_streambuf_seekoff(this, -1, SEEKDIR_cur, OPENMODE_in) == EOF)
        return EOF;
    if (!this->unbuffered && this->egptr) {
        /* 'c' should be the next character read */
        memmove(this->gptr + 1, this->gptr, this->egptr - this->gptr - 1);
        *this->gptr = c;
    }
    return c;
}

/* ?setb@streambuf@@IAEXPAD0H@Z */
/* ?setb@streambuf@@IEAAXPEAD0H@Z */
DEFINE_THISCALL_WRAPPER(streambuf_setb, 16)
void __thiscall streambuf_setb(streambuf *this, char *ba, char *eb, int delete)
{
    TRACE("(%p %p %p %d)\n", this, ba, eb, delete);
    if (this->allocated)
        MSVCRT_operator_delete(this->base);
    this->allocated = delete;
    this->base = ba;
    this->ebuf = eb;
}

/* ?setbuf@streambuf@@UAEPAV1@PADH@Z */
/* ?setbuf@streambuf@@UEAAPEAV1@PEADH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_setbuf, 12)
streambuf* __thiscall streambuf_setbuf(streambuf *this, char *buffer, int length)
{
    TRACE("(%p %p %d)\n", this, buffer, length);
    if (this->base != NULL)
        return NULL;

    if (buffer == NULL || !length) {
        this->unbuffered = 1;
        this->base = this->ebuf = NULL;
    } else {
        this->unbuffered = 0;
        this->base = buffer;
        this->ebuf = buffer + length;
    }
    return this;
}

/* ?setg@streambuf@@IAEXPAD00@Z */
/* ?setg@streambuf@@IEAAXPEAD00@Z */
DEFINE_THISCALL_WRAPPER(streambuf_setg, 16)
void __thiscall streambuf_setg(streambuf *this, char *ek, char *gp, char *eg)
{
    TRACE("(%p %p %p %p)\n", this, ek, gp, eg);
    this->eback = ek;
    this->gptr = gp;
    this->egptr = eg;
}

/* ?setlock@streambuf@@QAEXXZ */
/* ?setlock@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_setlock, 4)
void __thiscall streambuf_setlock(streambuf *this)
{
    TRACE("(%p)\n", this);
    this->do_lock--;
}

/* ?setp@streambuf@@IAEXPAD0@Z */
/* ?setp@streambuf@@IEAAXPEAD0@Z */
DEFINE_THISCALL_WRAPPER(streambuf_setp, 12)
void __thiscall streambuf_setp(streambuf *this, char *pb, char *ep)
{
    TRACE("(%p %p %p)\n", this, pb, ep);
    this->pbase = this->pptr = pb;
    this->epptr = ep;
}

/* ?sync@streambuf@@UAEHXZ */
/* ?sync@streambuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_sync, 4)
#define call_streambuf_sync(this) CALL_VTBL_FUNC(this, 4, int, (streambuf*), (this))
int __thiscall streambuf_sync(streambuf *this)
{
    TRACE("(%p)\n", this);
    return (this->gptr >= this->egptr && this->pbase >= this->pptr) ? 0 : EOF;
}

/* ?unbuffered@streambuf@@IAEXH@Z */
/* ?unbuffered@streambuf@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_unbuffered_set, 8)
void __thiscall streambuf_unbuffered_set(streambuf *this, int buf)
{
    TRACE("(%p %d)\n", this, buf);
    this->unbuffered = buf;
}

/* ?unbuffered@streambuf@@IBEHXZ */
/* ?unbuffered@streambuf@@IEBAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_unbuffered_get, 4)
int __thiscall streambuf_unbuffered_get(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return this->unbuffered;
}

/* Unexported */
DEFINE_THISCALL_WRAPPER(streambuf_underflow, 4)
#define call_streambuf_underflow(this) CALL_VTBL_FUNC(this, 32, int, (streambuf*), (this))
int __thiscall streambuf_underflow(streambuf *this)
{
    ERR("underflow is not implemented in streambuf\n");
    return EOF;
}

/* ?unlock@streambuf@@QAEXXZ */
/* ?unlock@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_unlock, 4)
void __thiscall streambuf_unlock(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock < 0)
        LeaveCriticalSection(&this->lock);
}

/* ?xsgetn@streambuf@@UAEHPADH@Z */
/* ?xsgetn@streambuf@@UEAAHPEADH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_xsgetn, 12)
#define call_streambuf_xsgetn(this, buffer, count) CALL_VTBL_FUNC(this, 24, int, (streambuf*, char*, int), (this, buffer, count))
int __thiscall streambuf_xsgetn(streambuf *this, char *buffer, int count)
{
    int copied = 0, chunk;

    TRACE("(%p %p %d)\n", this, buffer, count);

    if (this->unbuffered) {
        if (this->stored_char == EOF)
            this->stored_char = call_streambuf_underflow(this);
        while (copied < count && this->stored_char != EOF) {
            buffer[copied++] = this->stored_char;
            this->stored_char = call_streambuf_underflow(this);
        }
    } else {
        while (copied < count) {
            if (call_streambuf_underflow(this) == EOF)
                break;
            chunk = this->egptr - this->gptr;
            if (chunk > count - copied)
                chunk = count - copied;
            memcpy(buffer+copied, this->gptr, chunk);
            this->gptr += chunk;
            copied += chunk;
        }
    }
    return copied;
}

/* ?xsputn@streambuf@@UAEHPBDH@Z */
/* ?xsputn@streambuf@@UEAAHPEBDH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_xsputn, 12)
#define call_streambuf_xsputn(this, data, length) CALL_VTBL_FUNC(this, 20, int, (streambuf*, const char*, int), (this, data, length))
int __thiscall streambuf_xsputn(streambuf *this, const char *data, int length)
{
    int copied = 0, chunk;

    TRACE("(%p %p %d)\n", this, data, length);

    while (copied < length) {
        if (this->unbuffered || this->pptr == this->epptr) {
            if (call_streambuf_overflow(this, data[copied]) == EOF)
                break;
            copied++;
        } else {
            chunk = this->epptr - this->pptr;
            if (chunk > length - copied)
                chunk = length - copied;
            memcpy(this->pptr, data+copied, chunk);
            this->pptr += chunk;
            copied += chunk;
        }
    }
    return copied;
}

/* ?sgetc@streambuf@@QAEHXZ */
/* ?sgetc@streambuf@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_sgetc, 4)
int __thiscall streambuf_sgetc(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->unbuffered) {
        if (this->stored_char == EOF)
            this->stored_char = call_streambuf_underflow(this);
        return this->stored_char;
    } else
        return call_streambuf_underflow(this);
}

/* ?sputc@streambuf@@QAEHH@Z */
/* ?sputc@streambuf@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_sputc, 8)
int __thiscall streambuf_sputc(streambuf *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return (this->pptr < this->epptr) ? *this->pptr++ = ch : call_streambuf_overflow(this, ch);
}

/* ?sgetn@streambuf@@QAEHPADH@Z */
/* ?sgetn@streambuf@@QEAAHPEADH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_sgetn, 12)
int __thiscall streambuf_sgetn(streambuf *this, char *buffer, int count)
{
    return call_streambuf_xsgetn(this, buffer, count);
}

/* ?sputn@streambuf@@QAEHPBDH@Z */
/* ?sputn@streambuf@@QEAAHPEBDH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_sputn, 12)
int __thiscall streambuf_sputn(streambuf *this, const char *data, int length)
{
    return call_streambuf_xsputn(this, data, length);
}

/* ?snextc@streambuf@@QAEHXZ */
/* ?snextc@streambuf@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_snextc, 4)
int __thiscall streambuf_snextc(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->unbuffered) {
        if (this->stored_char == EOF)
            call_streambuf_underflow(this);
        return this->stored_char = call_streambuf_underflow(this);
    } else {
        if (this->gptr >= this->egptr)
            call_streambuf_underflow(this);
        this->gptr++;
        return (this->gptr < this->egptr) ? *this->gptr : call_streambuf_underflow(this);
    }
}

/* ?sbumpc@streambuf@@QAEHXZ */
/* ?sbumpc@streambuf@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_sbumpc, 4)
int __thiscall streambuf_sbumpc(streambuf *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if (this->unbuffered) {
        ret = this->stored_char;
        this->stored_char = EOF;
        if (ret == EOF)
            ret = call_streambuf_underflow(this);
    } else {
        ret = (this->gptr < this->egptr) ? *this->gptr : call_streambuf_underflow(this);
        this->gptr++;
    }
    return ret;
}

/* ?stossc@streambuf@@QAEXXZ */
/* ?stossc@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_stossc, 4)
void __thiscall streambuf_stossc(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->unbuffered) {
        if (this->stored_char == EOF)
            call_streambuf_underflow(this);
        else
            this->stored_char = EOF;
    } else {
        if (this->gptr >= this->egptr)
            call_streambuf_underflow(this);
        if (this->gptr < this->egptr)
            this->gptr++;
    }
}

/* ?sputbackc@streambuf@@QAEHD@Z */
/* ?sputbackc@streambuf@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(streambuf_sputbackc, 8)
int __thiscall streambuf_sputbackc(streambuf *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_streambuf_pbackfail(this, ch);
}

/* ?dbp@streambuf@@QAEXXZ */
/* ?dbp@streambuf@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(streambuf_dbp, 4)
void __thiscall streambuf_dbp(streambuf *this)
{
    printf("\nSTREAMBUF DEBUG INFO: this=%p, ", this);
    if (this->unbuffered) {
        printf("unbuffered\n");
    } else {
        printf("_fAlloc=%d\n", this->allocated);
        printf(" base()=%p, ebuf()=%p,  blen()=%d\n", this->base, this->ebuf, streambuf_blen(this));
        printf("pbase()=%p, pptr()=%p, epptr()=%d\n", this->pbase, this->pptr, this->epptr);
        printf("eback()=%p, gptr()=%p, egptr()=%d\n", this->eback, this->gptr, this->egptr);
    }
}

/* ??0filebuf@@QAE@ABV0@@Z */
/* ??0filebuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(filebuf_copy_ctor, 8)
filebuf* __thiscall filebuf_copy_ctor(filebuf* this, const filebuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &MSVCP_filebuf_vtable;
    return this;
}

/* ??0filebuf@@QAE@HPADH@Z */
/* ??0filebuf@@QEAA@HPEADH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_fd_reserve_ctor, 16)
filebuf* __thiscall filebuf_fd_reserve_ctor(filebuf* this, filedesc fd, char *buffer, int length)
{
    TRACE("(%p %d %p %d)\n", this, fd, buffer, length);
    streambuf_reserve_ctor(&this->base, buffer, length);
    this->base.vtable = &MSVCP_filebuf_vtable;
    this->fd = fd;
    this->close = 0;
    return this;
}

/* ??0filebuf@@QAE@H@Z */
/* ??0filebuf@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(filebuf_fd_ctor, 8)
filebuf* __thiscall filebuf_fd_ctor(filebuf* this, filedesc fd)
{
    filebuf_fd_reserve_ctor(this, fd, NULL, 0);
    this->base.unbuffered = 0;
    return this;
}

/* ??0filebuf@@QAE@XZ */
/* ??0filebuf@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(filebuf_ctor, 4)
filebuf* __thiscall filebuf_ctor(filebuf* this)
{
    return filebuf_fd_ctor(this, -1);
}

/* ??1filebuf@@UAE@XZ */
/* ??1filebuf@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(filebuf_dtor, 4)
void __thiscall filebuf_dtor(filebuf* this)
{
    TRACE("(%p)\n", this);
    if (this->close)
        filebuf_close(this);
    streambuf_dtor(&this->base);
}

/* ??4filebuf@@QAEAAV0@ABV0@@Z */
/* ??4filebuf@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(filebuf_assign, 8)
filebuf* __thiscall filebuf_assign(filebuf* this, const filebuf *rhs)
{
    filebuf_dtor(this);
    return filebuf_copy_ctor(this, rhs);
}

/* ??_Efilebuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(filebuf_vector_dtor, 8)
filebuf* __thiscall filebuf_vector_dtor(filebuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            filebuf_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        filebuf_dtor(this);
        if (flags & 1)
            MSVCRT_operator_delete(this);
    }
    return this;
}

/* ??_Gfilebuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(filebuf_scalar_dtor, 8)
filebuf* __thiscall filebuf_scalar_dtor(filebuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    filebuf_dtor(this);
    if (flags & 1) MSVCRT_operator_delete(this);
    return this;
}

/* ?attach@filebuf@@QAEPAV1@H@Z */
/* ?attach@filebuf@@QEAAPEAV1@H@Z */
DEFINE_THISCALL_WRAPPER(filebuf_attach, 8)
filebuf* __thiscall filebuf_attach(filebuf *this, filedesc fd)
{
    TRACE("(%p %d)\n", this, fd);
    if (this->fd != -1)
        return NULL;

    streambuf_lock(&this->base);
    this->fd = fd;
    streambuf_allocate(&this->base);
    streambuf_unlock(&this->base);
    return this;
}

/* ?close@filebuf@@QAEPAV1@XZ */
/* ?close@filebuf@@QEAAPEAV1@XZ */
DEFINE_THISCALL_WRAPPER(filebuf_close, 4)
filebuf* __thiscall filebuf_close(filebuf *this)
{
    filebuf *ret;

    TRACE("(%p)\n", this);
    if (this->fd == -1)
        return NULL;

    streambuf_lock(&this->base);
    if (call_streambuf_sync(&this->base) == EOF || _close(this->fd) < 0) {
        ret = NULL;
    } else {
        this->fd = -1;
        ret = this;
    }
    streambuf_unlock(&this->base);
    return ret;
}

/* ?fd@filebuf@@QBEHXZ */
/* ?fd@filebuf@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(filebuf_fd, 4)
filedesc __thiscall filebuf_fd(const filebuf *this)
{
    TRACE("(%p)\n", this);
    return this->fd;
}

/* ?is_open@filebuf@@QBEHXZ */
/* ?is_open@filebuf@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(filebuf_is_open, 4)
int __thiscall filebuf_is_open(const filebuf *this)
{
    TRACE("(%p)\n", this);
    return this->fd != -1;
}

/* ?open@filebuf@@QAEPAV1@PBDHH@Z */
/* ?open@filebuf@@QEAAPEAV1@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_open, 16)
filebuf* __thiscall filebuf_open(filebuf *this, const char *name, ios_open_mode mode, int protection)
{
    const int inout_mode[4] = {-1, _O_RDONLY, _O_WRONLY, _O_RDWR};
    const int share_mode[4] = {_SH_DENYRW, _SH_DENYWR, _SH_DENYRD, _SH_DENYNO};
    int op_flags, sh_flags, fd;

    TRACE("(%p %s %x %x)\n", this, name, mode, protection);
    if (this->fd != -1)
        return NULL;

    /* mode */
    if (mode & (OPENMODE_app|OPENMODE_trunc))
        mode |= OPENMODE_out;
    op_flags = inout_mode[mode & (OPENMODE_in|OPENMODE_out)];
    if (op_flags < 0)
        return NULL;
    if (mode & OPENMODE_app)
        op_flags |= _O_APPEND;
    if ((mode & OPENMODE_trunc) ||
            ((mode & OPENMODE_out) && !(mode & (OPENMODE_in|OPENMODE_app|OPENMODE_ate))))
        op_flags |= _O_TRUNC;
    if (!(mode & OPENMODE_nocreate))
        op_flags |= _O_CREAT;
    if (mode & OPENMODE_noreplace)
        op_flags |= _O_EXCL;
    op_flags |= (mode & OPENMODE_binary) ? _O_BINARY : _O_TEXT;

    /* share protection */
    sh_flags = (protection & filebuf_sh_none) ? share_mode[(protection >> 9) & 3] : _SH_DENYNO;

    TRACE("op_flags %x, sh_flags %x\n", op_flags, sh_flags);
    fd = _sopen(name, op_flags, sh_flags, _S_IREAD|_S_IWRITE);
    if (fd < 0)
        return NULL;

    streambuf_lock(&this->base);
    this->close = 1;
    this->fd = fd;
    if ((mode & OPENMODE_ate) &&
            call_streambuf_seekoff(&this->base, 0, SEEKDIR_end, mode & (OPENMODE_in|OPENMODE_out)) == EOF) {
        _close(fd);
        this->fd = -1;
    }
    streambuf_allocate(&this->base);
    streambuf_unlock(&this->base);
    return (this->fd == -1) ? NULL : this;
}

/* ?overflow@filebuf@@UAEHH@Z */
/* ?overflow@filebuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_overflow, 8)
int __thiscall filebuf_overflow(filebuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    if (call_streambuf_sync(&this->base) == EOF)
        return EOF;
    if (this->base.unbuffered)
        return (c == EOF) ? 1 : _write(this->fd, &c, 1);
    if (streambuf_allocate(&this->base) == EOF)
        return EOF;

    this->base.pbase = this->base.pptr = this->base.base;
    this->base.epptr = this->base.ebuf;
    if (c != EOF)
        *this->base.pptr++ = c;
    return 1;
}

/* ?seekoff@filebuf@@UAEJJW4seek_dir@ios@@H@Z */
/* ?seekoff@filebuf@@UEAAJJW4seek_dir@ios@@H@Z */
DEFINE_THISCALL_WRAPPER(filebuf_seekoff, 16)
streampos __thiscall filebuf_seekoff(filebuf *this, streamoff offset, ios_seek_dir dir, int mode)
{
    TRACE("(%p %d %d %d)\n", this, offset, dir, mode);
    if (call_streambuf_sync(&this->base) == EOF)
        return EOF;
    return _lseek(this->fd, offset, dir);
}

/* ?setbuf@filebuf@@UAEPAVstreambuf@@PADH@Z */
/* ?setbuf@filebuf@@UEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_setbuf, 12)
streambuf* __thiscall filebuf_setbuf(filebuf *this, char *buffer, int length)
{
    streambuf *ret;

    TRACE("(%p %p %d)\n", this, buffer, length);
    if (this->base.base != NULL)
        return NULL;

    streambuf_lock(&this->base);
    ret = streambuf_setbuf(&this->base, buffer, length);
    streambuf_unlock(&this->base);
    return ret;
}

/* ?setmode@filebuf@@QAEHH@Z */
/* ?setmode@filebuf@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_setmode, 8)
int __thiscall filebuf_setmode(filebuf *this, int mode)
{
    int ret;

    TRACE("(%p %d)\n", this, mode);
    if (mode != filebuf_text && mode != filebuf_binary)
        return -1;

    streambuf_lock(&this->base);
    ret = (call_streambuf_sync(&this->base) == EOF) ? -1 : _setmode(this->fd, mode);
    streambuf_unlock(&this->base);
    return ret;
}

/* ?sync@filebuf@@UAEHXZ */
/* ?sync@filebuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(filebuf_sync, 4)
int __thiscall filebuf_sync(filebuf *this)
{
    int count, mode;
    char *ptr;
    LONG offset;

    TRACE("(%p)\n", this);
    if (this->fd == -1)
        return EOF;
    if (this->base.unbuffered)
        return 0;

    /* flush output buffer */
    if (this->base.pptr != NULL) {
        count = this->base.pptr - this->base.pbase;
        if (count > 0 && _write(this->fd, this->base.pbase, count) != count)
            return EOF;
        this->base.pbase = this->base.pptr = this->base.epptr = NULL;
    }
    /* flush input buffer */
    if (this->base.egptr != NULL) {
        offset = this->base.egptr - this->base.gptr;
        if (offset > 0) {
            mode = _setmode(this->fd, _O_TEXT);
            _setmode(this->fd, mode);
            if (mode & _O_TEXT) {
                /* in text mode, '\n' in the buffer means '\r\n' in the file */
                for (ptr = this->base.gptr; ptr < this->base.egptr; ptr++)
                    if (*ptr == '\n')
                        offset++;
            }
            if (_lseek(this->fd, -offset, SEEK_CUR) < 0)
                return EOF;
        }
        this->base.eback = this->base.gptr = this->base.egptr = NULL;
    }
    return 0;
}

/* ?underflow@filebuf@@UAEHXZ */
/* ?underflow@filebuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(filebuf_underflow, 4)
int __thiscall filebuf_underflow(filebuf *this)
{
    int buffer_size, read_bytes;
    char c;

    TRACE("(%p)\n", this);

    if (this->base.unbuffered)
        return (_read(this->fd, &c, 1) < 1) ? EOF : c;

    if (this->base.gptr >= this->base.egptr) {
        if (call_streambuf_sync(&this->base) == EOF)
            return EOF;
        buffer_size = this->base.ebuf - this->base.base;
        read_bytes = _read(this->fd, this->base.base, buffer_size);
        if (read_bytes <= 0)
            return EOF;
        this->base.eback = this->base.gptr = this->base.base;
        this->base.egptr = this->base.base + read_bytes;
    }
    return *this->base.gptr;
}

/* ??0strstreambuf@@QAE@ABV0@@Z */
/* ??0strstreambuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_copy_ctor, 8)
strstreambuf* __thiscall strstreambuf_copy_ctor(strstreambuf *this, const strstreambuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &MSVCP_strstreambuf_vtable;
    return this;
}

/* ??0strstreambuf@@QAE@H@Z */
/* ??0strstreambuf@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_dynamic_ctor, 8)
strstreambuf* __thiscall strstreambuf_dynamic_ctor(strstreambuf* this, int length)
{
    TRACE("(%p %d)\n", this, length);
    streambuf_ctor(&this->base);
    this->base.vtable = &MSVCP_strstreambuf_vtable;
    this->dynamic = 1;
    this->increase = length;
    this->constant = 0;
    this->f_alloc = NULL;
    this->f_free = NULL;
    return this;
}

/* ??0strstreambuf@@QAE@P6APAXJ@ZP6AXPAX@Z@Z */
/* ??0strstreambuf@@QEAA@P6APEAXJ@ZP6AXPEAX@Z@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_funcs_ctor, 12)
strstreambuf* __thiscall strstreambuf_funcs_ctor(strstreambuf* this, allocFunction falloc, freeFunction ffree)
{
    TRACE("(%p %p %p)\n", this, falloc, ffree);
    strstreambuf_dynamic_ctor(this, 1);
    this->f_alloc = falloc;
    this->f_free = ffree;
    return this;
}

/* ??0strstreambuf@@QAE@PADH0@Z */
/* ??0strstreambuf@@QEAA@PEADH0@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_buffer_ctor, 16)
strstreambuf* __thiscall strstreambuf_buffer_ctor(strstreambuf *this, char *buffer, int length, char *put)
{
    char *end_buffer;

    TRACE("(%p %p %d %p)\n", this, buffer, length, put);

    if (length > 0)
        end_buffer = buffer + length;
    else if (length == 0)
        end_buffer = buffer + strlen(buffer);
    else
        end_buffer = (char*) -1;

    streambuf_ctor(&this->base);
    streambuf_setb(&this->base, buffer, end_buffer, 0);
    if (put == NULL) {
        streambuf_setg(&this->base, buffer, buffer, end_buffer);
    } else {
        streambuf_setg(&this->base, buffer, buffer, put);
        streambuf_setp(&this->base, put, end_buffer);
    }
    this->base.vtable = &MSVCP_strstreambuf_vtable;
    this->dynamic = 0;
    this->constant = 1;
    return this;
}

/* ??0strstreambuf@@QAE@PAEH0@Z */
/* ??0strstreambuf@@QEAA@PEAEH0@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_ubuffer_ctor, 16)
strstreambuf* __thiscall strstreambuf_ubuffer_ctor(strstreambuf *this, unsigned char *buffer, int length, unsigned char *put)
{
    TRACE("(%p %p %d %p)\n", this, buffer, length, put);
    return strstreambuf_buffer_ctor(this, (char*)buffer, length, (char*)put);
}

/* ??0strstreambuf@@QAE@XZ */
/* ??0strstreambuf@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor, 4)
strstreambuf* __thiscall strstreambuf_ctor(strstreambuf *this)
{
    TRACE("(%p)\n", this);
    return strstreambuf_dynamic_ctor(this, 1);
}

/* ??1strstreambuf@@UAE@XZ */
/* ??1strstreambuf@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_dtor, 4)
void __thiscall strstreambuf_dtor(strstreambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->dynamic && this->base.base) {
        if (this->f_free)
            this->f_free(this->base.base);
        else
            MSVCRT_operator_delete(this->base.base);
    }
    streambuf_dtor(&this->base);
}

/* ??4strstreambuf@@QAEAAV0@ABV0@@Z */
/* ??4strstreambuf@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_assign, 8)
strstreambuf* __thiscall strstreambuf_assign(strstreambuf *this, const strstreambuf *rhs)
{
    strstreambuf_dtor(this);
    return strstreambuf_copy_ctor(this, rhs);
}

/* ??_Estrstreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_vector_dtor, 8)
strstreambuf* __thiscall strstreambuf_vector_dtor(strstreambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            strstreambuf_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        strstreambuf_dtor(this);
        if (flags & 1)
            MSVCRT_operator_delete(this);
    }
    return this;
}

/* ??_Gstrstreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_scalar_dtor, 8)
strstreambuf* __thiscall strstreambuf_scalar_dtor(strstreambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    strstreambuf_dtor(this);
    if (flags & 1) MSVCRT_operator_delete(this);
    return this;
}

/* ?doallocate@strstreambuf@@MAEHXZ */
/* ?doallocate@strstreambuf@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_doallocate, 4)
int __thiscall strstreambuf_doallocate(strstreambuf *this)
{
    char *prev_buffer = this->base.base, *new_buffer;
    LONG prev_size = this->base.ebuf - this->base.base, new_size;

    TRACE("(%p)\n", this);

    /* calculate the size of the new buffer */
    new_size = (prev_size > 0 ? prev_size : 0) + (this->increase > 0 ? this->increase : 1);
    /* get a new buffer */
    if (this->f_alloc)
        new_buffer = this->f_alloc(new_size);
    else
        new_buffer = MSVCRT_operator_new(new_size);
    if (!new_buffer)
        return EOF;
    if (this->base.ebuf) {
        /* copy the contents and adjust the pointers */
        memcpy(new_buffer, this->base.base, prev_size);
        if (this->base.egptr) {
            this->base.eback += new_buffer - prev_buffer;
            this->base.gptr += new_buffer - prev_buffer;
            this->base.egptr += new_buffer - prev_buffer;
        }
        if (this->base.epptr) {
            this->base.pbase += new_buffer - prev_buffer;
            this->base.pptr += new_buffer - prev_buffer;
            this->base.epptr += new_buffer - prev_buffer;
        }
        /* free the old buffer */
        if (this->f_free)
            this->f_free(this->base.base);
        else
            MSVCRT_operator_delete(this->base.base);
    }
    streambuf_setb(&this->base, new_buffer, new_buffer + new_size, 0);
    return 1;
}

/* ?freeze@strstreambuf@@QAEXH@Z */
/* ?freeze@strstreambuf@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_freeze, 8)
void __thiscall strstreambuf_freeze(strstreambuf *this, int frozen)
{
    TRACE("(%p %d)\n", this, frozen);
    if (!this->constant)
        this->dynamic = !frozen;
}

/* ?overflow@strstreambuf@@UAEHH@Z */
/* ?overflow@strstreambuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_overflow, 8)
int __thiscall strstreambuf_overflow(strstreambuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    if (this->base.pptr >= this->base.epptr) {
        /* increase the buffer size if it's dynamic */
        if (!this->dynamic || call_streambuf_doallocate(&this->base) == EOF)
            return EOF;
        if (!this->base.epptr)
            this->base.pbase = this->base.pptr = this->base.egptr ? this->base.egptr : this->base.base;
        this->base.epptr = this->base.ebuf;
    }
    if (c != EOF)
        *this->base.pptr++ = c;
    return 1;
}

/* ?seekoff@strstreambuf@@UAEJJW4seek_dir@ios@@H@Z */
/* ?seekoff@strstreambuf@@UEAAJJW4seek_dir@ios@@H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_seekoff, 16)
streampos __thiscall strstreambuf_seekoff(strstreambuf *this, streamoff offset, ios_seek_dir dir, int mode)
{
    char *base[3];

    TRACE("(%p %d %d %d)\n", this, offset, dir, mode);

    if (dir < SEEKDIR_beg || dir > SEEKDIR_end || !(mode & (OPENMODE_in|OPENMODE_out)))
        return EOF;
    /* read buffer */
    if (mode & OPENMODE_in) {
        call_streambuf_underflow(&this->base);
        base[SEEKDIR_beg] = this->base.eback;
        base[SEEKDIR_cur] = this->base.gptr;
        base[SEEKDIR_end] = this->base.egptr;
        if (base[dir] + offset < this->base.eback || base[dir] + offset > this->base.egptr)
            return EOF;
        this->base.gptr = base[dir] + offset;
    }
    /* write buffer */
    if (mode & OPENMODE_out) {
        if (!this->base.epptr && call_streambuf_overflow(&this->base, EOF) == EOF)
            return EOF;
        base[SEEKDIR_beg] = this->base.pbase;
        base[SEEKDIR_cur] = this->base.pptr;
        base[SEEKDIR_end] = this->base.epptr;
        if (base[dir] + offset < this->base.pbase)
            return EOF;
        if (base[dir] + offset > this->base.epptr) {
            /* make room if the buffer is dynamic */
            if (!this->dynamic)
                return EOF;
            this->increase = offset;
            if (call_streambuf_doallocate(&this->base) == EOF)
                return EOF;
        }
        this->base.pptr = base[dir] + offset;
        return this->base.pptr - base[SEEKDIR_beg];
    }
    return this->base.gptr - base[SEEKDIR_beg];
}

/* ?setbuf@strstreambuf@@UAEPAVstreambuf@@PADH@Z */
/* ?setbuf@strstreambuf@@UEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_setbuf, 12)
streambuf* __thiscall strstreambuf_setbuf(strstreambuf *this, char *buffer, int length)
{
    TRACE("(%p %p %d)\n", this, buffer, length);
    if (length)
        this->increase = length;
    return &this->base;
}

/* ?str@strstreambuf@@QAEPADXZ */
/* ?str@strstreambuf@@QEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_str, 4)
char* __thiscall strstreambuf_str(strstreambuf *this)
{
    TRACE("(%p)\n", this);
    strstreambuf_freeze(this, 1);
    return this->base.base;
}

/* ?sync@strstreambuf@@UAEHXZ */
/* ?sync@strstreambuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_sync, 4)
int __thiscall strstreambuf_sync(strstreambuf *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?underflow@strstreambuf@@UAEHXZ */
/* ?underflow@strstreambuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_underflow, 4)
int __thiscall strstreambuf_underflow(strstreambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->base.gptr < this->base.egptr)
        return *this->base.gptr;
    /* extend the get area to include the characters written */
    if (this->base.egptr < this->base.pptr)
        this->base.egptr = this->base.pptr;
    return (this->base.gptr < this->base.egptr) ? *this->base.gptr : EOF;
}

/* ??0stdiobuf@@QAE@ABV0@@Z */
/* ??0stdiobuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_copy_ctor, 8)
stdiobuf* __thiscall stdiobuf_copy_ctor(stdiobuf *this, const stdiobuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &MSVCP_stdiobuf_vtable;
    return this;
}

/* ??0stdiobuf@@QAE@PAU_iobuf@@@Z */
/* ??0stdiobuf@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_file_ctor, 8)
stdiobuf* __thiscall stdiobuf_file_ctor(stdiobuf *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);
    streambuf_reserve_ctor(&this->base, NULL, 0);
    this->base.vtable = &MSVCP_stdiobuf_vtable;
    this->file = file;
    return this;
}

/* ??1stdiobuf@@UAE@XZ */
/* ??1stdiobuf@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(stdiobuf_dtor, 4)
void __thiscall stdiobuf_dtor(stdiobuf *this)
{
    TRACE("(%p)\n", this);
    call_streambuf_sync(&this->base);
    streambuf_dtor(&this->base);
}

/* ??4stdiobuf@@QAEAAV0@ABV0@@Z */
/* ??4stdiobuf@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_assign, 8)
stdiobuf* __thiscall stdiobuf_assign(stdiobuf *this, const stdiobuf *rhs)
{
    stdiobuf_dtor(this);
    return stdiobuf_copy_ctor(this, rhs);
}

/* ??_Estdiobuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_vector_dtor, 8)
stdiobuf* __thiscall stdiobuf_vector_dtor(stdiobuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            stdiobuf_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        stdiobuf_dtor(this);
        if (flags & 1)
            MSVCRT_operator_delete(this);
    }
    return this;
}

/* ??_Gstdiobuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_scalar_dtor, 8)
stdiobuf* __thiscall stdiobuf_scalar_dtor(stdiobuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    stdiobuf_dtor(this);
    if (flags & 1) MSVCRT_operator_delete(this);
    return this;
}

/* ?overflow@stdiobuf@@UAEHH@Z */
/* ?overflow@stdiobuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_overflow, 8)
int __thiscall stdiobuf_overflow(stdiobuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    if (this->base.unbuffered)
        return (c == EOF) ? 1 : fputc(c, this->file);
    if (streambuf_allocate(&this->base) == EOF)
        return EOF;

    if (!this->base.epptr) {
        /* set the put area to the second half of the buffer */
        streambuf_setp(&this->base,
            this->base.base + (this->base.ebuf - this->base.base) / 2, this->base.ebuf);
    } else if (this->base.pptr > this->base.pbase) {
        /* flush the put area */
        int count = this->base.pptr - this->base.pbase;
        if (fwrite(this->base.pbase, sizeof(char), count, this->file) != count)
            return EOF;
        this->base.pptr = this->base.pbase;
    }
    if (c != EOF) {
        if (this->base.pbase >= this->base.epptr)
            return fputc(c, this->file);
        *this->base.pptr++ = c;
    }
    return 1;
}

/* ?pbackfail@stdiobuf@@UAEHH@Z */
/* ?pbackfail@stdiobuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_pbackfail, 8)
int __thiscall stdiobuf_pbackfail(stdiobuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    return streambuf_pbackfail(&this->base, c);
}

/* ?seekoff@stdiobuf@@UAEJJW4seek_dir@ios@@H@Z */
/* ?seekoff@stdiobuf@@UEAAJJW4seek_dir@ios@@H@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_seekoff, 16)
streampos __thiscall stdiobuf_seekoff(stdiobuf *this, streamoff offset, ios_seek_dir dir, int mode)
{
    TRACE("(%p %d %d %d)\n", this, offset, dir, mode);
    call_streambuf_overflow(&this->base, EOF);
    if (fseek(this->file, offset, dir))
        return EOF;
    return ftell(this->file);
}

/* ?setrwbuf@stdiobuf@@QAEHHH@Z */
/* ?setrwbuf@stdiobuf@@QEAAHHH@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_setrwbuf, 12)
int __thiscall stdiobuf_setrwbuf(stdiobuf *this, int read_size, int write_size)
{
    char *reserve;
    int buffer_size = read_size + write_size;

    TRACE("(%p %d %d)\n", this, read_size, write_size);
    if (read_size < 0 || write_size < 0)
        return 0;
    if (!buffer_size) {
        this->base.unbuffered = 1;
        return 0;
    }
    /* get a new buffer */
    reserve = MSVCRT_operator_new(buffer_size);
    if (!reserve)
        return 0;
    streambuf_setb(&this->base, reserve, reserve + buffer_size, 1);
    this->base.unbuffered = 0;
    /* set the get/put areas */
    if (read_size > 0)
        streambuf_setg(&this->base, reserve, reserve + read_size, reserve + read_size);
    else
        streambuf_setg(&this->base, NULL, NULL, NULL);
    if (write_size > 0)
        streambuf_setp(&this->base, reserve + read_size, reserve + buffer_size);
    else
        streambuf_setp(&this->base, NULL, NULL);
    return 1;
}

/* ?stdiofile@stdiobuf@@QAEPAU_iobuf@@XZ */
/* ?stdiofile@stdiobuf@@QEAAPEAU_iobuf@@XZ */
DEFINE_THISCALL_WRAPPER(stdiobuf_stdiofile, 4)
FILE* __thiscall stdiobuf_stdiofile(stdiobuf *this)
{
    TRACE("(%p)\n", this);
    return this->file;
}

/* ?sync@stdiobuf@@UAEHXZ */
/* ?sync@stdiobuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(stdiobuf_sync, 4)
int __thiscall stdiobuf_sync(stdiobuf *this)
{
    TRACE("(%p)\n", this);
    if (this->base.unbuffered)
        return 0;
    /* flush the put area */
    if (call_streambuf_overflow(&this->base, EOF) == EOF)
        return EOF;
    /* flush the get area */
    if (this->base.gptr < this->base.egptr) {
        char *ptr;
        int fd, mode, offset = this->base.egptr - this->base.gptr;
        if ((fd = fileno(this->file)) < 0)
            return EOF;
        mode = _setmode(fd, _O_TEXT);
        _setmode(fd, mode);
        if (mode & _O_TEXT) {
            /* in text mode, '\n' in the buffer means '\r\n' in the file */
            for (ptr = this->base.gptr; ptr < this->base.egptr; ptr++)
                if (*ptr == '\n')
                    offset++;
        }
        if (fseek(this->file, -offset, SEEK_CUR))
            return EOF;
        this->base.gptr = this->base.egptr;
    }
    return 0;
}

/* ?underflow@stdiobuf@@UAEHXZ */
/* ?underflow@stdiobuf@@UEAAHXZ */
DEFINE_THISCALL_WRAPPER(stdiobuf_underflow, 4)
int __thiscall stdiobuf_underflow(stdiobuf *this)
{
    TRACE("(%p)\n", this);
    if (!this->file)
        return EOF;
    if (this->base.unbuffered)
        return fgetc(this->file);
    if (streambuf_allocate(&this->base) == EOF)
        return EOF;

    if (!this->base.egptr) {
        /* set the get area to the first half of the buffer */
        char *middle = this->base.base + (this->base.ebuf - this->base.base) / 2;
        streambuf_setg(&this->base, this->base.base, middle, middle);
    }
    if (this->base.gptr >= this->base.egptr) {
        /* read characters from the file */
        int buffer_size = this->base.egptr - this->base.eback, read_bytes;
        if (!this->base.eback ||
            (read_bytes = fread(this->base.eback, sizeof(char), buffer_size, this->file)) <= 0)
            return EOF;
        memmove(this->base.egptr - read_bytes, this->base.eback, read_bytes);
        this->base.gptr = this->base.egptr - read_bytes;
    }
    return *this->base.gptr++;
}

/* ??0ios@@IAE@ABV0@@Z */
/* ??0ios@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ios_copy_ctor, 8)
ios* __thiscall ios_copy_ctor(ios *this, const ios *copy)
{
    TRACE("(%p %p)\n", this, copy);
    ios_fLockcInit++;
    this->vtable = &MSVCP_ios_vtable;
    this->sb = NULL;
    this->delbuf = 0;
    InitializeCriticalSection(&this->lock);
    return ios_assign(this, copy);
}

/* ??0ios@@QAE@PAVstreambuf@@@Z */
/* ??0ios@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ios_sb_ctor, 8)
ios* __thiscall ios_sb_ctor(ios *this, streambuf *sb)
{
    TRACE("(%p %p)\n", this, sb);
    ios_fLockcInit++;
    this->vtable = &MSVCP_ios_vtable;
    this->sb = sb;
    this->state = sb ? IOSTATE_goodbit : IOSTATE_badbit;
    this->special[0] = this->special[1] = 0;
    this->delbuf = 0;
    this->tie = NULL;
    this->flags = 0;
    this->precision = 6;
    this->fill = ' ';
    this->width = 0;
    this->do_lock = -1;
    InitializeCriticalSection(&this->lock);
    return this;
}

/* ??0ios@@IAE@XZ */
/* ??0ios@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_ctor, 4)
ios* __thiscall ios_ctor(ios *this)
{
    return ios_sb_ctor(this, NULL);
}

/* ??1ios@@UAE@XZ */
/* ??1ios@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_dtor, 4)
void __thiscall ios_dtor(ios *this)
{
    TRACE("(%p)\n", this);
    ios_fLockcInit--;
    if (this->delbuf && this->sb)
        call_streambuf_vector_dtor(this->sb, 1);
    this->sb = NULL;
    this->state = IOSTATE_badbit;
    DeleteCriticalSection(&this->lock);
}

/* ??4ios@@IAEAAV0@ABV0@@Z */
/* ??4ios@@IEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ios_assign, 8)
ios* __thiscall ios_assign(ios *this, const ios *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    this->state = rhs->state;
    if (!this->sb)
        this->state |= IOSTATE_badbit;
    this->tie = rhs->tie;
    this->flags = rhs->flags;
    this->precision = (char) rhs->precision;
    this->fill = rhs->fill;
    this->width = (char) rhs->width;
    return this;
}

/* ??7ios@@QBEHXZ */
/* ??7ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_op_not, 4)
int __thiscall ios_op_not(const ios *this)
{
    TRACE("(%p)\n", this);
    return ios_fail(this);
}

/* ??Bios@@QBEPAXXZ */
/* ??Bios@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(ios_op_void, 4)
void* __thiscall ios_op_void(const ios *this)
{
    TRACE("(%p)\n", this);
    return ios_fail(this) ? NULL : (void*)this;
}

/* ??_Eios@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_vector_dtor, 8)
ios* __thiscall ios_vector_dtor(ios *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            ios_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        ios_dtor(this);
        if (flags & 1)
            MSVCRT_operator_delete(this);
    }
    return this;
}

/* ??_Gios@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_scalar_dtor, 8)
ios* __thiscall ios_scalar_dtor(ios *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    ios_dtor(this);
    if (flags & 1) MSVCRT_operator_delete(this);
    return this;
}

/* ?bad@ios@@QBEHXZ */
/* ?bad@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_bad, 4)
int __thiscall ios_bad(const ios *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_badbit);
}

/* ?bitalloc@ios@@SAJXZ */
LONG __cdecl ios_bitalloc(void)
{
    TRACE("()\n");
    ios_lockc();
    ios_maxbit <<= 1;
    ios_unlockc();
    return ios_maxbit;
}

/* ?clear@ios@@QAEXH@Z */
/* ?clear@ios@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_clear, 8)
void __thiscall ios_clear(ios *this, int state)
{
    TRACE("(%p %d)\n", this, state);
    ios_lock(this);
    this->state = state;
    ios_unlock(this);
}

/* ?clrlock@ios@@QAAXXZ */
/* ?clrlock@ios@@QEAAXXZ */
void __cdecl ios_clrlock(ios *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock <= 0)
        this->do_lock++;
    if (this->sb)
        streambuf_clrlock(this->sb);
}

/* ?delbuf@ios@@QAEXH@Z */
/* ?delbuf@ios@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_delbuf_set, 8)
void __thiscall ios_delbuf_set(ios *this, int delete)
{
    TRACE("(%p %d)\n", this, delete);
    this->delbuf = delete;
}

/* ?delbuf@ios@@QBEHXZ */
/* ?delbuf@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_delbuf_get, 4)
int __thiscall ios_delbuf_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->delbuf;
}

/* ?dec@@YAAAVios@@AAV1@@Z */
/* ?dec@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_dec(ios *this)
{
    TRACE("(%p)\n", this);
    ios_setf_mask(this, FLAGS_dec, ios_basefield);
    return this;
}

/* ?eof@ios@@QBEHXZ */
/* ?eof@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_eof, 4)
int __thiscall ios_eof(const ios *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_eofbit);
}

/* ?fail@ios@@QBEHXZ */
/* ?fail@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_fail, 4)
int __thiscall ios_fail(const ios *this)
{
    TRACE("(%p)\n", this);
    return (this->state & (IOSTATE_failbit|IOSTATE_badbit));
}

/* ?fill@ios@@QAEDD@Z */
/* ?fill@ios@@QEAADD@Z */
DEFINE_THISCALL_WRAPPER(ios_fill_set, 8)
char __thiscall ios_fill_set(ios *this, char fill)
{
    char prev = this->fill;

    TRACE("(%p %d)\n", this, fill);

    this->fill = fill;
    return prev;
}

/* ?fill@ios@@QBEDXZ */
/* ?fill@ios@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(ios_fill_get, 4)
char __thiscall ios_fill_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->fill;
}

/* ?flags@ios@@QAEJJ@Z */
/* ?flags@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_flags_set, 8)
LONG __thiscall ios_flags_set(ios *this, LONG flags)
{
    LONG prev = this->flags;

    TRACE("(%p %x)\n", this, flags);

    this->flags = flags;
    return prev;
}

/* ?flags@ios@@QBEJXZ */
/* ?flags@ios@@QEBAJXZ */
DEFINE_THISCALL_WRAPPER(ios_flags_get, 4)
LONG __thiscall ios_flags_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->flags;
}

/* ?good@ios@@QBEHXZ */
/* ?good@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_good, 4)
int __thiscall ios_good(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->state == IOSTATE_goodbit;
}

/* ?hex@@YAAAVios@@AAV1@@Z */
/* ?hex@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_hex(ios *this)
{
    TRACE("(%p)\n", this);
    ios_setf_mask(this, FLAGS_hex, ios_basefield);
    return this;
}

/* ?init@ios@@IAEXPAVstreambuf@@@Z */
/* ?init@ios@@IEAAXPEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ios_init, 8)
void __thiscall ios_init(ios *this, streambuf *sb)
{
    TRACE("(%p %p)\n", this, sb);
    if (this->delbuf && this->sb)
        call_streambuf_vector_dtor(this->sb, 1);
    this->sb = sb;
    if (sb == NULL)
        this->state |= IOSTATE_badbit;
    else
        this->state &= ~IOSTATE_badbit;
}

/* ?iword@ios@@QBEAAJH@Z */
/* ?iword@ios@@QEBAAEAJH@Z */
DEFINE_THISCALL_WRAPPER(ios_iword, 8)
LONG* __thiscall ios_iword(const ios *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return &ios_statebuf[index];
}

/* ?lock@ios@@QAAXXZ */
/* ?lock@ios@@QEAAXXZ */
void __cdecl ios_lock(ios *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock < 0)
        EnterCriticalSection(&this->lock);
}

/* ?lockbuf@ios@@QAAXXZ */
/* ?lockbuf@ios@@QEAAXXZ */
void __cdecl ios_lockbuf(ios *this)
{
    TRACE("(%p)\n", this);
    streambuf_lock(this->sb);
}

/* ?lockc@ios@@KAXXZ */
void __cdecl ios_lockc(void)
{
    TRACE("()\n");
    EnterCriticalSection(&ios_static_lock);
}

/* ?lockptr@ios@@IAEPAU_CRT_CRITICAL_SECTION@@XZ */
/* ?lockptr@ios@@IEAAPEAU_CRT_CRITICAL_SECTION@@XZ */
DEFINE_THISCALL_WRAPPER(ios_lockptr, 4)
CRITICAL_SECTION* __thiscall ios_lockptr(ios *this)
{
    TRACE("(%p)\n", this);
    return &this->lock;
}

/* ?oct@@YAAAVios@@AAV1@@Z */
/* ?oct@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_oct(ios *this)
{
    TRACE("(%p)\n", this);
    ios_setf_mask(this, FLAGS_oct, ios_basefield);
    return this;
}

/* ?precision@ios@@QAEHH@Z */
/* ?precision@ios@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_precision_set, 8)
int __thiscall ios_precision_set(ios *this, int prec)
{
    int prev = this->precision;

    TRACE("(%p %d)\n", this, prec);

    this->precision = prec;
    return prev;
}

/* ?precision@ios@@QBEHXZ */
/* ?precision@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_precision_get, 4)
int __thiscall ios_precision_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->precision;
}

/* ?pword@ios@@QBEAAPAXH@Z */
/* ?pword@ios@@QEBAAEAPEAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_pword, 8)
void** __thiscall ios_pword(const ios *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return (void**)&ios_statebuf[index];
}

/* ?rdbuf@ios@@QBEPAVstreambuf@@XZ */
/* ?rdbuf@ios@@QEBAPEAVstreambuf@@XZ */
DEFINE_THISCALL_WRAPPER(ios_rdbuf, 4)
streambuf* __thiscall ios_rdbuf(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->sb;
}

/* ?rdstate@ios@@QBEHXZ */
/* ?rdstate@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_rdstate, 4)
int __thiscall ios_rdstate(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->state;
}

/* ?setf@ios@@QAEJJ@Z */
/* ?setf@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_setf, 8)
LONG __thiscall ios_setf(ios *this, LONG flags)
{
    LONG prev = this->flags;

    TRACE("(%p %x)\n", this, flags);

    ios_lock(this);
    this->flags |= flags;
    ios_unlock(this);
    return prev;
}

/* ?setf@ios@@QAEJJJ@Z */
/* ?setf@ios@@QEAAJJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_setf_mask, 12)
LONG __thiscall ios_setf_mask(ios *this, LONG flags, LONG mask)
{
    LONG prev = this->flags;

    TRACE("(%p %x %x)\n", this, flags, mask);

    ios_lock(this);
    this->flags = (this->flags & (~mask)) | (flags & mask);
    ios_unlock(this);
    return prev;
}

/* ?setlock@ios@@QAAXXZ */
/* ?setlock@ios@@QEAAXXZ */
void __cdecl ios_setlock(ios *this)
{
    TRACE("(%p)\n", this);
    this->do_lock--;
    if (this->sb)
        streambuf_setlock(this->sb);
}

/* ?sync_with_stdio@ios@@SAXXZ */
void __cdecl ios_sync_with_stdio(void)
{
    FIXME("() stub\n");
}

/* ?tie@ios@@QAEPAVostream@@PAV2@@Z */
/* ?tie@ios@@QEAAPEAVostream@@PEAV2@@Z */
DEFINE_THISCALL_WRAPPER(ios_tie_set, 8)
ostream* __thiscall ios_tie_set(ios *this, ostream *ostr)
{
    ostream *prev = this->tie;

    TRACE("(%p %p)\n", this, ostr);

    this->tie = ostr;
    return prev;
}

/* ?tie@ios@@QBEPAVostream@@XZ */
/* ?tie@ios@@QEBAPEAVostream@@XZ */
DEFINE_THISCALL_WRAPPER(ios_tie_get, 4)
ostream* __thiscall ios_tie_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->tie;
}

/* ?unlock@ios@@QAAXXZ */
/* ?unlock@ios@@QEAAXXZ */
void __cdecl ios_unlock(ios *this)
{
    TRACE("(%p)\n", this);
    if (this->do_lock < 0)
        LeaveCriticalSection(&this->lock);
}

/* ?unlockbuf@ios@@QAAXXZ */
/* ?unlockbuf@ios@@QEAAXXZ */
void __cdecl ios_unlockbuf(ios *this)
{
    TRACE("(%p)\n", this);
    streambuf_unlock(this->sb);
}

/* ?unlockc@ios@@KAXXZ */
void __cdecl ios_unlockc(void)
{
    TRACE("()\n");
    LeaveCriticalSection(&ios_static_lock);
}

/* ?unsetf@ios@@QAEJJ@Z */
/* ?unsetf@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_unsetf, 8)
LONG __thiscall ios_unsetf(ios *this, LONG flags)
{
    LONG prev = this->flags;

    TRACE("(%p %x)\n", this, flags);

    ios_lock(this);
    this->flags &= ~flags;
    ios_unlock(this);
    return prev;
}

/* ?width@ios@@QAEHH@Z */
/* ?width@ios@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_width_set, 8)
int __thiscall ios_width_set(ios *this, int width)
{
    int prev = this->width;

    TRACE("(%p %d)\n", this, width);

    this->width = width;
    return prev;
}

/* ?width@ios@@QBEHXZ */
/* ?width@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_width_get, 4)
int __thiscall ios_width_get(const ios *this)
{
    TRACE("(%p)\n", this);
    return this->width;
}

/* ?xalloc@ios@@SAHXZ */
int __cdecl ios_xalloc(void)
{
    int ret;

    TRACE("()\n");

    ios_lockc();
    ret = (ios_curindex < STATEBUF_SIZE-1) ? ++ios_curindex : -1;
    ios_unlockc();
    return ret;
}

/******************************************************************
 *		 ??0ostrstream@@QAE@XZ (MSVCRTI.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_ostrstream_ctor,8)
void * __thiscall MSVCIRT_ostrstream_ctor(ostream *this, BOOL virt_init)
{
   FIXME("(%p %x) stub\n", this, virt_init);
   return this;
}

/******************************************************************
 *		 ??1ostrstream@@UAE@XZ (MSVCRTI.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_ostrstream_dtor,4)
void __thiscall MSVCIRT_ostrstream_dtor(ios *base)
{
    FIXME("(%p) stub\n", base);
}

/******************************************************************
 *		??6ostream@@QAEAAV0@E@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(unsigned char)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_uchar,8)
void * __thiscall MSVCIRT_operator_sl_uchar(ostream * _this, unsigned char ch)
{
   FIXME("(%p)->(%c) stub\n", _this, ch);
   return _this;
}

/******************************************************************
 *		 ??6ostream@@QAEAAV0@H@Z (MSVCRTI.@)
 *        class ostream & __thiscall ostream::operator<<(int)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_int,8)
void * __thiscall MSVCIRT_operator_sl_int(ostream * _this, int integer)
{
   FIXME("(%p)->(%d) stub\n", _this, integer);
   return _this;
}

/******************************************************************
 *		??6ostream@@QAEAAV0@PBD@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(char const *)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_pchar,8)
void * __thiscall MSVCIRT_operator_sl_pchar(ostream * _this, const char * string)
{
   FIXME("(%p)->(%s) stub\n", _this, debugstr_a(string));
   return _this;
}

/******************************************************************
 *		??6ostream@@QAEAAV0@P6AAAV0@AAV0@@Z@Z (MSVCRTI.@)
 *    class ostream & __thiscall ostream::operator<<(class ostream & (__cdecl*)(class ostream &))
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_operator_sl_callback,8)
void * __thiscall MSVCIRT_operator_sl_callback(ostream * _this, ostream * (__cdecl*func)(ostream*))
{
   TRACE("%p, %p\n", _this, func);
   return func(_this);
}

/******************************************************************
 *		?endl@@YAAAVostream@@AAV1@@Z (MSVCRTI.@)
 *           class ostream & __cdecl endl(class ostream &)
 */
void * CDECL MSVCIRT_endl(ostream * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return _this;
}

/******************************************************************
 *		?ends@@YAAAVostream@@AAV1@@Z (MSVCRTI.@)
 *           class ostream & __cdecl ends(class ostream &)
 */
void * CDECL MSVCIRT_ends(ostream * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return _this;
}

#ifdef __i386__

#define DEFINE_VTBL_WRAPPER(off)            \
    __ASM_GLOBAL_FUNC(vtbl_wrapper_ ## off, \
        "popl %eax\n\t"                     \
        "popl %ecx\n\t"                     \
        "pushl %eax\n\t"                    \
        "movl 0(%ecx), %eax\n\t"            \
        "jmp *" #off "(%eax)\n\t")

DEFINE_VTBL_WRAPPER(0);
DEFINE_VTBL_WRAPPER(4);
DEFINE_VTBL_WRAPPER(8);
DEFINE_VTBL_WRAPPER(12);
DEFINE_VTBL_WRAPPER(16);
DEFINE_VTBL_WRAPPER(20);
DEFINE_VTBL_WRAPPER(24);
DEFINE_VTBL_WRAPPER(28);
DEFINE_VTBL_WRAPPER(32);
DEFINE_VTBL_WRAPPER(36);
DEFINE_VTBL_WRAPPER(40);
DEFINE_VTBL_WRAPPER(44);
DEFINE_VTBL_WRAPPER(48);
DEFINE_VTBL_WRAPPER(52);
DEFINE_VTBL_WRAPPER(56);

#endif

void* (__cdecl *MSVCRT_operator_new)(SIZE_T);
void (__cdecl *MSVCRT_operator_delete)(void*);

static void init_cxx_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    if (sizeof(void *) > sizeof(int))  /* 64-bit has different names */
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPEAX_K@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPEAX@Z");
    }
    else
    {
        MSVCRT_operator_new = (void*)GetProcAddress(hmod, "??2@YAPAXI@Z");
        MSVCRT_operator_delete = (void*)GetProcAddress(hmod, "??3@YAXPAX@Z");
    }
}

static void init_io(void *base)
{
#ifdef __x86_64__
    init_streambuf_rtti(base);
    init_filebuf_rtti(base);
    init_strstreambuf_rtti(base);
    init_stdiobuf_rtti(base);
    init_ios_rtti(base);
#endif
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
   switch (reason)
   {
   case DLL_WINE_PREATTACH:
       return FALSE;  /* prefer native version */
   case DLL_PROCESS_ATTACH:
       init_cxx_funcs();
       init_exception(inst);
       init_io(inst);
       DisableThreadLibraryCalls( inst );
       break;
   }
   return TRUE;
}
