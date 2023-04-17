/*
 * Debugger break-points handling
 *
 * Copyright 1994 Martin von Loewis
 * Copyright 1995 Alexandre Julliard
 * Copyright 1999,2000 Eric Pouech
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

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

static BOOL is_xpoint_break(int bpnum)
{
    int type = dbg_curr_process->bp[bpnum].xpoint_type;

    if (type == be_xpoint_break || type == be_xpoint_watch_exec) return TRUE;
    if (type == be_xpoint_watch_read || type == be_xpoint_watch_write) return FALSE;
    RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
    return FALSE; /* never reached */
}

/***********************************************************************
 *           break_set_xpoints
 *
 * Set or remove all the breakpoints & watchpoints
 */
void  break_set_xpoints(BOOL set)
{
    static BOOL                 last; /* = 0 = FALSE */

    unsigned int                i, ret, size;
    void*                       addr;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    if (set == last) return;
    last = set;

    for (i = 0; i < dbg_curr_process->next_bp; i++)
    {
        if (!bp[i].refcount || !bp[i].enabled) continue;

        if (is_xpoint_break(i))
            size = 0;
        else
            size = bp[i].w.len + 1;
        addr = memory_to_linear_addr(&bp[i].addr);

        if (set)
            ret = dbg_curr_process->be_cpu->insert_Xpoint(dbg_curr_process->handle,
                dbg_curr_process->process_io, &dbg_context, bp[i].xpoint_type,
                addr, &bp[i].info, size);
        else
            ret = dbg_curr_process->be_cpu->remove_Xpoint(dbg_curr_process->handle,
                dbg_curr_process->process_io, &dbg_context, bp[i].xpoint_type,
                addr, bp[i].info, size);
        if (!ret)
        {
            dbg_printf("Invalid address (");
            print_address(&bp[i].addr, FALSE);
            dbg_printf(") for breakpoint %d, disabling it\n", i);
            bp[i].enabled = FALSE;
        }
    }
}

/***********************************************************************
 *           find_xpoint
 *
 * Find the breakpoint for a given address. Return the breakpoint
 * number or -1 if none.
 */
static int find_xpoint(const ADDRESS64* addr, enum be_xpoint_type type)
{
    int                         i;
    void*                       lin = memory_to_linear_addr(addr);
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    for (i = 0; i < dbg_curr_process->next_bp; i++)
    {
        if (bp[i].refcount && bp[i].enabled && bp[i].xpoint_type == type &&
            memory_to_linear_addr(&bp[i].addr) == lin)
            return i;
    }
    return -1;
}

/***********************************************************************
 *           init_xpoint
 *
 * Find an empty slot in BP table to add a new break/watch point
 */
static	int init_xpoint(int type, const ADDRESS64* addr)
{
    int	                        num;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    for (num = (dbg_curr_process->next_bp < MAX_BREAKPOINTS) ? 
             dbg_curr_process->next_bp++ : 1;
         num < MAX_BREAKPOINTS; num++)
    {
        if (bp[num].refcount == 0)
        {
            bp[num].refcount    = 1;
            bp[num].enabled     = TRUE;
            bp[num].xpoint_type = type;
            bp[num].skipcount   = 0;
            bp[num].addr        = *addr;
            return num;
        }
    }

    dbg_printf("Too many bp. Please delete some.\n");
    return -1;
}

/***********************************************************************
 *           get_watched_value
 *
 * Returns the value watched by watch point 'num'.
 */
static	BOOL	get_watched_value(int num, DWORD64* val)
{
    DWORD64     buf[1];

    if (!dbg_read_memory(memory_to_linear_addr(&dbg_curr_process->bp[num].addr),
                         buf, dbg_curr_process->bp[num].w.len + 1))
        return FALSE;

    switch (dbg_curr_process->bp[num].w.len + 1)
    {
    case 8:	*val = *(DWORD64*)buf;	break;
    case 4:	*val = *(DWORD*)buf;	break;
    case 2:	*val = *(WORD*)buf;	break;
    case 1:	*val = *(BYTE*)buf;	break;
    default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
    }
    return TRUE;
}

/***********************************************************************
 *           break_add_break
 *
 * Add a breakpoint.
 */
