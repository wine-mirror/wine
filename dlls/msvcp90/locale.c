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
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp90);

typedef int category;

typedef struct _locale_id {
    MSVCP_size_t id;
} locale_id;

typedef struct _locale_facet {
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
    const short *table;
    int delfl;
} _Ctypevec;

typedef struct {
    LCID handle;
    unsigned page;
} _Cvtvec;

/* ?_Id_cnt@id@locale@std@@0HA */
int locale_id__Id_cnt = 0;

/* ?_Clocptr@_Locimp@locale@std@@0PAV123@A */
/* ?_Clocptr@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *locale__Locimp__Clocptr = NULL;

static const vtable_ptr MSVCP_locale_facet_vtable[];

/* ??0id@locale@std@@QAE@I@Z */
/* ??0id@locale@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_id_ctor_id, 8)
locale_id* __thiscall locale_id_ctor_id(locale_id *this, MSVCP_size_t id)
{
    FIXME("(%p %lu) stub\n", this, id);
    return NULL;
}

/* ??_Fid@locale@std@@QAEXXZ */
/* ??_Fid@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_id_ctor, 4)
locale_id* __thiscall locale_id_ctor(locale_id *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??Bid@locale@std@@QAEIXZ */
/* ??Bid@locale@std@@QEAA_KXZ */
DEFINE_THISCALL_WRAPPER(locale_id_operator_size_t, 4)
MSVCP_size_t __thiscall locale_id_operator_size_t(locale_id *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?_Id_cnt_func@id@locale@std@@CAAAHXZ */
/* ?_Id_cnt_func@id@locale@std@@CAAEAHXZ */
int* __cdecl locale_id__Id_cnt_func(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ??_Ffacet@locale@std@@QAEXXZ */
/* ??_Ffacet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor, 4)
locale_facet* __thiscall locale_facet_ctor(locale_facet *this)
{
    FIXME("(%p) stub\n", this);
    this->vtable = MSVCP_locale_facet_vtable;
    return NULL;
}

/* ??0facet@locale@std@@IAE@I@Z */
/* ??0facet@locale@std@@IEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor_refs, 8)
locale_facet* __thiscall locale_facet_ctor_refs(locale_facet *this, MSVCP_size_t refs)
{
    FIXME("(%p %lu) stub\n", this, refs);
    return NULL;
}

/* ??1facet@locale@std@@UAE@XZ */
/* ??1facet@locale@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet_dtor, 4)
void __thiscall locale_facet_dtor(locale_facet *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale_facet_vector_dtor, 8)
locale_facet* __thiscall MSVCP_locale_facet_vector_dtor(locale_facet *this, unsigned int flags)
{
    TRACE("(%p %x) stub\n", this, flags);
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

/* ?_Incref@facet@locale@std@@QAEXXZ */
/* ?_Incref@facet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet__Incref, 4)
void __thiscall locale_facet__Incref(locale_facet *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?_Decref@facet@locale@std@@QAEPAV123@XZ */
/* ?_Decref@facet@locale@std@@QEAAPEAV123@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet__Decref, 4)
locale_facet* __thiscall locale_facet__Decref(locale_facet *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Getcat@facet@locale@std@@SAIPAPBV123@PBV23@@Z */
/* ?_Getcat@facet@locale@std@@SA_KPEAPEBV123@PEBV23@@Z */
MSVCP_size_t __cdecl locale_facet__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

static const vtable_ptr MSVCP_locale_facet_vtable[] = {
    (vtable_ptr)THISCALL_NAME(MSVCP_locale_facet_vector_dtor)
};

/* ??_F_Locimp@locale@std@@QAEXXZ */
/* ??_F_Locimp@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor, 4)
locale__Locimp* __thiscall locale__Locimp_ctor(locale__Locimp *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??0_Locimp@locale@std@@AAE@_N@Z */
/* ??0_Locimp@locale@std@@AEAA@_N@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor_transparent, 8)
locale__Locimp* __thiscall locale__Locimp_ctor_transparent(locale__Locimp *this, MSVCP_bool transparent)
{
    FIXME("(%p %d) stub\n", this, transparent);
    return NULL;
}

/* ??0_Locimp@locale@std@@AAE@ABV012@@Z */
/* ??0_Locimp@locale@std@@AEAA@AEBV012@@Z */
/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPAV123@ABV123@@Z */
/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPEAV123@AEBV123@@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_copy_ctor, 8)
locale__Locimp* __thiscall locale__Locimp_copy_ctor(locale__Locimp *this, const locale__Locimp *copy)
{
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
}

/* ??1_Locimp@locale@std@@MAE@XZ */
/* ??1_Locimp@locale@std@@MEAA@XZ */
/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPAV123@@Z */
/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPEAV123@@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_dtor, 4)
void __thiscall locale__Locimp_dtor(locale__Locimp *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale__Locimp_vector_dtor, 8)
locale__Locimp* __thiscall MSVCP_locale__Locimp_vector_dtor(locale__Locimp *this, unsigned int flags)
{
    TRACE("(%p %x) stub\n", this, flags);
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
    FIXME("(%p %p %lu) stub\n", locimp, facet, id);
}

