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

#include "msvcp90.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

typedef enum {
    IOSTATE_goodbit   = 0x00,
    IOSTATE_eofbit    = 0x01,
    IOSTATE_failbit   = 0x02,
    IOSTATE_badbit    = 0x04,
    IOSTATE__Hardfail = 0x10
} IOSB_iostate;

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
    FMTFLAG_floadfield  = FMTFLAG_scientific|FMTFLAG_fixed
} IOSB_fmtflags;

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

/* ?_Index@ios_base@std@@0HA */
int ios_base_Index = 0;
/* ?_Sync@ios_base@std@@0_NA */
MSVCP_bool ios_base_Sync = FALSE;

typedef struct _ios_base {
    const vtable_ptr *vtable;
    MSVCP_size_t stdstr;
    IOSB_iostate state;
    IOSB_iostate except;
    IOSB_fmtflags fmtfl;
    MSVCP_size_t prec;
    MSVCP_size_t wide;
    IOS_BASE_iosarray *arr;
    IOS_BASE_fnarray *calls;
    locale *loc;
} ios_base;

typedef struct {
    ios_base child;
    /*basic_streambuf_char*/void *strbuf;
    /*basic_ostream_char*/void *stream;
    char fillch;
} basic_ios_char;

/* ??_7ios_base@std@@6B@ */
extern const vtable_ptr MSVCP_ios_base_vtable;

/* ??_7?$basic_ios@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_basic_ios_char_vtable;

static const type_info ios_base_type_info = {
    &MSVCP_ios_base_vtable,
    NULL,
    ".?AVios_base@std@@"
};

static const rtti_base_descriptor ios_base_rtti_base_descriptor = {
    &ios_base_type_info,
    1,
    { 0, -1, 0 },
    64
};

static const type_info iosb_type_info = {
    &MSVCP_ios_base_vtable,
    NULL,
    ".?AV?$_Iosb@H@std@@"
};

static const rtti_base_descriptor iosb_rtti_base_descriptor = {
    &iosb_type_info,
    0,
    { 4, -1, 0 },
    64
};

static const rtti_base_array ios_base_rtti_base_array = {
    {
        &ios_base_rtti_base_descriptor,
        &iosb_rtti_base_descriptor,
        NULL
    }
};

static const rtti_object_hierarchy ios_base_type_hierarchy = {
    0,
    0,
    2,
    &ios_base_rtti_base_array
};

const rtti_object_locator ios_base_rtti = {
    0,
    0,
    0,
    &ios_base_type_info,
    &ios_base_type_hierarchy
};

static const type_info basic_ios_char_type_info = {
    &MSVCP_basic_ios_char_vtable,
    NULL,
    ".?AV?$basic_ios@DU?$char_traits@D@std@@@std@@"
};

static const rtti_base_descriptor basic_ios_char_rtti_base_descriptor = {
    &basic_ios_char_type_info,
    2,
    { 0, -1, 0 },
    64
};

static const rtti_base_array basic_ios_char_rtti_base_array = {
    {
        &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor,
        &iosb_rtti_base_descriptor
    }
};

static const rtti_object_hierarchy basic_ios_char_hierarchy = {
    0,
    0,
    3,
    &basic_ios_char_rtti_base_array
};

const rtti_object_locator basic_ios_char_rtti = {
    0,
    0,
    0,
    &basic_ios_char_type_info,
    &basic_ios_char_hierarchy
};

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(ios_base, "");
    __ASM_VTABLE(basic_ios_char, "");
#ifndef __GNUC__
}
#endif

/* ??0ios_base@std@@IAE@XZ */
/* ??0ios_base@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_ctor, 4)
ios_base* __thiscall ios_base_ctor(ios_base *this)
{
    FIXME("(%p) stub\n", this);

    this->vtable = &MSVCP_ios_base_vtable;
    return NULL;
}

/* ??0ios_base@std@@QAE@ABV01@@Z */
/* ??0ios_base@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copy_ctor, 8)
ios_base* __thiscall ios_base_copy_ctor(ios_base *this, const ios_base *copy)
{
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
}

/* ??1ios_base@std@@UAE@XZ */
/* ??1ios_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_dtor, 4)
void __thiscall ios_base_dtor(ios_base *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_ios_base_vector_dtor, 8)
ios_base* __thiscall MSVCP_ios_base_vector_dtor(ios_base *this, unsigned int flags)
{
    TRACE("(%p %x) stub\n", this, flags);
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

/* ??4ios_base@std@@QAEAAV01@ABV01@@Z */
/* ??4ios_base@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_assign, 8)
ios_base* __thiscall ios_base_assign(ios_base *this, const ios_base *right)
{
    FIXME("(%p %p) stub\n", this, right);
    return NULL;
}

/* ??7ios_base@std@@QBE_NXZ */
/* ??7ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_succ, 4)
MSVCP_bool __thiscall ios_base_op_succ(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return FALSE;
}

/* ??Bios_base@std@@QBEPAXXZ */
/* ??Bios_base@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_fail, 4)
void* __thiscall ios_base_op_fail(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Addstd@ios_base@std@@SAXPAV12@@Z */
/* ?_Addstd@ios_base@std@@SAXPEAV12@@Z */
void CDECL ios_base_Addstd(ios_base *add)
{
    FIXME("(%p) stub\n", add);
}

