/*
 * A Win32 based proxy implementing the GBD remote protocol
 * This allows to debug Wine (and any "emulated" program) under
 * Linux using GDB
 *
 * Copyright (c) Eric Pouech 2002-2003
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <sys/poll.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "tlhelp32.h"

/* those two are needed only for the SHOWNORMAL flag */
#include "wingdi.h"
#include "winuser.h"

#include "debugger.h"

#define GDBPXY_TRC_LOWLEVEL             0x01
#define GDBPXY_TRC_PACKET               0x02
#define GDBPXY_TRC_COMMAND              0x04
#define GDBPXY_TRC_COMMAND_ERROR        0x08
#define GDBPXY_TRC_WIN32_EVENT          0x10
#define GDBPXY_TRC_WIN32_ERROR          0x20
#define GDBPXY_TRC_COMMAND_FIXME        0x80

struct gdb_ctx_Xpoint
{
    int                         type;   /* -1 means free */
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
    DBG_THREAD*                 exec_thread;    /* thread used in step & continue */
    DBG_THREAD*                 other_thread;   /* thread to be used in any other operation */
    unsigned                    trace;
    /* current Win32 trap env */
    unsigned                    last_sig;
    BOOL                        in_trap;
    CONTEXT                     context;
    /* Win32 information */
    DBG_PROCESS*                process;
#define NUM_XPOINT      32
    struct gdb_ctx_Xpoint       Xpoints[NUM_XPOINT];
    /* Unix environment */
    unsigned long               wine_segs[3];   /* load addresses of the ELF wine exec segments (text, bss and data) */
};

extern int read_elf_info(const char* filename, unsigned long tab[]);

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

#define	OFFSET_OF(__c,__f)	((int)(((char*)&(((__c*)0)->__f))-((char*)0)))