BOOL break_add_break(const ADDRESS64* addr, BOOL verbose, BOOL swbp)
{
    int                         num;
    BYTE                        ch;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;
    int                         type = swbp ? be_xpoint_break : be_xpoint_watch_exec;

    if ((num = find_xpoint(addr, type)) >= 1)
    {
        bp[num].refcount++;
        dbg_printf("Breakpoint %d at ", num);
        print_address(&bp[num].addr, TRUE);
        dbg_printf(" (refcount=%d)\n", bp[num].refcount);
        return TRUE;
    }

    if (!dbg_read_memory(memory_to_linear_addr(addr), &ch, sizeof(ch)))
    {
        if (verbose)
        {
            dbg_printf("Invalid address ");
            print_bare_address(addr);
            dbg_printf(", can't set breakpoint\n");
        }
        return FALSE;
    }

    if ((num = init_xpoint(type, addr)) == -1)
        return FALSE;

    dbg_printf("Breakpoint %d at ", num);
    print_address(&bp[num].addr, TRUE);
    dbg_printf("\n");

    return TRUE;
}

/***********************************************************************
 *           break_add_break_from_lvalue
 *
 * Add a breakpoint.
 */
BOOL break_add_break_from_lvalue(const struct dbg_lvalue* lvalue, BOOL swbp)
{
    struct dbg_delayed_bp* new;
    ADDRESS64   addr;

    types_extract_as_address(lvalue, &addr);

    if (!break_add_break(&addr, TRUE, swbp))
    {
        if (!DBG_IVAR(CanDeferOnBPByAddr))
        {
            dbg_printf("Invalid address, can't set breakpoint\n"
                       "You can turn on deferring bp by address by setting $CanDeferOnBPByAddr to 1\n");
            return FALSE;
        }
        dbg_printf("Unable to add breakpoint, will check again any time a new DLL is loaded\n");
        new = realloc(dbg_curr_process->delayed_bp,
                      sizeof(struct dbg_delayed_bp) * (dbg_curr_process->num_delayed_bp + 1));
        if (!new) return FALSE;
        dbg_curr_process->delayed_bp = new;

        dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].is_symbol   = FALSE;
        dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].software_bp = swbp;
        dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].u.addr      = addr;
        dbg_curr_process->num_delayed_bp++;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           break_add_break_from_id
 *
 * Add a breakpoint from a function name (and eventually a line #)
 */
void	break_add_break_from_id(const char *name, int lineno, BOOL swbp)
{
    struct dbg_delayed_bp* new;
    struct dbg_lvalue 	lvalue;
    int		        i;

    switch (symbol_get_lvalue(name, lineno, &lvalue, TRUE))
    {
    case sglv_found:
        break_add_break(&lvalue.addr, TRUE, swbp);
        return;
    case sglv_unknown:
        break;
    case sglv_aborted: /* user aborted symbol lookup */
        return;
    }

    dbg_printf("Unable to add breakpoint, will check again when a new DLL is loaded\n");
    for (i = 0; i < dbg_curr_process->num_delayed_bp; i++)
    {
        if (dbg_curr_process->delayed_bp[i].is_symbol &&
            !strcmp(name, dbg_curr_process->delayed_bp[i].u.symbol.name) &&
            lineno == dbg_curr_process->delayed_bp[i].u.symbol.lineno)
            return;
    }
    new = realloc(dbg_curr_process->delayed_bp,
                  sizeof(struct dbg_delayed_bp) * (dbg_curr_process->num_delayed_bp + 1));
    if (!new) return;
    dbg_curr_process->delayed_bp = new;
    dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].is_symbol       = TRUE;
    dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].software_bp     = swbp;
    dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].u.symbol.name   = strdup(name);
    dbg_curr_process->delayed_bp[dbg_curr_process->num_delayed_bp].u.symbol.lineno = lineno;
    dbg_curr_process->num_delayed_bp++;
}

struct cb_break_lineno
{
    const char* filename;
    int         lineno;
    ADDRESS64   addr;
};

