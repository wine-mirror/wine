/*
 * A Win32 based proxy implementing the GBD remote protocol.
 * This makes it possible to debug Wine (and any "emulated"
 * program) under Linux using GDB.
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
 * http://sources.redhat.com/gdb/onlinedocs/gdb/Maintenance-Commands.html
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

static BOOL tgt_process_gdbproxy_read(HANDLE hProcess, const void* addr,
                                      void* buffer, SIZE_T len, SIZE_T* rlen)
{
    return ReadProcessMemory( hProcess, addr, buffer, len, rlen );
}

static BOOL tgt_process_gdbproxy_write(HANDLE hProcess, void* addr,
                                       const void* buffer, SIZE_T len, SIZE_T* wlen)
{
    return WriteProcessMemory( hProcess, addr, buffer, len, wlen );
}

static struct be_process_io be_process_gdbproxy_io =
{
    NULL, /* we shouldn't use close_process() in gdbproxy */
    tgt_process_gdbproxy_read,
    tgt_process_gdbproxy_write
};

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

/* This struct helps us to manage the different representations of a register:
 * ctx_offset and ctx_length are the location and size in Win32 CONTEXT
 * gdb_length is the length gdb expects on the wire
 * As the two sizes could be different, we have to convert between the two
 * (for example, on x86_64, Seg?s are 4 bytes on the wire and 2 in CONTEXT)
 */
struct cpu_register
{
    size_t      ctx_offset;
    size_t      ctx_length;
    size_t      gdb_length;
};

#define REG(r,gs)  {FIELD_OFFSET(CONTEXT, r), sizeof(((CONTEXT*)NULL)->r), gs}

#ifdef __i386__
static struct cpu_register cpu_register_map[] = {
    REG(Eax, 4),
    REG(Ecx, 4),
    REG(Edx, 4),
    REG(Ebx, 4),
    REG(Esp, 4),
    REG(Ebp, 4),
    REG(Esi, 4),
    REG(Edi, 4),
    REG(Eip, 4),
    REG(EFlags, 4),
    REG(SegCs, 4),
    REG(SegSs, 4),
    REG(SegDs, 4),
    REG(SegEs, 4),
    REG(SegFs, 4),
    REG(SegGs, 4),
};
#elif defined(__powerpc__)
static struct cpu_register cpu_register_map[] = {
    REG(Gpr0, 4),
    REG(Gpr1, 4),
    REG(Gpr2, 4),
    REG(Gpr3, 4),
    REG(Gpr4, 4),
    REG(Gpr5, 4),
    REG(Gpr6, 4),
    REG(Gpr7, 4),
    REG(Gpr8, 4),
    REG(Gpr9, 4),
    REG(Gpr10, 4),
    REG(Gpr11, 4),
    REG(Gpr12, 4),
    REG(Gpr13, 4),
    REG(Gpr14, 4),
    REG(Gpr15, 4),
    REG(Gpr16, 4),
    REG(Gpr17, 4),
    REG(Gpr18, 4),
    REG(Gpr19, 4),
    REG(Gpr20, 4),
    REG(Gpr21, 4),
    REG(Gpr22, 4),
    REG(Gpr23, 4),
    REG(Gpr24, 4),
    REG(Gpr25, 4),
    REG(Gpr26, 4),
    REG(Gpr27, 4),
    REG(Gpr28, 4),
    REG(Gpr29, 4),
    REG(Gpr30, 4),
    REG(Gpr31, 4),
    REG(Fpr0, 4),
    REG(Fpr1, 4),
    REG(Fpr2, 4),
    REG(Fpr3, 4),
    REG(Fpr4, 4),
    REG(Fpr5, 4),
    REG(Fpr6, 4),
    REG(Fpr7, 4),
    REG(Fpr8, 4),
    REG(Fpr9, 4),
    REG(Fpr10, 4),
    REG(Fpr11, 4),
    REG(Fpr12, 4),
    REG(Fpr13, 4),
    REG(Fpr14, 4),
    REG(Fpr15, 4),
    REG(Fpr16, 4),
    REG(Fpr17, 4),
    REG(Fpr18, 4),
    REG(Fpr19, 4),
    REG(Fpr20, 4),
    REG(Fpr21, 4),
    REG(Fpr22, 4),
    REG(Fpr23, 4),
    REG(Fpr24, 4),
    REG(Fpr25, 4),
    REG(Fpr26, 4),
    REG(Fpr27, 4),
    REG(Fpr28, 4),
    REG(Fpr29, 4),
    REG(Fpr30, 4),
    REG(Fpr31, 4),

