/*
 * Wine debugger utility routines
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
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

/***********************************************************************
 *           print_help
 *
 * Implementation of the 'help' command.
 */
void print_help(void)
{
    int i = 0;
    static const char * const helptext[] =
        {
            "The commands accepted by the Wine debugger are a reasonable",
            "subset of the commands that gdb accepts.",
            "The commands currently are:",
            "  help                                   quit",
            "  break [*<addr>]                        watch *<addr>",
            "  delete break bpnum                     disable bpnum",
            "  enable bpnum                           condition <bpnum> [<expr>]",
            "  finish                                 cont [N]",
            "  step [N]                               next [N]",
            "  stepi [N]                              nexti [N]",
            "  x <addr>                               print <expr>",
            "  display <expr>                         undisplay <disnum>",
            "  local display <expr>                   delete display <disnum>",                  
            "  enable display <disnum>                disable display <disnum>",
            "  bt [<tid>|all]                         frame <n>",
            "  up                                     down",
            "  list <lines>                           disassemble [<addr>][,<addr>]",
            "  show dir                               dir <path>",
            "  set <reg> = <expr>                     set *<addr> = <expr>",
            "  pass                                   whatis",
            "  info (see 'help info' for options)",

            "The 'x' command accepts repeat counts and formats (including 'i') in the",
            "same way that gdb does.\n",

            "The following are examples of legal expressions:",
            " $eax     $eax+0x3   0x1000   ($eip + 256)  *$eax   *($esp + 3)",
            " Also, a nm format symbol table can be read from a file using the",
            " symbolfile command.", /*  Symbols can also be defined individually with",
                                        " the define command.", */
            "",
            NULL
        };

    while (helptext[i]) dbg_printf("%s\n", helptext[i++]);
}


/***********************************************************************
 *           info_help
 *
 * Implementation of the 'help info' command.
 */
void info_help(void)
{
    int i = 0;
    static const char * const infotext[] =
        {
            "The info commands allow you to get assorted bits of interesting stuff",
            "to be displayed.  The options are:",
            "  info break           Displays information about breakpoints",
            "  info class <name>    Displays information about window class <name>",
            "  info display         Shows auto-display expressions in use",
            "  info except <pid>    Shows exception handler chain (in a given process)",
            "  info locals          Displays values of all local vars for current frame",
            "  info maps <pid>      Shows virtual mappings (in a given process)",
            "  info process         Shows all running processes",
            "  info reg             Displays values of the general registers at top of stack",
            "  info all-reg         Displays the general and floating point registers",
            "  info segments <pid>  Displays information about all known segments",
            "  info share           Displays all loaded modules",
            "  info share <addr>    Displays internal module state",
            "  info stack           Dumps information about top of stack",
            "  info symbol <sym>    Displays information about a given symbol",
            "  info thread          Shows all running threads",
            "  info wnd <handle>    Displays internal window state",
            "",
            NULL
        };

    while (infotext[i]) dbg_printf("%s\n", infotext[i++]);
}

static const char* get_symtype_str(const IMAGEHLP_MODULE64* mi)
{
    switch (mi->SymType)
    {
    default:
    case SymNone:       return "--none--";
    case SymCoff:       return "COFF";
    case SymCv:         return "CodeView";
    case SymPdb:        return "PDB";
    case SymExport:     return "Export";
    case SymDeferred:   return "Deferred";
    case SymSym:        return "Sym";
    case SymDia:
        switch (mi->CVSig)
        {
        case 'S' | ('T' << 8) | ('A' << 16) | ('B' << 24):
            return "Stabs";
        case 'D' | ('W' << 8) | ('A' << 16) | ('R' << 24):
            return "Dwarf";
        default:
            return "DIA";

        }
    }
}

struct info_module
{
    IMAGEHLP_MODULE64*  mi;
    DWORD               base;
    unsigned            num_alloc;
    unsigned            num_used;
};

static void module_print_info(const IMAGEHLP_MODULE64* mi, BOOL is_embedded)
{
    dbg_printf("%8s-%8s\t%-16s%s\n",
               wine_dbgstr_longlong(mi->BaseOfImage),
               wine_dbgstr_longlong(mi->BaseOfImage + mi->ImageSize),
               is_embedded ? "\\" : get_symtype_str(mi), mi->ModuleName);
}

static int      module_compare(const void* p1, const void* p2)
{
    LONGLONG val = ((const IMAGEHLP_MODULE64*)p1)->BaseOfImage -
        ((const IMAGEHLP_MODULE64*)p2)->BaseOfImage;
    if (val < 0) return -1;
    else if (val > 0) return 1;
    else return 0;
}

