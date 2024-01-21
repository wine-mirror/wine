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

DWORD expect_idx;
static int vector_alloc_count;
static int vector_elem_count;
typedef struct blocks_to_free{
    size_t first_block;
    void *blocks[sizeof(void*)*8];
    int size_check;
}compact_block;

#define DEFINE_EXPECT(func) \
    BOOL expect_ ## func, called_ ## func

struct expect_struct {
    DEFINE_EXPECT(queue_char__Allocate_page);
    DEFINE_EXPECT(queue_char__Deallocate_page);
    DEFINE_EXPECT(queue_char__Move_item);
    DEFINE_EXPECT(queue_char__Copy_item);
    DEFINE_EXPECT(queue_char__Assign_and_destroy_item);
    DEFINE_EXPECT(concurrent_vector_int_alloc);
    DEFINE_EXPECT(concurrent_vector_int_destroy);
    DEFINE_EXPECT(concurrent_vector_int_copy);
    DEFINE_EXPECT(concurrent_vector_int_assign);
};

#define SET_EXPECT(func) \
    do { \
        struct expect_struct *p = TlsGetValue(expect_idx); \
        ok(p != NULL, "expect_struct not initialized\n"); \
        p->expect_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        struct expect_struct *p = TlsGetValue(expect_idx); \
        ok(p != NULL, "expect_struct not initialized\n"); \
        ok(p->expect_ ##func, "unexpected call " #func "\n"); \
        p->called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        struct expect_struct *p = TlsGetValue(expect_idx); \
        ok(p != NULL, "expect_struct not initialized\n"); \
        CHECK_EXPECT2(func); \
        p->expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        struct expect_struct *p = TlsGetValue(expect_idx); \
        ok(p != NULL, "expect_struct not initialized\n"); \
        ok(p->called_ ## func, "expected " #func "\n"); \
        p->expect_ ## func = p->called_ ## func = FALSE; \
    }while(0)

static void alloc_expect_struct(void)
{
    struct expect_struct *p = malloc(sizeof(*p));
    memset(p, 0, sizeof(*p));
    ok(TlsSetValue(expect_idx, p), "TlsSetValue failed\n");
}

static void free_expect_struct(void)
{
    struct expect_struct *p = TlsGetValue(expect_idx);
    ok(TlsSetValue(expect_idx, NULL), "TlsSetValue failed\n");
    free(p);
}

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
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, void *a );
static void * (WINAPI *call_thiscall_func3)( void *func, void *this, void *a, void *b );
static void * (WINAPI *call_thiscall_func4)( void *func, void *this, void *a, void *b, void *c );
static void * (WINAPI *call_thiscall_func5)( void *func, void *this, void *a, void *b, void *c, void *d );
static void * (WINAPI *call_thiscall_func6)( void *func, void *this, void *a, void *b, void *c, void *d, void *e );
static void * (WINAPI *call_thiscall_func7)( void *func, void *this, void *a, void *b, void *c, void *d, void *e, void *f );

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
    call_thiscall_func4 = (void *)thunk;
    call_thiscall_func5 = (void *)thunk;
    call_thiscall_func6 = (void *)thunk;
    call_thiscall_func7 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(void*)(a))
#define call_func3(func,_this,a,b) call_thiscall_func3(func,_this,(void*)(a),(void*)(b))
#define call_func4(func,_this,a,b,c) call_thiscall_func4(func,_this,(void*)(a),(void*)(b),(void*)(c))
#define call_func5(func,_this,a,b,c,d) call_thiscall_func5(func,_this,(void*)(a),(void*)(b),(void*)(c),(void*)(d))
#define call_func6(func,_this,a,b,c,d,e) call_thiscall_func6(func,_this,(void*)(a),(void*)(b),(void*)(c),(void*)(d),(void*)(e))
#define call_func7(func,_this,a,b,c,d,e,f) call_thiscall_func7(func,_this,(void*)(a),(void*)(b),(void*)(c),(void*)(d),(void*)(e),(void*)(f))
#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)
#define call_func3(func,_this,a,b) func(_this,a,b)
#define call_func4(func,_this,a,b,c) func(_this,a,b,c)
#define call_func5(func,_this,a,b,c,d) func(_this,a,b,c,d)
#define call_func6(func,_this,a,b,c,d,e) func(_this,a,b,c,d,e)
#define call_func7(func,_this,a,b,c,d,e,f) func(_this,a,b,c,d,e,f)
#endif /* __i386__ */

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

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static char* (__cdecl *p_setlocale)(int, const char*);
static int (__cdecl *p__setmbcp)(int);
static int (__cdecl *p__ismbblead)(unsigned int);

static MSVCRT_long (__cdecl *p__Xtime_diff_to_millis2)(const xtime*, const xtime*);
static int (__cdecl *p_xtime_get)(xtime*, int);
static _Cvtvec (__cdecl *p__Getcvt)(void);
static void (CDECL *p__Call_once)(int *once, void (CDECL *func)(void));
static void (CDECL *p__Call_onceEx)(int *once, void (CDECL *func)(void*), void *argv);
static void (CDECL *p__Do_call)(void *this);
static short (__cdecl *p__Dtest)(double *d);
static short (__cdecl *p__Dscale)(double *d, int exp);
static short (__cdecl *p__FExp)(float *x, float y, int exp);
static const char* (__cdecl *p__Syserror_map)(int err);

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
static __int64 (__cdecl *p_tr2_sys__Last_write_time_wchar)(WCHAR const*);
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

#define MTX_PLAIN 0x1
#define MTX_TRY 0x2
#define MTX_TIMED 0x4
#define MTX_RECURSIVE 0x100
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
static int (__cdecl *p__Mtx_trylock)(_Mtx_t*);

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

static void (__cdecl *p_threads__Mtx_new)(void **mtx);
static void (__cdecl *p_threads__Mtx_delete)(void *mtx);
static void (__cdecl *p_threads__Mtx_lock)(void *mtx);
static void (__cdecl *p_threads__Mtx_unlock)(void *mtx);

static BOOLEAN (WINAPI *pCreateSymbolicLinkA)(LPCSTR,LPCSTR,DWORD);

static size_t (__cdecl *p_vector_base_v4__Segment_index_of)(size_t);

typedef struct
{
    const vtable_ptr *vtable;
    void *data;
    size_t alloc_count;
    size_t item_size;
} queue_base_v4;

typedef struct
{
    struct _Page *_Next;
    size_t _Mask;
    char data[1];
} _Page;

static queue_base_v4* (__thiscall *p_queue_base_v4_ctor)(queue_base_v4*, size_t);
static void (__thiscall *p_queue_base_v4_dtor)(queue_base_v4*);
static MSVCP_bool (__thiscall *p_queue_base_v4__Internal_empty)(queue_base_v4*);
static size_t (__thiscall *p_queue_base_v4__Internal_size)(queue_base_v4*);
static void (__thiscall *p_queue_base_v4__Internal_push)(queue_base_v4*, const void*);
static void (__thiscall *p_queue_base_v4__Internal_move_push)(queue_base_v4*, void*);
static MSVCP_bool (__thiscall *p_queue_base_v4__Internal_pop_if_present)(queue_base_v4*, void*);
static void (__thiscall *p_queue_base_v4__Internal_finish_clear)(queue_base_v4*);

typedef struct vector_base_v4
{
    void* (__cdecl *allocator)(struct vector_base_v4*, size_t);
    void *storage[3];
    size_t first_block;
    size_t early_size;
    void **segment;
} vector_base_v4;

static void (__thiscall *p_vector_base_v4_dtor)(vector_base_v4*);
static size_t (__thiscall *p_vector_base_v4__Internal_capacity)(vector_base_v4*);
static void* (__thiscall *p_vector_base_v4__Internal_push_back)(
        vector_base_v4*, size_t, size_t*);
static size_t (__thiscall *p_vector_base_v4__Internal_clear)(
        vector_base_v4*, void (__cdecl*)(void*, size_t));
static void (__thiscall *p_vector_base_v4__Internal_copy)(
        vector_base_v4*, vector_base_v4*, size_t, void (__cdecl*)(void*, const void*, size_t));
static void (__thiscall *p_vector_base_v4__Internal_assign)(
        vector_base_v4*, vector_base_v4*, size_t, void (__cdecl*)(void*, size_t),
        void (__cdecl*)(void*, const void*, size_t), void (__cdecl*)(void*, const void*, size_t));
static void (__thiscall *p_vector_base_v4__Internal_swap)(
        vector_base_v4*, const vector_base_v4*);
static void* (__thiscall *p_vector_base_v4__Internal_compact)(
        vector_base_v4*, size_t, void*, void (__cdecl*)(void*, size_t),
        void (__cdecl*)(void*, const void*, size_t));
static size_t (__thiscall *p_vector_base_v4__Internal_grow_by)(
        vector_base_v4*, size_t, size_t, void (__cdecl*)(void*, const void*, size_t), const void *);
static size_t (__thiscall *p_vector_base_v4__Internal_grow_to_at_least_with_result)(
        vector_base_v4*, size_t, size_t, void (__cdecl*)(void*, const void*, size_t), const void *);
static void (__thiscall *p_vector_base_v4__Internal_reserve)(
        vector_base_v4*, size_t, size_t, size_t);
static void (__thiscall *p_vector_base_v4__Internal_resize)(
        vector_base_v4*, size_t, size_t, size_t, void (__cdecl*)(void*, size_t),
        void (__cdecl *copy)(void*, const void*, size_t), const void*);

