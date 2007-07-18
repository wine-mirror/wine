/*
 * A Win32 based proxy implementing the GBD remote protocol
 * This allows to debug Wine (and any "emulated" program) under
 * Linux using GDB
 *
 * Copyright (c) Eric Pouech 2002-2004
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

/* Protocol specification can be found here:
 * http://sources.redhat.com/gdb/onlinedocs/gdb_32.html
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* if we don't have poll support on this system
 * we won't provide gdb proxy support here...
 */
#ifdef HAVE_POLL

#include "debugger.h"

#include "windef.h"
#include "winbase.h"
#include "tlhelp32.h"

#define GDBPXY_TRC_LOWLEVEL             0x01
#define GDBPXY_TRC_PACKET               0x02
#define GDBPXY_TRC_COMMAND              0x04
#define GDBPXY_TRC_COMMAND_ERROR        0x08
#define GDBPXY_TRC_WIN32_EVENT          0x10
#define GDBPXY_TRC_WIN32_ERROR          0x20
#define GDBPXY_TRC_COMMAND_FIXME        0x80

struct gdb_ctx_Xpoint
{
    enum be_xpoint_type         type;   /* -1 means free */
    void*                       addr;
    unsigned long               val;
};

struct gdb_context
{
    /* gdb information */
    int                         sock;
    /* incoming buffer */
    char*                       in_buf;
    int                         in_buf_alloc;
    int                         in_len;
    /* split into individual packet */
    char*                       in_packet;
    int                         in_packet_len;
    /* outgoing buffer */
    char*                       out_buf;
    int                         out_buf_alloc;
    int                         out_len;
    int                         out_curr_packet;
    /* generic GDB thread information */
    struct dbg_thread*          exec_thread;    /* thread used in step & continue */
    struct dbg_thread*          other_thread;   /* thread to be used in any other operation */
    unsigned                    trace;
    /* current Win32 trap env */
    unsigned                    last_sig;
    BOOL                        in_trap;
    CONTEXT                     context;
    /* Win32 information */
    struct dbg_process*         process;
#define NUM_XPOINT      32
    struct gdb_ctx_Xpoint       Xpoints[NUM_XPOINT];
    /* Unix environment */
    unsigned long               wine_segs[3];   /* load addresses of the ELF wine exec segments (text, bss and data) */
};

static struct be_process_io be_process_gdbproxy_io;

/* =============================================== *
 *       B A S I C   M A N I P U L A T I O N S     *
 * =============================================== *
 */

static inline int hex_from0(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;

    assert(0);
    return 0;
}

static inline unsigned char hex_to0(int x)
{
    assert(x >= 0 && x < 16);
    return "0123456789abcdef"[x];
}

static int hex_to_int(const char* src, size_t len)
{
    unsigned int returnval = 0;
    while (len--)
    {
        returnval <<= 4;
        returnval |= hex_from0(*src++);
    }
    return returnval;
}

static void hex_from(void* dst, const char* src, size_t len)
{
    unsigned char *p = dst;
    while (len--)
    {
        *p++ = (hex_from0(src[0]) << 4) | hex_from0(src[1]);
        src += 2;
    }
}

static void hex_to(char* dst, const void* src, size_t len)
{
    const unsigned char *p = src;
    while (len--)
    {
        *dst++ = hex_to0(*p >> 4);
        *dst++ = hex_to0(*p & 0x0F);
        p++;
    }
}

static unsigned char checksum(const char* ptr, int len)
{
    unsigned cksum = 0;

    while (len-- > 0)
        cksum += (unsigned char)*ptr++;
    return cksum;
}

/* =============================================== *
 *              C P U   H A N D L E R S            *
 * =============================================== *
 */

#ifdef __i386__
static size_t cpu_register_map[] = {
    FIELD_OFFSET(CONTEXT, Eax),
    FIELD_OFFSET(CONTEXT, Ecx),
    FIELD_OFFSET(CONTEXT, Edx),
    FIELD_OFFSET(CONTEXT, Ebx),
    FIELD_OFFSET(CONTEXT, Esp),
    FIELD_OFFSET(CONTEXT, Ebp),
    FIELD_OFFSET(CONTEXT, Esi),
    FIELD_OFFSET(CONTEXT, Edi),
    FIELD_OFFSET(CONTEXT, Eip),
    FIELD_OFFSET(CONTEXT, EFlags),
    FIELD_OFFSET(CONTEXT, SegCs),
    FIELD_OFFSET(CONTEXT, SegSs),
    FIELD_OFFSET(CONTEXT, SegDs),
    FIELD_OFFSET(CONTEXT, SegEs),
    FIELD_OFFSET(CONTEXT, SegFs),
    FIELD_OFFSET(CONTEXT, SegGs),
};
#elif defined(__powerpc__)
static size_t cpu_register_map[] = {
    FIELD_OFFSET(CONTEXT, Gpr0),
    FIELD_OFFSET(CONTEXT, Gpr1),
    FIELD_OFFSET(CONTEXT, Gpr2),
    FIELD_OFFSET(CONTEXT, Gpr3),
    FIELD_OFFSET(CONTEXT, Gpr4),
    FIELD_OFFSET(CONTEXT, Gpr5),
    FIELD_OFFSET(CONTEXT, Gpr6),
    FIELD_OFFSET(CONTEXT, Gpr7),
    FIELD_OFFSET(CONTEXT, Gpr8),
    FIELD_OFFSET(CONTEXT, Gpr9),
    FIELD_OFFSET(CONTEXT, Gpr10),
    FIELD_OFFSET(CONTEXT, Gpr11),
    FIELD_OFFSET(CONTEXT, Gpr12),
    FIELD_OFFSET(CONTEXT, Gpr13),
    FIELD_OFFSET(CONTEXT, Gpr14),
    FIELD_OFFSET(CONTEXT, Gpr15),
    FIELD_OFFSET(CONTEXT, Gpr16),
    FIELD_OFFSET(CONTEXT, Gpr17),
    FIELD_OFFSET(CONTEXT, Gpr18),
    FIELD_OFFSET(CONTEXT, Gpr19),
    FIELD_OFFSET(CONTEXT, Gpr20),
    FIELD_OFFSET(CONTEXT, Gpr21),
    FIELD_OFFSET(CONTEXT, Gpr22),
    FIELD_OFFSET(CONTEXT, Gpr23),
    FIELD_OFFSET(CONTEXT, Gpr24),
    FIELD_OFFSET(CONTEXT, Gpr25),
    FIELD_OFFSET(CONTEXT, Gpr26),
    FIELD_OFFSET(CONTEXT, Gpr27),
    FIELD_OFFSET(CONTEXT, Gpr28),
    FIELD_OFFSET(CONTEXT, Gpr29),
    FIELD_OFFSET(CONTEXT, Gpr30),
    FIELD_OFFSET(CONTEXT, Gpr31),
    FIELD_OFFSET(CONTEXT, Fpr0),
    FIELD_OFFSET(CONTEXT, Fpr1),
    FIELD_OFFSET(CONTEXT, Fpr2),
    FIELD_OFFSET(CONTEXT, Fpr3),
    FIELD_OFFSET(CONTEXT, Fpr4),
    FIELD_OFFSET(CONTEXT, Fpr5),
    FIELD_OFFSET(CONTEXT, Fpr6),
    FIELD_OFFSET(CONTEXT, Fpr7),
    FIELD_OFFSET(CONTEXT, Fpr8),
    FIELD_OFFSET(CONTEXT, Fpr9),
    FIELD_OFFSET(CONTEXT, Fpr10),
    FIELD_OFFSET(CONTEXT, Fpr11),
    FIELD_OFFSET(CONTEXT, Fpr12),
    FIELD_OFFSET(CONTEXT, Fpr13),
    FIELD_OFFSET(CONTEXT, Fpr14),
    FIELD_OFFSET(CONTEXT, Fpr15),
    FIELD_OFFSET(CONTEXT, Fpr16),
    FIELD_OFFSET(CONTEXT, Fpr17),
    FIELD_OFFSET(CONTEXT, Fpr18),
    FIELD_OFFSET(CONTEXT, Fpr19),
    FIELD_OFFSET(CONTEXT, Fpr20),
    FIELD_OFFSET(CONTEXT, Fpr21),
    FIELD_OFFSET(CONTEXT, Fpr22),
    FIELD_OFFSET(CONTEXT, Fpr23),
    FIELD_OFFSET(CONTEXT, Fpr24),
    FIELD_OFFSET(CONTEXT, Fpr25),
    FIELD_OFFSET(CONTEXT, Fpr26),
    FIELD_OFFSET(CONTEXT, Fpr27),
    FIELD_OFFSET(CONTEXT, Fpr28),
    FIELD_OFFSET(CONTEXT, Fpr29),
    FIELD_OFFSET(CONTEXT, Fpr30),
    FIELD_OFFSET(CONTEXT, Fpr31),

    FIELD_OFFSET(CONTEXT, Iar),
    FIELD_OFFSET(CONTEXT, Msr),
    FIELD_OFFSET(CONTEXT, Cr),
    FIELD_OFFSET(CONTEXT, Lr),
    FIELD_OFFSET(CONTEXT, Ctr),
    FIELD_OFFSET(CONTEXT, Xer),
    /* FIXME: MQ is missing? FIELD_OFFSET(CONTEXT, Mq), */
    /* see gdb/nlm/ppc.c */
};
#elif defined(__ALPHA__)
static size_t cpu_register_map[] = {
    FIELD_OFFSET(CONTEXT, IntV0),
    FIELD_OFFSET(CONTEXT, IntT0),
    FIELD_OFFSET(CONTEXT, IntT1),
    FIELD_OFFSET(CONTEXT, IntT2),
    FIELD_OFFSET(CONTEXT, IntT3),
    FIELD_OFFSET(CONTEXT, IntT4),
    FIELD_OFFSET(CONTEXT, IntT5),
    FIELD_OFFSET(CONTEXT, IntT6),
    FIELD_OFFSET(CONTEXT, IntT7),
    FIELD_OFFSET(CONTEXT, IntS0),
    FIELD_OFFSET(CONTEXT, IntS1),
    FIELD_OFFSET(CONTEXT, IntS2),
    FIELD_OFFSET(CONTEXT, IntS3),
    FIELD_OFFSET(CONTEXT, IntS4),
    FIELD_OFFSET(CONTEXT, IntS5),
    FIELD_OFFSET(CONTEXT, IntFp),
    FIELD_OFFSET(CONTEXT, IntA0),
    FIELD_OFFSET(CONTEXT, IntA1),
    FIELD_OFFSET(CONTEXT, IntA2),
    FIELD_OFFSET(CONTEXT, IntA3),
    FIELD_OFFSET(CONTEXT, IntA4),
    FIELD_OFFSET(CONTEXT, IntA5),
    FIELD_OFFSET(CONTEXT, IntT8),
    FIELD_OFFSET(CONTEXT, IntT9),
    FIELD_OFFSET(CONTEXT, IntT10),
    FIELD_OFFSET(CONTEXT, IntT11),
    FIELD_OFFSET(CONTEXT, IntRa),
    FIELD_OFFSET(CONTEXT, IntT12),
    FIELD_OFFSET(CONTEXT, IntAt),
    FIELD_OFFSET(CONTEXT, IntGp),
    FIELD_OFFSET(CONTEXT, IntSp),
    FIELD_OFFSET(CONTEXT, IntZero),
    FIELD_OFFSET(CONTEXT, FltF0),
    FIELD_OFFSET(CONTEXT, FltF1),
    FIELD_OFFSET(CONTEXT, FltF2),
    FIELD_OFFSET(CONTEXT, FltF3),
    FIELD_OFFSET(CONTEXT, FltF4),
    FIELD_OFFSET(CONTEXT, FltF5),
    FIELD_OFFSET(CONTEXT, FltF6),
    FIELD_OFFSET(CONTEXT, FltF7),
    FIELD_OFFSET(CONTEXT, FltF8),
    FIELD_OFFSET(CONTEXT, FltF9),
    FIELD_OFFSET(CONTEXT, FltF10),
    FIELD_OFFSET(CONTEXT, FltF11),
    FIELD_OFFSET(CONTEXT, FltF12),
    FIELD_OFFSET(CONTEXT, FltF13),
    FIELD_OFFSET(CONTEXT, FltF14),
    FIELD_OFFSET(CONTEXT, FltF15),
    FIELD_OFFSET(CONTEXT, FltF16),
    FIELD_OFFSET(CONTEXT, FltF17),
    FIELD_OFFSET(CONTEXT, FltF18),
    FIELD_OFFSET(CONTEXT, FltF19),
    FIELD_OFFSET(CONTEXT, FltF20),
    FIELD_OFFSET(CONTEXT, FltF21),
    FIELD_OFFSET(CONTEXT, FltF22),
    FIELD_OFFSET(CONTEXT, FltF23),
    FIELD_OFFSET(CONTEXT, FltF24),
    FIELD_OFFSET(CONTEXT, FltF25),
    FIELD_OFFSET(CONTEXT, FltF26),
    FIELD_OFFSET(CONTEXT, FltF27),
    FIELD_OFFSET(CONTEXT, FltF28),
    FIELD_OFFSET(CONTEXT, FltF29),
    FIELD_OFFSET(CONTEXT, FltF30),
    FIELD_OFFSET(CONTEXT, FltF31),

    /* FIXME: Didn't look for the right order yet */
    FIELD_OFFSET(CONTEXT, Fir),
    FIELD_OFFSET(CONTEXT, Fpcr),
    FIELD_OFFSET(CONTEXT, SoftFpcr),
};
#elif defined(__x86_64__)
static size_t cpu_register_map[] = {
    FIELD_OFFSET(CONTEXT, Rax),
    FIELD_OFFSET(CONTEXT, Rbx),
    FIELD_OFFSET(CONTEXT, Rcx),
    FIELD_OFFSET(CONTEXT, Rdx),
    FIELD_OFFSET(CONTEXT, Rsi),
    FIELD_OFFSET(CONTEXT, Rdi),
    FIELD_OFFSET(CONTEXT, Rbp),
    FIELD_OFFSET(CONTEXT, Rsp),
    FIELD_OFFSET(CONTEXT, R8),
    FIELD_OFFSET(CONTEXT, R9),
    FIELD_OFFSET(CONTEXT, R10),
    FIELD_OFFSET(CONTEXT, R11),
    FIELD_OFFSET(CONTEXT, R12),
    FIELD_OFFSET(CONTEXT, R13),
    FIELD_OFFSET(CONTEXT, R14),
    FIELD_OFFSET(CONTEXT, R15),
    FIELD_OFFSET(CONTEXT, Rip),
    FIELD_OFFSET(CONTEXT, EFlags),
    FIELD_OFFSET(CONTEXT, SegCs),
    FIELD_OFFSET(CONTEXT, SegSs),
    FIELD_OFFSET(CONTEXT, SegDs),
    FIELD_OFFSET(CONTEXT, SegEs),
    FIELD_OFFSET(CONTEXT, SegFs),
    FIELD_OFFSET(CONTEXT, SegGs),
};
#else
# error Define the registers map for your CPU
#endif

