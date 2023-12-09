/*
 * Copyright 2014 Yifu Wang for ESRI
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
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <fenv.h>
#include <limits.h>
#include <wctype.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"
#include <process.h>

#include <locale.h>

#define _MAX__TIME64_T     (((__time64_t)0x00000007 << 32) | 0x93406FFF)

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

static ULONG_PTR (WINAPI *call_thiscall_func1)( void *func, void *this );
static ULONG_PTR (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );
static ULONG_PTR (WINAPI *call_thiscall_func3)( void *func,
        void *this, const void *a, const void *b );

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
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(const void*)(a),(const void*)(b))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)

#endif /* __i386__ */

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef unsigned char MSVCRT_bool;

typedef struct cs_queue
{
    void *ctx;
    struct cs_queue *next;
    BOOL free;
    int unknown;
} cs_queue;

typedef struct
{
    cs_queue unk_active;
    void *unknown[2];
    cs_queue *head;
    void *tail;
} critical_section;

typedef struct
{
    critical_section *cs;
    void *unknown[4];
    int unknown2[2];
} critical_section_scoped_lock;

typedef struct {
    void *chain;
    critical_section lock;
} _Condition_variable;

struct MSVCRT_lconv
{
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    wchar_t* _W_decimal_point;
    wchar_t* _W_thousands_sep;
    wchar_t* _W_int_curr_symbol;
    wchar_t* _W_currency_symbol;
    wchar_t* _W_mon_decimal_point;
    wchar_t* _W_mon_thousands_sep;
    wchar_t* _W_positive_sign;
    wchar_t* _W_negative_sign;
};

typedef struct
{
    double r;
    double i;
} _Dcomplex;

typedef void (*vtable_ptr)(void);

typedef struct {
    const vtable_ptr *vtable;
} Context;

typedef struct {
    Context *ctx;
} _Context;

typedef struct _StructuredTaskCollection
{
    void *unk1;
    unsigned int unk2;
    void *unk3;
    Context *context;
    volatile LONG count;
    volatile LONG finished;
    void *unk4;
    void *event;
} _StructuredTaskCollection;

typedef struct _UnrealizedChore
{
    const vtable_ptr *vtable;
    void (__cdecl *proc)(struct _UnrealizedChore*);
    struct _StructuredTaskCollection *task_collection;
    void (__cdecl *proc_wrapper)(struct _UnrealizedChore*);
    void *unk[6];
} _UnrealizedChore;

typedef struct {
    long *cancelling;
} _Cancellation_beacon;

static char* (CDECL *p_setlocale)(int category, const char* locale);
static struct MSVCRT_lconv* (CDECL *p_localeconv)(void);
static size_t (CDECL *p_wcstombs_s)(size_t *ret, char* dest, size_t sz, const wchar_t* src, size_t max);
static int (CDECL *p__dsign)(double);
static int (CDECL *p__fdsign)(float);
static int (__cdecl *p__dpcomp)(double x, double y);
static wchar_t** (CDECL *p____lc_locale_name_func)(void);
static unsigned int (CDECL *p__GetConcurrency)(void);
static void* (CDECL *p__W_Gettnames)(void);
static void* (CDECL *p__Gettnames)(void);
static void (CDECL *p_free)(void*);
static float (CDECL *p_strtof)(const char *, char **);
static int (CDECL *p__finite)(double);
static float (CDECL *p_wcstof)(const wchar_t*, wchar_t**);
static double (CDECL *p_remainder)(double, double);
static int* (CDECL *p_errno)(void);
static int (CDECL *p_fegetenv)(fenv_t*);
static int (CDECL *p_fesetenv)(const fenv_t*);
static int (CDECL *p_fegetround)(void);
static int (CDECL *p_fesetround)(int);
static int (CDECL *p_fegetexceptflag)(fexcept_t*,int);
static int (CDECL *p_fesetexceptflag)(const fexcept_t*,int);
static int (CDECL *p_fetestexcept)(int);
static int (CDECL *p_feclearexcept)(int);
static int (CDECL *p_feupdateenv)(fenv_t*);
static int (CDECL *p__clearfp)(void);
static _locale_t (__cdecl *p_wcreate_locale)(int, const wchar_t *);
static void (__cdecl *p_free_locale)(_locale_t);
static unsigned short (__cdecl *p_wctype)(const char*);
static int (__cdecl *p_vsscanf)(const char*, const char *, va_list valist);
static _Dcomplex* (__cdecl *p__Cbuild)(_Dcomplex*, double, double);
static double (__cdecl *p_creal)(_Dcomplex);
static double (__cdecl *p_nexttoward)(double, double);
static float (__cdecl *p_nexttowardf)(float, double);
static double (__cdecl *p_nexttowardl)(double, double);
static wctrans_t (__cdecl *p_wctrans)(const char*);
static wint_t (__cdecl *p_towctrans)(wint_t, wctrans_t);
static int (__cdecl *p_strcmp)(const char *, const char *);
static int (__cdecl *p_strncmp)(const char *, const char *, size_t);
static struct tm* (__cdecl *p_gmtime32)(__time32_t*);
static errno_t    (__cdecl *p_gmtime32_s)(struct tm*, __time32_t*);
static struct tm* (__cdecl *p_gmtime64)(__time64_t*);
static errno_t    (__cdecl *p_gmtime64_s)(struct tm*, __time64_t*);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

static critical_section* (__thiscall *p_critical_section_ctor)(critical_section*);
static void (__thiscall *p_critical_section_dtor)(critical_section*);
static void (__thiscall *p_critical_section_lock)(critical_section*);
static void (__thiscall *p_critical_section_unlock)(critical_section*);
static critical_section* (__thiscall *p_critical_section_native_handle)(critical_section*);
static MSVCRT_bool (__thiscall *p_critical_section_try_lock)(critical_section*);
static MSVCRT_bool (__thiscall *p_critical_section_try_lock_for)(critical_section*, unsigned int);
static critical_section_scoped_lock* (__thiscall *p_critical_section_scoped_lock_ctor)
    (critical_section_scoped_lock*, critical_section *);
static void (__thiscall *p_critical_section_scoped_lock_dtor)(critical_section_scoped_lock*);

static _Condition_variable* (__thiscall *p__Condition_variable_ctor)(_Condition_variable*);
static void (__thiscall *p__Condition_variable_dtor)(_Condition_variable*);
static void (__thiscall *p__Condition_variable_wait)(_Condition_variable*, critical_section*);
static MSVCRT_bool (__thiscall *p__Condition_variable_wait_for)(_Condition_variable*, critical_section*, unsigned int);
static void (__thiscall *p__Condition_variable_notify_one)(_Condition_variable*);
static void (__thiscall *p__Condition_variable_notify_all)(_Condition_variable*);

static Context* (__cdecl *p_Context_CurrentContext)(void);
static _Context* (__cdecl *p__Context__CurrentContext)(_Context*);
static MSVCRT_bool (__cdecl *p_Context_IsCurrentTaskCollectionCanceling)(void);

static _StructuredTaskCollection* (__thiscall *p__StructuredTaskCollection_ctor)(_StructuredTaskCollection*, void*);
static void (__thiscall *p__StructuredTaskCollection_dtor)(_StructuredTaskCollection*);
static void (__thiscall *p__StructuredTaskCollection__Schedule)(_StructuredTaskCollection*, _UnrealizedChore*);
static int (__stdcall *p__StructuredTaskCollection__RunAndWait)(_StructuredTaskCollection*, _UnrealizedChore*);
static void (__thiscall *p__StructuredTaskCollection__Cancel)(_StructuredTaskCollection*);
static MSVCRT_bool (__thiscall *p__StructuredTaskCollection__IsCanceling)(_StructuredTaskCollection*);