static BOOL CALLBACK line_cb(SRCCODEINFO* sci, void* user)
{
    struct cb_break_lineno*      bkln = user;

    if (bkln->lineno == sci->LineNumber)
    {
        bkln->addr.Mode = AddrModeFlat;
        bkln->addr.Offset = sci->Address;
        return FALSE;
    }
    return TRUE;
}

static BOOL CALLBACK mcb(PCWSTR module, DWORD64 base, void* user)
{
    struct cb_break_lineno*      bkln = user;

    SymEnumLines(dbg_curr_process->handle, base, NULL, bkln->filename, line_cb, bkln);
    /* continue module enum if no addr found
     * FIXME: we don't report when several addresses match the same filename/lineno pair
     */
    return !bkln->addr.Offset;
}

/***********************************************************************
 *           break_add_break_from_lineno
 *
 * Add a breakpoint from a line number in current file
 */
void break_add_break_from_lineno(const char *filename, int lineno, BOOL swbp)
{
    struct cb_break_lineno      bkln;
    bkln.addr.Offset = 0;
    bkln.lineno = lineno;

    if (!filename)
    {
        DWORD disp;
        ADDRESS64 curr;
        IMAGEHLP_LINE64 il;
        DWORD_PTR linear;

        memory_get_current_pc(&curr);
        linear = (DWORD_PTR)memory_to_linear_addr(&curr);
        il.SizeOfStruct = sizeof(il);
        if (!SymGetLineFromAddr64(dbg_curr_process->handle, linear, &disp, &il))
        {
            dbg_printf("Unable to add breakpoint (unknown address %Ix)\n", linear);
            return;
        }
        filename = il.FileName;
        SymEnumLines(dbg_curr_process->handle, linear, NULL, filename, line_cb, &bkln);
    }
    else
    {
        /* we have to enumerate across modules */
        bkln.filename = filename;
        SymEnumerateModulesW64(dbg_curr_process->handle, mcb, &bkln);
    }
    if (bkln.addr.Offset)
        break_add_break(&bkln.addr, TRUE, swbp);
    else
    {
        /* winedbg's lexer can't tell in 'break foo : 3' whether foo shall be interpreted
         * as a debuggee symbol or as a debuggee source file.
         * Try now symbol since source file failed.
         */
        if (filename)
            break_add_break_from_id(filename, lineno, swbp);
        else
            dbg_printf("Unknown line number\n"
                       "(either out of file, or no code at given line number)\n");
    }
}

/***********************************************************************
 *           break_check_delayed_bp
 *
 * Check is a registered delayed BP is now available.
 */
void break_check_delayed_bp(void)
{
    struct dbg_lvalue	        lvalue;
    int			        i;
    struct dbg_delayed_bp*	dbp = dbg_curr_process->delayed_bp;
    char                        hexbuf[MAX_OFFSET_TO_STR_LEN];

    for (i = 0; i < dbg_curr_process->num_delayed_bp; i++)
    {
        if (dbp[i].is_symbol)
        {
            if (symbol_get_lvalue(dbp[i].u.symbol.name, dbp[i].u.symbol.lineno,
                                  &lvalue, TRUE) != sglv_found)
                continue;
            if (!lvalue.in_debuggee) continue;
        }
        else
            lvalue.addr = dbp[i].u.addr;
        WINE_TRACE("trying to add delayed %s-bp\n", dbp[i].is_symbol ? "S" : "A");
        if (!dbp[i].is_symbol)
            WINE_TRACE("\t%04x:%s\n", 
                       dbp[i].u.addr.Segment,
                       memory_offset_to_string(hexbuf, dbp[i].u.addr.Offset, 0));
        else
            WINE_TRACE("\t'%s' @ %d\n", 
                       dbp[i].u.symbol.name, dbp[i].u.symbol.lineno);

        if (break_add_break(&lvalue.addr, FALSE, dbp[i].software_bp))
            memmove(&dbp[i], &dbp[i+1], (--dbg_curr_process->num_delayed_bp - i) * sizeof(*dbp));
    }
}

/***********************************************************************
 *           break_add_watch
 *
 * Add a watchpoint.
 */