static inline BOOL module_is_container(const IMAGEHLP_MODULE64* wmod_cntnr,
                                       const IMAGEHLP_MODULE64* wmod_child)
{
    return wmod_cntnr->BaseOfImage <= wmod_child->BaseOfImage &&
        wmod_cntnr->BaseOfImage + wmod_cntnr->ImageSize >=
        wmod_child->BaseOfImage + wmod_child->ImageSize;
}

static BOOL CALLBACK info_mod_cb(PCSTR mod_name, DWORD64 base, PVOID ctx)
{
    struct info_module* im = (struct info_module*)ctx;

    if (im->num_used + 1 > im->num_alloc)
    {
        im->num_alloc += 16;
        im->mi = dbg_heap_realloc(im->mi, im->num_alloc * sizeof(*im->mi));
    }
    im->mi[im->num_used].SizeOfStruct = sizeof(im->mi[im->num_used]);
    if (SymGetModuleInfo64(dbg_curr_process->handle, base, &im->mi[im->num_used]))
    {
        im->num_used++;
    }   
    return TRUE;
}

/***********************************************************************
 *           info_win32_module
 *
 * Display information about a given module (DLL or EXE), or about all modules
 */
void info_win32_module(DWORD64 base)
{
    struct info_module  im;
    int                 i, j, num_printed = 0;
    DWORD               opt;

    if (!dbg_curr_process)
    {
        dbg_printf("Cannot get info on module while no process is loaded\n");
        return;
    }

    im.mi = NULL;
    im.num_alloc = im.num_used = 0;

    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    SymSetOptions((opt = SymGetOptions()) | 0x40000000);
    SymEnumerateModules64(dbg_curr_process->handle, info_mod_cb, (void*)&im);
    SymSetOptions(opt);

    qsort(im.mi, im.num_used, sizeof(im.mi[0]), module_compare);

    dbg_printf("Module\tAddress\t\t\tDebug info\tName (%d modules)\n", im.num_used);

    for (i = 0; i < im.num_used; i++)
    {
        if (base && 
            (base < im.mi[i].BaseOfImage || base >= im.mi[i].BaseOfImage + im.mi[i].ImageSize))
            continue;
        if (strstr(im.mi[i].ModuleName, "<elf>"))
        {
            dbg_printf("ELF\t");
            module_print_info(&im.mi[i], FALSE);
            /* print all modules embedded in this one */
            for (j = 0; j < im.num_used; j++)
            {
                if (!strstr(im.mi[j].ModuleName, "<elf>") && module_is_container(&im.mi[i], &im.mi[j]))
                {
                    dbg_printf("  \\-PE\t");
                    module_print_info(&im.mi[j], TRUE);
                }
            }
        }
        else
        {
            /* check module is not embedded in another module */
            for (j = 0; j < im.num_used; j++) 
            {
                if (strstr(im.mi[j].ModuleName, "<elf>") && module_is_container(&im.mi[j], &im.mi[i]))
                    break;
            }
            if (j < im.num_used) continue;
            if (strstr(im.mi[i].ModuleName, ".so") || strchr(im.mi[i].ModuleName, '<'))
                dbg_printf("ELF\t");
            else
                dbg_printf("PE\t");
            module_print_info(&im.mi[i], FALSE);
        }
        num_printed++;
    }
    HeapFree(GetProcessHeap(), 0, im.mi);

    if (base && !num_printed)
        dbg_printf("'0x%x%08x' is not a valid module address\n", (DWORD)(base >> 32), (DWORD)base);
}

struct class_walker
{
    ATOM*	table;
    int		used;
    int		alloc;
};

static void class_walker(HWND hWnd, struct class_walker* cw)
{
    char	clsName[128];
    int	        i;
    ATOM	atom;
    HWND	child;

    if (!GetClassName(hWnd, clsName, sizeof(clsName)))
        return;
    if ((atom = FindAtom(clsName)) == 0)
        return;

    for (i = 0; i < cw->used; i++)
    {
        if (cw->table[i] == atom)
            break;
    }
    if (i == cw->used)
    {
        if (cw->used >= cw->alloc)
        {
            cw->alloc += 16;
            cw->table = dbg_heap_realloc(cw->table, cw->alloc * sizeof(ATOM));
        }
        cw->table[cw->used++] = atom;
        info_win32_class(hWnd, clsName);
    }
    do
    {
        if ((child = GetWindow(hWnd, GW_CHILD)) != 0)
            class_walker(child, cw);
    } while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT)) != 0);
}

