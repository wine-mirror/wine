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
            "  attach <wpid>                          detach",
            "  break [*<addr>]                        watch | rwatch *<addr>",
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
            "  info (see 'help info' for options)     thread <tid>",

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
            "  info stack [<len>]   Dumps information about top of stack, up to len words",
            "  info symbol <sym>    Displays information about a given symbol",
            "  info thread          Shows all running threads",
            "  info wnd <handle>    Displays internal window state",
            "",
            NULL
        };

    while (infotext[i]) dbg_printf("%s\n", infotext[i++]);
}

struct info_module
{
    IMAGEHLP_MODULEW64                  mi;
    struct dhext_module_information     ext_module_info;
    char                                name[64];
};

struct info_modules
{
    struct info_module *modules;
    unsigned            num_alloc;
    unsigned            num_used;
};

static const char* get_module_type(const struct info_module* im, BOOL is_embedded)
{
    switch (im->ext_module_info.type)
    {
    case DMT_ELF:       return "ELF";
    case DMT_MACHO:     return "Mach-O";
    case DMT_PE:        return !is_embedded && im->ext_module_info.is_wine_builtin ? "PE-Wine" : "PE";
    default:            return "----";
    }
}

static const char* get_symtype_str(const struct info_module* im)
{
    switch (im->mi.SymType)
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
        if (im->ext_module_info.debug_format_bitmask)
        {
            static char tmp[64];
            tmp[0] = '\0';
            if (im->ext_module_info.debug_format_bitmask & DHEXT_FORMAT_STABS) strcpy(tmp, "stabs");
            if (im->ext_module_info.debug_format_bitmask & (DHEXT_FORMAT_DWARF2 | DHEXT_FORMAT_DWARF3 | DHEXT_FORMAT_DWARF4 | DHEXT_FORMAT_DWARF5))
            {
                if (tmp[0]) strcat(tmp, ", ");
                strcat(tmp, "Dwarf");
                if (im->ext_module_info.debug_format_bitmask & DHEXT_FORMAT_DWARF2) strcat(tmp, "-2");
                if (im->ext_module_info.debug_format_bitmask & DHEXT_FORMAT_DWARF3) strcat(tmp, "-3");
                if (im->ext_module_info.debug_format_bitmask & DHEXT_FORMAT_DWARF4) strcat(tmp, "-4");
                if (im->ext_module_info.debug_format_bitmask & DHEXT_FORMAT_DWARF5) strcat(tmp, "-5");
            }
            return tmp;
        }
        return "DIA";
    }
}

static const char* get_machine_str(DWORD machine)
{
    static char tmp[32];
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_AMD64: return "x86_64";
    case IMAGE_FILE_MACHINE_I386:  return "i386";
    case IMAGE_FILE_MACHINE_ARM64: return "arm64";
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_ARMNT: return "arm";
    default: sprintf(tmp, "<%lx>", machine); return tmp;
    }
}

static void module_print_info(const struct info_module *module, BOOL is_embedded, BOOL multi_machine)
{
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%s%s%s",
             is_embedded ? "  \\-" : "",
             get_module_type(module, is_embedded),
             module->ext_module_info.has_file_image ? "" : "^");

    if (multi_machine)
        dbg_printf("%-8s%16I64x-%16I64x       %-16s%-16s%s\n",
                   buffer,
                   module->mi.BaseOfImage,
                   module->mi.BaseOfImage + module->mi.ImageSize,
                   get_machine_str(module->mi.MachineType),
                   is_embedded ? "\\" : get_symtype_str(module), module->name);
    else
        dbg_printf("%-8s%*I64x-%*I64x       %-16s%s\n",
                   buffer,
                   ADDRWIDTH, module->mi.BaseOfImage,
                   ADDRWIDTH, module->mi.BaseOfImage + module->mi.ImageSize,
                   is_embedded ? "\\" : get_symtype_str(module), module->name);
}

static int __cdecl module_compare(const void* p1, const void* p2)
{
    struct info_module *left = (struct info_module *)p1;
    struct info_module *right = (struct info_module *)p2;
    LONGLONG val = left->mi.BaseOfImage - right->mi.BaseOfImage;

    if (val < 0) return -1;
    else if (val > 0) return 1;
    else return 0;
}