void break_add_watch(const struct dbg_lvalue* lvalue, BOOL is_write)
{
    int         num;
    DWORD       l = ADDRSIZE;

    if (!lvalue->in_debuggee)
    {
        dbg_printf("Cannot set a watch point on register or register-based variable\n");
        return;
    }
    num = init_xpoint((is_write) ? be_xpoint_watch_write : be_xpoint_watch_read,
                      &lvalue->addr);
    if (num == -1) return;

    /* FIXME: this validation part should be moved to CPU backends (likely in a new method) */
    if (lvalue->type.id != dbg_itype_none)
    {
        DWORD64 l64;
        if (types_get_info(&lvalue->type, TI_GET_LENGTH, &l64))
        {
            /* Only allow size of a power of 2, smaller than CPU's pointer size.
             * (Validation is done by CPU backend with insert_Xpoint method)
             */
            if (!(l64 & (l64 - 1)) && l64 <= l)
                l = l64;
            else
                dbg_printf("Unsupported length (%I64x) for watch-points, defaulting to %lu\n", l64, l);
            /* most CPUs request watch address to be aligned on length */
            if (lvalue->addr.Offset & (l - 1))
            {
                dbg_printf("Watchpoint on unaligned address is not supported\n");
                dbg_curr_process->bp[num].refcount = 0;
                return;
            }
        }
        else dbg_printf("Cannot get watch size, defaulting to %lu\n", l);
    }
    dbg_curr_process->bp[num].w.len = l - 1;

    if (!get_watched_value(num, &dbg_curr_process->bp[num].w.oldval))
    {
        dbg_printf("Bad address. Watchpoint not set\n");
        dbg_curr_process->bp[num].refcount = 0;
        return;
    }
    dbg_printf("Watchpoint %d at ", num);
    print_address(&dbg_curr_process->bp[num].addr, TRUE);
    dbg_printf("\n");
}

/***********************************************************************
 *           break_delete_xpoint
 *
 * Delete a breakpoint.
 */
void break_delete_xpoint(int num)
{
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    if ((num <= 0) || (num >= dbg_curr_process->next_bp) ||
        bp[num].refcount == 0)
    {
        dbg_printf("Invalid breakpoint number %d\n", num);
        return;
    }

    if (--bp[num].refcount > 0)
        return;

    if (bp[num].condition != NULL)
    {
        expr_free(bp[num].condition);
        bp[num].condition = NULL;
    }

    bp[num].enabled = FALSE;
    bp[num].refcount = 0;
    bp[num].skipcount = 0;
}

/******************************************************************
 *		break_delete_xpoints_from_module
 *
 * Remove all Xpoints from module which base is 'base'
 */
void break_delete_xpoints_from_module(DWORD64 base)
{
    IMAGEHLP_MODULE64           im, im_elf;
    int                         i;
    DWORD_PTR                   linear;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    /* FIXME: should do it also on the ELF sibling if any */
    im.SizeOfStruct = sizeof(im);
    im_elf.SizeOfStruct = sizeof(im_elf);
    if (!SymGetModuleInfo64(dbg_curr_process->handle, base, &im)) return;

    /* try to get in fact the underlying ELF module (if any) */
    if (SymGetModuleInfo64(dbg_curr_process->handle, im.BaseOfImage - 1, &im_elf) &&
        im_elf.BaseOfImage <= im.BaseOfImage &&
        im_elf.BaseOfImage + im_elf.ImageSize >= im.BaseOfImage + im.ImageSize)
        im = im_elf;

    for (i = 0; i < dbg_curr_process->next_bp; i++)
    {
        if (bp[i].refcount && bp[i].enabled)
        {
            linear = (DWORD_PTR)memory_to_linear_addr(&bp[i].addr);
            if (im.BaseOfImage <= linear && linear < im.BaseOfImage + im.ImageSize)
            {
                break_delete_xpoint(i);
            }
        }
    }
}

/***********************************************************************
 *           break_enable_xpoint
 *
 * Enable or disable a break point.
 */
void break_enable_xpoint(int num, BOOL enable)
{
    if ((num <= 0) || (num >= dbg_curr_process->next_bp) || 
        dbg_curr_process->bp[num].refcount == 0)
    {
        dbg_printf("Invalid breakpoint number %d\n", num);
        return;
    }
    dbg_curr_process->bp[num].enabled = enable != 0;
    dbg_curr_process->bp[num].skipcount = 0;
}


