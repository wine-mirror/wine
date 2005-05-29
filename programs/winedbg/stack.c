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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "debugger.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "tlhelp32.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

static int                      nframe;
static IMAGEHLP_STACK_FRAME*    frames = NULL;

/***********************************************************************
 *           stack_info
 *
 * Dump the top of the stack
 */
void stack_info(void)
{
    struct dbg_lvalue lvalue;

    lvalue.cookie = 0;
    lvalue.type.id = dbg_itype_none;
    lvalue.type.module = 0;

    /* FIXME: we assume stack grows the same way as on i386 */
    if (!memory_get_current_stack(&lvalue.addr))
        dbg_printf("Bad segment (%d)\n", lvalue.addr.Segment);

    dbg_printf("Stack dump:\n");
    switch (lvalue.addr.Mode)
    {
    case AddrModeFlat: /* 32-bit mode */
    case AddrMode1632: /* 32-bit mode */
        memory_examine(&lvalue, 24, 'x');
        break;
    case AddrModeReal:  /* 16-bit mode */
    case AddrMode1616:
        memory_examine(&lvalue, 24, 'w');
	break;
    }
}

int stack_set_frame(int newframe)
{
    ADDRESS     addr;

    dbg_curr_frame = newframe;
    if (dbg_curr_frame >= nframe) dbg_curr_frame = nframe - 1;
    if (dbg_curr_frame < 0)       dbg_curr_frame = 0;

    addr.Mode = AddrModeFlat;
    addr.Offset = frames[dbg_curr_frame].InstructionOffset;
    source_list_from_addr(&addr, 0);
    return TRUE;
}

BOOL stack_get_frame(SYMBOL_INFO* symbol, IMAGEHLP_STACK_FRAME* ihsf)
{
    DWORD64     disp;
    /*
     * If we don't have a valid backtrace, then just return.
     */
    if (frames == NULL) return FALSE;

    /*
     * If we don't know what the current function is, then we also have
     * nothing to report here.
     */
    if (!SymFromAddr(dbg_curr_process->handle, frames[dbg_curr_frame].InstructionOffset,
                     &disp, symbol))
        return FALSE;
    if (ihsf) *ihsf = frames[dbg_curr_frame];

    return TRUE;
}

/******************************************************************
 *		backtrace
 *
 * Do a backtrace on the the current thread
 */
static unsigned backtrace(BOOL with_frames, BOOL noisy)
{
    STACKFRAME  sf;
    unsigned    nf = 0;

    memset(&sf, 0, sizeof(sf));
    memory_get_current_frame(&sf.AddrFrame);
    memory_get_current_pc(&sf.AddrPC);

    /* don't confuse StackWalk by passing in inconsistent addresses */
    if ((sf.AddrPC.Mode == AddrModeFlat) && (sf.AddrFrame.Mode != AddrModeFlat))
    {
        sf.AddrFrame.Offset = (DWORD)memory_to_linear_addr(&sf.AddrFrame);
        sf.AddrFrame.Mode = AddrModeFlat;
    }

    if (noisy) dbg_printf("Backtrace:\n");
    while (StackWalk(IMAGE_FILE_MACHINE_I386, dbg_curr_process->handle, 
                     dbg_curr_thread->handle, &sf, &dbg_context, NULL,
                     SymFunctionTableAccess, SymGetModuleBase, NULL))
    {
        if (with_frames)
        {
            frames = dbg_heap_realloc(frames, 
                                      (nf + 1) * sizeof(IMAGEHLP_STACK_FRAME));

            frames[nf].InstructionOffset = (unsigned long)memory_to_linear_addr(&sf.AddrPC);
            frames[nf].FrameOffset = (unsigned long)memory_to_linear_addr(&sf.AddrFrame);
        }
        if (noisy)
        {
            dbg_printf("%s%d ", 
                       (with_frames && nf == dbg_curr_frame ? "=>" : "  "),
                       nf + 1);
            print_addr_and_args(&sf.AddrPC, &sf.AddrFrame);
            dbg_printf(" (");
            print_bare_address(&sf.AddrFrame);
            dbg_printf(")\n");
        }
        nf++;
        /* we've probably gotten ourselves into an infinite loop so bail */
        if (nf > 200)
            break;
    }
    return nf;
}

/******************************************************************
 *		backtrace_tid
 *
 * Do a backtrace on a thread from its process and its identifier
 * (preserves current thread and context information)
 */
static void backtrace_tid(struct dbg_process* pcs, DWORD tid, BOOL noisy)
{
    struct dbg_thread*  thread = dbg_curr_thread;

    if (!(dbg_curr_thread = dbg_get_thread(pcs, tid)))
        dbg_printf("Unknown thread id (0x%lx) in process (0x%lx)\n", tid, pcs->pid);
    else
    {
        CONTEXT saved_ctx = dbg_context;

        dbg_curr_tid = dbg_curr_thread->tid;
        memset(&dbg_context, 0, sizeof(dbg_context));
        dbg_context.ContextFlags = CONTEXT_FULL;
        if (SuspendThread(dbg_curr_thread->handle) != -1)
        {
            if (!GetThreadContext(dbg_curr_thread->handle, &dbg_context))
            {
                dbg_printf("Can't get context for thread 0x%lx in current process\n",
                           tid);
            }
            else backtrace(FALSE, noisy);
            ResumeThread(dbg_curr_thread->handle);
        }
        else dbg_printf("Can't suspend thread 0x%lx in current process\n", tid);
        dbg_context = saved_ctx;
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
        struct dbg_process* cp = dbg_curr_process;
        DWORD               cpid = dbg_curr_pid;
        do
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId()) continue;
            if (dbg_curr_process && dbg_curr_pid != cpid)
                dbg_detach_debuggee();

            if (entry.th32OwnerProcessID == cpid)
                dbg_curr_process = cp;
            else if (entry.th32OwnerProcessID != dbg_curr_pid)
            {
                if (!dbg_attach_debuggee(entry.th32OwnerProcessID, FALSE, TRUE))
                {
                    dbg_printf("\nwarning: could not attach to 0x%lx\n",
                               entry.th32OwnerProcessID);
                    continue;
                }
                dbg_curr_pid = dbg_curr_process->pid;
            }

            dbg_printf("\nBacktracing for thread 0x%lx in process 0x%lx (%s):\n",
                       entry.th32ThreadID, dbg_curr_pid, dbg_curr_process->imageName);
            backtrace_tid(dbg_curr_process, entry.th32ThreadID, TRUE);
        }
        while (Thread32Next(snapshot, &entry));

        if (dbg_curr_process && dbg_curr_pid != cpid)
            dbg_detach_debuggee();
        dbg_curr_process = cp;      dbg_curr_pid = cpid;
    }
    CloseHandle(snapshot);
}

void stack_backtrace(DWORD tid, BOOL noisy)
{
    /* backtrace every thread in every process except the debugger itself,
     * invoking via "bt all"
     */
    if (tid == -1) return backtrace_all();

    if (!dbg_curr_process) 
    {
        dbg_printf("You must be attached to a process to run this command.\n");
        return;
    }
    
    if (tid == dbg_curr_tid)
    {
        HeapFree(GetProcessHeap(), 0, frames);
        frames = NULL;
        nframe = backtrace(TRUE, noisy);
    }
    else
    {
        backtrace_tid(dbg_curr_process, tid, noisy);
    }
}