static inline BOOL module_is_container(const struct info_module *wmod_cntnr,
        const struct info_module *wmod_child)
{
    return (wmod_cntnr->ext_module_info.type == DMT_ELF || wmod_cntnr->ext_module_info.type == DMT_MACHO) &&
        (wmod_child->ext_module_info.type == DMT_PE) &&
        wmod_cntnr->mi.BaseOfImage <= wmod_child->mi.BaseOfImage &&
        wmod_cntnr->mi.BaseOfImage + wmod_cntnr->mi.ImageSize >=
        wmod_child->mi.BaseOfImage + wmod_child->mi.ImageSize;
}

static BOOL CALLBACK info_mod_cb(PCSTR mod_name, DWORD64 base, PVOID ctx)
{
    struct info_modules *im = ctx;

    if (im->num_used + 1 > im->num_alloc)
    {
        struct info_module* new = realloc(im->modules, (im->num_alloc + 16) * sizeof(*im->modules));
        if (!new) return FALSE; /* stop enumeration in case of OOM */
        im->num_alloc += 16;
        im->modules = new;
    }
    im->modules[im->num_used].mi.SizeOfStruct = sizeof(im->modules[im->num_used].mi);
    if (SymGetModuleInfoW64(dbg_curr_process->handle, base, &im->modules[im->num_used].mi) &&
        wine_get_module_information(dbg_curr_process->handle, base, &im->modules[im->num_used].ext_module_info,
                                    sizeof(im->modules[im->num_used].ext_module_info)))
    {
        const int dst_len = sizeof(im->modules[im->num_used].name);
        lstrcpynA(im->modules[im->num_used].name, mod_name, dst_len - 1);
        im->modules[im->num_used].name[dst_len - 1] = 0;
        im->num_used++;
    }
    return TRUE;
}

/***********************************************************************
 *           info_win32_module
 *
 * Display information about a given module (DLL or EXE), or about all modules
 */