static const size_t cpu_num_regs = (sizeof(cpu_register_map) / sizeof(cpu_register_map[0]));

static inline unsigned long* cpu_register(CONTEXT* ctx, unsigned idx)
{
    assert(idx < cpu_num_regs);
    return (unsigned long*)((char*)ctx + cpu_register_map[idx]);
}

/* =============================================== *
 *    W I N 3 2   D E B U G   I N T E R F A C E    *
 * =============================================== *
 */

static BOOL fetch_context(struct gdb_context* gdbctx, HANDLE h, CONTEXT* ctx)
{
    ctx->ContextFlags =  CONTEXT_CONTROL
                       | CONTEXT_INTEGER
#ifdef CONTEXT_SEGMENTS
	               | CONTEXT_SEGMENTS
#endif
#ifdef CONTEXT_DEBUG_REGISTERS
	               | CONTEXT_DEBUG_REGISTERS
#endif
		       ;
    if (!GetThreadContext(h, ctx))
    {
        if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
            fprintf(stderr, "Can't get thread's context\n");
        return FALSE;
    }
    return TRUE;
}

static BOOL handle_exception(struct gdb_context* gdbctx, EXCEPTION_DEBUG_INFO* exc)
{
    EXCEPTION_RECORD*   rec = &exc->ExceptionRecord;
    BOOL                ret = FALSE;

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_GUARD_PAGE:
        gdbctx->last_sig = SIGSEGV;
        ret = TRUE;
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        gdbctx->last_sig = SIGBUS;
        ret = TRUE;
        break;
    case EXCEPTION_SINGLE_STEP:
        /* fall thru */
    case EXCEPTION_BREAKPOINT:
        gdbctx->last_sig = SIGTRAP;
        ret = TRUE;
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        gdbctx->last_sig = SIGFPE;
        ret = TRUE;
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
        gdbctx->last_sig = SIGFPE;
        ret = TRUE;
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        gdbctx->last_sig = SIGILL;
        ret = TRUE;
        break;
    case CONTROL_C_EXIT:
        gdbctx->last_sig = SIGINT;
        ret = TRUE;
        break;
    case STATUS_POSSIBLE_DEADLOCK:
        gdbctx->last_sig = SIGALRM;
        ret = TRUE;
        /* FIXME: we could also add here a O packet with additional information */
        break;
    default:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "Unhandled exception code 0x%08x\n", rec->ExceptionCode);
        gdbctx->last_sig = SIGABRT;
        ret = TRUE;
        break;
    }
    return ret;
}

static	void	handle_debug_event(struct gdb_context* gdbctx, DEBUG_EVENT* de)
{
    char                buffer[256];

    dbg_curr_thread = dbg_get_thread(gdbctx->process, de->dwThreadId);

    switch (de->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        gdbctx->process = dbg_add_process(&be_process_gdbproxy_io, de->dwProcessId,
                                          de->u.CreateProcessInfo.hProcess);
        if (!gdbctx->process) break;
        memory_get_string_indirect(gdbctx->process,
                                   de->u.CreateProcessInfo.lpImageName,
                                   de->u.CreateProcessInfo.fUnicode,
                                   buffer, sizeof(buffer));
        dbg_set_process_name(gdbctx->process, buffer);

        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%04x:%04x: create process '%s'/%p @%p (%u<%u>)\n",
                    de->dwProcessId, de->dwThreadId,
                    buffer, de->u.CreateProcessInfo.lpImageName,
                    de->u.CreateProcessInfo.lpStartAddress,
                    de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                    de->u.CreateProcessInfo.nDebugInfoSize);

        /* de->u.CreateProcessInfo.lpStartAddress; */
        if (!SymInitialize(gdbctx->process->handle, NULL, TRUE))
            fprintf(stderr, "Couldn't initiate DbgHelp\n");

        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%04x:%04x: create thread I @%p\n",
                    de->dwProcessId, de->dwThreadId,
                    de->u.CreateProcessInfo.lpStartAddress);

        assert(dbg_curr_thread == NULL); /* shouldn't be there */
        dbg_add_thread(gdbctx->process, de->dwThreadId,
                       de->u.CreateProcessInfo.hThread,
                       de->u.CreateProcessInfo.lpThreadLocalBase);
        break;

    case LOAD_DLL_DEBUG_EVENT:
        assert(dbg_curr_thread);
        memory_get_string_indirect(gdbctx->process,
                                   de->u.LoadDll.lpImageName,
                                   de->u.LoadDll.fUnicode,
                                   buffer, sizeof(buffer));
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%04x:%04x: loads DLL %s @%p (%u<%u>)\n",
                    de->dwProcessId, de->dwThreadId,
                    buffer, de->u.LoadDll.lpBaseOfDll,
                    de->u.LoadDll.dwDebugInfoFileOffset,
                    de->u.LoadDll.nDebugInfoSize);
        SymLoadModule(gdbctx->process->handle, de->u.LoadDll.hFile, buffer, NULL,
                      (unsigned long)de->u.LoadDll.lpBaseOfDll, 0);
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: unload DLL @%p\n",
                    de->dwProcessId, de->dwThreadId, de->u.UnloadDll.lpBaseOfDll);
        SymUnloadModule(gdbctx->process->handle, 
                        (unsigned long)de->u.UnloadDll.lpBaseOfDll);
        break;

    case EXCEPTION_DEBUG_EVENT:
        assert(dbg_curr_thread);
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: exception code=0x%08x\n",
                    de->dwProcessId, de->dwThreadId,
                    de->u.Exception.ExceptionRecord.ExceptionCode);

        if (fetch_context(gdbctx, dbg_curr_thread->handle, &gdbctx->context))
        {
            gdbctx->in_trap = handle_exception(gdbctx, &de->u.Exception);
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: create thread D @%p\n",
                    de->dwProcessId, de->dwThreadId, de->u.CreateThread.lpStartAddress);

        dbg_add_thread(gdbctx->process,
                       de->dwThreadId,
                       de->u.CreateThread.hThread,
                       de->u.CreateThread.lpThreadLocalBase);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: exit thread (%u)\n",
                    de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);

        assert(dbg_curr_thread);
        if (dbg_curr_thread == gdbctx->exec_thread) gdbctx->exec_thread = NULL;
        if (dbg_curr_thread == gdbctx->other_thread) gdbctx->other_thread = NULL;
        dbg_del_thread(dbg_curr_thread);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: exit process (%u)\n",
                    de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);

        dbg_del_process(gdbctx->process);
        gdbctx->process = NULL;
        /* now signal gdb that we're done */
        gdbctx->last_sig = SIGTERM;
        gdbctx->in_trap = TRUE;
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        assert(dbg_curr_thread);
        memory_get_string(gdbctx->process,
                          de->u.DebugString.lpDebugStringData, TRUE,
                          de->u.DebugString.fUnicode, buffer, sizeof(buffer));
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: output debug string (%s)\n",
                    de->dwProcessId, de->dwThreadId, buffer);
        break;

    case RIP_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: rip error=%u type=%u\n",
                    de->dwProcessId, de->dwThreadId, de->u.RipInfo.dwError,
                    de->u.RipInfo.dwType);
        break;

    default:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: unknown event (%u)\n",
                    de->dwProcessId, de->dwThreadId, de->dwDebugEventCode);
    }
}

