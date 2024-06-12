/*
 * Unit test suite for vcomp
 *
 * Copyright 2012 Dan Kegel
 * Copyright 2015-2016 Sebastian Lackner
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
#include "wine/test.h"

static char         vcomp_manifest_file[MAX_PATH];
static HANDLE       vcomp_actctx_hctx;
static ULONG_PTR    vcomp_actctx_cookie;
static HMODULE      vcomp_handle;

typedef CRITICAL_SECTION *omp_lock_t;
typedef CRITICAL_SECTION *omp_nest_lock_t;

static void  (CDECL   *p_vcomp_atomic_add_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_add_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_add_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_add_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_add_r4)(float *dest, float val);
static void  (CDECL   *p_vcomp_atomic_add_r8)(double *dest, double val);
static void  (CDECL   *p_vcomp_atomic_and_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_and_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_and_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_and_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_div_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_div_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_div_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_div_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_div_r4)(float *dest, float val);
static void  (CDECL   *p_vcomp_atomic_div_r8)(double *dest, double val);
static void  (CDECL   *p_vcomp_atomic_div_ui1)(unsigned char *dest, unsigned char val);
static void  (CDECL   *p_vcomp_atomic_div_ui2)(unsigned short *dest, unsigned short val);
static void  (CDECL   *p_vcomp_atomic_div_ui4)(unsigned int *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_div_ui8)(ULONG64 *dest, ULONG64 val);
static void  (CDECL   *p_vcomp_atomic_mul_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_mul_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_mul_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_mul_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_mul_r4)(float *dest, float val);
static void  (CDECL   *p_vcomp_atomic_mul_r8)(double *dest, double val);
static void  (CDECL   *p_vcomp_atomic_or_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_or_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_or_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_or_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_shl_i1)(char *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shl_i2)(short *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shl_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_shl_i8)(LONG64 *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_i1)(char *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_i2)(short *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_shr_i8)(LONG64 *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_ui1)(unsigned char *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_ui2)(unsigned short *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_ui4)(unsigned int *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_shr_ui8)(ULONG64 *dest, unsigned int val);
static void  (CDECL   *p_vcomp_atomic_sub_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_sub_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_sub_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_sub_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_atomic_sub_r4)(float *dest, float val);
static void  (CDECL   *p_vcomp_atomic_sub_r8)(double *dest, double val);
static void  (CDECL   *p_vcomp_atomic_xor_i1)(char *dest, signed char val);
static void  (CDECL   *p_vcomp_atomic_xor_i2)(short *dest, short val);
static void  (CDECL   *p_vcomp_atomic_xor_i4)(int *dest, int val);
static void  (CDECL   *p_vcomp_atomic_xor_i8)(LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_barrier)(void);
static void  (CDECL   *p_vcomp_enter_critsect)(CRITICAL_SECTION **critsect);
static void  (CDECL   *p_vcomp_flush)(void);
static void  (CDECL   *p_vcomp_for_dynamic_init)(unsigned int flags, unsigned int first, unsigned int last,
                                                 int step, unsigned int chunksize);
static int   (CDECL   *p_vcomp_for_dynamic_next)(unsigned int *begin, unsigned int *end);
static void  (CDECL   *p_vcomp_for_static_end)(void);
static void  (CDECL   *p_vcomp_for_static_init)(int first, int last, int step, int chunksize, unsigned int *loops,
                                                int *begin, int *end, int *next, int *lastchunk);
static void  (CDECL   *p_vcomp_for_static_init_i8)(LONG64 first, LONG64 last, LONG64 step, LONG64 chunksize, ULONG64 *loops,
                                                LONG64 *begin, LONG64 *end, LONG64 *next, LONG64 *lastchunk);
static void  (CDECL   *p_vcomp_for_static_simple_init)(unsigned int first, unsigned int last, int step,
                                                       BOOL increment, unsigned int *begin, unsigned int *end);
static void  (CDECL   *p_vcomp_for_static_simple_init_i8)(ULONG64 first, ULONG64 last, LONG64 step,
                                                       BOOL increment, ULONG64 *begin, ULONG64 *end);
static void  (WINAPIV *p_vcomp_fork)(BOOL ifval, int nargs, void *wrapper, ...);
static int   (CDECL   *p_vcomp_get_thread_num)(void);
static void  (CDECL   *p_vcomp_leave_critsect)(CRITICAL_SECTION *critsect);
static int   (CDECL   *p_vcomp_master_begin)(void);
static void  (CDECL   *p_vcomp_master_end)(void);
static void  (CDECL   *p_vcomp_reduction_i1)(unsigned int flags, char *dest, char val);
static void  (CDECL   *p_vcomp_reduction_i2)(unsigned int flags, short *dest, short val);
static void  (CDECL   *p_vcomp_reduction_i4)(unsigned int flags, int *dest, int val);
static void  (CDECL   *p_vcomp_reduction_i8)(unsigned int flags, LONG64 *dest, LONG64 val);
static void  (CDECL   *p_vcomp_reduction_r4)(unsigned int flags, float *dest, float val);
static void  (CDECL   *p_vcomp_reduction_r8)(unsigned int flags, double *dest, double val);
static void  (CDECL   *p_vcomp_reduction_u1)(unsigned int flags, unsigned char *dest, unsigned char val);
static void  (CDECL   *p_vcomp_reduction_u2)(unsigned int flags, unsigned short *dest, unsigned short val);
static void  (CDECL   *p_vcomp_reduction_u4)(unsigned int flags, unsigned int *dest, unsigned int val);
static void  (CDECL   *p_vcomp_reduction_u8)(unsigned int flags, ULONG64 *dest, ULONG64 val);
static void  (CDECL   *p_vcomp_sections_init)(int n);
static int   (CDECL   *p_vcomp_sections_next)(void);
static void  (CDECL   *p_vcomp_set_num_threads)(int num_threads);
static int   (CDECL   *p_vcomp_single_begin)(int flags);
static void  (CDECL   *p_vcomp_single_end)(void);
static void  (CDECL   *pomp_destroy_lock)(omp_lock_t *lock);
static void  (CDECL   *pomp_destroy_nest_lock)(omp_nest_lock_t *lock);
static int   (CDECL   *pomp_get_max_threads)(void);
static int   (CDECL   *pomp_get_nested)(void);
static int   (CDECL   *pomp_get_num_procs)(void);
static int   (CDECL   *pomp_get_num_threads)(void);
static int   (CDECL   *pomp_get_thread_num)(void);
static int   (CDECL   *pomp_in_parallel)(void);
static void  (CDECL   *pomp_init_lock)(omp_lock_t *lock);
static void  (CDECL   *pomp_init_nest_lock)(omp_nest_lock_t *lock);
static void  (CDECL   *pomp_set_lock)(omp_lock_t *lock);
static void  (CDECL   *pomp_set_nest_lock)(omp_nest_lock_t *lock);
static void  (CDECL   *pomp_set_nested)(int nested);
static void  (CDECL   *pomp_set_num_threads)(int num_threads);
static int   (CDECL   *pomp_test_lock)(omp_lock_t *lock);
static int   (CDECL   *pomp_test_nest_lock)(omp_nest_lock_t *lock);
static void  (CDECL   *pomp_unset_lock)(omp_lock_t *lock);
static void  (CDECL   *pomp_unset_nest_lock)(omp_nest_lock_t *lock);

#define VCOMP_DYNAMIC_FLAGS_STATIC      0x01
#define VCOMP_DYNAMIC_FLAGS_CHUNKED     0x02
#define VCOMP_DYNAMIC_FLAGS_GUIDED      0x03
#define VCOMP_DYNAMIC_FLAGS_INCREMENT   0x40

#define VCOMP_REDUCTION_FLAGS_ADD       0x100
#define VCOMP_REDUCTION_FLAGS_MUL       0x200
#define VCOMP_REDUCTION_FLAGS_AND       0x300
#define VCOMP_REDUCTION_FLAGS_OR        0x400
#define VCOMP_REDUCTION_FLAGS_XOR       0x500
#define VCOMP_REDUCTION_FLAGS_BOOL_AND  0x600
#define VCOMP_REDUCTION_FLAGS_BOOL_OR   0x700

#define ULL(a,b) (((ULONG64)(a) << 32) | (b))

#ifdef __i386__
#define ARCH "x86"
#elif defined __aarch64__ || defined__arm64ec__
#define ARCH "arm64"
#elif defined(__x86_64__)
#define ARCH "amd64"
#elif defined __arm__
#define ARCH "arm"
#else
#define ARCH "none"
#endif

static const char vcomp_manifest[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
    "  <assemblyIdentity\n"
    "      type=\"win32\"\n"
    "      name=\"Wine.vcomp.Test\"\n"
    "      version=\"1.0.0.0\"\n"
    "      processorArchitecture=\"" ARCH "\"\n"
    "  />\n"
    "<description>Wine vcomp test suite</description>\n"
    "<dependency>\n"
    "  <dependentAssembly>\n"
    "    <assemblyIdentity\n"
    "        type=\"win32\"\n"
    "        name=\"Microsoft.VC80.OpenMP\"\n"
    "        version=\"8.0.50608.0\"\n"
    "        processorArchitecture=\"" ARCH "\"\n"
    "        publicKeyToken=\"1fc8b3b9a1e18e3b\"\n"
    "    />\n"
    "  </dependentAssembly>\n"
    "</dependency>\n"
    "</assembly>\n";

#undef ARCH

static void create_vcomp_manifest(void)
{
    char temp_path[MAX_PATH];
    DWORD written;
    ACTCTXA ctx;
    HANDLE file;

    if (!GetTempPathA(sizeof(temp_path), temp_path) ||
        !GetTempFileNameA(temp_path, "vcomp", 0, vcomp_manifest_file))
    {
        ok(0, "failed to create manifest file\n");
        return;
    }

    file = CreateFileA(vcomp_manifest_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        ok(0, "failed to open manifest file\n");
        return;
    }

    if (!WriteFile(file, vcomp_manifest, sizeof(vcomp_manifest) - 1, &written, NULL))
        written = 0;
    CloseHandle(file);

    if (written != sizeof(vcomp_manifest) - 1)
    {
        ok(0, "failed to write manifest file\n");
        DeleteFileA(vcomp_manifest_file);
        return;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.cbSize   = sizeof(ctx);
    ctx.lpSource = vcomp_manifest_file;
    vcomp_actctx_hctx = CreateActCtxA(&ctx);
    if (!vcomp_actctx_hctx)
    {
        ok(0, "failed to create activation context\n");
        DeleteFileA(vcomp_manifest_file);
        return;
    }

    if (!ActivateActCtx(vcomp_actctx_hctx, &vcomp_actctx_cookie))
    {
        win_skip("failed to activate context\n");
        ReleaseActCtx(vcomp_actctx_hctx);
        DeleteFileA(vcomp_manifest_file);
        vcomp_actctx_hctx = NULL;
    }
}

static void release_vcomp(void)
{
    if (vcomp_handle)
        FreeLibrary(vcomp_handle);

    if (vcomp_actctx_hctx)
    {
        DeactivateActCtx(0, vcomp_actctx_cookie);
        ReleaseActCtx(vcomp_actctx_hctx);
        DeleteFileA(vcomp_manifest_file);
    }
}

#define VCOMP_GET_PROC(func) \
    do \
    { \
        p ## func = (void *)GetProcAddress(vcomp_handle, #func); \
        if (!p ## func) trace("Failed to get address for %s\n", #func); \
    } \
    while (0)

static BOOL init_vcomp(void)
{
    create_vcomp_manifest();

    vcomp_handle = LoadLibraryA("vcomp.dll");
    if (!vcomp_handle)
    {
        win_skip("vcomp.dll not installed\n");
        release_vcomp();
        return FALSE;
    }

    VCOMP_GET_PROC(_vcomp_atomic_add_i1);
    VCOMP_GET_PROC(_vcomp_atomic_add_i2);
    VCOMP_GET_PROC(_vcomp_atomic_add_i4);
    VCOMP_GET_PROC(_vcomp_atomic_add_i8);
    VCOMP_GET_PROC(_vcomp_atomic_add_r4);
    VCOMP_GET_PROC(_vcomp_atomic_add_r8);
    VCOMP_GET_PROC(_vcomp_atomic_and_i1);
    VCOMP_GET_PROC(_vcomp_atomic_and_i2);
    VCOMP_GET_PROC(_vcomp_atomic_and_i4);
    VCOMP_GET_PROC(_vcomp_atomic_and_i8);
    VCOMP_GET_PROC(_vcomp_atomic_div_i1);
    VCOMP_GET_PROC(_vcomp_atomic_div_i2);
    VCOMP_GET_PROC(_vcomp_atomic_div_i4);
    VCOMP_GET_PROC(_vcomp_atomic_div_i8);
    VCOMP_GET_PROC(_vcomp_atomic_div_r4);
    VCOMP_GET_PROC(_vcomp_atomic_div_r8);
    VCOMP_GET_PROC(_vcomp_atomic_div_ui1);
    VCOMP_GET_PROC(_vcomp_atomic_div_ui2);
    VCOMP_GET_PROC(_vcomp_atomic_div_ui4);
    VCOMP_GET_PROC(_vcomp_atomic_div_ui8);
    VCOMP_GET_PROC(_vcomp_atomic_mul_i1);
    VCOMP_GET_PROC(_vcomp_atomic_mul_i2);
    VCOMP_GET_PROC(_vcomp_atomic_mul_i4);
    VCOMP_GET_PROC(_vcomp_atomic_mul_i8);
    VCOMP_GET_PROC(_vcomp_atomic_mul_r4);
    VCOMP_GET_PROC(_vcomp_atomic_mul_r8);
    VCOMP_GET_PROC(_vcomp_atomic_or_i1);
    VCOMP_GET_PROC(_vcomp_atomic_or_i2);
    VCOMP_GET_PROC(_vcomp_atomic_or_i4);
    VCOMP_GET_PROC(_vcomp_atomic_or_i8);
    VCOMP_GET_PROC(_vcomp_atomic_shl_i1);
    VCOMP_GET_PROC(_vcomp_atomic_shl_i2);
    VCOMP_GET_PROC(_vcomp_atomic_shl_i4);
    VCOMP_GET_PROC(_vcomp_atomic_shl_i8);
    VCOMP_GET_PROC(_vcomp_atomic_shr_i1);
    VCOMP_GET_PROC(_vcomp_atomic_shr_i2);
    VCOMP_GET_PROC(_vcomp_atomic_shr_i4);
    VCOMP_GET_PROC(_vcomp_atomic_shr_i8);
    VCOMP_GET_PROC(_vcomp_atomic_shr_ui1);
    VCOMP_GET_PROC(_vcomp_atomic_shr_ui2);
    VCOMP_GET_PROC(_vcomp_atomic_shr_ui4);
    VCOMP_GET_PROC(_vcomp_atomic_shr_ui8);
    VCOMP_GET_PROC(_vcomp_atomic_sub_i1);
    VCOMP_GET_PROC(_vcomp_atomic_sub_i2);
    VCOMP_GET_PROC(_vcomp_atomic_sub_i4);
    VCOMP_GET_PROC(_vcomp_atomic_sub_i8);
    VCOMP_GET_PROC(_vcomp_atomic_sub_r4);
    VCOMP_GET_PROC(_vcomp_atomic_sub_r8);
    VCOMP_GET_PROC(_vcomp_atomic_xor_i1);
    VCOMP_GET_PROC(_vcomp_atomic_xor_i2);
    VCOMP_GET_PROC(_vcomp_atomic_xor_i4);
    VCOMP_GET_PROC(_vcomp_atomic_xor_i8);
    VCOMP_GET_PROC(_vcomp_barrier);
    VCOMP_GET_PROC(_vcomp_enter_critsect);
    VCOMP_GET_PROC(_vcomp_flush);
    VCOMP_GET_PROC(_vcomp_for_dynamic_init);
    VCOMP_GET_PROC(_vcomp_for_dynamic_next);
    VCOMP_GET_PROC(_vcomp_for_static_end);
    VCOMP_GET_PROC(_vcomp_for_static_init);
    VCOMP_GET_PROC(_vcomp_for_static_init_i8);
    VCOMP_GET_PROC(_vcomp_for_static_simple_init);
    VCOMP_GET_PROC(_vcomp_for_static_simple_init_i8);
    VCOMP_GET_PROC(_vcomp_fork);
    VCOMP_GET_PROC(_vcomp_get_thread_num);
    VCOMP_GET_PROC(_vcomp_leave_critsect);
    VCOMP_GET_PROC(_vcomp_master_begin);
    VCOMP_GET_PROC(_vcomp_master_end);
    VCOMP_GET_PROC(_vcomp_reduction_i1);
    VCOMP_GET_PROC(_vcomp_reduction_i2);
    VCOMP_GET_PROC(_vcomp_reduction_i4);
    VCOMP_GET_PROC(_vcomp_reduction_i8);
    VCOMP_GET_PROC(_vcomp_reduction_r4);
    VCOMP_GET_PROC(_vcomp_reduction_r8);
    VCOMP_GET_PROC(_vcomp_reduction_u1);
    VCOMP_GET_PROC(_vcomp_reduction_u2);
    VCOMP_GET_PROC(_vcomp_reduction_u4);
    VCOMP_GET_PROC(_vcomp_reduction_u8);
    VCOMP_GET_PROC(_vcomp_sections_init);
    VCOMP_GET_PROC(_vcomp_sections_next);
    VCOMP_GET_PROC(_vcomp_set_num_threads);
    VCOMP_GET_PROC(_vcomp_single_begin);
    VCOMP_GET_PROC(_vcomp_single_end);
    VCOMP_GET_PROC(omp_destroy_lock);
    VCOMP_GET_PROC(omp_destroy_nest_lock);
    VCOMP_GET_PROC(omp_get_max_threads);
    VCOMP_GET_PROC(omp_get_nested);
    VCOMP_GET_PROC(omp_get_num_procs);
    VCOMP_GET_PROC(omp_get_num_threads);
    VCOMP_GET_PROC(omp_get_thread_num);
    VCOMP_GET_PROC(omp_in_parallel);
    VCOMP_GET_PROC(omp_init_lock);
    VCOMP_GET_PROC(omp_init_nest_lock);
    VCOMP_GET_PROC(omp_set_lock);
    VCOMP_GET_PROC(omp_set_nest_lock);
    VCOMP_GET_PROC(omp_set_nested);
    VCOMP_GET_PROC(omp_set_num_threads);
    VCOMP_GET_PROC(omp_test_lock);
    VCOMP_GET_PROC(omp_test_nest_lock);
    VCOMP_GET_PROC(omp_unset_lock);
    VCOMP_GET_PROC(omp_unset_nest_lock);

    return TRUE;
}

#undef VCOMP_GET_PROC

static void CDECL num_threads_cb2(int parallel, LONG *count)
{
    int is_parallel = pomp_in_parallel();
    ok(is_parallel == parallel, "expected %d, got %d\n", parallel, is_parallel);

    InterlockedIncrement(count);
}

static void CDECL num_threads_cb(BOOL nested, int parallel, int nested_threads, LONG *count)
{
    int is_parallel, num_threads, thread_num;
    LONG thread_count;

    InterlockedIncrement(count);
    p_vcomp_barrier();

    num_threads = pomp_get_num_threads();
    ok(num_threads == *count, "expected num_threads == %ld, got %d\n", *count, num_threads);
    thread_num = pomp_get_thread_num();
    ok(thread_num >= 0 && thread_num < num_threads,
       "expected thread_num in range [0, %d], got %d\n", num_threads - 1, thread_num);
    ok(thread_num == p_vcomp_get_thread_num(),
       "expected _vcomp_get_thread_num to return the same value\n");

    is_parallel = pomp_in_parallel();
    ok(is_parallel == parallel, "expected %d, got %d\n", parallel, is_parallel);

    /* limit number of nested threads */
    nested_threads = min( nested_threads, 256 / pomp_get_max_threads() );
    p_vcomp_set_num_threads(nested_threads);

    thread_count = 0;
    p_vcomp_fork(TRUE, 2, num_threads_cb2, TRUE, &thread_count);
    if (nested)
        ok(thread_count == nested_threads, "expected %d threads, got %ld\n", nested_threads, thread_count);
    else
        ok(thread_count == 1, "expected 1 thread, got %ld\n", thread_count);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == parallel, "expected %d, got %d\n", parallel, is_parallel);

    thread_count = 0;
    p_vcomp_fork(FALSE, 2, num_threads_cb2, parallel, &thread_count);
    ok(thread_count == 1, "expected 1 thread, got %ld\n", thread_count);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == parallel, "expected %d, got %d\n", parallel, is_parallel);
}

