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

#ifdef __x86_64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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
#ifdef HAVE_LIBUNWIND_H
# define UNW_LOCAL_ONLY
# include <libunwind.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);

struct _DISPATCHER_CONTEXT;

typedef LONG (WINAPI *PC_LANGUAGE_EXCEPTION_HANDLER)( EXCEPTION_POINTERS *ptrs, ULONG64 frame );
typedef EXCEPTION_DISPOSITION (WINAPI *PEXCEPTION_ROUTINE)( EXCEPTION_RECORD *rec,
                                                            ULONG64 frame,
                                                            CONTEXT *context,
                                                            struct _DISPATCHER_CONTEXT *dispatch );
typedef void (WINAPI *TERMINATION_HANDLER)( ULONG flags, ULONG64 frame );

typedef struct _DISPATCHER_CONTEXT
{
    ULONG64               ControlPc;
    ULONG64               ImageBase;
    PRUNTIME_FUNCTION     FunctionEntry;
    ULONG64               EstablisherFrame;
    ULONG64               TargetIp;
    PCONTEXT              ContextRecord;
    PEXCEPTION_ROUTINE    LanguageHandler;
    PVOID                 HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    ULONG                 ScopeIndex;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

typedef struct _SCOPE_TABLE
{
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;


/* layering violation: the setjmp buffer is defined in msvcrt, but used by RtlUnwindEx */
struct MSVCRT_JUMP_BUFFER
{
    ULONG64 Frame;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;
    ULONG64 Spare;
    M128A   Xmm6;
    M128A   Xmm7;
    M128A   Xmm8;
    M128A   Xmm9;
    M128A   Xmm10;
    M128A   Xmm11;
    M128A   Xmm12;
    M128A   Xmm13;
    M128A   Xmm14;
    M128A   Xmm15;
};

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#include <asm/prctl.h>
static inline int arch_prctl( int func, void *ptr ) { return syscall( __NR_arch_prctl, func, ptr ); }

#define RAX_sig(context)     ((context)->uc_mcontext.gregs[REG_RAX])
#define RBX_sig(context)     ((context)->uc_mcontext.gregs[REG_RBX])
#define RCX_sig(context)     ((context)->uc_mcontext.gregs[REG_RCX])
#define RDX_sig(context)     ((context)->uc_mcontext.gregs[REG_RDX])
#define RSI_sig(context)     ((context)->uc_mcontext.gregs[REG_RSI])
#define RDI_sig(context)     ((context)->uc_mcontext.gregs[REG_RDI])
#define RBP_sig(context)     ((context)->uc_mcontext.gregs[REG_RBP])
#define R8_sig(context)      ((context)->uc_mcontext.gregs[REG_R8])
#define R9_sig(context)      ((context)->uc_mcontext.gregs[REG_R9])
#define R10_sig(context)     ((context)->uc_mcontext.gregs[REG_R10])
#define R11_sig(context)     ((context)->uc_mcontext.gregs[REG_R11])
#define R12_sig(context)     ((context)->uc_mcontext.gregs[REG_R12])
#define R13_sig(context)     ((context)->uc_mcontext.gregs[REG_R13])
#define R14_sig(context)     ((context)->uc_mcontext.gregs[REG_R14])
#define R15_sig(context)     ((context)->uc_mcontext.gregs[REG_R15])

#define CS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 0))
#define GS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 1))
#define FS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 2))

#define RSP_sig(context)     ((context)->uc_mcontext.gregs[REG_RSP])
#define RIP_sig(context)     ((context)->uc_mcontext.gregs[REG_RIP])
#define EFL_sig(context)     ((context)->uc_mcontext.gregs[REG_EFL])
#define TRAP_sig(context)    ((context)->uc_mcontext.gregs[REG_TRAPNO])
#define ERROR_sig(context)   ((context)->uc_mcontext.gregs[REG_ERR])

#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.fpregs))

#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)

#define RAX_sig(context)     ((context)->uc_mcontext.mc_rax)
#define RBX_sig(context)     ((context)->uc_mcontext.mc_rbx)
#define RCX_sig(context)     ((context)->uc_mcontext.mc_rcx)
#define RDX_sig(context)     ((context)->uc_mcontext.mc_rdx)
#define RSI_sig(context)     ((context)->uc_mcontext.mc_rsi)
#define RDI_sig(context)     ((context)->uc_mcontext.mc_rdi)
#define RBP_sig(context)     ((context)->uc_mcontext.mc_rbp)
#define R8_sig(context)      ((context)->uc_mcontext.mc_r8)
#define R9_sig(context)      ((context)->uc_mcontext.mc_r9)
#define R10_sig(context)     ((context)->uc_mcontext.mc_r10)
#define R11_sig(context)     ((context)->uc_mcontext.mc_r11)
#define R12_sig(context)     ((context)->uc_mcontext.mc_r12)
#define R13_sig(context)     ((context)->uc_mcontext.mc_r13)
#define R14_sig(context)     ((context)->uc_mcontext.mc_r14)
#define R15_sig(context)     ((context)->uc_mcontext.mc_r15)

#define CS_sig(context)      ((context)->uc_mcontext.mc_cs)
#define DS_sig(context)      ((context)->uc_mcontext.mc_ds)
#define ES_sig(context)      ((context)->uc_mcontext.mc_es)
#define FS_sig(context)      ((context)->uc_mcontext.mc_fs)
#define GS_sig(context)      ((context)->uc_mcontext.mc_gs)
#define SS_sig(context)      ((context)->uc_mcontext.mc_ss)

#define EFL_sig(context)     ((context)->uc_mcontext.mc_rflags)

#define RIP_sig(context)     ((context)->uc_mcontext.mc_rip)
#define RSP_sig(context)     ((context)->uc_mcontext.mc_rsp)
#define TRAP_sig(context)    ((context)->uc_mcontext.mc_trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext.mc_err)

#define FPU_sig(context)   ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.mc_fpstate))

#elif defined(__NetBSD__)

#define RAX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RAX])
#define RBX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RBX])
#define RCX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RCX])
#define RDX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RDX])
#define RSI_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RSI])
#define RDI_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RDI])
#define RBP_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RBP])
#define R8_sig(context)     ((context)->uc_mcontext.__gregs[_REG_R8])
#define R9_sig(context)     ((context)->uc_mcontext.__gregs[_REG_R9])
#define R10_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R10])
#define R11_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R11])
#define R12_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R12])
#define R13_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R13])
#define R14_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R14])
#define R15_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R15])

#define CS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_CS])
#define DS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_DS])
#define ES_sig(context)     ((context)->uc_mcontext.__gregs[_REG_ES])
#define FS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_FS])
#define GS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_GS])
#define SS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_SS])

#define EFL_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RFL])

#define RIP_sig(context)    (*((unsigned long*)&(context)->uc_mcontext.__gregs[_REG_RIP]))
#define RSP_sig(context)    (*((unsigned long*)&(context)->uc_mcontext.__gregs[_REG_URSP]))

#define TRAP_sig(context)   ((context)->uc_mcontext.__gregs[_REG_TRAPNO])
#define ERROR_sig(context)  ((context)->uc_mcontext.__gregs[_REG_ERR])

#define FPU_sig(context)   ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.__fpregs))
#elif defined (__APPLE__)
#define RAX_sig(context)     ((context)->uc_mcontext->__ss.__rax)
#define RBX_sig(context)     ((context)->uc_mcontext->__ss.__rbx)
#define RCX_sig(context)     ((context)->uc_mcontext->__ss.__rcx)
#define RDX_sig(context)     ((context)->uc_mcontext->__ss.__rdx)
#define RSI_sig(context)     ((context)->uc_mcontext->__ss.__rsi)
#define RDI_sig(context)     ((context)->uc_mcontext->__ss.__rdi)
#define RBP_sig(context)     ((context)->uc_mcontext->__ss.__rbp)
#define R8_sig(context)      ((context)->uc_mcontext->__ss.__r8)
#define R9_sig(context)      ((context)->uc_mcontext->__ss.__r9)
#define R10_sig(context)     ((context)->uc_mcontext->__ss.__r10)
#define R11_sig(context)     ((context)->uc_mcontext->__ss.__r11)
#define R12_sig(context)     ((context)->uc_mcontext->__ss.__r12)
#define R13_sig(context)     ((context)->uc_mcontext->__ss.__r13)
#define R14_sig(context)     ((context)->uc_mcontext->__ss.__r14)
#define R15_sig(context)     ((context)->uc_mcontext->__ss.__r15)

#define CS_sig(context)      ((context)->uc_mcontext->__ss.__cs)
#define FS_sig(context)      ((context)->uc_mcontext->__ss.__fs)
#define GS_sig(context)      ((context)->uc_mcontext->__ss.__gs)

#define EFL_sig(context)     ((context)->uc_mcontext->__ss.__rflags)

#define RIP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__rip))
#define RSP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__rsp))

#define TRAP_sig(context)    ((context)->uc_mcontext->__es.__trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->__es.__err)

#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)&(context)->uc_mcontext->__fs.__fpu_fcw)

#else
#error You must define the signal context functions for your platform
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
static size_t signal_stack_size;

typedef void (*raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );
typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

struct amd64_thread_data
{
    DWORD_PTR dr0;           /* debug registers */
    DWORD_PTR dr1;
    DWORD_PTR dr2;
    DWORD_PTR dr3;
    DWORD_PTR dr6;
    DWORD_PTR dr7;
    void     *exit_frame;    /* exit frame pointer */
};

C_ASSERT( sizeof(struct amd64_thread_data) <= sizeof(((TEB *)0)->SystemReserved2) );
C_ASSERT( offsetof( TEB, SystemReserved2 ) + offsetof( struct amd64_thread_data, exit_frame ) == 0x330 );

static inline struct amd64_thread_data *amd64_thread_data(void)
{
    return (struct amd64_thread_data *)NtCurrentTeb()->SystemReserved2;
}

/***********************************************************************
 * Dynamic unwind table
 */

struct dynamic_unwind_entry
{
    struct list entry;

    /* memory region which matches this entry */
    DWORD64 base;
    DWORD64 end;

    /* lookup table */
    RUNTIME_FUNCTION *table;
    DWORD count;
    DWORD max_count;

    /* user defined callback */
    PGET_RUNTIME_FUNCTION_CALLBACK callback;
    PVOID context;
};

static struct list dynamic_unwind_list = LIST_INIT(dynamic_unwind_list);

static RTL_CRITICAL_SECTION dynamic_unwind_section;
static RTL_CRITICAL_SECTION_DEBUG dynamic_unwind_debug =
{
    0, 0, &dynamic_unwind_section,
    { &dynamic_unwind_debug.ProcessLocksList, &dynamic_unwind_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dynamic_unwind_section") }
};
static RTL_CRITICAL_SECTION dynamic_unwind_section = { &dynamic_unwind_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 * Definitions for Win32 unwind tables
 */

union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

struct opcode
{
    BYTE offset;
    BYTE code : 4;
    BYTE info : 4;
};

struct UNWIND_INFO
{
    BYTE version : 3;
    BYTE flags : 5;
    BYTE prolog;
    BYTE count;
    BYTE frame_reg : 4;
    BYTE frame_offset : 4;
    struct opcode opcodes[1];  /* info->count entries */
    /* followed by handler_data */
};

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

static void dump_unwind_info( ULONG64 base, RUNTIME_FUNCTION *function )
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    union handler_data *handler_data;
    struct UNWIND_INFO *info;
    unsigned int i, count;

    TRACE( "**** func %x-%x\n", function->BeginAddress, function->EndAddress );
    for (;;)
    {
        if (function->UnwindData & 1)
        {
            RUNTIME_FUNCTION *next = (RUNTIME_FUNCTION *)((char *)base + (function->UnwindData & ~1));
            TRACE( "unwind info for function %p-%p chained to function %p-%p\n",
                   (char *)base + function->BeginAddress, (char *)base + function->EndAddress,
                   (char *)base + next->BeginAddress, (char *)base + next->EndAddress );
            function = next;
            continue;
        }
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);

        TRACE( "unwind info at %p flags %x prolog 0x%x bytes function %p-%p\n",
               info, info->flags, info->prolog,
               (char *)base + function->BeginAddress, (char *)base + function->EndAddress );

        if (info->frame_reg)
            TRACE( "    frame register %s offset 0x%x(%%rsp)\n",
                   reg_names[info->frame_reg], info->frame_offset * 16 );

        for (i = 0; i < info->count; i++)
        {
            TRACE( "    0x%x: ", info->opcodes[i].offset );
            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:
                TRACE( "pushq %%%s\n", reg_names[info->opcodes[i].info] );
                break;
            case UWOP_ALLOC_LARGE:
                if (info->opcodes[i].info)
                {
                    count = *(DWORD *)&info->opcodes[i+1];
                    i += 2;
                }
                else
                {
                    count = *(USHORT *)&info->opcodes[i+1] * 8;
                    i++;
                }
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_ALLOC_SMALL:
                count = (info->opcodes[i].info + 1) * 8;
                TRACE( "subq $0x%x,%%rsp\n", count );
                break;
            case UWOP_SET_FPREG:
                TRACE( "leaq 0x%x(%%rsp),%s\n",
                     info->frame_offset * 16, reg_names[info->frame_reg] );
                break;
            case UWOP_SAVE_NONVOL:
                count = *(USHORT *)&info->opcodes[i+1] * 8;
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i++;
                break;
            case UWOP_SAVE_NONVOL_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movq %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
                i += 2;
                break;
            case UWOP_SAVE_XMM128:
                count = *(USHORT *)&info->opcodes[i+1] * 16;
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i++;
                break;
            case UWOP_SAVE_XMM128_FAR:
                count = *(DWORD *)&info->opcodes[i+1];
                TRACE( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
                i += 2;
                break;
            case UWOP_PUSH_MACHFRAME:
                TRACE( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );
                break;
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];
        if (info->flags & UNW_FLAG_CHAININFO)
        {
            TRACE( "    chained to function %p-%p\n",
                   (char *)base + handler_data->chain.BeginAddress,
                   (char *)base + handler_data->chain.EndAddress );
            function = &handler_data->chain;
            continue;
        }
        if (info->flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
            TRACE( "    handler %p data at %p\n",
                   (char *)base + handler_data->handler, &handler_data->handler + 1 );
        break;
    }
}

static void dump_scope_table( ULONG64 base, const SCOPE_TABLE *table )
{
    unsigned int i;

    TRACE( "scope table at %p\n", table );
    for (i = 0; i < table->Count; i++)
        TRACE( "  %u: %lx-%lx handler %lx target %lx\n", i,
               base + table->ScopeRecord[i].BeginAddress,
               base + table->ScopeRecord[i].EndAddress,
               base + table->ScopeRecord[i].HandlerAddress,
               base + table->ScopeRecord[i].JumpTarget );
}


/***********************************************************************
 * Definitions for Dwarf unwind tables
 */

enum dwarf_call_frame_info
{
    DW_CFA_advance_loc = 0x40,
    DW_CFA_offset = 0x80,
    DW_CFA_restore = 0xc0,
    DW_CFA_nop = 0x00,
    DW_CFA_set_loc = 0x01,
    DW_CFA_advance_loc1 = 0x02,
    DW_CFA_advance_loc2 = 0x03,
    DW_CFA_advance_loc4 = 0x04,
    DW_CFA_offset_extended = 0x05,
    DW_CFA_restore_extended = 0x06,
    DW_CFA_undefined = 0x07,
    DW_CFA_same_value = 0x08,
    DW_CFA_register = 0x09,
    DW_CFA_remember_state = 0x0a,
    DW_CFA_restore_state = 0x0b,
    DW_CFA_def_cfa = 0x0c,
    DW_CFA_def_cfa_register = 0x0d,
    DW_CFA_def_cfa_offset = 0x0e,
    DW_CFA_def_cfa_expression = 0x0f,
    DW_CFA_expression = 0x10,
    DW_CFA_offset_extended_sf = 0x11,
    DW_CFA_def_cfa_sf = 0x12,
    DW_CFA_def_cfa_offset_sf = 0x13,
    DW_CFA_val_offset = 0x14,
    DW_CFA_val_offset_sf = 0x15,
    DW_CFA_val_expression = 0x16,
};

enum dwarf_operation
{
    DW_OP_addr                 = 0x03,
    DW_OP_deref                = 0x06,
    DW_OP_const1u              = 0x08,
    DW_OP_const1s              = 0x09,
    DW_OP_const2u              = 0x0a,
    DW_OP_const2s              = 0x0b,
    DW_OP_const4u              = 0x0c,
    DW_OP_const4s              = 0x0d,
    DW_OP_const8u              = 0x0e,
    DW_OP_const8s              = 0x0f,
    DW_OP_constu               = 0x10,
    DW_OP_consts               = 0x11,
    DW_OP_dup                  = 0x12,
    DW_OP_drop                 = 0x13,
    DW_OP_over                 = 0x14,
    DW_OP_pick                 = 0x15,
    DW_OP_swap                 = 0x16,
    DW_OP_rot                  = 0x17,
    DW_OP_xderef               = 0x18,
    DW_OP_abs                  = 0x19,
    DW_OP_and                  = 0x1a,
    DW_OP_div                  = 0x1b,
    DW_OP_minus                = 0x1c,
    DW_OP_mod                  = 0x1d,
    DW_OP_mul                  = 0x1e,
    DW_OP_neg                  = 0x1f,
    DW_OP_not                  = 0x20,
    DW_OP_or                   = 0x21,
    DW_OP_plus                 = 0x22,
    DW_OP_plus_uconst          = 0x23,
    DW_OP_shl                  = 0x24,
    DW_OP_shr                  = 0x25,
    DW_OP_shra                 = 0x26,
    DW_OP_xor                  = 0x27,
    DW_OP_bra                  = 0x28,
    DW_OP_eq                   = 0x29,
    DW_OP_ge                   = 0x2a,
    DW_OP_gt                   = 0x2b,
    DW_OP_le                   = 0x2c,
    DW_OP_lt                   = 0x2d,
    DW_OP_ne                   = 0x2e,
    DW_OP_skip                 = 0x2f,
    DW_OP_lit0                 = 0x30,
    DW_OP_lit1                 = 0x31,
    DW_OP_lit2                 = 0x32,
    DW_OP_lit3                 = 0x33,
    DW_OP_lit4                 = 0x34,
    DW_OP_lit5                 = 0x35,
    DW_OP_lit6                 = 0x36,
    DW_OP_lit7                 = 0x37,
    DW_OP_lit8                 = 0x38,
    DW_OP_lit9                 = 0x39,
    DW_OP_lit10                = 0x3a,
    DW_OP_lit11                = 0x3b,
    DW_OP_lit12                = 0x3c,
    DW_OP_lit13                = 0x3d,
    DW_OP_lit14                = 0x3e,
    DW_OP_lit15                = 0x3f,
    DW_OP_lit16                = 0x40,
    DW_OP_lit17                = 0x41,
    DW_OP_lit18                = 0x42,
    DW_OP_lit19                = 0x43,
    DW_OP_lit20                = 0x44,
    DW_OP_lit21                = 0x45,
    DW_OP_lit22                = 0x46,
    DW_OP_lit23                = 0x47,
    DW_OP_lit24                = 0x48,
    DW_OP_lit25                = 0x49,
    DW_OP_lit26                = 0x4a,
    DW_OP_lit27                = 0x4b,
    DW_OP_lit28                = 0x4c,
    DW_OP_lit29                = 0x4d,
    DW_OP_lit30                = 0x4e,
    DW_OP_lit31                = 0x4f,
    DW_OP_reg0                 = 0x50,
    DW_OP_reg1                 = 0x51,
    DW_OP_reg2                 = 0x52,
    DW_OP_reg3                 = 0x53,
    DW_OP_reg4                 = 0x54,
    DW_OP_reg5                 = 0x55,
    DW_OP_reg6                 = 0x56,
    DW_OP_reg7                 = 0x57,
    DW_OP_reg8                 = 0x58,
    DW_OP_reg9                 = 0x59,
    DW_OP_reg10                = 0x5a,
    DW_OP_reg11                = 0x5b,
    DW_OP_reg12                = 0x5c,
    DW_OP_reg13                = 0x5d,
    DW_OP_reg14                = 0x5e,
    DW_OP_reg15                = 0x5f,
    DW_OP_reg16                = 0x60,
    DW_OP_reg17                = 0x61,
    DW_OP_reg18                = 0x62,
    DW_OP_reg19                = 0x63,
    DW_OP_reg20                = 0x64,
    DW_OP_reg21                = 0x65,
    DW_OP_reg22                = 0x66,
    DW_OP_reg23                = 0x67,
    DW_OP_reg24                = 0x68,
    DW_OP_reg25                = 0x69,
    DW_OP_reg26                = 0x6a,
    DW_OP_reg27                = 0x6b,
    DW_OP_reg28                = 0x6c,
    DW_OP_reg29                = 0x6d,
    DW_OP_reg30                = 0x6e,
    DW_OP_reg31                = 0x6f,
    DW_OP_breg0                = 0x70,
    DW_OP_breg1                = 0x71,
    DW_OP_breg2                = 0x72,
    DW_OP_breg3                = 0x73,
    DW_OP_breg4                = 0x74,
    DW_OP_breg5                = 0x75,
    DW_OP_breg6                = 0x76,
    DW_OP_breg7                = 0x77,
    DW_OP_breg8                = 0x78,
    DW_OP_breg9                = 0x79,
    DW_OP_breg10               = 0x7a,
    DW_OP_breg11               = 0x7b,
    DW_OP_breg12               = 0x7c,
    DW_OP_breg13               = 0x7d,
    DW_OP_breg14               = 0x7e,
    DW_OP_breg15               = 0x7f,
    DW_OP_breg16               = 0x80,
    DW_OP_breg17               = 0x81,
    DW_OP_breg18               = 0x82,
    DW_OP_breg19               = 0x83,
    DW_OP_breg20               = 0x84,
    DW_OP_breg21               = 0x85,
    DW_OP_breg22               = 0x86,
    DW_OP_breg23               = 0x87,
    DW_OP_breg24               = 0x88,
    DW_OP_breg25               = 0x89,
    DW_OP_breg26               = 0x8a,
    DW_OP_breg27               = 0x8b,
    DW_OP_breg28               = 0x8c,
    DW_OP_breg29               = 0x8d,
    DW_OP_breg30               = 0x8e,
    DW_OP_breg31               = 0x8f,
    DW_OP_regx                 = 0x90,
    DW_OP_fbreg                = 0x91,
    DW_OP_bregx                = 0x92,
    DW_OP_piece                = 0x93,
    DW_OP_deref_size           = 0x94,
    DW_OP_xderef_size          = 0x95,
    DW_OP_nop                  = 0x96,
    DW_OP_push_object_address  = 0x97,
    DW_OP_call2                = 0x98,
    DW_OP_call4                = 0x99,
    DW_OP_call_ref             = 0x9a,
    DW_OP_form_tls_address     = 0x9b,
    DW_OP_call_frame_cfa       = 0x9c,
    DW_OP_bit_piece            = 0x9d,
    DW_OP_lo_user              = 0xe0,
    DW_OP_hi_user              = 0xff,
    DW_OP_GNU_push_tls_address = 0xe0,
    DW_OP_GNU_uninit           = 0xf0,
    DW_OP_GNU_encoded_addr     = 0xf1,
};

#define DW_EH_PE_native   0x00
#define DW_EH_PE_leb128   0x01
#define DW_EH_PE_data2    0x02
#define DW_EH_PE_data4    0x03
#define DW_EH_PE_data8    0x04
#define DW_EH_PE_signed   0x08
#define DW_EH_PE_abs      0x00
#define DW_EH_PE_pcrel    0x10
#define DW_EH_PE_textrel  0x20
#define DW_EH_PE_datarel  0x30
#define DW_EH_PE_funcrel  0x40
#define DW_EH_PE_aligned  0x50
#define DW_EH_PE_indirect 0x80
#define DW_EH_PE_omit     0xff

struct dwarf_eh_bases
{
    void *tbase;
    void *dbase;
    void *func;
};

struct dwarf_cie
{
    unsigned int  length;
    int           id;
    unsigned char version;
    unsigned char augmentation[1];
};

struct dwarf_fde
{
    unsigned int length;
    unsigned int cie_offset;
};

extern const struct dwarf_fde *_Unwind_Find_FDE (void *, struct dwarf_eh_bases *);

static unsigned char dwarf_get_u1( const unsigned char **p )
{
    return *(*p)++;
}

static unsigned short dwarf_get_u2( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8);
    (*p) += 2;
    return ret;
}

static unsigned int dwarf_get_u4( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8) | ((*p)[2] << 16) | ((*p)[3] << 24);
    (*p) += 4;
    return ret;
}

static ULONG64 dwarf_get_u8( const unsigned char **p )
{
    ULONG64 low  = dwarf_get_u4( p );
    ULONG64 high = dwarf_get_u4( p );
    return low | (high << 32);
}

static ULONG_PTR dwarf_get_uleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);
    return ret;
}