static void resume_debuggee(struct gdb_context* gdbctx, DWORD cont)
{
    if (dbg_curr_thread)
    {
        if (!SetThreadContext(dbg_curr_thread->handle, &gdbctx->context))
            if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                fprintf(stderr, "Cannot set context on thread %04x\n", dbg_curr_thread->tid);
        if (!ContinueDebugEvent(gdbctx->process->pid, dbg_curr_thread->tid, cont))
            if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                fprintf(stderr, "Cannot continue on %04x (%x)\n",
                        dbg_curr_thread->tid, cont);
    }
    else if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
        fprintf(stderr, "Cannot find last thread\n");
}


static void resume_debuggee_thread(struct gdb_context* gdbctx, DWORD cont, unsigned int threadid)
{

    if (dbg_curr_thread)
    {
        if(dbg_curr_thread->tid  == threadid){
            /* Windows debug and GDB don't seem to work well here, windows only likes ContinueDebugEvent being used on the reporter of the event */
            if (!SetThreadContext(dbg_curr_thread->handle, &gdbctx->context))
                if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                    fprintf(stderr, "Cannot set context on thread %04x\n", dbg_curr_thread->tid);
            if (!ContinueDebugEvent(gdbctx->process->pid, dbg_curr_thread->tid, cont))
                if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                    fprintf(stderr, "Cannot continue on %04x (%x)\n",
                            dbg_curr_thread->tid, cont);
        }
    }
    else if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
        fprintf(stderr, "Cannot find last thread\n");
}

static BOOL	check_for_interrupt(struct gdb_context* gdbctx)
{
	struct pollfd       pollfd;
	int ret;
	char pkt;
				
	pollfd.fd = gdbctx->sock;
	pollfd.events = POLLIN;
	pollfd.revents = 0;
				
	if ((ret = poll(&pollfd, 1, 0)) == 1) {
		ret = read(gdbctx->sock, &pkt, 1);
		if (ret != 1) {
			if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR) {
				fprintf(stderr, "read failed\n");
			}
			return FALSE;
		}
		if (pkt != '\003') {
			if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR) {
				fprintf(stderr, "Unexpected break packet (%c/0x%X)\n", pkt, pkt);				
			}
			return FALSE;
		}
		return TRUE;
	} else if (ret == -1) {
		fprintf(stderr, "poll failed\n");
	}
	return FALSE;
}

static void    wait_for_debuggee(struct gdb_context* gdbctx)
{
    DEBUG_EVENT         de;

    gdbctx->in_trap = FALSE;
    for (;;)
    {
		if (!WaitForDebugEvent(&de, 10))
		{
			if (GetLastError() == ERROR_SEM_TIMEOUT)
			{
				if (check_for_interrupt(gdbctx)) {
					if (!DebugBreakProcess(gdbctx->process->handle)) {
						if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR) {
							fprintf(stderr, "Failed to break into debugee\n");
						}
						break;
					}
					WaitForDebugEvent(&de, INFINITE);	
				} else {
					continue;
				} 
			} else {
				break;
			} 
		}
        handle_debug_event(gdbctx, &de);
        assert(!gdbctx->process ||
               gdbctx->process->pid == 0 ||
               de.dwProcessId == gdbctx->process->pid);
        assert(!dbg_curr_thread || de.dwThreadId == dbg_curr_thread->tid);
        if (gdbctx->in_trap) break;
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    }
}

static void detach_debuggee(struct gdb_context* gdbctx, BOOL kill)
{
    be_cpu->single_step(&gdbctx->context, FALSE);
    resume_debuggee(gdbctx, DBG_CONTINUE);
    if (!kill)
        DebugActiveProcessStop(gdbctx->process->pid);
    dbg_del_process(gdbctx->process);
    gdbctx->process = NULL;
}

static void get_process_info(struct gdb_context* gdbctx, char* buffer, size_t len)
{
    DWORD status;

    if (!GetExitCodeProcess(gdbctx->process->handle, &status))
    {
        strcpy(buffer, "Unknown process");
        return;
    }
    if (status == STILL_ACTIVE)
    {
        strcpy(buffer, "Running");
    }
    else
        snprintf(buffer, len, "Terminated (%u)", status);

    switch (GetPriorityClass(gdbctx->process->handle))
    {
    case 0: break;
#ifdef ABOVE_NORMAL_PRIORITY_CLASS
    case ABOVE_NORMAL_PRIORITY_CLASS:   strcat(buffer, ", above normal priority");      break;
#endif
#ifdef BELOW_NORMAL_PRIORITY_CLASS
    case BELOW_NORMAL_PRIORITY_CLASS:   strcat(buffer, ", below normal priotity");      break;
#endif
    case HIGH_PRIORITY_CLASS:           strcat(buffer, ", high priority");              break;
    case IDLE_PRIORITY_CLASS:           strcat(buffer, ", idle priority");              break;
    case NORMAL_PRIORITY_CLASS:         strcat(buffer, ", normal priority");            break;
    case REALTIME_PRIORITY_CLASS:       strcat(buffer, ", realtime priority");          break;
    }
    strcat(buffer, "\n");
}

static void get_thread_info(struct gdb_context* gdbctx, unsigned tid,
                            char* buffer, size_t len)
{
    struct dbg_thread*  thd;
    DWORD               status;
    int                 prio;

    /* FIXME: use the size of buffer */
    thd = dbg_get_thread(gdbctx->process, tid);
    if (thd == NULL)
    {
        strcpy(buffer, "No information");
        return;
    }
    if (GetExitCodeThread(thd->handle, &status))
    {
        if (status == STILL_ACTIVE)
        {
            /* FIXME: this is a bit brutal... some nicer way shall be found */
            switch (status = SuspendThread(thd->handle))
            {
            case -1: break;
            case 0:  strcpy(buffer, "Running"); break;
            default: snprintf(buffer, len, "Suspended (%u)", status - 1);
            }
            ResumeThread(thd->handle);
        }
        else
            snprintf(buffer, len, "Terminated (exit code = %u)", status);
    }
    else
    {
        strcpy(buffer, "Unknown threadID");
    }
    switch (prio = GetThreadPriority(thd->handle))
    {
    case THREAD_PRIORITY_ERROR_RETURN:  break;
    case THREAD_PRIORITY_ABOVE_NORMAL:  strcat(buffer, ", priority +1 above normal"); break;
    case THREAD_PRIORITY_BELOW_NORMAL:  strcat(buffer, ", priority -1 below normal"); break;
    case THREAD_PRIORITY_HIGHEST:       strcat(buffer, ", priority +2 above normal"); break;
    case THREAD_PRIORITY_LOWEST:        strcat(buffer, ", priority -2 below normal"); break;
    case THREAD_PRIORITY_IDLE:          strcat(buffer, ", priority idle"); break;
    case THREAD_PRIORITY_NORMAL:        strcat(buffer, ", priority normal"); break;
    case THREAD_PRIORITY_TIME_CRITICAL: strcat(buffer, ", priority time-critical"); break;
    default: snprintf(buffer + strlen(buffer), len - strlen(buffer), ", priority = %d", prio);
    }
    assert(strlen(buffer) < len);
}

/* =============================================== *
 *          P A C K E T        U T I L S           *
 * =============================================== *
 */

enum packet_return {packet_error = 0x00, packet_ok = 0x01, packet_done = 0x02,
                    packet_last_f = 0x80};

static void packet_reply_grow(struct gdb_context* gdbctx, size_t size)
{
    if (gdbctx->out_buf_alloc < gdbctx->out_len + size)
    {
        gdbctx->out_buf_alloc = ((gdbctx->out_len + size) / 32 + 1) * 32;
        gdbctx->out_buf = realloc(gdbctx->out_buf, gdbctx->out_buf_alloc);
    }
}

static void packet_reply_hex_to(struct gdb_context* gdbctx, const void* src, int len)
{
    packet_reply_grow(gdbctx, len * 2);
    hex_to(&gdbctx->out_buf[gdbctx->out_len], src, len);
    gdbctx->out_len += len * 2;
}

