/*
 * Debugger stack handling
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
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

#include <stdlib.h>
#include <stdio.h>

#include "debugger.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "tlhelp32.h"

/***********************************************************************
 *           stack_info
 *
 * Dump the top of the stack. If len <= 0, a default length is used.
 */
void stack_info(int len)
{
    struct dbg_lvalue lvalue;

    if(len <= 0)
        len = 24;
    init_lvalue(&lvalue, TRUE, 0);
    lvalue.type.id = dbg_itype_segptr;

    /* FIXME: we assume stack grows the same way as on i386 */
    if (!memory_get_current_stack(&lvalue.addr))
        dbg_printf("Bad segment (%d)\n", lvalue.addr.Segment);

    dbg_printf("Stack dump:\n");
    switch (lvalue.addr.Mode)
    {
    case AddrModeFlat: /* 32-bit or 64-bit mode */
        memory_examine(&lvalue, len, 'a');
        break;
    case AddrMode1632: /* 32-bit mode */
        memory_examine(&lvalue, len, 'x');
        break;
    case AddrModeReal:  /* 16-bit mode */
    case AddrMode1616:
        memory_examine(&lvalue, len, 'w');
	break;
    }
}

static BOOL stack_set_local_scope(void)
{
    struct dbg_frame* frm = stack_get_thread_frame(dbg_curr_thread, dbg_curr_thread->curr_frame);

    if (!frm) return FALSE;
    /* if we're not the first frame, linear_pc is the return address
     * after the call instruction (at least on most processors I know of).
     * However, there are cases where this address is outside of the
     * current function or inline site.
     * This happens when the called function is marked <NO RETURN>, in which
     * case the compiler can omit the epilog (gcc 4 does it).
     * This happens also for inline sites, where the epilog (of the inline
     * site) isn't present.
     * Therefore, we decrement linear_pc in order to ensure that
     * the considered address is really inside the current function or inline site.
     */
    return SymSetScopeFromInlineContext(dbg_curr_process->handle,
                                        (dbg_curr_thread->curr_frame) ? frm->linear_pc - 1 : frm->linear_pc,
                                        frm->inline_ctx);
}

static BOOL stack_set_frame_internal(int newframe)
{
    if (newframe >= dbg_curr_thread->num_frames)
        newframe = dbg_curr_thread->num_frames - 1;
    if (newframe < 0)
        newframe = 0;

    if (dbg_curr_thread->curr_frame != newframe)
    {
        dbg_curr_thread->curr_frame = newframe;
        stack_set_local_scope();
    }
    return TRUE;
}

BOOL stack_get_register_frame(const struct dbg_internal_var* div, struct dbg_lvalue* lvalue)
{
    struct dbg_frame* currfrm = stack_get_curr_frame();
    if (currfrm == NULL) return FALSE;
    if (currfrm->is_ctx_valid)
        init_lvalue_in_debugger(lvalue, 0, div->typeid,
                                (char*)&currfrm->context + (DWORD_PTR)div->pval);
    else
    {
        enum be_cpu_addr        kind;
        DWORD                   itype = ADDRSIZE == 4 ? dbg_itype_unsigned_long32 : dbg_itype_unsigned_long64;

        if (!dbg_curr_process->be_cpu->get_register_info(div->val, &kind)) return FALSE;

        /* reuse some known registers directly out of stackwalk details */
        switch (kind)
        {
        case be_cpu_addr_pc:
            init_lvalue_in_debugger(lvalue, 0, itype, &currfrm->linear_pc);
            break;
        case be_cpu_addr_stack:
            init_lvalue_in_debugger(lvalue, 0, itype, &currfrm->linear_stack);
            break;
        case be_cpu_addr_frame:
            init_lvalue_in_debugger(lvalue, 0, itype, &currfrm->linear_frame);
            break;
        }
    }
    return TRUE;
}