static const BYTE *p_byte_reverse_table;

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
        SET(p_tr2_sys__Last_write_time_wchar,
                "?_Last_write_time@sys@tr2@std@@YA_JPEB_W@Z");
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
        SET(p_threads__Mtx_new,
                "?_Mtx_new@threads@stdext@@YAXAEAPEAX@Z");
        SET(p_threads__Mtx_delete,
                "?_Mtx_delete@threads@stdext@@YAXPEAX@Z");
        SET(p_threads__Mtx_lock,
                "?_Mtx_lock@threads@stdext@@YAXPEAX@Z");
        SET(p_threads__Mtx_unlock,
                "?_Mtx_unlock@threads@stdext@@YAXPEAX@Z");
        SET(p_vector_base_v4__Segment_index_of,
                "?_Segment_index_of@_Concurrent_vector_base_v4@details@Concurrency@@KA_K_K@Z");
        SET(p_queue_base_v4_ctor,
                "??0_Concurrent_queue_base_v4@details@Concurrency@@IEAA@_K@Z");
        SET(p_queue_base_v4_dtor,
                "??1_Concurrent_queue_base_v4@details@Concurrency@@MEAA@XZ");
        SET(p_queue_base_v4__Internal_empty,
                "?_Internal_empty@_Concurrent_queue_base_v4@details@Concurrency@@IEBA_NXZ");
        SET(p_queue_base_v4__Internal_size,
                "?_Internal_size@_Concurrent_queue_base_v4@details@Concurrency@@IEBA_KXZ");
        SET(p_queue_base_v4__Internal_push,
                "?_Internal_push@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXPEBX@Z");
        SET(p_queue_base_v4__Internal_move_push,
                "?_Internal_move_push@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXPEAX@Z");
        SET(p_queue_base_v4__Internal_pop_if_present,
                "?_Internal_pop_if_present@_Concurrent_queue_base_v4@details@Concurrency@@IEAA_NPEAX@Z");
        SET(p_queue_base_v4__Internal_finish_clear,
                "?_Internal_finish_clear@_Concurrent_queue_base_v4@details@Concurrency@@IEAAXXZ");
        SET(p_vector_base_v4_dtor,
                "??1_Concurrent_vector_base_v4@details@Concurrency@@IEAA@XZ");
        SET(p_vector_base_v4__Internal_capacity,
                "?_Internal_capacity@_Concurrent_vector_base_v4@details@Concurrency@@IEBA_KXZ");
        SET(p_vector_base_v4__Internal_push_back,
                "?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IEAAPEAX_KAEA_K@Z");
        SET(p_vector_base_v4__Internal_clear,
                "?_Internal_clear@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_KP6AXPEAX_K@Z@Z");
        SET(p_vector_base_v4__Internal_copy,
                "?_Internal_copy@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEBV123@_KP6AXPEAXPEBX1@Z@Z");
        SET(p_vector_base_v4__Internal_assign,
                "?_Internal_assign@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEBV123@_KP6AXPEAX1@ZP6AX2PEBX1@Z5@Z");
        SET(p_vector_base_v4__Internal_swap,
                "?_Internal_swap@_Concurrent_vector_base_v4@details@Concurrency@@IEAAXAEAV123@@Z");
        SET(p_vector_base_v4__Internal_compact,
                "?_Internal_compact@_Concurrent_vector_base_v4@details@Concurrency@@IEAAPEAX_KPEAXP6AX10@ZP6AX1PEBX0@Z@Z");
        SET(p_vector_base_v4__Internal_grow_by,
                "?_Internal_grow_by@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_K_K0P6AXPEAXPEBX0@Z2@Z");
        SET(p_vector_base_v4__Internal_grow_to_at_least_with_result,
                "?_Internal_grow_to_at_least_with_result@_Concurrent_vector_base_v4@details@Concurrency@@IEAA_K_K0P6AXPEAXPEBX0@Z2@Z");
        SET(p_vector_base_v4__Internal_reserve,
                "?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IEAAX_K00@Z");
        SET(p_vector_base_v4__Internal_resize,
                "?_Internal_resize@_Concurrent_vector_base_v4@details@Concurrency@@IEAAX_K00P6AXPEAX0@ZP6AX1PEBX0@Z3@Z");
        SET(p__Syserror_map,
                "?_Syserror_map@std@@YAPEBDH@Z");
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
        SET(p_tr2_sys__Last_write_time_wchar,
                "?_Last_write_time@sys@tr2@std@@YA_JPB_W@Z");
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
        SET(p_threads__Mtx_new,
                "?_Mtx_new@threads@stdext@@YAXAAPAX@Z");
        SET(p_threads__Mtx_delete,
                "?_Mtx_delete@threads@stdext@@YAXPAX@Z");
        SET(p_threads__Mtx_lock,
                "?_Mtx_lock@threads@stdext@@YAXPAX@Z");
        SET(p_threads__Mtx_unlock,
                "?_Mtx_unlock@threads@stdext@@YAXPAX@Z");
        SET(p_vector_base_v4__Segment_index_of,
                "?_Segment_index_of@_Concurrent_vector_base_v4@details@Concurrency@@KAII@Z");
        SET(p__Syserror_map,
                "?_Syserror_map@std@@YAPBDH@Z");
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
        SET(p_queue_base_v4_ctor,
                "??0_Concurrent_queue_base_v4@details@Concurrency@@IAE@I@Z");
        SET(p_queue_base_v4_dtor,
                "??1_Concurrent_queue_base_v4@details@Concurrency@@MAE@XZ");
        SET(p_queue_base_v4__Internal_empty,
                "?_Internal_empty@_Concurrent_queue_base_v4@details@Concurrency@@IBE_NXZ");
        SET(p_queue_base_v4__Internal_size,
                "?_Internal_size@_Concurrent_queue_base_v4@details@Concurrency@@IBEIXZ");
        SET(p_queue_base_v4__Internal_push,
                "?_Internal_push@_Concurrent_queue_base_v4@details@Concurrency@@IAEXPBX@Z");
        SET(p_queue_base_v4__Internal_move_push,
                "?_Internal_move_push@_Concurrent_queue_base_v4@details@Concurrency@@IAEXPAX@Z");
        SET(p_queue_base_v4__Internal_pop_if_present,
                "?_Internal_pop_if_present@_Concurrent_queue_base_v4@details@Concurrency@@IAE_NPAX@Z");
        SET(p_queue_base_v4__Internal_finish_clear,
                "?_Internal_finish_clear@_Concurrent_queue_base_v4@details@Concurrency@@IAEXXZ");
        SET(p_vector_base_v4_dtor,
                "??1_Concurrent_vector_base_v4@details@Concurrency@@IAE@XZ");
        SET(p_vector_base_v4__Internal_capacity,
                "?_Internal_capacity@_Concurrent_vector_base_v4@details@Concurrency@@IBEIXZ");
        SET(p_vector_base_v4__Internal_push_back,
                "?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IAEPAXIAAI@Z");
        SET(p_vector_base_v4__Internal_clear,
                "?_Internal_clear@_Concurrent_vector_base_v4@details@Concurrency@@IAEIP6AXPAXI@Z@Z");
        SET(p_vector_base_v4__Internal_copy,
                "?_Internal_copy@_Concurrent_vector_base_v4@details@Concurrency@@IAEXABV123@IP6AXPAXPBXI@Z@Z");
        SET(p_vector_base_v4__Internal_assign,
                "?_Internal_assign@_Concurrent_vector_base_v4@details@Concurrency@@IAEXABV123@IP6AXPAXI@ZP6AX1PBXI@Z4@Z");
        SET(p_vector_base_v4__Internal_swap,
                "?_Internal_swap@_Concurrent_vector_base_v4@details@Concurrency@@IAEXAAV123@@Z");
        SET(p_vector_base_v4__Internal_compact,
                "?_Internal_compact@_Concurrent_vector_base_v4@details@Concurrency@@IAEPAXIPAXP6AX0I@ZP6AX0PBXI@Z@Z");
        SET(p_vector_base_v4__Internal_grow_by,
                "?_Internal_grow_by@_Concurrent_vector_base_v4@details@Concurrency@@IAEIIIP6AXPAXPBXI@Z1@Z");
        SET(p_vector_base_v4__Internal_grow_to_at_least_with_result,
                "?_Internal_grow_to_at_least_with_result@_Concurrent_vector_base_v4@details@Concurrency@@IAEIIIP6AXPAXPBXI@Z1@Z");
        SET(p_vector_base_v4__Internal_reserve,
                "?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IAEXIII@Z");
        SET(p_vector_base_v4__Internal_resize,
                "?_Internal_resize@_Concurrent_vector_base_v4@details@Concurrency@@IAEXIIIP6AXPAXI@ZP6AX0PBXI@Z2@Z");
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
        SET(p_queue_base_v4_ctor,
                "??0_Concurrent_queue_base_v4@details@Concurrency@@IAA@I@Z");
        SET(p_queue_base_v4_dtor,
                "??1_Concurrent_queue_base_v4@details@Concurrency@@MAA@XZ");
        SET(p_queue_base_v4__Internal_empty,
                "?_Internal_empty@_Concurrent_queue_base_v4@details@Concurrency@@IBA_NXZ");
        SET(p_queue_base_v4__Internal_size,
                "?_Internal_size@_Concurrent_queue_base_v4@details@Concurrency@@IBAIXZ");
        SET(p_queue_base_v4__Internal_push,
                "?_Internal_push@_Concurrent_queue_base_v4@details@Concurrency@@IAAXPBX@Z");
        SET(p_queue_base_v4__Internal_move_push,
                "?_Internal_move_push@_Concurrent_queue_base_v4@details@Concurrency@@IAAXPAX@Z");
        SET(p_queue_base_v4__Internal_pop_if_present,
                "?_Internal_pop_if_present@_Concurrent_queue_base_v4@details@Concurrency@@IAA_NPAX@Z");
        SET(p_queue_base_v4__Internal_finish_clear,
                "?_Internal_finish_clear@_Concurrent_queue_base_v4@details@Concurrency@@IAAXXZ");
        SET(p_vector_base_v4_dtor,
                "??1_Concurrent_vector_base_v4@details@Concurrency@@IAA@XZ");
        SET(p_vector_base_v4__Internal_capacity,
                "?_Internal_capacity@_Concurrent_vector_base_v4@details@Concurrency@@IBAIXZ");
        SET(p_vector_base_v4__Internal_push_back,
                "?_Internal_push_back@_Concurrent_vector_base_v4@details@Concurrency@@IAAPAXIAAI@Z");
        SET(p_vector_base_v4__Internal_clear,
                "?_Internal_clear@_Concurrent_vector_base_v4@details@Concurrency@@IAAIP6AXPAXI@Z@Z");
        SET(p_vector_base_v4__Internal_copy,
                "?_Internal_copy@_Concurrent_vector_base_v4@details@Concurrency@@IAAXABV123@IP6AXPAXPBXI@Z@Z");
        SET(p_vector_base_v4__Internal_assign,
                "?_Internal_assign@_Concurrent_vector_base_v4@details@Concurrency@@IAAXABV123@IP6AXPAXI@ZP6AX1PBXI@Z4@Z");
        SET(p_vector_base_v4__Internal_swap,
                "?_Internal_swap@_Concurrent_vector_base_v4@details@Concurrency@@IAAXAAV123@@Z");
        SET(p_vector_base_v4__Internal_compact,
                "?_Internal_compact@_Concurrent_vector_base_v4@details@Concurrency@@IAAPAXIPAXP6AX0I@ZP6AX0PBXI@Z@Z");
        SET(p_vector_base_v4__Internal_grow_by,
                "?_Internal_grow_by@_Concurrent_vector_base_v4@details@Concurrency@@IAAIIIP6AXPAXPBXI@Z1@Z");
        SET(p_vector_base_v4__Internal_grow_to_at_least_with_result,
                "?_Internal_grow_to_at_least_with_result@_Concurrent_vector_base_v4@details@Concurrency@@IAAIIIP6AXPAXPBXI@Z1@Z");
        SET(p_vector_base_v4__Internal_reserve,
                "?_Internal_reserve@_Concurrent_vector_base_v4@details@Concurrency@@IAAXIII@Z");
        SET(p_vector_base_v4__Internal_resize,
                "?_Internal_resize@_Concurrent_vector_base_v4@details@Concurrency@@IAEXIIIP6AXPAXI@ZP6AX0PBXI@Z2@Z");
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
    SET(p__Mtx_trylock,
            "_Mtx_trylock");

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

    SET(p_byte_reverse_table, "?_Byte_reverse_table@details@Concurrency@@3QBEB");

    hdll = GetModuleHandleA("msvcr120.dll");
    p_setlocale = (void*)GetProcAddress(hdll, "setlocale");
    p__setmbcp = (void*)GetProcAddress(hdll, "_setmbcp");
    p__ismbblead = (void*)GetProcAddress(hdll, "_ismbblead");

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
        {1, 0, 0, 0, 0},
        {0, 1000000000, 0, 0, 0},
        {0x7FFFFFFF / 1000, 0, 0, 0, 0},
        {2147484, 0, 0, 0, 0}, /* ceil(0x80000000 / 1000) */
        {2147485, 0, 0, 0, 0}, /* ceil(0x80000000 / 1000) + 1*/
        {0, 0, 0x7FFFFFFF / 1000, 0, 2147483000},
        {0, 0, 0x7FFFFFFF / 1000, 647000000, 0x7FFFFFFF}, /* max */
        {0, 0, 0x7FFFFFFF / 1000, 647000001, -2147483648}, /* overflow. */
        {0, 0, 2147484, 0, -2147483296}, /* ceil(0x80000000 / 1000), overflow*/
        {0, 0, 0, -10000000, 0},
        {0, 0, -1, -100000000, 0},
        {-1, 0, 0, 0, 1000},
        {0, -100000000, 0, 0, 100},
        {-1, -100000000, 0, 0, 1100},
        {0, 0, -1, 2000000000, 1000},
        {0, 0, -2, 2000000000, 0},
        {0, 0, -2, 2100000000, 100},
        {0, 0, _I64_MAX / 1000, 0, -808}, /* Still fits in a signed 64 bit number */
        {0, 0, _I64_MAX / 1000, 1000000000, 192}, /* Overflows a signed 64 bit number */
        {0, 0, (((ULONGLONG)0x80000000 << 32) | 0x1000) / 1000, 1000000000, 4192}, /* 64 bit overflow */
        {_I64_MAX - 2, 0, _I64_MAX, 0, 2000}, /* Not an overflow */
        {_I64_MAX, 0, _I64_MAX - 2, 0, 0}, /* Not an overflow */

        /* October 11th 2017, 12:34:59 UTC */
        {1507725144, 983274000, 0, 0, 0},
        {0, 0, 1507725144, 983274000, 191624088},
        {1507725144, 983274000, 1507725145, 983274000, 1000},
        {1507725145, 983274000, 1507725145, 983274000, 0},
    };
    int i;
    MSVCRT_long ret;
    xtime t1, t2;

    for(i = 0; i < ARRAY_SIZE(tests); ++ i)
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

    for(i = 0; i < ARRAY_SIZE(tests); i ++)
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
    before.sec = 0xdeadbeef; before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 0);
    ok(i == 0, "expect xtime_get() to return 0, got: %d\n", i);
    ok(before.sec == 0xdeadbeef && before.nsec == 0xdeadbeef,
            "xtime_get() shouldn't have modified the xtime struct with the given option\n");

    before.sec = 0xdeadbeef; before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 1);
    ok(i == 1, "expect xtime_get() to return 1, got: %d\n", i);
    ok(before.sec != 0xdeadbeef && before.nsec != 0xdeadbeef,
            "xtime_get() should have modified the xtime struct with the given option\n");
}

