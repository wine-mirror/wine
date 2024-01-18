/*
 * Copyright (C) 2007 Alexandre Julliard
 * Copyright (C) 2015-2016 Iván Matellanes
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


#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <io.h>
#include <limits.h>
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
int ios_fLockcInit = 0;
/* ?sunk_with_stdio@ios@@0HA */
int ios_sunk_with_stdio = 0;
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
    const int *vbtable;
    int unknown;
} ostream;

/* class istream */
typedef struct {
    const int *vbtable;
    int extract_delim;
    int count;
} istream;

/* class iostream */
typedef struct {
    istream base1;
    ostream base2;
} iostream;

/* ??_7streambuf@@6B@ */
extern const vtable_ptr streambuf_vtable;
/* ??_7filebuf@@6B@ */
extern const vtable_ptr filebuf_vtable;
/* ??_7strstreambuf@@6B@ */
extern const vtable_ptr strstreambuf_vtable;
/* ??_7stdiobuf@@6B@ */
extern const vtable_ptr stdiobuf_vtable;
/* ??_7ios@@6B@ */
extern const vtable_ptr ios_vtable;
/* ??_7ostream@@6B@ */
extern const vtable_ptr ostream_vtable;
/* ??_7ostream_withassign@@6B@ */
extern const vtable_ptr ostream_withassign_vtable;
/* ??_7ostrstream@@6B@ */
extern const vtable_ptr ostrstream_vtable;
/* ??_7ofstream@@6B@ */
extern const vtable_ptr ofstream_vtable;
/* ??_7istream@@6B@ */
extern const vtable_ptr istream_vtable;
/* ??_7istream_withassign@@6B@ */
extern const vtable_ptr istream_withassign_vtable;
/* ??_7istrstream@@6B@ */
extern const vtable_ptr istrstream_vtable;
/* ??_7ifstream@@6B@ */
extern const vtable_ptr ifstream_vtable;
/* ??_7iostream@@6B@ */
extern const vtable_ptr iostream_vtable;
/* ??_7strstream@@6B@ */
extern const vtable_ptr strstream_vtable;
/* ??_7stdiostream@@6B@ */
extern const vtable_ptr stdiostream_vtable;
/* ??_7fstream@@6B@ */
extern const vtable_ptr fstream_vtable;

__ASM_BLOCK_BEGIN(vtables)
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
    __ASM_VTABLE(ostream,
            VTABLE_ADD_FUNC(ostream_vector_dtor));
    __ASM_VTABLE(ostream_withassign,
            VTABLE_ADD_FUNC(ostream_vector_dtor));
    __ASM_VTABLE(ostrstream,
            VTABLE_ADD_FUNC(ostream_vector_dtor));
    __ASM_VTABLE(ofstream,
            VTABLE_ADD_FUNC(ostream_vector_dtor));
    __ASM_VTABLE(istream,
            VTABLE_ADD_FUNC(istream_vector_dtor));
    __ASM_VTABLE(istream_withassign,
            VTABLE_ADD_FUNC(istream_vector_dtor));
    __ASM_VTABLE(istrstream,
            VTABLE_ADD_FUNC(istream_vector_dtor));
    __ASM_VTABLE(ifstream,
            VTABLE_ADD_FUNC(istream_vector_dtor));
    __ASM_VTABLE(iostream,
            VTABLE_ADD_FUNC(iostream_vector_dtor));
    __ASM_VTABLE(strstream,
            VTABLE_ADD_FUNC(iostream_vector_dtor));
    __ASM_VTABLE(stdiostream,
            VTABLE_ADD_FUNC(iostream_vector_dtor));
    __ASM_VTABLE(fstream,
            VTABLE_ADD_FUNC(iostream_vector_dtor));
__ASM_BLOCK_END

#define ALIGNED_SIZE(size, alignment) (((size)+((alignment)-1))/(alignment)*(alignment))
#define VBTABLE_ENTRY(class, offset, vbase) ALIGNED_SIZE(sizeof(class), TYPE_ALIGNMENT(vbase))-offset

/* ??_8ostream@@7B@ */
/* ??_8ostream_withassign@@7B@ */
/* ??_8ostrstream@@7B@ */
/* ??_8ofstream@@7B@ */
const int ostream_vbtable[] = {0, VBTABLE_ENTRY(ostream, FIELD_OFFSET(ostream, vbtable), ios)};
/* ??_8istream@@7B@ */
/* ??_8istream_withassign@@7B@ */
/* ??_8istrstream@@7B@ */
/* ??_8ifstream@@7B@ */
const int istream_vbtable[] = {0, VBTABLE_ENTRY(istream, FIELD_OFFSET(istream, vbtable), ios)};
/* ??_8iostream@@7Bistream@@@ */
/* ??_8stdiostream@@7Bistream@@@ */
/* ??_8strstream@@7Bistream@@@ */
const int iostream_vbtable_istream[] = {0, VBTABLE_ENTRY(iostream, FIELD_OFFSET(iostream, base1), ios)};
/* ??_8iostream@@7Bostream@@@ */
/* ??_8stdiostream@@7Bostream@@@ */
/* ??_8strstream@@7Bostream@@@ */
const int iostream_vbtable_ostream[] = {0, VBTABLE_ENTRY(iostream, FIELD_OFFSET(iostream, base2), ios)};

