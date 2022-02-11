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
#include <wctype.h>
#include <math.h>
#include <float.h>
#include <errno.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

static inline float __port_ind(void)
{
        static const unsigned __ind_bytes = 0xffc00000;
            return *(const float *)&__ind_bytes;
}
#define IND __port_ind()

typedef double LDOUBLE;  /* long double is just a double */

typedef struct {
    LCID handle;
    unsigned page;
    short *table;
    int delfl;
} MSVCP__Ctypevec;

typedef struct {
    LCID handle;
    unsigned page;
} MSVCP__Collvec;

/* basic_string<char, char_traits<char>, allocator<char>> */
#define BUF_SIZE_CHAR 16
typedef struct
{
    void *allocator;
    union {
        char buf[BUF_SIZE_CHAR];
        char *ptr;
    } data;
    size_t size;
    size_t res;
} basic_string_char;

/* class complex<float> */
typedef struct {
    float real;
    float imag;
} complex_float;

typedef struct {
    void *vtable;
    size_t refs;
} locale_facet;

typedef unsigned char MSVCP_bool;

typedef struct _locale__Locimp {
    locale_facet facet;
    locale_facet **facetvec;
    size_t facet_cnt;
    int catmask;
    MSVCP_bool transparent;
    basic_string_char name;
} locale__Locimp;

static void* (__cdecl *p_set_invalid_parameter_handler)(void*);
static _locale_t (__cdecl *p__get_current_locale)(void);
static void (__cdecl *p__free_locale)(_locale_t);
static void  (__cdecl *p_free)(void*);
static int * (__cdecl *p_errno)(void);

static void (__cdecl *p_char_assign)(void*, const void*);
static void (__cdecl *p_wchar_assign)(void*, const void*);
static void (__cdecl *p_short_assign)(void*, const void*);

static BYTE (__cdecl *p_char_eq)(const void*, const void*);
static BYTE (__cdecl *p_wchar_eq)(const void*, const void*);
static BYTE (__cdecl *p_short_eq)(const void*, const void*);

static char* (__cdecl *p_Copy_s)(char*, size_t, const char*, size_t);

static unsigned short (__cdecl *p_wctype)(const char*);
static MSVCP__Ctypevec (__cdecl *p__Getctype)(void);
static MSVCP__Collvec (__cdecl *p__Getcoll)(void);
static wctrans_t (__cdecl *p_wctrans)(const char*);
static wint_t (__cdecl *p_towctrans)(wint_t, wctrans_t);
static void (__cdecl *p_locale__Locimp__Locimp_Addfac)(locale__Locimp*,locale_facet*,size_t);
static size_t (__cdecl *p__Strxfrm)(char*, char*, const char*, const char*, const MSVCP__Collvec*);
static size_t (__cdecl *p__Wcsxfrm)(wchar_t*, wchar_t*, const wchar_t*,
        const wchar_t*, const MSVCP__Collvec*);

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

static char* (__thiscall *p_char_address)(void*, char*);
static void* (__thiscall *p_char_ctor)(void*);
static void (__thiscall *p_char_deallocate)(void*, char*, size_t);
static char* (__thiscall *p_char_allocate)(void*, size_t);
static void (__thiscall *p_char_construct)(void*, char*, const char*);
static size_t (__thiscall *p_char_max_size)(void*);

static void* (__thiscall *p_collate_char_ctor_refs)(void*, size_t);
static int (__thiscall *p_collate_char_compare)(const void*, const char*,
        const char*, const char*, const char*);
static void (__thiscall *p_collate_char_dtor)(void*);
static void* (__thiscall *p_numpunct_char_ctor)(void*);
static basic_string_char* (__thiscall *p_numpunct_char_falsename)(void*,basic_string_char*);
static void (__thiscall *p_numpunct_char_dtor)(void*);
static void (__thiscall *p_basic_string_char_dtor)(basic_string_char*);
static const char* (__thiscall *p_basic_string_char_cstr)(basic_string_char*);

static const int *basic_ostringstream_char_vbtable;
static /*basic_ostringstream_char*/void* (__thiscall *p_basic_ostringstream_char_ctor_mode)(
        /*basic_ostringstream_char*/void*, int, /*MSVCP_bool*/int);
static void (__thiscall *p_basic_ostringstream_char_dtor)(/*basic_ostringstream_char*/void*);
static void (__thiscall *p_basic_ostringstream_char_vbase_dtor)(/*basic_ostringstream_char*/void*);
static void (__thiscall *p_basic_ios_char_dtor)(/*basic_ios_char*/void*);

static BOOL (__cdecl *p_std_Ctraits_float__Isnan)(float);
static BOOL (__cdecl *p_std_Ctraits_double__Isnan)(double);
static BOOL (__cdecl *p_std_Ctraits_long_double__Isnan)(LDOUBLE);

static complex_float* (__thiscall *p_complex_float_ctor)(complex_float*, const float*, const float*);
static complex_float* (__cdecl *p_complex_float_add)(complex_float*, const complex_float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_div)(complex_float*, const complex_float*, const complex_float*);
static float (__cdecl *p_complex_float__Fabs)(const complex_float*, int*);
static complex_float* (__cdecl *p_complex_float_tan)(complex_float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_tanh)(complex_float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_log10)(complex_float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_sqrt)(complex_float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_pow_ci)(complex_float*, const complex_float*, int);
static complex_float* (__cdecl *p_complex_float_pow_fc)(complex_float*, const float*, const complex_float*);
static complex_float* (__cdecl *p_complex_float_pow_cf)(complex_float*, const complex_float*, const float*);