static void test_omp_get_num_threads(BOOL nested)
{
    int is_nested, is_parallel, max_threads, num_threads, thread_num;
    LONG thread_count;

    ok(pomp_get_thread_num != p_vcomp_get_thread_num,
       "expected omp_get_thread_num != _vcomp_get_thread_num\n");

    pomp_set_nested(nested);
    is_nested = pomp_get_nested();
    ok(is_nested == nested, "expected %d, got %d\n", nested, is_nested);

    max_threads = pomp_get_max_threads();
    ok(max_threads >= 1, "expected max_threads >= 1, got %d\n", max_threads);
    thread_num = pomp_get_thread_num();
    ok(thread_num == 0, "expected thread_num == 0, got %d\n", thread_num);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == FALSE, "expected FALSE, got %d\n", is_parallel);

    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, max_threads, &thread_count);
    ok(thread_count == max_threads, "expected %d threads, got %ld\n", max_threads, thread_count);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == FALSE, "expected FALSE, got %d\n", is_parallel);

    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(FALSE, 4, num_threads_cb, TRUE, FALSE, max_threads, &thread_count);
    ok(thread_count == 1, "expected 1 thread, got %ld\n", thread_count);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == FALSE, "expected FALSE, got %d\n", is_parallel);

    pomp_set_num_threads(1);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 1, &thread_count);
    ok(thread_count == 1, "expected 1 thread, got %ld\n", thread_count);

    is_parallel = pomp_in_parallel();
    ok(is_parallel == FALSE, "expected FALSE, got %d\n", is_parallel);

    pomp_set_num_threads(2);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 2, &thread_count);
    ok(thread_count == 2, "expected 2 threads, got %ld\n", thread_count);

    pomp_set_num_threads(4);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %ld\n", thread_count);

    p_vcomp_set_num_threads(8);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 4, &thread_count);
    ok(thread_count == 8, "expected 8 threads, got %ld\n", thread_count);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %ld\n", thread_count);

    p_vcomp_set_num_threads(0);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %ld\n", thread_count);

    pomp_set_num_threads(0);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 4, num_threads_cb, nested, TRUE, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %ld\n", thread_count);

    pomp_set_num_threads(max_threads);
    pomp_set_nested(FALSE);
}

static void CDECL fork_ptr_cb(LONG *a, LONG *b, LONG *c, LONG *d, LONG *e)
{
    InterlockedIncrement(a);
    InterlockedIncrement(b);
    InterlockedIncrement(c);
    InterlockedIncrement(d);
    InterlockedIncrement(e);
}

static void CDECL fork_uintptr_cb(UINT_PTR a, UINT_PTR b, UINT_PTR c, UINT_PTR d,
                                  UINT_PTR e, UINT_PTR f, UINT_PTR g, UINT_PTR h,
                                  UINT_PTR i, UINT_PTR j, UINT_PTR k)
{
    ok(a == 1, "expected a == 1, got %p\n", (void *)a);
    ok(b == MAXUINT_PTR - 2, "expected b == MAXUINT_PTR - 2, got %p\n", (void *)b);
    ok(c == 3, "expected c == 3, got %p\n", (void *)c);
    ok(d == MAXUINT_PTR - 4, "expected d == MAXUINT_PTR - 4, got %p\n", (void *)d);
    ok(e == 5, "expected e == 5, got %p\n", (void *)e);
    ok(f == 6, "expected f == 6, got %p\n", (void *)f);
    ok(g == 7, "expected g == 7, got %p\n", (void *)g);
    ok(h == 8, "expected h == 8, got %p\n", (void *)h);
    ok(i == 9, "expected i == 9, got %p\n", (void *)i);
    ok(j == 10, "expected j == 10, got %p\n", (void *)j);
    ok(k == 11, "expected k == 11, got %p\n", (void *)k);
}

