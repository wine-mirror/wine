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

/***********************************************************************
 *           stack_info
 *
 * Dump the top of the stack
 */
void stack_info(void)
{
    struct dbg_lvalue lvalue;

    lvalue.cookie = 0;
    lvalue.type.id = dbg_itype_segptr;
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

static BOOL stack_set_frame_internal(int newframe)
{
    if (newframe >= dbg_curr_thread->num_frames)
        newframe = dbg_curr_thread->num_frames - 1;
    if (newframe < 0)
        newframe = 0;

    if (dbg_curr_thread->curr_frame != newframe)
    {
        IMAGEHLP_STACK_FRAME    ihsf;

        dbg_curr_thread->curr_frame = newframe;
        stack_get_current_frame(&ihsf);
        SymSetContext(dbg_curr_process->handle, &ihsf, NULL);
    }
    return TRUE;
}

static BOOL stack_get_frame(int nf, IMAGEHLP_STACK_FRAME* ihsf)
{
    memset(ihsf, 0, sizeof(*ihsf));
    ihsf->InstructionOffset = (unsigned long)memory_to_linear_addr(&dbg_curr_thread->frames[nf].addr_pc);
    ihsf->FrameOffset = (unsigned long)memory_to_linear_addr(&dbg_curr_thread->frames[nf].addr_frame);
    return TRUE;
}

BOOL stack_get_current_frame(IMAGEHLP_STACK_FRAME* ihsf)
{
    /*
     * If we don't have a valid backtrace, then just return.
     */
    if (dbg_curr_thread->frames == NULL) return FALSE;
    return stack_get_frame(dbg_curr_thread->curr_frame, ihsf);
}

BOOL stack_set_frame(int newframe)
{
    ADDRESS     addr;
    if (!stack_set_frame_internal(newframe)) return FALSE;
    addr.Mode = AddrModeFlat;
    addr.Offset = (unsigned long)memory_to_linear_addr(&dbg_curr_thread->frames[dbg_curr_thread->curr_frame].addr_pc);
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
    IMAGEHLP_STACK_FRAME        ihsf;
    DWORD64                     disp;

    if (!stack_get_current_frame(&ihsf)) return FALSE;
    return SymFromAddr(dbg_curr_process->handle, ihsf.InstructionOffset,
                       &disp, symbol);
}

static BOOL CALLBACK stack_read_mem(HANDLE hProc, DWORD addr, 
                                    PVOID buffer, DWORD size, PDWORD written)
{
    struct dbg_process* pcs = dbg_get_process_h(hProc);
    if (!pcs) return FALSE;
    return pcs->process_io->read(hProc, (const void*)addr, buffer, size, written);
}

/******************************************************************
 *		stack_fetch_frames
 *
 * Do a backtrace on the the current thread
 */
unsigned stack_fetch_frames(void)
{
    STACKFRAME  sf;
    unsigned    nf = 0;

    HeapFree(GetProcessHeap(), 0, dbg_curr_thread->frames);
    dbg_curr_thread->frames = NULL;

    memset(&sf, 0, sizeof(sf));
    memory_get_current_frame(&sf.AddrFrame);
    memory_get_current_pc(&sf.AddrPC);

    /* don't confuse StackWalk by passing in inconsistent addresses */
    if ((sf.AddrPC.Mode == AddrModeFlat) && (sf.AddrFrame.Mode != AddrModeFlat))
    {
        sf.AddrFrame.Offset = (DWORD)memory_to_linear_addr(&sf.AddrFrame);
        sf.AddrFrame.Mode = AddrModeFlat;
    }

    while (StackWalk(IMAGE_FILE_MACHINE_I386, dbg_curr_process->handle, 
                     dbg_curr_thread->handle, &sf, &dbg_context, stack_read_mem,
                     SymFunctionTableAccess, SymGetModuleBase, NULL))
    {
        dbg_curr_thread->frames = dbg_heap_realloc(dbg_curr_thread->frames, 
                                                   (nf + 1) * sizeof(dbg_curr_thread->frames[0]));

        dbg_curr_thread->frames[nf].addr_pc = sf.AddrPC;
        dbg_curr_thread->frames[nf].addr_frame = sf.AddrFrame;
        nf++;
        /* we've probably gotten ourselves into an infinite loop so bail */
        if (nf > 200) break;
    }
    dbg_curr_thread->curr_frame = -1;
    dbg_curr_thread->num_frames = nf;
    stack_set_frame_internal(0);
    return nf;
}

struct sym_enum
{
    char*       tmp;
    DWORD       frame;
};

static BOOL WINAPI sym_enum_cb(SYMBOL_INFO* sym_info, ULONG size, void* user)
{
    struct sym_enum*    se = (struct sym_enum*)user;
    char                tmp[32];

    if (sym_info->Flags & SYMFLAG_PARAMETER)
    {
        if (se->tmp[0]) strcat(se->tmp, ", ");
    
        if (sym_info->Flags & SYMFLAG_REGREL)
        {
            unsigned    val;
            DWORD       addr = se->frame + sym_info->Address;

            if (!dbg_read_memory((char*)addr, &val, sizeof(val)))
                snprintf(tmp, sizeof(tmp), "<*** cannot read at 0x%lx ***>", addr);
            else
                snprintf(tmp, sizeof(tmp), "0x%x", val);
        }
        else if (sym_info->Flags & SYMFLAG_REGISTER)
        {
            DWORD* pval;

            if (memory_get_register(sym_info->Register, &pval, tmp, sizeof(tmp)))
                snprintf(tmp, sizeof(tmp), "0x%lx", *pval);
        }
        sprintf(se->tmp + strlen(se->tmp), "%s=%s", sym_info->Name, tmp);
    }
    return TRUE;
}

static void stack_print_addr_and_args(int nf)
{
    char                        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)buffer;
    IMAGEHLP_STACK_FRAME        ihsf;
    IMAGEHLP_LINE               il;
    IMAGEHLP_MODULE             im;
    DWORD64                     disp64;

    print_bare_address(&dbg_curr_thread->frames[nf].addr_pc);

    stack_get_frame(nf, &ihsf);

    /* grab module where symbol is. If we don't have a module, we cannot print more */
    im.SizeOfStruct = sizeof(im);
    if (!SymGetModuleInfo(dbg_curr_process->handle, ihsf.InstructionOffset, &im))
        return;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    if (SymFromAddr(dbg_curr_process->handle, ihsf.InstructionOffset, &disp64, si))
    {
        struct sym_enum se;
        char            tmp[1024];
        DWORD           disp;

        dbg_printf(" %s", si->Name);
        if (disp64) dbg_printf("+0x%lx", (DWORD_PTR)disp64);

        SymSetContext(dbg_curr_process->handle, &ihsf, NULL);
        se.tmp = tmp;
        se.frame = ihsf.FrameOffset;
        tmp[0] = '\0';
        SymEnumSymbols(dbg_curr_process->handle, 0, NULL, sym_enum_cb, &se);
        if (tmp[0]) dbg_printf("(%s)", tmp);

        il.SizeOfStruct = sizeof(il);
        if (SymGetLineFromAddr(dbg_curr_process->handle, ihsf.InstructionOffset,
                               &disp, &il))
            dbg_printf(" [%s:%lu]", il.FileName, il.LineNumber);
        dbg_printf(" in %s", im.ModuleName);
    }
    else dbg_printf(" in %s (+0x%lx)", 
                    im.ModuleName, (DWORD_PTR)(ihsf.InstructionOffset - im.BaseOfImage));
}

