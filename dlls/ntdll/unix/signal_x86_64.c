/*
 * x86-64 signal handling routines
 *
 * Copyright 1999, 2005 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#ifdef __x86_64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_MACHINE_SYSARCH_H
# include <machine/sysarch.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif
#ifdef HAVE_LIBUNWIND
# define UNW_LOCAL_ONLY
# include <libunwind.h>
#endif
#ifdef __APPLE__
# include <mach/mach.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#include <asm/prctl.h>
static inline int arch_prctl( int func, void *ptr ) { return syscall( __NR_arch_prctl, func, ptr ); }

#endif

enum i386_trap_code
{
    TRAP_x86_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
    TRAP_x86_DIVIDE     = 0,   /* Division by zero exception */
    TRAP_x86_TRCTRAP    = 1,   /* Single-step exception */
    TRAP_x86_NMI        = 2,   /* NMI interrupt */
    TRAP_x86_BPTFLT     = 3,   /* Breakpoint exception */
    TRAP_x86_OFLOW      = 4,   /* Overflow exception */
    TRAP_x86_BOUND      = 5,   /* Bound range exception */
    TRAP_x86_PRIVINFLT  = 6,   /* Invalid opcode exception */
    TRAP_x86_DNA        = 7,   /* Device not available exception */
    TRAP_x86_DOUBLEFLT  = 8,   /* Double fault exception */
    TRAP_x86_FPOPFLT    = 9,   /* Coprocessor segment overrun */
    TRAP_x86_TSSFLT     = 10,  /* Invalid TSS exception */
    TRAP_x86_SEGNPFLT   = 11,  /* Segment not present exception */
    TRAP_x86_STKFLT     = 12,  /* Stack fault */
    TRAP_x86_PROTFLT    = 13,  /* General protection fault */
    TRAP_x86_PAGEFLT    = 14,  /* Page fault */
    TRAP_x86_ARITHTRAP  = 16,  /* Floating point exception */
    TRAP_x86_ALIGNFLT   = 17,  /* Alignment check exception */
    TRAP_x86_MCHK       = 18,  /* Machine check exception */
    TRAP_x86_CACHEFLT   = 19   /* Cache flush exception */
};

static const size_t teb_size = 0x2000;  /* we reserve two pages for the TEB */


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS CDECL get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *             signal_init_threading
 */
void signal_init_threading(void)
{
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}

#ifdef __APPLE__
/**********************************************************************
 *		mac_thread_gsbase
 */
static void *mac_thread_gsbase(void)
{
    struct thread_identifier_info tiinfo;
    unsigned int info_count = THREAD_IDENTIFIER_INFO_COUNT;
    static int gsbase_offset = -1;

    kern_return_t kr = thread_info(mach_thread_self(), THREAD_IDENTIFIER_INFO, (thread_info_t) &tiinfo, &info_count);
    if (kr == KERN_SUCCESS) return (void*)tiinfo.thread_handle;

    if (gsbase_offset < 0)
    {
        /* Search for the array of TLS slots within the pthread data structure.
           That's what the macOS pthread implementation uses for gsbase. */
        const void* const sentinel1 = (const void*)0x2bffb6b4f11228ae;
        const void* const sentinel2 = (const void*)0x0845a7ff6ab76707;
        int rc;
        pthread_key_t key;
        const void** p = (const void**)pthread_self();
        int i;

        gsbase_offset = 0;
        if ((rc = pthread_key_create(&key, NULL))) return NULL;

        pthread_setspecific(key, sentinel1);

        for (i = key + 1; i < 2000; i++) /* arbitrary limit */
        {
            if (p[i] == sentinel1)
            {
                pthread_setspecific(key, sentinel2);

                if (p[i] == sentinel2)
                {
                    gsbase_offset = (i - key) * sizeof(*p);
                    break;
                }

                pthread_setspecific(key, sentinel1);
            }
        }

        pthread_key_delete(key);
    }

    if (gsbase_offset) return (char*)pthread_self() + gsbase_offset;
    return NULL;
}
#endif


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    const WORD fpu_cw = 0x27f;
    stack_t ss;

#if defined __linux__
    arch_prctl( ARCH_SET_GS, teb );
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
    amd64_set_gsbase( teb );
#elif defined(__NetBSD__)
    sysarch( X86_64_SET_GSBASE, &teb );
#elif defined (__APPLE__)
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1"
                      :
                      : "r" (teb->Tib.Self), "n" (FIELD_OFFSET(TEB, Tib.Self)));
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1"
                      :
                      : "r" (teb->ThreadLocalStoragePointer), "n" (FIELD_OFFSET(TEB, ThreadLocalStoragePointer)));

    /* alloc_tls_slot() needs to poke a value to an address relative to each
       thread's gsbase.  Have each thread record its gsbase pointer into its
       TEB so alloc_tls_slot() can find it. */
    teb->Reserved5[0] = mac_thread_gsbase();
#else
# error Please define setting %gs for your architecture
#endif

    ss.ss_sp    = (char *)teb + teb_size;
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1) perror( "sigaltstack" );

#ifdef __GNUC__
    __asm__ volatile ("fninit; fldcw %0" : : "m" (fpu_cw));
#else
    FIXME("FPU setup not implemented for this platform.\n");
#endif
}


/***********************************************************************
 *           signal_exit_thread
 */
__ASM_GLOBAL_FUNC( signal_exit_thread,
                   /* fetch exit frame */
                   "movq %gs:0x30,%rax\n\t"
                   "movq 0x330(%rax),%rdx\n\t"      /* amd64_thread_data()->exit_frame */
                   "testq %rdx,%rdx\n\t"
                   "jnz 1f\n\t"
                   "jmp *%rsi\n"
                   /* switch to exit frame stack */
                   "1:\tmovq $0,0x330(%rax)\n\t"
                   "movq %rdx,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 56\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,48\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,40\n\t")
                   __ASM_CFI(".cfi_rel_offset %r12,32\n\t")
                   __ASM_CFI(".cfi_rel_offset %r13,24\n\t")
                   __ASM_CFI(".cfi_rel_offset %r14,16\n\t")
                   __ASM_CFI(".cfi_rel_offset %r15,8\n\t")
                   "call *%rsi" )

#endif  /* __x86_64__ */