void info_win32_module(DWORD64 base, BOOL multi_machine)
{
    struct info_modules im;
    UINT                i, j, num_printed = 0;
    BOOL                opt;
    DWORD               machine;
    BOOL                has_missing_filename = FALSE;

    if (!dbg_curr_process)
    {
        dbg_printf("Cannot get info on module while no process is loaded\n");
        return;
    }

    im.modules = NULL;
    im.num_alloc = im.num_used = 0;

    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    opt = SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    SymEnumerateModules64(dbg_curr_process->handle, info_mod_cb, &im);
    SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, opt);

    if (!im.num_used) return;

    /* main module is the first PE module in enumeration */
    for (i = 0; i < im.num_used; i++)
        if (im.modules[i].ext_module_info.type == DMT_PE)
        {
            machine = im.modules[i].mi.MachineType;
            break;
        }
    if (i == im.num_used) machine = IMAGE_FILE_MACHINE_UNKNOWN;
    qsort(im.modules, im.num_used, sizeof(im.modules[0]), module_compare);

    if (multi_machine)
        dbg_printf("%-8s%-40s%-16s%-16sName (%d modules)\n", "Module", "Address", "Machine", "Debug info", im.num_used);
    else
    {
        unsigned same_machine = 0;
        for (i = 0; i < im.num_used; i++)
            if (machine == im.modules[i].mi.MachineType) same_machine++;
        dbg_printf("%-8s%-*s%-16sName (%d modules",
                   "Module", ADDRWIDTH == 16 ? 40 : 24, "Address", "Debug info", same_machine);
        if (same_machine != im.num_used)
            dbg_printf(", %u for wow64 not listed", im.num_used - same_machine);
        dbg_printf(")\n");
    }

    for (i = 0; i < im.num_used; i++)
    {
        if (base &&
            (base < im.modules[i].mi.BaseOfImage || base >= im.modules[i].mi.BaseOfImage + im.modules[i].mi.ImageSize))
            continue;
        if (!multi_machine && machine != im.modules[i].mi.MachineType) continue;
        if (!im.modules[i].ext_module_info.has_file_image) has_missing_filename = TRUE;
        if (im.modules[i].ext_module_info.type == DMT_ELF || im.modules[i].ext_module_info.type == DMT_MACHO)
        {
            module_print_info(&im.modules[i], FALSE, multi_machine);
            /* print all modules embedded in this one */
            for (j = 0; j < im.num_used; j++)
            {
                if (module_is_container(&im.modules[i], &im.modules[j]))
                    module_print_info(&im.modules[j], TRUE, multi_machine);
            }
        }
        else
        {
            /* check module is not embedded in another module */
            for (j = 0; j < im.num_used; j++)
            {
                if (module_is_container(&im.modules[j], &im.modules[i]))
                    break;
            }
            if (j < im.num_used) continue;
            module_print_info(&im.modules[i], FALSE, multi_machine);
        }
        num_printed++;
    }
    free(im.modules);

    if (base && !num_printed)
        dbg_printf("'0x%0*I64x' is not a valid module address\n", ADDRWIDTH, base);
    if (has_missing_filename)
        dbg_printf("^ denotes modules for which image file couldn't be found\n");
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

    if (!GetClassNameA(hWnd, clsName, sizeof(clsName)))
        return;
    if ((atom = FindAtomA(clsName)) == 0)
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
            ATOM* new = realloc(cw->table, (cw->alloc + 16) * sizeof(ATOM));
            if (!new) return;
            cw->alloc += 16;
            cw->table = new;
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
    HINSTANCE   hInst = hWnd ? (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE) : 0;

    if (!name)
    {
        struct class_walker cw;

        cw.table = NULL;
        cw.used = cw.alloc = 0;
        class_walker(GetDesktopWindow(), &cw);
        free(cw.table);
        return;
    }

    if (!GetClassInfoExA(hInst, name, &wca))
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
        if (!GetClassNameA(hWnd, clsName, sizeof(clsName)))
            strcpy(clsName, "-- Unknown --");
        if (!GetWindowTextA(hWnd, wndName, sizeof(wndName)))
            strcpy(wndName, "-- Empty --");

        dbg_printf("%*s%08Ix%*s %-17.17s %08lx %0*Ix %08lx %.14s\n",
                   indent, "", (DWORD_PTR)hWnd, 12 - indent, "",
                   clsName, GetWindowLongW(hWnd, GWL_STYLE),
                   ADDRWIDTH, (ULONG_PTR)GetWindowLongPtrW(hWnd, GWLP_WNDPROC),
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
    WORD	w;

    if (!IsWindow(hWnd)) hWnd = GetDesktopWindow();

    if (!detailed)
    {
        dbg_printf("%-20.20s %-17.17s %-8.8s %-*.*s %-8.8s %s\n",
                   "Window handle", "Class Name", "Style",
		   ADDRWIDTH, ADDRWIDTH, "WndProc", "Thread", "Text");
        info_window(hWnd, 0);
        return;
    }

    if (!GetClassNameA(hWnd, clsName, sizeof(clsName)))
        strcpy(clsName, "-- Unknown --");
    if (!GetWindowTextA(hWnd, wndName, sizeof(wndName)))
        strcpy(wndName, "-- Empty --");
    if (!GetClientRect(hWnd, &clientRect) || 
        !MapWindowPoints(hWnd, 0, (LPPOINT) &clientRect, 2))
        SetRectEmpty(&clientRect);
    if (!GetWindowRect(hWnd, &windowRect))
        SetRectEmpty(&windowRect);

    /* FIXME missing fields: hmemTaskQ, hrgnUpdate, dce, flags, pProp, scroll */
    dbg_printf("next=%p  child=%p  parent=%p  owner=%p  class='%s'\n"
               "inst=%p  active=%p  idmenu=%08Ix\n"
               "style=0x%08lx  exstyle=0x%08lx  wndproc=%p  text='%s'\n"
               "client=%ld,%ld-%ld,%ld  window=%ld,%ld-%ld,%ld sysmenu=%p\n",
               GetWindow(hWnd, GW_HWNDNEXT),
               GetWindow(hWnd, GW_CHILD),
               GetParent(hWnd),
               GetWindow(hWnd, GW_OWNER),
               clsName,
               (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE),
               GetLastActivePopup(hWnd),
               (ULONG_PTR)GetWindowLongPtrW(hWnd, GWLP_ID),
               GetWindowLongW(hWnd, GWL_STYLE),
               GetWindowLongW(hWnd, GWL_EXSTYLE),
               (void*)GetWindowLongPtrW(hWnd, GWLP_WNDPROC),
               wndName,
               clientRect.left, clientRect.top, clientRect.right, clientRect.bottom,
               windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
               GetSystemMenu(hWnd, FALSE));

    if (GetClassLongW(hWnd, GCL_CBWNDEXTRA))
    {
        UINT i;

        dbg_printf("Extra bytes:");
        for (i = 0; i < GetClassLongW(hWnd, GCL_CBWNDEXTRA) / 2; i++)
        {
            w = GetWindowWord(hWnd, i * 2);
            /* FIXME: depends on i386 endian-ity */
            dbg_printf(" %02x %02x", HIBYTE(w), LOBYTE(w));
	}
        dbg_printf("\n");
    }
    dbg_printf("\n");
}

