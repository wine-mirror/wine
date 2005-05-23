/*
 * Generate hash tables for Wine debugger symbols
 *
 * Copyright (C) 1993, Eric Youngdale.
 *               2004, Eric Pouech.
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

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

static BOOL symbol_get_debug_start(DWORD mod_base, DWORD typeid, ULONG64* start)
{
    DWORD                       count, tag;
    char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
    TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
    int                         i;
    struct dbg_type             type;

    type.module = mod_base;
    type.id = typeid;

    if (!types_get_info(&type, TI_GET_CHILDRENCOUNT, &count)) return FALSE;
    fcp->Start = 0;
    while (count)
    {
        fcp->Count = min(count, 256);
        if (types_get_info(&type, TI_FINDCHILDREN, fcp))
        {
            for (i = 0; i < min(fcp->Count, count); i++)
            {
                type.id = fcp->ChildId[i];
                types_get_info(&type, TI_GET_SYMTAG, &tag);
                if (tag != SymTagFuncDebugStart) continue;
                return types_get_info(&type, TI_GET_ADDRESS, start);
            }
            count -= min(count, 256);
            fcp->Start += 256;
        }
    }
    return FALSE;
}

struct sgv_data
{
#define NUMDBGV                 100
    struct {
        /* FIXME: NUMDBGV should be made variable */
        struct dbg_lvalue               lvalue;
        DWORD                           flags;
    }                           syms[NUMDBGV];  /* out     : will be filled in with various found symbols */
    int		                num;            /* out     : number of found symbols */
    int                         num_thunks;     /* out     : number of thunks found */
    const char*                 name;           /* in      : name of symbol to look up */
    const char*                 filename;       /* in (opt): filename where to look up symbol */
    int                         lineno;         /* in (opt): line number in filename where to look up symbol */
    unsigned                    bp_disp : 1,    /* in      : whether if we take into account func address or func first displayable insn */
                                do_thunks : 1;  /* in      : whether we return thunks tags */
    IMAGEHLP_STACK_FRAME        ihsf;           /* in      : frame for local & parameter variables look up */
};

static BOOL CALLBACK sgv_cb(SYMBOL_INFO* sym, ULONG size, void* ctx)
{
    struct sgv_data*    sgv = (struct sgv_data*)ctx;
    ULONG64             addr;
    IMAGEHLP_LINE       il;
    unsigned            cookie = DLV_TARGET, insp;

    if (sym->Flags & SYMFLAG_REGISTER)
    {
        const struct dbg_internal_var*  div;

        if (dbg_curr_frame != 0)
        {
            dbg_printf(" %s (register): << cannot display, not in correct frame\n",
                       sym->Name);
            return TRUE;
        }
        for (div = dbg_context_vars; div->name && div->val != sym->Register; div++);
        if (!div->name)
        {
            dbg_printf(" %s (register): couldn't find register %lu\n",
                       sym->Name, sym->Register);
            return TRUE;
        }
        addr = (ULONG64)(DWORD_PTR)div->pval;
        cookie = DLV_HOST;
    }
    else if (sym->Flags & SYMFLAG_FRAMEREL)
    {
        ULONG   offset;
        struct dbg_type type;

        type.module = sym->ModBase;
        type.id = sym->TypeIndex;
        types_get_info(&type, TI_GET_OFFSET, &offset);
        addr = sgv->ihsf.FrameOffset + offset;
    }
    else if (sym->Flags & SYMFLAG_THUNK)
    {
        if (!sgv->do_thunks) return TRUE;
        sgv->num_thunks++;
        addr = sym->Address;
    }
    else
    {
        DWORD disp;
        il.SizeOfStruct = sizeof(il);
        SymGetLineFromAddr(dbg_curr_process->handle, sym->Address, &disp, &il);
        if (sgv->filename && strcmp(sgv->filename, il.FileName))
        {
            WINE_FIXME("File name mismatch (%s / %s)\n", sgv->filename, il.FileName);
            return TRUE;
        }

        if (sgv->lineno == -1)
        {
            if (!sgv->bp_disp || 
                !symbol_get_debug_start(sym->ModBase, sym->TypeIndex, &addr))
                addr = sym->Address;
        }
        else
        {
            addr = 0;
            do
            {
                if (sgv->lineno == il.LineNumber)
                {
                    addr = il.Address;
                    break;
                }
            } while (SymGetLineNext(dbg_curr_process->handle, &il));
            if (!addr)
            {
                WINE_FIXME("No line (%d) found for %s (setting to symbol)\n", 
                           sgv->lineno, sgv->name);
                addr = sym->Address;
            }
        }
    }

    if (sgv->num >= NUMDBGV)
    {
        dbg_printf("Too many addresses for symbol '%s', limiting the first %d\n",
                   sgv->name, NUMDBGV);
        return FALSE;
    }
    WINE_TRACE("==> %s %s%s%s%s%s%s\n", 
               sym->Name, 
               (sym->Flags & SYMFLAG_FUNCTION) ? "func " : "",
               (sym->Flags & SYMFLAG_FRAMEREL) ? "framerel " : "",
               (sym->Flags & SYMFLAG_REGISTER) ? "register " : "",
               (sym->Flags & SYMFLAG_REGREL) ? "regrel " : "",
               (sym->Flags & SYMFLAG_PARAMETER) ? "param " : "",
               (sym->Flags & SYMFLAG_THUNK) ? "thunk " : "");

    /* always keep the thunks at end of the array */
    insp = sgv->num;
    if (sgv->num_thunks && !(sym->Flags & SYMFLAG_THUNK))
    {
        insp -= sgv->num_thunks;
        memmove(&sgv->syms[insp + 1], &sgv->syms[insp],
                sizeof(sgv->syms[0]) * sgv->num_thunks);
    }
    sgv->syms[insp].lvalue.cookie      = cookie;
    sgv->syms[insp].lvalue.addr.Mode   = AddrModeFlat;
    sgv->syms[insp].lvalue.addr.Offset = addr;
    sgv->syms[insp].lvalue.type.module = sym->ModBase;
    sgv->syms[insp].lvalue.type.id = sym->TypeIndex;
    types_get_info(&sgv->syms[insp].lvalue.type, TI_GET_TYPE, 
                   &sgv->syms[insp].lvalue.type.id);
    sgv->syms[insp].flags              = sym->Flags;
    sgv->num++;
  
    return TRUE;
}

