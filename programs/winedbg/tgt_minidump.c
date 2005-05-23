/*
 * Wine debugger - minidump handling
 *
 * Copyright 2005 Eric Pouech
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "debugger.h"
#include "wingdi.h"
#include "winuser.h"
#include "tlhelp32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

void minidump_write(const char* file, const EXCEPTION_RECORD* rec)
{
    HANDLE                              hFile;
    MINIDUMP_EXCEPTION_INFORMATION      mei;
    EXCEPTION_POINTERS                  ep;
    DWORD                               wine_opt;

    hFile = CreateFile(file, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return;

    if (rec)
    {
        mei.ThreadId = dbg_curr_thread->tid;
        mei.ExceptionPointers = &ep;
        ep.ExceptionRecord = (EXCEPTION_RECORD*)rec;
        ep.ContextRecord = &dbg_context;
        mei.ClientPointers = FALSE;
    }
    /* this is a wine specific options to return also ELF modules in the
     * dumping
     */
    SymSetOptions((wine_opt = SymGetOptions()) | 0x40000000);
    MiniDumpWriteDump(dbg_curr_process->handle, dbg_curr_process->pid,
                      hFile, MiniDumpNormal/*|MiniDumpWithDataSegs*/,
                      rec ? &mei : NULL, NULL, NULL);
    SymSetOptions(wine_opt);
    CloseHandle(hFile);
}
