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

#include <stdarg.h>
#include <stdio.h>

#include "msvcirt.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcirt);

#define RESERVE_SIZE 512

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

/* class ostream */
typedef struct _ostream {
    const vtable_ptr *vtable;
} ostream;

typedef struct {
    LPVOID VTable;
} class_strstreambuf;

/* ??_7streambuf@@6B@ */
extern const vtable_ptr MSVCP_streambuf_vtable;
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
    __ASM_VTABLE(ios,
            VTABLE_ADD_FUNC(ios_vector_dtor));
#ifndef __GNUC__
}
#endif

DEFINE_RTTI_DATA0(streambuf, 0, ".?AVstreambuf@@")
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

/* ?pbackfail@streambuf@@UAEHH@Z */
/* ?pbackfail@streambuf@@UEAAHH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_pbackfail, 8)
#define call_streambuf_pbackfail(this, c) CALL_VTBL_FUNC(this, 36, int, (streambuf*, int), (this, c))
int __thiscall streambuf_pbackfail(streambuf *this, int c)
{
    TRACE("(%p %d)\n", this, c);
    if (this->gptr <= this->eback)
        return EOF;
    return *--this->gptr = c;
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

/* ??0ios@@IAE@ABV0@@Z */
/* ??0ios@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ios_copy_ctor, 8)
ios* __thiscall ios_copy_ctor(ios *this, const ios *copy)
{
    TRACE("(%p %p)\n", this, copy);
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
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ??Bios@@QBEPAXXZ */
/* ??Bios@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(ios_op_void, 4)
void* __thiscall ios_op_void(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
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
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?bitalloc@ios@@SAJXZ */
LONG __cdecl ios_bitalloc(void)
{
    FIXME("() stub\n");
    return 0;
}