static LONG_PTR dwarf_get_sleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);

    if ((shift < 8 * sizeof(ret)) && (byte & 0x40)) ret |= -((ULONG_PTR)1 << shift);
    return ret;
}

static ULONG_PTR dwarf_get_ptr( const unsigned char **p, unsigned char encoding )
{
    ULONG_PTR base;

    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0xf0)
    {
    case DW_EH_PE_abs:
        base = 0;
        break;
    case DW_EH_PE_pcrel:
        base = (ULONG_PTR)*p;
        break;
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }

    switch (encoding & 0x0f)
    {
    case DW_EH_PE_native:
        return base + dwarf_get_u8( p );
    case DW_EH_PE_leb128:
        return base + dwarf_get_uleb128( p );
    case DW_EH_PE_data2:
        return base + dwarf_get_u2( p );
    case DW_EH_PE_data4:
        return base + dwarf_get_u4( p );
    case DW_EH_PE_data8:
        return base + dwarf_get_u8( p );
    case DW_EH_PE_signed|DW_EH_PE_leb128:
        return base + dwarf_get_sleb128( p );
    case DW_EH_PE_signed|DW_EH_PE_data2:
        return base + (signed short)dwarf_get_u2( p );
    case DW_EH_PE_signed|DW_EH_PE_data4:
        return base + (signed int)dwarf_get_u4( p );
    case DW_EH_PE_signed|DW_EH_PE_data8:
        return base + (LONG64)dwarf_get_u8( p );
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }
}

enum reg_rule
{
    RULE_UNSET,          /* not set at all */
    RULE_UNDEFINED,      /* undefined value */
    RULE_SAME,           /* same value as previous frame */
    RULE_CFA_OFFSET,     /* stored at cfa offset */
    RULE_OTHER_REG,      /* stored in other register */
    RULE_EXPRESSION,     /* address specified by expression */
    RULE_VAL_EXPRESSION  /* value specified by expression */
};

#define NB_FRAME_REGS 41
#define MAX_SAVED_STATES 16

struct frame_state
{
    ULONG_PTR     cfa_offset;
    unsigned char cfa_reg;
    enum reg_rule cfa_rule;
    enum reg_rule rules[NB_FRAME_REGS];
    ULONG64       regs[NB_FRAME_REGS];
};

struct frame_info
{
    ULONG_PTR     ip;
    ULONG_PTR     code_align;
    LONG_PTR      data_align;
    unsigned char retaddr_reg;
    unsigned char fde_encoding;
    unsigned char signal_frame;
    unsigned char state_sp;
    struct frame_state state;
    struct frame_state *state_stack;
};

static const char *dwarf_reg_names[NB_FRAME_REGS] =
{
/*  0-7  */ "%rax", "%rdx", "%rcx", "%rbx", "%rsi", "%rdi", "%rbp", "%rsp",
/*  8-16 */ "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rip",
/* 17-24 */ "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
/* 25-32 */ "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15",
/* 33-40 */ "%st0", "%st1", "%st2", "%st3", "%st4", "%st5", "%st6", "%st7"
};

static BOOL valid_reg( ULONG_PTR reg )
{
    if (reg >= NB_FRAME_REGS) FIXME( "unsupported reg %lx\n", reg );
    return (reg < NB_FRAME_REGS);
}

static void execute_cfa_instructions( const unsigned char *ptr, const unsigned char *end,
                                      ULONG_PTR last_ip, struct frame_info *info )
{
    while (ptr < end && info->ip < last_ip + info->signal_frame)
    {
        enum dwarf_call_frame_info op = *ptr++;

        if (op & 0xc0)
        {
            switch (op & 0xc0)
            {
            case DW_CFA_advance_loc:
            {
                ULONG_PTR offset = (op & 0x3f) * info->code_align;
                TRACE( "%lx: DW_CFA_advance_loc %lu\n", info->ip, offset );
                info->ip += offset;
                break;
            }
            case DW_CFA_offset:
            {
                ULONG_PTR reg = op & 0x3f;
                LONG_PTR offset = dwarf_get_uleb128( &ptr ) * info->data_align;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_offset %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
                info->state.regs[reg]  = offset;
                info->state.rules[reg] = RULE_CFA_OFFSET;
                break;
            }
            case DW_CFA_restore:
            {
                ULONG_PTR reg = op & 0x3f;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_restore %s\n", info->ip, dwarf_reg_names[reg] );
                info->state.rules[reg] = RULE_UNSET;
                break;
            }
            }
        }
        else switch (op)
        {
        case DW_CFA_nop:
            break;
        case DW_CFA_set_loc:
        {
            ULONG_PTR loc = dwarf_get_ptr( &ptr, info->fde_encoding );
            TRACE( "%lx: DW_CFA_set_loc %lx\n", info->ip, loc );
            info->ip = loc;
            break;
        }
        case DW_CFA_advance_loc1:
        {
            ULONG_PTR offset = *ptr++ * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc1 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc2:
        {
            ULONG_PTR offset = dwarf_get_u2( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc2 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc4:
        {
            ULONG_PTR offset = dwarf_get_u4( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc4 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            LONG_PTR offset = (op == DW_CFA_offset_extended) ? dwarf_get_uleb128( &ptr ) * info->data_align
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_offset_extended %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.regs[reg]  = offset;
            info->state.rules[reg] = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_restore_extended:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_restore_extended %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNSET;
            break;
        }
        case DW_CFA_undefined:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_undefined %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNDEFINED;
            break;
        }
        case DW_CFA_same_value:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_same_value %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.regs[reg]  = reg;
            info->state.rules[reg] = RULE_SAME;
            break;
        }
        case DW_CFA_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR reg2 = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg ) || !valid_reg( reg2 )) break;
            TRACE( "%lx: DW_CFA_register %s == %s\n", info->ip, dwarf_reg_names[reg], dwarf_reg_names[reg2] );
            info->state.regs[reg]  = reg2;
            info->state.rules[reg] = RULE_OTHER_REG;
            break;
        }
        case DW_CFA_remember_state:
            TRACE( "%lx: DW_CFA_remember_state\n", info->ip );
            if (info->state_sp >= MAX_SAVED_STATES)
                FIXME( "%lx: DW_CFA_remember_state too many nested saves\n", info->ip );
            else
                info->state_stack[info->state_sp++] = info->state;
            break;
        case DW_CFA_restore_state:
            TRACE( "%lx: DW_CFA_restore_state\n", info->ip );
            if (!info->state_sp)
                FIXME( "%lx: DW_CFA_restore_state without corresponding save\n", info->ip );
            else
                info->state = info->state_stack[--info->state_sp];
            break;
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR offset = (op == DW_CFA_def_cfa) ? dwarf_get_uleb128( &ptr )
                                                      : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa %s, %lu\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.cfa_reg    = reg;
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa_register %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.cfa_reg = reg;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        {
            ULONG_PTR offset = (op == DW_CFA_def_cfa_offset) ? dwarf_get_uleb128( &ptr )
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            TRACE( "%lx: DW_CFA_def_cfa_offset %lu\n", info->ip, offset );
            info->state.cfa_offset = offset;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_expression:
        {
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            TRACE( "%lx: DW_CFA_def_cfa_expression %lx-%lx\n", info->ip, expr, expr+len );
            info->state.cfa_offset = expr;
            info->state.cfa_rule = RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        case DW_CFA_expression:
        case DW_CFA_val_expression:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_%sexpression %s %lx-%lx\n",
                   info->ip, (op == DW_CFA_expression) ? "" : "val_", dwarf_reg_names[reg], expr, expr+len );
            info->state.regs[reg]  = expr;
            info->state.rules[reg] = (op == DW_CFA_expression) ? RULE_EXPRESSION : RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        default:
            FIXME( "%lx: unknown CFA opcode %02x\n", info->ip, op );
            break;
        }
    }
}

/* retrieve a context register from its dwarf number */
static void *get_context_reg( CONTEXT *context, ULONG_PTR dw_reg )
{
    switch (dw_reg)
    {
    case 0:  return &context->Rax;
    case 1:  return &context->Rdx;
    case 2:  return &context->Rcx;
    case 3:  return &context->Rbx;
    case 4:  return &context->Rsi;
    case 5:  return &context->Rdi;
    case 6:  return &context->Rbp;
    case 7:  return &context->Rsp;
    case 8:  return &context->R8;
    case 9:  return &context->R9;
    case 10: return &context->R10;
    case 11: return &context->R11;
    case 12: return &context->R12;
    case 13: return &context->R13;
    case 14: return &context->R14;
    case 15: return &context->R15;
    case 16: return &context->Rip;
    case 17: return &context->u.s.Xmm0;
    case 18: return &context->u.s.Xmm1;
    case 19: return &context->u.s.Xmm2;
    case 20: return &context->u.s.Xmm3;
    case 21: return &context->u.s.Xmm4;
    case 22: return &context->u.s.Xmm5;
    case 23: return &context->u.s.Xmm6;
    case 24: return &context->u.s.Xmm7;
    case 25: return &context->u.s.Xmm8;
    case 26: return &context->u.s.Xmm9;
    case 27: return &context->u.s.Xmm10;
    case 28: return &context->u.s.Xmm11;
    case 29: return &context->u.s.Xmm12;
    case 30: return &context->u.s.Xmm13;
    case 31: return &context->u.s.Xmm14;
    case 32: return &context->u.s.Xmm15;
    case 33: return &context->u.s.Legacy[0];
    case 34: return &context->u.s.Legacy[1];
    case 35: return &context->u.s.Legacy[2];
    case 36: return &context->u.s.Legacy[3];
    case 37: return &context->u.s.Legacy[4];
    case 38: return &context->u.s.Legacy[5];
    case 39: return &context->u.s.Legacy[6];
    case 40: return &context->u.s.Legacy[7];
    default: return NULL;
    }
}

