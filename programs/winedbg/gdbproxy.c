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
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

struct gdb_xpoint
{
    struct list entry;
    int pid;
    int tid;
    enum be_xpoint_type type;
    void *addr;
    int size;
    unsigned long value;
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
    int                         exec_tid; /* tid used in step & continue */
    int                         other_tid; /* tid to be used in any other operation */
    struct list                 xpoint_list;
    /* current Win32 trap env */
    DEBUG_EVENT                 de;
    DWORD                       de_reply;
    /* Win32 information */
    struct dbg_process*         process;
    /* Unix environment */
    unsigned long               wine_segs[3];   /* load addresses of the ELF wine exec segments (text, bss and data) */
    BOOL                        no_ack_mode;
};

static void gdbctx_delete_xpoint(struct gdb_context *gdbctx, struct dbg_thread *thread,
                                 dbg_ctx_t *ctx, struct gdb_xpoint *x)
{
    struct dbg_process *process = gdbctx->process;
    struct backend_cpu *cpu = process->be_cpu;

    if (!cpu->remove_Xpoint(process->handle, process->process_io, ctx, x->type, x->addr, x->value, x->size))
        ERR("%04x:%04x: Couldn't remove breakpoint at:%p/%x type:%d\n", process->pid, thread ? thread->tid : ~0, x->addr, x->size, x->type);

    list_remove(&x->entry);
    HeapFree(GetProcessHeap(), 0, x);
}

static void gdbctx_insert_xpoint(struct gdb_context *gdbctx, struct dbg_thread *thread,
                                 dbg_ctx_t *ctx, enum be_xpoint_type type, void *addr, int size)
{
    struct dbg_process *process = thread->process;
    struct backend_cpu *cpu = process->be_cpu;
    struct gdb_xpoint *x;
    unsigned long value;

    if (!cpu->insert_Xpoint(process->handle, process->process_io, ctx, type, addr, &value, size))
    {
        ERR("%04x:%04x: Couldn't insert breakpoint at:%p/%x type:%d\n", process->pid, thread->tid, addr, size, type);
        return;
    }

    if (!(x = HeapAlloc(GetProcessHeap(), 0, sizeof(struct gdb_xpoint))))
    {
        ERR("%04x:%04x: Couldn't allocate memory for breakpoint at:%p/%x type:%d\n", process->pid, thread->tid, addr, size, type);
        return;
    }

    x->pid = process->pid;
    x->tid = thread->tid;
    x->type = type;
    x->addr = addr;
    x->size = size;
    x->value = value;
    list_add_head(&gdbctx->xpoint_list, &x->entry);
}

static struct gdb_xpoint *gdb_find_xpoint(struct gdb_context *gdbctx, struct dbg_thread *thread,
                                          enum be_xpoint_type type, void *addr, int size)
{
    struct gdb_xpoint *x;

    LIST_FOR_EACH_ENTRY(x, &gdbctx->xpoint_list, struct gdb_xpoint, entry)
    {
        if (thread && (x->pid != thread->process->pid || x->tid != thread->tid))
            continue;
        if (x->type == type && x->addr == addr && x->size == size)
            return x;
    }

    return NULL;
}

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

static inline void* cpu_register_ptr(struct gdb_context *gdbctx,
    dbg_ctx_t *ctx, unsigned idx)
{
    assert(idx < gdbctx->process->be_cpu->gdb_num_regs);
    return (char*)ctx + gdbctx->process->be_cpu->gdb_register_map[idx].offset;
}

static inline DWORD64 cpu_register(struct gdb_context *gdbctx,
    dbg_ctx_t *ctx, unsigned idx)
{
    switch (gdbctx->process->be_cpu->gdb_register_map[idx].length)
    {
    case 1: return *(BYTE*)cpu_register_ptr(gdbctx, ctx, idx);
    case 2: return *(WORD*)cpu_register_ptr(gdbctx, ctx, idx);
    case 4: return *(DWORD*)cpu_register_ptr(gdbctx, ctx, idx);
    case 8: return *(DWORD64*)cpu_register_ptr(gdbctx, ctx, idx);
    default:
        ERR("got unexpected size: %u\n",
            (unsigned)gdbctx->process->be_cpu->gdb_register_map[idx].length);
        assert(0);
        return 0;
    }
}

static inline void cpu_register_hex_from(struct gdb_context *gdbctx,
    dbg_ctx_t* ctx, unsigned idx, const char **phex)
{
    const struct gdb_register *cpu_register_map = gdbctx->process->be_cpu->gdb_register_map;
    hex_from(cpu_register_ptr(gdbctx, ctx, idx), *phex, cpu_register_map[idx].length);
}

/* =============================================== *
 *    W I N 3 2   D E B U G   I N T E R F A C E    *
 * =============================================== *
 */

static struct dbg_thread* dbg_thread_from_tid(struct gdb_context* gdbctx, int tid)
{
    struct dbg_process *process = gdbctx->process;
    struct dbg_thread *thread;

    if (!process) return NULL;

    if (tid == 0) tid = gdbctx->de.dwThreadId;
    LIST_FOR_EACH_ENTRY(thread, &process->threads, struct dbg_thread, entry)
    {
        if (tid > 0 && thread->tid != tid) continue;
        return thread;
    }

    return NULL;
}

static void dbg_thread_set_single_step(struct dbg_thread *thread, BOOL enable)
{
    struct backend_cpu *backend;
    dbg_ctx_t ctx;

    if (!thread) return;
    if (!thread->process) return;
    if (!(backend = thread->process->be_cpu)) return;

    if (!backend->get_context(thread->handle, &ctx))
    {
        ERR("get_context failed for thread %04x:%04x\n", thread->process->pid, thread->tid);
        return;
    }
    backend->single_step(&ctx, enable);
    if (!backend->set_context(thread->handle, &ctx))
        ERR("set_context failed for thread %04x:%04x\n", thread->process->pid, thread->tid);
}