static void (CDECL **p_Raise_handler)(const void*);

static int invalid_parameter = 0;
static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %Ix\n", arg);
    invalid_parameter++;
}

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
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, const void *a, const void *b );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, const void *a, const void *b,
        const void *c, const void *d );

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
    call_thiscall_func5 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(const void*)(a),(const void*)(b), \
        (const void*)(c), (const void *)(d))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)

#endif /* __i386__ */

static HMODULE msvcr, msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcr = LoadLibraryA("msvcr90.dll");
    msvcp = LoadLibraryA("msvcp90.dll");
    if(!msvcr || !msvcp) {
        win_skip("msvcp90.dll or msvcrt90.dll not installed\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler = (void*)GetProcAddress(msvcr, "_set_invalid_parameter_handler");
    p__get_current_locale = (void*)GetProcAddress(msvcr, "_get_current_locale");
    p__free_locale = (void*)GetProcAddress(msvcr, "_free_locale");
    p_free = (void*)GetProcAddress(msvcr, "free");
    p_errno = (void*)GetProcAddress(msvcr, "_errno");
    if(!p_set_invalid_parameter_handler || !p__get_current_locale || !p__free_locale || !p_free || !p_errno) {
        win_skip("Error setting tests environment\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler(test_invalid_parameter_handler);

    SET(p_wctype, "wctype");
    SET(p__Getctype, "_Getctype");
    SET(p__Getcoll, "_Getcoll");
    SET(p_wctrans, "wctrans");
    SET(p_towctrans, "towctrans");
    SET(p__Strxfrm, "_Strxfrm");
    SET(p__Wcsxfrm, "_Wcsxfrm");
    SET(basic_ostringstream_char_vbtable, "??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@");

    SET(p_std_Ctraits_float__Isnan, "?_Isnan@?$_Ctraits@M@std@@SA_NM@Z");
    SET(p_std_Ctraits_double__Isnan, "?_Isnan@?$_Ctraits@N@std@@SA_NN@Z");
    SET(p_std_Ctraits_long_double__Isnan, "?_Isnan@?$_Ctraits@O@std@@SA_NO@Z");

    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_locale__Locimp__Locimp_Addfac,
                "?_Locimp_Addfac@_Locimp@locale@std@@CAXPEAV123@PEAVfacet@23@_K@Z");
        SET(p_char_assign, "?assign@?$char_traits@D@std@@SAXAEADAEBD@Z");
        SET(p_wchar_assign, "?assign@?$char_traits@_W@std@@SAXAEA_WAEB_W@Z");
        SET(p_short_assign, "?assign@?$char_traits@G@std@@SAXAEAGAEBG@Z");

        SET(p_char_eq, "?eq@?$char_traits@D@std@@SA_NAEBD0@Z");
        SET(p_wchar_eq, "?eq@?$char_traits@_W@std@@SA_NAEB_W0@Z");
        SET(p_short_eq, "?eq@?$char_traits@G@std@@SA_NAEBG0@Z");

        SET(p_Copy_s, "?_Copy_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z");

        SET(p_char_address, "?address@?$allocator@D@std@@QEBAPEADAEAD@Z");
        SET(p_char_ctor, "??0?$allocator@D@std@@QEAA@XZ");
        SET(p_char_deallocate, "?deallocate@?$allocator@D@std@@QEAAXPEAD_K@Z");
        SET(p_char_allocate, "?allocate@?$allocator@D@std@@QEAAPEAD_K@Z");
        SET(p_char_construct, "?construct@?$allocator@D@std@@QEAAXPEADAEBD@Z");
        SET(p_char_max_size, "?max_size@?$allocator@D@std@@QEBA_KXZ");

        SET(p_collate_char_ctor_refs, "??0?$collate@D@std@@QEAA@_K@Z");
        SET(p_collate_char_compare, "?compare@?$collate@D@std@@QEBAHPEBD000@Z");
        SET(p_collate_char_dtor, "??1?$collate@D@std@@MEAA@XZ");
        SET(p_numpunct_char_ctor, "??_F?$numpunct@D@std@@QEAAXXZ");
        SET(p_numpunct_char_falsename, "?falsename@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ");
        SET(p_numpunct_char_dtor, "??1?$numpunct@D@std@@MEAA@XZ");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ");

        SET(p_basic_ostringstream_char_ctor_mode,
                "??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z");
        SET(p_basic_ostringstream_char_dtor,
                "??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ");
        SET(p_basic_ostringstream_char_vbase_dtor,
                "??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ");
        SET(p_basic_ios_char_dtor,
                "??1?$basic_ios@DU?$char_traits@D@std@@@std@@UEAA@XZ");

        SET(p_complex_float_ctor,
                "??0?$complex@M@std@@QEAA@AEBM0@Z");
        SET(p_complex_float_add,
                "??$?HM@std@@YA?AV?$complex@M@0@AEBV10@0@Z");
        SET(p_complex_float_div,
                "??$?KM@std@@YA?AV?$complex@M@0@AEBV10@0@Z");
        SET(p_complex_float__Fabs,
                "??$_Fabs@M@std@@YAMAEBV?$complex@M@0@PEAH@Z");
        SET(p_complex_float_tan,
                "??$tan@M@std@@YA?AV?$complex@M@0@AEBV10@@Z");
        SET(p_complex_float_tanh,
                "??$tanh@M@std@@YA?AV?$complex@M@0@AEBV10@@Z");
        SET(p_complex_float_log10,
                "??$log10@M@std@@YA?AV?$complex@M@0@AEBV10@@Z");
        SET(p_complex_float_sqrt,
                "??$sqrt@M@std@@YA?AV?$complex@M@0@AEBV10@@Z");
        SET(p_complex_float_pow_ci,
                "??$pow@M@std@@YA?AV?$complex@M@0@AEBV10@H@Z");
        SET(p_complex_float_pow_fc,
                "??$pow@M@std@@YA?AV?$complex@M@0@AEBMAEBV10@@Z");
        SET(p_complex_float_pow_cf,
                "??$pow@M@std@@YA?AV?$complex@M@0@AEBV10@AEBM@Z");
        SET(p_Raise_handler,
                "?_Raise_handler@std@@3P6AXAEBVexception@stdext@@@ZEA");
    } else {
        SET(p_locale__Locimp__Locimp_Addfac,
                "?_Locimp_Addfac@_Locimp@locale@std@@CAXPAV123@PAVfacet@23@I@Z");
#ifdef __arm__
        SET(p_char_assign, "?assign@?$char_traits@D@std@@SAXAADABD@Z");
        SET(p_wchar_assign, "?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z");
        SET(p_short_assign, "?assign@?$char_traits@G@std@@SAXAAGABG@Z");

        SET(p_char_eq, "?eq@?$char_traits@D@std@@SA_NABD0@Z");
        SET(p_wchar_eq, "?eq@?$char_traits@_W@std@@SA_NAB_W0@Z");
        SET(p_short_eq, "?eq@?$char_traits@G@std@@SA_NABG0@Z");

        SET(p_Copy_s, "?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z");

        SET(p_char_address, "?address@?$allocator@D@std@@QBEPADAAD@Z");
        SET(p_char_ctor, "??0?$allocator@D@std@@QAE@XZ");
        SET(p_char_deallocate, "?deallocate@?$allocator@D@std@@QAEXPADI@Z");
        SET(p_char_allocate, "?allocate@?$allocator@D@std@@QAEPADI@Z");
        SET(p_char_construct, "?construct@?$allocator@D@std@@QAEXPADABD@Z");
        SET(p_char_max_size, "?max_size@?$allocator@D@std@@QBEIXZ");

        SET(p_collate_char_ctor_refs, "??0?$collate@D@std@@QAE@I@Z");
        SET(p_collate_char_compare, "?compare@?$collate@D@std@@QBEHPBD000@Z");
        SET(p_collate_char_dtor, "??1?$collate@D@std@@MAE@XZ");
        SET(p_numpunct_char_ctor, "??_F?$numpunct@D@std@@QAEXXZ");
        SET(p_numpunct_char_falsename, "?falsename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ");
        SET(p_numpunct_char_dtor, "??1?$numpunct@D@std@@MAE@XZ");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");

        SET(p_basic_ostringstream_char_ctor_mode,
                "??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z");
        SET(p_basic_ostringstream_char_dtor,
                "??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ");
        SET(p_basic_ostringstream_char_vbase_dtor,
                "??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ");
        SET(p_basic_ios_char_dtor,
                "??1?$basic_ios@DU?$char_traits@D@std@@@std@@UAA@XZ");

        SET(p_complex_float_ctor,
                "??0?$complex@M@std@@QAE@ABM0@Z");
        SET(p_complex_float_add,
                "??$?HM@std@@YA?AV?$complex@M@0@ABV10@0@Z");
        SET(p_complex_float_div,
                "??$?KM@std@@YA?AV?$complex@M@0@ABV10@0@Z");
        SET(p_complex_float__Fabs,
                "??$_Fabs@M@std@@YAMABV?$complex@M@0@PAH@Z");
        SET(p_complex_float_tan,
                "??$tan@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_tanh,
                "??$tanh@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_log10,
                "??$log10@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_sqrt,
                "??$sqrt@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_pow_ci,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABV10@H@Z");
        SET(p_complex_float_pow_fc,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABMABV10@@Z");
        SET(p_complex_float_pow_cf,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABV10@ABM@Z");
#else
        SET(p_char_assign, "?assign@?$char_traits@D@std@@SAXAADABD@Z");
        SET(p_wchar_assign, "?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z");
        SET(p_short_assign, "?assign@?$char_traits@G@std@@SAXAAGABG@Z");

        SET(p_char_eq, "?eq@?$char_traits@D@std@@SA_NABD0@Z");
        SET(p_wchar_eq, "?eq@?$char_traits@_W@std@@SA_NAB_W0@Z");
        SET(p_short_eq, "?eq@?$char_traits@G@std@@SA_NABG0@Z");

        SET(p_Copy_s, "?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z");

        SET(p_char_address, "?address@?$allocator@D@std@@QBEPADAAD@Z");
        SET(p_char_ctor, "??0?$allocator@D@std@@QAE@XZ");
        SET(p_char_deallocate, "?deallocate@?$allocator@D@std@@QAEXPADI@Z");
        SET(p_char_allocate, "?allocate@?$allocator@D@std@@QAEPADI@Z");
        SET(p_char_construct, "?construct@?$allocator@D@std@@QAEXPADABD@Z");
        SET(p_char_max_size, "?max_size@?$allocator@D@std@@QBEIXZ");

        SET(p_collate_char_ctor_refs, "??0?$collate@D@std@@QAE@I@Z");
        SET(p_collate_char_compare, "?compare@?$collate@D@std@@QBEHPBD000@Z");
        SET(p_collate_char_dtor, "??1?$collate@D@std@@MAE@XZ");
        SET(p_numpunct_char_ctor, "??_F?$numpunct@D@std@@QAEXXZ");
        SET(p_numpunct_char_falsename, "?falsename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ");
        SET(p_numpunct_char_dtor, "??1?$numpunct@D@std@@MAE@XZ");
        SET(p_basic_string_char_dtor,
                "??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ");
        SET(p_basic_string_char_cstr,
                "?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ");

        SET(p_basic_ostringstream_char_ctor_mode,
                "??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z");
        SET(p_basic_ostringstream_char_dtor,
                "??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ");
        SET(p_basic_ostringstream_char_vbase_dtor,
                "??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ");
        SET(p_basic_ios_char_dtor,
                "??1?$basic_ios@DU?$char_traits@D@std@@@std@@UAE@XZ");

        SET(p_complex_float_ctor,
                "??0?$complex@M@std@@QAE@ABM0@Z");
        SET(p_complex_float_add,
                "??$?HM@std@@YA?AV?$complex@M@0@ABV10@0@Z");
        SET(p_complex_float_div,
                "??$?KM@std@@YA?AV?$complex@M@0@ABV10@0@Z");
        SET(p_complex_float__Fabs,
                "??$_Fabs@M@std@@YAMABV?$complex@M@0@PAH@Z");
        SET(p_complex_float_tan,
                "??$tan@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_tanh,
                "??$tanh@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_log10,
                "??$log10@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_sqrt,
                "??$sqrt@M@std@@YA?AV?$complex@M@0@ABV10@@Z");
        SET(p_complex_float_pow_ci,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABV10@H@Z");
        SET(p_complex_float_pow_fc,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABMABV10@@Z");
        SET(p_complex_float_pow_cf,
                "??$pow@M@std@@YA?AV?$complex@M@0@ABV10@ABM@Z");
        SET(p_Raise_handler,
                "?_Raise_handler@std@@3P6AXABVexception@stdext@@@ZA");
#endif
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test_assign(void)
{
    const char in[] = "abc";
    char out[4];

    out[1] = '#';
    p_char_assign(out, in);
    ok(out[0] == in[0], "out[0] = %c\n", out[0]);
    ok(out[1] == '#', "out[1] = %c\n", out[1]);

    out[2] = '#';
    p_wchar_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);

    out[2] = '#';
    p_short_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);
}

static void test_equal(void)
{
    static const char in1[] = "abc";
    static const char in2[] = "ab";
    static const char in3[] = "a";
    static const char in4[] = "b";
    BYTE ret;

    ret = p_char_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in3);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_wchar_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_short_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
}

static void test_Copy_s(void)
{
    static const char src[] = "abcd";
    char dest[32], *ret;

    dest[4] = '#';
    dest[5] = '\0';
    ret = p_Copy_s(dest, 4, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    ret = p_Copy_s(dest, 32, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    *p_errno() = 0xdeadbeef;
    dest[0] = '#';
    ret = p_Copy_s(dest, 3, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[0] == '\0', "dest[0] != 0\n");
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(*p_errno() == ERANGE, "errno = %d\n", *p_errno());

    *p_errno() = 0xdeadbeef;
    p_Copy_s(NULL, 32, src, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(*p_errno() == EINVAL, "errno = %d\n", *p_errno());

    *p_errno() = 0xdeadbeef;
    p_Copy_s(dest, 32, NULL, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(*p_errno() == EINVAL, "errno = %d\n", *p_errno());
}

static void test_wctype(void)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum",  0x107 },
        { "alpha",  0x103 },
        { "cntrl",  0x020 },
        { "digit",  0x004 },
        { "graph",  0x117 },
        { "lower",  0x002 },
        { "print",  0x157 },
        { "punct",  0x010 },
        { "space",  0x008 },
        { "upper",  0x001 },
        { "xdigit", 0x080 },
        { "ALNUM",  0x000 },
        { "Alnum",  0x000 },
        { "",  0x000 }
    };
    int i, ret;

    for(i=0; i<ARRAY_SIZE(properties); i++) {
        ret = p_wctype(properties[i].name);
        ok(properties[i].mask == ret, "%d - Expected %x, got %x\n", i, properties[i].mask, ret);
    }
}

static void test__Getctype(void)
{
    MSVCP__Ctypevec ret;
    _locale_t locale;

    ret = p__Getctype();
    ok(ret.handle == 0, "ret.handle = %ld\n", ret.handle);
    ok(ret.page == 0, "ret.page = %d\n", ret.page);
    ok(ret.delfl == 1, "ret.delfl = %d\n", ret.delfl);
    ok(ret.table[0] == 32, "ret.table[0] = %d\n", ret.table[0]);
    p_free(ret.table);

    locale = p__get_current_locale();
    locale->locinfo->lc_handle[LC_COLLATE] = 0x1234567;
    p__free_locale(locale);
    ret = p__Getctype();
    ok(ret.handle == 0x1234567, "ret.handle = %ld\n", ret.handle);
    ok(ret.page == 0, "ret.page = %d\n", ret.page);
    ok(ret.delfl == 1, "ret.delfl = %d\n", ret.delfl);
    ok(ret.table[0] == 32, "ret.table[0] = %d\n", ret.table[0]);
    p_free(ret.table);
}

static void test__Getcoll(void)
{
#ifdef __i386__
    /* Workaround a gcc bug */
    ULONGLONG tmp;
#define call__Getcoll(ret) tmp = ((ULONGLONG (__cdecl*)(void))p__Getcoll)(); \
    memcpy(&ret, &tmp, sizeof(tmp))
#else
#define call__Getcoll(ret) ret = p__Getcoll()
#endif
    _locale_t locale;
    MSVCP__Collvec ret;

    locale = p__get_current_locale();
    locale->locinfo->lc_handle[LC_COLLATE] = 0x7654321;
    p__free_locale(locale);
    call__Getcoll(ret);
    ok(ret.handle == 0x7654321, "ret.handle = %lx\n", ret.handle);
    ok(ret.page == 0, "ret.page = %x\n", ret.page);
}

static void test_towctrans(void)
{
    wchar_t ret;

    ret = p_wctrans("tolower");
    ok(ret == 2, "wctrans returned %d, expected 2\n", ret);
    ret = p_wctrans("toupper");
    ok(ret == 1, "wctrans returned %d, expected 1\n", ret);
    ret = p_wctrans("toLower");
    ok(ret == 0, "wctrans returned %d, expected 0\n", ret);
    ret = p_wctrans("");
    ok(ret == 0, "wctrans returned %d, expected 0\n", ret);
    if(0) { /* crashes on windows */
        ret = p_wctrans(NULL);
        ok(ret == 0, "wctrans returned %d, expected 0\n", ret);
    }

    ret = p_towctrans('t', 2);
    ok(ret == 't', "towctrans('t', 2) returned %c, expected t\n", ret);
    ret = p_towctrans('T', 2);
    ok(ret == 't', "towctrans('T', 2) returned %c, expected t\n", ret);
    ret = p_towctrans('T', 0);
    ok(ret == 't', "towctrans('T', 0) returned %c, expected t\n", ret);
    ret = p_towctrans('T', 3);
    ok(ret == 't', "towctrans('T', 3) returned %c, expected t\n", ret);
    ret = p_towctrans('t', 1);
    ok(ret == 'T', "towctrans('t', 1) returned %c, expected T\n", ret);
    ret = p_towctrans('T', 1);
    ok(ret == 'T', "towctrans('T', 1) returned %c, expected T\n", ret);
}

static void test_allocator_char(void)
{
    void *allocator = (void*)0xdeadbeef;
    char *ptr;
    char val;
    unsigned int size;

    allocator = call_func1(p_char_ctor, allocator);
    ok(allocator == (void*)0xdeadbeef, "allocator = %p\n", allocator);

    ptr = call_func2(p_char_address, NULL, (void*)0xdeadbeef);
    ok(ptr == (void*)0xdeadbeef, "incorrect address (%p)\n", ptr);

    ptr = NULL;
    ptr = call_func2(p_char_allocate, allocator, 10);
    ok(ptr != NULL, "Memory allocation failed\n");

    ptr[0] = ptr[1] = '#';
    val = 'a';
    call_func3(p_char_construct, allocator, ptr, &val);
    val = 'b';
    call_func3(p_char_construct, allocator, (ptr+1), &val);
    ok(ptr[0] == 'a', "ptr[0] = %c\n", ptr[0]);
    ok(ptr[1] == 'b', "ptr[1] = %c\n", ptr[1]);

    call_func3(p_char_deallocate, allocator, ptr, -1);

    size = (unsigned int)call_func1(p_char_max_size, allocator);
    ok(size == (unsigned int)0xffffffff, "size = %x\n", size);
}

static void test_virtual_call(void)
{
    BYTE this[256];
    basic_string_char bstr;
    _locale_t locale;
    const char *p;
    char str1[] = "test";
    char str2[] = "TEST";
    int ret;

    locale = p__get_current_locale();
    locale->locinfo->lc_handle[LC_COLLATE] = 1;
    p__free_locale(locale);
    call_func2(p_collate_char_ctor_refs, this, 0);
    ret = (int)call_func5(p_collate_char_compare, this, str1, str1+4, str1, str1+4);
    ok(ret == 0, "collate<char>::compare returned %d\n", ret);
    ret = (int)call_func5(p_collate_char_compare, this, str2, str2+4, str1, str1+4);
    ok(ret == 1, "collate<char>::compare returned %d\n", ret);
    ret = (int)call_func5(p_collate_char_compare, this, str1, str1+3, str1, str1+4);
    ok(ret == -1, "collate<char>::compare returned %d\n", ret);
    call_func1(p_collate_char_dtor, this);

    call_func1(p_numpunct_char_ctor, this);
    call_func2(p_numpunct_char_falsename, this, &bstr);
    p = call_func1(p_basic_string_char_cstr, &bstr);
    ok(!strcmp(p, "false"), "numpunct<char>::falsename returned %s\n", p);
    call_func1(p_basic_string_char_dtor, &bstr);
    call_func1(p_numpunct_char_dtor, this);
}

static void test_virtual_base_dtors(void)
{
    char this[512];

    call_func3(p_basic_ostringstream_char_ctor_mode, this, 0, 1);
    call_func1(p_basic_ostringstream_char_vbase_dtor, this);

    /* this test uses vbtable set by earlier test */
    call_func3(p_basic_ostringstream_char_ctor_mode, this, 0, 0);
    call_func1(p_basic_ostringstream_char_dtor, this+basic_ostringstream_char_vbtable[1]);
    call_func1(p_basic_ios_char_dtor, this+((int**)this)[0][1]);
}

static BOOL almost_eq(float f1, float f2)
{
    f1 = (f1-f2)/f1;
    if(f1 < 0)
        f1 = -f1;
    return f1 < 0.0001;
}

static void test_Ctraits_math_functions(void)
{
    BYTE ret;

    ret = p_std_Ctraits_float__Isnan(0.0);
    ok(ret == FALSE, "ret = %d\n", ret);

    ret = p_std_Ctraits_float__Isnan(INFINITY);
    ok(ret == FALSE, "ret = %d\n", ret);

    ret = p_std_Ctraits_float__Isnan(-INFINITY);
    ok(ret == FALSE, "ret = %d\n", ret);

    ret = p_std_Ctraits_float__Isnan(IND);
    ok(ret == TRUE, "ret = %d\n", ret);

    ret = p_std_Ctraits_float__Isnan(NAN);
    ok(ret == TRUE, "ret = %d\n", ret);

    ret = p_std_Ctraits_double__Isnan(INFINITY);
    ok(ret == FALSE, "ret = %d\n", ret);

    ret = p_std_Ctraits_double__Isnan(IND);
    ok(ret == TRUE, "ret = %d\n", ret);

    ret = p_std_Ctraits_double__Isnan(NAN);
    ok(ret == TRUE, "ret = %d\n", ret);

    ret = p_std_Ctraits_long_double__Isnan(INFINITY);
    ok(ret == FALSE, "ret = %d\n", ret);

    ret = p_std_Ctraits_long_double__Isnan(IND);
    ok(ret == TRUE, "ret = %d\n", ret);

    ret = p_std_Ctraits_long_double__Isnan(NAN);
    ok(ret == TRUE, "ret = %d\n", ret);
}

static void test_complex(void)
{
    complex_float c1, c2, c3;
    float f1, f2;
    int scale;
    int r;

    f1 = 1;
    f2 = 2;
    call_func3(p_complex_float_ctor, &c1, &f1, &f2);
    ok(c1.real == 1, "c1.real = %f\n", c1.real);
    ok(c1.imag == 2, "c1.imag = %f\n", c1.imag);

    f1 = p_complex_float__Fabs(&c1, &scale);
    ok(almost_eq(f1, 0.559017), "abs(1+2i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    f1 = -1;
    f2 = 1;
    call_func3(p_complex_float_ctor, &c2, &f1, &f2);
    ok(c2.real == -1, "c2.real = %f\n", c1.real);
    ok(c2.imag == 1, "c2.imag = %f\n", c1.imag);

    f1 = p_complex_float__Fabs(&c2, &scale);
    ok(almost_eq(f1, 0.353553), "abs(1-1i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    p_complex_float_add(&c3, &c1, &c2);
    ok(c3.real == 0, "c3.real = %f\n", c3.real);
    ok(c3.imag == 3, "c3.imag = %f\n", c3.imag);

    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 0.75), "abs(0+3i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    p_complex_float_add(&c2, &c1, &c3);
    ok(c2.real == 1, "c2.real = %f\n", c1.real);
    ok(c2.imag == 5, "c2.imag = %f\n", c1.imag);

    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 0.75), "abs(1+5i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    /* (1+5i) / (0+3i) */
    p_complex_float_div(&c1, &c2, &c3);
    ok(almost_eq(c1.real, 1.666667), "c1.real = %f\n", c1.real);
    ok(almost_eq(c1.imag, -0.33333), "c1.imag = %f\n", c1.imag);

    /* (1+5i) / (2+0i) */
    c3.real = 2;
    c3.imag = 0;
    p_complex_float_div(&c1, &c2, &c3);
    ok(almost_eq(c1.real, 0.5), "c1.real = %f\n", c1.real);
    ok(almost_eq(c1.imag, 2.5), "c1.imag = %f\n", c1.imag);

    f1 = p_complex_float__Fabs(&c1, &scale);
    ok(almost_eq(f1, 0.637377), "abs(0.5+2.5i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    /* (1+5i) / 0 */
    c3.real = 0;
    p_complex_float_div(&c1, &c2, &c3);
    ok(_isnan(c1.real), "c1.real = %f\n", c1.real);
    ok(_isnan(c1.imag), "c1.imag = %f\n", c1.imag);

    f1 = p_complex_float__Fabs(&c1, &scale);
    ok(_isnan(f1), "abs(NaN+NaNi) = %f\n", f1);
    ok(scale == 0, "scale = %d\n", scale);

    /* (1+5i) / (NaN-2i) */
    c1.imag = -2;
    p_complex_float_div(&c3, &c2, &c1);
    ok(_isnan(c3.real), "c3.real = %f\n", c3.real);
    ok(_isnan(c3.imag), "c3.imag = %f\n", c3.imag);

    /* (NaN-2i) / (1+0i) */
    c2.imag = 0;
    p_complex_float_div(&c3, &c1, &c2);
    ok(_isnan(c3.real), "c3.real = %f\n", c3.real);
    ok(_isnan(c3.imag), "c3.imag = %f\n", c3.imag);

    /* (1+0i) + (NaN-2i) */
    p_complex_float_add(&c3, &c2, &c1);
    ok(_isnan(c3.real), "c3.real = %f\n", c3.real);
    ok(c3.imag == -2, "c3.imag = %f\n", c3.imag);

    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(_isnan(f1), "abs(NaN-2i) = %f\n", f1);
    ok(scale == 0, "scale = %d\n", scale);

    c3.real = 1000;
    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 250.000504), "abs(1000-2i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    c3.real = 0.1;
    c3.imag = 0.1;
    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 0.565685), "abs(0.1+0.1i) = %f\n", f1);
    ok(scale == -2, "scale = %d\n", scale);

    c3.real = 1;
    c3.imag = 0;
    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 0.25), "abs(1+0i) = %f\n", f1);
    ok(scale == 2, "scale = %d\n", scale);

    c3.real = 0.99;
    c3.imag = 0;
    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(almost_eq(f1, 3.96), "abs(0.99+0i) = %f\n", f1);
    ok(scale == -2, "scale = %d\n", scale);

    c3.real = 0;
    c3.imag = 0;
    f1 = p_complex_float__Fabs(&c3, &scale);
    ok(f1 == 0, "abs(0+0i) = %f\n", f1);
    ok(scale == 0, "scale = %d\n", scale);

    c1.real = 0;
    c1.imag = 0;
    r = 3;
    f1 = 3.312;
    p_complex_float_tan(&c2, &c1);
    ok(c2.real == 0, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_tanh(&c2, &c1);
    ok(c2.real == 0, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_log10(&c2, &c1);
    ok(c2.real < -FLT_MAX /* c2.real == -inf */, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %g\n", c2.imag);
    p_complex_float_sqrt(&c2, &c1);
    ok(c2.real == 0, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(c2.real == 0, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_pow_fc(&c2, &f1, &c1);
    ok(c2.real == 1, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_pow_cf(&c2, &c1, &f1);
    ok(c2.real == 0, "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);

    c1.real = 3.14159/2;
    c1.imag = 0;
    p_complex_float_tan(&c2, &c1);
    ok(almost_eq(c2.real, 788906.062500), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_tanh(&c2, &c1);
    ok(almost_eq(c2.real, 0.917152), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_log10(&c2, &c1);
    ok(almost_eq(c2.real, 0.196120), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %g\n", c2.imag);
    p_complex_float_sqrt(&c2, &c1);
    ok(almost_eq(c2.real, 1.253314), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, 3.875775), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %g\n", c2.imag);
    r = -r;
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, 0.258013), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %g\n", c2.imag);
    r = -r;
    p_complex_float_pow_fc(&c2, &f1, &c1);
    ok(almost_eq(c2.real, 6.560778), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);
    p_complex_float_pow_cf(&c2, &c1, &f1);
    ok(almost_eq(c2.real, 4.462188), "c2.real = %f\n", c2.real);
    ok(c2.imag == 0, "c2.imag = %f\n", c2.imag);

    c1.real = 7.12;
    c1.imag = 0.17;
    p_complex_float_tan(&c2, &c1);
    ok(almost_eq(c2.real, 1.040818), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.362651), "c2.imag = %f\n", c2.imag);
    p_complex_float_tanh(&c2, &c1);
    ok(almost_eq(c2.real, 0.999999), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 4.3627e-7), "c2.imag = %g\n", c2.imag);
    p_complex_float_log10(&c2, &c1);
    ok(almost_eq(c2.real, 0.852604), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.0103674), "c2.imag = %g\n", c2.imag);
    p_complex_float_sqrt(&c2, &c1);
    ok(almost_eq(c2.real, 2.668523), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.0318528), "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, 360.326782), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 25.849230), "c2.imag = %g\n", c2.imag);
    r = -r;
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, 0.002761), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, -0.000198073), "c2.imag = %g\n", c2.imag);
    r = -r;
    p_complex_float_pow_fc(&c2, &f1, &c1);
    ok(almost_eq(c2.real, 4942.879395), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 1020.427368), "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_cf(&c2, &c1, &f1);
    ok(almost_eq(c2.real, 664.453918), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 52.643879), "c2.imag = %g\n", c2.imag);

    c1.real = 0.14;
    c1.imag = 0.19;
    p_complex_float_tan(&c2, &c1);
    ok(almost_eq(c2.real, 0.135859), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.191341), "c2.imag = %f\n", c2.imag);
    p_complex_float_tanh(&c2, &c1);
    ok(almost_eq(c2.real, 0.144134), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.188464), "c2.imag = %f\n", c2.imag);
    p_complex_float_log10(&c2, &c1);
    ok(almost_eq(c2.real, -0.627072), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.4064), "c2.imag = %g\n", c2.imag);
    p_complex_float_sqrt(&c2, &c1);
    ok(almost_eq(c2.real, 0.433595), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.219099), "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, -0.012418), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.004313), "c2.imag = %g\n", c2.imag);
    r = -r;
    p_complex_float_pow_ci(&c2, &c1, r);
    ok(almost_eq(c2.real, -71.859810), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, -24.958229), "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_fc(&c2, &f1, &c1);
    ok(almost_eq(c2.real, 1.152052), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.266751), "c2.imag = %g\n", c2.imag);
    p_complex_float_pow_cf(&c2, &c1, &f1);
    ok(almost_eq(c2.real, -0.008370), "c2.real = %f\n", c2.real);
    ok(almost_eq(c2.imag, 0.00035447), "c2.imag = %g\n", c2.imag);
}