/* ?_Callfns@ios_base@std@@AAEXW4event@12@@Z */
/* ?_Callfns@ios_base@std@@AEAAXW4event@12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Callfns, 8)
void __thiscall ios_base_Callfns(ios_base *this, IOS_BASE_event event)
{
    FIXME("(%p %x) stub\n", this, event);
}

/* ?_Findarr@ios_base@std@@AAEAAU_Iosarray@12@H@Z */
/* ?_Findarr@ios_base@std@@AEAAAEAU_Iosarray@12@H@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Findarr, 8)
IOS_BASE_iosarray* __thiscall ios_base_Findarr(ios_base *this, int index)
{
    FIXME("(%p %d) stub\n", this, index);
    return NULL;
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
    FIXME("(%p) stub\n", this);
}

/* ?_Ios_base_dtor@ios_base@std@@CAXPAV12@@Z */
/* ?_Ios_base_dtor@ios_base@std@@CAXPEAV12@@Z */
void CDECL ios_base_Ios_base_dtor(ios_base *obj)
{
    FIXME("(%p) stub\n", obj);
}

/* ?_Sync_func@ios_base@std@@CAAA_NXZ */
/* ?_Sync_func@ios_base@std@@CAAEA_NXZ */
MSVCP_bool* CDECL ios_base_Sync_func(void)
{
    TRACE("\n");
    return &ios_base_Sync;
}