/* ?_Addfac@_Locimp@locale@std@@AAEXPAVfacet@23@I@Z */
/* ?_Addfac@_Locimp@locale@std@@AEAAXPEAVfacet@23@_K@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp__Addfac, 12)
void __thiscall locale__Locimp__Addfac(locale__Locimp *this, locale_facet *facet, MSVCP_size_t id)
{
    FIXME("(%p %p %lu) stub\n", this, facet, id);
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
    FIXME("(%p %p) stub\n", this, locimp);
    return NULL;
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
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
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
    FIXME("(%p %d) stub\n", this, uninitialized);
    return NULL;
}

/* ??0locale@std@@QAE@XZ */
/* ??0locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_ctor, 4)
locale* __thiscall locale_ctor(locale *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??1locale@std@@QAE@XZ */
/* ??1locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_dtor, 4)
void __thiscall locale_dtor(locale *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_locale_vector_dtor, 8)
locale* __thiscall MSVCP_locale_vector_dtor(locale *this, unsigned int flags)
{
    TRACE("(%p %x) stub\n", this, flags);
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
    FIXME("(%p %p %lu %lu) stub\n", this, facet, id, catmask);
    return NULL;
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
    basic_string_char ret = { 0 }; /* FIXME */
    FIXME( "(%p) stub\n", this);
    return ret;
}

/* ??0_Timevec@std@@QAE@ABV01@@Z */
/* ??0_Timevec@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_copy_ctor, 8)
_Timevec* __thiscall _Timevec_copy_ctor(_Timevec *this, const _Timevec *copy)
{
    FIXME("(%p %p) stub\n", this, copy);
    return NULL;
}

/* ??0_Timevec@std@@QAE@PAX@Z */
/* ??0_Timevec@std@@QEAA@PEAX@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor_timeptr, 8)
_Timevec* __thiscall _Timevec_ctor_timeptr(_Timevec *this, void *timeptr)
{
    FIXME("(%p %p) stub\n", this, timeptr);
    return NULL;
}

/* ??_F_Timevec@std@@QAEXXZ */
/* ??_F_Timevec@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor, 4)
_Timevec* __thiscall _Timevec_ctor(_Timevec *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??1_Timevec@std@@QAE@XZ */
/* ??1_Timevec@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Timevec_dtor, 4)
void __thiscall _Timevec_dtor(_Timevec *this)
{
    FIXME("(%p) stub\n", this);
}

/* ??4_Timevec@std@@QAEAAV01@ABV01@@Z */
/* ??4_Timevec@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_op_assign, 8)
_Timevec* __thiscall _Timevec_op_assign(_Timevec *this, _Timevec *right)
{
    FIXME("(%p %p) stub\n", this, right);
    return NULL;
}

/* ?_Getptr@_Timevec@std@@QBEPAXXZ */
/* ?_Getptr@_Timevec@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec__Getptr, 4)
void* __thiscall _Timevec__Getptr(_Timevec *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_bstr(_Locinfo *locinfo, const basic_string_char *locstr)
{
    FIXME("(%p %p) stub\n", locinfo, locstr);
    return NULL;
}

/* ??0_Locinfo@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0_Locinfo@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_bstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_bstr(_Locinfo *this, const basic_string_char *locstr)
{
    FIXME("(%p %p) stub\n", this, locstr);
    return NULL;
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@HPBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cat_cstr(_Locinfo *locinfo, int category, const char *locstr)
{
    FIXME("(%p %d %s) stub\n", locinfo, category, locstr);
    return NULL;
}

/* ??0_Locinfo@std@@QAE@HPBD@Z */
/* ??0_Locinfo@std@@QEAA@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cat_cstr, 12)
_Locinfo* __thiscall _Locinfo_ctor_cat_cstr(_Locinfo *this, int category, const char *locstr)
{
    FIXME("(%p %d %s) stub\n", this, category, locstr);
    return NULL;
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@PBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@PEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cstr(_Locinfo *locinfo, const char *locstr)
{
    FIXME("(%p %s) stub\n", locinfo, locstr);
    return NULL;
}

/* ??0_Locinfo@std@@QAE@PBD@Z */
/* ??0_Locinfo@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_cstr(_Locinfo *this, const char *locstr)
{
    FIXME("(%p %s) stub\n", this, locstr);
    return NULL;
}

/* ?_Locinfo_dtor@_Locinfo@std@@SAXPAV12@@Z */
/* ?_Locinfo_dtor@_Locinfo@std@@SAXPEAV12@@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_dtor(_Locinfo *locinfo)
{
    FIXME("(%p) stub\n", locinfo);
    return NULL;
}

/* ??_F_Locinfo@std@@QAEXXZ */
/* ??_F_Locinfo@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor, 4)
_Locinfo* __thiscall _Locinfo_ctor(_Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??1_Locinfo@std@@QAE@XZ */
/* ??1_Locinfo@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_dtor, 4)
void __thiscall _Locinfo_dtor(_Locinfo *this)
{
    FIXME("(%p) stub\n", this);
}

/* ?_Locinfo_Addcats@_Locinfo@std@@SAAAV12@PAV12@HPBD@Z */
/* ?_Locinfo_Addcats@_Locinfo@std@@SAAEAV12@PEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_Addcats(_Locinfo *locinfo, int category, const char *locstr)
{
    FIXME("%p %d %s) stub\n", locinfo, category, locstr);
    return NULL;
}

/* ?_Addcats@_Locinfo@std@@QAEAAV12@HPBD@Z */
/* ?_Addcats@_Locinfo@std@@QEAAAEAV12@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo__Addcats, 12)
_Locinfo* __thiscall _Locinfo__Addcats(_Locinfo *this, int category, const char *locstr)
{
    FIXME("(%p %d %s) stub\n", this, category, locstr);
    return NULL;
}

/* ?_Getcoll@_Locinfo@std@@QBE?AU_Collvec@@XZ */
/* ?_Getcoll@_Locinfo@std@@QEBA?AU_Collvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcoll, 4)
_Collvec __thiscall _Locinfo__Getcoll(const _Locinfo *this)
{
    _Collvec ret = { 0 }; /* FIXME */
    FIXME("(%p) stub\n", this);
    return ret;
}

