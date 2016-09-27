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

#include <locale.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "wine/test.h"
#include "winbase.h"

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

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
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()

static inline float __port_nan(void)
{
    static const unsigned __nan_bytes = 0x7fc00000;
    return *(const float *)&__nan_bytes;
}
#define NAN __port_nan()

typedef int MSVCRT_long;
typedef unsigned char MSVCP_bool;

/* xtime */
typedef struct {
    __time64_t sec;
    MSVCRT_long nsec;
} xtime;

typedef struct {
    unsigned page;
    int mb_max;
    int unk;
    BYTE isleadbyte[32];
} _Cvtvec;

struct space_info {
    ULONGLONG capacity;
    ULONGLONG free;
    ULONGLONG available;
};

enum file_type {
    status_unknown, file_not_found, regular_file, directory_file,
    symlink_file, block_file, character_file, fifo_file, socket_file,
    type_unknown
};

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
}

static char* (__cdecl *p_setlocale)(int, const char*);
static int (__cdecl *p__setmbcp)(int);
static int (__cdecl *p_isleadbyte)(int);

static MSVCRT_long (__cdecl *p__Xtime_diff_to_millis2)(const xtime*, const xtime*);
static int (__cdecl *p_xtime_get)(xtime*, int);
static _Cvtvec* (__cdecl *p__Getcvt)(_Cvtvec*);
static void (CDECL *p__Call_once)(int *once, void (CDECL *func)(void));
static void (CDECL *p__Call_onceEx)(int *once, void (CDECL *func)(void*), void *argv);
static void (CDECL *p__Do_call)(void *this);
static short (__cdecl *p__Dtest)(double *d);
static short (__cdecl *p__Dscale)(double *d, int exp);
static short (__cdecl *p__FExp)(float *x, float y, int exp);

/* filesystem */
static ULONGLONG(__cdecl *p_tr2_sys__File_size)(char const*);
static ULONGLONG(__cdecl *p_tr2_sys__File_size_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Equivalent)(char const*, char const*);
static int (__cdecl *p_tr2_sys__Equivalent_wchar)(WCHAR const*, WCHAR const*);
static char* (__cdecl *p_tr2_sys__Current_get)(char *);
static WCHAR* (__cdecl *p_tr2_sys__Current_get_wchar)(WCHAR *);
static MSVCP_bool (__cdecl *p_tr2_sys__Current_set)(char const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Current_set_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Make_dir)(char const*);
static int (__cdecl *p_tr2_sys__Make_dir_wchar)(WCHAR const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Remove_dir)(char const*);
static MSVCP_bool (__cdecl *p_tr2_sys__Remove_dir_wchar)(WCHAR const*);
static int (__cdecl *p_tr2_sys__Copy_file)(char const*, char const*, MSVCP_bool);
static int (__cdecl *p_tr2_sys__Copy_file_wchar)(WCHAR const*, WCHAR const*, MSVCP_bool);
static int (__cdecl *p_tr2_sys__Rename)(char const*, char const*);
static int (__cdecl *p_tr2_sys__Rename_wchar)(WCHAR const*, WCHAR const*);
static struct space_info* (__cdecl *p_tr2_sys__Statvfs)(struct space_info*, char const*);
static struct space_info* (__cdecl *p_tr2_sys__Statvfs_wchar)(struct space_info*, WCHAR const*);
static enum file_type (__cdecl *p_tr2_sys__Stat)(char const*, int *);
static enum file_type (__cdecl *p_tr2_sys__Stat_wchar)(WCHAR const*, int *);
static enum file_type (__cdecl *p_tr2_sys__Lstat)(char const*, int *);
static enum file_type (__cdecl *p_tr2_sys__Lstat_wchar)(WCHAR const*, int *);
static __int64 (__cdecl *p_tr2_sys__Last_write_time)(char const*);
static void (__cdecl *p_tr2_sys__Last_write_time_set)(char const*, __int64);
static void* (__cdecl *p_tr2_sys__Open_dir)(char*, char const*, int *, enum file_type*);
static char* (__cdecl *p_tr2_sys__Read_dir)(char*, void*, enum file_type*);
static void (__cdecl *p_tr2_sys__Close_dir)(void*);
static int (__cdecl *p_tr2_sys__Link)(char const*, char const*);
static int (__cdecl *p_tr2_sys__Symlink)(char const*, char const*);
static int (__cdecl *p_tr2_sys__Unlink)(char const*);

/* thrd */
typedef struct
{
    HANDLE hnd;
    DWORD  id;
} _Thrd_t;

#define TIMEDELTA 250  /* 250 ms uncertainty allowed */

typedef int (__cdecl *_Thrd_start_t)(void*);

static int (__cdecl *p__Thrd_equal)(_Thrd_t, _Thrd_t);
static int (__cdecl *p__Thrd_lt)(_Thrd_t, _Thrd_t);
static void (__cdecl *p__Thrd_sleep)(const xtime*);
static _Thrd_t (__cdecl *p__Thrd_current)(void);
static int (__cdecl *p__Thrd_create)(_Thrd_t*, _Thrd_start_t, void*);
static int (__cdecl *p__Thrd_join)(_Thrd_t, int*);
static int (__cdecl *p__Thrd_detach)(_Thrd_t);

#ifdef __i386__
static ULONGLONG (__cdecl *p_i386_Thrd_current)(void);
static _Thrd_t __cdecl i386_Thrd_current(void)
{
    union {
        _Thrd_t thr;
        ULONGLONG ull;
    } r;
    r.ull = p_i386_Thrd_current();
    return r.thr;
}
#endif

/* mtx */
typedef struct cs_queue
{
    struct cs_queue *next;
    BOOL free;
    int unknown;
} cs_queue;

typedef struct
{
    ULONG_PTR unk_thread_id;
    cs_queue unk_active;
    void *unknown[2];
    cs_queue *head;
    void *tail;
} critical_section;

typedef struct
{
    DWORD flags;
    critical_section cs;
    DWORD thread_id;
    DWORD count;
} *_Mtx_t;

static int (__cdecl *p__Mtx_init)(_Mtx_t*, int);
static void (__cdecl *p__Mtx_destroy)(_Mtx_t*);
static int (__cdecl *p__Mtx_lock)(_Mtx_t*);
static int (__cdecl *p__Mtx_unlock)(_Mtx_t*);

/* cnd */
typedef void *_Cnd_t;

static int (__cdecl *p__Cnd_init)(_Cnd_t*);
static void (__cdecl *p__Cnd_destroy)(_Cnd_t*);
static int (__cdecl *p__Cnd_wait)(_Cnd_t*, _Mtx_t*);
static int (__cdecl *p__Cnd_timedwait)(_Cnd_t*, _Mtx_t*, const xtime*);
static int (__cdecl *p__Cnd_broadcast)(_Cnd_t*);
static int (__cdecl *p__Cnd_signal)(_Cnd_t*);
static void (__cdecl *p__Cnd_register_at_thread_exit)(_Cnd_t*, _Mtx_t*, int*);
static void (__cdecl *p__Cnd_unregister_at_thread_exit)(_Mtx_t*);
static void (__cdecl *p__Cnd_do_broadcast_at_thread_exit)(void);

/* _Pad */
typedef void (*vtable_ptr)(void);

typedef struct
{
    const vtable_ptr *vtable;
    _Cnd_t cnd;
    _Mtx_t mtx;
    MSVCP_bool launched;
} _Pad;

static _Pad* (__thiscall *p__Pad_ctor)(_Pad*);
static _Pad* (__thiscall *p__Pad_copy_ctor)(_Pad*, const _Pad*);
static void (__thiscall *p__Pad_dtor)(_Pad*);
static _Pad* (__thiscall *p__Pad_op_assign)(_Pad*, const _Pad*);
static void (__thiscall *p__Pad__Launch)(_Pad*, _Thrd_t*);
static void (__thiscall *p__Pad__Release)(_Pad*);

