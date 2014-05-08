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

#include "msvcp.h"
#include "locale.h"
#include "errno.h"
#include "limits.h"
#include "math.h"
#include "stdio.h"
#include "wctype.h"
#include "time.h"

#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

char* __cdecl _Getdays(void);
char* __cdecl _Getmonths(void);
void* __cdecl _Gettnames(void);
unsigned int __cdecl ___lc_codepage_func(void);
LCID* __cdecl ___lc_handle_func(void);
const locale_facet* __thiscall locale__Getfacet(const locale*, MSVCP_size_t, MSVCP_bool);
MSVCP_size_t __cdecl _Strftime(char*, MSVCP_size_t, const char*,
        const struct tm*, struct __lc_time_data*);

typedef int category;

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
    locale_facet facet;
    _Collvec coll;
} collate;

typedef struct {
    locale_facet facet;
    const char *grouping;
    char dp;
    char sep;
    const char *false_name;
    const char *true_name;
} numpunct_char;

typedef struct {
    locale_facet facet;
    const char *grouping;
    wchar_t dp;
    wchar_t sep;
    const wchar_t *false_name;
    const wchar_t *true_name;
} numpunct_wchar;

typedef struct {
    locale_facet facet;
    _Timevec time;
    _Cvtvec cvt;
} time_put;

/* ?_Id_cnt@id@locale@std@@0HA */
int locale_id__Id_cnt = 0;

static locale classic_locale;

/* ?_Global@_Locimp@locale@std@@0PAV123@A */
/* ?_Global@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *global_locale = NULL;

/* ?_Clocptr@_Locimp@locale@std@@0PAV123@A */
/* ?_Clocptr@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *locale__Locimp__Clocptr = NULL;

static char istreambuf_iterator_char_val(istreambuf_iterator_char *this)
{
    if(this->strbuf && !this->got) {
        int c = basic_streambuf_char_sgetc(this->strbuf);
        if(c == EOF)
            this->strbuf = NULL;
        else
            this->val = c;
    }

    this->got = TRUE;
    return this->val;
}

static wchar_t istreambuf_iterator_wchar_val(istreambuf_iterator_wchar *this)
{
    if(this->strbuf && !this->got) {
        unsigned short c = basic_streambuf_wchar_sgetc(this->strbuf);
        if(c == WEOF)
            this->strbuf = NULL;
        else
            this->val = c;
    }

    this->got = TRUE;
    return this->val;
}

static void istreambuf_iterator_char_inc(istreambuf_iterator_char *this)
{
    if(!this->strbuf || basic_streambuf_char_sbumpc(this->strbuf)==EOF) {
        this->strbuf = NULL;
        this->got = TRUE;
    }else {
        this->got = FALSE;
        istreambuf_iterator_char_val(this);
    }
}

static void istreambuf_iterator_wchar_inc(istreambuf_iterator_wchar *this)
{
    if(!this->strbuf || basic_streambuf_wchar_sbumpc(this->strbuf)==WEOF) {
        this->strbuf = NULL;
        this->got = TRUE;
    }else {
        this->got = FALSE;
        istreambuf_iterator_wchar_val(this);
    }
}

static void ostreambuf_iterator_char_put(ostreambuf_iterator_char *this, char ch)
{
    if(this->failed || basic_streambuf_char_sputc(this->strbuf, ch)==EOF)
        this->failed = TRUE;
}

static void ostreambuf_iterator_wchar_put(ostreambuf_iterator_wchar *this, wchar_t ch)
{
    if(this->failed || basic_streambuf_wchar_sputc(this->strbuf, ch)==WEOF)
        this->failed = TRUE;
}

/* ??1facet@locale@std@@UAE@XZ */
/* ??1facet@locale@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet_dtor, 4)
void __thiscall locale_facet_dtor(locale_facet *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(locale_facet_vector_dtor, 8)
#define call_locale_facet_vector_dtor(this, flags) CALL_VTBL_FUNC(this, 0, \
        locale_facet*, (locale_facet*, unsigned int), (this, flags))
locale_facet* __thiscall locale_facet_vector_dtor(locale_facet *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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

typedef struct
{
    locale_facet *fac;
    struct list entry;
} facets_elem;
static struct list lazy_facets = LIST_INIT(lazy_facets);

/* Not exported from msvcp90 */
/* ?facet_Register@facet@locale@std@@CAXPAV123@@Z */
/* ?facet_Register@facet@locale@std@@CAXPEAV123@@Z */
static void locale_facet_register(locale_facet *add)
{
    facets_elem *head = MSVCRT_operator_new(sizeof(*head));
    if(!head) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }

    head->fac = add;
    list_add_head(&lazy_facets, &head->entry);
}

/* Not exported from msvcp90 */
/* ??_7facet@locale@std@@6B@ */
extern const vtable_ptr MSVCP_locale_facet_vtable;

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

/* ??_Ffacet@locale@std@@QAEXXZ */
/* ??_Ffacet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor, 4)
locale_facet* __thiscall locale_facet_ctor(locale_facet *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &MSVCP_locale_facet_vtable;
    this->refs = 0;
    return this;
}

/* ??0facet@locale@std@@IAE@I@Z */
/* ??0facet@locale@std@@IEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor_refs, 8)
locale_facet* __thiscall locale_facet_ctor_refs(locale_facet *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    this->vtable = &MSVCP_locale_facet_vtable;
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
static _Locinfo* _Locinfo__Locinfo_ctor_cat_cstr(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    /* This function is probably modifying more global objects */
    FIXME("(%p %d %s) semi-stub\n", locinfo, category, locstr);

    if(!locstr)
        throw_exception(EXCEPTION_RUNTIME_ERROR, "bad locale name");

    _Lockit_ctor_locktype(&locinfo->lock, _LOCK_LOCALE);
    basic_string_char_ctor_cstr(&locinfo->days, "");
    basic_string_char_ctor_cstr(&locinfo->months, "");
    basic_string_char_ctor_cstr(&locinfo->oldlocname, setlocale(LC_ALL, NULL));

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        basic_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        basic_string_char_ctor_cstr(&locinfo->newlocname, "*");

    return locinfo;
}

/* ??0_Locinfo@std@@QAE@HPBD@Z */
/* ??0_Locinfo@std@@QEAA@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cat_cstr, 12)
_Locinfo* __thiscall _Locinfo_ctor_cat_cstr(_Locinfo *this, int category, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, category, locstr);
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
static void _Locinfo__Locinfo_dtor(_Locinfo *locinfo)
{
    TRACE("(%p)\n", locinfo);

    setlocale(LC_ALL, basic_string_char_c_str(&locinfo->oldlocname));
    basic_string_char_dtor(&locinfo->days);
    basic_string_char_dtor(&locinfo->months);
    basic_string_char_dtor(&locinfo->oldlocname);
    basic_string_char_dtor(&locinfo->newlocname);
    _Lockit_dtor(&locinfo->lock);
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
static _Locinfo* _Locinfo__Locinfo_Addcats(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    /* This function is probably modifying more global objects */
    FIXME("(%p %d %s) semi-stub\n", locinfo, category, locstr);
    if(!locstr)
        throw_exception(EXCEPTION_RUNTIME_ERROR, "bad locale name");

    basic_string_char_dtor(&locinfo->newlocname);

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        basic_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        basic_string_char_ctor_cstr(&locinfo->newlocname, "*");

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
ULONGLONG __cdecl _Getcoll(void)
{
    union {
        _Collvec collvec;
        ULONGLONG ull;
    } ret;
    _locale_t locale = _get_current_locale();

    TRACE("\n");

    ret.collvec.page = locale->locinfo->lc_collate_cp;
    ret.collvec.handle = locale->locinfo->lc_handle[LC_COLLATE];
    _free_locale(locale);
    return ret.ull;
}

/* ?_Getcoll@_Locinfo@std@@QBE?AU_Collvec@@XZ */
/* ?_Getcoll@_Locinfo@std@@QEBA?AU_Collvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcoll, 8)
_Collvec* __thiscall _Locinfo__Getcoll(const _Locinfo *this, _Collvec *ret)
{
    ULONGLONG ull = _Getcoll();
    memcpy(ret, &ull, sizeof(ull));
    return ret;
}

/* _Getctype */
_Ctypevec* __cdecl _Getctype(_Ctypevec *ret)
{
    _locale_t locale = _get_current_locale();
    short *table;

    TRACE("\n");

    ret->page = locale->locinfo->lc_codepage;
    ret->handle = locale->locinfo->lc_handle[LC_COLLATE];
    ret->delfl = TRUE;
    table = malloc(sizeof(short[256]));
    if(!table) {
        _free_locale(locale);
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    memcpy(table, locale->locinfo->pctype, sizeof(short[256]));
    ret->table = table;
    _free_locale(locale);
    return ret;
}

/* ?_Getctype@_Locinfo@std@@QBE?AU_Ctypevec@@XZ */
/* ?_Getctype@_Locinfo@std@@QEBA?AU_Ctypevec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getctype, 8)
_Ctypevec* __thiscall _Locinfo__Getctype(const _Locinfo *this, _Ctypevec *ret)
{
    return _Getctype(ret);
}

/* _Getcvt */
ULONGLONG __cdecl _Getcvt(void)
{
    _locale_t locale = _get_current_locale();
    union {
        _Cvtvec cvtvec;
        ULONGLONG ull;
    } ret;

    TRACE("\n");

    ret.cvtvec.page = locale->locinfo->lc_codepage;
    ret.cvtvec.handle = locale->locinfo->lc_handle[LC_CTYPE];
    _free_locale(locale);
    return ret.ull;
}

/* ?_Getcvt@_Locinfo@std@@QBE?AU_Cvtvec@@XZ */
/* ?_Getcvt@_Locinfo@std@@QEBA?AU_Cvtvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcvt, 8)
_Cvtvec* __thiscall _Locinfo__Getcvt(const _Locinfo *this, _Cvtvec *ret)
{
    ULONGLONG ull = _Getcvt();
    memcpy(ret, &ull, sizeof(ull));
    return ret;
}

/* ?_Getdays@_Locinfo@std@@QBEPBDXZ */
/* ?_Getdays@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getdays, 4)
const char* __thiscall _Locinfo__Getdays(_Locinfo *this)
{
    char *days = _Getdays();

    TRACE("(%p)\n", this);

    if(days) {
        basic_string_char_dtor(&this->days);
        basic_string_char_ctor_cstr(&this->days, days);
        free(days);
    }

    return this->days.size ? basic_string_char_c_str(&this->days) :
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
        basic_string_char_dtor(&this->months);
        basic_string_char_ctor_cstr(&this->months, months);
        free(months);
    }

    return this->months.size ? basic_string_char_c_str(&this->months) :
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
DEFINE_THISCALL_WRAPPER(_Locinfo__Getname, 8)
basic_string_char* __thiscall _Locinfo__Getname(const _Locinfo *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);

    basic_string_char_copy_ctor(ret, &this->newlocname);
    return ret;
}

/* ?_Gettnames@_Locinfo@std@@QBE?AV_Timevec@2@XZ */
/* ?_Gettnames@_Locinfo@std@@QEBA?AV_Timevec@2@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettnames, 8)
_Timevec*__thiscall _Locinfo__Gettnames(const _Locinfo *this, _Timevec *ret)
{
    TRACE("(%p)\n", this);

    _Timevec_ctor_timeptr(ret, _Gettnames());
    return ret;
}

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
    _Locinfo__Getcoll(locinfo, &this->coll);
}

/* ??0?$collate@D@std@@IAE@PBDI@Z */
/* ??0?$collate@D@std@@IEAA@PEBD_K@Z */
static collate* collate_char_ctor_name(collate *this, const char *name, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %lu)\n", this, name, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_collate_char_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    collate_char__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$collate@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_locinfo, 12)
collate* __thiscall collate_char_ctor_locinfo(collate *this, const _Locinfo *locinfo, MSVCP_size_t refs)
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

/* ??1?$collate@D@std@@UAE@XZ */
/* ??1?$collate@D@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_char_dtor, 4)
void __thiscall collate_char_dtor(collate *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(collate_char_vector_dtor, 8)
collate* __thiscall collate_char_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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
static MSVCP_size_t collate_char__Getcat(const locale_facet **facet, const locale *loc)
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
                basic_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_COLLATE;
}

static collate* collate_char_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_char__Getcat(&fac, loc);
    obj = (collate*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Strcoll */
int __cdecl _Strcoll(const char *first1, const char *last1, const char *first2,
        const char *last2, const _Collvec *coll)
{
    LCID lcid;

    TRACE("(%s %s)\n", debugstr_an(first1, last1-first1), debugstr_an(first2, last2-first2));

    if(coll)
        lcid = coll->handle;
    else
        lcid = ___lc_handle_func()[LC_COLLATE];
    return CompareStringA(lcid, 0, first1, last1-first1, first2, last2-first2)-CSTR_EQUAL;
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
#define call_collate_char_do_hash(this, first, last) CALL_VTBL_FUNC(this, 12, LONG, \
        (const collate*, const char*, const char*), (this, first, last))
LONG __thiscall collate_char_do_hash(const collate *this,
        const char *first, const char *last)
{
    ULONG ret = 0;

    TRACE("(%p %p %p)\n", this, first, last);

    for(; first<last; first++)
        ret = (ret<<8 | ret>>24) + *first;
    return ret;
}

/* ?hash@?$collate@D@std@@QBEJPBD0@Z */
/* ?hash@?$collate@D@std@@QEBAJPEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_hash, 12)
LONG __thiscall collate_char_hash(const collate *this,
        const char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_collate_char_do_hash(this, first, last);
}

/* ?do_transform@?$collate@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?do_transform@?$collate@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_transform, 16)
basic_string_char* __thiscall collate_char_do_transform(const collate *this,
        basic_string_char *ret, const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?transform@?$collate@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_transform, 16)
basic_string_char* __thiscall collate_char_transform(const collate *this,
        basic_string_char *ret, const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?id@?$collate@_W@std@@2V0locale@2@A */
static locale_id collate_wchar_id = {0};
/* ?id@?$collate@G@std@@2V0locale@2@A */
locale_id collate_short_id = {0};

/* ??_7?$collate@_W@std@@6B@ */
extern const vtable_ptr MSVCP_collate_wchar_vtable;
/* ??_7?$collate@G@std@@6B@ */
extern const vtable_ptr MSVCP_collate_short_vtable;

/* ?_Init@?$collate@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar__Init, 8)
void __thiscall collate_wchar__Init(collate *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcoll(locinfo, &this->coll);
}

/* ??0?$collate@_W@std@@IAE@PBDI@Z */
/* ??0?$collate@_W@std@@IEAA@PEBD_K@Z */
static collate* collate_wchar_ctor_name(collate *this, const char *name, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %lu)\n", this, name, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    collate_wchar__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$collate@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static collate* collate_wchar_ctor_locinfo(collate *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_collate_wchar_vtable;
    collate_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$collate@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_locinfo, 12)
collate* __thiscall collate_short_ctor_locinfo(collate *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    collate *ret = collate_wchar_ctor_locinfo(this, locinfo, refs);
    ret->facet.vtable = &MSVCP_collate_short_vtable;
    return ret;
}