/* set a context register from its dwarf number */
static void set_context_reg( CONTEXT *context, ULONG_PTR dw_reg, void *val )
{
    switch (dw_reg)
    {
    case 0:  context->Rax = *(ULONG64 *)val; break;
    case 1:  context->Rdx = *(ULONG64 *)val; break;
    case 2:  context->Rcx = *(ULONG64 *)val; break;
    case 3:  context->Rbx = *(ULONG64 *)val; break;
    case 4:  context->Rsi = *(ULONG64 *)val; break;
    case 5:  context->Rdi = *(ULONG64 *)val; break;
    case 6:  context->Rbp = *(ULONG64 *)val; break;
    case 7:  context->Rsp = *(ULONG64 *)val; break;
    case 8:  context->R8  = *(ULONG64 *)val; break;
    case 9:  context->R9  = *(ULONG64 *)val; break;
    case 10: context->R10 = *(ULONG64 *)val; break;
    case 11: context->R11 = *(ULONG64 *)val; break;
    case 12: context->R12 = *(ULONG64 *)val; break;
    case 13: context->R13 = *(ULONG64 *)val; break;
    case 14: context->R14 = *(ULONG64 *)val; break;
    case 15: context->R15 = *(ULONG64 *)val; break;
    case 16: context->Rip = *(ULONG64 *)val; break;
    case 17: memcpy( &context->u.s.Xmm0, val, sizeof(M128A) ); break;
    case 18: memcpy( &context->u.s.Xmm1, val, sizeof(M128A) ); break;
    case 19: memcpy( &context->u.s.Xmm2, val, sizeof(M128A) ); break;
    case 20: memcpy( &context->u.s.Xmm3, val, sizeof(M128A) ); break;
    case 21: memcpy( &context->u.s.Xmm4, val, sizeof(M128A) ); break;
    case 22: memcpy( &context->u.s.Xmm5, val, sizeof(M128A) ); break;
    case 23: memcpy( &context->u.s.Xmm6, val, sizeof(M128A) ); break;
    case 24: memcpy( &context->u.s.Xmm7, val, sizeof(M128A) ); break;
    case 25: memcpy( &context->u.s.Xmm8, val, sizeof(M128A) ); break;
    case 26: memcpy( &context->u.s.Xmm9, val, sizeof(M128A) ); break;
    case 27: memcpy( &context->u.s.Xmm10, val, sizeof(M128A) ); break;
    case 28: memcpy( &context->u.s.Xmm11, val, sizeof(M128A) ); break;
    case 29: memcpy( &context->u.s.Xmm12, val, sizeof(M128A) ); break;
    case 30: memcpy( &context->u.s.Xmm13, val, sizeof(M128A) ); break;
    case 31: memcpy( &context->u.s.Xmm14, val, sizeof(M128A) ); break;
    case 32: memcpy( &context->u.s.Xmm15, val, sizeof(M128A) ); break;
    case 33: memcpy( &context->u.s.Legacy[0], val, sizeof(M128A) ); break;
    case 34: memcpy( &context->u.s.Legacy[1], val, sizeof(M128A) ); break;
    case 35: memcpy( &context->u.s.Legacy[2], val, sizeof(M128A) ); break;
    case 36: memcpy( &context->u.s.Legacy[3], val, sizeof(M128A) ); break;
    case 37: memcpy( &context->u.s.Legacy[4], val, sizeof(M128A) ); break;
    case 38: memcpy( &context->u.s.Legacy[5], val, sizeof(M128A) ); break;
    case 39: memcpy( &context->u.s.Legacy[6], val, sizeof(M128A) ); break;
    case 40: memcpy( &context->u.s.Legacy[7], val, sizeof(M128A) ); break;
    }
}

static ULONG_PTR eval_expression( const unsigned char *p, CONTEXT *context )
{
    ULONG_PTR reg, tmp, stack[64];
    int sp = -1;
    ULONG_PTR len = dwarf_get_uleb128(&p);
    const unsigned char *end = p + len;

    while (p < end)
    {
        unsigned char opcode = dwarf_get_u1(&p);

        if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31)
            stack[++sp] = opcode - DW_OP_lit0;
        else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_reg0 );
        else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_breg0 ) + dwarf_get_sleb128(&p);
        else switch (opcode)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const1u:     stack[++sp] = dwarf_get_u1(&p); break;
        case DW_OP_const1s:     stack[++sp] = (signed char)dwarf_get_u1(&p); break;
        case DW_OP_const2u:     stack[++sp] = dwarf_get_u2(&p); break;
        case DW_OP_const2s:     stack[++sp] = (short)dwarf_get_u2(&p); break;
        case DW_OP_const4u:     stack[++sp] = dwarf_get_u4(&p); break;
        case DW_OP_const4s:     stack[++sp] = (signed int)dwarf_get_u4(&p); break;
        case DW_OP_const8u:     stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const8s:     stack[++sp] = (LONG_PTR)dwarf_get_u8(&p); break;
        case DW_OP_constu:      stack[++sp] = dwarf_get_uleb128(&p); break;
        case DW_OP_consts:      stack[++sp] = dwarf_get_sleb128(&p); break;
        case DW_OP_deref:       stack[sp] = *(ULONG_PTR *)stack[sp]; break;
        case DW_OP_dup:         stack[sp + 1] = stack[sp]; sp++; break;
        case DW_OP_drop:        sp--; break;
        case DW_OP_over:        stack[sp + 1] = stack[sp - 1]; sp++; break;
        case DW_OP_pick:        stack[sp + 1] = stack[sp - dwarf_get_u1(&p)]; sp++; break;
        case DW_OP_swap:        tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp; break;
        case DW_OP_abs:         stack[sp] = labs(stack[sp]); break;
        case DW_OP_neg:         stack[sp] = -stack[sp]; break;
        case DW_OP_not:         stack[sp] = ~stack[sp]; break;
        case DW_OP_and:         stack[sp-1] &= stack[sp]; sp--; break;
        case DW_OP_or:          stack[sp-1] |= stack[sp]; sp--; break;
        case DW_OP_minus:       stack[sp-1] -= stack[sp]; sp--; break;
        case DW_OP_mul:         stack[sp-1] *= stack[sp]; sp--; break;
        case DW_OP_plus:        stack[sp-1] += stack[sp]; sp--; break;
        case DW_OP_xor:         stack[sp-1] ^= stack[sp]; sp--; break;
        case DW_OP_shl:         stack[sp-1] <<= stack[sp]; sp--; break;
        case DW_OP_shr:         stack[sp-1] >>= stack[sp]; sp--; break;
        case DW_OP_plus_uconst: stack[sp] += dwarf_get_uleb128(&p); break;
        case DW_OP_shra:        stack[sp-1] = (LONG_PTR)stack[sp-1] / (1 << stack[sp]); sp--; break;
        case DW_OP_div:         stack[sp-1] = (LONG_PTR)stack[sp-1] / (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_mod:         stack[sp-1] = (LONG_PTR)stack[sp-1] % (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_ge:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_gt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_le:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_lt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_eq:          stack[sp-1] = (stack[sp-1] == stack[sp]); sp--; break;
        case DW_OP_ne:          stack[sp-1] = (stack[sp-1] != stack[sp]); sp--; break;
        case DW_OP_skip:        tmp = (short)dwarf_get_u2(&p); p += tmp; break;
        case DW_OP_bra:         tmp = (short)dwarf_get_u2(&p); if (!stack[sp--]) p += tmp; break;
        case DW_OP_GNU_encoded_addr: tmp = *p++; stack[++sp] = dwarf_get_ptr( &p, tmp ); break;
        case DW_OP_regx:        stack[++sp] = *(ULONG_PTR *)get_context_reg( context, dwarf_get_uleb128(&p) ); break;
        case DW_OP_bregx:
            reg = dwarf_get_uleb128(&p);
            tmp = dwarf_get_sleb128(&p);
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, reg ) + tmp;
            break;
        case DW_OP_deref_size:
            switch (*p++)
            {
            case 1: stack[sp] = *(unsigned char *)stack[sp]; break;
            case 2: stack[sp] = *(unsigned short *)stack[sp]; break;
            case 4: stack[sp] = *(unsigned int *)stack[sp]; break;
            case 8: stack[sp] = *(ULONG_PTR *)stack[sp]; break;
            }
            break;
        default:
            FIXME( "unhandled opcode %02x\n", opcode );
        }
    }
    return stack[sp];
}

/* apply the computed frame info to the actual context */
static void apply_frame_state( CONTEXT *context, struct frame_state *state )
{
    unsigned int i;
    ULONG_PTR cfa, value;
    CONTEXT new_context = *context;

    switch (state->cfa_rule)
    {
    case RULE_EXPRESSION:
        cfa = *(ULONG_PTR *)eval_expression( (const unsigned char *)state->cfa_offset, context );
        break;
    case RULE_VAL_EXPRESSION:
        cfa = eval_expression( (const unsigned char *)state->cfa_offset, context );
        break;
    default:
        cfa = *(ULONG_PTR *)get_context_reg( context, state->cfa_reg ) + state->cfa_offset;
        break;
    }
    if (!cfa) return;

    for (i = 0; i < NB_FRAME_REGS; i++)
    {
        switch (state->rules[i])
        {
        case RULE_UNSET:
        case RULE_UNDEFINED:
        case RULE_SAME:
            break;
        case RULE_CFA_OFFSET:
            set_context_reg( &new_context, i, (char *)cfa + state->regs[i] );
            break;
        case RULE_OTHER_REG:
            set_context_reg( &new_context, i, get_context_reg( context, state->regs[i] ));
            break;
        case RULE_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context );
            set_context_reg( &new_context, i, (void *)value );
            break;
        case RULE_VAL_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context );
            set_context_reg( &new_context, i, &value );
            break;
        }
    }
    new_context.Rsp = cfa;
    *context = new_context;
}


/***********************************************************************
 *           dwarf_virtual_unwind
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
static NTSTATUS dwarf_virtual_unwind( ULONG64 ip, ULONG64 *frame,CONTEXT *context,
                                      const struct dwarf_fde *fde, const struct dwarf_eh_bases *bases,
                                      PEXCEPTION_ROUTINE *handler, void **handler_data )
{
    const struct dwarf_cie *cie;
    const unsigned char *ptr, *augmentation, *end;
    ULONG_PTR len, code_end;
    struct frame_info info;
    struct frame_state state_stack[MAX_SAVED_STATES];
    int aug_z_format = 0;
    unsigned char lsda_encoding = DW_EH_PE_omit;

    memset( &info, 0, sizeof(info) );
    info.state_stack = state_stack;
    info.ip = (ULONG_PTR)bases->func;
    *handler = NULL;

    cie = (const struct dwarf_cie *)((const char *)&fde->cie_offset - fde->cie_offset);

    /* parse the CIE first */

    if (cie->version != 1 && cie->version != 3)
    {
        FIXME( "unknown CIE version %u at %p\n", cie->version, cie );
        return STATUS_INVALID_DISPOSITION;
    }
    ptr = cie->augmentation + strlen((const char *)cie->augmentation) + 1;

    info.code_align = dwarf_get_uleb128( &ptr );
    info.data_align = dwarf_get_sleb128( &ptr );
    if (cie->version == 1)
        info.retaddr_reg = *ptr++;
    else
        info.retaddr_reg = dwarf_get_uleb128( &ptr );
    info.state.cfa_rule = RULE_CFA_OFFSET;

    TRACE( "function %lx base %p cie %p len %x id %x version %x aug '%s' code_align %lu data_align %ld retaddr %s\n",
           ip, bases->func, cie, cie->length, cie->id, cie->version, cie->augmentation,
           info.code_align, info.data_align, dwarf_reg_names[info.retaddr_reg] );

    end = NULL;
    for (augmentation = cie->augmentation; *augmentation; augmentation++)
    {
        switch (*augmentation)
        {
        case 'z':
            len = dwarf_get_uleb128( &ptr );
            end = ptr + len;
            aug_z_format = 1;
            continue;
        case 'L':
            lsda_encoding = *ptr++;
            continue;
        case 'P':
        {
            unsigned char encoding = *ptr++;
            *handler = (void *)dwarf_get_ptr( &ptr, encoding );
            continue;
        }
        case 'R':
            info.fde_encoding = *ptr++;
            continue;
        case 'S':
            info.signal_frame = 1;
            continue;
        }
        FIXME( "unknown augmentation '%c'\n", *augmentation );
        if (!end) return STATUS_INVALID_DISPOSITION;  /* cannot continue */
        break;
    }
    if (end) ptr = end;

    end = (const unsigned char *)(&cie->length + 1) + cie->length;
    execute_cfa_instructions( ptr, end, ip, &info );

    ptr = (const unsigned char *)(fde + 1);
    info.ip = dwarf_get_ptr( &ptr, info.fde_encoding );  /* fde code start */
    code_end = info.ip + dwarf_get_ptr( &ptr, info.fde_encoding & 0x0f );  /* fde code length */

    if (aug_z_format)  /* get length of augmentation data */
    {
        len = dwarf_get_uleb128( &ptr );
        end = ptr + len;
    }
    else end = NULL;

    *handler_data = (void *)dwarf_get_ptr( &ptr, lsda_encoding );
    if (end) ptr = end;

    end = (const unsigned char *)(&fde->length + 1) + fde->length;
    TRACE( "fde %p len %x personality %p lsda %p code %lx-%lx\n",
           fde, fde->length, *handler, *handler_data, info.ip, code_end );
    execute_cfa_instructions( ptr, end, ip, &info );
    *frame = context->Rsp;
    apply_frame_state( context, &info.state );

    TRACE( "next function rip=%016lx\n", context->Rip );
    TRACE( "  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
           context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE( "  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
           context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE( "   r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
           context->R8, context->R9, context->R10, context->R11 );
    TRACE( "  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
           context->R12, context->R13, context->R14, context->R15 );

    return STATUS_SUCCESS;
}


#ifdef HAVE_LIBUNWIND_H
/***********************************************************************
 *           libunwind_set_cursor_from_context
 */
static int libunwind_set_cursor_from_context( unw_cursor_t *cursor, CONTEXT *context, ULONG64 ip )
{
    int rc;

    rc = unw_set_reg(cursor, UNW_REG_IP, ip);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_REG_SP, context->Rsp);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RAX, context->Rax);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RDX, context->Rdx);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RCX, context->Rcx);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RBX, context->Rbx);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RSI, context->Rsi);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RDI, context->Rdi);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_RBP, context->Rbp);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R8, context->R8);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R9, context->R9);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R10, context->R10);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R11, context->R11);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R12, context->R12);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R13, context->R13);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R14, context->R14);
    if (rc == UNW_ESUCCESS)
        rc = unw_set_reg(cursor, UNW_X86_64_R15, context->R15);

    return rc;
}


/***********************************************************************
 *           libunwind_get_reg
 */
static int libunwind_get_reg( unw_cursor_t *cursor, unw_regnum_t reg, ULONG64 *val )
{
    int rc;
    unw_word_t word;

    rc = unw_get_reg(cursor, reg, &word);
    if (rc == UNW_ESUCCESS)
        *val = word;

    return rc;
}


/***********************************************************************
 *           libunwind_set_context_from_cursor
 */
static BOOL libunwind_set_context_from_cursor( CONTEXT *context, unw_cursor_t *cursor )
{
    int rc;

    rc = libunwind_get_reg(cursor, UNW_REG_IP, &context->Rip);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_REG_SP, &context->Rsp);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RAX, &context->Rax);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RDX, &context->Rdx);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RCX, &context->Rcx);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RBX, &context->Rbx);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RSI, &context->Rsi);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RDI, &context->Rdi);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_RBP, &context->Rbp);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R8, &context->R8);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R9, &context->R9);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R10, &context->R10);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R11, &context->R11);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R12, &context->R12);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R13, &context->R13);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R14, &context->R14);
    if (rc == UNW_ESUCCESS)
        rc = libunwind_get_reg(cursor, UNW_X86_64_R15, &context->R15);

    return rc;
}