static inline void packet_reply_hex_to_str(struct gdb_context* gdbctx, const char* src)
{
    packet_reply_hex_to(gdbctx, src, strlen(src));
}

static void packet_reply_val(struct gdb_context* gdbctx, unsigned long val, int len)
{
    int i, shift;

    shift = (len - 1) * 8;
    packet_reply_grow(gdbctx, len * 2);
    for (i = 0; i < len; i++, shift -= 8)
    {
        gdbctx->out_buf[gdbctx->out_len++] = hex_to0((val >> (shift + 4)) & 0x0F);
        gdbctx->out_buf[gdbctx->out_len++] = hex_to0((val >>  shift     ) & 0x0F);
    }
}

static inline void packet_reply_add(struct gdb_context* gdbctx, const char* str, int len)
{
    packet_reply_grow(gdbctx, len);
    memcpy(&gdbctx->out_buf[gdbctx->out_len], str, len);
    gdbctx->out_len += len;
}

static inline void packet_reply_cat(struct gdb_context* gdbctx, const char* str)
{
    packet_reply_add(gdbctx, str, strlen(str));
}

static inline void packet_reply_catc(struct gdb_context* gdbctx, char ch)
{
    packet_reply_add(gdbctx, &ch, 1);
}

static void packet_reply_open(struct gdb_context* gdbctx)
{
    assert(gdbctx->out_curr_packet == -1);
    packet_reply_catc(gdbctx, '$');
    gdbctx->out_curr_packet = gdbctx->out_len;
}

static void packet_reply_close(struct gdb_context* gdbctx)
{
    unsigned char       cksum;
    int plen;

    plen = gdbctx->out_len - gdbctx->out_curr_packet;
    packet_reply_catc(gdbctx, '#');
    cksum = checksum(&gdbctx->out_buf[gdbctx->out_curr_packet], plen);
    packet_reply_hex_to(gdbctx, &cksum, 1);
    if (gdbctx->trace & GDBPXY_TRC_PACKET)
        fprintf(stderr, "Reply : %*.*s\n",
                plen, plen, &gdbctx->out_buf[gdbctx->out_curr_packet]);
    gdbctx->out_curr_packet = -1;
}

static enum packet_return packet_reply(struct gdb_context* gdbctx, const char* packet, int len)
{
    packet_reply_open(gdbctx);

    if (len == -1) len = strlen(packet);
    assert(memchr(packet, '$', len) == NULL && memchr(packet, '#', len) == NULL);

    packet_reply_add(gdbctx, packet, len);

    packet_reply_close(gdbctx);

    return packet_done;
}

static enum packet_return packet_reply_error(struct gdb_context* gdbctx, int error)
{
    packet_reply_open(gdbctx);

    packet_reply_add(gdbctx, "E", 1);
    packet_reply_val(gdbctx, error, 1);

    packet_reply_close(gdbctx);

    return packet_done;
}

/* =============================================== *
 *          P A C K E T   H A N D L E R S          *
 * =============================================== *
 */

static enum packet_return packet_reply_status(struct gdb_context* gdbctx)
{
    enum packet_return ret = packet_done;

    packet_reply_open(gdbctx);

    if (gdbctx->process != NULL)
    {
        unsigned char           sig;
        unsigned                i;

        packet_reply_catc(gdbctx, 'T');
        sig = gdbctx->last_sig;
        packet_reply_val(gdbctx, sig, 1);
        packet_reply_add(gdbctx, "thread:", 7);
        packet_reply_val(gdbctx, dbg_curr_thread->tid, 4);
        packet_reply_catc(gdbctx, ';');

        for (i = 0; i < cpu_num_regs; i++)
        {
            /* FIXME: this call will also grow the buffer...
             * unneeded, but not harmful
             */
            packet_reply_val(gdbctx, i, 1);
            packet_reply_catc(gdbctx, ':');
            packet_reply_hex_to(gdbctx, cpu_register(&gdbctx->context, i), 4);
            packet_reply_catc(gdbctx, ';');
        }
    }
    else
    {
        /* Try to put an exit code
         * Cannot use GetExitCodeProcess, wouldn't fit in a 8 bit value, so
         * just indicate the end of process and exit */
        packet_reply_add(gdbctx, "W00", 3);
        /*if (!gdbctx->extended)*/ ret |= packet_last_f;
    }

    packet_reply_close(gdbctx);

    return ret;
}

#if 0
static enum packet_return packet_extended(struct gdb_context* gdbctx)
{
    gdbctx->extended = 1;
    return packet_ok;
}
#endif

static enum packet_return packet_last_signal(struct gdb_context* gdbctx)
{
    assert(gdbctx->in_packet_len == 0);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_continue(struct gdb_context* gdbctx)
{
    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 0);
    if (dbg_curr_thread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: cont on %04x, while last thread is %04x\n",
                    gdbctx->exec_thread->tid, dbg_curr_thread->tid);
    resume_debuggee(gdbctx, DBG_CONTINUE);
    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_verbose(struct gdb_context* gdbctx)
{
    int i;
    int defaultAction = -1; /* magic non action */
    unsigned char sig;
    int actions =0;
    int actionIndex[20]; /* allow for up to 20 actions */
    int threadIndex[20];
    int threadCount = 0;
    unsigned int threadIDs[100]; /* TODO: Should make this dynamic */
    unsigned int threadID = 0;
    struct dbg_thread*  thd;

    /* basic check */
    assert(gdbctx->in_packet_len >= 4);

    /* OK we have vCont followed by..
    * ? for query
    * c for packet_continue
    * Csig for packet_continue_signal
    * s for step
    * Ssig  for step signal
    * and then an optional thread ID at the end..
    * *******************************************/

    fprintf(stderr, "trying to process a verbose packet\n");
    /* now check that we've got Cont */
    assert(strncmp(gdbctx->in_packet, "Cont", 4) == 0);

    /* Query */
    if (gdbctx->in_packet[4] == '?')
    {
        /*
          Reply:
          `vCont[;action]...'
          The vCont packet is supported. Each action is a supported command in the vCont packet.
          `'
          The vCont packet is not supported.  (this didn't seem to be obeyed!)
        */
        packet_reply_open(gdbctx);
        packet_reply_add(gdbctx, "vCont", 5);
        /* add all the supported actions to the reply (all of them for now) */
        packet_reply_add(gdbctx, ";c", 2);
        packet_reply_add(gdbctx, ";C", 2);
        packet_reply_add(gdbctx, ";s", 2);
        packet_reply_add(gdbctx, ";S", 2);
        packet_reply_close(gdbctx);
        return packet_done;
    }

    /* This may not be the 'fastest' code in the world. but it should be nice and easy to debug.
    (as it's run when people are debugging break points I'm sure they won't notice the extra 100 cycles anyway)
    now if only gdb talked XML.... */
#if 0 /* handy for debugging */
    fprintf(stderr, "no, but can we find a default packet %.*s %d\n", gdbctx->in_packet_len, gdbctx->in_packet,  gdbctx->in_packet_len);
#endif

    /* go through the packet and identify where all the actions start at */
    for (i = 4; i < gdbctx->in_packet_len - 1; i++)
    {
        if (gdbctx->in_packet[i] == ';')
        {
            threadIndex[actions] = 0;
            actionIndex[actions++] = i;
        }
        else if (gdbctx->in_packet[i] == ':')
        {
            threadIndex[actions - 1] = i;
        }
    }

    /* now look up the default action */
    for (i = 0 ; i < actions; i++)
    {
        if (threadIndex[i] == 0)
        {
            if (defaultAction != -1)
            {
                fprintf(stderr,"Too many default actions specified\n");
                return packet_error;
            }
            defaultAction = i;
        }
    }

    /* Now, I have this default action thing that needs to be applied to all non counted threads */

    /* go through all the threads and stick their ids in the to be done list. */
    for (thd = gdbctx->process->threads; thd; thd = thd->next)
    {
        threadIDs[threadCount++] = thd->tid;
        /* check to see if we have more threads than I counted on, and tell the user what to do
         * (they're running winedbg, so I'm sure they can fix the problem from the error message!) */
        if (threadCount == 100)
        {
            fprintf(stderr, "Wow, that's a lot of threads, change threadIDs in wine/programms/winedgb/gdbproxy.c to be higher\n");
            break;
        }
    }

    /* Ok, now we have... actionIndex full of actions and we know what threads there are, so all
     * that remains is to apply the actions to the threads and the default action to any threads
     * left */
    if (dbg_curr_thread != gdbctx->exec_thread && gdbctx->exec_thread)
    if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
        fprintf(stderr, "NIY: cont on %04x, while last thread is %04x\n",
                gdbctx->exec_thread->tid, dbg_curr_thread->tid);

    /* deal with the threaded stuff first */
    for (i = 0; i < actions ; i++)
    {
        if (threadIndex[i] != 0)
        {
            int j, idLength = 0;
            if (i < actions - 1)
            {
                idLength = (actionIndex[i+1] - threadIndex[i]) - 1;
            }
            else
            {
                idLength = (gdbctx->in_packet_len - threadIndex[i]) - 1;
            }

            threadID = hex_to_int(gdbctx->in_packet + threadIndex[i] + 1 , idLength);
            /* process the action */
            switch (gdbctx->in_packet[actionIndex[i] + 1])
            {
            case 's': /* step */
                be_cpu->single_step(&gdbctx->context, TRUE);
                /* fall through*/
            case 'c': /* continue */
                resume_debuggee_thread(gdbctx, DBG_CONTINUE, threadID);
                break;
            case 'S': /* step Sig, */
                be_cpu->single_step(&gdbctx->context, TRUE);
                /* fall through */
            case 'C': /* continue sig */
                hex_from(&sig, gdbctx->in_packet + actionIndex[i] + 2, 1);
                /* cannot change signals on the fly */
                if (gdbctx->trace & GDBPXY_TRC_COMMAND)
                    fprintf(stderr, "sigs: %u %u\n", sig, gdbctx->last_sig);
                if (sig != gdbctx->last_sig)
                    return packet_error;
                resume_debuggee_thread(gdbctx, DBG_EXCEPTION_NOT_HANDLED, threadID);
                break;
            }
            for (j = 0 ; j < threadCount; j++)
            {
                if (threadIDs[j] == threadID)
                {
                    threadIDs[j] = 0;
                    break;
                }
            }
        }
    } /* for i=0 ; i< actions */

    /* now we have manage the default action */
    if (defaultAction >= 0)
    {
        for (i = 0 ; i< threadCount; i++)
        {
            /* check to see if we've already done something to the thread*/
            if (threadIDs[i] != 0)
            {
                /* if not apply the default action*/
                threadID = threadIDs[i];
                /* process the action (yes this is almost identical to the one above!) */
                switch (gdbctx->in_packet[actionIndex[defaultAction] + 1])
                {
                case 's': /* step */
                    be_cpu->single_step(&gdbctx->context, TRUE);
                    /* fall through */
                case 'c': /* continue */
                    resume_debuggee_thread(gdbctx, DBG_CONTINUE, threadID);
                    break;
                case 'S':
                     be_cpu->single_step(&gdbctx->context, TRUE);
                     /* fall through */
                case 'C': /* continue sig */
                    hex_from(&sig, gdbctx->in_packet + actionIndex[defaultAction] + 2, 1);
                    /* cannot change signals on the fly */
                    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
                        fprintf(stderr, "sigs: %u %u\n", sig, gdbctx->last_sig);
                    if (sig != gdbctx->last_sig)
                        return packet_error;
                    resume_debuggee_thread(gdbctx, DBG_EXCEPTION_NOT_HANDLED, threadID);
                    break;
                }
            }
        }
    } /* if(defaultAction >=0) */

    wait_for_debuggee(gdbctx);
    be_cpu->single_step(&gdbctx->context, FALSE);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_continue_signal(struct gdb_context* gdbctx)
{
    unsigned char sig;

    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 2);
    if (dbg_curr_thread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: cont/sig on %04x, while last thread is %04x\n",
                    gdbctx->exec_thread->tid, dbg_curr_thread->tid);
    hex_from(&sig, gdbctx->in_packet, 1);
    /* cannot change signals on the fly */
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "sigs: %u %u\n", sig, gdbctx->last_sig);
    if (sig != gdbctx->last_sig)
        return packet_error;
    resume_debuggee(gdbctx, DBG_EXCEPTION_NOT_HANDLED);
    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_detach(struct gdb_context* gdbctx)
{
    detach_debuggee(gdbctx, FALSE);
    return packet_ok | packet_last_f;
}

static enum packet_return packet_read_registers(struct gdb_context* gdbctx)
{
    int                 i;
    CONTEXT             ctx;
    CONTEXT*            pctx = &gdbctx->context;