DEFINE_RTTI_DATA0(streambuf, 0, ".?AVstreambuf@@")
DEFINE_RTTI_DATA1(filebuf, 0, &streambuf_rtti_base_descriptor, ".?AVfilebuf@@")
DEFINE_RTTI_DATA1(strstreambuf, 0, &streambuf_rtti_base_descriptor, ".?AVstrstreambuf@@")
DEFINE_RTTI_DATA1(stdiobuf, 0, &streambuf_rtti_base_descriptor, ".?AVstdiobuf@@")
DEFINE_RTTI_DATA0(ios, 0, ".?AVios@@")
DEFINE_RTTI_DATA1(ostream, sizeof(ostream), &ios_rtti_base_descriptor, ".?AVostream@@")
DEFINE_RTTI_DATA2(ostream_withassign, sizeof(ostream),
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVostream_withassign@@")
DEFINE_RTTI_DATA2(ostrstream, sizeof(ostream),
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVostrstream@@")
DEFINE_RTTI_DATA2(ofstream, sizeof(ostream),
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVofstream@@")
DEFINE_RTTI_DATA1(istream, sizeof(istream), &ios_rtti_base_descriptor, ".?AVistream@@")
DEFINE_RTTI_DATA2(istream_withassign, sizeof(istream),
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVistream_withassign@@")
DEFINE_RTTI_DATA2(istrstream, sizeof(istream),
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVistrstream@@")
DEFINE_RTTI_DATA2(ifstream, sizeof(istream),
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVifstream@@")
DEFINE_RTTI_DATA4(iostream, sizeof(iostream),
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor,
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AViostream@@")
DEFINE_RTTI_DATA5(strstream, sizeof(iostream), &iostream_rtti_base_descriptor,
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor,
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVstrstream@@")
DEFINE_RTTI_DATA5(stdiostream, sizeof(iostream), &iostream_rtti_base_descriptor,
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor,
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVstdiostream@@")
DEFINE_RTTI_DATA5(fstream, sizeof(iostream), &iostream_rtti_base_descriptor,
    &istream_rtti_base_descriptor, &ios_rtti_base_descriptor,
    &ostream_rtti_base_descriptor, &ios_rtti_base_descriptor, ".?AVfstream@@")

/* ?cin@@3Vistream_withassign@@A */
struct {
    istream is;
    ios vbase;
} cin = { { 0 } };

/* ?cout@@3Vostream_withassign@@A */
/* ?cerr@@3Vostream_withassign@@A */
/* ?clog@@3Vostream_withassign@@A */
struct {
    ostream os;
    ios vbase;
} cout = { { 0 } }, cerr = { { 0 } }, MSVCP_clog = { { 0 } };


/* ??0streambuf@@IAE@PADH@Z */
/* ??0streambuf@@IEAA@PEADH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_reserve_ctor, 12)
streambuf* __thiscall streambuf_reserve_ctor(streambuf *this, char *buffer, int length)
{
    TRACE("(%p %p %d)\n", this, buffer, length);
    this->vtable = &streambuf_vtable;
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
    this->vtable = &streambuf_vtable;
    return this;
}

/* ??1streambuf@@UAE@XZ */
/* ??1streambuf@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(streambuf_dtor, 4)
void __thiscall streambuf_dtor(streambuf *this)
{
    TRACE("(%p)\n", this);
    if (this->allocated)
        operator_delete(this->base);
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
        operator_delete(ptr);
    } else {
        streambuf_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gstreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(streambuf_scalar_dtor, 8)
streambuf* __thiscall streambuf_scalar_dtor(streambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    streambuf_dtor(this);
    if (flags & 1) operator_delete(this);
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
    reserve = operator_new(RESERVE_SIZE);
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
    return (this->egptr - this->gptr > 0) ? this->egptr - this->gptr : 0;
}

/* ?out_waiting@streambuf@@QBEHXZ */
/* ?out_waiting@streambuf@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(streambuf_out_waiting, 4)
int __thiscall streambuf_out_waiting(const streambuf *this)
{
    TRACE("(%p)\n", this);
    return (this->pptr - this->pbase > 0) ? this->pptr - this->pbase : 0;
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
    TRACE("(%p %ld %d %d)\n", this, offset, dir, mode);
    return EOF;
}

/* ?seekpos@streambuf@@UAEJJH@Z */
/* ?seekpos@streambuf@@UEAAJJH@Z */
DEFINE_THISCALL_WRAPPER(streambuf_seekpos, 12)
streampos __thiscall streambuf_seekpos(streambuf *this, streampos pos, int mode)
{
    TRACE("(%p %ld %d)\n", this, pos, mode);
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
        operator_delete(this->base);
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
            if (call_streambuf_overflow(this, (unsigned char)data[copied]) == EOF)
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
    return (this->pptr < this->epptr) ? (unsigned char)(*this->pptr++ = ch) : call_streambuf_overflow(this, ch);
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
        return (this->gptr < this->egptr) ? (unsigned char)(*this->gptr) : call_streambuf_underflow(this);
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
        ret = (this->gptr < this->egptr) ? (unsigned char)(*this->gptr) : call_streambuf_underflow(this);
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
        printf("pbase()=%p, pptr()=%p, epptr()=%p\n", this->pbase, this->pptr, this->epptr);
        printf("eback()=%p, gptr()=%p, egptr()=%p\n", this->eback, this->gptr, this->egptr);
    }
}

/* ??0filebuf@@QAE@ABV0@@Z */
/* ??0filebuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(filebuf_copy_ctor, 8)
filebuf* __thiscall filebuf_copy_ctor(filebuf* this, const filebuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &filebuf_vtable;
    return this;
}

/* ??0filebuf@@QAE@HPADH@Z */
/* ??0filebuf@@QEAA@HPEADH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_fd_reserve_ctor, 16)
filebuf* __thiscall filebuf_fd_reserve_ctor(filebuf* this, filedesc fd, char *buffer, int length)
{
    TRACE("(%p %d %p %d)\n", this, fd, buffer, length);
    streambuf_reserve_ctor(&this->base, buffer, length);
    this->base.vtable = &filebuf_vtable;
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
        operator_delete(ptr);
    } else {
        filebuf_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gfilebuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(filebuf_scalar_dtor, 8)
filebuf* __thiscall filebuf_scalar_dtor(filebuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    filebuf_dtor(this);
    if (flags & 1) operator_delete(this);
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
    TRACE("(%p %ld %d %d)\n", this, offset, dir, mode);
    if (call_streambuf_sync(&this->base) == EOF)
        return EOF;
    return _lseek(this->fd, offset, dir);
}

/* ?setbuf@filebuf@@UAEPAVstreambuf@@PADH@Z */
/* ?setbuf@filebuf@@UEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(filebuf_setbuf, 12)
streambuf* __thiscall filebuf_setbuf(filebuf *this, char *buffer, int length)
{
    TRACE("(%p %p %d)\n", this, buffer, length);

    if (filebuf_is_open(this) && this->base.base != NULL)
        return NULL;

    streambuf_lock(&this->base);

    if (buffer == NULL || !length) {
        this->base.unbuffered = 1;
    } else {
        if (this->base.allocated) {
            operator_delete(this->base.base);
            this->base.allocated = 0;
        }

        this->base.base = buffer;
        this->base.ebuf = buffer + length;
    }

    streambuf_unlock(&this->base);

    return &this->base;
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
    }
    this->base.pbase = this->base.pptr = this->base.epptr = NULL;
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
    }
    this->base.eback = this->base.gptr = this->base.egptr = NULL;
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
        return (_read(this->fd, &c, 1) < 1) ? EOF : (unsigned char) c;

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
    return (unsigned char) *this->base.gptr;
}

/* ??0strstreambuf@@QAE@ABV0@@Z */
/* ??0strstreambuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_copy_ctor, 8)
strstreambuf* __thiscall strstreambuf_copy_ctor(strstreambuf *this, const strstreambuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &strstreambuf_vtable;
    return this;
}

/* ??0strstreambuf@@QAE@H@Z */
/* ??0strstreambuf@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_dynamic_ctor, 8)
strstreambuf* __thiscall strstreambuf_dynamic_ctor(strstreambuf* this, int length)
{
    TRACE("(%p %d)\n", this, length);
    streambuf_ctor(&this->base);
    this->base.vtable = &strstreambuf_vtable;
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
    this->base.vtable = &strstreambuf_vtable;
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
            operator_delete(this->base.base);
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
        operator_delete(ptr);
    } else {
        strstreambuf_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gstrstreambuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_scalar_dtor, 8)
strstreambuf* __thiscall strstreambuf_scalar_dtor(strstreambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    strstreambuf_dtor(this);
    if (flags & 1) operator_delete(this);
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
        new_buffer = operator_new(new_size);
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
            operator_delete(this->base.base);
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

    TRACE("(%p %ld %d %d)\n", this, offset, dir, mode);

    if ((unsigned int)dir > SEEKDIR_end || !(mode & (OPENMODE_in|OPENMODE_out)))
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
        return (unsigned char) *this->base.gptr;
    /* extend the get area to include the characters written */
    if (this->base.egptr < this->base.pptr) {
        this->base.gptr = this->base.base + (this->base.gptr - this->base.eback);
        this->base.eback = this->base.base;
        this->base.egptr = this->base.pptr;
    }
    return (this->base.gptr < this->base.egptr) ? (unsigned char)(*this->base.gptr) : EOF;
}

/* ??0stdiobuf@@QAE@ABV0@@Z */
/* ??0stdiobuf@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_copy_ctor, 8)
stdiobuf* __thiscall stdiobuf_copy_ctor(stdiobuf *this, const stdiobuf *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->base.vtable = &stdiobuf_vtable;
    return this;
}

/* ??0stdiobuf@@QAE@PAU_iobuf@@@Z */
/* ??0stdiobuf@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_file_ctor, 8)
stdiobuf* __thiscall stdiobuf_file_ctor(stdiobuf *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);
    streambuf_reserve_ctor(&this->base, NULL, 0);
    this->base.vtable = &stdiobuf_vtable;
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
        operator_delete(ptr);
    } else {
        stdiobuf_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gstdiobuf@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(stdiobuf_scalar_dtor, 8)
stdiobuf* __thiscall stdiobuf_scalar_dtor(stdiobuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    stdiobuf_dtor(this);
    if (flags & 1) operator_delete(this);
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
    TRACE("(%p %ld %d %d)\n", this, offset, dir, mode);
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
    reserve = operator_new(buffer_size);
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
    return (unsigned char) *this->base.gptr++;
}

/* ??0ios@@IAE@ABV0@@Z */
/* ??0ios@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ios_copy_ctor, 8)
ios* __thiscall ios_copy_ctor(ios *this, const ios *copy)
{
    TRACE("(%p %p)\n", this, copy);
    ios_fLockcInit++;
    this->vtable = &ios_vtable;
    this->sb = NULL;
    this->delbuf = 0;
    this->do_lock = -1;
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
    this->vtable = &ios_vtable;
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
        operator_delete(ptr);
    } else {
        ios_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gios@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_scalar_dtor, 8)
ios* __thiscall ios_scalar_dtor(ios *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    ios_dtor(this);
    if (flags & 1) operator_delete(this);
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

    TRACE("(%p %lx)\n", this, flags);

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

    TRACE("(%p %lx)\n", this, flags);

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

    TRACE("(%p %lx %lx)\n", this, flags, mask);

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

    TRACE("(%p %lx)\n", this, flags);

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

static inline ios* ostream_get_ios(const ostream *this)
{
    return (ios*)((char*)this + this->vbtable[1]);
}

static inline ios* ostream_to_ios(const ostream *this)
{
    return (ios*)((char*)this + ostream_vbtable[1]);
}

static inline ostream* ios_to_ostream(const ios *base)
{
    return (ostream*)((char*)base - ostream_vbtable[1]);
}

/* ??0ostream@@IAE@XZ */
/* ??0ostream@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostream_ctor, 8)
ostream* __thiscall ostream_ctor(ostream *this, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %d)\n", this, virt_init);

    if (virt_init) {
        this->vbtable = ostream_vbtable;
        base = ostream_get_ios(this);
        ios_ctor(base);
    } else
        base = ostream_get_ios(this);
    base->vtable = &ostream_vtable;
    this->unknown = 0;
    return this;
}

/* ??0ostream@@QAE@PAVstreambuf@@@Z */
/* ??0ostream@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ostream_sb_ctor, 12)
ostream* __thiscall ostream_sb_ctor(ostream *this, streambuf *sb, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, sb, virt_init);
    ostream_ctor(this, virt_init);
    ios_init(ostream_get_ios(this), sb);
    return this;
}

/* ??0ostream@@IAE@ABV0@@Z */
/* ??0ostream@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ostream_copy_ctor, 12)
ostream* __thiscall ostream_copy_ctor(ostream *this, const ostream *copy, BOOL virt_init)
{
    return ostream_sb_ctor(this, ostream_get_ios(copy)->sb, virt_init);
}

/* ??1ostream@@UAE@XZ */
/* ??1ostream@@UEAA@XZ */
/* ??1ostream_withassign@@UAE@XZ */
/* ??1ostream_withassign@@UEAA@XZ */
/* ??1ostrstream@@UAE@XZ */
/* ??1ostrstream@@UEAA@XZ */
/* ??1ofstream@@UAE@XZ */
/* ??1ofstream@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostream_dtor, 4)
void __thiscall ostream_dtor(ios *base)
{
    ostream *this = ios_to_ostream(base);

    TRACE("(%p)\n", this);
}

/* ??4ostream@@IAEAAV0@PAVstreambuf@@@Z */
/* ??4ostream@@IEAAAEAV0@PEAVstreambuf@@@Z */
/* ??4ostream_withassign@@QAEAAVostream@@PAVstreambuf@@@Z */
/* ??4ostream_withassign@@QEAAAEAVostream@@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ostream_assign_sb, 8)
ostream* __thiscall ostream_assign_sb(ostream *this, streambuf *sb)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p %p)\n", this, sb);

    ios_init(base, sb);
    base->state &= IOSTATE_badbit;
    base->delbuf = 0;
    base->tie = NULL;
    base->flags = 0;
    base->precision = 6;
    base->fill = ' ';
    base->width = 0;
    return this;
}

/* ??4ostream@@IAEAAV0@ABV0@@Z */
/* ??4ostream@@IEAAAEAV0@AEBV0@@Z */
/* ??4ostream_withassign@@QAEAAV0@ABV0@@Z */
/* ??4ostream_withassign@@QEAAAEAV0@AEBV0@@Z */
/* ??4ostream_withassign@@QAEAAVostream@@ABV1@@Z */
/* ??4ostream_withassign@@QEAAAEAVostream@@AEBV1@@Z */
/* ??4ostrstream@@QAEAAV0@ABV0@@Z */
/* ??4ostrstream@@QEAAAEAV0@AEBV0@@Z */
/* ??4ofstream@@QAEAAV0@ABV0@@Z */
/* ??4ofstream@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ostream_assign, 8)
ostream* __thiscall ostream_assign(ostream *this, const ostream *rhs)
{
    return ostream_assign_sb(this, ostream_get_ios(rhs)->sb);
}

/* ??_Dostream@@QAEXXZ */
/* ??_Dostream@@QEAAXXZ */
/* ??_Dostream_withassign@@QAEXXZ */
/* ??_Dostream_withassign@@QEAAXXZ */
/* ??_Dostrstream@@QAEXXZ */
/* ??_Dostrstream@@QEAAXXZ */
/* ??_Dofstream@@QAEXXZ */
/* ??_Dofstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ostream_vbase_dtor, 4)
void __thiscall ostream_vbase_dtor(ostream *this)
{
    ios *base = ostream_to_ios(this);

    TRACE("(%p)\n", this);

    ostream_dtor(base);
    ios_dtor(base);
}

/* ??_Eostream@@UAEPAXI@Z */
/* ??_Eostream_withassign@@UAEPAXI@Z */
/* ??_Eostrstream@@UAEPAXI@Z */
/* ??_Eofstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(ostream_vector_dtor, 8)
ostream* __thiscall ostream_vector_dtor(ios *base, unsigned int flags)
{
    ostream *this = ios_to_ostream(base);

    TRACE("(%p %x)\n", this, flags);

    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            ostream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        ostream_vbase_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gostream@@UAEPAXI@Z */
/* ??_Gostream_withassign@@UAEPAXI@Z */
/* ??_Gostrstream@@UAEPAXI@Z */
/* ??_Gofstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(ostream_scalar_dtor, 8)
ostream* __thiscall ostream_scalar_dtor(ios *base, unsigned int flags)
{
    ostream *this = ios_to_ostream(base);

    TRACE("(%p %x)\n", this, flags);

    ostream_vbase_dtor(this);
    if (flags & 1) operator_delete(this);
    return this;
}

/* ?flush@ostream@@QAEAAV1@XZ */
/* ?flush@ostream@@QEAAAEAV1@XZ */
DEFINE_THISCALL_WRAPPER(ostream_flush, 4)
ostream* __thiscall ostream_flush(ostream *this)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p)\n", this);

    ios_lockbuf(base);
    if (call_streambuf_sync(base->sb) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return this;
}

/* ?opfx@ostream@@QAEHXZ */
/* ?opfx@ostream@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(ostream_opfx, 4)
int __thiscall ostream_opfx(ostream *this)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p)\n", this);

    if (!ios_good(base)) {
        ios_clear(base, base->state | IOSTATE_failbit);
        return 0;
    }
    ios_lock(base);
    ios_lockbuf(base);
    if (base->tie)
        ostream_flush(base->tie);
    return 1;
}

/* ?osfx@ostream@@QAEXXZ */
/* ?osfx@ostream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ostream_osfx, 4)
void __thiscall ostream_osfx(ostream *this)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p)\n", this);

    ios_unlockbuf(base);
    ios_width_set(base, 0);
    if (base->flags & FLAGS_unitbuf)
        ostream_flush(this);
    if (base->flags & FLAGS_stdio) {
        fflush(stdout);
        fflush(stderr);
    }
    ios_unlock(base);
}

/* ?put@ostream@@QAEAAV1@C@Z */
/* ?put@ostream@@QEAAAEAV1@C@Z */
/* ?put@ostream@@QAEAAV1@D@Z */
/* ?put@ostream@@QEAAAEAV1@D@Z */
/* ?put@ostream@@QAEAAV1@E@Z */
/* ?put@ostream@@QEAAAEAV1@E@Z */
DEFINE_THISCALL_WRAPPER(ostream_put, 8)
ostream* __thiscall ostream_put(ostream *this, char c)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p %c)\n", this, c);

    if (ostream_opfx(this)) {
        if (streambuf_sputc(base->sb, c) == EOF)
            base->state = IOSTATE_badbit | IOSTATE_failbit;
        ostream_osfx(this);
    }
    return this;
}

/* ?seekp@ostream@@QAEAAV1@J@Z */
/* ?seekp@ostream@@QEAAAEAV1@J@Z */
DEFINE_THISCALL_WRAPPER(ostream_seekp, 8)
ostream* __thiscall ostream_seekp(ostream *this, streampos pos)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p %ld)\n", this, pos);

    ios_lockbuf(base);
    if (streambuf_seekpos(base->sb, pos, OPENMODE_out) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return this;
}

/* ?seekp@ostream@@QAEAAV1@JW4seek_dir@ios@@@Z */
/* ?seekp@ostream@@QEAAAEAV1@JW4seek_dir@ios@@@Z */
DEFINE_THISCALL_WRAPPER(ostream_seekp_offset, 12)
ostream* __thiscall ostream_seekp_offset(ostream *this, streamoff off, ios_seek_dir dir)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p %ld %d)\n", this, off, dir);

    ios_lockbuf(base);
    if (call_streambuf_seekoff(base->sb, off, dir, OPENMODE_out) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return this;
}

/* ?tellp@ostream@@QAEJXZ */
/* ?tellp@ostream@@QEAAJXZ */
DEFINE_THISCALL_WRAPPER(ostream_tellp, 4)
streampos __thiscall ostream_tellp(ostream *this)
{
    ios *base = ostream_get_ios(this);
    streampos pos;

    TRACE("(%p)\n", this);

    ios_lockbuf(base);
    if ((pos = call_streambuf_seekoff(base->sb, 0, SEEKDIR_cur, OPENMODE_out)) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return pos;
}

/* ?write@ostream@@QAEAAV1@PBCH@Z */
/* ?write@ostream@@QEAAAEAV1@PEBCH@Z */
/* ?write@ostream@@QAEAAV1@PBDH@Z */
/* ?write@ostream@@QEAAAEAV1@PEBDH@Z */
/* ?write@ostream@@QAEAAV1@PBEH@Z */
/* ?write@ostream@@QEAAAEAV1@PEBEH@Z */
DEFINE_THISCALL_WRAPPER(ostream_write, 12)
ostream* __thiscall ostream_write(ostream *this, const char *str, int count)
{
    ios *base = ostream_get_ios(this);

    TRACE("(%p %p %d)\n", this, str, count);

    if (ostream_opfx(this)) {
        if (streambuf_sputn(base->sb, str, count) != count)
            base->state = IOSTATE_badbit | IOSTATE_failbit;
        ostream_osfx(this);
    }
    return this;
}

static ostream* ostream_writepad_len(ostream *this, const char *str1, const char *str2, int len2)
{
    ios *base = ostream_get_ios(this);
    int len1 = strlen(str1), i;

    TRACE("(%p %p %p %d)\n", this, str1, str2, len2);

    /* left of the padding */
    if (base->flags & (FLAGS_left|FLAGS_internal)) {
        if (streambuf_sputn(base->sb, str1, len1) != len1)
            base->state |= IOSTATE_failbit | IOSTATE_badbit;
        if (!(base->flags & FLAGS_internal))
            if (streambuf_sputn(base->sb, str2, len2) != len2)
                base->state |= IOSTATE_failbit | IOSTATE_badbit;
    }
    /* add padding to fill the width */
    for (i = len1 + len2; i < base->width; i++)
        if (streambuf_sputc(base->sb, base->fill) == EOF)
            base->state |= IOSTATE_failbit | IOSTATE_badbit;
    /* right of the padding */
    if ((base->flags & (FLAGS_left|FLAGS_internal)) != FLAGS_left) {
        if (!(base->flags & (FLAGS_left|FLAGS_internal)))
            if (streambuf_sputn(base->sb, str1, len1) != len1)
                base->state |= IOSTATE_failbit | IOSTATE_badbit;
        if (streambuf_sputn(base->sb, str2, len2) != len2)
            base->state |= IOSTATE_failbit | IOSTATE_badbit;
    }
    return this;
}

/* ?writepad@ostream@@AAEAAV1@PBD0@Z */
/* ?writepad@ostream@@AEAAAEAV1@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(ostream_writepad, 12)
ostream* __thiscall ostream_writepad(ostream *this, const char *str1, const char *str2)
{
    return ostream_writepad_len(this, str1, str2, strlen(str2));
}

static ostream* ostream_internal_print_integer(ostream *ostr, int n, BOOL unsig, BOOL shrt)
{
    ios *base = ostream_get_ios(ostr);
    char prefix_str[3] = {0}, number_str[12], sprintf_fmt[4] = {'%','d',0};

    TRACE("(%p %d %d %d)\n", ostr, n, unsig, shrt);

    if (ostream_opfx(ostr)) {
        if (base->flags & FLAGS_hex) {
            sprintf_fmt[1] = (base->flags & FLAGS_uppercase) ? 'X' : 'x';
            if (base->flags & FLAGS_showbase) {
                prefix_str[0] = '0';
                prefix_str[1] = (base->flags & FLAGS_uppercase) ? 'X' : 'x';
            }
        } else if (base->flags & FLAGS_oct) {
            sprintf_fmt[1]  = 'o';
            if (base->flags & FLAGS_showbase)
                prefix_str[0] = '0';
        } else { /* FLAGS_dec */
            if (unsig)
                sprintf_fmt[1] = 'u';
            if ((base->flags & FLAGS_showpos) && n != 0 && (unsig || n > 0))
                prefix_str[0] = '+';
        }

        if (shrt) {
            sprintf_fmt[2] = sprintf_fmt[1];
            sprintf_fmt[1] = 'h';
        }

        if (sprintf(number_str, sprintf_fmt, n) > 0)
            ostream_writepad(ostr, prefix_str, number_str);
        else
            base->state |= IOSTATE_failbit;
        ostream_osfx(ostr);
    }
    return ostr;
}