void info_win32_class(HWND hWnd, const char* name)
{
    WNDCLASSEXA	wca;
    HINSTANCE   hInst = hWnd ? (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE) : 0;

    if (!name)
    {
        struct class_walker cw;

        cw.table = NULL;
        cw.used = cw.alloc = 0;
        class_walker(GetDesktopWindow(), &cw);
        HeapFree(GetProcessHeap(), 0, cw.table);
        return;
    }

    if (!GetClassInfoEx(hInst, name, &wca))
    {
        dbg_printf("Cannot find class '%s'\n", name);
        return;
    }

    dbg_printf("Class '%s':\n", name);
    dbg_printf("style=0x%08x  wndProc=%p\n"
               "inst=%p  icon=%p  cursor=%p  bkgnd=%p\n"
               "clsExtra=%d  winExtra=%d\n",
               wca.style, wca.lpfnWndProc, wca.hInstance,
               wca.hIcon, wca.hCursor, wca.hbrBackground,
               wca.cbClsExtra, wca.cbWndExtra);

    if (hWnd && wca.cbClsExtra)
    {
        int		i;
        WORD		w;

        dbg_printf("Extra bytes:");
        for (i = 0; i < wca.cbClsExtra / 2; i++)
        {
            w = GetClassWord(hWnd, i * 2);
            /* FIXME: depends on i386 endian-ity */
            dbg_printf(" %02x %02x", HIBYTE(w), LOBYTE(w));
        }
        dbg_printf("\n");
    }
    dbg_printf("\n");
    /* FIXME:
     * + print #windows (or even list of windows...)
     * + print extra bytes => this requires a window handle on this very class...
     */
}

static void info_window(HWND hWnd, int indent)
{
    char	clsName[128];
    char	wndName[128];
    HWND	child;

    do
    {
        if (!GetClassName(hWnd, clsName, sizeof(clsName)))
            strcpy(clsName, "-- Unknown --");
        if (!GetWindowText(hWnd, wndName, sizeof(wndName)))
            strcpy(wndName, "-- Empty --");

        dbg_printf("%*s%08lx%*s %-17.17s %08x %08x %08x %.14s\n",
                   indent, "", (DWORD_PTR)hWnd, 12 - indent, "",
                   clsName, GetWindowLong(hWnd, GWL_STYLE),
                   GetWindowLongPtr(hWnd, GWLP_WNDPROC),
                   GetWindowThreadProcessId(hWnd, NULL), wndName);

        if ((child = GetWindow(hWnd, GW_CHILD)) != 0)
            info_window(child, indent + 1);
    } while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT)) != 0);
}

void info_win32_window(HWND hWnd, BOOL detailed)
{
    char	clsName[128];
    char	wndName[128];
    RECT	clientRect;
    RECT	windowRect;
    int         i;
    WORD	w;

    if (!IsWindow(hWnd)) hWnd = GetDesktopWindow();

    if (!detailed)
    {
        dbg_printf("%-20.20s %-17.17s %-8.8s %-8.8s %-8.8s %s\n",
                   "Window handle", "Class Name", "Style", "WndProc", "Thread", "Text");
        info_window(hWnd, 0);
        return;
    }

    if (!GetClassName(hWnd, clsName, sizeof(clsName)))
        strcpy(clsName, "-- Unknown --");
    if (!GetWindowText(hWnd, wndName, sizeof(wndName)))
        strcpy(wndName, "-- Empty --");
    if (!GetClientRect(hWnd, &clientRect) || 
        !MapWindowPoints(hWnd, 0, (LPPOINT) &clientRect, 2))
        SetRectEmpty(&clientRect);
    if (!GetWindowRect(hWnd, &windowRect))
        SetRectEmpty(&windowRect);

    /* FIXME missing fields: hmemTaskQ, hrgnUpdate, dce, flags, pProp, scroll */
    dbg_printf("next=%p  child=%p  parent=%p  owner=%p  class='%s'\n"
               "inst=%p  active=%p  idmenu=%08x\n"
               "style=0x%08x  exstyle=0x%08x  wndproc=0x%08x  text='%s'\n"
               "client=%d,%d-%d,%d  window=%d,%d-%d,%d sysmenu=%p\n",
               GetWindow(hWnd, GW_HWNDNEXT),
               GetWindow(hWnd, GW_CHILD),
               GetParent(hWnd),
               GetWindow(hWnd, GW_OWNER),
               clsName,
               (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
               GetLastActivePopup(hWnd),
               GetWindowLongPtr(hWnd, GWLP_ID),
               GetWindowLong(hWnd, GWL_STYLE),
               GetWindowLong(hWnd, GWL_EXSTYLE),
               GetWindowLongPtr(hWnd, GWLP_WNDPROC),
               wndName,
               clientRect.left, clientRect.top, clientRect.right, clientRect.bottom,
               windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
               GetSystemMenu(hWnd, FALSE));

    if (GetClassLong(hWnd, GCL_CBWNDEXTRA))
    {
        dbg_printf("Extra bytes:");
        for (i = 0; i < GetClassLong(hWnd, GCL_CBWNDEXTRA) / 2; i++)
        {
            w = GetWindowWord(hWnd, i * 2);
            /* FIXME: depends on i386 endian-ity */
            dbg_printf(" %02x %02x", HIBYTE(w), LOBYTE(w));
	}
        dbg_printf("\n");
    }
    dbg_printf("\n");
}

void info_win32_processes(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32  entry;
        DWORD           current = dbg_curr_process ? dbg_curr_process->pid : 0;
        BOOL            ok;

        entry.dwSize = sizeof(entry);
        ok = Process32First(snap, &entry);

        dbg_printf(" %-8.8s %-8.8s %-8.8s %s (all id:s are in hex)\n",
                   "pid", "threads", "parent", "executable");
        while (ok)
        {
            if (entry.th32ProcessID != GetCurrentProcessId())
                dbg_printf("%c%08x %-8d %08x '%s'\n",
                           (entry.th32ProcessID == current) ? '>' : ' ',
                           entry.th32ProcessID, entry.cntThreads,
                           entry.th32ParentProcessID, entry.szExeFile);
            ok = Process32Next(snap, &entry);
        }
        CloseHandle(snap);
    }
}