/***********************************************************************
 *           symbol_get_lvalue
 *
 * Get the address of a named symbol.
 * Return values:
 *      sglv_found:   if the symbol is found
 *      sglv_unknown: if the symbol isn't found
 *      sglv_aborted: some error occurred (likely, many symbols of same name exist,
 *          and user didn't pick one of them)
 */
enum sym_get_lval symbol_get_lvalue(const char* name, const int lineno,
                                    struct dbg_lvalue* rtn, BOOL bp_disp)
{
    struct sgv_data             sgv;
    int		                i = 0;
    char                        tmp[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)tmp;
    char                        buffer[512];
    DWORD                       opt;

    if (strlen(name) + 4 > sizeof(buffer))
    {
        WINE_WARN("Too long symbol (%s)\n", name);
        return sglv_unknown;
    }

    sgv.num        = 0;
    sgv.num_thunks = 0;
    sgv.name       = &buffer[2];
    sgv.filename   = NULL;
    sgv.lineno     = lineno;
    sgv.bp_disp    = bp_disp ? TRUE : FALSE;
    sgv.do_thunks  = DBG_IVAR(AlwaysShowThunks);

    buffer[0] = '*';
    buffer[1] = '!';
    strcpy(&buffer[2], name);

    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    SymSetOptions((opt = SymGetOptions()) | 0x40000000);
    if (!SymEnumSymbols(dbg_curr_process->handle, 0, buffer, sgv_cb, (void*)&sgv))
    {
        SymSetOptions(opt);
        return sglv_unknown;
    }

    if (!sgv.num && (name[0] != '_'))
    {
        buffer[2] = '_';
        strcpy(&buffer[3], name);
        if (!SymEnumSymbols(dbg_curr_process->handle, 0, buffer, sgv_cb, (void*)&sgv))
        {
            SymSetOptions(opt);
            return sglv_unknown;
        }
    }
    SymSetOptions(opt);

    /* now grab local symbols */
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = 256;
    if (stack_get_frame(si, &sgv.ihsf) && sgv.num < NUMDBGV)
    {
        if (SymSetContext(dbg_curr_process->handle, &sgv.ihsf, NULL))
            SymEnumSymbols(dbg_curr_process->handle, 0, name, sgv_cb, (void*)&sgv);
    }