static BOOLEAN (WINAPI *pCreateSymbolicLinkA)(LPCSTR,LPCSTR,DWORD);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    HANDLE hdll;

    msvcp = LoadLibraryA("msvcp120.dll");
    if(!msvcp)
    {
        win_skip("msvcp120.dll not installed\n");
        return FALSE;
    }

    SET(p__Xtime_diff_to_millis2,
            "_Xtime_diff_to_millis2");
    SET(p_xtime_get,
            "xtime_get");
    SET(p__Getcvt,
            "_Getcvt");
    SET(p__Call_once,
            "_Call_once");
    SET(p__Call_onceEx,
            "_Call_onceEx");
    SET(p__Do_call,
            "_Do_call");
    SET(p__Dtest,
            "_Dtest");
    SET(p__Dscale,
            "_Dscale");
    SET(p__FExp,
            "_FExp");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_tr2_sys__File_size,
                "?_File_size@sys@tr2@std@@YA_KPEBD@Z");
        SET(p_tr2_sys__File_size_wchar,
                "?_File_size@sys@tr2@std@@YA_KPEB_W@Z");
        SET(p_tr2_sys__Equivalent,
                "?_Equivalent@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Equivalent_wchar,
                "?_Equivalent@sys@tr2@std@@YAHPEB_W0@Z");
        SET(p_tr2_sys__Current_get,
                "?_Current_get@sys@tr2@std@@YAPEADAEAY0BAE@D@Z");
        SET(p_tr2_sys__Current_get_wchar,
                "?_Current_get@sys@tr2@std@@YAPEA_WAEAY0BAE@_W@Z");
        SET(p_tr2_sys__Current_set,
                "?_Current_set@sys@tr2@std@@YA_NPEBD@Z");
        SET(p_tr2_sys__Current_set_wchar,
                "?_Current_set@sys@tr2@std@@YA_NPEB_W@Z");
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPEBD@Z");
        SET(p_tr2_sys__Make_dir_wchar,
                "?_Make_dir@sys@tr2@std@@YAHPEB_W@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPEBD@Z");
        SET(p_tr2_sys__Remove_dir_wchar,
                "?_Remove_dir@sys@tr2@std@@YA_NPEB_W@Z");
        SET(p_tr2_sys__Copy_file,
                "?_Copy_file@sys@tr2@std@@YAHPEBD0_N@Z");
        SET(p_tr2_sys__Copy_file_wchar,
                "?_Copy_file@sys@tr2@std@@YAHPEB_W0_N@Z");
        SET(p_tr2_sys__Rename,
                "?_Rename@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Rename_wchar,
                "?_Rename@sys@tr2@std@@YAHPEB_W0@Z");
        SET(p_tr2_sys__Statvfs,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PEBD@Z");
        SET(p_tr2_sys__Statvfs_wchar,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PEB_W@Z");
        SET(p_tr2_sys__Stat,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z");
        SET(p_tr2_sys__Stat_wchar,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PEB_WAEAH@Z");
        SET(p_tr2_sys__Lstat,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z");
        SET(p_tr2_sys__Lstat_wchar,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PEB_WAEAH@Z");
        SET(p_tr2_sys__Last_write_time,
                "?_Last_write_time@sys@tr2@std@@YA_JPEBD@Z");
        SET(p_tr2_sys__Last_write_time_set,
                "?_Last_write_time@sys@tr2@std@@YAXPEBD_J@Z");
        SET(p_tr2_sys__Open_dir,
                "?_Open_dir@sys@tr2@std@@YAPEAXAEAY0BAE@DPEBDAEAHAEAW4file_type@123@@Z");
        SET(p_tr2_sys__Read_dir,
                "?_Read_dir@sys@tr2@std@@YAPEADAEAY0BAE@DPEAXAEAW4file_type@123@@Z");
        SET(p_tr2_sys__Close_dir,
                "?_Close_dir@sys@tr2@std@@YAXPEAX@Z");
        SET(p_tr2_sys__Link,
                "?_Link@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Symlink,
                "?_Symlink@sys@tr2@std@@YAHPEBD0@Z");
        SET(p_tr2_sys__Unlink,
                "?_Unlink@sys@tr2@std@@YAHPEBD@Z");
        SET(p__Thrd_current,
                "_Thrd_current");
        SET(p__Pad_ctor,
                "??0_Pad@std@@QEAA@XZ");
        SET(p__Pad_copy_ctor,
                "??0_Pad@std@@QEAA@AEBV01@@Z");
        SET(p__Pad_dtor,
                "??1_Pad@std@@QEAA@XZ");
        SET(p__Pad_op_assign,
                "??4_Pad@std@@QEAAAEAV01@AEBV01@@Z");
        SET(p__Pad__Launch,
                "?_Launch@_Pad@std@@QEAAXPEAU_Thrd_imp_t@@@Z");
        SET(p__Pad__Release,
                "?_Release@_Pad@std@@QEAAXXZ");
    } else {
        SET(p_tr2_sys__File_size,
                "?_File_size@sys@tr2@std@@YA_KPBD@Z");
        SET(p_tr2_sys__File_size_wchar,
                "?_File_size@sys@tr2@std@@YA_KPB_W@Z");
        SET(p_tr2_sys__Equivalent,
                "?_Equivalent@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Equivalent_wchar,
                "?_Equivalent@sys@tr2@std@@YAHPB_W0@Z");
        SET(p_tr2_sys__Current_get,
                "?_Current_get@sys@tr2@std@@YAPADAAY0BAE@D@Z");
        SET(p_tr2_sys__Current_get_wchar,
                "?_Current_get@sys@tr2@std@@YAPA_WAAY0BAE@_W@Z");
        SET(p_tr2_sys__Current_set,
                "?_Current_set@sys@tr2@std@@YA_NPBD@Z");
        SET(p_tr2_sys__Current_set_wchar,
                "?_Current_set@sys@tr2@std@@YA_NPB_W@Z");
        SET(p_tr2_sys__Make_dir,
                "?_Make_dir@sys@tr2@std@@YAHPBD@Z");
        SET(p_tr2_sys__Make_dir_wchar,
                "?_Make_dir@sys@tr2@std@@YAHPB_W@Z");
        SET(p_tr2_sys__Remove_dir,
                "?_Remove_dir@sys@tr2@std@@YA_NPBD@Z");
        SET(p_tr2_sys__Remove_dir_wchar,
                "?_Remove_dir@sys@tr2@std@@YA_NPB_W@Z");
        SET(p_tr2_sys__Copy_file,
                "?_Copy_file@sys@tr2@std@@YAHPBD0_N@Z");
        SET(p_tr2_sys__Copy_file_wchar,
                "?_Copy_file@sys@tr2@std@@YAHPB_W0_N@Z");
        SET(p_tr2_sys__Rename,
                "?_Rename@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Rename_wchar,
                "?_Rename@sys@tr2@std@@YAHPB_W0@Z");
        SET(p_tr2_sys__Statvfs,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PBD@Z");
        SET(p_tr2_sys__Statvfs_wchar,
                "?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PB_W@Z");
        SET(p_tr2_sys__Stat,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z");
        SET(p_tr2_sys__Stat_wchar,
                "?_Stat@sys@tr2@std@@YA?AW4file_type@123@PB_WAAH@Z");
        SET(p_tr2_sys__Lstat,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z");
        SET(p_tr2_sys__Lstat_wchar,
                "?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PB_WAAH@Z");
        SET(p_tr2_sys__Last_write_time,
                "?_Last_write_time@sys@tr2@std@@YA_JPBD@Z");
        SET(p_tr2_sys__Last_write_time_set,
                "?_Last_write_time@sys@tr2@std@@YAXPBD_J@Z");
        SET(p_tr2_sys__Open_dir,
                "?_Open_dir@sys@tr2@std@@YAPAXAAY0BAE@DPBDAAHAAW4file_type@123@@Z");
        SET(p_tr2_sys__Read_dir,
                "?_Read_dir@sys@tr2@std@@YAPADAAY0BAE@DPAXAAW4file_type@123@@Z");
        SET(p_tr2_sys__Close_dir,
                "?_Close_dir@sys@tr2@std@@YAXPAX@Z");
        SET(p_tr2_sys__Link,
                "?_Link@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Symlink,
                "?_Symlink@sys@tr2@std@@YAHPBD0@Z");
        SET(p_tr2_sys__Unlink,
                "?_Unlink@sys@tr2@std@@YAHPBD@Z");
#ifdef __i386__
        SET(p_i386_Thrd_current,
                "_Thrd_current");
        p__Thrd_current = i386_Thrd_current;
        SET(p__Pad_ctor,
                "??0_Pad@std@@QAE@XZ");
        SET(p__Pad_copy_ctor,
                "??0_Pad@std@@QAE@ABV01@@Z");
        SET(p__Pad_dtor,
                "??1_Pad@std@@QAE@XZ");
        SET(p__Pad_op_assign,
                "??4_Pad@std@@QAEAAV01@ABV01@@Z");
        SET(p__Pad__Launch,
                "?_Launch@_Pad@std@@QAEXPAU_Thrd_imp_t@@@Z");
        SET(p__Pad__Release,
                "?_Release@_Pad@std@@QAEXXZ");
#else
        SET(p__Thrd_current,
                "_Thrd_current");
        SET(p__Pad_ctor,
                "??0_Pad@std@@QAA@XZ");
        SET(p__Pad_copy_ctor,
                "??0_Pad@std@@QAA@ABV01@@Z");
        SET(p__Pad_dtor,
                "??1_Pad@std@@QAA@XZ");
        SET(p__Pad_op_assign,
                "??4_Pad@std@@QAAAAV01@ABV01@@Z");
        SET(p__Pad__Launch,
                "?_Launch@_Pad@std@@QAAXPAU_Thrd_imp_t@@@Z");
        SET(p__Pad__Release,
                "?_Release@_Pad@std@@QAAXXZ");
#endif
    }
    SET(p__Thrd_equal,
            "_Thrd_equal");
    SET(p__Thrd_lt,
            "_Thrd_lt");
    SET(p__Thrd_sleep,
            "_Thrd_sleep");
    SET(p__Thrd_create,
            "_Thrd_create");
    SET(p__Thrd_join,
            "_Thrd_join");
    SET(p__Thrd_detach,
            "_Thrd_detach");

    SET(p__Mtx_init,
            "_Mtx_init");
    SET(p__Mtx_destroy,
            "_Mtx_destroy");
    SET(p__Mtx_lock,
            "_Mtx_lock");
    SET(p__Mtx_unlock,
            "_Mtx_unlock");

    SET(p__Cnd_init,
            "_Cnd_init");
    SET(p__Cnd_destroy,
            "_Cnd_destroy");
    SET(p__Cnd_wait,
            "_Cnd_wait");
    SET(p__Cnd_timedwait,
            "_Cnd_timedwait");
    SET(p__Cnd_broadcast,
            "_Cnd_broadcast");
    SET(p__Cnd_signal,
            "_Cnd_signal");
    SET(p__Cnd_register_at_thread_exit,
            "_Cnd_register_at_thread_exit");
    SET(p__Cnd_unregister_at_thread_exit,
            "_Cnd_unregister_at_thread_exit");
    SET(p__Cnd_do_broadcast_at_thread_exit,
            "_Cnd_do_broadcast_at_thread_exit");

    hdll = GetModuleHandleA("msvcr120.dll");
    p_setlocale = (void*)GetProcAddress(hdll, "setlocale");
    p__setmbcp = (void*)GetProcAddress(hdll, "_setmbcp");
    p_isleadbyte = (void*)GetProcAddress(hdll, "isleadbyte");

    hdll = GetModuleHandleA("kernel32.dll");
    pCreateSymbolicLinkA = (void*)GetProcAddress(hdll, "CreateSymbolicLinkA");

    init_thiscall_thunk();
    return TRUE;
}

static void test__Xtime_diff_to_millis2(void)
{
    struct {
        __time64_t sec_before;
        MSVCRT_long nsec_before;
        __time64_t sec_after;
        MSVCRT_long nsec_after;
        MSVCRT_long expect;
    } tests[] = {
        {1, 0, 2, 0, 1000},
        {0, 1000000000, 0, 2000000000, 1000},
        {1, 100000000, 2, 100000000, 1000},
        {1, 100000000, 1, 200000000, 100},
        {0, 0, 0, 1000000000, 1000},
        {0, 0, 0, 1200000000, 1200},
        {0, 0, 0, 1230000000, 1230},
        {0, 0, 0, 1234000000, 1234},
        {0, 0, 0, 1234100000, 1235},
        {0, 0, 0, 1234900000, 1235},
        {0, 0, 0, 1234010000, 1235},
        {0, 0, 0, 1234090000, 1235},
        {0, 0, 0, 1234000001, 1235},
        {0, 0, 0, 1234000009, 1235},
        {0, 0, -1, 0, 0},
        {0, 0, 0, -10000000, 0},
        {0, 0, -1, -100000000, 0},
        {-1, 0, 0, 0, 1000},
        {0, -100000000, 0, 0, 100},
        {-1, -100000000, 0, 0, 1100},
        {0, 0, -1, 2000000000, 1000},
        {0, 0, -2, 2000000000, 0},
        {0, 0, -2, 2100000000, 100}
    };
    int i;
    MSVCRT_long ret;
    xtime t1, t2;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); ++ i)
    {
        t1.sec = tests[i].sec_before;
        t1.nsec = tests[i].nsec_before;
        t2.sec = tests[i].sec_after;
        t2.nsec = tests[i].nsec_after;
        ret = p__Xtime_diff_to_millis2(&t2, &t1);
        ok(ret == tests[i].expect,
                "_Xtime_diff_to_millis2(): test: %d expect: %d, got: %d\n",
                i, tests[i].expect, ret);
    }
}

static void test_xtime_get(void)
{
    static const MSVCRT_long tests[] = {1, 50, 100, 200, 500};
    MSVCRT_long diff;
    xtime before, after;
    int i;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i ++)
    {
        p_xtime_get(&before, 1);
        Sleep(tests[i]);
        p_xtime_get(&after, 1);

        diff = p__Xtime_diff_to_millis2(&after, &before);

        ok(diff >= tests[i],
                "xtime_get() not functioning correctly, test: %d, expect: %d, got: %d\n",
                i, tests[i], diff);
    }

    /* Test parameter and return value */
    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 0);
    ok(i == 0, "expect xtime_get() to return 0, got: %d\n", i);
    ok(before.sec == 0xdeadbeef && before.nsec == 0xdeadbeef,
            "xtime_get() shouldn't have modified the xtime struct with the given option\n");

    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 1);
    ok(i == 1, "expect xtime_get() to return 1, got: %d\n", i);
    ok(before.sec != 0xdeadbeef && before.nsec != 0xdeadbeef,
            "xtime_get() should have modified the xtime struct with the given option\n");
}

static void test__Getcvt(void)
{
    _Cvtvec cvtvec;
    int i;

    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 0, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 1, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    todo_wine ok(cvtvec.unk == 1, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    if(!p_setlocale(LC_ALL, ".936")) {
        win_skip("_Getcvt tests\n");
        return;
    }
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    p__setmbcp(936);
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++) {
        BYTE b = 0;
        int j;

        for(j=0; j<8; j++)
            b |= (p_isleadbyte(i*8+j) ? 1 : 0) << j;
        ok(cvtvec.isleadbyte[i] ==b, "cvtvec.isleadbyte[%d] = %x (%x)\n", i, cvtvec.isleadbyte[i], b);
    }
}

static int cnt;
static int once;

static void __cdecl call_once_func(void)
{
    ok(!once, "once != 0\n");
    cnt += 0x10000;
}

static void __cdecl call_once_ex_func(void *arg)
{
    int *i = arg;

    ok(!once, "once != 0\n");
    (*i)++;
}

static DWORD WINAPI call_once_thread(void *arg)
{
    p__Call_once(&once, call_once_func);
    return 0;
}

static DWORD WINAPI call_once_ex_thread(void *arg)
{
    p__Call_onceEx(&once, call_once_ex_func, &cnt);
    return 0;
}

static void test__Call_once(void)
{
    HANDLE h[4];
    int i;

    for(i=0; i<4; i++)
        h[i] = CreateThread(NULL, 0, call_once_thread, &once, 0, NULL);
    ok(WaitForMultipleObjects(4, h, TRUE, INFINITE) == WAIT_OBJECT_0,
            "error waiting for all threads to finish\n");
    ok(cnt == 0x10000, "cnt = %x\n", cnt);
    ok(once == 1, "once = %x\n", once);

    once = cnt = 0;
    for(i=0; i<4; i++)
        h[i] = CreateThread(NULL, 0, call_once_ex_thread, &once, 0, NULL);
    ok(WaitForMultipleObjects(4, h, TRUE, INFINITE) == WAIT_OBJECT_0,
            "error waiting for all threads to finish\n");
    ok(cnt == 1, "cnt = %x\n", cnt);
    ok(once == 1, "once = %x\n", once);
}

static void **vtbl_func0;
#ifdef __i386__
/* TODO: this should be a __thiscall function */
static void __stdcall thiscall_func(void)
{
    cnt = 1;
}
#else
static void __cdecl thiscall_func(void *this)
{
    ok(this == &vtbl_func0, "incorrect this value\n");
    cnt = 1;
}
#endif

static void test__Do_call(void)
{
    void *pfunc = thiscall_func;

    cnt = 0;
    vtbl_func0 = &pfunc;
    p__Do_call(&vtbl_func0);
    ok(cnt == 1, "func was not called\n");
}

static void test__Dtest(void)
{
    double d;
    short ret;

    d = 0;
    ret = p__Dtest(&d);
    ok(ret == FP_ZERO, "_Dtest(0) returned %x\n", ret);

    d = 1;
    ret = p__Dtest(&d);
    ok(ret == FP_NORMAL, "_Dtest(1) returned %x\n", ret);

    d = -1;
    ret = p__Dtest(&d);
    ok(ret == FP_NORMAL, "_Dtest(-1) returned %x\n", ret);

    d = INFINITY;
    ret = p__Dtest(&d);
    ok(ret == FP_INFINITE, "_Dtest(INF) returned %x\n", ret);

    d = NAN;
    ret = p__Dtest(&d);
    ok(ret == FP_NAN, "_Dtest(NAN) returned %x\n", ret);
}

static void test__Dscale(void)
{
    double d;
    short ret;

    d = 0;
    ret = p__Dscale(&d, 0);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 0;
    ret = p__Dscale(&d, 1);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 0;
    ret = p__Dscale(&d, -1);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 1;
    ret = p__Dscale(&d, 0);
    ok(d == 1, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 1;
    ret = p__Dscale(&d, 1);
    ok(d == 2, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 1;
    ret = p__Dscale(&d, -1);
    ok(d == 0.5, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 1;
    ret = p__Dscale(&d, -99999);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 1;
    ret = p__Dscale(&d, 999999);
    ok(d == INFINITY, "d = %f\n", d);
    ok(ret == FP_INFINITE, "ret = %x\n", ret);

    d = NAN;
    ret = p__Dscale(&d, 1);
    ok(ret == FP_NAN, "ret = %x\n", ret);
}

static void test__FExp(void)
{
    float d;
    short ret;

    d = 0;
    ret = p__FExp(&d, 0, 0);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 0;
    ret = p__FExp(&d, 1, 0);
    ok(d == 1.0, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 0;
    ret = p__FExp(&d, 1, 1);
    ok(d == 2.0, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 0;
    ret = p__FExp(&d, 1, 2);
    ok(d == 4.0, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 0;
    ret = p__FExp(&d, 10, 0);
    ok(d == 10.0, "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 1;
    ret = p__FExp(&d, 0, 0);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 1;
    ret = p__FExp(&d, 1, 0);
    ok(compare_float(d, 2.7182817, 4), "d = %f\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);

    d = 9e20;
    ret = p__FExp(&d, 0, 0);
    ok(d == 0, "d = %f\n", d);
    ok(ret == FP_ZERO, "ret = %x\n", ret);

    d = 90;
    ret = p__FExp(&d, 1, 0);
    ok(ret == FP_INFINITE, "ret = %x\n", ret);

    d = 90;
    ret = p__FExp(&d, 1, -50);
    ok(compare_float(d, 1.0839359e+024, 4), "d = %g\n", d);
    ok(ret == FP_NORMAL, "ret = %x\n", ret);
}

static void test_tr2_sys__File_size(void)
{
    ULONGLONG val;
    HANDLE file;
    LARGE_INTEGER file_size;
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f1");
    ok(val == 7, "file_size is %s\n", debugstr_longlong(val));
    val = p_tr2_sys__File_size_wchar(testW);
    ok(val == 7, "file_size is %s\n", debugstr_longlong(val));

    file = CreateFileA("tr2_test_dir/f2", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f2");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));

    val = p_tr2_sys__File_size("tr2_test_dir");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size("tr2_test_dir/not_exists_file");
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size(NULL);
    ok(val == 0, "file_size is %s\n", debugstr_longlong(val));
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir/f2"), "expect tr2_test_dir/f2 to exist\n");
    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
}

static void test_tr2_sys__Equivalent(void)
{
    int val, i;
    HANDLE file;
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};
    WCHAR testW2[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','2',0};
    struct {
        char const *path1;
        char const *path2;
        int equivalent;
    } tests[] = {
        { NULL, NULL, -1 },
        { NULL, "f1", -1 },
        { "f1", NULL, -1 },
        { "f1", "tr2_test_dir", -1 },
        { "tr2_test_dir", "f1", -1 },
        { "tr2_test_dir", "tr2_test_dir", -1 },
        { "tr2_test_dir/./f1", "tr2_test_dir/f2", 0 },
        { "tr2_test_dir/f1"  , "tr2_test_dir/f1", 1 },
        { "not_exists_file"  , "tr2_test_dir/f1", 0 },
        { "tr2_test_dir\\f1" , "tr2_test_dir/./f1", 1 },
        { "not_exists_file"  , "not_exists_file",  -1 },
        { "tr2_test_dir/f1"  , "not_exists_file",   0 },
        { "tr2_test_dir/../tr2_test_dir/f1", "tr2_test_dir/f1", 1 }
    };

    memset(current_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, current_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    file = CreateFileA("tr2_test_dir/f2", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        val = p_tr2_sys__Equivalent(tests[i].path1, tests[i].path2);
        ok(tests[i].equivalent == val, "tr2_sys__Equivalent(): test %d expect: %d, got %d\n", i+1, tests[i].equivalent, val);
        ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    }

    val = p_tr2_sys__Equivalent_wchar(testW, testW);
    ok(val == 1, "tr2_sys__Equivalent(): expect: 1, got %d\n", val);
    val = p_tr2_sys__Equivalent_wchar(testW, testW2);
    ok(val == 0, "tr2_sys__Equivalent(): expect: 0, got %d\n", val);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir/f2"), "expect tr2_test_dir/f2 to exist\n");
    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static void test_tr2_sys__Current_get(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH], origin_path[MAX_PATH];
    char *temp;
    WCHAR temp_path_wchar[MAX_PATH], current_path_wchar[MAX_PATH];
    WCHAR *temp_wchar;
    memset(origin_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, origin_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);

    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    GetTempPathW(MAX_PATH, temp_path_wchar);
    ok(SetCurrentDirectoryW(temp_path_wchar), "SetCurrentDirectoryW to temp_path_wchar failed\n");
    memset(current_path_wchar, 0, MAX_PATH);
    temp_wchar = p_tr2_sys__Current_get_wchar(current_path_wchar);
    ok(temp_wchar == current_path_wchar, "p_tr2_sys__Current_get_wchar returned different buffer\n");
    temp_wchar[wcslen(temp_wchar)] = '\\';
    ok(!wcscmp(temp_path_wchar, current_path_wchar), "test_tr2_sys__Current_get(): expect: %s, got %s\n", wine_dbgstr_w(temp_path_wchar), wine_dbgstr_w(current_path_wchar));

    ok(SetCurrentDirectoryA(origin_path), "SetCurrentDirectoryA to origin_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Current_set(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH], origin_path[MAX_PATH];
    char *temp;
    WCHAR testW[] = {'.','/',0};
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    memset(origin_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, origin_path);
    temp = p_tr2_sys__Current_get(origin_path);
    ok(temp == origin_path, "p_tr2_sys__Current_get returned different buffer\n");

    ok(p_tr2_sys__Current_set(temp_path), "p_tr2_sys__Current_set to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    ok(p_tr2_sys__Current_set_wchar(testW), "p_tr2_sys__Current_set_wchar to temp_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    temp[strlen(temp)] = '\\';
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("not_exisist_dir"), "p_tr2_sys__Current_set to not_exist_dir succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("??invalid_name>>"), "p_tr2_sys__Current_set to ??invalid_name>> succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(p_tr2_sys__Current_set(origin_path), "p_tr2_sys__Current_set to origin_path failed\n");
    memset(current_path, 0, MAX_PATH);
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Make_dir(void)
{
    int ret, i;
    WCHAR testW[] = {'w','d',0};
    struct {
        char const *path;
        int val;
    } tests[] = {
        { "tr2_test_dir", 1 },
        { "tr2_test_dir", 0 },
        { NULL, -1 },
        { "??invalid_name>>", -1 }
    };

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Make_dir(tests[i].path);
        ok(ret == tests[i].val, "tr2_sys__Make_dir(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Make_dir(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
    }
    ret = p_tr2_sys__Make_dir_wchar(testW);
    ok(ret == 1, "tr2_sys__Make_dir(): expect: 1, got %d\n", ret);

    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
    ok(p_tr2_sys__Remove_dir_wchar(testW), "expect wd to exist\n");
}

static void test_tr2_sys__Remove_dir(void)
{
    MSVCP_bool ret;
    int i;
    struct {
        char const *path;
        MSVCP_bool val;
    } tests[] = {
        { "tr2_test_dir", TRUE  },
        { "tr2_test_dir", FALSE },
        { NULL, FALSE },
        { "??invalid_name>>", FALSE }
    };

    ok(p_tr2_sys__Make_dir("tr2_test_dir"), "tr2_sys__Make_dir() failed\n");

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Remove_dir(tests[i].path);
        ok(ret == tests[i].val, "test_tr2_sys__Remove_dir(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Remove_dir(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
    }
}

static void test_tr2_sys__Copy_file(void)
{
    HANDLE file;
    int ret, i;
    LARGE_INTEGER file_size;
    WCHAR testW[] = {'f','1',0}, testW2[] = {'f','w',0};
    struct {
        char const *source;
        char const *dest;
        MSVCP_bool fail_if_exists;
        int last_error;
        int last_error2;
        MSVCP_bool is_todo;
    } tests[] = {
        { "f1", "f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS, FALSE },
        { "f1", "tr2_test_dir\\f1_copy", FALSE, ERROR_SUCCESS, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir", "f1", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED, FALSE },
        { "tr2_test_dir", "tr2_test_dir_copy", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED, FALSE },
        { NULL, "f1", TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER, TRUE },
        { "f1", NULL, TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER, TRUE },
        { "not_exist", "tr2_test_dir", TRUE, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND, FALSE },
        { "f1", "not_exist_dir\\f1_copy", TRUE, ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND, FALSE },
        { "f1", "tr2_test_dir", TRUE, ERROR_ACCESS_DENIED, ERROR_FILE_EXISTS, FALSE }
    };

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Copy_file(tests[i].source, tests[i].dest, tests[i].fail_if_exists);
        todo_wine_if(tests[i].is_todo)
            ok(ret == tests[i].last_error || ret == tests[i].last_error2,
                    "test_tr2_sys__Copy_file(): test %d expect: %d, got %d\n", i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Copy_file(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].source) == p_tr2_sys__File_size(tests[i].dest),
                    "test_tr2_sys__Copy_file(): test %d failed, two files' size are not equal\n", i+1);
    }
    ret = p_tr2_sys__Copy_file_wchar(testW, testW2, TRUE);
    ok(ret == ERROR_SUCCESS, "test_tr2_sys__Copy_file_wchar() expect ERROR_SUCCESS, got %d\n", ret);

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    ok(DeleteFileW(testW2), "expect fw to exist\n");
    ok(DeleteFileA("f1_copy"), "expect f1_copy to exist\n");
    ok(DeleteFileA("tr2_test_dir/f1_copy"), "expect tr2_test_dir/f1 to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
}

static void test_tr2_sys__Rename(void)
{
    int ret, i;
    HANDLE file, h1, h2;
    BY_HANDLE_FILE_INFORMATION info1, info2;
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    LARGE_INTEGER file_size;
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};
    WCHAR testW2[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','w',0};
    struct {
        char const *old_path;
        char const *new_path;
        int val;
    } tests[] = {
        { "tr2_test_dir\\f1", "tr2_test_dir\\f1_rename", ERROR_SUCCESS },
        { "tr2_test_dir\\f1", NULL, ERROR_INVALID_PARAMETER },
        { "tr2_test_dir\\f1", "tr2_test_dir\\f1_rename", ERROR_FILE_NOT_FOUND },
        { NULL, "tr2_test_dir\\NULL_rename", ERROR_INVALID_PARAMETER },
        { "tr2_test_dir\\f1_rename", "tr2_test_dir\\??invalid_name>>", ERROR_INVALID_NAME },
        { "tr2_test_dir\\not_exist_file", "tr2_test_dir\\not_exist_rename", ERROR_FILE_NOT_FOUND }
    };

    memset(current_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, current_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    ret = p_tr2_sys__Make_dir("tr2_test_dir");

    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("tr2_test_dir\\f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    ret = p_tr2_sys__Rename("tr2_test_dir\\f1", "tr2_test_dir\\f1");
    todo_wine ok(ERROR_SUCCESS == ret, "test_tr2_sys__Rename(): expect: ERROR_SUCCESS, got %d\n", ret);
    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        if(tests[i].val == ERROR_SUCCESS) {
            h1 = CreateFileA(tests[i].old_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h1 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h1, &info1), "GetFileInformationByHandle failed\n");
            CloseHandle(h1);
        }
        SetLastError(0xdeadbeef);
        ret = p_tr2_sys__Rename(tests[i].old_path, tests[i].new_path);
        ok(ret == tests[i].val, "test_tr2_sys__Rename(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Rename(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS) {
            h2 = CreateFileA(tests[i].new_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h2 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h2, &info2), "GetFileInformationByHandle failed\n");
            CloseHandle(h2);
            ok(info1.nFileIndexHigh == info2.nFileIndexHigh
                    && info1.nFileIndexLow == info2.nFileIndexLow,
                    "test_tr2_sys__Rename(): test %d expect two files equivalent\n", i+1);
        }
    }

    file = CreateFileA("tr2_test_dir\\f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    ret = p_tr2_sys__Rename("tr2_test_dir\\f1", "tr2_test_dir\\f1_rename");
    ok(ret == ERROR_ALREADY_EXISTS, "test_tr2_sys__Rename(): expect: ERROR_ALREADY_EXISTS, got %d\n", ret);
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1") == 7, "test_tr2_sys__Rename(): expect: 7, got %s\n", debugstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1")));
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1_rename") == 0, "test_tr2_sys__Rename(): expect: 0, got %s\n",debugstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1_rename")));
    ret = p_tr2_sys__Rename_wchar(testW, testW2);
    ok(ret == ERROR_SUCCESS, "tr2_sys__Rename_wchar(): expect: ERROR_SUCCESS, got %d\n",  ret);

    ok(DeleteFileW(testW2), "expect fw to exist\n");
    ok(DeleteFileA("tr2_test_dir\\f1_rename"), "expect f1_rename to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect %d got %d\n", 1, ret);
    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static void test_tr2_sys__Statvfs(void)
{
    struct space_info info;
    char current_path[MAX_PATH];
    WCHAR current_path_wchar[MAX_PATH];
    memset(current_path, 0, MAX_PATH);
    p_tr2_sys__Current_get(current_path);
    memset(current_path_wchar, 0, MAX_PATH);
    p_tr2_sys__Current_get_wchar(current_path_wchar);

    p_tr2_sys__Statvfs(&info, current_path);
    ok(info.capacity >= info.free, "test_tr2_sys__Statvfs(): info.capacity < info.free\n");
    ok(info.free >= info.available, "test_tr2_sys__Statvfs(): info.free < info.available\n");

    p_tr2_sys__Statvfs_wchar(&info, current_path_wchar);
    ok(info.capacity >= info.free, "tr2_sys__Statvfs_wchar(): info.capacity < info.free\n");
    ok(info.free >= info.available, "tr2_sys__Statvfs_wchar(): info.free < info.available\n");

    p_tr2_sys__Statvfs(&info, NULL);
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, debugstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, debugstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, debugstr_longlong(info.free));

    p_tr2_sys__Statvfs(&info, "not_exist");
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, debugstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, debugstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, debugstr_longlong(info.free));
}

static void test_tr2_sys__Stat(void)
{
    int i, err_code, ret;
    HANDLE file;
    enum file_type val;
    struct {
        char const *path;
        enum file_type ret;
        int err_code;
        int is_todo;
    } tests[] = {
        { NULL, status_unknown, ERROR_INVALID_PARAMETER, FALSE },
        { "tr2_test_dir",    directory_file, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1",  regular_file, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\not_exist_file  ", file_not_found, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\??invalid_name>>", file_not_found, ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1_link" ,   regular_file, ERROR_SUCCESS, TRUE },
        { "tr2_test_dir\\dir_link", directory_file, ERROR_SUCCESS, TRUE },
    };
    WCHAR testW[] = {'t','r','2','_','t','e','s','t','_','d','i','r',0};
    WCHAR testW2[] = {'t','r','2','_','t','e','s','t','_','d','i','r','/','f','1',0};

    CreateDirectoryA("tr2_test_dir", NULL);
    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(CloseHandle(file), "CloseHandle\n");
    SetLastError(0xdeadbeef);
    ret = pCreateSymbolicLinkA ? pCreateSymbolicLinkA("tr2_test_dir/f1_link", "tr2_test_dir/f1", 0) : FALSE;
    if(!ret && (!pCreateSymbolicLinkA || GetLastError()==ERROR_PRIVILEGE_NOT_HELD||GetLastError()==ERROR_INVALID_FUNCTION)) {
        tests[5].ret = tests[6].ret = file_not_found;
        win_skip("Privilege not held or symbolic link not supported, skipping symbolic link tests.\n");
    }else {
        ok(ret, "CreateSymbolicLinkA failed\n");
        ok(pCreateSymbolicLinkA("tr2_test_dir/dir_link", "tr2_test_dir", 1), "CreateSymbolicLinkA failed\n");
    }

    file = CreateNamedPipeA("\\\\.\\PiPe\\tests_pipe.c",
            PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 2, 1024, 1024,
            NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Stat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    todo_wine ok(regular_file == val, "tr2_sys__Stat(): expect: regular_file, got %d\n", val);
    todo_wine ok(ERROR_SUCCESS == err_code, "tr2_sys__Stat(): err_code expect: ERROR_SUCCESS, got %d\n", err_code);
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    ok(status_unknown == val, "tr2_sys__Lstat(): expect: status_unknown, got %d\n", val);
    todo_wine ok(ERROR_PIPE_BUSY == err_code, "tr2_sys__Lstat(): err_code expect: ERROR_PIPE_BUSY, got %d\n", err_code);
    ok(CloseHandle(file), "CloseHandle\n");
    file = CreateNamedPipeA("\\\\.\\PiPe\\tests_pipe.c",
            PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 2, 1024, 1024,
            NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat("\\\\.\\PiPe\\tests_pipe.c", &err_code);
    todo_wine ok(regular_file == val, "tr2_sys__Lstat(): expect: regular_file, got %d\n", val);
    todo_wine ok(ERROR_SUCCESS == err_code, "tr2_sys__Lstat(): err_code expect: ERROR_SUCCESS, got %d\n", err_code);
    ok(CloseHandle(file), "CloseHandle\n");

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        err_code = 0xdeadbeef;
        val = p_tr2_sys__Stat(tests[i].path, &err_code);
        todo_wine_if(tests[i].is_todo)
            ok(tests[i].ret == val, "tr2_sys__Stat(): test %d expect: %d, got %d\n", i+1, tests[i].ret, val);
        ok(tests[i].err_code == err_code, "tr2_sys__Stat(): test %d err_code expect: %d, got %d\n",
                i+1, tests[i].err_code, err_code);

        /* test tr2_sys__Lstat */
        err_code = 0xdeadbeef;
        val = p_tr2_sys__Lstat(tests[i].path, &err_code);
        todo_wine_if(tests[i].is_todo)
            ok(tests[i].ret == val, "tr2_sys__Lstat(): test %d expect: %d, got %d\n", i+1, tests[i].ret, val);
        ok(tests[i].err_code == err_code, "tr2_sys__Lstat(): test %d err_code expect: %d, got %d\n",
                i+1, tests[i].err_code, err_code);
    }

    err_code = 0xdeadbeef;
    val = p_tr2_sys__Stat_wchar(testW, &err_code);
    ok(directory_file == val, "tr2_sys__Stat_wchar() expect directory_file, got %d\n", val);
    ok(ERROR_SUCCESS == err_code, "tr2_sys__Stat_wchar(): err_code expect ERROR_SUCCESS, got %d\n", err_code);
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat_wchar(testW2, &err_code);
    ok(regular_file == val, "tr2_sys__Lstat_wchar() expect regular_file, got %d\n", val);
    ok(ERROR_SUCCESS == err_code, "tr2_sys__Lstat_wchar(): err_code expect ERROR_SUCCESS, got %d\n", err_code);

    if(ret) {
        todo_wine ok(DeleteFileA("tr2_test_dir/f1_link"), "expect tr2_test_dir/f1_link to exist\n");
        todo_wine ok(RemoveDirectoryA("tr2_test_dir/dir_link"), "expect tr2_test_dir/dir_link to exist\n");
    }
    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(RemoveDirectoryA("tr2_test_dir"), "expect tr2_test_dir to exist\n");
}

static void test_tr2_sys__Last_write_time(void)
{
    HANDLE file;
    int ret;
    __int64 last_write_time, newtime;
    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "tr2_sys__Make_dir() expect 1 got %d\n", ret);

    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    last_write_time = p_tr2_sys__Last_write_time("tr2_test_dir/f1");
    newtime = last_write_time + 123456789;
    p_tr2_sys__Last_write_time_set("tr2_test_dir/f1", newtime);
    todo_wine ok(last_write_time == p_tr2_sys__Last_write_time("tr2_test_dir/f1"),
            "last_write_time should have changed: %s\n",
            debugstr_longlong(last_write_time));

    errno = 0xdeadbeef;
    last_write_time = p_tr2_sys__Last_write_time("not_exist");
    ok(errno == 0xdeadbeef, "tr2_sys__Last_write_time(): errno expect 0xdeadbeef, got %d\n", errno);
    ok(last_write_time == 0, "expect 0 got %s\n", debugstr_longlong(last_write_time));
    last_write_time = p_tr2_sys__Last_write_time(NULL);
    ok(last_write_time == 0, "expect 0 got %s\n", debugstr_longlong(last_write_time));

    p_tr2_sys__Last_write_time_set("not_exist", newtime);
    errno = 0xdeadbeef;
    p_tr2_sys__Last_write_time_set(NULL, newtime);
    ok(errno == 0xdeadbeef, "tr2_sys__Last_write_time(): errno expect 0xdeadbeef, got %d\n", errno);

    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
}

static void test_tr2_sys__dir_operation(void)
{
    char *file_name, first_file_name[MAX_PATH], dest[MAX_PATH], longer_path[MAX_PATH];
    HANDLE file, result_handle;
    enum file_type type;
    int err, num_of_f1 = 0, num_of_f2 = 0, num_of_sub_dir = 0, num_of_other_files = 0;

    CreateDirectoryA("tr2_test_dir", NULL);
    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    file = CreateFileA("tr2_test_dir/f2", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    CreateDirectoryA("tr2_test_dir/sub_dir", NULL);
    file = CreateFileA("tr2_test_dir/sub_dir/sub_f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    memset(longer_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, longer_path);
    strcat(longer_path, "\\tr2_test_dir\\");
    while(lstrlenA(longer_path) < MAX_PATH-1)
        strcat(longer_path, "s");
    ok(lstrlenA(longer_path) == MAX_PATH-1, "tr2_sys__Open_dir(): expect MAX_PATH, got %d\n", lstrlenA(longer_path));
    memset(first_file_name, 0, MAX_PATH);
    type = err =  0xdeadbeef;
    result_handle = NULL;
    result_handle = p_tr2_sys__Open_dir(first_file_name, longer_path, &err, &type);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect NULL, got %p\n", result_handle);
    ok(!*first_file_name, "tr2_sys__Open_dir(): expect: 0, got %s\n", first_file_name);
    ok(err == ERROR_BAD_PATHNAME, "tr2_sys__Open_dir(): expect: ERROR_BAD_PATHNAME, got %d\n", err);
    ok((int)type == 0xdeadbeef, "tr2_sys__Open_dir(): expect 0xdeadbeef, got %d\n", type);

    memset(first_file_name, 0, MAX_PATH);
    memset(dest, 0, MAX_PATH);
    err = type = 0xdeadbeef;
    result_handle = NULL;
    result_handle = p_tr2_sys__Open_dir(first_file_name, "tr2_test_dir", &err, &type);
    ok(result_handle != NULL, "tr2_sys__Open_dir(): expect: not NULL, got %p\n", result_handle);
    ok(err == ERROR_SUCCESS, "tr2_sys__Open_dir(): expect: ERROR_SUCCESS, got %d\n", err);
    file_name = first_file_name;
    while(*file_name) {
        if (!strcmp(file_name, "f1")) {
            ++num_of_f1;
            ok(type == regular_file, "expect regular_file, got %d\n", type);
        }else if(!strcmp(file_name, "f2")) {
            ++num_of_f2;
            ok(type == regular_file, "expect regular_file, got %d\n", type);
        }else if(!strcmp(file_name, "sub_dir")) {
            ++num_of_sub_dir;
            ok(type == directory_file, "expect directory_file, got %d\n", type);
        }else {
            ++num_of_other_files;
        }
        file_name = p_tr2_sys__Read_dir(dest, result_handle, &type);
    }
    p_tr2_sys__Close_dir(result_handle);
    ok(result_handle != NULL, "tr2_sys__Open_dir(): expect: not NULL, got %p\n", result_handle);
    ok(num_of_f1 == 1, "found f1 %d times\n", num_of_f1);
    ok(num_of_f2 == 1, "found f2 %d times\n", num_of_f2);
    ok(num_of_sub_dir == 1, "found sub_dir %d times\n", num_of_sub_dir);
    ok(num_of_other_files == 0, "found %d other files\n", num_of_other_files);

    memset(first_file_name, 0, MAX_PATH);
    err = type = 0xdeadbeef;
    result_handle = file;
    result_handle = p_tr2_sys__Open_dir(first_file_name, "not_exist", &err, &type);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect: NULL, got %p\n", result_handle);
    todo_wine ok(err == ERROR_BAD_PATHNAME, "tr2_sys__Open_dir(): expect: ERROR_BAD_PATHNAME, got %d\n", err);
    ok((int)type == 0xdeadbeef, "tr2_sys__Open_dir(): expect: 0xdeadbeef, got %d\n", type);
    ok(!*first_file_name, "tr2_sys__Open_dir(): expect: 0, got %s\n", first_file_name);

    CreateDirectoryA("empty_dir", NULL);
    memset(first_file_name, 0, MAX_PATH);
    err = type = 0xdeadbeef;
    result_handle = file;
    result_handle = p_tr2_sys__Open_dir(first_file_name, "empty_dir", &err, &type);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect: NULL, got %p\n", result_handle);
    ok(err == ERROR_SUCCESS, "tr2_sys__Open_dir(): expect: ERROR_SUCCESS, got %d\n", err);
    ok(type == status_unknown, "tr2_sys__Open_dir(): expect: status_unknown, got %d\n", type);
    ok(!*first_file_name, "tr2_sys__Open_dir(): expect: 0, got %s\n", first_file_name);
    p_tr2_sys__Close_dir(result_handle);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect: NULL, got %p\n", result_handle);

    ok(RemoveDirectoryA("empty_dir"), "expect empty_dir to exist\n");
    ok(DeleteFileA("tr2_test_dir/sub_dir/sub_f1"), "expect tr2_test_dir/sub_dir/sub_f1 to exist\n");
    ok(RemoveDirectoryA("tr2_test_dir/sub_dir"), "expect tr2_test_dir/sub_dir to exist\n");
    ok(DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 to exist\n");
    ok(DeleteFileA("tr2_test_dir/f2"), "expect tr2_test_dir/f2 to exist\n");
    ok(RemoveDirectoryA("tr2_test_dir"), "expect tr2_test_dir to exist\n");
}

static void test_tr2_sys__Link(void)
{
    int ret, i;
    HANDLE file, h1, h2;
    BY_HANDLE_FILE_INFORMATION info1, info2;
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    LARGE_INTEGER file_size;
    struct {
        char const *existing_path;
        char const *new_path;
        MSVCP_bool fail_if_exists;
        int last_error;
    } tests[] = {
        { "f1", "f1_link", TRUE, ERROR_SUCCESS },
        { "f1", "tr2_test_dir\\f1_link", TRUE, ERROR_SUCCESS },
        { "tr2_test_dir\\f1_link", "tr2_test_dir\\f1_link_link", TRUE, ERROR_SUCCESS },
        { "tr2_test_dir", "dir_link", TRUE, ERROR_ACCESS_DENIED },
        { NULL, "NULL_link", TRUE, ERROR_INVALID_PARAMETER },
        { "f1", NULL, TRUE, ERROR_INVALID_PARAMETER },
        { "not_exist", "not_exist_link", TRUE, ERROR_FILE_NOT_FOUND },
        { "f1", "not_exist_dir\\f1_link", TRUE, ERROR_PATH_NOT_FOUND }
    };

    memset(current_path, 0, MAX_PATH);
    GetCurrentDirectoryA(MAX_PATH, current_path);
    memset(temp_path, 0, MAX_PATH);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Link(tests[i].existing_path, tests[i].new_path);
        ok(ret == tests[i].last_error, "tr2_sys__Link(): test %d expect: %d, got %d\n",
                i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Link(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].existing_path) == p_tr2_sys__File_size(tests[i].new_path),
                    "tr2_sys__Link(): test %d failed, two files' size are not equal\n", i+1);
    }

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    ok(p_tr2_sys__File_size("f1_link") == p_tr2_sys__File_size("tr2_test_dir/f1_link") &&
            p_tr2_sys__File_size("tr2_test_dir/f1_link") == p_tr2_sys__File_size("tr2_test_dir/f1_link_link"),
            "tr2_sys__Link(): expect links' size are equal, got %s\n", debugstr_longlong(p_tr2_sys__File_size("tr2_test_dir/f1_link_link")));
    ok(p_tr2_sys__File_size("f1_link") == 7, "tr2_sys__Link(): expect f1_link's size equals to 7, got %s\n", debugstr_longlong(p_tr2_sys__File_size("f1_link")));

    file = CreateFileA("f1_link", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 20;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    h1 = CreateFileA("f1_link", 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    ok(h1 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(GetFileInformationByHandle(h1, &info1), "GetFileInformationByHandle failed\n");
    CloseHandle(h1);
    h2 = CreateFileA("tr2_test_dir/f1_link", 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    ok(h2 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    ok(GetFileInformationByHandle(h2, &info2), "GetFileInformationByHandle failed\n");
    CloseHandle(h2);
    ok(info1.nFileIndexHigh == info2.nFileIndexHigh
            && info1.nFileIndexLow == info2.nFileIndexLow,
            "tr2_sys__Link(): test %d expect two files equivalent\n", i+1);
    ok(p_tr2_sys__File_size("f1_link") == 20, "tr2_sys__Link(): expect f1_link's size equals to 20, got %s\n", debugstr_longlong(p_tr2_sys__File_size("f1_link")));

    ok(DeleteFileA("f1_link"), "expect f1_link to exist\n");
    ok(DeleteFileA("tr2_test_dir/f1_link"), "expect tr2_test_dir/f1_link to exist\n");
    ok(DeleteFileA("tr2_test_dir/f1_link_link"), "expect tr2_test_dir/f1_link_link to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static void test_tr2_sys__Symlink(void)
{
    int ret, i;
    HANDLE file;
    LARGE_INTEGER file_size;
    struct {
        char const *existing_path;
        char const *new_path;
        int last_error;
        MSVCP_bool is_todo;
    } tests[] = {
        { "f1", "f1_link", ERROR_SUCCESS, FALSE },
        { "f1", "tr2_test_dir\\f1_link", ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1_link", "tr2_test_dir\\f1_link_link", ERROR_SUCCESS, FALSE },
        { "tr2_test_dir", "dir_link", ERROR_SUCCESS, FALSE },
        { NULL, "NULL_link", ERROR_INVALID_PARAMETER, FALSE },
        { "f1", NULL, ERROR_INVALID_PARAMETER, FALSE },
        { "not_exist",  "not_exist_link", ERROR_SUCCESS, FALSE },
        { "f1", "not_exist_dir\\f1_link", ERROR_PATH_NOT_FOUND, TRUE }
    };

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = p_tr2_sys__Symlink(tests[i].existing_path, tests[i].new_path);
        if(!i && (ret==ERROR_PRIVILEGE_NOT_HELD || ret==ERROR_INVALID_FUNCTION)) {
            win_skip("Privilege not held or symbolic link not supported, skipping symbolic link tests.\n");
            ok(DeleteFileA("f1"), "expect f1 to exist\n");
            ret = p_tr2_sys__Remove_dir("tr2_test_dir");
            ok(ret == 1, "tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
            return;
        }

        ok(errno == 0xdeadbeef, "tr2_sys__Symlink(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        todo_wine_if(tests[i].is_todo)
            ok(ret == tests[i].last_error, "tr2_sys__Symlink(): test %d expect: %d, got %d\n", i+1, tests[i].last_error, ret);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].new_path) == 0, "tr2_sys__Symlink(): expect 0, got %s\n", debugstr_longlong(p_tr2_sys__File_size(tests[i].new_path)));
    }

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    todo_wine ok(DeleteFileA("f1_link"), "expect f1_link to exist\n");
    todo_wine ok(DeleteFileA("tr2_test_dir/f1_link"), "expect tr2_test_dir/f1_link to exist\n");
    todo_wine ok(DeleteFileA("tr2_test_dir/f1_link_link"), "expect tr2_test_dir/f1_link_link to exist\n");
    todo_wine ok(DeleteFileA("not_exist_link"), "expect not_exist_link to exist\n");
    todo_wine ok(DeleteFileA("dir_link"), "expect dir_link to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "tr2_sys__Remove_dir(): expect 1 got %d\n", ret);
}

static void test_tr2_sys__Unlink(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH];
    int ret, i;
    HANDLE file;
    LARGE_INTEGER file_size;
    struct {
        char const *path;
        int last_error;
        MSVCP_bool is_todo;
    } tests[] = {
        { "tr2_test_dir\\f1_symlink", ERROR_SUCCESS, TRUE },
        { "tr2_test_dir\\f1_link", ERROR_SUCCESS, FALSE },
        { "tr2_test_dir\\f1", ERROR_SUCCESS, FALSE },
        { "tr2_test_dir", ERROR_ACCESS_DENIED, FALSE },
        { "not_exist", ERROR_FILE_NOT_FOUND, FALSE },
        { "not_exist_dir\\not_exist_file", ERROR_PATH_NOT_FOUND, FALSE },
        { NULL, ERROR_PATH_NOT_FOUND, FALSE }
    };

    GetCurrentDirectoryA(MAX_PATH, current_path);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    ret = p_tr2_sys__Symlink("tr2_test_dir/f1", "tr2_test_dir/f1_symlink");
    if(ret==ERROR_PRIVILEGE_NOT_HELD || ret==ERROR_INVALID_FUNCTION) {
        tests[0].last_error = ERROR_FILE_NOT_FOUND;
        win_skip("Privilege not held or symbolic link not supported, skipping symbolic link tests.\n");
    }else {
        ok(ret == ERROR_SUCCESS, "tr2_sys__Symlink(): expect: ERROR_SUCCESS, got %d\n", ret);
    }
    ret = p_tr2_sys__Link("tr2_test_dir/f1", "tr2_test_dir/f1_link");
    ok(ret == ERROR_SUCCESS, "tr2_sys__Link(): expect: ERROR_SUCCESS, got %d\n", ret);

    for(i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Unlink(tests[i].path);
        todo_wine_if(tests[i].is_todo)
            ok(ret == tests[i].last_error, "tr2_sys__Unlink(): test %d expect: %d, got %d\n",
                    i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Unlink(): test %d errno expect: 0xdeadbeef, got %d\n", i+1, ret);
    }

    ok(!DeleteFileA("tr2_test_dir/f1"), "expect tr2_test_dir/f1 not to exist\n");
    ok(!DeleteFileA("tr2_test_dir/f1_link"), "expect tr2_test_dir/f1_link not to exist\n");
    ok(!DeleteFileA("tr2_test_dir/f1_symlink"), "expect tr2_test_dir/f1_symlink not to exist\n");
    ret = p_tr2_sys__Remove_dir("tr2_test_dir");
    ok(ret == 1, "tr2_sys__Remove_dir(): expect 1 got %d\n", ret);

    ok(SetCurrentDirectoryA(current_path), "SetCurrentDirectoryA failed\n");
}

static int __cdecl thrd_thread(void *arg)
{
    _Thrd_t *thr = arg;

    if(thr)
        *thr = p__Thrd_current();
    return 0x42;
}

static void test_thrd(void)
{
    int ret, i, r;
    struct test {
        _Thrd_t a;
        _Thrd_t b;
        int     r;
    };
    const HANDLE hnd1 = (HANDLE)0xcccccccc;
    const HANDLE hnd2 = (HANDLE)0xdeadbeef;
    xtime xt, before, after;
    MSVCRT_long diff;
    _Thrd_t ta, tb;

    struct test testeq[] = {
        { {0,    0}, {0,    0}, 1 },
        { {0,    1}, {0,    0}, 0 },
        { {hnd1, 0}, {hnd1, 1}, 0 },
        { {hnd1, 0}, {hnd2, 0}, 1 }
    };

    struct test testlt[] = {
        { {0,    0}, {0,    0}, 0 },
        { {0,    0}, {0,    1}, 1 },
        { {0,    1}, {0,    0}, 0 },
        { {hnd1, 0}, {hnd2, 0}, 0 },
        { {hnd1, 0}, {hnd2, 1}, 1 }
    };

    /* test for equal */
    for(i=0; i<sizeof(testeq)/sizeof(testeq[0]); i++) {
        ret = p__Thrd_equal(testeq[i].a, testeq[i].b);
        ok(ret == testeq[i].r, "(%p %u) = (%p %u) expected %d, got %d\n",
            testeq[i].a.hnd, testeq[i].a.id, testeq[i].b.hnd, testeq[i].b.id, testeq[i].r, ret);
    }

    /* test for less than */
    for(i=0; i<sizeof(testlt)/sizeof(testlt[0]); i++) {
        ret = p__Thrd_lt(testlt[i].a, testlt[i].b);
        ok(ret == testlt[i].r, "(%p %u) < (%p %u) expected %d, got %d\n",
            testlt[i].a.hnd, testlt[i].a.id, testlt[i].b.hnd, testlt[i].b.id, testlt[i].r, ret);
    }

    /* test for sleep */
    if (0) /* crash on Windows */
        p__Thrd_sleep(NULL);
    p_xtime_get(&xt, 1);
    xt.sec += 2;
    p_xtime_get(&before, 1);
    p__Thrd_sleep(&xt);
    p_xtime_get(&after, 1);
    diff = p__Xtime_diff_to_millis2(&after, &before);
    ok(diff > 2000 - TIMEDELTA, "got %d\n", diff);

    /* test for current */
    ta = p__Thrd_current();
    tb = p__Thrd_current();
    ok(ta.id == tb.id, "got a %d b %d\n", ta.id, tb.id);
    ok(ta.id == GetCurrentThreadId(), "expected %d, got %d\n", GetCurrentThreadId(), ta.id);
    /* these can be different if new threads are created at same time */
    ok(ta.hnd == tb.hnd, "got a %p b %p\n", ta.hnd, tb.hnd);
    ok(!CloseHandle(ta.hnd), "handle %p not closed\n", ta.hnd);
    ok(!CloseHandle(tb.hnd), "handle %p not closed\n", tb.hnd);

    /* test for create/join */
    if (0) /* crash on Windows */
    {
        p__Thrd_create(NULL, thrd_thread, NULL);
        p__Thrd_create(&ta, NULL, NULL);
    }
    r = -1;
    ret = p__Thrd_create(&ta, thrd_thread, (void*)&tb);
    ok(!ret, "failed to create thread, got %d\n", ret);
    ret = p__Thrd_join(ta, &r);
    ok(!ret, "failed to join thread, got %d\n", ret);
    ok(ta.id == tb.id, "expected %d, got %d\n", ta.id, tb.id);
    ok(ta.hnd != tb.hnd, "same handles, got %p\n", ta.hnd);
    ok(r == 0x42, "expected 0x42, got %d\n", r);
    ret = p__Thrd_detach(ta);
    ok(ret == 4, "_Thrd_detach should have failed with error 4, got %d\n", ret);

    ret = p__Thrd_create(&ta, thrd_thread, NULL);
    ok(!ret, "failed to create thread, got %d\n", ret);
    ret = p__Thrd_detach(ta);
    ok(!ret, "_Thrd_detach failed, got %d\n", ret);

}

#define NUM_THREADS 10
struct cndmtx
{
    HANDLE initialized;
    int started;
    int thread_no;

    _Cnd_t cnd;
    _Mtx_t mtx;
    BOOL timed_wait;
};

static int __cdecl cnd_wait_thread(void *arg)
{
    struct cndmtx *cm = arg;
    int r;

    p__Mtx_lock(&cm->mtx);

    if(InterlockedIncrement(&cm->started) == cm->thread_no)
        SetEvent(cm->initialized);

    if(cm->timed_wait) {
        xtime xt;

        p_xtime_get(&xt, 1);
        xt.sec += 2;
        r = p__Cnd_timedwait(&cm->cnd, &cm->mtx, &xt);
        ok(!r, "timed wait failed\n");
    } else {
        r = p__Cnd_wait(&cm->cnd, &cm->mtx);
        ok(!r, "wait failed\n");
    }

    p__Mtx_unlock(&cm->mtx);
    return 0;
}

static void test_cnd(void)
{
    _Thrd_t threads[NUM_THREADS];
    xtime xt, before, after;
    MSVCRT_long diff;
    struct cndmtx cm;
    _Cnd_t cnd;
    _Mtx_t mtx;
    int r, i;

    r = p__Cnd_init(&cnd);
    ok(!r, "failed to init cnd\n");

    r = p__Mtx_init(&mtx, 0);
    ok(!r, "failed to init mtx\n");

    if (0) /* crash on Windows */
    {
        p__Cnd_init(NULL);
        p__Cnd_wait(NULL, &mtx);
        p__Cnd_wait(&cnd, NULL);
        p__Cnd_timedwait(NULL, &mtx, &xt);
        p__Cnd_timedwait(&cnd, &mtx, &xt);
    }
    p__Cnd_destroy(NULL);

    /* test _Cnd_signal/_Cnd_wait */
    cm.initialized = CreateEventW(NULL, FALSE, FALSE, NULL);
    cm.started = 0;
    cm.thread_no = 1;
    cm.cnd = cnd;
    cm.mtx = mtx;
    cm.timed_wait = FALSE;
    p__Thrd_create(&threads[0], cnd_wait_thread, (void*)&cm);

    WaitForSingleObject(cm.initialized, INFINITE);
    p__Mtx_lock(&mtx);
    p__Mtx_unlock(&mtx);

    r = p__Cnd_signal(&cm.cnd);
    ok(!r, "failed to signal\n");
    p__Thrd_join(threads[0], NULL);

    /* test _Cnd_timedwait time out */
    p__Mtx_lock(&mtx);
    p_xtime_get(&before, 1);
    xt = before;
    xt.sec += 1;
    r = p__Cnd_timedwait(&cnd, &mtx, &xt);
    p_xtime_get(&after, 1);
    p__Mtx_unlock(&mtx);

    diff = p__Xtime_diff_to_millis2(&after, &before);
    ok(r == 2, "should have timed out\n");
    ok(diff > 1000 - TIMEDELTA, "got %d\n", diff);

    /* test _Cnd_timedwait */
    cm.started = 0;
    cm.timed_wait = TRUE;
    p__Thrd_create(&threads[0], cnd_wait_thread, (void*)&cm);

    WaitForSingleObject(cm.initialized, INFINITE);
    p__Mtx_lock(&mtx);
    p__Mtx_unlock(&mtx);

    r = p__Cnd_signal(&cm.cnd);
    ok(!r, "failed to signal\n");
    p__Thrd_join(threads[0], NULL);

    /* test _Cnd_do_broadcast_at_thread_exit */
    cm.started = 0;
    cm.timed_wait = FALSE;
    p__Thrd_create(&threads[0], cnd_wait_thread, (void*)&cm);

    WaitForSingleObject(cm.initialized, INFINITE);
    p__Mtx_lock(&mtx);

    r = 0xcafe;
    p__Cnd_unregister_at_thread_exit((_Mtx_t*)0xdeadbeef);
    p__Cnd_register_at_thread_exit(&cnd, &mtx, &r);
    ok(r == 0xcafe, "r = %x\n", r);
    p__Cnd_register_at_thread_exit(&cnd, &mtx, &r);
    p__Cnd_unregister_at_thread_exit(&mtx);
    p__Cnd_do_broadcast_at_thread_exit();
    ok(mtx->count == 1, "mtx.count = %d\n", mtx->count);

    p__Cnd_register_at_thread_exit(&cnd, &mtx, &r);
    ok(r == 0xcafe, "r = %x\n", r);

    p__Cnd_do_broadcast_at_thread_exit();
    ok(r == 1, "r = %x\n", r);
    p__Thrd_join(threads[0], NULL);

    /* crash if _Cnd_do_broadcast_at_thread_exit is called on exit */
    p__Cnd_register_at_thread_exit((_Cnd_t*)0xdeadbeef, (_Mtx_t*)0xdeadbeef, (int*)0xdeadbeef);

    /* test _Cnd_broadcast */
    cm.started = 0;
    cm.thread_no = NUM_THREADS;

    for(i = 0; i < cm.thread_no; i++)
        p__Thrd_create(&threads[i], cnd_wait_thread, (void*)&cm);

    WaitForSingleObject(cm.initialized, INFINITE);
    p__Mtx_lock(&mtx);
    p__Mtx_unlock(&mtx);

    r = p__Cnd_broadcast(&cnd);
    ok(!r, "failed to broadcast\n");
    for(i = 0; i < cm.thread_no; i++)
        p__Thrd_join(threads[i], NULL);

    /* test broadcast with _Cnd_destroy */
    cm.started = 0;
    for(i = 0; i < cm.thread_no; i++)
        p__Thrd_create(&threads[i], cnd_wait_thread, (void*)&cm);

    WaitForSingleObject(cm.initialized, INFINITE);
    p__Mtx_lock(&mtx);
    p__Mtx_unlock(&mtx);

    p__Cnd_destroy(&cnd);
    for(i = 0; i < cm.thread_no; i++)
        p__Thrd_join(threads[i], NULL);

    p__Mtx_destroy(&mtx);
    CloseHandle(cm.initialized);
}

static struct {
    int value[2];
    const char* export_name;
} vbtable_size_exports_list[] = {
    {{0x20, 0x20}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@"},
    {{0x20, 0x20}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@"},
    {{0x20, 0x20}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x10, 0x10}, "??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@"},
    {{0x18, 0x18}, "??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@"},
    {{0x18, 0x18}, "??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@"},
    {{0x18, 0x18}, "??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@"},
    {{ 0x8, 0x10}, "??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@"},
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

HANDLE _Pad__Launch_returned;
_Pad pad;
#ifdef __i386__
/* TODO: this should be a __thiscall function */
static unsigned int __stdcall vtbl_func__Go(void)
#else
static unsigned int __cdecl vtbl_func__Go(_Pad *this)
#endif
{
    DWORD ret;

    ret = WaitForSingleObject(_Pad__Launch_returned, 100);
    ok(ret == WAIT_TIMEOUT, "WiatForSingleObject returned %x\n", ret);
    ok(!pad.mtx->count, "pad.mtx.count = %d\n", pad.mtx->count);
    ok(!pad.launched, "pad.launched = %x\n", pad.launched);
    call_func1(p__Pad__Release, &pad);
    ok(pad.launched, "pad.launched = %x\n", pad.launched);
    ret = WaitForSingleObject(_Pad__Launch_returned, 100);
    ok(ret == WAIT_OBJECT_0, "WiatForSingleObject returned %x\n", ret);
    ok(pad.mtx->count == 1, "pad.mtx.count = %d\n", pad.mtx->count);
    return 0;
}

static void test__Pad(void)
{
    _Pad pad_copy;
    _Thrd_t thrd;
    vtable_ptr pfunc = (vtable_ptr)&vtbl_func__Go;

    _Pad__Launch_returned = CreateEventW(NULL, FALSE, FALSE, NULL);

    pad.vtable = (void*)1;
    pad.cnd = (void*)2;
    pad.mtx = (void*)3;
    pad.launched = TRUE;
    memset(&pad_copy, 0, sizeof(pad_copy));
    call_func2(p__Pad_copy_ctor, &pad_copy, &pad);
    ok(pad_copy.vtable != (void*)1, "pad_copy.vtable was not set\n");
    ok(pad_copy.cnd == (void*)2, "pad_copy.cnd = %p\n", pad_copy.cnd);
    ok(pad_copy.mtx == (void*)3, "pad_copy.mtx = %p\n", pad_copy.mtx);
    ok(pad_copy.launched, "pad_copy.launched = %x\n", pad_copy.launched);

    memset(&pad_copy, 0xde, sizeof(pad_copy));
    pad_copy.vtable = (void*)4;
    pad_copy.cnd = (void*)5;
    pad_copy.mtx = (void*)6;
    pad_copy.launched = FALSE;
    call_func2(p__Pad_op_assign, &pad_copy, &pad);
    ok(pad_copy.vtable == (void*)4, "pad_copy.vtable was set\n");
    ok(pad_copy.cnd == (void*)2, "pad_copy.cnd = %p\n", pad_copy.cnd);
    ok(pad_copy.mtx == (void*)3, "pad_copy.mtx = %p\n", pad_copy.mtx);
    ok(pad_copy.launched, "pad_copy.launched = %x\n", pad_copy.launched);

    call_func1(p__Pad_ctor, &pad);
    call_func2(p__Pad_copy_ctor, &pad_copy, &pad);
    ok(pad.vtable == pad_copy.vtable, "pad.vtable = %p, pad_copy.vtable = %p\n", pad.vtable, pad_copy.vtable);
    ok(pad.cnd == pad_copy.cnd, "pad.cnd = %p, pad_copy.cnd = %p\n", pad.cnd, pad_copy.cnd);
    ok(pad.mtx == pad_copy.mtx, "pad.mtx = %p, pad_copy.mtx = %p\n", pad.mtx, pad_copy.mtx);
    ok(pad.launched == pad_copy.launched, "pad.launched = %x, pad_copy.launched = %x\n", pad.launched, pad_copy.launched);
    call_func1(p__Pad_dtor, &pad);
    /* call_func1(p__Pad_dtor, &pad_copy);  - copy constructor is broken, this causes a crash */

    memset(&pad, 0xfe, sizeof(pad));
    call_func1(p__Pad_ctor, &pad);
    ok(!pad.launched, "pad.launched = %x\n", pad.launched);
    ok(pad.mtx->count == 1, "pad.mtx.count = %d\n", pad.mtx->count);

    pad.vtable = &pfunc;
    call_func2(p__Pad__Launch, &pad, &thrd);
    SetEvent(_Pad__Launch_returned);
    ok(!p__Thrd_join(thrd, NULL), "_Thrd_join failed\n");

    call_func1(p__Pad_dtor, &pad);
    CloseHandle(_Pad__Launch_returned);
}

START_TEST(msvcp120)
{
    if(!init()) return;
    test__Xtime_diff_to_millis2();
    test_xtime_get();
    test__Getcvt();
    test__Call_once();
    test__Do_call();
    test__Dtest();
    test__Dscale();
    test__FExp();

    test_tr2_sys__File_size();
    test_tr2_sys__Equivalent();
    test_tr2_sys__Current_get();
    test_tr2_sys__Current_set();
    test_tr2_sys__Make_dir();
    test_tr2_sys__Remove_dir();
    test_tr2_sys__Copy_file();
    test_tr2_sys__Rename();
    test_tr2_sys__Statvfs();
    test_tr2_sys__Stat();
    test_tr2_sys__Last_write_time();
    test_tr2_sys__dir_operation();
    test_tr2_sys__Link();
    test_tr2_sys__Symlink();
    test_tr2_sys__Unlink();

    test_thrd();
    test_cnd();
    test__Pad();

    test_vbtable_size_exports();

    FreeLibrary(msvcp);
}