void info_win32_threads(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32	entry;
        BOOL 		ok;
	DWORD		lastProcessId = 0;

	entry.dwSize = sizeof(entry);
	ok = Thread32First(snap, &entry);

        dbg_printf("%-8.8s %-8.8s %s (all id:s are in hex)\n",
                   "process", "tid", "prio");
        while (ok)
        {
            if (entry.th32OwnerProcessID != GetCurrentProcessId())
	    {
		/* FIXME: this assumes that, in the snapshot, all threads of a same process are
		 * listed sequentially, which is not specified in the doc (Wine's implementation
		 * does it)
		 */
		if (entry.th32OwnerProcessID != lastProcessId)
		{
		    struct dbg_process*	p = dbg_get_process(entry.th32OwnerProcessID);

		    dbg_printf("%08x%s %s\n",
                               entry.th32OwnerProcessID, p ? " (D)" : "", p ? p->imageName : "");
		    lastProcessId = entry.th32OwnerProcessID;
		}
                dbg_printf("\t%08x %4d%s\n",
                           entry.th32ThreadID, entry.tpBasePri,
                           (entry.th32ThreadID == dbg_curr_tid) ? " <==" : "");

	    }
            ok = Thread32Next(snap, &entry);
        }

        CloseHandle(snap);
    }
}

/***********************************************************************
 *           info_win32_exceptions
 *
 * Get info on the exception frames of a given thread.
 */
void info_win32_exceptions(DWORD tid)
{
    struct dbg_thread*  thread;
    void*               next_frame;

    if (!dbg_curr_process || !dbg_curr_thread)
    {
        dbg_printf("Cannot get info on exceptions while no process is loaded\n");
        return;
    }

    dbg_printf("Exception frames:\n");

    if (tid == dbg_curr_tid) thread = dbg_curr_thread;
    else
    {
        thread = dbg_get_thread(dbg_curr_process, tid);

        if (!thread)
        {
            dbg_printf("Unknown thread id (%04x) in current process\n", tid);
            return;
        }
        if (SuspendThread(thread->handle) == -1)
        {
            dbg_printf("Can't suspend thread id (%04x)\n", tid);
            return;
        }
    }

    if (!dbg_read_memory(thread->teb, &next_frame, sizeof(next_frame)))
    {
        dbg_printf("Can't read TEB:except_frame\n");
        return;
    }

    while (next_frame != (void*)-1)
    {
        EXCEPTION_REGISTRATION_RECORD frame;

        dbg_printf("%p: ", next_frame);
        if (!dbg_read_memory(next_frame, &frame, sizeof(frame)))
        {
            dbg_printf("Invalid frame address\n");
            break;
        }
        dbg_printf("prev=%p handler=%p\n", frame.Prev, frame.Handler);
        next_frame = frame.Prev;
    }

    if (tid != dbg_curr_tid) ResumeThread(thread->handle);
}