BOOL stack_set_frame(int newframe)
{
    ADDRESS64   addr;
    if (!stack_set_frame_internal(newframe)) return FALSE;
    addr.Mode = AddrModeFlat;
    addr.Offset = (DWORD_PTR)memory_to_linear_addr(&stack_get_curr_frame()->addr_pc);
    source_list_from_addr(&addr, 0);
    return TRUE;
}

/******************************************************************
 *		stack_get_current_symbol
 *
 * Retrieves the symbol information for the current frame element
 */
BOOL stack_get_current_symbol(SYMBOL_INFO* symbol)
{
    DWORD64                     disp;
    struct dbg_frame*           frm = stack_get_curr_frame();

    if (frm == NULL) return FALSE;
    return SymFromInlineContext(dbg_curr_process->handle, frm->linear_pc, frm->inline_ctx, &disp, symbol);
}

static BOOL CALLBACK stack_read_mem(HANDLE hProc, DWORD64 addr, 
                                    PVOID buffer, DWORD size, PDWORD written)
{
    SIZE_T sz;
    BOOL ret;

    struct dbg_process* pcs = dbg_get_process_h(hProc);
    if (!pcs) return FALSE;
    ret = pcs->process_io->read(hProc, (const void*)(DWORD_PTR)addr, buffer,
                                size, &sz);
    if (written != NULL) *written = sz;
    return ret;
}

/******************************************************************
 *		stack_fetch_frames
 *
 * Do a backtrace on the current thread
 */
unsigned stack_fetch_frames(const dbg_ctx_t* _ctx)
{
    STACKFRAME_EX sf;
    unsigned      nf = 0;
    /* as native stackwalk can modify the context passed to it, simply copy
     * it to avoid any damage
     */
    dbg_ctx_t     ctx = *_ctx;
    BOOL          ret;

    free(dbg_curr_thread->frames);
    dbg_curr_thread->frames = NULL;

    memset(&sf, 0, sizeof(sf));
    sf.StackFrameSize = sizeof(sf);
    dbg_curr_process->be_cpu->get_addr(dbg_curr_thread->handle, &ctx, be_cpu_addr_frame, &sf.AddrFrame);
    dbg_curr_process->be_cpu->get_addr(dbg_curr_thread->handle, &ctx, be_cpu_addr_pc, &sf.AddrPC);
    dbg_curr_process->be_cpu->get_addr(dbg_curr_thread->handle, &ctx, be_cpu_addr_stack, &sf.AddrStack);
    sf.InlineFrameContext = INLINE_FRAME_CONTEXT_INIT;

    /* don't confuse StackWalk by passing in inconsistent addresses */
    if ((sf.AddrPC.Mode == AddrModeFlat) && (sf.AddrFrame.Mode != AddrModeFlat))
    {
        sf.AddrFrame.Offset = (ULONG_PTR)memory_to_linear_addr(&sf.AddrFrame);
        sf.AddrFrame.Mode = AddrModeFlat;
    }

    while ((ret = StackWalkEx(dbg_curr_process->be_cpu->machine, dbg_curr_process->handle,
                              dbg_curr_thread->handle, &sf, &ctx, stack_read_mem,
                              SymFunctionTableAccess64, SymGetModuleBase64, NULL, SYM_STKWALK_DEFAULT)) ||
           nf == 0) /* we always register first frame information */
    {
        struct dbg_frame* new = realloc(dbg_curr_thread->frames,
                                        (nf + 1) * sizeof(dbg_curr_thread->frames[0]));
        if (!new) break;
        dbg_curr_thread->frames = new;

        dbg_curr_thread->frames[nf].addr_pc      = sf.AddrPC;
        dbg_curr_thread->frames[nf].linear_pc    = (DWORD_PTR)memory_to_linear_addr(&sf.AddrPC);
        dbg_curr_thread->frames[nf].addr_frame   = sf.AddrFrame;
        dbg_curr_thread->frames[nf].linear_frame = (DWORD_PTR)memory_to_linear_addr(&sf.AddrFrame);
        dbg_curr_thread->frames[nf].addr_stack   = sf.AddrStack;
        dbg_curr_thread->frames[nf].linear_stack = (DWORD_PTR)memory_to_linear_addr(&sf.AddrStack);
        dbg_curr_thread->frames[nf].context      = ctx;
        dbg_curr_thread->frames[nf].inline_ctx   = sf.InlineFrameContext;
        /* FIXME: can this heuristic be improved: we declare first context always valid, and next ones
         * if it has been modified by the call to StackWalk...
         */
        dbg_curr_thread->frames[nf].is_ctx_valid =
            (nf == 0 ||
             (dbg_curr_thread->frames[nf - 1].is_ctx_valid &&
              memcmp(&dbg_curr_thread->frames[nf - 1].context, &ctx, sizeof(ctx))));
        nf++;
        /* bail if:
         * - we've (probably) gotten ourselves into an infinite loop,
         * - or StackWalk failed on first frame
         */
        if (nf > 200 || !ret) break;
    }
    dbg_curr_thread->curr_frame = -1;
    dbg_curr_thread->num_frames = nf;
    stack_set_frame_internal(0);
    return nf;
}