    assert(gdbctx->in_trap);

    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }

    packet_reply_open(gdbctx);
    for (i = 0; i < cpu_num_regs; i++)
    {
        packet_reply_hex_to(gdbctx, cpu_register(pctx, i), 4);
    }
    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_registers(struct gdb_context* gdbctx)
{
    unsigned    i;
    CONTEXT     ctx;
    CONTEXT*    pctx = &gdbctx->context;

    assert(gdbctx->in_trap);
    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }
    if (gdbctx->in_packet_len < cpu_num_regs * 2) return packet_error;

    for (i = 0; i < cpu_num_regs; i++)
        hex_from(cpu_register(pctx, i), &gdbctx->in_packet[8 * i], 4);
    if (pctx != &gdbctx->context && !SetThreadContext(gdbctx->other_thread->handle, pctx))
    {
        if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
            fprintf(stderr, "Cannot set context on thread %04x\n", gdbctx->other_thread->tid);
        return packet_error;
    }
    return packet_ok;
}

static enum packet_return packet_kill(struct gdb_context* gdbctx)
{
    detach_debuggee(gdbctx, TRUE);
#if 0
    if (!gdbctx->extended)
        /* dunno whether GDB cares or not */
#endif
    wait(NULL);
    exit(0);
    /* assume we can't really answer something here */
    /* return packet_done; */
}

static enum packet_return packet_thread(struct gdb_context* gdbctx)
{
    char* end;
    unsigned thread;

    switch (gdbctx->in_packet[0])
    {
    case 'c':
    case 'g':
        if (gdbctx->in_packet[1] == '-')
            thread = -strtol(gdbctx->in_packet + 2, &end, 16);
        else
            thread = strtol(gdbctx->in_packet + 1, &end, 16);
        if (end == NULL || end > gdbctx->in_packet + gdbctx->in_packet_len)
        {
            if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
                fprintf(stderr, "Cannot get threadid %*.*s\n",
                        gdbctx->in_packet_len - 1, gdbctx->in_packet_len - 1,
                        gdbctx->in_packet + 1);
            return packet_error;
        }
        if (gdbctx->in_packet[0] == 'c')
            gdbctx->exec_thread = dbg_get_thread(gdbctx->process, thread);
        else
            gdbctx->other_thread = dbg_get_thread(gdbctx->process, thread);
        return packet_ok;
    default:
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Unknown thread sub-command %c\n", gdbctx->in_packet[0]);
        return packet_error;
    }
}

static enum packet_return packet_read_memory(struct gdb_context* gdbctx)
{
    char               *addr;
    unsigned int        len, blk_len, nread;
    char                buffer[32];
    SIZE_T              r = 0;

    assert(gdbctx->in_trap);
    /* FIXME:check in_packet_len for reading %p,%x */
    if (sscanf(gdbctx->in_packet, "%p,%x", &addr, &len) != 2) return packet_error;
    if (len <= 0) return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Read mem at %p for %u bytes\n", addr, len);
    for (nread = 0; nread < len; nread += r, addr += r)
    {
        blk_len = min(sizeof(buffer), len - nread);
        if (!gdbctx->process->process_io->read(gdbctx->process->handle, addr, buffer, blk_len, &r) ||
            r == 0)
        {
            /* fail at first address, return error */
            if (nread == 0) return packet_reply_error(gdbctx, EFAULT);
            /* something has already been read, return partial information */
            break;
        }
        if (nread == 0) packet_reply_open(gdbctx);
        packet_reply_hex_to(gdbctx, buffer, r);
    }
    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_memory(struct gdb_context* gdbctx)
{
    char*               addr;
    unsigned int        len, blk_len;
    char*               ptr;
    char                buffer[32];
    SIZE_T              w;

    assert(gdbctx->in_trap);
    ptr = memchr(gdbctx->in_packet, ':', gdbctx->in_packet_len);
    if (ptr == NULL)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Cannot find ':' in %*.*s\n",
                    gdbctx->in_packet_len, gdbctx->in_packet_len, gdbctx->in_packet);
        return packet_error;
    }
    *ptr++ = '\0';

    if (sscanf(gdbctx->in_packet, "%p,%x", &addr, &len) != 2)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Cannot scan addr,len in %s\n", gdbctx->in_packet);
        return packet_error;
    }
    if (ptr - gdbctx->in_packet + len * 2 != gdbctx->in_packet_len)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Wrong sizes %u <> %u\n",
                    ptr - gdbctx->in_packet + len * 2, gdbctx->in_packet_len);
        return packet_error;
    }
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Write %u bytes at %p\n", len, addr);
    while (len > 0)
    {
        blk_len = min(sizeof(buffer), len);
        hex_from(buffer, ptr, blk_len);
        if (!gdbctx->process->process_io->write(gdbctx->process->handle, addr, buffer, blk_len, &w) ||
            w != blk_len)
            break;
        addr += blk_len;
        len -= blk_len;
        ptr += blk_len;
    }
    return packet_ok; /* FIXME: error while writing ? */
}

static enum packet_return packet_write_register(struct gdb_context* gdbctx)
{
    unsigned            reg;
    char*               ptr;
    char*               end;
    CONTEXT             ctx;
    CONTEXT*            pctx = &gdbctx->context;

    assert(gdbctx->in_trap);

    ptr = memchr(gdbctx->in_packet, '=', gdbctx->in_packet_len);
    *ptr++ = '\0';
    reg = strtoul(gdbctx->in_packet, &end, 16);
    if (end == NULL || reg > cpu_num_regs)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Invalid register index %s\n", gdbctx->in_packet);
        /* FIXME: if just the reg is above cpu_num_regs, don't tell gdb
         *        it wouldn't matter too much, and it fakes our support for all regs
         */
        return (end == NULL) ? packet_error : packet_ok;
    }
    if (ptr + 8 - gdbctx->in_packet != gdbctx->in_packet_len)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Wrong sizes %u <> %u\n",
                    ptr + 8 - gdbctx->in_packet, gdbctx->in_packet_len);
        return packet_error;
    }
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Writing reg %u <= %*.*s\n",
                reg, gdbctx->in_packet_len - (ptr - gdbctx->in_packet),
                gdbctx->in_packet_len - (ptr - gdbctx->in_packet), ptr);

    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }

    hex_from(cpu_register(pctx, reg), ptr, 4);
    if (pctx != &gdbctx->context && !SetThreadContext(gdbctx->other_thread->handle, pctx))
    {
        if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
            fprintf(stderr, "Cannot set context for thread %04x\n", gdbctx->other_thread->tid);
        return packet_error;
    }

    return packet_ok;
}

static void packet_query_monitor_wnd_helper(struct gdb_context* gdbctx, HWND hWnd, int indent)
{
    char        buffer[128];
    char	clsName[128];
    char	wndName[128];
    HWND	child;

    do {
       if (!GetClassName(hWnd, clsName, sizeof(clsName)))
	  strcpy(clsName, "-- Unknown --");
       if (!GetWindowText(hWnd, wndName, sizeof(wndName)))
	  strcpy(wndName, "-- Empty --");

       packet_reply_open(gdbctx);
       packet_reply_catc(gdbctx, 'O');
       snprintf(buffer, sizeof(buffer),
                "%*s%04lx%*s%-17.17s %08x %08x %.14s\n",
                indent, "", (ULONG_PTR)hWnd, 13 - indent, "",
                clsName, GetWindowLong(hWnd, GWL_STYLE),
                GetWindowLongPtr(hWnd, GWLP_WNDPROC), wndName);
       packet_reply_hex_to_str(gdbctx, buffer);
       packet_reply_close(gdbctx);

       if ((child = GetWindow(hWnd, GW_CHILD)) != 0)
	  packet_query_monitor_wnd_helper(gdbctx, child, indent + 1);
    } while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT)) != 0);
}