#ifdef __i386__
static size_t cpu_register_map[] = {
    OFFSET_OF(CONTEXT, Eax),
    OFFSET_OF(CONTEXT, Ecx),
    OFFSET_OF(CONTEXT, Edx),
    OFFSET_OF(CONTEXT, Ebx),
    OFFSET_OF(CONTEXT, Esp),
    OFFSET_OF(CONTEXT, Ebp),
    OFFSET_OF(CONTEXT, Esi),
    OFFSET_OF(CONTEXT, Edi),
    OFFSET_OF(CONTEXT, Eip),
    OFFSET_OF(CONTEXT, EFlags),
    OFFSET_OF(CONTEXT, SegCs),
    OFFSET_OF(CONTEXT, SegSs),
    OFFSET_OF(CONTEXT, SegDs),
    OFFSET_OF(CONTEXT, SegEs),
    OFFSET_OF(CONTEXT, SegFs),
    OFFSET_OF(CONTEXT, SegGs),
};
#else
# ifdef __powerpc__
static size_t cpu_register_map[] = {
    OFFSET_OF(CONTEXT, Gpr0),
    OFFSET_OF(CONTEXT, Gpr1),
    OFFSET_OF(CONTEXT, Gpr2),
    OFFSET_OF(CONTEXT, Gpr3),
    OFFSET_OF(CONTEXT, Gpr4),
    OFFSET_OF(CONTEXT, Gpr5),
    OFFSET_OF(CONTEXT, Gpr6),
    OFFSET_OF(CONTEXT, Gpr7),
    OFFSET_OF(CONTEXT, Gpr8),
    OFFSET_OF(CONTEXT, Gpr9),
    OFFSET_OF(CONTEXT, Gpr10),
    OFFSET_OF(CONTEXT, Gpr11),
    OFFSET_OF(CONTEXT, Gpr12),
    OFFSET_OF(CONTEXT, Gpr13),
    OFFSET_OF(CONTEXT, Gpr14),
    OFFSET_OF(CONTEXT, Gpr15),
    OFFSET_OF(CONTEXT, Gpr16),
    OFFSET_OF(CONTEXT, Gpr17),
    OFFSET_OF(CONTEXT, Gpr18),
    OFFSET_OF(CONTEXT, Gpr19),
    OFFSET_OF(CONTEXT, Gpr20),
    OFFSET_OF(CONTEXT, Gpr21),
    OFFSET_OF(CONTEXT, Gpr22),
    OFFSET_OF(CONTEXT, Gpr23),
    OFFSET_OF(CONTEXT, Gpr24),
    OFFSET_OF(CONTEXT, Gpr25),
    OFFSET_OF(CONTEXT, Gpr26),
    OFFSET_OF(CONTEXT, Gpr27),
    OFFSET_OF(CONTEXT, Gpr28),
    OFFSET_OF(CONTEXT, Gpr29),
    OFFSET_OF(CONTEXT, Gpr30),
    OFFSET_OF(CONTEXT, Gpr31),
    OFFSET_OF(CONTEXT, Fpr0),
    OFFSET_OF(CONTEXT, Fpr1),
    OFFSET_OF(CONTEXT, Fpr2),
    OFFSET_OF(CONTEXT, Fpr3),
    OFFSET_OF(CONTEXT, Fpr4),
    OFFSET_OF(CONTEXT, Fpr5),
    OFFSET_OF(CONTEXT, Fpr6),
    OFFSET_OF(CONTEXT, Fpr7),
    OFFSET_OF(CONTEXT, Fpr8),
    OFFSET_OF(CONTEXT, Fpr9),
    OFFSET_OF(CONTEXT, Fpr10),
    OFFSET_OF(CONTEXT, Fpr11),
    OFFSET_OF(CONTEXT, Fpr12),
    OFFSET_OF(CONTEXT, Fpr13),
    OFFSET_OF(CONTEXT, Fpr14),
    OFFSET_OF(CONTEXT, Fpr15),
    OFFSET_OF(CONTEXT, Fpr16),
    OFFSET_OF(CONTEXT, Fpr17),
    OFFSET_OF(CONTEXT, Fpr18),
    OFFSET_OF(CONTEXT, Fpr19),
    OFFSET_OF(CONTEXT, Fpr20),
    OFFSET_OF(CONTEXT, Fpr21),
    OFFSET_OF(CONTEXT, Fpr22),
    OFFSET_OF(CONTEXT, Fpr23),
    OFFSET_OF(CONTEXT, Fpr24),
    OFFSET_OF(CONTEXT, Fpr25),
    OFFSET_OF(CONTEXT, Fpr26),
    OFFSET_OF(CONTEXT, Fpr27),
    OFFSET_OF(CONTEXT, Fpr28),
    OFFSET_OF(CONTEXT, Fpr29),
    OFFSET_OF(CONTEXT, Fpr30),
    OFFSET_OF(CONTEXT, Fpr31),

    OFFSET_OF(CONTEXT, Iar),
    OFFSET_OF(CONTEXT, Msr),
    OFFSET_OF(CONTEXT, Cr),
    OFFSET_OF(CONTEXT, Lr),
    OFFSET_OF(CONTEXT, Ctr),
    OFFSET_OF(CONTEXT, Xer),
    /* FIXME: MQ is missing? OFFSET_OF(CONTEXT, Mq), */
    /* see gdb/nlm/ppc.c */
};
# else
#  error "Define the registers map for your CPU"
# endif
#endif
#undef OFFSET_OF

static const size_t cpu_num_regs = (sizeof(cpu_register_map) / sizeof(cpu_register_map[0]));

static inline unsigned long* cpu_register(const CONTEXT* ctx, unsigned idx)
{
    assert(idx < cpu_num_regs);
    return (unsigned long*)((char*)ctx + cpu_register_map[idx]);
}

static inline BOOL     cpu_enter_stepping(struct gdb_context* gdbctx)
{
#ifdef __i386__
    gdbctx->context.EFlags |= 0x100;
    return TRUE;
#elif __powerpc__
#ifndef MSR_SE
# define MSR_SE (1<<10)
#endif 
    gdbctx->context.Msr |= MSR_SE;
    return TRUE;
#else
#error "Define step mode enter for your CPU"
#endif
    return FALSE;
}

static inline BOOL     cpu_leave_stepping(struct gdb_context* gdbctx)
{
#ifdef __i386__
    /* The Win32 debug API always resets the Step bit in EFlags after
     * a single step instruction, so we don't need to clear when the
     * step is done.
     */
    return TRUE;
#elif __powerpc__
    gdbctx->context.Msr &= MSR_SE;
    return TRUE;
#else
#error "Define step mode leave for your CPU"
#endif
    return FALSE;
}

#ifdef __i386__
#define DR7_CONTROL_SHIFT	16
#define DR7_CONTROL_SIZE 	4

#define DR7_RW_EXECUTE 		(0x0)
#define DR7_RW_WRITE		(0x1)
#define DR7_RW_READ		(0x3)

#define DR7_LEN_1		(0x0)
#define DR7_LEN_2		(0x4)
#define DR7_LEN_4		(0xC)

