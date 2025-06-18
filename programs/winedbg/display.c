/*
 * File display.c - display handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 * Copyright (C) 2003, Michal Miroslaw
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
#include <string.h>

#include "debugger.h"

/* needs to be power of 2, search for MARK to see why :) */
#define DISPTAB_DELTA 8

struct display
{
    struct expr*        exp;
    int                 count;
    char                format;
    char                enabled;
    char                func_buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        func;
};

static struct display *displaypoints = NULL;
static unsigned int maxdisplays = 0, ndisplays = 0;

static inline BOOL cmp_symbol(const SYMBOL_INFO* si1, const SYMBOL_INFO* si2)
{
    /* FIXME: !memcmp(si1, si2, sizeof(SYMBOL_INFO) + si1->NameLen)
     * is wrong because sizeof(SYMBOL_INFO) can be aligned on 4-byte boundary
     * Note: we also need to zero out the structures before calling 
     * stack_get_frame, so that un-touched fields by stack_get_frame
     * get the same value!!
     */
    return !memcmp(si1, si2, FIELD_OFFSET(SYMBOL_INFO, Name)) &&
        !memcmp(si1->Name, si2->Name, si1->NameLen);
}

BOOL display_add(struct expr *exp, int count, char format)
{
    unsigned i;
    BOOL local_binding = FALSE;

    for (i = 0; i < ndisplays; i++)
        if (displaypoints[i].exp == NULL)
            break;

    if (i == maxdisplays)
    {
        struct display *new;
	/* no space left - expand */
        new = realloc(displaypoints,
                      (maxdisplays + DISPTAB_DELTA) * sizeof(*displaypoints));
        if (!new) return FALSE;
        displaypoints = new;
        maxdisplays += DISPTAB_DELTA;
    }

    if (i == ndisplays) ndisplays++;

    displaypoints[i].exp           = expr_clone(exp, &local_binding);
    displaypoints[i].count         = count;
    displaypoints[i].format        = format;
    displaypoints[i].enabled       = TRUE;
    if (local_binding)
    {
        displaypoints[i].func = (SYMBOL_INFO*)displaypoints[i].func_buffer;
        memset(displaypoints[i].func, 0, sizeof(SYMBOL_INFO));
        displaypoints[i].func->SizeOfStruct = sizeof(SYMBOL_INFO);
        displaypoints[i].func->MaxNameLen = sizeof(displaypoints[i].func_buffer) -
            sizeof(*displaypoints[i].func);
        if (!stack_get_current_symbol(displaypoints[i].func))
        {
            expr_free(displaypoints[i].exp);
            displaypoints[i].exp = NULL;
            return FALSE;
        }
    }
    else displaypoints[i].func = NULL;

    return TRUE;
}

BOOL display_info(void)
{
    unsigned            i;
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        func;
    const char*         info;

    func = (SYMBOL_INFO*)buffer;
    memset(func, 0, sizeof(SYMBOL_INFO));
    func->SizeOfStruct = sizeof(SYMBOL_INFO);
    func->MaxNameLen = sizeof(buffer) - sizeof(*func);
    if (!stack_get_current_symbol(func)) return FALSE;

    for (i = 0; i < ndisplays; i++)
    {
        if (displaypoints[i].exp == NULL) continue;

        dbg_printf("%d: ", i + 1);
        expr_print(displaypoints[i].exp);

        if (displaypoints[i].enabled)
        {
            if (displaypoints[i].func && !cmp_symbol(displaypoints[i].func, func))
                info = " (out of scope)";
            else
                info = "";
        }
        else
            info = " (disabled)";
        if (displaypoints[i].func)
            dbg_printf(" in %s", displaypoints[i].func->Name);
        dbg_printf("%s\n", info);
    }
    return TRUE;
}

static void print_one_display(int i)
{
    struct dbg_lvalue   lvalue;

    if (displaypoints[i].enabled) 
    {
        lvalue = expr_eval(displaypoints[i].exp);
        if (lvalue.type.id == dbg_itype_none)
        {
            dbg_printf("Unable to evaluate expression ");
            expr_print(displaypoints[i].exp);
            dbg_printf("\nDisabling display %d ...\n", i + 1);
            displaypoints[i].enabled = FALSE;
            return;
        }
    }

    dbg_printf("%d: ", i + 1);
    expr_print(displaypoints[i].exp);
    dbg_printf(" = ");
    if (!displaypoints[i].enabled)
        dbg_printf("(disabled)\n");
    else
	if (displaypoints[i].format == 'i')
            memory_examine(&lvalue, displaypoints[i].count, displaypoints[i].format);
	else
            print_value(&lvalue, displaypoints[i].format, 0);
}

BOOL display_print(void)
{
    unsigned            i;
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        func;

    func = (SYMBOL_INFO*)buffer;
    memset(func, 0, sizeof(SYMBOL_INFO));
    func->SizeOfStruct = sizeof(SYMBOL_INFO);
    func->MaxNameLen = sizeof(buffer) - sizeof(*func);
    if (!stack_get_current_symbol(func)) return FALSE;

    for (i = 0; i < ndisplays; i++)
    {
        if (displaypoints[i].exp == NULL || !displaypoints[i].enabled)
            continue;
        if (displaypoints[i].func && !cmp_symbol(displaypoints[i].func, func))
            continue;
        print_one_display(i);
    }

    return TRUE;
}

BOOL display_delete(int displaynum)
{
    if (displaynum > ndisplays || displaynum == 0 || displaynum < -1 ||
        displaypoints[displaynum - 1].exp == NULL)
    {
        dbg_printf("Invalid display number\n");
        return TRUE;
    }

    if (displaynum == -1)
    {
        unsigned i;

        for (i = 0; i < ndisplays; i++)
        {
            if (displaypoints[i].exp != NULL) 
            {
                expr_free(displaypoints[i].exp);
                displaypoints[i].exp = NULL;
            }
        }
        maxdisplays = DISPTAB_DELTA;
        displaypoints = realloc(displaypoints,
                                (maxdisplays = DISPTAB_DELTA) * sizeof(*displaypoints));
        ndisplays = 0;
    }
    else if (displaypoints[--displaynum].exp != NULL) 
    {
        expr_free(displaypoints[displaynum].exp);
        displaypoints[displaynum].exp = NULL;
        while (displaynum == ndisplays - 1 && displaypoints[displaynum].exp == NULL)
        {
            --ndisplays;
            --displaynum;
        }
        if (maxdisplays - ndisplays >= 2 * DISPTAB_DELTA)
        {
            /* MARK */
            maxdisplays = (ndisplays + DISPTAB_DELTA - 1) & ~(DISPTAB_DELTA - 1);
            displaypoints = realloc(displaypoints,
                                    maxdisplays * sizeof(*displaypoints));
        }
    }
    return TRUE;
}

BOOL display_enable(int displaynum, int enable)
{
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        func;

    func = (SYMBOL_INFO*)buffer;
    memset(func, 0, sizeof(SYMBOL_INFO));
    func->SizeOfStruct = sizeof(SYMBOL_INFO);
    func->MaxNameLen = sizeof(buffer) - sizeof(*func);
    if (!stack_get_current_symbol(func)) return FALSE;

    --displaynum;
    if (displaynum >= ndisplays || displaynum < 0 || 
        displaypoints[displaynum].exp == NULL) 
    {
        dbg_printf("Invalid display number\n");
        return TRUE;
    }

    displaypoints[displaynum].enabled = enable;
    if (!displaypoints[displaynum].func || 
        cmp_symbol(displaypoints[displaynum].func, func))
    {
        print_one_display(displaynum);
    }

    return TRUE;
}