    if (!sgv.num)
    {
        dbg_printf("No symbols found for %s\n", name);
        return sglv_unknown;
    }

    if (dbg_interactiveP)
    {
        if (sgv.num - sgv.num_thunks > 1 || /* many symbols non thunks (and showing only non thunks) */
            (sgv.num > 1 && DBG_IVAR(AlwaysShowThunks)) || /* many symbols (showing symbols & thunks) */
            (sgv.num == sgv.num_thunks && sgv.num_thunks > 1))
        {
            dbg_printf("Many symbols with name '%s', "
                       "choose the one you want (<cr> to abort):\n", name);
            for (i = 0; i < sgv.num; i++) 
            {
                if (sgv.num - sgv.num_thunks > 1 && (sgv.syms[i].flags & SYMFLAG_THUNK) && !DBG_IVAR(AlwaysShowThunks))
                    continue;
                dbg_printf("[%d]: ", i + 1);
                if (sgv.syms[i].flags & SYMFLAG_LOCAL)
                {
                    dbg_printf("local variable %sof %s\n", 
                               sgv.syms[i].flags & SYMFLAG_REGISTER ? "(in a register) " : "",
                               si->Name);
                }
                else if (sgv.syms[i].flags & SYMFLAG_PARAMETER)
                {
                    dbg_printf("parameter %sof %s\n", 
                               sgv.syms[i].flags & SYMFLAG_REGISTER ? "(in a register) " : "",
                               si->Name);
                }
                else if (sgv.syms[i].flags & SYMFLAG_THUNK) 
                {
                    print_address(&sgv.syms[i].lvalue.addr, TRUE);
                    /* FIXME: should display where the thunks points to */
                    dbg_printf(" thunk %s\n", name);
                }
                else
                {
                    print_address(&sgv.syms[i].lvalue.addr, TRUE);
                    dbg_printf("\n");
                }
            }
            do
            {
                i = 0;
                if (input_read_line("=> ", buffer, sizeof(buffer)))
                {
                    if (buffer[0] == '\0') return sglv_aborted;
                    i = atoi(buffer);
                    if (i < 1 || i > sgv.num)
                        dbg_printf("Invalid choice %d\n", i);
                }
            } while (i < 1 || i > sgv.num);

            /* The array is 0-based, but the choices are 1..n, 
             * so we have to subtract one before returning.
             */
            i--;
        }
    }
    else
    {
        /* FIXME: could display the list of non-picked up symbols */
        if (sgv.num > 1)
            dbg_printf("More than one symbol named %s, picking the first one\n", name);
    }
    *rtn = sgv.syms[i].lvalue;
    return sglv_found;
}

BOOL symbol_is_local(const char* name)
{
    struct sgv_data             sgv;
    char                        tmp[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)tmp;

    sgv.num        = 0;
    sgv.num_thunks = 0;
    sgv.name       = name;
    sgv.filename   = NULL;
    sgv.lineno     = 0;
    sgv.bp_disp    = FALSE;
    sgv.do_thunks  = FALSE;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = 256;
    if (stack_get_frame(si, &sgv.ihsf) &&
        SymSetContext(dbg_curr_process->handle, &sgv.ihsf, NULL))
        SymEnumSymbols(dbg_curr_process->handle, 0, name, sgv_cb, (void*)&sgv);
    return sgv.num > 0;
}

/***********************************************************************
 *           symbol_read_symtable
 *
 * Read a symbol file into the hash table.
 */
void symbol_read_symtable(const char* filename, unsigned long offset)
{
    dbg_printf("No longer supported\n");

#if 0
/* FIXME: have to implement SymAddSymbol in dbghelp, but likely we'll need to link
 * this with an already loaded module !! 
 */
    FILE*       symbolfile;
    unsigned    addr;
    char        type;
    char*       cpnt;
    char        buffer[256];
    char        name[256];

    if (!(symbolfile = fopen(filename, "r")))
    {
        WINE_WARN("Unable to open symbol table %s\n", filename);
        return;
    }

    dbg_printf("Reading symbols from file %s\n", filename);

    while (1)
    {
        fgets(buffer, sizeof(buffer), symbolfile);
        if (feof(symbolfile)) break;

        /* Strip any text after a # sign (i.e. comments) */
        cpnt = strchr(buffer, '#');
        if (cpnt) *cpnt = '\0';

        /* Quietly ignore any lines that have just whitespace */
        for (cpnt = buffer; *cpnt; cpnt++)
        {
            if (*cpnt != ' ' && *cpnt != '\t') break;
        }
        if (!*cpnt || *cpnt == '\n') continue;

        if (sscanf(buffer, "%lx %c %s", &addr, &type, name) == 3)
        {
            if (value.addr.off + offset < value.addr.off)
                WINE_WARN("Address wrap around\n");
            value.addr.off += offset;
            SymAddSymbol(current_process->handle, BaseOfDll,
                         name, addr, 0, 0);
        }
    }
    fclose(symbolfile);
#endif
}