/******************************************************************
 *		backtrace
 *
 * Do a backtrace on the the current thread
 */
static void backtrace(void)
{
    unsigned                    cf = dbg_curr_thread->curr_frame;
    IMAGEHLP_STACK_FRAME        ihsf;

    dbg_printf("Backtrace:\n");
    for (dbg_curr_thread->curr_frame = 0;
         dbg_curr_thread->curr_frame < dbg_curr_thread->num_frames;
         dbg_curr_thread->curr_frame++)
    {
        dbg_printf("%s%d ", 
                   (cf == dbg_curr_thread->curr_frame ? "=>" : "  "),
                   dbg_curr_thread->curr_frame + 1);
        stack_print_addr_and_args(dbg_curr_thread->curr_frame);
        dbg_printf(" (");
        print_bare_address(&dbg_curr_thread->frames[dbg_curr_thread->curr_frame].addr_pc);
        dbg_printf(")\n");
    }
    /* reset context to current stack frame */
    dbg_curr_thread->curr_frame = cf;
    stack_get_frame(dbg_curr_thread->curr_frame, &ihsf);
    SymSetContext(dbg_curr_process->handle, &ihsf, NULL);
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
            else backtrace();
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
        do
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId()) continue;
            if (dbg_curr_process && dbg_curr_pid != entry.th32OwnerProcessID)
                dbg_curr_process->process_io->close_process(dbg_curr_process, FALSE);

            if (entry.th32OwnerProcessID != dbg_curr_pid)
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
            backtrace_tid(dbg_curr_process, entry.th32ThreadID);
        }
        while (Thread32Next(snapshot, &entry));

        if (dbg_curr_process)
            dbg_curr_process->process_io->close_process(dbg_curr_process, FALSE);
    }
    CloseHandle(snapshot);
}

void stack_backtrace(DWORD tid)
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
        backtrace();
    }
    else
    {
        backtrace_tid(dbg_curr_process, tid);
    }
}