/***********************************************************************
 *           libunwind_virtual_unwind
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
static NTSTATUS libunwind_virtual_unwind( ULONG64 ip, BOOL* got_info, ULONG64 *frame, CONTEXT *context,
                                          PEXCEPTION_ROUTINE *handler, void **handler_data )
{
    unw_context_t unw_context;
    unw_cursor_t cursor;
    unw_proc_info_t info;
    int rc;

    rc = unw_getcontext( &unw_context );
    if (rc == UNW_ESUCCESS)
        rc = unw_init_local( &cursor, &unw_context );
    if (rc == UNW_ESUCCESS)
        rc = libunwind_set_cursor_from_context( &cursor, context, ip - 1 );
    if (rc != UNW_ESUCCESS)
    {
        WARN( "setup failed: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }

    rc = unw_get_proc_info(&cursor, &info);
    if (rc != UNW_ESUCCESS && rc != UNW_ENOINFO)
    {
        WARN( "failed to get info: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }
    if (rc == UNW_ENOINFO || ip < info.start_ip || info.end_ip <= ip || !info.format)
    {
        *got_info = FALSE;
        return STATUS_SUCCESS;
    }

    TRACE( "ip %#lx function %#lx-%#lx personality %#lx lsda %#lx fde %#lx\n",
           ip, (unsigned long)info.start_ip, (unsigned long)info.end_ip, (unsigned long)info.handler,
           (unsigned long)info.lsda, (unsigned long)info.unwind_info );

    rc = unw_step(&cursor);
    if (rc < 0)
    {
        WARN( "failed to unwind: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }

    *frame = context->Rsp;

    rc = libunwind_set_context_from_cursor( context, &cursor );
    if (rc != UNW_ESUCCESS)
    {
        WARN( "failed to update context after unwind: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }

    *handler = (void*)info.handler;
    *handler_data = (void*)info.lsda;
    *got_info = TRUE;

    TRACE( "next function rip=%016lx\n", context->Rip );
    TRACE( "  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
           context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE( "  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
           context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE( "   r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
           context->R8, context->R9, context->R10, context->R11 );
    TRACE( "  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
           context->R12, context->R13, context->R14, context->R15 );

    return STATUS_SUCCESS;
}
#endif


/***********************************************************************
 *           dispatch_signal
 */
static inline int dispatch_signal(unsigned int sig)
{
    if (handlers[sig] == NULL) return 0;
    return handlers[sig](sig);
}

/***********************************************************************
 *           get_signal_stack
 *
 * Get the base of the signal stack for the current thread.
 */
static inline void *get_signal_stack(void)
{
    return (char *)NtCurrentTeb() + teb_size;
}

/***********************************************************************
 *           is_inside_signal_stack
 *
 * Check if pointer is inside the signal stack.
 */
static inline BOOL is_inside_signal_stack( void *ptr )
{
    return ((char *)ptr >= (char *)get_signal_stack() &&
            (char *)ptr < (char *)get_signal_stack() + signal_stack_size);
}

/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{
    context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_DEBUG_REGISTERS;
    context->Rax    = RAX_sig(sigcontext);
    context->Rcx    = RCX_sig(sigcontext);
    context->Rdx    = RDX_sig(sigcontext);
    context->Rbx    = RBX_sig(sigcontext);
    context->Rsp    = RSP_sig(sigcontext);
    context->Rbp    = RBP_sig(sigcontext);
    context->Rsi    = RSI_sig(sigcontext);
    context->Rdi    = RDI_sig(sigcontext);
    context->R8     = R8_sig(sigcontext);
    context->R9     = R9_sig(sigcontext);
    context->R10    = R10_sig(sigcontext);
    context->R11    = R11_sig(sigcontext);
    context->R12    = R12_sig(sigcontext);
    context->R13    = R13_sig(sigcontext);
    context->R14    = R14_sig(sigcontext);
    context->R15    = R15_sig(sigcontext);
    context->Rip    = RIP_sig(sigcontext);
    context->SegCs  = CS_sig(sigcontext);
    context->SegFs  = FS_sig(sigcontext);
    context->SegGs  = GS_sig(sigcontext);
    context->EFlags = EFL_sig(sigcontext);
#ifdef DS_sig
    context->SegDs  = DS_sig(sigcontext);
#else
    __asm__("movw %%ds,%0" : "=m" (context->SegDs));
#endif
#ifdef ES_sig
    context->SegEs  = ES_sig(sigcontext);
#else
    __asm__("movw %%es,%0" : "=m" (context->SegEs));
#endif
#ifdef SS_sig
    context->SegSs  = SS_sig(sigcontext);
#else
    __asm__("movw %%ss,%0" : "=m" (context->SegSs));
#endif
    context->Dr0    = amd64_thread_data()->dr0;
    context->Dr1    = amd64_thread_data()->dr1;
    context->Dr2    = amd64_thread_data()->dr2;
    context->Dr3    = amd64_thread_data()->dr3;
    context->Dr6    = amd64_thread_data()->dr6;
    context->Dr7    = amd64_thread_data()->dr7;
    if (FPU_sig(sigcontext))
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->u.FltSave = *FPU_sig(sigcontext);
        context->MxCsr = context->u.FltSave.MxCsr;
    }
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    amd64_thread_data()->dr0 = context->Dr0;
    amd64_thread_data()->dr1 = context->Dr1;
    amd64_thread_data()->dr2 = context->Dr2;
    amd64_thread_data()->dr3 = context->Dr3;
    amd64_thread_data()->dr6 = context->Dr6;
    amd64_thread_data()->dr7 = context->Dr7;
    RAX_sig(sigcontext) = context->Rax;
    RCX_sig(sigcontext) = context->Rcx;
    RDX_sig(sigcontext) = context->Rdx;
    RBX_sig(sigcontext) = context->Rbx;
    RSP_sig(sigcontext) = context->Rsp;
    RBP_sig(sigcontext) = context->Rbp;
    RSI_sig(sigcontext) = context->Rsi;
    RDI_sig(sigcontext) = context->Rdi;
    R8_sig(sigcontext)  = context->R8;
    R9_sig(sigcontext)  = context->R9;
    R10_sig(sigcontext) = context->R10;
    R11_sig(sigcontext) = context->R11;
    R12_sig(sigcontext) = context->R12;
    R13_sig(sigcontext) = context->R13;
    R14_sig(sigcontext) = context->R14;
    R15_sig(sigcontext) = context->R15;
    RIP_sig(sigcontext) = context->Rip;
    CS_sig(sigcontext)  = context->SegCs;
    FS_sig(sigcontext)  = context->SegFs;
    GS_sig(sigcontext)  = context->SegGs;
    EFL_sig(sigcontext) = context->EFlags;
#ifdef DS_sig
    DS_sig(sigcontext) = context->SegDs;
#endif
#ifdef ES_sig
    ES_sig(sigcontext) = context->SegEs;
#endif
#ifdef SS_sig
    SS_sig(sigcontext) = context->SegSs;
#endif
    if (FPU_sig(sigcontext)) *FPU_sig(sigcontext) = context->u.FltSave;
}


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "pushfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "movl $0x001000f,0x30(%rcx)\n\t" /* context->ContextFlags */
                   "stmxcsr 0x34(%rcx)\n\t"         /* context->MxCsr */
                   "movw %cs,0x38(%rcx)\n\t"        /* context->SegCs */
                   "movw %ds,0x3a(%rcx)\n\t"        /* context->SegDs */
                   "movw %es,0x3c(%rcx)\n\t"        /* context->SegEs */
                   "movw %fs,0x3e(%rcx)\n\t"        /* context->SegFs */
                   "movw %gs,0x40(%rcx)\n\t"        /* context->SegGs */
                   "movw %ss,0x42(%rcx)\n\t"        /* context->SegSs */
                   "popq 0x44(%rcx)\n\t"            /* context->Eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "movq %rax,0x78(%rcx)\n\t"       /* context->Rax */
                   "movq %rcx,0x80(%rcx)\n\t"       /* context->Rcx */
                   "movq %rdx,0x88(%rcx)\n\t"       /* context->Rdx */
                   "movq %rbx,0x90(%rcx)\n\t"       /* context->Rbx */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"       /* context->Rsp */
                   "movq %rbp,0xa0(%rcx)\n\t"       /* context->Rbp */
                   "movq %rsi,0xa8(%rcx)\n\t"       /* context->Rsi */
                   "movq %rdi,0xb0(%rcx)\n\t"       /* context->Rdi */
                   "movq %r8,0xb8(%rcx)\n\t"        /* context->R8 */
                   "movq %r9,0xc0(%rcx)\n\t"        /* context->R9 */
                   "movq %r10,0xc8(%rcx)\n\t"       /* context->R10 */
                   "movq %r11,0xd0(%rcx)\n\t"       /* context->R11 */
                   "movq %r12,0xd8(%rcx)\n\t"       /* context->R12 */
                   "movq %r13,0xe0(%rcx)\n\t"       /* context->R13 */
                   "movq %r14,0xe8(%rcx)\n\t"       /* context->R14 */
                   "movq %r15,0xf0(%rcx)\n\t"       /* context->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"       /* context->Rip */
                   "fxsave 0x100(%rcx)\n\t"         /* context->FtlSave */
                   "movdqa %xmm0,0x1a0(%rcx)\n\t"   /* context->Xmm0 */
                   "movdqa %xmm1,0x1b0(%rcx)\n\t"   /* context->Xmm1 */
                   "movdqa %xmm2,0x1c0(%rcx)\n\t"   /* context->Xmm2 */
                   "movdqa %xmm3,0x1d0(%rcx)\n\t"   /* context->Xmm3 */
                   "movdqa %xmm4,0x1e0(%rcx)\n\t"   /* context->Xmm4 */
                   "movdqa %xmm5,0x1f0(%rcx)\n\t"   /* context->Xmm5 */
                   "movdqa %xmm6,0x200(%rcx)\n\t"   /* context->Xmm6 */
                   "movdqa %xmm7,0x210(%rcx)\n\t"   /* context->Xmm7 */
                   "movdqa %xmm8,0x220(%rcx)\n\t"   /* context->Xmm8 */
                   "movdqa %xmm9,0x230(%rcx)\n\t"   /* context->Xmm9 */
                   "movdqa %xmm10,0x240(%rcx)\n\t"  /* context->Xmm10 */
                   "movdqa %xmm11,0x250(%rcx)\n\t"  /* context->Xmm11 */
                   "movdqa %xmm12,0x260(%rcx)\n\t"  /* context->Xmm12 */
                   "movdqa %xmm13,0x270(%rcx)\n\t"  /* context->Xmm13 */
                   "movdqa %xmm14,0x280(%rcx)\n\t"  /* context->Xmm14 */
                   "movdqa %xmm15,0x290(%rcx)\n\t"  /* context->Xmm15 */
                   "ret" );

/***********************************************************************
 *           set_full_cpu_context
 *
 * Set the new CPU context.
 */
extern void set_full_cpu_context( const CONTEXT *context );
__ASM_GLOBAL_FUNC( set_full_cpu_context,
                   "subq $40,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 40\n\t")
                   "ldmxcsr 0x34(%rdi)\n\t"         /* context->MxCsr */
                   "movw 0x38(%rdi),%ax\n\t"        /* context->SegCs */
                   "movq %rax,8(%rsp)\n\t"
                   "movw 0x42(%rdi),%ax\n\t"        /* context->SegSs */
                   "movq %rax,32(%rsp)\n\t"
                   "movq 0x44(%rdi),%rax\n\t"       /* context->Eflags */
                   "movq %rax,16(%rsp)\n\t"
                   "movq 0x80(%rdi),%rcx\n\t"       /* context->Rcx */
                   "movq 0x88(%rdi),%rdx\n\t"       /* context->Rdx */
                   "movq 0x90(%rdi),%rbx\n\t"       /* context->Rbx */
                   "movq 0x98(%rdi),%rax\n\t"       /* context->Rsp */
                   "movq %rax,24(%rsp)\n\t"
                   "movq 0xa0(%rdi),%rbp\n\t"       /* context->Rbp */
                   "movq 0xa8(%rdi),%rsi\n\t"       /* context->Rsi */
                   "movq 0xb8(%rdi),%r8\n\t"        /* context->R8 */
                   "movq 0xc0(%rdi),%r9\n\t"        /* context->R9 */
                   "movq 0xc8(%rdi),%r10\n\t"       /* context->R10 */
                   "movq 0xd0(%rdi),%r11\n\t"       /* context->R11 */
                   "movq 0xd8(%rdi),%r12\n\t"       /* context->R12 */
                   "movq 0xe0(%rdi),%r13\n\t"       /* context->R13 */
                   "movq 0xe8(%rdi),%r14\n\t"       /* context->R14 */
                   "movq 0xf0(%rdi),%r15\n\t"       /* context->R15 */
                   "movq 0xf8(%rdi),%rax\n\t"       /* context->Rip */
                   "movq %rax,(%rsp)\n\t"
                   "fxrstor 0x100(%rdi)\n\t"        /* context->FtlSave */
                   "movdqa 0x1a0(%rdi),%xmm0\n\t"   /* context->Xmm0 */
                   "movdqa 0x1b0(%rdi),%xmm1\n\t"   /* context->Xmm1 */
                   "movdqa 0x1c0(%rdi),%xmm2\n\t"   /* context->Xmm2 */
                   "movdqa 0x1d0(%rdi),%xmm3\n\t"   /* context->Xmm3 */
                   "movdqa 0x1e0(%rdi),%xmm4\n\t"   /* context->Xmm4 */
                   "movdqa 0x1f0(%rdi),%xmm5\n\t"   /* context->Xmm5 */
                   "movdqa 0x200(%rdi),%xmm6\n\t"   /* context->Xmm6 */
                   "movdqa 0x210(%rdi),%xmm7\n\t"   /* context->Xmm7 */
                   "movdqa 0x220(%rdi),%xmm8\n\t"   /* context->Xmm8 */
                   "movdqa 0x230(%rdi),%xmm9\n\t"   /* context->Xmm9 */
                   "movdqa 0x240(%rdi),%xmm10\n\t"  /* context->Xmm10 */
                   "movdqa 0x250(%rdi),%xmm11\n\t"  /* context->Xmm11 */
                   "movdqa 0x260(%rdi),%xmm12\n\t"  /* context->Xmm12 */
                   "movdqa 0x270(%rdi),%xmm13\n\t"  /* context->Xmm13 */
                   "movdqa 0x280(%rdi),%xmm14\n\t"  /* context->Xmm14 */
                   "movdqa 0x290(%rdi),%xmm15\n\t"  /* context->Xmm15 */
                   "movq 0x78(%rdi),%rax\n\t"       /* context->Rax */
                   "movq 0xb0(%rdi),%rdi\n\t"       /* context->Rdi */
                   "iretq" );


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context. Used by NtSetContextThread.
 */
void DECLSPEC_HIDDEN set_cpu_context( const CONTEXT *context )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_AMD64;

    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        amd64_thread_data()->dr0 = context->Dr0;
        amd64_thread_data()->dr1 = context->Dr1;
        amd64_thread_data()->dr2 = context->Dr2;
        amd64_thread_data()->dr3 = context->Dr3;
        amd64_thread_data()->dr6 = context->Dr6;
        amd64_thread_data()->dr7 = context->Dr7;
    }
    if (flags & CONTEXT_FULL)
    {
        if (!(flags & CONTEXT_CONTROL))
            FIXME( "setting partial context (%x) not supported\n", flags );
        else
            set_full_cpu_context( context );
    }
}


/***********************************************************************
 *           get_server_context_flags
 *
 * Convert CPU-specific flags to generic server flags
 */
