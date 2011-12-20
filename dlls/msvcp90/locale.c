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

#include "msvcp90.h"
#include "locale.h"

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

char* __cdecl _Getdays(void);
char* __cdecl _Getmonths(void);
void* __cdecl _Gettnames(void);

typedef int category;

typedef struct {
    MSVCP_size_t id;
} locale_id;

typedef struct {
    const vtable_ptr *vtable;
    MSVCP_size_t refs;
} locale_facet;

typedef struct _locale__Locimp {
    locale_facet facet;
    locale_facet **facetvec;
    MSVCP_size_t facet_cnt;
    category catmask;
    MSVCP_bool transparent;
    basic_string_char name;
} locale__Locimp;

typedef struct {
    void *timeptr;
} _Timevec;

typedef struct {
    _Lockit lock;
    basic_string_char days;
    basic_string_char months;
    basic_string_char oldlocname;
    basic_string_char newlocname;
} _Locinfo;

typedef struct {
    LCID handle;
    unsigned page;
} _Collvec;

typedef struct {
    LCID handle;
    unsigned page;
    short *table;
    int delfl;
} _Ctypevec;

typedef struct {
    LCID handle;
    unsigned page;
} _Cvtvec;

typedef struct {
    locale_facet facet;
    _Collvec coll;
} collate;

typedef struct {
    locale_facet facet;
} ctype_base;

typedef struct {
    ctype_base base;
    _Ctypevec ctype;
} ctype_char;

typedef struct {
    ctype_base base;
    _Ctypevec ctype;
    _Cvtvec cvt;
} ctype_wchar;

/* ?_Id_cnt@id@locale@std@@0HA */
int locale_id__Id_cnt = 0;

/* ?_Clocptr@_Locimp@locale@std@@0PAV123@A */
/* ?_Clocptr@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *locale__Locimp__Clocptr = NULL;

/* ??1facet@locale@std@@UAE@XZ */
/* ??1facet@locale@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet_dtor, 4)
void __thiscall locale_facet_dtor(locale_facet *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale_facet_vector_dtor, 8)
#define call_locale_facet_vector_dtor(this, flags) CALL_VTBL_FUNC(this, 0, \
        locale_facet*, (locale_facet*, unsigned int), (this, flags))
locale_facet* __thiscall MSVCP_locale_facet_vector_dtor(locale_facet *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            locale_facet_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        locale_facet_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

const vtable_ptr MSVCP_locale_facet_vtable[] = {
    (vtable_ptr)THISCALL_NAME(MSVCP_locale_facet_vector_dtor)
};

/* ??0id@locale@std@@QAE@I@Z */
/* ??0id@locale@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_id_ctor_id, 8)
locale_id* __thiscall locale_id_ctor_id(locale_id *this, MSVCP_size_t id)
{
    TRACE("(%p %lu)\n", this, id);

    this->id = id;
    return this;
}

/* ??_Fid@locale@std@@QAEXXZ */
/* ??_Fid@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_id_ctor, 4)
locale_id* __thiscall locale_id_ctor(locale_id *this)
{
    TRACE("(%p)\n", this);

    this->id = 0;
    return this;
}

/* ??Bid@locale@std@@QAEIXZ */
/* ??Bid@locale@std@@QEAA_KXZ */
DEFINE_THISCALL_WRAPPER(locale_id_operator_size_t, 4)
MSVCP_size_t __thiscall locale_id_operator_size_t(locale_id *this)
{
    _Lockit lock;

    TRACE("(%p)\n", this);

    if(!this->id) {
        _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
        this->id = ++locale_id__Id_cnt;
        _Lockit_dtor(&lock);
    }

    return this->id;
}

/* ?_Id_cnt_func@id@locale@std@@CAAAHXZ */
/* ?_Id_cnt_func@id@locale@std@@CAAEAHXZ */
int* __cdecl locale_id__Id_cnt_func(void)
{
    TRACE("\n");
    return &locale_id__Id_cnt;
}

/* ??_Ffacet@locale@std@@QAEXXZ */
/* ??_Ffacet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor, 4)
locale_facet* __thiscall locale_facet_ctor(locale_facet *this)
{
    TRACE("(%p)\n", this);
    this->vtable = MSVCP_locale_facet_vtable;
    this->refs = 0;
    return this;
}

/* ??0facet@locale@std@@IAE@I@Z */
/* ??0facet@locale@std@@IEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor_refs, 8)
locale_facet* __thiscall locale_facet_ctor_refs(locale_facet *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    this->vtable = MSVCP_locale_facet_vtable;
    this->refs = refs;
    return this;
}

/* ?_Incref@facet@locale@std@@QAEXXZ */
/* ?_Incref@facet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet__Incref, 4)
void __thiscall locale_facet__Incref(locale_facet *this)
{
    _Lockit lock;

    TRACE("(%p)\n", this);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    this->refs++;
    _Lockit_dtor(&lock);
}

/* ?_Decref@facet@locale@std@@QAEPAV123@XZ */
/* ?_Decref@facet@locale@std@@QEAAPEAV123@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet__Decref, 4)
locale_facet* __thiscall locale_facet__Decref(locale_facet *this)
{
    _Lockit lock;
    locale_facet *ret;

    TRACE("(%p)\n", this);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(this->refs)
        this->refs--;

    ret = this->refs ? NULL : this;
    _Lockit_dtor(&lock);

    return ret;
}

/* ?_Getcat@facet@locale@std@@SAIPAPBV123@PBV23@@Z */
/* ?_Getcat@facet@locale@std@@SA_KPEAPEBV123@PEBV23@@Z */
MSVCP_size_t __cdecl locale_facet__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);
    return -1;
}

/* ??0_Timevec@std@@QAE@ABV01@@Z */
/* ??0_Timevec@std@@QEAA@AEBV01@@Z */
/* This copy constructor modifies copied object */
DEFINE_THISCALL_WRAPPER(_Timevec_copy_ctor, 8)
_Timevec* __thiscall _Timevec_copy_ctor(_Timevec *this, _Timevec *copy)
{
    TRACE("(%p %p)\n", this, copy);
    this->timeptr = copy->timeptr;
    copy->timeptr = NULL;
    return this;
}

/* ??0_Timevec@std@@QAE@PAX@Z */
/* ??0_Timevec@std@@QEAA@PEAX@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor_timeptr, 8)
_Timevec* __thiscall _Timevec_ctor_timeptr(_Timevec *this, void *timeptr)
{
    TRACE("(%p %p)\n", this, timeptr);
    this->timeptr = timeptr;
    return this;
}

/* ??_F_Timevec@std@@QAEXXZ */
/* ??_F_Timevec@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor, 4)
_Timevec* __thiscall _Timevec_ctor(_Timevec *this)
{
    TRACE("(%p)\n", this);
    this->timeptr = NULL;
    return this;
}

/* ??1_Timevec@std@@QAE@XZ */
/* ??1_Timevec@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Timevec_dtor, 4)
void __thiscall _Timevec_dtor(_Timevec *this)
{
    TRACE("(%p)\n", this);
    free(this->timeptr);
}

/* ??4_Timevec@std@@QAEAAV01@ABV01@@Z */
/* ??4_Timevec@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_op_assign, 8)
_Timevec* __thiscall _Timevec_op_assign(_Timevec *this, _Timevec *right)
{
    TRACE("(%p %p)\n", this, right);
    this->timeptr = right->timeptr;
    right->timeptr = NULL;
    return this;
}

/* ?_Getptr@_Timevec@std@@QBEPAXXZ */
/* ?_Getptr@_Timevec@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec__Getptr, 4)
void* __thiscall _Timevec__Getptr(_Timevec *this)
{
    TRACE("(%p)\n", this);
    return this->timeptr;
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@HPBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cat_cstr(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    /* This function is probably modifying more global objects */
    FIXME("(%p %d %s) semi-stub\n", locinfo, category, locstr);

    if(!locstr)
        throw_exception(EXCEPTION_RUNTIME_ERROR, "bad locale name");

    _Lockit_ctor_locktype(&locinfo->lock, _LOCK_LOCALE);
    MSVCP_basic_string_char_ctor_cstr(&locinfo->days, "");
    MSVCP_basic_string_char_ctor_cstr(&locinfo->months, "");
    MSVCP_basic_string_char_ctor_cstr(&locinfo->oldlocname, setlocale(LC_ALL, NULL));

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        MSVCP_basic_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        MSVCP_basic_string_char_ctor_cstr(&locinfo->newlocname, "*");
    _Lockit_dtor(&locinfo->lock);

    return locinfo;
}