#ifdef __i386__
static void CDECL fork_float_cb(float a, float b, float c, float d, float e)
{
    ok(1.4999 < a && a < 1.5001, "expected a == 1.5, got %f\n", a);
    ok(2.4999 < b && b < 2.5001, "expected b == 2.5, got %f\n", b);
    ok(3.4999 < c && c < 3.5001, "expected c == 3.5, got %f\n", c);
    ok(4.4999 < d && d < 4.5001, "expected d == 4.5, got %f\n", d);
    ok(5.4999 < e && e < 5.5001, "expected e == 5.5, got %f\n", e);
}
#endif

static void test_vcomp_fork(void)
{
    LONG a, b, c, d, e;
    int max_threads = pomp_get_max_threads();
    pomp_set_num_threads(4);

    a = 0; b = 1; c = 2; d = 3; e = 4;
    p_vcomp_fork(FALSE, 5, fork_ptr_cb, &a, &b, &c, &d, &e);
    ok(a == 1, "expected a == 1, got %ld\n", a);
    ok(b == 2, "expected b == 2, got %ld\n", b);
    ok(c == 3, "expected c == 3, got %ld\n", c);
    ok(d == 4, "expected d == 4, got %ld\n", d);
    ok(e == 5, "expected e == 5, got %ld\n", e);

    a = 0; b = 1; c = 2; d = 3; e = 4;
    p_vcomp_fork(TRUE, 5, fork_ptr_cb, &a, &b, &c, &d, &e);
    ok(a == 4, "expected a == 4, got %ld\n", a);
    ok(b == 5, "expected b == 5, got %ld\n", b);
    ok(c == 6, "expected c == 6, got %ld\n", c);
    ok(d == 7, "expected d == 7, got %ld\n", d);
    ok(e == 8, "expected e == 8, got %ld\n", e);

    p_vcomp_fork(TRUE, 11, fork_uintptr_cb, (UINT_PTR)1, (UINT_PTR)(MAXUINT_PTR - 2),
        (UINT_PTR)3, (UINT_PTR)(MAXUINT_PTR - 4), (UINT_PTR)5,
        (UINT_PTR)6, (UINT_PTR)7, (UINT_PTR)8, (UINT_PTR)9,
        (UINT_PTR)10, (UINT_PTR)11);

#ifdef __i386__
    {
        void (CDECL *func)(BOOL, int, void *, float, float, float, float, float) = (void *)p_vcomp_fork;
        func(TRUE, 5, fork_float_cb, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f);
    }
#else
    skip("skipping float test on non-x86\n");
#endif

    pomp_set_num_threads(max_threads);
}

static void CDECL section_cb(LONG *a, LONG *b, LONG *c)
{
    int i;

    p_vcomp_sections_init(20);
    while ((i = p_vcomp_sections_next()) != -1)
    {
        InterlockedIncrement(a);
        Sleep(1);
    }

    p_vcomp_sections_init(30);
    while ((i = p_vcomp_sections_next()) != -1)
    {
        InterlockedIncrement(b);
        Sleep(1);
    }

    p_vcomp_sections_init(40);
    while ((i = p_vcomp_sections_next()) != -1)
    {
        InterlockedIncrement(c);
        Sleep(1);
    }
}

static void test_vcomp_sections_init(void)
{
    LONG a, b, c;
    int max_threads = pomp_get_max_threads();
    int i;

if (0)
{
    /* calling _vcomp_sections_next without prior _vcomp_sections_init
     * returns uninitialized memory on Windows. */
    i = p_vcomp_sections_next();
    ok(i == -1, "expected -1, got %d\n", i);
}

    a = b = c = 0;
    section_cb(&a, &b, &c);
    ok(a == 20, "expected a == 20, got %ld\n", a);
    ok(b == 30, "expected b == 30, got %ld\n", b);
    ok(c == 40, "expected c == 40, got %ld\n", c);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = b = c = 0;
        p_vcomp_fork(TRUE, 3, section_cb, &a, &b, &c);
        ok(a == 20, "expected a == 20, got %ld\n", a);
        ok(b == 30, "expected b == 30, got %ld\n", b);
        ok(c == 40, "expected c == 40, got %ld\n", c);

        a = b = c = 0;
        p_vcomp_fork(FALSE, 3, section_cb, &a, &b, &c);
        ok(a == 20, "expected a == 20, got %ld\n", a);
        ok(b == 30, "expected b == 30, got %ld\n", b);
        ok(c == 40, "expected c == 40, got %ld\n", c);
    }

    pomp_set_num_threads(max_threads);
}

static void my_for_static_simple_init(BOOL dynamic, unsigned int first, unsigned int last, int step,
                                      BOOL increment, unsigned int *begin, unsigned int *end)
{
    unsigned int iterations, per_thread, remaining;
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();

    if (!dynamic && num_threads == 1)
    {
        *begin = first;
        *end   = last;
        return;
    }

    if (step <= 0)
    {
        *begin = 0;
        *end   = increment ? -1 : 1;
        return;
    }

    if (increment)
        iterations = 1 + (last - first) / step;
    else
    {
        iterations = 1 + (first - last) / step;
        step *= -1;
    }

    per_thread = iterations / num_threads;
    remaining  = iterations - per_thread * num_threads;

    if (thread_num < remaining)
        per_thread++;
    else if (per_thread)
        first += remaining * step;
    else
    {
        *begin = first;
        *end   = first - step;
        return;
    }

    *begin = first + per_thread * thread_num * step;
    *end   = *begin + (per_thread - 1) * step;
}

static void my_for_static_simple_init_i8(BOOL dynamic, ULONG64 first, ULONG64 last, LONG64 step,
                                      BOOL increment, ULONG64 *begin, ULONG64 *end)
{
    ULONG64 iterations, per_thread, remaining;
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();

    if (!dynamic && num_threads == 1)
    {
        *begin = first;
        *end   = last;
        return;
    }

    if (step <= 0)
    {
        *begin = 0;
        *end   = increment ? -1 : 1;
        return;
    }

    if (increment)
        iterations = 1 + (last - first) / step;
    else
    {
        iterations = 1 + (first - last) / step;
        step *= -1;
    }

    per_thread = iterations / num_threads;
    remaining  = iterations - per_thread * num_threads;

    if (thread_num < remaining)
        per_thread++;
    else if (per_thread)
        first += remaining * step;
    else
    {
        *begin = first;
        *end   = first - step;
        return;
    }

    *begin = first + per_thread * thread_num * step;
    *end   = *begin + (per_thread - 1) * step;
}