#define DR7_LOCAL_ENABLE_SHIFT	0
#define DR7_GLOBAL_ENABLE_SHIFT 1
#define DR7_ENABLE_SIZE 	2

#define DR7_LOCAL_ENABLE_MASK	(0x55)
#define DR7_GLOBAL_ENABLE_MASK	(0xAA)

#define DR7_CONTROL_RESERVED	(0xFC00)
#define DR7_LOCAL_SLOWDOWN	(0x100)
#define DR7_GLOBAL_SLOWDOWN	(0x200)

#define	DR7_ENABLE_MASK(dr)	(1<<(DR7_LOCAL_ENABLE_SHIFT+DR7_ENABLE_SIZE*(dr)))
#define	IS_DR7_SET(ctrl,dr) 	((ctrl)&DR7_ENABLE_MASK(dr))

static inline int       i386_get_unused_DR(struct gdb_context* gdbctx,
                                           unsigned long** r)
{
    if (!IS_DR7_SET(gdbctx->context.Dr7, 0))
    {
        *r = &gdbctx->context.Dr0;
        return 0;
    }
    if (!IS_DR7_SET(gdbctx->context.Dr7, 1))
    {
        *r = &gdbctx->context.Dr1;
        return 1;
    }
    if (!IS_DR7_SET(gdbctx->context.Dr7, 2))
    {
        *r = &gdbctx->context.Dr2;
        return 2;
    }
    if (!IS_DR7_SET(gdbctx->context.Dr7, 3))
    {
        *r = &gdbctx->context.Dr3;
        return 3;
    }
    return -1;
}
#endif

/******************************************************************
 *		cpu_insert_Xpoint
 *
 * returns  1 if ok
 *          0 if error
 *         -1 if operation isn't supported by CPU
 */
static inline int      cpu_insert_Xpoint(struct gdb_context* gdbctx,
                                         struct gdb_ctx_Xpoint* xpt, size_t len)
{
#ifdef __i386__
    unsigned char       ch;
    unsigned long       sz;
    unsigned long*      pr;
    int                 reg;
    unsigned long       bits;

    switch (xpt->type)
    {
    case '0':
        if (len != 1) return 0;
        if (!ReadProcessMemory(gdbctx->process->handle, xpt->addr, &ch, 1, &sz) || sz != 1) return 0;
        xpt->val = ch;
        ch = 0xcc;
        if (!WriteProcessMemory(gdbctx->process->handle, xpt->addr, &ch, 1, &sz) || sz != 1) return 0;
        break;
    case '1':
        bits = DR7_RW_EXECUTE;
        goto hw_bp;
    case '2':
        bits = DR7_RW_READ;
        goto hw_bp;
    case '3':
        bits = DR7_RW_WRITE;
    hw_bp:
        if ((reg = i386_get_unused_DR(gdbctx, &pr)) == -1) return 0;
        *pr = (unsigned long)xpt->addr;
        if (xpt->type != '1') switch (len)
        {
        case 4: bits |= DR7_LEN_4; break;
        case 2: bits |= DR7_LEN_2; break;
        case 1: bits |= DR7_LEN_1; break;
        default: return 0;
        }
        xpt->val = reg;
        /* clear old values */
        gdbctx->context.Dr7 &= ~(0x0F << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg));
        /* set the correct ones */
        gdbctx->context.Dr7 |= bits << (DR7_CONTROL_SHIFT + DR7_CONTROL_SIZE * reg);
	gdbctx->context.Dr7 |= DR7_ENABLE_MASK(reg) | DR7_LOCAL_SLOWDOWN;
        break;
    default:
        fprintf(stderr, "Unknown bp type %c\n", xpt->type);
        return 0;
    }
    return 1;
#elif defined(__powerpc__)
    unsigned long       xbp;
    unsigned long       sz;

    switch (xpt->type)
    {
    case '0':
        if (len != 4) return 0;
        if (!ReadProcessMemory(gdbctx->process->handle, xpt->addr, &xbp, 4, &sz) || sz != 4) return 0;
        xpt->val = xbp;
        xbp = 0x7d821008; /* 7d 82 10 08 ... in big endian */
        if (!WriteProcessMemory(gdbctx->process->handle, xpt->addr, &xbp, 4, &sz) || sz != 4) return 0;
        break;
    default:
        fprintf(stderr, "Unknown/unsupported bp type %c\n", xpt->type);
        return 0;
    }
    return 1;