    REG(Iar, 4),
    REG(Msr, 4),
    REG(Cr, 4),
    REG(Lr, 4),
    REG(Ctr, 4),
    REG(Xer, 4),
    /* FIXME: MQ is missing? FIELD_OFFSET(CONTEXT, Mq), */
    /* see gdb/nlm/ppc.c */
};
#elif defined(__x86_64__)
static struct cpu_register cpu_register_map[] = {
    REG(Rax, 8),
    REG(Rbx, 8),
    REG(Rcx, 8),
    REG(Rdx, 8),
    REG(Rsi, 8),
    REG(Rdi, 8),
    REG(Rbp, 8),
    REG(Rsp, 8),
    REG(R8, 8),
    REG(R9, 8),
    REG(R10, 8),
    REG(R11, 8),
    REG(R12, 8),
    REG(R13, 8),
    REG(R14, 8),
    REG(R15, 8),
    REG(Rip, 8),
    REG(EFlags, 4),
    REG(SegCs, 4),
    REG(SegSs, 4),
    REG(SegDs, 4),
    REG(SegEs, 4),
    REG(SegFs, 4),
    REG(SegGs, 4),
};
#elif defined(__sparc__)
static struct cpu_register cpu_register_map[] = {
    REG(g0, 4),
    REG(g1, 4),
    REG(g2, 4),
    REG(g3, 4),
    REG(g4, 4),
    REG(g5, 4),
    REG(g6, 4),
    REG(g7, 4),
    REG(o0, 4),
    REG(o1, 4),
    REG(o2, 4),
    REG(o3, 4),
    REG(o4, 4),
    REG(o5, 4),
    REG(o6, 4),
    REG(o7, 4),
    REG(l0, 4),
    REG(l1, 4),
    REG(l2, 4),
    REG(l3, 4),
    REG(l4, 4),
    REG(l5, 4),
    REG(l6, 4),
    REG(l7, 4),
    REG(i0, 4),
    REG(i1, 4),
    REG(i2, 4),
    REG(i3, 4),
    REG(i4, 4),
    REG(i5, 4),
    REG(i6, 4),
    REG(i7, 4),
};
#elif defined(__arm__)
static struct cpu_register cpu_register_map[] = {
    REG(R0, 4),
    REG(R1, 4),
    REG(R2, 4),
    REG(R3, 4),
    REG(R4, 4),
    REG(R5, 4),
    REG(R6, 4),
    REG(R7, 4),
    REG(R8, 4),
    REG(R9, 4),
    REG(R10, 4),
    REG(Fp, 4),
    REG(Ip, 4),
    REG(Sp, 4),
    REG(Lr, 4),
    REG(Pc, 4),
};
#else
# error Define the registers map for your CPU
#endif
#undef REG

static const size_t cpu_num_regs = (sizeof(cpu_register_map) / sizeof(cpu_register_map[0]));

static inline void* cpu_register_ptr(CONTEXT* ctx, unsigned idx)
{
    assert(idx < cpu_num_regs);
    return (char*)ctx + cpu_register_map[idx].ctx_offset;
}

static inline DWORD64   cpu_register(CONTEXT* ctx, unsigned idx)
{
    switch (cpu_register_map[idx].ctx_length)
    {
    case 2: return *(WORD*)cpu_register_ptr(ctx, idx);
    case 4: return *(DWORD*)cpu_register_ptr(ctx, idx);
    case 8: return *(DWORD64*)cpu_register_ptr(ctx, idx);
    default:
        fprintf(stderr, "got unexpected size: %u\n", (unsigned)cpu_register_map[idx].ctx_length);
        assert(0);
    }
}