/* ??0?$collate@_W@std@@QAE@I@Z */
/* ??0?$collate@_W@std@@QEAA@_K@Z */
static collate* collate_wchar_ctor_refs(collate *this, MSVCP_size_t refs)
{
    return collate_wchar_ctor_name(this, "C", refs);
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

/* ??1?$collate@G@std@@UAE@XZ */
/* ??1?$collate@G@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_wchar_dtor, 4)
void __thiscall collate_wchar_dtor(collate *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(collate_wchar_vector_dtor, 8)
collate* __thiscall collate_wchar_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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

/* ??_F?$collate@_W@std@@QAEXXZ */
/* ??_F?$collate@_W@std@@QEAAXXZ */
static collate* collate_wchar_ctor(collate *this)
{
    return collate_wchar_ctor_name(this, "C", 0);
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
static MSVCP_size_t collate_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(collate));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        collate_wchar_ctor_name((collate*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_COLLATE;
}

static collate* collate_wchar_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_wchar__Getcat(&fac, loc);
    obj = (collate*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t collate_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        collate_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &MSVCP_collate_short_vtable;
    }

    return LC_COLLATE;
}

static collate* collate_short_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_short__Getcat(&fac, loc);
    obj = (collate*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Wcscoll */
static int _Wcscoll(const wchar_t *first1, const wchar_t *last1, const wchar_t *first2,
        const wchar_t *last2, const _Collvec *coll)
{
    LCID lcid;

    TRACE("(%s %s)\n", debugstr_wn(first1, last1-first1), debugstr_wn(first2, last2-first2));

    if(coll)
        lcid = coll->handle;
    else
        lcid = ___lc_handle_func()[LC_COLLATE];
    return CompareStringW(lcid, 0, first1, last1-first1, first2, last2-first2)-CSTR_EQUAL;
}

/* ?do_compare@?$collate@_W@std@@MBEHPB_W000@Z */
/* ?do_compare@?$collate@_W@std@@MEBAHPEB_W000@Z */
/* ?do_compare@?$collate@G@std@@MBEHPBG000@Z */
/* ?do_compare@?$collate@G@std@@MEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_compare, 20)
#define call_collate_wchar_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 4, int, \
        (const collate*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*), \
        (this, first1, last1, first2, last2))
int __thiscall collate_wchar_do_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return _Wcscoll(first1, last1, first2, last2, &this->coll);
}

/* ?compare@?$collate@_W@std@@QBEHPB_W000@Z */
/* ?compare@?$collate@_W@std@@QEBAHPEB_W000@Z */
/* ?compare@?$collate@G@std@@QBEHPBG000@Z */
/* ?compare@?$collate@G@std@@QEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_compare, 20)
int __thiscall collate_wchar_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return call_collate_wchar_do_compare(this, first1, last1, first2, last2);
}

/* ?do_hash@?$collate@_W@std@@MBEJPB_W0@Z */
/* ?do_hash@?$collate@_W@std@@MEBAJPEB_W0@Z */
/* ?do_hash@?$collate@G@std@@MBEJPBG0@Z */
/* ?do_hash@?$collate@G@std@@MEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_hash, 12)
#define call_collate_wchar_do_hash(this, first, last) CALL_VTBL_FUNC(this, 12, LONG, \
        (const collate*, const wchar_t*, const wchar_t*), (this, first, last))
LONG __thiscall collate_wchar_do_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    ULONG ret = 0;

    TRACE("(%p %p %p)\n", this, first, last);

    for(; first<last; first++)
        ret = (ret<<8 | ret>>24) + *first;
    return ret;
}

/* ?hash@?$collate@_W@std@@QBEJPB_W0@Z */
/* ?hash@?$collate@_W@std@@QEBAJPEB_W0@Z */
/* ?hash@?$collate@G@std@@QBEJPBG0@Z */
/* ?hash@?$collate@G@std@@QEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_hash, 12)
LONG __thiscall collate_wchar_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_collate_wchar_do_hash(this, first, last);
}

/* ?do_transform@?$collate@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?do_transform@?$collate@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?do_transform@?$collate@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?do_transform@?$collate@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_transform, 16)
basic_string_wchar* __thiscall collate_wchar_do_transform(const collate *this,
        basic_string_wchar *ret, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?transform@?$collate@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?transform@?$collate@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?transform@?$collate@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_transform, 16)
basic_string_wchar* __thiscall collate_wchar_transform(const collate *this,
        basic_string_wchar *ret, const wchar_t *first, const wchar_t *last)
{
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

DEFINE_THISCALL_WRAPPER(ctype_base_vector_dtor, 8)
ctype_base* __thiscall ctype_base_vector_dtor(ctype_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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

/* ?id@?$ctype@D@std@@2V0locale@2@A */
locale_id ctype_char_id = {0};
/* ?table_size@?$ctype@D@std@@2IB */
/* ?table_size@?$ctype@D@std@@2_KB */
MSVCP_size_t ctype_char_table_size = 256;

/* ??_7?$ctype@D@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_char_vtable;

/* ?_Init@?$ctype@D@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@D@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Init, 8)
void __thiscall ctype_char__Init(ctype_char *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getctype(locinfo, &this->ctype);
}

/* ?_Tidy@?$ctype@D@std@@IAEXXZ */
/* ?_Tidy@?$ctype@D@std@@IEAAXXZ */
static void ctype_char__Tidy(ctype_char *this)
{
    TRACE("(%p)\n", this);

    if(this->ctype.delfl)
        free((short*)this->ctype.table);
}

/* ?classic_table@?$ctype@D@std@@KAPBFXZ */
/* ?classic_table@?$ctype@D@std@@KAPEBFXZ */
const short* __cdecl ctype_char_classic_table(void)
{
    TRACE("()\n");
    return &((short*)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_ctype"))[1];
}

/* ??0?$ctype@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_locinfo, 12)
ctype_char* __thiscall ctype_char_ctor_locinfo(ctype_char *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);
    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_ctype_char_vtable;
    ctype_char__Init(this, locinfo);
    return this;
}

/* ??0?$ctype@D@std@@QAE@PBF_NI@Z */
/* ??0?$ctype@D@std@@QEAA@PEBF_N_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_table, 16)
ctype_char* __thiscall ctype_char_ctor_table(ctype_char *this,
        const short *table, MSVCP_bool delete, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %p %d %lu)\n", this, table, delete, refs);

    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_ctype_char_vtable;

    _Locinfo_ctor(&locinfo);
    ctype_char__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);

    if(table) {
        ctype_char__Tidy(this);
        this->ctype.table = table;
        this->ctype.delfl = delete;
    }
    return this;
}

/* ??_F?$ctype@D@std@@QAEXXZ */
/* ??_F?$ctype@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor, 4)
ctype_char* __thiscall ctype_char_ctor(ctype_char *this)
{
    return ctype_char_ctor_table(this, NULL, FALSE, 0);
}

/* ??1?$ctype@D@std@@UAE@XZ */
/* ??1?$ctype@D@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_char_dtor, 4)
void __thiscall ctype_char_dtor(ctype_char *this)
{
    TRACE("(%p)\n", this);
    ctype_char__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(ctype_char_vector_dtor, 8)
ctype_char* __thiscall ctype_char_vector_dtor(ctype_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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
#define call_ctype_char_do_narrow_ch(this, ch, unused) CALL_VTBL_FUNC(this, 32, \
        char, (const ctype_char*, char, char), (this, ch, unused))
char __thiscall ctype_char_do_narrow_ch(const ctype_char *this, char ch, char unused)
{
    TRACE("(%p %c %c)\n", this, ch, unused);
    return ch;
}

/* ?do_narrow@?$ctype@D@std@@MBEPBDPBD0DPAD@Z */
/* ?do_narrow@?$ctype@D@std@@MEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_narrow, 20)
#define call_ctype_char_do_narrow(this, first, last, unused, dest) CALL_VTBL_FUNC(this, 28, \
        const char*, (const ctype_char*, const char*, const char*, char, char*), \
        (this, first, last, unused, dest))
const char* __thiscall ctype_char_do_narrow(const ctype_char *this,
        const char *first, const char *last, char unused, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    memcpy(dest, first, last-first);
    return last;
}

/* ?narrow@?$ctype@D@std@@QBEDDD@Z */
/* ?narrow@?$ctype@D@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow_ch, 12)
char __thiscall ctype_char_narrow_ch(const ctype_char *this, char ch, char dflt)
{
    TRACE("(%p %c %c)\n", this, ch, dflt);
    return call_ctype_char_do_narrow_ch(this, ch, dflt);
}

/* ?narrow@?$ctype@D@std@@QBEPBDPBD0DPAD@Z */
/* ?narrow@?$ctype@D@std@@QEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow, 20)
const char* __thiscall ctype_char_narrow(const ctype_char *this,
        const char *first, const char *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %c %p)\n", this, first, last, dflt, dest);
    return call_ctype_char_do_narrow(this, first, last, dflt, dest);
}

/* ?do_widen@?$ctype@D@std@@MBEDD@Z */
/* ?do_widen@?$ctype@D@std@@MEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen_ch, 8)
#define call_ctype_char_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 24, \
        char, (const ctype_char*, char), (this, ch))
char __thiscall ctype_char_do_widen_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ch;
}

/* ?do_widen@?$ctype@D@std@@MBEPBDPBD0PAD@Z */
/* ?do_widen@?$ctype@D@std@@MEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen, 16)
#define call_ctype_char_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 20, \
        const char*, (const ctype_char*, const char*, const char*, char*), \
        (this, first, last, dest))
const char* __thiscall ctype_char_do_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    memcpy(dest, first, last-first);
    return last;
}

/* ?widen@?$ctype@D@std@@QBEDD@Z */
/* ?widen@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen_ch, 8)
char __thiscall ctype_char_widen_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_widen_ch(this, ch);
}

/* ?widen@?$ctype@D@std@@QBEPBDPBD0PAD@Z */
/* ?widen@?$ctype@D@std@@QEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen, 16)
const char* __thiscall ctype_char_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_char_do_widen(this, first, last, dest);
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t ctype_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(ctype_char));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        ctype_char_ctor_locinfo((ctype_char*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

ctype_char* ctype_char_use_facet(const locale *loc)
{
    static ctype_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_char__Getcat(&fac, loc);
    obj = (ctype_char*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Tolower */
int __cdecl _Tolower(int ch, const _Ctypevec *ctype)
{
    unsigned int cp;

    TRACE("%d %p\n", ch, ctype);

    if(ctype)
        cp = ctype->page;
    else
        cp = ___lc_codepage_func();

    /* Don't convert to unicode in case of C locale */
    if(!cp) {
        if(ch>='A' && ch<='Z')
            ch = ch-'A'+'a';
        return ch;
    } else {
        WCHAR wide, lower;
        char str[2];
        int size;

        if(ch > 255) {
            str[0] = (ch>>8) & 255;
            str[1] = ch & 255;
            size = 2;
        } else {
            str[0] = ch & 255;
            size = 1;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, str, size, &wide, 1))
            return ch;

        lower = tolowerW(wide);
        if(lower == wide)
            return ch;

        WideCharToMultiByte(cp, 0, &lower, 1, str, 2, NULL, NULL);

        return str[0] + (str[1]<<8);
    }
}

/* ?do_tolower@?$ctype@D@std@@MBEDD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBADD@Z */
#define call_ctype_char_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 8, \
        char, (const ctype_char*, char), (this, ch))
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower_ch, 8)
char __thiscall ctype_char_do_tolower_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return _Tolower(ch, &this->ctype);
}

/* ?do_tolower@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
#define call_ctype_char_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 4, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower, 12)
const char* __thiscall ctype_char_do_tolower(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Tolower(*first, &this->ctype);
    return last;
}

/* ?tolower@?$ctype@D@std@@QBEDD@Z */
/* ?tolower@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower_ch, 8)
char __thiscall ctype_char_tolower_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_tolower_ch(this, ch);
}

/* ?tolower@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?tolower@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower, 12)
const char* __thiscall ctype_char_tolower(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_char_do_tolower(this, first, last);
}

/* _Toupper */
int __cdecl _Toupper(int ch, const _Ctypevec *ctype)
{
    unsigned int cp;

    TRACE("%d %p\n", ch, ctype);

    if(ctype)
        cp = ctype->page;
    else
        cp = ___lc_codepage_func();

    /* Don't convert to unicode in case of C locale */
    if(!cp) {
        if(ch>='a' && ch<='z')
            ch = ch-'a'+'A';
        return ch;
    } else {
        WCHAR wide, upper;
        char str[2];
        int size;

        if(ch > 255) {
            str[0] = (ch>>8) & 255;
            str[1] = ch & 255;
            size = 2;
        } else {
            str[0] = ch & 255;
            size = 1;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, str, size, &wide, 1))
            return ch;

        upper = toupperW(wide);
        if(upper == wide)
            return ch;

        WideCharToMultiByte(cp, 0, &upper, 1, str, 2, NULL, NULL);

        return str[0] + (str[1]<<8);
    }
}

/* ?do_toupper@?$ctype@D@std@@MBEDD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBADD@Z */
#define call_ctype_char_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 16, \
        char, (const ctype_char*, char), (this, ch))
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper_ch, 8)
char __thiscall ctype_char_do_toupper_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return _Toupper(ch, &this->ctype);
}

/* ?do_toupper@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
#define call_ctype_char_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 12, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper, 12)
const char* __thiscall ctype_char_do_toupper(const ctype_char *this,
        char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Toupper(*first, &this->ctype);
    return last;
}

/* ?toupper@?$ctype@D@std@@QBEDD@Z */
/* ?toupper@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper_ch, 8)
char __thiscall ctype_char_toupper_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_toupper_ch(this, ch);
}

/* ?toupper@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?toupper@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper, 12)
const char* __thiscall ctype_char_toupper(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_char_do_toupper(this, first, last);
}

/* ?is@?$ctype@D@std@@QBE_NFD@Z */
/* ?is@?$ctype@D@std@@QEBA_NFD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is_ch, 12)
MSVCP_bool __thiscall ctype_char_is_ch(const ctype_char *this, short mask, char ch)
{
    TRACE("(%p %x %c)\n", this, mask, ch);
    return (this->ctype.table[(unsigned char)ch] & mask) != 0;
}

/* ?is@?$ctype@D@std@@QBEPBDPBD0PAF@Z */
/* ?is@?$ctype@D@std@@QEBAPEBDPEBD0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is, 16)
const char* __thiscall ctype_char_is(const ctype_char *this, const char *first, const char *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    for(; first<last; first++)
        *dest++ = this->ctype.table[(unsigned char)*first];
    return last;
}

/* ?scan_is@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_is@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_is, 16)
const char* __thiscall ctype_char_scan_is(const ctype_char *this, short mask, const char *first, const char *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(!ctype_char_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_not@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_not@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_not, 16)
const char* __thiscall ctype_char_scan_not(const ctype_char *this, short mask, const char *first, const char *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(ctype_char_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?table@?$ctype@D@std@@IBEPBFXZ */
/* ?table@?$ctype@D@std@@IEBAPEBFXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_table, 4)
const short* __thiscall ctype_char_table(const ctype_char *this)
{
    TRACE("(%p)\n", this);
    return this->ctype.table;
}

/* ?id@?$ctype@_W@std@@2V0locale@2@A */
static locale_id ctype_wchar_id = {0};
/* ?id@?$ctype@G@std@@2V0locale@2@A */
locale_id ctype_short_id = {0};

/* ??_7?$ctype@_W@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_wchar_vtable;
/* ??_7?$ctype@G@std@@6B@ */
extern const vtable_ptr MSVCP_ctype_short_vtable;

/* ?_Init@?$ctype@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Init, 8)
void __thiscall ctype_wchar__Init(ctype_wchar *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getctype(locinfo, &this->ctype);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$ctype@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static ctype_wchar* ctype_wchar_ctor_locinfo(ctype_wchar *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);
    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_ctype_wchar_vtable;
    ctype_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$ctype@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_locinfo, 12)
ctype_wchar* __thiscall ctype_short_ctor_locinfo(ctype_wchar *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    ctype_wchar *ret = ctype_wchar_ctor_locinfo(this, locinfo, refs);
    this->base.facet.vtable = &MSVCP_ctype_short_vtable;
    return ret;
}

/* ??0?$ctype@_W@std@@QAE@I@Z */
/* ??0?$ctype@_W@std@@QEAA@_K@Z */
static ctype_wchar* ctype_wchar_ctor_refs(ctype_wchar *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_ctype_wchar_vtable;

    _Locinfo_ctor(&locinfo);
    ctype_wchar__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
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

/* ??_F?$ctype@_W@std@@QAEXXZ */
/* ??_F?$ctype@_W@std@@QEAAXXZ */
static ctype_wchar* ctype_wchar_ctor(ctype_wchar *this)
{
    TRACE("(%p)\n", this);
    return ctype_short_ctor_refs(this, 0);
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

/* ??1?$ctype@G@std@@UAE@XZ */
/* ??1?$ctype@G@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_wchar_dtor, 4)
void __thiscall ctype_wchar_dtor(ctype_wchar *this)
{
    TRACE("(%p)\n", this);
    if(this->ctype.delfl)
        free((void*)this->ctype.table);
}

DEFINE_THISCALL_WRAPPER(ctype_wchar_vector_dtor, 8)
ctype_wchar* __thiscall ctype_wchar_vector_dtor(ctype_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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

/* _Wcrtomb */
int __cdecl _Wcrtomb(char *s, wchar_t wch, int *state, const _Cvtvec *cvt)
{
    int cp, size;
    BOOL def;

    TRACE("%p %d %p %p\n", s, wch, state, cvt);

    if(cvt)
        cp = cvt->page;
    else
        cp = ___lc_codepage_func();

    if(!cp) {
        if(wch > 255) {
           *_errno() = EILSEQ;
           return -1;
        }

        *s = wch & 255;
        return 1;
    }

    size = WideCharToMultiByte(cp, 0, &wch, 1, s, MB_LEN_MAX, NULL, &def);
    if(!size || def) {
        *_errno() = EILSEQ;
        return -1;
    }

    return size;
}

/* ?_Donarrow@?$ctype@_W@std@@IBED_WD@Z */
/* ?_Donarrow@?$ctype@_W@std@@IEBAD_WD@Z */
/* ?_Donarrow@?$ctype@G@std@@IBEDGD@Z */
/* ?_Donarrow@?$ctype@G@std@@IEBADGD@Z */
static char ctype_wchar__Donarrow(const ctype_wchar *this, wchar_t ch, char dflt)
{
    char buf[MB_LEN_MAX];

    TRACE("(%p %d %d)\n", this, ch, dflt);

    return _Wcrtomb(buf, ch, NULL, &this->cvt)==1 ? buf[0] : dflt;
}

/* ?do_narrow@?$ctype@_W@std@@MBED_WD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAD_WD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEDGD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow_ch, 12)
#define call_ctype_wchar_do_narrow_ch(this, ch, dflt) CALL_VTBL_FUNC(this, 48, \
        char, (const ctype_wchar*, wchar_t, char), (this, ch, dflt))
char __thiscall ctype_wchar_do_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    return ctype_wchar__Donarrow(this, ch, dflt);
}

/* ?do_narrow@?$ctype@_W@std@@MBEPB_WPB_W0DPAD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAPEB_WPEB_W0DPEAD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEPBGPBG0DPAD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow, 20)
#define call_ctype_wchar_do_narrow(this, first, last, dflt, dest) CALL_VTBL_FUNC(this, 44, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, char, char*), \
        (this, first, last, dflt, dest))
const wchar_t* __thiscall ctype_wchar_do_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %d %p)\n", this, first, last, dflt, dest);
    for(; first<last; first++)
        *dest++ = ctype_wchar__Donarrow(this, *first, dflt);
    return last;
}

/* ?narrow@?$ctype@_W@std@@QBED_WD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAD_WD@Z */
/* ?narrow@?$ctype@G@std@@QBEDGD@Z */
/* ?narrow@?$ctype@G@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow_ch, 12)
char __thiscall ctype_wchar_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    TRACE("(%p %d %d)\n", this, ch, dflt);
    return call_ctype_wchar_do_narrow_ch(this, ch, dflt);
}

/* ?narrow@?$ctype@_W@std@@QBEPB_WPB_W0DPAD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAPEB_WPEB_W0DPEAD@Z */
/* ?narrow@?$ctype@G@std@@QBEPBGPBG0DPAD@Z */
/* ?narrow@?$ctype@G@std@@QEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow, 20)
const wchar_t* __thiscall ctype_wchar_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %d %p)\n", this, first, last, dflt, dest);
    return call_ctype_wchar_do_narrow(this, first, last, dflt, dest);
}

/* _Mbrtowc */
int __cdecl _Mbrtowc(wchar_t *out, const char *in, MSVCP_size_t len, int *state, const _Cvtvec *cvt)
{
    int i, cp;
    CPINFO cp_info;
    BOOL is_lead;

    TRACE("(%p %p %lu %p %p)\n", out, in, len, state, cvt);

    if(!len)
        return 0;

    if(cvt)
        cp = cvt->page;
    else
        cp = ___lc_codepage_func();

    if(!cp) {
        if(out)
            *out = (unsigned char)*in;

        *state = 0;
        return *in ? 1 : 0;
    }

    if(*state) {
        ((char*)state)[1] = *in;

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, (char*)state, 2, out, out ? 1 : 0)) {
            *state = 0;
            *_errno() = EILSEQ;
            return -1;
        }

        *state = 0;
        return 2;
    }

    GetCPInfo(cp, &cp_info);
    is_lead = FALSE;
    for(i=0; i<MAX_LEADBYTES; i+=2) {
        if(!cp_info.LeadByte[i+1])
            break;
        if((unsigned char)*in>=cp_info.LeadByte[i] && (unsigned char)*in<=cp_info.LeadByte[i+1]) {
            is_lead = TRUE;
            break;
        }
    }

    if(is_lead) {
        if(len == 1) {
            *state = (unsigned char)*in;
            return -2;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, in, 2, out, out ? 1 : 0)) {
            *_errno() = EILSEQ;
            return -1;
        }
        return 2;
    }

    if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, in, 1, out, out ? 1 : 0)) {
        *_errno() = EILSEQ;
        return -1;
    }
    return 1;
}

/* ?_Dowiden@?$ctype@_W@std@@IBE_WD@Z */
/* ?_Dowiden@?$ctype@_W@std@@IEBA_WD@Z */
/* ?_Dowiden@?$ctype@G@std@@IBEGD@Z */
/* ?_Dowiden@?$ctype@G@std@@IEBAGD@Z */
static wchar_t ctype_wchar__Dowiden(const ctype_wchar *this, char ch)
{
    wchar_t ret;
    int state = 0;
    TRACE("(%p %d)\n", this, ch);
    return _Mbrtowc(&ret, &ch, 1, &state, &this->cvt)<0 ? WEOF : ret;
}

/* ?do_widen@?$ctype@_W@std@@MBE_WD@Z */
/* ?do_widen@?$ctype@_W@std@@MEBA_WD@Z */
/* ?do_widen@?$ctype@G@std@@MBEGD@Z */
/* ?do_widen@?$ctype@G@std@@MEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen_ch, 8)
#define call_ctype_wchar_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 40, \
        wchar_t, (const ctype_wchar*, char), (this, ch))
wchar_t __thiscall ctype_wchar_do_widen_ch(const ctype_wchar *this, char ch)
{
    return ctype_wchar__Dowiden(this, ch);
}

/* ?do_widen@?$ctype@_W@std@@MBEPBDPBD0PA_W@Z */
/* ?do_widen@?$ctype@_W@std@@MEBAPEBDPEBD0PEA_W@Z */
/* ?do_widen@?$ctype@G@std@@MBEPBDPBD0PAG@Z */
/* ?do_widen@?$ctype@G@std@@MEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen, 16)
#define call_ctype_wchar_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 36, \
        const char*, (const ctype_wchar*, const char*, const char*, wchar_t*), \
        (this, first, last, dest))
const char* __thiscall ctype_wchar_do_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    for(; first<last; first++)
        *dest++ = ctype_wchar__Dowiden(this, *first);
    return last;
}

/* ?widen@?$ctype@_W@std@@QBE_WD@Z */
/* ?widen@?$ctype@_W@std@@QEBA_WD@Z */
/* ?widen@?$ctype@G@std@@QBEGD@Z */
/* ?widen@?$ctype@G@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen_ch, 8)
wchar_t __thiscall ctype_wchar_widen_ch(const ctype_wchar *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_widen_ch(this, ch);
}

/* ?widen@?$ctype@_W@std@@QBEPBDPBD0PA_W@Z */
/* ?widen@?$ctype@_W@std@@QEBAPEBDPEBD0PEA_W@Z */
/* ?widen@?$ctype@G@std@@QBEPBDPBD0PAG@Z */
/* ?widen@?$ctype@G@std@@QEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen, 16)
const char* __thiscall ctype_wchar_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_wchar_do_widen(this, first, last, dest);
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t ctype_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(ctype_wchar));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        ctype_wchar_ctor_locinfo((ctype_wchar*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t ctype_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        ctype_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &MSVCP_ctype_short_vtable;
    }

    return LC_CTYPE;
}

/* _Towlower */
static wchar_t _Towlower(wchar_t ch, const _Ctypevec *ctype)
{
    TRACE("(%d %p)\n", ch, ctype);
    return tolowerW(ch);
}