struct sym_enum
{
    DWORD_PTR   frame;
    BOOL        first;
};

static BOOL WINAPI sym_enum_cb(PSYMBOL_INFO sym_info, ULONG size, PVOID user)
{
    struct sym_enum*    se = user;

    if (sym_info->Flags & SYMFLAG_PARAMETER)
    {
        if (!se->first) dbg_printf(", "); else se->first = FALSE;
        dbg_printf("%s=", sym_info->Name);
        symbol_print_localvalue(sym_info, se->frame, FALSE);
    }
    return TRUE;
}

static void stack_print_addr_and_args(void)
{
    char                        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)buffer;
    IMAGEHLP_LINE64             il;
    IMAGEHLP_MODULE             im;
    DWORD64                     disp64;
    struct dbg_frame*           frm = stack_get_curr_frame();

    if (!frm) return;
    print_bare_address(&frm->addr_pc);

    /* grab module where symbol is. If we don't have a module, we cannot print more */
    im.SizeOfStruct = sizeof(im);
    if (!SymGetModuleInfo(dbg_curr_process->handle, frm->linear_pc, &im))
        return;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    if (SymFromInlineContext(dbg_curr_process->handle, frm->linear_pc, frm->inline_ctx, &disp64, si))
    {
        struct sym_enum se;
        DWORD           disp;

        dbg_printf(" %s", si->Name);
        if (disp64) dbg_printf("+0x%I64x", disp64);

        stack_set_local_scope();
        se.first = TRUE;
        se.frame = frm->linear_frame;
        dbg_printf("(");
        SymEnumSymbols(dbg_curr_process->handle, 0, NULL, sym_enum_cb, &se);
        dbg_printf(")");

        il.SizeOfStruct = sizeof(il);
        if (SymGetLineFromInlineContext(dbg_curr_process->handle, frm->linear_pc, frm->inline_ctx,
                                        0, &disp, &il))
            dbg_printf(" [%s:%lu]", il.FileName, il.LineNumber);
        dbg_printf(" in %s", im.ModuleName);
    }
    else dbg_printf(" in %s (+0x%Ix)", im.ModuleName, frm->linear_pc - im.BaseOfImage);
}

/******************************************************************
 *		backtrace
 *
 * Do a backtrace on the current thread
 */
static void backtrace(void)
{
    unsigned                    cf = dbg_curr_thread->curr_frame;

    dbg_printf("Backtrace:\n");
    for (dbg_curr_thread->curr_frame = 0;
         dbg_curr_thread->curr_frame < dbg_curr_thread->num_frames;
         dbg_curr_thread->curr_frame++)
    {
        dbg_printf("%s%d ", 
                   (cf == dbg_curr_thread->curr_frame ? "=>" : "  "),
                   dbg_curr_thread->curr_frame);
        stack_print_addr_and_args();
        dbg_printf(" (");
        print_bare_address(&dbg_curr_thread->frames[dbg_curr_thread->curr_frame].addr_frame);
        dbg_printf(")\n");
    }
    /* reset context to current stack frame */
    dbg_curr_thread->curr_frame = cf;
    if (!dbg_curr_thread->frames) return;
    stack_set_local_scope();
}