static void test__Getcvt(void)
{
    _Cvtvec cvtvec;
    int i;

    cvtvec = p__Getcvt();
    ok(cvtvec.page == 0, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 1, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    todo_wine ok(cvtvec.unk == 1, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    if(!p_setlocale(LC_ALL, ".936")) {
        win_skip("_Getcvt tests\n");
        return;
    }
    cvtvec = p__Getcvt();
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++) {
        BYTE b = 0;
        int j;

        for(j=0; j<8; j++)
            b |= (p__ismbblead(i*8+j) ? 1 : 0) << j;
        ok(cvtvec.isleadbyte[i] ==b, "cvtvec.isleadbyte[%d] = %x (%x)\n", i, cvtvec.isleadbyte[i], b);
    }

    p__setmbcp(936);
    cvtvec = p__Getcvt();
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++) {
        BYTE b = 0;
        int j;

        for(j=0; j<8; j++)
            b |= (p__ismbblead(i*8+j) ? 1 : 0) << j;
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

static void test__Syserror_map(void)
{
    const char *r;

    r = p__Syserror_map(0);
    ok(!r, "_Syserror_map(0) returned %p\n", r);
}

static void test_tr2_sys__File_size(void)
{
    ULONGLONG val;
    HANDLE file;
    LARGE_INTEGER file_size;
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f1");
    ok(val == 7, "file_size is %s\n", wine_dbgstr_longlong(val));
    val = p_tr2_sys__File_size_wchar(L"tr2_test_dir/f1");
    ok(val == 7, "file_size is %s\n", wine_dbgstr_longlong(val));

    file = CreateFileA("tr2_test_dir/f2", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    val = p_tr2_sys__File_size("tr2_test_dir/f2");
    ok(val == 0, "file_size is %s\n", wine_dbgstr_longlong(val));

    val = p_tr2_sys__File_size("tr2_test_dir");
    ok(val == 0, "file_size is %s\n", wine_dbgstr_longlong(val));

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size("tr2_test_dir/not_exists_file");
    ok(val == 0, "file_size is %s\n", wine_dbgstr_longlong(val));
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    val = p_tr2_sys__File_size(NULL);
    ok(val == 0, "file_size is %s\n", wine_dbgstr_longlong(val));
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

    GetCurrentDirectoryA(MAX_PATH, current_path);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    CreateDirectoryA("tr2_test_dir", NULL);

    file = CreateFileA("tr2_test_dir/f1", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);
    file = CreateFileA("tr2_test_dir/f2", 0, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = 0xdeadbeef;
        val = p_tr2_sys__Equivalent(tests[i].path1, tests[i].path2);
        ok(tests[i].equivalent == val, "tr2_sys__Equivalent(): test %d expect: %d, got %d\n", i+1, tests[i].equivalent, val);
        ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    }

    val = p_tr2_sys__Equivalent_wchar(L"tr2_test_dir/f1", L"tr2_test_dir/f1");
    ok(val == 1, "tr2_sys__Equivalent(): expect: 1, got %d\n", val);
    val = p_tr2_sys__Equivalent_wchar(L"tr2_test_dir/f1", L"tr2_test_dir/f2");
    ok(val == 0, "tr2_sys__Equivalent(): expect: 0, got %d\n", val);
    val = p_tr2_sys__Equivalent_wchar(L"tr2_test_dir", L"tr2_test_dir");
    ok(val == -1, "tr2_sys__Equivalent(): expect: -1, got %d\n", val);

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

    GetCurrentDirectoryA(MAX_PATH, origin_path);
    GetTempPathA(MAX_PATH, temp_path);

    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    strcat(temp, "\\");
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    GetTempPathW(MAX_PATH, temp_path_wchar);
    ok(SetCurrentDirectoryW(temp_path_wchar), "SetCurrentDirectoryW to temp_path_wchar failed\n");
    temp_wchar = p_tr2_sys__Current_get_wchar(current_path_wchar);
    ok(temp_wchar == current_path_wchar, "p_tr2_sys__Current_get_wchar returned different buffer\n");
    wcscat(temp_wchar, L"\\");
    ok(!wcscmp(temp_path_wchar, current_path_wchar), "test_tr2_sys__Current_get(): expect: %s, got %s\n", wine_dbgstr_w(temp_path_wchar), wine_dbgstr_w(current_path_wchar));

    ok(SetCurrentDirectoryA(origin_path), "SetCurrentDirectoryA to origin_path failed\n");
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Current_set(void)
{
    char temp_path[MAX_PATH], current_path[MAX_PATH], origin_path[MAX_PATH];
    char *temp;

    GetTempPathA(MAX_PATH, temp_path);
    GetCurrentDirectoryA(MAX_PATH, origin_path);
    temp = p_tr2_sys__Current_get(origin_path);
    ok(temp == origin_path, "p_tr2_sys__Current_get returned different buffer\n");

    ok(p_tr2_sys__Current_set(temp_path), "p_tr2_sys__Current_set to temp_path failed\n");
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    strcat(temp, "\\");
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    ok(p_tr2_sys__Current_set_wchar(L"./"), "p_tr2_sys__Current_set_wchar to temp_path failed\n");
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    strcat(temp, "\\");
    ok(!strcmp(temp_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", temp_path, current_path);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("not_exisist_dir"), "p_tr2_sys__Current_set to not_exist_dir succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ok(!p_tr2_sys__Current_set("??invalid_name>>"), "p_tr2_sys__Current_set to ??invalid_name>> succeed\n");
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(p_tr2_sys__Current_set(origin_path), "p_tr2_sys__Current_set to origin_path failed\n");
    temp = p_tr2_sys__Current_get(current_path);
    ok(temp == current_path, "p_tr2_sys__Current_get returned different buffer\n");
    ok(!strcmp(origin_path, current_path), "test_tr2_sys__Current_get(): expect: %s, got %s\n", origin_path, current_path);
}

static void test_tr2_sys__Make_dir(void)
{
    int ret, i;
    struct {
        char const *path;
        int val;
    } tests[] = {
        { "tr2_test_dir", 1 },
        { "tr2_test_dir", 0 },
        { NULL, -1 },
        { "??invalid_name>>", -1 }
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Make_dir(tests[i].path);
        ok(ret == tests[i].val, "tr2_sys__Make_dir(): test %d expect: %d, got %d\n", i+1, tests[i].val, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Make_dir(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
    }
    ret = p_tr2_sys__Make_dir_wchar(L"wd");
    ok(ret == 1, "tr2_sys__Make_dir(): expect: 1, got %d\n", ret);

    ok(p_tr2_sys__Remove_dir("tr2_test_dir"), "expect tr2_test_dir to exist\n");
    ok(p_tr2_sys__Remove_dir_wchar(L"wd"), "expect wd to exist\n");
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

    for(i=0; i<ARRAY_SIZE(tests); i++) {
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
    struct {
        char const *source;
        char const *dest;
        MSVCP_bool fail_if_exists;
        int last_error;
        int last_error2;
    } tests[] = {
        { "f1", "f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_SUCCESS, ERROR_SUCCESS },
        { "f1", "tr2_test_dir\\f1_copy", TRUE, ERROR_FILE_EXISTS, ERROR_FILE_EXISTS },
        { "f1", "tr2_test_dir\\f1_copy", FALSE, ERROR_SUCCESS, ERROR_SUCCESS },
        { "tr2_test_dir", "f1", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED },
        { "tr2_test_dir", "tr2_test_dir_copy", TRUE, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED },
        { NULL, "f1", TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER },
        { "f1", NULL, TRUE, ERROR_INVALID_PARAMETER, ERROR_INVALID_PARAMETER },
        { "not_exist", "tr2_test_dir", TRUE, ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND },
        { "f1", "not_exist_dir\\f1_copy", TRUE, ERROR_PATH_NOT_FOUND, ERROR_FILE_NOT_FOUND },
        { "f1", "tr2_test_dir", TRUE, ERROR_ACCESS_DENIED, ERROR_FILE_EXISTS }
    };

    ret = p_tr2_sys__Make_dir("tr2_test_dir");
    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    file_size.QuadPart = 7;
    ok(SetFilePointerEx(file, file_size, NULL, FILE_BEGIN), "SetFilePointerEx failed\n");
    ok(SetEndOfFile(file), "SetEndOfFile failed\n");
    CloseHandle(file);

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Copy_file(tests[i].source, tests[i].dest, tests[i].fail_if_exists);
        ok(ret == tests[i].last_error || ret == tests[i].last_error2,
                "test_tr2_sys__Copy_file(): test %d expect: %d, got %d\n", i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Copy_file(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].source) == p_tr2_sys__File_size(tests[i].dest),
                    "test_tr2_sys__Copy_file(): test %d failed, mismatched file sizes\n", i+1);
    }
    ret = p_tr2_sys__Copy_file_wchar(L"f1", L"fw", TRUE);
    ok(ret == ERROR_SUCCESS, "test_tr2_sys__Copy_file_wchar() expect ERROR_SUCCESS, got %d\n", ret);

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    ok(DeleteFileW(L"fw"), "expect fw to exist\n");
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
    static const struct {
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
    static const struct {
        const WCHAR *old_path;
        const WCHAR *new_path;
        int val;
    } testsW[] = {
        { L"tr2_test_dir/f1", L"tr2_test_dir\\f1_rename", ERROR_SUCCESS },
        { L"tr2_test_dir/f1", NULL, ERROR_FILE_NOT_FOUND }, /* Differs from the A version */
        { L"tr2_test_dir/f1", L"tr2_test_dir\\f1_rename", ERROR_FILE_NOT_FOUND },
        { NULL, L"tr2_test_dir\\f1_rename2", ERROR_PATH_NOT_FOUND }, /* Differs from the A version */
        { L"tr2_test_dir\\f1_rename", L"tr2_test_dir\\??invalid>", ERROR_INVALID_NAME },
        { L"tr2_test_dir\\not_exist", L"tr2_test_dir\\not_exist2", ERROR_FILE_NOT_FOUND },
        { L"tr2_test_dir\\not_exist", L"tr2_test_dir\\??invalid>", ERROR_FILE_NOT_FOUND }
    };

    GetCurrentDirectoryA(MAX_PATH, current_path);
    GetTempPathA(MAX_PATH, temp_path);
    ok(SetCurrentDirectoryA(temp_path), "SetCurrentDirectoryA to temp_path failed\n");
    ret = p_tr2_sys__Make_dir("tr2_test_dir");

    ok(ret == 1, "test_tr2_sys__Make_dir(): expect 1 got %d\n", ret);
    file = CreateFileA("tr2_test_dir\\f1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
    CloseHandle(file);

    ret = p_tr2_sys__Rename("tr2_test_dir\\f1", "tr2_test_dir\\f1");
    ok(ERROR_SUCCESS == ret, "test_tr2_sys__Rename(): expect: ERROR_SUCCESS, got %d\n", ret);
    for(i=0; i<ARRAY_SIZE(tests); i++) {
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
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1") == 7, "test_tr2_sys__Rename(): expect: 7, got %s\n", wine_dbgstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1")));
    ok(p_tr2_sys__File_size("tr2_test_dir\\f1_rename") == 0, "test_tr2_sys__Rename(): expect: 0, got %s\n",wine_dbgstr_longlong(p_tr2_sys__File_size("tr2_test_dir\\f1_rename")));

    ok(DeleteFileA("tr2_test_dir\\f1_rename"), "expect f1_rename to exist\n");

    for(i=0; i<ARRAY_SIZE(testsW); i++) {
        errno = 0xdeadbeef;
        if(testsW[i].val == ERROR_SUCCESS) {
            h1 = CreateFileW(testsW[i].old_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h1 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h1, &info1), "GetFileInformationByHandle failed\n");
            CloseHandle(h1);
        }
        SetLastError(0xdeadbeef);
        ret = p_tr2_sys__Rename_wchar(testsW[i].old_path, testsW[i].new_path);
        ok(ret == testsW[i].val, "test_tr2_sys__Rename_wchar(): test %d expect: %d, got %d\n", i+1, testsW[i].val, ret);
        ok(errno == 0xdeadbeef, "test_tr2_sys__Rename_wchar(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS) {
            h2 = CreateFileW(testsW[i].new_path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, 0);
            ok(h2 != INVALID_HANDLE_VALUE, "create file failed: INVALID_HANDLE_VALUE\n");
            ok(GetFileInformationByHandle(h2, &info2), "GetFileInformationByHandle failed\n");
            CloseHandle(h2);
            ok(info1.nFileIndexHigh == info2.nFileIndexHigh
                    && info1.nFileIndexLow == info2.nFileIndexLow,
                    "test_tr2_sys__Rename_wchar(): test %d expect two files equivalent\n", i+1);
        }
    }

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

    p_tr2_sys__Current_get(current_path);
    p_tr2_sys__Current_get_wchar(current_path_wchar);

    p_tr2_sys__Statvfs(&info, current_path);
    ok(info.capacity >= info.free, "test_tr2_sys__Statvfs(): info.capacity < info.free\n");
    ok(info.free >= info.available, "test_tr2_sys__Statvfs(): info.free < info.available\n");

    p_tr2_sys__Statvfs_wchar(&info, current_path_wchar);
    ok(info.capacity >= info.free, "tr2_sys__Statvfs_wchar(): info.capacity < info.free\n");
    ok(info.free >= info.available, "tr2_sys__Statvfs_wchar(): info.free < info.available\n");

    p_tr2_sys__Statvfs(&info, NULL);
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.free));

    p_tr2_sys__Statvfs(&info, "not_exist");
    ok(info.available == 0, "test_tr2_sys__Statvfs(): info.available expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.available));
    ok(info.capacity == 0, "test_tr2_sys__Statvfs(): info.capacity expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.capacity));
    ok(info.free == 0, "test_tr2_sys__Statvfs(): info.free expect: %d, got %s\n",
            0, wine_dbgstr_longlong(info.free));
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

    for(i=0; i<ARRAY_SIZE(tests); i++) {
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
    val = p_tr2_sys__Stat_wchar(L"tr2_test_dir", &err_code);
    ok(directory_file == val, "tr2_sys__Stat_wchar() expect directory_file, got %d\n", val);
    ok(ERROR_SUCCESS == err_code, "tr2_sys__Stat_wchar(): err_code expect ERROR_SUCCESS, got %d\n", err_code);
    err_code = 0xdeadbeef;
    val = p_tr2_sys__Lstat_wchar(L"tr2_test_dir/f1", &err_code);
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
    newtime = p_tr2_sys__Last_write_time_wchar(L"tr2_test_dir/f1");
    ok(last_write_time == newtime, "last_write_time = %s, newtime = %s\n",
            wine_dbgstr_longlong(last_write_time), wine_dbgstr_longlong(newtime));
    newtime = last_write_time + 123456789;
    p_tr2_sys__Last_write_time_set("tr2_test_dir/f1", newtime);
    todo_wine ok(last_write_time == p_tr2_sys__Last_write_time("tr2_test_dir/f1"),
            "last_write_time should have changed: %s\n",
            wine_dbgstr_longlong(last_write_time));

    errno = 0xdeadbeef;
    last_write_time = p_tr2_sys__Last_write_time("not_exist");
    ok(errno == 0xdeadbeef, "tr2_sys__Last_write_time(): errno expect 0xdeadbeef, got %d\n", errno);
    ok(last_write_time == 0, "expect 0 got %s\n", wine_dbgstr_longlong(last_write_time));
    last_write_time = p_tr2_sys__Last_write_time(NULL);
    ok(last_write_time == 0, "expect 0 got %s\n", wine_dbgstr_longlong(last_write_time));

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

    GetCurrentDirectoryA(MAX_PATH, longer_path);
    strcat(longer_path, "\\tr2_test_dir\\");
    while(lstrlenA(longer_path) < MAX_PATH-1)
        strcat(longer_path, "s");
    ok(lstrlenA(longer_path) == MAX_PATH-1, "tr2_sys__Open_dir(): expect MAX_PATH, got %d\n", lstrlenA(longer_path));
    memset(first_file_name, 0xff, MAX_PATH);
    type = err =  0xdeadbeef;
    result_handle = NULL;
    result_handle = p_tr2_sys__Open_dir(first_file_name, longer_path, &err, &type);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect NULL, got %p\n", result_handle);
    ok(!*first_file_name, "tr2_sys__Open_dir(): expect: 0, got %s\n", first_file_name);
    ok(err == ERROR_BAD_PATHNAME, "tr2_sys__Open_dir(): expect: ERROR_BAD_PATHNAME, got %d\n", err);
    ok((int)type == 0xdeadbeef, "tr2_sys__Open_dir(): expect 0xdeadbeef, got %d\n", type);

    memset(first_file_name, 0xff, MAX_PATH);
    memset(dest, 0, MAX_PATH);
    type = err = 0xdeadbeef;
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
    ok(type == status_unknown, "p_tr2_sys__Read_dir(): expect: status_unknown, got %d\n", type);
    p_tr2_sys__Close_dir(result_handle);
    ok(result_handle != NULL, "tr2_sys__Open_dir(): expect: not NULL, got %p\n", result_handle);
    ok(num_of_f1 == 1, "found f1 %d times\n", num_of_f1);
    ok(num_of_f2 == 1, "found f2 %d times\n", num_of_f2);
    ok(num_of_sub_dir == 1, "found sub_dir %d times\n", num_of_sub_dir);
    ok(num_of_other_files == 0, "found %d other files\n", num_of_other_files);

    memset(first_file_name, 0xff, MAX_PATH);
    type = err = 0xdeadbeef;
    result_handle = file;
    result_handle = p_tr2_sys__Open_dir(first_file_name, "not_exist", &err, &type);
    ok(result_handle == NULL, "tr2_sys__Open_dir(): expect: NULL, got %p\n", result_handle);
    ok(err == ERROR_BAD_PATHNAME, "tr2_sys__Open_dir(): expect: ERROR_BAD_PATHNAME, got %d\n", err);
    ok((int)type == 0xdeadbeef, "tr2_sys__Open_dir(): expect: 0xdeadbeef, got %d\n", type);
    ok(!*first_file_name, "tr2_sys__Open_dir(): expect: 0, got %s\n", first_file_name);

    CreateDirectoryA("empty_dir", NULL);
    memset(first_file_name, 0xff, MAX_PATH);
    type = err = 0xdeadbeef;
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

    GetCurrentDirectoryA(MAX_PATH, current_path);
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

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = 0xdeadbeef;
        ret = p_tr2_sys__Link(tests[i].existing_path, tests[i].new_path);
        ok(ret == tests[i].last_error, "tr2_sys__Link(): test %d expect: %d, got %d\n",
                i+1, tests[i].last_error, ret);
        ok(errno == 0xdeadbeef, "tr2_sys__Link(): test %d errno expect 0xdeadbeef, got %d\n", i+1, errno);
        if(ret == ERROR_SUCCESS)
            ok(p_tr2_sys__File_size(tests[i].existing_path) == p_tr2_sys__File_size(tests[i].new_path),
                    "tr2_sys__Link(): test %d failed, mismatched file sizes\n", i+1);
    }

    ok(DeleteFileA("f1"), "expect f1 to exist\n");
    ok(p_tr2_sys__File_size("f1_link") == p_tr2_sys__File_size("tr2_test_dir/f1_link") &&
            p_tr2_sys__File_size("tr2_test_dir/f1_link") == p_tr2_sys__File_size("tr2_test_dir/f1_link_link"),
            "tr2_sys__Link(): expect links' size are equal, got %s\n", wine_dbgstr_longlong(p_tr2_sys__File_size("tr2_test_dir/f1_link_link")));
    ok(p_tr2_sys__File_size("f1_link") == 7, "tr2_sys__Link(): expect f1_link's size equals to 7, got %s\n", wine_dbgstr_longlong(p_tr2_sys__File_size("f1_link")));

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
    ok(p_tr2_sys__File_size("f1_link") == 20, "tr2_sys__Link(): expect f1_link's size equals to 20, got %s\n", wine_dbgstr_longlong(p_tr2_sys__File_size("f1_link")));

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

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        errno = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = p_tr2_sys__Symlink(tests[i].existing_path, tests[i].new_path);
        if(!i && (ret==ERROR_PRIVILEGE_NOT_HELD || ret==ERROR_INVALID_FUNCTION || ret==ERROR_CALL_NOT_IMPLEMENTED)) {
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
            ok(p_tr2_sys__File_size(tests[i].new_path) == 0, "tr2_sys__Symlink(): expect 0, got %s\n", wine_dbgstr_longlong(p_tr2_sys__File_size(tests[i].new_path)));
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
    if(ret==ERROR_PRIVILEGE_NOT_HELD || ret==ERROR_INVALID_FUNCTION || ret==ERROR_CALL_NOT_IMPLEMENTED) {
        tests[0].last_error = ERROR_FILE_NOT_FOUND;
        win_skip("Privilege not held or symbolic link not supported, skipping symbolic link tests.\n");
    }else {
        ok(ret == ERROR_SUCCESS, "tr2_sys__Symlink(): expect: ERROR_SUCCESS, got %d\n", ret);
    }
    ret = p_tr2_sys__Link("tr2_test_dir/f1", "tr2_test_dir/f1_link");
    ok(ret == ERROR_SUCCESS, "tr2_sys__Link(): expect: ERROR_SUCCESS, got %d\n", ret);

    for(i=0; i<ARRAY_SIZE(tests); i++) {
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
    for(i=0; i<ARRAY_SIZE(testeq); i++) {
        ret = p__Thrd_equal(testeq[i].a, testeq[i].b);
        ok(ret == testeq[i].r, "(%p %lu) = (%p %lu) expected %d, got %d\n",
            testeq[i].a.hnd, testeq[i].a.id, testeq[i].b.hnd, testeq[i].b.id, testeq[i].r, ret);
    }

    /* test for less than */
    for(i=0; i<ARRAY_SIZE(testlt); i++) {
        ret = p__Thrd_lt(testlt[i].a, testlt[i].b);
        ok(ret == testlt[i].r, "(%p %lu) < (%p %lu) expected %d, got %d\n",
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
    ok(ta.id == tb.id, "got a %ld b %ld\n", ta.id, tb.id);
    ok(ta.id == GetCurrentThreadId(), "expected %ld, got %ld\n", GetCurrentThreadId(), ta.id);
    /* the handles can be different if new threads are created at same time */
    ok(ta.hnd != NULL, "handle a is NULL\n");
    ok(tb.hnd != NULL, "handle b is NULL\n");
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
    ok(ta.id == tb.id, "expected %ld, got %ld\n", ta.id, tb.id);
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
    LONG started;
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
    ok(mtx->count == 1, "mtx.count = %ld\n", mtx->count);

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
    ok(ret == WAIT_TIMEOUT, "WiatForSingleObject returned %lx\n", ret);
    ok(!pad.mtx->count, "pad.mtx.count = %ld\n", pad.mtx->count);
    ok(!pad.launched, "pad.launched = %x\n", pad.launched);
    call_func1(p__Pad__Release, &pad);
    ok(pad.launched, "pad.launched = %x\n", pad.launched);
    ret = WaitForSingleObject(_Pad__Launch_returned, 100);
    ok(ret == WAIT_OBJECT_0, "WiatForSingleObject returned %lx\n", ret);
    ok(pad.mtx->count == 1, "pad.mtx.count = %ld\n", pad.mtx->count);
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
    ok(pad.mtx->count == 1, "pad.mtx.count = %ld\n", pad.mtx->count);

    pad.vtable = &pfunc;
    call_func2(p__Pad__Launch, &pad, &thrd);
    SetEvent(_Pad__Launch_returned);
    ok(!p__Thrd_join(thrd, NULL), "_Thrd_join failed\n");

    call_func1(p__Pad_dtor, &pad);
    CloseHandle(_Pad__Launch_returned);
}

static void test__Mtx(void)
{
#
    static int flags[] =
    {
        0, MTX_PLAIN, MTX_TRY, MTX_TIMED, MTX_RECURSIVE,
        MTX_PLAIN|MTX_TRY, MTX_PLAIN|MTX_RECURSIVE, MTX_PLAIN|0xbeef
    };
    _Mtx_t mtx;
    int i, r, expect;

    for (i=0; i<ARRAY_SIZE(flags); i++)
    {
        if (flags[i] == MTX_PLAIN || flags[i] & MTX_RECURSIVE)
            expect = 0;
        else
            expect = 3;

        r = p__Mtx_init(&mtx, flags[i]);
        ok(!r, "failed to init mtx (flags %x)\n", flags[i]);
        ok(mtx->thread_id == -1, "mtx.thread_id = %lx (flags %x)\n", mtx->thread_id, flags[i]);
        ok(mtx->count == 0, "mtx.count = %lu (flags %x)\n", mtx->count, flags[i]);

        r = p__Mtx_trylock(&mtx);
        ok(!r, "_Mtx_trylock returned %x (flags %x)\n", r, flags[i]);
        ok(mtx->thread_id == GetCurrentThreadId(), "mtx.thread_id = %lx (flags %x)\n", mtx->thread_id, flags[i]);
        ok(mtx->count == 1, "mtx.count = %lu (flags %x)\n", mtx->count, flags[i]);
        r = p__Mtx_trylock(&mtx);
        ok(r == expect, "_Mtx_trylock returned %x (flags %x)\n", r, flags[i]);
        ok(mtx->thread_id == GetCurrentThreadId(), "mtx.thread_id = %lx (flags %x)\n", mtx->thread_id, flags[i]);
        ok(mtx->count == r ? 1 : 2, "mtx.count = %lu, expected %u (flags %x)\n", mtx->count, r ? 1 : 2, flags[i]);
        if(!r) p__Mtx_unlock(&mtx);

        r = p__Mtx_lock(&mtx);
        ok(r == expect, "_Mtx_lock returned %x (flags %x)\n", r, flags[i]);
        ok(mtx->thread_id == GetCurrentThreadId(), "mtx.thread_id = %lx (flags %x)\n", mtx->thread_id, flags[i]);
        ok(mtx->count == r ? 1 : 2, "mtx.count = %lu, expected %u (flags %x)\n", mtx->count, r ? 1 : 2, flags[i]);
        if(!r) p__Mtx_unlock(&mtx);

        p__Mtx_unlock(&mtx);
        ok(mtx->thread_id == -1, "mtx.thread_id = %lx (flags %x)\n", mtx->thread_id, flags[i]);
        ok(mtx->count == 0, "mtx.count = %lu (flags %x)\n", mtx->count, flags[i]);
        p__Mtx_destroy(&mtx);
    }
}

static void test_threads__Mtx(void)
{
    void *mtx = NULL;

    p_threads__Mtx_new(&mtx);
    ok(mtx != NULL, "mtx == NULL\n");

    p_threads__Mtx_lock(mtx);
    p_threads__Mtx_lock(mtx);
    p_threads__Mtx_unlock(mtx);
    p_threads__Mtx_unlock(mtx);
    p_threads__Mtx_unlock(mtx);

    p_threads__Mtx_delete(mtx);
}

static void test_vector_base_v4__Segment_index_of(void)
{
    size_t i;
    size_t ret;
    struct {
        size_t x;
        size_t expect;
    } tests[] = {
        {0, 0},
        {1, 0},
        {2, 1},
        {3, 1},
        {4, 2},
        {7, 2},
        {8, 3},
        {15, 3},
        {16, 4},
        {31, 4},
        {32, 5},
        {~0, 8*sizeof(void*)-1}
    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        ret = p_vector_base_v4__Segment_index_of(tests[i].x);
        ok(ret == tests[i].expect, "expected %ld, got %ld for %ld\n",
            (long)tests[i].expect, (long)ret, (long)tests[i].x);
    }
}

static HANDLE block_start, block_end;
static BOOL block;

static void __thiscall queue_char__Move_item(
#ifndef __i386__
        queue_base_v4 *this,
#endif
        _Page *dst, size_t idx, void *src)
{
    CHECK_EXPECT(queue_char__Move_item);
    if(block) {
        block = FALSE;
        SetEvent(block_start);
        WaitForSingleObject(block_end, INFINITE);
    }
    memcpy(dst->data + idx, src, sizeof(char));
}

static void __thiscall queue_char__Copy_item(
#ifndef __i386__
        queue_base_v4 *this,
#endif
        _Page *dst, size_t idx, const void *src)
{
    CHECK_EXPECT(queue_char__Copy_item);
    if(block) {
        block = FALSE;
        SetEvent(block_start);
        WaitForSingleObject(block_end, INFINITE);
    }
    memcpy(dst->data + idx, src, sizeof(char));
}

static void __thiscall queue_char__Assign_and_destroy_item(
#ifndef __i386__
        queue_base_v4 *this,
#endif
        void *dst, _Page *src, size_t idx)
{
    CHECK_EXPECT(queue_char__Assign_and_destroy_item);
    if(block) {
        block = FALSE;
        SetEvent(block_start);
        WaitForSingleObject(block_end, INFINITE);
    }
    memcpy(dst, src->data + idx, sizeof(char));
}

#ifndef __i386__
static _Page* __thiscall queue_char__Allocate_page(queue_base_v4 *this)
#else
static _Page* __thiscall queue_char__Allocate_page(void)
#endif
{
    CHECK_EXPECT(queue_char__Allocate_page);
    return malloc(sizeof(_Page) + sizeof(char[256]));
}

static void __thiscall queue_char__Deallocate_page(
#ifndef __i386__
        queue_base_v4 *this,
#endif
        _Page *page)
{
    CHECK_EXPECT2(queue_char__Deallocate_page);
    free(page);
}

static const void* queue_char_vtbl[] =
{
    queue_char__Move_item,
    queue_char__Copy_item,
    queue_char__Assign_and_destroy_item,
    NULL, /* dtor */
    queue_char__Allocate_page,
    queue_char__Deallocate_page
};

static DWORD WINAPI queue_move_push_thread(void *arg)
{
    queue_base_v4 *queue = arg;
    char c = 'm';

    alloc_expect_struct();
    SET_EXPECT(queue_char__Allocate_page); /* ignore page allocations */
    SET_EXPECT(queue_char__Move_item);
    call_func2(p_queue_base_v4__Internal_move_push, queue, &c);
    CHECK_CALLED(queue_char__Move_item);
    free_expect_struct();
    return 0;
}

static DWORD WINAPI queue_push_thread(void *arg)
{
    queue_base_v4 *queue = arg;
    char c = 'c';

    alloc_expect_struct();
    SET_EXPECT(queue_char__Copy_item);
    call_func2(p_queue_base_v4__Internal_push, queue, &c);
    CHECK_CALLED(queue_char__Copy_item);
    free_expect_struct();
    return 0;
}

static DWORD WINAPI queue_pop_thread(void*arg)
{
    queue_base_v4 *queue = arg;
    char c;

    alloc_expect_struct();
    SET_EXPECT(queue_char__Assign_and_destroy_item);
    call_func2(p_queue_base_v4__Internal_pop_if_present, queue, &c);
    CHECK_CALLED(queue_char__Assign_and_destroy_item);
    free_expect_struct();
    return 0;
}

static void* __cdecl concurrent_vector_int_alloc(vector_base_v4 *this, size_t n)
{
    CHECK_EXPECT(concurrent_vector_int_alloc);
    vector_alloc_count++;
    return malloc(n*sizeof(int));
}

static void __cdecl concurrent_vector_int_destroy(void *ptr, size_t n)
{
    CHECK_EXPECT2(concurrent_vector_int_destroy);
    ok(vector_elem_count >= n, "invalid destroy\n");
    vector_elem_count -= n;
    memset(ptr, 0xff, sizeof(int)*n);
}

static void concurrent_vector_int_ctor(vector_base_v4 *this)
{
    memset(this, 0, sizeof(*this));
    this->allocator = concurrent_vector_int_alloc;
    this->segment = &this->storage[0];
}

static void concurrent_vector_int_dtor(vector_base_v4 *this)
{
    size_t blocks;

    blocks = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            this, concurrent_vector_int_destroy);
    for(blocks--; blocks >= this->first_block; blocks--) {
        vector_alloc_count--;
        free(this->segment[blocks]);
    }

    if(this->first_block) {
        vector_alloc_count--;
        free(this->segment[0]);
    }

    call_func1(p_vector_base_v4_dtor, this);
}

static void __cdecl concurrent_vector_int_copy(void *dst, const void *src, size_t n)
{
    CHECK_EXPECT2(concurrent_vector_int_copy);
    vector_elem_count += n;
    memcpy(dst, src, n*sizeof(int));
}

static void __cdecl concurrent_vector_int_assign(void *dst, const void *src, size_t n)
{
    CHECK_EXPECT2(concurrent_vector_int_assign);
    memcpy(dst, src, n*sizeof(int));
}

static void test_queue_base_v4(void)
{
    queue_base_v4 queue;
    HANDLE thread[2];
    MSVCP_bool b;
    size_t size;
    DWORD ret;
    int i;
    char c;

    block_start = CreateEventW(NULL, FALSE, FALSE, NULL);
    block_end = CreateEventW(NULL, FALSE, FALSE, NULL);

    call_func2(p_queue_base_v4_ctor, &queue, 0);
    ok(queue.data != NULL, "queue.data = NULL\n");
    ok(queue.alloc_count == 32, "queue.alloc_count = %ld\n", (long)queue.alloc_count);
    ok(queue.item_size == 0, "queue.item_size = %ld\n", (long)queue.item_size);
    call_func1(p_queue_base_v4_dtor, &queue);

    call_func2(p_queue_base_v4_ctor, &queue, 8);
    ok(queue.data != NULL, "queue.data = NULL\n");
    ok(queue.alloc_count == 32, "queue.alloc_count = %ld\n", (long)queue.alloc_count);
    ok(queue.item_size == 8, "queue.item_size = %ld\n", (long)queue.item_size);
    call_func1(p_queue_base_v4_dtor, &queue);

    call_func2(p_queue_base_v4_ctor, &queue, 16);
    ok(queue.data != NULL, "queue.data = NULL\n");
    ok(queue.alloc_count == 16, "queue.alloc_count = %ld\n", (long)queue.alloc_count);
    ok(queue.item_size == 16, "queue.item_size = %ld\n", (long)queue.item_size);
    b = (DWORD_PTR)call_func1(p_queue_base_v4__Internal_empty, &queue);
    ok(b, "queue is not empty\n");
    size = (size_t)call_func1(p_queue_base_v4__Internal_size, &queue);
    ok(!size, "size = %ld\n", (long)size);
    call_func1(p_queue_base_v4_dtor, &queue);

    call_func2(p_queue_base_v4_ctor, &queue, 1);
    queue.vtable = (void*)&queue_char_vtbl;

    for(i=0; i<8; i++) {
        SET_EXPECT(queue_char__Allocate_page);
        SET_EXPECT(queue_char__Copy_item);
        c = 'a'+i;
        call_func2(p_queue_base_v4__Internal_push, &queue, &c);
        CHECK_CALLED(queue_char__Allocate_page);
        CHECK_CALLED(queue_char__Copy_item);

        b = (MSVCP_bool)(DWORD_PTR)call_func1(p_queue_base_v4__Internal_empty, &queue);
        ok(!b, "queue is empty\n");
        size = (size_t)call_func1(p_queue_base_v4__Internal_size, &queue);
        ok(size == i+1, "size = %ld, expected %ld\n", (long)size, (long)i);
    }

    SET_EXPECT(queue_char__Copy_item);
    c = 'a'+i;
    call_func2(p_queue_base_v4__Internal_push, &queue, &c);
    CHECK_CALLED(queue_char__Copy_item);

    for(i=0; i<9; i++) {
        SET_EXPECT(queue_char__Assign_and_destroy_item);
        b = (DWORD_PTR)call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
        CHECK_CALLED(queue_char__Assign_and_destroy_item);
        ok(b, "pop returned false\n");
        ok(c == 'a'+i, "got '%c', expected '%c'\n", c, 'a'+i);
    }
    b = (DWORD_PTR)call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
    ok(!b, "pop returned true\n");

    for(i=0; i<247; i++) {
        SET_EXPECT(queue_char__Copy_item);
        c = 'a'+i;
        call_func2(p_queue_base_v4__Internal_push, &queue, &c);
        CHECK_CALLED(queue_char__Copy_item);

        size = (size_t)call_func1(p_queue_base_v4__Internal_size, &queue);
        ok(size == i+1, "size = %ld, expected %ld\n", (long)size, (long)i);
    }

    SET_EXPECT(queue_char__Allocate_page);
    SET_EXPECT(queue_char__Copy_item);
    c = 'a'+i;
    call_func2(p_queue_base_v4__Internal_push, &queue, &c);
    CHECK_CALLED(queue_char__Allocate_page);
    CHECK_CALLED(queue_char__Copy_item);

    for(i=0; i<239; i++) {
        SET_EXPECT(queue_char__Assign_and_destroy_item);
        b = (DWORD_PTR)call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
        CHECK_CALLED(queue_char__Assign_and_destroy_item);
        ok(b, "pop returned false\n");
        ok(c == (char)('a'+i), "got '%c', expected '%c'\n", c, 'a'+i);
    }

    SET_EXPECT(queue_char__Assign_and_destroy_item);
    SET_EXPECT(queue_char__Deallocate_page);
    b = (DWORD_PTR)call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
    CHECK_CALLED(queue_char__Assign_and_destroy_item);
    CHECK_CALLED(queue_char__Deallocate_page);
    ok(b, "pop returned false\n");
    ok(c == (char)('a'+i), "got '%c', expected '%c'\n", c, 'a'+i);

    /* destructor doesn't clear the memory, _Internal_finish_clear needs to be called */
    SET_EXPECT(queue_char__Deallocate_page);
    call_func1(p_queue_base_v4__Internal_finish_clear, &queue);
    CHECK_CALLED(queue_char__Deallocate_page);

    call_func1(p_queue_base_v4_dtor, &queue);

    /* test parallel push */
    call_func2(p_queue_base_v4_ctor, &queue, 1);
    queue.vtable = (void*)&queue_char_vtbl;

    block = TRUE;
    thread[0] = CreateThread(NULL, 0, queue_move_push_thread, &queue, 0, NULL);
    WaitForSingleObject(block_start, INFINITE);

    c = 'a';
    for(i=0; i<7; i++) {
        SET_EXPECT(queue_char__Allocate_page);
        SET_EXPECT(queue_char__Copy_item);
        call_func2(p_queue_base_v4__Internal_push, &queue, &c);
        CHECK_CALLED(queue_char__Allocate_page);
        CHECK_CALLED(queue_char__Copy_item);
    }

    thread[1] = CreateThread(NULL, 0, queue_push_thread, &queue, 0, NULL);
    ret = WaitForSingleObject(thread[1], 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", ret);

    SetEvent(block_end);
    WaitForSingleObject(thread[0], INFINITE);
    WaitForSingleObject(thread[1], INFINITE);
    CloseHandle(thread[0]);
    CloseHandle(thread[1]);

    /* test parallel pop */
    block = TRUE;
    thread[0] = CreateThread(NULL, 0, queue_pop_thread, &queue, 0, NULL);
    WaitForSingleObject(block_start, INFINITE);

    for(i=0; i<7; i++) {
        SET_EXPECT(queue_char__Assign_and_destroy_item);
        b = (DWORD_PTR)call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
        CHECK_CALLED(queue_char__Assign_and_destroy_item);
        ok(b, "pop returned false\n");
        ok(c == 'a', "got '%c', expected 'a'\n", c);
    }

    thread[1] = CreateThread(NULL, 0, queue_pop_thread, &queue, 0, NULL);
    ret = WaitForSingleObject(thread[1], 100);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", ret);

    SetEvent(block_end);
    WaitForSingleObject(thread[0], INFINITE);
    WaitForSingleObject(thread[1], INFINITE);
    CloseHandle(thread[0]);
    CloseHandle(thread[1]);

    /* test parallel push/pop */
    for(i=0; i<8; i++) {
        SET_EXPECT(queue_char__Copy_item);
        call_func2(p_queue_base_v4__Internal_push, &queue, &c);
        CHECK_CALLED(queue_char__Copy_item);
    }

    block = TRUE;
    thread[0] = CreateThread(NULL, 0, queue_push_thread, &queue, 0, NULL);
    WaitForSingleObject(block_start, INFINITE);

    SET_EXPECT(queue_char__Assign_and_destroy_item);
    call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
    CHECK_CALLED(queue_char__Assign_and_destroy_item);

    SetEvent(block_end);
    WaitForSingleObject(thread[0], INFINITE);
    CloseHandle(thread[0]);

    for(i=0; i<8; i++) {
        SET_EXPECT(queue_char__Assign_and_destroy_item);
        call_func2(p_queue_base_v4__Internal_pop_if_present, &queue, &c);
        CHECK_CALLED(queue_char__Assign_and_destroy_item);
    }

    SET_EXPECT(queue_char__Deallocate_page);
    call_func1(p_queue_base_v4__Internal_finish_clear, &queue);
    CHECK_CALLED(queue_char__Deallocate_page);
    call_func1(p_queue_base_v4_dtor, &queue);

    CloseHandle(block_start);
    CloseHandle(block_end);
}

static void test_vector_base_v4(void)
{
    vector_base_v4 vector, v2;
    size_t idx, size;
    compact_block b;
    int i, *data;

    concurrent_vector_int_ctor(&vector);

    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 0, "size of vector got %ld expected 0\n", (long)size);

    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &vector, sizeof(int), &idx);
    if(!data) {
        skip("_Internal_push_back not yet implemented\n");
        return;
    }
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    ok(idx == 0, "idx got %ld expected %d\n", (long)idx, 0);
    vector_elem_count++;
    *data = 1;
    ok(data == vector.storage[0], "vector.storage[0] got %p expected %p\n",
            vector.storage[0], data);
    ok(vector.first_block == 1, "vector.first_block got %ld expected 1\n",
            (long)vector.first_block);
    ok(vector.early_size == 1, "vector.early_size got %ld expected 1\n",
            (long)vector.early_size);
    ok(vector.segment == vector.storage, "vector.segment got %p expected %p\n",
            vector.segment, vector.storage);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 2, "size of vector got %ld expected 2\n", (long)size);

    data = call_func3(p_vector_base_v4__Internal_push_back, &vector, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    ok(idx == 1, "idx got %ld expected 1\n", (long)idx);
    vector_elem_count++;
    *data = 2;
    ok(vector.first_block == 1, "vector.first_block got %ld expected 1\n",
            (long)vector.first_block);
    ok(vector.early_size == 2, "vector.early_size got %ld expected 2\n",
            (long)vector.early_size);

    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 2, "size of vector got %ld expected 2\n", (long)size);

    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &vector, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    ok(idx == 2, "idx got %ld expected 2\n", (long)idx);
    vector_elem_count++;
    *data = 3;
    ok(vector.segment == vector.storage, "vector.segment got %p expected %p\n",
            vector.segment, vector.storage);
    ok(vector.first_block == 1, "vector.first_block got %ld expected 1\n",
            (long)vector.first_block);
    ok(vector.early_size == 3, "vector.early_size got %ld expected 3\n",
            (long)vector.early_size);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 4, "size of vector got %ld expected %d\n", (long)size, 4);

    data = call_func3(p_vector_base_v4__Internal_push_back, &vector, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    ok(idx == 3, "idx got %ld expected 3\n", (long)idx);
    vector_elem_count++;
    *data = 4;
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 4, "size of vector got %ld expected 4\n", (long)size);

    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &vector, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    ok(idx == 4, "idx got %ld expected 4\n", (long)idx);
    vector_elem_count++;
    *data = 5;
    ok(vector.segment == vector.storage, "vector.segment got %p expected %p\n",
            vector.segment, vector.storage);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &vector);
    ok(size == 8, "size of vector got %ld expected 8\n", (long)size);

    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    call_func4(p_vector_base_v4__Internal_copy, &v2, &vector,
            sizeof(int), concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(v2.first_block == 3, "v2.first_block got %ld expected 3\n",
            (long)v2.first_block);
    ok(v2.early_size == 5, "v2.early_size got %ld expected 5\n",
            (long)v2.early_size);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(size == 3, "_Internal_clear returned %ld expected 3\n", (long)size);
    concurrent_vector_int_dtor(&v2);

    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    CHECK_CALLED(concurrent_vector_int_alloc);
    vector_elem_count += 3;
    ok(idx == 2, "idx got %ld expected 2\n", (long)idx);
    SET_EXPECT(concurrent_vector_int_assign);
    SET_EXPECT(concurrent_vector_int_copy);
    SET_EXPECT(concurrent_vector_int_alloc);
    call_func6(p_vector_base_v4__Internal_assign, &v2, &vector, sizeof(int),
            concurrent_vector_int_destroy, concurrent_vector_int_assign,
            concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_assign);
    CHECK_CALLED(concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n",
            (long)v2.first_block);
    ok(v2.early_size == 5, "v2.early_size got %ld expected 5\n",
            (long)v2.early_size);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(size == 3, "_Internal_clear returned %ld expected 3\n", (long)size);
    concurrent_vector_int_dtor(&v2);

    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    CHECK_CALLED(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    CHECK_CALLED(concurrent_vector_int_alloc);
    vector_elem_count += 3;
    ok(idx == 2, "idx got %ld expected 2\n", (long)idx);
    call_func2(p_vector_base_v4__Internal_swap,
            &v2, &vector);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n",
            (long)v2.first_block);
    ok(v2.early_size == 5, "v2.early_size got %ld expected 5\n",
            (long)v2.early_size);
    ok(vector.early_size == 3, "vector.early_size got %ld expected 3\n",
            (long)vector.early_size);
    call_func2(p_vector_base_v4__Internal_swap,
            &v2, &vector);
    ok(v2.early_size == 3, "v2.early_size got %ld expected 3\n",
            (long)v2.early_size);
    ok(vector.early_size == 5, "vector.early_size got %ld expected 5\n",
            (long)vector.early_size);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(size == 2, "_Internal_clear returned %ld expected 2\n", (long)size);
    concurrent_vector_int_dtor(&v2);

    /* test for _Internal_compact */
    concurrent_vector_int_ctor(&v2);
    for(i=0; i<2; i++) {
        SET_EXPECT(concurrent_vector_int_alloc);
        data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
        CHECK_CALLED(concurrent_vector_int_alloc);
        ok(data != NULL, "_Internal_push_back returned NULL\n");
        data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
        ok(data != NULL, "_Internal_push_back returned NULL\n");
        vector_elem_count += 2;
    }
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 4, "v2.early_size got %ld expected 4\n", (long)v2.early_size);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    memset(&b, 0xff, sizeof(b));
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    SET_EXPECT(concurrent_vector_int_destroy);
    data = call_func5(p_vector_base_v4__Internal_compact,
            &v2, sizeof(int), &b, concurrent_vector_int_destroy,
            concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(v2.first_block == 2, "v2.first_block got %ld expected 2\n", (long)v2.first_block);
    ok(v2.early_size == 4,"v2.early_size got %ld expected 4\n", (long)v2.early_size);
    ok(b.first_block == 1, "b.first_block got %ld expected 1\n", (long)b.first_block);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    for(i=0; i<2; i++){
        ok(b.blocks[i] != NULL, "b.blocks[%d] got NULL\n", i);
        free(b.blocks[i]);
        vector_alloc_count--;
    }
    for(; i<ARRAY_SIZE(b.blocks); i++)
        ok(!b.blocks[i], "b.blocks[%d] != NULL\n", i);
    ok(b.size_check == -1, "b.size_check = %x\n", b.size_check);

    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    for(i=0; i<3; i++){
        data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
        ok(data != NULL, "_Internal_push_back returned NULL\n");
    }
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    vector_elem_count += 5;
    ok(v2.first_block == 2, "v2.first_block got %ld expected 2\n", (long)v2.first_block);
    ok(v2.early_size == 9, "v2.early_size got %ld expected 9\n", (long)v2.early_size);
    ok(v2.segment != v2.storage, "v2.segment got %p expected %p\n", v2.segment, v2.storage);
    for(i = 4;i < 32;i++)
        ok(v2.segment[i] == 0, "v2.segment[%d] got %p expected 0\n",
                i, v2.segment[i]);
    memset(&b, 0xff, sizeof(b));
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    SET_EXPECT(concurrent_vector_int_destroy);
    data = call_func5(p_vector_base_v4__Internal_compact,
            &v2, sizeof(int), &b, concurrent_vector_int_destroy,
            concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(v2.first_block == 4, "v2.first_block got %ld expected 4\n", (long)v2.first_block);
    ok(v2.early_size == 9, "v2.early_size got %ld expected 9\n", (long)v2.early_size);
    ok(b.first_block == 2, "b.first_block got %ld expected 2\n", (long)b.first_block);
    ok(v2.segment != v2.storage, "v2.segment got %p expected %p\n", v2.segment, v2.storage);
    for(i = 4;i < 32;i++)
        ok(v2.segment[i] == 0, "v2.segment[%d] got %p\n",
                i, v2.segment[i]);
    for(i=0; i<4; i++){
        ok(b.blocks[i] != NULL, "b.blocks[%d] got NULL\n", i);
        /* only b.blocks[0] and b.blocks[>=b.first_block] are used */
        if(i == b.first_block-1) continue;
        free(b.blocks[i]);
        vector_alloc_count--;
    }
    for(; i<ARRAY_SIZE(b.blocks); i++)
        ok(!b.blocks[i], "b.blocks[%d] != NULL\n", i);
    SET_EXPECT(concurrent_vector_int_alloc);
    call_func4(p_vector_base_v4__Internal_reserve,
            &v2, 17, sizeof(int), 32);
    CHECK_CALLED(concurrent_vector_int_alloc);
    data = call_func5(p_vector_base_v4__Internal_compact,
            &v2, sizeof(int), &b, concurrent_vector_int_destroy,
            concurrent_vector_int_copy);
    ok(v2.first_block == 4, "v2.first_block got %ld expected 4\n", (long)v2.first_block);
    ok(v2.early_size == 9, "v2.early_size got %ld expected 9\n", (long)v2.early_size);
    ok(b.first_block == 4, "b.first_block got %ld expected 2\n", (long)b.first_block);
    ok(v2.segment != v2.storage, "v2.segment got %p expected %p\n", v2.segment, v2.storage);
    for(i = 4; i < 32; i++)
        ok(v2.segment[i] == 0, "v2.segment[%d] got %p\n",
                i, v2.segment[i]);
    for(i=0; i<4; i++)
        ok(!b.blocks[i], "b.blocks[%d] != NULL\n", i);
    for(; i < 5; i++) {
        ok(b.blocks[i] != NULL, "b.blocks[%d] got NULL\n", i);
        free(b.blocks[i]);
        vector_alloc_count--;
    }
    for(; i<ARRAY_SIZE(b.blocks); i++)
        ok(!b.blocks[i], "b.blocks[%d] != NULL\n", i);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(size == 4, "_Internal_clear returned %ld expected 4\n", (long)size);
    concurrent_vector_int_dtor(&v2);

    /* test for Internal_grow_by */
    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    vector_elem_count += 2;
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    i = 0;
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    idx = (size_t)call_func5(p_vector_base_v4__Internal_grow_by,
            &v2, 1, sizeof(int), concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(idx == 2, "_Internal_grow_by returned %ld expected 2\n", (long)idx);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 3, "v2.early_size got %ld expected 3\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    idx = (size_t)call_func5(p_vector_base_v4__Internal_grow_by,
            &v2, 2, sizeof(int), concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(idx == 3, "_Internal_grow_by returned %ld expected 3\n", (long)idx);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 5, "v2.early_size got %ld expected 5\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    ok(size == 3, "_Internal_clear returned %ld expected 3\n", (long)size);
    CHECK_CALLED(concurrent_vector_int_destroy);
    concurrent_vector_int_dtor(&v2);

    /* test for Internal_grow_to_at_least_with_result */
    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    vector_elem_count += 2;
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    i = 0;
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    idx = (size_t)call_func5(p_vector_base_v4__Internal_grow_to_at_least_with_result,
            &v2, 3, sizeof(int), concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(idx == 2, "_Internal_grow_to_at_least_with_result returned %ld expected 2\n", (long)idx);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 3, "v2.early_size got %ld expected 3\n", (long)v2.early_size);
    i = 0;
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    idx = (size_t)call_func5(p_vector_base_v4__Internal_grow_to_at_least_with_result,
            &v2, 5, sizeof(int), concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(idx == 3, "_Internal_grow_to_at_least_with_result returned %ld expected 3\n", (long)idx);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 5, "v2.early_size got %ld expected 5\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    ok(size == 3, "_Internal_clear returned %ld expected 3\n", (long)size);
    CHECK_CALLED(concurrent_vector_int_destroy);
    concurrent_vector_int_dtor(&v2);

    /* test for _Internal_reserve */
    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    vector_elem_count += 2;
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_alloc);
    call_func4(p_vector_base_v4__Internal_reserve,
            &v2, 3, sizeof(int), 4);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &v2);
    ok(size == 4, "size of vector got %ld expected 4\n", (long)size);
    SET_EXPECT(concurrent_vector_int_alloc);
    call_func4(p_vector_base_v4__Internal_reserve,
            &v2, 5, sizeof(int), 8);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    ok(v2.segment == v2.storage, "v2.segment got %p expected %p\n",
            v2.segment, v2.storage);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &v2);
    ok(size == 8, "size of vector got %ld expected 8\n", (long)size);
    SET_EXPECT(concurrent_vector_int_alloc);
    call_func4(p_vector_base_v4__Internal_reserve,
            &v2, 9, sizeof(int), 16);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    ok(v2.segment != v2.storage, "v2.segment got %p expected %p\n", v2.segment, v2.storage);
    for(i = 4;i < 32;i++)
        ok(v2.segment[i] == 0, "v2.segment[%d] got %p\n",
            i, v2.segment[i]);
    size = (size_t)call_func1(p_vector_base_v4__Internal_capacity, &v2);
    ok(size == 16, "size of vector got %ld expected 8\n", (long)size);

    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    ok(size == 4, "_Internal_clear returned %ld expected 4\n", (long)size);
    CHECK_CALLED(concurrent_vector_int_destroy);
    concurrent_vector_int_dtor(&v2);

    /* test for _Internal_resize */
    concurrent_vector_int_ctor(&v2);
    SET_EXPECT(concurrent_vector_int_alloc);
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    CHECK_CALLED(concurrent_vector_int_alloc);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    data = call_func3(p_vector_base_v4__Internal_push_back, &v2, sizeof(int), &idx);
    ok(data != NULL, "_Internal_push_back returned NULL\n");
    vector_elem_count += 2;
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 2, "v2.early_size got %ld expected 2\n", (long)v2.early_size);
    i = 0;
    SET_EXPECT(concurrent_vector_int_destroy);
    call_func7(p_vector_base_v4__Internal_resize,
            &v2, 1, sizeof(int), 4, concurrent_vector_int_destroy, concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 1, "v2.early_size got %ld expected 1\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_alloc);
    SET_EXPECT(concurrent_vector_int_copy);
    call_func7(p_vector_base_v4__Internal_resize,
            &v2, 3, sizeof(int), 4, concurrent_vector_int_destroy, concurrent_vector_int_copy, &i);
    CHECK_CALLED(concurrent_vector_int_alloc);
    CHECK_CALLED(concurrent_vector_int_copy);
    ok(v2.first_block == 1, "v2.first_block got %ld expected 1\n", (long)v2.first_block);
    ok(v2.early_size == 3, "v2.early_size got %ld expected 3\n", (long)v2.early_size);
    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &v2, concurrent_vector_int_destroy);
    ok(size == 2, "_Internal_clear returned %ld expected 2\n", (long)size);
    CHECK_CALLED(concurrent_vector_int_destroy);
    concurrent_vector_int_dtor(&v2);

    SET_EXPECT(concurrent_vector_int_destroy);
    size = (size_t)call_func2(p_vector_base_v4__Internal_clear,
            &vector, concurrent_vector_int_destroy);
    CHECK_CALLED(concurrent_vector_int_destroy);
    ok(size == 3, "_Internal_clear returned %ld\n", (long)size);
    ok(vector.first_block == 1, "vector.first_block got %ld expected 1\n",
            (long)vector.first_block);
    ok(vector.early_size == 0, "vector.early_size got %ld expected 0\n",
            (long)vector.early_size);
    concurrent_vector_int_dtor(&vector);

    ok(!vector_elem_count, "vector_elem_count = %d, expected 0\n", vector_elem_count);
    ok(!vector_alloc_count, "vector_alloc_count = %d, expected 0\n", vector_alloc_count);
}

static BYTE byte_reverse(BYTE b)
{
    b = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
    b = ((b & 0xcc) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xaa) >> 1) | ((b & 0x55) << 1);
    return b;
}

static void test_data_exports(void)
{
    unsigned int i;

    ok(IsBadWritePtr((BYTE *)p_byte_reverse_table, 256), "byte_reverse_table is writeable.\n");
    for (i = 0; i < 256; ++i)
    {
        ok(p_byte_reverse_table[i] == byte_reverse(i), "Got unexpected byte %#x, expected %#x.\n",
                p_byte_reverse_table[i], byte_reverse(i));
    }
}

START_TEST(msvcp120)
{
    if(!init()) return;
    expect_idx = TlsAlloc();
    ok(expect_idx != TLS_OUT_OF_INDEXES, "TlsAlloc failed\n");
    alloc_expect_struct();

    test__Xtime_diff_to_millis2();
    test_xtime_get();
    test__Getcvt();
    test__Call_once();
    test__Do_call();
    test__Dtest();
    test__Dscale();
    test__FExp();
    test__Syserror_map();

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
    test__Mtx();
    test_threads__Mtx();

    test_vector_base_v4__Segment_index_of();
    test_queue_base_v4();
    test_vector_base_v4();

    test_vbtable_size_exports();

    test_data_exports();

    free_expect_struct();
    TlsFree(expect_idx);
    FreeLibrary(msvcp);
}