void info_win32_segments(DWORD start, int length)
{
    char 	flags[3];
    DWORD 	i;
    LDT_ENTRY	le;

    if (length == -1) length = (8192 - start);

    for (i = start; i < start + length; i++)
    {
        if (!GetThreadSelectorEntry(dbg_curr_thread->handle, (i << 3) | 7, &le))
            continue;

        if (le.HighWord.Bits.Type & 0x08)
        {
            flags[0] = (le.HighWord.Bits.Type & 0x2) ? 'r' : '-';
            flags[1] = '-';
            flags[2] = 'x';
        }
        else
        {
            flags[0] = 'r';
            flags[1] = (le.HighWord.Bits.Type & 0x2) ? 'w' : '-';
            flags[2] = '-';
        }
        dbg_printf("%04x: sel=%04x base=%08x limit=%08x %d-bit %c%c%c\n",
                   i, (i << 3) | 7,
                   (le.HighWord.Bits.BaseHi << 24) +
                   (le.HighWord.Bits.BaseMid << 16) + le.BaseLow,
                   ((le.HighWord.Bits.LimitHi << 8) + le.LimitLow) <<
                   (le.HighWord.Bits.Granularity ? 12 : 0),
                   le.HighWord.Bits.Default_Big ? 32 : 16,
                   flags[0], flags[1], flags[2]);
    }
}

void info_win32_virtual(DWORD pid)
{
    MEMORY_BASIC_INFORMATION    mbi;
    char*                       addr = 0;
    const char*                 state;
    const char*                 type;
    char                        prot[3+1];
    HANDLE                      hProc;

    if (pid == dbg_curr_pid)
    {
        if (dbg_curr_process == NULL)
        {
            dbg_printf("Cannot look at mapping of current process, while no process is loaded\n");
            return;
        }
        hProc = dbg_curr_process->handle;
    }
    else
    {
        hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProc == NULL)
        {
            dbg_printf("Cannot open process <%04x>\n", pid);
            return;
        }
    }

    dbg_printf("Address  Size     State   Type    RWX\n");

    while (VirtualQueryEx(hProc, addr, &mbi, sizeof(mbi)) >= sizeof(mbi))
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
            memset(prot, ' ' , sizeof(prot) - 1);
            prot[sizeof(prot) - 1] = '\0';
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
        dbg_printf("%08lx %08lx %s %s %s\n",
                   (DWORD_PTR)addr, (DWORD_PTR)addr + mbi.RegionSize - 1, state, type, prot);
        if (addr + mbi.RegionSize < addr) /* wrap around ? */
            break;
        addr += mbi.RegionSize;
    }
    if (pid != dbg_curr_pid) CloseHandle(hProc);
}

void info_wine_dbg_channel(BOOL turn_on, const char* cls, const char* name)
{
    struct dbg_lvalue           lvalue;
    struct __wine_debug_channel channel;
    unsigned char               mask;
    int                         done = 0;
    BOOL                        bAll;
    void*                       addr;

    if (!dbg_curr_process || !dbg_curr_thread)
    {
        dbg_printf("Cannot set/get debug channels while no process is loaded\n");
        return;
    }

    if (symbol_get_lvalue("debug_options", -1, &lvalue, FALSE) != sglv_found)
    {
        return;
    }
    addr = memory_to_linear_addr(&lvalue.addr);

    if (!cls)                          mask = ~0;
    else if (!strcmp(cls, "fixme"))    mask = (1 << __WINE_DBCL_FIXME);
    else if (!strcmp(cls, "err"))      mask = (1 << __WINE_DBCL_ERR);
    else if (!strcmp(cls, "warn"))     mask = (1 << __WINE_DBCL_WARN);
    else if (!strcmp(cls, "trace"))    mask = (1 << __WINE_DBCL_TRACE);
    else
    {
        dbg_printf("Unknown debug class %s\n", cls);
        return;
    }

    bAll = !strcmp("all", name);
    while (addr && dbg_read_memory(addr, &channel, sizeof(channel)))
    {
        if (!channel.name[0]) break;
        if (bAll || !strcmp( channel.name, name ))
        {
            if (turn_on) channel.flags |= mask;
            else channel.flags &= ~mask;
            if (dbg_write_memory(addr, &channel, sizeof(channel))) done++;
        }
        addr = (struct __wine_debug_channel *)addr + 1;
    }
    if (!done) dbg_printf("Unable to find debug channel %s\n", name);
    else WINE_TRACE("Changed %d channel instances\n", done);
}
