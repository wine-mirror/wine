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

void stack_backtrace(DWORD tid, BOOL noisy)
{
    STACKFRAME                  sf;
    CONTEXT                     saved_dbg_context;
    struct dbg_thread*          thread;
    unsigned                    nf;

    if (tid == -1)  /* backtrace every thread in every process except the debugger itself, invoking via "bt all" */
    {
        THREADENTRY32 entry;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            dbg_printf("unable to create toolhelp snapshot\n");
            return;
        }
        
        entry.dwSize = sizeof(entry);
        
        if (!Thread32First(snapshot, &entry))
        {
            CloseHandle(snapshot);
            return;
        }
        
        do
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId()) continue;
            if (dbg_curr_process) dbg_detach_debuggee();

            dbg_printf("\n");
            if (!dbg_attach_debuggee(entry.th32OwnerProcessID, FALSE, TRUE))
            {
                dbg_printf("\nwarning: could not attach to 0x%lx\n", entry.th32OwnerProcessID);
                continue;
            }

            dbg_printf("Backtracing for thread 0x%lx in process 0x%lx (%s):\n", entry.th32ThreadID, dbg_curr_pid, dbg_curr_process->imageName);
            
            stack_backtrace(entry.th32ThreadID, TRUE);
        }
        while (Thread32Next(snapshot, &entry));

        if (dbg_curr_process) dbg_detach_debuggee();
        CloseHandle(snapshot);
        return;
    }

    if (!dbg_curr_process) 
    {
        dbg_printf("You must be attached to a process to run this command.\n");
        return;
    }
    
    saved_dbg_context = dbg_context; /* as we may modify dbg_context... */
    if (tid == dbg_curr_tid)
    {
        thread = dbg_curr_thread;
        HeapFree(GetProcessHeap(), 0, frames);
        frames = NULL;
    }
    else
    {
         thread = dbg_get_thread(dbg_curr_process, tid);
         if (!thread)
         {
              dbg_printf("Unknown thread id (0x%08lx) in current process\n", tid);
              return;
         }
         memset(&dbg_context, 0, sizeof(dbg_context));
         dbg_context.ContextFlags = CONTEXT_FULL;
         if (SuspendThread(thread->handle) != -1)
         {
             if (!GetThreadContext(thread->handle, &dbg_context))
             {
                 dbg_printf("Can't get context for thread 0x%lx in current process\n",
                            tid);
                 ResumeThread(thread->handle);
                 return;
             }
         }
         else
         {
             dbg_printf("Can't suspend thread 0x%lx in current process\n", tid);
             return;
         }
    }

    nf = 0;
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
                     thread->handle, &sf, &dbg_context, NULL, SymFunctionTableAccess,
                     SymGetModuleBase, NULL))
    {
        if (tid == dbg_curr_tid)
        {
            frames = dbg_heap_realloc(frames, 
                                      (nf + 1) * sizeof(IMAGEHLP_STACK_FRAME));

            frames[nf].InstructionOffset = (unsigned long)memory_to_linear_addr(&sf.AddrPC);
            frames[nf].FrameOffset = (unsigned long)memory_to_linear_addr(&sf.AddrFrame);
        }
        if (noisy)
        {
            dbg_printf("%s%d ", 
                       (tid == dbg_curr_tid && nf == dbg_curr_frame ? "=>" : "  "),
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

    dbg_context = saved_dbg_context;
    if (tid == dbg_curr_tid)
    {
        nframe = nf;
    }
    else
    {
        ResumeThread(thread->handle);
    }
}