static inline   void    cpu_register_hex_from(CONTEXT* ctx, unsigned idx, const char** phex)
{
    DWORD64     val = 0;
    unsigned    i;
    BYTE        b;

    for (i = 0; i < cpu_register_map[idx].gdb_length; i++)
    {
        hex_from(&b, *phex, 1);
        *phex += 2;
        val += (DWORD64)b << (8 * i);
    }
    switch (cpu_register_map[idx].ctx_length)
    {
    case 2: *(WORD*)cpu_register_ptr(ctx, idx) = (WORD)val; break;
    case 4: *(DWORD*)cpu_register_ptr(ctx, idx) = (DWORD)val; break;
    case 8: *(DWORD64*)cpu_register_ptr(ctx, idx) = val; break;
    default: assert(0);
    }
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
        /* fall through */
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
    union {
        char                bufferA[256];
        WCHAR               buffer[256];
    } u;

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
                                   u.buffer, sizeof(u.buffer) / sizeof(WCHAR));
        dbg_set_process_name(gdbctx->process, u.buffer);

        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%04x:%04x: create process '%s'/%p @%p (%u<%u>)\n",
                    de->dwProcessId, de->dwThreadId,
                    dbg_W2A(u.buffer, -1),
                    de->u.CreateProcessInfo.lpImageName,
                    de->u.CreateProcessInfo.lpStartAddress,
                    de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                    de->u.CreateProcessInfo.nDebugInfoSize);

        /* de->u.CreateProcessInfo.lpStartAddress; */
        if (!dbg_init(gdbctx->process->handle, u.buffer, TRUE))
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
                                   u.buffer, sizeof(u.buffer) / sizeof(WCHAR));
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%04x:%04x: loads DLL %s @%p (%u<%u>)\n",
                    de->dwProcessId, de->dwThreadId,
                    dbg_W2A(u.buffer, -1),
                    de->u.LoadDll.lpBaseOfDll,
                    de->u.LoadDll.dwDebugInfoFileOffset,
                    de->u.LoadDll.nDebugInfoSize);
        dbg_load_module(gdbctx->process->handle, de->u.LoadDll.hFile, u.buffer,
                        (DWORD_PTR)de->u.LoadDll.lpBaseOfDll, 0);
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: unload DLL @%p\n",
                    de->dwProcessId, de->dwThreadId, de->u.UnloadDll.lpBaseOfDll);
        SymUnloadModule(gdbctx->process->handle,
                        (DWORD_PTR)de->u.UnloadDll.lpBaseOfDll);
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
                          de->u.DebugString.fUnicode, u.bufferA, sizeof(u.bufferA));
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08x:%08x: output debug string (%s)\n",
                    de->dwProcessId, de->dwThreadId, u.bufferA);
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

static char* packet_realloc(char* buf, int size)
{
    if (!buf)
        return HeapAlloc(GetProcessHeap(), 0, size);
    return HeapReAlloc(GetProcessHeap(), 0, buf, size);

}