static void packet_query_monitor_wnd(struct gdb_context* gdbctx, int len, const char* str)
{
    char        buffer[128];

    /* we do the output in several 'O' packets, with the last one being just OK for
     * marking the end of the output */
    packet_reply_open(gdbctx);
    packet_reply_catc(gdbctx, 'O');
    snprintf(buffer, sizeof(buffer),
             "%-16.16s %-17.17s %-8.8s %s\n",
             "hwnd", "Class Name", " Style", " WndProc Text");
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);

    /* FIXME: could also add a pmt to this command in str... */
    packet_query_monitor_wnd_helper(gdbctx, GetDesktopWindow(), 0);
    packet_reply(gdbctx, "OK", 2);
}

static void packet_query_monitor_process(struct gdb_context* gdbctx, int len, const char* str)
{
    HANDLE              snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    char                buffer[128];
    char                deco;
    PROCESSENTRY32      entry;
    BOOL                ok;

    if (snap == INVALID_HANDLE_VALUE)
        return;

    entry.dwSize = sizeof(entry);
    ok = Process32First(snap, &entry);

    /* we do the output in several 'O' packets, with the last one being just OK for
     * marking the end of the output */

    packet_reply_open(gdbctx);
    packet_reply_catc(gdbctx, 'O');
    snprintf(buffer, sizeof(buffer),
             " %-8.8s %-8.8s %-8.8s %s\n",
             "pid", "threads", "parent", "executable");
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);

    while (ok)
    {
        deco = ' ';
        if (entry.th32ProcessID == gdbctx->process->pid) deco = '>';
        packet_reply_open(gdbctx);
        packet_reply_catc(gdbctx, 'O');
        snprintf(buffer, sizeof(buffer),
                 "%c%08x %-8d %08x '%s'\n",
                 deco, entry.th32ProcessID, entry.cntThreads,
                 entry.th32ParentProcessID, entry.szExeFile);
        packet_reply_hex_to_str(gdbctx, buffer);
        packet_reply_close(gdbctx);
        ok = Process32Next(snap, &entry);
    }
    CloseHandle(snap);
    packet_reply(gdbctx, "OK", 2);
}

static void packet_query_monitor_mem(struct gdb_context* gdbctx, int len, const char* str)
{
    MEMORY_BASIC_INFORMATION    mbi;
    char*                       addr = 0;
    const char*                 state;
    const char*                 type;
    char                        prot[3+1];
    char                        buffer[128];

    /* we do the output in several 'O' packets, with the last one being just OK for
     * marking the end of the output */
    packet_reply_open(gdbctx);
    packet_reply_catc(gdbctx, 'O');
    packet_reply_hex_to_str(gdbctx, "Address  Size     State   Type    RWX\n");
    packet_reply_close(gdbctx);

    while (VirtualQueryEx(gdbctx->process->handle, addr, &mbi, sizeof(mbi)) >= sizeof(mbi))
    {
        switch (mbi.State)
        {
        case MEM_COMMIT:        state = "commit "; break;
        case MEM_FREE:          state = "free   "; break;
        case MEM_RESERVE:       state = "reserve"; break;
        default:                state = "???    "; break;
        }
        if (mbi.State != MEM_FREE)
        {
            switch (mbi.Type)
            {
            case MEM_IMAGE:         type = "image  "; break;
            case MEM_MAPPED:        type = "mapped "; break;
            case MEM_PRIVATE:       type = "private"; break;
            case 0:                 type = "       "; break;
            default:                type = "???    "; break;
            }
            memset(prot, ' ' , sizeof(prot)-1);
            prot[sizeof(prot)-1] = '\0';
            if (mbi.AllocationProtect & (PAGE_READONLY|PAGE_READWRITE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE))
                prot[0] = 'R';
            if (mbi.AllocationProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
                prot[1] = 'W';
            if (mbi.AllocationProtect & (PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))
                prot[1] = 'C';
            if (mbi.AllocationProtect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE))
                prot[2] = 'X';
        }
        else
        {
            type = "";
            prot[0] = '\0';
        }
        packet_reply_open(gdbctx);
        snprintf(buffer, sizeof(buffer), "%08lx %08lx %s %s %s\n",
                 (DWORD_PTR)addr, mbi.RegionSize, state, type, prot);
        packet_reply_catc(gdbctx, 'O');
        packet_reply_hex_to_str(gdbctx, buffer);
        packet_reply_close(gdbctx);

        if (addr + mbi.RegionSize < addr) /* wrap around ? */
            break;
        addr += mbi.RegionSize;
    }
    packet_reply(gdbctx, "OK", 2);
}

static void packet_query_monitor_trace(struct gdb_context* gdbctx,
                                       int len, const char* str)
{
    char        buffer[128];

    if (len == 0)
    {
        snprintf(buffer, sizeof(buffer), "trace=%x\n", gdbctx->trace);
    }
    else if (len >= 2 && str[0] == '=')
    {
        unsigned val = atoi(&str[1]);
        snprintf(buffer, sizeof(buffer), "trace: %x => %x\n", gdbctx->trace, val);
        gdbctx->trace = val;
    }
    else
    {
        /* FIXME: ugly but can use error packet here */
        packet_reply_cat(gdbctx, "E00");
        return;
    }
    packet_reply_open(gdbctx);
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);
}

struct query_detail
{
    int         with_arg;
    const char* name;
    size_t      len;
    void        (*handler)(struct gdb_context*, int, const char*);
} query_details[] =
{
    {0, "wnd",     3, packet_query_monitor_wnd},
    {0, "window",  6, packet_query_monitor_wnd},
    {0, "proc",    4, packet_query_monitor_process},
    {0, "process", 7, packet_query_monitor_process},
    {0, "mem",     3, packet_query_monitor_mem},
    {1, "trace",   5, packet_query_monitor_trace},
    {0, NULL,      0, NULL},
};

static enum packet_return packet_query_remote_command(struct gdb_context* gdbctx,
                                                      const char* hxcmd, size_t len)
{
    char                        buffer[128];
    struct query_detail*        qd;

    assert((len & 1) == 0 && len < 2 * sizeof(buffer));
    len /= 2;
    hex_from(buffer, hxcmd, len);

    for (qd = &query_details[0]; qd->name != NULL; qd++)
    {
        if (len < qd->len || strncmp(buffer, qd->name, qd->len) != 0) continue;
        if (!qd->with_arg && len != qd->len) continue;

        (qd->handler)(gdbctx, len - qd->len, buffer + qd->len);
        return packet_done;
    }
    return packet_reply_error(gdbctx, EINVAL);
}

static enum packet_return packet_query(struct gdb_context* gdbctx)
{
    switch (gdbctx->in_packet[0])
    {
    case 'f':
        if (strncmp(gdbctx->in_packet + 1, "ThreadInfo", gdbctx->in_packet_len - 1) == 0)
        {
            struct dbg_thread*  thd;

            packet_reply_open(gdbctx);
            packet_reply_add(gdbctx, "m", 1);
            for (thd = gdbctx->process->threads; thd; thd = thd->next)
            {
                packet_reply_val(gdbctx, thd->tid, 4);
                if (thd->next != NULL)
                    packet_reply_add(gdbctx, ",", 1);
            }
            packet_reply_close(gdbctx);
            return packet_done;
        }
        else if (strncmp(gdbctx->in_packet + 1, "ProcessInfo", gdbctx->in_packet_len - 1) == 0)
        {
            char        result[128];

            packet_reply_open(gdbctx);
            packet_reply_catc(gdbctx, 'O');
            get_process_info(gdbctx, result, sizeof(result));
            packet_reply_hex_to_str(gdbctx, result);
            packet_reply_close(gdbctx);
            return packet_done;
        }
        break;
    case 's':
        if (strncmp(gdbctx->in_packet + 1, "ThreadInfo", gdbctx->in_packet_len - 1) == 0)
        {
            packet_reply(gdbctx, "l", 1);
            return packet_done;
        }
        else if (strncmp(gdbctx->in_packet + 1, "ProcessInfo", gdbctx->in_packet_len - 1) == 0)
        {
            packet_reply(gdbctx, "l", 1);
            return packet_done;
        }
        break;
    case 'C':
        if (gdbctx->in_packet_len == 1)
        {
            struct dbg_thread*  thd;
            /* FIXME: doc says 16 bit val ??? */
            /* grab first created thread, aka last in list */
            assert(gdbctx->process && gdbctx->process->threads);
            for (thd = gdbctx->process->threads; thd->next; thd = thd->next);
            packet_reply_open(gdbctx);
            packet_reply_add(gdbctx, "QC", 2);
            packet_reply_val(gdbctx, thd->tid, 4);
            packet_reply_close(gdbctx);
            return packet_done;
        }
        break;
    case 'O':
        if (strncmp(gdbctx->in_packet, "Offsets", gdbctx->in_packet_len) == 0)
        {
            char    buf[64];

            if (gdbctx->wine_segs[0] == 0 && gdbctx->wine_segs[1] == 0 &&
                gdbctx->wine_segs[2] == 0)
                return packet_error;
            snprintf(buf, sizeof(buf), 
                     "Text=%08lx;Data=%08lx;Bss=%08lx",
                     gdbctx->wine_segs[0], gdbctx->wine_segs[1],
                     gdbctx->wine_segs[2]);
            return packet_reply(gdbctx, buf, -1);
        }
        break;
    case 'R':
        if (gdbctx->in_packet_len > 5 && strncmp(gdbctx->in_packet, "Rcmd,", 5) == 0)
        {
            return packet_query_remote_command(gdbctx, gdbctx->in_packet + 5,
                                               gdbctx->in_packet_len - 5);
        }
        break;
    case 'S':
        if (strncmp(gdbctx->in_packet, "Symbol::", gdbctx->in_packet_len) == 0)
            return packet_ok;
        break;
    case 'T':
        if (gdbctx->in_packet_len > 15 &&
            strncmp(gdbctx->in_packet, "ThreadExtraInfo", 15) == 0 &&
            gdbctx->in_packet[15] == ',')
        {
            unsigned    tid;
            char*       end;
            char        result[128];

            tid = strtol(gdbctx->in_packet + 16, &end, 16);
            if (end == NULL) break;
            get_thread_info(gdbctx, tid, result, sizeof(result));
            packet_reply_open(gdbctx);
            packet_reply_hex_to_str(gdbctx, result);
            packet_reply_close(gdbctx);
            return packet_done;
        }
        break;
    }
    if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
        fprintf(stderr, "Unknown or malformed query %*.*s\n",
                gdbctx->in_packet_len, gdbctx->in_packet_len, gdbctx->in_packet);
    return packet_error;
}