struct dump_proc_entry
{
    PROCESSENTRY32         proc;
    unsigned               children; /* index in dump_proc.entries of first child */
    unsigned               sibling;  /* index in dump_proc.entries of next sibling */
};

struct dump_proc
{
    struct dump_proc_entry*entries;
    unsigned               count;
    unsigned               alloc;
};

static unsigned get_parent(const struct dump_proc* dp, unsigned idx)
{
    unsigned i;

    for (i = 0; i < dp->count; i++)
    {
        if (i != idx && dp->entries[i].proc.th32ProcessID == dp->entries[idx].proc.th32ParentProcessID)
            return i;
    }
    return -1;
}

static void dump_proc_info(const struct dump_proc* dp, unsigned idx, unsigned depth)
{
    struct dump_proc_entry* dpe;
    char info;
    for ( ; idx != -1; idx = dp->entries[idx].sibling)
    {
        assert(idx < dp->count);
        dpe = &dp->entries[idx];
        if (dbg_curr_process && dpe->proc.th32ProcessID == dbg_curr_process->pid)
            info = '>';
        else if (dpe->proc.th32ProcessID == GetCurrentProcessId())
            info = '=';
        else
            info = ' ';
        dbg_printf("%c%08lx %-8ld ", info, dpe->proc.th32ProcessID, dpe->proc.cntThreads);
        if (depth)
        {
            unsigned i;
            for (i = 3 * (depth - 1); i > 0; i--) dbg_printf(" ");
            dbg_printf("\\_ ");
        }
        dbg_printf("'%s'\n", dpe->proc.szExeFile);
        dump_proc_info(dp, dpe->children, depth + 1);
    }
}

void info_win32_processes(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        struct dump_proc  dp;
        unsigned          i, first = -1;
        BOOL              ok;

        dp.count   = 0;
        dp.alloc   = 16;
        dp.entries = malloc(sizeof(*dp.entries) * dp.alloc);
        if (!dp.entries)
        {
             CloseHandle(snap);
             return;
        }
        dp.entries[dp.count].proc.dwSize = sizeof(dp.entries[dp.count].proc);
        ok = Process32First(snap, &dp.entries[dp.count].proc);

        /* fetch all process information into dp */
        while (ok)
        {
            dp.entries[dp.count++].children = -1;
            if (dp.count >= dp.alloc)
            {
                struct dump_proc_entry* new = realloc(dp.entries, sizeof(*dp.entries) * (dp.alloc * 2));
                if (!new)
                {
                    CloseHandle(snap);
                    free(dp.entries);
                    return;
                }
                dp.alloc *= 2;
                dp.entries = new;
            }
            dp.entries[dp.count].proc.dwSize = sizeof(dp.entries[dp.count].proc);
            ok = Process32Next(snap, &dp.entries[dp.count].proc);
        }
        CloseHandle(snap);
        /* chain the siblings wrt. their parent */
        for (i = 0; i < dp.count; i++)
        {
            unsigned parent = get_parent(&dp, i);
            unsigned *chain = parent == -1 ? &first : &dp.entries[parent].children;
            dp.entries[i].sibling = *chain;
            *chain = i;
        }
        dbg_printf(" %-8.8s %-8.8s %s (all id:s are in hex)\n", "pid", "threads", "executable");
        dump_proc_info(&dp, first, 0);
        free(dp.entries);
    }
}

static BOOL get_process_name(DWORD pid, PROCESSENTRY32W* entry)
{
    BOOL   ret = FALSE;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snap != INVALID_HANDLE_VALUE)
    {
        entry->dwSize = sizeof(*entry);
        if (Process32FirstW(snap, entry))
            while (!(ret = (entry->th32ProcessID == pid)) &&
                   Process32NextW(snap, entry));
        CloseHandle(snap);
    }
    return ret;
}