/* ?clear@ios@@QAEXH@Z */
/* ?clear@ios@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_clear, 8)
void __thiscall ios_clear(ios *this, int state)
{
    FIXME("(%p %d) stub\n", this, state);
}

/* ?clrlock@ios@@QAAXXZ */
/* ?clrlock@ios@@QEAAXXZ */
void __cdecl ios_clrlock(ios *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?delbuf@ios@@QAEXH@Z */
/* ?delbuf@ios@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_delbuf_set, 8)
void __thiscall ios_delbuf_set(ios *this, int delete)
{
    FIXME("(%p %d) stub\n", this, delete);
}

/* ?delbuf@ios@@QBEHXZ */
/* ?delbuf@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_delbuf_get, 4)
int __thiscall ios_delbuf_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?dec@@YAAAVios@@AAV1@@Z */
/* ?dec@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_dec(ios *this)
{
    FIXME("(%p) stub\n", this);
    return this;
}

/* ?eof@ios@@QBEHXZ */
/* ?eof@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_eof, 4)
int __thiscall ios_eof(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?fail@ios@@QBEHXZ */
/* ?fail@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_fail, 4)
int __thiscall ios_fail(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?fill@ios@@QAEDD@Z */
/* ?fill@ios@@QEAADD@Z */
DEFINE_THISCALL_WRAPPER(ios_fill_set, 8)
char __thiscall ios_fill_set(ios *this, char fill)
{
    FIXME("(%p %d) stub\n", this, fill);
    return EOF;
}

/* ?fill@ios@@QBEDXZ */
/* ?fill@ios@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(ios_fill_get, 4)
char __thiscall ios_fill_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return EOF;
}

/* ?flags@ios@@QAEJJ@Z */
/* ?flags@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_flags_set, 8)
LONG __thiscall ios_flags_set(ios *this, LONG flags)
{
    FIXME("(%p %x) stub\n", this, flags);
    return 0;
}

/* ?flags@ios@@QBEJXZ */
/* ?flags@ios@@QEBAJXZ */
DEFINE_THISCALL_WRAPPER(ios_flags_get, 4)
LONG __thiscall ios_flags_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?good@ios@@QBEHXZ */
/* ?good@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_good, 4)
int __thiscall ios_good(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?hex@@YAAAVios@@AAV1@@Z */
/* ?hex@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_hex(ios *this)
{
    FIXME("(%p) stub\n", this);
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
    FIXME("(%p %d) stub\n", this, index);
    return NULL;
}

/* ?lock@ios@@QAAXXZ */
/* ?lock@ios@@QEAAXXZ */
void __cdecl ios_lock(ios *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?lockbuf@ios@@QAAXXZ */
/* ?lockbuf@ios@@QEAAXXZ */
void __cdecl ios_lockbuf(ios *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?lockc@ios@@KAXXZ */
void __cdecl ios_lockc(void)
{
    FIXME("() stub\n");
}

/* ?lockptr@ios@@IAEPAU_CRT_CRITICAL_SECTION@@XZ */
/* ?lockptr@ios@@IEAAPEAU_CRT_CRITICAL_SECTION@@XZ */
DEFINE_THISCALL_WRAPPER(ios_lockptr, 4)
CRITICAL_SECTION* __thiscall ios_lockptr(ios *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?oct@@YAAAVios@@AAV1@@Z */
/* ?oct@@YAAEAVios@@AEAV1@@Z */
ios* __cdecl ios_oct(ios *this)
{
    FIXME("(%p) stub\n", this);
    return this;
}

/* ?precision@ios@@QAEHH@Z */
/* ?precision@ios@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_precision_set, 8)
int __thiscall ios_precision_set(ios *this, int prec)
{
    FIXME("(%p %d) stub\n", this, prec);
    return 0;
}

/* ?precision@ios@@QBEHXZ */
/* ?precision@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_precision_get, 4)
int __thiscall ios_precision_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?pword@ios@@QBEAAPAXH@Z */
/* ?pword@ios@@QEBAAEAPEAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_pword, 8)
void** __thiscall ios_pword(const ios *this, int index)
{
    FIXME("(%p %d) stub\n", this, index);
    return NULL;
}

/* ?rdbuf@ios@@QBEPAVstreambuf@@XZ */
/* ?rdbuf@ios@@QEBAPEAVstreambuf@@XZ */
DEFINE_THISCALL_WRAPPER(ios_rdbuf, 4)
streambuf* __thiscall ios_rdbuf(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?rdstate@ios@@QBEHXZ */
/* ?rdstate@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_rdstate, 4)
int __thiscall ios_rdstate(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?setf@ios@@QAEJJ@Z */
/* ?setf@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_setf, 8)
LONG __thiscall ios_setf(ios *this, LONG flags)
{
    FIXME("(%p %x) stub\n", this, flags);
    return 0;
}

/* ?setf@ios@@QAEJJJ@Z */
/* ?setf@ios@@QEAAJJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_setf_mask, 12)
LONG __thiscall ios_setf_mask(ios *this, LONG flags, LONG mask)
{
    FIXME("(%p %x %x) stub\n", this, flags, mask);
    return 0;
}

/* ?setlock@ios@@QAAXXZ */
/* ?setlock@ios@@QEAAXXZ */
void __cdecl ios_setlock(ios *this)
{
    FIXME("(%p) stub\n", this);
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
    FIXME("(%p %p) stub\n", this, ostr);
    return NULL;
}

/* ?tie@ios@@QBEPAVostream@@XZ */
/* ?tie@ios@@QEBAPEAVostream@@XZ */
DEFINE_THISCALL_WRAPPER(ios_tie_get, 4)
ostream* __thiscall ios_tie_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?unlock@ios@@QAAXXZ */
/* ?unlock@ios@@QEAAXXZ */
void __cdecl ios_unlock(ios *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?unlockbuf@ios@@QAAXXZ */
/* ?unlockbuf@ios@@QEAAXXZ */
void __cdecl ios_unlockbuf(ios *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?unlockc@ios@@KAXXZ */
void __cdecl ios_unlockc(void)
{
    FIXME("() stub\n");
}

/* ?unsetf@ios@@QAEJJ@Z */
/* ?unsetf@ios@@QEAAJJ@Z */
DEFINE_THISCALL_WRAPPER(ios_unsetf, 8)
LONG __thiscall ios_unsetf(ios *this, LONG flags)
{
    FIXME("(%p %x) stub\n", this, flags);
    return 0;
}

/* ?width@ios@@QAEHH@Z */
/* ?width@ios@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_width_set, 8)
int __thiscall ios_width_set(ios *this, int width)
{
    FIXME("(%p %d) stub\n", this, width);
    return 0;
}

/* ?width@ios@@QBEHXZ */
/* ?width@ios@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_width_get, 4)
int __thiscall ios_width_get(const ios *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?xalloc@ios@@SAHXZ */
int __cdecl ios_xalloc(void)
{
    FIXME("() stub\n");
    return 0;
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

/******************************************************************
 *		?str@strstreambuf@@QAEPADXZ (MSVCRTI.@)
 *           class strstreambuf & __thiscall strstreambuf::str(class strstreambuf &)
 */
DEFINE_THISCALL_WRAPPER(MSVCIRT_str_sl_void,4)
char * __thiscall MSVCIRT_str_sl_void(class_strstreambuf * _this)
{
   FIXME("(%p)->() stub\n", _this);
   return 0;
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