/* ?_Tidy@ios_base@std@@AAAXXZ */
/* ?_Tidy@ios_base@std@@AEAAXXZ */
void CDECL ios_base_Tidy(ios_base *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?bad@ios_base@std@@QBE_NXZ */
/* ?bad@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_bad, 4)
MSVCP_bool __thiscall ios_base_bad(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return FALSE;
}

/* ?clear@ios_base@std@@QAEXH_N@Z */
/* ?clear@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_reraise, 12)
void __thiscall ios_base_clear_reraise(ios_base *this, IOSB_iostate state, MSVCP_bool reraise)
{
    FIXME("(%p %x %x) stub\n", this, state, reraise);
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

/* ?copyfmt@ios_base@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@ios_base@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copyfmt, 8)
ios_base* __thiscall ios_base_copyfmt(ios_base *this, const ios_base *obj)
{
    FIXME("(%p %p) stub\n", this, obj);
    return NULL;
}

/* ?eof@ios_base@std@@QBE_NXZ */
/* ?eof@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_eof, 4)
MSVCP_bool __thiscall ios_base_eof(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return FALSE;
}

/* ?exceptions@ios_base@std@@QAEXH@Z */
/* ?exceptions@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exception_set, 8)
void __thiscall ios_base_exception_set(ios_base *this, IOSB_iostate state)
{
    FIXME("(%p %x) stub\n", this, state);
}

/* ?exceptions@ios_base@std@@QAEXI@Z */
/* ?exceptions@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exception_set_unsigned, 8)
void __thiscall ios_base_exception_set_unsigned(ios_base *this, unsigned int state)
{
    FIXME("(%p %x) stub\n", this, state);
}

/* ?exceptions@ios_base@std@@QBEHXZ */
/* ?exceptions@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_exception_get, 4)
IOSB_iostate __thiscall ios_base_exception_get(ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?fail@ios_base@std@@QBE_NXZ */
/* ?fail@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_fail, 4)
MSVCP_bool __thiscall ios_base_fail(ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?flags@ios_base@std@@QAEHH@Z */
/* ?flags@ios_base@std@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_flags_set, 8)
IOSB_fmtflags __thiscall ios_base_flags_set(ios_base *this, IOSB_fmtflags flags)
{
    FIXME("(%p %x) stub\n", this, flags);
    return 0;
}

/* ?flags@ios_base@std@@QBEHXZ */
/* ?flags@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_flags_get, 4)
IOSB_fmtflags __thiscall ios_base_flags_get(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?getloc@ios_base@std@@QBE?AVlocale@2@XZ */
/* ?getloc@ios_base@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_getloc, 4)
locale __thiscall ios_base_getloc(const ios_base *this)
{
    locale ret = { NULL };
    FIXME("(%p) stub\n", this);
    return ret;
}

/* ?good@ios_base@std@@QBE_NXZ */
/* ?good@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_good, 4)
MSVCP_bool __thiscall ios_base_good(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return FALSE;
}

/* ?imbue@ios_base@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@ios_base@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_imbue, 8)
locale __thiscall ios_base_imbue(ios_base *this, const locale *loc)
{
    locale ret = { NULL };
    FIXME("(%p %p) stub\n", this, loc);
    return ret;
}

/* ?iword@ios_base@std@@QAEAAJH@Z */
/* ?iword@ios_base@std@@QEAAAEAJH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_iword, 8)
MSVCP_long* __thiscall ios_base_iword(ios_base *this, int index)
{
    FIXME("(%p %d) stub\n", this, index);
    return NULL;
}

/* ?precision@ios_base@std@@QAEHH@Z */
/* ?precision@ios_base@std@@QEAA_J_J@Z */
DEFINE_THISCALL_WRAPPER(ios_base_precision_set, 8)
MSVCP_size_t __thiscall ios_base_precision_set(ios_base *this, MSVCP_size_t precision)
{
    FIXME("(%p %lu) stub\n", this, precision);
    return 0;
}

/* ?precision@ios_base@std@@QBEHXZ */
/* ?precision@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_precision_get, 4)
MSVCP_size_t __thiscall ios_base_precision_get(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?pword@ios_base@std@@QAEAAPAXH@Z */
/* ?pword@ios_base@std@@QEAAAEAPEAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_pword, 8)
void** __thiscall ios_base_pword(ios_base *this, int index)
{
    FIXME("(%p %d) stub\n", this, index);
    return NULL;
}

/* ?rdstate@ios_base@std@@QBEHXZ */
/* ?rdstate@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_rdstate, 4)
IOSB_iostate __thiscall ios_base_rdstate(const ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?register_callback@ios_base@std@@QAEXP6AXW4event@12@AAV12@H@ZH@Z */
/* ?register_callback@ios_base@std@@QEAAXP6AXW4event@12@AEAV12@H@ZH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_register_callback, 12)
void __thiscall ios_base_register_callback(ios_base *this, IOS_BASE_event_callback callback, int index)
{
    FIXME("(%p %p %d) stub\n", this, callback, index);
}

/* ?setf@ios_base@std@@QAEHHH@Z */
/* ?setf@ios_base@std@@QEAAHHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setf_mask, 12)
IOSB_fmtflags __thiscall ios_base_setf_mask(ios_base *this, IOSB_fmtflags flags, IOSB_fmtflags mask)
{
    FIXME("(%p %x %x) stub\n", this, flags, mask);
    return 0;
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
    FIXME("(%p %x %x) stub\n", this, state, reraise);
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
    FIXME("(%x) stub\n", sync);
    return FALSE;
}

/* ?unsetf@ios_base@std@@QAEXH@Z */
/* ?unsetf@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_unsetf, 8)
void __thiscall ios_base_unsetf(ios_base *this, IOSB_fmtflags flags)
{
    FIXME("(%p %x) stub\n", this, flags);
}

/* ?width@ios_base@std@@QAEHH@Z */
/* ?width@ios_base@std@@QEAA_J_J@Z */
DEFINE_THISCALL_WRAPPER(ios_base_width_set, 8)
MSVCP_size_t __thiscall ios_base_width_set(ios_base *this, MSVCP_size_t width)
{
    FIXME("(%p %lu) stub\n", this, width);
    return 0;
}

/* ?width@ios_base@std@@QBEHXZ */
/* ?width@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_width_get, 4)
MSVCP_size_t __thiscall ios_base_width_get(ios_base *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?xalloc@ios_base@std@@SAHXZ */
int CDECL ios_base_xalloc(void)
{
    FIXME("stub\n");
    return 0;
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_ctor, 4)
basic_ios_char* __thiscall basic_ios_char_ctor(basic_ios_char *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_copy_ctor, 8)
basic_ios_char* __thiscall basic_ios_char_copy_ctor(basic_ios_char *this, const basic_ios_char *copy)
{
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
}

/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_dtor, 4)
void __thiscall basic_ios_char_dtor(basic_ios_char *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_basic_ios_char_vector_dtor, 8)
basic_ios_char* __thiscall MSVCP_basic_ios_char_vector_dtor(basic_ios_char *this, unsigned int flags)
{
    TRACE("(%p %x) stub\n", this, flags);
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
    FIXME("(%p %x %x) stub\n", this, state, reraise);
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
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEDD@Z */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_set, 8)
char __thiscall basic_ios_char_fill_set(basic_ios_char *this, char fill)
{
    FIXME("(%p %c) stub\n", this, fill);
    return 0;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDXZ */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_get, 4)