ctype_wchar* ctype_wchar_use_facet(const locale *loc)
{
    static ctype_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_wchar__Getcat(&fac, loc);
    obj = (ctype_wchar*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

ctype_wchar* ctype_short_use_facet(const locale *loc)
{
    static ctype_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_short__Getcat(&fac, loc);
    obj = (ctype_wchar*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_tolower@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEGG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower_ch, 8)
#define call_ctype_wchar_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 24, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
wchar_t __thiscall ctype_wchar_do_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    return _Towlower(ch, &this->ctype);
}

/* ?do_tolower@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower, 12)
#define call_ctype_wchar_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 20, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
const wchar_t* __thiscall ctype_wchar_do_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Towlower(*first, &this->ctype);
    return last;
}

/* ?tolower@?$ctype@_W@std@@QBE_W_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?tolower@?$ctype@G@std@@QBEGG@Z */
/* ?tolower@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower_ch, 8)
wchar_t __thiscall ctype_wchar_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_tolower_ch(this, ch);
}

/* ?tolower@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?tolower@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?tolower@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower, 12)
const wchar_t* __thiscall ctype_wchar_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_wchar_do_tolower(this, first, last);
}

/* _Towupper */
static wchar_t _Towupper(wchar_t ch, const _Ctypevec *ctype)
{
    TRACE("(%d %p)\n", ch, ctype);
    return toupperW(ch);
}

/* ?do_toupper@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEGG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper_ch, 8)
#define call_ctype_wchar_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 32, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
wchar_t __thiscall ctype_wchar_do_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    return _Towupper(ch, &this->ctype);
}

/* ?do_toupper@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper, 12)
#define call_ctype_wchar_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 28, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
const wchar_t* __thiscall ctype_wchar_do_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Towupper(*first, &this->ctype);
    return last;
}

/* ?toupper@?$ctype@_W@std@@QBE_W_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?toupper@?$ctype@G@std@@QBEGG@Z */
/* ?toupper@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper_ch, 8)
wchar_t __thiscall ctype_wchar_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_toupper_ch(this, ch);
}

/* ?toupper@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?toupper@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?toupper@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper, 12)
const wchar_t* __thiscall ctype_wchar_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_wchar_do_toupper(this, first, last);
}

/* _Getwctypes */
static const wchar_t* _Getwctypes(const wchar_t *first, const wchar_t *last,
        short *mask, const _Ctypevec *ctype)
{
    TRACE("(%p %p %p %p)\n", first, last, mask, ctype);
    GetStringTypeW(CT_CTYPE1, first, last-first, (WORD*)mask);
    return last;
}

/* _Getwctype */
static short _Getwctype(wchar_t ch, const _Ctypevec *ctype)
{
    short mask = 0;
    _Getwctypes(&ch, &ch+1, &mask, ctype);
    return mask;
}

/* ?do_is@?$ctype@_W@std@@MBE_NF_W@Z */
/* ?do_is@?$ctype@_W@std@@MEBA_NF_W@Z */
/* ?do_is@?$ctype@G@std@@MBE_NFG@Z */
/* ?do_is@?$ctype@G@std@@MEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is_ch, 12)
#define call_ctype_wchar_do_is_ch(this, mask, ch) CALL_VTBL_FUNC(this, 8, \
        MSVCP_bool, (const ctype_wchar*, short, wchar_t), (this, mask, ch))
MSVCP_bool __thiscall ctype_wchar_do_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    TRACE("(%p %x %d)\n", this, mask, ch);
    return (_Getwctype(ch, &this->ctype) & mask) != 0;
}

/* ?do_is@?$ctype@_W@std@@MBEPB_WPB_W0PAF@Z */
/* ?do_is@?$ctype@_W@std@@MEBAPEB_WPEB_W0PEAF@Z */
/* ?do_is@?$ctype@G@std@@MBEPBGPBG0PAF@Z */
/* ?do_is@?$ctype@G@std@@MEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is, 16)
#define call_ctype_wchar_do_is(this, first, last, dest) CALL_VTBL_FUNC(this, 4, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, short*), \
        (this, first, last, dest))
const wchar_t* __thiscall ctype_wchar_do_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return _Getwctypes(first, last, dest, &this->ctype);
}

/* ?is@?$ctype@_W@std@@QBE_NF_W@Z */
/* ?is@?$ctype@_W@std@@QEBA_NF_W@Z */
/* ?is@?$ctype@G@std@@QBE_NFG@Z */
/* ?is@?$ctype@G@std@@QEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is_ch, 12)
MSVCP_bool __thiscall ctype_wchar_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    TRACE("(%p %x %d)\n", this, mask, ch);
    return call_ctype_wchar_do_is_ch(this, mask, ch);
}

/* ?is@?$ctype@_W@std@@QBEPB_WPB_W0PAF@Z */
/* ?is@?$ctype@_W@std@@QEBAPEB_WPEB_W0PEAF@Z */
/* ?is@?$ctype@G@std@@QBEPBGPBG0PAF@Z */
/* ?is@?$ctype@G@std@@QEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is, 16)
const wchar_t* __thiscall ctype_wchar_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_wchar_do_is(this, first, last, dest);
}

/* ?do_scan_is@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_is@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_is@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_is@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_is, 16)
#define call_ctype_wchar_do_scan_is(this, mask, first, last) CALL_VTBL_FUNC(this, 12, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
const wchar_t* __thiscall ctype_wchar_do_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %d %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(!ctype_wchar_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_is@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_is@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_is@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_is@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_is, 16)
const wchar_t* __thiscall ctype_wchar_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    return call_ctype_wchar_do_scan_is(this, mask, first, last);
}

/* ?do_scan_not@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_not@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_not@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_not@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_not, 16)
#define call_ctype_wchar_do_scan_not(this, mask, first, last) CALL_VTBL_FUNC(this, 16, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
const wchar_t* __thiscall ctype_wchar_do_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(ctype_wchar_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_not@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_not@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_not@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_not@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_not, 16)
const wchar_t* __thiscall ctype_wchar_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    return call_ctype_wchar_do_scan_not(this, mask, first, last);
}

/* ??_7codecvt_base@std@@6B@ */
extern const vtable_ptr MSVCP_codecvt_base_vtable;

/* ??0codecvt_base@std@@QAE@I@Z */
/* ??0codecvt_base@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_base_ctor_refs, 8)
codecvt_base* __thiscall codecvt_base_ctor_refs(codecvt_base *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_codecvt_base_vtable;
    return this;
}

/* ??_Fcodecvt_base@std@@QAEXXZ */
/* ??_Fcodecvt_base@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_ctor, 4)
codecvt_base* __thiscall codecvt_base_ctor(codecvt_base *this)
{
    return codecvt_base_ctor_refs(this, 0);
}

/* ??1codecvt_base@std@@UAE@XZ */
/* ??1codecvt_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_dtor, 4)
void __thiscall codecvt_base_dtor(codecvt_base *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(codecvt_base_vector_dtor, 8)
codecvt_base* __thiscall codecvt_base_vector_dtor(codecvt_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_base_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        codecvt_base_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?do_always_noconv@codecvt_base@std@@MBE_NXZ */
/* ?do_always_noconv@codecvt_base@std@@MEBA_NXZ */
#define call_codecvt_base_do_always_noconv(this) CALL_VTBL_FUNC(this, 4, \
        MSVCP_bool, (const codecvt_base*), (this))
DEFINE_THISCALL_WRAPPER(codecvt_base_do_always_noconv, 4)
MSVCP_bool __thiscall codecvt_base_do_always_noconv(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return TRUE;
}

/* ?always_noconv@codecvt_base@std@@QBE_NXZ */
/* ?always_noconv@codecvt_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_always_noconv, 4)
MSVCP_bool __thiscall codecvt_base_always_noconv(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_always_noconv(this);
}

/* ?do_max_length@codecvt_base@std@@MBEHXZ */
/* ?do_max_length@codecvt_base@std@@MEBAHXZ */
#define call_codecvt_base_do_max_length(this) CALL_VTBL_FUNC(this, 8, \
        int, (const codecvt_base*), (this))
DEFINE_THISCALL_WRAPPER(codecvt_base_do_max_length, 4)
int __thiscall codecvt_base_do_max_length(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return 1;
}

/* ?max_length@codecvt_base@std@@QBEHXZ */
/* ?max_length@codecvt_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_max_length, 4)
int __thiscall codecvt_base_max_length(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_max_length(this);
}

/* ?do_encoding@codecvt_base@std@@MBEHXZ */
/* ?do_encoding@codecvt_base@std@@MEBAHXZ */
#define call_codecvt_base_do_encoding(this) CALL_VTBL_FUNC(this, 12, \
        int, (const codecvt_base*), (this))
DEFINE_THISCALL_WRAPPER(codecvt_base_do_encoding, 4)
int __thiscall codecvt_base_do_encoding(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return 1;
}

/* ?encoding@codecvt_base@std@@QBEHXZ */
/* ?encoding@codecvt_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_encoding, 4)
int __thiscall codecvt_base_encoding(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_encoding(this);
}

/* ?id@?$codecvt@DDH@std@@2V0locale@2@A */
locale_id codecvt_char_id = {0};

/* ??_7?$codecvt@DDH@std@@6B@ */
extern const vtable_ptr MSVCP_codecvt_char_vtable;

/* ?_Init@?$codecvt@DDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@DDH@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char__Init, 8)
void __thiscall codecvt_char__Init(codecvt_char *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
}

/* ??0?$codecvt@DDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@DDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor_locinfo, 12)
codecvt_char* __thiscall codecvt_char_ctor_locinfo(codecvt_char *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);
    codecvt_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_codecvt_char_vtable;
    return this;
}

/* ??0?$codecvt@DDH@std@@QAE@I@Z */
/* ??0?$codecvt@DDH@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor_refs, 8)
codecvt_char* __thiscall codecvt_char_ctor_refs(codecvt_char *this, MSVCP_size_t refs)
{
    return codecvt_char_ctor_locinfo(this, NULL, refs);
}

/* ??_F?$codecvt@DDH@std@@QAEXXZ */
/* ??_F?$codecvt@DDH@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor, 4)
codecvt_char* __thiscall codecvt_char_ctor(codecvt_char *this)
{
    return codecvt_char_ctor_locinfo(this, NULL, 0);
}

/* ??1?$codecvt@DDH@std@@UAE@XZ */
/* ??1?$codecvt@DDH@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_char_dtor, 4)
void __thiscall codecvt_char_dtor(codecvt_char *this)
{
    TRACE("(%p)\n", this);
    codecvt_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(codecvt_char_vector_dtor, 8)
codecvt_char* __thiscall codecvt_char_vector_dtor(codecvt_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        codecvt_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t codecvt_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(codecvt_char));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        codecvt_char_ctor((codecvt_char*)*facet);
    }

    return LC_CTYPE;
}

codecvt_char* codecvt_char_use_facet(const locale *loc)
{
    static codecvt_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_char__Getcat(&fac, loc);
    obj = (codecvt_char*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_in@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?do_in@?$codecvt@DDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
#define call_codecvt_char_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 16, int, \
            (const codecvt_char*, int*, const char*, const char*, const char**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char_do_in, 32)
int __thiscall codecvt_char_do_in(const codecvt_char *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    *from_next = from;
    *to_next = to;
    return CODECVT_noconv;
}

/* ?in@?$codecvt@DDH@std@@QBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?in@?$codecvt@DDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_in, 32)
int __thiscall codecvt_char_in(const codecvt_char *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char_do_in(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_out@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?do_out@?$codecvt@DDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
#define call_codecvt_char_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 20, int, \
            (const codecvt_char*, int*, const char*, const char*, const char**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char_do_out, 32)
int __thiscall codecvt_char_do_out(const codecvt_char *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    *from_next = from;
    *to_next = to;
    return CODECVT_noconv;
}

/* ?out@?$codecvt@DDH@std@@QBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?out@?$codecvt@DDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_out, 32)
int __thiscall codecvt_char_out(const codecvt_char *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char_do_out(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@DDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@DDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
#define call_codecvt_char_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 24, \
        int, (const codecvt_char*, int*, char*, char*, char**), (this, state, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char_do_unshift, 20)
int __thiscall codecvt_char_do_unshift(const codecvt_char *this,
        int *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    *to_next = to;
    return CODECVT_noconv;
}

/* ?do_length@?$codecvt@DDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@DDH@std@@MEBAHAEBHPEBD1_K@Z */
#define call_codecvt_char_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 28, \
        int, (const codecvt_char*, const int*, const char*, const char*, MSVCP_size_t), \
        (this, state, from, from_end, max))
DEFINE_THISCALL_WRAPPER(codecvt_char_do_length, 20)
int __thiscall codecvt_char_do_length(const codecvt_char *this, const int *state,
        const char *from, const char *from_end, MSVCP_size_t max)
{
    TRACE("(%p %p %p %p %lu)\n", this, state, from, from_end, max);
    return (from_end-from > max ? max : from_end-from);
}

/* ?id@?$codecvt@_WDH@std@@2V0locale@2@A */
static locale_id codecvt_wchar_id = {0};
/* ?id@?$codecvt@GDH@std@@2V0locale@2@A */
locale_id codecvt_short_id = {0};

/* ??_7?$codecvt@_WDH@std@@6B@ */
extern const vtable_ptr MSVCP_codecvt_wchar_vtable;
/* ??_7?$codecvt@GDH@std@@6B@ */
extern const vtable_ptr MSVCP_codecvt_short_vtable;

/* ?_Init@?$codecvt@GDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@GDH@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_WDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_WDH@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar__Init, 8)
void __thiscall codecvt_wchar__Init(codecvt_wchar *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$codecvt@_WDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_WDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static codecvt_wchar* codecvt_wchar_ctor_locinfo(codecvt_wchar *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %ld)\n", this, locinfo, refs);

    codecvt_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &MSVCP_codecvt_wchar_vtable;

    codecvt_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$codecvt@GDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@GDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor_locinfo, 12)
codecvt_wchar* __thiscall codecvt_short_ctor_locinfo(codecvt_wchar *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %ld)\n", this, locinfo, refs);

    codecvt_wchar_ctor_locinfo(this, locinfo, refs);
    this->base.facet.vtable = &MSVCP_codecvt_short_vtable;
    return this;
}

/* ??0?$codecvt@GDH@std@@QAE@I@Z */
/* ??0?$codecvt@GDH@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor_refs, 8)
codecvt_wchar* __thiscall codecvt_short_ctor_refs(codecvt_wchar *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %ld)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    codecvt_short_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$codecvt@GDH@std@@QAEXXZ */
/* ??_F?$codecvt@GDH@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor, 4)
codecvt_wchar* __thiscall codecvt_short_ctor(codecvt_wchar *this)
{
    return codecvt_short_ctor_refs(this, 0);
}

/* ??1?$codecvt@GDH@std@@UAE@XZ */
/* ??1?$codecvt@GDH@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_dtor, 4)
void __thiscall codecvt_wchar_dtor(codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    codecvt_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(codecvt_wchar_vector_dtor, 8)
codecvt_wchar* __thiscall codecvt_wchar_vector_dtor(codecvt_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        codecvt_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@_WDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@_WDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t codecvt_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(codecvt_wchar));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        codecvt_wchar_ctor_locinfo((codecvt_wchar*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

static codecvt_wchar* codecvt_wchar_use_facet(const locale *loc)
{
    static codecvt_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_wchar__Getcat(&fac, loc);
    obj = (codecvt_wchar*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$codecvt@GDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@GDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t codecvt_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(codecvt_wchar));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        codecvt_short_ctor((codecvt_wchar*)*facet);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

codecvt_wchar* codecvt_short_use_facet(const locale *loc)
{
    static codecvt_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_short__Getcat(&fac, loc);
    obj = (codecvt_wchar*)fac;
    locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_always_noconv@?$codecvt@GDH@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@GDH@std@@MEBA_NXZ */
/* ?do_always_noconv@?$codecvt@_WDH@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@_WDH@std@@MEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_always_noconv, 4)
MSVCP_bool __thiscall codecvt_wchar_do_always_noconv(const codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    return FALSE;
}

/* ?do_max_length@?$codecvt@GDH@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@GDH@std@@MEBAHXZ */
/* ?do_max_length@?$codecvt@_WDH@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@_WDH@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_max_length, 4)
int __thiscall codecvt_wchar_do_max_length(const codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    return MB_LEN_MAX;
}

/* ?do_in@?$codecvt@GDH@std@@MBEHAAHPBD1AAPBDPAG3AAPAG@Z */
/* ?do_in@?$codecvt@GDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAG3AEAPEAG@Z */
/* ?do_in@?$codecvt@_WDH@std@@MBEHAAHPBD1AAPBDPA_W3AAPA_W@Z */
/* ?do_in@?$codecvt@_WDH@std@@MEBAHAEAHPEBD1AEAPEBDPEA_W3AEAPEA_W@Z */
#define call_codecvt_wchar_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 16, int, \
            (const codecvt_wchar*, int*, const char*, const char*, const char**, wchar_t*, wchar_t*, wchar_t**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_in, 32)
int __thiscall codecvt_wchar_do_in(const codecvt_wchar *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        wchar_t *to, wchar_t *to_end, wchar_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);

    *from_next = from;
    *to_next = to;

    while(*from_next!=from_end && *to_next!=to_end) {
        switch(_Mbrtowc(*to_next, *from_next, from_end-*from_next, state, &this->cvt)) {
        case -2:
            *from_next = from_end;
            return CODECVT_partial;
        case -1:
            return CODECVT_error;
        case 2:
            (*from_next)++;
            /* fall through */
        case 0:
        case 1:
            (*from_next)++;
            (*to_next)++;
        }
    }

    return CODECVT_ok;
}

/* ?in@?$codecvt@GDH@std@@QBEHAAHPBD1AAPBDPAG3AAPAG@Z */
/* ?in@?$codecvt@GDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAG3AEAPEAG@Z */
/* ?in@?$codecvt@_WDH@std@@QBEHAAHPBD1AAPBDPA_W3AAPA_W@Z */
/* ?in@?$codecvt@_WDH@std@@QEBAHAEAHPEBD1AEAPEBDPEA_W3AEAPEA_W@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_in, 32)
int __thiscall codecvt_wchar_in(const codecvt_wchar *this, int *state,
        const char *from, const char *from_end, const char **from_next,
        wchar_t *to, wchar_t *to_end, wchar_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_wchar_do_in(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_out@?$codecvt@GDH@std@@MBEHAAHPBG1AAPBGPAD3AAPAD@Z */
/* ?do_out@?$codecvt@GDH@std@@MEBAHAEAHPEBG1AEAPEBGPEAD3AEAPEAD@Z */
/* ?do_out@?$codecvt@_WDH@std@@MBEHAAHPB_W1AAPB_WPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_WDH@std@@MEBAHAEAHPEB_W1AEAPEB_WPEAD3AEAPEAD@Z */
#define call_codecvt_wchar_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 20, int, \
            (const codecvt_wchar*, int*, const wchar_t*, const wchar_t*, const wchar_t**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_out, 32)
int __thiscall codecvt_wchar_do_out(const codecvt_wchar *this, int *state,
        const wchar_t *from, const wchar_t *from_end, const wchar_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);

    *from_next = from;
    *to_next = to;

    while(*from_next!=from_end && *to_next!=to_end) {
        int old_state = *state, size;
        char buf[MB_LEN_MAX];

        switch((size = _Wcrtomb(buf, **from_next, state, &this->cvt))) {
        case -1:
            return CODECVT_error;
        default:
            if(size > from_end-*from_next) {
                *state = old_state;
                return CODECVT_partial;
            }

            (*from_next)++;
            memcpy_s(*to_next, to_end-*to_next, buf, size);
            (*to_next) += size;
        }
    }

    return CODECVT_ok;
}