static ostream* ostream_internal_print_float(ostream *ostr, double d, BOOL dbl)
{
    ios *base = ostream_get_ios(ostr);
    char prefix_str[2] = {0}, number_str[24], sprintf_fmt[6] = {'%','.','*','f',0};
    int prec, max_prec = dbl ? 15 : 6;
    int str_length = 1; /* null end char */

    TRACE("(%p %lf %d)\n", ostr, d, dbl);

    if (ostream_opfx(ostr)) {
        if ((base->flags & FLAGS_showpos) && d > 0) {
            prefix_str[0] = '+';
            str_length++; /* plus sign */
        }
        if ((base->flags & (FLAGS_scientific|FLAGS_fixed)) == FLAGS_scientific)
            sprintf_fmt[3] = (base->flags & FLAGS_uppercase) ? 'E' : 'e';
        else if ((base->flags & (FLAGS_scientific|FLAGS_fixed)) != FLAGS_fixed)
            sprintf_fmt[3] = (base->flags & FLAGS_uppercase) ? 'G' : 'g';
        if (base->flags & FLAGS_showpoint) {
            sprintf_fmt[4] = sprintf_fmt[3];
            sprintf_fmt[3] = sprintf_fmt[2];
            sprintf_fmt[2] = sprintf_fmt[1];
            sprintf_fmt[1] = '#';
        }

        prec = (base->precision >= 0 && base->precision <= max_prec) ? base->precision : max_prec;
        str_length += _scprintf(sprintf_fmt, prec, d); /* number representation */
        if (str_length > 24) {
            /* when the output length exceeds 24 characters, Windows prints an empty string with padding */
            ostream_writepad(ostr, "", "");
        } else {
            if (sprintf(number_str, sprintf_fmt, prec, d) > 0)
                ostream_writepad(ostr, prefix_str, number_str);
            else
                base->state |= IOSTATE_failbit;
        }
        ostream_osfx(ostr);
    }
    return ostr;
}

/* ??6ostream@@QAEAAV0@C@Z */
/* ??6ostream@@QEAAAEAV0@C@Z */
/* ??6ostream@@QAEAAV0@D@Z */
/* ??6ostream@@QEAAAEAV0@D@Z */
/* ??6ostream@@QAEAAV0@E@Z */
/* ??6ostream@@QEAAAEAV0@E@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_char, 8)
ostream* __thiscall ostream_print_char(ostream *this, char c)
{
    TRACE("(%p %d)\n", this, c);

    if (ostream_opfx(this)) {
        ostream_writepad_len(this, "", &c, 1);
        ostream_osfx(this);
    }
    return this;
}

/* ??6ostream@@QAEAAV0@PBC@Z */
/* ??6ostream@@QEAAAEAV0@PEBC@Z */
/* ??6ostream@@QAEAAV0@PBD@Z */
/* ??6ostream@@QEAAAEAV0@PEBD@Z */
/* ??6ostream@@QAEAAV0@PBE@Z */
/* ??6ostream@@QEAAAEAV0@PEBE@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_str, 8)
ostream* __thiscall ostream_print_str(ostream *this, const char *str)
{
    TRACE("(%p %s)\n", this, str);
    if (ostream_opfx(this)) {
        ostream_writepad(this, "", str);
        ostream_osfx(this);
    }
    return this;
}

/* ??6ostream@@QAEAAV0@F@Z */
/* ??6ostream@@QEAAAEAV0@F@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_short, 8)
ostream* __thiscall ostream_print_short(ostream *this, short n)
{
    return ostream_internal_print_integer(this, n, FALSE, TRUE);
}

/* ??6ostream@@QAEAAV0@G@Z */
/* ??6ostream@@QEAAAEAV0@G@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_unsigned_short, 8)
ostream* __thiscall ostream_print_unsigned_short(ostream *this, unsigned short n)
{
    return ostream_internal_print_integer(this, n, TRUE, TRUE);
}

/* ??6ostream@@QAEAAV0@H@Z */
/* ??6ostream@@QEAAAEAV0@H@Z */
/* ??6ostream@@QAEAAV0@J@Z */
/* ??6ostream@@QEAAAEAV0@J@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_int, 8)
ostream* __thiscall ostream_print_int(ostream *this, int n)
{
    return ostream_internal_print_integer(this, n, FALSE, FALSE);
}

/* ??6ostream@@QAEAAV0@I@Z */
/* ??6ostream@@QEAAAEAV0@I@Z */
/* ??6ostream@@QAEAAV0@K@Z */
/* ??6ostream@@QEAAAEAV0@K@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_unsigned_int, 8)
ostream* __thiscall ostream_print_unsigned_int(ostream *this, unsigned int n)
{
    return ostream_internal_print_integer(this, n, TRUE, FALSE);
}

/* ??6ostream@@QAEAAV0@M@Z */
/* ??6ostream@@QEAAAEAV0@M@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_float, 8)
ostream* __thiscall ostream_print_float(ostream *this, float f)
{
    return ostream_internal_print_float(this, f, FALSE);
}

/* ??6ostream@@QAEAAV0@N@Z */
/* ??6ostream@@QEAAAEAV0@N@Z */
/* ??6ostream@@QAEAAV0@O@Z */
/* ??6ostream@@QEAAAEAV0@O@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_double, 12)
ostream* __thiscall ostream_print_double(ostream *this, double d)
{
    return ostream_internal_print_float(this, d, TRUE);
}

/* ??6ostream@@QAEAAV0@PBX@Z */
/* ??6ostream@@QEAAAEAV0@PEBX@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_ptr, 8)
ostream* __thiscall ostream_print_ptr(ostream *this, const void *ptr)
{
    ios *base = ostream_get_ios(this);
    char prefix_str[3] = {'0','x',0}, pointer_str[17];

    TRACE("(%p %p)\n", this, ptr);

    if (ostream_opfx(this)) {
        if (ptr && base->flags & FLAGS_uppercase)
            prefix_str[1] = 'X';

        if (sprintf(pointer_str, "%p", ptr) > 0)
            ostream_writepad(this, prefix_str, pointer_str);
        else
            base->state |= IOSTATE_failbit;
        ostream_osfx(this);
    }
    return this;
}

/* ??6ostream@@QAEAAV0@PAVstreambuf@@@Z */
/* ??6ostream@@QEAAAEAV0@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_streambuf, 8)
ostream* __thiscall ostream_print_streambuf(ostream *this, streambuf *sb)
{
    ios *base = ostream_get_ios(this);
    int c;

    TRACE("(%p %p)\n", this, sb);

    if (ostream_opfx(this)) {
        while ((c = streambuf_sbumpc(sb)) != EOF) {
            if (streambuf_sputc(base->sb, c) == EOF) {
                base->state |= IOSTATE_failbit;
                break;
            }
        }
        ostream_osfx(this);
    }
    return this;
}

/* ??6ostream@@QAEAAV0@P6AAAV0@AAV0@@Z@Z */
/* ??6ostream@@QEAAAEAV0@P6AAEAV0@AEAV0@@Z@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_manip, 8)
ostream* __thiscall ostream_print_manip(ostream *this, ostream* (__cdecl *func)(ostream*))
{
    TRACE("(%p %p)\n", this, func);
    return func(this);
}

/* ??6ostream@@QAEAAV0@P6AAAVios@@AAV1@@Z@Z */
/* ??6ostream@@QEAAAEAV0@P6AAEAVios@@AEAV1@@Z@Z */
DEFINE_THISCALL_WRAPPER(ostream_print_ios_manip, 8)
ostream* __thiscall ostream_print_ios_manip(ostream *this, ios* (__cdecl *func)(ios*))
{
    TRACE("(%p %p)\n", this, func);
    func(ostream_get_ios(this));
    return this;
}

/* ?endl@@YAAAVostream@@AAV1@@Z */
/* ?endl@@YAAEAVostream@@AEAV1@@Z */
ostream* __cdecl ostream_endl(ostream *this)
{
   TRACE("(%p)\n", this);
   ostream_put(this, '\n');
   return ostream_flush(this);
}

/* ?ends@@YAAAVostream@@AAV1@@Z */
/* ?ends@@YAAEAVostream@@AEAV1@@Z */
ostream* __cdecl ostream_ends(ostream *this)
{
   TRACE("(%p)\n", this);
   return ostream_put(this, 0);
}

/* ?flush@@YAAAVostream@@AAV1@@Z */
/* ?flush@@YAAEAVostream@@AEAV1@@Z */
ostream* __cdecl ostream_flush_manip(ostream *this)
{
   TRACE("(%p)\n", this);
   return ostream_flush(this);
}

/* ??0ostream_withassign@@QAE@ABV0@@Z */
/* ??0ostream_withassign@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ostream_withassign_copy_ctor, 12)
ostream* __thiscall ostream_withassign_copy_ctor(ostream *this, const ostream *copy, BOOL virt_init)
{
    ios *base, *base_copy;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    base_copy = ostream_get_ios(copy);
    if (virt_init) {
        this->vbtable = ostream_vbtable;
        base = ostream_get_ios(this);
        ios_copy_ctor(base, base_copy);
    } else
        base = ostream_get_ios(this);
    ios_init(base, base_copy->sb);
    base->vtable = &ostream_withassign_vtable;
    this->unknown = 0;
    return this;
}

/* ??0ostream_withassign@@QAE@PAVstreambuf@@@Z */
/* ??0ostream_withassign@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(ostream_withassign_sb_ctor, 12)
ostream* __thiscall ostream_withassign_sb_ctor(ostream *this, streambuf *sb, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %p %d)\n", this, sb, virt_init);

    ostream_sb_ctor(this, sb, virt_init);
    base = ostream_get_ios(this);
    base->vtable = &ostream_withassign_vtable;
    return this;
}

/* ??0ostream_withassign@@QAE@XZ */
/* ??0ostream_withassign@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostream_withassign_ctor, 8)
ostream* __thiscall ostream_withassign_ctor(ostream *this, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %d)\n", this, virt_init);

    ostream_ctor(this, virt_init);
    base = ostream_get_ios(this);
    base->vtable = &ostream_withassign_vtable;
    return this;
}

static ostream* ostrstream_internal_sb_ctor(ostream *this, strstreambuf *ssb, BOOL virt_init)
{
    ios *base;

    if (ssb)
        ostream_sb_ctor(this, &ssb->base, virt_init);
    else
        ostream_ctor(this, virt_init);
    base = ostream_get_ios(this);
    base->vtable = &ostrstream_vtable;
    base->delbuf = 1;
    return this;
}

/* ??0ostrstream@@QAE@ABV0@@Z */
/* ??0ostrstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ostrstream_copy_ctor, 12)
ostream* __thiscall ostrstream_copy_ctor(ostream *this, const ostream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    ostream_withassign_copy_ctor(this, copy, virt_init);
    ostream_get_ios(this)->vtable = &ostrstream_vtable;
    return this;
}

/* ??0ostrstream@@QAE@PADHH@Z */
/* ??0ostrstream@@QEAA@PEADHH@Z */
DEFINE_THISCALL_WRAPPER(ostrstream_buffer_ctor, 20)
ostream* __thiscall ostrstream_buffer_ctor(ostream *this, char *buffer, int length, int mode, BOOL virt_init)
{
    strstreambuf *ssb = operator_new(sizeof(strstreambuf));

    TRACE("(%p %p %d %d %d)\n", this, buffer, length, mode, virt_init);

    if (!ssb) {
        FIXME("out of memory\n");
        return NULL;
    }

    strstreambuf_buffer_ctor(ssb, buffer, length, buffer);
    if (mode & (OPENMODE_app|OPENMODE_ate))
        ssb->base.pptr = buffer + strlen(buffer);

    return ostrstream_internal_sb_ctor(this, ssb, virt_init);
}