static void CDECL for_static_simple_cb(void)
{
    static const struct
    {
        unsigned int first;
        unsigned int last;
        int step;
    }
    tests[] =
    {
        {          0,          0,   1 }, /* 0 */
        {          0,          1,   1 },
        {          0,          2,   1 },
        {          0,          3,   1 },
        {          0,        100,   0 },
        {          0,        100,   1 },
        {          0,        100,   2 },
        {          0,        100,   3 },
        {          0,        100,  -1 },
        {          0,        100,  -2 },
        {          0,        100,  -3 }, /* 10 */
        {          0,        100,  10 },
        {          0,        100,  50 },
        {          0,        100, 100 },
        {          0,        100, 150 },
        {          0, 0x80000000,   1 },
        {          0, 0xfffffffe,   1 },
        {          0, 0xffffffff,   1 },
        {         50,         50,   0 },
        {         50,         50,   1 },
        {         50,         50,   2 }, /* 20 */
        {         50,         50,   3 },
        {         50,         50,  -1 },
        {         50,         50,  -2 },
        {         50,         50,  -3 },
        {        100,        200,   1 },
        {        100,        200,   5 },
        {        100,        200,  10 },
        {        100,        200,  50 },
        {        100,        200, 100 },
        {        100,        200, 150 }, /* 30 */
    };
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        unsigned int my_begin, my_end, begin, end;

        begin = end = 0xdeadbeef;
        my_for_static_simple_init(FALSE, tests[i].first, tests[i].last, tests[i].step, FALSE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init(tests[i].first, tests[i].last, tests[i].step, FALSE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %u, got %u\n",
           i, thread_num, num_threads, my_begin, begin);
        ok(end == my_end, "test %d, thread %d/%d: expected end == %u, got %u\n",
           i, thread_num, num_threads, my_end, end);

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        begin = end = 0xdeadbeef;
        my_for_static_simple_init(FALSE, tests[i].first, tests[i].last, tests[i].step, TRUE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init(tests[i].first, tests[i].last, tests[i].step, TRUE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %u, got %u\n",
           i, thread_num, num_threads, my_begin, begin);
        ok(end == my_end, "test %d, thread %d/%d: expected end == %u, got %u\n",
           i, thread_num, num_threads, my_end, end);

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        if (tests[i].first == tests[i].last) continue;

        begin = end = 0xdeadbeef;
        my_for_static_simple_init(FALSE, tests[i].last, tests[i].first, tests[i].step, FALSE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init(tests[i].last, tests[i].first, tests[i].step, FALSE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %u, got %u\n",
           i, thread_num, num_threads, my_begin, begin);
        ok(end == my_end, "test %d, thread %d/%d: expected end == %u, got %u\n",
           i, thread_num, num_threads, my_end, end);

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        begin = end = 0xdeadbeef;
        my_for_static_simple_init(FALSE, tests[i].last, tests[i].first, tests[i].step, TRUE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init(tests[i].last, tests[i].first, tests[i].step, TRUE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %u, got %u\n",
           i, thread_num, num_threads, my_begin, begin);
        ok(end == my_end, "test %d, thread %d/%d: expected end == %u, got %u\n",
           i, thread_num, num_threads, my_end, end);

        p_vcomp_for_static_end();
        p_vcomp_barrier();
    }
}

static void CDECL for_static_simple_i8_cb(void)
{
    static const struct
    {
        ULONG64 first;
        ULONG64 last;
        LONG64 step;
    }
    tests[] =
    {
        {          0,          0,   1 }, /* 0 */
        {          0,          1,   1 },
        {          0,          2,   1 },
        {          0,          3,   1 },
        {          0,        100,   0 },
        {          0,        100,   1 },
        {          0,        100,   2 },
        {          0,        100,   3 },
        {          0,        100,  -1 },
        {          0,        100,  -2 },
        {          0,        100,  -3 }, /* 10 */
        {          0,        100,  10 },
        {          0,        100,  50 },
        {          0,        100, 100 },
        {          0,        100, 150 },
        {          0, 0x80000000,   1 },
        {          0, 0xfffffffe,   1 },
        {          0, 0xffffffff,   1 },
        {         50,         50,   0 },
        {         50,         50,   1 },
        {         50,         50,   2 }, /* 20 */
        {         50,         50,   3 },
        {         50,         50,  -1 },
        {         50,         50,  -2 },
        {         50,         50,  -3 },
        {        100,        200,   1 },
        {        100,        200,   5 },
        {        100,        200,  10 },
        {        100,        200,  50 },
        {        100,        200, 100 },
        {        100,        200, 150 }, /* 30 */
    };
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ULONG64 my_begin, my_end, begin, end;

        begin = end = -0xdeadbeef;
        my_for_static_simple_init_i8(FALSE, tests[i].first, tests[i].last, tests[i].step, FALSE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init_i8(tests[i].first, tests[i].last, tests[i].step, FALSE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
        ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        begin = end = -0xdeadbeef;
        my_for_static_simple_init_i8(FALSE, tests[i].first, tests[i].last, tests[i].step, TRUE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init_i8(tests[i].first, tests[i].last, tests[i].step, TRUE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
        ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        if (tests[i].first == tests[i].last) continue;

        begin = end = -0xdeadbeef;
        my_for_static_simple_init_i8(FALSE, tests[i].last, tests[i].first, tests[i].step, FALSE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init_i8(tests[i].last, tests[i].first, tests[i].step, FALSE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
        ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        begin = end = -0xdeadbeef;
        my_for_static_simple_init_i8(FALSE, tests[i].last, tests[i].first, tests[i].step, TRUE, &my_begin, &my_end);
        p_vcomp_for_static_simple_init_i8(tests[i].last, tests[i].first, tests[i].step, TRUE, &begin, &end);

        ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
        ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
           i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));

        p_vcomp_for_static_end();
        p_vcomp_barrier();
    }
}

static void test_vcomp_for_static_simple_init(void)
{
    int max_threads = pomp_get_max_threads();
    int i;

    for_static_simple_cb();
    for_static_simple_i8_cb();

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);
        p_vcomp_fork(TRUE, 0, for_static_simple_cb);
        p_vcomp_fork(FALSE, 0, for_static_simple_cb);
        p_vcomp_fork(TRUE, 0, for_static_simple_i8_cb);
        p_vcomp_fork(FALSE, 0, for_static_simple_i8_cb);
    }

    pomp_set_num_threads(max_threads);
}

#define VCOMP_FOR_STATIC_BROKEN_LOOP 1
#define VCOMP_FOR_STATIC_BROKEN_NEXT 2

static DWORD CDECL my_for_static_init(int first, int last, int step, int chunksize, unsigned int *loops,
                               int *begin, int *end, int *next, int *lastchunk)
{
    unsigned int iterations, num_chunks, per_thread, remaining;
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();

    if (num_threads == 1 && chunksize != 1)
    {
        *loops      = 1;
        *begin      = first;
        *end        = last;
        *next       = 0;
        *lastchunk  = first;
        return 0;
    }

    if (first == last)
    {
        *loops = !thread_num;
        if (!thread_num)
        {
            /* The value in *next on Windows is either uninitialized, or contains
             * garbage. The value shouldn't matter for *loops <= 1, so no need to
             * reproduce that. */
            *begin      = first;
            *end        = last;
            *next       = 0;
            *lastchunk  = first;
        }
        return thread_num ? 0 : VCOMP_FOR_STATIC_BROKEN_NEXT;
    }

    if (step <= 0)
    {
        /* The total number of iterations depends on the number of threads here,
         * which doesn't make any sense. This is most likely a bug in the Windows
         * implementation. */
        return VCOMP_FOR_STATIC_BROKEN_LOOP;
    }

    if (first < last)
        iterations = 1 + (last - first) / step;
    else
    {
        iterations = 1 + (first - last) / step;
        step *= -1;
    }

    if (chunksize < 1)
        chunksize = 1;

    num_chunks  = ((DWORD64)iterations + chunksize - 1) / chunksize;
    per_thread  = num_chunks / num_threads;
    remaining   = num_chunks - per_thread * num_threads;

    *loops      = per_thread + (thread_num < remaining);
    *begin      = first + thread_num * chunksize * step;
    *end        = *begin + (chunksize - 1) * step;
    *next       = chunksize * num_threads * step;
    *lastchunk  = first + (num_chunks - 1) * chunksize * step;
    return 0;
}

static DWORD CDECL my_for_static_init_i8(LONG64 first, LONG64 last, LONG64 step, LONG64 chunksize, ULONG64 *loops,
                               LONG64 *begin, LONG64 *end, LONG64 *next, LONG64 *lastchunk)
{
    ULONG64 iterations, num_chunks, per_thread, remaining;
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();

    if (num_threads == 1 && chunksize != 1)
    {
        *loops      = 1;
        *begin      = first;
        *end        = last;
        *next       = 0;
        *lastchunk  = first;
        return 0;
    }

    if (first == last)
    {
        *loops = !thread_num;
        if (!thread_num)
        {
            /* The value in *next on Windows is either uninitialized, or contains
             * garbage. The value shouldn't matter for *loops <= 1, so no need to
             * reproduce that. */
            *begin      = first;
            *end        = last;
            *next       = 0;
            *lastchunk  = first;
        }
        return thread_num ? 0 : VCOMP_FOR_STATIC_BROKEN_NEXT;
    }

    if (step <= 0)
    {
        /* The total number of iterations depends on the number of threads here,
         * which doesn't make any sense. This is most likely a bug in the Windows
         * implementation. */
        return VCOMP_FOR_STATIC_BROKEN_LOOP;
    }

    if (first < last)
        iterations = 1 + (last - first) / step;
    else
    {
        iterations = 1 + (first - last) / step;
        step *= -1;
    }

    if (chunksize < 1)
        chunksize = 1;

    num_chunks  = iterations / chunksize;
    if (iterations % chunksize) num_chunks++;
    per_thread  = num_chunks / num_threads;
    remaining   = num_chunks - per_thread * num_threads;

    *loops      = per_thread + (thread_num < remaining);
    *begin      = first + thread_num * chunksize * step;
    *end        = *begin + (chunksize - 1) * step;
    *next       = chunksize * num_threads * step;
    *lastchunk  = first + (num_chunks - 1) * chunksize * step;
    return 0;
}

static void CDECL for_static_cb(void)
{
    static const struct
    {
        int first;
        int last;
        int step;
        int chunksize;
    }
    tests[] =
    {
        {           0,           0,           1,      1 }, /* 0 */
        {           0,           1,           1,      1 },
        {           0,           2,           1,      1 },
        {           0,           3,           1,      1 },
        {           0,         100,           1,      0 },
        {           0,         100,           1,      1 },
        {           0,         100,           1,      5 },
        {           0,         100,           1,     10 },
        {           0,         100,           1,     50 },
        {           0,         100,           1,    100 },
        {           0,         100,           1,    150 }, /* 10 */
        {           0,         100,           3,      0 },
        {           0,         100,           3,      1 },
        {           0,         100,           3,      5 },
        {           0,         100,           3,     10 },
        {           0,         100,           3,     50 },
        {           0,         100,           3,    100 },
        {           0,         100,           3,    150 },
        {           0,         100,           5,      1 },
        {           0,         100,          -3,      0 },
        {           0,         100,          -3,      1 }, /* 20 */
        {           0,         100,          -3,      5 },
        {           0,         100,          -3,     10 },
        {           0,         100,          -3,     50 },
        {           0,         100,          -3,    100 },
        {           0,         100,          -3,    150 },
        {           0,         100,          10,      1 },
        {           0,         100,          50,      1 },
        {           0,         100,         100,      1 },
        {           0,         100,         150,      1 },
        {           0,  0x10000000,           1,    123 }, /* 30 */
        {           0,  0x20000000,           1,    123 },
        {           0,  0x40000000,           1,    123 },
        {           0, -0x80000000,           1,    123 },
        {          50,          50,           1,      1 },
        {          50,          50,           1,      2 },
        {          50,          50,           1,     -1 },
        {          50,          50,           1,     -2 },
        {          50,          50,           2,      1 },
        {          50,          50,           3,      1 },
        {         100,         200,           3,      1 }, /* 40 */
        {         100,         200,           3,     -1 },
        {  0x7ffffffe, -0x80000000,           1,    123 },
        {  0x7fffffff, -0x80000000,           1,    123 },
    };
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        int my_begin, my_end, my_next, my_lastchunk;
        int begin, end, next, lastchunk;
        unsigned int my_loops, loops;
        DWORD broken_flags;

        my_loops = my_begin = my_end = my_next = my_lastchunk = 0xdeadbeef;
        loops = begin = end = next = lastchunk = 0xdeadbeef;
        broken_flags = my_for_static_init(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                          &my_loops, &my_begin, &my_end, &my_next, &my_lastchunk);
        p_vcomp_for_static_init(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                &loops, &begin, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %u\n",
               i, thread_num, num_threads, loops);
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %u, got %u\n",
               i, thread_num, num_threads, my_loops, loops);
            ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %d, got %d\n",
               i, thread_num, num_threads, my_begin, begin);
            ok(end == my_end, "test %d, thread %d/%d: expected end == %d, got %d\n",
               i, thread_num, num_threads, my_end, end);
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %d, got %d\n", i, thread_num, num_threads, my_next, next);
            ok(lastchunk == my_lastchunk, "test %d, thread %d/%d: expected lastchunk == %d, got %d\n",
               i, thread_num, num_threads, my_lastchunk, lastchunk);
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        loops = end = next = lastchunk = 0xdeadbeef;
        p_vcomp_for_static_init(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                &loops, NULL, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %u\n",
               i, thread_num, num_threads, loops);
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %u, got %u\n",
               i, thread_num, num_threads, my_loops, loops);
            ok(end == my_end, "test %d, thread %d/%d: expected end == %d, got %d\n",
               i, thread_num, num_threads, my_end, end);
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %d, got %d\n", i, thread_num, num_threads, my_next, next);
            ok(lastchunk == 0xdeadbeef, "test %d, thread %d/%d: expected lastchunk == 0xdeadbeef, got %d\n",
               i, thread_num, num_threads, lastchunk);
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        if (tests[i].first == tests[i].last) continue;

        my_loops = my_begin = my_end = my_next = my_lastchunk = 0xdeadbeef;
        loops = begin = end = next = lastchunk = 0xdeadbeef;
        broken_flags = my_for_static_init(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                          &my_loops, &my_begin, &my_end, &my_next, &my_lastchunk);
        p_vcomp_for_static_init(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                &loops, &begin, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %u\n",
               i, thread_num, num_threads, loops);
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %u, got %u\n",
               i, thread_num, num_threads, my_loops, loops);
            ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %d, got %d\n",
               i, thread_num, num_threads, my_begin, begin);
            ok(end == my_end, "test %d, thread %d/%d: expected end == %d, got %d\n",
               i, thread_num, num_threads, my_end, end);
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %d, got %d\n", i, thread_num, num_threads, my_next, next);
            ok(lastchunk == my_lastchunk, "test %d, thread %d/%d: expected lastchunk == %d, got %d\n",
               i, thread_num, num_threads, my_lastchunk, lastchunk);
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        loops = end = next = lastchunk = 0xdeadbeef;
        p_vcomp_for_static_init(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                &loops, NULL, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %u\n",
               i, thread_num, num_threads, loops);
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %u, got %u\n",
               i, thread_num, num_threads, my_loops, loops);
            ok(end == my_end, "test %d, thread %d/%d: expected end == %d, got %d\n",
               i, thread_num, num_threads, my_end, end);
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %d, got %d\n", i, thread_num, num_threads, my_next, next);
            ok(lastchunk == 0xdeadbeef, "test %d, thread %d/%d: expected lastchunk == 0xdeadbeef, got %d\n",
               i, thread_num, num_threads, lastchunk);
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();
    }
}