static unsigned int get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
    if (flags & CONTEXT_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
    if (flags & CONTEXT_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
    if (flags & CONTEXT_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
    return ret;
}


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
static void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Rbp    = from->Rbp;
        to->Rip    = from->Rip;
        to->Rsp    = from->Rsp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Rax = from->Rax;
        to->Rcx = from->Rcx;
        to->Rdx = from->Rdx;
        to->Rbx = from->Rbx;
        to->Rsi = from->Rsi;
        to->Rdi = from->Rdi;
        to->R8  = from->R8;
        to->R9  = from->R9;
        to->R10 = from->R10;
        to->R11 = from->R11;
        to->R12 = from->R12;
        to->R13 = from->R13;
        to->R14 = from->R14;
        to->R15 = from->R15;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->MxCsr = from->MxCsr;
        to->u.FltSave = from->u.FltSave;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->Dr0 = from->Dr0;
        to->Dr1 = from->Dr1;
        to->Dr2 = from->Dr2;
        to->Dr3 = from->Dr3;
        to->Dr6 = from->Dr6;
        to->Dr7 = from->Dr7;
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~CONTEXT_AMD64;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_x86_64;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.x86_64_regs.rbp   = from->Rbp;
        to->ctl.x86_64_regs.rip   = from->Rip;
        to->ctl.x86_64_regs.rsp   = from->Rsp;
        to->ctl.x86_64_regs.cs    = from->SegCs;
        to->ctl.x86_64_regs.ss    = from->SegSs;
        to->ctl.x86_64_regs.flags = from->EFlags;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.x86_64_regs.rax = from->Rax;
        to->integer.x86_64_regs.rcx = from->Rcx;
        to->integer.x86_64_regs.rdx = from->Rdx;
        to->integer.x86_64_regs.rbx = from->Rbx;
        to->integer.x86_64_regs.rsi = from->Rsi;
        to->integer.x86_64_regs.rdi = from->Rdi;
        to->integer.x86_64_regs.r8  = from->R8;
        to->integer.x86_64_regs.r9  = from->R9;
        to->integer.x86_64_regs.r10 = from->R10;
        to->integer.x86_64_regs.r11 = from->R11;
        to->integer.x86_64_regs.r12 = from->R12;
        to->integer.x86_64_regs.r13 = from->R13;
        to->integer.x86_64_regs.r14 = from->R14;
        to->integer.x86_64_regs.r15 = from->R15;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->flags |= SERVER_CTX_SEGMENTS;
        to->seg.x86_64_regs.ds = from->SegDs;
        to->seg.x86_64_regs.es = from->SegEs;
        to->seg.x86_64_regs.fs = from->SegFs;
        to->seg.x86_64_regs.gs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        memcpy( to->fp.x86_64_regs.fpregs, &from->u.FltSave, sizeof(to->fp.x86_64_regs.fpregs) );
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        to->debug.x86_64_regs.dr0 = from->Dr0;
        to->debug.x86_64_regs.dr1 = from->Dr1;
        to->debug.x86_64_regs.dr2 = from->Dr2;
        to->debug.x86_64_regs.dr3 = from->Dr3;
        to->debug.x86_64_regs.dr6 = from->Dr6;
        to->debug.x86_64_regs.dr7 = from->Dr7;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           context_from_server
 *
 * Convert a register context from the server format.
 */
NTSTATUS context_from_server( CONTEXT *to, const context_t *from )
{
    if (from->cpu != CPU_x86_64) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_AMD64;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Rbp    = from->ctl.x86_64_regs.rbp;
        to->Rip    = from->ctl.x86_64_regs.rip;
        to->Rsp    = from->ctl.x86_64_regs.rsp;
        to->SegCs  = from->ctl.x86_64_regs.cs;
        to->SegSs  = from->ctl.x86_64_regs.ss;
        to->EFlags = from->ctl.x86_64_regs.flags;
    }

    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->Rax = from->integer.x86_64_regs.rax;
        to->Rcx = from->integer.x86_64_regs.rcx;
        to->Rdx = from->integer.x86_64_regs.rdx;
        to->Rbx = from->integer.x86_64_regs.rbx;
        to->Rsi = from->integer.x86_64_regs.rsi;
        to->Rdi = from->integer.x86_64_regs.rdi;
        to->R8  = from->integer.x86_64_regs.r8;
        to->R9  = from->integer.x86_64_regs.r9;
        to->R10 = from->integer.x86_64_regs.r10;
        to->R11 = from->integer.x86_64_regs.r11;
        to->R12 = from->integer.x86_64_regs.r12;
        to->R13 = from->integer.x86_64_regs.r13;
        to->R14 = from->integer.x86_64_regs.r14;
        to->R15 = from->integer.x86_64_regs.r15;
    }
    if (from->flags & SERVER_CTX_SEGMENTS)
    {
        to->ContextFlags |= CONTEXT_SEGMENTS;
        to->SegDs = from->seg.x86_64_regs.ds;
        to->SegEs = from->seg.x86_64_regs.es;
        to->SegFs = from->seg.x86_64_regs.fs;
        to->SegGs = from->seg.x86_64_regs.gs;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        memcpy( &to->u.FltSave, from->fp.x86_64_regs.fpregs, sizeof(from->fp.x86_64_regs.fpregs) );
        to->MxCsr = to->u.FltSave.MxCsr;
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        to->Dr0 = from->debug.x86_64_regs.dr0;
        to->Dr1 = from->debug.x86_64_regs.dr1;
        to->Dr2 = from->debug.x86_64_regs.dr2;
        to->Dr3 = from->debug.x86_64_regs.dr3;
        to->Dr6 = from->debug.x86_64_regs.dr6;
        to->Dr7 = from->debug.x86_64_regs.dr7;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret = STATUS_SUCCESS;
    BOOL self = (handle == GetCurrentThread());

    /* debug registers require a server call */
    if (self && (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64)))
        self = (amd64_thread_data()->dr0 == context->Dr0 &&
                amd64_thread_data()->dr1 == context->Dr1 &&
                amd64_thread_data()->dr2 == context->Dr2 &&
                amd64_thread_data()->dr3 == context->Dr3 &&
                amd64_thread_data()->dr6 == context->Dr6 &&
                amd64_thread_data()->dr7 == context->Dr7);

    if (!self)
    {
        context_t server_context;
        context_to_server( &server_context, context );
        ret = set_thread_context( handle, &server_context, &self );
    }
    if (self && ret == STATUS_SUCCESS) set_cpu_context( context );
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD needed_flags;
    BOOL self = (handle == GetCurrentThread());

    if (!context) return STATUS_INVALID_PARAMETER;

    needed_flags = context->ContextFlags;

    /* debug registers require a server call */
    if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64)) self = FALSE;

    if (!self)
    {
        context_t server_context;
        unsigned int server_flags = get_server_context_flags( context->ContextFlags );

        if ((ret = get_thread_context( handle, &server_context, server_flags, &self ))) return ret;
        if ((ret = context_from_server( context, &server_context ))) return ret;
        needed_flags &= ~context->ContextFlags;
    }

    if (self)
    {
        if (needed_flags)
        {
            CONTEXT ctx;
            RtlCaptureContext( &ctx );
            copy_context( context, &ctx, ctx.ContextFlags & needed_flags );
            context->ContextFlags |= ctx.ContextFlags & needed_flags;
        }
        /* update the cached version of the debug registers */
        if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64))
        {
            amd64_thread_data()->dr0 = context->Dr0;
            amd64_thread_data()->dr1 = context->Dr1;
            amd64_thread_data()->dr2 = context->Dr2;
            amd64_thread_data()->dr3 = context->Dr3;
            amd64_thread_data()->dr6 = context->Dr6;
            amd64_thread_data()->dr7 = context->Dr7;
        }
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           wow64_get_server_context_flags
 */
static unsigned int wow64_get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~WOW64_CONTEXT_i386;  /* get rid of CPU id */
    if (flags & WOW64_CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & WOW64_CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
    if (flags & WOW64_CONTEXT_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
    if (flags & WOW64_CONTEXT_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
    if (flags & WOW64_CONTEXT_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
    if (flags & WOW64_CONTEXT_EXTENDED_REGISTERS) ret |= SERVER_CTX_EXTENDED_REGISTERS;
    return ret;
}

/***********************************************************************
 *           wow64_context_from_server
 */
static NTSTATUS wow64_context_from_server( WOW64_CONTEXT *to, const context_t *from )
{
    if (from->cpu != CPU_x86) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = WOW64_CONTEXT_i386;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= WOW64_CONTEXT_CONTROL;
        to->Ebp    = from->ctl.i386_regs.ebp;
        to->Esp    = from->ctl.i386_regs.esp;
        to->Eip    = from->ctl.i386_regs.eip;
        to->SegCs  = from->ctl.i386_regs.cs;
        to->SegSs  = from->ctl.i386_regs.ss;
        to->EFlags = from->ctl.i386_regs.eflags;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= WOW64_CONTEXT_INTEGER;
        to->Eax = from->integer.i386_regs.eax;
        to->Ebx = from->integer.i386_regs.ebx;
        to->Ecx = from->integer.i386_regs.ecx;
        to->Edx = from->integer.i386_regs.edx;
        to->Esi = from->integer.i386_regs.esi;
        to->Edi = from->integer.i386_regs.edi;
    }
    if (from->flags & SERVER_CTX_SEGMENTS)
    {
        to->ContextFlags |= WOW64_CONTEXT_SEGMENTS;
        to->SegDs = from->seg.i386_regs.ds;
        to->SegEs = from->seg.i386_regs.es;
        to->SegFs = from->seg.i386_regs.fs;
        to->SegGs = from->seg.i386_regs.gs;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= WOW64_CONTEXT_FLOATING_POINT;
        to->FloatSave.ControlWord   = from->fp.i386_regs.ctrl;
        to->FloatSave.StatusWord    = from->fp.i386_regs.status;
        to->FloatSave.TagWord       = from->fp.i386_regs.tag;
        to->FloatSave.ErrorOffset   = from->fp.i386_regs.err_off;
        to->FloatSave.ErrorSelector = from->fp.i386_regs.err_sel;
        to->FloatSave.DataOffset    = from->fp.i386_regs.data_off;
        to->FloatSave.DataSelector  = from->fp.i386_regs.data_sel;
        to->FloatSave.Cr0NpxState   = from->fp.i386_regs.cr0npx;
        memcpy( to->FloatSave.RegisterArea, from->fp.i386_regs.regs, sizeof(to->FloatSave.RegisterArea) );
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= WOW64_CONTEXT_DEBUG_REGISTERS;
        to->Dr0 = from->debug.i386_regs.dr0;
        to->Dr1 = from->debug.i386_regs.dr1;
        to->Dr2 = from->debug.i386_regs.dr2;
        to->Dr3 = from->debug.i386_regs.dr3;
        to->Dr6 = from->debug.i386_regs.dr6;
        to->Dr7 = from->debug.i386_regs.dr7;
    }
    if (from->flags & SERVER_CTX_EXTENDED_REGISTERS)
    {
        to->ContextFlags |= WOW64_CONTEXT_EXTENDED_REGISTERS;
        memcpy( to->ExtendedRegisters, from->ext.i386_regs, sizeof(to->ExtendedRegisters) );
    }
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           wow64_context_to_server
 */
static void wow64_context_to_server( context_t *to, const WOW64_CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~WOW64_CONTEXT_i386;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_x86;

    if (flags & WOW64_CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.i386_regs.ebp    = from->Ebp;
        to->ctl.i386_regs.esp    = from->Esp;
        to->ctl.i386_regs.eip    = from->Eip;
        to->ctl.i386_regs.cs     = from->SegCs;
        to->ctl.i386_regs.ss     = from->SegSs;
        to->ctl.i386_regs.eflags = from->EFlags;
    }
    if (flags & WOW64_CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.i386_regs.eax = from->Eax;
        to->integer.i386_regs.ebx = from->Ebx;
        to->integer.i386_regs.ecx = from->Ecx;
        to->integer.i386_regs.edx = from->Edx;
        to->integer.i386_regs.esi = from->Esi;
        to->integer.i386_regs.edi = from->Edi;
    }
    if (flags & WOW64_CONTEXT_SEGMENTS)
    {
        to->flags |= SERVER_CTX_SEGMENTS;
        to->seg.i386_regs.ds = from->SegDs;
        to->seg.i386_regs.es = from->SegEs;
        to->seg.i386_regs.fs = from->SegFs;
        to->seg.i386_regs.gs = from->SegGs;
    }
    if (flags & WOW64_CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        to->fp.i386_regs.ctrl     = from->FloatSave.ControlWord;
        to->fp.i386_regs.status   = from->FloatSave.StatusWord;
        to->fp.i386_regs.tag      = from->FloatSave.TagWord;
        to->fp.i386_regs.err_off  = from->FloatSave.ErrorOffset;
        to->fp.i386_regs.err_sel  = from->FloatSave.ErrorSelector;
        to->fp.i386_regs.data_off = from->FloatSave.DataOffset;
        to->fp.i386_regs.data_sel = from->FloatSave.DataSelector;
        to->fp.i386_regs.cr0npx   = from->FloatSave.Cr0NpxState;
        memcpy( to->fp.i386_regs.regs, from->FloatSave.RegisterArea, sizeof(to->fp.i386_regs.regs) );
    }
    if (flags & WOW64_CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        to->debug.i386_regs.dr0 = from->Dr0;
        to->debug.i386_regs.dr1 = from->Dr1;
        to->debug.i386_regs.dr2 = from->Dr2;
        to->debug.i386_regs.dr3 = from->Dr3;
        to->debug.i386_regs.dr6 = from->Dr6;
        to->debug.i386_regs.dr7 = from->Dr7;
    }
    if (flags & WOW64_CONTEXT_EXTENDED_REGISTERS)
    {
        to->flags |= SERVER_CTX_EXTENDED_REGISTERS;
        memcpy( to->ext.i386_regs, from->ExtendedRegisters, sizeof(to->ext.i386_regs) );
    }
}


/******************************************************************************
 *              RtlWow64GetThreadContext  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context )
{
    BOOL self;
    NTSTATUS ret;
    context_t server_context;
    unsigned int server_flags = wow64_get_server_context_flags( context->ContextFlags );

    if ((ret = get_thread_context( handle, &server_context, server_flags, &self ))) return ret;
    if (self) return STATUS_INVALID_PARAMETER;
    return wow64_context_from_server( context, &server_context );
}


/******************************************************************************
 *              RtlWow64SetThreadContext  (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64SetThreadContext( HANDLE handle, const WOW64_CONTEXT *context )
{
    BOOL self;
    context_t server_context;

    wow64_context_to_server( &server_context, context );
    return set_thread_context( handle, &server_context, &self );
}


extern void raise_func_trampoline( EXCEPTION_RECORD *rec, CONTEXT *context, raise_func func );
__ASM_GLOBAL_FUNC( raise_func_trampoline,
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa %rbp,160\n\t")  /* red zone + rip + rbp + rdi + rsi */
                   __ASM_CFI(".cfi_rel_offset %rip,24\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,16\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,0\n\t")
                   "call *%rdx\n\t"
                   "int $3")

/***********************************************************************
 *           setup_exception
 *
 * Setup a proper stack frame for the raise function, and modify the
 * sigcontext so that the return from the signal handler will call
 * the raise function.
 */
static EXCEPTION_RECORD *setup_exception( ucontext_t *sigcontext, raise_func func )
{
    struct stack_layout
    {
        CONTEXT           context;
        EXCEPTION_RECORD  rec;
        ULONG64           rsi;
        ULONG64           rdi;
        ULONG64           rbp;
        ULONG64           rip;
        ULONG64           red_zone[16];
    } *stack;
    ULONG64 *rsp_ptr;
    DWORD exception_code = 0;

    stack = (struct stack_layout *)(RSP_sig(sigcontext) & ~15);

    /* stack sanity checks */

    if (is_inside_signal_stack( stack ))
    {
        ERR( "nested exception on signal stack in thread %04x eip %016lx esp %016lx stack %p-%p\n",
             GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext), (ULONG_PTR)RSP_sig(sigcontext),
             NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }

    if (stack - 1 > stack || /* check for overflow in subtraction */
        (char *)stack <= (char *)NtCurrentTeb()->DeallocationStack ||
        (char *)stack > (char *)NtCurrentTeb()->Tib.StackBase)
    {
        WARN( "exception outside of stack limits in thread %04x eip %016lx esp %016lx stack %p-%p\n",
              GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext), (ULONG_PTR)RSP_sig(sigcontext),
              NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->DeallocationStack + 4096)
    {
        /* stack overflow on last page, unrecoverable */
        UINT diff = (char *)NtCurrentTeb()->DeallocationStack + 4096 - (char *)(stack - 1);
        ERR( "stack overflow %u bytes in thread %04x eip %016lx esp %016lx stack %p-%p-%p\n",
             diff, GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext),
             (ULONG_PTR)RSP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
             NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->Tib.StackLimit)
    {
        /* stack access below stack limit, may be recoverable */
        if (virtual_handle_stack_fault( stack - 1 )) exception_code = EXCEPTION_STACK_OVERFLOW;
        else
        {
            UINT diff = (char *)NtCurrentTeb()->Tib.StackLimit - (char *)(stack - 1);
            ERR( "stack overflow %u bytes in thread %04x eip %016lx esp %016lx stack %p-%p-%p\n",
                 diff, GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext),
                 (ULONG_PTR)RSP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            abort_thread(1);
        }
    }

    stack--;  /* push the stack_layout structure */
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_MAKE_MEM_UNDEFINED(stack, sizeof(*stack));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_MAKE_WRITABLE(stack, sizeof(*stack));
#endif
    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (void *)RIP_sig(sigcontext);
    stack->rec.NumberParameters = 0;
    save_context( &stack->context, sigcontext );

    /* store return address and %rbp without aligning, so that the offset is fixed */
    rsp_ptr = (ULONG64 *)RSP_sig(sigcontext) - 16;
    *(--rsp_ptr) = RIP_sig(sigcontext);
    *(--rsp_ptr) = RBP_sig(sigcontext);
    *(--rsp_ptr) = RDI_sig(sigcontext);
    *(--rsp_ptr) = RSI_sig(sigcontext);

    /* now modify the sigcontext to return to the raise function */
    RIP_sig(sigcontext) = (ULONG_PTR)raise_func_trampoline;
    RDI_sig(sigcontext) = (ULONG_PTR)&stack->rec;
    RSI_sig(sigcontext) = (ULONG_PTR)&stack->context;
    RDX_sig(sigcontext) = (ULONG_PTR)func;
    RBP_sig(sigcontext) = (ULONG_PTR)rsp_ptr;
    RSP_sig(sigcontext) = (ULONG_PTR)stack;
    /* clear single-step, direction, and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x400|0x40000);

    return &stack->rec;
}


/***********************************************************************
 *           get_exception_context
 *
 * Get a pointer to the context built by setup_exception.
 */
static inline CONTEXT *get_exception_context( EXCEPTION_RECORD *rec )
{
    return (CONTEXT *)rec - 1;  /* cf. stack_layout structure */
}


/**********************************************************************
 *           find_function_info
 */
static RUNTIME_FUNCTION *find_function_info( ULONG64 pc, HMODULE module,
                                             RUNTIME_FUNCTION *func, ULONG size )
{
    int min = 0;
    int max = size - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if ((char *)pc < (char *)module + func[pos].BeginAddress) max = pos - 1;
        else if ((char *)pc >= (char *)module + func[pos].EndAddress) min = pos + 1;
        else
        {
            func += pos;
            while (func->UnwindData & 1)  /* follow chained entry */
                func = (RUNTIME_FUNCTION *)((char *)module + (func->UnwindData & ~1));
            return func;
        }
    }
    return NULL;
}

/**********************************************************************
 *           lookup_function_info
 */
static RUNTIME_FUNCTION *lookup_function_info( ULONG64 pc, ULONG64 *base, LDR_MODULE **module )
{
    RUNTIME_FUNCTION *func = NULL;
    struct dynamic_unwind_entry *entry;
    ULONG size;

    /* PE module or wine module */
    if (!LdrFindEntryForAddress( (void *)pc, module ))
    {
        *base = (ULONG64)(*module)->BaseAddress;
        if ((func = RtlImageDirectoryEntryToData( (*module)->BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size )))
        {
            /* lookup in function table */
            func = find_function_info( pc, (*module)->BaseAddress, func, size/sizeof(*func) );
        }
    }
    else
    {
        *module = NULL;

        RtlEnterCriticalSection( &dynamic_unwind_section );
        LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
        {
            if (pc >= entry->base && pc < entry->end)
            {
                *base = entry->base;

                /* use callback or lookup in function table */
                if (entry->callback)
                    func = entry->callback( pc, entry->context );
                else
                    func = find_function_info( pc, (HMODULE)entry->base, entry->table, entry->count );
                break;
            }
        }
        RtlLeaveCriticalSection( &dynamic_unwind_section );
    }

    return func;
}

static DWORD __cdecl nested_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                               CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)) return ExceptionContinueSearch;

    /* FIXME */
    return ExceptionNestedException;
}