/* ??0_Locinfo@std@@QAE@HPBD@Z */
/* ??0_Locinfo@std@@QEAA@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cat_cstr, 12)
_Locinfo* __thiscall _Locinfo_ctor_cat_cstr(_Locinfo *this, int category, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, category, locstr);
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_bstr(_Locinfo *locinfo, const basic_string_char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(locinfo, 1/*FIXME*/, MSVCP_basic_string_char_c_str(locstr));
}

/* ??0_Locinfo@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0_Locinfo@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_bstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_bstr(_Locinfo *this, const basic_string_char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, MSVCP_basic_string_char_c_str(locstr));
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@PBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@PEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cstr(_Locinfo *locinfo, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(locinfo, 1/*FIXME*/, locstr);
}

/* ??0_Locinfo@std@@QAE@PBD@Z */
/* ??0_Locinfo@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_cstr(_Locinfo *this, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, locstr);
}

/* ?_Locinfo_dtor@_Locinfo@std@@SAXPAV12@@Z */
/* ?_Locinfo_dtor@_Locinfo@std@@SAXPEAV12@@Z */
void __cdecl _Locinfo__Locinfo_dtor(_Locinfo *locinfo)
{
    TRACE("(%p)\n", locinfo);

    setlocale(LC_ALL, MSVCP_basic_string_char_c_str(&locinfo->oldlocname));
    MSVCP_basic_string_char_dtor(&locinfo->days);
    MSVCP_basic_string_char_dtor(&locinfo->months);
    MSVCP_basic_string_char_dtor(&locinfo->oldlocname);
    MSVCP_basic_string_char_dtor(&locinfo->newlocname);
}

/* ??_F_Locinfo@std@@QAEXXZ */
/* ??_F_Locinfo@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor, 4)
_Locinfo* __thiscall _Locinfo_ctor(_Locinfo *this)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, "C");
}

/* ??1_Locinfo@std@@QAE@XZ */
/* ??1_Locinfo@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_dtor, 4)
void __thiscall _Locinfo_dtor(_Locinfo *this)
{
    _Locinfo__Locinfo_dtor(this);
}

/* ?_Locinfo_Addcats@_Locinfo@std@@SAAAV12@PAV12@HPBD@Z */
/* ?_Locinfo_Addcats@_Locinfo@std@@SAAEAV12@PEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_Addcats(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    /* This function is probably modifying more global objects */
    FIXME("(%p %d %s) semi-stub\n", locinfo, category, locstr);
    if(!locstr)
        throw_exception(EXCEPTION_RUNTIME_ERROR, "bad locale name");

    _Lockit_ctor_locktype(&locinfo->lock, _LOCK_LOCALE);
    MSVCP_basic_string_char_dtor(&locinfo->newlocname);

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        MSVCP_basic_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        MSVCP_basic_string_char_ctor_cstr(&locinfo->newlocname, "*");
    _Lockit_dtor(&locinfo->lock);

    return locinfo;
}

/* ?_Addcats@_Locinfo@std@@QAEAAV12@HPBD@Z */
/* ?_Addcats@_Locinfo@std@@QEAAAEAV12@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo__Addcats, 12)
_Locinfo* __thiscall _Locinfo__Addcats(_Locinfo *this, int category, const char *locstr)
{
    return _Locinfo__Locinfo_Addcats(this, category, locstr);
}

/* _Getcoll */
_Collvec __cdecl _Getcoll(void)
{
    _Collvec ret;
    _locale_t locale = _get_current_locale();

    TRACE("\n");

    ret.page = locale->locinfo->lc_collate_cp;
    ret.handle = locale->locinfo->lc_handle[LC_COLLATE];
    _free_locale(locale);
    return ret;
}