/***********************************************************************
 *           symbol_get_function_line_status
 *
 * Find the symbol nearest to a given address.
 */
enum dbg_line_status symbol_get_function_line_status(const ADDRESS* addr)
{
    IMAGEHLP_LINE       il;
    DWORD               disp, size;
    ULONG64             disp64, start;
    DWORD               lin = (DWORD)memory_to_linear_addr(addr);
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        sym = (SYMBOL_INFO*)buffer;
    struct dbg_type     type;

    il.SizeOfStruct = sizeof(il);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO);

    /* do we have some info for lin address ? */
    if (!SymFromAddr(dbg_curr_process->handle, lin, &disp64, sym))
        return dbg_no_line_info;

    switch (sym->Tag)
    {
    case SymTagThunk:
        /* FIXME: so far dbghelp doesn't return the 16 <=> 32 thunks
         * and furthermore, we no longer take care of them !!!
         */
        return dbg_in_a_thunk;
    case SymTagFunction:
    case SymTagPublicSymbol: break;
    default:
        WINE_FIXME("Unexpected sym-tag 0x%08lx\n", sym->Tag);
    case SymTagData:
        return dbg_no_line_info;
    }
    /* we should have a function now */
    if (!SymGetLineFromAddr(dbg_curr_process->handle, lin, &disp, &il))
        return dbg_no_line_info;

    if (symbol_get_debug_start(sym->ModBase, sym->TypeIndex, &start) && lin < start)
        return dbg_not_on_a_line_number;
    type.module = sym->ModBase;
    type.id = sym->TypeIndex;
    if (!types_get_info(&type, TI_GET_LENGTH, &size) || size == 0)
        size = 0x100000;
    if (il.FileName && il.FileName[0] && disp < size)
        return (disp == 0) ? dbg_on_a_line_number : dbg_not_on_a_line_number;

    return dbg_no_line_info;
}

/***********************************************************************
 *           symbol_get_line
 *
 * Find the symbol nearest to a given address.
 * Returns sourcefile name and line number in a format that the listing
 * handler can deal with.
 */
BOOL symbol_get_line(const char* filename, const char* name, IMAGEHLP_LINE* line)
{
    struct sgv_data     sgv;
    char                buffer[512];
    DWORD               opt, disp;

    sgv.num        = 0;
    sgv.num_thunks = 0;
    sgv.name       = &buffer[2];
    sgv.filename   = filename;
    sgv.lineno     = -1;
    sgv.bp_disp    = FALSE;
    sgv.do_thunks  = FALSE;

    buffer[0] = '*';
    buffer[1] = '!';
    strcpy(&buffer[2], name);

    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    SymSetOptions((opt = SymGetOptions()) | 0x40000000);
    if (!SymEnumSymbols(dbg_curr_process->handle, 0, buffer, sgv_cb, (void*)&sgv))
    {
        SymSetOptions(opt);
        return sglv_unknown;
    }

    if (!sgv.num && (name[0] != '_'))
    {
        buffer[2] = '_';
        strcpy(&buffer[3], name);
        if (!SymEnumSymbols(dbg_curr_process->handle, 0, buffer, sgv_cb, (void*)&sgv))
        {
            SymSetOptions(opt);
            return sglv_unknown;
        }
    }
    SymSetOptions(opt);

    switch (sgv.num)
    {
    case 0:
        if (filename)   dbg_printf("No such function %s in %s\n", name, filename);
	else            dbg_printf("No such function %s\n", name);
        return FALSE;
    default:
        WINE_FIXME("Several found, returning first (may not be what you want)...\n");
    case 1:
        return SymGetLineFromAddr(dbg_curr_process->handle, 
                                  (DWORD)memory_to_linear_addr(&sgv.syms[0].lvalue.addr), 
                                  &disp, line);
    }
    return TRUE;
}