static void packet_reply_grow(struct gdb_context* gdbctx, size_t size)
{
    if (gdbctx->out_buf_alloc < gdbctx->out_len + size)
    {
        gdbctx->out_buf_alloc = ((gdbctx->out_len + size) / 32 + 1) * 32;
        gdbctx->out_buf = packet_realloc(gdbctx->out_buf, gdbctx->out_buf_alloc);
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

static inline void packet_reply_register_hex_to(struct gdb_context* gdbctx, unsigned idx)
{
    DWORD64     val = cpu_register(&gdbctx->context, idx);
    unsigned    i;

    for (i = 0; i < cpu_register_map[idx].gdb_length; i++)
    {
        BYTE    b = val;
        packet_reply_hex_to(gdbctx, &b, 1);
        val >>= 8;
    }
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
            packet_reply_register_hex_to(gdbctx, i);
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

static enum packet_return packet_verbose_cont(struct gdb_context* gdbctx)
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

    /* OK we have vCont followed by..
    * ? for query
    * c for packet_continue
    * Csig for packet_continue_signal
    * s for step
    * Ssig  for step signal
    * and then an optional thread ID at the end..
    * *******************************************/

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
    LIST_FOR_EACH_ENTRY(thd, &gdbctx->process->threads, struct dbg_thread, entry)
    {
        threadIDs[threadCount++] = thd->tid;
        /* check to see if we have more threads than I counted on, and tell the user what to do
         * (they're running winedbg, so I'm sure they can fix the problem from the error message!) */
        if (threadCount == 100)
        {
            fprintf(stderr, "Wow, that's a lot of threads, change threadIDs in wine/programs/winedbg/gdbproxy.c to be higher\n");
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

struct verbose_defail
{
    const char*         name;
    unsigned            len;
    enum packet_return  (*handler)(struct gdb_context*);
} verbose_details[] =
{
    /* {"Attach",           6}, */
    {"Cont",             4, packet_verbose_cont},
    /* {"File",             4},
    {"FlashErase",      10},
    {"FlashWrite",      10},
    {"FlashDone",        9},
    {"Kill",             4},
    {"Run",              3},
    {"Stopped",          7},*/
};

static enum packet_return packet_verbose(struct gdb_context* gdbctx)
{
    unsigned i;
    unsigned klen;

    for (klen = 0; ; klen++)
    {
        if (klen == gdbctx->in_packet_len ||
            gdbctx->in_packet[klen] == ';' ||
            gdbctx->in_packet[klen] == ':' ||
            gdbctx->in_packet[klen] == '?')
        {
            if (gdbctx->trace & GDBPXY_TRC_COMMAND)
                fprintf(stderr, "trying to process a verbose packet %*.*s\n",
                        gdbctx->in_packet_len, gdbctx->in_packet_len, gdbctx->in_packet);
            for (i = 0; i < sizeof(verbose_details)/sizeof(verbose_details[0]); i++)
            {
                if (klen == verbose_details[i].len &&
                    !memcmp(gdbctx->in_packet, verbose_details[i].name, verbose_details[i].len))
                {
                    return verbose_details[i].handler(gdbctx);
                }
            }
            /* no matching handler found, abort */
            break;
        }
    }

    if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
        fprintf(stderr, "No support for verbose packet %*.*s\n",
                gdbctx->in_packet_len, gdbctx->in_packet_len, gdbctx->in_packet);
    return packet_error;
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
        packet_reply_register_hex_to(gdbctx, i);
    }
    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_registers(struct gdb_context* gdbctx)
{
    unsigned    i;
    CONTEXT     ctx;
    CONTEXT*    pctx = &gdbctx->context;
    const char* ptr;

    assert(gdbctx->in_trap);
    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }
    if (gdbctx->in_packet_len < cpu_num_regs * 2) return packet_error;

    ptr = gdbctx->in_packet;
    for (i = 0; i < cpu_num_regs; i++)
    {
        cpu_register_hex_from(pctx, i, &ptr);
    }
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
                    (int)(ptr - gdbctx->in_packet) + len * 2, gdbctx->in_packet_len);
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

static enum packet_return packet_read_register(struct gdb_context* gdbctx)
{
    unsigned            reg;
    CONTEXT             ctx;
    CONTEXT*            pctx = &gdbctx->context;

    assert(gdbctx->in_trap);
    reg = hex_to_int(gdbctx->in_packet, gdbctx->in_packet_len);
    if (reg >= cpu_num_regs)
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Register out of bounds %x\n", reg);
        return packet_error;
    }
    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Read register %x => %08x%08x\n", reg,
                (unsigned)(cpu_register(pctx, reg) >> 32), (unsigned)cpu_register(pctx, reg));
    packet_reply_open(gdbctx);
    packet_reply_register_hex_to(gdbctx, reg);
    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_register(struct gdb_context* gdbctx)
{
    unsigned            reg;
    char*               ptr;
    CONTEXT             ctx;
    CONTEXT*            pctx = &gdbctx->context;

    assert(gdbctx->in_trap);

    reg = strtoul(gdbctx->in_packet, &ptr, 16);
    if (ptr == NULL || reg > cpu_num_regs || *ptr++ != '=')
    {
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_ERROR)
            fprintf(stderr, "Invalid register index %s\n", gdbctx->in_packet);
        /* FIXME: if just the reg is above cpu_num_regs, don't tell gdb
         *        it wouldn't matter too much, and it fakes our support for all regs
         */
        return (ptr == NULL) ? packet_error : packet_ok;
    }
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
    {
        int len = gdbctx->in_packet_len - (ptr - gdbctx->in_packet);
        fprintf(stderr, "Writing reg %u <= %*.*s\n", reg, len, len, ptr);
    }

    if (dbg_curr_thread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }

    cpu_register_hex_from(pctx, reg, (const char**)&ptr);
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
       if (!GetClassNameA(hWnd, clsName, sizeof(clsName)))
	  strcpy(clsName, "-- Unknown --");
       if (!GetWindowTextA(hWnd, wndName, sizeof(wndName)))
	  strcpy(wndName, "-- Empty --");

       packet_reply_open(gdbctx);
       packet_reply_catc(gdbctx, 'O');
       snprintf(buffer, sizeof(buffer),
                "%*s%04lx%*s%-17.17s %08x %0*lx %.14s\n",
                indent, "", (ULONG_PTR)hWnd, 13 - indent, "",
                clsName, GetWindowLongW(hWnd, GWL_STYLE),
                ADDRWIDTH, (ULONG_PTR)GetWindowLongPtrW(hWnd, GWLP_WNDPROC),
                wndName);
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
        snprintf(buffer, sizeof(buffer), "%0*lx %0*lx %s %s %s\n",
                 (unsigned)sizeof(void*), (DWORD_PTR)addr,
                 (unsigned)sizeof(void*), mbi.RegionSize, state, type, prot);
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

    for (qd = query_details; qd->name != NULL; qd++)
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
            LIST_FOR_EACH_ENTRY(thd, &gdbctx->process->threads, struct dbg_thread, entry)
            {
                packet_reply_val(gdbctx, thd->tid, 4);
                if (list_next(&gdbctx->process->threads, &thd->entry) != NULL)
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
    case 'A':
        if (strncmp(gdbctx->in_packet, "Attached", gdbctx->in_packet_len) == 0)
        {
            char    buf[2];

            buf[0] = '1';
            buf[1] = 0;
            return packet_reply(gdbctx, buf, -1);
        }
        break;
    case 'C':
        if (gdbctx->in_packet_len == 1)
        {
            struct dbg_thread*  thd;
            /* FIXME: doc says 16 bit val ??? */
            /* grab first created thread, aka last in list */
            assert(gdbctx->process && !list_empty(&gdbctx->process->threads));
            thd = LIST_ENTRY(list_tail(&gdbctx->process->threads), struct dbg_thread, entry);
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
        if (strncmp(gdbctx->in_packet, "Supported", 9) == 0)
        {
            /* no features supported */
            packet_reply_open(gdbctx);
            packet_reply_close(gdbctx);
            return packet_done;
        }
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
        if (strncmp(gdbctx->in_packet, "TStatus", 7) == 0)
        {
            /* Tracepoints not supported */
            packet_reply_open(gdbctx);
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
        {'p', packet_read_register},
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
                /* FIXME: If we have more than one packet in our input buffer,
                 * it's very likely that we took too long to answer to a given packet
                 * and gdb is sending us the same packet again.
                 * So we simply drop the second packet. This will lower the risk of error,
                 * but there's still some race conditions here.
                 * A better fix (yet not perfect) would be to have two threads:
                 * - one managing the packets for gdb
                 * - the second one managing the commands...
                 * This would allow us to send the reply with the '+' character (Ack of
                 * the command) way sooner than we do now.
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
            gdbctx->in_buf = packet_realloc(gdbctx->in_buf, gdbctx->in_buf_alloc += STEP);
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
    IMAGEHLP_MODULE64   imh_mod;

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
            signal(SIGINT, SIG_IGN);
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
    for (i = 0; i < sizeof(gdbctx->wine_segs) / sizeof(gdbctx->wine_segs[0]); i++)
        gdbctx->wine_segs[i] = 0;

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