WCHAR* fetch_thread_description(DWORD tid)
{
    static HRESULT (WINAPI *my_GetThreadDescription)(HANDLE, PWSTR*) = NULL;
    static BOOL resolved = FALSE;
    HANDLE h;
    WCHAR* desc = NULL;

    if (!resolved)
    {
        HMODULE kernelbase = GetModuleHandleA("kernelbase.dll");
        if (kernelbase)
            my_GetThreadDescription = (void *)GetProcAddress(kernelbase, "GetThreadDescription");
        resolved = TRUE;
    }

    if (!my_GetThreadDescription)
        return NULL;

    h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!h)
        return NULL;

    my_GetThreadDescription(h, &desc);
    CloseHandle(h);

    if (desc && desc[0] == '\0')
    {
        LocalFree(desc);
        return NULL;
    }
    return desc;
}

void info_win32_threads(void)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32	entry;
        BOOL 		ok;
	DWORD		lastProcessId = 0;
        struct dbg_process* p = NULL;
        struct dbg_thread* t = NULL;
        WCHAR *description;

	entry.dwSize = sizeof(entry);
	ok = Thread32First(snap, &entry);

        dbg_printf("%-8.8s %-8.8s %s    %s (all IDs are in hex)\n",
                   "process", "tid", "prio", "name");
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
                    PROCESSENTRY32W pcs_entry;
                    const WCHAR* exename;

                    p = dbg_get_process(entry.th32OwnerProcessID);
                    if (p)
                        exename = p->imageName;
                    else if (get_process_name(entry.th32OwnerProcessID, &pcs_entry))
                        exename = pcs_entry.szExeFile;
                    else
                        exename = L"";

		    dbg_printf("%08lx%s %ls\n",
                               entry.th32OwnerProcessID, p ? " (D)" : "", exename);
                    lastProcessId = entry.th32OwnerProcessID;
		}
                dbg_printf("\t%08lx %4ld%s ",
                           entry.th32ThreadID, entry.tpBasePri,
                           (entry.th32ThreadID == dbg_curr_tid) ? " <==" : "    ");

                if ((description = fetch_thread_description(entry.th32ThreadID)))
                {
                    dbg_printf("%ls\n", description);
                    LocalFree(description);
                }
                else
                {
                    t = dbg_get_thread(p, entry.th32ThreadID);
                    dbg_printf("%s\n", t ? t->name : "");
                }
	    }
            ok = Thread32Next(snap, &entry);
        }

        CloseHandle(snap);
    }
}

/***********************************************************************
 *           info_win32_frame_exceptions
 *
 * Get info on the exception frames of a given thread.
 */
void info_win32_frame_exceptions(DWORD tid)
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
            dbg_printf("Unknown thread id (%04lx) in current process\n", tid);
            return;
        }
        if (SuspendThread(thread->handle) == -1)
        {
            dbg_printf("Can't suspend thread id (%04lx)\n", tid);
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
        if (!dbg_curr_process->process_io->get_selector(dbg_curr_thread->handle, (i << 3) | 7, &le))
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
        dbg_printf("%04lx: sel=%04lx base=%08x limit=%08x %d-bit %c%c%c\n",
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
            dbg_printf("Cannot open process <%04lx>\n", pid);
            return;
        }
    }

    dbg_printf("Address  End      State   Type    RWX\n");

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
        dbg_printf("%0*Ix %0*Ix %s %s %s\n",
                   ADDRWIDTH, (DWORD_PTR)addr, ADDRWIDTH, (DWORD_PTR)addr + mbi.RegionSize - 1, state, type, prot);
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