/* ?out@?$codecvt@GDH@std@@QBEHAAHPBG1AAPBGPAD3AAPAD@Z */
/* ?out@?$codecvt@GDH@std@@QEBAHAEAHPEBG1AEAPEBGPEAD3AEAPEAD@Z */
/* ?out@?$codecvt@_WDH@std@@QBEHAAHPB_W1AAPB_WPAD3AAPAD@Z */
/* ?out@?$codecvt@_WDH@std@@QEBAHAEAHPEB_W1AEAPEB_WPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_out, 32)
int __thiscall codecvt_wchar_out(const codecvt_wchar *this, int *state,
        const wchar_t *from, const wchar_t *from_end, const wchar_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_wchar_do_out(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@GDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@GDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
/* ?do_unshift@?$codecvt@_WDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_WDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
#define call_codecvt_wchar_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 24, \
        int, (const codecvt_wchar*, int*, char*, char*, char**), (this, state, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_unshift, 20)
int __thiscall codecvt_wchar_do_unshift(const codecvt_wchar *this,
        int *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    if(*state)
        WARN("unexpected state: %x\n", *state);

    *to_next = to;
    return CODECVT_ok;
}

/* ?do_length@?$codecvt@GDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@GDH@std@@MEBAHAEBHPEBD1_K@Z */
/* ?do_length@?$codecvt@_WDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@_WDH@std@@MEBAHAEBHPEBD1_K@Z */
#define call_codecvt_wchar_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 28, \
        int, (const codecvt_wchar*, const int*, const char*, const char*, MSVCP_size_t), \
        (this, state, from, from_end, max))
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_length, 20)
int __thiscall codecvt_wchar_do_length(const codecvt_wchar *this, const int *state,
        const char *from, const char *from_end, MSVCP_size_t max)
{
    int tmp_state = *state, ret=0;

    TRACE("(%p %p %p %p %ld)\n", this, state, from, from_end, max);

    while(ret<max && from!=from_end) {
        switch(_Mbrtowc(NULL, from, from_end-from, &tmp_state, &this->cvt)) {
        case -2:
        case -1:
            return ret;
        case 2:
            from++;
            /* fall through */
        case 0:
        case 1:
            from++;
            ret++;
        }
    }

    return ret;
}

/* ?id@?$numpunct@D@std@@2V0locale@2@A */
locale_id numpunct_char_id = {0};

/* ??_7?$numpunct@D@std@@6B@ */
extern const vtable_ptr MSVCP_numpunct_char_vtable;

/* ?_Init@?$numpunct@D@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@D@std@@IEAAXAEBV_Locinfo@2@_N@Z */
static void numpunct_char__Init(numpunct_char *this, const _Locinfo *locinfo, MSVCP_bool isdef)
{
    int len;

    TRACE("(%p %p %d)\n", this, locinfo, isdef);

    len = strlen(_Locinfo__Getfalse(locinfo))+1;
    this->false_name = MSVCRT_operator_new(len);
    if(this->false_name)
        memcpy((char*)this->false_name, _Locinfo__Getfalse(locinfo), len);

    len = strlen(_Locinfo__Gettrue(locinfo))+1;
    this->true_name = MSVCRT_operator_new(len);
    if(this->true_name)
        memcpy((char*)this->true_name, _Locinfo__Gettrue(locinfo), len);

    if(isdef) {
        this->grouping = MSVCRT_operator_new(1);
        if(this->grouping)
            *(char*)this->grouping = 0;

        this->dp = '.';
        this->sep = ',';
    } else {
        const struct lconv *lc = _Locinfo__Getlconv(locinfo);

        len = strlen(lc->grouping)+1;
        this->grouping = MSVCRT_operator_new(len);
        if(this->grouping)
            memcpy((char*)this->grouping, lc->grouping, len);

        this->dp = lc->decimal_point[0];
        this->sep = lc->thousands_sep[0];
    }

    if(!this->false_name || !this->true_name || !this->grouping) {
        MSVCRT_operator_delete((char*)this->grouping);
        MSVCRT_operator_delete((char*)this->false_name);
        MSVCRT_operator_delete((char*)this->true_name);

        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
}

/* ?_Tidy@?$numpunct@D@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@D@std@@AEAAXXZ */
static void numpunct_char__Tidy(numpunct_char *this)
{
    TRACE("(%p)\n", this);

    MSVCRT_operator_delete((char*)this->grouping);
    MSVCRT_operator_delete((char*)this->false_name);
    MSVCRT_operator_delete((char*)this->true_name);
}

/* ??0?$numpunct@D@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@D@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
static numpunct_char* numpunct_char_ctor_locinfo(numpunct_char *this,
        const _Locinfo *locinfo, MSVCP_size_t refs, MSVCP_bool usedef)
{
    TRACE("(%p %p %lu %d)\n", this, locinfo, refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_numpunct_char_vtable;
    numpunct_char__Init(this, locinfo, usedef);
    return this;
}

/* ??0?$numpunct@D@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@D@std@@IEAA@PEBD_K_N@Z */
static numpunct_char* numpunct_char_ctor_name(numpunct_char *this,
        const char *name, MSVCP_size_t refs, MSVCP_bool usedef)
{
    _Locinfo locinfo;

    TRACE("(%p %s %lu %d)\n", this, debugstr_a(name), refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_numpunct_char_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    numpunct_char__Init(this, &locinfo, usedef);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$numpunct@D@std@@QAE@I@Z */
/* ??0?$numpunct@D@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor_refs, 8)
numpunct_char* __thiscall numpunct_char_ctor_refs(numpunct_char *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    return numpunct_char_ctor_name(this, "C", refs, FALSE);
}

/* ??_F?$numpunct@D@std@@QAEXXZ */
/* ??_F?$numpunct@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor, 4)
numpunct_char* __thiscall numpunct_char_ctor(numpunct_char *this)
{
    return numpunct_char_ctor_refs(this, 0);
}

/* ??1?$numpunct@D@std@@UAE@XZ */
/* ??1?$numpunct@D@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_dtor, 4)
void __thiscall numpunct_char_dtor(numpunct_char *this)
{
    TRACE("(%p)\n", this);
    numpunct_char__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(numpunct_char_vector_dtor, 8)
numpunct_char* __thiscall numpunct_char_vector_dtor(numpunct_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            numpunct_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        numpunct_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t numpunct_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(numpunct_char));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        numpunct_char_ctor_name((numpunct_char*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

static numpunct_char* numpunct_char_use_facet(const locale *loc)
{
    static numpunct_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_char__Getcat(&fac, loc);
    obj = (numpunct_char*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_decimal_point@?$numpunct@D@std@@MBEDXZ */
/* ?do_decimal_point@?$numpunct@D@std@@MEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_decimal_point, 4)
#define call_numpunct_char_do_decimal_point(this) CALL_VTBL_FUNC(this, 4, \
        char, (const numpunct_char *this), (this))
char __thiscall numpunct_char_do_decimal_point(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return this->dp;
}

/* ?decimal_point@?$numpunct@D@std@@QBEDXZ */
/* ?decimal_point@?$numpunct@D@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_decimal_point, 4)
char __thiscall numpunct_char_decimal_point(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_decimal_point(this);
}

/* ?do_thousands_sep@?$numpunct@D@std@@MBEDXZ */
/* ?do_thousands_sep@?$numpunct@D@std@@MEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_thousands_sep, 4)
#define call_numpunct_char_do_thousands_sep(this) CALL_VTBL_FUNC(this, 8, \
        char, (const numpunct_char*), (this))
char __thiscall numpunct_char_do_thousands_sep(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return this->sep;
}

/* ?thousands_sep@?$numpunct@D@std@@QBEDXZ */
/* ?thousands_sep@?$numpunct@D@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_thousands_sep, 4)
char __thiscall numpunct_char_thousands_sep(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_thousands_sep(this);
}

/* ?do_grouping@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_grouping, 8)
#define call_numpunct_char_do_grouping(this, ret) CALL_VTBL_FUNC(this, 12, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
basic_string_char* __thiscall numpunct_char_do_grouping(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_char_ctor_cstr(ret, this->grouping);
}

/* ?grouping@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_grouping, 8)
basic_string_char* __thiscall numpunct_char_grouping(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_grouping(this, ret);
}

/* ?do_falsename@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_falsename@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_falsename, 8)
#define call_numpunct_char_do_falsename(this, ret) CALL_VTBL_FUNC(this, 16, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
basic_string_char* __thiscall numpunct_char_do_falsename(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_char_ctor_cstr(ret, this->false_name);
}

/* ?falsename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?falsename@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_falsename, 8)
basic_string_char* __thiscall numpunct_char_falsename(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_falsename(this, ret);
}

/* ?do_truename@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_truename@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_truename, 8)
#define call_numpunct_char_do_truename(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
basic_string_char* __thiscall numpunct_char_do_truename(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_char_ctor_cstr(ret, this->true_name);
}

/* ?truename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?truename@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_truename, 8)
basic_string_char* __thiscall numpunct_char_truename(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_truename(this, ret);
}

/* ?id@?$numpunct@_W@std@@2V0locale@2@A */
static locale_id numpunct_wchar_id = {0};
/* ?id@?$numpunct@G@std@@2V0locale@2@A */
locale_id numpunct_short_id = {0};

/* ??_7?$numpunct@_W@std@@6B@ */
extern const vtable_ptr MSVCP_numpunct_wchar_vtable;
/* ??_7?$numpunct@G@std@@6B@ */
extern const vtable_ptr MSVCP_numpunct_short_vtable;

/* ?_Init@?$numpunct@_W@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@_W@std@@IEAAXAEBV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@G@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@G@std@@IEAAXAEBV_Locinfo@2@_N@Z */
static void numpunct_wchar__Init(numpunct_wchar *this,
        const _Locinfo *locinfo, MSVCP_bool isdef)
{
    const char *to_convert;
    _Cvtvec cvt;
    int len;

    TRACE("(%p %p %d)\n", this, locinfo, isdef);

    _Locinfo__Getcvt(locinfo, &cvt);

    to_convert = _Locinfo__Getfalse(locinfo);
    len = MultiByteToWideChar(cvt.page, 0, to_convert, -1, NULL, 0);
    this->false_name = MSVCRT_operator_new(len*sizeof(WCHAR));
    if(this->false_name)
        MultiByteToWideChar(cvt.page, 0, to_convert, -1,
                (wchar_t*)this->false_name, len);

    to_convert = _Locinfo__Gettrue(locinfo);
    len = MultiByteToWideChar(cvt.page, 0, to_convert, -1, NULL, 0);
    this->true_name = MSVCRT_operator_new(len*sizeof(WCHAR));
    if(this->true_name)
        MultiByteToWideChar(cvt.page, 0, to_convert, -1,
                (wchar_t*)this->true_name, len);

    if(isdef) {
        this->grouping = MSVCRT_operator_new(1);
        if(this->grouping)
            *(char*)this->grouping = 0;

        this->dp = '.';
        this->sep = ',';
    } else {
        const struct lconv *lc = _Locinfo__Getlconv(locinfo);

        len = strlen(lc->grouping)+1;
        this->grouping = MSVCRT_operator_new(len);
        if(this->grouping)
            memcpy((char*)this->grouping, lc->grouping, len);

        this->dp = lc->decimal_point[0];
        this->sep = lc->thousands_sep[0];
    }

    if(!this->false_name || !this->true_name || !this->grouping) {
        MSVCRT_operator_delete((char*)this->grouping);
        MSVCRT_operator_delete((wchar_t*)this->false_name);
        MSVCRT_operator_delete((wchar_t*)this->true_name);

        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
}

/* ?_Tidy@?$numpunct@_W@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@_W@std@@AEAAXXZ */
/* ?_Tidy@?$numpunct@G@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@G@std@@AEAAXXZ */
static void numpunct_wchar__Tidy(numpunct_wchar *this)
{
    TRACE("(%p)\n", this);

    MSVCRT_operator_delete((char*)this->grouping);
    MSVCRT_operator_delete((wchar_t*)this->false_name);
    MSVCRT_operator_delete((wchar_t*)this->true_name);
}

/* ??0?$numpunct@_W@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@_W@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
static numpunct_wchar* numpunct_wchar_ctor_locinfo(numpunct_wchar *this,
        const _Locinfo *locinfo, MSVCP_size_t refs, MSVCP_bool usedef)
{
    TRACE("(%p %p %lu %d)\n", this, locinfo, refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_numpunct_wchar_vtable;
    numpunct_wchar__Init(this, locinfo, usedef);
    return this;
}

/* ??0?$numpunct@G@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@G@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
static numpunct_wchar* numpunct_short_ctor_locinfo(numpunct_wchar *this,
        const _Locinfo *locinfo, MSVCP_size_t refs, MSVCP_bool usedef)
{
    numpunct_wchar_ctor_locinfo(this, locinfo, refs, usedef);
    this->facet.vtable = &MSVCP_numpunct_short_vtable;
    return this;
}

/* ??0?$numpunct@_W@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@_W@std@@IEAA@PEBD_K_N@Z */
static numpunct_wchar* numpunct_wchar_ctor_name(numpunct_wchar *this,
        const char *name, MSVCP_size_t refs, MSVCP_bool usedef)
{
    _Locinfo locinfo;

    TRACE("(%p %s %lu %d)\n", this, debugstr_a(name), refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_numpunct_wchar_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    numpunct_wchar__Init(this, &locinfo, usedef);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$numpunct@G@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@G@std@@IEAA@PEBD_K_N@Z */
static numpunct_wchar* numpunct_short_ctor_name(numpunct_wchar *this,
        const char *name, MSVCP_size_t refs, MSVCP_bool usedef)
{
    numpunct_wchar_ctor_name(this, name, refs, usedef);
    this->facet.vtable = &MSVCP_numpunct_short_vtable;
    return this;
}

/* ??0?$numpunct@_W@std@@QAE@I@Z */
/* ??0?$numpunct@_W@std@@QEAA@_K@Z */
static numpunct_wchar* numpunct_wchar_ctor_refs(numpunct_wchar *this, MSVCP_size_t refs)
{
    TRACE("(%p %lu)\n", this, refs);
    return numpunct_wchar_ctor_name(this, "C", refs, FALSE);
}

/* ??0?$numpunct@G@std@@QAE@I@Z */
/* ??0?$numpunct@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(numpunct_short_ctor_refs, 8)
numpunct_wchar* __thiscall numpunct_short_ctor_refs(numpunct_wchar *this, MSVCP_size_t refs)
{
    numpunct_wchar_ctor_refs(this, refs);
    this->facet.vtable = &MSVCP_numpunct_short_vtable;
    return this;
}

/* ??_F?$numpunct@G@std@@QAEXXZ */
/* ??_F?$numpunct@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_short_ctor, 4)
numpunct_wchar* __thiscall numpunct_short_ctor(numpunct_wchar *this)
{
    return numpunct_short_ctor_refs(this, 0);
}

/* ??1?$numpunct@G@std@@UAE@XZ */
/* ??1?$numpunct@G@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_dtor, 4)
void __thiscall numpunct_wchar_dtor(numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    numpunct_wchar__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(numpunct_wchar_vector_dtor, 8)
numpunct_wchar* __thiscall numpunct_wchar_vector_dtor(numpunct_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            numpunct_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        numpunct_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t numpunct_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(numpunct_wchar));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        numpunct_wchar_ctor_name((numpunct_wchar*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

static numpunct_wchar* numpunct_wchar_use_facet(const locale *loc)
{
    static numpunct_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_wchar__Getcat(&fac, loc);
    obj = (numpunct_wchar*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t numpunct_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(numpunct_wchar));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        numpunct_short_ctor_name((numpunct_wchar*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

static numpunct_wchar* numpunct_short_use_facet(const locale *loc)
{
    static numpunct_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_short__Getcat(&fac, loc);
    obj = (numpunct_wchar*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_decimal_point@?$numpunct@_W@std@@MBE_WXZ */
/* ?do_decimal_point@?$numpunct@_W@std@@MEBA_WXZ */
/* ?do_decimal_point@?$numpunct@G@std@@MBEGXZ */
/* ?do_decimal_point@?$numpunct@G@std@@MEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_decimal_point, 4)
#define call_numpunct_wchar_do_decimal_point(this) CALL_VTBL_FUNC(this, 4, \
        wchar_t, (const numpunct_wchar *this), (this))
wchar_t __thiscall numpunct_wchar_do_decimal_point(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->dp;
}

/* ?decimal_point@?$numpunct@_W@std@@QBE_WXZ */
/* ?decimal_point@?$numpunct@_W@std@@QEBA_WXZ */
/* ?decimal_point@?$numpunct@G@std@@QBEGXZ */
/* ?decimal_point@?$numpunct@G@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_decimal_point, 4)
wchar_t __thiscall numpunct_wchar_decimal_point(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_decimal_point(this);
}

/* ?do_thousands_sep@?$numpunct@_W@std@@MBE_WXZ */
/* ?do_thousands_sep@?$numpunct@_W@std@@MEBA_WXZ */
/* ?do_thousands_sep@?$numpunct@G@std@@MBEGXZ */
/* ?do_thousands_sep@?$numpunct@G@std@@MEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_thousands_sep, 4)
#define call_numpunct_wchar_do_thousands_sep(this) CALL_VTBL_FUNC(this, 8, \
        wchar_t, (const numpunct_wchar *this), (this))
wchar_t __thiscall numpunct_wchar_do_thousands_sep(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->sep;
}

/* ?thousands_sep@?$numpunct@_W@std@@QBE_WXZ */
/* ?thousands_sep@?$numpunct@_W@std@@QEBA_WXZ */
/* ?thousands_sep@?$numpunct@G@std@@QBEGXZ */
/* ?thousands_sep@?$numpunct@G@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_thousands_sep, 4)
wchar_t __thiscall numpunct_wchar_thousands_sep(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_thousands_sep(this);
}

/* ?do_grouping@?$numpunct@_W@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@_W@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@G@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@G@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_grouping, 8)
#define call_numpunct_wchar_do_grouping(this, ret) CALL_VTBL_FUNC(this, 12, \
        basic_string_char*, (const numpunct_wchar*, basic_string_char*), (this, ret))
basic_string_char* __thiscall numpunct_wchar_do_grouping(const numpunct_wchar *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_char_ctor_cstr(ret, this->grouping);
}

/* ?grouping@?$numpunct@_W@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@_W@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@G@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@G@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_grouping, 8)
basic_string_char* __thiscall numpunct_wchar_grouping(const numpunct_wchar *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_grouping(this, ret);
}

/* ?do_falsename@?$numpunct@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_falsename@?$numpunct@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_falsename@?$numpunct@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?do_falsename@?$numpunct@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_falsename, 8)
#define call_numpunct_wchar_do_falsename(this, ret) CALL_VTBL_FUNC(this, 16, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
basic_string_wchar* __thiscall numpunct_wchar_do_falsename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_wchar_ctor_cstr(ret, this->false_name);
}

/* ?falsename@?$numpunct@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?falsename@?$numpunct@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?falsename@?$numpunct@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?falsename@?$numpunct@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_falsename, 8)
basic_string_wchar* __thiscall numpunct_wchar_falsename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_falsename(this, ret);
}

/* ?do_truename@?$numpunct@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_truename@?$numpunct@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_truename@?$numpunct@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?do_truename@?$numpunct@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_truename, 8)
#define call_numpunct_wchar_do_truename(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
basic_string_wchar* __thiscall numpunct_wchar_do_truename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return basic_string_wchar_ctor_cstr(ret, this->true_name);
}

/* ?truename@?$numpunct@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?truename@?$numpunct@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?truename@?$numpunct@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?truename@?$numpunct@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_truename, 8)
basic_string_wchar* __thiscall numpunct_wchar_truename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_truename(this, ret);
}

double __cdecl _Stod(const char *buf, char **buf_end, LONG exp)
{
    double ret = strtod(buf, buf_end);

    if(exp)
        ret *= pow(10, exp);
    return ret;
}

static double _Stodx(const char *buf, char **buf_end, LONG exp, int *err)
{
    double ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _Stod(buf, buf_end, exp);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

float __cdecl _Stof(const char *buf, char **buf_end, LONG exp)
{
    return _Stod(buf, buf_end, exp);
}

static float _Stofx(const char *buf, char **buf_end, LONG exp, int *err)
{
    return _Stodx(buf, buf_end, exp, err);
}

static __int64 _Stollx(const char *buf, char **buf_end, int base, int *err)
{
    __int64 ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _strtoi64(buf, buf_end, base);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

static LONG _Stolx(const char *buf, char **buf_end, int base, int *err)
{
    __int64 i = _Stollx(buf, buf_end, base, err);
    if(!*err && i!=(__int64)((LONG)i))
        *err = ERANGE;
    return i;
}

static unsigned __int64 _Stoullx(const char *buf, char **buf_end, int base, int *err)
{
    unsigned __int64 ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _strtoui64(buf, buf_end, base);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

static ULONG _Stoulx(const char *buf, char **buf_end, int base, int *err)
{
    unsigned __int64 i = _Stoullx(buf[0]=='-' ? buf+1 : buf, buf_end, base, err);
    if(!*err && i!=(unsigned __int64)((ULONG)i))
        *err = ERANGE;
    return buf[0]=='-' ? -i : i;
}

/* ?id@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
static locale_id num_get_wchar_id = {0};
/* ?id@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id num_get_short_id = {0};

/* ??_7?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_get_wchar_vtable;
/* ??_7?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_get_short_vtable;

/* ?_Init@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar__Init, 8)
void __thiscall num_get_wchar__Init(num_get *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static num_get* num_get_wchar_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_num_get_wchar_vtable;

    num_get_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor_locinfo, 12)
num_get* __thiscall num_get_short_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    num_get_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &MSVCP_num_get_short_vtable;
    return this;
}

/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
static num_get* num_get_wchar_ctor_refs(num_get *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_get_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor_refs, 8)
num_get* __thiscall num_get_short_ctor_refs(num_get *this, MSVCP_size_t refs)
{
    num_get_wchar_ctor_refs(this, refs);
    this->facet.vtable = &MSVCP_num_get_short_vtable;
    return this;
}

/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor, 4)
num_get* __thiscall num_get_short_ctor(num_get *this)
{
    return num_get_short_ctor_refs(this, 0);
}

/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_get_wchar_dtor, 4)
void __thiscall num_get_wchar_dtor(num_get *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_get_wchar_vector_dtor, 8)
num_get* __thiscall num_get_wchar_vector_dtor(num_get *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_get_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        num_get_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_get_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(num_get));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        num_get_wchar_ctor_locinfo((num_get*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

static num_get* num_get_wchar_use_facet(const locale *loc)
{
        static num_get *obj = NULL;

        _Lockit lock;
        const locale_facet *fac;

        _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
        fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_wchar_id), TRUE);
        if(fac) {
            _Lockit_dtor(&lock);
            return (num_get*)fac;
        }

        if(obj) {
            _Lockit_dtor(&lock);
            return obj;
        }

        num_get_wchar__Getcat(&fac, loc);
        obj = (num_get*)fac;
        locale_facet__Incref(&obj->facet);
        locale_facet_register(&obj->facet);
        _Lockit_dtor(&lock);

        return obj;
}

/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_get_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        num_get_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &MSVCP_num_get_short_vtable;
    }

    return LC_NUMERIC;
}

num_get* num_get_short_use_facet(const locale *loc)
{
    static num_get *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_get*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_get_short__Getcat(&fac, loc);
    obj = (num_get*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

static inline wchar_t mb_to_wc(char ch, const _Cvtvec *cvt)
{
    int state = 0;
    wchar_t ret;

    return _Mbrtowc(&ret, &ch, 1, &state, cvt) == 1 ? ret : 0;
}

static int num_get__Getffld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
        istreambuf_iterator_wchar *last, const locale *loc, numpunct_wchar *numpunct)
{
    int i, exp = 0;
    char *dest_beg = dest, *num_end = dest+25, *exp_end = dest+31;
    wchar_t digits[11], *digits_pos;
    BOOL error = FALSE, got_digit = FALSE, got_nonzero = FALSE;

    TRACE("(%p %p %p %p)\n", dest, first, last, loc);

    for(i=0; i<10; i++)
        digits[i] = mb_to_wc('0'+i, &this->cvt);
    digits[10] = 0;

    istreambuf_iterator_wchar_val(first);
    /* get sign */
    if(first->strbuf && first->val==mb_to_wc('-', &this->cvt)) {
        *dest++ = '-';
        istreambuf_iterator_wchar_inc(first);
    }else if(first->strbuf && first->val==mb_to_wc('+', &this->cvt)) {
        *dest++ = '+';
        istreambuf_iterator_wchar_inc(first);
    }

    /* read numbers before decimal */
    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val))) {
            break;
        }else {
            got_digit = TRUE; /* found a digit, zero or non-zero */
            /* write digit to dest if not a leading zero (to not waste dest buffer) */
            if(!got_nonzero && first->val == digits[0])
                continue;
            got_nonzero = TRUE;
            if(dest < num_end)
                *dest++ = '0'+digits_pos-digits;
            else
                exp++; /* too many digits, just multiply by 10 */
        }
    }
    /* if all leading zeroes, we didn't write anything so put a zero we check for a decimal */
    if(got_digit && !got_nonzero)
        *dest++ = '0';

    /* get decimal, if any */
    if(first->strbuf && first->val==numpunct_wchar_decimal_point(numpunct)) {
        if(dest < num_end)
            *dest++ = *localeconv()->decimal_point;
        istreambuf_iterator_wchar_inc(first);
    }

    /* read non-grouped after decimal */
    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val)))
            break;
        else if(dest<num_end) {
            got_digit = TRUE;
            *dest++ = '0'+digits_pos-digits;
        }
    }

    /* read exponent, if any */
    if(first->strbuf && (first->val==mb_to_wc('e', &this->cvt) || first->val==mb_to_wc('E', &this->cvt))) {
        *dest++ = 'e';
        istreambuf_iterator_wchar_inc(first);

        if(first->strbuf && first->val==mb_to_wc('-', &this->cvt)) {
            *dest++ = '-';
            istreambuf_iterator_wchar_inc(first);
        }else if(first->strbuf && first->val==mb_to_wc('+', &this->cvt)) {
            *dest++ = '+';
            istreambuf_iterator_wchar_inc(first);
        }

        got_digit = got_nonzero = FALSE;
        error = TRUE;
        /* skip any leading zeroes */
        for(; first->strbuf && first->val==digits[0]; istreambuf_iterator_wchar_inc(first))
            error = FALSE;

        for(; first->strbuf && (digits_pos = wcschr(digits, first->val)); istreambuf_iterator_wchar_inc(first)) {
            got_digit = got_nonzero = TRUE; /* leading zeroes would have been skipped, so first digit is non-zero */
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0'+digits_pos-digits;
        }

        /* if just found zeroes for exponent, use that */
        if(got_digit && !got_nonzero)
        {
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0';
        }
    }

    if(error) {
        *dest_beg = '\0';
        return 0;
    }
    *dest++ = '\0';
    return exp;
}