static struct {
    int value[2];
    const char* export_name;
} vbtable_size_exports_list[] = {
    {{0x5c, 0xa8}, "??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x54, 0x98}, "??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x5c, 0xa8}, "??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x54, 0x98}, "??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x5c, 0xa8}, "??_8?$basic_fstream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x54, 0x98}, "??_8?$basic_fstream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x58, 0xa0}, "??_8?$basic_ifstream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x58, 0xa0}, "??_8?$basic_ifstream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x58, 0xa0}, "??_8?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0xc, 0x18}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{ 0x4,  0x8}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{ 0xc, 0x18}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{ 0x4,  0x8}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{ 0xc, 0x18}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{ 0x4,  0x8}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{ 0x8, 0x10}, "??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{0x50, 0x90}, "??_8?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@"},
    {{0x50, 0x90}, "??_8?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@"},
    {{0x50, 0x90}, "??_8?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_ofstream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_ofstream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0x4,  0x8}, "??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x4,  0x8}, "??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x4,  0x8}, "??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{0x4c, 0x88}, "??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@"},
    {{0x4c, 0x88}, "??_8?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@"},
    {{0x4c, 0x88}, "??_8?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@"},
    {{0x54, 0x98}, "??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x4c, 0x88}, "??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x54, 0x98}, "??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x4c, 0x88}, "??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x54, 0x98}, "??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x4c, 0x88}, "??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{ 0x0,  0x0}, 0}
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