static unsigned char signal_from_debug_event(DEBUG_EVENT* de)
{
    DWORD ec;

    if (de->dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        return SIGTERM;
    if (de->dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
        return SIGTRAP;

    ec = de->u.Exception.ExceptionRecord.ExceptionCode;
    switch (ec)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_GUARD_PAGE:
        return SIGSEGV;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return SIGBUS;
    case EXCEPTION_SINGLE_STEP:
    case EXCEPTION_BREAKPOINT:
        return SIGTRAP;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        return SIGFPE;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
        return SIGFPE;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return SIGILL;
    case CONTROL_C_EXIT:
        return SIGINT;
    case STATUS_POSSIBLE_DEADLOCK:
        return SIGALRM;
    /* should not be here */
    case EXCEPTION_INVALID_HANDLE:
    case EXCEPTION_WINE_NAME_THREAD:
        return SIGTRAP;
    default:
        ERR("Unknown exception code 0x%08x\n", ec);
        return SIGABRT;
    }
}

static BOOL handle_exception(struct gdb_context* gdbctx, EXCEPTION_DEBUG_INFO* exc)
{
    EXCEPTION_RECORD* rec = &exc->ExceptionRecord;

    switch (rec->ExceptionCode)
    {
    case EXCEPTION_WINE_NAME_THREAD:
    {
        const THREADNAME_INFO *threadname = (const THREADNAME_INFO *)rec->ExceptionInformation;
        struct dbg_thread *thread;
        char name[9];
        SIZE_T read;

        if (threadname->dwThreadID == -1)
            thread = dbg_get_thread(gdbctx->process, gdbctx->de.dwThreadId);
        else
            thread = dbg_get_thread(gdbctx->process, threadname->dwThreadID);
        if (thread)
        {
            if (gdbctx->process->process_io->read( gdbctx->process->handle,
                threadname->szName, name, sizeof(name), &read) && read == sizeof(name))
            {
                fprintf(stderr, "Thread ID=%04x renamed to \"%.9s\"\n",
                    threadname->dwThreadID, name);
            }
        }
        else
            ERR("Cannot set name of thread %04x\n", threadname->dwThreadID);
        return TRUE;
    }
    case EXCEPTION_INVALID_HANDLE:
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOL handle_debug_event(struct gdb_context* gdbctx, BOOL stop_on_dll_load_unload)
{
    DEBUG_EVENT *de = &gdbctx->de;
    struct dbg_thread *thread;

    union {
        char                bufferA[256];
        WCHAR               buffer[256];
    } u;
    DWORD size;

    gdbctx->exec_tid = de->dwThreadId;
    gdbctx->other_tid = de->dwThreadId;
    gdbctx->de_reply = DBG_REPLY_LATER;

    switch (de->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        gdbctx->process = dbg_add_process(&be_process_gdbproxy_io, de->dwProcessId,
                                          de->u.CreateProcessInfo.hProcess);
        if (!gdbctx->process)
            return TRUE;

        size = ARRAY_SIZE(u.buffer);
        QueryFullProcessImageNameW( gdbctx->process->handle, 0, u.buffer, &size );
        dbg_set_process_name(gdbctx->process, u.buffer);

        fprintf(stderr, "%04x:%04x: create process '%s'/%p @%p (%u<%u>)\n",
                    de->dwProcessId, de->dwThreadId,
                    dbg_W2A(u.buffer, -1),
                    de->u.CreateProcessInfo.lpImageName,
                    de->u.CreateProcessInfo.lpStartAddress,
                    de->u.CreateProcessInfo.dwDebugInfoFileOffset,
                    de->u.CreateProcessInfo.nDebugInfoSize);

        /* de->u.CreateProcessInfo.lpStartAddress; */
        if (!dbg_init(gdbctx->process->handle, u.buffer, TRUE))
            ERR("Couldn't initiate DbgHelp\n");

        fprintf(stderr, "%04x:%04x: create thread I @%p\n", de->dwProcessId,
            de->dwThreadId, de->u.CreateProcessInfo.lpStartAddress);

        dbg_load_module(gdbctx->process->handle, de->u.CreateProcessInfo.hFile, u.buffer,
                        (DWORD_PTR)de->u.CreateProcessInfo.lpBaseOfImage, 0);

        dbg_add_thread(gdbctx->process, de->dwThreadId,
                       de->u.CreateProcessInfo.hThread,
                       de->u.CreateProcessInfo.lpThreadLocalBase);
        return TRUE;

    case LOAD_DLL_DEBUG_EVENT:
        fetch_module_name( de->u.LoadDll.lpImageName, de->u.LoadDll.lpBaseOfDll,
                           u.buffer, ARRAY_SIZE(u.buffer) );
        fprintf(stderr, "%04x:%04x: loads DLL %s @%p (%u<%u>)\n",
                de->dwProcessId, de->dwThreadId,
                dbg_W2A(u.buffer, -1),
                de->u.LoadDll.lpBaseOfDll,
                de->u.LoadDll.dwDebugInfoFileOffset,
                de->u.LoadDll.nDebugInfoSize);
        dbg_load_module(gdbctx->process->handle, de->u.LoadDll.hFile, u.buffer,
                        (DWORD_PTR)de->u.LoadDll.lpBaseOfDll, 0);
        if (stop_on_dll_load_unload)
            break;
        return TRUE;

    case UNLOAD_DLL_DEBUG_EVENT:
        fprintf(stderr, "%08x:%08x: unload DLL @%p\n",
                de->dwProcessId, de->dwThreadId, de->u.UnloadDll.lpBaseOfDll);
        SymUnloadModule(gdbctx->process->handle,
                        (DWORD_PTR)de->u.UnloadDll.lpBaseOfDll);
        if (stop_on_dll_load_unload)
            break;
        return TRUE;

    case EXCEPTION_DEBUG_EVENT:
        TRACE("%08x:%08x: exception code=0x%08x\n", de->dwProcessId,
            de->dwThreadId, de->u.Exception.ExceptionRecord.ExceptionCode);

        if (handle_exception(gdbctx, &de->u.Exception))
            return TRUE;
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        fprintf(stderr, "%08x:%08x: create thread D @%p\n", de->dwProcessId,
            de->dwThreadId, de->u.CreateThread.lpStartAddress);

        dbg_add_thread(gdbctx->process,
                       de->dwThreadId,
                       de->u.CreateThread.hThread,
                       de->u.CreateThread.lpThreadLocalBase);
        return TRUE;

    case EXIT_THREAD_DEBUG_EVENT:
        fprintf(stderr, "%08x:%08x: exit thread (%u)\n",
                de->dwProcessId, de->dwThreadId, de->u.ExitThread.dwExitCode);
        if ((thread = dbg_get_thread(gdbctx->process, de->dwThreadId)))
            dbg_del_thread(thread);
        return TRUE;

    case EXIT_PROCESS_DEBUG_EVENT:
        fprintf(stderr, "%08x:%08x: exit process (%u)\n",
                de->dwProcessId, de->dwThreadId, de->u.ExitProcess.dwExitCode);

        dbg_del_process(gdbctx->process);
        gdbctx->process = NULL;
        return FALSE;

    case OUTPUT_DEBUG_STRING_EVENT:
        memory_get_string(gdbctx->process,
                          de->u.DebugString.lpDebugStringData, TRUE,
                          de->u.DebugString.fUnicode, u.bufferA, sizeof(u.bufferA));
        fprintf(stderr, "%08x:%08x: output debug string (%s)\n",
            de->dwProcessId, de->dwThreadId, debugstr_a(u.bufferA));
        return TRUE;

    case RIP_EVENT:
        fprintf(stderr, "%08x:%08x: rip error=%u type=%u\n", de->dwProcessId,
            de->dwThreadId, de->u.RipInfo.dwError, de->u.RipInfo.dwType);
        return TRUE;

    default:
        FIXME("%08x:%08x: unknown event (%u)\n",
            de->dwProcessId, de->dwThreadId, de->dwDebugEventCode);
    }

    LIST_FOR_EACH_ENTRY(thread, &gdbctx->process->threads, struct dbg_thread, entry)
    {
        if (!thread->suspended) SuspendThread(thread->handle);
        thread->suspended = TRUE;
    }

    return FALSE;
}

static void handle_step_or_continue(struct gdb_context* gdbctx, int tid, BOOL step, int sig)
{
    struct dbg_process *process = gdbctx->process;
    struct dbg_thread *thread;

    if (tid == 0) tid = gdbctx->de.dwThreadId;
    LIST_FOR_EACH_ENTRY(thread, &process->threads, struct dbg_thread, entry)
    {
        if (tid != -1 && thread->tid != tid) continue;
        if (!thread->suspended) continue;
        thread->suspended = FALSE;

        if (process->pid == gdbctx->de.dwProcessId && thread->tid == gdbctx->de.dwThreadId)
            gdbctx->de_reply = (sig == -1 ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED);

        dbg_thread_set_single_step(thread, step);
        ResumeThread(thread->handle);
    }
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
			ERR("read failed\n");
			return FALSE;
		}
		if (pkt != '\003') {
			ERR("Unexpected break packet %#02x\n", pkt);
			return FALSE;
		}
		return TRUE;
	} else if (ret == -1) {
		ERR("poll failed\n");
	}
	return FALSE;
}

static void    wait_for_debuggee(struct gdb_context* gdbctx)
{
    if (gdbctx->de.dwDebugEventCode)
        ContinueDebugEvent(gdbctx->de.dwProcessId, gdbctx->de.dwThreadId, gdbctx->de_reply);

    for (;;)
    {
		if (!WaitForDebugEvent(&gdbctx->de, 10))
		{
			if (GetLastError() == ERROR_SEM_TIMEOUT)
			{
				if (check_for_interrupt(gdbctx)) {
					if (!DebugBreakProcess(gdbctx->process->handle)) {
						ERR("Failed to break into debuggee\n");
						break;
					}
					WaitForDebugEvent(&gdbctx->de, INFINITE);
				} else {
					continue;
				} 
			} else {
				break;
			} 
		}
        if (!handle_debug_event(gdbctx, TRUE))
            break;
        ContinueDebugEvent(gdbctx->de.dwProcessId, gdbctx->de.dwThreadId, DBG_CONTINUE);
    }
}

static void detach_debuggee(struct gdb_context* gdbctx, BOOL kill)
{
    handle_step_or_continue(gdbctx, -1, FALSE, -1);

    if (gdbctx->de.dwDebugEventCode)
        ContinueDebugEvent(gdbctx->de.dwProcessId, gdbctx->de.dwThreadId, DBG_CONTINUE);

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
    case BELOW_NORMAL_PRIORITY_CLASS:   strcat(buffer, ", below normal priority");      break;
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

static int addr_width(struct gdb_context* gdbctx)
{
    int sz = (gdbctx && gdbctx->process && gdbctx->process->be_cpu) ?
        gdbctx->process->be_cpu->pointer_size : (int)sizeof(void*);
    return sz * 2;
}

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

static inline void packet_reply_add(struct gdb_context* gdbctx, const char* str)
{
    int len = strlen(str);
    packet_reply_grow(gdbctx, len);
    memcpy(&gdbctx->out_buf[gdbctx->out_len], str, len);
    gdbctx->out_len += len;
}

static void packet_reply_open(struct gdb_context* gdbctx)
{
    assert(gdbctx->out_curr_packet == -1);
    packet_reply_add(gdbctx, "$");
    gdbctx->out_curr_packet = gdbctx->out_len;
}

static void packet_reply_close(struct gdb_context* gdbctx)
{
    unsigned char       cksum;
    int plen;

    plen = gdbctx->out_len - gdbctx->out_curr_packet;
    packet_reply_add(gdbctx, "#");
    cksum = checksum(&gdbctx->out_buf[gdbctx->out_curr_packet], plen);
    packet_reply_hex_to(gdbctx, &cksum, 1);
    gdbctx->out_curr_packet = -1;
}

static void packet_reply_open_xfer(struct gdb_context* gdbctx)
{
    packet_reply_open(gdbctx);
    packet_reply_add(gdbctx, "m");
}

static void packet_reply_close_xfer(struct gdb_context* gdbctx, int off, int len)
{
    int begin = gdbctx->out_curr_packet + 1;
    int plen;

    if (begin + off < gdbctx->out_len)
    {
        gdbctx->out_len -= off;
        memmove(gdbctx->out_buf + begin, gdbctx->out_buf + begin + off, gdbctx->out_len);
    }
    else
    {
        gdbctx->out_buf[gdbctx->out_curr_packet] = 'l';
        gdbctx->out_len = gdbctx->out_curr_packet + 1;
    }

    plen = gdbctx->out_len - begin;
    if (len >= 0 && plen > len) gdbctx->out_len -= (plen - len);
    else gdbctx->out_buf[gdbctx->out_curr_packet] = 'l';

    packet_reply_close(gdbctx);
}

static enum packet_return packet_reply(struct gdb_context* gdbctx, const char* packet)
{
    packet_reply_open(gdbctx);

    assert(strchr(packet, '$') == NULL && strchr(packet, '#') == NULL);

    packet_reply_add(gdbctx, packet);

    packet_reply_close(gdbctx);

    return packet_done;
}

static enum packet_return packet_reply_error(struct gdb_context* gdbctx, int error)
{
    packet_reply_open(gdbctx);

    packet_reply_add(gdbctx, "E");
    packet_reply_val(gdbctx, error, 1);

    packet_reply_close(gdbctx);

    return packet_done;
}

static inline void packet_reply_register_hex_to(struct gdb_context* gdbctx, dbg_ctx_t* ctx, unsigned idx)
{
    const struct gdb_register *cpu_register_map = gdbctx->process->be_cpu->gdb_register_map;
    packet_reply_hex_to(gdbctx, cpu_register_ptr(gdbctx, ctx, idx), cpu_register_map[idx].length);
}

/* =============================================== *
 *          P A C K E T   H A N D L E R S          *
 * =============================================== *
 */

static void packet_reply_status_xpoints(struct gdb_context* gdbctx, struct dbg_thread *thread,
                                        dbg_ctx_t *ctx)
{
    struct dbg_process *process = thread->process;
    struct backend_cpu *cpu = process->be_cpu;
    struct gdb_xpoint *x;

    LIST_FOR_EACH_ENTRY(x, &gdbctx->xpoint_list, struct gdb_xpoint, entry)
    {
        if (x->pid != process->pid || x->tid != thread->tid)
            continue;
        if (!cpu->is_watchpoint_set(ctx, x->value))
            continue;
        if (x->type == be_xpoint_watch_write)
        {
            packet_reply_add(gdbctx, "watch:");
            packet_reply_val(gdbctx, (unsigned long)x->addr, sizeof(x->addr));
            packet_reply_add(gdbctx, ";");
        }
        if (x->type == be_xpoint_watch_read)
        {
            packet_reply_add(gdbctx, "rwatch:");
            packet_reply_val(gdbctx, (unsigned long)x->addr, sizeof(x->addr));
            packet_reply_add(gdbctx, ";");
        }
    }
}

static enum packet_return packet_reply_status(struct gdb_context* gdbctx)
{
    struct dbg_process *process = gdbctx->process;
    struct dbg_thread *thread;
    struct backend_cpu *backend;
    dbg_ctx_t ctx;
    size_t i;

    switch (gdbctx->de.dwDebugEventCode)
    {
    default:
        if (!process) return packet_error;
        if (!(backend = process->be_cpu)) return packet_error;
        if (!(thread = dbg_get_thread(process, gdbctx->de.dwThreadId)) ||
            !backend->get_context(thread->handle, &ctx))
            return packet_error;

        packet_reply_open(gdbctx);
        packet_reply_add(gdbctx, "T");
        packet_reply_val(gdbctx, signal_from_debug_event(&gdbctx->de), 1);
        packet_reply_add(gdbctx, "thread:");
        packet_reply_val(gdbctx, gdbctx->de.dwThreadId, 4);
        packet_reply_add(gdbctx, ";");
        packet_reply_status_xpoints(gdbctx, thread, &ctx);

        for (i = 0; i < backend->gdb_num_regs; i++)
        {
            packet_reply_val(gdbctx, i, 1);
            packet_reply_add(gdbctx, ":");
            packet_reply_register_hex_to(gdbctx, &ctx, i);
            packet_reply_add(gdbctx, ";");
        }

        packet_reply_close(gdbctx);
        return packet_done;

    case EXIT_PROCESS_DEBUG_EVENT:
        packet_reply_open(gdbctx);
        packet_reply_add(gdbctx, "W");
        packet_reply_val(gdbctx, gdbctx->de.u.ExitProcess.dwExitCode, 4);
        packet_reply_close(gdbctx);
        return packet_done | packet_last_f;

    case LOAD_DLL_DEBUG_EVENT:
    case UNLOAD_DLL_DEBUG_EVENT:
        packet_reply_open(gdbctx);
        packet_reply_add(gdbctx, "T");
        packet_reply_val(gdbctx, SIGTRAP, 1);
        packet_reply_add(gdbctx, "library:;");
        packet_reply_close(gdbctx);
        return packet_done;
    }
}

static enum packet_return packet_last_signal(struct gdb_context* gdbctx)
{
    assert(gdbctx->in_packet_len == 0);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_continue(struct gdb_context* gdbctx)
{
    void *addr;

    if (sscanf(gdbctx->in_packet, "%p", &addr) == 1)
        FIXME("Continue at address %p not supported\n", addr);

    handle_step_or_continue(gdbctx, gdbctx->exec_tid, FALSE, -1);

    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_verbose_cont(struct gdb_context* gdbctx)
{
    char *buf = gdbctx->in_packet, *end = gdbctx->in_packet + gdbctx->in_packet_len;

    if (gdbctx->in_packet[4] == '?')
    {
        packet_reply_open(gdbctx);
        packet_reply_add(gdbctx, "vCont");
        packet_reply_add(gdbctx, ";c");
        packet_reply_add(gdbctx, ";C");
        packet_reply_add(gdbctx, ";s");
        packet_reply_add(gdbctx, ";S");
        packet_reply_close(gdbctx);
        return packet_done;
    }

    while (buf < end && (buf = memchr(buf + 1, ';', end - buf - 1)))
    {
        int tid = -1, sig = -1;
        int action, n;

        switch ((action = buf[1]))
        {
        default:
            return packet_error;
        case 'c':
        case 's':
            buf += 2;
            break;
        case 'C':
        case 'S':
            if (sscanf(buf, ";%*c%2x", &sig) <= 0 ||
                sig != signal_from_debug_event(&gdbctx->de))
                return packet_error;
            buf += 4;
            break;
        }

        if (buf > end)
            return packet_error;
        if (buf < end && *buf == ':' && (n = sscanf(buf, ":%x", &tid)) <= 0)
            return packet_error;

        handle_step_or_continue(gdbctx, tid, action == 's' || action == 'S', sig);
    }

    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_verbose(struct gdb_context* gdbctx)
{
    if (gdbctx->in_packet_len >= 4 && !memcmp(gdbctx->in_packet, "Cont", 4))
    {
        return packet_verbose_cont(gdbctx);
    }

    if (gdbctx->in_packet_len == 14 && !memcmp(gdbctx->in_packet, "MustReplyEmpty", 14))
        return packet_reply(gdbctx, "");

    return packet_error;
}

static enum packet_return packet_continue_signal(struct gdb_context* gdbctx)
{
    void *addr;
    int sig, n;

    if ((n = sscanf(gdbctx->in_packet, "%x;%p", &sig, &addr)) == 2)
        FIXME("Continue at address %p not supported\n", addr);
    if (n < 1) return packet_error;

    if (sig != signal_from_debug_event(&gdbctx->de))
    {
        ERR("Changing signals is not supported.\n");
        return packet_error;
    }

    handle_step_or_continue(gdbctx, gdbctx->exec_tid, FALSE, sig);

    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

static enum packet_return packet_delete_breakpoint(struct gdb_context* gdbctx)
{
    struct dbg_process *process = gdbctx->process;
    struct dbg_thread *thread;
    struct backend_cpu *cpu;
    struct gdb_xpoint *x;
    dbg_ctx_t ctx;
    char type;
    void *addr;
    int size;

    if (!process) return packet_error;
    if (!(cpu = process->be_cpu)) return packet_error;

    if (sscanf(gdbctx->in_packet, "%c,%p,%x", &type, &addr, &size) < 3)
        return packet_error;

    if (type == '0')
        return packet_error;

    LIST_FOR_EACH_ENTRY(thread, &process->threads, struct dbg_thread, entry)
    {
        if (!cpu->get_context(thread->handle, &ctx))
            continue;
        if ((type == '1') && (x = gdb_find_xpoint(gdbctx, thread, be_xpoint_watch_exec, addr, size)))
            gdbctx_delete_xpoint(gdbctx, thread, &ctx, x);
        if ((type == '2' || type == '4') && (x = gdb_find_xpoint(gdbctx, thread, be_xpoint_watch_read, addr, size)))
            gdbctx_delete_xpoint(gdbctx, thread, &ctx, x);
        if ((type == '3' || type == '4') && (x = gdb_find_xpoint(gdbctx, thread, be_xpoint_watch_write, addr, size)))
            gdbctx_delete_xpoint(gdbctx, thread, &ctx, x);
        cpu->set_context(thread->handle, &ctx);
    }

    while ((type == '1') && (x = gdb_find_xpoint(gdbctx, NULL, be_xpoint_watch_exec, addr, size)))
        gdbctx_delete_xpoint(gdbctx, NULL, NULL, x);
    while ((type == '2' || type == '4') && (x = gdb_find_xpoint(gdbctx, NULL, be_xpoint_watch_read, addr, size)))
        gdbctx_delete_xpoint(gdbctx, NULL, NULL, x);
    while ((type == '3' || type == '4') && (x = gdb_find_xpoint(gdbctx, NULL, be_xpoint_watch_write, addr, size)))
        gdbctx_delete_xpoint(gdbctx, NULL, NULL, x);

    return packet_ok;
}

static enum packet_return packet_insert_breakpoint(struct gdb_context* gdbctx)
{
    struct dbg_process *process = gdbctx->process;
    struct dbg_thread *thread;
    struct backend_cpu *cpu;
    dbg_ctx_t ctx;
    char type;
    void *addr;
    int size;

    if (!process) return packet_error;
    if (!(cpu = process->be_cpu)) return packet_error;

    if (memchr(gdbctx->in_packet, ';', gdbctx->in_packet_len))
    {
        FIXME("breakpoint commands not supported\n");
        return packet_error;
    }

    if (sscanf(gdbctx->in_packet, "%c,%p,%x", &type, &addr, &size) < 3)
        return packet_error;

    if (type == '0')
        return packet_error;

    LIST_FOR_EACH_ENTRY(thread, &process->threads, struct dbg_thread, entry)
    {
        if (!cpu->get_context(thread->handle, &ctx))
            continue;
        if (type == '1')
            gdbctx_insert_xpoint(gdbctx, thread, &ctx, be_xpoint_watch_exec, addr, size);
        if (type == '2' || type == '4')
            gdbctx_insert_xpoint(gdbctx, thread, &ctx, be_xpoint_watch_read, addr, size);
        if (type == '3' || type == '4')
            gdbctx_insert_xpoint(gdbctx, thread, &ctx, be_xpoint_watch_write, addr, size);
        cpu->set_context(thread->handle, &ctx);
    }

    return packet_ok;
}

static enum packet_return packet_detach(struct gdb_context* gdbctx)
{
    detach_debuggee(gdbctx, FALSE);
    return packet_ok | packet_last_f;
}

static enum packet_return packet_read_registers(struct gdb_context* gdbctx)
{
    struct dbg_thread *thread = dbg_thread_from_tid(gdbctx, gdbctx->other_tid);
    struct backend_cpu *backend;
    dbg_ctx_t ctx;
    size_t i;

    if (!thread) return packet_error;
    if (!thread->process) return packet_error;
    if (!(backend = thread->process->be_cpu)) return packet_error;

    if (!backend->get_context(thread->handle, &ctx))
        return packet_error;

    packet_reply_open(gdbctx);
    for (i = 0; i < backend->gdb_num_regs; i++)
        packet_reply_register_hex_to(gdbctx, &ctx, i);

    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_registers(struct gdb_context* gdbctx)
{
    struct dbg_thread *thread = dbg_thread_from_tid(gdbctx, gdbctx->other_tid);
    struct backend_cpu *backend;
    dbg_ctx_t ctx;
    const char *ptr;
    size_t i;

    if (!thread) return packet_error;
    if (!thread->process) return packet_error;
    if (!(backend = thread->process->be_cpu)) return packet_error;

    if (!backend->get_context(thread->handle, &ctx))
        return packet_error;

    if (gdbctx->in_packet_len < backend->gdb_num_regs * 2)
        return packet_error;

    ptr = gdbctx->in_packet;
    for (i = 0; i < backend->gdb_num_regs; i++)
        cpu_register_hex_from(gdbctx, &ctx, i, &ptr);

    if (!backend->set_context(thread->handle, &ctx))
    {
        ERR("Failed to set context for tid %04x, error %u\n", thread->tid, GetLastError());
        return packet_error;
    }

    return packet_ok;
}

static enum packet_return packet_kill(struct gdb_context* gdbctx)
{
    detach_debuggee(gdbctx, TRUE);
    return packet_ok | packet_last_f;
}

static enum packet_return packet_thread(struct gdb_context* gdbctx)
{
    switch (gdbctx->in_packet[0])
    {
    case 'c':
        if (sscanf(gdbctx->in_packet, "c%x", &gdbctx->exec_tid) == 1)
            return packet_ok;
        return packet_error;
    case 'g':
        if (sscanf(gdbctx->in_packet, "g%x", &gdbctx->other_tid) == 1)
            return packet_ok;
        return packet_error;
    default:
        FIXME("Unknown thread sub-command %c\n", gdbctx->in_packet[0]);
        return packet_error;
    }
}

static enum packet_return packet_read_memory(struct gdb_context* gdbctx)
{
    char               *addr;
    unsigned int        len, blk_len, nread;
    char                buffer[32];
    SIZE_T              r = 0;

    if (sscanf(gdbctx->in_packet, "%p,%x", &addr, &len) != 2) return packet_error;
    if (len <= 0) return packet_error;
    TRACE("Read %u bytes at %p\n", len, addr);
    for (nread = 0; nread < len; nread += r, addr += r)
    {
        blk_len = min(sizeof(buffer), len - nread);
        if (!gdbctx->process->process_io->read(gdbctx->process->handle, addr,
            buffer, blk_len, &r) || r == 0)
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

    ptr = memchr(gdbctx->in_packet, ':', gdbctx->in_packet_len);
    if (ptr == NULL)
    {
        ERR("Cannot find ':' in %s\n", debugstr_an(gdbctx->in_packet, gdbctx->in_packet_len));
        return packet_error;
    }
    *ptr++ = '\0';

    if (sscanf(gdbctx->in_packet, "%p,%x", &addr, &len) != 2)
    {
        ERR("Failed to parse %s\n", debugstr_a(gdbctx->in_packet));
        return packet_error;
    }
    if (ptr - gdbctx->in_packet + len * 2 != gdbctx->in_packet_len)
    {
        ERR("Length %u does not match packet length %u\n",
            (int)(ptr - gdbctx->in_packet) + len * 2, gdbctx->in_packet_len);
        return packet_error;
    }
    TRACE("Write %u bytes at %p\n", len, addr);
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
    struct dbg_thread *thread = dbg_thread_from_tid(gdbctx, gdbctx->other_tid);
    struct backend_cpu *backend;
    dbg_ctx_t ctx;
    size_t reg;

    if (!thread) return packet_error;
    if (!thread->process) return packet_error;
    if (!(backend = thread->process->be_cpu)) return packet_error;

    if (!backend->get_context(thread->handle, &ctx))
        return packet_error;

    if (sscanf(gdbctx->in_packet, "%zx", &reg) != 1)
        return packet_error;
    if (reg >= backend->gdb_num_regs)
    {
        WARN("Unhandled register %zu\n", reg);
        return packet_error;
    }

    TRACE("%zu => %s\n", reg, wine_dbgstr_longlong(cpu_register(gdbctx, &ctx, reg)));

    packet_reply_open(gdbctx);
    packet_reply_register_hex_to(gdbctx, &ctx, reg);
    packet_reply_close(gdbctx);
    return packet_done;
}

static enum packet_return packet_write_register(struct gdb_context* gdbctx)
{
    struct dbg_thread *thread = dbg_thread_from_tid(gdbctx, gdbctx->other_tid);
    struct backend_cpu *backend;
    dbg_ctx_t ctx;
    size_t reg;
    char *ptr;

    if (!thread) return packet_error;
    if (!thread->process) return packet_error;
    if (!(backend = thread->process->be_cpu)) return packet_error;

    if (!backend->get_context(thread->handle, &ctx))
        return packet_error;

    if (!(ptr = strchr(gdbctx->in_packet, '=')))
        return packet_error;
    *ptr++ = '\0';

    if (sscanf(gdbctx->in_packet, "%zx", &reg) != 1)
        return packet_error;
    if (reg >= backend->gdb_num_regs)
    {
        /* FIXME: if just the reg is above cpu_num_regs, don't tell gdb
         *        it wouldn't matter too much, and it fakes our support for all regs
         */
        WARN("Unhandled register %zu\n", reg);
        return packet_ok;
    }

    TRACE("%zu <= %s\n", reg, debugstr_an(ptr, (int)(gdbctx->in_packet_len - (ptr - gdbctx->in_packet))));

    cpu_register_hex_from(gdbctx, &ctx, reg, (const char**)&ptr);
    if (!backend->set_context(thread->handle, &ctx))
    {
        ERR("Failed to set context for tid %04x, error %u\n", thread->tid, GetLastError());
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
       packet_reply_add(gdbctx, "O");
       snprintf(buffer, sizeof(buffer),
                "%*s%04lx%*s%-17.17s %08x %0*lx %.14s\n",
                indent, "", (ULONG_PTR)hWnd, 13 - indent, "",
                clsName, GetWindowLongW(hWnd, GWL_STYLE),
                addr_width(gdbctx), (ULONG_PTR)GetWindowLongPtrW(hWnd, GWLP_WNDPROC),
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
    packet_reply_add(gdbctx, "O");
    snprintf(buffer, sizeof(buffer),
             "%-16.16s %-17.17s %-8.8s %s\n",
             "hwnd", "Class Name", " Style", " WndProc Text");
    packet_reply_hex_to_str(gdbctx, buffer);
    packet_reply_close(gdbctx);

    /* FIXME: could also add a pmt to this command in str... */
    packet_query_monitor_wnd_helper(gdbctx, GetDesktopWindow(), 0);
    packet_reply(gdbctx, "OK");
}

static void packet_query_monitor_process(struct gdb_context* gdbctx, int len, const char* str)
{
    HANDLE              snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    char                buffer[31+MAX_PATH];
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
    packet_reply_add(gdbctx, "O");
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
        packet_reply_add(gdbctx, "O");
        snprintf(buffer, sizeof(buffer),
                 "%c%08x %-8d %08x '%s'\n",
                 deco, entry.th32ProcessID, entry.cntThreads,
                 entry.th32ParentProcessID, entry.szExeFile);
        packet_reply_hex_to_str(gdbctx, buffer);
        packet_reply_close(gdbctx);
        ok = Process32Next(snap, &entry);
    }
    CloseHandle(snap);
    packet_reply(gdbctx, "OK");
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
    packet_reply_add(gdbctx, "O");
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
            if (mbi.AllocationProtect & (PAGE_READONLY|PAGE_READWRITE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))
                prot[0] = 'R';
            if (mbi.AllocationProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
                prot[1] = 'W';
            if (mbi.AllocationProtect & (PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))
                prot[1] = 'C';
            if (mbi.AllocationProtect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))
                prot[2] = 'X';
        }
        else
        {
            type = "";
            prot[0] = '\0';
        }
        packet_reply_open(gdbctx);
        snprintf(buffer, sizeof(buffer), "%0*lx %0*lx %s %s %s\n",
                 addr_width(gdbctx), (DWORD_PTR)addr,
                 addr_width(gdbctx), mbi.RegionSize, state, type, prot);
        packet_reply_add(gdbctx, "O");
        packet_reply_hex_to_str(gdbctx, buffer);
        packet_reply_close(gdbctx);

        if (addr + mbi.RegionSize < addr) /* wrap around ? */
            break;
        addr += mbi.RegionSize;
    }
    packet_reply(gdbctx, "OK");
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

static BOOL CALLBACK packet_query_libraries_cb(PCSTR mod_name, DWORD64 base, PVOID ctx)
{
    struct gdb_context* gdbctx = ctx;
    MEMORY_BASIC_INFORMATION mbi;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_DOS_HEADER *dos = NULL;
    IMAGE_NT_HEADERS *nth = NULL;
    IMAGEHLP_MODULE64 mod;
    SIZE_T size, i;
    BOOL is_wow64;
    char buffer[0x400];

    mod.SizeOfStruct = sizeof(mod);
    SymGetModuleInfo64(gdbctx->process->handle, base, &mod);

    packet_reply_add(gdbctx, "<library name=\"");
    if (strcmp(mod.LoadedImageName, "[vdso].so") == 0)
        packet_reply_add(gdbctx, "linux-vdso.so.1");
    else if (mod.LoadedImageName[0] == '/')
        packet_reply_add(gdbctx, mod.LoadedImageName);
    else
    {
        UNICODE_STRING nt_name;
        ANSI_STRING ansi_name;
        char *unix_path, *tmp;

        RtlInitAnsiString(&ansi_name, mod.LoadedImageName);
        RtlAnsiStringToUnicodeString(&nt_name, &ansi_name, TRUE);

        if ((unix_path = wine_get_unix_file_name(nt_name.Buffer)))
        {
            if (IsWow64Process(gdbctx->process->handle, &is_wow64) &&
                is_wow64 && (tmp = strstr(unix_path, "system32")))
                memcpy(tmp, "syswow64", 8);
            packet_reply_add(gdbctx, unix_path);
        }
        else
            packet_reply_add(gdbctx, mod.LoadedImageName);

        HeapFree(GetProcessHeap(), 0, unix_path);
        RtlFreeUnicodeString(&nt_name);
    }
    packet_reply_add(gdbctx, "\">");

    size = sizeof(buffer);
    if (VirtualQueryEx(gdbctx->process->handle, (void *)(UINT_PTR)mod.BaseOfImage, &mbi, sizeof(mbi)) >= sizeof(mbi) &&
        mbi.Type == MEM_IMAGE && mbi.State != MEM_FREE)
    {
        if (ReadProcessMemory(gdbctx->process->handle, (void *)(UINT_PTR)mod.BaseOfImage, buffer, size, &size) &&
            size >= sizeof(IMAGE_DOS_HEADER))
            dos = (IMAGE_DOS_HEADER *)buffer;

        if (dos && dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew < size)
            nth = (IMAGE_NT_HEADERS *)(buffer + dos->e_lfanew);

        if (nth && memcmp(&nth->Signature, "PE\0\0", 4))
            nth = NULL;
    }

    if (!nth) memset(buffer, 0, sizeof(buffer));

    /* if the module is not PE we have cleared buffer with 0, this makes
     * the following computation valid in all cases. */
    dos = (IMAGE_DOS_HEADER *)buffer;
    nth = (IMAGE_NT_HEADERS *)(buffer + dos->e_lfanew);
    if (IsWow64Process(gdbctx->process->handle, &is_wow64) && is_wow64)
        sec = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS32 *)nth);
    else
        sec = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS64 *)nth);

    for (i = 0; i < max(nth->FileHeader.NumberOfSections, 1); ++i)
    {
        if ((char *)(sec + i) >= buffer + size) break;
        packet_reply_add(gdbctx, "<segment address=\"0x");
        packet_reply_val(gdbctx, mod.BaseOfImage + sec[i].VirtualAddress, sizeof(unsigned long));
        packet_reply_add(gdbctx, "\"/>");
    }

    packet_reply_add(gdbctx, "</library>");

    return TRUE;
}

static void packet_query_libraries(struct gdb_context* gdbctx)
{
    BOOL opt;

    /* this will resynchronize builtin dbghelp's internal ELF module list */
    SymLoadModule(gdbctx->process->handle, 0, 0, 0, 0, 0);

    packet_reply_add(gdbctx, "<library-list>");
    opt = SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    SymEnumerateModules64(gdbctx->process->handle, packet_query_libraries_cb, gdbctx);
    SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, opt);
    packet_reply_add(gdbctx, "</library-list>");
}

static void packet_query_threads(struct gdb_context* gdbctx)
{
    struct dbg_process* process = gdbctx->process;
    struct dbg_thread* thread;

    packet_reply_add(gdbctx, "<threads>");
    LIST_FOR_EACH_ENTRY(thread, &process->threads, struct dbg_thread, entry)
    {
        packet_reply_add(gdbctx, "<thread ");
        packet_reply_add(gdbctx, "id=\"");
        packet_reply_val(gdbctx, thread->tid, 4);
        packet_reply_add(gdbctx, "\" name=\"");
        packet_reply_add(gdbctx, thread->name);
        packet_reply_add(gdbctx, "\"/>");
    }
    packet_reply_add(gdbctx, "</threads>");
}

static void packet_query_target_xml(struct gdb_context* gdbctx, struct backend_cpu* cpu)
{
    const char* feature_prefix = NULL;
    const char* feature = NULL;
    char buffer[256];
    int i;

    packet_reply_add(gdbctx, "<target>");
    switch (cpu->machine)
    {
    case IMAGE_FILE_MACHINE_AMD64:
        packet_reply_add(gdbctx, "<architecture>i386:x86-64</architecture>");
        feature_prefix = "org.gnu.gdb.i386.";
        break;
    case IMAGE_FILE_MACHINE_I386:
        packet_reply_add(gdbctx, "<architecture>i386</architecture>");
        feature_prefix = "org.gnu.gdb.i386.";
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        packet_reply_add(gdbctx, "<architecture>arm</architecture>");
        feature_prefix = "org.gnu.gdb.arm.";
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        packet_reply_add(gdbctx, "<architecture>aarch64</architecture>");
        feature_prefix = "org.gnu.gdb.aarch64.";
        break;
    }

    for (i = 0; i < cpu->gdb_num_regs; ++i)
    {
        if (cpu->gdb_register_map[i].feature)
        {
            if (feature) packet_reply_add(gdbctx, "</feature>");
            feature = cpu->gdb_register_map[i].feature;

            packet_reply_add(gdbctx, "<feature name=\"");
            if (feature_prefix) packet_reply_add(gdbctx, feature_prefix);
            packet_reply_add(gdbctx, feature);
            packet_reply_add(gdbctx, "\">");

            if (strcmp(feature_prefix, "org.gnu.gdb.i386.") == 0 &&
                strcmp(feature, "core") == 0)
                packet_reply_add(gdbctx, "<flags id=\"i386_eflags\" size=\"4\">"
                                         "<field name=\"CF\" start=\"0\" end=\"0\"/>"
                                         "<field name=\"\" start=\"1\" end=\"1\"/>"
                                         "<field name=\"PF\" start=\"2\" end=\"2\"/>"
                                         "<field name=\"AF\" start=\"4\" end=\"4\"/>"
                                         "<field name=\"ZF\" start=\"6\" end=\"6\"/>"
                                         "<field name=\"SF\" start=\"7\" end=\"7\"/>"
                                         "<field name=\"TF\" start=\"8\" end=\"8\"/>"
                                         "<field name=\"IF\" start=\"9\" end=\"9\"/>"
                                         "<field name=\"DF\" start=\"10\" end=\"10\"/>"
                                         "<field name=\"OF\" start=\"11\" end=\"11\"/>"
                                         "<field name=\"NT\" start=\"14\" end=\"14\"/>"
                                         "<field name=\"RF\" start=\"16\" end=\"16\"/>"
                                         "<field name=\"VM\" start=\"17\" end=\"17\"/>"
                                         "<field name=\"AC\" start=\"18\" end=\"18\"/>"
                                         "<field name=\"VIF\" start=\"19\" end=\"19\"/>"
                                         "<field name=\"VIP\" start=\"20\" end=\"20\"/>"
                                         "<field name=\"ID\" start=\"21\" end=\"21\"/>"
                                         "</flags>");

            if (strcmp(feature_prefix, "org.gnu.gdb.i386.") == 0 &&
                strcmp(feature, "sse") == 0)
                packet_reply_add(gdbctx, "<vector id=\"v4f\" type=\"ieee_single\" count=\"4\"/>"
                                         "<vector id=\"v2d\" type=\"ieee_double\" count=\"2\"/>"
                                         "<vector id=\"v16i8\" type=\"int8\" count=\"16\"/>"
                                         "<vector id=\"v8i16\" type=\"int16\" count=\"8\"/>"
                                         "<vector id=\"v4i32\" type=\"int32\" count=\"4\"/>"
                                         "<vector id=\"v2i64\" type=\"int64\" count=\"2\"/>"
                                         "<union id=\"vec128\">"
                                         "<field name=\"v4_float\" type=\"v4f\"/>"
                                         "<field name=\"v2_double\" type=\"v2d\"/>"
                                         "<field name=\"v16_int8\" type=\"v16i8\"/>"
                                         "<field name=\"v8_int16\" type=\"v8i16\"/>"
                                         "<field name=\"v4_int32\" type=\"v4i32\"/>"
                                         "<field name=\"v2_int64\" type=\"v2i64\"/>"
                                         "<field name=\"uint128\" type=\"uint128\"/>"
                                         "</union>"
                                         "<flags id=\"i386_mxcsr\" size=\"4\">"
                                         "<field name=\"IE\" start=\"0\" end=\"0\"/>"
                                         "<field name=\"DE\" start=\"1\" end=\"1\"/>"
                                         "<field name=\"ZE\" start=\"2\" end=\"2\"/>"
                                         "<field name=\"OE\" start=\"3\" end=\"3\"/>"
                                         "<field name=\"UE\" start=\"4\" end=\"4\"/>"
                                         "<field name=\"PE\" start=\"5\" end=\"5\"/>"
                                         "<field name=\"DAZ\" start=\"6\" end=\"6\"/>"
                                         "<field name=\"IM\" start=\"7\" end=\"7\"/>"
                                         "<field name=\"DM\" start=\"8\" end=\"8\"/>"
                                         "<field name=\"ZM\" start=\"9\" end=\"9\"/>"
                                         "<field name=\"OM\" start=\"10\" end=\"10\"/>"
                                         "<field name=\"UM\" start=\"11\" end=\"11\"/>"
                                         "<field name=\"PM\" start=\"12\" end=\"12\"/>"
                                         "<field name=\"FZ\" start=\"15\" end=\"15\"/>"
                                         "</flags>");
        }

        snprintf(buffer, ARRAY_SIZE(buffer), "<reg name=\"%s\" bitsize=\"%zu\"",
                 cpu->gdb_register_map[i].name, 8 * cpu->gdb_register_map[i].length);
        packet_reply_add(gdbctx, buffer);

        if (cpu->gdb_register_map[i].type)
        {
            packet_reply_add(gdbctx, " type=\"");
            packet_reply_add(gdbctx, cpu->gdb_register_map[i].type);
            packet_reply_add(gdbctx, "\"");
        }

        packet_reply_add(gdbctx, "/>");
    }

    if (feature) packet_reply_add(gdbctx, "</feature>");
    packet_reply_add(gdbctx, "</target>");
}

static enum packet_return packet_query(struct gdb_context* gdbctx)
{
    int off, len;
    struct backend_cpu *cpu;

    switch (gdbctx->in_packet[0])
    {
    case 'f':
        if (strncmp(gdbctx->in_packet + 1, "ThreadInfo", gdbctx->in_packet_len - 1) == 0)
        {
            struct dbg_thread*  thd;

            packet_reply_open(gdbctx);
            packet_reply_add(gdbctx, "m");
            LIST_FOR_EACH_ENTRY(thd, &gdbctx->process->threads, struct dbg_thread, entry)
            {
                packet_reply_val(gdbctx, thd->tid, 4);
                if (list_next(&gdbctx->process->threads, &thd->entry) != NULL)
                    packet_reply_add(gdbctx, ",");
            }
            packet_reply_close(gdbctx);
            return packet_done;
        }
        else if (strncmp(gdbctx->in_packet + 1, "ProcessInfo", gdbctx->in_packet_len - 1) == 0)
        {
            char        result[128];

            packet_reply_open(gdbctx);
            packet_reply_add(gdbctx, "O");
            get_process_info(gdbctx, result, sizeof(result));
            packet_reply_hex_to_str(gdbctx, result);
            packet_reply_close(gdbctx);
            return packet_done;
        }
        break;
    case 's':
        if (strncmp(gdbctx->in_packet + 1, "ThreadInfo", gdbctx->in_packet_len - 1) == 0)
        {
            packet_reply(gdbctx, "l");
            return packet_done;
        }
        else if (strncmp(gdbctx->in_packet + 1, "ProcessInfo", gdbctx->in_packet_len - 1) == 0)
        {
            packet_reply(gdbctx, "l");
            return packet_done;
        }
        break;
    case 'A':
        if (strncmp(gdbctx->in_packet, "Attached", gdbctx->in_packet_len) == 0)
            return packet_reply(gdbctx, "1");
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
            packet_reply_add(gdbctx, "QC");
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
            return packet_reply(gdbctx, buf);
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
            packet_reply_open(gdbctx);
            packet_reply_add(gdbctx, "QStartNoAckMode+;");
            packet_reply_add(gdbctx, "qXfer:libraries:read+;");
            packet_reply_add(gdbctx, "qXfer:threads:read+;");
            packet_reply_add(gdbctx, "qXfer:features:read+;");
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
    case 'X':
        if (sscanf(gdbctx->in_packet, "Xfer:libraries:read::%x,%x", &off, &len) == 2)
        {
            if (!gdbctx->process) return packet_error;

            packet_reply_open_xfer(gdbctx);
            packet_query_libraries(gdbctx);
            packet_reply_close_xfer(gdbctx, off, len);
            return packet_done;
        }

        if (sscanf(gdbctx->in_packet, "Xfer:threads:read::%x,%x", &off, &len) == 2)
        {
            if (!gdbctx->process) return packet_error;

            packet_reply_open_xfer(gdbctx);
            packet_query_threads(gdbctx);
            packet_reply_close_xfer(gdbctx, off, len);
            return packet_done;
        }

        if (sscanf(gdbctx->in_packet, "Xfer:features:read:target.xml:%x,%x", &off, &len) == 2)
        {
            if (!gdbctx->process) return packet_error;
            if (!(cpu = gdbctx->process->be_cpu)) return packet_error;

            packet_reply_open_xfer(gdbctx);
            packet_query_target_xml(gdbctx, cpu);
            packet_reply_close_xfer(gdbctx, off, len);
            return packet_done;
        }
        break;
    }
    ERR("Unhandled query %s\n", debugstr_an(gdbctx->in_packet, gdbctx->in_packet_len));
    return packet_error;
}

static enum packet_return packet_set(struct gdb_context* gdbctx)
{
    if (strncmp(gdbctx->in_packet, "StartNoAckMode", 14) == 0)
    {
        gdbctx->no_ack_mode = TRUE;
        return packet_ok;
    }

    return packet_error;
}

static enum packet_return packet_step(struct gdb_context* gdbctx)
{
    void *addr;

    if (sscanf(gdbctx->in_packet, "%p", &addr) == 1)
        FIXME("Continue at address %p not supported\n", addr);

    handle_step_or_continue(gdbctx, gdbctx->exec_tid, TRUE, -1);

    wait_for_debuggee(gdbctx);
    return packet_reply_status(gdbctx);
}

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
        {'Q', packet_set},
        {'s', packet_step},        
        {'T', packet_thread_alive},
        {'v', packet_verbose},
        {'z', packet_delete_breakpoint},
        {'Z', packet_insert_breakpoint},
};

static BOOL extract_packets(struct gdb_context* gdbctx)
{
    char *ptr, *sum = gdbctx->in_buf, *end = gdbctx->in_buf + gdbctx->in_len;
    enum packet_return ret = packet_error;
    unsigned int cksum;
    int i, len;

    /* ptr points to the beginning ('$') of the current packet
     * sum points to the beginning ('#') of the current packet checksum ("#xx")
     * len is the length of the current packet data (sum - ptr - 1)
     * end points to the end of the received data buffer
     */

    while (!gdbctx->no_ack_mode &&
           (ptr = memchr(sum, '$', end - sum)) &&
           (sum = memchr(ptr, '#', end - ptr)) &&
           (end - sum >= 3) && sscanf(sum, "#%02x", &cksum) == 1)
    {
        len = sum - ptr - 1;
        sum += 3;

        if (cksum == checksum(ptr + 1, len))
        {
            TRACE("Acking: %s\n", debugstr_an(ptr, sum - ptr));
            write(gdbctx->sock, "+", 1);
        }
        else
        {
            ERR("Nacking: %s (checksum: %d != %d)\n", debugstr_an(ptr, sum - ptr),
                cksum, checksum(ptr + 1, len));
            write(gdbctx->sock, "-", 1);
        }
    }

    while ((ret & packet_last_f) == 0 &&
           (ptr = memchr(gdbctx->in_buf, '$', gdbctx->in_len)) &&
           (sum = memchr(ptr, '#', end - ptr)) &&
           (end - sum >= 3) && sscanf(sum, "#%02x", &cksum) == 1)
    {
        if (ptr != gdbctx->in_buf)
            WARN("Ignoring: %s\n", debugstr_an(gdbctx->in_buf, ptr - gdbctx->in_buf));

        len = sum - ptr - 1;
        sum += 3;

        if (cksum == checksum(ptr + 1, len))
        {
            TRACE("Handling: %s\n", debugstr_an(ptr, sum - ptr));

            ret = packet_error;
            gdbctx->in_packet = ptr + 2;
            gdbctx->in_packet_len = len - 1;
            gdbctx->in_packet[gdbctx->in_packet_len] = '\0';

            for (i = 0; i < ARRAY_SIZE(packet_entries); i++)
                if (packet_entries[i].key == ptr[1])
                    break;

            if (i == ARRAY_SIZE(packet_entries))
                WARN("Unhandled: %s\n", debugstr_an(ptr + 1, len));
            else if (((ret = (packet_entries[i].handler)(gdbctx)) & ~packet_last_f) == packet_error)
                WARN("Failed: %s\n", debugstr_an(ptr + 1, len));

            switch (ret & ~packet_last_f)
            {
            case packet_error:  packet_reply(gdbctx, ""); break;
            case packet_ok:     packet_reply(gdbctx, "OK"); break;
            case packet_done:   break;
            }

            TRACE("Reply: %s\n", debugstr_an(gdbctx->out_buf, gdbctx->out_len));
            i = write(gdbctx->sock, gdbctx->out_buf, gdbctx->out_len);
            assert(i == gdbctx->out_len);
            gdbctx->out_len = 0;
        }
        else
            WARN("Ignoring: %s (checksum: %d != %d)\n", debugstr_an(ptr, sum - ptr),
                cksum, checksum(ptr + 1, len));

        gdbctx->in_len = end - sum;
        memmove(gdbctx->in_buf, sum, end - sum);
        end = gdbctx->in_buf + gdbctx->in_len;
    }

    return (ret & packet_last_f);
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
        len = read(gdbctx->sock, gdbctx->in_buf + gdbctx->in_len, gdbctx->in_buf_alloc - gdbctx->in_len - 1);
        if (len <= 0) break;
        gdbctx->in_len += len;
        assert(gdbctx->in_len <= gdbctx->in_buf_alloc);
        if (len < gdbctx->in_buf_alloc - gdbctx->in_len) break;
    }

    gdbctx->in_buf[gdbctx->in_len] = '\0';
    return gdbctx->in_len - in_len;
}

#define FLAG_NO_START   1
#define FLAG_WITH_XTERM 2

static BOOL gdb_exec(unsigned port, unsigned flags)
{
    char            buf[MAX_PATH];
    int             fd;
    const char      *gdb_path, *tmp_path;
    FILE*           f;

    if (!(gdb_path = getenv("WINE_GDB"))) gdb_path = "gdb";
    if (!(tmp_path = getenv("TMPDIR"))) tmp_path = "/tmp";
    strcpy(buf, tmp_path);
    strcat(buf, "/winegdb.XXXXXX");
    fd = mkstemps(buf, 0);
    if (fd == -1) return FALSE;
    if ((f = fdopen(fd, "w+")) == NULL) return FALSE;
    fprintf(f, "target remote localhost:%d\n", ntohs(port));
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

static BOOL gdb_startup(struct gdb_context* gdbctx, unsigned flags, unsigned port)
{
    int                 sock;
    struct sockaddr_in  s_addrs = {0};
    socklen_t           s_len = sizeof(s_addrs);
    struct pollfd       pollfd;
    BOOL                ret = FALSE;

    /* step 1: create socket for gdb connection request */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        ERR("Failed to create socket: %s\n", strerror(errno));
        return FALSE;
    }

    s_addrs.sin_family = AF_INET;
    s_addrs.sin_addr.s_addr = INADDR_ANY;
    s_addrs.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&s_addrs, sizeof(s_addrs)) == -1)
        goto cleanup;

    if (listen(sock, 1) == -1 || getsockname(sock, (struct sockaddr*)&s_addrs, &s_len) == -1)
        goto cleanup;

    /* step 2: do the process internal creation */
    handle_debug_event(gdbctx, FALSE);

    /* step 3: fire up gdb (if requested) */
    if (flags & FLAG_NO_START)
        fprintf(stderr, "target remote localhost:%d\n", ntohs(s_addrs.sin_port));
    else
        switch (fork())
        {
        case -1: /* error in parent... */
            ERR("Failed to start gdb: fork: %s\n", strerror(errno));
            goto cleanup;
        default: /* in parent... success */
            signal(SIGINT, SIG_IGN);
            break;
        case 0: /* in child... and alive */
            gdb_exec(s_addrs.sin_port, flags);
            /* if we're here, exec failed, so report failure */
            goto cleanup;
        }

    /* step 4: wait for gdb to connect actually */
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
            ret = TRUE;
            TRACE("connected on %d\n", gdbctx->sock);
            /* don't keep our small packets too long: send them ASAP back to GDB
             * without this, GDB really crawls
             */
            setsockopt(gdbctx->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&dummy, sizeof(dummy));
        }
        break;
    case 0:
        ERR("Timed out connecting to gdb\n");
        break;
    case -1:
        ERR("Failed to connect to gdb: poll: %s\n", strerror(errno));
        break;
    default:
        assert(0);
    }

