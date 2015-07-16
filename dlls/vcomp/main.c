/*
 *
 * vcomp implementation
 *
 * Copyright 2011 Austin English
 * Copyright 2012 Dan Kegel
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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vcomp);

static DWORD   vcomp_context_tls = TLS_OUT_OF_INDEXES;
static int     vcomp_max_threads;
static int     vcomp_num_threads;
static BOOL    vcomp_nested_fork = FALSE;

struct vcomp_thread_data
{
    int                     thread_num;
    int                     fork_threads;
};

#if defined(__i386__)

extern void CDECL _vcomp_fork_call_wrapper(void *wrapper, int nargs, __ms_va_list args);
__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-4\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-8\n\t")
                   "movl 12(%ebp),%edx\n\t"
                   "movl %esp,%edi\n\t"
                   "shll $2,%edx\n\t"
                   "jz 1f\n\t"
                   "subl %edx,%edi\n\t"
                   "andl $~15,%edi\n\t"
                   "movl %edi,%esp\n\t"
                   "movl 12(%ebp),%ecx\n\t"
                   "movl 16(%ebp),%esi\n\t"
                   "cld\n\t"
                   "rep; movsl\n"
                   "1:\tcall *8(%ebp)\n\t"
                   "leal -8(%ebp),%esp\n\t"
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )

#elif defined(__x86_64__)

extern void CDECL _vcomp_fork_call_wrapper(void *wrapper, int nargs, __ms_va_list args);
__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   "movq %rcx,%rax\n\t"
                   "movq $4,%rcx\n\t"
                   "cmp %rcx,%rdx\n\t"
                   "cmovgq %rdx,%rcx\n\t"
                   "leaq 0(,%rcx,8),%rdx\n\t"
                   "subq %rdx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %r8,%rsi\n\t"
                   "rep; movsq\n\t"
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")

#else

static void CDECL _vcomp_fork_call_wrapper(void *wrapper, int nargs, __ms_va_list args)
{
    ERR("Not implemented for this architecture\n");
}

#endif

static inline struct vcomp_thread_data *vcomp_get_thread_data(void)
{
    return (struct vcomp_thread_data *)TlsGetValue(vcomp_context_tls);
}

static inline void vcomp_set_thread_data(struct vcomp_thread_data *thread_data)
{
    TlsSetValue(vcomp_context_tls, thread_data);
}

static struct vcomp_thread_data *vcomp_init_thread_data(void)
{
    struct vcomp_thread_data *thread_data = vcomp_get_thread_data();
    if (thread_data) return thread_data;

    if (!(thread_data = HeapAlloc(GetProcessHeap(), 0, sizeof(*thread_data))))
    {
        ERR("could not create thread data\n");
        ExitProcess(1);
    }

    thread_data->thread_num = 0;
    thread_data->fork_threads = 0;

    vcomp_set_thread_data(thread_data);
    return thread_data;
}

static void vcomp_free_thread_data(void)
{
    struct vcomp_thread_data *thread_data = vcomp_get_thread_data();
    if (!thread_data) return;

    HeapFree(GetProcessHeap(), 0, thread_data);
    vcomp_set_thread_data(NULL);
}

int CDECL omp_get_dynamic(void)
{
    TRACE("stub\n");
    return 0;
}

int CDECL omp_get_max_threads(void)
{
    TRACE("()\n");
    return vcomp_max_threads;
}

int CDECL omp_get_nested(void)
{
    TRACE("stub\n");
    return vcomp_nested_fork;
}

int CDECL omp_get_num_procs(void)
{
    TRACE("stub\n");
    return 1;
}

int CDECL omp_get_num_threads(void)
{
    TRACE("stub\n");
    return 1;
}

int CDECL omp_get_thread_num(void)
{
    TRACE("()\n");
    return vcomp_init_thread_data()->thread_num;
}

/* Time in seconds since "some time in the past" */
double CDECL omp_get_wtime(void)
{
    return GetTickCount() / 1000.0;
}

void CDECL omp_set_dynamic(int val)
{
    TRACE("(%d): stub\n", val);
}

void CDECL omp_set_nested(int nested)
{
    TRACE("(%d)\n", nested);
    vcomp_nested_fork = (nested != 0);
}

void CDECL omp_set_num_threads(int num_threads)
{
    TRACE("(%d)\n", num_threads);
    if (num_threads >= 1)
        vcomp_num_threads = num_threads;
}

void CDECL _vcomp_barrier(void)
{
    TRACE("stub\n");
}

void CDECL _vcomp_set_num_threads(int num_threads)
{
    TRACE("(%d)\n", num_threads);
    if (num_threads >= 1)
        vcomp_init_thread_data()->fork_threads = num_threads;
}

int CDECL _vcomp_single_begin(int flags)
{
    TRACE("(%x): stub\n", flags);
    return TRUE;
}

void CDECL _vcomp_single_end(void)
{
    TRACE("stub\n");
}

void WINAPIV _vcomp_fork(BOOL ifval, int nargs, void *wrapper, ...)
{
    __ms_va_list valist;

    TRACE("(%d, %d, %p, ...)\n", ifval, nargs, wrapper);

    __ms_va_start(valist, wrapper);
    _vcomp_fork_call_wrapper(wrapper, nargs, valist);
    __ms_va_end(valist);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */

        case DLL_PROCESS_ATTACH:
        {
            SYSTEM_INFO sysinfo;

            if ((vcomp_context_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES)
            {
                ERR("Failed to allocate TLS index\n");
                return FALSE;
            }

            GetSystemInfo(&sysinfo);
            vcomp_max_threads = sysinfo.dwNumberOfProcessors;
            vcomp_num_threads = sysinfo.dwNumberOfProcessors;
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (reserved) break;
            if (vcomp_context_tls != TLS_OUT_OF_INDEXES)
            {
                vcomp_free_thread_data();
                TlsFree(vcomp_context_tls);
            }
            break;
        }

        case DLL_THREAD_DETACH:
        {
            vcomp_free_thread_data();
            break;
        }
    }

    return TRUE;
}