char __thiscall basic_ios_char_fill_get(basic_ios_char *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_imbue, 4)
locale __thiscall basic_ios_char_imbue(basic_ios_char *this)
{
    locale ret = { NULL };
    FIXME("(%p) stub\n", this);
    return ret;
}

/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IAEXPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_init, 12)
void __thiscall basic_ios_char_init(basic_ios_char *this, /*basic_streambuf_char*/void *streambuf, MSVCP_bool isstd)
{
    FIXME("(%p %p %x) stub\n", this, streambuf, isstd);
}

/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDDD@Z */
/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_narrow, 12)
char __thiscall basic_ios_char_narrow(basic_ios_char *this, char ch, char def)
{
    FIXME("(%p %c %c) stub\n", this, ch, def);
    return def;
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_set, 8)
/*basic_streambuf_char*/void* __thiscall basic_ios_char_rdbuf_set(basic_ios_char *this, /*basic_streambuf_char*/void *streambuf)
{
    FIXME("(%p %p) stub\n", this, streambuf);
    return NULL;
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_get, 4)
/*basic_streambuf_char*/void* __thiscall basic_ios_char_rdbuf_get(const basic_ios_char *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_setstate_reraise, 12)
void __thiscall basic_ios_char_setstate_reraise(basic_ios_char *this, IOSB_iostate state, MSVCP_bool reraise)
{
    FIXME("(%p %x %x) stub\n", this, state, reraise);
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
/*basic_ostream_char*/void* __thiscall basic_ios_char_tie_set(basic_ios_char *this, /*basic_ostream_char*/void *ostream)
{
    FIXME("(%p %p) stub\n", this, ostream);
    return NULL;
}

/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_tie_get, 4)
/*basic_ostream_char*/void* __thiscall basic_ios_char_tie_get(const basic_ios_char *this)
{
    FIXME("(%p)\n", this);
    return NULL;
}

/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDD@Z */
/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_widen, 8)
char __thiscall basic_ios_char_widen(basic_ios_char *this, char ch)
{
    FIXME("(%p %c)\n", this, ch);
    return 0;
}