static int num_get__Getifld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, int fmtflags, const locale *loc, numpunct_wchar *numpunct)
{
    wchar_t digits[23], *digits_pos;
    int i, basefield, base;
    char *dest_beg = dest, *dest_end = dest+24;
    BOOL error = TRUE, dest_empty = TRUE, found_zero = FALSE;

    TRACE("(%p %p %p %04x %p)\n", dest, first, last, fmtflags, loc);

    for(i=0; i<10; i++)
        digits[i] = mb_to_wc('0'+i, &this->cvt);
    for(i=0; i<6; i++) {
        digits[10+i] = mb_to_wc('a'+i, &this->cvt);
        digits[16+i] = mb_to_wc('A'+i, &this->cvt);
    }

    basefield = fmtflags & FMTFLAG_basefield;
    if(basefield == FMTFLAG_oct)
        base = 8;
    else if(basefield == FMTFLAG_hex)
        base = 22; /* equal to the size of digits buffer */
    else if(!basefield)
        base = 0;
    else
        base = 10;

    istreambuf_iterator_wchar_val(first);
    if(first->strbuf && first->val==mb_to_wc('-', &this->cvt)) {
        *dest++ = '-';
        istreambuf_iterator_wchar_inc(first);
    }else if(first->strbuf && first->val==mb_to_wc('+', &this->cvt)) {
        *dest++ = '+';
        istreambuf_iterator_wchar_inc(first);
    }

    if(first->strbuf && first->val==digits[0]) {
        found_zero = TRUE;
        istreambuf_iterator_wchar_inc(first);
        if(first->strbuf && (first->val==mb_to_wc('x', &this->cvt) || first->val==mb_to_wc('X', &this->cvt))) {
            if(!base || base == 22) {
                found_zero = FALSE;
                istreambuf_iterator_wchar_inc(first);
                base = 22;
            }else {
                base = 10;
            }
        }else {
            error = FALSE;
            if(!base) base = 8;
        }
    }else {
        if(!base) base = 10;
    }
    digits[base] = 0;

    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val))) {
            break;
        }else {
            error = FALSE;
            if(dest_empty && first->val == digits[0]) {
                found_zero = TRUE;
                continue;
            }
            dest_empty = FALSE;
            /* skip digits that can't be copied to dest buffer, other
             * functions are responsible for detecting overflows */
            if(dest < dest_end)
                *dest++ = (digits_pos-digits<10 ? '0'+digits_pos-digits :
                        (digits_pos-digits<16 ? 'a'+digits_pos-digits-10 :
                         'A'+digits_pos-digits-16));
        }
    }

    if(error) {
        if (found_zero)
            *dest++ = '0';
        else
            dest = dest_beg;
    }else if(dest_empty)
        *dest++ = '0';
    *dest = '\0';

    return (base==22 ? 16 : base);
}

/* ?_Getifld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1HABVlocale@2@@Z */
/* ?_Getifld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1HAEBVlocale@2@@Z */
static int num_get_wchar__Getifld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, int fmtflags, const locale *loc)
{
    return num_get__Getifld(this, dest, first, last,
            fmtflags, loc, numpunct_wchar_use_facet(loc));
}