#else
#error "Define insert Xpoint for your CPU"
#endif
    return -1;
}

/******************************************************************
 *		cpu_remove_Xpoint
 *
 * returns  1 if ok
 *          0 if error
 *         -1 if operation isn't supported by CPU
 */
static inline BOOL      cpu_remove_Xpoint(struct gdb_context* gdbctx,
                                          struct gdb_ctx_Xpoint* xpt, size_t len)
{
#ifdef __i386__
    unsigned long       sz;
    unsigned char       ch;

    switch (xpt->type)
    {
    case '0':
        if (len != 1) return 0;
        ch = (unsigned char)xpt->val;
        if (!WriteProcessMemory(gdbctx->process->handle, xpt->addr, &ch, 1, &sz) || sz != 1) return 0;
        break;
    case '1':
    case '2':
    case '3':
        /* simply disable the entry */
        gdbctx->context.Dr7 &= ~DR7_ENABLE_MASK(xpt->val);
        break;
    default:
        fprintf(stderr, "Unknown bp type %c\n", xpt->type);
        return 0;
    }
    return 1;
#elif defined(__powerpc__)
    unsigned long       sz;
    unsigned long       xbp;

    switch (xpt->type)
    {
    case '0':
        if (len != 4) return 0;
        xbp = xpt->val;
        if (!WriteProcessMemory(gdbctx->process->handle, xpt->addr, &xbp, 4, &sz) || sz != 4) return 0;
        break;
    case '1':
    case '2':
    case '3':
    default:
        fprintf(stderr, "Unknown/unsupported bp type %c\n", xpt->type);
        return 0;
    }
    return 1;
#else
#error "Define remove Xpoint for your CPU"
#endif
    return -1;
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
            fprintf(stderr, "Unhandled exception code %08lx\n", rec->ExceptionCode);
        gdbctx->last_sig = SIGABRT;
        ret = TRUE;
        break;
    }
    return ret;
}

static	void	handle_debug_event(struct gdb_context* gdbctx, DEBUG_EVENT* de)
{
    char                buffer[256];

    DEBUG_CurrThread = DEBUG_GetThread(gdbctx->process, de->dwThreadId);

    switch (de->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer),
                                       de->u.CreateProcessInfo.hProcess,
                                       de->u.CreateProcessInfo.lpImageName,
                                       de->u.CreateProcessInfo.fUnicode);

        /* FIXME unicode ? de->u.CreateProcessInfo.fUnicode */
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: create process '%s'/%p @%08lx (%ld<%ld>)\n",
                    de->dwProcessId, de->dwThreadId,
                    buffer, de->u.CreateProcessInfo.lpImageName,
                    (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress,
                    de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                    de->u.CreateProcessInfo.nDebugInfoSize);

        gdbctx->process = DEBUG_AddProcess(de->dwProcessId,
                                           de->u.CreateProcessInfo.hProcess,
                                           buffer);
        /* de->u.CreateProcessInfo.lpStartAddress; */

        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: create thread I @%08lx\n",
                    de->dwProcessId, de->dwThreadId,
                    (unsigned long)(LPVOID)de->u.CreateProcessInfo.lpStartAddress);

        assert(DEBUG_CurrThread == NULL); /* shouldn't be there */
        DEBUG_AddThread(gdbctx->process, de->dwThreadId,
                        de->u.CreateProcessInfo.hThread,
                        de->u.CreateProcessInfo.lpStartAddress,
                        de->u.CreateProcessInfo.lpThreadLocalBase);
#if 0
        DEBUG_LoadModule32(DEBUG_CurrProcess->imageName, de->u.CreateProcessInfo.hFile,
                           de->u.CreateProcessInfo.lpBaseOfImage);

        if (buffer[0])  /* we got a process name */
        {
            DWORD type;
            if (!GetBinaryTypeA( buffer, &type ))
            {
                /* not a Windows binary, assume it's a Unix executable then */
                char unixname[MAX_PATH];
                /* HACK!! should fix DEBUG_ReadExecutableDbgInfo to accept DOS filenames */
                if (wine_get_unix_file_name( buffer, unixname, sizeof(unixname) ))
                {
                    DEBUG_ReadExecutableDbgInfo( unixname );
                    break;
                }
            }
        }
        /* if it is a Windows binary, or an invalid or missing file name,
         * we use wine itself as the main executable */
        DEBUG_ReadExecutableDbgInfo( "wine" );