/***********************************************************************
 *           find_triggered_watch
 *
 * Lookup the watchpoints to see if one has been triggered
 * Return >= (watch point index) if one is found
 * Return -1 if none found
 *
 * Unfortunately, Linux used to *NOT* (A REAL PITA) report with ptrace
 * the DR6 register value, so we have to look with our own need the
 * cause of the TRAP.
 * -EP
 */
static int find_triggered_watch(void)
{
    int                         found = -1;
    int                         i;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    /* Method 1 => get triggered watchpoint from context (doesn't work on Linux
     * 2.2.x). This should be fixed in >= 2.2.16
     */
    for (i = 0; i < dbg_curr_process->next_bp; i++)
    {
        DWORD64 val = 0;

        if (bp[i].refcount && bp[i].enabled && !is_xpoint_break(i) &&
            (dbg_curr_process->be_cpu->is_watchpoint_set(&dbg_context, bp[i].info)))
        {
            dbg_curr_process->be_cpu->clear_watchpoint(&dbg_context, bp[i].info);

            if (get_watched_value(i, &val))
            {
                bp[i].w.oldval = val;
                return i;
            }
        }
    }

    /* Method 1 failed, trying method 2 */

    /* Method 2 => check if value has changed among registered watchpoints
     * this really sucks, but this is how gdb 4.18 works on my linux box
     * -EP
     */
    for (i = 0; i < dbg_curr_process->next_bp; i++)
    {
        DWORD64 val = 0;

        if (bp[i].refcount && bp[i].enabled && !is_xpoint_break(i) &&
            get_watched_value(i, &val))
        {
            if (val != bp[i].w.oldval)
            {
                dbg_curr_process->be_cpu->clear_watchpoint(&dbg_context, bp[i].info);
                bp[i].w.oldval = val;
                found = i;
                /* cannot break, because two watch points may have been triggered on
                 * the same access
                 * only one will be reported to the user (FIXME ?)
                 */
            }
        }
    }
    return found;
}

/***********************************************************************
 *           break_info
 *
 * Display break & watch points information.
 */
void break_info(void)
{
    int                         i;
    int                         nbp = 0, nwp = 0;
    struct dbg_delayed_bp*	dbp = dbg_curr_process->delayed_bp;
    struct dbg_breakpoint*      bp = dbg_curr_process->bp;

    for (i = 1; i < dbg_curr_process->next_bp; i++)
    {
        if (bp[i].refcount)
        {
            if (is_xpoint_break(i)) nbp++; else nwp++;
        }
    }

    if (nbp)
    {
        dbg_printf("Breakpoints:\n");
        for (i = 1; i < dbg_curr_process->next_bp; i++)
        {
            if (!bp[i].refcount || !is_xpoint_break(i))
                continue;
            dbg_printf("%d: %c ", i, bp[i].enabled ? 'y' : 'n');
            print_address(&bp[i].addr, TRUE);
            dbg_printf(" (%u)%s\n", bp[i].refcount, 
                       bp[i].xpoint_type == be_xpoint_watch_exec ? " (hardware assisted)" : "");
	    if (bp[i].condition != NULL)
	    {
	        dbg_printf("\t\tstop when  ");
 		expr_print(bp[i].condition);
		dbg_printf("\n");
	    }
        }
    }
    else dbg_printf("No breakpoints\n");
    if (nwp)
    {
        dbg_printf("Watchpoints:\n");
        for (i = 1; i < dbg_curr_process->next_bp; i++)
        {
            if (!bp[i].refcount || is_xpoint_break(i))
                continue;
            dbg_printf("%d: %c ", i, bp[i].enabled ? 'y' : 'n');
            print_address(&bp[i].addr, TRUE);
            dbg_printf(" on %d byte%s (%c)\n",
                       bp[i].w.len + 1, bp[i].w.len > 0 ? "s" : "",
                       bp[i].xpoint_type == be_xpoint_watch_write ? 'W' : 'R');
	    if (bp[i].condition != NULL)
	    {
	        dbg_printf("\t\tstop when ");
 		expr_print(bp[i].condition);
		dbg_printf("\n");
	    }
        }
    }
    else dbg_printf("No watchpoints\n");
    if (dbg_curr_process->num_delayed_bp)
    {
        dbg_printf("Delayed breakpoints:\n");
        for (i = 0; i < dbg_curr_process->num_delayed_bp; i++)
        {
            if (dbp[i].is_symbol)
            {
                dbg_printf("%d: %s", i, dbp[i].u.symbol.name);
                if (dbp[i].u.symbol.lineno != -1)
                    dbg_printf(" at line %u", dbp[i].u.symbol.lineno);
            }
            else
            {
                dbg_printf("%d: ", i);
                print_address(&dbp[i].u.addr, FALSE);
            }
            dbg_printf("\n");
        }
    }
}