/**********************************************************************
 *           call_handler
 *
 * Call a single exception handler.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DWORD res;

    frame.Handler = nested_exception_handler;
    __wine_push_frame( &frame );

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
           dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = dispatch->LanguageHandler( rec, dispatch->EstablisherFrame, context, dispatch );
    TRACE( "handler at %p returned %u\n", dispatch->LanguageHandler, res );

    __wine_pop_frame( &frame );
    return res;
}


/**********************************************************************
 *           call_teb_handler
 *
 * Call a single exception handler from the TEB chain.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_teb_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                                  EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, context, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %u\n", teb_frame->Handler, res );
    return res;
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    LDR_MODULE *module;
    NTSTATUS status;

    context = *orig_context;
    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    for (;;)
    {
        /* FIXME: should use the history table to make things faster */

        dispatch.ImageBase = 0;
        dispatch.ControlPc = context.Rip;
        dispatch.ScopeIndex = 0;

        /* first look for PE exception information */

        if ((dispatch.FunctionEntry = lookup_function_info( dispatch.ControlPc, &dispatch.ImageBase, &module )))
        {
            dispatch.LanguageHandler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, dispatch.ImageBase,
                                                         dispatch.ControlPc, dispatch.FunctionEntry,
                                                         &context, &dispatch.HandlerData,
                                                         &dispatch.EstablisherFrame, NULL );
            goto unwind_done;
        }

        /* then look for host system exception information */

        if (!module || (module->Flags & LDR_WINE_INTERNAL))
        {
            BOOL got_info = FALSE;
            struct dwarf_eh_bases bases;
            const struct dwarf_fde *fde = _Unwind_Find_FDE( (void *)(dispatch.ControlPc - 1), &bases );

            if (fde)
            {
                status = dwarf_virtual_unwind( dispatch.ControlPc, &dispatch.EstablisherFrame, &context,
                                               fde, &bases, &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) return status;
                got_info = TRUE;
            }
#ifdef HAVE_LIBUNWIND_H
            else
            {
                status = libunwind_virtual_unwind( dispatch.ControlPc, &got_info, &dispatch.EstablisherFrame, &context,
                                                   &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) return status;
            }
#endif

            if (got_info)
            {
                dispatch.FunctionEntry = NULL;
                if (dispatch.LanguageHandler && !module)
                {
                    FIXME( "calling personality routine in system library not supported yet\n" );
                    dispatch.LanguageHandler = NULL;
                }
                goto unwind_done;
            }
        }
        else WARN( "exception data not found in %s\n", debugstr_w(module->BaseDllName.Buffer) );

        context.Rip = *(ULONG64 *)context.Rsp;
        context.Rsp = context.Rsp + sizeof(ULONG64);
        dispatch.EstablisherFrame = context.Rsp;
        dispatch.LanguageHandler = NULL;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if ((dispatch.EstablisherFrame & 7) ||
            dispatch.EstablisherFrame < (ULONG64)NtCurrentTeb()->Tib.StackLimit ||
            dispatch.EstablisherFrame > (ULONG64)NtCurrentTeb()->Tib.StackBase)
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            switch (call_handler( rec, orig_context, &dispatch ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                FIXME( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &context, NULL, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while ((ULONG64)teb_frame < context.Rsp)
        {
            TRACE( "found wine frame %p rsp %lx handler %p\n",
                    teb_frame, context.Rsp, teb_frame->Handler );
            dispatch.EstablisherFrame = (ULONG64)teb_frame;
            switch (call_teb_handler( rec, orig_context, &dispatch, teb_frame ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                FIXME( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &context, NULL, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Rsp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;

    if (first_chance)
    {
        DWORD c;

        TRACE( "code=%x flags=%x addr=%p ip=%lx tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Rip, GetCurrentThreadId() );
        for (c = 0; c < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); c++)
            TRACE( " info[%d]=%016lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
        else
        {
            TRACE(" rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
                  context->Rax, context->Rbx, context->Rcx, context->Rdx );
            TRACE(" rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
                  context->Rsi, context->Rdi, context->Rbp, context->Rsp );
            TRACE("  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
                  context->R8, context->R9, context->R10, context->R11 );
            TRACE(" r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
                  context->R12, context->R13, context->R14, context->R15 );
        }
        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED) goto done;

        /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
        if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Rip--;

        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION) goto done;

        if ((status = call_stack_handlers( rec, context )) == STATUS_SUCCESS) goto done;
        if (status != STATUS_UNHANDLED_EXCEPTION) return status;
    }

    /* last chance exception */

    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }

done:
    return NtSetContextThread( GetCurrentThread(), context );
}


/**********************************************************************
 *		raise_segv_exception
 */
static void raise_segv_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
        {
            if (!(rec->ExceptionCode = virtual_handle_fault( (void *)rec->ExceptionInformation[1],
                                                             rec->ExceptionInformation[0], FALSE )))
                set_cpu_context( context );
        }
        break;
    case EXCEPTION_BREAKPOINT:
        switch (rec->ExceptionInformation[0])
        {
            case 1: /* BREAKPOINT_PRINT */
            case 3: /* BREAKPOINT_LOAD_SYMBOLS */
            case 4: /* BREAKPOINT_UNLOAD_SYMBOLS */
            case 5: /* BREAKPOINT_COMMAND_STRING (>= Win2003) */
                set_cpu_context( context );
        }
        break;
    }
    status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
}


/**********************************************************************
 *		raise_trap_exception
 */
static void raise_trap_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        /* when single stepping can't tell whether this is a hw bp or a
         * single step interrupt. try to avoid as much overhead as possible
         * and only do a server call if there is any hw bp enabled. */

        if( !(context->EFlags & 0x100) || (amd64_thread_data()->dr7 & 0xff) )
        {
            /* (possible) hardware breakpoint, fetch the debug registers */
            DWORD saved_flags = context->ContextFlags;
            context->ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread(GetCurrentThread(), context);
            context->ContextFlags |= saved_flags;  /* restore flags */
        }

        context->EFlags &= ~0x100;  /* clear single-step flag */
    }

    status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
}


/**********************************************************************
 *		raise_generic_exception
 *
 * Generic raise function for exceptions that don't need special treatment.
 */
static void raise_generic_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 */
static inline DWORD is_privileged_instr( CONTEXT *context )
{
    const BYTE *instr = (BYTE *)context->Rip;
    unsigned int prefix_count = 0;

    for (;;) switch(*instr)
    {
    /* instruction prefixes */
    case 0x2e:  /* %cs: */
    case 0x36:  /* %ss: */
    case 0x3e:  /* %ds: */
    case 0x26:  /* %es: */
    case 0x40:  /* rex */
    case 0x41:  /* rex */
    case 0x42:  /* rex */
    case 0x43:  /* rex */
    case 0x44:  /* rex */
    case 0x45:  /* rex */
    case 0x46:  /* rex */
    case 0x47:  /* rex */
    case 0x48:  /* rex */
    case 0x49:  /* rex */
    case 0x4a:  /* rex */
    case 0x4b:  /* rex */
    case 0x4c:  /* rex */
    case 0x4d:  /* rex */
    case 0x4e:  /* rex */
    case 0x4f:  /* rex */
    case 0x64:  /* %fs: */
    case 0x65:  /* %gs: */
    case 0x66:  /* opcode size */
    case 0x67:  /* addr size */
    case 0xf0:  /* lock */
    case 0xf2:  /* repne */
    case 0xf3:  /* repe */
        if (++prefix_count >= 15) return EXCEPTION_ILLEGAL_INSTRUCTION;
        instr++;
        continue;

    case 0x0f: /* extended instruction */
        switch(instr[1])
        {
        case 0x06: /* clts */
        case 0x08: /* invd */
        case 0x09: /* wbinvd */
        case 0x20: /* mov crX, reg */
        case 0x21: /* mov drX, reg */
        case 0x22: /* mov reg, crX */
        case 0x23: /* mov reg drX */
            return EXCEPTION_PRIV_INSTRUCTION;
        }
        return 0;
    case 0x6c: /* insb (%dx) */
    case 0x6d: /* insl (%dx) */
    case 0x6e: /* outsb (%dx) */
    case 0x6f: /* outsl (%dx) */
    case 0xcd: /* int $xx */
    case 0xe4: /* inb al,XX */
    case 0xe5: /* in (e)ax,XX */
    case 0xe6: /* outb XX,al */
    case 0xe7: /* out XX,(e)ax */
    case 0xec: /* inb (%dx),%al */
    case 0xed: /* inl (%dx),%eax */
    case 0xee: /* outb %al,(%dx) */
    case 0xef: /* outl %eax,(%dx) */
    case 0xf4: /* hlt */
    case 0xfa: /* cli */
    case 0xfb: /* sti */
        return EXCEPTION_PRIV_INSTRUCTION;
    default:
        return 0;
    }
}


/***********************************************************************
 *           handle_interrupt
 *
 * Handle an interrupt.
 */
static inline BOOL handle_interrupt( unsigned int interrupt, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    switch(interrupt)
    {
    case 0x2c:
        rec->ExceptionCode = STATUS_ASSERTION_FAILURE;
        return TRUE;
    case 0x2d:
        context->Rip += 3;
        rec->ExceptionCode = EXCEPTION_BREAKPOINT;
        rec->ExceptionAddress = (void *)context->Rip;
        rec->NumberParameters = 1;
        rec->ExceptionInformation[0] = context->Rax;
        return TRUE;
    default:
        return FALSE;
    }
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec;
    ucontext_t *ucontext = sigcontext;

    /* check for page fault inside the thread stack */
    if (TRAP_sig(ucontext) == TRAP_x86_PAGEFLT &&
        (char *)siginfo->si_addr >= (char *)NtCurrentTeb()->DeallocationStack &&
        (char *)siginfo->si_addr < (char *)NtCurrentTeb()->Tib.StackBase &&
        virtual_handle_stack_fault( siginfo->si_addr ))
    {
        /* check if this was the last guard page */
        if ((char *)siginfo->si_addr < (char *)NtCurrentTeb()->DeallocationStack + 2*4096)
        {
            rec = setup_exception( sigcontext, raise_segv_exception );
            rec->ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        }
        return;
    }

    rec = setup_exception( sigcontext, raise_segv_exception );
    if (rec->ExceptionCode == EXCEPTION_STACK_OVERFLOW) return;

    switch(TRAP_sig(ucontext))
    {
    case TRAP_x86_OFLOW:   /* Overflow exception */
        rec->ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case TRAP_x86_BOUND:   /* Bound range exception */
        rec->ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case TRAP_x86_PRIVINFLT:   /* Invalid opcode exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case TRAP_x86_STKFLT:  /* Stack fault */
        rec->ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case TRAP_x86_SEGNPFLT:  /* Segment not present exception */
    case TRAP_x86_PROTFLT:   /* General protection fault */
    case TRAP_x86_UNKNOWN:   /* Unknown fault code */
        {
            CONTEXT *win_context = get_exception_context( rec );
            WORD err = ERROR_sig(ucontext);
            if (!err && (rec->ExceptionCode = is_privileged_instr( win_context ))) break;
            if ((err & 7) == 2 && handle_interrupt( err >> 3, rec, win_context )) break;
            rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            rec->NumberParameters = 2;
            rec->ExceptionInformation[0] = 0;
            rec->ExceptionInformation[1] = 0xffffffffffffffff;
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        rec->NumberParameters = 2;
        rec->ExceptionInformation[0] = (ERROR_sig(ucontext) >> 1) & 0x09;
        rec->ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        rec->ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %ld\n", (ULONG_PTR)TRAP_sig(ucontext) );
        /* fall through */
    case TRAP_x86_NMI:       /* NMI interrupt */
    case TRAP_x86_DNA:       /* Device not available exception */
    case TRAP_x86_DOUBLEFLT: /* Double fault exception */
    case TRAP_x86_TSSFLT:    /* Invalid TSS exception */
    case TRAP_x86_MCHK:      /* Machine check exception */
    case TRAP_x86_CACHEFLT:  /* Cache flush exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
}

/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_trap_exception );

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:  /* Single-step exception */
    case 4 /* TRAP_HWBKPT */: /* Hardware breakpoint exception */
        rec->ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:   /* Breakpoint exception */
#ifdef SI_KERNEL
    case SI_KERNEL:
#endif
        /* Check if this is actually icebp instruction */
        if (((unsigned char *)rec->ExceptionAddress)[-1] == 0xF1)
        {
            rec->ExceptionCode = EXCEPTION_SINGLE_STEP;
            break;
        }
        rec->ExceptionAddress = (char *)rec->ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec->ExceptionCode = EXCEPTION_BREAKPOINT;
        rec->NumberParameters = 1;
        rec->ExceptionInformation[0] = 0;
        break;
    }
}

/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_generic_exception );

    switch (siginfo->si_code)
    {
    case FPE_FLTSUB:
        rec->ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case FPE_INTDIV:
        rec->ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case FPE_INTOVF:
        rec->ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case FPE_FLTDIV:
        rec->ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
    case FPE_FLTOVF:
        rec->ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
    case FPE_FLTUND:
        rec->ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
    case FPE_FLTRES:
        rec->ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
    case FPE_FLTINV:
    default:
        rec->ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
}

/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_generic_exception );
        rec->ExceptionCode = CONTROL_C_EXIT;
    }
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_generic_exception );
    rec->ExceptionCode = EXCEPTION_WINE_ASSERTION;
    rec->ExceptionFlags = EH_NONCONTINUABLE;
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    CONTEXT context;

    save_context( &context, ucontext );
    wait_suspend( &context );
    restore_context( &context, ucontext );
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig >= ARRAY_SIZE(handlers)) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    static size_t sigstack_alignment;
    SIZE_T size;
    NTSTATUS status;

    if (!sigstack_alignment)
    {
        size_t min_size = teb_size + max( MINSIGSTKSZ, 8192 );
        /* find the first power of two not smaller than min_size */
        sigstack_alignment = 12;
        while ((1u << sigstack_alignment) < min_size) sigstack_alignment++;
        signal_stack_size = (1 << sigstack_alignment) - teb_size;
        assert( sizeof(TEB) <= teb_size );
    }

    size = 1 << sigstack_alignment;
    *teb = NULL;
    if (!(status = virtual_alloc_aligned( (void **)teb, 0, &size, MEM_COMMIT | MEM_TOP_DOWN,
                                          PAGE_READWRITE, sigstack_alignment )))
    {
        (*teb)->Tib.Self = &(*teb)->Tib;
        (*teb)->Tib.ExceptionList = (void *)~0UL;
    }
    return status;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size = 0;

    NtFreeVirtualMemory( NtCurrentProcess(), (void **)&teb, &size, MEM_RELEASE );
}