/* ?_Getcoll@_Locinfo@std@@QBE?AU_Collvec@@XZ */
/* ?_Getcoll@_Locinfo@std@@QEBA?AU_Collvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcoll, 4)
_Collvec __thiscall _Locinfo__Getcoll(const _Locinfo *this)
{
    return _Getcoll();
}

/* _Getctype */
_Ctypevec __cdecl _Getctype(void)
{
    _Ctypevec ret;
    _locale_t locale = _get_current_locale();

    TRACE("\n");

    ret.page = locale->locinfo->lc_codepage;
    ret.handle = locale->locinfo->lc_handle[LC_COLLATE];
    ret.delfl = TRUE;
    ret.table = malloc(sizeof(short[256]));
    if(!ret.table) {
        _free_locale(locale);
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    memcpy(ret.table, locale->locinfo->pctype, sizeof(short[256]));
    _free_locale(locale);
    return ret;
}

/* ?_Getctype@_Locinfo@std@@QBE?AU_Ctypevec@@XZ */
/* ?_Getctype@_Locinfo@std@@QEBA?AU_Ctypevec@@XZ */
DEFINE_THISCALL_WRAPPER_RETPTR(_Locinfo__Getctype, 4)
_Ctypevec __thiscall _Locinfo__Getctype(const _Locinfo *this)
{
    return _Getctype();
}

/* _Getcvt */
_Cvtvec __cdecl _Getcvt(void)
{
    _Cvtvec ret;
    _locale_t locale = _get_current_locale();

    TRACE("\n");

    ret.page = locale->locinfo->lc_codepage;
    ret.handle = locale->locinfo->lc_handle[LC_CTYPE];
    _free_locale(locale);
    return ret;
}

/* ?_Getcvt@_Locinfo@std@@QBE?AU_Cvtvec@@XZ */
/* ?_Getcvt@_Locinfo@std@@QEBA?AU_Cvtvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcvt, 4)
_Cvtvec __thiscall _Locinfo__Getcvt(const _Locinfo *this)
{
    return _Getcvt();
}

/* ?_Getdateorder@_Locinfo@std@@QBEHXZ */
/* ?_Getdateorder@_Locinfo@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getdateorder, 4)
int __thiscall _Locinfo__Getdateorder(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?_Getdays@_Locinfo@std@@QBEPBDXZ */
/* ?_Getdays@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getdays, 4)
const char* __thiscall _Locinfo__Getdays(_Locinfo *this)
{
    char *days = _Getdays();

    TRACE("(%p)\n", this);

    if(days) {
        MSVCP_basic_string_char_dtor(&this->days);
        MSVCP_basic_string_char_ctor_cstr(&this->days, days);
        free(days);
    }

    return this->days.size ? MSVCP_basic_string_char_c_str(&this->days) :
        ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
}

/* ?_Getmonths@_Locinfo@std@@QBEPBDXZ */
/* ?_Getmonths@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getmonths, 4)
const char* __thiscall _Locinfo__Getmonths(_Locinfo *this)
{
    char *months = _Getmonths();

    TRACE("(%p)\n", this);

    if(months) {
        MSVCP_basic_string_char_dtor(&this->months);
        MSVCP_basic_string_char_ctor_cstr(&this->months, months);
        free(months);
    }

    return this->months.size ? MSVCP_basic_string_char_c_str(&this->months) :
        ":Jan:January:Feb:February:Mar:March:Apr:April:May:May:Jun:June:Jul:July"
        ":Aug:August:Sep:September:Oct:October:Nov:November:Dec:December";
}

/* ?_Getfalse@_Locinfo@std@@QBEPBDXZ */
/* ?_Getfalse@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getfalse, 4)
const char* __thiscall _Locinfo__Getfalse(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return "false";
}

/* ?_Gettrue@_Locinfo@std@@QBEPBDXZ */
/* ?_Gettrue@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettrue, 4)
const char* __thiscall _Locinfo__Gettrue(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return "true";
}

/* ?_Getlconv@_Locinfo@std@@QBEPBUlconv@@XZ */
/* ?_Getlconv@_Locinfo@std@@QEBAPEBUlconv@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getlconv, 4)
const struct lconv* __thiscall _Locinfo__Getlconv(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return localeconv();
}

/* ?_Getname@_Locinfo@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?_Getname@_Locinfo@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER_RETPTR(_Locinfo__Getname, 4)
basic_string_char __thiscall _Locinfo__Getname(const _Locinfo *this)
{
    basic_string_char ret;

    TRACE("(%p)\n", this);

    MSVCP_basic_string_char_copy_ctor(&ret, &this->newlocname);
    return ret;
}

/* ?_Gettnames@_Locinfo@std@@QBE?AV_Timevec@2@XZ */
/* ?_Gettnames@_Locinfo@std@@QEBA?AV_Timevec@2@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettnames, 4)
_Timevec __thiscall _Locinfo__Gettnames(const _Locinfo *this)
{
    _Timevec ret;

    TRACE("(%p)\n", this);

    _Timevec_ctor_timeptr(&ret, _Gettnames());
    return ret;
}

static const type_info locale_facet_type_info = {
    MSVCP_locale_facet_vtable,
    NULL,
    ".?AVfacet@locale@std@@"
};

/* ?id@?$collate@D@std@@2V0locale@2@A */
locale_id collate_char_id = {0};

/* ??_7?$collate@D@std@@6B@ */
extern const vtable_ptr MSVCP_collate_char_vtable;

/* ?_Init@?$collate@D@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@D@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(collate_char__Init, 8)
void __thiscall collate_char__Init(collate *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    this->coll = _Locinfo__Getcoll(locinfo);
}

/* ??0?$collate@D@std@@IAE@PBDI@Z */
/* ??0?$collate@D@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_name, 12)
collate* __thiscall collate_char_ctor_name(collate *this, const char *name, MSVCP_size_t refs)
{
    _Lockit lockit;
    _Locinfo locinfo;

    TRACE("(%p %s %lu)\n", this, name, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_collate_char_vtable;

    _Lockit_ctor_locktype(&lockit, _LOCK_LOCALE);
    _Locinfo_ctor_cstr(&locinfo, name);
    collate_char__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    _Lockit_dtor(&lockit);
    return this;
}

/* ??0?$collate@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_locinfo, 12)
collate* __thiscall collate_char_ctor_locinfo(collate *this, _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_collate_char_vtable;
    collate_char__Init(this, locinfo);
    return this;
}

/* ??0?$collate@D@std@@QAE@I@Z */
/* ??0?$collate@D@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_refs, 8)
collate* __thiscall collate_char_ctor_refs(collate *this, MSVCP_size_t refs)
{
    return collate_char_ctor_name(this, "C", refs);
}

/* ??1?$collate@D@std@@MAE@XZ */
/* ??1?$collate@D@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_char_dtor, 4)
void __thiscall collate_char_dtor(collate *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_collate_char_vector_dtor, 8)
collate* __thiscall MSVCP_collate_char_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            collate_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        collate_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ??_F?$collate@D@std@@QAEXXZ */
/* ??_F?$collate@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_char_ctor, 4)
collate* __thiscall collate_char_ctor(collate *this)
{
    return collate_char_ctor_name(this, "C", 0);
}

/* ?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
MSVCP_size_t __cdecl collate_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(collate));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        collate_char_ctor_name((collate*)*facet,
                MSVCP_basic_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_COLLATE;
}

/* _Strcoll */
int __cdecl _Strcoll(const char *first1, const char *last1, const char *first2,
        const char *last2, const _Collvec *coll)
{
    TRACE("(%s %s)\n", debugstr_an(first1, last1-first1), debugstr_an(first2, last2-first2));
    return CompareStringA(coll->handle, 0, first1, last1-first1, first2, last2-first2)-2;
}

/* ?do_compare@?$collate@D@std@@MBEHPBD000@Z */
/* ?do_compare@?$collate@D@std@@MEBAHPEBD000@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_compare, 20)
#define call_collate_char_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 4, int, \
        (const collate*, const char*, const char*, const char*, const char*), \
        (this, first1, last1, first2, last2))
int __thiscall collate_char_do_compare(const collate *this, const char *first1,
        const char *last1, const char *first2, const char *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return _Strcoll(first1, last1, first2, last2, &this->coll);
}

/* ?compare@?$collate@D@std@@QBEHPBD000@Z */
/* ?compare@?$collate@D@std@@QEBAHPEBD000@Z */
DEFINE_THISCALL_WRAPPER(collate_char_compare, 20)
int __thiscall collate_char_compare(const collate *this, const char *first1,
        const char *last1, const char *first2, const char *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return call_collate_char_do_compare(this, first1, last1, first2, last2);
}

/* ?do_hash@?$collate@D@std@@MBEJPBD0@Z */
/* ?do_hash@?$collate@D@std@@MEBAJPEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_hash, 12)
LONG __thiscall collate_char_do_hash(const collate *this,
        const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return 0;
}

/* ?hash@?$collate@D@std@@QBEJPBD0@Z */
/* ?hash@?$collate@D@std@@QEBAJPEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_hash, 12)
LONG __thiscall collate_char_hash(const collate *this,
        const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return 0;
}

/* ?do_transform@?$collate@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?do_transform@?$collate@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER_RETPTR(collate_char_do_transform, 12)
basic_string_char __thiscall collate_char_do_transform(const collate *this,
        const char *first, const char *last)
{
    basic_string_char ret = {0}; /* FIXME */
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?transform@?$collate@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER_RETPTR(collate_char_transform, 12)
basic_string_char __thiscall collate_char_transform(const collate *this,
        const char *first, const char *last)
{
    basic_string_char ret = {0}; /* FIXME */
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?id@?$collate@_W@std@@2V0locale@2@A */
locale_id collate_wchar_id = {0};
/* ?id@?$collate@G@std@@2V0locale@2@A */
locale_id collate_short_id = {0};

/* ??_7?$collate@_W@std@@6B@ */
extern const vtable_ptr MSVCP_collate_wchar_vtable;
/* ??_7?$collate@G@std@@6B@ */
extern const vtable_ptr MSVCP_collate_short_vtable;

/* ??0?$collate@_W@std@@IAE@PBDI@Z */
/* ??0?$collate@_W@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_name, 12)
collate* __thiscall collate_wchar_ctor_name(collate *this, const char *name, MSVCP_size_t refs)
{
    FIXME("(%p %s %lu) stub\n", this, name, refs);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;
    return NULL;
}

/* ??0?$collate@G@std@@IAE@PBDI@Z */
/* ??0?$collate@G@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_name, 12)
collate* __thiscall collate_short_ctor_name(collate *this, const char *name, MSVCP_size_t refs)
{
    collate *ret = collate_wchar_ctor_name(this, name, refs);
    ret->facet.vtable = &MSVCP_collate_short_vtable;
    return ret;
}

/* ??0?$collate@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_locinfo, 12)
collate* __thiscall collate_wchar_ctor_locinfo(collate *this, _Locinfo *locinfo, MSVCP_size_t refs)
{
    FIXME("(%p %p %lu) stub\n", this, locinfo, refs);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;
    return NULL;
}

/* ??0?$collate@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_locinfo, 12)
collate* __thiscall collate_short_ctor_locinfo(collate *this, _Locinfo *locinfo, MSVCP_size_t refs)
{
    collate *ret = collate_wchar_ctor_locinfo(this, locinfo, refs);
    ret->facet.vtable = &MSVCP_collate_short_vtable;
    return ret;
}

/* ??0?$collate@_W@std@@QAE@I@Z */
/* ??0?$collate@_W@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_refs, 8)
collate* __thiscall collate_wchar_ctor_refs(collate *this, MSVCP_size_t refs)
{
    FIXME("(%p %lu) stub\n", this, refs);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;
    return NULL;
}

/* ??0?$collate@G@std@@QAE@I@Z */
/* ??0?$collate@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_refs, 8)
collate* __thiscall collate_short_ctor_refs(collate *this, MSVCP_size_t refs)
{
    collate *ret = collate_wchar_ctor_refs(this, refs);
    ret->facet.vtable = &MSVCP_collate_short_vtable;
    return ret;
}

/* ??1?$collate@_W@std@@MAE@XZ */
/* ??1?$collate@_W@std@@MEAA@XZ */
/* ??1?$collate@G@std@@MAE@XZ */
/* ??1?$collate@G@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_wchar_dtor, 4)
void __thiscall collate_wchar_dtor(collate *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_collate_wchar_vector_dtor, 8)
collate* __thiscall MSVCP_collate_wchar_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            collate_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        collate_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_collate_short_vector_dtor, 8)
collate* __thiscall MSVCP_collate_short_vector_dtor(collate *this, unsigned int flags)
{
    return MSVCP_collate_wchar_vector_dtor(this, flags);
}

/* ??_F?$collate@_W@std@@QAEXXZ */
/* ??_F?$collate@_W@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor, 4)
collate* __thiscall collate_wchar_ctor(collate *this)
{
    FIXME("(%p) stub\n", this);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;
    return NULL;
}

/* ??_F?$collate@G@std@@QAEXXZ */
/* ??_F?$collate@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_short_ctor, 4)
collate* __thiscall collate_short_ctor(collate *this)
{
    collate *ret = collate_wchar_ctor(this);
    ret->facet.vtable = &MSVCP_collate_short_vtable;
    return ret;
}

/* ?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
MSVCP_size_t __cdecl collate_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

/* ?_Init@?$collate@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar__Init, 8)
void __thiscall collate_wchar__Init(collate *this, const _Locinfo *locinfo)
{
    FIXME("(%p %p) stub\n", this, locinfo);
}

/* ?do_compare@?$collate@_W@std@@MBEHPB_W000@Z */
/* ?do_compare@?$collate@_W@std@@MEBAHPEB_W000@Z */
/* ?do_compare@?$collate@G@std@@MBEHPBG000@Z */
/* ?do_compare@?$collate@G@std@@MEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_compare, 20)
int __thiscall collate_wchar_do_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    FIXME("(%p %p %p %p %p) stub\n", this, first1, last1, first2, last2);
    return 0;
}

/* ?compare@?$collate@_W@std@@QBEHPB_W000@Z */
/* ?compare@?$collate@_W@std@@QEBAHPEB_W000@Z */
/* ?compare@?$collate@G@std@@QBEHPBG000@Z */
/* ?compare@?$collate@G@std@@QEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_compare, 20)
int __thiscall collate_wchar_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    FIXME("(%p %p %p %p %p) stub\n", this, first1, last1, first2, last2);
    return 0;
}

/* ?do_hash@?$collate@_W@std@@MBEJPB_W0@Z */
/* ?do_hash@?$collate@_W@std@@MEBAJPEB_W0@Z */
/* ?do_hash@?$collate@G@std@@MBEJPBG0@Z */
/* ?do_hash@?$collate@G@std@@MEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_hash, 12)
LONG __thiscall collate_wchar_do_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return 0;
}

/* ?hash@?$collate@_W@std@@QBEJPB_W0@Z */
/* ?hash@?$collate@_W@std@@QEBAJPEB_W0@Z */
/* ?hash@?$collate@G@std@@QBEJPBG0@Z */
/* ?hash@?$collate@G@std@@QEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_hash, 12)
LONG __thiscall collate_wchar_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return 0;
}

/* ?do_transform@?$collate@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?do_transform@?$collate@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?do_transform@?$collate@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?do_transform@?$collate@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER_RETPTR(collate_wchar_do_transform, 12)
basic_string_wchar __thiscall collate_wchar_do_transform(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    basic_string_wchar ret = {0}; /* FIXME */
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?transform@?$collate@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?transform@?$collate@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?transform@?$collate@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER_RETPTR(collate_wchar_transform, 12)
basic_string_wchar __thiscall collate_wchar_transform(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    basic_string_wchar ret = {0}; /* FIXME */
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ??_7ctype_base@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_base_vtable;

/* ??0ctype_base@std@@QAE@I@Z */
/* ??0ctype_base@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_base_ctor_refs, 8)
ctype_base* __thiscall ctype_base_ctor_refs(ctype_base *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_ctype_base_vtable;
    return this;
}

/* ??_Fctype_base@std@@QAEXXZ */
/* ??_Fctype_base@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_base_ctor, 4)
ctype_base* __thiscall ctype_base_ctor(ctype_base *this)
{
    TRACE("(%p)\n", this);
    locale_facet_ctor_refs(&this->facet, 0);
    this->facet.vtable = &MSVCP_ctype_base_vtable;
    return this;
}

/* ??1ctype_base@std@@UAE@XZ */
/* ??1ctype_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_base_dtor, 4)
void __thiscall ctype_base_dtor(ctype_base *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_ctype_base_vector_dtor, 8)
ctype_base* __thiscall MSVCP_ctype_base_vector_dtor(ctype_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_base_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        ctype_base_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Xran@ctype_base@std@@KAXXZ */
void __cdecl ctype_base__Xran(void)
{
    throw_exception(EXCEPTION_OUT_OF_RANGE, "out of range in ctype<T>");
}

/* ?id@?$ctype@D@std@@2V0locale@2@A */
locale_id ctype_char_id = {0};
/* ?table_size@?$ctype@D@std@@2IB */
/* ?table_size@?$ctype@D@std@@2_KB */
MSVCP_size_t ctype_char_table_size = 256;

/* ??_7?$ctype@D@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_char_vtable;

/* ?_Id_func@?$ctype@D@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@D@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_char__Id_func(void)
{
    FIXME("() stub\n");
    return NULL;
}

/* ?_Init@?$ctype@D@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@D@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Init, 8)
void __thiscall ctype_char__Init(ctype_char *this, _Locinfo *locinfo)
{
    FIXME("(%p %p) stub\n", this, locinfo);
}

/* ?_Tidy@?$ctype@D@std@@IAEXXZ */
/* ?_Tidy@?$ctype@D@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_char__Tidy, 4)
void __thiscall ctype_char__Tidy(ctype_char *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?classic_table@?$ctype@D@std@@KAPBFXZ */
/* ?classic_table@?$ctype@D@std@@KAPEBFXZ */
const short* __cdecl ctype_char_classic_table(void)
{
    FIXME("() stub\n");
    return NULL;
}

/* ??0?$ctype@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_locinfo, 12)
ctype_char* __thiscall ctype_char_ctor_locinfo(ctype_char *this,
        _Locinfo *locinfo, MSVCP_size_t refs)
{
    FIXME("(%p %p %lu) stub\n", this, locinfo, refs);
    this->base.facet.vtable = &MSVCP_ctype_char_vtable;
    return NULL;
}

/* ??0?$ctype@D@std@@QAE@PBF_NI@Z */
/* ??0?$ctype@D@std@@QEAA@PEBF_N_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_table, 16)
ctype_char* __thiscall ctype_char_ctor_table(ctype_char *this,
        const short *table, MSVCP_bool delete, MSVCP_size_t refs)
{
    FIXME("(%p %p %d %lu) stub\n", this, table, delete, refs);
    this->base.facet.vtable = &MSVCP_ctype_char_vtable;
    return NULL;
}

/* ??_F?$ctype@D@std@@QAEXXZ */
/* ??_F?$ctype@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor, 4)
ctype_char* __thiscall ctype_char_ctor(ctype_char *this)
{
    FIXME("(%p) stub\n", this);
    this->base.facet.vtable = &MSVCP_ctype_char_vtable;
    return NULL;
}

/* ??1?$ctype@D@std@@MAE@XZ */
/* ??1?$ctype@D@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_char_dtor, 4)
void __thiscall ctype_char_dtor(ctype_char *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_ctype_char_vector_dtor, 8)
ctype_char* __thiscall MSVCP_ctype_char_vector_dtor(ctype_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        ctype_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?do_narrow@?$ctype@D@std@@MBEDDD@Z */
/* ?do_narrow@?$ctype@D@std@@MEBADDD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_narrow_ch, 12)
char __thiscall ctype_char_do_narrow_ch(const ctype_char *this, char ch, char unused)
{
    FIXME("(%p %c %c) stub\n", this, ch, unused);
    return 0;
}

/* ?do_narrow@?$ctype@D@std@@MBEPBDPBD0DPAD@Z */
/* ?do_narrow@?$ctype@D@std@@MEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_narrow, 20)
const char* __thiscall ctype_char_do_narrow(const ctype_char *this,
        const char *first, const char *last, char unused, char *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?_Do_narrow_s@?$ctype@D@std@@MBEPBDPBD0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@D@std@@MEBAPEBDPEBD0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Do_narrow_s, 24)
const char* __thiscall ctype_char__Do_narrow_s(const ctype_char *this, const char *first,
        const char *last, char unused, char *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?narrow@?$ctype@D@std@@QBEDDD@Z */
/* ?narrow@?$ctype@D@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow_ch, 12)
char __thiscall ctype_char_narrow_ch(const ctype_char *this, char ch, char dflt)
{
    FIXME("(%p %c %c) stub\n", this, ch, dflt);
    return 0;
}

/* ?narrow@?$ctype@D@std@@QBEPBDPBD0DPAD@Z */
/* ?narrow@?$ctype@D@std@@QEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow, 20)
const char* __thiscall ctype_char_narrow(const ctype_char *this,
        const char *first, const char *last, char dflt, char *dest)
{
    FIXME("(%p %p %p %c %p) stub\n", this, first, last, dflt, dest);
    return NULL;
}

/* ?_Narrow_s@?$ctype@D@std@@QBEPBDPBD0DPADI@Z */
/* ?_Narrow_s@?$ctype@D@std@@QEBAPEBDPEBD0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Narrow_s, 24)
const char* __thiscall ctype_char__Narrow_s(const ctype_char *this, const char *first,
        const char *last, char unused, char *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?do_widen@?$ctype@D@std@@MBEDD@Z */
/* ?do_widen@?$ctype@D@std@@MEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen_ch, 8)
char __thiscall ctype_char_do_widen_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?do_widen@?$ctype@D@std@@MBEPBDPBD0PAD@Z */
/* ?do_widen@?$ctype@D@std@@MEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen, 16)
const char* __thiscall ctype_char_do_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?_Do_widen_s@?$ctype@D@std@@MBEPBDPBD0PADI@Z */
/* ?_Do_widen_s@?$ctype@D@std@@MEBAPEBDPEBD0PEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Do_widen_s, 20)
const char* __thiscall ctype_char__Do_widen_s(const ctype_char *this,
        const char *first, const char *last, char *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?widen@?$ctype@D@std@@QBEDD@Z */
/* ?widen@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen_ch, 8)
char __thiscall ctype_char_widen_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?widen@?$ctype@D@std@@QBEPBDPBD0PAD@Z */
/* ?widen@?$ctype@D@std@@QEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen, 16)
const char* __thiscall ctype_char_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?_Widen_s@?$ctype@D@std@@QBEPBDPBD0PADI@Z */
/* ?_Widen_s@?$ctype@D@std@@QEBAPEBDPEBD0PEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Widen_s, 20)
const char* __thiscall ctype_char__Widen_s(const ctype_char *this,
        const char *first, const char *last, char *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
MSVCP_size_t __cdecl ctype_char__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

/* ?do_tolower@?$ctype@D@std@@MBEDD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower_ch, 8)
char __thiscall ctype_char_do_tolower_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?do_tolower@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower, 12)
const char* __thiscall ctype_char_do_tolower(const ctype_char *this, char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?tolower@?$ctype@D@std@@QBEDD@Z */
/* ?tolower@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower_ch, 8)
char __thiscall ctype_char_tolower_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?tolower@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?tolower@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower, 12)
const char* __thiscall ctype_char_tolower(const ctype_char *this, char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?do_toupper@?$ctype@D@std@@MBEDD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper_ch, 8)
char __thiscall ctype_char_do_toupper_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?do_toupper@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper, 12)
const char* __thiscall ctype_char_do_toupper(const ctype_char *this,
        char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?toupper@?$ctype@D@std@@QBEDD@Z */
/* ?toupper@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper_ch, 8)
char __thiscall ctype_char_toupper_ch(const ctype_char *this, char ch)
{
    FIXME("(%p %c) stub\n", this, ch);
    return 0;
}

/* ?toupper@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?toupper@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper, 12)
const char* __thiscall ctype_char_toupper(const ctype_char *this, char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?is@?$ctype@D@std@@QBE_NFD@Z */
/* ?is@?$ctype@D@std@@QEBA_NFD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is_ch, 12)
MSVCP_bool __thiscall ctype_char_is_ch(const ctype_char *this, short mask, char ch)
{
    FIXME("(%p %x %c) stub\n", this, mask, ch);
    return 0;
}

/* ?is@?$ctype@D@std@@QBEPBDPBD0PAF@Z */
/* ?is@?$ctype@D@std@@QEBAPEBDPEBD0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is, 16)
const char* __thiscall ctype_char_is(const ctype_char *this, const char *first, const char *last, short *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?scan_is@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_is@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_is, 16)
const char* __thiscall ctype_char_scan_is(const ctype_char *this, short mask, const char *first, const char *last)
{
    FIXME("(%p %x %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ?scan_not@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_not@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_not, 16)
const char* __thiscall ctype_char_scan_not(const ctype_char *this, short mask, const char *first, const char *last)
{
    FIXME("(%p %x %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ?table@?$ctype@D@std@@IBEPBFXZ */
/* ?table@?$ctype@D@std@@IEBAPEBFXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_table, 4)
const short* __thiscall ctype_char_table(const ctype_char *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?id@?$ctype@_W@std@@2V0locale@2@A */
locale_id ctype_wchar_id = {0};
/* ?id@?$ctype@G@std@@2V0locale@2@A */
locale_id ctype_short_id = {0};

/* ??_7?$ctype@_W@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_wchar_vtable;
/* ??_7?$ctype@G@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_short_vtable;

/* ?_Id_func@?$ctype@_W@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@_W@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_wchar__Id_func(void)
{
    FIXME("() stub\n");
    return NULL;
}

/* ?_Id_func@?$ctype@G@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@G@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_short__Id_func(void)
{
    FIXME("() stub\n");
    return NULL;
}

/* ?_Init@?$ctype@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Init, 8)
void __thiscall ctype_wchar__Init(ctype_wchar *this, _Locinfo *locinfo)
{
    FIXME("(%p %p) stub\n", this, locinfo);
}

/* ??0?$ctype@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor_locinfo, 12)
ctype_wchar* __thiscall ctype_wchar_ctor_locinfo(ctype_wchar *this,
        _Locinfo *locinfo, MSVCP_size_t refs)
{
    FIXME("(%p %p %lu) stub\n", this, locinfo, refs);
    this->base.facet.vtable = &MSVCP_ctype_wchar_vtable;
    return NULL;
}

/* ??0?$ctype@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_locinfo, 12)
ctype_wchar* __thiscall ctype_short_ctor_locinfo(ctype_wchar *this,
        _Locinfo *locinfo, MSVCP_size_t refs)
{
    ctype_wchar *ret = ctype_wchar_ctor_locinfo(this, locinfo, refs);
    this->base.facet.vtable = &MSVCP_ctype_short_vtable;
    return ret;
}

/* ??0?$ctype@_W@std@@QAE@I@Z */
/* ??0?$ctype@_W@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor_refs, 8)
ctype_wchar* __thiscall ctype_wchar_ctor_refs(ctype_wchar *this, MSVCP_size_t refs)
{
    FIXME("(%p %lu) stub\n", this, refs);
    this->base.facet.vtable = &MSVCP_ctype_wchar_vtable;
    return NULL;
}

/* ??0?$ctype@G@std@@QAE@I@Z */
/* ??0?$ctype@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_refs, 8)
ctype_wchar* __thiscall ctype_short_ctor_refs(ctype_wchar *this, MSVCP_size_t refs)
{
    ctype_wchar *ret = ctype_wchar_ctor_refs(this, refs);
    this->base.facet.vtable = &MSVCP_ctype_short_vtable;
    return ret;
}

/* ??0?$ctype@G@std@@IAE@PBDI@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_name, 12)
ctype_wchar* __thiscall ctype_short_ctor_name(ctype_wchar *this,
    const char *name, MSVCP_size_t refs)
{
    FIXME("(%p %s %lu) stub\n", this, debugstr_a(name), refs);
    this->base.facet.vtable = &MSVCP_ctype_short_vtable;
    return NULL;
}

/* ??_F?$ctype@_W@std@@QAEXXZ */
/* ??_F?$ctype@_W@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor, 4)
ctype_wchar* __thiscall ctype_wchar_ctor(ctype_wchar *this)
{
    FIXME("(%p) stub\n", this);
    this->base.facet.vtable = &MSVCP_ctype_wchar_vtable;
    return NULL;
}

/* ??_F?$ctype@G@std@@QAEXXZ */
/* ??_F?$ctype@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor, 4)
ctype_wchar* __thiscall ctype_short_ctor(ctype_wchar *this)
{
    ctype_wchar *ret = ctype_wchar_ctor(this);
    this->base.facet.vtable = &MSVCP_ctype_short_vtable;
    return ret;
}

/* ??1?$ctype@_W@std@@MAE@XZ */
/* ??1?$ctype@_W@std@@MEAA@XZ */
/* ??1?$ctype@G@std@@MAE@XZ */
/* ??1?$ctype@G@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_wchar_dtor, 4)
void __thiscall ctype_wchar_dtor(ctype_wchar *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_ctype_wchar_vector_dtor, 8)
ctype_wchar* __thiscall MSVCP_ctype_wchar_vector_dtor(ctype_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        ctype_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_ctype_short_vector_dtor, 8)
ctype_wchar* __thiscall MSVCP_ctype_short_vector_dtor(ctype_wchar *this, unsigned int flags)
{
    return MSVCP_ctype_wchar_vector_dtor(this, flags);
}

/* ?_Donarrow@?$ctype@_W@std@@IBED_WD@Z */
/* ?_Donarrow@?$ctype@_W@std@@IEBAD_WD@Z */
/* ?_Donarrow@?$ctype@G@std@@IBEDGD@Z */
/* ?_Donarrow@?$ctype@G@std@@IEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Donarrow, 12)
char __thiscall ctype_wchar__Donarrow(const ctype_wchar *this, wchar_t ch, char dflt)
{
    FIXME("(%p %d %d) stub\n", this, ch, dflt);
    return 0;
}

/* ?do_narrow@?$ctype@_W@std@@MBED_WD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAD_WD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEDGD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow_ch, 12)
wchar_t __thiscall ctype_wchar_do_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    FIXME("(%p %d %d) stub\n", this, ch, dflt);
    return 0;
}

/* ?do_narrow@?$ctype@_W@std@@MBEPB_WPB_W0DPAD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAPEB_WPEB_W0DPEAD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEPBGPBG0DPAD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow, 20)
const wchar_t* __thiscall ctype_wchar_do_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    FIXME("(%p %p %p %d %p) stub\n", this, first, last, dflt, dest);
    return NULL;
}

/* ?_Do_narrow_s@?$ctype@_W@std@@MBEPB_WPB_W0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@_W@std@@MEBAPEB_WPEB_W0DPEAD_K@Z */
/* ?_Do_narrow_s@?$ctype@G@std@@MBEPBGPBG0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@G@std@@MEBAPEBGPEBG0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Do_narrow_s, 24)
const wchar_t* __thiscall ctype_wchar__Do_narrow_s(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %d %p %lu) stub\n", this, first, last, dflt, dest, size);
    return NULL;
}

/* ?narrow@?$ctype@_W@std@@QBED_WD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAD_WD@Z */
/* ?narrow@?$ctype@G@std@@QBEDGD@Z */
/* ?narrow@?$ctype@G@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow_ch, 12)
char __thiscall ctype_wchar_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    FIXME("(%p %d %d) stub\n", this, ch, dflt);
    return 0;
}

/* ?narrow@?$ctype@_W@std@@QBEPB_WPB_W0DPAD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAPEB_WPEB_W0DPEAD@Z */
/* ?narrow@?$ctype@G@std@@QBEPBGPBG0DPAD@Z */
/* ?narrow@?$ctype@G@std@@QEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow, 20)
const wchar_t* __thiscall ctype_wchar_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    FIXME("(%p %p %p %d %p) stub\n", this, first, last, dflt, dest);
    return NULL;
}

/* ?_Narrow_s@?$ctype@_W@std@@QBEPB_WPB_W0DPADI@Z */
/* ?_Narrow_s@?$ctype@_W@std@@QEBAPEB_WPEB_W0DPEAD_K@Z */
/* ?_Narrow_s@?$ctype@G@std@@QBEPBGPBG0DPADI@Z */
/* ?_Narrow_s@?$ctype@G@std@@QEBAPEBGPEBG0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Narrow_s, 24)
const wchar_t* __thiscall ctype_wchar__Narrow_s(const ctype_wchar *this, const wchar_t *first,
        const wchar_t *last, char dflt, char *dest, unsigned int size)
{
    FIXME("(%p %p %p %d %p %d) stub\n", this, first, last, dflt, dest, size);
    return NULL;
}

/* ?_Dowiden@?$ctype@_W@std@@IBE_WD@Z */
/* ?_Dowiden@?$ctype@_W@std@@IEBA_WD@Z */
/* ?_Dowiden@?$ctype@G@std@@IBEGD@Z */
/* ?_Dowiden@?$ctype@G@std@@IEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Dowiden, 8)
wchar_t __thiscall ctype_wchar__Dowiden(const ctype_wchar *this, char ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?do_widen@?$ctype@_W@std@@MBE_WD@Z */
/* ?do_widen@?$ctype@_W@std@@MEBA_WD@Z */
/* ?do_widen@?$ctype@G@std@@MBEGD@Z */
/* ?do_widen@?$ctype@G@std@@MEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen_ch, 8)
wchar_t __thiscall ctype_wchar_do_widen_ch(const ctype_wchar *this, char ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?do_widen@?$ctype@_W@std@@MBEPBDPBD0PA_W@Z */
/* ?do_widen@?$ctype@_W@std@@MEBAPEBDPEBD0PEA_W@Z */
/* ?do_widen@?$ctype@G@std@@MBEPBDPBD0PAG@Z */
/* ?do_widen@?$ctype@G@std@@MEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen, 16)
const char* __thiscall ctype_wchar_do_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?_Do_widen_s@?$ctype@_W@std@@MBEPBDPBD0PA_WI@Z */
/* ?_Do_widen_s@?$ctype@_W@std@@MEBAPEBDPEBD0PEA_W_K@Z */
/* ?_Do_widen_s@?$ctype@G@std@@MBEPBDPBD0PAGI@Z */
/* ?_Do_widen_s@?$ctype@G@std@@MEBAPEBDPEBD0PEAG_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Do_widen_s, 20)
const char* __thiscall ctype_wchar__Do_widen_s(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?widen@?$ctype@_W@std@@QBE_WD@Z */
/* ?widen@?$ctype@_W@std@@QEBA_WD@Z */
/* ?widen@?$ctype@G@std@@QBEGD@Z */
/* ?widen@?$ctype@G@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen_ch, 8)
wchar_t __thiscall ctype_wchar_widen_ch(const ctype_wchar *this, char ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?widen@?$ctype@_W@std@@QBEPBDPBD0PA_W@Z */
/* ?widen@?$ctype@_W@std@@QEBAPEBDPEBD0PEA_W@Z */
/* ?widen@?$ctype@G@std@@QBEPBDPBD0PAG@Z */
/* ?widen@?$ctype@G@std@@QEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen, 16)
const char* __thiscall ctype_wchar_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?_Widen_s@?$ctype@_W@std@@QBEPBDPBD0PA_WI@Z */
/* ?_Widen_s@?$ctype@_W@std@@QEBAPEBDPEBD0PEA_W_K@Z */
/* ?_Widen_s@?$ctype@G@std@@QBEPBDPBD0PAGI@Z */
/* ?_Widen_s@?$ctype@G@std@@QEBAPEBDPEBD0PEAG_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Widen_s, 20)
const char* __thiscall ctype_wchar__Widen_s(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest, MSVCP_size_t size)
{
    FIXME("(%p %p %p %p %lu) stub\n", this, first, last, dest, size);
    return NULL;
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
MSVCP_size_t __cdecl ctype_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

/* ?do_tolower@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEGG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower_ch, 8)
char __thiscall ctype_wchar_do_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?do_tolower@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower, 12)
const wchar_t* __thiscall ctype_wchar_do_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?tolower@?$ctype@_W@std@@QBE_W_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?tolower@?$ctype@G@std@@QBEGG@Z */
/* ?tolower@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower_ch, 8)
wchar_t __thiscall ctype_wchar_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?tolower@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?tolower@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?tolower@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower, 12)
const wchar_t* __thiscall ctype_wchar_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?do_toupper@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEGG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper_ch, 8)
wchar_t __thiscall ctype_wchar_do_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?do_toupper@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper, 12)
const wchar_t* __thiscall ctype_wchar_do_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?toupper@?$ctype@_W@std@@QBE_W_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?toupper@?$ctype@G@std@@QBEGG@Z */
/* ?toupper@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper_ch, 8)
wchar_t __thiscall ctype_wchar_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    FIXME("(%p %d) stub\n", this, ch);
    return 0;
}

/* ?toupper@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?toupper@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?toupper@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper, 12)
const wchar_t* __thiscall ctype_wchar_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return NULL;
}

/* ?do_is@?$ctype@_W@std@@MBE_NF_W@Z */
/* ?do_is@?$ctype@_W@std@@MEBA_NF_W@Z */
/* ?do_is@?$ctype@G@std@@MBE_NFG@Z */
/* ?do_is@?$ctype@G@std@@MEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is_ch, 12)
MSVCP_bool __thiscall ctype_wchar_do_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    FIXME("(%p %x %d) stub\n", this, mask, ch);
    return 0;
}

/* ?do_is@?$ctype@_W@std@@MBEPB_WPB_W0PAF@Z */
/* ?do_is@?$ctype@_W@std@@MEBAPEB_WPEB_W0PEAF@Z */
/* ?do_is@?$ctype@G@std@@MBEPBGPBG0PAF@Z */
/* ?do_is@?$ctype@G@std@@MEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is, 16)
const wchar_t* __thiscall ctype_wchar_do_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?is@?$ctype@_W@std@@QBE_NF_W@Z */
/* ?is@?$ctype@_W@std@@QEBA_NF_W@Z */
/* ?is@?$ctype@G@std@@QBE_NFG@Z */
/* ?is@?$ctype@G@std@@QEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is_ch, 12)
MSVCP_bool __thiscall ctype_wchar_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    FIXME("(%p %x %d) stub\n", this, mask, ch);
    return 0;
}

/* ?is@?$ctype@_W@std@@QBEPB_WPB_W0PAF@Z */
/* ?is@?$ctype@_W@std@@QEBAPEB_WPEB_W0PEAF@Z */
/* ?is@?$ctype@G@std@@QBEPBGPBG0PAF@Z */
/* ?is@?$ctype@G@std@@QEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is, 16)
const wchar_t* __thiscall ctype_wchar_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    FIXME("(%p %p %p %p) stub\n", this, first, last, dest);
    return NULL;
}

/* ?do_scan_is@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_is@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_is@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_is@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_is, 16)
const wchar_t* __thiscall ctype_wchar_do_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %d %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ?scan_is@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_is@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_is@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_is@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_is, 16)
const wchar_t* __thiscall ctype_wchar_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %x %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ?do_scan_not@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_not@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_not@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_not@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_not, 16)
const wchar_t* __thiscall ctype_wchar_do_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %x %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ?scan_not@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_not@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_not@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_not@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_not, 16)
const wchar_t* __thiscall ctype_wchar_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %x %p %p) stub\n", this, mask, first, last);
    return NULL;
}

/* ??0_Locimp@locale@std@@AAE@_N@Z */
/* ??0_Locimp@locale@std@@AEAA@_N@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor_transparent, 8)
locale__Locimp* __thiscall locale__Locimp_ctor_transparent(locale__Locimp *this, MSVCP_bool transparent)
{
    TRACE("(%p %d)\n", this, transparent);

    memset(this, 0, sizeof(locale__Locimp));
    locale_facet_ctor_refs(&this->facet, 1);
    this->transparent = transparent;
    MSVCP_basic_string_char_ctor_cstr(&this->name, "*");
    return this;
}

/* ??_F_Locimp@locale@std@@QAEXXZ */
/* ??_F_Locimp@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor, 4)
locale__Locimp* __thiscall locale__Locimp_ctor(locale__Locimp *this)
{
    return locale__Locimp_ctor_transparent(this, FALSE);
}

/* ??0_Locimp@locale@std@@AAE@ABV012@@Z */
/* ??0_Locimp@locale@std@@AEAA@AEBV012@@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_copy_ctor, 8)
locale__Locimp* __thiscall locale__Locimp_copy_ctor(locale__Locimp *this, const locale__Locimp *copy)
{
    _Lockit lock;
    MSVCP_size_t i;

    TRACE("(%p %p)\n", this, copy);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    memcpy(this, copy, sizeof(locale__Locimp));
    locale_facet_ctor_refs(&this->facet, 1);
    if(copy->facetvec) {
        this->facetvec = MSVCRT_operator_new(copy->facet_cnt*sizeof(locale_facet*));
        if(!this->facetvec) {
            _Lockit_dtor(&lock);
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return NULL;
        }
        for(i=0; i<this->facet_cnt; i++)
            if(this->facetvec[i])
                locale_facet__Incref(this->facetvec[i]);
    }
    MSVCP_basic_string_char_copy_ctor(&this->name, &copy->name);
    _Lockit_dtor(&lock);
    return this;
}

/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPAV123@ABV123@@Z */
/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPEAV123@AEBV123@@Z */
locale__Locimp* __cdecl locale__Locimp__Locimp_ctor(locale__Locimp *this, const locale__Locimp *copy)
{
    return locale__Locimp_copy_ctor(this, copy);
}

/* ??1_Locimp@locale@std@@MAE@XZ */
/* ??1_Locimp@locale@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale__Locimp_dtor, 4)
void __thiscall locale__Locimp_dtor(locale__Locimp *this)
{
    TRACE("(%p)\n", this);

    if(locale_facet__Decref(&this->facet)) {
        MSVCP_size_t i;
        for(i=0; i<this->facet_cnt; i++)
            if(this->facetvec[i] && locale_facet__Decref(this->facetvec[i]))
                call_locale_facet_vector_dtor(this->facetvec[i], 0);

        MSVCRT_operator_delete(this->facetvec);
        MSVCP_basic_string_char_dtor(&this->name);
    }
}

/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPAV123@@Z */
/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPEAV123@@Z */
void __cdecl locale__Locimp__Locimp_dtor(locale__Locimp *this)
{
    locale__Locimp_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale__Locimp_vector_dtor, 8)
locale__Locimp* __thiscall MSVCP_locale__Locimp_vector_dtor(locale__Locimp *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            locale__Locimp_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        locale__Locimp_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Locimp_Addfac@_Locimp@locale@std@@CAXPAV123@PAVfacet@23@I@Z */
/* ?_Locimp_Addfac@_Locimp@locale@std@@CAXPEAV123@PEAVfacet@23@_K@Z */
void __cdecl locale__Locimp__Locimp_Addfac(locale__Locimp *locimp, locale_facet *facet, MSVCP_size_t id)
{
    _Lockit lock;

    TRACE("(%p %p %lu)\n", locimp, facet, id);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(id >= locimp->facet_cnt) {
        MSVCP_size_t new_size = id+1;
        locale_facet **new_facetvec;

        if(new_size < locale_id__Id_cnt+1)
            new_size = locale_id__Id_cnt+1;

        new_facetvec = MSVCRT_operator_new(sizeof(locale_facet*)*new_size);
        if(!new_facetvec) {
            _Lockit_dtor(&lock);
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return;
        }

        memset(new_facetvec, 0, sizeof(locale_facet*)*new_size);
        memcpy(new_facetvec, locimp->facetvec, sizeof(locale_facet*)*locimp->facet_cnt);
        MSVCRT_operator_delete(locimp->facetvec);
        locimp->facetvec = new_facetvec;
        locimp->facet_cnt = new_size;
    }

    if(locimp->facetvec[id] && locale_facet__Decref(locimp->facetvec[id]))
        call_locale_facet_vector_dtor(locimp->facetvec[id], 0);

    locimp->facetvec[id] = facet;
    if(facet)
        locale_facet__Incref(facet);
    _Lockit_dtor(&lock);
}

/* ?_Addfac@_Locimp@locale@std@@AAEXPAVfacet@23@I@Z */
/* ?_Addfac@_Locimp@locale@std@@AEAAXPEAVfacet@23@_K@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp__Addfac, 12)
void __thiscall locale__Locimp__Addfac(locale__Locimp *this, locale_facet *facet, MSVCP_size_t id)
{
    locale__Locimp__Locimp_Addfac(this, facet, id);
}

/* ?_Clocptr_func@_Locimp@locale@std@@CAAAPAV123@XZ */
/* ?_Clocptr_func@_Locimp@locale@std@@CAAEAPEAV123@XZ */
locale__Locimp** __cdecl locale__Locimp__Clocptr_func(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ?_Makeloc@_Locimp@locale@std@@CAPAV123@ABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeloc@_Locimp@locale@std@@CAPEAV123@AEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
locale__Locimp* __cdecl locale__Locimp__Makeloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) stub\n", locinfo, cat, locimp, loc);
    return NULL;
}

/* ?_Makeushloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeushloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
void __cdecl locale__Locimp__Makeushloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) stub\n", locinfo, cat, locimp, loc);
}

/* ?_Makewloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makewloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
void __cdecl locale__Locimp__Makewloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) stub\n", locinfo, cat, locimp, loc);
}

/* ?_Makexloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makexloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
void __cdecl locale__Locimp__Makexloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) stub\n", locinfo, cat, locimp, loc);
}

/* ??_7_Locimp@locale@std@@6B@ */
const vtable_ptr MSVCP_locale__Locimp_vtable[] = {
    (vtable_ptr)THISCALL_NAME(MSVCP_locale__Locimp_vector_dtor)
};

/* ??0locale@std@@AAE@PAV_Locimp@01@@Z */
/* ??0locale@std@@AEAA@PEAV_Locimp@01@@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locimp, 8)
locale* __thiscall locale_ctor_locimp(locale *this, locale__Locimp *locimp)
{
    TRACE("(%p %p)\n", this, locimp);
    /* Don't change locimp reference counter */
    this->ptr = locimp;
    return this;
}

/* ??0locale@std@@QAE@ABV01@0H@Z */
/* ??0locale@std@@QEAA@AEBV01@0H@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locale_locale, 16)
locale* __thiscall locale_ctor_locale_locale(locale *this, const locale *loc, const locale *other, category cat)
{
    FIXME("(%p %p %p %d) stub\n", this, loc, other, cat);
    return NULL;
}

/* ??0locale@std@@QAE@ABV01@@Z */
/* ??0locale@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_copy_ctor, 8)
locale* __thiscall locale_copy_ctor(locale *this, const locale *copy)
{
    TRACE("(%p %p)\n", this, copy);
    this->ptr = copy->ptr;
    locale_facet__Incref(&this->ptr->facet);
    return this;
}

/* ??0locale@std@@QAE@ABV01@PBDH@Z */
/* ??0locale@std@@QEAA@AEBV01@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locale_cstr, 16)
locale* __thiscall locale_ctor_locale_cstr(locale *this, const locale *loc, const char *locname, category cat)
{
    FIXME("(%p %p %s %d) stub\n", this, loc, locname, cat);
    return NULL;
}

/* ??0locale@std@@QAE@PBDH@Z */
/* ??0locale@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_cstr, 12)
locale* __thiscall locale_ctor_cstr(locale *this, const char *locname, category cat)
{
    FIXME("(%p %s %d) stub\n", this, locname, cat);
    return NULL;
}

/* ??0locale@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0locale@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_uninitialized, 8)
locale* __thiscall locale_ctor_uninitialized(locale *this, int uninitialized)
{
    TRACE("(%p)\n", this);
    this->ptr = NULL;
    return this;
}

/* ??0locale@std@@QAE@XZ */
/* ??0locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_ctor, 4)
locale* __thiscall locale_ctor(locale *this)
{
    TRACE("(%p)\n", this);
    this->ptr = MSVCRT_operator_new(sizeof(locale__Locimp));
    if(!this->ptr) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
        return NULL;
    }

    locale__Locimp_ctor(this->ptr);
    return this;
}

/* ??1locale@std@@QAE@XZ */
/* ??1locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_dtor, 4)
void __thiscall locale_dtor(locale *this)
{
    TRACE("(%p)\n", this);
    if(this->ptr)
        locale__Locimp_dtor(this->ptr);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale_vector_dtor, 8)
locale* __thiscall MSVCP_locale_vector_dtor(locale *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)this-1;

        for(i=*ptr-1; i>=0; i--)
            locale_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        locale_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ??4locale@std@@QAEAAV01@ABV01@@Z */
/* ??4locale@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_assign, 8)
locale* __thiscall locale_operator_assign(locale *this, const locale *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return NULL;
}

/* ??8locale@std@@QBE_NABV01@@Z */
/* ??8locale@std@@QEBA_NAEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_equal, 8)
MSVCP_bool __thiscall locale_operator_equal(const locale *this, const locale *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return 0;
}

/* ??9locale@std@@QBE_NABV01@@Z */
/* ??9locale@std@@QEBA_NAEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_not_equal, 8)
MSVCP_bool __thiscall locale_operator_not_equal(const locale *this, locale const *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return 0;
}

/* ?_Addfac@locale@std@@QAEAAV12@PAVfacet@12@II@Z */
/* ?_Addfac@locale@std@@QEAAAEAV12@PEAVfacet@12@_K1@Z */
DEFINE_THISCALL_WRAPPER(locale__Addfac, 16)
locale* __thiscall locale__Addfac(locale *this, locale_facet *facet, MSVCP_size_t id, MSVCP_size_t catmask)
{
    TRACE("(%p %p %lu %lu)\n", this, facet, id, catmask);

    if(this->ptr->facet.refs > 1) {
        locale__Locimp *new_ptr = MSVCRT_operator_new(sizeof(locale__Locimp));
        if(!new_ptr) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return NULL;
        }
        locale__Locimp_copy_ctor(new_ptr, this->ptr);
        locale_facet__Decref(&this->ptr->facet);
        this->ptr = new_ptr;
    }

    locale__Locimp__Addfac(this->ptr, facet, id);

    if(catmask) {
        MSVCP_basic_string_char_dtor(&this->ptr->name);
        MSVCP_basic_string_char_ctor_cstr(&this->ptr->name, "*");
    }
    return this;
}

/* ?_Getfacet@locale@std@@QBEPBVfacet@12@I@Z */
/* ?_Getfacet@locale@std@@QEBAPEBVfacet@12@_K@Z */
DEFINE_THISCALL_WRAPPER(locale__Getfacet, 8)
const locale_facet* __thiscall locale__Getfacet(const locale *this, MSVCP_size_t id)
{
    FIXME("(%p %lu) stub\n", this, id);
    return NULL;
}

/* ?_Init@locale@std@@CAPAV_Locimp@12@XZ */
/* ?_Init@locale@std@@CAPEAV_Locimp@12@XZ */
locale__Locimp* __cdecl locale__Init(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ?_Getgloballocale@locale@std@@CAPAV_Locimp@12@XZ */
/* ?_Getgloballocale@locale@std@@CAPEAV_Locimp@12@XZ */
locale__Locimp* __cdecl locale__Getgloballocale(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ?_Setgloballocale@locale@std@@CAXPAX@Z */
/* ?_Setgloballocale@locale@std@@CAXPEAX@Z */
void __cdecl locale__Setgloballocale(void *locimp)
{
    FIXME("(%p) stub\n", locimp);
}

/* ?classic@locale@std@@SAABV12@XZ */
/* ?classic@locale@std@@SAAEBV12@XZ */
const locale* __cdecl locale_classic(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ?name@locale@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?name@locale@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER_RETPTR(locale_name, 4)
basic_string_char __thiscall locale_name(const locale *this)
{
    TRACE( "(%p)\n", this);
    return this->ptr->name;
}

static const rtti_base_descriptor locale_facet_rtti_base_descriptor = {
    &locale_facet_type_info,
    0,
    { 0, -1, 0},
    64
};

DEFINE_RTTI_DATA(collate_char, 0, 1, &locale_facet_rtti_base_descriptor, NULL, NULL, ".?AV?$collate@D@std@@");
DEFINE_RTTI_DATA(collate_wchar, 0, 1, &locale_facet_rtti_base_descriptor, NULL, NULL, ".?AV?$collate@_W@std@@");
DEFINE_RTTI_DATA(collate_short, 0, 1, &locale_facet_rtti_base_descriptor, NULL, NULL, ".?AV?$collate@G@std@@");
DEFINE_RTTI_DATA(ctype_base, 0, 1, &locale_facet_rtti_base_descriptor, NULL, NULL, ".?AUctype_base@std@@");
DEFINE_RTTI_DATA(ctype_char, 0, 2, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, NULL, ".?AV?$ctype@D@std@@");
DEFINE_RTTI_DATA(ctype_wchar, 0, 2, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, NULL, ".?AV?$ctype@_W@std@@");
DEFINE_RTTI_DATA(ctype_short, 0, 2, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, NULL, ".?AV?$ctype@G@std@@");

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(collate_char,
            VTABLE_ADD_FUNC(collate_char_do_compare)
            VTABLE_ADD_FUNC(collate_char_do_transform)
            VTABLE_ADD_FUNC(collate_char_do_hash));
    __ASM_VTABLE(collate_wchar,
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(collate_short,
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(ctype_base, "");
    __ASM_VTABLE(ctype_char,
            VTABLE_ADD_FUNC(ctype_char_do_tolower)
            VTABLE_ADD_FUNC(ctype_char_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_char_do_toupper)
            VTABLE_ADD_FUNC(ctype_char_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_char_do_widen)
            VTABLE_ADD_FUNC(ctype_char_do_widen_ch)
            VTABLE_ADD_FUNC(ctype_char__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_char_do_narrow)
            VTABLE_ADD_FUNC(ctype_char_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_char__Do_narrow_s));
    __ASM_VTABLE(ctype_wchar,
            VTABLE_ADD_FUNC(ctype_wchar_do_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_is_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_not)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_narrow_s));
    __ASM_VTABLE(ctype_short,
            VTABLE_ADD_FUNC(ctype_wchar_do_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_is_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_not)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_narrow_s));
#ifndef __GNUC__
}
#endif