#endif
        break;

    case LOAD_DLL_DEBUG_EVENT:
        assert(DEBUG_CurrThread);
        DEBUG_ProcessGetStringIndirect(buffer, sizeof(buffer),
                                       gdbctx->process->handle,
                                       de->u.LoadDll.lpImageName,
                                       de->u.LoadDll.fUnicode);

        /* FIXME unicode: de->u.LoadDll.fUnicode */
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: loads DLL %s @%08lx (%ld<%ld>)\n",
                    de->dwProcessId, de->dwThreadId,
                    buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll,
                    de->u.LoadDll.dwDebugInfoFileOffset,
                    de->u.LoadDll.nDebugInfoSize);
#if 0
        _strupr(buffer);
        DEBUG_LoadModule32(buffer, de->u.LoadDll.hFile, de->u.LoadDll.lpBaseOfDll);
        DEBUG_CheckDelayedBP();
        if (DBG_IVAR(BreakOnDllLoad))
        {
            DEBUG_Printf("Stopping on DLL %s loading at %08lx\n",
                         buffer, (unsigned long)de->u.LoadDll.lpBaseOfDll);
            DEBUG_Parser();
        }
#endif
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: unload DLL @%08lx\n",
                    de->dwProcessId, de->dwThreadId, (unsigned long)de->u.UnloadDll.lpBaseOfDll);
        break;

    case EXCEPTION_DEBUG_EVENT:
        assert(DEBUG_CurrThread);
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: exception code=%08lx\n",
                    de->dwProcessId, de->dwThreadId,
                    de->u.Exception.ExceptionRecord.ExceptionCode);

        if (fetch_context(gdbctx, DEBUG_CurrThread->handle, &gdbctx->context))
        {
            gdbctx->in_trap = handle_exception(gdbctx, &de->u.Exception);
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: create thread D @%08lx\n",
                    de->dwProcessId, de->dwThreadId, (unsigned long)(LPVOID)de->u.CreateThread.lpStartAddress);

        DEBUG_AddThread(gdbctx->process,
                        de->dwThreadId,
                        de->u.CreateThread.hThread,
                        de->u.CreateThread.lpStartAddress,
                        de->u.CreateThread.lpThreadLocalBase);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: exit thread (%ld)\n",
                    de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);

        assert(DEBUG_CurrThread);
        if (DEBUG_CurrThread == gdbctx->exec_thread) gdbctx->exec_thread = NULL;
        if (DEBUG_CurrThread == gdbctx->other_thread) gdbctx->other_thread = NULL;
        DEBUG_DelThread(DEBUG_CurrThread);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: exit process (%ld)\n",
                    de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);

        DEBUG_DelProcess(gdbctx->process);
        gdbctx->process = NULL;
        /* now signal gdb that we're done */
        gdbctx->last_sig = SIGTERM;
        gdbctx->in_trap = TRUE;
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        assert(DEBUG_CurrThread);
        DEBUG_ProcessGetString(buffer, sizeof(buffer),
                               gdbctx->process->handle,
                               de->u.DebugString.lpDebugStringData);
        /* FIXME unicode de->u.DebugString.fUnicode ? */
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: output debug string (%s)\n",
                    de->dwProcessId, de->dwThreadId, buffer);
        break;

    case RIP_EVENT:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: rip error=%ld type=%ld\n",
                    de->dwProcessId, de->dwThreadId, de->u.RipInfo.dwError,
                    de->u.RipInfo.dwType);
        break;

    default:
        if (gdbctx->trace & GDBPXY_TRC_WIN32_EVENT)
            fprintf(stderr, "%08lx:%08lx: unknown event (%ld)\n",
                    de->dwProcessId, de->dwThreadId, de->dwDebugEventCode);
    }
}

static void    resume_debuggee(struct gdb_context* gdbctx, unsigned long cont)
{
    if (DEBUG_CurrThread)
    {
        if (!SetThreadContext(DEBUG_CurrThread->handle, &gdbctx->context))
            if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                fprintf(stderr, "Cannot set context on thread %lu\n", DEBUG_CurrThread->tid);
        if (!ContinueDebugEvent(gdbctx->process->pid, DEBUG_CurrThread->tid, cont))
            if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
                fprintf(stderr, "Cannot continue on %lu (%lu)\n",
                        DEBUG_CurrThread->tid, cont);
    }
    else if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
        fprintf(stderr, "Cannot find last thread (%lu)\n", DEBUG_CurrThread->tid);
}

