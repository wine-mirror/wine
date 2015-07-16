/*
 * Unit test suite for vcomp
 *
 * Copyright 2015 Sebastian Lackner
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

#include "wine/test.h"

static char         vcomp_manifest_file[MAX_PATH];
static HANDLE       vcomp_actctx_hctx;
static ULONG_PTR    vcomp_actctx_cookie;
static HMODULE      vcomp_handle;

static HANDLE (WINAPI *pCreateActCtxA)(ACTCTXA*);
static BOOL   (WINAPI *pActivateActCtx)(HANDLE, ULONG_PTR*);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD, ULONG_PTR);
static VOID   (WINAPI *pReleaseActCtx)(HANDLE);

static void  (CDECL   *p_vcomp_barrier)(void);
static void  (WINAPIV *p_vcomp_fork)(BOOL ifval, int nargs, void *wrapper, ...);
static void  (CDECL   *p_vcomp_set_num_threads)(int num_threads);
static int   (CDECL   *pomp_get_max_threads)(void);
static int   (CDECL   *pomp_get_nested)(void);
static int   (CDECL   *pomp_get_num_threads)(void);
static int   (CDECL   *pomp_get_thread_num)(void);
static void  (CDECL   *pomp_set_nested)(int nested);
static void  (CDECL   *pomp_set_num_threads)(int num_threads);

#ifdef __i386__
#define ARCH "x86"
#elif defined(__x86_64__)
#define ARCH "amd64"
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
    HMODULE kernel32;
    DWORD written;
    ACTCTXA ctx;
    HANDLE file;

    kernel32 = GetModuleHandleA("kernel32.dll");
    pCreateActCtxA      = (void *)GetProcAddress(kernel32, "CreateActCtxA");
    pActivateActCtx     = (void *)GetProcAddress(kernel32, "ActivateActCtx");
    pDeactivateActCtx   = (void *)GetProcAddress(kernel32, "DeactivateActCtx");
    pReleaseActCtx      = (void *)GetProcAddress(kernel32, "ReleaseActCtx");
    if (!pCreateActCtxA) return;

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
    vcomp_actctx_hctx = pCreateActCtxA(&ctx);
    if (!vcomp_actctx_hctx)
    {
        ok(0, "failed to create activation context\n");
        DeleteFileA(vcomp_manifest_file);
        return;
    }

    if (!pActivateActCtx(vcomp_actctx_hctx, &vcomp_actctx_cookie))
    {
        win_skip("failed to activate context\n");
        pReleaseActCtx(vcomp_actctx_hctx);
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
        pDeactivateActCtx(0, vcomp_actctx_cookie);
        pReleaseActCtx(vcomp_actctx_hctx);
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

    VCOMP_GET_PROC(_vcomp_barrier);
    VCOMP_GET_PROC(_vcomp_fork);
    VCOMP_GET_PROC(_vcomp_set_num_threads);
    VCOMP_GET_PROC(omp_get_max_threads);
    VCOMP_GET_PROC(omp_get_nested);
    VCOMP_GET_PROC(omp_get_num_threads);
    VCOMP_GET_PROC(omp_get_thread_num);
    VCOMP_GET_PROC(omp_set_nested);
    VCOMP_GET_PROC(omp_set_num_threads);

    return TRUE;
}

#undef VCOMP_GET_PROC

static void CDECL num_threads_cb2(LONG *count)
{
    InterlockedIncrement(count);
}

static void CDECL num_threads_cb(BOOL nested, int nested_threads, LONG *count)
{
    int num_threads, thread_num;
    LONG thread_count;

    InterlockedIncrement(count);
    p_vcomp_barrier();

    num_threads = pomp_get_num_threads();
    ok(num_threads == *count, "expected num_threads == %d, got %d\n", *count, num_threads);
    thread_num = pomp_get_thread_num();
    ok(thread_num >= 0 && thread_num < num_threads,
       "expected thread_num in range [0, %d], got %d\n", num_threads - 1, thread_num);

    thread_count = 0;
    p_vcomp_fork(TRUE, 1, num_threads_cb2, &thread_count);
    if (nested)
        ok(thread_count == nested_threads, "expected %d thread, got %d\n", nested_threads, thread_count);
    else
        ok(thread_count == 1, "expected 1 thread, got %d\n", thread_count);

    p_vcomp_set_num_threads(4);
    thread_count = 0;
    p_vcomp_fork(TRUE, 1, num_threads_cb2, &thread_count);
    if (nested)
        ok(thread_count == 4 , "expected 4 thread, got %d\n", thread_count);
    else
        ok(thread_count == 1 , "expected 1 thread, got %d\n", thread_count);
}

static void test_omp_get_num_threads(BOOL nested)
{
    int is_nested, max_threads, num_threads, thread_num;
    LONG thread_count;

    pomp_set_nested(nested);
    is_nested = pomp_get_nested();
    ok(is_nested == nested, "expected %d, got %d\n", nested, is_nested);

    max_threads = pomp_get_max_threads();
    ok(max_threads >= 1, "expected max_threads >= 1, got %d\n", max_threads);
    thread_num = pomp_get_thread_num();
    ok(thread_num == 0, "expected thread_num == 0, got %d\n", thread_num);

    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, max_threads, &thread_count);
    ok(thread_count == max_threads, "expected %d threads, got %d\n", max_threads, thread_count);

    pomp_set_num_threads(2);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 2, &thread_count);
    ok(thread_count == 2, "expected 2 threads, got %d\n", thread_count);

    pomp_set_num_threads(4);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %d\n", thread_count);

    p_vcomp_set_num_threads(8);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 4, &thread_count);
    ok(thread_count == 8, "expected 8 threads, got %d\n", thread_count);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %d\n", thread_count);

    p_vcomp_set_num_threads(0);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %d\n", thread_count);

    pomp_set_num_threads(0);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "expected num_threads == 1, got %d\n", num_threads);
    thread_count = 0;
    p_vcomp_fork(TRUE, 3, num_threads_cb, nested, 4, &thread_count);
    ok(thread_count == 4, "expected 4 threads, got %d\n", thread_count);

    pomp_set_num_threads(max_threads);
    pomp_set_nested(FALSE);
}

START_TEST(vcomp)
{
    if (!init_vcomp())
        return;

    test_omp_get_num_threads(FALSE);
    test_omp_get_num_threads(TRUE);

    release_vcomp();
}
