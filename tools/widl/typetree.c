/*
 * IDL Type Tree
 *
 * Copyright 2008 Robert Shearman
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

#include <stdio.h>
#include <stdlib.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "typetree.h"
#include "header.h"

type_t *type_new_function(var_list_t *args)
{
    type_t *t = make_type(RPC_FC_FUNCTION, NULL);
    t->details.function = xmalloc(sizeof(*t->details.function));
    t->details.function->args = args;
    t->details.function->idx = -1;
    return t;
}

type_t *type_new_pointer(type_t *ref, attr_list_t *attrs)
{
    type_t *t = make_type(pointer_default, ref);
    t->attrs = attrs;
    return t;
}

static int compute_method_indexes(type_t *iface)
{
    int idx;
    statement_t *stmt;

    if (!iface->details.iface)
        return 0;

    if (iface->ref)
        idx = compute_method_indexes(iface->ref);
    else
        idx = 0;

    STATEMENTS_FOR_EACH_FUNC( stmt, iface->details.iface->stmts )
    {
        var_t *func = stmt->u.var;
        if (!is_callas(func->attrs))
            func->type->details.function->idx = idx++;
    }

    return idx;
}

void type_interface_define(type_t *iface, type_t *inherit, statement_list_t *stmts)
{
    iface->ref = inherit;
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = NULL;
    iface->details.iface->disp_methods = NULL;
    iface->details.iface->stmts = stmts;
    iface->defined = TRUE;
    compute_method_indexes(iface);
}

void type_dispinterface_define(type_t *iface, var_list_t *props, func_list_t *methods)
{
    iface->ref = find_type("IDispatch", 0);
    if (!iface->ref) error_loc("IDispatch is undefined\n");
    iface->details.iface = xmalloc(sizeof(*iface->details.iface));
    iface->details.iface->disp_props = props;
    iface->details.iface->disp_methods = methods;
    iface->details.iface->stmts = NULL;
    iface->defined = TRUE;
    compute_method_indexes(iface);
}

void type_dispinterface_define_from_iface(type_t *dispiface, type_t *iface)
{
    type_dispinterface_define(dispiface, iface->details.iface->disp_props,
                              iface->details.iface->disp_methods);
}

void type_module_define(type_t *module, statement_list_t *stmts)
{
    if (module->details.module) error_loc("multiple definition error\n");
    module->details.module = xmalloc(sizeof(*module->details.module));
    module->details.module->stmts = stmts;
    module->defined = TRUE;
}