static void    wait_for_debuggee(struct gdb_context* gdbctx)
{
    DEBUG_EVENT         de;

    gdbctx->in_trap = FALSE;
    while (WaitForDebugEvent(&de, INFINITE))
    {
        handle_debug_event(gdbctx, &de);
        assert(!gdbctx->process ||
               gdbctx->process->pid == 0 ||
               de.dwProcessId == gdbctx->process->pid);
        assert(!DEBUG_CurrThread || de.dwThreadId == DEBUG_CurrThread->tid);
        if (gdbctx->in_trap) break;
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
    }
}

static void detach_debuggee(struct gdb_context* gdbctx, BOOL kill)
{
    cpu_leave_stepping(gdbctx);
    resume_debuggee(gdbctx, DBG_CONTINUE);
    if (!kill)
        DebugActiveProcessStop(gdbctx->process->pid);
    DEBUG_DelProcess(gdbctx->process);
    gdbctx->process = NULL;
}

static void get_process_info(struct gdb_context* gdbctx, char* buffer, size_t len)
{
    unsigned long       status;

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
        snprintf(buffer, len, "Terminated (%lu)", status);

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
    DBG_THREAD*         thd;
    unsigned long       status;
    int                 prio;

    /* FIXME: use the size of buffer */
    thd = DEBUG_GetThread(gdbctx->process, tid);
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
            default: snprintf(buffer, len, "Suspended (%lu)", status - 1);
            }
            ResumeThread(thd->handle);
        }
        else
            snprintf(buffer, len, "Terminated (exit code = %lu)", status);
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
        packet_reply_val(gdbctx, DEBUG_CurrThread->tid, 4);
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
    if (DEBUG_CurrThread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: cont on %lu, while last thread is %lu\n",
                    gdbctx->exec_thread->tid, DEBUG_CurrThread->tid);
    resume_debuggee(gdbctx, DBG_CONTINUE);
    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_continue_signal(struct gdb_context* gdbctx)
{
    unsigned char sig;

    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 2);
    if (DEBUG_CurrThread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: cont/sig on %lu, while last thread is %lu\n",
                    gdbctx->exec_thread->tid, DEBUG_CurrThread->tid);
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

    if (DEBUG_CurrThread != gdbctx->other_thread && gdbctx->other_thread)
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
    if (DEBUG_CurrThread != gdbctx->other_thread && gdbctx->other_thread)
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
            fprintf(stderr, "Cannot set context on thread %lu\n", gdbctx->other_thread->tid);
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
            gdbctx->exec_thread = DEBUG_GetThread(gdbctx->process, thread);
        else
            gdbctx->other_thread = DEBUG_GetThread(gdbctx->process, thread);
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
    size_t              len, blk_len, nread;
    char                buffer[32];
    unsigned long       r = 0;

    assert(gdbctx->in_trap);
    /* FIXME:check in_packet_len for reading %p,%x */
    if (sscanf(gdbctx->in_packet, "%p,%x", &addr, &len) != 2) return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Read mem at %p for %u bytes\n", addr, len);
    for (nread = 0; nread < len > 0; nread += r, addr += r)
    {
        blk_len = min(sizeof(buffer), len - nread);
        if (!ReadProcessMemory(gdbctx->process->handle, addr, buffer, blk_len, &r) ||
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
    size_t              len, blk_len;
    char*               ptr;
    char                buffer[32];
    unsigned long       w;

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
        {
            BOOL ret;

            ret = WriteProcessMemory(gdbctx->process->handle, addr, buffer, blk_len, &w);
            if (!ret || w != blk_len)
                break;
        }
        addr += w;
        len -= w;
        ptr += w;
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

    if (DEBUG_CurrThread != gdbctx->other_thread && gdbctx->other_thread)
    {
        if (!fetch_context(gdbctx, gdbctx->other_thread->handle, pctx = &ctx))
            return packet_error;
    }

    hex_from(cpu_register(pctx, reg), ptr, 4);
    if (pctx != &gdbctx->context && !SetThreadContext(gdbctx->other_thread->handle, pctx))
    {
        if (gdbctx->trace & GDBPXY_TRC_WIN32_ERROR)
            fprintf(stderr, "Cannot set context for thread %lu\n", gdbctx->other_thread->tid);
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
                "%*s%04x%*s%-17.17s %08lx %08lx %.14s\n",
                indent, "", (UINT)hWnd, 13 - indent, "",
                clsName, GetWindowLong(hWnd, GWL_STYLE),
                GetWindowLong(hWnd, GWL_WNDPROC), wndName);
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
    ok = Process32First( snap, &entry );

    /* we do the output in several 'O' packets, with the last one being just OK for
     * marking the end of the output */

    packet_reply_open(gdbctx);
    packet_reply_catc(gdbctx, 'O');
    snprintf(buffer, sizeof(buffer),
             " %-8.8s %-8.8s %-8.8s %s\n",
             "pid", "threads", "parent", "executable" );
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);

    while (ok)
    {
        deco = ' ';
        if (entry.th32ProcessID == gdbctx->process->pid) deco = '>';
        packet_reply_open(gdbctx);
        packet_reply_catc(gdbctx, 'O');
        snprintf(buffer, sizeof(buffer),
                 "%c%08lx %-8ld %08lx '%s'\n",
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
        snprintf(buffer, sizeof(buffer), 
                 "%08lx %08lx %s %s %s\n",
                 (DWORD)addr, mbi.RegionSize, state, type, prot);
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

#ifdef __i386__
static void packet_query_monitor_linear(struct gdb_context* gdbctx,
                                       int len, const char* str)
{
    unsigned    seg, ofs;
    LDT_ENTRY	le;
    unsigned    linear;
    char        buffer[32];

    while (len > 0 && (*str == ' ' || *str == '\t'))
    {
        str++; len--;
    }
    /* FIXME: do a better scanning (allow both decimal and hex numbers) */
    if (!len || sscanf(str, "%x:%x", &seg, &ofs) != 2)
    {
        packet_reply_error(gdbctx, 0);
        return;
    }

    /* V86 mode ? */
    if (gdbctx->context.EFlags & 0x00020000) linear = (LOWORD(seg) << 4) + ofs;
    /* linux system selector ? */
    else if (!(seg & 4) || ((seg >> 3) < 17)) linear = ofs;
    /* standard selector */
    else if (GetThreadSelectorEntry(gdbctx->process->threads->handle, seg, &le))
        linear = (le.HighWord.Bits.BaseHi << 24) + (le.HighWord.Bits.BaseMid << 16) +
            le.BaseLow + ofs;
    /* error */
    else linear = 0;
    snprintf(buffer, sizeof(buffer), "0x%x", linear);
    packet_reply_open(gdbctx);
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);
}
#endif

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
#ifdef __i386__
    {1, "linear",  6, packet_query_monitor_linear},
#endif
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
            DBG_THREAD* thd;

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
            DBG_THREAD* thd;
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
    if (DEBUG_CurrThread != gdbctx->exec_thread && gdbctx->exec_thread)
        if (gdbctx->trace & GDBPXY_TRC_COMMAND_FIXME)
            fprintf(stderr, "NIY: step on %lu, while last thread is %lu\n",
                    gdbctx->exec_thread->tid, DEBUG_CurrThread->tid);
    if (!cpu_enter_stepping(gdbctx)) return packet_error;
    resume_debuggee(gdbctx, DBG_CONTINUE);
    wait_for_debuggee(gdbctx);
    if (!cpu_leave_stepping(gdbctx)) return packet_error;
    return packet_reply_status(gdbctx);
}

#if 0
static enum packet_return packet_step_signal(struct gdb_context* gdbctx)
{
    unsigned char sig;

    /* FIXME: add support for address in packet */
    assert(gdbctx->in_packet_len == 2);
    if (DEBUG_CurrThread->tid != gdbctx->exec_thread && gdbctx->exec_thread)
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
    if (DEBUG_GetThread(gdbctx->process, tid) != NULL)
        return packet_ok;
    return packet_reply_error(gdbctx, ESRCH);
}

static enum packet_return packet_remove_breakpoint(struct gdb_context* gdbctx)
{
    void*                       addr;
    unsigned                    len;
    struct gdb_ctx_Xpoint*      xpt;

    /* FIXME: check packet_len */
    if (gdbctx->in_packet[0] < '0' || gdbctx->in_packet[0] > '4' ||
        gdbctx->in_packet[1] != ',' ||
        sscanf(gdbctx->in_packet + 2, "%p,%x", &addr, &len) != 2)
        return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Remove bp %p[%u] typ=%c\n",
                addr, len, gdbctx->in_packet[0]);
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->addr == addr && xpt->type == gdbctx->in_packet[0])
        {
            switch (cpu_remove_Xpoint(gdbctx, xpt, len))
            {
            case  1:    xpt->type = -1; return packet_ok;
            case  0:                    return packet_error;
            case -1:                    return packet_done;
            default:                    assert(0);
            }
        }
    }
    return packet_error;
}