static istreambuf_iterator_wchar* num_get_do_get_void(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        void **pval, numpunct_wchar *numpunct)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, FMTFLAG_hex, &base->loc, numpunct), &err);
    if(v!=(unsigned __int64)((INT_PTR)v))
        *state |= IOSTATE_failbit;
    else if(end!=tmp && !err)
        *pval = (void*)((INT_PTR)v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
#define call_num_get_wchar_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 4, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    return num_get_do_get_void(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    return num_get_do_get_void(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_void(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_double(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        double *pval, numpunct_wchar *numpunct)
{
    double v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stodx(tmp, &end, num_get__Getffld(this, tmp, &first, &last, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
#define call_num_get_wchar_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_wchar_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    return num_get_do_get_double(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    return num_get_do_get_double(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ldouble,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ldouble(const num_get *this, istreambuf_iterator_wchar *ret,
        istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ldouble(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_double(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_float(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        float *pval, numpunct_wchar *numpunct)
{
    float v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stofx(tmp, &end, num_get__Getffld(this, tmp, &first,
                &last, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
#define call_num_get_wchar_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    return num_get_do_get_float(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    return num_get_do_get_float(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_float(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_uint64(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        ULONGLONG *pval, numpunct_wchar *numpunct)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
#define call_num_get_wchar_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONGLONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONGLONG *pval)
{
    return num_get_do_get_uint64(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONGLONG *pval)
{
    return num_get_do_get_uint64(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONGLONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_uint64(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_int64(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        LONGLONG *pval, numpunct_wchar *numpunct)
{
    __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stollx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
#define call_num_get_wchar_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONGLONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONGLONG *pval)
{
    return num_get_do_get_int64(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONGLONG *pval)
{
    return num_get_do_get_int64(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONGLONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_int64(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_ulong(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        ULONG *pval, numpunct_wchar *numpunct)
{
    ULONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoulx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
#define call_num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    return num_get_do_get_ulong(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    return num_get_do_get_ulong(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_long(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        LONG *pval, numpunct_wchar *numpunct)
{
    LONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stolx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, &base->loc, numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
#define call_num_get_wchar_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    return num_get_do_get_long(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    return num_get_do_get_long(this, ret, first, last, base,
        state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_long(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
#define call_num_get_wchar_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_short_do_get_ulong(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_uint(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
#define call_num_get_wchar_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    ULONG v;
    char tmp[25], *beg, *end;
    int err, b;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    b = num_get_wchar__Getifld(this, tmp,
            &first, &last, base->fmtfl, &base->loc);
    beg = tmp + (tmp[0]=='-' ? 1 : 0);
    v = _Stoulx(beg, &end, b, &err);

    if(v != (ULONG)((unsigned short)v))
        *state |= IOSTATE_failbit;
    else if(end!=beg && !err)
        *pval = (tmp[0]=='-' ? -((unsigned short)v) : v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    FIXME("(%p %p %p %p %p) stub\n", this, ret, base, state, pval);
    return ret;
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAG@ */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ushort(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_bool(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        MSVCP_bool *pval, numpunct_wchar *numpunct)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        basic_string_wchar false_bstr, true_bstr;
        const wchar_t *pfalse, *ptrue;

        numpunct_wchar_falsename(numpunct, &false_bstr);
        numpunct_wchar_truename(numpunct, &true_bstr);
        pfalse = basic_string_wchar_c_str(&false_bstr);
        ptrue = basic_string_wchar_c_str(&true_bstr);

        for(istreambuf_iterator_wchar_val(&first); first.strbuf;) {
            if(pfalse && *pfalse && first.val!=*pfalse)
                pfalse = NULL;
            if(ptrue && *ptrue && first.val!=*ptrue)
                ptrue = NULL;

            if(pfalse && *pfalse && ptrue && !*ptrue)
                ptrue = NULL;
            if(ptrue && *ptrue && pfalse && !*pfalse)
                pfalse = NULL;

            if(pfalse)
                pfalse++;
            if(ptrue)
                ptrue++;

            if(pfalse || ptrue)
                istreambuf_iterator_wchar_inc(&first);

            if((!pfalse || !*pfalse) && (!ptrue || !*ptrue))
                break;
        }

        if(ptrue)
            *pval = TRUE;
        else if(pfalse)
            *pval = FALSE;
        else
            *state |= IOSTATE_failbit;

        basic_string_wchar_dtor(&false_bstr);
        basic_string_wchar_dtor(&true_bstr);
    }else {
        char tmp[25], *end;
        int err;
        LONG v = _Stolx(tmp, &end, num_get__Getifld(this, tmp, &first,
                    &last, base->fmtfl, &base->loc, numpunct), &err);

        if(end!=tmp && err==0 && (v==0 || v==1))
            *pval = v;
        else
            *state |= IOSTATE_failbit;
    }

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;
    memcpy(ret, &first, sizeof(first));
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
#define call_num_get_wchar_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, MSVCP_bool*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, MSVCP_bool *pval)
{
    return num_get_do_get_bool(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(&base->loc));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, MSVCP_bool *pval)
{
    return num_get_do_get_bool(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(&base->loc));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, MSVCP_bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_bool(this, ret, first, last, base, state, pval);
}

/* ?id@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id num_get_char_id = {0};

/* ??_7?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_get_char_vtable;

/* ?_Init@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_get_char__Init, 8)
void __thiscall num_get_char__Init(num_get *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor_locinfo, 12)
num_get* __thiscall num_get_char_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_num_get_char_vtable;

    num_get_char__Init(this, locinfo);
    return this;
}

/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor_refs, 8)
num_get* __thiscall num_get_char_ctor_refs(num_get *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_get_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor, 4)
num_get* __thiscall num_get_char_ctor(num_get *this)
{
    return num_get_char_ctor_refs(this, 0);
}

/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_get_char_dtor, 4)
void __thiscall num_get_char_dtor(num_get *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_get_char_vector_dtor, 8)
num_get* __thiscall num_get_char_vector_dtor(num_get *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_get_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        num_get_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_get_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(num_get));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        num_get_char_ctor_locinfo((num_get*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

num_get* num_get_char_use_facet(const locale *loc)
{
    static num_get *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_get*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_get_char__Getcat(&fac, loc);
    obj = (num_get*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getffld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1ABVlocale@2@@Z */
/* ?_Getffld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1AEBVlocale@2@@Z */
/* Copies number to dest buffer, validates grouping and skips separators.
 * Updates first so it points past the number, all digits are skipped.
 * Returns how exponent needs to changed.
 * Size of dest buffer is not specified, assuming it's not smaller than 32:
 * strlen(+0.e+) + 22(digits) + 4(expontent) + 1(nullbyte)
 */
static int num_get_char__Getffld(const num_get *this, char *dest, istreambuf_iterator_char *first,
        istreambuf_iterator_char *last, const locale *loc)
{
    numpunct_char *numpunct = numpunct_char_use_facet(loc);
    int exp = 0;
    char *dest_beg = dest, *num_end = dest+25, *exp_end = dest+31;
    BOOL error = FALSE, got_digit = FALSE, got_nonzero = FALSE;

    TRACE("(%p %p %p %p)\n", dest, first, last, loc);

    istreambuf_iterator_char_val(first);
    /* get sign */
    if(first->strbuf && (first->val=='-' || first->val=='+')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);
    }


    /* read numbers before decimal */
    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(first->val<'0' || first->val>'9') {
            break;
        }else {
            got_digit = TRUE; /* found a digit, zero or non-zero */
            /* write digit to dest if not a leading zero (to not waste dest buffer) */
            if(!got_nonzero && first->val == '0')
                continue;
            got_nonzero = TRUE;
            if(dest < num_end)
                *dest++ = first->val;
            else
                exp++; /* too many digits, just multiply by 10 */
        }
    }
    /* if all leading zeroes, we didn't write anything so put a zero we check for a decimal */
    if(got_digit && !got_nonzero)
        *dest++ = '0';

    /* get decimal, if any */
    if(first->strbuf && first->val==numpunct_char_decimal_point(numpunct)) {
        if(dest < num_end)
            *dest++ = *localeconv()->decimal_point;
        istreambuf_iterator_char_inc(first);
    }

    /* read non-grouped after decimal */
    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(first->val<'0' || first->val>'9')
            break;
        else if(dest<num_end) {
            got_digit = TRUE;
            *dest++ = first->val;
        }
    }

    /* read exponent, if any */
    if(first->strbuf && (first->val=='e' || first->val=='E')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);

        if(first->strbuf && (first->val=='-' || first->val=='+')) {
            *dest++ = first->val;
            istreambuf_iterator_char_inc(first);
        }

        got_digit = got_nonzero = FALSE;
        error = TRUE;
        /* skip any leading zeroes */
        for(; first->strbuf && first->val=='0'; istreambuf_iterator_char_inc(first))
            got_digit = TRUE;

        for(; first->strbuf && first->val>='0' && first->val<='9'; istreambuf_iterator_char_inc(first)) {
            got_digit = got_nonzero = TRUE; /* leading zeroes would have been skipped, so first digit is non-zero */
            error = FALSE;
            if(dest<exp_end)
                *dest++ = first->val;
        }

        /* if just found zeroes for exponent, use that */
        if(got_digit && !got_nonzero)
        {
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0';
        }
    }

    if(error) {
        *dest_beg = '\0';
        return 0;
    }
    *dest++ = '\0';
    return exp;
}

/* ?_Getifld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1HABVlocale@2@@Z */
/* ?_Getifld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1HAEBVlocale@2@@Z */
/* Copies number to dest buffer, validates grouping and skips separators.
 * Updates first so it points past the number, all digits are skipped.
 * Returns number base (8, 10 or 16).
 * Size of dest buffer is not specified, assuming it's not smaller than 25:
 * 22(8^22>2^64)+1(detect overflows)+1(sign)+1(nullbyte) = 25
 */
static int num_get_char__Getifld(const num_get *this, char *dest, istreambuf_iterator_char *first,
        istreambuf_iterator_char *last, int fmtflags, const locale *loc)
{
    static const char digits[] = "0123456789abcdefABCDEF";

    int basefield, base;
    char *dest_beg = dest, *dest_end = dest+24;
    BOOL error = TRUE, dest_empty = TRUE, found_zero = FALSE;

    TRACE("(%p %p %p %04x %p)\n", dest, first, last, fmtflags, loc);

    basefield = fmtflags & FMTFLAG_basefield;
    if(basefield == FMTFLAG_oct)
        base = 8;
    else if(basefield == FMTFLAG_hex)
        base = 22; /* equal to the size of digits buffer */
    else if(!basefield)
        base = 0;
    else
        base = 10;

    istreambuf_iterator_char_val(first);
    if(first->strbuf && (first->val=='-' || first->val=='+')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);
    }

    if(first->strbuf && first->val=='0') {
        found_zero = TRUE;
        istreambuf_iterator_char_inc(first);
        if(first->strbuf && (first->val=='x' || first->val=='X')) {
            if(!base || base == 22) {
                found_zero = FALSE;
                istreambuf_iterator_char_inc(first);
                base = 22;
            }else {
                base = 10;
            }
        }else {
            error = FALSE;
            if(!base) base = 8;
        }
    }else {
        if (!base) base = 10;
    }

    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(!memchr(digits, first->val, base)) {
            break;
        }else {
            error = FALSE;
            if(dest_empty && first->val == '0')
            {
                found_zero = TRUE;
                continue;
            }
            dest_empty = FALSE;
            /* skip digits that can't be copied to dest buffer, other
             * functions are responsible for detecting overflows */
            if(dest < dest_end)
                *dest++ = first->val;
        }
    }

    if(error) {
        if (found_zero)
            *dest++ = '0';
        else
            dest = dest_beg;
    }else if(dest_empty)
        *dest++ = '0';
    *dest = '\0';

    return (base==22 ? 16 : base);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
#define call_num_get_char_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 4, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_void,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_void(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, void **pval)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, FMTFLAG_hex, &base->loc), &err);
    if(v!=(unsigned __int64)((INT_PTR)v))
        *state |= IOSTATE_failbit;
    else if(end!=tmp && !err)
        *pval = (void*)((INT_PTR)v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_void,36)
istreambuf_iterator_char *__thiscall num_get_char_get_void(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, void **pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_void(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
#define call_num_get_char_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_char_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_double,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_double(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    double v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stodx(tmp, &end, num_get_char__Getffld(this, tmp, &first, &last, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ldouble,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ldouble(const num_get *this, istreambuf_iterator_char *ret,
        istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ldouble(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_double,36)
istreambuf_iterator_char *__thiscall num_get_char_get_double(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_double(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
#define call_num_get_char_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_float,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_float(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, float *pval)
{
    float v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stofx(tmp, &end, num_get_char__Getffld(this, tmp, &first, &last, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_float,36)
istreambuf_iterator_char *__thiscall num_get_char_get_float(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, float *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_float(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
#define call_num_get_char_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONGLONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_uint64,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_uint64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONGLONG *pval)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_uint64,36)
istreambuf_iterator_char *__thiscall num_get_char_get_uint64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONGLONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_uint64(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
#define call_num_get_char_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONGLONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_int64,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_int64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONGLONG *pval)
{
    __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stollx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_int64,36)
istreambuf_iterator_char *__thiscall num_get_char_get_int64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONGLONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_int64(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
#define call_num_get_char_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_ulong,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_ulong(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONG *pval)
{
    ULONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoulx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ulong,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ulong(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ulong(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
#define call_num_get_char_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_long,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_long(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONG *pval)
{
    LONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stolx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, &base->loc), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_long,36)
istreambuf_iterator_char *__thiscall num_get_char_get_long(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_long(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
#define call_num_get_char_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_uint,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_uint(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_char_do_get_ulong(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_uint,36)
istreambuf_iterator_char *__thiscall num_get_char_get_uint(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_uint(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
#define call_num_get_char_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_ushort,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_ushort(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned short *pval)
{
    ULONG v;
    char tmp[25], *beg, *end;
    int err, b;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    b = num_get_char__Getifld(this, tmp,
            &first, &last, base->fmtfl, &base->loc);
    beg = tmp + (tmp[0]=='-' ? 1 : 0);
    v = _Stoulx(beg, &end, b, &err);

    if(v != (ULONG)((unsigned short)v))
        *state |= IOSTATE_failbit;
    else if(end!=beg && !err)
        *pval = (tmp[0]=='-' ? -((unsigned short)v) : v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ushort,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ushort(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned short *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ushort(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
#define call_num_get_char_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, MSVCP_bool*), \
        (this, ret, first, last, base, state, pval))
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_bool,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_bool(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, MSVCP_bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_char *numpunct = numpunct_char_use_facet(&base->loc);
        basic_string_char false_bstr, true_bstr;
        const char *pfalse, *ptrue;

        numpunct_char_falsename(numpunct, &false_bstr);
        numpunct_char_truename(numpunct, &true_bstr);
        pfalse = basic_string_char_c_str(&false_bstr);
        ptrue = basic_string_char_c_str(&true_bstr);

        for(istreambuf_iterator_char_val(&first); first.strbuf;) {
            if(pfalse && *pfalse && first.val!=*pfalse)
                pfalse = NULL;
            if(ptrue && *ptrue && first.val!=*ptrue)
                ptrue = NULL;

            if(pfalse && *pfalse && ptrue && !*ptrue)
                ptrue = NULL;
            if(ptrue && *ptrue && pfalse && !*pfalse)
                pfalse = NULL;

            if(pfalse)
                pfalse++;
            if(ptrue)
                ptrue++;

            if(pfalse || ptrue)
                istreambuf_iterator_char_inc(&first);

            if((!pfalse || !*pfalse) && (!ptrue || !*ptrue))
                break;
        }

        if(ptrue)
            *pval = TRUE;
        else if(pfalse)
            *pval = FALSE;
        else
            *state |= IOSTATE_failbit;

        basic_string_char_dtor(&false_bstr);
        basic_string_char_dtor(&true_bstr);
    }else {
        char tmp[25], *end;
        int err;
        LONG v = _Stolx(tmp, &end, num_get_char__Getifld(this, tmp,
                    &first, &last, base->fmtfl, &base->loc), &err);

        if(end!=tmp && err==0 && (v==0 || v==1))
            *pval = v;
        else
            *state |= IOSTATE_failbit;
    }

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;
    memcpy(ret, &first, sizeof(first));
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_bool,36)
istreambuf_iterator_char *__thiscall num_get_char_get_bool(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, MSVCP_bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_bool(this, ret, first, last, base, state, pval);
}

/* ?id@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id num_put_char_id = {0};

/* num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_put_char_vtable;

/* ?_Init@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_put_char__Init, 8)
void __thiscall num_put_char__Init(num_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor_locinfo, 12)
num_put* __thiscall num_put_char_ctor_locinfo(num_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %ld)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_num_put_char_vtable;

    num_put_char__Init(this, locinfo);
    return this;
}

/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor_refs, 8)
num_put* __thiscall num_put_char_ctor_refs(num_put *this, MSVCP_size_t refs)
{
     _Locinfo locinfo;

     TRACE("(%p %lu)\n", this, refs);

     _Locinfo_ctor(&locinfo);
     num_put_char_ctor_locinfo(this, &locinfo, refs);
     _Locinfo_dtor(&locinfo);
     return this;
}

/* ??_F?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor, 4)
num_put* __thiscall num_put_char_ctor(num_put *this)
{
    return num_put_char_ctor_refs(this, 0);
}

/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_put_char_dtor, 4)
void __thiscall num_put_char_dtor(num_put *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_put_char_vector_dtor, 8)
num_put* __thiscall num_put_char_vector_dtor(num_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_put_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        num_put_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_put_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(num_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        num_put_char_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

num_put* num_put_char_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_char__Getcat(&fac, loc);
    obj = (num_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Putc@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PEBD_K@Z */
static ostreambuf_iterator_char* num_put_char__Putc(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, const char *ptr, MSVCP_size_t count)
{
    TRACE("(%p %p %p %ld)\n", this, ret, ptr, count);

    for(; count>0; count--)
        ostreambuf_iterator_char_put(&dest, *ptr++);

    *ret = dest;
    return ret;
}

/* ?_Rep@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@DI@Z */
/* ?_Rep@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@D_K@Z */
static ostreambuf_iterator_char* num_put_char__Rep(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, char c, MSVCP_size_t count)
{
    TRACE("(%p %p %d %ld)\n", this, ret, c, count);

    for(; count>0; count--)
        ostreambuf_iterator_char_put(&dest, c);

    *ret = dest;
    return ret;
}

/* ?_Ffmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAPEADPEADDH@Z */
static char* num_put_char__Ffmt(const num_put *this, char *fmt, char spec, int fmtfl)
{
    int type = fmtfl & FMTFLAG_floatfield;
    char *p = fmt;

    TRACE("(%p %p %d %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';
    *p++ = '.';
    *p++ = '*';
    if(spec)
        *p++ = spec;

    if(type == FMTFLAG_fixed)
        *p++ = 'f';
    else if(type == FMTFLAG_scientific)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'E' : 'e';
    else if(type == (FMTFLAG_fixed|FMTFLAG_scientific))
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'A' : 'a';
    else
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'G' : 'g';

    *p++ = '\0';
    return fmt;
}

/* TODO: This function should be removed when num_put_char__Fput is implemented */
static ostreambuf_iterator_char* num_put_char_fput(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, char *buf, MSVCP_size_t count)
{
    numpunct_char *numpunct = numpunct_char_use_facet(&base->loc);
    char *p, sep = *localeconv()->decimal_point;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    MSVCP_size_t pad;

    TRACE("(%p %p %p %d %s %ld)\n", this, ret, base, fill, buf, count);

    /* Change decimal point */
    for(p=buf; p<buf+count; p++) {
        if(*p == sep) {
            *p = numpunct_char_decimal_point(numpunct);
            break;
        }
    }
    p--;

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_char__Putc(this, &dest, dest, buf, 1);
        buf++;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_char__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }
    num_put_char__Putc(this, &dest, dest, buf, count);
    return num_put_char__Rep(this, ret, dest, fill, pad);
}

/* ?_Ifmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
static char* num_put_char__Ifmt(const num_put *this, char *fmt, const char *spec, int fmtfl)
{
    int base = fmtfl & FMTFLAG_basefield;
    char *p = fmt;

    TRACE("(%p %p %p %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';

    *p++ = *spec++;
    if(*spec == 'l')
        *p++ = *spec++;

    if(base == FMTFLAG_oct)
        *p++ = 'o';
    else if(base == FMTFLAG_hex)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'X' : 'x';
    else
        *p++ = *spec;

    *p++ = '\0';
    return fmt;
}

/* ?_Iput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPADI@Z */
/* ?_Iput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEAD_K@Z */
static ostreambuf_iterator_char* num_put_char__Iput(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, char *buf, MSVCP_size_t count)
{
    numpunct_char *numpunct = numpunct_char_use_facet(&base->loc);
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p, sep;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    MSVCP_size_t pad;

    TRACE("(%p %p %p %d %s %ld)\n", this, ret, base, fill, buf, count);

    /* Add separators to number */
    numpunct_char_grouping(numpunct, &grouping_bstr);
    grouping = basic_string_char_c_str(&grouping_bstr);
    sep = grouping[0] ? numpunct_char_thousands_sep(numpunct) : '\0';

    for(p=buf+count-1; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = sep;
            count++;
        }
    }
    basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_char__Putc(this, &dest, dest, buf, 1);
        buf++;
    }else if((adjustfield & FMTFLAG_internal) && (buf[1]=='x' || buf[1]=='X')) {
        num_put_char__Putc(this, &dest, dest, buf, 2);
        buf += 2;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_char__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }
    num_put_char__Putc(this, &dest, dest, buf, count);
    return num_put_char__Rep(this, ret, dest, fill, pad);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DJ@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DJ@Z */
#define call_num_put_char_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, LONG), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_long, 28)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_long(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DJ@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DJ@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_long, 28)
ostreambuf_iterator_char* __thiscall num_put_char_put_long(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, LONG v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DK@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DK@Z */
#define call_num_put_char_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, ULONG), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ulong, 28)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_ulong(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DK@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DK@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_ulong, 28)
ostreambuf_iterator_char* __thiscall num_put_char_put_ulong(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, ULONG v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ulong(this, ret, dest, base, fill, v);
}

static inline streamsize get_precision(const ios_base *base)
{
    return base->prec <= 0 && !(base->fmtfl & FMTFLAG_fixed) ? 6 : base->prec;
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DN@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DN@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DO@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DO@Z */
#define call_num_put_char_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_char_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 8, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_double, 32)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_double(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;
    streamsize prec;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_char__Ffmt(this, fmt, '\0', base->fmtfl);
    prec = get_precision(base);
    size = _scprintf(fmt, prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = MSVCRT_operator_new(size*2);
    if(!tmp) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    num_put_char_fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, prec, v));
    MSVCRT_operator_delete(tmp);
    return ret;
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DN@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DN@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_double, 32)
ostreambuf_iterator_char* __thiscall num_put_char_put_double(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_double(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DO@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DO@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_ldouble, 32)
ostreambuf_iterator_char* __thiscall num_put_char_put_ldouble(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ldouble(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBX@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBX@Z */
#define call_num_put_char_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, const void*), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ptr, 28)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_ptr(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators beetwen every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBX@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBX@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_ptr, 28)
ostreambuf_iterator_char* __thiscall num_put_char_put_ptr(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const void *v)
{
    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ptr(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_J@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_J@Z */
#define call_num_put_char_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, __int64), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_int64, 32)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_int64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_J@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_J@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_int64, 32)
ostreambuf_iterator_char* __thiscall num_put_char_put_int64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_char_do_put_int64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_K@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_K@Z */
#define call_num_put_char_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, unsigned __int64), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_uint64, 32)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_uint64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_K@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_uint64, 32)
ostreambuf_iterator_char* __thiscall num_put_char_put_uint64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, unsigned __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_char_do_put_uint64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_N@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_N@Z */
#define call_num_put_char_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, MSVCP_bool), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_bool, 28)
ostreambuf_iterator_char* __thiscall num_put_char_do_put_bool(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, MSVCP_bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_char *numpunct = numpunct_char_use_facet(&base->loc);
        basic_string_char str;
        MSVCP_size_t pad, len;

        if(v)
            numpunct_char_truename(numpunct, &str);
        else
            numpunct_char_falsename(numpunct, &str);

        len = basic_string_char_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_char__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_char__Putc(this, &dest, dest, basic_string_char_c_str(&str), len);
        basic_string_char_dtor(&str);
        return num_put_char__Rep(this, ret, dest, fill, pad);
    }

    return num_put_char_put_long(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_N@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_N@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_put_bool, 28)
ostreambuf_iterator_char* __thiscall num_put_char_put_bool(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, MSVCP_bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_bool(this, ret, dest, base, fill, v);
}

/* ?id@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
static locale_id num_put_wchar_id = {0};
/* ?id@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id num_put_short_id = {0};

/* num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_put_wchar_vtable;
/* num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_num_put_short_vtable;

/* ?_Init@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar__Init, 8)
void __thiscall num_put_wchar__Init(num_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static num_put* num_put_wchar_ctor_locinfo(num_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %ld)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_num_put_wchar_vtable;

    num_put_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor_locinfo, 12)
num_put* __thiscall num_put_short_ctor_locinfo(num_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    num_put_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &MSVCP_num_put_short_vtable;
    return this;
}

/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
static num_put* num_put_wchar_ctor_refs(num_put *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor_refs, 8)
num_put* __thiscall num_put_short_ctor_refs(num_put *this, MSVCP_size_t refs)
{
    num_put_wchar_ctor_refs(this, refs);
    this->facet.vtable = &MSVCP_num_put_short_vtable;
    return this;
}

/* ??_F?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor, 4)
num_put* __thiscall num_put_short_ctor(num_put *this)
{
    return num_put_short_ctor_refs(this, 0);
}

/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_put_wchar_dtor, 4)
void __thiscall num_put_wchar_dtor(num_put *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_put_wchar_vector_dtor, 8)
num_put* __thiscall num_put_wchar_vector_dtor(num_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_put_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        num_put_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_put_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(num_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        num_put_wchar_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t num_put_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(num_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        num_put_short_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

static num_put* num_put_wchar_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_wchar__Getcat(&fac, loc);
    obj = (num_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

num_put* num_put_short_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_short__Getcat(&fac, loc);
    obj = (num_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PB_WI@Z */
/* ?_Put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PEB_W_K@Z */
/* ?_Put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PBGI@Z */
/* ?_Put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PEBG_K@Z */
static ostreambuf_iterator_wchar* num_put_wchar__Put(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, const wchar_t *ptr, MSVCP_size_t count)
{
    TRACE("(%p %p %s %ld)\n", this, ret, debugstr_wn(ptr, count), count);

    for(; count>0; count--)
        ostreambuf_iterator_wchar_put(&dest, *ptr++);

    *ret = dest;
    return ret;
}

/* ?_Putc@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PEBD_K@Z */
/* ?_Putc@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PEBD_K@Z */
static ostreambuf_iterator_wchar* num_put_wchar__Putc(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, const char *ptr, MSVCP_size_t count)
{
    int state = 0;
    wchar_t ch;

    TRACE("(%p %p %s %ld)\n", this, ret, debugstr_an(ptr, count), count);

    for(; count>0; count--) {
        if(_Mbrtowc(&ch, ptr++, 1, &state, &this->cvt) == 1)
            ostreambuf_iterator_wchar_put(&dest, ch);
    }

    *ret = dest;
    return ret;
}

/* ?_Rep@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@_WI@Z */
/* ?_Rep@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@_W_K@Z */
/* ?_Rep@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@GI@Z */
/* ?_Rep@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@G_K@Z */
static ostreambuf_iterator_wchar* num_put_wchar__Rep(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, wchar_t c, MSVCP_size_t count)
{
    TRACE("(%p %p %d %ld)\n", this, ret, c, count);

    for(; count>0; count--)
        ostreambuf_iterator_wchar_put(&dest, c);

    *ret = dest;
    return ret;
}

/* ?_Ffmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAPEADPEADDH@Z */
/* ?_Ffmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAPEADPEADDH@Z */
static char* num_put_wchar__Ffmt(const num_put *this, char *fmt, char spec, int fmtfl)
{
    int type = fmtfl & FMTFLAG_floatfield;
    char *p = fmt;

    TRACE("(%p %p %d %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';
    *p++ = '.';
    *p++ = '*';
    if(spec)
        *p++ = spec;

    if(type == FMTFLAG_fixed)
        *p++ = 'f';
    else if(type == FMTFLAG_scientific)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'E' : 'e';
    else if(type == (FMTFLAG_fixed|FMTFLAG_scientific))
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'A' : 'a';
    else
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'G' : 'g';

    *p++ = '\0';
    return fmt;
}

/* TODO: This function should be removed when num_put_wchar__Fput is implemented */
static ostreambuf_iterator_wchar* num_put__fput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf,
        MSVCP_size_t count, numpunct_wchar *numpunct)
{
    char *p, dec_point = *localeconv()->decimal_point;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    MSVCP_size_t i, pad;

    TRACE("(%p %p %p %d %s %ld)\n", this, ret, base, fill, buf, count);

    for(p=buf; p<buf+count; p++) {
        if(*p == dec_point)
            break;
    }
    p--;

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_wchar__Putc(this, &dest, dest, buf, 1);
        buf++;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_wchar__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }

    for(i=0; i<count; i++) {
        if(buf[i] == dec_point)
            num_put_wchar__Rep(this, &dest, dest, numpunct_wchar_decimal_point(numpunct), 1);
        else
            num_put_wchar__Putc(this, &dest, dest, buf+i, 1);
    }

    return num_put_wchar__Rep(this, ret, dest, fill, pad);
}

/* ?_Ifmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
/* ?_Ifmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
static char* num_put_wchar__Ifmt(const num_put *this, char *fmt, const char *spec, int fmtfl)
{
    int base = fmtfl & FMTFLAG_basefield;
    char *p = fmt;

    TRACE("(%p %p %p %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';

    *p++ = *spec++;
    if(*spec == 'l')
        *p++ = *spec++;

    if(base == FMTFLAG_oct)
        *p++ = 'o';
    else if(base == FMTFLAG_hex)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'X' : 'x';
    else
        *p++ = *spec;

    *p++ = '\0';
    return fmt;
}

static ostreambuf_iterator_wchar* num_put__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf,
        MSVCP_size_t count, numpunct_wchar *numpunct)
{
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p;
    wchar_t sep;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    MSVCP_size_t i, pad;

    TRACE("(%p %p %p %d %s %ld)\n", this, ret, base, fill, buf, count);

    /* Add separators to number */
    numpunct_wchar_grouping(numpunct, &grouping_bstr);
    grouping = basic_string_char_c_str(&grouping_bstr);
    sep = grouping[0] ? numpunct_wchar_thousands_sep(numpunct) : '\0';

    for(p=buf+count-1; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = '\0'; /* mark thousands separator positions */
            count++;
        }
    }
    basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_wchar__Putc(this, &dest, dest, buf, 1);
        buf++;
    }else if((adjustfield & FMTFLAG_internal) && (buf[1]=='x' || buf[1]=='X')) {
        num_put_wchar__Putc(this, &dest, dest, buf, 2);
        buf += 2;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_wchar__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }

    for(i=0; i<count; i++) {
        if(!buf[i])
            num_put_wchar__Rep(this, &dest, dest, sep, 1);
        else
            num_put_wchar__Putc(this, &dest, dest, buf+i, 1);
    }

    return num_put_wchar__Rep(this, ret, dest, fill, pad);
}

/* ?_Iput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPADI@Z */
/* ?_Iput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEAD_K@Z */
static ostreambuf_iterator_wchar* num_put_wchar__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf, MSVCP_size_t count)
{
    return num_put__Iput(this, ret, dest, base, fill, buf, count, numpunct_wchar_use_facet(&base->loc));
}

/* ?_Iput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPADI@Z */
/* ?_Iput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEAD_K@Z */
static ostreambuf_iterator_wchar* num_put_short__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf, MSVCP_size_t count)
{
    return num_put__Iput(this, ret, dest, base, fill, buf, count, numpunct_short_use_facet(&base->loc));
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WJ@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WJ@Z */
#define call_num_put_wchar_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, LONG), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_long, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GJ@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GJ@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_long, 28)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WJ@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WJ@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GJ@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GJ@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_long, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WK@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WK@Z */
#define call_num_put_wchar_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, ULONG), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ulong, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GK@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GK@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ulong, 28)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WK@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WK@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GK@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GK@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ulong, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ulong(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WN@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WN@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WO@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WO@Z */
#define call_num_put_wchar_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_wchar_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 8, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_double, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_wchar__Ffmt(this, fmt, '\0', base->fmtfl);
    size = _scprintf(fmt, base->prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = MSVCRT_operator_new(size*2);
    if(!tmp) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    num_put__fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, base->prec, v),
            numpunct_wchar_use_facet(&base->loc));
    MSVCRT_operator_delete(tmp);
    return ret;
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GN@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GN@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GO@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GO@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_double, 32)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;
    streamsize prec;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_wchar__Ffmt(this, fmt, '\0', base->fmtfl);
    prec = get_precision(base);
    size = _scprintf(fmt, prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = MSVCRT_operator_new(size*2);
    if(!tmp) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    num_put__fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, prec, v),
            numpunct_short_use_facet(&base->loc));
    MSVCRT_operator_delete(tmp);
    return ret;
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WN@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WN@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GN@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GN@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_double, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_double(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WO@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WO@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GO@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GO@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ldouble, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ldouble(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ldouble(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBX@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBX@Z */
#define call_num_put_wchar_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, const void*), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ptr, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators beetwen every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBX@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBX@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ptr, 28)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators beetwen every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBX@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBX@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBX@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBX@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ptr, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ptr(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_J@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_J@Z */
#define call_num_put_wchar_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, __int64), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_int64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_J@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_J@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_int64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_J@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_J@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_J@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_J@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_int64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_wchar_do_put_int64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_K@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_K@Z */
#define call_num_put_wchar_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, unsigned __int64), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_uint64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_K@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_uint64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators beetwen every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_K@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_K@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_K@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_uint64, 32)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_wchar_do_put_uint64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_N@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_N@Z */
#define call_num_put_wchar_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, MSVCP_bool), \
        (this, ret, dest, base, fill, v))
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_bool, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, MSVCP_bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_wchar *numpunct = numpunct_wchar_use_facet(&base->loc);
        basic_string_wchar str;
        MSVCP_size_t pad, len;

        if(v)
            numpunct_wchar_truename(numpunct, &str);
        else
            numpunct_wchar_falsename(numpunct, &str);

        len = basic_string_wchar_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_wchar__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_wchar__Put(this, &dest, dest, basic_string_wchar_c_str(&str), len);
        basic_string_wchar_dtor(&str);
        return num_put_wchar__Rep(this, ret, dest, fill, pad);
    }

    return num_put_wchar_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_N@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_N@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_bool, 28)
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, MSVCP_bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_wchar *numpunct = numpunct_short_use_facet(&base->loc);
        basic_string_wchar str;
        MSVCP_size_t pad, len;

        if(v)
            numpunct_wchar_truename(numpunct, &str);
        else
            numpunct_wchar_falsename(numpunct, &str);

        len = basic_string_wchar_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_wchar__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_wchar__Put(this, &dest, dest, basic_string_wchar_c_str(&str), len);
        basic_string_wchar_dtor(&str);
        return num_put_wchar__Rep(this, ret, dest, fill, pad);
    }

    return num_put_wchar_put_long(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_N@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_N@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_N@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_N@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_bool, 28)
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, MSVCP_bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_bool(this, ret, dest, base, fill, v);
}

/* ?id@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id time_put_char_id = {0};

/* ??_7?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_time_put_char_vtable;

/* ?_Init@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_put_char__Init, 8)
void __thiscall time_put_char__Init(time_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Gettnames(locinfo, &this->time);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor_locinfo, 12)
time_put* __thiscall time_put_char_ctor_locinfo(time_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_time_put_char_vtable;
    time_put_char__Init(this, locinfo);
    return this;
}

/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor_refs, 8)
time_put* __thiscall time_put_char_ctor_refs(time_put *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_put_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor, 4)
time_put* __thiscall time_put_char_ctor(time_put *this)
{
    return time_put_char_ctor_refs(this, 0);
}

/* ??1?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_put_char_dtor, 4)
void __thiscall time_put_char_dtor(time_put *this)
{
    TRACE("(%p)\n", this);
    _Timevec_dtor(&this->time);
}

DEFINE_THISCALL_WRAPPER(time_put_char_vector_dtor, 8)
time_put* __thiscall time_put_char_vector_dtor(time_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_put_char_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        time_put_char_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t time_put_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = MSVCRT_operator_new(sizeof(time_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }

        _Locinfo_ctor_cstr(&locinfo, basic_string_char_c_str(&loc->ptr->name));
        time_put_char_ctor_locinfo((time_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_TIME;
}

static time_put* time_put_char_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_char_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_char__Getcat(&fac, loc);
    obj = (time_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@DD@Z */
/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_do_put, 32)
#define call_time_put_char_do_put(this, ret, dest, base, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_char*, \
        (const time_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, const struct tm*, char, char), \
        (this, ret, dest, base, t, spec, mod))
ostreambuf_iterator_char* __thiscall time_put_char_do_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    MSVCP_size_t i, len;

    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++)
        ostreambuf_iterator_char_put(&dest, buf[i]);

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@DD@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_put, 32)
ostreambuf_iterator_char* __thiscall time_put_char_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);
    return call_time_put_char_do_put(this, ret, dest, base, t, spec, mod);
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@PBD3@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@PEBD3@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_put_format, 32)
ostreambuf_iterator_char* __thiscall time_put_char_put_format(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, const char *pat, const char *pat_end)
{
    TRACE("(%p %p %p %p %s)\n", this, ret, base, t, debugstr_an(pat, pat_end-pat));

    while(pat < pat_end) {
        if(*pat != '%') {
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
        }else if(*pat=='#' && pat+1==pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else {
            char mod;

            if(*pat == '#') {
                mod = '#';
                pat++;
            }else {
                mod = 0;
            }

            time_put_char_put(this, &dest, dest, base, t, *pat++, mod);
        }
    }

    *ret = dest;
    return ret;
}

/* ?id@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
static locale_id time_put_wchar_id = {0};
/* ?id@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id time_put_short_id = {0};

/* ??_7?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_time_put_wchar_vtable;
/* ??_7?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr MSVCP_time_put_short_vtable;

/* ?_Init@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar__Init, 8)
void __thiscall time_put_wchar__Init(time_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Gettnames(locinfo, &this->time);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
static time_put* time_put_wchar_ctor_locinfo(time_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    TRACE("(%p %p %lu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &MSVCP_time_put_wchar_vtable;
    time_put_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor_locinfo, 12)
time_put* __thiscall time_put_short_ctor_locinfo(time_put *this, const _Locinfo *locinfo, MSVCP_size_t refs)
{
    time_put_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &MSVCP_time_put_short_vtable;
    return this;
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAA@PEBD_K@Z */
static time_put* time_put_wchar_ctor_name(time_put *this, const char *name, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %lu)\n", this, debugstr_a(name), refs);

    _Locinfo_ctor_cstr(&locinfo, name);
    time_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAA@PEBD_K@Z */
static time_put* time_put_short_ctor_name(time_put *this, const char *name, MSVCP_size_t refs)
{
    time_put_wchar_ctor_name(this, name, refs);
    this->facet.vtable = &MSVCP_time_put_short_vtable;
    return this;
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
static time_put* time_put_wchar_ctor_refs(time_put *this, MSVCP_size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %lu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor_refs, 8)
time_put* __thiscall time_put_short_ctor_refs(time_put *this, MSVCP_size_t refs)
{
    time_put_wchar_ctor_refs(this, refs);
    this->facet.vtable = &MSVCP_time_put_short_vtable;
    return this;
}

/* ??_F?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
static time_put* time_put_wchar_ctor(time_put *this)
{
    return time_put_wchar_ctor_refs(this, 0);
}

/* ??_F?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor, 4)
time_put* __thiscall time_put_short_ctor(time_put *this)
{
    time_put_wchar_ctor(this);
    this->facet.vtable = &MSVCP_time_put_short_vtable;
    return this;
}

/* ??1?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEAA@XZ */
/* ??1?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_put_wchar_dtor, 4)
void __thiscall time_put_wchar_dtor(time_put *this)
{
    TRACE("(%p)\n", this);
    _Timevec_dtor(&this->time);
}

DEFINE_THISCALL_WRAPPER(time_put_wchar_vector_dtor, 8)
time_put* __thiscall time_put_wchar_vector_dtor(time_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_put_wchar_dtor(this+i);
        MSVCRT_operator_delete(ptr);
    } else {
        time_put_wchar_dtor(this);
        if(flags & 1)
            MSVCRT_operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t time_put_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(time_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        time_put_wchar_ctor_name((time_put*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_TIME;
}

static time_put* time_put_wchar_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_wchar_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_wchar__Getcat(&fac, loc);
    obj = (time_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
static MSVCP_size_t time_put_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = MSVCRT_operator_new(sizeof(time_put));
        if(!*facet) {
            ERR("Out of memory\n");
            throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            return 0;
        }
        time_put_short_ctor_name((time_put*)*facet,
                basic_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_TIME;
}

static time_put* time_put_short_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_short_id), TRUE);
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_short__Getcat(&fac, loc);
    obj = (time_put*)fac;
    locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@DD@Z */
/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@DD@Z */
/* ?do_put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@DD@Z */
/* ?do_put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_do_put, 32)
#define call_time_put_wchar_do_put(this, ret, dest, base, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_wchar*, \
        (const time_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, const struct tm*, char, char), \
        (this, ret, dest, base, t, spec, mod))
ostreambuf_iterator_wchar* __thiscall time_put_wchar_do_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    MSVCP_size_t i, len;
    wchar_t c;

    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++) {
        c = mb_to_wc(buf[i], &this->cvt);
        ostreambuf_iterator_wchar_put(&dest, c);
    }

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@DD@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@DD@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@DD@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_put, 32)
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);
    return call_time_put_wchar_do_put(this, ret, dest, base, t, spec, mod);
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@PBG3@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@PEBG3@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@PB_W4@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@PEB_W4@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_put_format, 32)
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put_format(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, const wchar_t *pat, const wchar_t *pat_end)
{
    wchar_t percent = mb_to_wc('%', &this->cvt);
    char c[MB_LEN_MAX];

    TRACE("(%p %p %p %p %s)\n", this, ret, base, t, debugstr_wn(pat, pat_end-pat));

    while(pat < pat_end) {
        if(*pat != percent) {
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_wchar_put(&dest, percent);
        }else if(!_Wcrtomb(c, *pat, NULL, &this->cvt) || (*c=='#' && pat+1==pat_end)) {
            ostreambuf_iterator_wchar_put(&dest, percent);
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else {
            if(*c == '#') {
                if(!_Wcrtomb(c, *pat++, NULL, &this->cvt)) {
                    ostreambuf_iterator_wchar_put(&dest, percent);
                    ostreambuf_iterator_wchar_put(&dest, *(pat-1));
                    ostreambuf_iterator_wchar_put(&dest, *pat);
                }else {
                    time_put_wchar_put(this, &dest, dest, base, t, *c, '#');
                }
            }else {
                time_put_wchar_put(this, &dest, dest, base, t, *c, 0);
            }
        }
    }

    *ret = dest;
    return ret;
}

/* ??0_Locimp@locale@std@@AAE@_N@Z */
/* ??0_Locimp@locale@std@@AEAA@_N@Z */
static locale__Locimp* locale__Locimp_ctor_transparent(locale__Locimp *this, MSVCP_bool transparent)
{
    TRACE("(%p %d)\n", this, transparent);

    memset(this, 0, sizeof(locale__Locimp));
    locale_facet_ctor_refs(&this->facet, 1);
    this->transparent = transparent;
    basic_string_char_ctor_cstr(&this->name, "*");
    return this;
}

/* ??_F_Locimp@locale@std@@QAEXXZ */
/* ??_F_Locimp@locale@std@@QEAAXXZ */
static locale__Locimp* locale__Locimp_ctor(locale__Locimp *this)
{
    return locale__Locimp_ctor_transparent(this, FALSE);
}

/* ??0_Locimp@locale@std@@AAE@ABV012@@Z */
/* ??0_Locimp@locale@std@@AEAA@AEBV012@@Z */
static locale__Locimp* locale__Locimp_copy_ctor(locale__Locimp *this, const locale__Locimp *copy)
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
        {
            this->facetvec[i] = copy->facetvec[i];
            if(this->facetvec[i])
                locale_facet__Incref(this->facetvec[i]);
        }
    }
    basic_string_char_copy_ctor(&this->name, &copy->name);
    _Lockit_dtor(&lock);
    return this;
}

/* ??1_Locimp@locale@std@@MAE@XZ */
/* ??1_Locimp@locale@std@@MEAA@XZ */
static void locale__Locimp_dtor(locale__Locimp *this)
{
    MSVCP_size_t i;

    TRACE("(%p)\n", this);

    locale_facet_dtor(&this->facet);
    for(i=0; i<this->facet_cnt; i++)
        if(this->facetvec[i] && locale_facet__Decref(this->facetvec[i]))
            call_locale_facet_vector_dtor(this->facetvec[i], 1);

    MSVCRT_operator_delete(this->facetvec);
    basic_string_char_dtor(&this->name);
}

DEFINE_THISCALL_WRAPPER(locale__Locimp_vector_dtor, 8)
locale__Locimp* __thiscall locale__Locimp_vector_dtor(locale__Locimp *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

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
static void locale__Locimp__Locimp_Addfac(locale__Locimp *locimp, locale_facet *facet, MSVCP_size_t id)
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
        call_locale_facet_vector_dtor(locimp->facetvec[id], 1);

    locimp->facetvec[id] = facet;
    if(facet)
        locale_facet__Incref(facet);
    _Lockit_dtor(&lock);
}

/* ?_Addfac@_Locimp@locale@std@@AAEXPAVfacet@23@I@Z */
/* ?_Addfac@_Locimp@locale@std@@AEAAXPEAVfacet@23@_K@Z */
static void locale__Locimp__Addfac(locale__Locimp *this, locale_facet *facet, MSVCP_size_t id)
{
    locale__Locimp__Locimp_Addfac(this, facet, id);
}

/* ?_Makeushloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeushloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct, time_get
 */
static void locale__Locimp__Makeushloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_short__Getcat(NULL, NULL)-1))) {
        ctype_wchar *ctype;

        if(loc) {
            ctype = ctype_short_use_facet(loc);
        }else {
            ctype = MSVCRT_operator_new(sizeof(ctype_wchar));
            if(!ctype) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            ctype_short_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_short_id));
    }

    if(cat & (1<<(num_get_short__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_short_use_facet(loc);
        }else {
            numget = MSVCRT_operator_new(sizeof(num_get));
            if(!numget) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_get_short_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_short_id));
    }

    if(cat & (1<<(num_put_short__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_short_use_facet(loc);
        }else {
            numput = MSVCRT_operator_new(sizeof(num_put));
            if(!numput) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_put_short_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_short_id));
    }

    if(cat & (1<<(numpunct_short__Getcat(NULL, NULL)-1))) {
        numpunct_wchar *numpunct;

        if(loc) {
            numpunct = numpunct_short_use_facet(loc);
        }else {
            numpunct = MSVCRT_operator_new(sizeof(numpunct_wchar));
            if(!numpunct) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            numpunct_short_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_short_id));
    }

    if(cat & (1<<(collate_short__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_short_use_facet(loc);
        }else {
            c = MSVCRT_operator_new(sizeof(collate));
            if(!c) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            collate_short_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_short_id));
    }

     if(cat & (1<<(time_put_short__Getcat(NULL, NULL)-1))) {
         time_put *t;

         if(loc) {
             t = time_put_short_use_facet(loc);
         }else {
             t = MSVCRT_operator_new(sizeof(time_put));
             if(!t) {
                 ERR("Out of memory\n");
                 throw_exception(EXCEPTION_BAD_ALLOC, NULL);
             }
             time_put_short_ctor_locinfo(t, locinfo, 0);
         }
         locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_short_id));
     }

    if(cat & (1<<(codecvt_short__Getcat(NULL, NULL)-1))) {
        codecvt_wchar *codecvt;

        if(loc) {
            codecvt = codecvt_short_use_facet(loc);
        }else {
            codecvt = MSVCRT_operator_new(sizeof(codecvt_wchar));
            if(!codecvt) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            codecvt_short_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_short_id));
    }
}

/* ?_Makewloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makewloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct, time_get
 */
static void locale__Locimp__Makewloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_wchar__Getcat(NULL, NULL)-1))) {
        ctype_wchar *ctype;

        if(loc) {
            ctype = ctype_wchar_use_facet(loc);
        }else {
            ctype = MSVCRT_operator_new(sizeof(ctype_wchar));
            if(!ctype) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            ctype_wchar_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_wchar_id));
    }

    if(cat & (1<<(num_get_wchar__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_wchar_use_facet(loc);
        }else {
            numget = MSVCRT_operator_new(sizeof(num_get));
            if(!numget) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_get_wchar_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_wchar_id));
    }

    if(cat & (1<<(num_put_wchar__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_wchar_use_facet(loc);
        }else {
            numput = MSVCRT_operator_new(sizeof(num_put));
            if(!numput) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_put_wchar_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_wchar_id));
    }

    if(cat & (1<<(numpunct_wchar__Getcat(NULL, NULL)-1))) {
        numpunct_wchar *numpunct;

        if(loc) {
            numpunct = numpunct_wchar_use_facet(loc);
        }else {
            numpunct = MSVCRT_operator_new(sizeof(numpunct_wchar));
            if(!numpunct) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            numpunct_wchar_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_wchar_id));
    }

    if(cat & (1<<(collate_wchar__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_wchar_use_facet(loc);
        }else {
            c = MSVCRT_operator_new(sizeof(collate));
            if(!c) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            collate_wchar_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_wchar_id));
    }

    if(cat & (1<<(time_put_wchar__Getcat(NULL, NULL)-1))) {
        time_put *t;

        if(loc) {
            t = time_put_wchar_use_facet(loc);
        }else {
            t = MSVCRT_operator_new(sizeof(time_put));
            if(!t) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            time_put_wchar_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_wchar_id));
    }

    if(cat & (1<<(codecvt_wchar__Getcat(NULL, NULL)-1))) {
        codecvt_wchar *codecvt;

        if(loc) {
            codecvt = codecvt_wchar_use_facet(loc);
        }else {
            codecvt = MSVCRT_operator_new(sizeof(codecvt_wchar));
            if(!codecvt) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            codecvt_wchar_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_wchar_id));
    }
}

/* ?_Makexloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makexloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct, time_get
 */
static void locale__Locimp__Makexloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_char__Getcat(NULL, NULL)-1))) {
        ctype_char *ctype;

        if(loc) {
            ctype = ctype_char_use_facet(loc);
        }else {
            ctype = MSVCRT_operator_new(sizeof(ctype_char));
            if(!ctype) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            ctype_char_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_char_id));
    }

    if(cat & (1<<(num_get_char__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_char_use_facet(loc);
        }else {
            numget = MSVCRT_operator_new(sizeof(num_get));
            if(!numget) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_get_char_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_char_id));
    }

    if(cat & (1<<(num_put_char__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_char_use_facet(loc);
        }else {
            numput = MSVCRT_operator_new(sizeof(num_put));
            if(!numput) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            num_put_char_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_char_id));
    }

    if(cat & (1<<(numpunct_char__Getcat(NULL, NULL)-1))) {
        numpunct_char *numpunct;

        if(loc) {
            numpunct = numpunct_char_use_facet(loc);
        }else {
            numpunct = MSVCRT_operator_new(sizeof(numpunct_char));
            if(!numpunct) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            numpunct_char_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_char_id));
    }

    if(cat & (1<<(collate_char__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_char_use_facet(loc);
        }else {
            c = MSVCRT_operator_new(sizeof(collate));
            if(!c) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            collate_char_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_char_id));
    }

    if(cat & (1<<(time_put_char__Getcat(NULL, NULL)-1))) {
        time_put *t;

        if(loc) {
            t = time_put_char_use_facet(loc);
        }else {
            t = MSVCRT_operator_new(sizeof(time_put));
            if(!t) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            time_put_char_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_char_id));
    }

    if(cat & (1<<(codecvt_char__Getcat(NULL, NULL)-1))) {
        codecvt_char *codecvt;

        if(loc) {
            codecvt = codecvt_char_use_facet(loc);
        }else {
            codecvt = MSVCRT_operator_new(sizeof(codecvt_char));
            if(!codecvt) {
                ERR("Out of memory\n");
                throw_exception(EXCEPTION_BAD_ALLOC, NULL);
            }
            codecvt_char_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_char_id));
    }
}