static void test_locale__Locimp__Locimp_Addfac(void)
{
    locale__Locimp locimp;
    locale_facet facet;

    memset(&locimp, 0, sizeof(locimp));
    memset(&facet, 0, sizeof(facet));
    p_locale__Locimp__Locimp_Addfac(&locimp, &facet, 1);
    ok(locimp.facet_cnt == 40, "locimp.facet_cnt = %d\n", (int)locimp.facet_cnt);
}

static void CDECL raise_handler(const void *except)
{
    ok(0, "unexpected call\n");
}

static void test_raise_handler(void)
{
    ok(!*p_Raise_handler, "_Raise_handler = %p\n", *p_Raise_handler);
    *p_Raise_handler = raise_handler;
    *p_Raise_handler = NULL;
}

static void test__Strxfrm(void)
{
    const char in[] = "abc";

    MSVCP__Collvec coll;
    char out[64];
    size_t ret;

    memset(&coll, 0, sizeof(coll));

    out[0] = 'z';
    ret = p__Strxfrm(out, out + 1, in, in + 2, &coll);
    ok(ret == 2, "ret = %d\n", (int)ret);
    ok(out[0] == 'z', "out[0] = %x\n", out[0]);

    ret = p__Strxfrm(out, out + sizeof(out), in, in + 4, &coll);
    ok(ret == 4, "ret = %d\n", (int)ret);
    ok(!strcmp(in, out), "out = %s\n", out);
}