void info_win32_exception(void)
{
    const EXCEPTION_RECORD*     rec;
    ADDRESS64                   addr;
    char                        hexbuf[MAX_OFFSET_TO_STR_LEN];

    if (!dbg_curr_thread->in_exception)
    {
        dbg_printf("Thread isn't in an exception\n");
        return;
    }
    rec = &dbg_curr_thread->excpt_record;
    memory_get_current_pc(&addr);

    /* print some infos */
    dbg_printf("%s: ",
               dbg_curr_thread->first_chance ? "First chance exception" : "Unhandled exception");
    switch (rec->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
        dbg_printf("breakpoint");
        break;
    case EXCEPTION_SINGLE_STEP:
        dbg_printf("single step");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        dbg_printf("divide by zero");
        break;
    case EXCEPTION_INT_OVERFLOW:
        dbg_printf("overflow");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        dbg_printf("array bounds");
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        dbg_printf("illegal instruction");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        dbg_printf("stack overflow");
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        dbg_printf("privileged instruction");
        break;
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
            dbg_printf("page fault on %s access to 0x%0*Ix",
                       rec->ExceptionInformation[0] == EXCEPTION_WRITE_FAULT ? "write" :
                       rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT ? "execute" : "read",
                       ADDRWIDTH, rec->ExceptionInformation[1]);
        else
            dbg_printf("page fault");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        dbg_printf("Alignment");
        break;
    case DBG_CONTROL_C:
        dbg_printf("^C");
        break;
    case CONTROL_C_EXIT:
        dbg_printf("^C");
        break;
    case STATUS_POSSIBLE_DEADLOCK:
        {
            ADDRESS64       recaddr;

            recaddr.Mode   = AddrModeFlat;
            recaddr.Offset = rec->ExceptionInformation[0];

            dbg_printf("wait failed on critical section ");
            print_address(&recaddr, FALSE);
        }
        break;
    case EXCEPTION_WINE_STUB:
        {
            char dll[64], name[256];
            memory_get_string(dbg_curr_process,
                              (void*)rec->ExceptionInformation[0], TRUE, FALSE,
                              dll, sizeof(dll));
            if (HIWORD(rec->ExceptionInformation[1]))
                memory_get_string(dbg_curr_process,
                                  (void*)rec->ExceptionInformation[1], TRUE, FALSE,
                                  name, sizeof(name));
            else
                sprintf( name, "%Id", rec->ExceptionInformation[1] );
            dbg_printf("unimplemented function %s.%s called", dll, name);
        }
        break;
    case EXCEPTION_WINE_ASSERTION:
        dbg_printf("assertion failed");
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        dbg_printf("denormal float operand");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        dbg_printf("divide by zero");
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        dbg_printf("inexact float result");
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        dbg_printf("invalid float operation");
        break;
    case EXCEPTION_FLT_OVERFLOW:
        dbg_printf("floating point overflow");
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        dbg_printf("floating point underflow");
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        dbg_printf("floating point stack check");
        break;
    case EXCEPTION_WINE_CXX_EXCEPTION:
        if(rec->NumberParameters == 3 && rec->ExceptionInformation[0] == EXCEPTION_WINE_CXX_FRAME_MAGIC)
            dbg_printf("C++ exception(object = 0x%0*Ix, type = 0x%0*Ix)",
                       ADDRWIDTH, rec->ExceptionInformation[1], ADDRWIDTH, rec->ExceptionInformation[2]);
        else if(rec->NumberParameters == 4 && rec->ExceptionInformation[0] == EXCEPTION_WINE_CXX_FRAME_MAGIC)
            dbg_printf("C++ exception(object = %p, type = %p, base = %p)",
                       (void*)rec->ExceptionInformation[1], (void*)rec->ExceptionInformation[2],
                       (void*)rec->ExceptionInformation[3]);
        else
            dbg_printf("C++ exception with strange parameter count %ld or magic 0x%0*Ix",
                       rec->NumberParameters, ADDRWIDTH, rec->ExceptionInformation[0]);
        break;
    default:
        dbg_printf("0x%08lx", rec->ExceptionCode);
        break;
    }
    if (rec->ExceptionFlags & EH_STACK_INVALID)
        dbg_printf(", invalid program stack");

    switch (addr.Mode)
    {
    case AddrModeFlat:
        dbg_printf(" in %s%ld-bit code (%s)",
                   dbg_curr_process->is_wow64 ? "wow64 " : "",
                   dbg_curr_process->be_cpu->pointer_size * 8,
                   memory_offset_to_string(hexbuf, addr.Offset, 0));
        break;
    case AddrModeReal:
        dbg_printf(" in vm86 code (%04x:%04x)", addr.Segment, (unsigned) addr.Offset);
        break;
    case AddrMode1616:
        dbg_printf(" in 16-bit code (%04x:%04x)", addr.Segment, (unsigned) addr.Offset);
        break;
    case AddrMode1632:
        dbg_printf(" in segmented 32-bit code (%04x:%08x)", addr.Segment, (unsigned) addr.Offset);
        break;
    default: dbg_printf(" bad address");
    }
    dbg_printf(".\n");
}