static BOOL CALLBACK info_locals_cb(SYMBOL_INFO* sym, ULONG size, void* ctx)
{
    ULONG               v, val;
    const char*         explain = NULL;
    char                buf[128];
    struct dbg_type     type;

    dbg_printf("\t");
    type.module = sym->ModBase;
    type.id = sym->TypeIndex;
    types_get_info(&type, TI_GET_TYPE, &type.id);
    types_print_type(&type, FALSE);

    if (sym->Flags & SYMFLAG_LOCAL) explain = "local";
    else if (sym->Flags & SYMFLAG_PARAMETER) explain = "parameter";
    else if (sym->Flags & SYMFLAG_REGISTER) explain = buf;

    if (sym->Flags & SYMFLAG_REGISTER)
    {
        const struct dbg_internal_var*  div;

        if (dbg_curr_frame != 0)
        {
            dbg_printf(" %s (register): << cannot display, not in correct frame\n",
                       sym->Name);
            return TRUE;
        }
        for (div = dbg_context_vars; div->name; div++)
        {
            if (div->val == sym->Register)
            {
                val = *div->pval;
                sprintf(buf, "local in register %s", div->name);
                break;
            }
        }
    }
    else if (sym->Flags & SYMFLAG_FRAMEREL)
    {
        type.id = sym->TypeIndex;
        types_get_info(&type, TI_GET_OFFSET, &v);
        v += ((IMAGEHLP_STACK_FRAME*)ctx)->FrameOffset;

        if (!dbg_read_memory((void*)v, &val, sizeof(val)))
        {
            dbg_printf(" %s (%s) *** cannot read value at 0x%08lx\n", sym->Name, explain, v);
            return TRUE;
        }
    }
    dbg_printf(" %s = 0x%8.8lx (%s)\n", sym->Name, val, explain);

    return TRUE;
}

int symbol_info_locals(void)
{
    IMAGEHLP_STACK_FRAME        ihsf;
    char                        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)buffer;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = 256;
    if (stack_get_frame(si, &ihsf))
    {
        dbg_printf("%s:\n", si->Name);
        if (SymSetContext(dbg_curr_process->handle, &ihsf, NULL))
            SymEnumSymbols(dbg_curr_process->handle, 0, NULL, info_locals_cb, &ihsf);
    }
    return TRUE;
}

static BOOL CALLBACK symbols_info_cb(SYMBOL_INFO* sym, ULONG size, void* ctx)
{
    struct dbg_type     type;
    IMAGEHLP_MODULE     mi;

    mi.SizeOfStruct = sizeof(mi);

    if (!SymGetModuleInfo(dbg_curr_process->handle, sym->ModBase, &mi))
        mi.ModuleName[0] = '\0';
    else
    {
        size_t  len = strlen(mi.ModuleName);
        if (len > 5 && !strcmp(mi.ModuleName + len - 5, "<elf>"))
            mi.ModuleName[len - 5] = '\0';
    }

    dbg_printf("%08lx: %s!%s", (ULONG_PTR)sym->Address, mi.ModuleName, sym->Name);
    type.id = sym->TypeIndex;
    type.module = sym->ModBase;

    if (sym->TypeIndex != dbg_itype_none && sym->TypeIndex != 0 &&
        types_get_info(&type, TI_GET_TYPE, &type.id))
    {
        dbg_printf(" ");
        types_print_type(&type, FALSE);
    }
    dbg_printf("\n");
    return TRUE;
}

void symbol_info(const char* str)
{
    char        buffer[512];
    DWORD       opt;

    if (strlen(str) + 3 >= sizeof(buffer))
    {
        dbg_printf("Symbol too long (%s)\n", str);
        return;
    }
    buffer[0] = '*';
    buffer[1] = '!';
    strcpy(&buffer[2], str);
    /* this is a wine specific options to return also ELF modules in the
     * enumeration
     */
    SymSetOptions((opt = SymGetOptions()) | 0x40000000);
    SymEnumSymbols(dbg_curr_process->handle, 0, buffer, symbols_info_cb, NULL);
    SymSetOptions(opt);
}