cleanup:
    close(sock);
    return ret;
}

static BOOL gdb_init_context(struct gdb_context* gdbctx, unsigned flags, unsigned port)
{
    int                 i;

    gdbctx->sock = -1;
    gdbctx->in_buf = NULL;
    gdbctx->in_buf_alloc = 0;
    gdbctx->in_len = 0;
    gdbctx->out_buf = NULL;
    gdbctx->out_buf_alloc = 0;
    gdbctx->out_len = 0;
    gdbctx->out_curr_packet = -1;

    gdbctx->exec_tid = -1;
    gdbctx->other_tid = -1;
    list_init(&gdbctx->xpoint_list);
    gdbctx->process = NULL;
    gdbctx->no_ack_mode = FALSE;
    for (i = 0; i < ARRAY_SIZE(gdbctx->wine_segs); i++)
        gdbctx->wine_segs[i] = 0;

    /* wait for first trap */
    while (WaitForDebugEvent(&gdbctx->de, INFINITE))
    {
        if (gdbctx->de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            /* this should be the first event we get,
             * and the only one of this type  */
            assert(gdbctx->process == NULL && gdbctx->de.dwProcessId == dbg_curr_pid);
            /* gdbctx->dwProcessId = pid; */
            if (!gdb_startup(gdbctx, flags, port)) return FALSE;
        }
        else if (!handle_debug_event(gdbctx, FALSE))
            break;
        ContinueDebugEvent(gdbctx->de.dwProcessId, gdbctx->de.dwThreadId, DBG_CONTINUE);
    }
    return TRUE;
}

static int gdb_remote(unsigned flags, unsigned port)
{
    struct pollfd       pollfd;
    struct gdb_context  gdbctx;
    BOOL                doLoop;

    for (doLoop = gdb_init_context(&gdbctx, flags, port); doLoop;)
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
                ERR("gdb hung up\n");
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
            ERR("poll failed: %s\n", strerror(errno));
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
    unsigned gdb_flags = 0, port = 0;
    char *port_end;

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
        if (strcmp(argv[0], "--port") == 0 && argc > 1)
        {
            port = strtoul(argv[1], &port_end, 10);
            if (*port_end)
            {
                fprintf(stderr, "Invalid port: %s\n", argv[1]);
                return -1;
            }
            argc -= 2; argv += 2;
            continue;
        }
        return -1;
    }
    if (dbg_active_attach(argc, argv) == start_ok ||
        dbg_active_launch(argc, argv) == start_ok)
        return gdb_remote(gdb_flags, port);
#else
    fprintf(stderr, "GdbProxy mode not supported on this platform\n");
#endif
    return -1;
}