/***********************************************************************
 *           should_stop
 *
 * Check whether or not the condition (bp / skipcount) of a break/watch
 * point are met.
 */
static	BOOL should_stop(int bpnum)
{
    struct dbg_breakpoint*      bp = &dbg_curr_process->bp[bpnum];

    if (bp->condition != NULL)
    {
        struct dbg_lvalue lvalue = expr_eval(bp->condition);

        if (lvalue.type.id == dbg_itype_none)
        {
	    /*
	     * Something wrong - unable to evaluate this expression.
	     */
	    dbg_printf("Unable to evaluate expression ");
	    expr_print(bp->condition);
	    dbg_printf("\nTurning off condition\n");
	    break_add_condition(bpnum, NULL);
        }
        else if (!types_extract_as_integer(&lvalue))
        {
	    return FALSE;
        }
    }

    if (bp->skipcount > 0) bp->skipcount--;
    return bp->skipcount == 0;
}

/***********************************************************************
 *           break_should_continue
 *
 * Determine if we should continue execution after a SIGTRAP signal when
 * executing in the given mode.
 */
BOOL break_should_continue(ADDRESS64* addr, DWORD code)
{
    enum dbg_exec_mode  mode = dbg_curr_thread->exec_mode;


    if (dbg_curr_thread->stopped_xpoint > 0)
    {
        if (!should_stop(dbg_curr_thread->stopped_xpoint)) return TRUE;

        switch (dbg_curr_process->bp[dbg_curr_thread->stopped_xpoint].xpoint_type)
        {
        case be_xpoint_break:
        case be_xpoint_watch_exec:
            dbg_printf("Stopped on breakpoint %d at ", dbg_curr_thread->stopped_xpoint);
            print_address(&dbg_curr_process->bp[dbg_curr_thread->stopped_xpoint].addr, TRUE);
            dbg_printf("\n");
            break;
        case be_xpoint_watch_read:
        case be_xpoint_watch_write:
            dbg_printf("Stopped on watchpoint %d at ", dbg_curr_thread->stopped_xpoint);
            print_address(addr, TRUE);
            dbg_printf(" new value %I64x\n",
                       dbg_curr_process->bp[dbg_curr_thread->stopped_xpoint].w.oldval);
        }
        return FALSE;
    }

    /*
     * If our mode indicates that we are stepping line numbers,
     * get the current function, and figure out if we are exactly
     * on a line number or not.
     */
    if (mode == dbg_exec_step_over_line || mode == dbg_exec_step_into_line)
    {
	if (symbol_get_function_line_status(addr) == dbg_on_a_line_number)
            dbg_curr_thread->exec_count--;
    }
    else if (mode == dbg_exec_step_over_insn || mode == dbg_exec_step_into_insn)
        dbg_curr_thread->exec_count--;

    if (dbg_curr_thread->exec_count > 0 || mode == dbg_exec_finish)
    {
	/*
	 * We still need to execute more instructions.
	 */
	return TRUE;
    }

    /* no breakpoint, continue if in continuous mode */
    return mode == dbg_exec_cont || mode == dbg_exec_finish;
}

/***********************************************************************
 *           break_adjust_pc
 *
 * Adjust PC to the address where the trap (if any) actually occurred
 * Also sets dbg_curr_thread->stopped_xpoint
 */