/* ?_Makeloc@_Locimp@locale@std@@CAPAV123@ABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeloc@_Locimp@locale@std@@CAPEAV123@AEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
static locale__Locimp* locale__Locimp__Makeloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    TRACE("(%p %d %p %p)\n", locinfo, cat, locimp, loc);

    locale__Locimp__Makexloc(locinfo, cat, locimp, loc);
    locale__Locimp__Makewloc(locinfo, cat, locimp, loc);
    locale__Locimp__Makeushloc(locinfo, cat, locimp, loc);

    locimp->catmask |= cat;
    basic_string_char_assign(&locimp->name, &locinfo->newlocname);
    return locimp;
}

/* ??_7_Locimp@locale@std@@6B@ */
const vtable_ptr MSVCP_locale__Locimp_vtable[] = {
    (vtable_ptr)THISCALL_NAME(locale__Locimp_vector_dtor)
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

/* ?_Init@locale@std@@CAPAV_Locimp@12@XZ */
/* ?_Init@locale@std@@CAPEAV_Locimp@12@XZ */
locale__Locimp* __cdecl locale__Init(void)
{
    _Lockit lock;

    TRACE("\n");

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(global_locale) {
        _Lockit_dtor(&lock);
        return global_locale;
    }

    global_locale = MSVCRT_operator_new(sizeof(locale__Locimp));
    if(!global_locale) {
        _Lockit_dtor(&lock);
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
        return NULL;
    }

    locale__Locimp_ctor(global_locale);
    global_locale->catmask = (1<<(LC_MAX+1))-1;
    basic_string_char_dtor(&global_locale->name);
    basic_string_char_ctor_cstr(&global_locale->name, "C");

    locale__Locimp__Clocptr = global_locale;
    global_locale->facet.refs++;
    locale_ctor_locimp(&classic_locale, locale__Locimp__Clocptr);
    _Lockit_dtor(&lock);

    return global_locale;
}

/* ?_Iscloc@locale@std@@QBE_NXZ */
/* ?_Iscloc@locale@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(locale__Iscloc, 4)
MSVCP_bool __thiscall locale__Iscloc(const locale *this)
{
    TRACE("(%p)\n", this);
    return this->ptr == locale__Locimp__Clocptr;
}

/* ??0locale@std@@QAE@ABV01@0H@Z */
/* ??0locale@std@@QEAA@AEBV01@0H@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locale_locale, 16)
locale* __thiscall locale_ctor_locale_locale(locale *this, const locale *loc, const locale *other, category cat)
{
    _Locinfo locinfo;

    TRACE("(%p %p %p %d)\n", this, loc, other, cat);

    this->ptr = MSVCRT_operator_new(sizeof(locale__Locimp));
    if(!this->ptr) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    locale__Locimp_copy_ctor(this->ptr, loc->ptr);

    _Locinfo_ctor_cat_cstr(&locinfo, loc->ptr->catmask, basic_string_char_c_str(&loc->ptr->name));
    _Locinfo__Addcats(&locinfo, cat & other->ptr->catmask, basic_string_char_c_str(&other->ptr->name));
    locale__Locimp__Makeloc(&locinfo, cat, this->ptr, other);
    _Locinfo_dtor(&locinfo);

    return this;
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
    _Locinfo locinfo;

    TRACE("(%p %s %d)\n", this, locname, cat);

    this->ptr = MSVCRT_operator_new(sizeof(locale__Locimp));
    if(!this->ptr) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    locale__Locimp_ctor(this->ptr);

    locale__Init();

    _Locinfo_ctor_cat_cstr(&locinfo, cat, locname);
    if(!memcmp(basic_string_char_c_str(&locinfo.newlocname), "*", 2)) {
        _Locinfo_dtor(&locinfo);
        MSVCRT_operator_delete(this->ptr);
        throw_exception(EXCEPTION_RUNTIME_ERROR, "bad locale name");
    }

    locale__Locimp__Makeloc(&locinfo, cat, this->ptr, NULL);
    _Locinfo_dtor(&locinfo);

    return this;
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
    this->ptr = locale__Init();
    locale_facet__Incref(&this->ptr->facet);
    return this;
}

/* ??1locale@std@@QAE@XZ */
/* ??1locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_dtor, 4)
void __thiscall locale_dtor(locale *this)
{
    TRACE("(%p)\n", this);
    if(this->ptr && locale_facet__Decref(&this->ptr->facet)) {
        locale__Locimp_dtor(this->ptr);
        MSVCRT_operator_delete(this->ptr);
    }
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
        basic_string_char_dtor(&this->ptr->name);
        basic_string_char_ctor_cstr(&this->ptr->name, "*");
    }
    return this;
}

/* ?_Getfacet@locale@std@@QBEPBVfacet@12@I_N@Z */
/* ?_Getfacet@locale@std@@QEBAPEBVfacet@12@_K_N@Z */
DEFINE_THISCALL_WRAPPER(locale__Getfacet, 12)
const locale_facet* __thiscall locale__Getfacet(const locale *this,
        MSVCP_size_t id, MSVCP_bool allow_transparent)
{
    locale_facet *fac;

    TRACE("(%p %lu)\n", this, id);

    fac = id < this->ptr->facet_cnt ? this->ptr->facetvec[id] : NULL;
    if(fac || !this->ptr->transparent || !allow_transparent)
        return fac;

    return id < global_locale->facet_cnt ? global_locale->facetvec[id] : NULL;
}

/* ?classic@locale@std@@SAABV12@XZ */
/* ?classic@locale@std@@SAAEBV12@XZ */
const locale* __cdecl locale_classic(void)
{
    TRACE("\n");
    locale__Init();
    return &classic_locale;
}

/* ?empty@locale@std@@SA?AV12@XZ */
locale* __cdecl locale_empty(locale *ret)
{
    TRACE("\n");

    locale__Init();

    ret->ptr = MSVCRT_operator_new(sizeof(locale__Locimp));
    if(!ret->ptr) {
        ERR("Out of memory\n");
        throw_exception(EXCEPTION_BAD_ALLOC, NULL);
    }
    locale__Locimp_ctor_transparent(ret->ptr, TRUE);
    return ret;
}

/* ?name@locale@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?name@locale@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(locale_name, 8)
basic_string_char* __thiscall locale_name(const locale *this, basic_string_char *ret)
{
    TRACE( "(%p)\n", this);
    basic_string_char_copy_ctor(ret, &this->ptr->name);
    return ret;
}

/* ?global@locale@std@@SA?AV12@ABV12@@Z */
/* ?global@locale@std@@SA?AV12@AEBV12@@Z */
locale* __cdecl locale_global(locale *ret, const locale *loc)
{
    _Lockit lock;
    int i;

    TRACE("(%p %p)\n", loc, ret);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    locale_ctor(ret);

    if(loc->ptr != global_locale) {
        locale_facet__Decref(&global_locale->facet);
        global_locale = loc->ptr;
        locale_facet__Incref(&global_locale->facet);

        for(i=LC_ALL+1; i<=LC_MAX; i++) {
            if((global_locale->catmask & (1<<(i-1))) == 0)
                continue;
            setlocale(i, basic_string_char_c_str(&global_locale->name));
        }
    }
    _Lockit_dtor(&lock);
    return ret;
}

/* wctrans */
wctrans_t __cdecl wctrans(const char *property)
{
    static const char str_tolower[] = "tolower";
    static const char str_toupper[] = "toupper";

    if(!strcmp(property, str_tolower))
        return 2;
    if(!strcmp(property, str_toupper))
        return 1;
    return 0;
}

/* towctrans */
wint_t __cdecl towctrans(wint_t c, wctrans_t category)
{
    if(category == 1)
        return towupper(c);
    return towlower(c);
}

DEFINE_RTTI_DATA0(locale_facet, 0, ".?AVfacet@locale@std@@")
DEFINE_RTTI_DATA1(collate_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@D@std@@")
DEFINE_RTTI_DATA1(collate_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@_W@std@@")
DEFINE_RTTI_DATA1(collate_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@G@std@@")
DEFINE_RTTI_DATA1(ctype_base, 0, &locale_facet_rtti_base_descriptor, ".?AUctype_base@std@@")
DEFINE_RTTI_DATA2(ctype_char, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@D@std@@")
DEFINE_RTTI_DATA2(ctype_wchar, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@_W@std@@")
DEFINE_RTTI_DATA2(ctype_short, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@G@std@@")
DEFINE_RTTI_DATA1(codecvt_base, 0, &locale_facet_rtti_base_descriptor, ".?AVcodecvt_base@std@@")
DEFINE_RTTI_DATA2(codecvt_char, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@DDH@std@@")
DEFINE_RTTI_DATA2(codecvt_wchar, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@_WDH@std@@")
DEFINE_RTTI_DATA2(codecvt_short, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@GDH@std@@")
DEFINE_RTTI_DATA1(numpunct_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@D@std@@")
DEFINE_RTTI_DATA1(numpunct_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@_W@std@@")
DEFINE_RTTI_DATA1(numpunct_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@G@std@@")
DEFINE_RTTI_DATA1(num_get_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_get_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_get_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")

#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif
    __ASM_VTABLE(locale_facet,
            VTABLE_ADD_FUNC(locale_facet_vector_dtor));
    __ASM_VTABLE(collate_char,
            VTABLE_ADD_FUNC(collate_char_vector_dtor)
            VTABLE_ADD_FUNC(collate_char_do_compare)
            VTABLE_ADD_FUNC(collate_char_do_transform)
            VTABLE_ADD_FUNC(collate_char_do_hash));
    __ASM_VTABLE(collate_wchar,
            VTABLE_ADD_FUNC(collate_wchar_vector_dtor)
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(collate_short,
            VTABLE_ADD_FUNC(collate_wchar_vector_dtor)
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(ctype_base,
            VTABLE_ADD_FUNC(ctype_base_vector_dtor));
    __ASM_VTABLE(ctype_char,
            VTABLE_ADD_FUNC(ctype_char_vector_dtor)
            VTABLE_ADD_FUNC(ctype_char_do_tolower)
            VTABLE_ADD_FUNC(ctype_char_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_char_do_toupper)
            VTABLE_ADD_FUNC(ctype_char_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_char_do_widen)
            VTABLE_ADD_FUNC(ctype_char_do_widen_ch)
            VTABLE_ADD_FUNC(ctype_char_do_narrow)
            VTABLE_ADD_FUNC(ctype_char_do_narrow_ch));
    __ASM_VTABLE(ctype_wchar,
            VTABLE_ADD_FUNC(ctype_wchar_vector_dtor)
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
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch));
    __ASM_VTABLE(ctype_short,
            VTABLE_ADD_FUNC(ctype_wchar_vector_dtor)
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
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch));
    __ASM_VTABLE(codecvt_base,
            VTABLE_ADD_FUNC(codecvt_base_vector_dtor)
            VTABLE_ADD_FUNC(codecvt_base_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_base_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding));
    __ASM_VTABLE(codecvt_char,
            VTABLE_ADD_FUNC(codecvt_char_vector_dtor)
            VTABLE_ADD_FUNC(codecvt_base_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_base_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
            VTABLE_ADD_FUNC(codecvt_char_do_in)
            VTABLE_ADD_FUNC(codecvt_char_do_out)
            VTABLE_ADD_FUNC(codecvt_char_do_unshift)
            VTABLE_ADD_FUNC(codecvt_char_do_length));
    __ASM_VTABLE(codecvt_wchar,
            VTABLE_ADD_FUNC(codecvt_wchar_vector_dtor)
            VTABLE_ADD_FUNC(codecvt_wchar_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_wchar_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
            VTABLE_ADD_FUNC(codecvt_wchar_do_in)
            VTABLE_ADD_FUNC(codecvt_wchar_do_out)
            VTABLE_ADD_FUNC(codecvt_wchar_do_unshift)
            VTABLE_ADD_FUNC(codecvt_wchar_do_length));
    __ASM_VTABLE(codecvt_short,
            VTABLE_ADD_FUNC(codecvt_wchar_vector_dtor)
            VTABLE_ADD_FUNC(codecvt_wchar_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_wchar_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
            VTABLE_ADD_FUNC(codecvt_wchar_do_in)
            VTABLE_ADD_FUNC(codecvt_wchar_do_out)
            VTABLE_ADD_FUNC(codecvt_wchar_do_unshift)
            VTABLE_ADD_FUNC(codecvt_wchar_do_length));
    __ASM_VTABLE(numpunct_char,
            VTABLE_ADD_FUNC(numpunct_char_vector_dtor)
            VTABLE_ADD_FUNC(numpunct_char_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_char_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_char_do_grouping)
            VTABLE_ADD_FUNC(numpunct_char_do_falsename)
            VTABLE_ADD_FUNC(numpunct_char_do_truename));
    __ASM_VTABLE(numpunct_wchar,
            VTABLE_ADD_FUNC(numpunct_wchar_vector_dtor)
            VTABLE_ADD_FUNC(numpunct_wchar_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_wchar_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_wchar_do_grouping)
            VTABLE_ADD_FUNC(numpunct_wchar_do_falsename)
            VTABLE_ADD_FUNC(numpunct_wchar_do_truename));
    __ASM_VTABLE(numpunct_short,
            VTABLE_ADD_FUNC(numpunct_wchar_vector_dtor)
            VTABLE_ADD_FUNC(numpunct_wchar_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_wchar_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_wchar_do_grouping)
            VTABLE_ADD_FUNC(numpunct_wchar_do_falsename)
            VTABLE_ADD_FUNC(numpunct_wchar_do_truename));
    __ASM_VTABLE(num_get_char,
            VTABLE_ADD_FUNC(num_get_char_vector_dtor)
            VTABLE_ADD_FUNC(num_get_char_do_get_void)
            VTABLE_ADD_FUNC(num_get_char_do_get_double)
            VTABLE_ADD_FUNC(num_get_char_do_get_double)
            VTABLE_ADD_FUNC(num_get_char_do_get_float)
            VTABLE_ADD_FUNC(num_get_char_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_char_do_get_int64)
            VTABLE_ADD_FUNC(num_get_char_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_char_do_get_long)
            VTABLE_ADD_FUNC(num_get_char_do_get_uint)
            VTABLE_ADD_FUNC(num_get_char_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_char_do_get_bool));
    __ASM_VTABLE(num_get_short,
            VTABLE_ADD_FUNC(num_get_wchar_vector_dtor)
            VTABLE_ADD_FUNC(num_get_short_do_get_void)
            VTABLE_ADD_FUNC(num_get_short_do_get_double)
            VTABLE_ADD_FUNC(num_get_short_do_get_double)
            VTABLE_ADD_FUNC(num_get_short_do_get_float)
            VTABLE_ADD_FUNC(num_get_short_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_short_do_get_int64)
            VTABLE_ADD_FUNC(num_get_short_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_short_do_get_long)
            VTABLE_ADD_FUNC(num_get_short_do_get_uint)
            VTABLE_ADD_FUNC(num_get_short_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_short_do_get_bool));
    __ASM_VTABLE(num_get_wchar,
            VTABLE_ADD_FUNC(num_get_wchar_vector_dtor)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_void)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_double)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_double)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_float)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_int64)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_long)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_uint)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_bool));
    __ASM_VTABLE(num_put_char,
            VTABLE_ADD_FUNC(num_put_char_vector_dtor)
            VTABLE_ADD_FUNC(num_put_char_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_char_do_put_double)
            VTABLE_ADD_FUNC(num_put_char_do_put_double)
            VTABLE_ADD_FUNC(num_put_char_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_char_do_put_int64)
            VTABLE_ADD_FUNC(num_put_char_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_char_do_put_long)
            VTABLE_ADD_FUNC(num_put_char_do_put_bool));
    __ASM_VTABLE(num_put_wchar,
            VTABLE_ADD_FUNC(num_put_wchar_vector_dtor)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_double)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_double)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_int64)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_long)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_bool));
    __ASM_VTABLE(num_put_short,
            VTABLE_ADD_FUNC(num_put_wchar_vector_dtor)
            VTABLE_ADD_FUNC(num_put_short_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_short_do_put_double)
            VTABLE_ADD_FUNC(num_put_short_do_put_double)
            VTABLE_ADD_FUNC(num_put_short_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_short_do_put_int64)
            VTABLE_ADD_FUNC(num_put_short_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_short_do_put_long)
            VTABLE_ADD_FUNC(num_put_short_do_put_bool));
    __ASM_VTABLE(time_put_char,
            VTABLE_ADD_FUNC(time_put_char_vector_dtor)
            VTABLE_ADD_FUNC(time_put_char_do_put));
    __ASM_VTABLE(time_put_wchar,
            VTABLE_ADD_FUNC(time_put_wchar_vector_dtor)
            VTABLE_ADD_FUNC(time_put_wchar_do_put));
    __ASM_VTABLE(time_put_short,
            VTABLE_ADD_FUNC(time_put_wchar_vector_dtor)
            VTABLE_ADD_FUNC(time_put_wchar_do_put));
#ifndef __GNUC__
}
#endif

void init_locale(void *base)
{
#ifdef __x86_64__
    init_locale_facet_rtti(base);
    init_collate_char_rtti(base);
    init_collate_wchar_rtti(base);
    init_collate_short_rtti(base);
    init_ctype_base_rtti(base);
    init_ctype_char_rtti(base);
    init_ctype_wchar_rtti(base);
    init_ctype_short_rtti(base);
    init_codecvt_base_rtti(base);
    init_codecvt_char_rtti(base);
    init_codecvt_wchar_rtti(base);
    init_codecvt_short_rtti(base);
    init_numpunct_char_rtti(base);
    init_numpunct_wchar_rtti(base);
    init_numpunct_short_rtti(base);
    init_num_get_char_rtti(base);
    init_num_get_wchar_rtti(base);
    init_num_get_short_rtti(base);
    init_num_put_char_rtti(base);
    init_num_put_wchar_rtti(base);
    init_num_put_short_rtti(base);
    init_time_put_char_rtti(base);
    init_time_put_wchar_rtti(base);
    init_time_put_short_rtti(base);
#endif
}

void free_locale(void)
{
    facets_elem *iter, *safe;

    if(global_locale) {
        locale_dtor(&classic_locale);
        locale__Locimp_dtor(global_locale);
        MSVCRT_operator_delete(global_locale);
    }

    LIST_FOR_EACH_ENTRY_SAFE(iter, safe, &lazy_facets, facets_elem, entry) {
        list_remove(&iter->entry);
        if(locale_facet__Decref(iter->fac))
            call_locale_facet_vector_dtor(iter->fac, 1);
        MSVCRT_operator_delete(iter);
    }
}