static void CDECL for_static_i8_cb(void)
{
    static const struct
    {
        LONG64 first;
        LONG64 last;
        LONG64 step;
        LONG64 chunksize;
    }
    tests[] =
    {
        {           0,           0,           1,      1 }, /* 0 */
        {           0,           1,           1,      1 },
        {           0,           2,           1,      1 },
        {           0,           3,           1,      1 },
        {           0,         100,           1,      0 },
        {           0,         100,           1,      1 },
        {           0,         100,           1,      5 },
        {           0,         100,           1,     10 },
        {           0,         100,           1,     50 },
        {           0,         100,           1,    100 },
        {           0,         100,           1,    150 }, /* 10 */
        {           0,         100,           3,      0 },
        {           0,         100,           3,      1 },
        {           0,         100,           3,      5 },
        {           0,         100,           3,     10 },
        {           0,         100,           3,     50 },
        {           0,         100,           3,    100 },
        {           0,         100,           3,    150 },
        {           0,         100,           5,      1 },
        {           0,         100,          -3,      0 },
        {           0,         100,          -3,      1 }, /* 20 */
        {           0,         100,          -3,      5 },
        {           0,         100,          -3,     10 },
        {           0,         100,          -3,     50 },
        {           0,         100,          -3,    100 },
        {           0,         100,          -3,    150 },
        {           0,         100,          10,      1 },
        {           0,         100,          50,      1 },
        {           0,         100,         100,      1 },
        {           0,         100,         150,      1 },
        {           0,  0x10000000,           1,    123 }, /* 30 */
        {           0,  0x20000000,           1,    123 },
        {           0,  0x40000000,           1,    123 },
        {           0, -0x80000000,           1,    123 },
        {          50,          50,           1,      1 },
        {          50,          50,           1,      2 },
        {          50,          50,           1,     -1 },
        {          50,          50,           1,     -2 },
        {          50,          50,           2,      1 },
        {          50,          50,           3,      1 },
        {         100,         200,           3,      1 }, /* 40 */
        {         100,         200,           3,     -1 },
        {  0x7ffffffe, -0x80000000,           1,    123 },
        {  0x7fffffff, -0x80000000,           1,    123 },
        { I64_MAX - 1,     I64_MIN,           1,    123 },
        {     I64_MAX,     I64_MIN,           1,    123 },
    };
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        LONG64 my_begin, my_end, my_next, my_lastchunk;
        LONG64 begin, end, next, lastchunk;
        ULONG64 my_loops, loops;
        DWORD broken_flags;

        my_loops = my_begin = my_end = my_next = my_lastchunk = -0xdeadbeef;
        loops = begin = end = next = lastchunk = -0xdeadbeef;
        broken_flags = my_for_static_init_i8(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                          &my_loops, &my_begin, &my_end, &my_next, &my_lastchunk);
        p_vcomp_for_static_init_i8(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                &loops, &begin, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(loops));
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_loops), wine_dbgstr_longlong(loops));
            ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
            ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %s, got %s\n", i, thread_num, num_threads,
               wine_dbgstr_longlong(my_next), wine_dbgstr_longlong(next));
            ok(lastchunk == my_lastchunk, "test %d, thread %d/%d: expected lastchunk == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_lastchunk), wine_dbgstr_longlong(lastchunk));
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        loops = end = next = lastchunk = -0xdeadbeef;
        p_vcomp_for_static_init_i8(tests[i].first, tests[i].last, tests[i].step, tests[i].chunksize,
                                &loops, NULL, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(loops));
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_loops), wine_dbgstr_longlong(loops));
            ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %s, got %s\n", i, thread_num, num_threads,
               wine_dbgstr_longlong(my_next), wine_dbgstr_longlong(next));
            ok(lastchunk == -0xdeadbeef, "test %d, thread %d/%d: expected lastchunk == -0xdeadbeef, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(lastchunk));
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        if (tests[i].first == tests[i].last) continue;

        my_loops = my_begin = my_end = my_next = my_lastchunk = -0xdeadbeef;
        loops = begin = end = next = lastchunk = -0xdeadbeef;
        broken_flags = my_for_static_init_i8(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                          &my_loops, &my_begin, &my_end, &my_next, &my_lastchunk);
        p_vcomp_for_static_init_i8(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                &loops, &begin, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(loops));
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_loops), wine_dbgstr_longlong(loops));
            ok(begin == my_begin, "test %d, thread %d/%d: expected begin == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_begin), wine_dbgstr_longlong(begin));
            ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %s, got %s\n", i, thread_num, num_threads,
               wine_dbgstr_longlong(my_next), wine_dbgstr_longlong(next));
            ok(lastchunk == my_lastchunk, "test %d, thread %d/%d: expected lastchunk == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_lastchunk), wine_dbgstr_longlong(lastchunk));
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();

        loops = end = next = lastchunk = -0xdeadbeef;
        p_vcomp_for_static_init_i8(tests[i].last, tests[i].first, tests[i].step, tests[i].chunksize,
                                &loops, NULL, &end, &next, &lastchunk);

        if (broken_flags & VCOMP_FOR_STATIC_BROKEN_LOOP)
        {
            ok(loops == 0 || loops == 1, "test %d, thread %d/%d: expected loops == 0 or 1, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(loops));
        }
        else
        {
            ok(loops == my_loops, "test %d, thread %d/%d: expected loops == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_loops), wine_dbgstr_longlong(loops));
            ok(end == my_end, "test %d, thread %d/%d: expected end == %s, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(my_end), wine_dbgstr_longlong(end));
            ok(next == my_next || broken(broken_flags & VCOMP_FOR_STATIC_BROKEN_NEXT),
               "test %d, thread %d/%d: expected next == %s, got %s\n", i, thread_num, num_threads,
               wine_dbgstr_longlong(my_next), wine_dbgstr_longlong(next));
            ok(lastchunk == -0xdeadbeef, "test %d, thread %d/%d: expected lastchunk == -0xdeadbeef, got %s\n",
               i, thread_num, num_threads, wine_dbgstr_longlong(lastchunk));
        }

        p_vcomp_for_static_end();
        p_vcomp_barrier();
    }
}

#undef VCOMP_FOR_STATIC_BROKEN_LOOP
#undef VCOMP_FOR_STATIC_BROKEN_NEXT

static void test_vcomp_for_static_init(void)
{
    int max_threads = pomp_get_max_threads();
    int i;

    for_static_cb();
    for_static_i8_cb();

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);
        p_vcomp_fork(TRUE, 0, for_static_cb);
        p_vcomp_fork(FALSE, 0, for_static_cb);
        p_vcomp_fork(TRUE, 0, for_static_i8_cb);
        p_vcomp_fork(FALSE, 0, for_static_i8_cb);
    }

    pomp_set_num_threads(max_threads);
}

static void CDECL for_dynamic_static_cb(void)
{
    unsigned int my_begin, my_end, begin, end;
    int ret;

    begin = end = 0xdeadbeef;
    my_for_static_simple_init(TRUE, 0, 1000, 7, TRUE, &my_begin, &my_end);
    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_STATIC | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 1);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == TRUE, "expected ret == TRUE, got %d\n", ret);
    ok(begin == my_begin, "expected begin == %u, got %u\n", my_begin, begin);
    ok(end == my_end, "expected end == %u, got %u\n", my_end, end);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == FALSE, "expected ret == FALSE, got %d\n", ret);

    begin = end = 0xdeadbeef;
    my_for_static_simple_init(TRUE, 1000, 0, 7, FALSE, &my_begin, &my_end);
    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_STATIC, 1000, 0, 7, 1);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == TRUE, "expected ret == TRUE, got %d\n", ret);
    ok(begin == my_begin, "expected begin == %u, got %u\n", my_begin, begin);
    ok(end == my_end, "expected end == %u, got %u\n", my_end, end);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == FALSE, "expected ret == FALSE, got %d\n", ret);

    begin = end = 0xdeadbeef;
    my_for_static_simple_init(TRUE, 0, 1000, 7, TRUE, &my_begin, &my_end);
    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_STATIC | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 5);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == TRUE, "expected ret == TRUE, got %d\n", ret);
    ok(begin == my_begin, "expected begin == %u, got %u\n", my_begin, begin);
    ok(end == my_end, "expected end == %u, got %u\n", my_end, end);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == FALSE, "expected ret == FALSE, got %d\n", ret);

    begin = end = 0xdeadbeef;
    my_for_static_simple_init(TRUE, 1000, 0, 7, FALSE, &my_begin, &my_end);
    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_STATIC, 1000, 0, 7, 5);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == TRUE, "expected ret == TRUE, got %d\n", ret);
    ok(begin == my_begin, "expected begin == %u, got %u\n", my_begin, begin);
    ok(end == my_end, "expected end == %u, got %u\n", my_end, end);
    ret = p_vcomp_for_dynamic_next(&begin, &end);
    ok(ret == FALSE, "expected ret == FALSE, got %d\n", ret);
}

static void CDECL for_dynamic_chunked_cb(LONG *a, LONG *b, LONG *c, LONG *d)
{
    unsigned int begin, end;

    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_CHUNKED | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 1);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        if (begin == 994) ok(end == 1000, "expected end == 1000, got %u\n", end);
        else ok(begin == end, "expected begin == end, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(a, begin);
    }

    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_CHUNKED, 1000, 0, 7, 1);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        if (begin == 6) ok(end == 0, "expected end == 0, got %u\n", end);
        else ok(begin == end, "expected begin == end, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(b, begin);
    }

    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_CHUNKED | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 5);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        if (begin == 980) ok(end == 1000, "expected end == 1000, got %u\n", end);
        else ok(begin + 28 == end, "expected begin + 28 == end, got %u and %u\n", begin + 28, end);
        InterlockedExchangeAdd(c, begin);
    }

    p_vcomp_for_dynamic_init(VCOMP_DYNAMIC_FLAGS_CHUNKED, 1000, 0, 7, 5);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        if (begin == 20) ok(end == 0, "expected end == 0, got %u\n", end);
        else ok(begin - 28 == end, "expected begin - 28 == end, got %u and %u\n", begin - 28, end);
        InterlockedExchangeAdd(d, begin);
    }
}

static void CDECL for_dynamic_guided_cb(unsigned int flags, LONG *a, LONG *b, LONG *c, LONG *d)
{
    int num_threads = pomp_get_num_threads();
    unsigned int begin, end;

    p_vcomp_for_dynamic_init(flags | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 1);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        ok(num_threads != 1 || (begin == 0 && end == 1000),
           "expected begin == 0 and end == 1000, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(a, begin);
    }

    p_vcomp_for_dynamic_init(flags, 1000, 0, 7, 1);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        ok(num_threads != 1 || (begin == 1000 && end == 0),
           "expected begin == 1000 and end == 0, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(b, begin);
    }

    p_vcomp_for_dynamic_init(flags | VCOMP_DYNAMIC_FLAGS_INCREMENT, 0, 1000, 7, 5);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        ok(num_threads != 1 || (begin == 0 && end == 1000),
           "expected begin == 0 and end == 1000, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(c, begin);
    }

    p_vcomp_for_dynamic_init(flags, 1000, 0, 7, 5);
    while (p_vcomp_for_dynamic_next(&begin, &end))
    {
        ok(num_threads != 1 || (begin == 1000 && end == 0),
           "expected begin == 1000 and end == 0, got %u and %u\n", begin, end);
        InterlockedExchangeAdd(d, begin);
    }
}

static void test_vcomp_for_dynamic_init(void)
{
    static const int guided_a[] = {0, 6041, 9072, 11179};
    static const int guided_b[] = {1000, 1959, 2928, 3821};
    static const int guided_c[] = {0, 4067, 6139, 7273};
    static const int guided_d[] = {1000, 1933, 2861, 3727};
    LONG a, b, c, d;
    int max_threads = pomp_get_max_threads();
    int i;

    /* test static scheduling */
    for_dynamic_static_cb();

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);
        p_vcomp_fork(TRUE, 0, for_dynamic_static_cb);
        p_vcomp_fork(FALSE, 0, for_dynamic_static_cb);
    }

    /* test chunked scheduling */
    a = b = c = d = 0;
    for_dynamic_chunked_cb(&a, &b, &c, &d);
    ok(a == 71071, "expected a == 71071, got %ld\n", a);
    ok(b == 71929, "expected b == 71929, got %ld\n", b);
    ok(c == 14210, "expected c == 14210, got %ld\n", c);
    ok(d == 14790, "expected d == 14790, got %ld\n", d);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = b = c = d = 0;
        p_vcomp_fork(TRUE, 4, for_dynamic_chunked_cb, &a, &b, &c, &d);
        ok(a == 71071, "expected a == 71071, got %ld\n", a);
        ok(b == 71929, "expected b == 71929, got %ld\n", b);
        ok(c == 14210, "expected c == 14210, got %ld\n", c);
        ok(d == 14790, "expected d == 14790, got %ld\n", d);

        a = b = c = d = 0;
        p_vcomp_fork(FALSE, 4, for_dynamic_chunked_cb, &a, &b, &c, &d);
        ok(a == 71071, "expected a == 71071, got %ld\n", a);
        ok(b == 71929, "expected b == 71929, got %ld\n", b);
        ok(c == 14210, "expected c == 14210, got %ld\n", c);
        ok(d == 14790, "expected d == 14790, got %ld\n", d);
    }

    /* test guided scheduling */
    a = b = c = d = 0;
    for_dynamic_guided_cb(VCOMP_DYNAMIC_FLAGS_GUIDED, &a, &b, &c, &d);
    ok(a == guided_a[0], "expected a == %d, got %ld\n", guided_a[0], a);
    ok(b == guided_b[0], "expected b == %d, got %ld\n", guided_b[0], b);
    ok(c == guided_c[0], "expected c == %d, got %ld\n", guided_c[0], c);
    ok(d == guided_d[0], "expected d == %d, got %ld\n", guided_d[0], d);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = b = c = d = 0;
        p_vcomp_fork(TRUE, 5, for_dynamic_guided_cb, VCOMP_DYNAMIC_FLAGS_GUIDED, &a, &b, &c, &d);
        ok(a == guided_a[i - 1], "expected a == %d, got %ld\n", guided_a[i - 1], a);
        ok(b == guided_b[i - 1], "expected b == %d, got %ld\n", guided_b[i - 1], b);
        ok(c == guided_c[i - 1], "expected c == %d, got %ld\n", guided_c[i - 1], c);
        ok(d == guided_d[i - 1], "expected d == %d, got %ld\n", guided_d[i - 1], d);

        a = b = c = d = 0;
        p_vcomp_fork(FALSE, 5, for_dynamic_guided_cb, VCOMP_DYNAMIC_FLAGS_GUIDED, &a, &b, &c, &d);
        ok(a == guided_a[0], "expected a == %d, got %ld\n", guided_a[0], a);
        ok(b == guided_b[0], "expected b == %d, got %ld\n", guided_b[0], b);
        ok(c == guided_c[0], "expected c == %d, got %ld\n", guided_c[0], c);
        ok(d == guided_d[0], "expected d == %d, got %ld\n", guided_d[0], d);
    }

    /* test with empty flags */
    a = b = c = d = 0;
    for_dynamic_guided_cb(0, &a, &b, &c, &d);
    ok(a == guided_a[0], "expected a == %d, got %ld\n", guided_a[0], a);
    ok(b == guided_b[0], "expected b == %d, got %ld\n", guided_b[0], b);
    ok(c == guided_c[0], "expected c == %d, got %ld\n", guided_c[0], c);
    ok(d == guided_d[0], "expected d == %d, got %ld\n", guided_d[0], d);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = b = c = d = 0;
        p_vcomp_fork(TRUE, 5, for_dynamic_guided_cb, 0, &a, &b, &c, &d);
        ok(a == guided_a[i - 1], "expected a == %d, got %ld\n", guided_a[i - 1], a);
        ok(b == guided_b[i - 1], "expected b == %d, got %ld\n", guided_b[i - 1], b);
        ok(c == guided_c[i - 1], "expected c == %d, got %ld\n", guided_c[i - 1], c);
        ok(d == guided_d[i - 1], "expected d == %d, got %ld\n", guided_d[i - 1], d);

        a = b = c = d = 0;
        p_vcomp_fork(FALSE, 5, for_dynamic_guided_cb, 0, &a, &b, &c, &d);
        ok(a == guided_a[0], "expected a == %d, got %ld\n", guided_a[0], a);
        ok(b == guided_b[0], "expected b == %d, got %ld\n", guided_b[0], b);
        ok(c == guided_c[0], "expected c == %d, got %ld\n", guided_c[0], c);
        ok(d == guided_d[0], "expected d == %d, got %ld\n", guided_d[0], d);
    }

    pomp_set_num_threads(max_threads);
}