static enum packet_return packet_set_breakpoint(struct gdb_context* gdbctx)
{
    void*                       addr;
    unsigned                    len;
    struct gdb_ctx_Xpoint*      xpt;

    /* FIXME: check packet_len */
    if (gdbctx->in_packet[0] < '0' || gdbctx->in_packet[0] > '4' ||
        gdbctx->in_packet[1] != ',' ||
        sscanf(gdbctx->in_packet + 2, "%p,%x", &addr, &len) != 2)
        return packet_error;
    if (gdbctx->trace & GDBPXY_TRC_COMMAND)
        fprintf(stderr, "Set bp %p[%u] typ=%c\n",
                addr, len, gdbctx->in_packet[0]);
    /* because of packet command handling, this should be made idempotent */
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->addr == addr && xpt->type == gdbctx->in_packet[0])
            return packet_ok; /* nothing to do */
    }
    /* really set the Xpoint */
    for (xpt = &gdbctx->Xpoints[NUM_XPOINT - 1]; xpt >= gdbctx->Xpoints; xpt--)
    {
        if (xpt->type == -1)
        {
            xpt->addr = addr;
            xpt->type = gdbctx->in_packet[0];
            switch (cpu_insert_Xpoint(gdbctx, xpt, len))
            {
            case  1:    return packet_ok;
            case  0:    return packet_error;
            case -1:    return packet_done;
            default: assert(0);
            }
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
/*        {'!', packet_extended}, */
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
        {'s', packet_step},
        /*{'S', packet_step_signal}, hard(er) to implement */
        {'T', packet_thread_alive},
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

static BOOL gdb_startup(struct gdb_context* gdbctx, DEBUG_EVENT* de, unsigned flags)
{
    int                 sock;
    struct sockaddr_in  s_addrs;
    int                 s_len = sizeof(s_addrs);
    struct pollfd       pollfd;
    char                wine_path[MAX_PATH];
    char*               ptr;

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

    /* step 2: find out wine executable location (as a Unix filename) */
    ptr = getenv("WINELOADER");
    strcpy(wine_path, ptr ? ptr : "wine");

    fprintf(stderr, "Using wine_path: %s\n", wine_path);
    read_elf_info(wine_path, gdbctx->wine_segs);

    /* step 3: fire up gdb (if requested) */
    if (flags & 1)
        fprintf(stderr, "target remote localhost:%d\n", ntohs(s_addrs.sin_port));
    else
        switch (fork())
        {
        case -1: /* error in parent... */
            fprintf(stderr, "Cannot create gdb\n");
            return FALSE;
            break;
        default: /* in parent... success */
            break;
        case 0: /* in child... and alive */
            {
                char            buf[MAX_PATH];
                int             fd;
                const char*     gdb_path;
                FILE*           f;

                if (!(gdb_path = getenv("WINE_GDB"))) gdb_path = "gdb";
		strcpy(buf,"/tmp/winegdb.XXXXXX");
		fd = mkstemps(buf,0);
		if (fd == -1) return FALSE;
                if ((f = fdopen(fd, "w+")) == NULL) return FALSE;
                fprintf(f, "file %s\n", wine_path);
                fprintf(f, "target remote localhost:%d\n", ntohs(s_addrs.sin_port));
                fprintf(f, "monitor trace=%d\n", GDBPXY_TRC_COMMAND_FIXME);
                fprintf(f, "set prompt Wine-gdb>\\ \n");
                /* gdb 5.1 seems to require it, won't hurt anyway */
                fprintf(f, "sharedlibrary\n");
                /* tell gdb to delete this file when done handling it... */
                fprintf(f, "shell rm -f \"%s\"\n", buf);
                fclose(f);
                if (flags & 2)
                    execlp("xterm", "xterm", "-e", gdb_path, "-x", buf, NULL);
                else
                    execlp(gdb_path, gdb_path, "-x", buf, NULL);
                assert(0); /* never reached */
                break;
            }
            break;
        }

    /* step 4: do the process internal creation */
    handle_debug_event(gdbctx, de);

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
            assert(gdbctx->process == NULL && de.dwProcessId == DEBUG_CurrPid);
            /*gdbctx->dwProcessId = pid; */
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

BOOL DEBUG_GdbRemote(unsigned flags)
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