/* ??0ostrstream@@QAE@XZ */
/* ??0ostrstream@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostrstream_ctor, 8)
ostream* __thiscall ostrstream_ctor(ostream *this, BOOL virt_init)
{
    strstreambuf *ssb = operator_new(sizeof(strstreambuf));

    TRACE("(%p %d)\n", this, virt_init);

    if (!ssb) {
        FIXME("out of memory\n");
        return NULL;
    }

    strstreambuf_ctor(ssb);

    return ostrstream_internal_sb_ctor(this, ssb, virt_init);
}

/* ?pcount@ostrstream@@QBEHXZ */
/* ?pcount@ostrstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ostrstream_pcount, 4)
int __thiscall ostrstream_pcount(const ostream *this)
{
    return streambuf_out_waiting(ostream_get_ios(this)->sb);
}

/* ?rdbuf@ostrstream@@QBEPAVstrstreambuf@@XZ */
/* ?rdbuf@ostrstream@@QEBAPEAVstrstreambuf@@XZ */
DEFINE_THISCALL_WRAPPER(ostrstream_rdbuf, 4)
strstreambuf* __thiscall ostrstream_rdbuf(const ostream *this)
{
    return (strstreambuf*) ostream_get_ios(this)->sb;
}

/* ?str@ostrstream@@QAEPADXZ */
/* ?str@ostrstream@@QEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(ostrstream_str, 4)
char* __thiscall ostrstream_str(ostream *this)
{
    return strstreambuf_str(ostrstream_rdbuf(this));
}

/* ??0ofstream@@QAE@ABV0@@Z */
/* ??0ofstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ofstream_copy_ctor, 12)
ostream* __thiscall ofstream_copy_ctor(ostream *this, const ostream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    ostream_withassign_copy_ctor(this, copy, virt_init);
    ostream_get_ios(this)->vtable = &ofstream_vtable;
    return this;
}

/* ??0ofstream@@QAE@HPADH@Z */
/* ??0ofstream@@QEAA@HPEADH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_buffer_ctor, 20)
ostream* __thiscall ofstream_buffer_ctor(ostream *this, filedesc fd, char *buffer, int length, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %p %d %d)\n", this, fd, buffer, length, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_reserve_ctor(fb, fd, buffer, length);
    ostream_sb_ctor(this, &fb->base, virt_init);

    base = ostream_get_ios(this);
    base->vtable = &ofstream_vtable;
    base->delbuf = 1;

    return this;
}

/* ??0ofstream@@QAE@H@Z */
/* ??0ofstream@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(ofstream_fd_ctor, 12)
ostream* __thiscall ofstream_fd_ctor(ostream *this, filedesc fd, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %d)\n", this, fd, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_ctor(fb, fd);
    ostream_sb_ctor(this, &fb->base, virt_init);

    base = ostream_get_ios(this);
    base->vtable = &ofstream_vtable;
    base->delbuf = 1;

    return this;
}

/* ??0ofstream@@QAE@PBDHH@Z */
/* ??0ofstream@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_open_ctor, 20)
ostream* __thiscall ofstream_open_ctor(ostream *this, const char *name, int mode, int protection, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %s %d %d %d)\n", this, name, mode, protection, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_ctor(fb);
    ostream_sb_ctor(this, &fb->base, virt_init);

    base = ostream_get_ios(this);
    base->vtable = &ofstream_vtable;
    base->delbuf = 1;

    if (filebuf_open(fb, name, mode|OPENMODE_out, protection) == NULL)
        base->state |= IOSTATE_failbit;
    return this;
}

/* ??0ofstream@@QAE@XZ */
/* ??0ofstream@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ofstream_ctor, 8)
ostream* __thiscall ofstream_ctor(ostream *this, BOOL virt_init)
{
    return ofstream_fd_ctor(this, -1, virt_init);
}

/* ?rdbuf@ofstream@@QBEPAVfilebuf@@XZ */
/* ?rdbuf@ofstream@@QEBAPEAVfilebuf@@XZ */
DEFINE_THISCALL_WRAPPER(ofstream_rdbuf, 4)
filebuf* __thiscall ofstream_rdbuf(const ostream *this)
{
    TRACE("(%p)\n", this);
    return (filebuf*) ostream_get_ios(this)->sb;
}

/* ?fd@ofstream@@QBEHXZ */
/* ?fd@ofstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ofstream_fd, 4)
filedesc __thiscall ofstream_fd(ostream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_fd(ofstream_rdbuf(this));
}

/* ?attach@ofstream@@QAEXH@Z */
/* ?attach@ofstream@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_attach, 8)
void __thiscall ofstream_attach(ostream *this, filedesc fd)
{
    ios *base = ostream_get_ios(this);
    TRACE("(%p %d)\n", this, fd);
    if (filebuf_attach(ofstream_rdbuf(this), fd) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?close@ofstream@@QAEXXZ */
/* ?close@ofstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ofstream_close, 4)
void __thiscall ofstream_close(ostream *this)
{
    ios *base = ostream_get_ios(this);
    TRACE("(%p)\n", this);
    if (filebuf_close(ofstream_rdbuf(this)) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
    else
        ios_clear(base, IOSTATE_goodbit);
}

/* ?is_open@ofstream@@QBEHXZ */
/* ?is_open@ofstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ofstream_is_open, 4)
int __thiscall ofstream_is_open(const ostream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_is_open(ofstream_rdbuf(this));
}

/* ?open@ofstream@@QAEXPBDHH@Z */
/* ?open@ofstream@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_open, 16)
void __thiscall ofstream_open(ostream *this, const char *name, ios_open_mode mode, int protection)
{
    ios *base = ostream_get_ios(this);
    TRACE("(%p %s %d %d)\n", this, name, mode, protection);
    if (filebuf_open(ofstream_rdbuf(this), name, mode|OPENMODE_out, protection) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?setbuf@ofstream@@QAEPAVstreambuf@@PADH@Z */
/* ?setbuf@ofstream@@QEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_setbuf, 12)
streambuf* __thiscall ofstream_setbuf(ostream *this, char *buffer, int length)
{
    ios *base = ostream_get_ios(this);
    filebuf* fb = ofstream_rdbuf(this);

    TRACE("(%p %p %d)\n", this, buffer, length);

    if (filebuf_is_open(fb)) {
        ios_clear(base, base->state | IOSTATE_failbit);
        return NULL;
    }

    return filebuf_setbuf(fb, buffer, length);
}

/* ?setmode@ofstream@@QAEHH@Z */
/* ?setmode@ofstream@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ofstream_setmode, 8)
int __thiscall ofstream_setmode(ostream *this, int mode)
{
    TRACE("(%p %d)\n", this, mode);
    return filebuf_setmode(ofstream_rdbuf(this), mode);
}

static inline ios* istream_get_ios(const istream *this)
{
    return (ios*)((char*)this + this->vbtable[1]);
}

static inline ios* istream_to_ios(const istream *this)
{
    return (ios*)((char*)this + istream_vbtable[1]);
}

static inline istream* ios_to_istream(const ios *base)
{
    return (istream*)((char*)base - istream_vbtable[1]);
}

/* ??0istream@@IAE@XZ */
/* ??0istream@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(istream_ctor, 8)
istream* __thiscall istream_ctor(istream *this, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %d)\n", this, virt_init);

    if (virt_init) {
        this->vbtable = istream_vbtable;
        base = istream_get_ios(this);
        ios_ctor(base);
    } else
        base = istream_get_ios(this);
    base->vtable = &istream_vtable;
    base->flags |= FLAGS_skipws;
    this->extract_delim = 0;
    this->count = 0;
    return this;
}

/* ??0istream@@QAE@PAVstreambuf@@@Z */
/* ??0istream@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(istream_sb_ctor, 12)
istream* __thiscall istream_sb_ctor(istream *this, streambuf *sb, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, sb, virt_init);
    istream_ctor(this, virt_init);
    ios_init(istream_get_ios(this), sb);
    return this;
}

/* ??0istream@@IAE@ABV0@@Z */
/* ??0istream@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(istream_copy_ctor, 12)
istream* __thiscall istream_copy_ctor(istream *this, const istream *copy, BOOL virt_init)
{
    return istream_sb_ctor(this, istream_get_ios(copy)->sb, virt_init);
}

/* ??1istream@@UAE@XZ */
/* ??1istream@@UEAA@XZ */
/* ??1istream_withassign@@UAE@XZ */
/* ??1istream_withassign@@UEAA@XZ */
/* ??1istrstream@@UAE@XZ */
/* ??1istrstream@@UEAA@XZ */
/* ??1ifstream@@UAE@XZ */
/* ??1ifstream@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(istream_dtor, 4)
void __thiscall istream_dtor(ios *base)
{
    istream *this = ios_to_istream(base);

    TRACE("(%p)\n", this);
}

/* ??4istream@@IAEAAV0@PAVstreambuf@@@Z */
/* ??4istream@@IEAAAEAV0@PEAVstreambuf@@@Z */
/* ??4istream_withassign@@QAEAAVistream@@PAVstreambuf@@@Z */
/* ??4istream_withassign@@QEAAAEAVistream@@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(istream_assign_sb, 8)
istream* __thiscall istream_assign_sb(istream *this, streambuf *sb)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %p)\n", this, sb);

    ios_init(base, sb);
    base->state &= IOSTATE_badbit;
    base->delbuf = 0;
    base->tie = NULL;
    base->flags = FLAGS_skipws;
    base->precision = 6;
    base->fill = ' ';
    base->width = 0;
    this->count = 0;
    return this;
}

/* ??4istream@@IAEAAV0@ABV0@@Z */
/* ??4istream@@IEAAAEAV0@AEBV0@@Z */
/* ??4istream_withassign@@QAEAAV0@ABV0@@Z */
/* ??4istream_withassign@@QEAAAEAV0@AEBV0@@Z */
/* ??4istream_withassign@@QAEAAVistream@@ABV1@@Z */
/* ??4istream_withassign@@QEAAAEAVistream@@AEBV1@@Z */
/* ??4istrstream@@QAEAAV0@ABV0@@Z */
/* ??4istrstream@@QEAAAEAV0@AEBV0@@Z */
/* ??4ifstream@@QAEAAV0@ABV0@@Z */
/* ??4ifstream@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(istream_assign, 8)
istream* __thiscall istream_assign(istream *this, const istream *rhs)
{
    return istream_assign_sb(this, istream_get_ios(rhs)->sb);
}

/* ??_Distream@@QAEXXZ */
/* ??_Distream@@QEAAXXZ */
/* ??_Distream_withassign@@QAEXXZ */
/* ??_Distream_withassign@@QEAAXXZ */
/* ??_Distrstream@@QAEXXZ */
/* ??_Distrstream@@QEAAXXZ */
/* ??_Difstream@@QAEXXZ */
/* ??_Difstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(istream_vbase_dtor, 4)
void __thiscall istream_vbase_dtor(istream *this)
{
    ios *base = istream_to_ios(this);

    TRACE("(%p)\n", this);

    istream_dtor(base);
    ios_dtor(base);
}

/* ??_Eistream@@UAEPAXI@Z */
/* ??_Eistream_withassign@@UAEPAXI@Z */
/* ??_Eistrstream@@UAEPAXI@Z */
/* ??_Eifstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(istream_vector_dtor, 8)
istream* __thiscall istream_vector_dtor(ios *base, unsigned int flags)
{
    istream *this = ios_to_istream(base);

    TRACE("(%p %x)\n", this, flags);

    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            istream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        istream_vbase_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Gistream@@UAEPAXI@Z */
/* ??_Gistream_withassign@@UAEPAXI@Z */
/* ??_Gistrstream@@UAEPAXI@Z */
/* ??_Gifstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(istream_scalar_dtor, 8)
istream* __thiscall istream_scalar_dtor(ios *base, unsigned int flags)
{
    istream *this = ios_to_istream(base);

    TRACE("(%p %x)\n", this, flags);

    istream_vbase_dtor(this);
    if (flags & 1) operator_delete(this);
    return this;
}

/* ?eatwhite@istream@@QAEXXZ */
/* ?eatwhite@istream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(istream_eatwhite, 4)
void __thiscall istream_eatwhite(istream *this)
{
    ios *base = istream_get_ios(this);
    int c;

    TRACE("(%p)\n", this);

    ios_lockbuf(base);
    for (c = streambuf_sgetc(base->sb); isspace(c); c = streambuf_snextc(base->sb));
    ios_unlockbuf(base);
    if (c == EOF)
        ios_clear(base, base->state | IOSTATE_eofbit);
}

/* ?gcount@istream@@QBEHXZ */
/* ?gcount@istream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(istream_gcount, 4)
int __thiscall istream_gcount(const istream *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?ipfx@istream@@QAEHH@Z */
/* ?ipfx@istream@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(istream_ipfx, 8)
int __thiscall istream_ipfx(istream *this, int need)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %d)\n", this, need);

    if (need)
        this->count = 0;
    if (!ios_good(base)) {
        ios_clear(base, base->state | IOSTATE_failbit);
        return 0;
    }
    ios_lock(base);
    ios_lockbuf(base);
    if (base->tie && (!need || streambuf_in_avail(base->sb) < need))
        ostream_flush(base->tie);
    if ((base->flags & FLAGS_skipws) && !need) {
        istream_eatwhite(this);
        if (base->state & IOSTATE_eofbit) {
            base->state |= IOSTATE_failbit;
            ios_unlockbuf(base);
            ios_unlock(base);
            return 0;
        }
    }
    return 1;
}

/* ?isfx@istream@@QAEXXZ */
/* ?isfx@istream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(istream_isfx, 4)
void __thiscall istream_isfx(istream *this)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p)\n", this);

    ios_unlockbuf(base);
    ios_unlock(base);
}

/* ?get@istream@@IAEAAV1@PADHH@Z */
/* ?get@istream@@IEAAAEAV1@PEADHH@Z */
DEFINE_THISCALL_WRAPPER(istream_get_str_delim, 16)
istream* __thiscall istream_get_str_delim(istream *this, char *str, int count, int delim)
{
    ios *base = istream_get_ios(this);
    int ch, i = 0;

    TRACE("(%p %p %d %d)\n", this, str, count, delim);

    if (istream_ipfx(this, 1)) {
        while (i < count - 1) {
            if ((ch = streambuf_sgetc(base->sb)) == EOF) {
                base->state |= IOSTATE_eofbit;
                if (!i) /* tried to read, but not a single character was obtained */
                    base->state |= IOSTATE_failbit;
                break;
            }
            if (ch == delim) {
                if (this->extract_delim) { /* discard the delimiter */
                    streambuf_stossc(base->sb);
                    this->count++;
                }
                break;
            }
            if (str)
                str[i] = ch;
            streambuf_stossc(base->sb);
            i++;
        }
        this->count += i;
        istream_isfx(this);
    }
    if (str && count) /* append a null terminator, unless a string of 0 characters was requested */
        str[i] = 0;
    this->extract_delim = 0;
    return this;
}