static _Cancellation_beacon* (__thiscall *p__Cancellation_beacon_ctor)(_Cancellation_beacon*);
static void (__thiscall *p__Cancellation_beacon_dtor)(_Cancellation_beacon*);
static MSVCRT_bool (__thiscall *p__Cancellation_beacon__Confirm_cancel)(_Cancellation_beacon*);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(module,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("msvcr120.dll");
    if (!module)
    {
        win_skip("msvcr120.dll not installed\n");
        return FALSE;
    }

    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    p_localeconv = (void*)GetProcAddress(module, "localeconv");
    p_wcstombs_s = (void*)GetProcAddress(module, "wcstombs_s");
    p__dsign = (void*)GetProcAddress(module, "_dsign");
    p__fdsign = (void*)GetProcAddress(module, "_fdsign");
    p__dpcomp = (void*)GetProcAddress(module, "_dpcomp");
    p____lc_locale_name_func = (void*)GetProcAddress(module, "___lc_locale_name_func");
    p__GetConcurrency = (void*)GetProcAddress(module,"?_GetConcurrency@details@Concurrency@@YAIXZ");
    p__W_Gettnames = (void*)GetProcAddress(module, "_W_Gettnames");
    p__Gettnames = (void*)GetProcAddress(module, "_Gettnames");
    p_free = (void*)GetProcAddress(module, "free");
    p_strtof = (void*)GetProcAddress(module, "strtof");
    p__finite = (void*)GetProcAddress(module, "_finite");
    p_wcstof = (void*)GetProcAddress(module, "wcstof");
    p_remainder = (void*)GetProcAddress(module, "remainder");
    p_errno = (void*)GetProcAddress(module, "_errno");
    p_wcreate_locale = (void*)GetProcAddress(module, "_wcreate_locale");
    p_free_locale = (void*)GetProcAddress(module, "_free_locale");
    p_gmtime64 = (void*)GetProcAddress(module, "_gmtime64");
    p_gmtime64_s = (void*)GetProcAddress(module, "_gmtime64_s");
    p_gmtime32 = (void*)GetProcAddress(module, "_gmtime32");
    p_gmtime32_s = (void*)GetProcAddress(module, "_gmtime32_s");
    SET(p_wctype, "wctype");
    SET(p_fegetenv, "fegetenv");
    SET(p_fesetenv, "fesetenv");
    SET(p_fegetround, "fegetround");
    SET(p_fesetround, "fesetround");
    SET(p_fegetexceptflag, "fegetexceptflag");
    SET(p_fesetexceptflag, "fesetexceptflag");
    SET(p_fetestexcept, "fetestexcept");
    SET(p_feclearexcept, "feclearexcept");
    SET(p_feupdateenv, "feupdateenv");

    SET(p__clearfp, "_clearfp");
    SET(p_vsscanf, "vsscanf");
    SET(p__Cbuild, "_Cbuild");
    SET(p_creal, "creal");
    SET(p_nexttoward, "nexttoward");
    SET(p_nexttowardf, "nexttowardf");
    SET(p_nexttowardl, "nexttowardl");
    SET(p_wctrans, "wctrans");
    SET(p_towctrans, "towctrans");
    SET(p__Context__CurrentContext, "?_CurrentContext@_Context@details@Concurrency@@SA?AV123@XZ");
    SET(p_strcmp, "strcmp");
    SET(p_strncmp, "strncmp");
    SET(p_Context_IsCurrentTaskCollectionCanceling, "?IsCurrentTaskCollectionCanceling@Context@Concurrency@@SA_NXZ");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p__StructuredTaskCollection_ctor,
                "??0_StructuredTaskCollection@details@Concurrency@@QEAA@PEAV_CancellationTokenState@12@@Z");
        SET(p__StructuredTaskCollection_dtor,
                "??1_StructuredTaskCollection@details@Concurrency@@QEAA@XZ");
        SET(p__StructuredTaskCollection__Schedule,
                "?_Schedule@_StructuredTaskCollection@details@Concurrency@@QEAAXPEAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__RunAndWait,
                "?_RunAndWait@_StructuredTaskCollection@details@Concurrency@@QEAA?AW4_TaskCollectionStatus@23@PEAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__Cancel,
                "?_Cancel@_StructuredTaskCollection@details@Concurrency@@QEAAXXZ");
        SET(p__StructuredTaskCollection__IsCanceling,
                "?_IsCanceling@_StructuredTaskCollection@details@Concurrency@@QEAA_NXZ");
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QEAA@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QEAA@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QEAAXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QEAAXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QEAAAEAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QEAA_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QEAA_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QEAA@AEAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QEAA@XZ");
        SET(p__Condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QEAA@XZ");
        SET(p__Condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QEAA@XZ");
        SET(p__Condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QEAAXAEAVcritical_section@3@@Z");
        SET(p__Condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QEAA_NAEAVcritical_section@3@I@Z");
        SET(p__Condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QEAAXXZ");
        SET(p__Condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QEAAXXZ");
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPEAV12@XZ");
        SET(p__Cancellation_beacon_ctor,
                "??0_Cancellation_beacon@details@Concurrency@@QEAA@XZ");
        SET(p__Cancellation_beacon_dtor,
                "??1_Cancellation_beacon@details@Concurrency@@QEAA@XZ");
        SET(p__Cancellation_beacon__Confirm_cancel,
                "?_Confirm_cancel@_Cancellation_beacon@details@Concurrency@@QEAA_NXZ");
    } else {
#ifdef __arm__
        SET(p__StructuredTaskCollection_ctor,
                "??0_StructuredTaskCollection@details@Concurrency@@QAA@PAV_CancellationTokenState@12@@Z");
        SET(p__StructuredTaskCollection_dtor,
                "??1_StructuredTaskCollection@details@Concurrency@@QAA@XZ");
        SET(p__StructuredTaskCollection__Schedule,
                "?_Schedule@_StructuredTaskCollection@details@Concurrency@@QAAXPAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__RunAndWait,
                "?_RunAndWait@_StructuredTaskCollection@details@Concurrency@@QAA?AW4_TaskCollectionStatus@23@PAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__Cancel,
                "?_Cancel@_StructuredTaskCollection@details@Concurrency@@QAAXXZ");
        SET(p__StructuredTaskCollection__IsCanceling,
                "?_IsCanceling@_StructuredTaskCollection@details@Concurrency@@QAA_NXZ");
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QAA@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QAA@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QAAXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QAAXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QAAAAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QAA_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QAA_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QAA@AAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QAA@XZ");
        SET(p__Condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QAA@XZ");
        SET(p__Condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QAA@XZ");
        SET(p__Condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QAAXAAVcritical_section@3@@Z");
        SET(p__Condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QAA_NAAVcritical_section@3@I@Z");
        SET(p__Condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QAAXXZ");
        SET(p__Condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QAAXXZ");
        SET(p__Cancellation_beacon_ctor,
                "??0_Cancellation_beacon@details@Concurrency@@QAA@XZ");
        SET(p__Cancellation_beacon_dtor,
                "??1_Cancellation_beacon@details@Concurrency@@QAA@XZ");
        SET(p__Cancellation_beacon__Confirm_cancel,
                "?_Confirm_cancel@_Cancellation_beacon@details@Concurrency@@QAA_NXZ");
#else
        SET(p__StructuredTaskCollection_ctor,
                "??0_StructuredTaskCollection@details@Concurrency@@QAE@PAV_CancellationTokenState@12@@Z");
        SET(p__StructuredTaskCollection_dtor,
                "??1_StructuredTaskCollection@details@Concurrency@@QAE@XZ");
        SET(p__StructuredTaskCollection__Schedule,
                "?_Schedule@_StructuredTaskCollection@details@Concurrency@@QAEXPAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__RunAndWait,
                "?_RunAndWait@_StructuredTaskCollection@details@Concurrency@@QAG?AW4_TaskCollectionStatus@23@PAV_UnrealizedChore@23@@Z");
        SET(p__StructuredTaskCollection__Cancel,
                "?_Cancel@_StructuredTaskCollection@details@Concurrency@@QAEXXZ");
        SET(p__StructuredTaskCollection__IsCanceling,
                "?_IsCanceling@_StructuredTaskCollection@details@Concurrency@@QAE_NXZ");
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QAE@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QAE@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QAEXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QAEXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QAEAAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QAE_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QAE_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QAE@AAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QAE@XZ");
        SET(p__Condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QAE@XZ");
        SET(p__Condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QAE@XZ");
        SET(p__Condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QAEXAAVcritical_section@3@@Z");
        SET(p__Condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QAE_NAAVcritical_section@3@I@Z");
        SET(p__Condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QAEXXZ");
        SET(p__Condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QAEXXZ");
        SET(p__Cancellation_beacon_ctor,
                "??0_Cancellation_beacon@details@Concurrency@@QAE@XZ");
        SET(p__Cancellation_beacon_dtor,
                "??1_Cancellation_beacon@details@Concurrency@@QAE@XZ");
        SET(p__Cancellation_beacon__Confirm_cancel,
                "?_Confirm_cancel@_Cancellation_beacon@details@Concurrency@@QAE_NXZ");
#endif
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPAV12@XZ");
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test_lconv_helper(const char *locstr)
{
    struct MSVCRT_lconv *lconv;
    char mbs[256];
    size_t i;

    if(!p_setlocale(LC_ALL, locstr))
    {
        win_skip("locale %s not available\n", locstr);
        return;
    }

    lconv = p_localeconv();

    /* If multi-byte version available, asserts that wide char version also available.
     * If wide char version can be converted to a multi-byte string , asserts that the
     * conversion result is the same as the multi-byte version.
     */
    if(strlen(lconv->decimal_point) > 0)
        ok(wcslen(lconv->_W_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->thousands_sep) > 0)
        ok(wcslen(lconv->_W_thousands_sep) > 0, "%s: thousands_sep\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_thousands_sep, 256) == 0)
        ok(strcmp(mbs, lconv->thousands_sep) == 0, "%s: thousands_sep\n", locstr);

    if(strlen(lconv->int_curr_symbol) > 0)
        ok(wcslen(lconv->_W_int_curr_symbol) > 0, "%s: int_curr_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_int_curr_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->int_curr_symbol) == 0, "%s: int_curr_symbol\n", locstr);

    if(strlen(lconv->currency_symbol) > 0)
        ok(wcslen(lconv->_W_currency_symbol) > 0, "%s: currency_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_currency_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->currency_symbol) == 0, "%s: currency_symbol\n", locstr);

    if(strlen(lconv->mon_decimal_point) > 0)
        ok(wcslen(lconv->_W_mon_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_mon_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->mon_decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->positive_sign) > 0)
        ok(wcslen(lconv->_W_positive_sign) > 0, "%s: positive_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_positive_sign, 256) == 0)
        ok(strcmp(mbs, lconv->positive_sign) == 0, "%s: positive_sign\n", locstr);

    if(strlen(lconv->negative_sign) > 0)
        ok(wcslen(lconv->_W_negative_sign) > 0, "%s: negative_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_negative_sign, 256) == 0)
        ok(strcmp(mbs, lconv->negative_sign) == 0, "%s: negative_sign\n", locstr);
}

static void test_lconv(void)
{
    int i;
    const char *locstrs[] =
    {
        "American", "Belgian", "Chinese",
        "Dutch", "English", "French",
        "German", "Hungarian", "Icelandic",
        "Japanese", "Korean", "Spanish"
    };

    for(i = 0; i < ARRAY_SIZE(locstrs); i ++)
        test_lconv_helper(locstrs[i]);
}

static void test__dsign(void)
{
    int ret;

    ret = p__dsign(1);
    ok(ret == 0, "p_dsign(1) = %x\n", ret);
    ret = p__dsign(0);
    ok(ret == 0, "p_dsign(0) = %x\n", ret);
    ret = p__dsign(-1);
    ok(ret == 0x8000, "p_dsign(-1) = %x\n", ret);

    ret = p__fdsign(1);
    ok(ret == 0, "p_fdsign(1) = %x\n", ret);
    ret = p__fdsign(0);
    ok(ret == 0, "p_fdsign(0) = %x\n", ret);
    ret = p__fdsign(-1);
    ok(ret == 0x8000, "p_fdsign(-1) = %x\n", ret);
}

static void test__dpcomp(void)
{
    struct {
        double x, y;
        int ret;
    } tests[] = {
        {0, 0, 2}, {1, 1, 2}, {-1, -1, 2},
        {-2, -1, 1}, {-1, 1, 1}, {1, 2, 1},
        {1, -1, 4}, {2, 1, 4}, {-1, -2, 4},
        {NAN, NAN, 0}, {NAN, 1, 0}, {1, NAN, 0},
        {INFINITY, INFINITY, 2}, {-1, INFINITY, 1}, {1, INFINITY, 1},
    };
    int i, ret;

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        ret = p__dpcomp(tests[i].x, tests[i].y);
        ok(ret == tests[i].ret, "%d) dpcomp(%f, %f) = %x\n", i, tests[i].x, tests[i].y, ret);
    }
}

static void test____lc_locale_name_func(void)
{
    struct {
        const char *locale;
        const WCHAR name[10];
        const WCHAR broken_name[10];
    } tests[] = {
        { "American",  {'e','n',0}, {'e','n','-','U','S',0} },
        { "Belgian",   {'n','l','-','B','E',0} },
        { "Chinese",   {'z','h',0}, {'z','h','-','C','N',0} },
        { "Dutch",     {'n','l',0}, {'n','l','-','N','L',0} },
        { "English",   {'e','n',0}, {'e','n','-','U','S',0} },
        { "French",    {'f','r',0}, {'f','r','-','F','R',0} },
        { "German",    {'d','e',0}, {'d','e','-','D','E',0} },
        { "Hungarian", {'h','u',0}, {'h','u','-','H','U',0} },
        { "Icelandic", {'i','s',0}, {'i','s','-','I','S',0} },
        { "Japanese",  {'j','a',0}, {'j','a','-','J','P',0} },
        { "Korean",    {'k','o',0}, {'k','o','-','K','R',0} }
    };
    int i, j;
    wchar_t **lc_names;

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        if(!p_setlocale(LC_ALL, tests[i].locale))
            continue;

        lc_names = p____lc_locale_name_func();
        ok(lc_names[0] == NULL, "%d - lc_names[0] = %s\n", i, wine_dbgstr_w(lc_names[0]));
        ok(!lstrcmpW(lc_names[1], tests[i].name) || broken(!lstrcmpW(lc_names[1], tests[i].broken_name)),
           "%d - lc_names[1] = %s\n", i, wine_dbgstr_w(lc_names[1]));

        for(j=LC_MIN+2; j<=LC_MAX; j++) {
            ok(!lstrcmpW(lc_names[1], lc_names[j]), "%d - lc_names[%d] = %s, expected %s\n",
                    i, j, wine_dbgstr_w(lc_names[j]), wine_dbgstr_w(lc_names[1]));
        }
    }

    p_setlocale(LC_ALL, "zh-Hans");
    lc_names = p____lc_locale_name_func();
    ok(!lstrcmpW(lc_names[1], L"zh-Hans"), "lc_names[1] expected zh-Hans got %s\n", wine_dbgstr_w(lc_names[1]));

    p_setlocale(LC_ALL, "zh-Hant");
    lc_names = p____lc_locale_name_func();
    ok(!lstrcmpW(lc_names[1], L"zh-Hant"), "lc_names[1] expected zh-Hant got %s\n", wine_dbgstr_w(lc_names[1]));

    p_setlocale(LC_ALL, "C");
    lc_names = p____lc_locale_name_func();
    ok(!lc_names[1], "___lc_locale_name_func()[1] = %s\n", wine_dbgstr_w(lc_names[1]));
}

static void test__GetConcurrency(void)
{
    SYSTEM_INFO si;
    unsigned int c;

    GetSystemInfo(&si);
    c = (*p__GetConcurrency)();
    ok(c == si.dwNumberOfProcessors, "expected %lu, got %u\n", si.dwNumberOfProcessors, c);
}

static void test_gettnames(void* (CDECL *p_gettnames)(void))
{
    static const char *str[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday",
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
        "January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December",
        "AM", "PM", "M/d/yyyy"
    };

    struct {
        char *str[43];
        int  unk[2];
        wchar_t *wstr[43];
        wchar_t *locname;
        char data[1];
    } *ret;
    int i, size;
    WCHAR buf[64];

    if(!p_setlocale(LC_ALL, "english"))
        return;

    ret = p_gettnames();
    size = ret->str[0]-(char*)ret;
    if(sizeof(void*) == 8)
        ok(size==0x2c0, "structure size: %x\n", size);
    else
        ok(size==0x164, "structure size: %x\n", size);

    for(i=0; i<ARRAY_SIZE(str); i++) {
        ok(!strcmp(ret->str[i], str[i]), "ret->str[%d] = %s, expected %s\n",
                i, ret->str[i], str[i]);

        MultiByteToWideChar(CP_ACP, 0, str[i], strlen(str[i])+1, buf, ARRAY_SIZE(buf));
        ok(!lstrcmpW(ret->wstr[i], buf), "ret->wstr[%d] = %s, expected %s\n",
                i, wine_dbgstr_w(ret->wstr[i]), wine_dbgstr_w(buf));
    }

    ok(ret->str[42] + strlen(ret->str[42]) + 1 == (char*)ret->wstr[0] ||
            ret->str[42] + strlen(ret->str[42]) + 2 == (char*)ret->wstr[0],
            "ret->str[42] = %p len = %Id, ret->wstr[0] = %p\n",
            ret->str[42], strlen(ret->str[42]), ret->wstr[0]);
    p_free(ret);

    p_setlocale(LC_ALL, "C");
}

static void test__strtof(void)
{
    const char float1[] = "12.0";
    const char float2[] = "3.402823466e+38";          /* FLT_MAX */
    const char float3[] = "-3.402823466e+38";
    const char float4[] = "1.7976931348623158e+308";  /* DBL_MAX */

    BOOL is_arabic;
    char *end;
    float f;

    f = p_strtof(float1, &end);
    ok(f == 12.0, "f = %lf\n", f);
    ok(end == float1+4, "incorrect end (%d)\n", (int)(end-float1));

    f = p_strtof(float2, &end);
    ok(f == FLT_MAX, "f = %lf\n", f);
    ok(end == float2+15, "incorrect end (%d)\n", (int)(end-float2));

    f = p_strtof(float3, &end);
    ok(f == -FLT_MAX, "f = %lf\n", f);
    ok(end == float3+16, "incorrect end (%d)\n", (int)(end-float3));

    f = p_strtof(float4, &end);
    ok(!p__finite(f), "f = %lf\n", f);
    ok(end == float4+23, "incorrect end (%d)\n", (int)(end-float4));

    f = p_strtof("inf", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("INF", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("1.#inf", NULL);
    ok(f == 1, "f = %lf\n", f);

    f = p_strtof("INFINITY", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("0x12", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_wcstof(L"12.0", NULL);
    ok(f == 12.0, "f = %lf\n", f);

    f = p_wcstof(L"\x0662\x0663", NULL); /* 23 in Arabic numerals */
    is_arabic = (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_ARABIC);
    todo_wine_if(is_arabic) ok(f == (is_arabic ? 23.0 : 0.0), "f = %lf\n", f);

    f = p_wcstof(L"\x0662\x34\x0663", NULL); /* Arabic + Roman numerals mix */
    todo_wine_if(is_arabic) ok(f == (is_arabic ? 243.0 : 0.0), "f = %lf\n", f);

    if(!p_setlocale(LC_ALL, "Arabic")) {
        win_skip("Arabic locale not available\n");
        return;
    }

    f = p_wcstof(L"12.0", NULL);
    ok(f == 12.0, "f = %lf\n", f);

    f = p_wcstof(L"\x0662\x0663", NULL); /* 23 in Arabic numerals */
    todo_wine_if(is_arabic) ok(f == (is_arabic ? 23.0 : 0.0), "f = %lf\n", f);

    p_setlocale(LC_ALL, "C");
}

static void test_remainder(void)
{
    struct {
        double  x, y, r;
        errno_t e;
    } tests[] = {
        { 3.0,      2.0,       -1.0,    -1   },
        { 1.0,      1.0,        0.0,    -1   },
        { INFINITY, 0.0,        NAN,    EDOM },
        { INFINITY, 42.0,       NAN,    EDOM },
        { NAN,      0.0,        NAN,    EDOM },
        { NAN,      42.0,       NAN,    EDOM },
        { 0.0,      INFINITY,   0.0,    -1   },
        { 42.0,     INFINITY,   42.0,   -1   },
        { 0.0,      NAN,        NAN,    EDOM },
        { 42.0,     NAN,        NAN,    EDOM },
        { 1.0,      0.0,        NAN,    EDOM },
        { INFINITY, INFINITY,   NAN,    EDOM },
    };
    errno_t e;
    double r;
    int i;

    if(sizeof(void*) != 8) /* errno handling slightly different on 32-bit */
        return;

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = -1;
        r = p_remainder(tests[i].x, tests[i].y);
        e = errno;

        ok(tests[i].e == e, "expected errno %i, but got %i\n", tests[i].e, e);
        if(_isnan(tests[i].r))
            ok(_isnan(r), "expected NAN, but got %f\n", r);
        else
            ok(tests[i].r == r, "expected result %f, but got %f\n", tests[i].r, r);
    }
}

static int enter_flag;
static critical_section cs;
static unsigned __stdcall test_critical_section_lock(void *arg)
{
    critical_section *native_handle;
    native_handle = (critical_section*)call_func1(p_critical_section_native_handle, &cs);
    ok(native_handle == &cs, "native_handle = %p\n", native_handle);
    call_func1(p_critical_section_lock, &cs);
    ok(enter_flag == 1, "enter_flag = %d\n", enter_flag);
    call_func1(p_critical_section_unlock, &cs);
    return 0;
}

static unsigned __stdcall test_critical_section_try_lock(void *arg)
{
    ok(!(MSVCRT_bool)call_func1(p_critical_section_try_lock, &cs),
            "critical_section_try_lock succeeded\n");
    return 0;
}

static unsigned __stdcall test_critical_section_try_lock_for(void *arg)
{
    ok((MSVCRT_bool)call_func2(p_critical_section_try_lock_for, &cs, 5000),
            "critical_section_try_lock_for failed\n");
    ok(enter_flag == 1, "enter_flag = %d\n", enter_flag);
    call_func1(p_critical_section_unlock, &cs);
    return 0;
}

static unsigned __stdcall test_critical_section_scoped_lock(void *arg)
{
    critical_section_scoped_lock counter_scope_lock;

    call_func2(p_critical_section_scoped_lock_ctor, &counter_scope_lock, &cs);
    ok(enter_flag == 1, "enter_flag = %d\n", enter_flag);
    call_func1(p_critical_section_scoped_lock_dtor, &counter_scope_lock);
    return 0;
}

static void test_critical_section(void)
{
    HANDLE thread;
    DWORD ret;

    enter_flag = 0;
    call_func1(p_critical_section_ctor, &cs);
    call_func1(p_critical_section_lock, &cs);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned  %ld\n", ret);
    enter_flag = 1;
    call_func1(p_critical_section_unlock, &cs);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "CloseHandle failed\n");

    ok((MSVCRT_bool)call_func1(p_critical_section_try_lock, &cs),
            "critical_section_try_lock failed\n");
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "CloseHandle failed\n");
    call_func1(p_critical_section_unlock, &cs);

    enter_flag = 0;
    ok((MSVCRT_bool)call_func2(p_critical_section_try_lock_for, &cs, 50),
            "critical_section_try_lock_for failed\n");
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "CloseHandle failed\n");
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock_for, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    enter_flag = 1;
    Sleep(10);
    call_func1(p_critical_section_unlock, &cs);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "CloseHandle failed\n");

    enter_flag = 0;
    call_func1(p_critical_section_lock, &cs);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_scoped_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned  %ld\n", ret);
    enter_flag = 1;
    call_func1(p_critical_section_unlock, &cs);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "CloseHandle failed\n");
    call_func1(p_critical_section_dtor, &cs);
}

static void test_feenv(void)
{
    static const int tests[] = {
        0,
        FE_INEXACT,
        FE_UNDERFLOW,
        FE_OVERFLOW,
        FE_DIVBYZERO,
        FE_INVALID,
        FE_ALL_EXCEPT,
    };
    static const struct {
        fexcept_t except;
        unsigned int flag;
        unsigned int get;
        fexcept_t expect;
    } tests2[] = {
        /* except                   flag                     get             expect */
        { 0,                        0,                       0,              0 },
        { FE_ALL_EXCEPT,            FE_INEXACT,              0,              0 },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_ALL_EXCEPT,  FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_INEXACT,     FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_OVERFLOW,    0 },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           FE_ALL_EXCEPT,  FE_ALL_EXCEPT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           FE_INEXACT,     FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           0,              0 },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           ~0,             FE_ALL_EXCEPT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           ~FE_ALL_EXCEPT, 0 },
        { FE_INEXACT,               FE_ALL_EXCEPT,           FE_ALL_EXCEPT,  FE_INEXACT },
        { FE_INEXACT,               FE_UNDERFLOW,            FE_ALL_EXCEPT,  0 },
        { FE_UNDERFLOW,             FE_INEXACT,              FE_ALL_EXCEPT,  0 },
        { FE_INEXACT|FE_UNDERFLOW,  FE_UNDERFLOW,            FE_ALL_EXCEPT,  FE_UNDERFLOW },
        { FE_UNDERFLOW,             FE_INEXACT|FE_UNDERFLOW, FE_ALL_EXCEPT,  FE_UNDERFLOW },
    };
    fenv_t env, env2;
    fexcept_t except;
    int i, ret, flags;

    p__clearfp();

    ret = p_fegetenv(&env);
    ok(!ret, "fegetenv returned %x\n", ret);
    p_fesetround(FE_UPWARD);
    ok(env._Fe_ctl == (_EM_INEXACT|_EM_UNDERFLOW|_EM_OVERFLOW|_EM_ZERODIVIDE|_EM_INVALID),
            "env._Fe_ctl = %lx\n", env._Fe_ctl);
    ok(!env._Fe_stat, "env._Fe_stat = %lx\n", env._Fe_stat);
    ret = p_fegetenv(&env2);
    ok(!ret, "fegetenv returned %x\n", ret);
    ok(env2._Fe_ctl == (_EM_INEXACT|_EM_UNDERFLOW|_EM_OVERFLOW|_EM_ZERODIVIDE|_EM_INVALID | FE_UPWARD),
            "env2._Fe_ctl = %lx\n", env2._Fe_ctl);
    ret = p_fesetenv(&env);
    ok(!ret, "fesetenv returned %x\n", ret);
    ret = p_fegetround();
    ok(ret == FE_TONEAREST, "Got unexpected round mode %#x.\n", ret);

    if(0) { /* crash on windows */
        p_fesetexceptflag(NULL, FE_ALL_EXCEPT);
        p_fegetexceptflag(NULL, 0);
    }
    except = FE_ALL_EXCEPT;
    ret = p_fesetexceptflag(&except, FE_INEXACT|FE_UNDERFLOW);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    ret = p_feclearexcept(~FE_ALL_EXCEPT);
    ok(!ret, "feclearexceptflag returned %x\n", ret);
    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    /* no crash, but no-op */
    ret = p_fesetexceptflag(NULL, 0);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    /* zero clears all */
    except = 0;
    ret = p_fesetexceptflag(&except, FE_ALL_EXCEPT);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);

    ret = p_fetestexcept(FE_ALL_EXCEPT);
    ok(!ret, "fetestexcept returned %x\n", ret);

    flags = 0;
    /* adding bits with flags */
    for(i=0; i<ARRAY_SIZE(tests); i++) {
        except = FE_ALL_EXCEPT;
        ret = p_fesetexceptflag(&except, tests[i]);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = p_fetestexcept(tests[i]);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);

        flags |= tests[i];
        ret = p_fetestexcept(FE_ALL_EXCEPT);
        ok(ret == flags, "Test %d: expected %x, got %x\n", i, flags, ret);

        except = ~0;
        ret = p_fegetexceptflag(&except, ~0);
        ok(!ret, "Test %d: fegetexceptflag returned %x.\n", i, ret);
        ok(except == flags, "Test %d: expected %x, got %lx\n", i, flags, except);

        except = ~0;
        ret = p_fegetexceptflag(&except, tests[i]);
        ok(!ret, "Test %d: fegetexceptflag returned %x.\n", i, ret);
        ok(except == tests[i], "Test %d: expected %x, got %lx\n", i, tests[i], except);
    }

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        ret = p_feclearexcept(tests[i]);
        ok(!ret, "Test %d: feclearexceptflag returned %x\n", i, ret);

        flags &= ~tests[i];
        except = p_fetestexcept(tests[i]);
        ok(!except, "Test %d: expected %x, got %lx\n", i, flags, except);
    }

    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);

    /* setting bits with except */
    for(i=0; i<ARRAY_SIZE(tests); i++) {
        except = tests[i];
        ret = p_fesetexceptflag(&except, FE_ALL_EXCEPT);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = p_fetestexcept(tests[i]);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);

        ret = p_fetestexcept(FE_ALL_EXCEPT);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);
    }

    for(i=0; i<ARRAY_SIZE(tests2); i++) {
        p__clearfp();

        except = tests2[i].except;
        ret = p_fesetexceptflag(&except, tests2[i].flag);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = p_fetestexcept(tests2[i].get);
        ok(ret == tests2[i].expect, "Test %d: expected %lx, got %x\n", i, tests2[i].expect, ret);
    }

    ret = p_feclearexcept(FE_ALL_EXCEPT);
    ok(!ret, "feclearexceptflag returned %x\n", ret);
    except = p_fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);

    p__clearfp();
    except = FE_DIVBYZERO;
    ret = p_fesetexceptflag(&except, FE_DIVBYZERO);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    ret = p_fegetenv(&env);
    ok(!ret, "fegetenv returned %x\n", ret);
    p__clearfp();
    except = FE_INVALID;
    ret = p_fesetexceptflag(&except, FE_INVALID);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    ret = p_feupdateenv(&env);
    ok(!ret, "feupdateenv returned %x\n", ret);
    ret = _statusfp();
    ok(ret == (_EM_ZERODIVIDE | _EM_INVALID), "_statusfp returned %x\n", ret);
    p__clearfp();
}

static void test__wcreate_locale(void)
{
    _locale_t lcl;
    errno_t e;

    /* simple success */
    errno = -1;
    lcl = p_wcreate_locale(LC_ALL, L"C");
    e = errno;
    ok(!!lcl, "expected success, but got NULL\n");
    ok(errno == -1, "expected errno -1, but got %i\n", e);
    p_free_locale(lcl);

    errno = -1;
    lcl = p_wcreate_locale(LC_ALL, L"");
    e = errno;
    ok(!!lcl, "expected success, but got NULL\n");
    ok(errno == -1, "expected errno -1, but got %i\n", e);
    p_free_locale(lcl);

    /* bogus category */
    errno = -1;
    lcl = p_wcreate_locale(-1, L"C");
    e = errno;
    ok(!lcl, "expected failure, but got %p\n", lcl);
    ok(errno == -1, "expected errno -1, but got %i\n", e);

    /* bogus names */
    errno = -1;
    lcl = p_wcreate_locale(LC_ALL, L"bogus");
    e = errno;
    ok(!lcl, "expected failure, but got %p\n", lcl);
    ok(errno == -1, "expected errno -1, but got %i\n", e);

    errno = -1;
    lcl = p_wcreate_locale(LC_ALL, NULL);
    e = errno;
    ok(!lcl, "expected failure, but got %p\n", lcl);
    ok(errno == -1, "expected errno -1, but got %i\n", e);
}

struct wait_thread_arg
{
    critical_section *cs;
    _Condition_variable *cv;
    HANDLE thread_initialized;
};

static DWORD WINAPI condition_variable_wait_thread(void *varg)
{
    struct wait_thread_arg *arg = varg;

    call_func1(p_critical_section_lock, arg->cs);
    SetEvent(arg->thread_initialized);
    call_func2(p__Condition_variable_wait, arg->cv, arg->cs);
    call_func1(p_critical_section_unlock, arg->cs);
    return 0;
}

static void test__Condition_variable(void)
{
    critical_section cs;
    _Condition_variable cv;
    HANDLE thread_initialized = CreateEventW(NULL, FALSE, FALSE, NULL);
    struct wait_thread_arg wait_thread_arg = { &cs, &cv, thread_initialized };
    HANDLE threads[2];
    DWORD ret;
    MSVCRT_bool b;

    ok(thread_initialized != NULL, "CreateEvent failed\n");

    call_func1(p_critical_section_ctor, &cs);
    call_func1(p__Condition_variable_ctor, &cv);

    call_func1(p__Condition_variable_notify_one, &cv);
    call_func1(p__Condition_variable_notify_all, &cv);

    threads[0] = CreateThread(0, 0, condition_variable_wait_thread,
            &wait_thread_arg, 0, 0);
    ok(threads[0] != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread_initialized, INFINITE);
    call_func1(p_critical_section_lock, &cs);
    call_func1(p_critical_section_unlock, &cs);

    threads[1] = CreateThread(0, 0, condition_variable_wait_thread,
            &wait_thread_arg, 0, 0);
    ok(threads[1] != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread_initialized, INFINITE);
    call_func1(p_critical_section_lock, &cs);
    call_func1(p_critical_section_unlock, &cs);

    call_func1(p__Condition_variable_notify_one, &cv);
    ret = WaitForSingleObject(threads[1], 500);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    call_func1(p__Condition_variable_notify_one, &cv);
    ret = WaitForSingleObject(threads[0], 500);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);

    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    call_func1(p_critical_section_lock, &cs);
    b = call_func3(p__Condition_variable_wait_for, &cv, &cs, 1);
    ok(!b, "_Condition_variable_wait_for returned TRUE\n");
    call_func1(p_critical_section_unlock, &cs);

    call_func1(p_critical_section_dtor, &cs);
    call_func1(p__Condition_variable_dtor, &cv);

    CloseHandle(thread_initialized);
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

static int WINAPIV _vsscanf_wrapper(const char *buffer, const char *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = p_vsscanf(buffer, format, valist);
    va_end(valist);
    return ret;
}

static void test_vsscanf(void)
{
    static const char *fmt = "%d";
    char buff[16];
    int ret, v;

    v = 0;
    strcpy(buff, "10");
    ret = _vsscanf_wrapper(buff, fmt, &v);
    ok(ret == 1, "Unexpected ret %d.\n", ret);
    ok(v == 10, "got %d.\n", v);
}

static void test__Cbuild(void)
{
    _Dcomplex c;
    double d;

    memset(&c, 0, sizeof(c));
    p__Cbuild(&c, 1.0, 2.0);
    ok(c.r == 1.0, "c.r = %lf\n", c.r);
    ok(c.i == 2.0, "c.i = %lf\n", c.i);
    d = p_creal(c);
    ok(d == 1.0, "creal returned %lf\n", d);

    p__Cbuild(&c, 3.0, NAN);
    ok(c.r == 3.0, "c.r = %lf\n", c.r);
    ok(_isnan(c.i), "c.i = %lf\n", c.i);
    d = p_creal(c);
    ok(d == 3.0, "creal returned %lf\n", d);
}

static void test_nexttoward(void)
{
    errno_t e;
    double d;
    float f;
    int i;

    struct
    {
        double source;
        double dir;
        float f;
        double d;
    }
    tests[] =
    {
        {0.0,                      0.0,                      0.0f,        0.0},
        {0.0,                      1.0,                      1.0e-45f,    5.0e-324},
        {0.0,                     -1.0,                     -1.0e-45f,   -5.0e-324},
        {2.2250738585072009e-308,  0.0,                      0.0f,        2.2250738585072004e-308},
        {2.2250738585072009e-308,  2.2250738585072010e-308,  1.0e-45f,    2.2250738585072009e-308},
        {2.2250738585072009e-308,  1.0,                      1.0e-45f,    2.2250738585072014e-308},
        {2.2250738585072014e-308,  0.0,                      0.0f,        2.2250738585072009e-308},
        {2.2250738585072014e-308,  2.2250738585072014e-308,  1.0e-45f,    2.2250738585072014e-308},
        {2.2250738585072014e-308,  1.0,                      1.0e-45f,    2.2250738585072019e-308},
        {1.0,                      2.0,                      1.00000012f, 1.0000000000000002},
        {1.0,                      0.0,                      0.99999994f, 0.9999999999999999},
        {1.0,                      1.0,                      1.0f,        1.0},
        {0.0,                      INFINITY,                 1.0e-45f,    5.0e-324},
        {FLT_MAX,                  INFINITY,                 INFINITY,    3.402823466385289e+038},
        {DBL_MAX,                  INFINITY,                 INFINITY,    INFINITY},
        {INFINITY,                 INFINITY,                 INFINITY,    INFINITY},
        {INFINITY,                 0,                        FLT_MAX,     DBL_MAX},
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        f = p_nexttowardf(tests[i].source, tests[i].dir);
        ok(f == tests[i].f, "Test %d: expected %0.8ef, got %0.8ef.\n", i, tests[i].f, f);

        errno = -1;
        d = p_nexttoward(tests[i].source, tests[i].dir);
        e = errno;
        ok(d == tests[i].d, "Test %d: expected %0.16e, got %0.16e.\n", i, tests[i].d, d);
        if (!isnormal(d) && !isinf(tests[i].source))
            ok(e == ERANGE, "Test %d: expected ERANGE, got %d.\n", i, e);
        else
            ok(e == -1, "Test %d: expected no error, got %d.\n", i, e);

        d = p_nexttowardl(tests[i].source, tests[i].dir);
        ok(d == tests[i].d, "Test %d: expected %0.16e, got %0.16e.\n", i, tests[i].d, d);
    }

    errno = -1;
    d = p_nexttoward(NAN, 0);
    e = errno;
    ok(_isnan(d), "Expected NAN, got %0.16e.\n", d);
    ok(e == -1, "Expected no error, got %d.\n", e);

    errno = -1;
    d = p_nexttoward(NAN, NAN);
    e = errno;
    ok(_isnan(d), "Expected NAN, got %0.16e.\n", d);
    ok(e == -1, "Expected no error, got %d.\n", e);

    errno = -1;
    d = p_nexttoward(0, NAN);
    e = errno;
    ok(_isnan(d), "Expected NAN, got %0.16e.\n", d);
    ok(e == -1, "Expected no error, got %d.\n", e);
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

static void test_CurrentContext(void)
{
    _Context _ctx, *ret;
    Context *ctx;

    ctx = p_Context_CurrentContext();
    ok(!!ctx, "got NULL\n");

    memset(&_ctx, 0xcc, sizeof(_ctx));
    ret = p__Context__CurrentContext(&_ctx);
    ok(_ctx.ctx == ctx, "expected %p, got %p\n", ctx, _ctx.ctx);
    ok(ret == &_ctx, "expected %p, got %p\n", &_ctx, ret);
}

static void _UnrealizedChore_ctor(_UnrealizedChore *chore, void (__cdecl *proc)(_UnrealizedChore*))
{
    memset(chore, 0, sizeof(*chore));
    chore->proc = proc;
}

struct chore
{
    _UnrealizedChore chore;
    BOOL executed;
    HANDLE start_event;
    HANDLE set_event;
    HANDLE wait_event;
    DWORD main_tid;
    BOOL check_canceling;
};

static void __cdecl chore_proc(_UnrealizedChore *_this)
{
    struct chore *chore = CONTAINING_RECORD(_this, struct chore, chore);
    _Cancellation_beacon beacon, beacon2;

    if (chore->start_event)
    {
        DWORD ret = WaitForSingleObject(chore->start_event, 5000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    }

    ok(!chore->executed, "Chore was already executed\n");

    if (chore->main_tid)
    {
        ok(GetCurrentThreadId() == chore->main_tid,
                "Main chore is not running on the main thread: 0x%lx != 0x%lx\n",
                GetCurrentThreadId(), chore->main_tid);
    }

    if (chore->check_canceling)
    {
        MSVCRT_bool canceling = call_func1(
                p__StructuredTaskCollection__IsCanceling,
                chore->chore.task_collection);
        ok(!canceling, "Task is already canceling\n");

        ok(!p_Context_IsCurrentTaskCollectionCanceling(),
                "IsCurrentTaskCollectionCanceling returned TRUE\n");

        call_func1(p__Cancellation_beacon_ctor, &beacon);
        ok(!*beacon.cancelling, "beacon signalled %lx\n", *beacon.cancelling);

        call_func1(p__Cancellation_beacon_ctor, &beacon2);
        ok(beacon.cancelling != beacon2.cancelling, "beacons point to the same data\n");
        ok(!*beacon.cancelling, "beacon signalled %lx\n", *beacon.cancelling);

        canceling = call_func1(p__Cancellation_beacon__Confirm_cancel, &beacon);
        ok(!canceling, "_Confirm_cancel returned TRUE\n");
        ok(*beacon.cancelling == -1, "beacon signalled %lx\n", *beacon.cancelling);
        *beacon.cancelling = 0;
    }

    if (!chore->wait_event)
        chore->executed = TRUE;

    if (chore->set_event)
    {
        BOOL b = SetEvent(chore->set_event);
        ok(b, "SetEvent failed\n");
    }

    if (chore->wait_event)
    {
        DWORD ret = WaitForSingleObject(chore->wait_event, 5000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
        chore->executed = TRUE;
    }

    if (chore->check_canceling)
    {
        MSVCRT_bool canceling = call_func1(
                p__StructuredTaskCollection__IsCanceling,
                chore->chore.task_collection);
        ok(canceling, "Task is not canceling\n");

        ok(p_Context_IsCurrentTaskCollectionCanceling(),
                "IsCurrentTaskCollectionCanceling returned FALSE\n");

        ok(*beacon.cancelling == 1, "beacon not signalled (%lx)\n", *beacon.cancelling);
        canceling = call_func1(p__Cancellation_beacon__Confirm_cancel, &beacon);
        ok(canceling, "_Confirm_cancel returned FALSE\n");
        ok(*beacon.cancelling == 1, "beacon not signalled (%lx)\n", *beacon.cancelling);
        call_func1(p__Cancellation_beacon_dtor, &beacon);
        ok(*beacon2.cancelling == 1, "beacon not signalled (%lx)\n", *beacon2.cancelling);
        call_func1(p__Cancellation_beacon_dtor, &beacon2);

        call_func1(p__Cancellation_beacon_ctor, &beacon);
        ok(*beacon.cancelling == 1, "beacon not signalled (%lx)\n", *beacon.cancelling);
        canceling = call_func1(p__Cancellation_beacon__Confirm_cancel, &beacon);
        ok(canceling, "_Confirm_cancel returned FALSE\n");
        ok(*beacon.cancelling == 1, "beacon not signalled (%lx)\n", *beacon.cancelling);
        call_func1(p__Cancellation_beacon_dtor, &beacon);
    }
}

static void chore_ctor(struct chore *chore)
{
    memset(chore, 0, sizeof(*chore));
    _UnrealizedChore_ctor(&chore->chore, chore_proc);
}

static void test_StructuredTaskCollection(void)
{
    HANDLE chore_start_evt, chore_evt1, chore_evt2;
    _StructuredTaskCollection task_coll;
    struct chore chore1, chore2;
    _Cancellation_beacon beacon;
    DWORD main_thread_id;
    Context *context;
    int status;
    DWORD ret;
    BOOL b;

    main_thread_id = GetCurrentThreadId();
    context = p_Context_CurrentContext();

    memset(&task_coll, 0x55, sizeof(task_coll));
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    todo_wine ok(task_coll.unk2 == 0x1fffffff,
            "_StructuredTaskCollection ctor set wrong unk2: 0x%x != 0x1fffffff\n", task_coll.unk2);
    ok(task_coll.unk3 == NULL,
            "_StructuredTaskCollection ctor set wrong unk3: %p != NULL\n", task_coll.unk3);
    ok(task_coll.context == NULL,
            "_StructuredTaskCollection ctor set wrong context: %p != NULL\n", task_coll.context);
    ok(task_coll.count == 0,
            "_StructuredTaskCollection ctor set wrong count: %ld != 0\n", task_coll.count);
    ok(task_coll.finished == LONG_MIN,
            "_StructuredTaskCollection ctor set wrong finished: %ld != %ld\n", task_coll.finished, LONG_MIN);
    ok(task_coll.unk4 == NULL,
            "_StructuredTaskCollection ctor set wrong unk4: %p != NULL\n", task_coll.unk4);

    chore_start_evt = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(chore_start_evt != NULL, "CreateEvent failed: 0x%lx\n", GetLastError());
    chore_evt1 = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(chore_evt1 != NULL, "CreateEvent failed: 0x%lx\n", GetLastError());
    chore_evt2 = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(chore_evt2 != NULL, "CreateEvent failed: 0x%lx\n", GetLastError());

    /* test running chores (only main) */
    chore_ctor(&chore1);
    chore1.main_tid = main_thread_id;

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, &chore1.chore);
    ok(status == 1, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(chore1.executed, "Scheduled chore was not executed\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* test running chores (main + scheduled) */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    ResetEvent(chore_start_evt);
    chore_ctor(&chore1);
    chore1.start_event = chore_start_evt;
    chore_ctor(&chore2);
    chore2.main_tid = main_thread_id;

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore1.chore);
    ok(chore1.chore.task_collection == &task_coll, "Wrong chore #1 task_collection: expected %p, actual %p\n",
            &task_coll, chore1.chore.task_collection);
    ok(chore1.chore.proc_wrapper != NULL, "Chore #1 proc_wrapper was not set\n");
    ok(task_coll.count == 1, "Wrong chore count: %ld != 1\n", task_coll.count);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);

    b = SetEvent(chore_start_evt);
    ok(b, "SetEvent failed\n");

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, &chore2.chore);
    ok(status == 1, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(chore1.executed, "Scheduled chore was not executed\n");
    ok(chore2.executed, "Main chore was not executed\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* test that scheduled chores may run in parallel */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    ResetEvent(chore_start_evt);
    chore_ctor(&chore1);
    chore1.start_event = chore_start_evt;
    chore1.set_event = chore_evt2;
    chore1.wait_event = chore_evt1;
    chore_ctor(&chore2);
    chore2.start_event = chore_start_evt;
    chore2.set_event = chore_evt1;
    chore2.wait_event = chore_evt2;

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore1.chore);
    ok(chore1.chore.task_collection == &task_coll, "Wrong chore #1 task_collection: expected %p, actual %p\n",
            &task_coll, chore1.chore.task_collection);
    ok(chore1.chore.proc_wrapper != NULL, "Chore #1 proc_wrapper was not set\n");
    ok(task_coll.count == 1, "Wrong chore count: %ld != 1\n", task_coll.count);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore2.chore);
    ok(chore2.chore.task_collection == &task_coll, "Wrong chore #2 task_collection: expected %p, actual %p\n",
            &task_coll, chore2.chore.task_collection);
    ok(chore2.chore.proc_wrapper != NULL, "Chore #2 proc_wrapper was not set\n");
    ok(task_coll.count == 2, "Wrong chore count: %ld != 2\n", task_coll.count);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);

    b = SetEvent(chore_start_evt);
    ok(b, "SetEvent failed\n");

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, NULL);
    ok(status == 1, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(chore1.executed, "Chore #1 was not executed\n");
    ok(chore2.executed, "Chore #2 was not executed\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* test that scheduled chores are executed even if _RunAndWait is not called */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    ResetEvent(chore_start_evt);
    chore_ctor(&chore1);
    chore1.start_event = chore_start_evt;
    chore1.set_event = chore_evt1;

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore1.chore);
    ok(chore1.chore.task_collection == &task_coll, "Wrong chore #1 task_collection: expected %p, actual %p\n",
            &task_coll, chore1.chore.task_collection);
    ok(chore1.chore.proc_wrapper != NULL, "Chore #1 proc_wrapper was not set\n");
    ok(task_coll.count == 1, "Wrong chore count: %ld != 1\n", task_coll.count);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);

    b = SetEvent(chore_start_evt);
    ok(b, "SetEvent failed\n");

    ret = WaitForSingleObject(chore_evt1, 5000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);
    ok(chore1.executed, "Chore was not executed\n");

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, NULL);
    ok(status == 1, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(chore1.chore.task_collection == NULL, "Chore's task collection was not reset after execution\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* test that finished chores can be scheduled again */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    ResetEvent(chore_start_evt);
    chore1.set_event = NULL;
    chore1.executed = FALSE;

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore1.chore);
    ok(chore1.chore.task_collection == &task_coll, "Wrong chore #1 task_collection: expected %p, actual %p\n",
            &task_coll, chore1.chore.task_collection);
    ok(chore1.chore.proc_wrapper != NULL, "Chore #1 proc_wrapper was not set\n");
    ok(task_coll.count == 1, "Wrong chore count: %ld != 1\n", task_coll.count);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);

    b = SetEvent(chore_start_evt);
    ok(b, "SetEvent failed\n");

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, NULL);
    ok(status == 1, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(chore1.executed, "Chore was not executed\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* test that running chores can be canceled */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    ResetEvent(chore_start_evt);
    chore_ctor(&chore1);
    chore1.check_canceling = TRUE;
    chore1.wait_event = chore_evt2;
    chore1.set_event = chore_evt1;

    call_func2(p__StructuredTaskCollection__Schedule, &task_coll, &chore1.chore);

    ret = WaitForSingleObject(chore_evt1, 5000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %ld\n", ret);

    ok(!p_Context_IsCurrentTaskCollectionCanceling(),
            "IsCurrentTaskCollectionCanceling returned TRUE\n");
    call_func1(p__Cancellation_beacon_ctor, &beacon);
    ok(!*beacon.cancelling, "beacon signalled\n");

    call_func1(p__StructuredTaskCollection__Cancel, &task_coll);
    ok(!p_Context_IsCurrentTaskCollectionCanceling(),
            "IsCurrentTaskCollectionCanceling returned TRUE\n");
    ok(!*beacon.cancelling, "beacon signalled\n");

    b = SetEvent(chore_evt2);
    ok(b, "SetEvent failed\n");
    ok(!*beacon.cancelling, "beacon signalled\n");

    status = p__StructuredTaskCollection__RunAndWait(&task_coll, NULL);
    ok(status == 2, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(!*beacon.cancelling, "beacon signalled\n");
    call_func1(p__Cancellation_beacon_dtor, &beacon);
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    /* cancel task collection without scheduled tasks */
    call_func2(p__StructuredTaskCollection_ctor, &task_coll, NULL);
    call_func1(p__StructuredTaskCollection__Cancel, &task_coll);
    ok(task_coll.context == context, "Unexpected context: %p != %p\n", task_coll.context, context);
    chore_ctor(&chore1);
    status = p__StructuredTaskCollection__RunAndWait(&task_coll, NULL);
    ok(status == 2, "_StructuredTaskCollection::_RunAndWait failed: %d\n", status);
    ok(!chore1.executed, "Canceled collection executed chore\n");
    call_func1(p__StructuredTaskCollection_dtor, &task_coll);

    CloseHandle(chore_start_evt);
    CloseHandle(chore_evt1);
    CloseHandle(chore_evt2);
}

static void test_strcmp(void)
{
    int ret = p_strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = p_strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

static void test_gmtime64(void)
{
    struct tm *ptm, tm;
    __time64_t t;
    int ret;

    t = -1;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 23 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43200;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = p_gmtime32((__time32_t *)&t);
    ok(!!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime32_s(&tm, (__time32_t *)&t);
    ok(!ret, "got %d.\n", ret);
    todo_wine_if(tm.tm_year == 69 && tm.tm_hour == 12)
    ok(tm.tm_year == 70 && tm.tm_hour == -12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43201;
    ptm = p_gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = p_gmtime32((__time32_t *)&t);
    ok(!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime32_s(&tm, (__time32_t *)&t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 46800;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 1101 && tm.tm_hour == 20 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 46801;
    ptm = p_gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

START_TEST(msvcr120)
{
    if (!init()) return;
    test_lconv();
    test__dsign();
    test__dpcomp();
    test____lc_locale_name_func();
    test__GetConcurrency();
    test_gettnames(p__W_Gettnames);
    test_gettnames(p__Gettnames);
    test__strtof();
    test_remainder();
    test_critical_section();
    test_feenv();
    test__wcreate_locale();
    test__Condition_variable();
    test_wctype();
    test_vsscanf();
    test__Cbuild();
    test_nexttoward();
    test_towctrans();
    test_CurrentContext();
    test_StructuredTaskCollection();
    test_strcmp();
    test_gmtime64();
}