/* ?_Getctype@_Locinfo@std@@QBE?AU_Ctypevec@@XZ */
/* ?_Getctype@_Locinfo@std@@QEBA?AU_Ctypevec@@XZ */
DEFINE_THISCALL_WRAPPER_RETPTR(_Locinfo__Getctype, 4)
_Ctypevec __thiscall _Locinfo__Getctype(const _Locinfo *this)
{
    _Ctypevec ret = { 0 }; /* FIXME */
    FIXME("(%p) stub\n", this);
    return ret;
}

/* ?_Getcvt@_Locinfo@std@@QBE?AU_Cvtvec@@XZ */
/* ?_Getcvt@_Locinfo@std@@QEBA?AU_Cvtvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcvt, 4)
_Cvtvec __thiscall _Locinfo__Getcvt(const _Locinfo *this)
{
    _Cvtvec ret = { 0 }; /* FIXME */
    FIXME("(%p) stub\n", this);
    return ret;
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
const char* __thiscall _Locinfo__Getdays(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Getmonths@_Locinfo@std@@QBEPBDXZ */
/* ?_Getmonths@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getmonths, 4)
const char* __thiscall _Locinfo__Getmonths(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Getfalse@_Locinfo@std@@QBEPBDXZ */
/* ?_Getfalse@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getfalse, 4)
const char* __thiscall _Locinfo__Getfalse(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Gettrue@_Locinfo@std@@QBEPBDXZ */
/* ?_Gettrue@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettrue, 4)
const char* __thiscall _Locinfo__Gettrue(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Getlconv@_Locinfo@std@@QBEPBUlconv@@XZ */
/* ?_Getlconv@_Locinfo@std@@QEBAPEBUlconv@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getlconv, 4)
const struct lconv* __thiscall _Locinfo__Getlconv(const _Locinfo *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ?_Getname@_Locinfo@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?_Getname@_Locinfo@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER_RETPTR(_Locinfo__Getname, 4)
basic_string_char __thiscall _Locinfo__Getname(const _Locinfo *this)
{
    basic_string_char ret = { 0 }; /* FIXME */
    FIXME("(%p) stub\n", this);
    return ret;
}

/* ?_Gettnames@_Locinfo@std@@QBE?AV_Timevec@2@XZ */
/* ?_Gettnames@_Locinfo@std@@QEBA?AV_Timevec@2@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettnames, 4)
_Timevec __thiscall _Locinfo__Gettnames(const _Locinfo *this)
{
    _Timevec ret = { 0 }; /* FIXME */
    FIXME("(%p) stub\n", this);
    return ret;
}