/* ?get@istream@@QAEAAV1@PACHD@Z */
/* ?get@istream@@QEAAAEAV1@PEACHD@Z */
/* ?get@istream@@QAEAAV1@PADHD@Z */
/* ?get@istream@@QEAAAEAV1@PEADHD@Z */
/* ?get@istream@@QAEAAV1@PAEHD@Z */
/* ?get@istream@@QEAAAEAV1@PEAEHD@Z */
DEFINE_THISCALL_WRAPPER(istream_get_str, 16)
istream* __thiscall istream_get_str(istream *this, char *str, int count, char delim)
{
    return istream_get_str_delim(this, str, count, (unsigned char) delim);
}

static int istream_internal_get_char(istream *this, char *ch)
{
    ios *base = istream_get_ios(this);
    int ret = EOF;

    TRACE("(%p %p)\n", this, ch);

    if (istream_ipfx(this, 1)) {
        if ((ret = streambuf_sbumpc(base->sb)) != EOF) {
            this->count = 1;
        } else {
            base->state |= IOSTATE_eofbit;
            if (ch)
                base->state |= IOSTATE_failbit;
        }
        if (ch)
            *ch = ret;
        istream_isfx(this);
    }
    return ret;
}

/* ?get@istream@@QAEAAV1@AAC@Z */
/* ?get@istream@@QEAAAEAV1@AEAC@Z */
/* ?get@istream@@QAEAAV1@AAD@Z */
/* ?get@istream@@QEAAAEAV1@AEAD@Z */
/* ?get@istream@@QAEAAV1@AAE@Z */
/* ?get@istream@@QEAAAEAV1@AEAE@Z */
DEFINE_THISCALL_WRAPPER(istream_get_char, 8)
istream* __thiscall istream_get_char(istream *this, char *ch)
{
    istream_internal_get_char(this, ch);
    return this;
}

/* ?get@istream@@QAEHXZ */
/* ?get@istream@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(istream_get, 4)
int __thiscall istream_get(istream *this)
{
    return istream_internal_get_char(this, NULL);
}

/* ?get@istream@@QAEAAV1@AAVstreambuf@@D@Z */
/* ?get@istream@@QEAAAEAV1@AEAVstreambuf@@D@Z */
DEFINE_THISCALL_WRAPPER(istream_get_sb, 12)
istream* __thiscall istream_get_sb(istream *this, streambuf *sb, char delim)
{
    ios *base = istream_get_ios(this);
    int ch;

    TRACE("(%p %p %c)\n", this, sb, delim);

    if (istream_ipfx(this, 1)) {
        for (ch = streambuf_sgetc(base->sb); ch != delim; ch = streambuf_snextc(base->sb)) {
            if (ch == EOF) {
                base->state |= IOSTATE_eofbit;
                break;
            }
            if (streambuf_sputc(sb, ch) == EOF)
                base->state |= IOSTATE_failbit;
            this->count++;
        }
        istream_isfx(this);
    }
    return this;
}

/* ?getline@istream@@QAEAAV1@PACHD@Z */
/* ?getline@istream@@QEAAAEAV1@PEACHD@Z */
/* ?getline@istream@@QAEAAV1@PADHD@Z */
/* ?getline@istream@@QEAAAEAV1@PEADHD@Z */
/* ?getline@istream@@QAEAAV1@PAEHD@Z */
/* ?getline@istream@@QEAAAEAV1@PEAEHD@Z */
DEFINE_THISCALL_WRAPPER(istream_getline, 16)
istream* __thiscall istream_getline(istream *this, char *str, int count, char delim)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %p %d %c)\n", this, str, count, delim);

    ios_lock(base);
    this->extract_delim++;
    istream_get_str_delim(this, str, count, (unsigned char) delim);
    ios_unlock(base);
    return this;
}

/* ?ignore@istream@@QAEAAV1@HH@Z */
/* ?ignore@istream@@QEAAAEAV1@HH@Z */
DEFINE_THISCALL_WRAPPER(istream_ignore, 12)
istream* __thiscall istream_ignore(istream *this, int count, int delim)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %d %d)\n", this, count, delim);

    ios_lock(base);
    this->extract_delim++;
    istream_get_str_delim(this, NULL, count + 1, delim);
    ios_unlock(base);
    return this;
}

/* ?peek@istream@@QAEHXZ */
/* ?peek@istream@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(istream_peek, 4)
int __thiscall istream_peek(istream *this)
{
    ios *base = istream_get_ios(this);
    int ret = EOF;

    TRACE("(%p)\n", this);

    if (istream_ipfx(this, 1)) {
        ret = streambuf_sgetc(base->sb);
        istream_isfx(this);
    }
    return ret;
}

/* ?putback@istream@@QAEAAV1@D@Z */
/* ?putback@istream@@QEAAAEAV1@D@Z */
DEFINE_THISCALL_WRAPPER(istream_putback, 8)
istream* __thiscall istream_putback(istream *this, char ch)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if (ios_good(base)) {
        ios_lockbuf(base);
        if (streambuf_sputbackc(base->sb, ch) == EOF)
            ios_clear(base, base->state | IOSTATE_failbit);
        ios_unlockbuf(base);
    }
    return this;
}

/* ?read@istream@@QAEAAV1@PACH@Z */
/* ?read@istream@@QEAAAEAV1@PEACH@Z */
/* ?read@istream@@QAEAAV1@PADH@Z */
/* ?read@istream@@QEAAAEAV1@PEADH@Z */
/* ?read@istream@@QAEAAV1@PAEH@Z */
/* ?read@istream@@QEAAAEAV1@PEAEH@Z */
DEFINE_THISCALL_WRAPPER(istream_read, 12)
istream* __thiscall istream_read(istream *this, char *str, int count)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %p %d)\n", this, str, count);

    if (istream_ipfx(this, 1)) {
        if ((this->count = streambuf_sgetn(base->sb, str, count)) != count)
            base->state = IOSTATE_eofbit | IOSTATE_failbit;
        istream_isfx(this);
    }
    return this;
}

/* ?seekg@istream@@QAEAAV1@J@Z */
/* ?seekg@istream@@QEAAAEAV1@J@Z */
DEFINE_THISCALL_WRAPPER(istream_seekg, 8)
istream* __thiscall istream_seekg(istream *this, streampos pos)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %ld)\n", this, pos);

    ios_lockbuf(base);
    if (streambuf_seekpos(base->sb, pos, OPENMODE_in) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return this;
}

