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
#include <string.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "typetree.h"
#include "header.h"

type_t *duptype(type_t *t, int dupname)
{
  type_t *d = alloc_type();

  *d = *t;
  if (dupname && t->name)
    d->name = xstrdup(t->name);

  return d;
}

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

type_t *type_new_alias(type_t *t, const char *name)
{
    type_t *a = duptype(t, 0);

    a->name = xstrdup(name);
    a->attrs = NULL;
    a->orig = t;
    a->is_alias = TRUE;
    init_loc_info(&a->loc_info);

    return a;
}

type_t *type_new_module(char *name)
{
    type_t *type = make_type(RPC_FC_MODULE, NULL);
    type->name = name;
    /* FIXME: register type to detect multiple definitions */
    return type;
}

type_t *type_new_array(const char *name, type_t *element, int declptr,
                       unsigned int dim, expr_t *size_is, expr_t *length_is)
{
    type_t *t = make_type(RPC_FC_LGFARRAY, element);
    if (name) t->name = xstrdup(name);
    t->details.array.declptr = declptr;
    t->details.array.length_is = length_is;
    if (size_is)
        t->details.array.size_is = size_is;
    else
        t->details.array.dim = dim;
    return t;
}

type_t *type_new_basic(enum type_basic_type basic_type)
{
    type_t *t = make_type(TYPE_BASIC, NULL);
    t->details.basic.type = basic_type;
    t->details.basic.sign = 0;
    return t;
}

type_t *type_new_int(enum type_basic_type basic_type, int sign)
{
    static type_t *int_types[TYPE_BASIC_INT_MAX+1][3];

    assert(basic_type <= TYPE_BASIC_INT_MAX);

    /* map sign { -1, 0, 1 } -> { 0, 1, 2 } */
    if (!int_types[basic_type][sign + 1])
    {
        int_types[basic_type][sign + 1] = type_new_basic(basic_type);
        int_types[basic_type][sign + 1]->details.basic.sign = sign;
    }
    return int_types[basic_type][sign + 1];
}

type_t *type_new_void(void)
{
    static type_t *void_type = NULL;
    if (!void_type)
        void_type = make_type(0, NULL);
    return void_type;
}

static int compute_method_indexes(type_t *iface)
{
    int idx;
    statement_t *stmt;

    if (!iface->details.iface)
        return 0;

    if (type_iface_get_inherit(iface))
        idx = compute_method_indexes(type_iface_get_inherit(iface));
    else
        idx = 0;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
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

type_t *type_coclass_define(type_t *coclass, ifref_list_t *ifaces)
{
    coclass->details.coclass.ifaces = ifaces;
    coclass->defined = TRUE;
    return coclass;
}

int type_is_equal(const type_t *type1, const type_t *type2)
{
    if (type_get_type_detect_alias(type1) != type_get_type_detect_alias(type2))
        return FALSE;

    if (type1->name && type2->name)
        return !strcmp(type1->name, type2->name);
    else if ((!type1->name && type2->name) || (type1->name && !type2->name))
        return FALSE;

    /* FIXME: do deep inspection of types to determine if they are equal */

    return FALSE;
}