static void CDECL master_cb(HANDLE semaphore)
{
    int num_threads = pomp_get_num_threads();
    int thread_num = pomp_get_thread_num();

    if (p_vcomp_master_begin())
    {
        ok(thread_num == 0, "expected thread_num == 0, got %d\n", thread_num);
        if (num_threads >= 2)
        {
            DWORD result = WaitForSingleObject(semaphore, 1000);
            ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
        }
        p_vcomp_master_end();
    }

    if (thread_num == 1)
        ReleaseSemaphore(semaphore, 1, NULL);
}

static void test_vcomp_master_begin(void)
{
    int max_threads = pomp_get_max_threads();
    HANDLE semaphore;
    int i;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    master_cb(semaphore);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);
        p_vcomp_fork(TRUE, 1, master_cb, semaphore);
        p_vcomp_fork(FALSE, 1, master_cb, semaphore);
    }

    CloseHandle(semaphore);
    pomp_set_num_threads(max_threads);
}

static void CDECL single_cb(int flags, HANDLE semaphore)
{
    int num_threads = pomp_get_num_threads();

    if (p_vcomp_single_begin(flags))
    {
        if (num_threads >= 2)
        {
            DWORD result = WaitForSingleObject(semaphore, 1000);
            ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
        }
    }
    p_vcomp_single_end();

    if (p_vcomp_single_begin(flags))
    {
        if (num_threads >= 2)
            ReleaseSemaphore(semaphore, 1, NULL);
    }
    p_vcomp_single_end();
}

static void test_vcomp_single_begin(void)
{
    int max_threads = pomp_get_max_threads();
    HANDLE semaphore;
    int i;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    single_cb(0, semaphore);
    single_cb(1, semaphore);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);
        p_vcomp_fork(TRUE, 2, single_cb, 0, semaphore);
        p_vcomp_fork(TRUE, 2, single_cb, 1, semaphore);
        p_vcomp_fork(FALSE, 2, single_cb, 0, semaphore);
        p_vcomp_fork(FALSE, 2, single_cb, 1, semaphore);
    }

    CloseHandle(semaphore);
    pomp_set_num_threads(max_threads);
}

static void CDECL critsect_cb(LONG *a)
{
    static CRITICAL_SECTION *critsect;
    LONG tmp;

    p_vcomp_enter_critsect(&critsect);
    tmp = *a;
    Sleep(50);
    *a = tmp + 1;
    p_vcomp_leave_critsect(critsect);

    ok(critsect != NULL, "expected critsect != NULL\n");

    EnterCriticalSection(critsect);
    tmp = *a;
    Sleep(50);
    *a = tmp + 1;
    LeaveCriticalSection(critsect);
}

static void test_vcomp_enter_critsect(void)
{
    int max_threads = pomp_get_max_threads();
    LONG a;
    int i;

    a = 0;
    critsect_cb(&a);
    ok(a == 2, "expected a == 2, got %ld\n", a);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = 0;
        p_vcomp_fork(TRUE, 1, critsect_cb, &a);
        ok(a == 2 * i, "expected a == %d, got %ld\n", 2 * i, a);

        a = 0;
        p_vcomp_fork(FALSE, 1, critsect_cb, &a);
        ok(a == 2, "expected a == 2, got %ld\n", a);
    }

    pomp_set_num_threads(max_threads);
}

static void test_vcomp_flush(void)
{
    p_vcomp_flush();
    p_vcomp_flush();
    p_vcomp_flush();
}

static void test_omp_init_lock(void)
{
    omp_lock_t lock;
    int ret;

    pomp_init_lock(&lock);

    /* test omp_set_lock */
    pomp_set_lock(&lock);
    pomp_unset_lock(&lock);

    /* test omp_test_lock */
    ret = pomp_test_lock(&lock);
    ok(ret == 1, "expected ret == 1, got %d\n", ret);
    ret = pomp_test_lock(&lock);
    ok(ret == 0, "expected ret == 0, got %d\n", ret);
    pomp_unset_lock(&lock);

    /* test with EnterCriticalSection */
    EnterCriticalSection(lock);
    ret = pomp_test_lock(&lock);
    todo_wine
    ok(ret == 1, "expected ret == 1, got %d\n", ret);
    if (ret)
    {
        ret = pomp_test_lock(&lock);
        ok(ret == 0, "expected ret == 0, got %d\n", ret);
        pomp_unset_lock(&lock);
    }
    LeaveCriticalSection(lock);

    pomp_destroy_lock(&lock);
}

static void test_omp_init_nest_lock(void)
{
    omp_nest_lock_t lock;
    int ret;

    ok(pomp_init_nest_lock == pomp_init_lock, "expected omp_init_nest_lock == %p, got %p\n",
       pomp_init_lock, pomp_init_nest_lock);
    ok(pomp_destroy_nest_lock == pomp_destroy_lock, "expected omp_destroy_nest_lock == %p, got %p\n",
       pomp_destroy_lock, pomp_destroy_nest_lock);

    pomp_init_nest_lock(&lock);

    /* test omp_set_nest_lock */
    pomp_set_nest_lock(&lock);
    pomp_set_nest_lock(&lock);
    pomp_unset_nest_lock(&lock);
    pomp_unset_nest_lock(&lock);

    /* test omp_test_nest_lock */
    ret = pomp_test_nest_lock(&lock);
    ok(ret == 1, "expected ret == 1, got %d\n", ret);
    ret = pomp_test_nest_lock(&lock);
    ok(ret == 2, "expected ret == 2, got %d\n", ret);
    ret = pomp_test_nest_lock(&lock);
    ok(ret == 3, "expected ret == 3, got %d\n", ret);
    pomp_unset_nest_lock(&lock);
    pomp_unset_nest_lock(&lock);
    pomp_unset_nest_lock(&lock);

    /* test with EnterCriticalSection */
    EnterCriticalSection(lock);
    ret = pomp_test_nest_lock(&lock);
    todo_wine
    ok(ret == 1, "expected ret == 1, got %d\n", ret);
    pomp_unset_nest_lock(&lock);
    LeaveCriticalSection(lock);

    pomp_destroy_nest_lock(&lock);
}