static enum packet_return packet_step(struct gdb_context* gdbctx)
{
    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 0);
    if (dbg_curr_thread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: step on %04x, while last thread is %04x\n",
                    gdbctx->exec_thread->tid, dbg_curr_thread->tid);
    be_cpu->single_step(&gdbctx->context, TRUE);
    resume_debuggee(gdbctx, DBG_CONTINUE);
    wait_for_debuggee(gdbctx);
    be_cpu->single_step(&gdbctx->context, FALSE);
    return packet_reply_status(gdbctx);
}

#if 0
static enum packet_return packet_step_signal(struct gdb_context* gdbctx)
{
    unsigned char sig;

    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 2);
    if (dbg_curr_thread->tid != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "NIY: step/sig on %u, while last thread is %u\n",
                    gdbctx->exec_thread, DEBUG_CurrThread->tid);
    hex_from(&sig, gdbctx->in_packet, 1);
    /* cannot change signals on the fly */
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "sigs: %u %u\n", sig, gdbctx->last_sig);
    if (sig != gdbctx->last_sig)
        return packet_error;
    resume_debuggee(gdbctx, DBG_EXCEPTION_NOT_HANDLED);
    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}
#endif

static enum packet_return packet_thread_alive(struct gdb_context* gdbctx)
{
    char*       end;
    unsigned    tid;

    tid = strtol(gdbctx->in_packet, &end, 16);
    if (tid == -1 || tid == 0)
        return packet_reply_error(gdbctx, EINVAL);
    if (dbg_get_thread(gdbctx->process, tid) != NULL)
        return packet_ok;
    return packet_reply_error(gdbctx, ESRCH);
}

static enum packet_return packet_remove_breakpoint(struct gdb_context* gdbctx)
{
    void*                       addr;
    unsigned                    len;
    struct gdb_ctx_Xpoint*      xpt;
    enum be_xpoint_type         t;

    /* FIXME: check packet_len */
    if (gdbctx->in_packet[0] < '0' || gdbctx->in_packet[0] > '4' ||
        gdbctx->in_packet[1] != ',' ||
        sscanf(gdbctx->in_packet + 2, "%p,%x", &addr, &len) != 2)
        return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Remove bp %p[%u] typ=%c\n",
                addr, len, gdbctx->in_packet[0]);
    switch (gdbctx->in_packet[0])
    {
    case '0': t = be_xpoint_break; len = 0; break;
    case '1': t = be_xpoint_watch_exec; break;
    case '2': t = be_xpoint_watch_read; break;
    case '3': t = be_xpoint_watch_write; break;
    default: return packet_error;
    }
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->addr == addr && xpt->type == t)
        {
            if (be_cpu->remove_Xpoint(gdbctx->process->handle,
                                      gdbctx->process->process_io, &gdbctx->context,
                                      t, xpt->addr, xpt->val, len))
            {
                xpt->type = -1;
                return packet_ok;
            }
            break;
        }
    }
    return packet_error;
}

static enum packet_return packet_set_breakpoint(struct gdb_context* gdbctx)
{
    void*                       addr;
    unsigned                    len;
    struct gdb_ctx_Xpoint*      xpt;
    enum be_xpoint_type         t;

    /* FIXME: check packet_len */
    if (gdbctx->in_packet[0] < '0' || gdbctx->in_packet[0] > '4' ||
        gdbctx->in_packet[1] != ',' ||
        sscanf(gdbctx->in_packet + 2, "%p,%x", &addr, &len) != 2)
        return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Set bp %p[%u] typ=%c\n",
                addr, len, gdbctx->in_packet[0]);
    switch (gdbctx->in_packet[0])
    {
    case '0': t = be_xpoint_break; len = 0; break;
    case '1': t = be_xpoint_watch_exec; break;
    case '2': t = be_xpoint_watch_read; break;
    case '3': t = be_xpoint_watch_write; break;
    default: return packet_error;
    }
    /* because of packet command handling, this should be made idempotent */
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->addr == addr && xpt->type == t)
            return packet_ok; /* nothing to do */
    }
    /* really set the Xpoint */
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->type == -1)
        {
            if (be_cpu->insert_Xpoint(gdbctx->process->handle,
                                      gdbctx->process->process_io, &gdbctx->context, 
                                      t, addr, &xpt->val, len))
            {
                xpt->addr = addr;
                xpt->type = t;
                return packet_ok;
            }
            fprintf(stderr, "cannot set xpoint\n");
            break;
        }
    }
    /* no more entries... eech */
    fprintf(stderr, "Running out of spots for {break|watch}points\n");
    return packet_error;
}

/* =============================================== *
 *    P A C K E T  I N F R A S T R U C T U R E     *
 * =============================================== *
 */

struct packet_entry
{
    char                key;
    enum packet_return  (*handler)(struct gdb_context* gdbctx);
};

static struct packet_entry packet_entries[] =
{
        /*{'!', packet_extended}, */
        {'?', packet_last_signal},
        {'c', packet_continue},
        {'C', packet_continue_signal},
        {'D', packet_detach},
        {'g', packet_read_registers},
        {'G', packet_write_registers},
        {'k', packet_kill},
        {'H', packet_thread},
        {'m', packet_read_memory},
        {'M', packet_write_memory},
        /* {'p', packet_read_register}, doesn't seem needed */
        {'P', packet_write_register},
        {'q', packet_query},
        /* {'Q', packet_set}, */
        /* {'R', packet,restart}, only in extended mode ! */
        {'s', packet_step},        
        /*{'S', packet_step_signal}, hard(er) to implement */
        {'T', packet_thread_alive},
        {'v', packet_verbose},
        {'z', packet_remove_breakpoint},
        {'Z', packet_set_breakpoint},
};

static BOOL extract_packets(struct gdb_context* gdbctx)
{
    char*               end;
    int                 plen;
    unsigned char       in_cksum, loc_cksum;
    char*               ptr;
    enum packet_return  ret = packet_error;
    int                 num_packet = 0;

    while ((ret & packet_last_f) == 0)
    {
        if (gdbctx->in_len && (gdbctx->trace & GDBPXY_TRC_LOWLEVEL))
            fprintf(stderr, "In-buf: %*.*s\n",
                    gdbctx->in_len, gdbctx->in_len, gdbctx->in_buf);
        ptr = memchr(gdbctx->in_buf, '$', gdbctx->in_len);
        if (ptr == NULL) return FALSE;
        if (ptr != gdbctx->in_buf)
        {
            int glen = ptr - gdbctx->in_buf; /* garbage len */
            if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
                fprintf(stderr, "Removing garbage: %*.*s\n",
                        glen, glen, gdbctx->in_buf);
            gdbctx->in_len -= glen;
            memmove(gdbctx->in_buf, ptr, gdbctx->in_len);
        }
        end = memchr(gdbctx->in_buf + 1, '#', gdbctx->in_len);
        if (end == NULL) return FALSE;
        /* no checksum yet */
        if (end + 3 > gdbctx->in_buf + gdbctx->in_len) return FALSE;
        plen = end - gdbctx->in_buf - 1;
        hex_from(&in_cksum, end + 1, 1);
        loc_cksum = checksum(gdbctx->in_buf + 1, plen);
        if (loc_cksum == in_cksum)
        {
            if (num_packet == 0) {
                int                 i;
                
                ret = packet_error;
                
                write(gdbctx->sock, "+", 1);
                assert(plen);
                
                /* FIXME: should use bsearch if packet_entries was sorted */
                for (i = 0; i < sizeof(packet_entries)/sizeof(packet_entries[0]); i++)
                {
                    if (packet_entries[i].key == gdbctx->in_buf[1]) break;
                }
                if (i == sizeof(packet_entries)/sizeof(packet_entries[0]))
                {
                    if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
                        fprintf(stderr, "Unknown packet request %*.*s\n",
                                plen, plen, &gdbctx->in_buf[1]);
                }
                else
                {
                    gdbctx->in_packet = gdbctx->in_buf + 2;
                    gdbctx->in_packet_len = plen - 1;
                    if (gdbctx->trace & GDBPXY_TRC_PACKET)
                        fprintf(stderr, "Packet: %c%*.*s\n",
                                gdbctx->in_buf[1],
                                gdbctx->in_packet_len, gdbctx->in_packet_len,
                                gdbctx->in_packet);
                    ret = (packet_entries[i].handler)(gdbctx);
                }
                switch (ret & ~packet_last_f)
                {
                case packet_error:  packet_reply(gdbctx, "", 0); break;
                case packet_ok:     packet_reply(gdbctx, "OK", 2); break;
                case packet_done:   break;
                }
                if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
                    fprintf(stderr, "Reply-full: %*.*s\n",
                            gdbctx->out_len, gdbctx->out_len, gdbctx->out_buf);
                i = write(gdbctx->sock, gdbctx->out_buf, gdbctx->out_len);
                assert(i == gdbctx->out_len);
                /* if this fails, we'll have to use POLLOUT...
                 */
                gdbctx->out_len = 0;
                num_packet++;
            }
            else 
            {
                /* FIXME: if we have in our input buffer more than one packet, 
                 * it's very likely that we took too long to answer to a given packet
                 * and gdb is sending us again the same packet
                 * We simply drop the second packet. This will lower the risk of error, 
                 * but there's still some race conditions here
                 * A better fix (yet not perfect) would be to have two threads:
                 * - one managing the packets for gdb
                 * - the second one managing the commands...
                 * This would allow us also the reply with the '+' character (Ack of
                 * the command) way sooner than what we do now
                 */
                if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
                    fprintf(stderr, "Dropping packet, I was too slow to respond\n");
            }
        }
        else
        {
            write(gdbctx->sock, "+", 1);
            if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
                fprintf(stderr, "Dropping packet, invalid checksum %d <> %d\n", in_cksum, loc_cksum);
        }
        gdbctx->in_len -= plen + 4;
        memmove(gdbctx->in_buf, end + 3, gdbctx->in_len);
    }
    return TRUE;
}