#ifdef __APPLE__
/**********************************************************************
 *		mac_thread_gsbase
 */
static void *mac_thread_gsbase(void)
{
    static int gsbase_offset = -1;
    void *ret;

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
        if ((rc = pthread_key_create(&key, NULL)))
        {
            ERR("failed to create sentinel key for gsbase search: %d\n", rc);
            return NULL;
        }

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

    if (gsbase_offset)
    {
        ret = (char*)pthread_self() + gsbase_offset;
        TRACE("pthread_self() %p + offset 0x%08x -> gsbase %p\n", pthread_self(), gsbase_offset, ret);
    }
    else
    {
        ret = NULL;
        ERR("failed to locate gsbase; won't be able to poke ThreadLocalStoragePointer into pthread TLS; expect crashes\n");
    }

    return ret;
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

/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

    sig_act.sa_sigaction = int_handler;
    if (sigaction( SIGINT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = fpe_handler;
    if (sigaction( SIGFPE, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = abrt_handler;
    if (sigaction( SIGABRT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = quit_handler;
    if (sigaction( SIGQUIT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = usr1_handler;
    if (sigaction( SIGUSR1, &sig_act, NULL ) == -1) goto error;

    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
#ifdef SIGBUS
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
#endif

#ifdef SIGTRAP
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
#endif
    return;

 error:
    perror("sigaction");
    exit(1);
}


/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, DWORD64 addr )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p %u %lx\n", table, count, addr );

    /* NOTE: Windows doesn't check if table is aligned or a NULL pointer */

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return FALSE;

    entry->base      = addr;
    entry->end       = addr + (count ? table[count - 1].EndAddress : 0);
    entry->table     = table;
    entry->count     = count;
    entry->max_count = 0;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    return TRUE;
}


/**********************************************************************
 *              RtlInstallFunctionTableCallback   (NTDLL.@)
 */
BOOLEAN CDECL RtlInstallFunctionTableCallback( DWORD64 table, DWORD64 base, DWORD length,
                                               PGET_RUNTIME_FUNCTION_CALLBACK callback, PVOID context, PCWSTR dll )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%lx %lx %d %p %p %s\n", table, base, length, callback, context, wine_dbgstr_w(dll) );

    /* NOTE: Windows doesn't check if the provided callback is a NULL pointer */

    /* both low-order bits must be set */
    if ((table & 0x3) != 0x3)
        return FALSE;

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return FALSE;

    entry->base      = base;
    entry->end       = base + length;
    entry->table     = (RUNTIME_FUNCTION *)table;
    entry->count     = 0;
    entry->max_count = 0;
    entry->callback  = callback;
    entry->context   = context;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    return TRUE;
}


/*************************************************************************
 *              RtlAddGrowableFunctionTable   (NTDLL.@)
 */
DWORD WINAPI RtlAddGrowableFunctionTable( void **table, RUNTIME_FUNCTION *functions, DWORD count, DWORD max_count,
                                          ULONG_PTR base, ULONG_PTR end )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %p, %u, %u, %lx, %lx\n", table, functions, count, max_count, base, end );

    entry = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return STATUS_NO_MEMORY;

    entry->base      = base;
    entry->end       = end;
    entry->table     = functions;
    entry->count     = count;
    entry->max_count = max_count;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    *table = entry;

    return STATUS_SUCCESS;
}


/*************************************************************************
 *              RtlGrowFunctionTable   (NTDLL.@)
 */
void WINAPI RtlGrowFunctionTable( void *table, DWORD count )
{
    struct dynamic_unwind_entry *entry;

    TRACE( "%p, %u\n", table, count );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            if (count > entry->count && count <= entry->max_count)
                entry->count = count;
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );
}


/*************************************************************************
 *              RtlDeleteGrowableFunctionTable   (NTDLL.@)
 */
void WINAPI RtlDeleteGrowableFunctionTable( void *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
}


/**********************************************************************
 *              RtlDeleteFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlDeleteFunctionTable( RUNTIME_FUNCTION *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    TRACE( "%p\n", table );

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry->table == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    if (!to_free)
        return FALSE;

    RtlFreeHeap( GetProcessHeap(), 0, to_free );
    return TRUE;
}


/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG64 pc, ULONG64 *base, UNWIND_HISTORY_TABLE *table )
{
    LDR_MODULE *module;
    RUNTIME_FUNCTION *func;

    /* FIXME: should use the history table to make things faster */

    func = lookup_function_info( pc, base, &module );
    if (!func)
    {
        *base = 0;
        if (module)
            WARN( "no exception table found in module %p pc %lx\n", module->BaseAddress, pc );
        else
            WARN( "module not found for %lx\n", pc );
    }

    return func;
}

static ULONG64 get_int_reg( CONTEXT *context, int reg )
{
    return *(&context->Rax + reg);
}

static void set_int_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, ULONG64 *val )
{
    *(&context->Rax + reg) = *val;
    if (ctx_ptr) ctx_ptr->u2.IntegerContext[reg] = val;
}

static void set_float_reg( CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr, int reg, M128A *val )
{
    *(&context->u.s.Xmm0 + reg) = *val;
    if (ctx_ptr) ctx_ptr->u.FloatingContext[reg] = val;
}

static int get_opcode_size( struct opcode op )
{
    switch (op.code)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.info != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

static BOOL is_inside_epilog( BYTE *pc, ULONG64 base, const RUNTIME_FUNCTION *function )
{
    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((pc[0] & 0xf8) == 0x48)
    {
        switch (pc[1])
        {
        case 0x81: /* add $nnnn,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (pc[0] == 0x48 && pc[2] == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (pc[0] & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((pc[2] >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((pc[2] & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((pc[2] >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((pc[2] >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */

    for (;;)
    {
        if ((*pc & 0xf0) == 0x40) pc++;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            if (pc - (BYTE *)base >= function->BeginAddress && pc - (BYTE *)base < function->EndAddress)
                continue;
            break;
        case 0xf3: /* rep; ret (for amd64 prediction bug) */
            return pc[1] == 0xc3;
        }
        return FALSE;
    }
}

/* execute a function epilog, which must have been validated with is_inside_epilog() */
static void interpret_epilog( BYTE *pc, CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    for (;;)
    {
        BYTE rex = 0;

        if ((*pc & 0xf0) == 0x40) rex = *pc++ & 0x0f;  /* rex prefix */

        switch (*pc)
        {
        case 0x58: /* pop %rax/r8 */
        case 0x59: /* pop %rcx/r9 */
        case 0x5a: /* pop %rdx/r10 */
        case 0x5b: /* pop %rbx/r11 */
        case 0x5c: /* pop %rsp/r12 */
        case 0x5d: /* pop %rbp/r13 */
        case 0x5e: /* pop %rsi/r14 */
        case 0x5f: /* pop %rdi/r15 */
            set_int_reg( context, ctx_ptr, *pc - 0x58 + (rex & 1) * 8, (ULONG64 *)context->Rsp );
            context->Rsp += sizeof(ULONG64);
            pc++;
            continue;
        case 0x81: /* add $nnnn,%rsp */
            context->Rsp += *(LONG *)(pc + 2);
            pc += 2 + sizeof(LONG);
            continue;
        case 0x83: /* add $n,%rsp */
            context->Rsp += (signed char)pc[2];
            pc += 3;
            continue;
        case 0x8d:
            if ((pc[1] >> 6) == 1)  /* lea n(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + (signed char)pc[2];
                pc += 3;
            }
            else  /* lea nnnn(reg),%rsp */
            {
                context->Rsp = get_int_reg( context, (pc[1] & 7) + (rex & 1) * 8 ) + *(LONG *)(pc + 2);
                pc += 2 + sizeof(LONG);
            }
            continue;
        case 0xc2: /* ret $nn */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64) + *(WORD *)(pc + 1);
            return;
        case 0xc3: /* ret */
        case 0xf3: /* rep; ret */
            context->Rip = *(ULONG64 *)context->Rsp;
            context->Rsp += sizeof(ULONG64);
            return;
        case 0xe9: /* jmp nnnn */
            pc += 5 + *(LONG *)(pc + 1);
            continue;
        case 0xeb: /* jmp n */
            pc += 2 + (signed char)pc[1];
            continue;
        }
        return;
    }
}

/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG64 base, ULONG64 pc,
                               RUNTIME_FUNCTION *function, CONTEXT *context,
                               PVOID *data, ULONG64 *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    union handler_data *handler_data;
    ULONG64 frame, off;
    struct UNWIND_INFO *info;
    unsigned int i, prolog_offset;

    TRACE( "type %x rip %lx rsp %lx\n", type, pc, context->Rsp );
    if (TRACE_ON(seh)) dump_unwind_info( base, function );

    frame = *frame_ret = context->Rsp;
    for (;;)
    {
        info = (struct UNWIND_INFO *)((char *)base + function->UnwindData);
        handler_data = (union handler_data *)&info->opcodes[(info->count + 1) & ~1];

        if (info->version != 1)
        {
            FIXME( "unknown unwind info version %u at %p\n", info->version, info );
            return NULL;
        }

        if (info->frame_reg)
            frame = get_int_reg( context, info->frame_reg ) - info->frame_offset * 16;

        /* check if in prolog */
        if (pc >= base + function->BeginAddress && pc < base + function->BeginAddress + info->prolog)
        {
            prolog_offset = pc - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            if (is_inside_epilog( (BYTE *)pc, base, function ))
            {
                interpret_epilog( (BYTE *)pc, context, ctx_ptr );
                *frame_ret = frame;
                return NULL;
            }
        }

        for (i = 0; i < info->count; i += get_opcode_size(info->opcodes[i]))
        {
            if (prolog_offset < info->opcodes[i].offset) continue; /* skip it */

            switch (info->opcodes[i].code)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)context->Rsp );
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->opcodes[i].info) context->Rsp += *(DWORD *)&info->opcodes[i+1];
                else context->Rsp += *(USHORT *)&info->opcodes[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->opcodes[i].info + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = *frame_ret = frame;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 8;
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_int_reg( context, ctx_ptr, info->opcodes[i].info, (ULONG64 *)off );
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = frame + *(USHORT *)&info->opcodes[i+1] * 16;
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = frame + *(DWORD *)&info->opcodes[i+1];
                set_float_reg( context, ctx_ptr, info->opcodes[i].info, (M128A *)off );
                break;
            case UWOP_PUSH_MACHFRAME:
                FIXME( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );
                break;
            default:
                FIXME( "unknown code %u\n", info->opcodes[i].code );
                break;
            }
        }

        if (!(info->flags & UNW_FLAG_CHAININFO)) break;
        function = &handler_data->chain;  /* restart with the chained info */
    }

    /* now pop return address */
    context->Rip = *(ULONG64 *)context->Rsp;
    context->Rsp += sizeof(ULONG64);

    if (!(info->flags & type)) return NULL;  /* no matching handler */
    if (prolog_offset != ~0) return NULL;  /* inside prolog */

    *data = &handler_data->handler + 1;
    return (char *)base + handler_data->handler;
}

struct unwind_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DISPATCHER_CONTEXT *dispatch;
};

/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
static DWORD __cdecl unwind_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                               CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct unwind_exception_frame *unwind_frame = (struct unwind_exception_frame *)frame;
    DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

    /* copy the original dispatcher into the current one, except for the TargetIp */
    dispatch->ControlPc        = unwind_frame->dispatch->ControlPc;
    dispatch->ImageBase        = unwind_frame->dispatch->ImageBase;
    dispatch->FunctionEntry    = unwind_frame->dispatch->FunctionEntry;
    dispatch->EstablisherFrame = unwind_frame->dispatch->EstablisherFrame;
    dispatch->ContextRecord    = unwind_frame->dispatch->ContextRecord;
    dispatch->LanguageHandler  = unwind_frame->dispatch->LanguageHandler;
    dispatch->HandlerData      = unwind_frame->dispatch->HandlerData;
    dispatch->HistoryTable     = unwind_frame->dispatch->HistoryTable;
    dispatch->ScopeIndex       = unwind_frame->dispatch->ScopeIndex;
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}

/**********************************************************************
 *           call_unwind_handler
 *
 * Call a single unwind handler.
 */
static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    struct unwind_exception_frame frame;
    DWORD res;

    frame.frame.Handler = unwind_exception_handler;
    frame.dispatch = dispatch;
    __wine_push_frame( &frame.frame );

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
         dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = dispatch->LanguageHandler( rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    TRACE( "handler %p returned %x\n", dispatch->LanguageHandler, res );

    __wine_pop_frame( &frame.frame );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_teb_unwind_handler
 *
 * Call a single unwind handler from the TEB chain.
 */
static DWORD call_teb_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch,
                                     EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, dispatch->ContextRecord, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %u\n", teb_frame->Handler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from somewhere
 * else. All CFI instructions are either DW_CFA_def_cfa_expression or
 * DW_CFA_expression, and the expressions have the following format:
 *
 * DW_OP_breg6; sleb128 0x10            | Load %rbp + 0x10
 * DW_OP_deref                          | Get *(%rbp + 0x10) == context
 * DW_OP_plus_uconst; uleb128 <OFFSET>  | Add offset to get struct member
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "subq $0x20,%rsp\n\t"
                   "movq %rcx,0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x07,0x76,0x10,0x06,0x23,0x98,0x01,0x06\n\t") /* CFA    */
                   __ASM_CFI(".cfi_escape 0x10,0x03,0x06,0x76,0x10,0x06,0x23,0x90,0x01\n\t") /* %rbx   */
                   __ASM_CFI(".cfi_escape 0x10,0x04,0x06,0x76,0x10,0x06,0x23,0xa8,0x01\n\t") /* %rsi   */
                   __ASM_CFI(".cfi_escape 0x10,0x05,0x06,0x76,0x10,0x06,0x23,0xb0,0x01\n\t") /* %rdi   */
                   __ASM_CFI(".cfi_escape 0x10,0x06,0x06,0x76,0x10,0x06,0x23,0xa0,0x01\n\t") /* %rbp   */
                   __ASM_CFI(".cfi_escape 0x10,0x0c,0x06,0x76,0x10,0x06,0x23,0xd8,0x01\n\t") /* %r12   */
                   __ASM_CFI(".cfi_escape 0x10,0x0d,0x06,0x76,0x10,0x06,0x23,0xe0,0x01\n\t") /* %r13   */
                   __ASM_CFI(".cfi_escape 0x10,0x0e,0x06,0x76,0x10,0x06,0x23,0xe8,0x01\n\t") /* %r14   */
                   __ASM_CFI(".cfi_escape 0x10,0x0f,0x06,0x76,0x10,0x06,0x23,0xf0,0x01\n\t") /* %r15   */
                   __ASM_CFI(".cfi_escape 0x10,0x10,0x06,0x76,0x10,0x06,0x23,0xf8,0x01\n\t") /* %rip   */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x06,0x76,0x10,0x06,0x23,0x80,0x04\n\t") /* %xmm6  */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x06,0x76,0x10,0x06,0x23,0x90,0x04\n\t") /* %xmm7  */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x06,0x76,0x10,0x06,0x23,0xa0,0x04\n\t") /* %xmm8  */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x06,0x76,0x10,0x06,0x23,0xb0,0x04\n\t") /* %xmm9  */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x06,0x76,0x10,0x06,0x23,0xc0,0x04\n\t") /* %xmm10 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x06,0x76,0x10,0x06,0x23,0xd0,0x04\n\t") /* %xmm11 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x06,0x76,0x10,0x06,0x23,0xe0,0x04\n\t") /* %xmm12 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x06,0x76,0x10,0x06,0x23,0xf0,0x04\n\t") /* %xmm13 */
                   __ASM_CFI(".cfi_escape 0x10,0x1f,0x06,0x76,0x10,0x06,0x23,0x80,0x05\n\t") /* %xmm14 */
                   __ASM_CFI(".cfi_escape 0x10,0x20,0x06,0x76,0x10,0x06,0x23,0x90,0x05\n\t") /* %xmm15 */
                   "movq %r8,%rcx\n\t"
                   "callq *%rdx\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "movq %rbp,%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")