static void test_atomic_integer8(void)
{
    struct
    {
        void (CDECL *func)(char *, signed char);
        signed char v1, v2, expected;
    }
    tests1[] =
    {
        { p_vcomp_atomic_add_i1,  0x11,  0x77, -0x78 },
        { p_vcomp_atomic_and_i1,  0x11,  0x77,  0x11 },
        { p_vcomp_atomic_div_i1,  0x77,  0x11,     7 },
        { p_vcomp_atomic_div_i1,  0x77, -0x11,    -7 },
        { p_vcomp_atomic_mul_i1,  0x11,  0x77, -0x19 },
        { p_vcomp_atomic_mul_i1,  0x11, -0x77,  0x19 },
        { p_vcomp_atomic_or_i1,   0x11,  0x77,  0x77 },
        { p_vcomp_atomic_sub_i1,  0x11,  0x77, -0x66 },
        { p_vcomp_atomic_xor_i1,  0x11,  0x77,  0x66 },
    };
    struct
    {
        void (CDECL *func)(char *, unsigned int);
        signed char v1;
        unsigned int v2;
        signed char expected;
    }
    tests2[] =
    {
        { p_vcomp_atomic_shl_i1,  0x11,  3, -0x78 },
        { p_vcomp_atomic_shl_i1, -0x11,  3,  0x78 },
        { p_vcomp_atomic_shr_i1,  0x11,  3,     2 },
        { p_vcomp_atomic_shr_i1, -0x11,  3,    -3 },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shl_i1,  0x11, 11,     0 },
        { p_vcomp_atomic_shl_i1,  0x11, 19,     0 },
        { p_vcomp_atomic_shl_i1,  0x11, 35, -0x78 },
        { p_vcomp_atomic_shr_i1,  0x11, 11,     0 },
        { p_vcomp_atomic_shr_i1,  0x11, 19,     0 },
        { p_vcomp_atomic_shr_i1,  0x11, 35,     2 },
#endif
    };
    struct
    {
        void (CDECL *func)(unsigned char *, unsigned char);
        unsigned char v1, v2, expected;
    }
    tests3[] =
    {
        { p_vcomp_atomic_div_ui1, 0x77, 0x11, 7 },
        { p_vcomp_atomic_div_ui1, 0x77, 0xef, 0 },
    };
    struct
    {
        void (CDECL *func)(unsigned char *, unsigned int);
        unsigned char v1;
        unsigned int v2;
        unsigned char expected;
    }
    tests4[] =
    {
        { p_vcomp_atomic_shr_ui1, 0x11,  3,    2 },
        { p_vcomp_atomic_shr_ui1, 0xef,  3, 0x1d },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shr_ui1, 0x11, 11,    0 },
        { p_vcomp_atomic_shr_ui1, 0x11, 19,    0 },
        { p_vcomp_atomic_shr_ui1, 0x11, 35,    2 },
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests1); i++)
    {
        signed char val = tests1[i].v1;
        tests1[i].func((char *)&val, tests1[i].v2);
        ok(val == tests1[i].expected, "test %d: expected val == %d, got %d\n", i, tests1[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests2); i++)
    {
        signed char val = tests2[i].v1;
        tests2[i].func((char *)&val, tests2[i].v2);
        ok(val == tests2[i].expected, "test %d: expected val == %d, got %d\n", i, tests2[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests3); i++)
    {
        unsigned char val = tests3[i].v1;
        tests3[i].func(&val, tests3[i].v2);
        ok(val == tests3[i].expected, "test %d: expected val == %u, got %u\n", i, tests3[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests4); i++)
    {
        unsigned char val = tests4[i].v1;
        tests4[i].func(&val, tests4[i].v2);
        ok(val == tests4[i].expected, "test %d: expected val == %u, got %u\n", i, tests4[i].expected, val);
    }
}

static void test_atomic_integer16(void)
{
    struct
    {
        void (CDECL *func)(short *, short);
        short v1, v2, expected;
    }
    tests1[] =
    {
        { p_vcomp_atomic_add_i2,  0x1122,  0x7766, -0x7778 },
        { p_vcomp_atomic_and_i2,  0x1122,  0x7766,  0x1122 },
        { p_vcomp_atomic_div_i2,  0x7766,  0x1122,       6 },
        { p_vcomp_atomic_div_i2,  0x7766, -0x1122,      -6 },
        { p_vcomp_atomic_mul_i2,  0x1122,  0x7766, -0x5e74 },
        { p_vcomp_atomic_mul_i2,  0x1122, -0x7766,  0x5e74 },
        { p_vcomp_atomic_or_i2,   0x1122,  0x7766,  0x7766 },
        { p_vcomp_atomic_sub_i2,  0x1122,  0x7766, -0x6644 },
        { p_vcomp_atomic_xor_i2,  0x1122,  0x7766,  0x6644 },
    };
    struct
    {
        void (CDECL *func)(short *, unsigned int);
        short v1;
        unsigned int v2;
        short expected;
    }
    tests2[] =
    {
        { p_vcomp_atomic_shl_i2,  0x1122,  3, -0x76f0 },
        { p_vcomp_atomic_shl_i2, -0x1122,  3,  0x76f0 },
        { p_vcomp_atomic_shr_i2,  0x1122,  3,   0x224 },
        { p_vcomp_atomic_shr_i2, -0x1122,  3,  -0x225 },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shl_i2,  0x1122, 19,       0 },
        { p_vcomp_atomic_shl_i2,  0x1122, 35, -0x76f0 },
        { p_vcomp_atomic_shr_i2,  0x1122, 19,       0 },
        { p_vcomp_atomic_shr_i2,  0x1122, 35,   0x224 },
#endif
    };
    struct
    {
        void (CDECL *func)(unsigned short *, unsigned short);
        unsigned short v1, v2, expected;
    }
    tests3[] =
    {
        { p_vcomp_atomic_div_ui2, 0x7766, 0x1122, 6 },
        { p_vcomp_atomic_div_ui2, 0x7766, 0xeede, 0 },
    };
    struct
    {
        void (CDECL *func)(unsigned short *, unsigned int);
        unsigned short v1;
        unsigned int v2;
        unsigned short expected;
    }
    tests4[] =
    {
        { p_vcomp_atomic_shr_ui2, 0x1122,  3,  0x224 },
        { p_vcomp_atomic_shr_ui2, 0xeede,  3, 0x1ddb },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shr_ui2, 0x1122, 19,      0 },
        { p_vcomp_atomic_shr_ui2, 0x1122, 35,  0x224 },
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests1); i++)
    {
        short val = tests1[i].v1;
        tests1[i].func(&val, tests1[i].v2);
        ok(val == tests1[i].expected, "test %d: expected val == %d, got %d\n", i, tests1[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests2); i++)
    {
        short val = tests2[i].v1;
        tests2[i].func(&val, tests2[i].v2);
        ok(val == tests2[i].expected, "test %d: expected val == %d, got %d\n", i, tests2[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests3); i++)
    {
        unsigned short val = tests3[i].v1;
        tests3[i].func(&val, tests3[i].v2);
        ok(val == tests3[i].expected, "test %d: expected val == %u, got %u\n", i, tests3[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests4); i++)
    {
        unsigned short val = tests4[i].v1;
        tests4[i].func(&val, tests4[i].v2);
        ok(val == tests4[i].expected, "test %d: expected val == %u, got %u\n", i, tests4[i].expected, val);
    }
}

static void test_atomic_integer32(void)
{
    struct
    {
        void (CDECL *func)(int *, int);
        int v1, v2, expected;
    }
    tests1[] =
    {
        { p_vcomp_atomic_add_i4,  0x11223344,  0x77665544, -0x77777778 },
        { p_vcomp_atomic_and_i4,  0x11223344,  0x77665544,  0x11221144 },
        { p_vcomp_atomic_div_i4,  0x77665544,  0x11223344,           6 },
        { p_vcomp_atomic_div_i4,  0x77665544, -0x11223344,          -6 },
        { p_vcomp_atomic_mul_i4,  0x11223344,  0x77665544,  -0xecccdf0 },
        { p_vcomp_atomic_mul_i4,  0x11223344, -0x77665544,   0xecccdf0 },
        { p_vcomp_atomic_or_i4,   0x11223344,  0x77665544,  0x77667744 },
        { p_vcomp_atomic_shl_i4,  0x11223344,           3, -0x76ee65e0 },
        { p_vcomp_atomic_shl_i4, -0x11223344,           3,  0x76ee65e0 },
        { p_vcomp_atomic_shr_i4,  0x11223344,           3,   0x2244668 },
        { p_vcomp_atomic_shr_i4, -0x11223344,           3,  -0x2244669 },
        { p_vcomp_atomic_sub_i4,  0x11223344,  0x77665544, -0x66442200 },
        { p_vcomp_atomic_xor_i4,  0x11223344,  0x77665544,  0x66446600 },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shl_i4,  0x11223344,          35, -0x76ee65e0 },
        { p_vcomp_atomic_shr_i4,  0x11223344,          35,   0x2244668 },
#endif
    };
    struct
    {
        void (CDECL *func)(unsigned int *, unsigned int);
        unsigned int v1, v2, expected;
    }
    tests2[] =
    {
        { p_vcomp_atomic_div_ui4, 0x77665544, 0x11223344,          6 },
        { p_vcomp_atomic_div_ui4, 0x77665544, 0xeeddccbc,          0 },
        { p_vcomp_atomic_shr_ui4, 0x11223344,          3,  0x2244668 },
        { p_vcomp_atomic_shr_ui4, 0xeeddccbc,          3, 0x1ddbb997 },
#if defined(__i386__) || defined(__x86_64__)
        { p_vcomp_atomic_shr_ui4, 0x11223344,         35,  0x2244668 },
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests1); i++)
    {
        int val = tests1[i].v1;
        tests1[i].func(&val, tests1[i].v2);
        ok(val == tests1[i].expected, "test %d: expected val == %d, got %d\n", i, tests1[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests2); i++)
    {
        unsigned int val = tests2[i].v1;
        tests2[i].func(&val, tests2[i].v2);
        ok(val == tests2[i].expected, "test %d: expected val == %u, got %u\n", i, tests2[i].expected, val);
    }
}

static void test_atomic_integer64(void)
{
    struct
    {
        void (CDECL *func)(LONG64 *, LONG64);
        LONG64 v1, v2, expected;
    }
    tests1[] =
    {
        { p_vcomp_atomic_add_i8,  ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100), -ULL(0x77777777,0x77777778) },
        { p_vcomp_atomic_and_i8,  ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x11221144,0x11221100) },
        { p_vcomp_atomic_div_i8,  ULL(0x77665544,0x33221100),  ULL(0x11223344,0x55667788),                           6 },
        { p_vcomp_atomic_div_i8,  ULL(0x77665544,0x33221100), -ULL(0x11223344,0x55667788),                          -6 },
        { p_vcomp_atomic_mul_i8,  ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x3e963337,0xc6000800) },
        { p_vcomp_atomic_mul_i8,  ULL(0x11223344,0x55667788), -ULL(0x77665544,0x33221100),  ULL(0xc169ccc8,0x39fff800) },
        { p_vcomp_atomic_or_i8,   ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x77667744,0x77667788) },
        { p_vcomp_atomic_sub_i8,  ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100), -ULL(0x664421ff,0xddbb9978) },
        { p_vcomp_atomic_xor_i8,  ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x66446600,0x66446688) },
    };
    struct
    {
        void (CDECL *func)(LONG64 *, unsigned int);
        LONG64 v1;
        unsigned int v2;
        LONG64 expected;
        BOOL todo;
    }
    tests2[] =
    {
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788),  3, -ULL(0x76ee65dd,0x54cc43c0) },
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788), 60,  ULL(0x80000000,0x00000000) },
        { p_vcomp_atomic_shl_i8, -ULL(0x11223344,0x55667788),  3,  ULL(0x76ee65dd,0x54cc43c0) },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788),  3,  ULL(0x02244668,0x8aaccef1) },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788), 60,                           1 },
        { p_vcomp_atomic_shr_i8, -ULL(0x11223344,0x55667788),  3, -ULL(0x02244668,0x8aaccef1) },
#if defined(__i386__)
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788), 64,                           0, TRUE },
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788), 67,                           0, TRUE },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788), 64,                           0, TRUE },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788), 67,                           0, TRUE },
#elif defined(__x86_64__)
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788), 64,  ULL(0x11223344,0x55667788) },
        { p_vcomp_atomic_shl_i8,  ULL(0x11223344,0x55667788), 67, -ULL(0x76ee65dd,0x54cc43c0) },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788), 64,  ULL(0x11223344,0x55667788) },
        { p_vcomp_atomic_shr_i8,  ULL(0x11223344,0x55667788), 67,  ULL(0x02244668,0x8aaccef1) },
#endif
    };
    struct
    {
        void (CDECL *func)(ULONG64 *, ULONG64);
        ULONG64 v1, v2, expected;
    }
    tests3[] =
    {
        { p_vcomp_atomic_div_ui8, ULL(0x77665544,0x55667788), ULL(0x11223344,0x33221100), 6 },
        { p_vcomp_atomic_div_ui8, ULL(0x77665544,0x55667788), ULL(0xeeddccbb,0xaa998878), 0 },
    };
    struct
    {
        void (CDECL *func)(ULONG64 *, unsigned int);
        ULONG64 v1;
        unsigned int v2;
        ULONG64 expected;
        BOOL todo;
    }
    tests4[] =
    {
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788),  3, ULL(0x02244668,0x8aaccef1) },
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788), 60,                          1 },
        { p_vcomp_atomic_shr_ui8, ULL(0xeeddccbb,0xaa998878),  3, ULL(0x1ddbb997,0x7553310f) },