static int fetch_data(struct gdb_context* gdbctx)
{
    int len, in_len = gdbctx->in_len;

    assert(gdbctx->in_len <= gdbctx->in_buf_alloc);
    for (;;)
    {
#define STEP 128
        if (gdbctx->in_len + STEP > gdbctx->in_buf_alloc)
            gdbctx->in_buf = realloc(gdbctx->in_buf, gdbctx->in_buf_alloc += STEP);
#undef STEP
        if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
            fprintf(stderr, "%d %d %*.*s\n",
                    gdbctx->in_len, gdbctx->in_buf_alloc,
                    gdbctx->in_len, gdbctx->in_len, gdbctx->in_buf);
        len = read(gdbctx->sock, gdbctx->in_buf + gdbctx->in_len, gdbctx->in_buf_alloc - gdbctx->in_len);
        if (len <= 0) break;
        gdbctx->in_len += len;
        assert(gdbctx->in_len <= gdbctx->in_buf_alloc);
        if (len < gdbctx->in_buf_alloc - gdbctx->in_len) break;
    }
    if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
        fprintf(stderr, "=> %d\n", gdbctx->in_len - in_len);
    return gdbctx->in_len - in_len;
}

#define FLAG_NO_START   1
#define FLAG_WITH_XTERM 2

static BOOL gdb_exec(const char* wine_path, unsigned port, unsigned flags)
{
    char            buf[MAX_PATH];
    int             fd;
    const char*     gdb_path;
    FILE*           f;

    if (!(gdb_path = getenv("WINE_GDB"))) gdb_path = "gdb";
    strcpy(buf,"/tmp/winegdb.XXXXXX");
    fd = mkstemps(buf, 0);
    if (fd == -1) return FALSE;
    if ((f = fdopen(fd, "w+")) == NULL) return FALSE;
    fprintf(f, "file %s\n", wine_path);
    fprintf(f, "target remote localhost:%d\n", ntohs(port));
    fprintf(f, "monitor trace=%d\n", GDBPXY_TRC_COMMAND_FIXME);
    fprintf(f, "set prompt Wine-gdb>\\ \n");
    /* gdb 5.1 seems to require it, won't hurt anyway */
    fprintf(f, "sharedlibrary\n");
    /* This is needed (but not a decent & final fix)
     * Without this, gdb would skip our inter-DLL relay code (because
     * we don't have any line number information for the relay code)
     * With this, we will stop on first instruction of the stub, and
     * reusing step, will get us through the relay stub at the actual
     * function we're looking at.
     */
    fprintf(f, "set step-mode on\n");
    /* tell gdb to delete this file when done handling it... */
    fprintf(f, "shell rm -f \"%s\"\n", buf);
    fclose(f);
    if (flags & FLAG_WITH_XTERM)
        execlp("xterm", "xterm", "-e", gdb_path, "-x", buf, NULL);
    else
        execlp(gdb_path, gdb_path, "-x", buf, NULL);
    assert(0); /* never reached */
    return TRUE;
}

static BOOL gdb_startup(struct gdb_context* gdbctx, DEBUG_EVENT* de, unsigned flags)
{
    int                 sock;
    struct sockaddr_in  s_addrs;
    unsigned int        s_len = sizeof(s_addrs);
    struct pollfd       pollfd;
    IMAGEHLP_MODULE     imh_mod;

    /* step 1: create socket for gdb connection request */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
            fprintf(stderr, "Can't create socket");
        return FALSE;
    }

    if (listen(sock, 1) == -1 ||
        getsockname(sock, (struct sockaddr*)&s_addrs, &s_len) == -1)
        return FALSE;

    /* step 2: do the process internal creation */
    handle_debug_event(gdbctx, de);

    /* step3: get the wine loader name */
    if (!dbg_get_debuggee_info(gdbctx->process->handle, &imh_mod)) return FALSE;

    /* step 4: fire up gdb (if requested) */
    if (flags & FLAG_NO_START)
        fprintf(stderr, "target remote localhost:%d\n", ntohs(s_addrs.sin_port));
    else
        switch (fork())
        {
        case -1: /* error in parent... */
            fprintf(stderr, "Cannot create gdb\n");
            return FALSE;
        default: /* in parent... success */
            break;
        case 0: /* in child... and alive */
            gdb_exec(imh_mod.LoadedImageName, s_addrs.sin_port, flags);
            /* if we're here, exec failed, so report failure */
            return FALSE;
        }

    /* step 5: wait for gdb to connect actually */
    pollfd.fd = sock;
    pollfd.events = POLLIN;
    pollfd.revents = 0;

    switch (poll(&pollfd, 1, -1))
    {
    case 1:
        if (pollfd.revents & POLLIN)
        {
            int dummy = 1;
            gdbctx->sock = accept(sock, (struct sockaddr*)&s_addrs, &s_len);
            if (gdbctx->sock == -1)
                break;
            if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
                fprintf(stderr, "Connected on %d\n", gdbctx->sock);
            /* don't keep our small packets too long: send them ASAP back to GDB
             * without this, GDB really crawls
             */
            setsockopt(gdbctx->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&dummy, sizeof(dummy));
        }
        break;
    case 0:
        if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
            fprintf(stderr, "Poll for cnx failed (timeout)\n");
        return FALSE;
    case -1:
        if (gdbctx->trace & GDBPXY_TRC_LOWLEVEL)
            fprintf(stderr, "Poll for cnx failed (error)\n");
        return FALSE;
    default:
        assert(0);
    }

    close(sock);
    return TRUE;
}

static BOOL gdb_init_context(struct gdb_context* gdbctx, unsigned flags)
{
    DEBUG_EVENT         de;
    int                 i;

    gdbctx->sock = -1;
    gdbctx->in_buf = NULL;
    gdbctx->in_buf_alloc = 0;
    gdbctx->in_len = 0;
    gdbctx->out_buf = NULL;
    gdbctx->out_buf_alloc = 0;
    gdbctx->out_len = 0;
    gdbctx->out_curr_packet = -1;

    gdbctx->exec_thread = gdbctx->other_thread = NULL;
    gdbctx->last_sig = 0;
    gdbctx->in_trap = FALSE;
    gdbctx->trace = /*GDBPXY_TRC_PACKET | GDBPXY_TRC_COMMAND |*/ GDBPXY_TRC_COMMAND_ERROR | GDBPXY_TRC_COMMAND_FIXME | GDBPXY_TRC_WIN32_EVENT;
    gdbctx->process = NULL;
    for (i = 0; i < NUM_XPOINT; i++)
        gdbctx->Xpoints[i].type = -1;

    /* wait for first trap */
    while (WaitForDebugEvent(&de, INFINITE))
    {
        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            /* this should be the first event we get,
             * and the only one of this type  */
            assert(gdbctx->process == NULL && de.dwProcessId == dbg_curr_pid);
            /* gdbctx->dwProcessId = pid; */
            if (!gdb_startup(gdbctx, &de, flags)) return FALSE;
            assert(!gdbctx->in_trap);
        }
        else
        {
            handle_debug_event(gdbctx, &de);
            if (gdbctx->in_trap) break;
        }
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    }
    return TRUE;
}

static int gdb_remote(unsigned flags)
{
    struct pollfd       pollfd;
    struct gdb_context  gdbctx;
    BOOL                doLoop;

    for (doLoop = gdb_init_context(&gdbctx, flags); doLoop;)
    {
        pollfd.fd = gdbctx.sock;
        pollfd.events = POLLIN;
        pollfd.revents = 0;

        switch (poll(&pollfd, 1, -1))
        {
        case 1:
            /* got something */
            if (pollfd.revents & (POLLHUP | POLLERR))
            {
                if (gdbctx.trace & GDBPXY_TRC_LOWLEVEL)
                    fprintf(stderr, "Gdb hung up\n");
                /* kill also debuggee process - questionnable - */
                detach_debuggee(&gdbctx, TRUE);
                doLoop = FALSE;
                break;
            }
            if ((pollfd.revents & POLLIN) && fetch_data(&gdbctx) > 0)
            {
                if (extract_packets(&gdbctx)) doLoop = FALSE;
            }
            break;
        case 0:
            /* timeout, should never happen (infinite timeout) */
            break;
        case -1:
            if (gdbctx.trace & GDBPXY_TRC_LOWLEVEL)
                fprintf(stderr, "Poll failed\n");
            doLoop = FALSE;
            break;
        }
    }
    wait(NULL);
    return 0;
}
#endif

int gdb_main(int argc, char* argv[])
{
#ifdef HAVE_POLL
    unsigned gdb_flags = 0;

    argc--; argv++;
    while (argc > 0 && argv[0][0] == '-')
    {
        if (strcmp(argv[0], "--no-start") == 0)
        {
            gdb_flags |= FLAG_NO_START;
            argc--; argv++;
            continue;
        }
        if (strcmp(argv[0], "--with-xterm") == 0)
        {
            gdb_flags |= FLAG_WITH_XTERM;
            argc--; argv++;
            continue;
        }
        return -1;
    }
    if (dbg_active_attach(argc, argv) == start_ok ||
        dbg_active_launch(argc, argv) == start_ok)
        return gdb_remote(gdb_flags);
#else
    fprintf(stderr, "GdbProxy mode not supported on this platform\n");
#endif
    return -1;
}

static struct be_process_io be_process_gdbproxy_io =
{
    NULL, /* we shouldn't use close_process() in gdbproxy */
    ReadProcessMemory,
    WriteProcessMemory
};