void break_adjust_pc(ADDRESS64* addr, DWORD code, BOOL first_chance, BOOL* is_break)
{
    /* break / watch points are handled on first chance */
    if ( !first_chance )
    {
        *is_break = TRUE;
        dbg_curr_thread->stopped_xpoint = -1;
        return;
    }
    *is_break = FALSE;

    /* If not single-stepping, back up to the break instruction */
    if (code == EXCEPTION_BREAKPOINT)
        addr->Offset += dbg_curr_process->be_cpu->adjust_pc_for_break(&dbg_context, TRUE);

    dbg_curr_thread->stopped_xpoint = find_xpoint(addr, be_xpoint_break);
    dbg_curr_process->bp[0].enabled = FALSE;  /* disable the step-over breakpoint */

    if (dbg_curr_thread->stopped_xpoint > 0) return;

    if (dbg_curr_thread->stopped_xpoint < 0)
    {
        dbg_curr_thread->stopped_xpoint = find_xpoint(addr, be_xpoint_watch_exec);
        if (dbg_curr_thread->stopped_xpoint < 0)
            dbg_curr_thread->stopped_xpoint = find_triggered_watch();
        if (dbg_curr_thread->stopped_xpoint > 0)
        {
            /* If not single-stepping, do not back up over the break instruction */
            if (code == EXCEPTION_BREAKPOINT)
                addr->Offset += dbg_curr_process->be_cpu->adjust_pc_for_break(&dbg_context, FALSE);
            return;
        }
    }

    /* If there's no breakpoint and we are not single-stepping, then
     * either we must have encountered a break insn in the Windows program
     * or someone is trying to stop us
     */
    if (dbg_curr_thread->stopped_xpoint == -1 && code == EXCEPTION_BREAKPOINT)
    {
        *is_break = TRUE;
        addr->Offset += dbg_curr_process->be_cpu->adjust_pc_for_break(&dbg_context, FALSE);
    }
}

/***********************************************************************
 *           break_suspend_execution
 *
 * Remove all bp before entering the debug loop
 */
void	break_suspend_execution(void)
{
    break_set_xpoints(FALSE);
    dbg_curr_process->bp[0] = dbg_curr_thread->step_over_bp;
}

/***********************************************************************
 *           break_restart_execution
 *
 * Set the bp to the correct state to restart execution
 * in the given mode.
 */