#if defined(__i386__)
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788), 64,                          0, TRUE },
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788), 67,                          0, TRUE },
#elif defined(__x86_64__)
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788), 64, ULL(0x11223344,0x55667788) },
        { p_vcomp_atomic_shr_ui8, ULL(0x11223344,0x55667788), 67, ULL(0x02244668,0x8aaccef1) },
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests1); i++)
    {
        LONG64 val = tests1[i].v1;
        tests1[i].func(&val, tests1[i].v2);
        ok(val == tests1[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
    for (i = 0; i < ARRAY_SIZE(tests2); i++)
    {
        LONG64 val = tests2[i].v1;
        tests2[i].func(&val, tests2[i].v2);
        todo_wine_if(tests2[i].todo)
        ok(val == tests2[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
    for (i = 0; i < ARRAY_SIZE(tests3); i++)
    {
        ULONG64 val = tests3[i].v1;
        tests3[i].func(&val, tests3[i].v2);
        ok(val == tests3[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
    for (i = 0; i < ARRAY_SIZE(tests4); i++)
    {
        ULONG64 val = tests4[i].v1;
        tests4[i].func(&val, tests4[i].v2);
        todo_wine_if(tests4[i].todo)
        ok(val == tests4[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
}

static void test_atomic_float(void)
{
    struct
    {
        void (CDECL *func)(float *, float);
        float v1, v2, expected;
    }
    tests[] =
    {
        { p_vcomp_atomic_add_r4, 42.0, 17.0, 42.0 + 17.0 },
        { p_vcomp_atomic_div_r4, 42.0, 17.0, 42.0 / 17.0 },
        { p_vcomp_atomic_mul_r4, 42.0, 17.0, 42.0 * 17.0 },
        { p_vcomp_atomic_sub_r4, 42.0, 17.0, 42.0 - 17.0 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        float val = tests[i].v1;
        tests[i].func(&val, tests[i].v2);
        ok(tests[i].expected - 0.001 < val && val < tests[i].expected + 0.001,
           "test %d: expected val == %f, got %f\n", i, tests[i].expected, val);
    }
}

static void test_atomic_double(void)
{
    struct
    {
        void (CDECL *func)(double *, double);
        double v1, v2, expected;
    }
    tests[] =
    {
        { p_vcomp_atomic_add_r8, 42.0, 17.0, 42.0 + 17.0 },
        { p_vcomp_atomic_div_r8, 42.0, 17.0, 42.0 / 17.0 },
        { p_vcomp_atomic_mul_r8, 42.0, 17.0, 42.0 * 17.0 },
        { p_vcomp_atomic_sub_r8, 42.0, 17.0, 42.0 - 17.0 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        double val = tests[i].v1;
        tests[i].func(&val, tests[i].v2);
        ok(tests[i].expected - 0.001 < val && val < tests[i].expected + 0.001,
           "test %d: expected val == %f, got %f\n", i, tests[i].expected, val);
    }
}

static void test_reduction_integer8(void)
{
    static const struct
    {
        unsigned int flags;
        char v1, v2, expected;
    }
    tests[] =
    {
        { 0x000,                            0x11,  0x77, -0x78 },
        { VCOMP_REDUCTION_FLAGS_ADD,        0x11,  0x77, -0x78 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x11,  0x77, -0x19 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x11, -0x77,  0x19 },
        { VCOMP_REDUCTION_FLAGS_AND,        0x11,  0x77,  0x11 },
        { VCOMP_REDUCTION_FLAGS_OR,         0x11,  0x77,  0x77 },
        { VCOMP_REDUCTION_FLAGS_XOR,        0x11,  0x77,  0x66 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,      1,     2,     1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,       0,     2,     1 },
        { 0x800,                               0,     2,     1 },
        { 0x900,                               0,     2,     1 },
        { 0xa00,                               0,     2,     1 },
        { 0xb00,                               0,     2,     1 },
        { 0xc00,                               0,     2,     1 },
        { 0xd00,                               0,     2,     1 },
        { 0xe00,                               0,     2,     1 },
        { 0xf00,                               0,     2,     1 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        char val = tests[i].v1;
        p_vcomp_reduction_i1(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: expected val == %d, got %d\n", i, tests[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        unsigned char val = tests[i].v1;
        p_vcomp_reduction_u1(tests[i].flags, &val, tests[i].v2);
        ok(val == (unsigned char)tests[i].expected,
           "test %d: expected val == %u, got %u\n", i, (unsigned char)tests[i].expected, val);
    }
}

static void test_reduction_integer16(void)
{
    static const struct
    {
        unsigned int flags;
        short v1, v2, expected;
    }
    tests[] =
    {
        { 0x000,                            0x1122,  0x7766, -0x7778 },
        { VCOMP_REDUCTION_FLAGS_ADD,        0x1122,  0x7766, -0x7778 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x1122,  0x7766, -0x5e74 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x1122, -0x7766,  0x5e74 },
        { VCOMP_REDUCTION_FLAGS_AND,        0x1122,  0x7766,  0x1122 },
        { VCOMP_REDUCTION_FLAGS_OR,         0x1122,  0x7766,  0x7766 },
        { VCOMP_REDUCTION_FLAGS_XOR,        0x1122,  0x7766,  0x6644 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,        1,       2,       1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,         0,       2,       1 },
        { 0x800,                                 0,       2,       1 },
        { 0x900,                                 0,       2,       1 },
        { 0xa00,                                 0,       2,       1 },
        { 0xb00,                                 0,       2,       1 },
        { 0xc00,                                 0,       2,       1 },
        { 0xd00,                                 0,       2,       1 },
        { 0xe00,                                 0,       2,       1 },
        { 0xf00,                                 0,       2,       1 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        short val = tests[i].v1;
        p_vcomp_reduction_i2(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: expected val == %d, got %d\n", i, tests[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        unsigned short val = tests[i].v1;
        p_vcomp_reduction_u2(tests[i].flags, &val, tests[i].v2);
        ok(val == (unsigned short)tests[i].expected,
           "test %d: expected val == %u, got %u\n", i, (unsigned short)tests[i].expected, val);
    }
}

static void CDECL reduction_cb(int *a, int *b)
{
    p_vcomp_reduction_i4(VCOMP_REDUCTION_FLAGS_ADD, a, 1);
    p_vcomp_reduction_i4(VCOMP_REDUCTION_FLAGS_ADD | 0xfffff0ff, b, 1);
}

static void test_reduction_integer32(void)
{
    static const struct
    {
        unsigned int flags;
        int v1, v2, expected;
    }
    tests[] =
    {
        { 0x000,                            0x11223344,  0x77665544, -0x77777778 },
        { VCOMP_REDUCTION_FLAGS_ADD,        0x11223344,  0x77665544, -0x77777778 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x11223344,  0x77665544,  -0xecccdf0 },
        { VCOMP_REDUCTION_FLAGS_MUL,        0x11223344, -0x77665544,   0xecccdf0 },
        { VCOMP_REDUCTION_FLAGS_AND,        0x11223344,  0x77665544,  0x11221144 },
        { VCOMP_REDUCTION_FLAGS_OR,         0x11223344,  0x77665544,  0x77667744 },
        { VCOMP_REDUCTION_FLAGS_XOR,        0x11223344,  0x77665544,  0x66446600 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            0,           0,           0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            0,           2,           0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            1,           0,           0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            1,           2,           1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            2,           0,           0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,            2,           2,           1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             0,           0,           0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             0,           2,           1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             1,           0,           1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             1,           2,           1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             2,           0,           2 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,             2,           2,           2 },
        { 0x800,                                     0,           2,           1 },
        { 0x900,                                     0,           2,           1 },
        { 0xa00,                                     0,           2,           1 },
        { 0xb00,                                     0,           2,           1 },
        { 0xc00,                                     0,           2,           1 },
        { 0xd00,                                     0,           2,           1 },
        { 0xe00,                                     0,           2,           1 },
        { 0xf00,                                     0,           2,           1 },
    };
    int max_threads = pomp_get_max_threads();
    int a, b, i;

    a = b = 42;
    reduction_cb(&a, &b);
    ok(a == 43, "expected a == 43, got %d\n", a);
    ok(b == 43, "expected b == 43, got %d\n", b);

    for (i = 1; i <= 4; i++)
    {
        pomp_set_num_threads(i);

        a = b = 42;
        p_vcomp_fork(TRUE, 2, reduction_cb, &a, &b);
        ok(a == 42 + i, "expected a == %d, got %d\n", 42 + i, a);
        ok(b == 42 + i, "expected b == %d, got %d\n", 42 + i, b);

        a = b = 42;
        p_vcomp_fork(FALSE, 2, reduction_cb, &a, &b);
        ok(a == 43, "expected a == 43, got %d\n", a);
        ok(b == 43, "expected b == 43, got %d\n", b);
    }

    pomp_set_num_threads(max_threads);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        int val = tests[i].v1;
        p_vcomp_reduction_i4(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: expected val == %d, got %d\n", i, tests[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        unsigned int val = tests[i].v1;
        p_vcomp_reduction_u4(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: expected val == %u, got %u\n", i, tests[i].expected, val);
    }
}

static void test_reduction_integer64(void)
{
    static const struct
    {
        unsigned int flags;
        LONG64 v1, v2, expected;
    }
    tests[] =
    {
        { 0x000,                            ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100), -ULL(0x77777777,0x77777778) },
        { VCOMP_REDUCTION_FLAGS_ADD,        ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100), -ULL(0x77777777,0x77777778) },
        { VCOMP_REDUCTION_FLAGS_MUL,        ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x3e963337,0xc6000800) },
        { VCOMP_REDUCTION_FLAGS_MUL,        ULL(0x11223344,0x55667788), -ULL(0x77665544,0x33221100),  ULL(0xc169ccc8,0x39fff800) },
        { VCOMP_REDUCTION_FLAGS_AND,        ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x11221144,0x11221100) },
        { VCOMP_REDUCTION_FLAGS_OR,         ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x77667744,0x77667788) },
        { VCOMP_REDUCTION_FLAGS_XOR,        ULL(0x11223344,0x55667788),  ULL(0x77665544,0x33221100),  ULL(0x66446600,0x66446688) },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,   1, 2, 1 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,    0, 2, 1 },
        { 0x800,                            0, 2, 1 },
        { 0x900,                            0, 2, 1 },
        { 0xa00,                            0, 2, 1 },
        { 0xb00,                            0, 2, 1 },
        { 0xc00,                            0, 2, 1 },
        { 0xd00,                            0, 2, 1 },
        { 0xe00,                            0, 2, 1 },
        { 0xf00,                            0, 2, 1 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        LONG64 val = tests[i].v1;
        p_vcomp_reduction_i8(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ULONG64 val = tests[i].v1;
        p_vcomp_reduction_u8(tests[i].flags, &val, tests[i].v2);
        ok(val == tests[i].expected, "test %d: unexpectedly got %s\n", i, wine_dbgstr_longlong(val));
    }
}

static void test_reduction_float_double(void)
{
    static const struct
    {
        unsigned int flags;
        float v1, v2, expected;
    }
    tests[] =
    {
        { 0x000,                            42.0,   17.0, 42.0 + 17.0 },
        { VCOMP_REDUCTION_FLAGS_ADD,        42.0,   17.0, 42.0 + 17.0 },
        { VCOMP_REDUCTION_FLAGS_MUL,        42.0,   17.0, 42.0 * 17.0 },
        { 0x300,                             0.0,    2.0,         1.0 },
        { 0x400,                             0.0,    2.0,         1.0 },
        { 0x500,                             0.0,    2.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,   -0.0,    1.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    0.0,    0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    0.0,    2.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    1.0,   -0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    1.0,    0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    1.0, 1.0e-5,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    1.0,    2.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    2.0,    0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_AND,    2.0,    2.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,    -0.0,    0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     0.0,   -0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     0.0,    0.0,         0.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     0.0, 1.0e-5,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     0.0,    2.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     1.0,    0.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     1.0,    2.0,         1.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     2.0,    0.0,         2.0 },
        { VCOMP_REDUCTION_FLAGS_BOOL_OR,     2.0,    2.0,         2.0 },
        { 0x800,                             0.0,    2.0,         1.0 },
        { 0x900,                             0.0,    2.0,         1.0 },
        { 0xa00,                             0.0,    2.0,         1.0 },
        { 0xb00,                             0.0,    2.0,         1.0 },
        { 0xc00,                             0.0,    2.0,         1.0 },
        { 0xd00,                             0.0,    2.0,         1.0 },
        { 0xe00,                             0.0,    2.0,         1.0 },
        { 0xf00,                             0.0,    2.0,         1.0 },
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        float val = tests[i].v1;
        p_vcomp_reduction_r4(tests[i].flags, &val, tests[i].v2);
        ok(tests[i].expected - 0.001 < val && val < tests[i].expected + 0.001,
           "test %d: expected val == %f, got %f\n", i, tests[i].expected, val);
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        double val = tests[i].v1;
        p_vcomp_reduction_r8(tests[i].flags, &val, tests[i].v2);
        ok(tests[i].expected - 0.001 < val && val < tests[i].expected + 0.001,
           "test %d: expected val == %f, got %f\n", i, tests[i].expected, val);
    }
}

static void test_omp_get_num_procs(void)
{
    SYSTEM_INFO sysinfo;
    int num_procs;

    num_procs = pomp_get_num_procs();
    ok(num_procs > 0, "expected non-zero num_procs\n");
    GetSystemInfo(&sysinfo);
    ok(sysinfo.dwNumberOfProcessors > 0, "expected non-zero dwNumberOfProcessors\n");
    ok(num_procs == sysinfo.dwNumberOfProcessors, "got dwNumberOfProcessors %ld num_procs %d\n", sysinfo.dwNumberOfProcessors, num_procs);
}

START_TEST(vcomp)
{
    if (!init_vcomp())
        return;

    test_omp_get_num_procs();
    test_omp_get_num_threads(FALSE);
    test_omp_get_num_threads(TRUE);
    test_vcomp_fork();
    test_vcomp_sections_init();
    test_vcomp_for_static_simple_init();
    test_vcomp_for_static_init();
    test_vcomp_for_dynamic_init();
    test_vcomp_master_begin();
    test_vcomp_single_begin();
    test_vcomp_enter_critsect();
    test_vcomp_flush();
    test_omp_init_lock();
    test_omp_init_nest_lock();
    test_atomic_integer8();
    test_atomic_integer16();
    test_atomic_integer32();
    test_atomic_integer64();
    test_atomic_float();
    test_atomic_double();
    test_reduction_integer8();
    test_reduction_integer16();
    test_reduction_integer32();
    test_reduction_integer64();
    test_reduction_float_double();

    release_vcomp();
}