/******************************************************************
 *		backtrace_tid
 *
 * Do a backtrace on a thread from its process and its identifier
 * (preserves current thread and context information)
 */
static void backtrace_tid(struct dbg_process* pcs, DWORD tid)
{
    struct dbg_thread*  thread = dbg_curr_thread;

    if (!(dbg_curr_thread = dbg_get_thread(pcs, tid)))
        dbg_printf("Unknown thread id (%04lx) in process (%04lx)\n", tid, pcs->pid);
    else
    {
        dbg_ctx_t ctx = {{0}};

        dbg_curr_tid = dbg_curr_thread->tid;
        if (SuspendThread(dbg_curr_thread->handle) != -1)
        {
            if (!pcs->be_cpu->get_context(dbg_curr_thread->handle, &ctx))
            {
                dbg_printf("Can't get context for thread %04lx in current process\n",
                           tid);
            }
            else
            {
                stack_fetch_frames(&ctx);
                backtrace();
            }
            ResumeThread(dbg_curr_thread->handle);
        }
        else dbg_printf("Can't suspend thread %04lx in current process\n", tid);
    }
    dbg_curr_thread = thread;
    dbg_curr_tid = thread ? thread->tid : 0;
}

/******************************************************************
 *		backtrace_all
 *
 * Do a backtrace on every running thread in the system (except the debugger)
 * (preserves current process information)
 */
static void backtrace_all(void)
{
    struct dbg_process* process = dbg_curr_process;
    struct dbg_thread*  thread = dbg_curr_thread;
    dbg_ctx_t ctx = dbg_context;
    DWORD               cpid = dbg_curr_pid;
    THREADENTRY32       entry;
    HANDLE              snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        dbg_printf("Unable to create toolhelp snapshot\n");
        return;
    }

    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry))
    {
        do
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId()) continue;
            if (dbg_curr_process && dbg_curr_pid != entry.th32OwnerProcessID &&
                cpid != dbg_curr_pid)
                dbg_curr_process->process_io->close_process(dbg_curr_process, FALSE);

            if (entry.th32OwnerProcessID == cpid)
            {
                dbg_curr_process = process;
                dbg_curr_pid = cpid;
            }
            else if (entry.th32OwnerProcessID != dbg_curr_pid)
            {
                if (!dbg_attach_debuggee(entry.th32OwnerProcessID))
                {
                    dbg_printf("\nwarning: could not attach to %04lx\n",
                               entry.th32OwnerProcessID);
                    continue;
                }
                dbg_curr_pid = dbg_curr_process->pid;
                dbg_active_wait_for_first_exception();
            }

            dbg_printf("\nBacktracing for thread %04lx in process %04lx (%ls):\n",
                       entry.th32ThreadID, dbg_curr_pid, dbg_curr_process->imageName);
            backtrace_tid(dbg_curr_process, entry.th32ThreadID);
        }
        while (Thread32Next(snapshot, &entry));

        if (dbg_curr_process && cpid != dbg_curr_pid)
            dbg_curr_process->process_io->close_process(dbg_curr_process, FALSE);
    }
    CloseHandle(snapshot);
    dbg_curr_process = process;
    dbg_curr_pid = cpid;
    dbg_curr_thread = thread;
    dbg_curr_tid = thread ? thread->tid : 0;
    dbg_context = ctx;
}

void stack_backtrace(DWORD tid)
{
    /* backtrace every thread in every process except the debugger itself,
     * invoking via "bt all"
     */
    if (tid == -1)
    {
        backtrace_all();
        return;
    }

    if (!dbg_curr_process) 
    {
        dbg_printf("You must be attached to a process to run this command.\n");
        return;
    }
    
    if (tid == dbg_curr_tid)
    {
        backtrace();
    }
    else
    {
        backtrace_tid(dbg_curr_process, tid);
    }
}