void break_restart_execution(int count)
{
    ADDRESS64                   addr;
    enum dbg_line_status        status;
    enum dbg_exec_mode          mode, ret_mode;
    ADDRESS64                   callee;
    void*                       linear;

    memory_get_current_pc(&addr);
    linear = memory_to_linear_addr(&addr);

    /*
     * This is the mode we will be running in after we finish.  We would like
     * to be able to modify this in certain cases.
     */
    ret_mode = mode = dbg_curr_thread->exec_mode;

    /* we've stopped on a xpoint (other than step over) */
    if (dbg_curr_thread->stopped_xpoint > 0)
    {
	/*
	 * If we have set a new value, then save it in the BP number.
	 */
	if (count != 0 && mode == dbg_exec_cont)
        {
	    dbg_curr_process->bp[dbg_curr_thread->stopped_xpoint].skipcount = count;
        }
        /* If we've stopped on a breakpoint, single step over it (, then run) */
        if (is_xpoint_break(dbg_curr_thread->stopped_xpoint))
            mode = dbg_exec_step_into_insn;
    }
    else if (mode == dbg_exec_cont && count > 1)
    {
        dbg_printf("Not stopped at any breakpoint; argument ignored.\n");
    }

    if (mode == dbg_exec_finish && dbg_curr_process->be_cpu->is_function_return(linear))
    {
	mode = ret_mode = dbg_exec_step_into_insn;
    }

    /*
     * See if the function we are stepping into has debug info
     * and line numbers.  If not, then we step over it instead.
     * FIXME - we need to check for things like thunks or trampolines,
     * as the actual function may in fact have debug info.
     */
    if (dbg_curr_process->be_cpu->is_function_call(linear, &callee))
    {
	status = symbol_get_function_line_status(&callee);
#if 0
        /* FIXME: we need to get the thunk type */
	/*
	 * Anytime we have a trampoline, step over it.
	 */
	if ((mode == EXEC_STEP_OVER || mode == EXEC_STEPI_OVER)
	    && status == dbg_in_a_thunk)
        {
            WINE_WARN("Not stepping into trampoline at %p (no lines)\n", 
                      memory_to_linear_addr(&callee));
	    mode = EXEC_STEP_OVER_TRAMPOLINE;
        }
#endif
	if (mode == dbg_exec_step_into_line && status == dbg_no_line_info)
        {
            WINE_WARN("Not stepping into function at %p (no lines)\n",
                      memory_to_linear_addr(&callee));
	    mode = dbg_exec_step_over_line;
        }
    }

    if (mode == dbg_exec_step_into_line && 
        symbol_get_function_line_status(&addr) == dbg_no_line_info)
    {
        dbg_printf("Single stepping until exit from function,\n"
                   "which has no line number information.\n");
        ret_mode = mode = dbg_exec_finish;
    }

    switch (mode)
    {
    case dbg_exec_cont: /* Continuous execution */
        dbg_curr_process->be_cpu->single_step(&dbg_context, FALSE);
        break_set_xpoints(TRUE);
        break;

#if 0
    case EXEC_STEP_OVER_TRAMPOLINE:
        /*
         * This is the means by which we step over our conversion stubs
         * in callfrom*.s and callto*.s.  We dig the appropriate address
         * off the stack, and we set the breakpoint there instead of the
         * address just after the call.
         */
        be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context,
                         be_cpu_addr_stack, &addr);
        /* FIXME: we assume stack grows as on an i386 */
        addr.Offset += 2 * sizeof(unsigned int);
        dbg_read_memory(memory_to_linear_addr(&addr),
                        &addr.Offset, sizeof(addr.Offset));
        dbg_curr_process->bp[0].addr = addr;
        dbg_curr_process->bp[0].enabled = TRUE;
        dbg_curr_process->bp[0].refcount = 1;
        dbg_curr_process->bp[0].skipcount = 0;
        dbg_curr_process->bp[0].xpoint_type = be_xpoint_break;
        dbg_curr_process->bp[0].condition = NULL;
        be_cpu->single_step(&dbg_context, FALSE);
        break_set_xpoints(TRUE);
        break;
#endif

    case dbg_exec_finish:
    case dbg_exec_step_over_insn:  /* Stepping over a call */
    case dbg_exec_step_over_line:  /* Stepping over a call */
        if (dbg_curr_process->be_cpu->is_step_over_insn(linear))
        {
            dbg_curr_process->be_cpu->disasm_one_insn(&addr, FALSE);
            dbg_curr_process->bp[0].addr = addr;
            dbg_curr_process->bp[0].enabled = TRUE;
            dbg_curr_process->bp[0].refcount = 1;
	    dbg_curr_process->bp[0].skipcount = 0;
            dbg_curr_process->bp[0].xpoint_type = be_xpoint_break;
            dbg_curr_process->bp[0].condition = NULL;
            dbg_curr_process->be_cpu->single_step(&dbg_context, FALSE);
            break_set_xpoints(TRUE);
            break;
        }
        /* else fall through to single-stepping */

    case dbg_exec_step_into_line: /* Single-stepping a line */
    case dbg_exec_step_into_insn: /* Single-stepping an instruction */
        dbg_curr_process->be_cpu->single_step(&dbg_context, TRUE);
        break;
    default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
    }
    dbg_curr_thread->step_over_bp = dbg_curr_process->bp[0];
    dbg_curr_thread->exec_mode = ret_mode;
}

BOOL break_add_condition(int num, struct expr* exp)
{
    if (num <= 0 || num >= dbg_curr_process->next_bp || 
        !dbg_curr_process->bp[num].refcount)
    {
        dbg_printf("Invalid breakpoint number %d\n", num);
        return FALSE;
    }

    if (dbg_curr_process->bp[num].condition != NULL)
    {
	expr_free(dbg_curr_process->bp[num].condition);
	dbg_curr_process->bp[num].condition = NULL;
    }

    if (exp != NULL) dbg_curr_process->bp[num].condition = expr_clone(exp, NULL);

    return TRUE;
}