static void test__Wcsxfrm(void)
{
    const wchar_t in[] = L"abc";

    MSVCP__Collvec coll;
    wchar_t out[64];
    size_t ret;

    memset(&coll, 0, sizeof(coll));

    out[0] = 'z';
    ret = p__Wcsxfrm(out, out + 1, in, in + 2, &coll);
    ok(ret == 2, "ret = %d\n", (int)ret);
    ok(out[0] == 'z', "out[0] = %x\n", out[0]);

    ret = p__Wcsxfrm(out, out + ARRAY_SIZE(out), in, in + 4, &coll);
    ok(ret == 4, "ret = %d\n", (int)ret);
    ok(!wcscmp(in, out), "out = %s\n", wine_dbgstr_w(out));
}

START_TEST(misc)
{
    if(!init())
        return;

    test_assign();
    test_equal();
    test_Copy_s();
    test_wctype();
    test__Getctype();
    test__Getcoll();
    test_towctrans();
    test_virtual_call();
    test_virtual_base_dtors();
    test_allocator_char();
    test_Ctraits_math_functions();
    test_complex();
    test_vbtable_size_exports();
    test_locale__Locimp__Locimp_Addfac();
    test_raise_handler();
    test__Strxfrm();
    test__Wcsxfrm();

    ok(!invalid_parameter, "invalid_parameter_handler was invoked too many times\n");

    FreeLibrary(msvcr);
    FreeLibrary(msvcp);
}