static const struct
{
    int type;
    int platform;
    int major;
    int minor;
    const char *str;
}
version_table[] =
{
    { 0,                   VER_PLATFORM_WIN32s,        2,  0, "2.0" },
    { 0,                   VER_PLATFORM_WIN32s,        3,  0, "3.0" },
    { 0,                   VER_PLATFORM_WIN32s,        3, 10, "3.1" },
    { 0,                   VER_PLATFORM_WIN32_WINDOWS, 4,  0, "95" },
    { 0,                   VER_PLATFORM_WIN32_WINDOWS, 4, 10, "98" },
    { 0,                   VER_PLATFORM_WIN32_WINDOWS, 4, 90, "ME" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      3, 51, "NT 3.51" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      4,  0, "NT 4.0" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      5,  0, "2000" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      5,  1, "XP" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      5,  2, "XP" },
    { VER_NT_SERVER,       VER_PLATFORM_WIN32_NT,      5,  2, "Server 2003" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      6,  0, "Vista" },
    { VER_NT_SERVER,       VER_PLATFORM_WIN32_NT,      6,  0, "Server 2008" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      6,  1, "7" },
    { VER_NT_SERVER,       VER_PLATFORM_WIN32_NT,      6,  1, "Server 2008 R2" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      6,  2, "8" },
    { VER_NT_SERVER,       VER_PLATFORM_WIN32_NT,      6,  2, "Server 2012" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,      6,  3, "8.1" },
    { VER_NT_SERVER,       VER_PLATFORM_WIN32_NT,      6,  3, "Server 2012 R2" },
    { VER_NT_WORKSTATION,  VER_PLATFORM_WIN32_NT,     10,  0, "10" },
};

static const char *get_windows_version(void)
{
    RTL_OSVERSIONINFOEXW info = { sizeof(RTL_OSVERSIONINFOEXW) };
    static char str[64];
    int i;

    RtlGetVersion( &info );

    for (i = 0; i < ARRAY_SIZE(version_table); i++)
    {
        if (version_table[i].type == info.wProductType &&
            version_table[i].platform == info.dwPlatformId &&
            version_table[i].major == info.dwMajorVersion &&
            version_table[i].minor == info.dwMinorVersion)
        {
            return version_table[i].str;
        }
    }

    snprintf( str, sizeof(str), "%ld.%ld (%d)", info.dwMajorVersion,
              info.dwMinorVersion, info.wProductType );
    return str;
}

static BOOL is_guest(USHORT native, USHORT guest)
{
    BOOLEAN supported;

    return native != guest && !RtlWow64IsWowGuestMachineSupported(guest, &supported) && supported;
}

void info_win32_system(void)
{
    USHORT current, native;
    int i, count;

    const char *(CDECL *wine_get_build_id)(void);
    void (CDECL *wine_get_host_version)( const char **sysname, const char **release );

    static USHORT guest_machines[] =
    {
        IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_ARM, IMAGE_FILE_MACHINE_ARMNT,
    };

    wine_get_build_id = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_build_id");
    wine_get_host_version = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_host_version");

    RtlWow64GetProcessMachines( GetCurrentProcess(), &current, &native );

    dbg_printf( "System information:\n" );
    if (wine_get_build_id) dbg_printf( "    Wine build: %s\n", wine_get_build_id() );
    dbg_printf( "    Platform: %s", get_machine_str(native));
    for (count = i = 0; i < ARRAY_SIZE(guest_machines); i++)
    {
        if (is_guest(native, guest_machines[i]))
        {
            if (!count++) dbg_printf(" (guest:");
            dbg_printf(" %s", get_machine_str(guest_machines[i]));
        }
    }
    dbg_printf("%s\n", count ? ")" : "");

    dbg_printf( "    Version: Windows %s\n", get_windows_version() );
    if (wine_get_host_version)
    {
        const char *sysname, *release;
        wine_get_host_version( &sysname, &release );
        dbg_printf( "    Host system: %s\n", sysname );
        dbg_printf( "    Host version: %s\n", release );
    }
}