/* ?seekg@istream@@QAEAAV1@JW4seek_dir@ios@@@Z */
/* ?seekg@istream@@QEAAAEAV1@JW4seek_dir@ios@@@Z */
DEFINE_THISCALL_WRAPPER(istream_seekg_offset, 12)
istream* __thiscall istream_seekg_offset(istream *this, streamoff off, ios_seek_dir dir)
{
    ios *base = istream_get_ios(this);

    TRACE("(%p %ld %d)\n", this, off, dir);

    ios_lockbuf(base);
    if (call_streambuf_seekoff(base->sb, off, dir, OPENMODE_in) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return this;
}

/* ?sync@istream@@QAEHXZ */
/* ?sync@istream@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(istream_sync, 4)
int __thiscall istream_sync(istream *this)
{
    ios *base = istream_get_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    ios_lockbuf(base);
    if ((ret = call_streambuf_sync(base->sb)) == EOF)
        ios_clear(base, base->state | IOSTATE_badbit | IOSTATE_failbit);
    ios_unlockbuf(base);
    return ret;
}

/* ?tellg@istream@@QAEJXZ */
/* ?tellg@istream@@QEAAJXZ */
DEFINE_THISCALL_WRAPPER(istream_tellg, 4)
streampos __thiscall istream_tellg(istream *this)
{
    ios *base = istream_get_ios(this);
    streampos pos;

    TRACE("(%p)\n", this);

    ios_lockbuf(base);
    if ((pos = call_streambuf_seekoff(base->sb, 0, SEEKDIR_cur, OPENMODE_in)) == EOF)
        ios_clear(base, base->state | IOSTATE_failbit);
    ios_unlockbuf(base);
    return pos;
}

static int getint_is_valid_digit(char ch, int base)
{
    if (base == 8) return (ch >= '0' && ch <= '7');
    if (base == 16) return isxdigit(ch);
    return isdigit(ch);
}

/* ?getint@istream@@AAEHPAD@Z */
/* ?getint@istream@@AEAAHPEAD@Z */
DEFINE_THISCALL_WRAPPER(istream_getint, 8)
int __thiscall istream_getint(istream *this, char *str)
{
    ios *base = istream_get_ios(this);
    int ch, num_base = 0, i = 0;
    BOOL scan_sign = TRUE, scan_prefix = TRUE, scan_x = FALSE, valid_integer = FALSE;

    TRACE("(%p %p)\n", this, str);

    if (istream_ipfx(this, 0)) {
        num_base = (base->flags & FLAGS_dec) ? 10 :
            (base->flags & FLAGS_hex) ? 16 :
            (base->flags & FLAGS_oct) ? 8 : 0; /* 0 = autodetect */
        /* scan valid characters, up to 15 (hard limit on Windows) */
        for (ch = streambuf_sgetc(base->sb); i < 15; ch = streambuf_snextc(base->sb)) {
            if ((ch == '+' || ch == '-') && scan_sign) {
                /* no additional sign allowed */
                scan_sign = FALSE;
            } else if ((ch == 'x' || ch == 'X') && scan_x) {
                /* only hex digits can (and must) follow */
                scan_x = valid_integer = FALSE;
                num_base = 16;
            } else if (ch == '0' && scan_prefix) {
                /* might be the octal prefix, the beginning of the hex prefix or a decimal zero */
                scan_sign = scan_prefix = FALSE;
                scan_x = !num_base || num_base == 16;
                valid_integer = TRUE;
                if (!num_base)
                    num_base = 8;
            } else if (getint_is_valid_digit(ch, num_base)) {
                /* only digits in the corresponding base can follow */
                scan_sign = scan_prefix = scan_x = FALSE;
                valid_integer = TRUE;
            } else {
                /* unexpected character, stop scanning */
                if (!valid_integer) {
                    /* the result is not a valid integer */
                    base->state |= IOSTATE_failbit;
                    /* put any extracted character back into the stream */
                    while (i > 0)
                        if (streambuf_sputbackc(base->sb, str[--i]) == EOF)
                            base->state |= IOSTATE_badbit; /* characters have been lost for good */
                } else if (ch == EOF) {
                    base->state |= IOSTATE_eofbit;
                    if (scan_x && !(base->flags & ios_basefield)) {
                        /* when autodetecting, a single zero followed by EOF is regarded as decimal */
                        num_base = 0;
                    }
                }
                break;
            }
            str[i++] = ch;
        }
        /* append a null terminator */
        str[i] = 0;
        istream_isfx(this);
    }
    return num_base;
}

/* ?getdouble@istream@@AAEHPADH@Z */
/* ?getdouble@istream@@AEAAHPEADH@Z */
DEFINE_THISCALL_WRAPPER(istream_getdouble, 12)
int __thiscall istream_getdouble(istream *this, char *str, int count)
{
    ios *base = istream_get_ios(this);
    int ch, i = 0;
    BOOL scan_sign = TRUE, scan_dot = TRUE, scan_exp = TRUE,
        valid_mantissa = FALSE, valid_exponent = FALSE;

    TRACE("(%p %p %d)\n", this, str, count);

    if (istream_ipfx(this, 0)) {
        if (!count) {
            /* can't output anything */
            base->state |= IOSTATE_failbit;
            i = -1;
        } else {
            /* valid mantissas: +d. +.d +d.d (where d are sequences of digits and the sign is optional) */
            /* valid exponents: e+d E+d (where d are sequences of digits and the sign is optional) */
            for (ch = streambuf_sgetc(base->sb); i < count; ch = streambuf_snextc(base->sb)) {
                if ((ch == '+' || ch == '-') && scan_sign) {
                    /* no additional sign allowed */
                    scan_sign = FALSE;
                } else if (ch == '.' && scan_dot) {
                    /* no sign or additional dot allowed */
                    scan_sign = scan_dot = FALSE;
                } else if ((ch == 'e' || ch == 'E') && scan_exp) {
                    /* sign is allowed again but not dots or exponents */
                    scan_sign = TRUE;
                    scan_dot = scan_exp = FALSE;
                } else if (isdigit(ch)) {
                    if (scan_exp)
                        valid_mantissa = TRUE;
                    else
                        valid_exponent = TRUE;
                     /* no sign allowed after a digit */
                    scan_sign = FALSE;
                } else {
                    /* unexpected character, stop scanning */
                    /* check whether the result is a valid double */
                    if (!scan_exp && !valid_exponent) {
                        /* put the last character back into the stream, usually the 'e' or 'E' */
                        if (streambuf_sputbackc(base->sb, str[i--]) == EOF)
                            base->state |= IOSTATE_badbit; /* characters have been lost for good */
                    } else if (ch == EOF)
                        base->state |= IOSTATE_eofbit;
                    if (!valid_mantissa)
                        base->state |= IOSTATE_failbit;
                    break;
                }
                str[i++] = ch;
            }
            /* check if character limit has been reached */
            if (i == count) {
                base->state |= IOSTATE_failbit;
                i--;
            }
            /* append a null terminator */
            str[i] = 0;
        }
        istream_isfx(this);
    }
    return i;
}

/* ??5istream@@QAEAAV0@AAC@Z */
/* ??5istream@@QEAAAEAV0@AEAC@Z */
/* ??5istream@@QAEAAV0@AAD@Z */
/* ??5istream@@QEAAAEAV0@AEAD@Z */
/* ??5istream@@QAEAAV0@AAE@Z */
/* ??5istream@@QEAAAEAV0@AEAE@Z */
DEFINE_THISCALL_WRAPPER(istream_read_char, 8)
istream* __thiscall istream_read_char(istream *this, char *ch)
{
    ios *base = istream_get_ios(this);
    int ret;

    TRACE("(%p %p)\n", this, ch);

    if (istream_ipfx(this, 0)) {
        if ((ret = streambuf_sbumpc(base->sb)) == EOF)
            base->state |= IOSTATE_eofbit | IOSTATE_failbit;
        else
            *ch = ret;
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@PAC@Z */
/* ??5istream@@QEAAAEAV0@PEAC@Z */
/* ??5istream@@QAEAAV0@PAD@Z */
/* ??5istream@@QEAAAEAV0@PEAD@Z */
/* ??5istream@@QAEAAV0@PAE@Z */
/* ??5istream@@QEAAAEAV0@PEAE@Z */
DEFINE_THISCALL_WRAPPER(istream_read_str, 8)
istream* __thiscall istream_read_str(istream *this, char *str)
{
    ios *base = istream_get_ios(this);
    int ch, count = 0;

    TRACE("(%p %p)\n", this, str);

    if (istream_ipfx(this, 0)) {
        if (str) {
            for (ch = streambuf_sgetc(base->sb);
                count < (unsigned int) base->width - 1 && !isspace(ch);
                ch = streambuf_snextc(base->sb)) {
                if (ch == EOF) {
                    base->state |= IOSTATE_eofbit;
                    break;
                }
                str[count++] = ch;
            }
        }
        if (!count) /* nothing to output */
            base->state |= IOSTATE_failbit;
        else /* append a null terminator */
            str[count] = 0;
        base->width = 0;
        istream_isfx(this);
    }
    return this;
}

static LONG istream_internal_read_integer(istream *this, LONG min_value, LONG max_value, BOOL set_flag)
{
    ios *base = istream_get_ios(this);
    char buffer[16];
    int num_base;
    LONG ret;

    TRACE("(%p %ld %ld %d)\n", this, min_value, max_value, set_flag);

    num_base = istream_getint(this, buffer);
    errno = 0;
    ret = strtol(buffer, NULL, num_base);
    /* check for overflow and whether the value fits in the output var */
    if (set_flag && errno == ERANGE) {
        base->state |= IOSTATE_failbit;
    } else if (ret > max_value) {
        base->state |= IOSTATE_failbit;
        ret = max_value;
    } else if (ret < min_value) {
        base->state |= IOSTATE_failbit;
        ret = min_value;
    }
    return ret;
}

static ULONG istream_internal_read_unsigned_integer(istream *this, LONG min_value, ULONG max_value)
{
    ios *base = istream_get_ios(this);
    char buffer[16];
    int num_base;
    ULONG ret;

    TRACE("(%p %ld %lu)\n", this, min_value, max_value);

    num_base = istream_getint(this, buffer);
    errno = 0;
    ret = strtoul(buffer, NULL, num_base);
    /* check for overflow and whether the value fits in the output var */
    if ((ret == ULONG_MAX && errno == ERANGE) ||
        (ret > max_value && ret < (ULONG) min_value)) {
        base->state |= IOSTATE_failbit;
        ret = max_value;
    }
    return ret;
}

/* ??5istream@@QAEAAV0@AAF@Z */
/* ??5istream@@QEAAAEAV0@AEAF@Z */
DEFINE_THISCALL_WRAPPER(istream_read_short, 8)
istream* __thiscall istream_read_short(istream *this, short *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_integer(this, SHRT_MIN, SHRT_MAX, FALSE);
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAG@Z */
/* ??5istream@@QEAAAEAV0@AEAG@Z */
DEFINE_THISCALL_WRAPPER(istream_read_unsigned_short, 8)
istream* __thiscall istream_read_unsigned_short(istream *this, unsigned short *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_unsigned_integer(this, SHRT_MIN, USHRT_MAX);
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAH@Z */
/* ??5istream@@QEAAAEAV0@AEAH@Z */
DEFINE_THISCALL_WRAPPER(istream_read_int, 8)
istream* __thiscall istream_read_int(istream *this, int *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_integer(this, INT_MIN, INT_MAX, FALSE);
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAI@Z */
/* ??5istream@@QEAAAEAV0@AEAI@Z */
DEFINE_THISCALL_WRAPPER(istream_read_unsigned_int, 8)
istream* __thiscall istream_read_unsigned_int(istream *this, unsigned int *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_unsigned_integer(this, INT_MIN, UINT_MAX);
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAJ@Z */
/* ??5istream@@QEAAAEAV0@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(istream_read_long, 8)
istream* __thiscall istream_read_long(istream *this, LONG *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_integer(this, LONG_MIN, LONG_MAX, TRUE);
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAK@Z */
/* ??5istream@@QEAAAEAV0@AEAK@Z */
DEFINE_THISCALL_WRAPPER(istream_read_unsigned_long, 8)
istream* __thiscall istream_read_unsigned_long(istream *this, ULONG *p)
{
    if (istream_ipfx(this, 0)) {
        *p = istream_internal_read_unsigned_integer(this, LONG_MIN, ULONG_MAX);
        istream_isfx(this);
    }
    return this;
}

static BOOL istream_internal_read_float(istream *this, int max_chars, double *out)
{
    char buffer[32];
    BOOL read = FALSE;

    TRACE("(%p %d %p)\n", this, max_chars, out);

    if (istream_ipfx(this, 0)) {
        /* character count is limited on Windows */
        if (istream_getdouble(this, buffer, max_chars) > 0) {
            *out = strtod(buffer, NULL);
            read = TRUE;
        }
        istream_isfx(this);
    }
    return read;
}

/* ??5istream@@QAEAAV0@AAM@Z */
/* ??5istream@@QEAAAEAV0@AEAM@Z */
DEFINE_THISCALL_WRAPPER(istream_read_float, 8)
istream* __thiscall istream_read_float(istream *this, float *f)
{
    double tmp;
    if (istream_internal_read_float(this, 20, &tmp)) {
        /* check whether the value fits in the output var */
        if (tmp > FLT_MAX)
            tmp = FLT_MAX;
        else if (tmp < -FLT_MAX)
            tmp = -FLT_MAX;
        else if (tmp > 0 && tmp < FLT_MIN)
            tmp = FLT_MIN;
        else if (tmp < 0 && tmp > -FLT_MIN)
            tmp = -FLT_MIN;
        *f = tmp;
    }
    return this;
}

/* ??5istream@@QAEAAV0@AAN@Z */
/* ??5istream@@QEAAAEAV0@AEAN@Z */
DEFINE_THISCALL_WRAPPER(istream_read_double, 8)
istream* __thiscall istream_read_double(istream *this, double *d)
{
    istream_internal_read_float(this, 28, d);
    return this;
}

/* ??5istream@@QAEAAV0@AAO@Z */
/* ??5istream@@QEAAAEAV0@AEAO@Z */
DEFINE_THISCALL_WRAPPER(istream_read_long_double, 8)
istream* __thiscall istream_read_long_double(istream *this, double *ld)
{
    istream_internal_read_float(this, 32, ld);
    return this;
}

/* ??5istream@@QAEAAV0@PAVstreambuf@@@Z */
/* ??5istream@@QEAAAEAV0@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(istream_read_streambuf, 8)
istream* __thiscall istream_read_streambuf(istream *this, streambuf *sb)
{
    ios *base = istream_get_ios(this);
    int ch;

    TRACE("(%p %p)\n", this, sb);

    if (istream_ipfx(this, 0)) {
        while ((ch = streambuf_sbumpc(base->sb)) != EOF)
            if (streambuf_sputc(sb, ch) == EOF)
                base->state |= IOSTATE_failbit;
        istream_isfx(this);
    }
    return this;
}

/* ??5istream@@QAEAAV0@P6AAAV0@AAV0@@Z@Z */
/* ??5istream@@QEAAAEAV0@P6AAEAV0@AEAV0@@Z@Z */
DEFINE_THISCALL_WRAPPER(istream_read_manip, 8)
istream* __thiscall istream_read_manip(istream *this, istream* (__cdecl *func)(istream*))
{
    TRACE("(%p %p)\n", this, func);
    return func(this);
}

/* ??5istream@@QAEAAV0@P6AAAVios@@AAV1@@Z@Z */
/* ??5istream@@QEAAAEAV0@P6AAEAVios@@AEAV1@@Z@Z */
DEFINE_THISCALL_WRAPPER(istream_read_ios_manip, 8)
istream* __thiscall istream_read_ios_manip(istream *this, ios* (__cdecl *func)(ios*))
{
    TRACE("(%p %p)\n", this, func);
    func(istream_get_ios(this));
    return this;
}

/* ?ws@@YAAAVistream@@AAV1@@Z */
/* ?ws@@YAAEAVistream@@AEAV1@@Z */
istream* __cdecl istream_ws(istream *this)
{
    TRACE("(%p)\n", this);
    istream_eatwhite(this);
    return this;
}

/* ??0istream_withassign@@QAE@ABV0@@Z */
/* ??0istream_withassign@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(istream_withassign_copy_ctor, 12)
istream* __thiscall istream_withassign_copy_ctor(istream *this, const istream *copy, BOOL virt_init)
{
    ios *base, *base_copy;

    TRACE("(%p %p %d)\n", this, copy, virt_init);

    base_copy = istream_get_ios(copy);
    if (virt_init) {
        this->vbtable = istream_vbtable;
        base = istream_get_ios(this);
        ios_copy_ctor(base, base_copy);
    } else
        base = istream_get_ios(this);
    ios_init(base, base_copy->sb);
    base->vtable = &istream_withassign_vtable;
    base->flags |= FLAGS_skipws;
    this->extract_delim = 0;
    this->count = 0;
    return this;
}

/* ??0istream_withassign@@QAE@PAVstreambuf@@@Z */
/* ??0istream_withassign@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(istream_withassign_sb_ctor, 12)
istream* __thiscall istream_withassign_sb_ctor(istream *this, streambuf *sb, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %p %d)\n", this, sb, virt_init);

    istream_sb_ctor(this, sb, virt_init);
    base = istream_get_ios(this);
    base->vtable = &istream_withassign_vtable;
    return this;
}

/* ??0istream_withassign@@QAE@XZ */
/* ??0istream_withassign@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(istream_withassign_ctor, 8)
istream* __thiscall istream_withassign_ctor(istream *this, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %d)\n", this, virt_init);

    istream_ctor(this, virt_init);
    base = istream_get_ios(this);
    base->vtable = &istream_withassign_vtable;
    return this;
}

/* ??0istrstream@@QAE@ABV0@@Z */
/* ??0istrstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(istrstream_copy_ctor, 12)
istream* __thiscall istrstream_copy_ctor(istream *this, const istream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    istream_withassign_copy_ctor(this, copy, virt_init);
    istream_get_ios(this)->vtable = &istrstream_vtable;
    return this;
}

/* ??0istrstream@@QAE@PADH@Z */
/* ??0istrstream@@QEAA@PEADH@Z */
DEFINE_THISCALL_WRAPPER(istrstream_buffer_ctor, 16)
istream* __thiscall istrstream_buffer_ctor(istream *this, char *buffer, int length, BOOL virt_init)
{
    ios *base;
    strstreambuf *ssb = operator_new(sizeof(strstreambuf));

    TRACE("(%p %p %d %d)\n", this, buffer, length, virt_init);

    if (!ssb) {
        FIXME("out of memory\n");
        return NULL;
    }

    strstreambuf_buffer_ctor(ssb, buffer, length, NULL);
    istream_sb_ctor(this, &ssb->base, virt_init);

    base = istream_get_ios(this);
    base->vtable = &istrstream_vtable;
    base->delbuf = 1;
    return this;
}

/* ??0istrstream@@QAE@PAD@Z */
/* ??0istrstream@@QEAA@PEAD@Z */
DEFINE_THISCALL_WRAPPER(istrstream_str_ctor, 12)
istream* __thiscall istrstream_str_ctor(istream *this, char *str, BOOL virt_init)
{
    return istrstream_buffer_ctor(this, str, 0, virt_init);
}

/* ?rdbuf@istrstream@@QBEPAVstrstreambuf@@XZ */
/* ?rdbuf@istrstream@@QEBAPEAVstrstreambuf@@XZ */
DEFINE_THISCALL_WRAPPER(istrstream_rdbuf, 4)
strstreambuf* __thiscall istrstream_rdbuf(const istream *this)
{
    return (strstreambuf*) istream_get_ios(this)->sb;
}

/* ?str@istrstream@@QAEPADXZ */
/* ?str@istrstream@@QEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(istrstream_str, 4)
char* __thiscall istrstream_str(istream *this)
{
    return strstreambuf_str(istrstream_rdbuf(this));
}

/* ??0ifstream@@QAE@ABV0@@Z */
/* ??0ifstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(ifstream_copy_ctor, 12)
istream* __thiscall ifstream_copy_ctor(istream *this, const istream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    istream_withassign_copy_ctor(this, copy, virt_init);
    istream_get_ios(this)->vtable = &ifstream_vtable;
    return this;
}

/* ??0ifstream@@QAE@HPADH@Z */
/* ??0ifstream@@QEAA@HPEADH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_buffer_ctor, 20)
istream* __thiscall ifstream_buffer_ctor(istream *this, filedesc fd, char *buffer, int length, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %p %d %d)\n", this, fd, buffer, length, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_reserve_ctor(fb, fd, buffer, length);
    istream_sb_ctor(this, &fb->base, virt_init);

    base = istream_get_ios(this);
    base->vtable = &ifstream_vtable;
    base->delbuf = 1;

    return this;
}

/* ??0ifstream@@QAE@H@Z */
/* ??0ifstream@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(ifstream_fd_ctor, 12)
istream* __thiscall ifstream_fd_ctor(istream *this, filedesc fd, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %d)\n", this, fd, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_ctor(fb, fd);
    istream_sb_ctor(this, &fb->base, virt_init);

    base = istream_get_ios(this);
    base->vtable = &ifstream_vtable;
    base->delbuf = 1;

    return this;
}

/* ??0ifstream@@QAE@PBDHH@Z */
/* ??0ifstream@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_open_ctor, 20)
istream* __thiscall ifstream_open_ctor(istream *this, const char *name, ios_open_mode mode, int protection, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %s %d %d %d)\n", this, name, mode, protection, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_ctor(fb);
    istream_sb_ctor(this, &fb->base, virt_init);

    base = istream_get_ios(this);
    base->vtable = &ifstream_vtable;
    base->delbuf = 1;

    if (filebuf_open(fb, name, mode|OPENMODE_in, protection) == NULL)
        base->state |= IOSTATE_failbit;
    return this;
}

/* ??0ifstream@@QAE@XZ */
/* ??0ifstream@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ifstream_ctor, 8)
istream* __thiscall ifstream_ctor(istream *this, BOOL virt_init)
{
    return ifstream_fd_ctor(this, -1, virt_init);
}

/* ?rdbuf@ifstream@@QBEPAVfilebuf@@XZ */
/* ?rdbuf@ifstream@@QEBAPEAVfilebuf@@XZ */
DEFINE_THISCALL_WRAPPER(ifstream_rdbuf, 4)
filebuf* __thiscall ifstream_rdbuf(const istream *this)
{
    TRACE("(%p)\n", this);
    return (filebuf*) istream_get_ios(this)->sb;
}

/* ?fd@ifstream@@QBEHXZ */
/* ?fd@ifstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ifstream_fd, 4)
filedesc __thiscall ifstream_fd(istream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_fd(ifstream_rdbuf(this));
}

/* ?attach@ifstream@@QAEXH@Z */
/* ?attach@ifstream@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_attach, 8)
void __thiscall ifstream_attach(istream *this, filedesc fd)
{
    ios *base = istream_get_ios(this);
    TRACE("(%p %d)\n", this, fd);
    if (filebuf_attach(ifstream_rdbuf(this), fd) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?close@ifstream@@QAEXXZ */
/* ?close@ifstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ifstream_close, 4)
void __thiscall ifstream_close(istream *this)
{
    ios *base = istream_get_ios(this);
    TRACE("(%p)\n", this);
    if (filebuf_close(ifstream_rdbuf(this)) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
    else
        ios_clear(base, IOSTATE_goodbit);
}

/* ?is_open@ifstream@@QBEHXZ */
/* ?is_open@ifstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ifstream_is_open, 4)
int __thiscall ifstream_is_open(const istream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_is_open(ifstream_rdbuf(this));
}

/* ?open@ifstream@@QAEXPBDHH@Z */
/* ?open@ifstream@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_open, 16)
void __thiscall ifstream_open(istream *this, const char *name, ios_open_mode mode, int protection)
{
    ios *base = istream_get_ios(this);
    TRACE("(%p %s %d %d)\n", this, name, mode, protection);
    if (filebuf_open(ifstream_rdbuf(this), name, mode|OPENMODE_in, protection) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?setbuf@ifstream@@QAEPAVstreambuf@@PADH@Z */
/* ?setbuf@ifstream@@QEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_setbuf, 12)
streambuf* __thiscall ifstream_setbuf(istream *this, char *buffer, int length)
{
    ios *base = istream_get_ios(this);
    filebuf* fb = ifstream_rdbuf(this);

    TRACE("(%p %p %d)\n", this, buffer, length);

    if (filebuf_is_open(fb)) {
        ios_clear(base, base->state | IOSTATE_failbit);
        return NULL;
    }

    return filebuf_setbuf(fb, buffer, length);
}

/* ?setmode@ifstream@@QAEHH@Z */
/* ?setmode@ifstream@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ifstream_setmode, 8)
int __thiscall ifstream_setmode(istream *this, int mode)
{
    TRACE("(%p %d)\n", this, mode);
    return filebuf_setmode(ifstream_rdbuf(this), mode);
}

static inline ios* iostream_to_ios(const iostream *this)
{
    return (ios*)((char*)this + iostream_vbtable_istream[1]);
}

static inline iostream* ios_to_iostream(const ios *base)
{
    return (iostream*)((char*)base - iostream_vbtable_istream[1]);
}

/* ??0iostream@@IAE@XZ */
/* ??0iostream@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(iostream_ctor, 8)
iostream* __thiscall iostream_ctor(iostream *this, BOOL virt_init)
{
    ios *base;

    TRACE("(%p %d)\n", this, virt_init);

    if (virt_init) {
        this->base1.vbtable = iostream_vbtable_istream;
        this->base2.vbtable = iostream_vbtable_ostream;
        base = istream_get_ios(&this->base1);
        ios_ctor(base);
    } else
        base = istream_get_ios(&this->base1);
    istream_ctor(&this->base1, FALSE);
    ostream_ctor(&this->base2, FALSE);
    base->vtable = &iostream_vtable;
    return this;
}

/* ??0iostream@@QAE@PAVstreambuf@@@Z */
/* ??0iostream@@QEAA@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(iostream_sb_ctor, 12)
iostream* __thiscall iostream_sb_ctor(iostream *this, streambuf *sb, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, sb, virt_init);
    iostream_ctor(this, virt_init);
    ios_init(istream_get_ios(&this->base1), sb);
    return this;
}

/* ??0iostream@@IAE@ABV0@@Z */
/* ??0iostream@@IEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(iostream_copy_ctor, 12)
iostream* __thiscall iostream_copy_ctor(iostream *this, const iostream *copy, BOOL virt_init)
{
    return iostream_sb_ctor(this, istream_get_ios(&copy->base1)->sb, virt_init);
}

/* ??1iostream@@UAE@XZ */
/* ??1iostream@@UEAA@XZ */
/* ??1stdiostream@@UAE@XZ */
/* ??1stdiostream@@UEAA@XZ */
/* ??1strstream@@UAE@XZ */
/* ??1strstream@@UEAA@XZ */
/* ??1fstream@@UAE@XZ */
/* ??1fstream@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(iostream_dtor, 4)
void __thiscall iostream_dtor(ios *base)
{
    iostream *this = ios_to_iostream(base);

    TRACE("(%p)\n", this);

    ostream_dtor(ostream_to_ios(&this->base2));
    istream_dtor(istream_to_ios(&this->base1));
}

/* ??4iostream@@IAEAAV0@PAVstreambuf@@@Z */
/* ??4iostream@@IEAAAEAV0@PEAVstreambuf@@@Z */
DEFINE_THISCALL_WRAPPER(iostream_assign_sb, 8)
iostream* __thiscall iostream_assign_sb(iostream *this, streambuf *sb)
{
    TRACE("(%p %p)\n", this, sb);
    this->base1.count = 0;
    ostream_assign_sb(&this->base2, sb);
    return this;
}

/* ??4iostream@@IAEAAV0@AAV0@@Z */
/* ??4iostream@@IEAAAEAV0@AEAV0@@Z */
/* ??4stdiostream@@QAEAAV0@AAV0@@Z */
/* ??4stdiostream@@QEAAAEAV0@AEAV0@@Z */
/* ??4strstream@@QAEAAV0@ABV0@@Z */
/* ??4strstream@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(iostream_assign, 8)
iostream* __thiscall iostream_assign(iostream *this, const iostream *rhs)
{
    return iostream_assign_sb(this, istream_get_ios(&rhs->base1)->sb);
}

/* ??_Diostream@@QAEXXZ */
/* ??_Diostream@@QEAAXXZ */
/* ??_Dstdiostream@@QAEXXZ */
/* ??_Dstdiostream@@QEAAXXZ */
/* ??_Dstrstream@@QAEXXZ */
/* ??_Dstrstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(iostream_vbase_dtor, 4)
void __thiscall iostream_vbase_dtor(iostream *this)
{
    ios *base = iostream_to_ios(this);

    TRACE("(%p)\n", this);

    iostream_dtor(base);
    ios_dtor(base);
}

/* ??_Eiostream@@UAEPAXI@Z */
/* ??_Estdiostream@@UAEPAXI@Z */
/* ??_Estrstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(iostream_vector_dtor, 8)
iostream* __thiscall iostream_vector_dtor(ios *base, unsigned int flags)
{
    iostream *this = ios_to_iostream(base);

    TRACE("(%p %x)\n", this, flags);

    if (flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for (i = *ptr-1; i >= 0; i--)
            iostream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        iostream_vbase_dtor(this);
        if (flags & 1)
            operator_delete(this);
    }
    return this;
}

/* ??_Giostream@@UAEPAXI@Z */
/* ??_Gstdiostream@@UAEPAXI@Z */
/* ??_Gstrstream@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(iostream_scalar_dtor, 8)
iostream* __thiscall iostream_scalar_dtor(ios *base, unsigned int flags)
{
    iostream *this = ios_to_iostream(base);

    TRACE("(%p %x)\n", this, flags);

    iostream_vbase_dtor(this);
    if (flags & 1) operator_delete(this);
    return this;
}

static iostream* iostream_internal_copy_ctor(iostream *this, const iostream *copy, const vtable_ptr *vtbl, BOOL virt_init)
{
    ios *base, *base_copy = istream_get_ios(&copy->base1);

    if (virt_init) {
        this->base1.vbtable = iostream_vbtable_istream;
        this->base2.vbtable = iostream_vbtable_ostream;
        base = istream_get_ios(&this->base1);
        ios_copy_ctor(base, base_copy);
    } else
        base = istream_get_ios(&this->base1);
    ios_init(base, base_copy->sb);
    istream_ctor(&this->base1, FALSE);
    ostream_ctor(&this->base2, FALSE);
    base->vtable = vtbl;
    return this;
}

static iostream* iostream_internal_sb_ctor(iostream *this, streambuf *sb, const vtable_ptr *vtbl, BOOL virt_init)
{
    ios *base;

    iostream_ctor(this, virt_init);
    base = istream_get_ios(&this->base1);
    if (sb)
        ios_init(base, sb);
    base->vtable = vtbl;
    base->delbuf = 1;
    return this;
}

/* ??0strstream@@QAE@ABV0@@Z */
/* ??0strstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(strstream_copy_ctor, 12)
iostream* __thiscall strstream_copy_ctor(iostream *this, const iostream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    return iostream_internal_copy_ctor(this, copy, &strstream_vtable, virt_init);
}

/* ??0strstream@@QAE@PADHH@Z */
/* ??0strstream@@QEAA@PEADHH@Z */
DEFINE_THISCALL_WRAPPER(strstream_buffer_ctor, 20)
iostream* __thiscall strstream_buffer_ctor(iostream *this, char *buffer, int length, int mode, BOOL virt_init)
{
    strstreambuf *ssb = operator_new(sizeof(strstreambuf));

    TRACE("(%p %p %d %d %d)\n", this, buffer, length, mode, virt_init);

    if (!ssb) {
        FIXME("out of memory\n");
        return NULL;
    }

    strstreambuf_buffer_ctor(ssb, buffer, length, buffer);

    if ((mode & OPENMODE_out) && (mode & (OPENMODE_app|OPENMODE_ate)))
        ssb->base.pptr = buffer + strlen(buffer);

    return iostream_internal_sb_ctor(this, &ssb->base, &strstream_vtable, virt_init);
}

/* ??0strstream@@QAE@XZ */
/* ??0strstream@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstream_ctor, 8)
iostream* __thiscall strstream_ctor(iostream *this, BOOL virt_init)
{
    strstreambuf *ssb = operator_new(sizeof(strstreambuf));

    TRACE("(%p %d)\n", this, virt_init);

    if (!ssb) {
        FIXME("out of memory\n");
        return NULL;
    }

    strstreambuf_ctor(ssb);

    return iostream_internal_sb_ctor(this, &ssb->base, &strstream_vtable, virt_init);
}

/* ?pcount@strstream@@QBEHXZ */
/* ?pcount@strstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(strstream_pcount, 4)
int __thiscall strstream_pcount(const iostream *this)
{
    return streambuf_out_waiting(istream_get_ios(&this->base1)->sb);
}

/* ?rdbuf@strstream@@QBEPAVstrstreambuf@@XZ */
/* ?rdbuf@strstream@@QEBAPEAVstrstreambuf@@XZ */
DEFINE_THISCALL_WRAPPER(strstream_rdbuf, 4)
strstreambuf* __thiscall strstream_rdbuf(const iostream *this)
{
    return (strstreambuf*) istream_get_ios(&this->base1)->sb;
}

/* ?str@strstream@@QAEPADXZ */
/* ?str@strstream@@QEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(strstream_str, 4)
char* __thiscall strstream_str(iostream *this)
{
    return strstreambuf_str(strstream_rdbuf(this));
}

/* ??0stdiostream@@QAE@ABV0@@Z */
/* ??0stdiostream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(stdiostream_copy_ctor, 12)
iostream* __thiscall stdiostream_copy_ctor(iostream *this, const iostream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    return iostream_internal_copy_ctor(this, copy, &stdiostream_vtable, virt_init);
}

/* ??0stdiostream@@QAE@PAU_iobuf@@@Z */
/* ??0stdiostream@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(stdiostream_file_ctor, 12)
iostream* __thiscall stdiostream_file_ctor(iostream *this, FILE *file, BOOL virt_init)
{
    stdiobuf *stb = operator_new(sizeof(stdiobuf));

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if (!stb) {
        FIXME("out of memory\n");
        return NULL;
    }

    stdiobuf_file_ctor(stb, file);

    return iostream_internal_sb_ctor(this, &stb->base, &stdiostream_vtable, virt_init);
}

/* ?rdbuf@stdiostream@@QBEPAVstdiobuf@@XZ */
/* ?rdbuf@stdiostream@@QEBAPEAVstdiobuf@@XZ */
DEFINE_THISCALL_WRAPPER(stdiostream_rdbuf, 4)
stdiobuf* __thiscall stdiostream_rdbuf(const iostream *this)
{
    return (stdiobuf*) istream_get_ios(&this->base1)->sb;
}

/* ??0fstream@@QAE@ABV0@@Z */
/* ??0fstream@@QEAA@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(fstream_copy_ctor, 12)
iostream* __thiscall fstream_copy_ctor(iostream *this, const iostream *copy, BOOL virt_init)
{
    TRACE("(%p %p %d)\n", this, copy, virt_init);
    iostream_internal_copy_ctor(this, copy, &fstream_vtable, virt_init);
    return this;
}

/* ??0fstream@@QAE@HPADH@Z */
/* ??0fstream@@QEAA@HPEADH@Z */
DEFINE_THISCALL_WRAPPER(fstream_buffer_ctor, 20)
iostream* __thiscall fstream_buffer_ctor(iostream *this, filedesc fd, char *buffer, int length, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %p %d %d)\n", this, fd, buffer, length, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_reserve_ctor(fb, fd, buffer, length);

    iostream_internal_sb_ctor(this, &fb->base, &fstream_vtable, virt_init);

    base = istream_get_ios(&this->base1);
    base->delbuf = 1;

    return this;
}

/* ??0fstream@@QAE@H@Z */
/* ??0fstream@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(fstream_fd_ctor, 12)
iostream* __thiscall fstream_fd_ctor(iostream *this, filedesc fd, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %d %d)\n", this, fd, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_fd_ctor(fb, fd);

    iostream_internal_sb_ctor(this, &fb->base, &fstream_vtable, virt_init);

    base = istream_get_ios(&this->base1);
    base->delbuf = 1;

    return this;
}

/* ??0fstream@@QAE@PBDHH@Z */
/* ??0fstream@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(fstream_open_ctor, 20)
iostream* __thiscall fstream_open_ctor(iostream *this, const char *name, ios_open_mode mode, int protection, BOOL virt_init)
{
    ios *base;
    filebuf *fb = operator_new(sizeof(filebuf));

    TRACE("(%p %s %d %d %d)\n", this, name, mode, protection, virt_init);

    if (!fb) {
        FIXME("out of memory\n");
        return NULL;
    }

    filebuf_ctor(fb);

    iostream_internal_sb_ctor(this, &fb->base, &fstream_vtable, virt_init);

    base = istream_get_ios(&this->base1);
    base->delbuf = 1;

    if (filebuf_open(fb, name, mode, protection) == NULL)
        base->state |= IOSTATE_failbit;
    return this;
}

/* ??0fstream@@QAE@XZ */
/* ??0fstream@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(fstream_ctor, 8)
iostream* __thiscall fstream_ctor(iostream *this, BOOL virt_init)
{
    return fstream_fd_ctor(this, -1, virt_init);
}

/* ?rdbuf@fstream@@QBEPAVfilebuf@@XZ */
/* ?rdbuf@fstream@@QEBAPEAVfilebuf@@XZ */
DEFINE_THISCALL_WRAPPER(fstream_rdbuf, 4)
filebuf* __thiscall fstream_rdbuf(const iostream *this)
{
    TRACE("(%p)\n", this);
    return (filebuf*) istream_get_ios(&this->base1)->sb;
}

/* ?fd@fstream@@QBEHXZ */
/* ?fd@fstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(fstream_fd, 4)
filedesc __thiscall fstream_fd(iostream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_fd(fstream_rdbuf(this));
}

/* ?attach@fstream@@QAEXH@Z */
/* ?attach@fstream@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(fstream_attach, 8)
void __thiscall fstream_attach(iostream *this, filedesc fd)
{
    ios *base = istream_get_ios(&this->base1);
    TRACE("(%p %d)\n", this, fd);
    if (filebuf_attach(fstream_rdbuf(this), fd) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?close@fstream@@QAEXXZ */
/* ?close@fstream@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(fstream_close, 4)
void __thiscall fstream_close(iostream *this)
{
    ios *base = istream_get_ios(&this->base1);
    TRACE("(%p)\n", this);
    if (filebuf_close(fstream_rdbuf(this)) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
    else
        ios_clear(base, IOSTATE_goodbit);
}

/* ?is_open@fstream@@QBEHXZ */
/* ?is_open@fstream@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(fstream_is_open, 4)
int __thiscall fstream_is_open(const iostream *this)
{
    TRACE("(%p)\n", this);
    return filebuf_is_open(fstream_rdbuf(this));
}

/* ?open@fstream@@QAEXPBDHH@Z */
/* ?open@fstream@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(fstream_open, 16)
void __thiscall fstream_open(iostream *this, const char *name, ios_open_mode mode, int protection)
{
    ios *base = istream_get_ios(&this->base1);
    TRACE("(%p %s %d %d)\n", this, name, mode, protection);
    if (filebuf_open(fstream_rdbuf(this), name, mode|OPENMODE_out, protection) == NULL)
        ios_clear(base, base->state | IOSTATE_failbit);
}

/* ?setbuf@fstream@@QAEPAVstreambuf@@PADH@Z */
/* ?setbuf@fstream@@QEAAPEAVstreambuf@@PEADH@Z */
DEFINE_THISCALL_WRAPPER(fstream_setbuf, 12)
streambuf* __thiscall fstream_setbuf(iostream *this, char *buffer, int length)
{
    ios *base = istream_get_ios(&this->base1);
    filebuf* fb = fstream_rdbuf(this);

    TRACE("(%p %p %d)\n", this, buffer, length);

    if (filebuf_is_open(fb)) {
        ios_clear(base, base->state | IOSTATE_failbit);
        return NULL;
    }

    return filebuf_setbuf(fb, buffer, length);
}

/* ?setmode@fstream@@QAEHH@Z */
/* ?setmode@fstream@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(fstream_setmode, 8)
int __thiscall fstream_setmode(iostream *this, int mode)
{
    TRACE("(%p %d)\n", this, mode);
    return filebuf_setmode(fstream_rdbuf(this), mode);
}

/* ??0Iostream_init@@QAE@AAVios@@H@Z */
/* ??0Iostream_init@@QEAA@AEAVios@@H@Z */
DEFINE_THISCALL_WRAPPER(Iostream_init_ios_ctor, 12)
void* __thiscall Iostream_init_ios_ctor(void *this, ios *obj, int n)
{
    TRACE("(%p %p %d)\n", this, obj, n);
    obj->delbuf = 1;
    if (n >= 0) {
        obj->tie = &cout.os;
        if (n > 0)
            ios_setf(obj, FLAGS_unitbuf);
    }
    return this;
}

/* ??0Iostream_init@@QAE@XZ */
/* ??0Iostream_init@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(Iostream_init_ctor, 4)
void* __thiscall Iostream_init_ctor(void *this)
{
    TRACE("(%p)\n", this);
    return this;
}

/* ??1Iostream_init@@QAE@XZ */
/* ??1Iostream_init@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(Iostream_init_dtor, 4)
void __thiscall Iostream_init_dtor(void *this)
{
    TRACE("(%p)\n", this);
}

/* ??4Iostream_init@@QAEAAV0@ABV0@@Z */
/* ??4Iostream_init@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(Iostream_init_assign, 8)
void* __thiscall Iostream_init_assign(void *this, const void *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    return this;
}

/* ?sync_with_stdio@ios@@SAXXZ */
void __cdecl ios_sync_with_stdio(void)
{
    if (!ios_sunk_with_stdio) {
        stdiobuf *new_buf;

        TRACE("()\n");

        /* run at most once */
        ios_sunk_with_stdio++;

         /* calls to [io]stream_assign_sb automatically destroy the old buffers */
        if ((new_buf = operator_new(sizeof(stdiobuf)))) {
            stdiobuf_file_ctor(new_buf, stdin);
            istream_assign_sb(&cin.is, &new_buf->base);
        } else
            istream_assign_sb(&cin.is, NULL);
        cin.vbase.delbuf = 1;
        ios_setf(&cin.vbase, FLAGS_stdio);

        if ((new_buf = operator_new(sizeof(stdiobuf)))) {
            stdiobuf_file_ctor(new_buf, stdout);
            stdiobuf_setrwbuf(new_buf, 0, 80);
            ostream_assign_sb(&cout.os, &new_buf->base);
        } else
            ostream_assign_sb(&cout.os, NULL);
        cout.vbase.delbuf = 1;
        ios_setf(&cout.vbase, FLAGS_unitbuf | FLAGS_stdio);

        if ((new_buf = operator_new(sizeof(stdiobuf)))) {
            stdiobuf_file_ctor(new_buf, stderr);
            stdiobuf_setrwbuf(new_buf, 0, 80);
            ostream_assign_sb(&cerr.os, &new_buf->base);
        } else
            ostream_assign_sb(&cerr.os, NULL);
        cerr.vbase.delbuf = 1;
        ios_setf(&cerr.vbase, FLAGS_unitbuf | FLAGS_stdio);

        if ((new_buf = operator_new(sizeof(stdiobuf)))) {
            stdiobuf_file_ctor(new_buf, stderr);
            stdiobuf_setrwbuf(new_buf, 0, 512);
            ostream_assign_sb(&MSVCP_clog.os, &new_buf->base);
        } else
            ostream_assign_sb(&MSVCP_clog.os, NULL);
        MSVCP_clog.vbase.delbuf = 1;
        ios_setf(&MSVCP_clog.vbase, FLAGS_stdio);
    }
}


#ifdef __ASM_USE_THISCALL_WRAPPER

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

void __cdecl _mtlock(CRITICAL_SECTION *crit)
{
    TRACE("(%p)\n", crit);
    EnterCriticalSection(crit);
}

void __cdecl _mtunlock(CRITICAL_SECTION *crit)
{
    TRACE("(%p)\n", crit);
    LeaveCriticalSection(crit);
}

static void* (__cdecl *MSVCRT_operator_new)(SIZE_T);
static void (__cdecl *MSVCRT_operator_delete)(void*);

void* __cdecl operator_new(SIZE_T size)
{
    return MSVCRT_operator_new(size);
}

void __cdecl operator_delete(void *mem)
{
    MSVCRT_operator_delete(mem);
}

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
    filebuf *fb;

#ifdef __x86_64__
    init_streambuf_rtti(base);
    init_filebuf_rtti(base);
    init_strstreambuf_rtti(base);
    init_stdiobuf_rtti(base);
    init_ios_rtti(base);
    init_ostream_rtti(base);
    init_ostream_withassign_rtti(base);
    init_ostrstream_rtti(base);
    init_ofstream_rtti(base);
    init_istream_rtti(base);
    init_istream_withassign_rtti(base);
    init_istrstream_rtti(base);
    init_ifstream_rtti(base);
    init_iostream_rtti(base);
    init_strstream_rtti(base);
    init_stdiostream_rtti(base);
    init_fstream_rtti(base);
#endif

    if ((fb = operator_new(sizeof(filebuf)))) {
        filebuf_fd_ctor(fb, 0);
        istream_withassign_sb_ctor(&cin.is, &fb->base, TRUE);
    } else
        istream_withassign_sb_ctor(&cin.is, NULL, TRUE);
    Iostream_init_ios_ctor(NULL, &cin.vbase, 0);

    if ((fb = operator_new(sizeof(filebuf)))) {
        filebuf_fd_ctor(fb, 1);
        ostream_withassign_sb_ctor(&cout.os, &fb->base, TRUE);
    } else
        ostream_withassign_sb_ctor(&cout.os, NULL, TRUE);
    Iostream_init_ios_ctor(NULL, &cout.vbase, -1);

    if ((fb = operator_new(sizeof(filebuf)))) {
        filebuf_fd_ctor(fb, 2);
        ostream_withassign_sb_ctor(&cerr.os, &fb->base, TRUE);
    } else
        ostream_withassign_sb_ctor(&cerr.os, NULL, TRUE);
    Iostream_init_ios_ctor(NULL, &cerr.vbase, 1);

    if ((fb = operator_new(sizeof(filebuf)))) {
        filebuf_fd_ctor(fb, 2);
        ostream_withassign_sb_ctor(&MSVCP_clog.os, &fb->base, TRUE);
    } else
        ostream_withassign_sb_ctor(&MSVCP_clog.os, NULL, TRUE);
    Iostream_init_ios_ctor(NULL, &MSVCP_clog.vbase, 0);
}

static void free_io(void)
{
    /* destructors take care of deleting the buffers */
    istream_vbase_dtor(&cin.is);
    ostream_vbase_dtor(&cout.os);
    ostream_vbase_dtor(&cerr.os);
    ostream_vbase_dtor(&MSVCP_clog.os);
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
       init_cxx_funcs();
       init_exception(inst);
       init_io(inst);
       DisableThreadLibraryCalls( inst );
       break;
   case DLL_PROCESS_DETACH:
       if (reserved) break;
       free_io();
       break;
   }
   return TRUE;
}
