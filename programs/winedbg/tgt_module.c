/*
 * Wine debugger - loading a module for debug purposes
 *
 * Copyright 2006 Eric Pouech
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
#include <string.h>
#include <stdarg.h>

#include "debugger.h"


static struct be_process_io be_process_module_io;

static BOOL tgt_process_module_read(HANDLE hProcess, const void* addr,
                                    void* buffer, SIZE_T len, SIZE_T* rlen)
{
    return FALSE;
}

static BOOL tgt_process_module_write(HANDLE hProcess, void* addr,
                                     const void* buffer, SIZE_T len, SIZE_T* wlen)
{
    return FALSE;
}

enum dbg_start tgt_module_load(const char* name, BOOL keep)
{
    DWORD opts = SymGetOptions();
    BOOL native;
    HANDLE hDummy = (HANDLE)0x87654321;
    enum dbg_start ret = start_ok;
    WCHAR* nameW;
    unsigned len;

    SymSetOptions((opts & ~(SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS)) |
                  SYMOPT_LOAD_LINES | SYMOPT_AUTO_PUBLICS);
    native = SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    if (!dbg_init(hDummy, NULL, FALSE))
        return start_error_init;
    len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    nameW = malloc(len * sizeof(WCHAR));
    if (!nameW)
    {
        ret = start_error_init;
        keep = FALSE;
    }
    else
    {
        len = MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, len);
        if (!dbg_load_module(hDummy, NULL, nameW, 0, 0))
        {
            ret = start_error_init;
            keep = FALSE;
        }
        free(nameW);
    }

    if (keep)
    {
        dbg_printf("Non supported mode... errors may occur\n"
                   "Use at your own risks\n");
        SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
        dbg_curr_process = dbg_add_process(&be_process_module_io, 1, hDummy);
        dbg_curr_pid = 1;
        dbg_curr_thread = dbg_add_thread(dbg_curr_process, 2, NULL, NULL);

        /* FIXME: missing thread creation, fetching frames, restoring dbghelp's options... */
    }
    else
    {
        SymCleanup(hDummy);
        SymSetOptions(opts);
        SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, native);
    }

    return ret;
}

static BOOL tgt_process_module_close_process(struct dbg_process* pcs, BOOL kill)
{
    SymCleanup(pcs->handle);
    dbg_del_process(pcs);
    return TRUE;
}

static BOOL tgt_process_module_get_selector(HANDLE hThread, DWORD sel, LDT_ENTRY* le)
{
    return FALSE;
}

static struct be_process_io be_process_module_io =
{
    tgt_process_module_close_process,
    tgt_process_module_read,
    tgt_process_module_write,
    tgt_process_module_get_selector,
};