/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void WINAPI RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct MSVCRT_JUMP_BUFFER *jmp = (struct MSVCRT_JUMP_BUFFER *)rec->ExceptionInformation[0];
        context->Rbx       = jmp->Rbx;
        context->Rsp       = jmp->Rsp;
        context->Rbp       = jmp->Rbp;
        context->Rsi       = jmp->Rsi;
        context->Rdi       = jmp->Rdi;
        context->R12       = jmp->R12;
        context->R13       = jmp->R13;
        context->R14       = jmp->R14;
        context->R15       = jmp->R15;
        context->Rip       = jmp->Rip;
        context->u.s.Xmm6  = jmp->Xmm6;
        context->u.s.Xmm7  = jmp->Xmm7;
        context->u.s.Xmm8  = jmp->Xmm8;
        context->u.s.Xmm9  = jmp->Xmm9;
        context->u.s.Xmm10 = jmp->Xmm10;
        context->u.s.Xmm11 = jmp->Xmm11;
        context->u.s.Xmm12 = jmp->Xmm12;
        context->u.s.Xmm13 = jmp->Xmm13;
        context->u.s.Xmm14 = jmp->Xmm14;
        context->u.s.Xmm15 = jmp->Xmm15;
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        context->Rip = (ULONG64)call_consolidate_callback( context, consolidate, rec );
    }

    /* hack: remove no longer accessible TEB frames */
    while ((ULONG64)teb_frame < context->Rsp)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %lx stack %lx\n", context->Rip, context->Rsp );
    set_cpu_context( context );
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    LDR_MODULE *module;
    NTSTATUS status;
    DWORD i;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (end_frame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%x flags=%x end_frame=%p target_ip=%p rip=%016lx\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip, context->Rip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%d]=%016lx\n", i, rec->ExceptionInformation[i] );
    TRACE(" rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
          context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE(" rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
          context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE("  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
          context->R8, context->R9, context->R10, context->R11 );
    TRACE(" r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
          context->R12, context->R13, context->R14, context->R15 );

    dispatch.EstablisherFrame = context->Rsp;
    dispatch.TargetIp         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;

    for (;;)
    {
        /* FIXME: should use the history table to make things faster */

        dispatch.ImageBase = 0;
        dispatch.ScopeIndex = 0;
        dispatch.ControlPc = context->Rip;

        /* first look for PE exception information */

        if ((dispatch.FunctionEntry = lookup_function_info( context->Rip, &dispatch.ImageBase, &module )))
        {
            dispatch.LanguageHandler = RtlVirtualUnwind( UNW_FLAG_UHANDLER, dispatch.ImageBase,
                                                         context->Rip, dispatch.FunctionEntry,
                                                         &new_context, &dispatch.HandlerData,
                                                         &dispatch.EstablisherFrame, NULL );
            goto unwind_done;
        }

        /* then look for host system exception information */

        if (!module || (module->Flags & LDR_WINE_INTERNAL))
        {
            BOOL got_info = FALSE;
            struct dwarf_eh_bases bases;
            const struct dwarf_fde *fde = _Unwind_Find_FDE( (void *)(context->Rip - 1), &bases );

            if (fde)
            {
                status = dwarf_virtual_unwind( context->Rip, &dispatch.EstablisherFrame, &new_context, fde,
                                               &bases, &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) raise_status( status, rec );
                got_info = TRUE;
            }
#ifdef HAVE_LIBUNWIND_H
            else
            {
                status = libunwind_virtual_unwind( context->Rip, &got_info, &dispatch.EstablisherFrame, &new_context,
                                                   &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) raise_status( status, rec );
            }
#endif

            if (got_info)
            {
                dispatch.FunctionEntry = NULL;
                if (dispatch.LanguageHandler && !module)
                {
                    FIXME( "calling personality routine in system library not supported yet\n" );
                    dispatch.LanguageHandler = NULL;
                }
                goto unwind_done;
            }
        }
        else WARN( "exception data not found in %s\n", debugstr_w(module->BaseDllName.Buffer) );

        /* no exception information, treat as a leaf function */

        new_context.Rip = *(ULONG64 *)context->Rsp;
        new_context.Rsp = context->Rsp + sizeof(ULONG64);
        dispatch.EstablisherFrame = context->Rsp;
        dispatch.LanguageHandler = NULL;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (is_inside_signal_stack( (void *)dispatch.EstablisherFrame ))
        {
            TRACE( "frame %lx is inside signal stack (%p-%p)\n", dispatch.EstablisherFrame,
                   get_signal_stack(), (char *)get_signal_stack() + signal_stack_size );
            *context = new_context;
            continue;
        }

        if ((dispatch.EstablisherFrame & 7) ||
            dispatch.EstablisherFrame < (ULONG64)NtCurrentTeb()->Tib.StackLimit ||
            dispatch.EstablisherFrame > (ULONG64)NtCurrentTeb()->Tib.StackBase)
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %lx/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG64 frame;

                *context = new_context = *dispatch.ContextRecord;
                dispatch.ContextRecord = context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                        dispatch.ControlPc, dispatch.FunctionEntry,
                        &new_context, NULL, &frame, NULL );
                rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                goto unwind_done;
            }
            rec->ExceptionFlags &= ~EH_COLLIDED_UNWIND;
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            DWORD64 backup_frame = dispatch.EstablisherFrame;
            while ((ULONG64)teb_frame < new_context.Rsp && (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "found builtin frame %p handler %p\n", teb_frame, teb_frame->Handler );
                dispatch.EstablisherFrame = (ULONG64)teb_frame;
                if (call_teb_unwind_handler( rec, &dispatch, teb_frame ) == ExceptionCollidedUnwind)
                {
                    ULONG64 frame;

                    teb_frame = __wine_pop_frame( teb_frame );

                    *context = new_context = *dispatch.ContextRecord;
                    dispatch.ContextRecord = context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                            dispatch.ControlPc, dispatch.FunctionEntry,
                            &new_context, NULL, &frame, NULL );
                    rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                    goto unwind_done;
                }
                teb_frame = __wine_pop_frame( teb_frame );
            }
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Rsp) break;
            dispatch.EstablisherFrame = backup_frame;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    context->Rax = (ULONG64)retval;
    context->Rip = (ULONG64)target_ip;
    RtlRestoreContext(context, rec);
}


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}


/*******************************************************************
 *		_local_unwind (NTDLL.@)
 */
void WINAPI _local_unwind( void *frame, void *target_ip )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, NULL, NULL, &context, NULL );
}

/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec,
                                                   ULONG64 frame,
                                                   CONTEXT *context,
                                                   struct _DISPATCHER_CONTEXT *dispatch )
{
    SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG i;

    TRACE( "%p %lx %p %p\n", rec, frame, context, dispatch );
    if (TRACE_ON(seh)) dump_scope_table( dispatch->ImageBase, table );

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        for (i = dispatch->ScopeIndex; i < table->Count; i++)
        {
            if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
            {
                TERMINATION_HANDLER handler;

                if (table->ScopeRecord[i].JumpTarget) continue;

                if (rec->ExceptionFlags & EH_TARGET_UNWIND &&
                    dispatch->TargetIp >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
                    dispatch->TargetIp < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
                {
                    break;
                }

                handler = (TERMINATION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                dispatch->ScopeIndex = i+1;

                TRACE( "calling __finally %p frame %lx\n", handler, frame );
                handler( 1, frame );
            }
        }
        return ExceptionContinueSearch;
    }

    for (i = dispatch->ScopeIndex; i < table->Count; i++)
    {
        if (dispatch->ControlPc >= dispatch->ImageBase + table->ScopeRecord[i].BeginAddress &&
            dispatch->ControlPc < dispatch->ImageBase + table->ScopeRecord[i].EndAddress)
        {
            if (!table->ScopeRecord[i].JumpTarget) continue;
            if (table->ScopeRecord[i].HandlerAddress != EXCEPTION_EXECUTE_HANDLER)
            {
                EXCEPTION_POINTERS ptrs;
                PC_LANGUAGE_EXCEPTION_HANDLER filter;

                filter = (PC_LANGUAGE_EXCEPTION_HANDLER)(dispatch->ImageBase + table->ScopeRecord[i].HandlerAddress);
                ptrs.ExceptionRecord = rec;
                ptrs.ContextRecord = context;
                TRACE( "calling filter %p ptrs %p frame %lx\n", filter, &ptrs, frame );
                switch (filter( &ptrs, frame ))
                {
                case EXCEPTION_EXECUTE_HANDLER:
                    break;
                case EXCEPTION_CONTINUE_SEARCH:
                    continue;
                case EXCEPTION_CONTINUE_EXECUTION:
                    return ExceptionContinueExecution;
                }
            }
            TRACE( "unwinding to target %lx\n", dispatch->ImageBase + table->ScopeRecord[i].JumpTarget );
            RtlUnwindEx( (void *)frame, (char *)dispatch->ImageBase + table->ScopeRecord[i].JumpTarget,
                         rec, 0, dispatch->ContextRecord, dispatch->HistoryTable );
        }
    }
    return ExceptionContinueSearch;
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlRaiseException,
                   "movq %rcx,8(%rsp)\n\t"
                   "sub $0x4f8,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x4f8\n\t")
                   "leaq 0x20(%rsp),%rcx\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "leaq 0x20(%rsp),%rdx\n\t"   /* context pointer */
                   "leaq 0x500(%rsp),%rax\n\t"  /* orig stack pointer */
                   "movq %rax,0x98(%rdx)\n\t"   /* context->Rsp */
                   "movq (%rax),%rcx\n\t"       /* original first parameter */
                   "movq %rcx,0x80(%rdx)\n\t"   /* context->Rcx */
                   "movq 0x4f8(%rsp),%rax\n\t"  /* return address */
                   "movq %rax,0xf8(%rdx)\n\t"   /* context->Rip */
                   "movq %rax,0x10(%rcx)\n\t"   /* rec->ExceptionAddress */
                   "movl $1,%r8d\n\t"
                   "call " __ASM_NAME("NtRaiseException") "\n\t"
                   "movq %rax,%rcx\n\t"
                   "call " __ASM_NAME("RtlRaiseStatus") /* does not return */ );


static inline ULONG hash_pointers( void **ptrs, ULONG count )
{
    /* Based on MurmurHash2, which is in the public domain */
    static const ULONG m = 0x5bd1e995;
    static const ULONG r = 24;
    ULONG hash = count * sizeof(void*);
    for (; count > 0; ptrs++, count--)
    {
        ULONG_PTR data = (ULONG_PTR)*ptrs;
        ULONG k1 = (ULONG)(data & 0xffffffff), k2 = (ULONG)(data >> 32);
        k1 *= m;
        k1 = (k1 ^ (k1 >> r)) * m;
        k2 *= m;
        k2 = (k2 ^ (k2 >> r)) * m;
        hash = (((hash * m) ^ k1) * m) ^ k2;
    }
    hash = (hash ^ (hash >> 13)) * m;
    return hash ^ (hash >> 15);
}


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    LDR_MODULE *module;
    NTSTATUS status;
    ULONG i;
    USHORT num_entries = 0;

    TRACE( "(%u, %u, %p, %p)\n", skip, count, buffer, hash );

    RtlCaptureContext( &context );
    dispatch.TargetIp      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    if (hash) *hash = 0;
    for (i = 0; i < skip + count; i++)
    {
        /* FIXME: should use the history table to make things faster */

        dispatch.ImageBase = 0;
        dispatch.ControlPc = context.Rip;
        dispatch.ScopeIndex = 0;

        /* first look for PE exception information */

        if ((dispatch.FunctionEntry = lookup_function_info( dispatch.ControlPc, &dispatch.ImageBase, &module )))
        {
            RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase, dispatch.ControlPc,
                              dispatch.FunctionEntry, &context, &dispatch.HandlerData,
                              &dispatch.EstablisherFrame, NULL );
            goto unwind_done;
        }

        /* then look for host system exception information */

        if (!module || (module->Flags & LDR_WINE_INTERNAL))
        {
            BOOL got_info = FALSE;
            struct dwarf_eh_bases bases;
            const struct dwarf_fde *fde = _Unwind_Find_FDE( (void *)(dispatch.ControlPc - 1), &bases );

            if (fde)
            {
                status = dwarf_virtual_unwind( dispatch.ControlPc, &dispatch.EstablisherFrame, &context,
                                               fde, &bases, &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) return status;
                got_info = TRUE;
            }
#ifdef HAVE_LIBUNWIND_H
            else
            {
                status = libunwind_virtual_unwind( dispatch.ControlPc, &got_info, &dispatch.EstablisherFrame, &context,
                                                   &dispatch.LanguageHandler, &dispatch.HandlerData );
                if (status != STATUS_SUCCESS) return i;
            }
#endif

            if (got_info)
            {
                dispatch.FunctionEntry = NULL;
                goto unwind_done;
            }
        }
        else WARN( "exception data not found in %s\n", debugstr_w(module->BaseDllName.Buffer) );

        context.Rip = *(ULONG64 *)context.Rsp;
        context.Rsp = context.Rsp + sizeof(ULONG64);
        dispatch.EstablisherFrame = context.Rsp;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if ((dispatch.EstablisherFrame & 7) ||
            dispatch.EstablisherFrame < (ULONG64)NtCurrentTeb()->Tib.StackLimit ||
            dispatch.EstablisherFrame > (ULONG64)NtCurrentTeb()->Tib.StackBase)
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            break;
        }

        if (context.Rsp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;

        if (i >= skip) buffer[num_entries++] = (void *)context.Rip;
    }
    if (hash && num_entries > 0) *hash = hash_pointers( buffer, num_entries );
    TRACE( "captured %hu frames\n", num_entries );
    return num_entries;
}


/***********************************************************************
 *           call_thread_func
 */
static void WINAPI call_thread_func( LPTHREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        RtlExitUserThread( entry( arg ));
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}


extern void DECLSPEC_NORETURN start_thread( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend,
                                            void *relay );
__ASM_GLOBAL_FUNC( start_thread,
                   "subq $56,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 56\n\t")
                   "movq %rbp,48(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbp,48\n\t")
                   "movq %rbx,40(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,40\n\t")
                   "movq %r12,32(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r12,32\n\t")
                   "movq %r13,24(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r13,24\n\t")
                   "movq %r14,16(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r14,16\n\t")
                   "movq %r15,8(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r15,8\n\t")
                   /* store exit frame */
                   "movq %gs:0x30,%rax\n\t"
                   "movq %rsp,0x330(%rax)\n\t"      /* amd64_thread_data()->exit_frame */
                   /* switch to thread stack */
                   "movq 8(%rax),%rax\n\t"          /* NtCurrentTeb()->Tib.StackBase */
                   "leaq -0x1000(%rax),%rsp\n\t"
                   /* attach dlls */
                   "call " __ASM_NAME("attach_thread") "\n\t"
                   "movq %rax,%rsp\n\t"
                   /* clear the stack */
                   "andq $~0xfff,%rax\n\t"  /* round down to page size */
                   "movq %rax,%rdi\n\t"
                   "call " __ASM_NAME("virtual_clear_thread_stack") "\n\t"
                   /* switch to the initial context */
                   "movq %rsp,%rdi\n\t"
                   "call " __ASM_NAME("set_cpu_context") )

extern void DECLSPEC_NORETURN call_thread_exit_func( int status, void (*func)(int) );
__ASM_GLOBAL_FUNC( call_thread_exit_func,
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


/***********************************************************************
 *           init_thread_context
 */
static void init_thread_context( CONTEXT *context, LPTHREAD_START_ROUTINE entry, void *arg, void *relay )
{
    __asm__( "movw %%cs,%0" : "=m" (context->SegCs) );
    __asm__( "movw %%ss,%0" : "=m" (context->SegSs) );
    __asm__( "fxsave %0" : "=m" (context->u.FltSave) );
    context->Rcx    = (ULONG_PTR)entry;
    context->Rdx    = (ULONG_PTR)arg;
    context->Rsp    = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase - 0x28;
    context->Rip    = (ULONG_PTR)relay;
    context->EFlags = 0x200;
}


/***********************************************************************
 *           attach_thread
 */
PCONTEXT DECLSPEC_HIDDEN attach_thread( LPTHREAD_START_ROUTINE entry, void *arg,
                                        BOOL suspend, void *relay )
{
    CONTEXT *ctx;

    if (suspend)
    {
        CONTEXT context = { 0 };

        context.ContextFlags = CONTEXT_ALL;
        init_thread_context( &context, entry, arg, relay );
        wait_suspend( &context );
        ctx = (CONTEXT *)((ULONG_PTR)context.Rsp & ~15) - 1;
        *ctx = context;
    }
    else
    {
        ctx = (CONTEXT *)((char *)NtCurrentTeb()->Tib.StackBase - 0x30) - 1;
        init_thread_context( ctx, entry, arg, relay );
    }
    ctx->ContextFlags = CONTEXT_FULL;
    LdrInitializeThunk( ctx, (void **)&ctx->Rcx, 0, 0 );
    return ctx;
}


/***********************************************************************
 *           signal_start_thread
 *
 * Thread startup sequence:
 * signal_start_thread()
 *   -> start_thread()
 *     -> call_thread_func()
 */
void signal_start_thread( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend )
{
    start_thread( entry, arg, suspend, call_thread_func );
}


/**********************************************************************
 *		signal_start_process
 *
 * Process startup sequence:
 * signal_start_process()
 *   -> start_thread()
 *     -> kernel32_start_process()
 */
void signal_start_process( LPTHREAD_START_ROUTINE entry, BOOL suspend )
{
    start_thread( entry, NtCurrentTeb()->Peb, suspend, kernel32_start_process );
}


/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status )
{
    call_thread_exit_func( status, exit_thread );
}

/***********************************************************************
 *           signal_exit_process
 */
void signal_exit_process( int status )
{
    call_thread_exit_func( status, exit );
}

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "int $3; ret")

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgUserBreakPoint, 0, "int $3; ret")

#endif  /* __x86_64__ */
